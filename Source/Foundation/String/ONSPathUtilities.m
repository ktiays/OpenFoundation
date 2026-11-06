//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSPathUtilities.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/Collections/OCFArray.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"
#import "CoreFoundation/Objective-C/OCFNSArray.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#import "Foundation/String/ONSAllocateObject.h"
#import "Foundation/String/ONSFileUtilities.h"
#import "Foundation/String/ONSLog.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSStringOtherEncodings.h"

#include <mach-o/loader.h>
#include <malloc/malloc.h>

extern struct mach_header __dso_handle;
extern id objc_opt_self(id object);
extern id objc_autorelease(id object);
extern id objc_retain(id object);
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <os/lock.h>
#include <stdlib.h>
#include <string.h>
#include <sys/attr.h>
#include <sys/stat.h>
#include <unistd.h>

// Inferred organization: the path functions and the NSPathUtilities category of NSString lie together in the
// iOS Simulator __text section (0x1c058cb70-0x1c0591238, with NSHomeDirectoryForUser at 0x1c0591f0c after
// functions outside this family), so they form one translation unit, in native address order. The unit is
// Objective-C compiled with manual reference counting (objc_release of the temporary strings, no autorelease
// of the path stores it receives). Unless a definition says otherwise: iOS Simulator 24A434, arm64,
// Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full,
// iphone-simulator-foundation-link.json; where the IDA pseudocode drops a register argument, the assembly
// settles it). iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Foundation/String/ONSStringReview.md.
//
// Paths are processed as UTF-16 character buffers with a separate length (passed by address where a function
// changes it). GOT slots outside the extracted image are identified by their use and by the alphabetical
// order of the shared cache's import slots (confidence: medium): 0x22227F4F8
// NSInternalInconsistencyException, 0x22227F500 NSInvalidArgumentException, 0x22227F5D8 NSMallocException,
// 0x22227FAB8 the NSData class, 0x22227FAE0 the NSException class, 0x22227FB18 the NSMutableArray class,
// 0x22227FB38 the NSMutableSet class and 0x222280B70 kCFAllocatorSystemDefault. 0x222417EE8 is the stack
// guard of the stack protector.

// Class symbols the unit references: NSPathStore2, NSCheapMutableString and NSFileManager (Foundation, not
// restored), NSMutableArray, NSMutableSet, NSData and NSException (CoreFoundation).

// The stat cache (__MergedGlobals.11, __DATA,__bss 0x22e76c8e8): the unfair lock at 0x22e76c8ec guards the
// dictionary at 0x22e76c8f8 (C string keys, C string or NULL values), created on first use. The automount
// cache: the unfair lock at 0x22e76c8f0 guards the set at 0x22e76c900 of the /private mount points known to
// exist. All zero-initialized. The four bytes at 0x22e76c8e8 are not referenced by this unit.
static os_unfair_lock statCacheLock;
static os_unfair_lock automountCacheLock;
static OCFMutableDictionaryRef statCache;
static id automountCache;

// These non-external callbacks are owned by this translation unit.
static Boolean _isEqualCString(const void *first, const void *second);
static OCFHashCode _hashCString2(const void *value);

// The stat cache's key callbacks (__DATA_CONST 0x22281cea8, 48 bytes, copied to the stack before use):
// version 0, no retain, release or description callbacks, __isEqualCString and __hashCString2.
static const OCFDictionaryKeyCallBacks statCacheKeyCallBacks = {
    0, NULL, NULL, NULL, _isEqualCString, _hashCString2
};

// Native __NSStandardizePathRewriteAutomountIsPathExcluded.exclusionList, 0x2227d6058 (non-external,
// __DATA_CONST,__const): the NULL-terminated list of top-level directories that are never rewritten out of
// /private or the automount directories.
static const char *const exclusionList[] = {
    "/Applications",
    "/Library",
    "/System",
    "/Users",
    "/Volumes",
    "/bin",
    "/cores",
    "/dev",
    "/opt",
    "/private",
    "/sbin",
    "/usr",
    NULL,
};

static void _addToStatCache(const char *path, const char *target);
static BOOL _NSTransmutePathSlashes(unsigned short *characters, ONSInteger *lengthPtr);
static BOOL _pathHasDotDot(const unsigned short *characters, ONSInteger length, BOOL isUNC);
static BOOL
_NSStandardizePathRewriteAutomountIsPathExcluded(const unsigned short *characters, ONSInteger length);

// Native __NSPathComponents, 0x1c058cb70. Non-external. The components of the characters as an autoreleased
// +[NSMutableArray array] of +[NSString stringWithCharacters:length:] strings: a leading "/" when the first
// character is a slash, each run between slashes, and a trailing "/" when there are at least two characters
// and the last is a slash. A length of 0 answers the empty array; when the leading slash is the whole input,
// the trailing check is skipped.
static id _NSPathComponents(const unsigned short *characters, ONSInteger length) {
    id components = [(id)pathMutableArrayClassSymbol array];
    if (length == 0) {
        return components;
    }
    ONSInteger index = 0;
    if (length >= 1 && characters[0] == '/') {
        index = 1;
        [components addObject:[ONSString stringWithCharacters:characters length:1]];
    }
    if (index >= length) {
        return components;
    }
    do {
        const unsigned short *start = characters + index;
        if (*start == '/') {
            index++;
            continue;
        }
        if (index >= length) {
            [components addObject:[ONSString stringWithCharacters:start length:0]];
            break;
        }
        ONSInteger count = 0;
        BOOL ended = NO;
        for (;;) {
            if (start[count] == '/') {
                [components addObject:[ONSString stringWithCharacters:start length:count]];
                index += count;
                break;
            }
            count++;
            if (index + count == length) {
                [components addObject:[ONSString stringWithCharacters:start length:length - index]];
                ended = YES;
                break;
            }
        }
        if (ended) {
            break;
        }
    } while (index < length);
    if (length >= 2 && characters[length - 1] == '/') {
        [components addObject:[ONSString stringWithCharacters:characters + length - 1 length:1]];
    }
    return components;
}

// Native __convertToUnichars, 0x1c058ccd4. Non-external. Converts the UTF-8 bytes with
// __NSConvertToASCIIorUnicode2 (encoding 4, always Unicode, no zone) into a stack conversion record whose
// first 512 bytes are zeroed (the native frame reserves 0x418 bytes for it). A failed conversion or an empty
// result stores 0 through the length. Otherwise the length becomes min(*lengthPtr, result length) (signed);
// an eight-bit result throws NSInternalInconsistencyException
// ("_convertToUnichars: Didn't get Unicode"); a UTF-16 result is copied into the characters and freed when
// the record says so (byte 1 equal to 1).
static void
_convertToUnichars(const char *bytes, size_t length, unsigned short *characters, ONSInteger *lengthPtr) {
    StringConversionBuffer result;
    memset(&result, 0, 512);
    if (!_ONSConvertToASCIIorUnicode2(bytes, length, 4, YES, &result, NULL) || result.numChars == 0) {
        *lengthPtr = 0;
        return;
    }
    ONSInteger count = (ONSInteger)result.numChars;
    if (*lengthPtr < count) {
        count = *lengthPtr;
    }
    *lengthPtr = count;
    if (result.isASCII & 1) {
        objc_exception_throw([(id)pathExceptionClassSymbol
            exceptionWithName:(id)ONSInternalInconsistencyException
                       reason:(id)OF_STR("_convertToUnichars: Didn't get Unicode")
                     userInfo:nil]);
    }
    memmove(characters, result.unicode, 2 * count);
    if (result.shouldFreeChars == 1) {
        free(result.unicode);
    }
}

// Native __NSResolveSymlinksInPathUsingCache, 0x1c058ce00. Non-external. Resolves the symbolic links of the
// path in place (the result is at most 1024 characters). An empty path is left alone. The characters are
// wrapped without copying in an NSCheapMutableString and the default file manager forms their file system
// representation (-getFileSystemRepresentation:maxLength:1025 withPath:; a failure releases the string and
// returns). Without the cache, a path starting with "~" or "/" first asks getattrlist() for ATTR_CMN_FULLPATH
// (bitmap count 5, a 0x40C-byte reply buffer whose first 512 bytes are zeroed); a reply with a path length L
// where 0 < L and L - 1 < 0x401 (unsigned) is converted with __convertToUnichars (L - 1 bytes) and returned.
// Otherwise each component is visited in turn: the component is terminated in place; with the cache, the stat
// cache is consulted under its lock (a cached target is copied with __strlcpy_chk (0x402); a cached NULL
// means "not a link"); otherwise lstat() decides (a failure returns, leaving the path unchanged); a component
// that is not a symbolic link is added to the cache with no target. A link (at most 33 are followed; the 34th
// returns) is read with readlink() (0x401 bytes; a failure returns), cached with its target, and spliced in:
// an absolute target replaces everything up to the component, a relative one the component; a result above
// 0x401 bytes returns; scanning restarts at the start of the buffer (absolute) or of the component
// (relative). When the terminator is reached, the length is set to 1024 and the result converted back with
// __convertToUnichars. The getattrlist() reply size 0x40C exceeds the 0x402 bytes the code treats as the
// buffer's size
// (__strlcpy_chk); the native frame reserves 0x410 bytes there, as this restoration does.
static void _NSResolveSymlinksInPathUsingCache(unsigned short *path, ONSInteger *lengthPtr, BOOL useCache) {
    ONSInteger length = *lengthPtr;
    if (length == 0) {
        return;
    }
    struct stat status;
    memset(&status, 0, sizeof(status));
    char buffer[1026];
    char target[0x410];
    id string = [[(Class)pathCheapMutableStringClassSymbol allocWithZone:NULL] init];
    [string setContentsNoCopy:path length:length freeWhenDone:NO isUnicode:YES];
    if (!([[(id)pathFileManagerClassSymbol defaultManager] getFileSystemRepresentation:buffer
                                                                             maxLength:1025
                                                                              withPath:string] &
          1)) {
        [string release];
        return;
    }
    ONSInteger bufferLength = (ONSInteger)strlen(buffer);
    [string release];
    if (!useCache && *lengthPtr >= 1 && (path[0] == '~' || path[0] == '/')) {
        memset(target, 0, 512);
        struct attrlist attributes = {0};
        attributes.bitmapcount = ATTR_BIT_MAP_COUNT;
        attributes.commonattr = ATTR_CMN_FULLPATH;
        if (getattrlist(buffer, &attributes, target, 0x40C, 0) == 0) {
            uint32_t pathLength = *(uint32_t *)(target + 8);
            if (pathLength != 0 && pathLength - 1 < 0x401) {
                int32_t offset = *(int32_t *)(target + 4);
                *lengthPtr = 1024;
                _convertToUnichars(target + 4 + offset, pathLength - 1, path, lengthPtr);
                return;
            }
        }
    }
    ONSUInteger linkCount = 0;
    char *cursor = buffer;
    for (;;) {
        char *componentStart = cursor;
        ONSInteger componentOffset = componentStart - buffer;
        ONSInteger index = 0;
        char character;
        do {
            character = componentStart[index];
            index++;
        } while (character == '/');
        while (character != '\0' && character != '/') {
            character = componentStart[index];
            index++;
        }
        char *terminator = componentStart + index - 1;
        ONSInteger terminatorOffset = terminator - buffer;
        *terminator = '\0';
        ONSInteger targetLength;
        BOOL cachedLink = NO;
        BOOL cachedPlain = NO;
        const void *value = NULL;
        if (useCache) {
            os_unfair_lock_lock(&statCacheLock);
            if (statCache == NULL) {
                os_unfair_lock_unlock(&statCacheLock);
            } else {
                value = NULL;
                Boolean found = OCFDictionaryGetValueIfPresent(statCache, buffer, &value);
                os_unfair_lock_unlock(&statCacheLock);
                if (found && value != NULL) {
                    cachedLink = YES;
                } else if (found) {
                    cachedPlain = YES;
                }
            }
        }
        if (cachedLink) {
            if (linkCount > 32) {
                return;
            }
            __builtin___strlcpy_chk(target, (const char *)value, 0x402, 0x402);
            targetLength = (ONSInteger)strlen(target);
        } else {
            if (!cachedPlain) {
                if (lstat(buffer, &status) < 0) {
                    return;
                }
            }
            if (cachedPlain || (status.st_mode & S_IFMT) != S_IFLNK) {
                if (!cachedPlain && useCache) {
                    _addToStatCache(buffer, NULL);
                }
                *terminator = character;
                if (character != '\0') {
                    cursor = terminator;
                    continue;
                }
                *lengthPtr = 1024;
                _convertToUnichars(buffer, bufferLength, path, lengthPtr);
                return;
            }
            if (linkCount > 32) {
                return;
            }
            ssize_t linkLength = readlink(buffer, target, 0x401);
            if (linkLength < 0) {
                return;
            }
            targetLength = linkLength;
            target[linkLength] = '\0';
            if (useCache) {
                _addToStatCache(buffer, target);
            }
        }
        *terminator = character;
        ONSInteger prefix = target[0] == '/' ? 0 : componentOffset + 1;
        ONSInteger newLength = bufferLength + targetLength + prefix - terminatorOffset;
        if (newLength > 0x401) {
            return;
        }
        if (bufferLength - terminatorOffset >= 1) {
            memmove(buffer + targetLength + prefix, terminator, bufferLength - terminatorOffset);
        }
        linkCount++;
        memmove(buffer + prefix, target, targetLength);
        buffer[bufferLength + prefix + targetLength - terminatorOffset] = '\0';
        cursor = target[0] == '/' ? buffer : componentStart;
        bufferLength = newLength;
    }
}

// Native __addToStatCache, 0x1c058d240. Non-external. Duplicates the path with strdup() and the target with a
// second strdup() unless the target is the path itself (the same pointer; the value is then the key) or NULL.
// Under the stat cache's lock, the dictionary is created on first use (kCFAllocatorSystemDefault, capacity 0,
// the C string key callbacks, no value callbacks); a key already present frees the copies (the value only
// when it differs from the key), otherwise the pair is added.
static void _addToStatCache(const char *path, const char *target) {
    OCFDictionaryKeyCallBacks callBacks = statCacheKeyCallBacks;
    char *key = strdup(path);
    char *entry = key;
    if (path != target) {
        if (target != NULL) {
            entry = strdup(target);
        } else {
            entry = NULL;
        }
    }
    os_unfair_lock_lock(&statCacheLock);
    if (statCache == NULL) {
        statCache = OCFDictionaryCreateMutable(kOCFAllocatorSystemDefault, 0, &callBacks, NULL);
    }
    if (OCFDictionaryContainsKey(statCache, key)) {
        free(key);
        if (key != entry) {
            free(entry);
        }
    } else {
        OCFDictionaryAddValue(statCache, key, entry);
    }
    os_unfair_lock_unlock(&statCacheLock);
}

// Native __NSStandardizePathUsingCache, 0x1c058d364. Non-external. Standardizes the path in place:
// 1. Collapses slashes (__NSTransmutePathSlashes) and removes trailing slashes, keeping one character; a path
//    starting with two backslashes is a UNC path.
// 2. When the path has a ".." component (__pathHasDotDot), resolves symbolic links on a 1024-character copy
//    (__memmove_chk into the 0x800-byte buffer); a result of 1025 or more characters returns.
// 3. Removes trailing "/." pairs (from index 4 for UNC paths), then leading "./" pairs when the second
//    character is a slash, then each "/./" from index 3 (UNC) or 0.
// 4. While the path has ".." components and more than two characters: removes "/../" and a trailing "/.."
//    after the UNC server and share (or from the start), then cancels the first "component/../" or a trailing
//    "component/..".
// 5. Collapses slashes again and removes trailing slashes (at most down to one character; a path that becomes
//    one character returns).
// 6. Rewrites /private/var/automount/X (23 or more characters), /var/automount/X (15 or more) and /private/X
//    (9 or more) to /X when X's top-level directory is not excluded and the shortened path exists
//    (__NSFileExistsAtPath with the default file manager); for /private with the cache, the top-level
//    directory
//    ("/X") is looked up in the automount cache under its lock and, when missing, checked with
//    -[NSFileManager fileExistsAtPath:] and added to the set (created on first use with
//    [[NSMutableSet alloc] init]); a missing directory leaves the path unchanged.
// The IDA pseudocode follows the assembly; the branches below follow the assembly's block order.
static void _NSStandardizePathUsingCache(unsigned short *path, ONSInteger *lengthPtr, BOOL useCache) {
    unsigned short resolved[1024];
    struct stat status;
    _NSTransmutePathSlashes(path, lengthPtr);
    ONSInteger length = *lengthPtr;
    BOOL isUNC = NO;
    if (length >= 2) {
        ONSInteger last = length - 1;
        length = 1;
        for (;;) {
            if (path[last] != '/') {
                length = last + 1;
                isUNC = path[0] == '\\' && path[1] == '\\';
                break;
            }
            *lengthPtr = last;
            if (last + 1 <= 2) {
                break;
            }
            last--;
        }
    }
    BOOL hasDotDot = _pathHasDotDot(path, length, isUNC);
    if (hasDotDot) {
        ONSInteger resolvedLength = length;
        __builtin___memmove_chk(resolved, path, 2 * (length < 0x400 ? length : 0x400), 0x800);
        _NSResolveSymlinksInPathUsingCache(resolved, &resolvedLength, useCache);
        length = resolvedLength;
        if (length >= 0x401) {
            return;
        }
        memmove(path, resolved, 2 * length);
        *lengthPtr = length;
    }
    ONSInteger start = 3;
    for (;;) {
        if (isUNC) {
            if (length <= 4) {
                goto removeDotSlashes;
            }
        } else if (length < 3) {
            if (length != 2) {
                goto collapseTrailing;
            }
            goto removeLeadingDotSlashes;
        }
        unsigned short *end = path + length;
        if (end[-2] == '/' && end[-1] == '.') {
            length -= 2;
            *lengthPtr = length;
            continue;
        }
        if (isUNC) {
            start = 3;
            goto removeDotSlashes;
        }
        goto removeLeadingDotSlashes;
    }
removeLeadingDotSlashes:
    if (path[1] == '/') {
        ONSInteger removed = 0;
        const unsigned short *source = path;
        for (;;) {
            if (source[0] != '.') {
                if (removed == 0) {
                    start = 0;
                    goto removeDotSlashes;
                }
                break;
            }
            removed += 2;
            unsigned short next = source[3];
            source += 2;
            if (next != '/') {
                break;
            }
        }
        *lengthPtr = length - removed;
        memmove(path, source, 2 * (length - removed));
        start = 0;
        length = *lengthPtr;
    } else {
        start = 0;
    }
removeDotSlashes:
    if (length >= 3) {
        while (start < length - 2) {
            unsigned short *slash = path + start;
            if (slash[0] == '/' && slash[1] == '.' && slash[2] == '/') {
                memmove(slash + 1, slash + 3, 2 * (length - start) - 6);
                length = *lengthPtr - 2;
                *lengthPtr = length;
                start--;
            }
            if (length < 3) {
                break;
            }
            start++;
        }
    }
    if (length < 3 || !hasDotDot) {
        goto collapseTrailing;
    }
    do {
        ONSInteger componentStart;
        if (isUNC) {
            ONSInteger scan;
            if ((ONSUInteger)length < 4) {
                scan = 4;
            } else {
                scan = 3;
                while (path[scan] != '/') {
                    scan++;
                    if (length == scan) {
                        scan = length;
                        break;
                    }
                }
                scan = scan + 1;
            }
            componentStart = length > scan ? length : scan;
            while (scan < length) {
                if (path[scan] == '/') {
                    componentStart = scan;
                    break;
                }
                scan++;
            }
        } else {
            componentStart = 0;
        }
        ONSInteger limit = length - 2;
        BOOL longEnough;
        if (componentStart >= length - 2) {
            longEnough = YES;
        } else {
            unsigned short *component = path + componentStart;
            for (;;) {
                ONSInteger index;
                if (length != 3 && componentStart < length - 4 && component[0] == '/' &&
                    component[1] == '.' && component[2] == '.' && component[3] == '/') {
                    memmove(component + 1, component + 4, 2 * (length - componentStart) - 8);
                    length = *lengthPtr - 3;
                    *lengthPtr = length;
                    index = componentStart - 1;
                } else {
                    index = componentStart;
                }
                if (length >= 3 && index == length - 3) {
                    unsigned short *tail = path + index;
                    if (tail[0] == '/' && tail[1] == '.' && tail[2] == '.') {
                        length -= 2;
                        *lengthPtr = length;
                    }
                }
                limit = length - 2;
                longEnough = length > 2;
                if (length < 3) {
                    break;
                }
                index++;
                if (componentStart != index) {
                    break;
                }
                if (index >= limit) {
                    break;
                }
            }
        }
        if (longEnough && componentStart < length) {
            const unsigned short *tail = path + length - 3;
            for (;;) {
                ONSInteger next = componentStart + 1;
                if (next >= limit) {
                    break;
                }
                ONSInteger count = 0;
                size_t moveLength = 2 * (length - 5 - componentStart);
                unsigned short *cursor = path + 3 + componentStart;
                BOOL found = YES;
                while (cursor[-2] != '/') {
                    count++;
                    moveLength -= 2;
                    cursor++;
                    if (3 - length + componentStart + count == 0) {
                        found = NO;
                        break;
                    }
                }
                if (!found) {
                    break;
                }
                ONSInteger slash = componentStart + count + 1;
                if (length >= 4 && slash < length - 4 && cursor[-1] == '.' && cursor[0] == '.' &&
                    cursor[1] == '/') {
                    *lengthPtr = length - count - 4;
                    memmove(path + 1 + componentStart, cursor + 2, moveLength);
                    length = *lengthPtr;
                    break;
                }
                if (componentStart + count + 4 - length == 0 && tail[0] == '/' && tail[1] == '.' &&
                    tail[2] == '.') {
                    *lengthPtr = next;
                    length = next;
                    break;
                }
                componentStart = slash;
                if (slash >= length) {
                    break;
                }
            }
        }
    } while (_pathHasDotDot(path, length, isUNC) && length > 2);
collapseTrailing:
    _NSTransmutePathSlashes(path, lengthPtr);
    length = *lengthPtr;
    if (length < 2) {
        return;
    }
    while (path[length - 1] == '/') {
        BOOL more = length > 2;
        length--;
        *lengthPtr = length;
        if (!more) {
            return;
        }
    }
    memset(&status, 0, sizeof(status));
    unsigned short *second = path + 1;
    unsigned short *mountPoint;
    unsigned short character;
    if ((ONSUInteger)length >= 23) {
        if (path[0] != '/') {
            return;
        }
        character = second[0];
        if (character != 'p') {
            goto checkVar;
        }
        if (path[2] == 'r' && path[3] == 'i' && path[4] == 'v' && path[5] == 'a' && path[6] == 't' &&
            path[7] == 'e' && path[8] == '/' && path[9] == 'v' && path[10] == 'a' && path[11] == 'r' &&
            path[12] == '/' && path[13] == 'a' && path[14] == 'u' && path[15] == 't' && path[16] == 'o' &&
            path[17] == 'm' && path[18] == 'o' && path[19] == 'u' && path[20] == 'n' && path[21] == 't' &&
            path[22] == '/') {
            mountPoint = path + 22;
            if (_NSStandardizePathRewriteAutomountIsPathExcluded(mountPoint, length - 22) & 1) {
                return;
            }
            if (!_NSFileExistsAtPath(
                    [(id)pathFileManagerClassSymbol defaultManager], mountPoint, *lengthPtr - 22, &status
                )) {
                return;
            }
            length = *lengthPtr - 22;
            *lengthPtr = length;
            memmove(path, mountPoint, 2 * length);
            return;
        }
        goto checkPrivate;
    } else if ((ONSUInteger)length >= 15) {
        if (path[0] != '/') {
            return;
        }
        character = second[0];
    checkVar:
        if (character != 'v') {
            goto checkP;
        }
        if (path[2] != 'a' || path[3] != 'r' || path[4] != '/' || path[5] != 'a' || path[6] != 'u' ||
            path[7] != 't' || path[8] != 'o' || path[9] != 'm' || path[10] != 'o' || path[11] != 'u' ||
            path[12] != 'n' || path[13] != 't' || path[14] != '/') {
            return;
        }
        mountPoint = path + 14;
        if (_NSStandardizePathRewriteAutomountIsPathExcluded(mountPoint, length - 14) & 1) {
            return;
        }
        if (!_NSFileExistsAtPath(
                [(id)pathFileManagerClassSymbol defaultManager], mountPoint, *lengthPtr - 14, &status
            )) {
            return;
        }
        length = *lengthPtr - 14;
        *lengthPtr = length;
        memmove(path, mountPoint, 2 * length);
        return;
    } else {
        if (length < 9) {
            return;
        }
        if (path[0] != '/') {
            return;
        }
        character = second[0];
    checkP:
        if (character != 'p') {
            return;
        }
    }
checkPrivate:
    if (path[2] != 'r' || path[3] != 'i' || path[4] != 'v' || path[5] != 'a' || path[6] != 't' ||
        path[7] != 'e' || path[8] != '/') {
        return;
    }
    mountPoint = path + 8;
    if (_NSStandardizePathRewriteAutomountIsPathExcluded(mountPoint, length - 8) & 1) {
        return;
    }
    if (useCache) {
        unsigned short *end = path + 9;
        if (length >= 10 && path[9] != '/') {
            ONSInteger index = 10;
            for (;;) {
                if (length <= index) {
                    break;
                }
                if (path[index] == '/') {
                    break;
                }
                index++;
            }
            end = path + index;
        }
        id directory = [[ONSString alloc] initWithCharacters:mountPoint length:end - mountPoint];
        os_unfair_lock_lock(&automountCacheLock);
        if (!([automountCache containsObject:directory] & 1)) {
            if (![[(id)pathFileManagerClassSymbol defaultManager] fileExistsAtPath:directory]) {
                os_unfair_lock_unlock(&automountCacheLock);
                [directory release];
                return;
            }
            if (automountCache == nil) {
                automountCache = [[(Class)pathMutableSetClassSymbol alloc] init];
            }
            [automountCache addObject:directory];
        }
        os_unfair_lock_unlock(&automountCacheLock);
        [directory release];
    } else if (!(_NSFileExistsAtPath(
                     [(id)pathFileManagerClassSymbol defaultManager], mountPoint, *lengthPtr - 8, &status
                 ) &
                 1)) {
        return;
    }
    length = *lengthPtr - 8;
    *lengthPtr = length;
    memmove(path, mountPoint, 2 * length);
}

// Native __NSTransmutePathSlashes, 0x1c058dd14. Non-external. Collapses each run of slashes into one, in
// place, after a leading pair of backslashes (kept when the length is at least 2); a run reaching the end
// keeps its single slash. Stores the new length and answers whether it changed.
static BOOL _NSTransmutePathSlashes(unsigned short *characters, ONSInteger *lengthPtr) {
    ONSInteger length = *lengthPtr;
    ONSInteger write = 0;
    if (length >= 2 && characters[0] == '\\') {
        write = characters[1] == '\\' ? 2 : 0;
    }
    if (write < length) {
        ONSInteger read = write;
        for (;;) {
            unsigned short character = characters[read];
            if (character == '/') {
                characters[write] = '/';
                ONSInteger bound = length > read + 1 ? length : read + 1;
                if (read == bound - 1) {
                    write++;
                    break;
                }
                read++;
                BOOL ended = NO;
                while (characters[read] == '/') {
                    read++;
                    if (bound == read) {
                        ended = YES;
                        break;
                    }
                }
                write++;
                if (ended || read >= length) {
                    break;
                }
                continue;
            }
            read++;
            characters[write] = character;
            write++;
            if (read >= length) {
                break;
            }
        }
    }
    *lengthPtr = write;
    return length != write;
}

// Native __pathHasDotDot, 0x1c058ddc8. Non-external. Whether a path of more than one character that starts
// with "~" or "/" has a ".." component: after the UNC server and share for a UNC path, a ".." that is
// preceded by a slash and followed by a slash or the end; at index 0 (length other than 2) a ".." followed by
// a slash.
static BOOL _pathHasDotDot(const unsigned short *characters, ONSInteger length, BOOL isUNC) {
    if (length < 1) {
        return NO;
    }
    BOOL unrooted = characters[0] != '~' && characters[0] != '/';
    if (length - 1 == 0 || unrooted) {
        return NO;
    }
    ONSInteger start;
    if (isUNC) {
        ONSInteger index = 3;
        if ((ONSUInteger)length >= 4) {
            while (characters[index] != '/') {
                index++;
                if (length == index) {
                    index = length;
                    break;
                }
            }
        }
        start = (length > index + 1 ? length : index + 1) - 1;
        if (index + 1 < length) {
            ONSInteger probe = index;
            for (;;) {
                if (characters[probe + 1] == '/') {
                    start = probe;
                    break;
                }
                probe++;
                if (probe + 1 >= length) {
                    break;
                }
            }
        }
        start += 2;
    } else {
        start = 0;
    }
    if (start >= length - 1) {
        return NO;
    }
    ONSInteger end = start + 2;
    do {
        if (characters[end - 2] == '.' && characters[end - 1] == '.') {
            if (length != 2 && end == 2) {
                if (characters[2] == '/') {
                    return YES;
                }
            } else if (end - 2 >= 1 && characters[end - 3] == '/') {
                if (end < length) {
                    if (characters[end] == '/') {
                        return YES;
                    }
                } else if (end - length == 0) {
                    return YES;
                }
            }
        }
        end++;
    } while (end - length != 1);
    return NO;
}

// Native __NSStandardizePathRewriteAutomountIsPathExcluded, 0x1c058df14. Non-external. Whether the
// characters start with an entry of the exclusion list (each byte sign-extended to 16 bits) that ends there
// or is followed by a slash.
static BOOL
_NSStandardizePathRewriteAutomountIsPathExcluded(const unsigned short *characters, ONSInteger length) {
    for (const char *const *entry = exclusionList; *entry != NULL; entry++) {
        const char *prefix = *entry;
        const unsigned short *cursor = characters;
        ONSInteger matched = 0;
        BOOL mismatch = NO;
        char character = *prefix++;
        while (character != '\0') {
            if (length <= matched) {
                mismatch = YES;
                break;
            }
            if (*cursor++ != (unsigned short)(signed char)character) {
                mismatch = YES;
                break;
            }
            character = *prefix++;
            matched++;
        }
        if (mismatch) {
            continue;
        }
        if (length <= matched || *cursor == '/') {
            return YES;
        }
    }
    return NO;
}

// Native __NSAppendPathComponent, 0x1c058e9e0. Non-external. Appends the component to the path buffer of the
// given capacity, inserting a slash unless the path is empty, is "/" or is a two-backslash UNC prefix; an
// empty component succeeds without change. Answers NO, unchanged, when the result would exceed the capacity.
static BOOL _NSAppendPathComponent(
    unsigned short *characters,
    ONSInteger *lengthPtr,
    ONSInteger capacity,
    const unsigned short *component,
    ONSInteger componentLength
) {
    if (componentLength == 0) {
        return YES;
    }
    ONSInteger length = *lengthPtr;
    if (length == 0) {
        if (capacity < componentLength) {
            return NO;
        }
    } else {
        BOOL needsSlash;
        if (length == 2) {
            needsSlash = characters[0] != '\\' || characters[1] != '\\';
        } else if (length == 1) {
            needsSlash = characters[0] != '/';
        } else {
            needsSlash = YES;
        }
        if (capacity < length + componentLength + (ONSInteger)needsSlash) {
            return NO;
        }
        if (needsSlash) {
            characters[length] = '/';
            length++;
            *lengthPtr = length;
        }
    }
    memmove(characters + length, component, 2 * componentLength);
    *lengthPtr = *lengthPtr + componentLength;
    return YES;
}

// Native __NSStartOfPathExtension, 0x1c058efb0. Non-external. The index of the extension's dot in the last
// component (found with _CFGetPathExtensionRangesFromPathComponentUniChars, primary range only), or 0 when
// the component has no extension.
static ONSInteger _NSStartOfPathExtension(const unsigned short *characters, ONSInteger length) {
    ONSInteger componentStart = 0;
    if (length >= 2) {
        componentStart = length - 1;
        const unsigned short *cursor = characters + length - 2;
        while (componentStart != 0) {
            if (*cursor-- == '/') {
                break;
            }
            componentStart--;
        }
    }
    OCFRange range = {0, 0};
    _OCFGetPathExtensionRangesFromPathComponentUniChars(
        characters + componentStart, length - componentStart, &range, NULL
    );
    return range.location == -1 ? 0 : componentStart + range.location - 1;
}

// Native __NSExpandTildeInPath, 0x1c058f95c. Non-external. Replaces a leading "~" or "~user" (up to the first
// slash) with NSHomeDirectoryForUser (nil for the current user); nothing changes when there is no home
// directory or the result would exceed 1024 characters.
static void _NSExpandTildeInPath(unsigned short *path, ONSInteger *lengthPtr) {
    ONSInteger length = *lengthPtr;
    if (length < 1 || path[0] != '~') {
        return;
    }
    id userName;
    ONSInteger end = 1;
    if (length == 1) {
        userName = nil;
    } else {
        while (path[end] != '/') {
            end++;
            if (length == end) {
                end = length;
                break;
            }
        }
        if (end - 1 == 0) {
            userName = nil;
        } else {
            userName = [ONSString stringWithCharacters:path + 1 length:end - 1];
        }
    }
    id home = ONSHomeDirectoryForUser(userName);
    if (home == nil) {
        return;
    }
    ONSInteger homeLength = (ONSInteger)[home length];
    ONSInteger current = *lengthPtr;
    ONSInteger rest = current - end;
    if (rest + homeLength > 0x400) {
        return;
    }
    if (current > end) {
        memmove(path + homeLength, path + end, 2 * rest);
    }
    [home getCharacters:path range:(ONSRange){0, (ONSUInteger)homeLength}];
    *lengthPtr = homeLength - end + *lengthPtr;
}

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. iOS Device 24A446: unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FB-bundle-paths-plist.
// Additional uniqued shared-cache GOT bindings (medium): NSRangeException 0x22227f5e0 and
// NSGenericException 0x22227f4e0. Keep these methods with their file-local path helpers.

@implementation ONSPathStore2

// Native +[NSPathStore2 pathStoreWithCharacters:length:], 0x1c058dfa0. Non-external.
+ (id)pathStoreWithCharacters:(const unsigned short *)characters length:(ONSUInteger)length {
    if (length > 1024) {
        return [ONSString stringWithCharacters:characters length:length];
    }
    Class cls = (Class)objc_opt_self((id)pathStoreClassSymbol);
    ONSPathStore2 *path = ONSAllocateObject(cls, 2 * length, NULL);
    if (length != 0) {
        memmove(path->_characters, characters, 2 * length);
        while (length > 1 && path->_characters[length - 1] == '/') {
            --length;
        }
    }
    path->_lengthAndRefCount = (uint32_t)length << 20;
    return objc_autorelease(path);
}

// Native -[NSPathStore2 length], 0x1c058e08c. Non-external.
- (ONSUInteger)length {
    return _lengthAndRefCount >> 20;
}

// Native -[NSPathStore2 characterAtIndex:], 0x1c058e0a0. Non-external.
// Hex-Rays depicts control falling into getCharacters:range: after the nonreturning throw.
- (unsigned short)characterAtIndex:(ONSUInteger)index {
    if (index >= _lengthAndRefCount >> 20) {
        id proem = _ONSMethodExceptionProem(self, _cmd);
        id reason = [ONSString stringWithFormat:(id)OF_STR("%@: index (%lu) beyond bounds (%d)"),
                                                proem,
                                                index,
                                                _lengthAndRefCount >> 20];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSRangeException reason:reason
                                                    userInfo:nil]);
    }
    return _characters[index];
}

// Native -[NSPathStore2 getCharacters:range:], 0x1c058e138. Non-external.
// Foundation defect: the unsigned addition at 0x1c058e14c can wrap, defeating the bound check and
// allowing memmove to read outside the object. Route a wrapped end through the same exception path.
- (void)getCharacters:(unsigned short *)buffer range:(ONSRange)range {
    ONSUInteger end = range.location + range.length;
    if (end < range.location || end >= (_lengthAndRefCount >> 20) + 1) {
        id proem = _ONSMethodExceptionProem(self, _cmd);
        id reason = [ONSString stringWithFormat:(id)OF_STR("%@: index (%lu) beyond bounds (%d)"),
                                                proem,
                                                end,
                                                (_lengthAndRefCount >> 20) + 1];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSRangeException reason:reason
                                                    userInfo:nil]);
    }
    if (range.length != 0) {
        memmove(buffer, _characters + range.location, 2 * range.length);
    }
}

// Native -[NSPathStore2 _fastCharacterContents], 0x1c058e1f4. Non-external.
- (const unsigned short *)_fastCharacterContents {
    return _characters;
}

// Native -[NSPathStore2 copyWithZone:], 0x1c058e204. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return objc_retain(self);
}

// Native -[NSPathStore2 copy], 0x1c058e208. Non-external.
- (id)copy {
    return objc_retain(self);
}

// Native -[NSPathStore2 hash], 0x1c058e20c. Non-external.
- (ONSUInteger)hash {
    return OCFStringHashCharacters(_characters, _lengthAndRefCount >> 20);
}

// Native -[NSPathStore2 isEqualToString:], 0x1c058e228. Non-external.
- (BOOL)isEqualToString:(id)string {
    if (string == self) {
        return YES;
    }
    if (string == nil) {
        return NO;
    }
    unsigned short stack[1040];
    unsigned short *allocated = NULL;
    const unsigned short *characters;
    ONSUInteger length;
    if (object_getClass(string) == (Class)pathStoreClassSymbol) {
        ONSPathStore2 *path = string;
        length = path->_lengthAndRefCount >> 20;
        characters = path->_characters;
    } else {
        length = [string length];
        unsigned short *buffer = stack;
        if ((ONSInteger)length >= 1041) {
            buffer = allocated = malloc_type_malloc(2 * length, 0x1000040BDFB0063);
        }
        [string getCharacters:buffer range:(ONSRange){0, length}];
        characters = buffer;
    }

    ONSUInteger ownLength = _lengthAndRefCount >> 20;
    ONSUInteger limit = ownLength < length ? ownLength : length;
    BOOL equal = ownLength == length;
    for (ONSUInteger index = 0; index < limit; ++index) {
        if (_characters[index] != characters[index]) {
            equal = NO;
            break;
        }
    }
    if (allocated != NULL) {
        free(allocated);
    }
    return equal;
}

// Native +[NSPathStore2 pathWithComponents:], 0x1c058e3e4. Non-external.
+ (id)pathWithComponents:(id)components {
    if (components == nil || !_ONSIsNSArray(components)) {
        id reason = [ONSString stringWithFormat:(id)OF_STR("%@: components argument is not an array"),
                                                _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if ([components count] == 0) {
        return (id)OF_STR("");
    }

    ONSUInteger count = [components count];
    if (count >> 60 != 0) {
        OCFStringRef reason = OCFStringCreateWithFormat(
            NULL,
            NULL,
            OF_STR(
                "*** attempt to create a temporary id buffer which is too large or with a negative count "
                "(%lu) -- possibly data is corrupt"
            ),
            count
        );
        id exception = [ONSException exceptionWithName:(id)ONSGenericException
                                                reason:(id)reason
                                              userInfo:nil];
        OCFRelease(reason);
        objc_exception_throw(exception);
    }

    ONSUInteger storageCount = count > 1 ? count : 1;
    id objectStack[count <= 256 ? storageCount : 1];
    ONSInteger lengthStack[count <= 256 ? storageCount : 1];
    id *allocatedObjects = NULL;
    id *objects = objectStack;
    ONSInteger *lengths = lengthStack;
    if (count > 256) {
        size_t actualCount = 0;
        objects = allocatedObjects = _OCFCreateArrayStorage(storageCount, false, &actualCount);
        lengths = malloc_type_malloc(storageCount * sizeof(*lengths), 0x100004000313F17);
    } else {
        bzero(lengths, storageCount * sizeof(*lengths));
    }

    id result = nil;
    @try {
        ONSInteger sum = 0;
        ONSInteger longest = 0;
        for (ONSUInteger i = 0; i < count; ++i) {
            id object = [components objectAtIndexedSubscript:i];
            objects[i] = object;
            ONSInteger length = [object length];
            lengths[i] = length;
            if (length >= longest) {
                longest = length;
            }
            sum += length;
        }
        ONSUInteger capacity = count + (ONSUInteger)sum - 1;
        ONSUInteger pathCount = capacity > 1 ? capacity : 1;
        unsigned short pathStack[capacity <= 256 ? pathCount : 1];
        unsigned short *path = pathStack;
        if (capacity > 256) {
            path = malloc_type_malloc(pathCount * sizeof(*path), 0x1000040BDFB0063);
        } else {
            bzero(path, pathCount * sizeof(*path));
        }
        ONSUInteger longestCount = (ONSUInteger)longest;
        ONSUInteger partCount = longestCount > 1 ? longestCount : 1;
        unsigned short partStack[longestCount <= 256 ? partCount : 1];
        unsigned short *part = partStack;
        if (longestCount > 256) {
            part = malloc_type_malloc(partCount * sizeof(*part), 0x1000040BDFB0063);
        } else {
            bzero(part, partCount * sizeof(*part));
        }

        ONSInteger length = 0;
        for (ONSUInteger i = 0; i < count; ++i) {
            [objects[i] getCharacters:part range:(ONSRange){0, (ONSUInteger)lengths[i]}];
            if (length + lengths[i] >= 1019) {
                _NSTransmutePathSlashes(path, &length);
                while (length > 1 && path[length - 1] == '/') {
                    --length;
                }
            }
            if (!_NSAppendPathComponent(path, &length, (ONSInteger)capacity, part, lengths[i])) {
                id proem = _ONSMethodExceptionProem(self, _cmd);
                id current = [ONSString stringWithCharacters:path length:length];
                ONSLog((id)OF_STR("%@: cannot append path '%@' to path '%@'"), proem, objects[i], current);
            }
        }
        if (longestCount > 256) {
            free(part);
        }

        _NSTransmutePathSlashes(path, &length);
        while (length > 1 && path[length - 1] == '/') {
            --length;
        }
        ONSInteger index = length >= 2 && path[0] == '\\' && path[1] == '\\' ? 2 : 0;
        while (index < length && path[index] != 0) {
            ++index;
        }
        if (index >= length) {
            result = [ONSPathStore2 pathStoreWithCharacters:path length:length];
        } else {
            os_log_t log = _NSOSLog();
            if (os_log_type_enabled(log, OS_LOG_TYPE_FAULT)) {
                id lengthsObject = [(id<PathStoreArrayMessages>)components valueForKey:(id)OF_STR("length")];

                struct __attribute__((packed)) {
                    uint32_t header;
                    id components;
                    uint16_t lengthsHeader;
                    id lengths;
                } payload = {138478083, components, 2112, lengthsObject};

                _os_log_fault_impl(
                    &__dso_handle,
                    log,
                    OS_LOG_TYPE_FAULT,
                    "Creating path from components (%{private}@, lengths: %@) resulted in an embedded NUL "
                    "character",
                    (uint8_t *)&payload,
                    sizeof(payload)
                );
            }
        }
        if (capacity > 256) {
            free(path);
        }
        if (count > 256) {
            free(lengths);
        }
    } @finally {
        free(allocatedObjects);
    }
    return result;
}

// Native -[NSPathStore2 pathComponents], 0x1c058eaac. Non-external.
- (id)pathComponents {
    return _NSPathComponents(_characters, _lengthAndRefCount >> 20);
}

// Native -[NSPathStore2 lastPathComponent], 0x1c058eb04. Non-external.
- (id)lastPathComponent {
    ONSUInteger length = _lengthAndRefCount >> 20;
    for (ONSUInteger start = length; start > 1; --start) {
        if (_characters[start - 2] == '/') {
            return [ONSPathStore2 pathStoreWithCharacters:_characters + start - 1 length:length - start + 1];
        }
    }
    return self;
}

// Native -[NSPathStore2 stringByDeletingLastPathComponent], 0x1c058eb68. Non-external.
- (id)stringByDeletingLastPathComponent {
    ONSUInteger length = _lengthAndRefCount >> 20;
    ONSUInteger end = length == 1 && _characters[0] == '/';
    for (ONSUInteger start = length; start > 1; --start) {
        if (_characters[start - 2] == '/') {
            end = start > 2 ? start - 2 : 1;
            break;
        }
    }
    return [ONSPathStore2 pathStoreWithCharacters:_characters length:end];
}

// Native -[NSPathStore2 stringByAppendingPathComponent:], 0x1c058ebe0. Non-external.
// Foundation defect: error returns at 0x1c058edec and after the embedded-NUL check skip frees.
// Release both heap allocations after emitting the same diagnostic and before returning nil.
- (id)stringByAppendingPathComponent:(id)string {
    if (string == nil) {
        return self;
    }

    ONSUInteger length = _lengthAndRefCount >> 20;
    unsigned short componentStack[1040];
    unsigned short *allocated = NULL;
    const unsigned short *component;
    ONSUInteger componentLength;
    if (object_getClass(string) == (Class)pathStoreClassSymbol) {
        ONSPathStore2 *path = string;
        component = path->_characters;
        componentLength = path->_lengthAndRefCount >> 20;
    } else {
        componentLength = [string length];
        unsigned short *buffer = componentStack;
        if ((ONSInteger)componentLength >= 1041) {
            buffer = allocated = malloc_type_malloc(2 * componentLength, 0x1000040BDFB0063);
        }
        [string getCharacters:buffer range:(ONSRange){0, componentLength}];
        component = buffer;
    }

    ONSUInteger capacity = length + componentLength + 1;
    ONSUInteger count = capacity > 1 ? capacity : 1;
    unsigned short pathStack[capacity <= 256 ? count : 1];
    unsigned short *path = pathStack;
    if (capacity > 256) {
        path = malloc_type_malloc(count * sizeof(*path), 0x1000040BDFB0063);
    } else {
        bzero(path, count * sizeof(*path));
    }
    memmove(path, _characters, 2 * length);
    ONSInteger resultLength = length;
    id result = nil;
    if (!_NSAppendPathComponent(path, &resultLength, capacity, component, componentLength)) {
        ONSLog(
            (id)OF_STR("%@: cannot append path '%@' to path '%@'"),
            _ONSMethodExceptionProem(self, _cmd),
            string,
            self
        );
    } else {
        _NSTransmutePathSlashes(path, &resultLength);
        while (resultLength > 1 && path[resultLength - 1] == '/') {
            --resultLength;
        }
        ONSInteger index = resultLength >= 2 && path[0] == '\\' && path[1] == '\\' ? 2 : 0;
        while (index < resultLength && path[index] != 0) {
            ++index;
        }
        if (index >= resultLength) {
            if (allocated != NULL) {
                free(allocated);
                allocated = NULL;
            }
            result = [ONSPathStore2 pathStoreWithCharacters:path length:resultLength];
        } else {
            os_log_t log = _NSOSLog();
            if (os_log_type_enabled(log, OS_LOG_TYPE_FAULT)) {
                ONSUInteger appendedLength = [string length];
                ONSUInteger receiverLength = [self length];

                struct __attribute__((packed)) {
                    uint32_t header;
                    id component;
                    uint16_t componentLengthHeader;
                    ONSUInteger componentLength;
                    uint16_t receiverHeader;
                    id receiver;
                    uint16_t receiverLengthHeader;
                    ONSUInteger receiverLength;
                } payload = {138478595, string, 2048, appendedLength, 2113, self, 2048, receiverLength};

                _os_log_fault_impl(
                    &__dso_handle,
                    log,
                    OS_LOG_TYPE_FAULT,
                    "Appending path component %{private}@ (length: %lu) to string %{private}@ (length: %lu) "
                    "resulted in an embedded NUL character",
                    (uint8_t *)&payload,
                    sizeof(payload)
                );
            }
        }
    }
    if (allocated != NULL) {
        free(allocated);
    }
    if (capacity > 256) {
        free(path);
    }
    return result;
}

// Native -[NSPathStore2 pathExtension], 0x1c058ef48. Non-external.
- (id)pathExtension {
    ONSInteger length = _lengthAndRefCount >> 20;
    ONSInteger start = _NSStartOfPathExtension(_characters, length);
    if (start <= 0) {
        return (id)OF_STR("");
    }
    return [self substringWithRange:(ONSRange){(ONSUInteger)start + 1, (ONSUInteger)(length - start - 1)}];
}

// Native -[NSPathStore2 stringByDeletingPathExtension], 0x1c058f05c. Non-external.
- (id)stringByDeletingPathExtension {
    ONSInteger length = _lengthAndRefCount >> 20;
    ONSInteger start = _NSStartOfPathExtension(_characters, length);
    return [ONSPathStore2 pathStoreWithCharacters:_characters length:start <= 0 ? length : start];
}

// Native -[NSPathStore2 _stringByStandardizingPathUsingCache:], 0x1c058fa5c. Non-external.
// The assembly passes only the buffer and length to _NSExpandTildeInPath.
- (id)_stringByStandardizingPathUsingCache:(BOOL)useCache {
    unsigned short path[1024];
    ONSInteger length = _lengthAndRefCount >> 20;
    ONSInteger copyLength = length < 1024 ? length : 1024;
    memmove(path, _characters, 2 * copyLength);
    _NSExpandTildeInPath(path, &length);
    _NSStandardizePathUsingCache(path, &length, useCache);
    ONSInteger ownLength = _lengthAndRefCount >> 20;
    ONSInteger limit = ownLength < length ? ownLength : length;
    for (ONSInteger index = 0; index < limit; ++index) {
        if (_characters[index] != path[index]) {
            return [ONSPathStore2 pathStoreWithCharacters:path length:length];
        }
    }
    return ownLength == length ? self : [ONSPathStore2 pathStoreWithCharacters:path length:length];
}

// Native -[NSPathStore2 stringByStandardizingPath], 0x1c058fb7c. Non-external.
- (id)stringByStandardizingPath {
    return [self _stringByStandardizingPathUsingCache:NO];
}

// Native -[NSPathStore2 _stringByResolvingSymlinksInPathUsingCache:], 0x1c058fb84. Non-external.
// The assembly passes only the buffer and length to _NSExpandTildeInPath.
- (id)_stringByResolvingSymlinksInPathUsingCache:(BOOL)useCache {
    unsigned short path[1024];
    ONSInteger length = _lengthAndRefCount >> 20;
    ONSInteger copyLength = length < 1024 ? length : 1024;
    memmove(path, _characters, 2 * copyLength);
    _NSExpandTildeInPath(path, &length);
    _NSResolveSymlinksInPathUsingCache(path, &length, useCache);
    _NSStandardizePathUsingCache(path, &length, useCache);
    ONSInteger ownLength = _lengthAndRefCount >> 20;
    ONSInteger limit = ownLength < length ? ownLength : length;
    for (ONSInteger index = 0; index < limit; ++index) {
        if (_characters[index] != path[index]) {
            return [ONSPathStore2 pathStoreWithCharacters:path length:length];
        }
    }
    return ownLength == length ? self : [ONSPathStore2 pathStoreWithCharacters:path length:length];
}

@end

@implementation ONSString (NSPathUtilities)

// Native +[NSString(NSPathUtilities) pathWithComponents:], 0x1c058fcbc. Non-external. A tail call of
// +[NSPathStore2 pathWithComponents:].
+ (id)pathWithComponents:(id)components {
    return [(id)pathStoreClassSymbol pathWithComponents:components];
}

// Native -[NSString(NSPathUtilities) pathComponents], 0x1c058fcc8. Non-external. The characters are copied
// into a zeroed stack buffer (up to 256 characters) or a malloc_type_malloc buffer (type 0x1000040BDFB0063,
// freed afterwards) of max(length, 1) characters; the result is __NSPathComponents.
- (id)pathComponents {
    ONSUInteger length = [self length];
    ONSUInteger count = length > 1 ? length : 1;
    unsigned short stackBuffer[length < 0x101 ? count : 1];
    unsigned short *buffer = stackBuffer;
    if (length > 0x100) {
        buffer = (unsigned short *)malloc_type_malloc(2 * count, 0x1000040BDFB0063);
    } else {
        bzero(buffer, 2 * count);
    }
    [self getCharacters:buffer range:(ONSRange){0, length}];
    id components = _NSPathComponents(buffer, (ONSInteger)length);
    if (length >= 0x101) {
        free(buffer);
    }
    return components;
}

// Native -[NSString(NSPathUtilities) lastPathComponent], 0x1c058fe74. Non-external. The buffer as in
// -pathComponents; after __NSTransmutePathSlashes, trailing slashes are dropped (a path of only slashes keeps
// one character) and the result is the NSPathStore2 of the characters after the last remaining slash.
- (id)lastPathComponent {
    ONSUInteger length = [self length];
    ONSUInteger count = length > 1 ? length : 1;
    unsigned short stackBuffer[length < 0x101 ? count : 1];
    unsigned short *buffer = stackBuffer;
    ONSInteger pathLength = (ONSInteger)length;
    if (length > 0x100) {
        buffer = (unsigned short *)malloc_type_malloc(2 * count, 0x1000040BDFB0063);
    } else {
        bzero(buffer, 2 * count);
    }
    [self getCharacters:buffer range:(ONSRange){0, length}];
    _NSTransmutePathSlashes(buffer, &pathLength);
    ONSInteger start = 0;
    if (pathLength >= 2) {
        BOOL scan = YES;
        if (buffer[pathLength - 1] == '/') {
            if (pathLength == 2) {
                pathLength = 1;
                scan = NO;
            } else {
                ONSInteger counter = pathLength + 1;
                const unsigned short *cursor = buffer + pathLength - 2;
                for (;;) {
                    if (*cursor-- != '/') {
                        pathLength = counter - 2;
                        break;
                    }
                    counter--;
                    if (counter <= 3) {
                        pathLength = 1;
                        scan = NO;
                        break;
                    }
                }
            }
        }
        if (scan) {
            ONSInteger index = pathLength - 1;
            if (index != 0) {
                const unsigned short *cursor = buffer + pathLength - 2;
                for (;;) {
                    if (*cursor-- == '/') {
                        break;
                    }
                    index--;
                    if (index == 0) {
                        break;
                    }
                }
            }
            start = index;
        }
    }
    id component = [(id)pathStoreClassSymbol pathStoreWithCharacters:buffer + start
                                                              length:pathLength - start];
    if (length >= 0x101) {
        free(buffer);
    }
    return component;
}

// Native -[NSString(NSPathUtilities) stringByDeletingLastPathComponent], 0x1c0590018. Non-external. The
// buffer as in -pathComponents; after __NSTransmutePathSlashes and dropping trailing slashes, the result is
// the NSPathStore2 of the characters before the last slash (the slash is kept when it is the first
// character); a path without a slash gives the empty path, and "/" or a path of only slashes gives "/".
- (id)stringByDeletingLastPathComponent {
    ONSUInteger length = [self length];
    ONSUInteger count = length > 1 ? length : 1;
    unsigned short stackBuffer[length < 0x101 ? count : 1];
    unsigned short *buffer = stackBuffer;
    ONSInteger pathLength = (ONSInteger)length;
    if (length > 0x100) {
        buffer = (unsigned short *)malloc_type_malloc(2 * count, 0x1000040BDFB0063);
    } else {
        bzero(buffer, 2 * count);
    }
    [self getCharacters:buffer range:(ONSRange){0, length}];
    _NSTransmutePathSlashes(buffer, &pathLength);
    ONSInteger resultLength = 0;
    BOOL rootOnly = NO;
    if (pathLength < 2) {
        rootOnly = pathLength == 1;
        pathLength = -1;
    } else if (buffer[pathLength - 1] == '/') {
        if (pathLength == 2) {
            rootOnly = YES;
        } else {
            ONSInteger counter = pathLength + 1;
            const unsigned short *cursor = buffer + pathLength - 2;
            for (;;) {
                if (*cursor-- != '/') {
                    pathLength = counter - 2;
                    break;
                }
                counter--;
                if (counter <= 3) {
                    rootOnly = YES;
                    break;
                }
            }
        }
    }
    if (rootOnly) {
        resultLength = buffer[0] == '/' ? 1 : 0;
    } else if (pathLength != -1 && pathLength != 1) {
        ONSInteger index = pathLength - 2;
        for (;;) {
            if (buffer[index] == '/') {
                resultLength = index != 0 ? index : 1;
                break;
            }
            index--;
            if (index == -1) {
                break;
            }
        }
    }
    id result = [(id)pathStoreClassSymbol pathStoreWithCharacters:buffer length:resultLength];
    if (length >= 0x101) {
        free(buffer);
    }
    return result;
}

// Native -[NSString(NSPathUtilities) stringByAppendingPathComponent:], 0x1c05901c8. Non-external. The
// component's characters go into a buffer as in -pathComponents; the receiver's characters go into a second
// buffer of max(length + componentLength + 1, 1) characters (stack up to 256). When __NSAppendPathComponent
// fails, NSLog reports "%@: cannot append path '%@' to path '%@'" (the method's exception proem, the
// component, the receiver) and the result is nil. Otherwise slashes are collapsed and trailing slashes
// dropped (a path of only slashes keeps one character); a NUL character after the UNC prefix makes the result
// nil after an os_log fault on __NSOSLog ("Appending path component %{private}@ (length: %lu) to string
// %{private}@ (length: %lu) resulted in an embedded NUL character"); the result is the NSPathStore2 of the
// characters.
// Foundation defect: both nil results return without freeing the malloc_type_malloc buffers (lengths above
// 256), a leak (native 0x1c0590384 and 0x1c0590448, confidence: verified). This restoration frees them before
// returning nil; nothing else differs.
- (id)stringByAppendingPathComponent:(id)string {
    ONSUInteger componentLength = [string length];
    ONSUInteger componentCount = componentLength > 1 ? componentLength : 1;
    unsigned short componentStack[componentLength < 0x101 ? componentCount : 1];
    unsigned short *component = componentStack;
    if (componentLength > 0x100) {
        component = (unsigned short *)malloc_type_malloc(2 * componentCount, 0x1000040BDFB0063);
    } else {
        bzero(component, 2 * componentCount);
    }
    [string getCharacters:component range:(ONSRange){0, componentLength}];
    ONSUInteger selfLength = [self length];
    ONSInteger pathLength = (ONSInteger)selfLength;
    ONSUInteger total = componentLength + selfLength + 1;
    ONSUInteger count = total <= 1 ? 1 : total;
    unsigned short pathStack[total < 0x101 ? count : 1];
    unsigned short *path = pathStack;
    if (total > 0x100) {
        path = (unsigned short *)malloc_type_malloc(2 * count, 0x1000040BDFB0063);
    } else {
        bzero(path, 2 * count);
    }
    [self getCharacters:path range:(ONSRange){0, selfLength}];
    if (!(_NSAppendPathComponent(
              path, &pathLength, (ONSInteger)total, component, (ONSInteger)componentLength
          ) &
          1)) {
        ONSLog(
            (id)OF_STR("%@: cannot append path '%@' to path '%@'"),
            _ONSMethodExceptionProem(self, _cmd),
            string,
            self
        );
        goto failed;
    }
    _NSTransmutePathSlashes(path, &pathLength);
    ONSInteger resultLength = pathLength;
    ONSInteger index = 0;
    if (pathLength >= 2) {
        if (path[pathLength - 1] == '/') {
            if (pathLength == 2) {
                resultLength = 1;
                goto scanForNul;
            }
            ONSInteger counter = pathLength + 1;
            const unsigned short *cursor = path + pathLength - 2;
            resultLength = 1;
            for (;;) {
                if (*cursor-- != '/') {
                    resultLength = counter - 2;
                    break;
                }
                counter--;
                if (counter <= 3) {
                    goto scanForNul;
                }
            }
        }
        if (path[0] == '\\') {
            index = path[1] == '\\' ? 2 : 0;
        }
    }
    if (index >= resultLength) {
        goto make;
    }
scanForNul:
    for (;;) {
        if (path[index] == 0) {
            os_log_t log = _NSOSLog();
            if (os_log_type_enabled(log, OS_LOG_TYPE_FAULT)) {
                ONSUInteger appendedLength = [string length];
                ONSUInteger receiverLength = [self length];

                // Native 0x1c05904a4-0x1c05904c8 packs 42 bytes before _os_log_fault_impl.
                // Calling the os_log_fault macro here would repeat os_log_type_enabled and change order.
                struct __attribute__((packed)) {
                    uint32_t header;
                    id component;
                    uint16_t componentLengthHeader;
                    ONSUInteger componentLength;
                    uint16_t receiverHeader;
                    id receiver;
                    uint16_t receiverLengthHeader;
                    ONSUInteger receiverLength;
                } payload = {138478595, string, 2048, appendedLength, 2113, self, 2048, receiverLength};

                _Static_assert(sizeof(payload) == 42, "The native fault payload is 42 bytes.");
                _os_log_fault_impl(
                    &__dso_handle,
                    log,
                    OS_LOG_TYPE_FAULT,
                    "Appending path component %{private}@ (length: %lu) to string %{private}@ (length: %lu) "
                    "resulted in an embedded NUL character",
                    (uint8_t *)&payload,
                    sizeof(payload)
                );
            }
            goto failed;
        }
        index++;
        if (resultLength == index) {
            break;
        }
    }
make:;
    id result = [(id)pathStoreClassSymbol pathStoreWithCharacters:path length:resultLength];
    if (total >= 0x101) {
        free(path);
    }
    if (componentLength >= 0x101) {
        free(component);
    }
    return result;
failed:
    if (total >= 0x101) {
        free(path);
    }
    if (componentLength >= 0x101) {
        free(component);
    }
    return nil;
}

// Native -[NSString(NSPathUtilities) pathExtension], 0x1c05904f8. Non-external. The buffer as in
// -pathComponents; after __NSTransmutePathSlashes and dropping trailing slashes, the NSPathStore2 of the
// characters after the extension's dot (__NSStartOfPathExtension), or the constant empty string when it
// answers 0.
- (id)pathExtension {
    ONSUInteger length = [self length];
    ONSUInteger count = length > 1 ? length : 1;
    unsigned short stackBuffer[length < 0x101 ? count : 1];
    unsigned short *buffer = stackBuffer;
    ONSInteger pathLength = (ONSInteger)length;
    if (length > 0x100) {
        buffer = (unsigned short *)malloc_type_malloc(2 * count, 0x1000040BDFB0063);
    } else {
        bzero(buffer, 2 * count);
    }
    [self getCharacters:buffer range:(ONSRange){0, length}];
    _NSTransmutePathSlashes(buffer, &pathLength);
    ONSInteger trimmed = pathLength;
    if (trimmed > 1 && buffer[trimmed - 1] == '/') {
        if (trimmed == 2) {
            trimmed = 1;
        } else {
            ONSInteger counter = trimmed + 1;
            const unsigned short *cursor = buffer + trimmed - 2;
            trimmed = 1;
            for (;;) {
                if (*cursor-- != '/') {
                    trimmed = counter - 2;
                    break;
                }
                counter--;
                if (counter <= 3) {
                    break;
                }
            }
        }
    }
    ONSInteger start = _NSStartOfPathExtension(buffer, trimmed);
    id extension;
    if (start >= 1) {
        extension = [(id)pathStoreClassSymbol pathStoreWithCharacters:buffer + start + 1
                                                               length:trimmed + ~start];
    } else {
        extension = (id)OF_STR("");
    }
    if (length >= 0x101) {
        free(buffer);
    }
    return extension;
}

// Native -[NSString(NSPathUtilities) stringByDeletingPathExtension], 0x1c059069c. Non-external. As
// -pathExtension; the NSPathStore2 of the characters before the extension's dot, or of the whole trimmed path
// when __NSStartOfPathExtension answers 0 or less.
- (id)stringByDeletingPathExtension {
    ONSUInteger length = [self length];
    ONSUInteger count = length > 1 ? length : 1;
    unsigned short stackBuffer[length < 0x101 ? count : 1];
    unsigned short *buffer = stackBuffer;
    ONSInteger pathLength = (ONSInteger)length;
    if (length > 0x100) {
        buffer = (unsigned short *)malloc_type_malloc(2 * count, 0x1000040BDFB0063);
    } else {
        bzero(buffer, 2 * count);
    }
    [self getCharacters:buffer range:(ONSRange){0, length}];
    _NSTransmutePathSlashes(buffer, &pathLength);
    ONSInteger trimmed = pathLength;
    if (trimmed >= 2 && buffer[trimmed - 1] == '/') {
        if (trimmed == 2) {
            trimmed = 1;
        } else {
            ONSInteger counter = trimmed + 1;
            const unsigned short *cursor = buffer + trimmed - 2;
            trimmed = 1;
            for (;;) {
                if (*cursor-- != '/') {
                    trimmed = counter - 2;
                    break;
                }
                counter--;
                if (counter <= 3) {
                    break;
                }
            }
        }
    }
    ONSInteger start = _NSStartOfPathExtension(buffer, trimmed);
    id result = [(id)pathStoreClassSymbol pathStoreWithCharacters:buffer length:start > 0 ? start : trimmed];
    if (length >= 0x101) {
        free(buffer);
    }
    return result;
}

// Native -[NSString(NSPathUtilities) _stringByStandardizingPathUsingCache:], 0x1c0590dd0. Non-external. The
// first min(length, 1024) characters (-length is sent again when it is at most 1023, signed) are copied into
// a 1024-character stack buffer; __NSExpandTildeInPath and __NSStandardizePathUsingCache rewrite it, and the
// result is its NSPathStore2.
- (id)_stringByStandardizingPathUsingCache:(BOOL)useCache {
    unsigned short buffer[1024];
    ONSInteger length;
    if ((ONSInteger)[self length] > 0x3FF) {
        length = 0x400;
    } else {
        length = (ONSInteger)[self length];
    }
    [self getCharacters:buffer range:(ONSRange){0, (ONSUInteger)length}];
    _NSExpandTildeInPath(buffer, &length);
    _NSStandardizePathUsingCache(buffer, &length, useCache);
    return [(id)pathStoreClassSymbol pathStoreWithCharacters:buffer length:length];
}

// Native -[NSString(NSPathUtilities) stringByStandardizingPath], 0x1c0590e90. Non-external. A tail call of
// -_stringByStandardizingPathUsingCache:NO.
- (id)stringByStandardizingPath {
    return [self _stringByStandardizingPathUsingCache:NO];
}

// Native -[NSString(NSPathUtilities) _stringByResolvingSymlinksInPathUsingCache:], 0x1c0590e98. Non-external.
// As -_stringByStandardizingPathUsingCache:, with __NSResolveSymlinksInPathUsingCache between the tilde
// expansion and the standardization.
- (id)_stringByResolvingSymlinksInPathUsingCache:(BOOL)useCache {
    unsigned short buffer[1024];
    ONSInteger length;
    if ((ONSInteger)[self length] > 0x3FF) {
        length = 0x400;
    } else {
        length = (ONSInteger)[self length];
    }
    [self getCharacters:buffer range:(ONSRange){0, (ONSUInteger)length}];
    _NSExpandTildeInPath(buffer, &length);
    _NSResolveSymlinksInPathUsingCache(buffer, &length, useCache);
    _NSStandardizePathUsingCache(buffer, &length, useCache);
    return [(id)pathStoreClassSymbol pathStoreWithCharacters:buffer length:length];
}

// Native -[NSString(NSPathUtilities) fileSystemRepresentation], 0x1c05910a4. Non-external. The empty string
// (-isEqual: the constant empty string) and a CFStringGetMaximumSizeOfFileSystemRepresentation of -1 throw
// NSInvalidArgumentException; a failed malloc_type_malloc (type 0x100004077774924) throws NSMallocException
// with the size; a failed -__swiftFillFileSystemRepresentationWithPointer:maxLength: (bit 0) frees the buffer
// and throws NSInvalidArgumentException. The reasons are +[NSString stringWithFormat:] strings. On success
// the buffer is adopted by +[NSData dataWithBytesNoCopy:length:] (strlen + 1 bytes) and its -bytes is the
// result (a tail call).
- (const char *)fileSystemRepresentation {
    id name;
    id reason;
    if ([self isEqual:(id)OF_STR("")]) {
        reason = [ONSString stringWithFormat:(id)OF_STR(
                                                 "*** -[NSString fileSystemRepresentation]: Cannot form "
                                                 "file system representation of empty string"
                                             )];
        name = (id)ONSInvalidArgumentException;
    } else {
        OCFIndex size = OCFStringGetMaximumSizeOfFileSystemRepresentation((OCFStringRef)self);
        if (size == -1) {
            reason = [ONSString
                stringWithFormat:(id)OF_STR(
                                     "*** -[NSString fileSystemRepresentation]: unable to calculate size "
                                     "of file system representation buffer for string"
                                 )];
            name = (id)ONSInvalidArgumentException;
        } else {
            char *buffer = (char *)malloc_type_malloc(size, 0x100004077774924);
            if (buffer == NULL) {
                reason = [ONSString stringWithFormat:(id)OF_STR(
                                                         "*** -[NSString fileSystemRepresentation]: unable "
                                                         "to allocate memory for length (%ld)"
                                                     ),
                                                     size];
                name = (id)ONSMallocException;
            } else {
                if ([(id)self __swiftFillFileSystemRepresentationWithPointer:buffer maxLength:size] & 1) {
                    id data = [(id)pathDataClassSymbol dataWithBytesNoCopy:buffer length:strlen(buffer) + 1];
                    return (const char *)[data bytes];
                }
                free(buffer);
                reason = [ONSString stringWithFormat:(id)OF_STR(
                                                         "*** -[NSString fileSystemRepresentation]: Unable "
                                                         "to form file system representation for string"
                                                     )];
                name = (id)ONSInvalidArgumentException;
            }
        }
    }
    objc_exception_throw([(id)pathExceptionClassSymbol exceptionWithName:name reason:reason userInfo:nil]);
}

// Native -[NSString(NSPathUtilities) getFileSystemRepresentation:maxLength:], 0x1c05911dc. Non-external. The
// empty string (bit 0 of -isEqual: the constant empty string) answers NO; otherwise a tail call of
// -__swiftFillFileSystemRepresentationWithPointer:maxLength:.
- (BOOL)getFileSystemRepresentation:(char *)buffer maxLength:(ONSUInteger)maxLength {
    if ([self isEqual:(id)OF_STR("")] & 1) {
        return NO;
    }
    return [(id)self __swiftFillFileSystemRepresentationWithPointer:buffer maxLength:(ONSInteger)maxLength];
}

@end

// Native NSHomeDirectoryForUser, 0x1c0591f0c. External. The -path of CFCopyHomeDirectoryURLForUser's URL (nil
// when it answers NULL; the URL is released), then a tail call of -stringByStandardizingPath (a message to
// nil for a missing URL).
id ONSHomeDirectoryForUser(id userName) {
    id path;
    OCFURLRef url = OCFCopyHomeDirectoryURLForUser((OCFStringRef)userName);
    if (url != NULL) {
        path = [(id)url path];
        OCFRelease(url);
    } else {
        path = nil;
    }
    return [path stringByStandardizingPath];
}

// Native __isEqualCString, 0x1c0592180. Non-external.
static Boolean _isEqualCString(const void *first, const void *second) {
    if (first == second) {
        return true;
    }
    if (first == NULL || second == NULL) {
        return false;
    }
    return strcmp(first, second) == 0;
}

// Native __hashCString2, 0x1c05921c0. Non-external.
// The native frame clears the first 512 bytes of the conversion record.
static OCFHashCode _hashCString2(const void *value) {
    const char *bytes = value;
    StringConversionBuffer result;
    memset(&result, 0, 512);
    size_t length = strlen(bytes);
    if (_ONSConvertToASCIIorUnicode2(bytes, length, 4, YES, &result, NULL) && result.numChars != 0 &&
        (result.isASCII & 1) == 0) {
        OCFHashCode hash = OCFStringHashCharacters(result.unicode, (OCFIndex)result.numChars);
        if (result.shouldFreeChars == 1) {
            free(result.unicode);
        }
        return hash;
    }
    return OCFStringHashCString((const UInt8 *)bytes, (OCFIndex)strlen(bytes));
}

// Native String.standardizingPath, 0x1c0b63918, inlined FileManager dispatch at the end of the getter.
// The Swift-to-Objective-C boundary uses the restored NSString class and preserves the +1 releases.
// Foundation divergence: no such native function. Natively the Swift code converts the path with
// String._bridgeToObjectiveC() and messages the file manager itself; the Swift String to NSString bridge
// is not restored, so the conversion and the sends stay in this Objective-C helper until it is.
bool swiftStandardizingPathFileExists(const uint8_t *bytes, OCFIndex count) {
    id manager = objc_retain([(id)pathFileManagerClassSymbol defaultManager]);
    id path = [[ONSString alloc] initWithBytes:bytes length:(ONSUInteger)count encoding:4];
    bool exists = [manager fileExistsAtPath:path];
    objc_release(manager);
    objc_release(path);
    return exists;
}

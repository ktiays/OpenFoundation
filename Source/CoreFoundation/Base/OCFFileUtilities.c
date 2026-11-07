//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/OFPortHooks.h"
#include "CoreFoundation/Base/OCFFileUtilities.h"

#include "CoreFoundation/Date/OCFDate.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/URL/OCFURL.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// The definitions without their own identity line (_CFGetFileProperties and the path and directory helpers,
// 0x1c019cc60-0x1c019d9b8) are iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly); iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBundle.md (pending; the coordinator writes the reading).

// Native _CFCreateDirectory, 0x1c019c214. Non-external (was a private external).
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC, confidence: verified.
// iOS Device 24A446: unread. Reading: Local/Evidence/cf-core/iphone-simulator-corpus-delta13.json.
bool _CFCreateDirectory(const char *path) {
    int autofsDescriptor = __CFProphylacticAutofsAccess ? open("/dev/autofs_nowait", O_RDONLY) : -1;
    int result = mkdir(path, 0777);
    if (autofsDescriptor != -1) {
        close(autofsDescriptor);
    }
    return result == 0;
}

// Native _CFReadBytesFromFile, iOS Simulator 24A434, 0x1c019c36c, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external (was a private external). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta2.json. iOS Device 24A446: unread.
// The URL's file-system representation (resolved against its base, 1,026 bytes) or false. *bytes is cleared;
// with __CFProphylacticAutofsAccess, "/dev/autofs_nowait" is opened first (its result is not checked). The
// file is opened with extraOpenFlags and mode 0666. A failed fstat keeps its errno across the closes; a mode
// that is not a regular file (st_mode as a signed 16-bit value not below -0x7000) closes and sets errno 13
// (EACCES). A nonzero size reads min(maxLength, size) bytes (the whole size when maxLength is 0) into a typed
// allocation (0x65D2BE0B, event name "CFUtilities (file-bytes)"); a failed allocation, or a failed read
// (which frees it), answers false; the count read is not checked. A zero size allocates 4 bytes (0x1ACC47C3,
// not checked) and answers length 0. On success the autofs descriptor is closed before the file's.
bool _CFReadBytesFromFile(
    OCFAllocatorRef alloc,
    OCFURLRef url,
    void **bytes,
    OCFIndex *length,
    OCFIndex maxLength,
    int32_t extraOpenFlags
) {
    char path[0x402];
    if (!OCFURLGetFileSystemRepresentation(url, true, (uint8_t *)path, 0x402)) {
        return false;
    }
    *bytes = NULL;
    int autofsDescriptor;
    int fd;
    if (__CFProphylacticAutofsAccess) {
        autofsDescriptor = open("/dev/autofs_nowait", O_RDONLY);
        fd = open(path, extraOpenFlags, 0666);
        if (fd < 0) {
            if (autofsDescriptor != -1) {
                close(autofsDescriptor);
            }
            return false;
        }
    } else {
        fd = open(path, extraOpenFlags, 0666);
        if (fd < 0) {
            return false;
        }
        autofsDescriptor = -1;
    }
    struct stat statBuffer;
    memset(&statBuffer, 0, sizeof(statBuffer));
    if (fstat(fd, &statBuffer) < 0) {
        int savedError = errno;
        close(fd);
        if (autofsDescriptor != -1) {
            close(autofsDescriptor);
        }
        errno = savedError;
        return false;
    }
    if ((int16_t)statBuffer.st_mode >= -0x7000) {
        close(fd);
        if (autofsDescriptor != -1) {
            close(autofsDescriptor);
        }
        errno = 13;
        return false;
    }
    OCFIndex size = statBuffer.st_size;
    OCFIndex desiredLength;
    if (size == 0) {
        *bytes = OCFAllocatorAllocateTyped(alloc, 4, 0x1ACC47C3, 0);
        if (__OCFOASafe == 1) {
            __OCFSetLastAllocationEventName(*bytes, "CFUtilities (file-bytes)");
        }
        desiredLength = 0;
    } else {
        OCFIndex smaller = maxLength < size ? maxLength : size;
        desiredLength = maxLength == 0 ? size : smaller;
        *bytes = OCFAllocatorAllocateTyped(alloc, desiredLength, 0x65D2BE0B, 0);
        if (*bytes == NULL) {
            close(fd);
            if (autofsDescriptor != -1) {
                close(autofsDescriptor);
            }
            return false;
        }
        if (__OCFOASafe == 1) {
            __OCFSetLastAllocationEventName(*bytes, "CFUtilities (file-bytes)");
        }
        if (read(fd, *bytes, (size_t)desiredLength) < 0) {
            OCFAllocatorDeallocate(alloc, *bytes);
            close(fd);
            if (autofsDescriptor != -1) {
                close(autofsDescriptor);
            }
            return false;
        }
    }
    *length = desiredLength;
    if (autofsDescriptor != -1) {
        close(autofsDescriptor);
    }
    close(fd);
    return true;
}

// Native _CFCreateContentsOfDirectory, iOS Simulator 24A434, 0x1c019c778,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external (was a private external).
// iOS Device 24A446: unread.
// dirSpec (X2) is never read. A retained, nonempty matchingAbstractType is converted with the file-system
// encoding into 1,024 bytes and terminated at the used length; its dots after an optional leading one are
// counted. Without dirPath, the file-system representation of dirURL (1,024 bytes, resolved against its base)
// is used, or NULL answers. With __CFProphylacticAutofsAccess, "/dev/autofs_nowait" is held open around the
// listing. Entries other than "." and ".." are kept when no extension is requested, or when the name is at
// least as long as the extension, does not end in '.', and the text after the selected dot (the last dot
// without dots in the extension; otherwise the first dot, since the native search restarts at that dot and so
// always counts the required number of dots there; the dot itself is kept when the extension starts with '.')
// has the extension's length and bytes. Each entry is a URL relative to dirURL (created from dirPath, a
// directory, on first use and released at the end), a directory for d_type DT_DIR or, after stat of
// dirPath/name (strlcpy/strlcat into 1,024 bytes), for DT_UNKNOWN, DT_LNK and DT_WHT. A failing closedir
// releases the array and answers NULL. Name lengths are 32-bit as in the native code. Reading:
// Documentation/Reference/Research/CoreFoundationBundle.md (pending; the coordinator writes the reading).
OCFMutableArrayRef _CFCreateContentsOfDirectory(
    OCFAllocatorRef alloc, char *dirPath, void *dirSpec, OCFURLRef dirURL, OCFStringRef matchingAbstractType
) {
    (void)dirSpec;
    // CoreFoundation defect: the terminator is stored at the used length, which can be 1,024; natively that
    // byte lands in frame padding below the stack guard (buffer at SP+0x8DC, guard at SP+0xCE0). One spare
    // byte keeps the store inside the array; behavior is otherwise identical.
    char extension[1025];
    char pathBuffer[1024];
    char entryPath[1024];
    OCFIndex dirPathLength = dirPath != NULL ? (OCFIndex)strlen(dirPath) : 0;
    OCFStringRef extensionString = NULL;
    int32_t extensionDots = 0;
    Boolean extensionNotOwned = true;
    OCFIndex extensionLength = 0;
    if (matchingAbstractType != NULL) {
        extensionString = (OCFStringRef)OCFRetain(matchingAbstractType);
    }
    if (extensionString != NULL) {
        extensionNotOwned = false;
        extensionLength = OCFStringGetLength(extensionString);
        if (extensionLength >= 1) {
            OCFStringGetBytes(
                extensionString,
                (OCFRange){0, extensionLength},
                OCFStringFileSystemEncoding(),
                0,
                false,
                (UInt8 *)extension,
                1024,
                &extensionLength
            );
            extension[extensionLength] = 0;
            const char *dot = strchr(extension[0] == '.' ? extension + 1 : extension, '.');
            while (dot != NULL) {
                extensionDots++;
                dot = strchr(dot + 1, '.');
            }
        }
    }
    if (dirPath == NULL) {
        if (!OCFURLGetFileSystemRepresentation(dirURL, true, (UInt8 *)pathBuffer, 1024)) {
            if (!extensionNotOwned) {
                OCFRelease(extensionString);
            }
            return NULL;
        }
        dirPath = pathBuffer;
        dirPathLength = (OCFIndex)strlen(pathBuffer);
    }
    int autofsDescriptor = __CFProphylacticAutofsAccess ? open("/dev/autofs_nowait", O_RDONLY) : -1;
    DIR *directory = opendir(dirPath);
    if (directory == NULL) {
        if (!extensionNotOwned) {
            OCFRelease(extensionString);
        }
        if (autofsDescriptor != -1) {
            close(autofsDescriptor);
        }
        return NULL;
    }
    OCFMutableArrayRef files = OCFArrayCreateMutable(alloc, 0, &kOCFTypeArrayCallBacks);
    Boolean createdURL = false;
    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        const char *name = entry->d_name;
        uint32_t nameLength = (uint32_t)strlen(name);
        if (name[0] == '.' && (nameLength == 1 || (nameLength == 2 && name[1] == '.'))) {
            continue;
        }
        if (extensionLength > (OCFIndex)nameLength) {
            continue;
        }
        if (extensionLength >= 1) {
            if (name[(uint32_t)(nameLength - 1)] == '.') {
                continue;
            }
            const char *match;
            if (extensionDots != 0) {
                const char *dot = strchr(name, '.');
                if (dot == NULL) {
                    continue;
                }
                do {
                    int32_t count = 0;
                    const char *scan = dot;
                    while ((scan = strchr(scan, '.')) != NULL) {
                        count++;
                        if (count == extensionDots) {
                            break;
                        }
                    }
                    match = count == extensionDots ? dot : NULL;
                    const char *next = strchr(dot, '.');
                    if (next == NULL) {
                        break;
                    }
                    dot = next;
                } while (match == NULL);
                if (match == NULL) {
                    continue;
                }
            } else {
                match = strrchr(name, '.');
                if (match == NULL) {
                    continue;
                }
            }
            const char *candidate = extension[0] == '.' ? match : match + 1;
            if ((OCFIndex)strlen(candidate) != extensionLength) {
                continue;
            }
            if (strncmp(candidate, extension, extensionLength) != 0) {
                continue;
            }
        }
        if (dirURL == NULL) {
            createdURL = true;
            dirURL =
                OCFURLCreateFromFileSystemRepresentation(alloc, (const UInt8 *)dirPath, dirPathLength, true);
        }
        Boolean isDirectory;
        uint8_t rotated = (uint8_t)((entry->d_type >> 1) | (entry->d_type << 7));
        if (rotated > 7 || ((1 << rotated) & 0xA5) == 0) {
            isDirectory = false;
        } else if (entry->d_type == DT_DIR) {
            isDirectory = true;
        } else {
            struct stat status = {0};
            __builtin___strlcpy_chk(entryPath, dirPath, 1024, 1024);
            __builtin___strlcat_chk(entryPath, "/", 1024, 1024);
            __builtin___strlcat_chk(entryPath, name, 1024, 1024);
            isDirectory = stat(entryPath, &status) == 0 && (status.st_mode & S_IFMT) == S_IFDIR;
        }
        OCFURLRef url = OCFURLCreateFromFileSystemRepresentationRelativeToBase(
            alloc,
            (const UInt8 *)name,
            OF_PORT_ANSWER(direntNameLength, entry->d_namlen, entry),
            isDirectory,
            dirURL
        );
        OCFArrayAppendValue(files, url);
        OCFRelease(url);
    }
    Boolean urlNotCreated = !createdURL;
    int closeResult = closedir(directory);
    if (autofsDescriptor != -1) {
        close(autofsDescriptor);
    }
    if (closeResult != 0) {
        OCFRelease(files);
        if (!urlNotCreated) {
            OCFRelease(dirURL);
        }
        if (!extensionNotOwned) {
            OCFRelease(extensionString);
        }
        return NULL;
    }
    if (!extensionNotOwned) {
        OCFRelease(extensionString);
    }
    if (!urlNotCreated) {
        OCFRelease(dirURL);
    }
    return files;
}

// Native _CFWriteBytesToFile, 0x1c019c5f4. Hidden (was a private external).
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC, confidence: verified.
// iOS Device 24A446: unread. Reading: Local/Evidence/cf-core/iphone-simulator-corpus-delta13.json.
// The native path disregards fsync and close results (kept). A short write preserves errno across closes.
bool _CFWriteBytesToFile(OCFURLRef url, const void *bytes, size_t length) {
    UInt8 path[1026];
    struct stat status = {0};
    if (!OCFURLGetFileSystemRepresentation(url, true, path, sizeof(path))) {
        return false;
    }
    int autofs = __CFProphylacticAutofsAccess ? open("/dev/autofs_nowait", O_RDONLY) : -1;
    int fd = -1;
    if (stat((const char *)path, &status) == 0 || errno == ENOENT) {
        fd = open((const char *)path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }
    if (fd < 0) {
        if (autofs != -1) {
            close(autofs);
        }
        return false;
    }
    if (length != 0 && (size_t)write(fd, bytes, length) != length) {
        int savedError = errno;
        close(fd);
        if (autofs != -1) {
            close(autofs);
        }
        errno = savedError;
        return false;
    }
    fsync(fd);
    close(fd);
    if (autofs != -1) {
        close(autofs);
    }
    return true;
}

// Native _CFGetFileProperties, 0x1c019cc60. Non-external (was a private external). A failed file-system
// representation (1,024 bytes, resolved against the base) answers -1; with every out pointer NULL, 0 without
// a stat. A stat failure other than ENOENT answers errno; ENOENT reports a missing file (mode 0, size 0, no
// date, owner -1, no contents). The date is (mtime seconds - kCFAbsoluteTimeIntervalSince1970) plus the
// nanoseconds times 1e-9 (FMADD; the constant at 0x1c0373650). The contents of a directory come from
// _CFCreateContentsOfDirectory with the path buffer; its third argument (X2) is left unset natively and never
// read, NULL here.
SInt32 _CFGetFileProperties(
    OCFAllocatorRef allocator,
    OCFURLRef url,
    Boolean *exists,
    SInt32 *posixMode,
    SInt64 *size,
    OCFTypeRef *modificationTime,
    SInt32 *ownerID,
    OCFArrayRef *directoryContents
) {
    UInt8 buffer[1026];
    if (!OCFURLGetFileSystemRepresentation(url, true, buffer, 1024)) {
        return -1;
    }
    if (exists == NULL && posixMode == NULL && size == NULL && modificationTime == NULL && ownerID == NULL &&
        directoryContents == NULL) {
        return 0;
    }
    struct stat status;
    memset(&status, 0, sizeof(status));
    int statResult = stat((const char *)buffer, &status);
    Boolean isDirectory;
    Boolean fileExists;
    if (statResult == 0) {
        isDirectory = (status.st_mode & S_IFMT) == S_IFDIR;
        fileExists = true;
    } else {
        if (*__error() != ENOENT) {
            return *__error();
        }
        isDirectory = false;
        fileExists = false;
    }
    if (exists != NULL) {
        *exists = fileExists;
    }
    if (posixMode != NULL) {
        *posixMode = statResult == 0 ? (SInt32)status.st_mode : 0;
    }
    if (size != NULL) {
        *size = statResult == 0 ? status.st_size : 0;
    }
    if (modificationTime != NULL) {
        if (statResult == 0) {
            *modificationTime = OCFDateCreate(
                allocator,
                fma((double)status.st_mtimespec.tv_nsec,
                    1.0e-9,
                    (double)status.st_mtimespec.tv_sec - kOCFAbsoluteTimeIntervalSince1970)
            );
        } else {
            *modificationTime = NULL;
        }
    }
    if (ownerID != NULL) {
        *ownerID = statResult == 0 ? (SInt32)status.st_uid : -1;
    }
    if (directoryContents == NULL) {
        return 0;
    }
    if (isDirectory) {
        *directoryContents = _CFCreateContentsOfDirectory(allocator, (char *)buffer, NULL, NULL, NULL);
    } else {
        *directoryContents = NULL;
    }
    return 0;
}

// Native _CFGetSlash, iOS Simulator 24A434, 0x1c019cefc, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external (was a private external).
// iOS Device 24A446: 0x18066f0b8, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// identical instructions (MOV W0, #0x2F; RET).
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
uint16_t _CFGetSlash(void) {
    return '/';
}

// Native _CFGetSlashStr, 0x1c019cf04. Non-external (was a private external); the constant string "/"
// (0x2227adb00).
OCFStringRef _CFGetSlashStr(void) {
    return OF_STR("/");
}

// Native _CFAppendTrailingPathSlash2.slash, 0x1c0373658 (__TEXT,__const, 2F 00). Non-external.
static const UniChar _CFAppendTrailingPathSlash2Slash = '/';

// Native _CFAppendPathExtension2.dotChar, 0x1c037365a (__TEXT,__const, 2E 00). Non-external.
static const UniChar _CFAppendPathExtension2DotChar = '.';

static Boolean _hasDrive(OCFStringRef path);
static Boolean _hasNet(OCFStringRef path);

// Native _CFAppendTrailingPathSlash2, 0x1c019cf10. Non-external (was a private external). An empty path, "/",
// and a two-unit path that is a drive ("X:") or a network prefix ("\\") are left alone; every other path,
// including one of three or more units that already ends in '/', receives a '/' (the native code tests no
// last unit beyond length 1).
void _CFAppendTrailingPathSlash2(OCFMutableStringRef path) {
    OCFIndex length = OCFStringGetLength(path);
    if (length == 0) {
        return;
    }
    if (length == 2) {
        if (_hasDrive(path) || _hasNet(path)) {
            return;
        }
    } else if (length == 1) {
        if (OCFStringGetCharacterAtIndex(path, 0) == '/') {
            return;
        }
    }
    OCFStringAppendCharacters(path, &_CFAppendTrailingPathSlash2Slash, 1);
}

// Native _hasDrive, 0x1c019cf90. Non-external. Two or more units with ':' second and an ASCII letter first
// (bit 5 masked, then 'A'..'Z'); both units are read before the test.
static Boolean _hasDrive(OCFStringRef path) {
    if (OCFStringGetLength(path) < 2) {
        return false;
    }
    UniChar first = OCFStringGetCharacterAtIndex(path, 0);
    if (OCFStringGetCharacterAtIndex(path, 1) != ':') {
        return false;
    }
    return (uint32_t)((first & 0xFFFFFFDF) - 'A') < 26;
}

// Native _hasNet, 0x1c019cff8. Non-external. Two or more units starting with two backslashes; both units are
// read before the test.
static Boolean _hasNet(OCFStringRef path) {
    if (OCFStringGetLength(path) < 2) {
        return false;
    }
    UniChar first = OCFStringGetCharacterAtIndex(path, 0);
    UniChar second = OCFStringGetCharacterAtIndex(path, 1);
    return first == '\\' && second == '\\';
}

// Native _CFAppendPathComponent2, 0x1c019d060. Non-external (was a private external); appends the trailing
// slash, then the component (a tail call of CFStringAppend).
void _CFAppendPathComponent2(OCFMutableStringRef path, OCFStringRef component) {
    _CFAppendTrailingPathSlash2(path);
    OCFStringAppend(path, component);
}

// Native _CFAppendPathExtension2, 0x1c019d08c. Non-external (was a private external). Refuses a NULL path, an
// extension that starts with '/' or is a drive, and, after trailing slashes are deleted (down to length 1, or
// 3 after a drive), an empty path, "/", "~", a two-unit drive or network prefix, a drive followed by '/', and
// a path starting with '~' that has no '/' after its first unit. Otherwise appends '.' and the extension and
// answers true.
Boolean _CFAppendPathExtension2(OCFMutableStringRef path, OCFStringRef extension) {
    if (path == NULL) {
        return false;
    }
    if (OCFStringGetLength(extension) >= 1 && OCFStringGetCharacterAtIndex(extension, 0) == '/') {
        return false;
    }
    if (OCFStringGetLength(extension) > 1 && _hasDrive(extension)) {
        return false;
    }
    Boolean drive = OCFStringGetLength(path) >= 2 ? _hasDrive(path) : false;
    while (true) {
        OCFIndex length = OCFStringGetLength(path);
        if (drive ? length <= 3 : length < 2) {
            break;
        }
        if (OCFStringGetCharacterAtIndex(path, OCFStringGetLength(path) - 1) != '/') {
            break;
        }
        OCFStringDelete(path, (OCFRange){OCFStringGetLength(path) - 1, 1});
    }
    if (OCFStringGetLength(path) == 0) {
        return false;
    }
    UniChar first = OCFStringGetCharacterAtIndex(path, 0);
    OCFIndex length = OCFStringGetLength(path);
    if (length <= 1) {
        if (length == 0) {
            return false;
        }
        if (length == 1 && (first == '/' || first == '~')) {
            return false;
        }
    } else if (length == 2) {
        if (_hasDrive(path)) {
            return false;
        }
        if (_hasNet(path)) {
            return false;
        }
    } else if (length == 3) {
        if (OCFStringGetCharacterAtIndex(path, 2) == '/' && _hasDrive(path)) {
            return false;
        }
    }
    if (length - 1 >= 0 && first == '~') {
        if (!OCFStringFindWithOptions(path, OF_STR("/"), (OCFRange){1, length - 1}, 0, NULL)) {
            return false;
        }
    }
    OCFStringAppendCharacters(path, &_CFAppendPathExtension2DotChar, 1);
    OCFStringAppend(path, extension);
    return true;
}

// Native _CFCreateLastPathComponent, 0x1c019d27c. Non-external (was a private external). Below two units, or
// without a '/' (the scan covers every index from length - 1 down to 0) and without a drive prefix of a
// longer path, the path itself is retained and *slashIndex (when given) is -1. A '/' at index i gives the
// substring after it and i; a drive prefix gives the substring from 2 and -1.
OCFStringRef _CFCreateLastPathComponent(OCFAllocatorRef allocator, OCFStringRef path, OCFIndex *slashIndex) {
    OCFIndex length = OCFStringGetLength(path);
    if (length >= 2) {
        OCFIndex offset = 0;
        do {
            OCFIndex end = length + offset;
            OCFIndex index = end - 1;
            if (OCFStringGetCharacterAtIndex(path, index) == '/') {
                if (slashIndex != NULL) {
                    *slashIndex = index;
                }
                return OCFStringCreateWithSubstring(allocator, path, (OCFRange){end, -offset});
            }
            offset--;
        } while (length + offset + 1 >= 2);
        OCFIndex remaining = length - 2;
        if (remaining != 0 && _hasDrive(path)) {
            if (slashIndex != NULL) {
                *slashIndex = -1;
            }
            return OCFStringCreateWithSubstring(allocator, path, (OCFRange){2, remaining});
        }
    }
    if (slashIndex != NULL) {
        *slashIndex = -1;
    }
    return OCFRetain(path);
}

// Native _CFStartOfLastPathComponent, iOS Simulator 24A434, 0x1c019d374,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external (was a private
// external).
// iOS Device divergence: _CFStartOfLastPathComponent at 0x180698360 (UUID
// 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified) moves the no-slash drive test into
// _CFStartOfLastPathComponent.cold.1 (0x1807bd49c, 44 bytes; confidence: verified): it sets W8 to 1 when
// unichars[1] is not ':', otherwise to 0 for (unichars[0] & ~0x20) - 'A' <= 25 and to all ones (CSETM)
// for any other unit, then branches to the shared _OUTLINED_FUNCTION_0_19 (0x18065c694: AND W0, W8, #1;
// RET; confidence: verified); the caller
// returns 0 when bit 0 of W0 is set and 2 otherwise. The selected units equal the simulator's.
// Lengths below 2 give 0. The scan from index length - 2 down to 0 returns one past the last '/'
// (a trailing '/' at length - 1 is ignored). Without one, a length above 2 with "X:" in front gives 2;
// otherwise 0. X is an ASCII letter: the native test masks bit 5 and compares with 'A'..'Z', which
// selects the same units as the two ranges below.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
OCFIndex _CFStartOfLastPathComponent(const uint16_t *unichars, OCFIndex length) {
    if (length < 2) {
        return 0;
    }
    for (OCFIndex index = length - 2; index >= 0; --index) {
        if (unichars[index] == '/') {
            return index + 1;
        }
    }
    if (length != 2 && unichars[1] == ':' &&
        ((unichars[0] >= 'A' && unichars[0] <= 'Z') || (unichars[0] >= 'a' && unichars[0] <= 'z'))) {
        return 2;
    }
    return 0;
}

// Native _CFStartOfLastPathComponent2, 0x1c019d3dc. Non-external (was a private external). Below two units,
// 0; one past the last '/' before index length - 1; without one, 2 after a drive prefix of a path longer than
// two units, otherwise 0.
OCFIndex _CFStartOfLastPathComponent2(OCFStringRef path) {
    OCFIndex length = OCFStringGetLength(path);
    if (length < 2) {
        return 0;
    }
    for (OCFIndex index = length - 1; index != 0; index--) {
        if (OCFStringGetCharacterAtIndex(path, index - 1) == '/') {
            return index;
        }
    }
    if (length == 2) {
        return 0;
    }
    return _hasDrive(path) ? 2 : 0;
}

// Native _CFLengthAfterDeletingLastPathComponent, iOS Simulator 24A434, 0x1c019d458,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external (was a private
// external).
// iOS Device divergence: _CFLengthAfterDeletingLastPathComponent at 0x18069ab0c (UUID
// 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified) moves the no-slash drive test into
// _CFLengthAfterDeletingLastPathComponent.cold.1 (0x1807bd4c8, 44 bytes; confidence: verified), the same
// instructions as _CFStartOfLastPathComponent.cold.1 ending in a branch to _OUTLINED_FUNCTION_0_19
// (AND W0, W8, #1; confidence: verified); the caller returns 0 when bit 0 of W0 is set and 2 otherwise. The
// scan, the slash-at-2 drive test and their results agree. Lengths below 2 give 0. The last '/' before index
// length - 1 at index i gives i, except that i = 0 gives 1 (the root) and i = 2 after "X:" gives 3. Without
// one, a length above 2 with "X:" in front gives 2; otherwise 0. The drive test after a slash at index 2
// compares the two letter ranges separately; the no-slash test masks bit 5 as in _CFStartOfLastPathComponent.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
OCFIndex _CFLengthAfterDeletingLastPathComponent(const uint16_t *unichars, OCFIndex length) {
    if (length < 2) {
        return 0;
    }
    for (OCFIndex index = length - 2; index >= 0; --index) {
        if (unichars[index] == '/') {
            if (index == 0 || (index == 2 && unichars[1] == ':' &&
                               ((unichars[0] >= 'A' && unichars[0] <= 'Z') ||
                                (unichars[0] >= 'a' && unichars[0] <= 'z')))) {
                return index + 1;
            }
            return index;
        }
    }
    if (length != 2 && unichars[1] == ':' &&
        ((unichars[0] >= 'A' && unichars[0] <= 'Z') || (unichars[0] >= 'a' && unichars[0] <= 'z'))) {
        return 2;
    }
    return 0;
}

// Native _CFStartOfPathExtension2, 0x1c019d50c. Non-external (was a private external). The drive test runs
// first; the scan from index length - 1 down to 1 stops with 0 at a '/' and answers the index of a '.',
// except 0 for a '.' at index 2 after a drive prefix. Below two units, 0.
OCFIndex _CFStartOfPathExtension2(OCFStringRef path) {
    if (OCFStringGetLength(path) < 2) {
        return 0;
    }
    Boolean drive = _hasDrive(path);
    for (OCFIndex index = OCFStringGetLength(path) - 1; index != 0; index--) {
        UniChar character = OCFStringGetCharacterAtIndex(path, index);
        if (character == '/') {
            return 0;
        }
        if (character == '.') {
            return drive && index == 2 ? 0 : index;
        }
    }
    return 0;
}

// Native _CFStartOfPathExtension, 0x1c019d590. Non-external (was a private external). For each index i from
// length - 1 down to 1, a '/' at i - 1 answers 0 before the '.' test at i; a '.' at i answers i, except 0 for
// i = 2 after "X:" (bit 5 masked, then 'A'..'Z'). Below two units, 0.
OCFIndex _CFStartOfPathExtension(const UniChar *unichars, OCFIndex length) {
    if (length < 2) {
        return 0;
    }
    for (OCFIndex index = length - 1; index != 0; index--) {
        if (unichars[index - 1] == '/') {
            return 0;
        }
        if (unichars[index] == '.') {
            if (index == 2 && unichars[1] == ':' && (uint32_t)((unichars[0] & 0xFFFFFFDF) - 'A') < 26) {
                return 0;
            }
            return index;
        }
    }
    return 0;
}

// Native _CFLengthAfterDeletingPathExtension2, 0x1c019d604. Non-external (was a private external). A
// positive extension start, otherwise the length (a tail call of CFStringGetLength).
OCFIndex _CFLengthAfterDeletingPathExtension2(OCFStringRef path) {
    OCFIndex start = _CFStartOfPathExtension2(path);
    if (start > 0) {
        return start;
    }
    return OCFStringGetLength(path);
}

// Native _CFLengthAfterDeletingPathExtension, 0x1c019d63c. Non-external (was a private external). The
// extension start, or the length when it is 0.
OCFIndex _CFLengthAfterDeletingPathExtension(const UniChar *unichars, OCFIndex length) {
    OCFIndex start = _CFStartOfPathExtension(unichars, length);
    return start == 0 ? length : start;
}

// Native _CFIterateDirectory, 0x1c019d664. Non-external (was a private external). The directory's file-system
// representation (1,026 bytes) is opened; entries with a zero name length or inode, ".", ".." and names
// starting with "._" are skipped. Each name becomes a string (kCFAllocatorSystemDefault). With stuffToPrefix,
// its strings are converted with the file-system encoding into a 1,025-byte prefix buffer, each followed by
// '/' when it does not end in one; the running length adds CFStringGetBytes's converted-character count, and
// the slash test reads the used byte length when that exceeds the running length, as natively. With
// appendSlashForDirectories, a DT_DIR entry, or a DT_UNKNOWN one that stat ("%s/%s", 1,024 bytes) reports as
// a directory, appends its name and "/" to the prefix; otherwise, with a prefix array, the name alone.
// Without either, the name string is retained as the prefixed name. A prefixed name that fails to convert is
// replaced by the retained name; a name that fails to convert skips the entry. The handler receives the name,
// the prefixed name and d_type; both are released after it, and a false answer ends the iteration. The native
// function leaves closedir's result (or the failed conversion or opendir result) in X0; no caller reads it.
// CoreFoundation defect: when the prefixes fill the 1,025-byte buffer, the native code stores the slash and
// the terminator past it, into the directory path buffer that follows it in the frame (prefix at SP+0x4DD,
// path at SP+0x8DE). The prefix buffer has two spare bytes here, so the stores stay inside it and the
// directory path is not overwritten; behavior is otherwise identical.
void _CFIterateDirectory(
    OCFStringRef directoryPath,
    Boolean appendSlashForDirectories,
    OCFArrayRef stuffToPrefix,
    Boolean (^fileHandler)(OCFStringRef fileName, OCFStringRef fileNameWithPrefix, uint8_t fileType)
) {
    char buffer[1026];
    char prefix[1027];
    char statPath[1024];
    if (!OCFStringGetFileSystemRepresentation(directoryPath, buffer, 1026)) {
        return;
    }
    DIR *directory = opendir(buffer);
    if (directory == NULL) {
        return;
    }
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        uint16_t nameLength = OF_PORT_ANSWER(direntNameLength, entry->d_namlen, entry);
        if (nameLength == 0 || entry->d_ino == 0) {
            continue;
        }
        const char *name = entry->d_name;
        if (name[0] == '.') {
            if (nameLength == 1) {
                continue;
            }
            if (nameLength == 2 && name[1] == '.') {
                continue;
            }
            if (name[1] == '_') {
                continue;
            }
        }
        OCFStringRef fileName = OCFStringCreateWithFileSystemRepresentation(allocator, name);
        prefix[0] = 0;
        if (stuffToPrefix != NULL) {
            OCFIndex total = 0;
            if (OCFArrayGetCount(stuffToPrefix) >= 1) {
                OCFIndex index = 0;
                do {
                    OCFStringRef component = OCFArrayGetValueAtIndex(stuffToPrefix, index);
                    OCFIndex usedLength = 0;
                    OCFIndex componentLength = OCFStringGetLength(component);
                    OCFStringEncoding encoding = OCFStringFileSystemEncoding();
                    total += OCFStringGetBytes(
                        component,
                        (OCFRange){0, componentLength},
                        encoding,
                        0,
                        false,
                        (UInt8 *)prefix + total,
                        1025 - total,
                        &usedLength
                    );
                    if (total >= 1) {
                        if (total < usedLength) {
                            if (prefix[usedLength - 1] != '/') {
                                prefix[usedLength] = '/';
                                total = usedLength + 1;
                            }
                        } else if (prefix[total - 1] != '/') {
                            prefix[total] = '/';
                            total++;
                        }
                    }
                    index++;
                } while (index < OCFArrayGetCount(stuffToPrefix));
            }
            prefix[total] = 0;
        }
        Boolean isDirectory = false;
        if (appendSlashForDirectories) {
            if (entry->d_type == DT_DIR) {
                isDirectory = true;
            } else if (entry->d_type == DT_UNKNOWN) {
                struct stat status;
                memset(&status, 0, sizeof(status));
                int written = snprintf(statPath, 1024, "%s/%s", buffer, name);
                if ((uint32_t)(written - 1) <= 0x3FE && stat(statPath, &status) == 0 &&
                    (status.st_mode & S_IFMT) == S_IFDIR) {
                    isDirectory = true;
                }
            }
        }
        OCFStringRef fileNameWithPrefix;
        if (isDirectory || stuffToPrefix != NULL) {
            if (isDirectory) {
                __builtin___strlcat_chk(prefix, name, 1025, 1025);
                name = "/";
            }
            __builtin___strlcat_chk(prefix, name, 1025, 1025);
            fileNameWithPrefix = OCFStringCreateWithFileSystemRepresentation(allocator, prefix);
            if (fileName == NULL) {
                if (fileNameWithPrefix != NULL) {
                    OCFRelease(fileNameWithPrefix);
                }
                continue;
            }
            if (fileNameWithPrefix == NULL) {
                fileNameWithPrefix = OCFRetain(fileName);
            }
        } else {
            if (fileName == NULL) {
                continue;
            }
            fileNameWithPrefix = OCFRetain(fileName);
        }
        Boolean proceed = fileHandler(fileName, fileNameWithPrefix, entry->d_type);
        OCFRelease(fileName);
        OCFRelease(fileNameWithPrefix);
        if (!proceed) {
            break;
        }
    }
    closedir(directory);
}

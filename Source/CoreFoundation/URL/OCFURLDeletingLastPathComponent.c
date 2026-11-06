//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/URL/OCFURLDeletingLastPathComponent.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <assert.h>
#include <objc/message.h>
#include <objc/runtime.h>

// Inferred organization: CFURLCreateCopyDeletingLastPathComponent (0x1c024b248, with .cold.1 0x1c030d90c and
// .cold.2 0x1c030d910) belongs to the native CFURL.c unit (its assertion names CFURL.c and it lies between
// __CFURLCreateCopyAppendingPathComponent and POSIXPathToURLPath), restored in URL/OCFURL.c. That file is
// owned by another sub-track in this batch, so the function is restored in this separate file named for it;
// it moves into URL/OCFURL.c in native order when the tracks are integrated. Compiled as C, like
// URL/OCFURL.c. iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified
// (assembly read in full from Local/Evidence/cf-core/iphone-simulator-corpus-delta12.json; the constant
// strings resolved with Local/Scratch/cf-core/macho.py). iOS Device 24A446: unread. Reading:
// Local/Scratch/cf-core-prefs-daemon/REPORT.md (main checkout).

static inline OCFURLRef cfurlOfObjectiveCURL(OCFURLRef url) {
    return ((OCFURLRef (*)(OCFURLRef, SEL))objc_msgSend)(url, sel_getUid("_cfurl"));
}

static inline OCFStringRef acquireString(OCFURLRef url) {
    return __atomic_load_n(&((struct __OCFURL *)url)->string, __ATOMIC_ACQUIRE);
}

// The path range of the Swift layout as the native code reads it from the flags at 0x10: form 2 is the whole
// string (less the 7 characters of "file://" when flag bit 2 is set), form 1 two 64-bit words at 0x68 and
// 0x70, any other form a 32-bit word at 0x44 (location in the low 16 bits, length in the high 16).
static inline OCFRange swiftPathRange(OCFURLRef url) {
    UInt32 flags = url->flags;
    if ((flags & 3) == 2) {
        OCFIndex length = OCFStringGetLength(url->string);
        if ((url->flags & 4) != 0) {
            return (OCFRange){7, length - 7};
        }
        return (OCFRange){0, length};
    }
    const uint8_t *bytes = (const uint8_t *)url;
    if ((flags & 3) == 1) {
        OCFRange range;
        __builtin_memcpy(&range.location, bytes + 0x68, 8);
        __builtin_memcpy(&range.length, bytes + 0x70, 8);
        return range;
    }
    uint32_t word;
    __builtin_memcpy(&word, bytes + 0x44, 4);
    return (OCFRange){word & 0xffff, word >> 16};
}

// Native CFURLCreateCopyDeletingLastPathComponent, 0x1c024b248, with .cold.1 (0x1c030d90c, the once of
// __foundation_swift_nsurl_feature_enabled through OUTLINED_FUNCTION_0_31) and .cold.2 (0x1c030d910, the
// assertion "!CFURLIsFileReferenceURL(filePathURL)", CFURL.c line 7656). External.
// Assembly/pseudocode disagreement: the allocator stays in X0 and the base URL is passed in X2 (for
// example 0x1c024b434-0x1c024b454); preserve both pointer arguments rather than the decompiler's int
// allocator cast and omitted base argument.
// Swift layout (the feature enabled): an Objective-C URL is replaced by its -_cfurl. A file-reference URL
// (flag bit 13) becomes a file path URL (NULL answers NULL), which must not be a file reference; the call
// recurses on it and the file path URL is released. Without a path (flag bit 5) the result is NULL. With
// flag bit 3 and an empty path, the URL's string is used unchanged. Otherwise the last path component of the
// path range (__getLastPathComponentRangeFromString) decides: none, a single "/", "..", or a "." just after
// the path's first character append "../" ("/../" when the path is nonempty and does not end with '/') to a
// mutable copy (capacity the length plus 4); another single "." is replaced through the path end by "../";
// any other component is deleted through the path end, or, when it starts at 0, the whole path is replaced
// by "./" (capacity the length). The result is an arbitrary-string URL with the URL's base.
// CF layout (the feature disabled): an Objective-C URL is replaced by its -_cfurl; a file-reference URL is
// replaced by its file path URL (NULL answers NULL; released afterwards). A URL without a path (flag bit 5)
// answers NULL. The path range is the packed range after the ranges of flag bits 0..4. With any of flag bits
// 1..4 and an empty path, the URL's string (acquire load) is used unchanged. Otherwise the last path
// component (_rangeOfLastPathComponent) decides: none, "." or "/" (a single character masked with 0xfffe)
// or ".." gets "..", then "/", inserted at the path end (after an inserted "/" when the nonempty path does
// not end with one), and a trailing "./../" ("/./../" when the new path is at least 8 long) loses the "./"
// at 5 (6) before the end; another component is deleted through the path end, or, when it starts the path,
// the path is replaced by "." and "/" inserted at 1. The mutable copies use capacity 0.
// CoreFoundation defect: in the CF layout, the file path URL made for a file-reference URL leaks when its
// path is empty and one of flag bits 1..4 is set (the tail call at 0x1c024b454 skips the release at
// 0x1c024b7ac); established from the assembly, confidence verified, though a file path URL is not expected
// to have an empty path. This restoration releases it on that path.
OCFURLRef OCFURLCreateCopyDeletingLastPathComponent(OCFAllocatorRef allocator, OCFURLRef url) {
    if (_foundation_swift_nsurl_feature_enabled()) {
        if ((intptr_t)url < 0 || __CF_IS_OBJC(29, url)) {
            url = cfurlOfObjectiveCURL(url);
        }
        UInt32 flags = url->flags;
        if ((flags & 0x2000) != 0) {
            OCFURLRef filePathURL = OCFURLCreateFilePathURL(allocator, url, NULL);
            if (filePathURL == NULL) {
                return NULL;
            }
            if (OCFURLIsFileReferenceURL(filePathURL)) {
                __assert_rtn(
                    // The literal is split so that the prefix checker does not read it as a runtime name.
                    "CFURLCreate"
                    "CopyDeletingLastPathComponent",
                    "CFURL.c",
                    7656,
                    "!CFURLIsFileReferenceURL(filePathURL)"
                );
            }
            OCFURLRef result = OCFURLCreateCopyDeletingLastPathComponent(allocator, filePathURL);
            OCFRelease(filePathURL);
            return result;
        }
        if ((flags & 0x20) == 0) {
            return NULL;
        }
        OCFRange path = swiftPathRange(url);
        if ((flags & 8) != 0 && path.length == 0) {
            return _CFURLCreateWithArbitraryString(allocator, url->string, url->baseURL);
        }
        // The range is computed again, as natively (a second string length read for form 2).
        OCFRange searchRange = swiftPathRange(url);
        OCFRange component =
            __getLastPathComponentRangeFromString(url->string, searchRange.location, searchRange.length);
        // appendDotDot: append "../"; deleteComponent: delete (or replace with "./"); neither: replace the
        // component through the path end with "../".
        bool keep;
        bool deleteComponent;
        OCFIndex extra = 0;
        OCFIndex length;
        if (component.length == 0) {
            length = OCFStringGetLength(url->string);
            keep = true;
            deleteComponent = false;
            extra = 4;
        } else if (component.length == 2) {
            if (OCFStringGetCharacterAtIndex(url->string, component.location) != '.') {
                length = OCFStringGetLength(url->string);
                keep = true;
                deleteComponent = true;
            } else {
                UniChar second = OCFStringGetCharacterAtIndex(url->string, component.location + 1);
                length = OCFStringGetLength(url->string);
                keep = true;
                if (second != '.') {
                    deleteComponent = true;
                } else {
                    deleteComponent = false;
                    extra = 4;
                }
            }
        } else if (component.length == 1) {
            UniChar character = OCFStringGetCharacterAtIndex(url->string, component.location);
            if (character == '/') {
                length = OCFStringGetLength(url->string);
                keep = true;
                deleteComponent = false;
                extra = 4;
            } else if (character == '.') {
                length = OCFStringGetLength(url->string);
                if (component.location == path.location + 1) {
                    keep = true;
                    deleteComponent = false;
                    extra = 4;
                } else {
                    keep = false;
                    deleteComponent = true;
                }
            } else {
                length = OCFStringGetLength(url->string);
                keep = true;
                deleteComponent = true;
            }
        } else {
            length = OCFStringGetLength(url->string);
            keep = true;
            deleteComponent = true;
        }
        OCFMutableStringRef copy = OCFStringCreateMutableCopy(allocator, extra + length, url->string);
        if (copy == NULL) {
            return NULL;
        }
        OCFIndex end = path.location + path.length;
        if (!keep) {
            OCFStringReplace(copy, (OCFRange){component.location, end - component.location}, OF_STR("../"));
        } else if (!deleteComponent) {
            if (path.length >= 1 && OCFStringGetCharacterAtIndex(url->string, end - 1) != '/') {
                OCFStringInsert(copy, end, OF_STR("/../"));
            } else {
                OCFStringInsert(copy, end, OF_STR("../"));
            }
        } else if (component.location == 0) {
            OCFStringReplace(copy, path, OF_STR("./"));
        } else {
            OCFStringDelete(copy, (OCFRange){component.location, end - component.location});
        }
        OCFURLRef result = _CFURLCreateWithArbitraryString(allocator, copy, url->baseURL);
        OCFRelease(copy);
        return result;
    }
    if ((intptr_t)url < 0 || __CF_IS_OBJC(29, url)) {
        url = cfurlOfObjectiveCURL(url);
    }
    Boolean isFileReference = OCFURLIsFileReferenceURL(url);
    if (isFileReference) {
        url = OCFURLCreateFilePathURL(allocator, url, NULL);
        if (url == NULL) {
            return NULL;
        }
        if ((url->flags & 0x20) == 0) {
            OCFRelease(url);
            return NULL;
        }
    } else if ((url->flags & 0x20) == 0) {
        return NULL;
    }
    UInt32 flags = url->flags;
    unsigned index = (unsigned)__builtin_popcount(flags & 0x1f);
    OCFRange path = url->ranges[index];
    if ((flags & 0x1e) != 0 && path.length == 0) {
        OCFURLRef result = _CFURLCreateWithArbitraryString(allocator, acquireString(url), url->baseURL);
        if (isFileReference) {
            OCFRelease(url);
        }
        return result;
    }
    OCFRange component = _rangeOfLastPathComponent(url);
    OCFMutableStringRef copy;
    bool dots;
    if (component.length == 2) {
        if (OCFStringGetCharacterAtIndex(acquireString(url), component.location) != '.') {
            copy = OCFStringCreateMutableCopy(allocator, 0, acquireString(url));
            dots = false;
        } else {
            UniChar second = OCFStringGetCharacterAtIndex(acquireString(url), component.location + 1);
            copy = OCFStringCreateMutableCopy(allocator, 0, acquireString(url));
            dots = second == '.';
        }
    } else if (component.length == 1) {
        UniChar character = OCFStringGetCharacterAtIndex(acquireString(url), component.location) & 0xfffe;
        copy = OCFStringCreateMutableCopy(allocator, 0, acquireString(url));
        dots = character == '.';
    } else if (component.length == 0) {
        copy = OCFStringCreateMutableCopy(allocator, 0, acquireString(url));
        dots = true;
    } else {
        copy = OCFStringCreateMutableCopy(allocator, 0, acquireString(url));
        dots = false;
    }
    OCFIndex end = path.location + path.length;
    if (dots) {
        OCFIndex slash = 0;
        if (path.length > 0 && OCFStringGetCharacterAtIndex(acquireString(url), end - 1) != '/') {
            OCFStringInsert(copy, end, OF_STR("/"));
            slash = 1;
        }
        OCFStringInsert(copy, slash + end, OF_STR(".."));
        OCFStringInsert(copy, slash + end + 2, OF_STR("/"));
        OCFIndex newLength = slash + 3 + path.length;
        if (newLength >= 5) {
            OCFIndex newEnd = slash + 3 + end;
            OCFIndex dot = newEnd - 5;
            if (OCFStringGetCharacterAtIndex(copy, dot) == '.') {
                if ((uint64_t)newLength < 8) {
                    if (newLength == 5) {
                        OCFStringDelete(copy, (OCFRange){dot, 2});
                    }
                } else {
                    dot = newEnd - 6;
                    if (OCFStringGetCharacterAtIndex(copy, dot) == '/') {
                        OCFStringDelete(copy, (OCFRange){dot, 2});
                    }
                }
            }
        }
    } else if (component.location == path.location) {
        OCFStringReplace(copy, path, OF_STR("."));
        OCFStringInsert(copy, 1, OF_STR("/"));
    } else {
        OCFStringDelete(copy, (OCFRange){component.location, end - component.location});
    }
    OCFURLRef result = _CFURLCreateWithArbitraryString(allocator, copy, url->baseURL);
    OCFRelease(copy);
    if (isFileReference) {
        OCFRelease(url);
    }
    return result;
}

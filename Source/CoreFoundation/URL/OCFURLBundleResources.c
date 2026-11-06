//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/URL/OCFURLBundleResources.h"

#include "CoreFoundation/PlugIn/OCFBundle.h"

#include <errno.h>
#include <fcntl.h>
#include <os/log.h>
#include <sys/param.h>
#include <unistd.h>

// Inferred organization: _CFURLCreateResolvedDirectoryWithString (0x1c016a23c) follows
// _CFBundleResourceLogger (0x1c016a128) and its block (0x1c016a158) in the bundle-resource code of the iOS
// Simulator __text section, apart from the CFURL unit; it is kept in its own translation unit until that unit
// is restored. Its .cold.1 fragment (0x1c0306478, the logger's dispatch_once through OUTLINED_FUNCTION_4) and
// .cold.2 fragment (0x1c030648c, the error log through OUTLINED_FUNCTION_8) belong to it. iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446: unread.
// Reading: Documentation/Reference/Research/CoreFoundationURL.md (pending; the coordinator writes the
// reading).
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_4 (0x1c016f86c).
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_8 (0x1c016f8ac).

// Native _CFURLCreateResolvedDirectoryWithString, 0x1c016a23c. Hidden (was private external).
// The absolute URL of URLString against baseURL gives a POSIX path (both intermediate URLs released) whose
// file system representation (1,024 bytes; NULL when it does not fit) is opened read-only; F_GETPATH (50)
// resolves it into a second 1,024-byte buffer, which becomes a directory URL with the given allocator. A
// failed open answers NULL; a failed F_GETPATH logs "Unable to resolve directory (%d)" with errno at the
// error level of _CFBundleResourceLogger (inlined natively: its once check and the dispatch_once of .cold.1)
// before closing and answering NULL.
OCFURLRef _CFURLCreateResolvedDirectoryWithString(
    OCFAllocatorRef allocator, OCFStringRef URLString, OCFURLRef baseURL
) {
    char resolved[1024];
    char path[1024];
    OCFURLRef url = OCFURLCreateWithString(allocator, URLString, baseURL);
    OCFURLRef absolute = OCFURLCopyAbsoluteURL(url);
    OCFRelease(url);
    OCFStringRef fileSystemPath = OCFURLCopyFileSystemPath(absolute, kOCFURLPOSIXPathStyle);
    OCFRelease(absolute);
    Boolean converted = OCFStringGetFileSystemRepresentation(fileSystemPath, path, 1024);
    OCFRelease(fileSystemPath);
    if (!converted) {
        return NULL;
    }
    int descriptor = open(path, O_RDONLY);
    if (descriptor < 1) {
        return NULL;
    }
    if (fcntl(descriptor, F_GETPATH, resolved) == -1) {
        os_log_error(_CFBundleResourceLogger(), "Unable to resolve directory (%d)", errno);
        close(descriptor);
        return NULL;
    }
    close(descriptor);
    OCFStringRef resolvedPath = OCFStringCreateWithFileSystemRepresentation(allocator, resolved);
    OCFURLRef result = OCFURLCreateWithFileSystemPath(allocator, resolvedPath, kOCFURLPOSIXPathStyle, true);
    OCFRelease(resolvedPath);
    return result;
}

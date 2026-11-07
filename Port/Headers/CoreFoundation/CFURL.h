//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFError.h>
#include <CoreFoundation/CFString.h>

CF_EXTERN_C_BEGIN

typedef CF_ENUM(CFIndex, CFURLPathStyle) {
    kCFURLPOSIXPathStyle = 0,
    kCFURLHFSPathStyle = 1,
    kCFURLWindowsPathStyle = 2
};

typedef const struct CF_BRIDGED_TYPE(NSURL) __CFURL *CFURLRef;

CF_EXPORT CFTypeID CFURLGetTypeID(void);
CF_EXPORT CFURLRef CFURLCreateWithBytes(
    CFAllocatorRef allocator,
    const UInt8 *URLBytes,
    CFIndex length,
    CFStringEncoding encoding,
    CFURLRef baseURL
);
CF_EXPORT CFURLRef CFURLCreateWithString(CFAllocatorRef allocator, CFStringRef URLString, CFURLRef baseURL);
CF_EXPORT CFURLRef CFURLCreateWithFileSystemPath(
    CFAllocatorRef allocator,
    CFStringRef filePath,
    CFURLPathStyle pathStyle,
    Boolean isDirectory
);
CF_EXPORT CFURLRef CFURLCreateFromFileSystemRepresentation(
    CFAllocatorRef allocator,
    const UInt8 *buffer,
    CFIndex bufLen,
    Boolean isDirectory
);
CF_EXPORT CFURLRef CFURLCreateWithFileSystemPathRelativeToBase(
    CFAllocatorRef allocator,
    CFStringRef filePath,
    CFURLPathStyle pathStyle,
    Boolean isDirectory,
    CFURLRef baseURL
);
CF_EXPORT CFURLRef CFURLCreateFromFileSystemRepresentationRelativeToBase(
    CFAllocatorRef allocator,
    const UInt8 *buffer,
    CFIndex bufLen,
    Boolean isDirectory,
    CFURLRef baseURL
);
CF_EXPORT Boolean CFURLGetFileSystemRepresentation(
    CFURLRef url,
    Boolean resolveAgainstBase,
    UInt8 *buffer,
    CFIndex maxBufLen
);
CF_EXPORT CFURLRef CFURLCopyAbsoluteURL(CFURLRef relativeURL);
CF_EXPORT CFStringRef CFURLGetString(CFURLRef anURL);
CF_EXPORT CFURLRef CFURLGetBaseURL(CFURLRef anURL);
CF_EXPORT CFStringRef CFURLCopyScheme(CFURLRef anURL);
CF_EXPORT CFStringRef CFURLCopyNetLocation(CFURLRef anURL);
CF_EXPORT CFStringRef CFURLCopyPath(CFURLRef anURL);
CF_EXPORT CFStringRef CFURLCopyFileSystemPath(CFURLRef anURL, CFURLPathStyle pathStyle);
CF_EXPORT Boolean CFURLHasDirectoryPath(CFURLRef anURL);
CF_EXPORT CFStringRef CFURLCopyResourceSpecifier(CFURLRef anURL);
CF_EXPORT CFStringRef CFURLCopyParameterString(CFURLRef anURL, CFStringRef charactersToLeaveEscaped);
CF_EXPORT CFStringRef CFURLCopyQueryString(CFURLRef anURL, CFStringRef charactersToLeaveEscaped);
CF_EXPORT CFStringRef CFURLCopyFragment(CFURLRef anURL, CFStringRef charactersToLeaveEscaped);
CF_EXPORT CFStringRef CFURLCopyLastPathComponent(CFURLRef url);
CF_EXPORT CFURLRef CFURLCreateCopyAppendingPathComponent(
    CFAllocatorRef allocator,
    CFURLRef url,
    CFStringRef pathComponent,
    Boolean isDirectory
);
CF_EXPORT CFURLRef CFURLCreateCopyDeletingLastPathComponent(CFAllocatorRef allocator, CFURLRef url);
CF_EXPORT CFStringRef CFURLCreateStringByReplacingPercentEscapes(
    CFAllocatorRef allocator,
    CFStringRef originalString,
    CFStringRef charactersToLeaveEscaped
);
CF_EXPORT CFURLRef CFURLCreateFilePathURL(CFAllocatorRef allocator, CFURLRef url, CFErrorRef *error);

CF_EXTERN_C_END

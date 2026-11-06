//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/URL/OCFURLKeys.h"

#include "CoreFoundation/String/OCFString.h"

// Inferred organization: the CFURL resource-property keys are one run of eight-byte pointers in the iOS
// Simulator __DATA_CONST,__const section (kCFURLNameKey at 0x2227a93e8 through the private volume keys past
// 0x2227a9a58), each pointing at its constant string in __DATA_CONST,__cfstring; the run is the URL keys
// table, kept in its own translation unit. Only the seven keys the restored URL functions read are defined
// here, in native address order; the other keys of the table are not restored on this branch. The native
// source path is not in the evidence. iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified (each pointer and its constant string, flags 0x7c8, read with
// Local/Scratch/cf-core/macho.py; the strings are the Foundation names, NSURL... and _NSURL...). iOS Device
// 24A446: unread. Reading: not yet written; the evidence is the static data at the addresses below and
// Local/Evidence/cf-core/iphone-simulator-corpus-delta7.json ("data" of _CFURLCopyFileReferenceURLPath).

// Native kCFURLNameKey, 0x2227a93e8 (its constant string at 0x2227ba740). External.
static const OCFConstantString urlNameKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSURLNameKey", 12
};
const OCFStringRef kOCFURLNameKey = (OCFStringRef)&urlNameKeyString;

// Native kCFURLIsDirectoryKey, 0x2227a9400 (its constant string at 0x2227ba7a0). External.
static const OCFConstantString urlIsDirectoryKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSURLIsDirectoryKey", 19
};
const OCFStringRef kOCFURLIsDirectoryKey = (OCFStringRef)&urlIsDirectoryKeyString;

// Native kCFURLParentDirectoryURLKey, 0x2227a9468 (its constant string at 0x2227ba940). External.
static const OCFConstantString urlParentDirectoryURLKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSURLParentDirectoryURLKey", 26
};
const OCFStringRef kOCFURLParentDirectoryURLKey = (OCFStringRef)&urlParentDirectoryURLKeyString;

// Native _kCFURLVolumeIDKey, 0x2227a98b0 (its constant string at 0x2227bba60). External.
static const OCFConstantString urlVolumeIDKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "_NSURLVolumeIDKey", 17
};
const OCFStringRef _kOCFURLVolumeIDKey = (OCFStringRef)&urlVolumeIDKeyString;

// Native _kCFURLFileIDKey, 0x2227a98c0 (its constant string at 0x2227bbaa0). External.
static const OCFConstantString urlFileIDKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "_NSURLFileIDKey", 15
};
const OCFStringRef _kOCFURLFileIDKey = (OCFStringRef)&urlFileIDKeyString;

// Native _kCFURLParentDirectoryIDKey, 0x2227a98c8 (its constant string at 0x2227bbac0). External.
static const OCFConstantString urlParentDirectoryIDKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "_NSURLParentDirectoryIDKey", 26
};
const OCFStringRef _kOCFURLParentDirectoryIDKey = (OCFStringRef)&urlParentDirectoryIDKeyString;

// Native _kCFURLVolumeIsHFSStandardKey, 0x2227a9a58 (its constant string at 0x2227bc100). External.
static const OCFConstantString urlVolumeIsHFSStandardKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "_NSURLVolumeIsHFSStandardKey", 28
};
const OCFStringRef _kOCFURLVolumeIsHFSStandardKey = (OCFStringRef)&urlVolumeIsHFSStandardKeyString;

//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

// The CFURL resource-property keys that _CFURLCopyFileReferenceURLPath and CFURLCreateFileReferenceURL read
// (SDK CFURL.h names spelled OCF; the underscore keys are exported without an SDK declaration), defined in
// URL/OCFURLKeys.c in native address order (0x2227a93e8-0x2227a9a58, __DATA_CONST,__const). External.
OF_EXPORT const OCFStringRef kOCFURLNameKey;
OF_EXPORT const OCFStringRef kOCFURLIsDirectoryKey;
OF_EXPORT const OCFStringRef kOCFURLParentDirectoryURLKey;
OF_EXPORT const OCFStringRef _kOCFURLVolumeIDKey;
OF_EXPORT const OCFStringRef _kOCFURLFileIDKey;
OF_EXPORT const OCFStringRef _kOCFURLParentDirectoryIDKey;
OF_EXPORT const OCFStringRef _kOCFURLVolumeIsHFSStandardKey;

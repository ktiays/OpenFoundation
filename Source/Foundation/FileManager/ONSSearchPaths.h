//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <sysdir.h>

#include <OpenCoreFoundation/OCFBuild.h>
#import <objc/NSObjCRuntime.h>

// Native NSSearchPathForDirectoriesInDomains, 0x1c05920b8. External.
OF_EXPORT id ONSSearchPathForDirectoriesInDomains(NSUInteger directory, NSUInteger domains, BOOL expandTilde);

// Objective-C message boundary for the Swift search-path specialization. Returns an owned object pointer.
OF_EXTERN const void *__attribute__((swiftcall)) copySearchPathFileSystemString(const char *path);

// libsystem directory enumeration entry called by the native Swift specialization.
OF_EXTERN unsigned int
sysdir_start_search_path_enumeration_private(unsigned int directory, unsigned int domains);

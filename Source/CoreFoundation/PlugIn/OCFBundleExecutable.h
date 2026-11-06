//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/PlugIn/OCFBundle.h"
#include "CoreFoundation/PlugIn/OCFBundleInfoPlist.h"
#include "CoreFoundation/String/OCFString.h"

// The CFBundle_Executable.c unit (iOS Simulator 0x1c018c870-0x1c018d508; the source path is the one its
// public functions log).
OF_EXPORT OCFURLRef OCFBundleCopyExecutableURL(OCFBundleRef bundle);
OF_EXPORT OCFURLRef OCFBundleCopyAuxiliaryExecutableURL(OCFBundleRef bundle, OCFStringRef executableName);
OF_EXPORT OCFURLRef _OCFBundleCopyBundleURLForExecutableURL(OCFURLRef url);
OF_EXTERN OCFURLRef _CFBundleCopyResolvedURLForExecutableURL(OCFURLRef url);

// The Foundation class the Swift bundle path messages (CoreFoundation/CFSwiftBundle feature).
#ifdef __OBJC__
@class _ONSSwiftBundle;
#endif

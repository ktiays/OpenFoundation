//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/PlugIn/OCFBundle.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/URL/OCFURL.h"

#include <mach/machine.h>

// The bundle loader unit (dyld and dlfcn queries, iOS Simulator 0x1c01a1960-0x1c01a41ec). Hidden (were
// private externals; original names are preserved).
// The cputype of NXGetLocalArchInfo's answer (LDR W0, [X0,#8]), or 0 when it answers NULL.
OF_EXTERN cpu_type_t _CFBundleCurrentArchitecture(void);
OF_EXTERN OCFArrayRef _CFBundleDYLDCopyLoadedImagePathsForHint(OCFStringRef hint);
// NULL when the dyld image count equals the count cached by the previous call.
OF_EXTERN OCFArrayRef _CFBundleDYLDCopyLoadedImagePathsIfChanged(void);
// Called with the bundle's loading lock (+88) held; answers the loaded flag (+52).
OF_EXTERN Boolean _CFBundleDlfcnCheckLoaded(OCFBundleRef bundle);
// Each takes the loading lock (+88) itself; an error is stored only for a false result.
OF_EXTERN Boolean _CFBundleDlfcnPreflight(OCFBundleRef bundle, OCFErrorRef *error);
OF_EXTERN Boolean _CFBundleDlfcnLoadBundle(OCFBundleRef bundle, Boolean forceGlobal, OCFErrorRef *error);
OF_EXTERN Boolean _CFBundleDlfcnLoadFramework(OCFBundleRef bundle, OCFErrorRef *error);
// Called with the bundle's loading lock (+88) held; drops and retakes it around dlclose.
OF_EXTERN void _CFBundleDlfcnUnload(OCFBundleRef bundle);
OF_EXTERN OCFStringRef _CFBundleCopyLoadedImagePathForPointer(const void *pointer);
// The variants of an image file name with each colon-separated DYLD_IMAGE_SUFFIX entry removed before its
// extension, followed by the name itself.
OF_EXTERN OCFMutableArrayRef
_CFBundleDYLDCreateLoadedImagePathVariants(const char *suffix, OCFStringRef name);

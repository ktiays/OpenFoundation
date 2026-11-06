//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/PlugIn/OCFBundle.h"

// The CFBundle_Main.c unit (inferred from the source path that CFBundleGetMainBundle logs).
OF_EXPORT OCFBundleRef OCFBundleGetMainBundle(void);

// Was a private external (hidden; native names). __CFBundleMainID is also read by CFLog
// (Base/OCFUtilities.c).
OF_EXTERN const char *__CFBundleMainID;
OF_EXTERN const char *__CFBundleShortVersionString;

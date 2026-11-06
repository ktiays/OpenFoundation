//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <xpc/xpc.h>

// The CF/XPC object bridge the preference messages use (Preferences/OCFXPCBridge.c). Exported natively,
// absent from the public SDK headers; signatures from the native register use. Both answer +1 objects (or
// NULL).
OF_EXPORT xpc_object_t _OCFXPCCreateXPCObjectFromCFObject(OCFTypeRef object);

OF_EXPORT OCFTypeRef _OCFXPCCreateCFObjectFromXPCObject(xpc_object_t object);

//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Runtime/OCFRuntime.h"

#include <stdint.h>

// The CFKeyedArchiverUID object, iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified: the runtime header, then the 32-bit value at offset 16 (_CFKeyedArchiverUIDCreate asks for 8
// extra bytes and stores W1 there; every reader loads 32 bits). The field name is a local descriptive
// spelling.
typedef const struct __OCFKeyedArchiverUID *OCFKeyedArchiverUIDRef;

struct __OCFKeyedArchiverUID {
    OCFRuntimeBase base;
    uint32_t value;
};

_Static_assert(
    sizeof(struct __OCFKeyedArchiverUID) == 24, "The native CFKeyedArchiverUID object is 24 bytes."
);

// Private exported functions (no SDK declaration; signatures read from the native code and its call sites).
OF_EXPORT OCFTypeID _OCFKeyedArchiverUIDGetTypeID(void);
OF_EXPORT OCFKeyedArchiverUIDRef _OCFKeyedArchiverUIDCreate(OCFAllocatorRef allocator, uint32_t value);
OF_EXPORT uint32_t _OCFKeyedArchiverUIDGetValue(OCFKeyedArchiverUIDRef uid);

// The builtin class record of type 41.
OF_EXTERN const OCFRuntimeClass __CFKeyedArchiverUIDClass;

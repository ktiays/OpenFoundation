//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Locale/OCFLocale.h"
#include "CoreFoundation/Locale/OCFLocaleKeys.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"

#include <stdint.h>

// CFNumberFormatter.h (iOS Simulator SDK interface), spelled OCF. Locale/OCFNumberFormatter.c restores the
// functions of the CFNumberFormatter unit that the scope reaches; the property keys are declared by
// Locale/OCFLocaleKeys.h.
typedef struct __OCFNumberFormatter *OCFNumberFormatterRef;

OF_EXPORT OCFNumberFormatterRef
OCFNumberFormatterCreate(OCFAllocatorRef allocator, OCFLocaleRef locale, OCFIndex style);
OF_EXPORT OCFLocaleRef OCFNumberFormatterGetLocale(OCFNumberFormatterRef formatter);
OF_EXPORT OCFStringRef OCFNumberFormatterGetFormat(OCFNumberFormatterRef formatter);
OF_EXPORT void OCFNumberFormatterSetFormat(OCFNumberFormatterRef formatter, OCFStringRef formatString);
OF_EXPORT OCFTypeRef OCFNumberFormatterCopyProperty(OCFNumberFormatterRef formatter, OCFStringRef key);
OF_EXPORT OCFStringRef OCFNumberFormatterCreateStringWithNumber(
    OCFAllocatorRef allocator, OCFNumberFormatterRef formatter, OCFNumberRef number
);
OF_EXPORT OCFStringRef OCFNumberFormatterCreateStringWithValue(
    OCFAllocatorRef allocator, OCFNumberFormatterRef formatter, OCFNumberType numberType, const void *valuePtr
);
OF_EXPORT OCFNumberRef OCFNumberFormatterCreateNumberFromString(
    OCFAllocatorRef allocator,
    OCFNumberFormatterRef formatter,
    OCFStringRef string,
    OCFRange *rangep,
    OCFOptionFlags options
);
OF_EXPORT Boolean OCFNumberFormatterGetValueFromString(
    OCFNumberFormatterRef formatter,
    OCFStringRef string,
    OCFRange *rangep,
    OCFNumberType numberType,
    void *valuePtr
);
OF_EXPORT void
OCFNumberFormatterSetProperty(OCFNumberFormatterRef formatter, OCFStringRef key, OCFTypeRef value);

// Native CFNumberFormatterCreateCopyWithICUFormat, 0x1c01ae128 (was a private external; hidden, native
// name).
OF_EXTERN OCFNumberFormatterRef
CFNumberFormatterCreateCopyWithICUFormat(OCFAllocatorRef allocator, OCFNumberFormatterRef formatter);

// Native _CFNumberFormatterSetPropertyNoRefresh (0x1c01aff3c) and _CFNumberFormatterSetInt32PropertyNoRefresh
// (0x1c01b0940), both were private externals.
OF_EXTERN void
_CFNumberFormatterSetPropertyNoRefresh(OCFNumberFormatterRef formatter, OCFStringRef key, OCFTypeRef value);
OF_EXTERN void
_CFNumberFormatterSetInt32PropertyNoRefresh(OCFNumberFormatterRef formatter, OCFStringRef key, int32_t value);

// Native __CFNumberFormatterClass, 0x2227a2620 (__DATA_CONST,__const); was a private external. Type 57.
OF_EXTERN const OCFRuntimeClass __CFNumberFormatterClass;

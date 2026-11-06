//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Locale/OCFListFormatter.h"

#include "CoreFoundation/String/OCFString.h"

#include <assert.h>

// The CFListFormatter unit, CFListFormatter.c (the source path its assertions name), iOS Simulator
// 0x1c02a7724-0x1c02a7b08; only the class callbacks and the class record are in the scope
// (_CFListFormatterCreate and the joining functions are not restored on this branch). iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json. The instance's only field read here is the object
// at offset 0x10 (released by the finalizer).

// Native __CFListFormatterDeallocate, 0x1c02a772c. Non-external. A NULL object is
// __CFListFormatterDeallocate.cold.1 (0x1c030fe10): __assert_rtn("__CFListFormatterDeallocate",
// "CFListFormatter.c", 26, "cf != NULL"). The object at offset 0x10 is released when not NULL.
static void __CFListFormatterDeallocate(OCFTypeRef cf) {
    if (cf == NULL) {
        __assert_rtn("__CFListFormatterDeallocate", "CFListFormatter.c", 26, "cf != NULL");
    }
    OCFTypeRef formatter = *(OCFTypeRef const *)((const uint8_t *)cf + 0x10);
    if (formatter != NULL) {
        OCFRelease(formatter);
    }
}

// Native __CFListFormatterCopyDescription, 0x1c02a774c. Non-external. A NULL object is
// __CFListFormatterCopyDescription.cold.1 (0x1c030fe38): __assert_rtn("__CFListFormatterCopyDescription",
// "CFListFormatter.c", 32, "cf != NULL"). CFGetAllocator is called twice: first for the result's allocator,
// then for the second format argument.
static OCFStringRef __CFListFormatterCopyDescription(OCFTypeRef cf) {
    if (cf == NULL) {
        __assert_rtn("__CFListFormatterCopyDescription", "CFListFormatter.c", 32, "cf != NULL");
    }
    OCFAllocatorRef allocator = OCFGetAllocator(cf);
    OCFAllocatorRef described = OCFGetAllocator(cf);
    return OCFStringCreateWithFormat(allocator, NULL, OF_STR("<CFListFormatter %p>[%p]"), cf, described);
}

// Native __CFListFormatterClass, 0x2227a8698 (__DATA_CONST,__const). Hidden (was private external); version
// 0, name "CFListFormatter" (0x1c049441d), the finalizer and the debug description only.
const OCFRuntimeClass __CFListFormatterClass = {
    0,
    "CFListFormatter",
    NULL,
    NULL,
    __CFListFormatterDeallocate,
    NULL,
    NULL,
    NULL,
    __CFListFormatterCopyDescription,
    NULL,
    NULL,
    0,
};

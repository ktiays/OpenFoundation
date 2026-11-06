//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Date/OCFDateInterval.h"

#include "CoreFoundation/Base/OCFHashBytes.h"
#include "CoreFoundation/String/OCFString.h"

// Inferred organization: the CFDateInterval functions (__CFDateIntervalDeallocate, 0x1c028f034, through
// CFDateIntervalCreateIntersectionWithDateInterval, 0x1c028f418) are contiguous in the iOS Simulator __text
// section; only the class callbacks are restored here. The native source path is not in the evidence. Unless
// a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full); iOS Device 24A446: unread. Reading: not yet written; the evidence is the
// assembly in Local/Evidence/cf-core/iphone-simulator-core-corpus.json.

static void __CFDateIntervalDeallocate(OCFTypeRef cf);
static Boolean __CFDateIntervalEqual(OCFTypeRef cf1, OCFTypeRef cf2);
static OCFHashCode __CFDateIntervalHash(OCFTypeRef cf);
static OCFStringRef __CFDateIntervalCopyDescription(OCFTypeRef cf);

// Native __CFDateIntervalClass, 0x2227a84b8. Version 0, name "CFDateInterval", finalize, equal, hash and the
// debug description.
const OCFRuntimeClass __CFDateIntervalClass = {
    0,
    "CFDateInterval",
    NULL,
    NULL,
    __CFDateIntervalDeallocate,
    __CFDateIntervalEqual,
    __CFDateIntervalHash,
    NULL,
    __CFDateIntervalCopyDescription,
    NULL,
    NULL,
    0,
};

// Native __CFDateIntervalDeallocate, 0x1c028f034. Non-external. Releases the start date (a tail call).
static void __CFDateIntervalDeallocate(OCFTypeRef cf) {
    OCFRelease(((const struct __OCFDateInterval *)cf)->start);
}

// Native __CFDateIntervalEqual, 0x1c028f03c. Non-external. The same object is equal; different durations (or
// an unordered comparison) are not; otherwise the start dates decide (CFEqual).
static Boolean __CFDateIntervalEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    if (cf1 == cf2) {
        return true;
    }
    const struct __OCFDateInterval *first = cf1;
    const struct __OCFDateInterval *second = cf2;
    if (first->duration != second->duration) {
        return false;
    }
    return OCFEqual(first->start, second->start) != 0;
}

// Native __CFDateIntervalHash, 0x1c028f088. Non-external. CFHashBytes over the start time and the start time
// plus the duration (sixteen bytes on the stack).
static OCFHashCode __CFDateIntervalHash(OCFTypeRef cf) {
    const struct __OCFDateInterval *interval = cf;
    OCFAbsoluteTime start = OCFDateGetAbsoluteTime(interval->start);
    double times[2] = {start, start + interval->duration};
    return OCFHashBytes((const uint8_t *)times, 16);
}

// Native __CFDateIntervalCopyDescription, 0x1c028f0f8. Non-external. "<CFDateInterval %p [%p]> %@ %f" with
// the object, its allocator, the start date and the duration, formatted with kCFAllocatorSystemDefault (read
// before CFGetAllocator).
static OCFStringRef __CFDateIntervalCopyDescription(OCFTypeRef cf) {
    OCFAllocatorRef systemDefault = kOCFAllocatorSystemDefault;
    OCFAllocatorRef allocator = OCFGetAllocator(cf);
    const struct __OCFDateInterval *interval = cf;
    return OCFStringCreateWithFormat(
        systemDefault,
        NULL,
        OF_STR("<CFDateInterval %p [%p]> %@ %f"),
        cf,
        allocator,
        interval->start,
        interval->duration
    );
}

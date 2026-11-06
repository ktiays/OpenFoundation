//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/String/OCFStringBundleSupport.h"

// Inferred organization: _CFStringCreateAppend3 (0x1c02c2704) and _CFStringCreateAppend2 (0x1c02c2784)
// precede _CFBundleAddProvenanceKeyPathIfPresent, and __CFStringsDictAddFunction (0x1c02c38dc) follows
// __loadStringsFromData's blocks: all three are statics of the native bundle-strings unit (another
// sub-track), kept in this translation unit until that unit is merged. The native source path is not in the
// evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationString.md.

// Native _CFStringCreateAppend3, 0x1c02c2704. Non-external. A mutable copy (system default allocator,
// capacity the sum of the three lengths, measured in argument order) with the second and third strings
// appended.
OCFMutableStringRef _CFStringCreateAppend3(OCFStringRef first, OCFStringRef second, OCFStringRef third) {
    OCFIndex length = OCFStringGetLength(first);
    length = OCFStringGetLength(second) + length;
    length = length + OCFStringGetLength(third);
    OCFMutableStringRef result = OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, length, first);
    OCFStringAppend(result, second);
    OCFStringAppend(result, third);
    return result;
}

// Native _CFStringCreateAppend2, 0x1c02c2784. Non-external.
OCFMutableStringRef _CFStringCreateAppend2(OCFStringRef first, OCFStringRef second) {
    OCFIndex length = OCFStringGetLength(first);
    length = OCFStringGetLength(second) + length;
    OCFMutableStringRef result = OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, length, first);
    OCFStringAppend(result, second);
    return result;
}

// Native __CFStringsDictAddFunction, 0x1c02c38dc. Non-external. A dictionary applier: adds the pair to the
// mutable dictionary passed as the context (a tail call to CFDictionaryAddValue).
void __CFStringsDictAddFunction(const void *key, const void *value, void *context) {
    OCFDictionaryAddValue((OCFMutableDictionaryRef)context, key, value);
}

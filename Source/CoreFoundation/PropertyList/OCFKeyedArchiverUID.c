//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PropertyList/OCFKeyedArchiverUID.h"

#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

// Inferred organization: the CFKeyedArchiverUID functions are contiguous in the iOS Simulator __text section
// (__CFKeyedArchiverUIDCopyFormattingDescription 0x1c0162948 through _CFKeyedArchiverUIDGetValue
// 0x1c0162a18), immediately before the binary property-list writer, and the class record lies apart from the
// binary property-list data; they are kept in their own translation unit. The native source path is not in
// the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (disassembly); iOS Device 24A446: unread.
// Reading: Local/Scratch/cf-core/tasks/plist.txt (sub-track reading; no versioned reading yet).

static OCFStringRef __CFKeyedArchiverUIDCopyFormattingDescription(OCFTypeRef cf, const void *formatOptions);
static OCFStringRef __CFKeyedArchiverUIDCopyDescription(OCFTypeRef cf);

// Native __CFKeyedArchiverUIDClass, 0x22279c998 (__DATA_CONST,__const). Hidden (was private external);
// version 0, name "CFKeyedArchiverUID", the formatting and the debug descriptions.
const OCFRuntimeClass __CFKeyedArchiverUIDClass = {
    0,
    "CFKeyedArchiverUID",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    __CFKeyedArchiverUIDCopyFormattingDescription,
    __CFKeyedArchiverUIDCopyDescription,
    NULL,
    NULL,
    0,
};

// Native __CFKeyedArchiverUIDCopyFormattingDescription, 0x1c0162948. Non-external.
// Formats "@%u@" with the 32-bit value in the system default allocator; formatOptions is not read.
static OCFStringRef __CFKeyedArchiverUIDCopyFormattingDescription(OCFTypeRef cf, const void *formatOptions) {
    (void)formatOptions;
    OCFKeyedArchiverUIDRef uid = (OCFKeyedArchiverUIDRef)cf;
    return OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR("@%u@"), uid->value);
}

// Native __CFKeyedArchiverUIDCopyDescription, 0x1c0162988. Non-external.
// Loads kCFAllocatorSystemDefault before CFGetAllocator, then formats the object, its allocator and the
// value.
static OCFStringRef __CFKeyedArchiverUIDCopyDescription(OCFTypeRef cf) {
    OCFKeyedArchiverUIDRef uid = (OCFKeyedArchiverUIDRef)cf;
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    return OCFStringCreateWithFormat(
        allocator,
        NULL,
        OF_STR("<CFKeyedArchiverUID %p [%p]>{value = %u}"),
        cf,
        OCFGetAllocator(cf),
        uid->value
    );
}

// Native _CFKeyedArchiverUIDGetTypeID, 0x1c01629dc. External; returns the builtin type ID 41.
OCFTypeID _OCFKeyedArchiverUIDGetTypeID(void) {
    return 41;
}

// Native _CFKeyedArchiverUIDCreate, 0x1c01629e4. External.
// Creates a type-41 instance with 8 extra bytes and no category; on success stores the 32-bit value at
// offset 16.
OCFKeyedArchiverUIDRef _OCFKeyedArchiverUIDCreate(OCFAllocatorRef allocator, uint32_t value) {
    struct __OCFKeyedArchiverUID *uid =
        (struct __OCFKeyedArchiverUID *)_OCFRuntimeCreateInstance(allocator, 41, 8, NULL);
    if (uid != NULL) {
        uid->value = value;
    }
    return uid;
}

// Native _CFKeyedArchiverUIDGetValue, 0x1c0162a18. External.
// Any object whose non-Objective-C type ID is not 41 reaches _CFAssertMismatchedTypeID(41, typeID).
uint32_t _OCFKeyedArchiverUIDGetValue(OCFKeyedArchiverUIDRef uid) {
    OCFTypeID typeID = _CFGetNonObjCTypeID(uid);
    if (typeID != 41) {
        _CFAssertMismatchedTypeID(41, typeID);
    }
    return uid->value;
}

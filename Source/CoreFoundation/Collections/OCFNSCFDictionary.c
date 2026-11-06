//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Collections/OCFNSCFDictionary.h"

#include <dispatch/dispatch.h>
#include <objc/runtime.h>

// Inferred organization: __NSDictionaryImmutablePlaceholder (0x1c02f337c) and
// __NSDictionaryMutablePlaceholder (0x1c02f3388) lie among the __NSPlaceholderDictionary methods, with the
// static placeholder instances they answer (0x2227ab930 and 0x2227ab940, __DATA_CONST,__const);
// __NSCFDictionaryCreateTransfer (0x1c02f3d40), __NSCFDictionaryCreate (0x1c02f3e4c),
// __NSCFDictionaryCreateMutable (0x1c02f3fbc), ____FoundationPresent_block_invoke (0x1c02f40b4, listed as
// _____FoundationPresent_block_invoke_0) and OUTLINED_FUNCTION_0_60 (0x1c02f40e0), with the three .cold.1
// thunks (0x1c03171c0-0x1c03171c8), follow the __NSPlaceholderDictionary methods in the iOS Simulator __text
// section; they are the C helpers of that Objective-C unit and are kept in this translation unit until the
// unit itself is restored. The native source path is not in the evidence. Unless a definition says otherwise:
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationCollections.md (to be written from this
// unit's comments).

// libobjc's objc_retain, which the native image imports; the SDK installs no declaration for it.
extern id objc_retain(id object);

// Native __FoundationPresent, inlined into the three functions below: a dispatch_once (the inline predicate
// test, then the .cold.1 thunks and OUTLINED_FUNCTION_0_60) running ____FoundationPresent_block_invoke
// (0x1c02f40b4, non-external), which records whether objc_lookUpClass("NSXMLParser") answers a class.
// Statics: __FoundationPresent.static_init and __FoundationPresent.present of this translation unit
// (listed with an _0 suffix in the evidence).
static bool __FoundationPresent(void) {
    static dispatch_once_t static_init;
    static bool present;
    dispatch_once(&static_init, ^{
      present = objc_lookUpClass("NSXMLParser") != NULL;
    });
    return present;
}

// The __NSPlaceholderDictionary class symbol, the isa of the two placeholder instances (defined by
// Objective-C/OCFNSPlaceholderDictionary.m).
extern char placeholderDictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSPlaceholderDictionary));

// Native ___immutablePlaceholderDictionary (0x2227ab930) and ___mutablePlaceholderDictionary (0x2227ab940),
// __DATA_CONST,__const, non-external: 16 bytes each, the isa __NSPlaceholderDictionary and a zero word (read
// with Local/Scratch/cf-core/macho.py). Hidden rather than static: the __NSPlaceholderDictionary methods
// (Objective-C/OCFNSPlaceholderDictionary.m), natively in this unit, compare their receiver with these
// addresses.
const OCFRuntimeBase __immutablePlaceholderDictionary = {(uintptr_t)placeholderDictionaryClass, 0};
const OCFRuntimeBase __mutablePlaceholderDictionary = {(uintptr_t)placeholderDictionaryClass, 0};

// Native __NSDictionaryImmutablePlaceholder, 0x1c02f337c. Hidden (was private external). The address of the
// immutable placeholder instance.
id __NSDictionaryImmutablePlaceholder(void) {
    return (id)&__immutablePlaceholderDictionary;
}

// Native __NSDictionaryMutablePlaceholder, 0x1c02f3388. Hidden (was private external). The address of the
// mutable placeholder instance.
id __NSDictionaryMutablePlaceholder(void) {
    return (id)&__mutablePlaceholderDictionary;
}

// _CFAllocatorIsSystemDefault, inlined into the functions below: the system default allocator, or NULL or
// kCFAllocatorDefault while the default allocator is the system default.
static bool isSystemDefaultAllocator(OCFAllocatorRef allocator) {
    if (allocator == kOCFAllocatorSystemDefault) {
        return true;
    }
    if (allocator == NULL || allocator == kOCFAllocatorDefault) {
        return kOCFAllocatorSystemDefault == OCFAllocatorGetDefault();
    }
    return false;
}

// Native __NSCFDictionaryCreateTransfer, 0x1c02f3d40, with .cold.1 (0x1c03171c0). Hidden (was private
// external). Without Foundation, or with an allocator other than the system default, NULL. Otherwise the
// keys and values are transferred (options 0xc, no retains): one entry makes __NSSingleEntryDictionaryI_new,
// none the retained empty singleton (objc_retain, a tail call), more __NSDictionaryI_new.
OCFDictionaryRef __NSCFDictionaryCreateTransfer(
    OCFAllocatorRef allocator, const void **keys, const void **values, OCFIndex count
) {
    if (!__FoundationPresent() || !isSystemDefaultAllocator(allocator)) {
        return NULL;
    }
    if (count == 1) {
        return __NSSingleEntryDictionaryI_new(keys[0], values[0], 0xC);
    }
    if (count == 0) {
        return (OCFDictionaryRef)objc_retain((id)__ONSDictionary0__);
    }
    return __NSDictionaryI_new(keys, values, NULL, count, 0xC);
}

// Native __NSCFDictionaryCreate, 0x1c02f3e4c, with .cold.1 (0x1c03171c4). Hidden (was private external).
// Only with Foundation, the system default allocator, the CFType value callbacks and either the CFType or the
// copy-string key callbacks (pointer comparisons); otherwise NULL. CFType keys always make
// __NSDictionaryI_new with options 0. Copy-string keys: no entries the retained empty singleton, one entry
// __NSSingleEntryDictionaryI_new with options 1, more __NSDictionaryI_new with options 1.
OCFDictionaryRef __NSCFDictionaryCreate(
    OCFAllocatorRef allocator,
    const void **keys,
    const void **values,
    OCFIndex numValues,
    const OCFDictionaryKeyCallBacks *keyCallBacks,
    const OCFDictionaryValueCallBacks *valueCallBacks
) {
    if (!__FoundationPresent() || !isSystemDefaultAllocator(allocator)) {
        return NULL;
    }
    if ((keyCallBacks != &kOCFCopyStringDictionaryKeyCallBacks &&
         keyCallBacks != &kOCFTypeDictionaryKeyCallBacks) ||
        valueCallBacks != &kOCFTypeDictionaryValueCallBacks) {
        return NULL;
    }
    if (keyCallBacks != &kOCFCopyStringDictionaryKeyCallBacks) {
        return __NSDictionaryI_new(keys, values, NULL, numValues, 0);
    }
    if (numValues == 0) {
        return (OCFDictionaryRef)objc_retain((id)__ONSDictionary0__);
    }
    if (numValues == 1) {
        return __NSSingleEntryDictionaryI_new(keys[0], values[0], 1);
    }
    return __NSDictionaryI_new(keys, values, NULL, numValues, 1);
}

// Native __NSCFDictionaryCreateMutable, 0x1c02f3fbc, with .cold.1 (0x1c03171c8). Hidden (was private
// external). Only with Foundation and the system default allocator: CFType key and value callbacks make
// __NSDictionaryM_new(NULL, NULL, capacity, 0), copy-string keys with CFType values options 3; anything else
// NULL.
OCFMutableDictionaryRef __NSCFDictionaryCreateMutable(
    OCFAllocatorRef allocator,
    OCFIndex capacity,
    const OCFDictionaryKeyCallBacks *keyCallBacks,
    const OCFDictionaryValueCallBacks *valueCallBacks
) {
    if (!__FoundationPresent() || !isSystemDefaultAllocator(allocator)) {
        return NULL;
    }
    uintptr_t options;
    if (keyCallBacks == &kOCFTypeDictionaryKeyCallBacks &&
        valueCallBacks == &kOCFTypeDictionaryValueCallBacks) {
        options = 0;
    } else if (
        keyCallBacks == &kOCFCopyStringDictionaryKeyCallBacks &&
        valueCallBacks == &kOCFTypeDictionaryValueCallBacks
    ) {
        options = 3;
    } else {
        return NULL;
    }
    return __NSDictionaryM_new(NULL, NULL, capacity, options);
}

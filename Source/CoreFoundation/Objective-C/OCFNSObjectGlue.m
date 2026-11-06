//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSObjectGlue.h"

#import "CoreFoundation/Objective-C/OCFNSDate.h"

#include <objc/runtime.h>

// Inferred organization: _NSIsNSDate (0x1c029e19c), _NSObjectStoreWeak (0x1c029edfc) and
// _NSObjectLoadWeakRetained (0x1c029eec4) lie in the Objective-C runtime glue of the iOS Simulator __text
// section, with _NSIsNSNumber, __CFAllocateObject and __CFLookUpClass, which Objective-C/OCFNSObject.m
// restores; that file belongs to another family, so these three have their own translation unit, in native
// address order. The unit is Objective-C, compiled with manual reference counting. The native source path is
// not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full); iOS Device 24A446:
// unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json.

// libobjc entry points the native unit imports whose declarations the SDK does not install
// (objc-internal.h); supplied, never reimplemented. The GOT slots 0x222409408, 0x222409420 and 0x222409138
// lie outside the image; they are bound by use as Objective-C/OCFNSObject.m binds them (confidence: medium).
extern IMP object_getMethodImplementation(id object, SEL name);
extern Class objc_opt_class(id object);
extern BOOL objc_opt_isKindOfClass(id object, Class cls);
extern void _objc_msgForward(void);
extern id objc_storeWeakOrNil(id *location, id object);
extern id objc_loadWeakRetained(id *location);
extern id objc_retain(id object);
extern id objc_autorelease(id object);
extern uintptr_t objc_debug_taggedpointer_obfuscator;
extern const uint8_t objc_debug_tag60_permutations[8];

// The NSDate class symbol, loaded natively through its class reference (off_2227CE2D0).
extern char dateClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDate));
// libobjc's root class, which this image imports.
extern char nsObjectClass[] __asm("_OBJC_CLASS_$_NSObject");

// Native _NSIsNSDate, 0x1c029e19c. External.
// A tagged pointer answers whether its tag is 6 (the inline decoding _NSIsNSNumber uses: the obfuscator, read
// as a 32-bit word, is skipped for pointers with bits 63, 62 and 0-2 set, and the low three bits are found in
// the seven-entry permutation table). nil answers NO. Otherwise the object's isNSDate__ implementation
// (selector 0x2387b031b, named by use) is called (a tail call) unless it is the forwarding implementation, in
// which case the answer is whether the object is a kind of objc_opt_class(NSDate).
BOOL _ONSIsNSDate(id object) {
    if ((intptr_t)object < 0) {
        uintptr_t pointer = (uintptr_t)object;
        uint32_t obfuscator = (pointer & 0xC000000000000007) == 0xC000000000000007
                                  ? 0
                                  : (uint32_t)objc_debug_taggedpointer_obfuscator;
        uintptr_t value = (obfuscator ^ (uint32_t)pointer) & 7;
        uintptr_t index = 0;
        while (value != objc_debug_tag60_permutations[index]) {
            index++;
            if (index == 7) {
                break;
            }
        }
        return (index & 7) == 6;
    }
    if (object == nil) {
        return NO;
    }
    SEL selector = sel_getUid("isNSDate__");
    IMP implementation = object_getMethodImplementation(object, selector);
    if (implementation != (IMP)_objc_msgForward) {
        return ((BOOL (*)(id, SEL))implementation)(object, selector);
    }
    return objc_opt_isKindOfClass(object, objc_opt_class((id)dateClass)) & 1;
}

// Native _NSIsNSObject, 0x1c029e2e8. External. nil answers NO; otherwise the object's isNSObject__
// implementation (selector 0x2387b0344) is called unless it is the forwarding implementation, in which case
// the answer is whether the object is a kind of objc_opt_class(NSObject). Reading:
// Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3; iOS Device divergence: unread).
BOOL _ONSIsNSObject(id object) {
    if (object == nil) {
        return NO;
    }
    SEL selector = sel_getUid("isNSObject__");
    IMP implementation = object_getMethodImplementation(object, selector);
    if (implementation != (IMP)_objc_msgForward) {
        return ((BOOL (*)(id, SEL))implementation)(object, selector);
    }
    return objc_opt_isKindOfClass(object, objc_opt_class((id)nsObjectClass)) & 1;
}

// Native _NSObjectStoreWeak, 0x1c029edfc. External.
// A non-nil object first clears the location when the fallback flag is 1, then is stored weakly
// (objc_storeWeakOrNil): a non-nil result clears the flag and is answered; a nil result (the object cannot be
// referenced weakly) sets the flag and stores the object itself (unretained), which is answered. A nil object
// answers nil: an empty location is left alone, a fallback location is cleared, and a weak location is
// cleared with objc_storeWeakOrNil. The flag is compared with 1 as a byte.
id _ONSObjectStoreWeak(id *location, id object, bool *useFallback) {
    if (object != nil) {
        if (*(uint8_t *)useFallback == 1) {
            *location = nil;
        }
        id stored = objc_storeWeakOrNil(location, object);
        if (stored != nil) {
            *useFallback = false;
            return stored;
        }
        *useFallback = true;
        *location = object;
        return object;
    }
    if (*location == nil) {
        return nil;
    }
    if (*(uint8_t *)useFallback == 1) {
        *location = nil;
        return nil;
    }
    objc_storeWeakOrNil(location, nil);
    return nil;
}

// Native _NSObjectLoadWeak, 0x1c029ee9c. External. With bit 0 of the fallback flag set, the stored object is
// retained and autoreleased; otherwise the weak load is autoreleased. Reading:
// Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3; iOS Device divergence: unread).
id _ONSObjectLoadWeak(id *location, bool *useFallback) {
    if ((*(uint8_t *)useFallback & 1) != 0) {
        return objc_autorelease(objc_retain(*location));
    }
    return objc_autorelease(objc_loadWeakRetained(location));
}

// Native _NSObjectLoadWeakRetained, 0x1c029eec4. External. With bit 0 of the fallback flag set, the stored
// object is retained (objc_retain); otherwise objc_loadWeakRetained. Both are tail calls.
id _ONSObjectLoadWeakRetained(id *location, bool *useFallback) {
    if ((*(uint8_t *)useFallback & 1) != 0) {
        return objc_retain(*location);
    }
    return objc_loadWeakRetained(location);
}

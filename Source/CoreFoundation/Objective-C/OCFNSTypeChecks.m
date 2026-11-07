//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSTypeChecks.h"

#include <objc/runtime.h>
#include <stdint.h>

// Inferred organization: _NSIsNSArray (0x1c029e0b4), _NSIsNSData (0x1c029e128), _NSIsNSDictionary
// (0x1c029e274), _NSIsNSOrderedSet (0x1c029e35c), _NSIsNSSet (0x1c029e4d0), _NSIsNSString (0x1c029e544),
// _NSIsNSTimeZone (0x1c029e664) and _NSIsNSIndexSet (0x1c029e7f4) belong to the Objective-C runtime glue unit
// restored in Objective-C/OCFNSObject.m (between the isNS*__ methods of NSObject and getAtomTarget,
// interleaved with _NSIsNSDate, _NSIsNSObject, _NSIsNSNumber, _NSIsNSValue, _NSIsNSURL and
// _NSIsNSCFConstantString). That file is outside this track's ownership, so these eight checks have this
// translation unit, named for the cluster, in native address order; they join the native unit when the owner
// integrates it. _NSIsNSTimeZone's evidence is Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json.
// Objective-C, manual reference counting. OUTLINED_FUNCTION_0_43 (0x1c029eed8, a tail call of
// __CFFullMethodName with X19 and X21) is the shared fragment of __CFRequireConcreteImplementation and is
// restored with it in Objective-C/OCFRequireConcreteImplementation.m. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full).
// iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not
// written yet).
//
// Selector names: the isNS*__ selector strings (0x2387b02ed isNSArray__, 0x2387b0310 isNSData__, 0x2387b0326
// isNSDictionary__, 0x2387b0351 isNSOrderedSet__, 0x2387b0362 isNSSet__, 0x2387b036c isNSString__) lie in the
// shared cache outside the extracted image; they are named by use, consistent with the sorted string pool
// spacing of the isNSArray__/isNSCFConstantString__/isNSData__/isNSDate__/isNSDictionary__ run (confidence:
// medium). The selector string 0x2387b0379 (isNSTimeZone__) is named by use likewise, after the isNSString__
// string of the same pool (confidence: medium). The GOT slot 0x222409138 is bound to _objc_msgForward,
// 0x222409420 to objc_debug_taggedpointer_obfuscator and 0x222409408 to objc_debug_tag60_permutations by use,
// as in Objective-C/OCFNSObject.m (confidence: medium).

// libobjc entry points and variables without SDK declarations (objc-internal.h); supplied, never
// reimplemented.
extern IMP object_getMethodImplementation(id object, SEL name);
extern Class objc_opt_class(id object);
extern BOOL objc_opt_isKindOfClass(id object, Class cls);
extern void _objc_msgForward(void);
extern uintptr_t objc_debug_taggedpointer_obfuscator;
extern const uint8_t objc_debug_tag60_permutations[8];

// The class symbols the checks load through their class references.
extern char arrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSArray));
extern char dataClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSData));
extern char dictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSDictionary));
extern char orderedSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSOrderedSet));
extern char setClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSSet));
extern char timeZoneClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSTimeZone));

// Native __NSStringClass (0x22e6d3820) and __NSIndexSetClass (0x22e6d3830), __DATA,__bss. Non-external; the
// NSString and NSIndexSet classes, looked up on first use.
static Class _NSStringClass;
static Class _NSIndexSetClass;

static Class _NSValueClass;
static Class _NSURLClass;
static Class _NSAttributedStringClass;

// The shared body of the class-reference checks (inlined natively in each): NULL answers false; the object's
// isNS*__ implementation is called (a tail call) unless it is the forwarding implementation, in which case
// the answer is whether the object is a kind of the class (objc_opt_class of the class reference).
// isKindByMethod is a local descriptive name.
static inline BOOL isKindByMethod(id object, const char *selectorName, const char *classSymbol) {
    if (object == nil) {
        return NO;
    }
    SEL selector = sel_getUid(selectorName);
    IMP implementation = object_getMethodImplementation(object, selector);
    if (implementation != (IMP)_objc_msgForward) {
        return ((BOOL (*)(id, SEL))implementation)(object, selector);
    }
    Class cls = objc_opt_class((id)classSymbol);
    return objc_opt_isKindOfClass(object, cls) & 1;
}

// Native _NSIsNSArray, 0x1c029e0b4. External. isNSArray__, else a kind of NSArray.
BOOL _ONSIsNSArray(id object) {
    return isKindByMethod(object, "isNSArray__", arrayClass);
}

// Native _NSIsNSData, 0x1c029e128. External. isNSData__, else a kind of NSData.
BOOL _ONSIsNSData(id object) {
    return isKindByMethod(object, "isNSData__", dataClass);
}

// Native _NSIsNSDictionary, 0x1c029e274. External. isNSDictionary__, else a kind of NSDictionary.
BOOL _ONSIsNSDictionary(id object) {
    return isKindByMethod(object, "isNSDictionary__", dictionaryClass);
}

// Native _NSIsNSOrderedSet, 0x1c029e35c. External. isNSOrderedSet__, else a kind of NSOrderedSet.
BOOL _ONSIsNSOrderedSet(id object) {
    return isKindByMethod(object, "isNSOrderedSet__", orderedSetClass);
}

// Native _NSIsNSSet, 0x1c029e4d0. External. isNSSet__, else a kind of NSSet.
BOOL _ONSIsNSSet(id object) {
    return isKindByMethod(object, "isNSSet__", setClass);
}

// Native _NSIsNSString, 0x1c029e544. External. A tagged pointer (bit 63 set) is decoded inline: the
// obfuscator, read as a 64-bit word, is skipped for pointers with bits 63, 62 and 0-2 set; the low three bits
// are found in the seven-entry permutation table; index 7 means an extended tag, bits 55-62 of (the decoded
// pointer | index) plus 8. Tags below 23 answer bit tag of 0x400005 (tags 0, 2 and 22); others false.
// Otherwise NSString (OF_NAME_STRING(ONSString), the prefixed class in the prefixed build) is looked up with
// objc_getClass on first use (a failed lookup answers false and is retried on the next call); nil answers
// false. The object's isNSString__ implementation is called (a tail call) unless it is the forwarding
// implementation, in which case the answer is whether the object is a kind of NSString.
BOOL _ONSIsNSString(id object) {
    if ((intptr_t)object < 0) {
        uintptr_t pointer = (uintptr_t)object;
        uintptr_t obfuscator =
            (pointer & 0xC000000000000007) == 0xC000000000000007 ? 0 : objc_debug_taggedpointer_obfuscator;
        uintptr_t value = obfuscator ^ pointer;
        uintptr_t index = 0;
        while ((value & 7) != objc_debug_tag60_permutations[index]) {
            index++;
            if (index == 7) {
                break;
            }
        }
        uint32_t tag = (uint32_t)(index & 7);
        if (tag == 7) {
            tag = (uint32_t)((OF_WORD_SHIFT((index | value), 55)) & 0xFF) + 8;
        }
        if (tag >= 0x17) {
            return NO;
        }
        return (0x400005u >> tag) & 1;
    }
    Class cls = _NSStringClass;
    if (cls == Nil) {
        cls = objc_getClass(OF_NAME_STRING(ONSString));
        _NSStringClass = cls;
        if (cls == Nil) {
            return NO;
        }
    }
    if (object == nil) {
        return NO;
    }
    SEL selector = sel_getUid("isNSString__");
    IMP implementation = object_getMethodImplementation(object, selector);
    if (implementation != (IMP)_objc_msgForward) {
        return ((BOOL (*)(id, SEL))implementation)(object, selector);
    }
    return objc_opt_isKindOfClass(object, cls) & 1;
}

// Native _NSIsNSTimeZone, 0x1c029e664. External. NULL answers false; the object's isNSTimeZone__
// implementation is called (a tail call) unless it is the forwarding implementation, in which case the
// answer is whether the object is a kind of NSTimeZone (objc_opt_class of the class reference
// off_2227CE3D8; bit 0).
BOOL _ONSIsNSTimeZone(id object) {
    return isKindByMethod(object, "isNSTimeZone__", timeZoneClass);
}

// Native _NSIsNSValue, 0x1c029e6d8. External. iOS Device divergence: unread.
// Selector 0x2387b0392 is bound to isNSValue__ by use and selector-pool order (confidence: medium).
BOOL _ONSIsNSValue(id object) {
    Class cls = _NSValueClass;
    if (cls == Nil) {
        cls = objc_getClass(OF_NAME_STRING(ONSValue));
        _NSValueClass = cls;
        if (cls == Nil) {
            return NO;
        }
    }
    if (object == nil) {
        return NO;
    }
    SEL selector = sel_getUid("isNSValue__");
    IMP implementation = object_getMethodImplementation(object, selector);
    if (implementation != (IMP)_objc_msgForward) {
        return ((BOOL (*)(id, SEL))implementation)(object, selector);
    }
    return objc_opt_isKindOfClass(object, cls) & 1;
}

// Native _NSIsNSIndexSet, 0x1c029e7f4. External. NSIndexSet (Foundation's class, looked up by its native name
// in both naming modes) is looked up with objc_getClass on first use; a failed lookup answers false (retried
// on the next call); otherwise the answer is whether the object is a kind of it (nil included, which
// objc_opt_isKindOfClass answers false).
BOOL _ONSIsNSIndexSet(id object) {
    Class cls = _NSIndexSetClass;
    if (cls == Nil) {
        cls = objc_getClass(OF_NAME_STRING(ONSIndexSet));
        _NSIndexSetClass = cls;
        if (cls == Nil) {
            return NO;
        }
    }
    return objc_opt_isKindOfClass(object, cls) & 1;
}

// Native _NSIsNSURL, 0x1c029e844. External. iOS Device divergence: unread.
// Selector 0x2387b0388 is bound to isNSURL__ by use and selector-pool order (confidence: medium).
BOOL _ONSIsNSURL(id object) {
    Class cls = _NSURLClass;
    if (cls == Nil) {
        cls = objc_getClass(OF_NAME_STRING(ONSURL));
        _NSURLClass = cls;
        if (cls == Nil) {
            return NO;
        }
    }
    if (object == nil) {
        return NO;
    }
    SEL selector = sel_getUid("isNSURL__");
    IMP implementation = object_getMethodImplementation(object, selector);
    if (implementation != (IMP)_objc_msgForward) {
        return ((BOOL (*)(id, SEL))implementation)(object, selector);
    }
    return objc_opt_isKindOfClass(object, cls) & 1;
}

// Native _NSIsNSAttributedString, 0x1c029e8ec. External. iOS Device divergence: unread.
BOOL _ONSIsNSAttributedString(id object) {
    Class cls = _NSAttributedStringClass;
    if (cls == Nil) {
        cls = objc_getClass(OF_NAME_STRING(ONSAttributedString));
        _NSAttributedStringClass = cls;
        if (cls == Nil) {
            return NO;
        }
    }
    return objc_opt_isKindOfClass(object, cls) & 1;
}

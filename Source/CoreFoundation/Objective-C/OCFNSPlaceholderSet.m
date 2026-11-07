//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSPlaceholderSet.h"
#import "CoreFoundation/Objective-C/OCFNSSetI.h"
#import "CoreFoundation/Objective-C/OCFNSSetM.h"
#import "CoreFoundation/Objective-C/OCFNSSingleObjectSetI.h"
#import "CoreFoundation/Objective-C/OCFNSEmptyCollections.h"

#include <stdint.h>

// Inferred organization: the __NSPlaceholderSet methods (0x1c0196cec-0x1c019779f) lie together in the iOS
// Simulator __text section, with the unit's C functions __NSSetImmutablePlaceholder and
// __NSSetMutablePlaceholder (0x1c019713c, 0x1c0197148) between +initialize and -initWithObjects:count:, and
// the two static placeholder objects they answer; its trapping +allocWithZone: (0x1c0307fac) and -dealloc
// (0x1c0307fb0) lie in the cold region. Following the owner's decision, the class has its own translation
// unit, which keeps the native order (the two functions are defined inside the @implementation). Objective-C
// compiled with manual reference counting (objc_retain). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json (the two functions) and
// iphone-simulator-corpus-delta10.json (the methods); data read with Local/Scratch/cf-core/macho.py. Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The __NSPlaceholderSet class symbol (0x22e6cc8c0), the isa of both placeholders, defined by this unit's
// @implementation.
extern char placeholderSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSPlaceholderSet));

// The __NSSetI, __NSSetM and __NSFrozenSetM class symbols, loaded natively through their GOT slots
// (___SCR___NSSetI, ___SCR___NSSetM, ___SCR___NSFrozenSetM) and compared with the class of the argument of
// -initWithSet:copyItems: (all three restored in Objective-C/OCFNSSetI.m and Objective-C/OCFNSSetM.m).
extern char placeholderSetIClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSSetI));
extern char placeholderSetMClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSSetM));
extern char placeholderFrozenSetMClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSFrozenSetM));

// The 16-byte record of a static placeholder object: the isa and a zero word. PlaceholderSetObject is a local
// descriptive type name.
typedef struct {
    void *isa;
    uintptr_t reserved;
} PlaceholderSetObject;

// Native ___immutablePlaceholderSet (0x22279da70) and ___mutablePlaceholderSet (0x22279da80),
// __DATA_CONST,__const, non-external: { __NSPlaceholderSet, 0 } each.
static const PlaceholderSetObject ___immutablePlaceholderSet = {placeholderSetClass, 0};
static const PlaceholderSetObject ___mutablePlaceholderSet = {placeholderSetClass, 0};

// The raise every primitive method of the placeholder inlines: the class looked up by name (__CFLookUpClass
// of "NSSet" for the immutable placeholder, "NSMutableSet" for the mutable one, "<unknown>" otherwise), its
// __CFFullMethodName with the receiver and selector, and the kind ("immutable", "mutable" or "<unknown>")
// form the reason "*** %@: method sent to an uninitialized %s set object" (made with
// kCFAllocatorSystemDefault and autoreleased through _CFAutoreleasePoolAddObject);
// +[NSException exceptionWithName:reason:userInfo:] with NSInvalidArgumentException and no user info is
// thrown. The format is the set member of the image's four "method sent to an uninitialized %s ... object"
// strings (array, dictionary, ordered set, set; the capture names it cfstr_MethodSentToAn_0, confidence:
// medium for the binding). raiseUninitializedPlaceholderSet is a local descriptive name for the inlined code.
static inline __attribute__((always_inline, noreturn)) void
raiseUninitializedPlaceholderSet(id self, SEL _cmd) {
    const char *kind = self == (id)&___mutablePlaceholderSet ? "mutable" : "<unknown>";
    const char *className =
        self == (id)&___mutablePlaceholderSet ? OF_NAME_STRING(ONSMutableSet) : "<unknown>";
    if (self == (id)&___immutablePlaceholderSet) {
        kind = "immutable";
        className = OF_NAME_STRING(ONSSet);
    }
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFStringRef reason = OCFStringCreateWithFormat(
        allocator,
        NULL,
        OF_STR("*** %@: method sent to an uninitialized %s set object"),
        __CFFullMethodName((Class)__CFLookUpClass(className), self, _cmd),
        kind
    );
    objc_exception_throw([(id<SetUnrestoredMessages>)(id)setExceptionClass
        exceptionWithName:(id)ONSInvalidArgumentException
                   reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                 userInfo:nil]);
}

@implementation __ONSPlaceholderSet

// Native -[__NSPlaceholderSet count], 0x1c0196cec. Non-external. The uninitialized-placeholder raise.
- (NSUInteger)count {
    raiseUninitializedPlaceholderSet(self, _cmd);
}

// Native -[__NSPlaceholderSet member:], 0x1c0196dbc. Non-external. The uninitialized-placeholder raise (the
// same body as -count).
- (id)member:(id)object {
    raiseUninitializedPlaceholderSet(self, _cmd);
}

// Native -[__NSPlaceholderSet objectEnumerator], 0x1c0196e8c. Non-external. The uninitialized-placeholder
// raise.
- (id)objectEnumerator {
    raiseUninitializedPlaceholderSet(self, _cmd);
}

// Native -[__NSPlaceholderSet addObject:], 0x1c0196f5c. Non-external. The uninitialized-placeholder raise.
- (void)addObject:(id)object {
    raiseUninitializedPlaceholderSet(self, _cmd);
}

// Native -[__NSPlaceholderSet removeObject:], 0x1c019702c. Non-external. The uninitialized-placeholder raise.
- (void)removeObject:(id)object {
    raiseUninitializedPlaceholderSet(self, _cmd);
}

// Native +[__NSPlaceholderSet initialize], 0x1c01970fc. Non-external. For the class itself (compared with the
// ___SCR___NSPlaceholderSet GOT slot, not a subclass), the empty immutable set __NSSetI_new(NULL, NULL, 0, 0)
// is stored into __NSSet0__.
+ (void)initialize {
    if (self == (Class)placeholderSetClass) {
        __NSSet0__ = __NSSetI_new(NULL, NULL, 0, NO);
    }
}

// Native __NSSetImmutablePlaceholder, 0x1c019713c. Hidden (was a private external). The address of
// ___immutablePlaceholderSet.
id __NSSetImmutablePlaceholder(void) {
    return (id)&___immutablePlaceholderSet;
}

// Native __NSSetMutablePlaceholder, 0x1c0197148. Hidden (was a private external). The address of
// ___mutablePlaceholderSet.
id __NSSetMutablePlaceholder(void) {
    return (id)&___mutablePlaceholderSet;
}

// Native -[__NSPlaceholderSet initWithObjects:count:], 0x1c0197154. Non-external. A NULL array with a nonzero
// count raises first, then a count with any of its top three bits set, then the first nil object (each with
// os_log pack buffer 0x16: the method name and the count or the index). The immutable placeholder answers the
// retained __NSSet0__ for no objects, __NSSingleObjectSetI_new for one and __NSSetI_new (objects, no va_list)
// otherwise; the mutable placeholder answers __NSSetM_new; the objects are retained by the constructors (bit
// 0 of noRetain clear). Any other receiver is a BRK #1.
- (id)initWithObjects:(const id *)objects count:(NSUInteger)count {
    if (objects == NULL && count != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[" OF_NAME_STRING(__ONSPlaceholderSet) " initWithObjects:count:]",
            (unsigned long)count
        );
    }
    if ((OF_WORD_SHIFT(count, 61)) != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSPlaceholderSet) " initWithObjects:count:]",
            (unsigned long)count
        );
    }
    for (NSUInteger index = 0; index != count; index++) {
        if (objects[index] == nil) {
            raiseWithLogPack(
                ONSInvalidArgumentException,
                "*** %s: attempt to insert nil object from objects[%lu]",
                "-[" OF_NAME_STRING(__ONSPlaceholderSet) " initWithObjects:count:]",
                (unsigned long)index
            );
        }
    }
    if (self == (id)&___immutablePlaceholderSet) {
        if (count == 1) {
            return __NSSingleObjectSetI_new(objects[0], NO);
        }
        if (count == 0) {
            return objc_retain(__NSSet0__);
        }
        return __NSSetI_new(objects, NULL, count, NO);
    }
    if (self != (id)&___mutablePlaceholderSet) {
        __builtin_trap();
    }
    return __NSSetM_new(objects, count, NO);
}

// Native -[__NSPlaceholderSet initWithSet:copyItems:], 0x1c0197490. Non-external. Without copyItems (bit 0
// clear), an argument whose class (objc_opt_class) is __NSSetI, __NSSetM or __NSFrozenSetM is copied
// directly: the mutable placeholder answers its -mutableCopyWithZone: NULL, the immutable one its
// -copyWithZone: NULL (tail calls). Every other case (copyItems, another class, another receiver) is super's
// -initWithSet:copyItems: (objc_msgSendSuper2 from __NSPlaceholderSet).
- (id)initWithSet:(id)set copyItems:(BOOL)flag {
    if ((flag & 1) == 0) {
        Class cls = [set class];
        if (cls == (Class)placeholderSetIClass || cls == (Class)placeholderSetMClass ||
            cls == (Class)placeholderFrozenSetMClass) {
            if (self == (id)&___mutablePlaceholderSet) {
                return [set mutableCopyWithZone:NULL];
            }
            if (self == (id)&___immutablePlaceholderSet) {
                return [set copyWithZone:NULL];
            }
        }
    }
    return [super initWithSet:set copyItems:flag];
}

// Native -[__NSPlaceholderSet initWithCapacity:], 0x1c01975e0. Non-external. A capacity with any of its top
// three bits set raises (os_log pack buffer 0x16: the method name and the capacity). The mutable placeholder
// answers __NSSetM_new(NULL, capacity, 0) (a tail call); the immutable one sends itself
// -doesNotRecognizeSelector: with the selector and then executes BRK #1; any other receiver is a BRK #1.
- (id)initWithCapacity:(NSUInteger)capacity {
    if ((OF_WORD_SHIFT(capacity, 61)) != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            "*** %s: capacity (%lu) is ridiculous",
            "-[" OF_NAME_STRING(__ONSPlaceholderSet) " initWithCapacity:]",
            (unsigned long)capacity
        );
    }
    if (self == (id)&___immutablePlaceholderSet) {
        [(id)&___immutablePlaceholderSet doesNotRecognizeSelector:_cmd];
        __builtin_trap();
    }
    if (self != (id)&___mutablePlaceholderSet) {
        __builtin_trap();
    }
    return __NSSetM_new(NULL, capacity, NO);
}

// Native -[__NSPlaceholderSet init], 0x1c0197748. Non-external. The immutable placeholder answers its own
// -initWithObjects:NULL count:0, the mutable one its -initWithCapacity:0 (tail calls to the placeholder
// objects); any other receiver is a BRK #1. The native init reaches no designated initializer of super, so
// that diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-designated-initializers"

- (id)init {
    if (self == (id)&___immutablePlaceholderSet) {
        return [(id)&___immutablePlaceholderSet initWithObjects:NULL count:0];
    }
    if (self != (id)&___mutablePlaceholderSet) {
        __builtin_trap();
    }
    return [(id)&___mutablePlaceholderSet initWithCapacity:0];
}

#pragma clang diagnostic pop

// Native -[__NSPlaceholderSet retain], 0x1c0197790. Non-external. The receiver.
- (instancetype)retain {
    return self;
}

// Native -[__NSPlaceholderSet release], 0x1c0197794. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[__NSPlaceholderSet retainCount], 0x1c0197798. Non-external.
- (NSUInteger)retainCount {
    return UINTPTR_MAX;
}

// Native +[__NSPlaceholderSet allocWithZone:], 0x1c0307fac (cold region). Non-external. A bare BRK #1.
+ (id)allocWithZone:(struct _NSZone *)zone {
    __builtin_trap();
}

// Native -[__NSPlaceholderSet dealloc], 0x1c0307fb0 (cold region). Non-external. A bare BRK #1; super is
// never reached, so the missing-super-call diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"

- (void)dealloc {
    __builtin_trap();
}

#pragma clang diagnostic pop

@end

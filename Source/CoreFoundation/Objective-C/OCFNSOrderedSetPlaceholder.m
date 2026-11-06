//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSOrderedSetPlaceholder.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Objective-C/OCFNSOrderedSetI.h"
#include "CoreFoundation/Objective-C/OCFNSOrderedSetM.h"
#include "CoreFoundation/String/OCFString.h"

#include <alloca.h>
#include <dispatch/dispatch.h>
#include <mach-o/loader.h>
#include <objc/objc-exception.h>
#include <stdint.h>

// Inferred organization: the __NSPlaceholderOrderedSet methods (0x1c02be6c4-0x1c02becb3 and the primitive
// methods up to 0x1c02bf0c3) lie together in the iOS Simulator __text section, with the unit's C functions
// __NSOrderedSetImmutablePlaceholder and __NSOrderedSetMutablePlaceholder (0x1c02be6c4, 0x1c02be6d0) first,
// followed by -initWithObjects:count: (0x1c02be6dc) and its block (0x1c02be9fc); the trapping
// +allocWithZone: (0x1c0312098), -dealloc (0x1c031209c) and -initWithObjects:count:.cold.1 (0x1c03120a0) lie
// in the cold region. Following the owner's decision, the class has its own translation unit, which keeps
// the native order (the two functions are defined inside the @implementation). Objective-C compiled with
// manual reference counting. Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta11.json (family
// K-mutable-ordered-set; the two functions from iphone-simulator-corpus-delta10.json; the block of
// -initWithObjects:count: from iphone-simulator-corpus-delta12.json, family P-collections-url); data read
// with Local/Scratch/cf-core/macho.py. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// libobjc's objc_retain, which the native image imports; the SDK installs no declaration for it. Supplied,
// never reimplemented.
extern id objc_retain(id object);

// The NSException class symbol (off_2227CE300), sent to directly.
extern char exceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));

// The __NSPlaceholderOrderedSet class symbol (0x22e6ceb20), the isa of both placeholders, defined by this
// unit's @implementation.
extern char placeholderOrderedSetClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSPlaceholderOrderedSet));

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// The messages this unit sends that the shared declarations do not carry; declared for the sends only.
@protocol PlaceholderOrderedSetMessages

+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
+ (id)exceptionWithName:(id)name
                 reason:(id)reason
               userInfo:(id)userInfo
              osLogPack:(void *)pack
                   size:(size_t)size;

@end

// The 16-byte record of a static placeholder object: the isa and a zero word. PlaceholderOrderedSetObject is
// a local descriptive type name.
typedef struct {
    void *isa;
    uintptr_t reserved;
} PlaceholderOrderedSetObject;

// Native ___immutablePlaceholderOrderedSet (0x2227a9e00) and ___mutablePlaceholderOrderedSet (0x2227a9e10),
// __DATA_CONST,__const, non-external: { __NSPlaceholderOrderedSet, 0 } each.
static const PlaceholderOrderedSetObject ___immutablePlaceholderOrderedSet = {placeholderOrderedSetClass, 0};
static const PlaceholderOrderedSetObject ___mutablePlaceholderOrderedSet = {placeholderOrderedSetClass, 0};

// The raise of -initWithObjects:count: (os_log pack buffer 0x16; the method name and one unsigned long):
// NSInvalidArgumentException with no user info, the reason made with kCFAllocatorSystemDefault and
// autoreleased through _CFAutoreleasePoolAddObject. RAISE_INIT_EXCEPTION is a local descriptive name
// for the inlined code.
#define RAISE_INIT_EXCEPTION(format, value)                                                                  \
    do {                                                                                                     \
        size_t packSize = _os_log_pack_size(0x16);                                                           \
        void *pack = alloca((packSize + 15) & ~(size_t)15);                                                  \
        uint8_t *packBuffer = _os_log_pack_fill(pack, packSize, 0, &__dso_handle, format);                   \
        __builtin_os_log_format(                                                                             \
            packBuffer,                                                                                      \
            format,                                                                                          \
            "-[" OF_NAME_STRING(__ONSPlaceholderOrderedSet) " initWithObjects:count:]",                      \
            (unsigned long)(value)                                                                           \
        );                                                                                                   \
        OCFStringRef reason = OCFStringCreateWithFormat(                                                     \
            kOCFAllocatorSystemDefault,                                                                      \
            NULL,                                                                                            \
            OF_STR(format),                                                                                  \
            "-[" OF_NAME_STRING(__ONSPlaceholderOrderedSet) " initWithObjects:count:]",                      \
            (unsigned long)(value)                                                                           \
        );                                                                                                   \
        objc_exception_throw([(id<PlaceholderOrderedSetMessages>)(id)exceptionClass                          \
            exceptionWithName:(id)ONSInvalidArgumentException                                                \
                       reason:_OCFAutoreleasePoolAddObject(NULL, reason)                                     \
                     userInfo:nil                                                                            \
                    osLogPack:pack                                                                           \
                         size:packSize]);                                                                    \
    } while (0)

// The raise every primitive method of the placeholder inlines: the class looked up by name (__CFLookUpClass
// of "NSOrderedSet" for the immutable placeholder, "NSMutableOrderedSet" for the mutable one, "<unknown>"
// otherwise), its __CFFullMethodName with the receiver and selector, and the kind ("immutable", "mutable" or
// "<unknown>") form the reason "*** %@: method sent to an uninitialized %s ordered set object" (made with
// kCFAllocatorSystemDefault, loaded before the lookup, and autoreleased through _CFAutoreleasePoolAddObject);
// +[NSException exceptionWithName:reason:userInfo:] with NSInvalidArgumentException and no user info is
// thrown. raiseUninitializedPlaceholderOrderedSet is a local descriptive name for the inlined code.
static inline __attribute__((always_inline, noreturn)) void
raiseUninitializedPlaceholderOrderedSet(id self, SEL _cmd) {
    const char *kind = self == (id)&___mutablePlaceholderOrderedSet ? "mutable" : "<unknown>";
    const char *className =
        self == (id)&___mutablePlaceholderOrderedSet ? OF_NAME_STRING(ONSMutableOrderedSet) : "<unknown>";
    if (self == (id)&___immutablePlaceholderOrderedSet) {
        kind = "immutable";
        className = OF_NAME_STRING(ONSOrderedSet);
    }
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFStringRef reason = OCFStringCreateWithFormat(
        allocator,
        NULL,
        OF_STR("*** %@: method sent to an uninitialized %s ordered set object"),
        __CFFullMethodName((Class)__CFLookUpClass(className), self, _cmd),
        kind
    );
    objc_exception_throw([(id<PlaceholderOrderedSetMessages>)(id)exceptionClass
        exceptionWithName:(id)ONSInvalidArgumentException
                   reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                 userInfo:nil]);
}

@implementation __ONSPlaceholderOrderedSet

// Native __NSOrderedSetImmutablePlaceholder, 0x1c02be6c4. Hidden (was a private external). The address of
// ___immutablePlaceholderOrderedSet.
id __NSOrderedSetImmutablePlaceholder(void) {
    return (id)&___immutablePlaceholderOrderedSet;
}

// Native __NSOrderedSetMutablePlaceholder, 0x1c02be6d0. Hidden (was a private external). The address of
// ___mutablePlaceholderOrderedSet.
id __NSOrderedSetMutablePlaceholder(void) {
    return (id)&___mutablePlaceholderOrderedSet;
}

// Native -[__NSPlaceholderOrderedSet initWithObjects:count:], 0x1c02be6dc, with its .cold.1 (0x1c03120a0)
// and its function statics initWithObjects:count:.static_init (0x22e6d3b20) and
// initWithObjects:count:.__NSOrderedSet0__ (0x22e6d3b18). Non-external. A NULL array with a nonzero count
// raises first ("pointer to objects array is NULL but length is %lu"), then a count with any of its top
// three bits set ("count (%lu) of objects array is ridiculous"), then the first nil object ("attempt to
// insert nil object from objects[%lu]", the index). With objects, the immutable placeholder answers
// __NSOrderedSetI_new(objects, count, 0) (a tail call). With none, it answers the retained __NSOrderedSet0__
// (objc_retain, a tail call), made once: while the predicate is not ~0 the .cold.1 fragment runs
// dispatch_once with ___block_literal_global_42, whose invoke ___51-[__NSPlaceholderOrderedSet
// initWithObjects:count:]_block_invoke (0x1c02be9fc, non-external) stores __NSOrderedSetI_new(NULL, 0, 0).
// The mutable placeholder answers __NSOrderedSetM_new(objects, count, 0) for any count (a tail call); any
// other receiver is a BRK #1.
- (id)initWithObjects:(const id *)objects count:(NSUInteger)count {
    static dispatch_once_t static_init;
    static id __NSOrderedSet0__;
    if (objects == NULL && count != 0) {
        RAISE_INIT_EXCEPTION("*** %s: pointer to objects array is NULL but length is %lu", count);
    }
    if ((count >> 61) != 0) {
        RAISE_INIT_EXCEPTION("*** %s: count (%lu) of objects array is ridiculous", count);
    }
    if (count != 0) {
        for (NSUInteger index = 0; index != count; index++) {
            if (objects[index] == nil) {
                RAISE_INIT_EXCEPTION("*** %s: attempt to insert nil object from objects[%lu]", index);
            }
        }
        if (self == (id)&___immutablePlaceholderOrderedSet) {
            return __NSOrderedSetI_new(objects, count, NO);
        }
    } else if (self == (id)&___immutablePlaceholderOrderedSet) {
        dispatch_once(&static_init, ^{
          __NSOrderedSet0__ = __NSOrderedSetI_new(NULL, 0, NO);
        });
        return objc_retain(__NSOrderedSet0__);
    }
    if (self != (id)&___mutablePlaceholderOrderedSet) {
        __builtin_trap();
    }
    return __NSOrderedSetM_new(objects, count, NO);
}

// Native -[__NSPlaceholderOrderedSet initWithCapacity:], 0x1c02bea24. Non-external. A capacity with any of
// its top three bits set raises NSInvalidArgumentException "*** %s: capacity (%lu) is ridiculous" (os_log
// pack buffer 0x16: the method name and the capacity). The immutable placeholder sends itself
// -doesNotRecognizeSelector: with the selector and then executes BRK #1; the mutable placeholder answers
// __NSOrderedSetM_new(NULL, capacity, 0) (a tail call); any other receiver is a BRK #1.
- (id)initWithCapacity:(NSUInteger)capacity {
    if ((capacity >> 61) != 0) {
        size_t packSize = _os_log_pack_size(0x16);
        void *pack = alloca((packSize + 15) & ~(size_t)15);
        uint8_t *packBuffer =
            _os_log_pack_fill(pack, packSize, 0, &__dso_handle, "*** %s: capacity (%lu) is ridiculous");
        __builtin_os_log_format(
            packBuffer,
            "*** %s: capacity (%lu) is ridiculous",
            "-[" OF_NAME_STRING(__ONSPlaceholderOrderedSet) " initWithCapacity:]",
            (unsigned long)capacity
        );
        OCFStringRef reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("*** %s: capacity (%lu) is ridiculous"),
            "-[" OF_NAME_STRING(__ONSPlaceholderOrderedSet) " initWithCapacity:]",
            (unsigned long)capacity
        );
        objc_exception_throw([(id<PlaceholderOrderedSetMessages>)(id)exceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                     userInfo:nil
                    osLogPack:pack
                         size:packSize]);
    }
    if (self == (id)&___immutablePlaceholderOrderedSet) {
        [(id)&___immutablePlaceholderOrderedSet doesNotRecognizeSelector:_cmd];
        __builtin_trap();
    }
    if (self != (id)&___mutablePlaceholderOrderedSet) {
        __builtin_trap();
    }
    return __NSOrderedSetM_new(NULL, capacity, NO);
}

// Native -[__NSPlaceholderOrderedSet init], 0x1c02beb8c. Non-external. The immutable placeholder answers its
// own -initWithObjects:NULL count:0, the mutable one its -initWithCapacity:0 (tail calls to the placeholder
// objects); any other receiver is a BRK #1. The native init reaches no designated initializer of super, so
// that diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-designated-initializers"

- (id)init {
    if (self == (id)&___immutablePlaceholderOrderedSet) {
        return [(id)&___immutablePlaceholderOrderedSet initWithObjects:NULL count:0];
    }
    if (self != (id)&___mutablePlaceholderOrderedSet) {
        __builtin_trap();
    }
    return [(id)&___mutablePlaceholderOrderedSet initWithCapacity:0];
}

#pragma clang diagnostic pop

// Native -[__NSPlaceholderOrderedSet retain], 0x1c02bebd4. Non-external. The receiver (a bare RET).
- (instancetype)retain {
    return self;
}

// Native -[__NSPlaceholderOrderedSet release], 0x1c02bebd8. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[__NSPlaceholderOrderedSet retainCount], 0x1c02bebdc. Non-external.
- (NSUInteger)retainCount {
    return UINTPTR_MAX;
}

// Native -[__NSPlaceholderOrderedSet count], 0x1c02bebe4. Non-external. The uninitialized-placeholder raise.
- (NSUInteger)count {
    raiseUninitializedPlaceholderOrderedSet(self, _cmd);
}

// Native -[__NSPlaceholderOrderedSet indexOfObject:], 0x1c02becb4. Non-external. The
// uninitialized-placeholder raise (the same instructions as -count).
- (NSUInteger)indexOfObject:(id)object {
    raiseUninitializedPlaceholderOrderedSet(self, _cmd);
}

// Native -[__NSPlaceholderOrderedSet objectAtIndex:], 0x1c02bed84. Non-external. The
// uninitialized-placeholder raise.
- (id)objectAtIndex:(NSUInteger)index {
    raiseUninitializedPlaceholderOrderedSet(self, _cmd);
}

// Native -[__NSPlaceholderOrderedSet insertObject:atIndex:], 0x1c02bee54. Non-external. The
// uninitialized-placeholder raise.
- (void)insertObject:(id)object atIndex:(NSUInteger)index {
    raiseUninitializedPlaceholderOrderedSet(self, _cmd);
}

// Native -[__NSPlaceholderOrderedSet removeObjectAtIndex:], 0x1c02bef24. Non-external. The
// uninitialized-placeholder raise.
- (void)removeObjectAtIndex:(NSUInteger)index {
    raiseUninitializedPlaceholderOrderedSet(self, _cmd);
}

// Native -[__NSPlaceholderOrderedSet replaceObjectAtIndex:withObject:], 0x1c02beff4. Non-external. The
// uninitialized-placeholder raise.
- (void)replaceObjectAtIndex:(NSUInteger)index withObject:(id)object {
    raiseUninitializedPlaceholderOrderedSet(self, _cmd);
}

// Native +[__NSPlaceholderOrderedSet allocWithZone:], 0x1c0312098 (cold region). Non-external. A bare BRK #1.
+ (id)allocWithZone:(struct _NSZone *)zone {
    __builtin_trap();
}

// Native -[__NSPlaceholderOrderedSet dealloc], 0x1c031209c (cold region). Non-external. A bare BRK #1; super
// is never reached, so the missing-super-call diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"

- (void)dealloc {
    __builtin_trap();
}

#pragma clang diagnostic pop

@end

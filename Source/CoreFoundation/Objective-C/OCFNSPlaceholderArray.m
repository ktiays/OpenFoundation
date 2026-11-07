//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSPlaceholderArray.h"
#include "CoreFoundation/Objective-C/OCFNSEmptyCollections.h"
#include "CoreFoundation/Objective-C/OCFNSArrayI.h"
#include "CoreFoundation/Objective-C/OCFNSArrayM.h"
#include "CoreFoundation/Objective-C/OCFNSSingleObjectArrayI.h"
#include "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <alloca.h>
#include <mach-o/loader.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdint.h>

// Inferred organization: the __NSPlaceholderArray methods lie together in the iOS Simulator __text section
// (0x1c0161b4c-0x1c0162947), with __NSArrayImmutablePlaceholder (0x1c0161f5c), __NSArrayMutablePlaceholder
// (0x1c0161f68) and the non-external __initWithArrayDontCopyItems (0x1c0162040) among them; +allocWithZone:
// and -dealloc (0x1c03063a4, 0x1c03063a8) lie in the cold region. The placeholder objects they answer,
// ___immutablePlaceholderArray (0x22279c978) and ___mutablePlaceholderArray (0x22279c988), and the static
// class cache initWithContentsOfURL:.__cls_NSURL are statics of the unit. Following the owner's decision, the
// class has this translation unit; the methods follow in native address order, the C functions at their
// native positions inside the @implementation. Objective-C compiled with manual reference counting
// (objc_retain). Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json (family
// H-array-concrete; the two placeholder functions from iphone-simulator-corpus-delta8.json),
// iphone-simulator-corpus-delta11.json (family L-collection-leftovers: -retain, listed by instructions) and
// Local/Scratch/cf-core/macho.py (the objects).
// Unless a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).
//
// Argument errors raise as the other bridged array classes do (an os_log pack built on the stack, the reason
// made with kCFAllocatorSystemDefault and autoreleased, +[NSException
// exceptionWithName:reason:userInfo:osLogPack:size:] thrown); the first format argument is the method's name,
// spelled with the class's selected name.

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// The class symbols the unit references directly: __NSPlaceholderArray (the isa of both placeholder
// objects), NSException (off_2227CE300), NSArray (off_2227CE2A0) and the concrete classes
// __initWithArrayDontCopyItems compares against (___SCR___NSArrayI, ___SCR___NSArrayM,
// ___SCR___NSFrozenArrayM).
extern char placeholderArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSPlaceholderArray));
extern char exceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));
extern char arrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSArray));
extern char arrayIClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSArrayI));
extern char arrayMClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSArrayM));
extern char frozenArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSFrozenArrayM));

// The messages this unit sends that the shared declarations do not carry; declared for the sends only.
@protocol PlaceholderArrayMessages

+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
+ (id)exceptionWithName:(id)name
                 reason:(id)reason
               userInfo:(id)userInfo
              osLogPack:(void *)pack
                   size:(size_t)size;
+ (id)newWithContentsOf:(id)source immutable:(BOOL)immutable;
- (id)initWithCapacity:(NSUInteger)capacity;

@end

#define methodName(selectorText) "-[" OF_NAME_STRING(__ONSPlaceholderArray) " " selectorText "]"

#define RAISE_LOGGED_EXCEPTION(name, bufferSize, format, ...)                                                \
    do {                                                                                                     \
        size_t packSize = _os_log_pack_size(bufferSize);                                                     \
        void *pack = alloca((packSize + 15) & ~(size_t)15);                                                  \
        uint8_t *packBuffer = _os_log_pack_fill(pack, packSize, 0, &__dso_handle, format);                   \
        __builtin_os_log_format(packBuffer, format, __VA_ARGS__);                                            \
        OCFStringRef reason =                                                                                \
            OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR(format), __VA_ARGS__);        \
        objc_exception_throw([(id)exceptionClass                                                             \
            exceptionWithName:(id)(name)                                                                     \
                       reason:_OCFAutoreleasePoolAddObject(NULL, reason)                                     \
                     userInfo:nil                                                                            \
                    osLogPack:pack                                                                           \
                         size:packSize]);                                                                    \
    } while (0)

// The layout of the two placeholder objects (16 bytes each: the isa, then a zero word). PlaceholderObject is
// a local descriptive name.
typedef struct {
    Class isa;
    uintptr_t reserved;
} PlaceholderObject;

// Native ___immutablePlaceholderArray (0x22279c978) and ___mutablePlaceholderArray (0x22279c988),
// __DATA_CONST,__const. Non-external; read with Local/Scratch/cf-core/macho.py: each holds
// _OBJC_CLASS_$___NSPlaceholderArray (0x22e6cc780) and 0.
static const PlaceholderObject __immutablePlaceholderArray = {(Class)(void *)placeholderArrayClass, 0};
static const PlaceholderObject __mutablePlaceholderArray = {(Class)(void *)placeholderArrayClass, 0};

// The placeholder objects as object pointers. immutablePlaceholder and mutablePlaceholder are local
// descriptive names.
static inline id immutablePlaceholder(void) {
    return (id)&__immutablePlaceholderArray;
}

static inline id mutablePlaceholder(void) {
    return (id)&__mutablePlaceholderArray;
}

// The raise the five primitive methods share (each native method inlines it): the kind is "immutable" with
// the class name "NSArray" for the immutable placeholder, "mutable" with "NSMutableArray" for the mutable
// one,
// "<unknown>" for both otherwise; the reason "*** %@: method sent to an uninitialized %s array object" with
// __CFFullMethodName of the class __CFLookUpClass finds by that name, the receiver and the selector
// (kCFAllocatorSystemDefault), autoreleased through _CFAutoreleasePoolAddObject; +[NSException
// exceptionWithName:reason:userInfo:] NSInvalidArgumentException with no user info is thrown.
// raiseUninitialized is a local descriptive name.
static inline __attribute__((noreturn)) void raiseUninitialized(id object, SEL selector) {
    const char *kind = object == mutablePlaceholder() ? "mutable" : "<unknown>";
    const char *className = object == mutablePlaceholder() ? OF_NAME_STRING(ONSMutableArray) : "<unknown>";
    if (object == immutablePlaceholder()) {
        kind = "immutable";
        className = OF_NAME_STRING(ONSArray);
    }
    OCFStringRef name = __CFFullMethodName((Class)__CFLookUpClass(className), object, selector);
    OCFStringRef reason = OCFStringCreateWithFormat(
        kOCFAllocatorSystemDefault,
        NULL,
        OF_STR("*** %@: method sent to an uninitialized %s array object"),
        name,
        kind
    );
    objc_exception_throw([(id)exceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                        reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                                                      userInfo:nil]);
}

static id __initWithArrayDontCopyItems(id placeholder, id array);

@implementation __ONSPlaceholderArray

// Native -[__NSPlaceholderArray count], 0x1c0161b4c. Non-external. raiseUninitialized.
- (NSUInteger)count {
    raiseUninitialized(self, _cmd);
}

// Native -[__NSPlaceholderArray objectAtIndex:], 0x1c0161c1c. Non-external. raiseUninitialized.
- (id)objectAtIndex:(NSUInteger)index {
    raiseUninitialized(self, _cmd);
}

// Native -[__NSPlaceholderArray insertObject:atIndex:], 0x1c0161cec. Non-external. raiseUninitialized.
- (void)insertObject:(id)object atIndex:(NSUInteger)index {
    raiseUninitialized(self, _cmd);
}

// Native -[__NSPlaceholderArray removeObjectAtIndex:], 0x1c0161dbc. Non-external. raiseUninitialized.
- (void)removeObjectAtIndex:(NSUInteger)index {
    raiseUninitialized(self, _cmd);
}

// Native -[__NSPlaceholderArray replaceObjectAtIndex:withObject:], 0x1c0161e8c. Non-external.
// raiseUninitialized.
- (void)replaceObjectAtIndex:(NSUInteger)index withObject:(id)object {
    raiseUninitialized(self, _cmd);
}

// Native __NSArrayImmutablePlaceholder, 0x1c0161f5c. Hidden (was a private external). Answers the immutable
// placeholder object. Evidence: iphone-simulator-corpus-delta8.json.
id __NSArrayImmutablePlaceholder(void) {
    return (id)&__immutablePlaceholderArray;
}

// Native __NSArrayMutablePlaceholder, 0x1c0161f68. Hidden (was a private external). Answers the mutable
// placeholder object. Evidence: iphone-simulator-corpus-delta8.json.
id __NSArrayMutablePlaceholder(void) {
    return (id)&__mutablePlaceholderArray;
}

// Native -[__NSPlaceholderArray init], 0x1c0161f74. Non-external. The immutable placeholder answers its
// -initWithObjects:NULL count:0, the mutable one its -initWithCapacity:0 (tail calls, the placeholder object
// as the receiver); any other receiver is BRK #1.
- (id)init {
    if (self == immutablePlaceholder()) {
        return [immutablePlaceholder() initWithObjects:NULL count:0];
    }
    if (self != mutablePlaceholder()) {
        __builtin_trap();
    }
    return [mutablePlaceholder() initWithCapacity:0];
}

// Native -[__NSPlaceholderArray initWithArray:], 0x1c0161fbc. Non-external. __initWithArrayDontCopyItems, or
// when that answers nil, super's -initWithArray:copyItems: NO (selector reference 0x2387af641, outside the
// extracted image; the selector follows from its arguments and from -initWithArray:copyItems:, which sends
// it with its own).
- (id)initWithArray:(id)array {
    id result = __initWithArrayDontCopyItems(self, array);
    if (result != nil) {
        return result;
    }
    return [super initWithArray:array copyItems:NO];
}

// Native __initWithArrayDontCopyItems, 0x1c0162040. Non-external. An array whose class (objc_opt_class) is
// __NSArrayI, __NSArrayM or __NSFrozenArrayM answers its -mutableCopyWithZone:NULL for the mutable
// placeholder or its -copyWithZone:NULL for the immutable one (tail calls); anything else answers nil.
static id __initWithArrayDontCopyItems(id placeholder, id array) {
    Class cls = [array class];
    if (cls == (Class)(void *)arrayIClass || cls == (Class)(void *)arrayMClass ||
        cls == (Class)(void *)frozenArrayClass) {
        if (placeholder == mutablePlaceholder()) {
            return [array mutableCopyWithZone:NULL];
        }
        if (placeholder == immutablePlaceholder()) {
            return [array copyWithZone:NULL];
        }
    }
    return nil;
}

// Native -[__NSPlaceholderArray initWithArray:copyItems:], 0x1c01620fc. Non-external. Without bit 0 of
// copyItems, __initWithArrayDontCopyItems first; otherwise, or when that answers nil, super's
// -initWithArray:copyItems: with the same arguments.
- (id)initWithArray:(id)array copyItems:(BOOL)copyItems {
    if (!(copyItems & 1)) {
        id result = __initWithArrayDontCopyItems(self, array);
        if (result != nil) {
            return result;
        }
    }
    return [super initWithArray:array copyItems:copyItems];
}

// Native -[__NSPlaceholderArray initWithObjects:count:], 0x1c0162194. Non-external. Order: a NULL buffer with
// a nonzero count, a count of 2^61 or more, a nil object ("attempt to insert nil object from objects[%lu]";
// buffer 0x16 each). The immutable placeholder answers __NSSingleObjectArrayI_new for one object,
// __NSArrayI_new (no variable arguments) for more, __NSArray0__ retained for none; the mutable one
// __NSArrayM_new (all retaining; tail calls); any other receiver is BRK #1.
- (id)initWithObjects:(const id *)objects count:(NSUInteger)count {
    if (objects == NULL && count != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            methodName("initWithObjects:count:"),
            (unsigned long)count
        );
    }
    if ((OF_WORD_SHIFT(count, 61)) != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            methodName("initWithObjects:count:"),
            (unsigned long)count
        );
    }
    for (NSUInteger index = 0; index < count; index++) {
        if (objects[index] == nil) {
            RAISE_LOGGED_EXCEPTION(
                ONSInvalidArgumentException,
                0x16,
                "*** %s: attempt to insert nil object from objects[%lu]",
                methodName("initWithObjects:count:"),
                (unsigned long)index
            );
        }
    }
    if (self == immutablePlaceholder()) {
        if (count == 1) {
            return __NSSingleObjectArrayI_new(objects[0], NO);
        }
        if (count != 0) {
            return __NSArrayI_new(objects, NULL, count, NO);
        }
        return [__ONSArray0__ retain];
    }
    if (self != mutablePlaceholder()) {
        __builtin_trap();
    }
    return __NSArrayM_new(objects, count, NO);
}

// Native -[__NSPlaceholderArray _initByAdoptingBuffer:count:size:], 0x1c01624d4. Non-external. The immutable
// placeholder answers __NSArrayI_transferNew, the mutable one __NSArrayM_transferNew, of the buffer, count
// and size (tail calls); any other receiver answers nil.
- (id)_initByAdoptingBuffer:(id *)buffer count:(NSUInteger)count size:(NSUInteger)size {
    if (self == immutablePlaceholder()) {
        return __NSArrayI_transferNew(buffer, count, size);
    }
    if (self == mutablePlaceholder()) {
        return __NSArrayM_transferNew(buffer, count, size);
    }
    return nil;
}

// Native -[__NSPlaceholderArray initWithCapacity:], 0x1c016251c. Non-external. A capacity of 2^61 or more
// raises ("*** %s: capacity (%lu) is ridiculous", buffer 0x16). The immutable placeholder sends itself
// -doesNotRecognizeSelector: with the selector and then BRK #1; the mutable one answers __NSArrayM_new with
// no objects and the capacity (a tail call); any other receiver is BRK #1.
- (id)initWithCapacity:(NSUInteger)capacity {
    if ((OF_WORD_SHIFT(capacity, 61)) != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: capacity (%lu) is ridiculous",
            methodName("initWithCapacity:"),
            (unsigned long)capacity
        );
    }
    if (self == immutablePlaceholder()) {
        [immutablePlaceholder() doesNotRecognizeSelector:_cmd];
        __builtin_trap();
    }
    if (self != mutablePlaceholder()) {
        __builtin_trap();
    }
    return __NSArrayM_new(NULL, capacity, NO);
}

// Native -[__NSPlaceholderArray initWithContentsOfFile:], 0x1c0162684. Non-external. A path that is not nil
// and not an NSString (_NSIsNSString) raises ("*** %s: string argument is not an NSString", buffer 0xC);
// otherwise +[NSArray newWithContentsOf:immutable:] with the path and whether the receiver is the immutable
// placeholder (a tail call).
- (id)initWithContentsOfFile:(id)path {
    if (path != nil && !_ONSIsNSString(path)) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: string argument is not an NSString",
            methodName("initWithContentsOfFile:")
        );
    }
    return [(id)arrayClass newWithContentsOf:path immutable:self == immutablePlaceholder()];
}

// Native -[__NSPlaceholderArray initWithContentsOfURL:], 0x1c01627c8. Non-external. The NSURL class is
// looked up once (objc_lookUpClass("NSURL"), spelled with the selected class name) into the static
// initWithContentsOfURL:.__cls_NSURL (load-acquire, store-release). A URL that is not nil and not a kind of
// it (objc_opt_isKindOfClass) raises ("*** %s: url argument is not an NSURL", buffer 0xC); otherwise
// +[NSArray newWithContentsOf:immutable:] with the URL and whether the receiver is the immutable placeholder
// (a tail call).
- (id)initWithContentsOfURL:(id)url {
    // The cached class, kept as a pointer word for the atomic builtins.
    static void *__cls_NSURL;
    if (__atomic_load_n(&__cls_NSURL, __ATOMIC_ACQUIRE) == NULL) {
        __atomic_store_n(&__cls_NSURL, (void *)objc_lookUpClass(OF_NAME_STRING(ONSURL)), __ATOMIC_RELEASE);
    }
    if (url != nil && ![url isKindOfClass:(Class)__atomic_load_n(&__cls_NSURL, __ATOMIC_ACQUIRE)]) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: url argument is not an NSURL",
            methodName("initWithContentsOfURL:")
        );
    }
    return [(id)arrayClass newWithContentsOf:url immutable:self == immutablePlaceholder()];
}

// Native -[__NSPlaceholderArray retain], 0x1c0162938. Non-external. The receiver (a bare RET; the capture
// lists its one instruction, IDA has no function there).
- (instancetype)retain {
    return self;
}

// Native -[__NSPlaceholderArray release], 0x1c016293c. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[__NSPlaceholderArray retainCount], 0x1c0162940. Non-external. Answers NSUIntegerMax.
- (NSUInteger)retainCount {
    return NSUIntegerMax;
}

// Native +[__NSPlaceholderArray allocWithZone:], 0x1c03063a4 (cold region). Non-external. BRK #1.
+ (id)allocWithZone:(struct _NSZone *)zone {
    __builtin_trap();
}

// Native -[__NSPlaceholderArray dealloc], 0x1c03063a8 (cold region). Non-external. BRK #1; it never reaches
// super's -dealloc, so the missing-super-call diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"

- (void)dealloc {
    __builtin_trap();
}

#pragma clang diagnostic pop

@end

//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCFArray.h"
#include "CoreFoundation/Base/OCFUtilities.h"

#include <alloca.h>
#include <mach-o/loader.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <objc/objc-exception.h>
#include <stddef.h>

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// The NSException class symbol (_OBJC_CLASS_$_NSException), sent to directly.
extern char exceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));

// Native _NSArrayRaiseBoundException, 0x1c030aea0; defined after the class, in native address order.
static __attribute__((noreturn)) void
_NSArrayRaiseBoundException(const void *object, SEL selector, OCFIndex index, OCFIndex count);

// Native _NSArrayRaiseInsertNilException, 0x1c030afc0; defined after the class, in native address order.
static __attribute__((noreturn)) void _NSArrayRaiseInsertNilException(const void *object, SEL selector);

// The mutability raise of the three mutation fragments.
// raiseImmutableArray is a local descriptive name.
static __attribute__((noreturn)) void raiseImmutableArray(id object, SEL selector) {
    _CFThrowFormattedException(
        ONSInternalInconsistencyException,
        OF_STR("%@: mutating method sent to immutable object"),
        __CFExceptionProem(object, selector)
    );
}

// Inferred organization: the __NSCFArray methods (0x1c01ccce4-0x1c01cd698) lie together in the iOS Simulator
// __text section, interleaved with CFArrayCreateCopy and CFArrayCreateMutableCopy (restored in
// Collections/OCFArray.c); following the owner's decision, the class has its own translation unit.
// Objective-C compiled with manual reference counting (objc_retain, objc_autorelease).
// -insertObject:atIndex:, -replaceObjectAtIndex:withObject: and -removeObjectAtIndex: are restored from the
// IDA capture of their .cold fragments (iphone-simulator-corpus-delta2.json); the mutability fragments end in
// OUTLINED_FUNCTION_2_13 (0x1c01cd9f0: NSInternalInconsistencyException into X19, then
// __CFExceptionProem(X0, X1)) and _1_15 (0x1c01cd9dc: the raise with "%@: mutating method sent to immutable
// object"), read in iphone-simulator-corpus-delta3.json (confidence: verified). After the class come the
// unit's statics PROEM_CLS and PROEM_SEL (0x1c01cd924, 0x1c01cd94c: they follow CFArrayCreateMutable, which
// with CFArrayCreate and the copy creators is restored in Collections/OCFArray.c, and precede this unit's
// os_log helper and OUTLINED_FUNCTION pieces, so they belong to the __NSCFArray unit, whose only callers are
// the two raises below), then the cold-region statics _NSArrayRaiseBoundException (0x1c030aea0) and
// _NSArrayRaiseInsertNilException (0x1c030afc0), from the IDA capture iphone-simulator-corpus-delta3.json.
// Unless
// a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

@implementation __ONSCFArray

// Native -[__NSCFArray count], 0x1c01ccce4. Non-external.
- (NSUInteger)count {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableArray);
    return (NSUInteger)_CFNonObjCArrayGetCount((OCFArrayRef)self);
}

// Native -[__NSCFArray objectAtIndex:], 0x1c01ccd30. Non-external. An index out of bounds raises through
// _NSArrayRaiseBoundException with the count.
- (id)objectAtIndex:(NSUInteger)index {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableArray);
    Boolean outOfBounds = 0;
    const void *value = _OCFArrayCheckAndGetValueAtIndex((OCFArrayRef)self, (OCFIndex)index, &outOfBounds);
    if (outOfBounds) {
        _NSArrayRaiseBoundException(self, _cmd, (OCFIndex)index, _CFNonObjCArrayGetCount((OCFArrayRef)self));
    }
    return (id)value;
}

// Native -[__NSCFArray objectAtIndexedSubscript:], 0x1c01ccdf4. Non-external. The same body as
// -objectAtIndex:.
- (id)objectAtIndexedSubscript:(NSUInteger)index {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableArray);
    Boolean outOfBounds = 0;
    const void *value = _OCFArrayCheckAndGetValueAtIndex((OCFArrayRef)self, (OCFIndex)index, &outOfBounds);
    if (outOfBounds) {
        _NSArrayRaiseBoundException(self, _cmd, (OCFIndex)index, _CFNonObjCArrayGetCount((OCFArrayRef)self));
    }
    return (id)value;
}

// Native -[__NSCFArray getObjects:range:], 0x1c01cceb8. Non-external. A negative location or length, or an
// end past the count (unsigned), raises with the last index of the range; an empty range copies nothing.
- (void)getObjects:(id *)objects range:(OCFRange)range {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableArray);
    OCFIndex count = _CFNonObjCArrayGetCount((OCFArrayRef)self);
    OCFIndex end = range.location + range.length;
    if (range.location < 0 || range.length < 0 || (uintptr_t)count < (uintptr_t)end) {
        _NSArrayRaiseBoundException(self, _cmd, end - 1, count);
    }
    if (range.length == 0) {
        return;
    }
    OCFArrayGetValues((OCFArrayRef)self, range, (const void **)objects);
}

// Native +[__NSCFArray automaticallyNotifiesObserversForKey:], 0x1c01ccf78. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSCFArray countByEnumeratingWithState:objects:count:], 0x1c01ccf80. Non-external.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableArray);
    return _OCFArrayFastEnumeration((OCFArrayRef)self, state, buffer, length);
}

// Native -[__NSCFArray addObject:], 0x1c01ccfec. Non-external. -insertObject:atIndex: at the count.
- (void)addObject:(id)object {
    OCFIndex count = _CFNonObjCArrayGetCount((OCFArrayRef)self);
    [self insertObject:object atIndex:(NSUInteger)count];
}

// Native -[__NSCFArray removeLastObject], 0x1c01cd01c. Non-external. Apps linked on or after version 7 do
// nothing for an empty array; older apps send -removeObjectAtIndex: 0 for it.
- (void)removeLastObject {
    OCFIndex count = _CFNonObjCArrayGetCount((OCFArrayRef)self);
    NSUInteger index;
    if (_OCFExecutableLinkedOnOrAfter(7)) {
        if (count == 0) {
            return;
        }
        index = (NSUInteger)count - 1;
    } else {
        index = count == 0 ? 0 : (NSUInteger)count - 1;
    }
    [self removeObjectAtIndex:index];
}

// Native -[__NSCFArray insertObject:atIndex:], 0x1c01cd070, with its .cold.1 (0x1c030b0ac) and .cold.2
// (0x1c030b0c4). Non-external. Order: the mutability check (.cold.1), the write check with the method's
// return address, the nil check (_NSArrayRaiseInsertNilException), the bound check (an index above the count
// raises with the count plus one, .cold.2), then _CFArrayReplaceValues of one value at the index.
- (void)insertObject:(id)object atIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    id value = object;
    if (!_OCFArrayIsMutable((OCFArrayRef)self)) {
        raiseImmutableArray(self, _cmd);
    }
    tsanWrite(self, returnAddress, __OCFTSANTagMutableArray);
    if (object == nil) {
        _NSArrayRaiseInsertNilException(self, _cmd);
    }
    if ((NSUInteger)_CFNonObjCArrayGetCount((OCFArrayRef)self) < index) {
        _NSArrayRaiseBoundException(
            self, _cmd, (OCFIndex)index, _CFNonObjCArrayGetCount((OCFArrayRef)self) + 1
        );
    }
    _OCFArrayReplaceValues(
        (OCFMutableArrayRef)self, (OCFRange){(OCFIndex)index, 0}, (const void **)&value, 1
    );
}

// Native -[__NSCFArray replaceObjectAtIndex:withObject:], 0x1c01cd16c, with its .cold.1 (0x1c030b0f8).
// Non-external. As -insertObject:atIndex:, but an index at or above the count raises with the count, and the
// value is set through CFArraySetValueAtIndex (a tail call).
- (void)replaceObjectAtIndex:(NSUInteger)index withObject:(id)object {
    void *returnAddress = __builtin_return_address(0);
    if (!_OCFArrayIsMutable((OCFArrayRef)self)) {
        raiseImmutableArray(self, _cmd);
    }
    tsanWrite(self, returnAddress, __OCFTSANTagMutableArray);
    if (object == nil) {
        _NSArrayRaiseInsertNilException(self, _cmd);
    }
    if ((NSUInteger)_CFNonObjCArrayGetCount((OCFArrayRef)self) <= index) {
        _NSArrayRaiseBoundException(self, _cmd, (OCFIndex)index, _CFNonObjCArrayGetCount((OCFArrayRef)self));
    }
    OCFArraySetValueAtIndex((OCFMutableArrayRef)self, (OCFIndex)index, object);
}

// Native -[__NSCFArray removeObjectAtIndex:], 0x1c01cd230, with its .cold.1 (0x1c030b110). Non-external. The
// mutability check, the write check, the bound check (raising with the count), then _CFArrayReplaceValues of
// one value with none (a tail call).
- (void)removeObjectAtIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    if (!_OCFArrayIsMutable((OCFArrayRef)self)) {
        raiseImmutableArray(self, _cmd);
    }
    tsanWrite(self, returnAddress, __OCFTSANTagMutableArray);
    if ((NSUInteger)_CFNonObjCArrayGetCount((OCFArrayRef)self) <= index) {
        _NSArrayRaiseBoundException(self, _cmd, (OCFIndex)index, _CFNonObjCArrayGetCount((OCFArrayRef)self));
    }
    _OCFArrayReplaceValues((OCFMutableArrayRef)self, (OCFRange){(OCFIndex)index, 1}, NULL, 0);
}

// Native -[__NSCFArray classForCoder], 0x1c01cd2e0. Non-external. The class references of NSMutableArray and
// NSArray (0x2227ce360, 0x2227ce2a0), sent +self.
- (Class)classForCoder {
    if (_OCFArrayIsMutable((OCFArrayRef)self)) {
        return [ONSMutableArray self];
    }
    return [ONSArray self];
}

// Native +[__NSCFArray allocWithZone:], 0x1c01cd310. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}

// Native -[__NSCFArray isEqual:], 0x1c01cd32c. Non-external. nil answers NO and the receiver itself YES
// before _CFNonObjCEqual.
- (BOOL)isEqual:(id)object {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableArray);
    if (object == nil) {
        return NO;
    }
    if (self == object) {
        return YES;
    }
    return _OCFNonObjCEqual(self, object) != 0;
}

// Native -[__NSCFArray hash], 0x1c01cd3a8. Non-external.
- (NSUInteger)hash {
    return _OCFNonObjCHash(self);
}

// Native -[__NSCFArray description], 0x1c01cd3ac. Non-external. Arrays whose copy-description callback is
// neither NULL nor CFCopyDescription describe through CF; the rest go to super.
- (id)description {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableArray);
    OCFArrayCopyDescriptionCallBack copyDescription =
        __CFArrayGetCallBacks((OCFArrayRef)self)->copyDescription;
    if (copyDescription != OCFCopyDescription && copyDescription != NULL) {
        return [(id)OCFCopyDescription(self) autorelease];
    }
    return [super description];
}

// Native -[__NSCFArray retain], 0x1c01cd494. Non-external.
- (id)retain {
    return (id)_OCFNonObjCRetain(self);
}

// Native -[__NSCFArray release], 0x1c01cd498. Non-external.
- (oneway void)release {
    _OCFNonObjCRelease(self);
}

// Native -[__NSCFArray _tryRetain], 0x1c01cd49c. Non-external.
- (BOOL)_tryRetain {
    return _OCFTryRetain(self) != NULL;
}

// Native -[__NSCFArray _isDeallocating], 0x1c01cd4b8. Non-external.
- (BOOL)_isDeallocating {
    return _OCFIsDeallocating(self) != 0;
}

// Native -[__NSCFArray retainCount], 0x1c01cd4d4. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)OCFGetRetainCount(self);
}

// Native -[__NSCFArray copyWithZone:], 0x1c01cd4d8. Non-external. A mutable array is copied (NULL allocator);
// an immutable one is retained.
- (id)copyWithZone:(struct _NSZone *)zone {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableArray);
    if (_OCFArrayIsMutable((OCFArrayRef)self)) {
        return (id)OCFArrayCreateCopy(NULL, (OCFArrayRef)self);
    }
    return [self retain];
}

// Native -[__NSCFArray mutableCopyWithZone:], 0x1c01cd648. Non-external.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    tsanRead(self, __builtin_return_address(0), __OCFTSANTagMutableArray);
    return (id)OCFArrayCreateMutableCopy(NULL, 0, (OCFArrayRef)self);
}

@end

// Native PROEM_CLS, 0x1c01cd924. Non-external (a static of this unit). The class name of the object's class
// (objc_opt_class, then class_getName as a tail call), or "(null class)" when that class is nil. Evidence:
// iphone-simulator-corpus-delta3.json.
static const char *PROEM_CLS(const void *object) {
    Class cls = [(id)object class];
    if (cls == Nil) {
        return "(null class)";
    }
    return class_getName(cls);
}

// Native PROEM_SEL, 0x1c01cd94c. Non-external (a static of this unit; the image holds other units' copies at
// 0x1c01e83d4 and 0x1c01f73f8). The selector's name (sel_getName as a tail call), or "(null selector)" for a
// NULL selector. Evidence: iphone-simulator-corpus-delta3.json.
static const char *PROEM_SEL(SEL selector) {
    if (selector == NULL) {
        return "(null selector)";
    }
    return sel_getName(selector);
}

// Native _NSArrayRaiseBoundException, 0x1c030aea0. Non-external. Builds an os_log pack (_os_log_pack_size of
// the 52-byte format buffer, on the stack, filled with errno 0 and the image's header) whose arguments are
// the PROEM_CLS and PROEM_SEL strings, the index, " (or possibly larger)" when the index's top two bits are
// not both clear (else ""), and the count; the native argument packing is the compiler's
// __os_log_helper_1_2_5_8_32_8_32_8_0_8_32_8_0 (0x1c01cd960), which __builtin_os_log_format generates.
// PROEM_CLS and PROEM_SEL are then called again for the reason, made with kCFAllocatorSystemDefault, and
// +[NSException exceptionWithName:reason:userInfo:osLogPack:size:] (NSInvalidArgumentException, the reason
// autoreleased through _CFAutoreleasePoolAddObject, no user info, the pack) is thrown. The exception call is
// OUTLINED_FUNCTION_5_5 (0x1c01cda38: _CFAutoreleasePoolAddObject(NULL, reason)) and _3_7 (0x1c01cda00: the
// NSException class, NSInvalidArgumentException, the reason, nil, the pack and its size) and the pack fill
// OUTLINED_FUNCTION_4_5 (0x1c01cda28: _os_log_pack_fill(pack, size, 0, ...)), read in
// iphone-simulator-corpus-delta3.json (confidence: verified).
static void _NSArrayRaiseBoundException(const void *object, SEL selector, OCFIndex index, OCFIndex count) {
    size_t size = _os_log_pack_size(0x34);
    void *pack = alloca((size + 15) & ~(size_t)15);
    uint8_t *buffer =
        _os_log_pack_fill(pack, size, 0, &__dso_handle, "-[%s %s]: index (%ld%s) beyond bounds (%ld)");
    const char *className = PROEM_CLS(object);
    const char *selectorName = PROEM_SEL(selector);
    const char *larger = (OF_WORD_SHIFT((uintptr_t)index, 62)) != 0 ? " (or possibly larger)" : "";
    __builtin_os_log_format(
        buffer, "-[%s %s]: index (%ld%s) beyond bounds (%ld)", className, selectorName, index, larger, count
    );
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    const char *reasonClassName = PROEM_CLS(object);
    const char *reasonSelectorName = PROEM_SEL(selector);
    OCFStringRef reason = OCFStringCreateWithFormat(
        allocator,
        NULL,
        OF_STR("-[%s %s]: index (%ld%s) beyond bounds (%ld)"),
        reasonClassName,
        reasonSelectorName,
        index,
        larger,
        count
    );
    id exception = ((id (*)(id, SEL, id, id, id, void *, size_t))objc_msgSend)(
        (id)exceptionClass,
        sel_getUid("exceptionWithName:reason:userInfo:osLogPack:size:"),
        (id)ONSInvalidArgumentException,
        _OCFAutoreleasePoolAddObject(NULL, reason),
        nil,
        pack,
        size
    );
    objc_exception_throw(exception);
}

// Native _NSArrayRaiseInsertNilException, 0x1c030afc0. Non-external. The pattern of
// _NSArrayRaiseBoundException with the format "-[%s %s]: attempt to insert nil": _os_log_pack_size(0x16), the
// pack on the stack filled through OUTLINED_FUNCTION_4_5 (errno 0, the image's header), PROEM_CLS and
// PROEM_SEL stored into the pack's two string arguments (the compiler's inline __builtin_os_log_format
// encoding: header 0x02 0x02, then 0x20 0x08 and the pointer, twice), PROEM_CLS and PROEM_SEL again for the
// reason (kCFAllocatorSystemDefault), then OUTLINED_FUNCTION_5_5 and _3_7 (the autoreleased reason,
// NSInvalidArgumentException, no user info, the pack and its size) and objc_exception_throw. Evidence:
// iphone-simulator-corpus-delta3.json.
static void _NSArrayRaiseInsertNilException(const void *object, SEL selector) {
    size_t size = _os_log_pack_size(0x16);
    void *pack = alloca((size + 15) & ~(size_t)15);
    uint8_t *buffer = _os_log_pack_fill(pack, size, 0, &__dso_handle, "-[%s %s]: attempt to insert nil");
    const char *className = PROEM_CLS(object);
    const char *selectorName = PROEM_SEL(selector);
    __builtin_os_log_format(buffer, "-[%s %s]: attempt to insert nil", className, selectorName);
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    const char *reasonClassName = PROEM_CLS(object);
    const char *reasonSelectorName = PROEM_SEL(selector);
    OCFStringRef reason = OCFStringCreateWithFormat(
        allocator, NULL, OF_STR("-[%s %s]: attempt to insert nil"), reasonClassName, reasonSelectorName
    );
    id exception = ((id (*)(id, SEL, id, id, id, void *, size_t))objc_msgSend)(
        (id)exceptionClass,
        sel_getUid("exceptionWithName:reason:userInfo:osLogPack:size:"),
        (id)ONSInvalidArgumentException,
        _OCFAutoreleasePoolAddObject(NULL, reason),
        nil,
        pack,
        size
    );
    objc_exception_throw(exception);
}

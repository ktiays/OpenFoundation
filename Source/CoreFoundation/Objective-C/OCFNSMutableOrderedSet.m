//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSMutableOrderedSet.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFSortFunctions.h"
#include "CoreFoundation/Objective-C/OCFNSArray.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <alloca.h>
#include <mach-o/loader.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdlib.h>

// Inferred organization: the NSMutableOrderedSet methods (0x1c017d794-0x1c0189373) lie together in the iOS
// Simulator __text section with the block of -sortRange:options:usingComparator: and the unit's
// OUTLINED_FUNCTION_0_4 (0x1c0189374), _1_3 (0x1c0189388) and _2_2 (0x1c0189390); the four abstract methods
// -insertObject:atIndex:, -removeObjectAtIndex:, -replaceObjectAtIndex:withObject: and -initWithCapacity: lie
// in the cold region (0x1c03079e0-0x1c0307a5b). Following the owner's decision, the class has its own
// translation unit; the methods follow in native address order, the outlined fragments inside the methods
// that call them. Objective-C compiled with manual reference counting (objc_retain, objc_release and
// objc_autorelease calls, no ARC entry points). Every method of the class that the image holds is restored
// here; the evidence is the IDA capture iphone-simulator-corpus-delta11.json (family K-mutable-ordered-set).
// Most methods match the NSMutableArray method of the same selector instruction for instruction
// (Objective-C/OCFNSMutableArray.m), with the ordered set's names, ThreadSanitizer tag and "ordered set"
// reasons; the others are named at their definitions. Unless a definition says otherwise: iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device
// 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written
// yet).
//
// Shared shape of the mutation methods: each sends -_mutate to the receiver first, then calls the
// ThreadSanitizer write hook (when set) with the receiver, the method's return address (X30 at entry) and
// __CFTSANTagMutableOrderedSet, then checks its arguments in the order written. Argument errors raise through
// +[NSException exceptionWithName:reason:userInfo:osLogPack:size:] with an os_log pack built on the stack:
// _os_log_pack_size of the format's buffer size (12 for "%s", 22 for "%s %lu", 32 for three arguments, 42
// for four), the pack filled through _os_log_pack_fill (errno 0, the image's mach header, the format), the
// arguments encoded by the compiler's __builtin_os_log_format (natively inline for one or two arguments,
// __os_log_helper_1_2_3_8_32_8_0_8_0 and __os_log_helper_1_2_4_8_32_8_0_8_0_8_0 for three and four), and the
// reason made with kCFAllocatorSystemDefault and the same format as a constant string, autoreleased through
// _CFAutoreleasePoolAddObject, with no user info. The first format argument is the method's name as a
// literal C string ("-[NSMutableOrderedSet addObject:]"), spelled here with the class's selected name so
// that the system-name build carries the native bytes. The temporary object buffers follow the native
// new_id_array pattern: a count with any of the top four bits set throws NSGenericException with a
// CFRelease'd reason; otherwise up to 256 objects live on the stack (at least one slot) and larger counts use
// _CFCreateArrayStorage, freed (free(NULL) for the stack case) after use. Fast enumeration (for ... in) is
// the compiler's: a 16-object stack buffer, the mutation word saved after the first batch and checked before
// every object (objc_enumerationMutation).

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// libobjc's super send with the current class (the entry point [super ...] compiles to); the SDK installs
// no declaration. Supplied, never reimplemented.
extern id objc_msgSendSuper2(struct objc_super *super, SEL op, ...);

// Class symbols the methods use directly (_OBJC_CLASS_$_NSException, _OBJC_CLASS_$_NSMutableOrderedSet).
extern char exceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char mutableOrderedSetClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableOrderedSet));

// Selectors the methods send to their arguments and to classes that the restored classes of this unit do
// not declare (NSArray's, NSIndexSet's, NSSet's and NSException's); declared for the message sends only. The
// category below adopts it so that the receiver's own sends resolve; neither a protocol object nor a
// category is emitted.
@protocol MutableOrderedSetMessages

+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
+ (id)exceptionWithName:(id)name
                 reason:(id)reason
               userInfo:(id)userInfo
              osLogPack:(void *)pack
                   size:(size_t)size;
- (NSUInteger)count;
- (id)objectAtIndex:(NSUInteger)index;
- (void)getObjects:(id *)objects range:(OCFRange)range;
- (void)getObjects:(id *)objects count:(NSUInteger)count;
- (NSUInteger)countForObject:(id)object;
- (NSUInteger)lastIndex;
- (NSUInteger)rangeCount;
- (OCFRange)rangeAtIndex:(NSUInteger)index;
- (NSUInteger)countOfIndexesInRange:(OCFRange)range;

@end

@interface ONSMutableOrderedSet (MutableOrderedSetMessages) <MutableOrderedSetMessages>
@end

// The ThreadSanitizer write hook every mutation method inlines: load the hook, and when it is set, call it
// with the receiver, the method's return address and the tag. tsanWriteCheck is a local descriptive name.
static inline void tsanWriteCheck(const void *object, void *returnAddress, void *tag) {
    void (*hook)(const void *, void *, void *) =
        (void (*)(const void *, void *, void *))__cf_tsanWriteFunction;
    if (hook != NULL) {
        hook(object, returnAddress, tag);
    }
}

// The method name every reason starts with, "-[NSMutableOrderedSet selector]" in the system-name build.
#define methodName(selectorText) "-[" OF_NAME_STRING(ONSMutableOrderedSet) " " selectorText "]"

// The inlined os_log pack raise (see the header comment); a statement macro so that the pack stays in the
// method's frame, as natively. raiseWithLogPack is a local descriptive name.
#define raiseWithLogPack(exceptionName, bufferSize, format, ...)                                             \
    do {                                                                                                     \
        size_t packSize = _os_log_pack_size(bufferSize);                                                     \
        void *pack = alloca((packSize + 15) & ~(size_t)15);                                                  \
        uint8_t *packBuffer = _os_log_pack_fill(pack, packSize, 0, &__dso_handle, format);                   \
        __builtin_os_log_format(packBuffer, format, __VA_ARGS__);                                            \
        OCFStringRef reason =                                                                                \
            OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR(format), __VA_ARGS__);        \
        id exception = [(id)exceptionClass exceptionWithName:(id)(exceptionName)                             \
                                                      reason:_OCFAutoreleasePoolAddObject(NULL, reason)      \
                                                    userInfo:nil                                             \
                                                   osLogPack:pack                                            \
                                                        size:packSize];                                      \
        objc_exception_throw(exception);                                                                     \
    } while (0)

// The inlined index check failure: "beyond bounds [0 .. count - 1]" for a nonempty ordered set, "beyond
// bounds for empty ordered set" otherwise (NSRangeException).
#define raiseIndexBeyondBounds(method, index, count)                                                         \
    do {                                                                                                     \
        NSUInteger boundCount = (count);                                                                     \
        if (boundCount != 0) {                                                                               \
            NSUInteger lastIndex = boundCount - 1;                                                           \
            raiseWithLogPack(                                                                                \
                ONSRangeException,                                                                           \
                32,                                                                                          \
                "*** %s: index %lu beyond bounds [0 .. %lu]",                                                \
                method,                                                                                      \
                index,                                                                                       \
                lastIndex                                                                                    \
            );                                                                                               \
        }                                                                                                    \
        raiseWithLogPack(                                                                                    \
            ONSRangeException, 22, "*** %s: index %lu beyond bounds for empty ordered set", method, index    \
        );                                                                                                   \
    } while (0)

// The inlined range check failure (NSRangeException), as raiseIndexBeyondBounds for a range.
#define raiseRangeBeyondBounds(method, location, length, count)                                              \
    do {                                                                                                     \
        NSUInteger boundCount = (count);                                                                     \
        if (boundCount != 0) {                                                                               \
            NSUInteger lastIndex = boundCount - 1;                                                           \
            raiseWithLogPack(                                                                                \
                ONSRangeException,                                                                           \
                42,                                                                                          \
                "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",                                 \
                method,                                                                                      \
                location,                                                                                    \
                length,                                                                                      \
                lastIndex                                                                                    \
            );                                                                                               \
        }                                                                                                    \
        raiseWithLogPack(                                                                                    \
            ONSRangeException,                                                                               \
            32,                                                                                              \
            "*** %s: range {%lu, %lu} extends beyond bounds for empty ordered set",                          \
            method,                                                                                          \
            location,                                                                                        \
            length                                                                                           \
        );                                                                                                   \
    } while (0)

// The inlined index set check failure (NSRangeException): the index set's last index against the bound.
#define raiseIndexSetBeyondBounds(method, index, count)                                                      \
    do {                                                                                                     \
        NSUInteger boundCount = (count);                                                                     \
        if (boundCount != 0) {                                                                               \
            NSUInteger lastIndex = boundCount - 1;                                                           \
            raiseWithLogPack(                                                                                \
                ONSRangeException,                                                                           \
                32,                                                                                          \
                "*** %s: index %lu in index set beyond bounds [0 .. %lu]",                                   \
                method,                                                                                      \
                index,                                                                                       \
                lastIndex                                                                                    \
            );                                                                                               \
        }                                                                                                    \
        raiseWithLogPack(                                                                                    \
            ONSRangeException,                                                                               \
            22,                                                                                              \
            "*** %s: index %lu in index set beyond bounds for empty ordered set",                            \
            method,                                                                                          \
            index                                                                                            \
        );                                                                                                   \
    } while (0)

// The inlined new_id_array failure: NSGenericException with a CFRelease'd reason made with the default
// allocator.
#define raiseTemporaryBufferTooLarge(count)                                                                  \
    do {                                                                                                     \
        OCFStringRef reason = OCFStringCreateWithFormat(                                                     \
            NULL,                                                                                            \
            NULL,                                                                                            \
            OF_STR(                                                                                          \
                "*** attempt to create a temporary id buffer which is too large or with a negative count "   \
                "(%lu) -- possibly data is corrupt"                                                          \
            ),                                                                                               \
            count                                                                                            \
        );                                                                                                   \
        id exception = [(id)exceptionClass exceptionWithName:(id)ONSGenericException                         \
                                                      reason:(id)reason                                      \
                                                    userInfo:nil];                                           \
        OCFRelease(reason);                                                                                  \
        objc_exception_throw(exception);                                                                     \
    } while (0)

// The cleanup of the temporary buffers whose landing pads free them before resuming (-intersectOrderedSet:
// and -sortRange:options:usingComparator:). releaseTemporaryStorage is a local descriptive name.
static inline void releaseTemporaryStorage(void **storage) {
    free(*storage);
}

@implementation ONSMutableOrderedSet

// Native -[NSMutableOrderedSet _mutate], 0x1c017d794. Non-external. Does nothing.
- (void)_mutate {
}

// Native -[NSMutableOrderedSet addObject:], 0x1c017d798. Non-external. A nil object raises
// NSInvalidArgumentException; otherwise -insertObject:atIndex: at the count (a tail call).
- (void)addObject:(id)object {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (object == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException, 12, "*** %s: object cannot be nil", methodName("addObject:")
        );
    }
    [self insertObject:object atIndex:[self count]];
}

// Native -[NSMutableOrderedSet addObjects:count:], 0x1c017d8fc. Non-external. A NULL objects pointer with a
// nonzero count, a count with any of the top three bits set and a nil object (the first, by index) raise
// NSInvalidArgumentException in that order; then -insertObjects:count:atIndex: at the count.
- (void)addObjects:(const id *)objects count:(NSUInteger)count {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (objects == NULL && count != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            22,
            "*** %s: pointer to objects array is NULL but length is %lu",
            methodName("addObjects:count:"),
            count
        );
    }
    if (count >> 61 != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            22,
            "*** %s: count (%lu) of objects array is ridiculous",
            methodName("addObjects:count:"),
            count
        );
    }
    for (NSUInteger index = 0; index < count; ++index) {
        if (objects[index] == nil) {
            raiseWithLogPack(
                ONSInvalidArgumentException,
                22,
                "*** %s: attempt to insert nil object from objects[%lu]",
                methodName("addObjects:count:"),
                index
            );
        }
    }
    [self insertObjects:objects count:count atIndex:[self count]];
}

// Native -[NSMutableOrderedSet addObjectsFromArray:range:], 0x1c017dbec. Non-external. A non-nil argument
// that is not an NSArray raises NSInvalidArgumentException; a range outside the argument's count (negative
// location or length, or an end above the count) raises NSRangeException; then
// -insertObjectsFromArray:range:atIndex: at the receiver's count (a tail call).
- (void)addObjectsFromArray:(id)array range:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: array argument is not an NSArray",
            methodName("addObjectsFromArray:range:")
        );
    }
    NSUInteger otherCount = [array count];
    if (range.location < 0 || range.length < 0 ||
        otherCount < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("addObjectsFromArray:range:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            otherCount
        );
    }
    [self insertObjectsFromArray:array range:range atIndex:[self count]];
}

// Native -[NSMutableOrderedSet addObjectsFromArray:], 0x1c017deec. Non-external. The NSArray check, then
// -insertObjectsFromArray:range:atIndex: of the whole argument at the receiver's count (a tail call); the
// receiver's count is read before the argument's. Unlike -[NSMutableArray addObjectsFromArray:], a
// non-NSArray argument is not logged or collected first.
- (void)addObjectsFromArray:(id)array {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: array argument is not an NSArray",
            methodName("addObjectsFromArray:")
        );
    }
    NSUInteger count = [self count];
    NSUInteger otherCount = [array count];
    [self insertObjectsFromArray:array range:(OCFRange){0, (OCFIndex)otherCount} atIndex:count];
}

// Native -[NSMutableOrderedSet addObjectsFromOrderedSet:range:], 0x1c017e074. Non-external. The body of
// -addObjectsFromArray:range: for an NSOrderedSet argument ("ordered set argument is not an NSOrderedSet"),
// ending in -insertObjectsFromOrderedSet:range:atIndex: (a tail call).
- (void)addObjectsFromOrderedSet:(id)orderedSet range:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: ordered set argument is not an NSOrderedSet",
            methodName("addObjectsFromOrderedSet:range:")
        );
    }
    NSUInteger otherCount = [orderedSet count];
    if (range.location < 0 || range.length < 0 ||
        otherCount < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("addObjectsFromOrderedSet:range:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            otherCount
        );
    }
    [self insertObjectsFromOrderedSet:orderedSet range:range atIndex:[self count]];
}

// Native -[NSMutableOrderedSet addObjectsFromOrderedSet:], 0x1c017e374. Non-external. The NSOrderedSet check,
// then -insertObjectsFromOrderedSet:range:atIndex: of the whole argument at the receiver's count (a tail
// call); the receiver's count is read before the argument's.
- (void)addObjectsFromOrderedSet:(id)orderedSet {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: ordered set argument is not an NSOrderedSet",
            methodName("addObjectsFromOrderedSet:")
        );
    }
    NSUInteger count = [self count];
    NSUInteger otherCount = [orderedSet count];
    [self insertObjectsFromOrderedSet:orderedSet range:(OCFRange){0, (OCFIndex)otherCount} atIndex:count];
}

// Native -[NSMutableOrderedSet addObjectsFromSet:], 0x1c017e4fc. Non-external. The NSSet check ("set argument
// is not an NSSet"), then -insertObjectsFromSet:atIndex: at the count (a tail call).
- (void)addObjectsFromSet:(id)set {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (set != nil && !_ONSIsNSSet(set)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: set argument is not an NSSet",
            methodName("addObjectsFromSet:")
        );
    }
    [self insertObjectsFromSet:set atIndex:[self count]];
}

// Native -[NSMutableOrderedSet exchangeObjectAtIndex:withObjectAtIndex:], 0x1c017e670. Non-external. Each
// index is checked against a fresh count (negative or at/above the count raises NSRangeException). Equal
// indexes do nothing. Otherwise the objects at the lower and the higher index are read (lower first) and
// both retained (lower first); the object at the higher index is removed, the lower index receives the
// higher object (-replaceObjectAtIndex:withObject:) and the lower object is inserted at the higher index;
// then the lower object and the higher object are released (the second a tail call).
- (void)exchangeObjectAtIndex:(NSUInteger)index1 withObjectAtIndex:(NSUInteger)index2 {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    NSUInteger count = [self count];
    if ((NSInteger)index1 < 0 || count <= index1) {
        raiseIndexBeyondBounds(methodName("exchangeObjectAtIndex:withObjectAtIndex:"), index1, count);
    }
    NSUInteger secondCount = [self count];
    if ((NSInteger)index2 < 0 || secondCount <= index2) {
        raiseIndexBeyondBounds(methodName("exchangeObjectAtIndex:withObjectAtIndex:"), index2, secondCount);
    }
    if (index1 == index2) {
        return;
    }
    NSUInteger higher = index2 > index1 ? index2 : index1;
    NSUInteger lower = index2 < index1 ? index2 : index1;
    id lowerObject = [self objectAtIndex:lower];
    id higherObject = [self objectAtIndex:higher];
    [lowerObject retain];
    [higherObject retain];
    [self removeObjectAtIndex:higher];
    [self replaceObjectAtIndex:lower withObject:higherObject];
    [self insertObject:lowerObject atIndex:higher];
    [lowerObject release];
    [higherObject release];
}

// Native -[NSMutableOrderedSet insertObjects:count:atIndex:], 0x1c017ea88. Non-external. The argument checks
// of -addObjects:count:, then the index check against the count (negative or above the count raises
// NSRangeException). The count is read again and the index kept as its distance from the end
// (index - count, read before the loop and unchanged for an empty list); each object is then inserted with
// -insertObject:atIndex: at a fresh count plus that distance, so objects the ordered set already holds
// (which are not inserted) do not shift the position of the following ones.
- (void)insertObjects:(const id *)objects count:(NSUInteger)count atIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (objects == NULL && count != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            22,
            "*** %s: pointer to objects array is NULL but length is %lu",
            methodName("insertObjects:count:atIndex:"),
            count
        );
    }
    if (count >> 61 != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            22,
            "*** %s: count (%lu) of objects array is ridiculous",
            methodName("insertObjects:count:atIndex:"),
            count
        );
    }
    for (NSUInteger position = 0; position < count; ++position) {
        if (objects[position] == nil) {
            raiseWithLogPack(
                ONSInvalidArgumentException,
                22,
                "*** %s: attempt to insert nil object from objects[%lu]",
                methodName("insertObjects:count:atIndex:"),
                position
            );
        }
    }
    NSUInteger orderedSetCount = [self count];
    if ((NSInteger)index < 0 || orderedSetCount < index) {
        raiseIndexBeyondBounds(methodName("insertObjects:count:atIndex:"), index, orderedSetCount);
    }
    NSUInteger currentCount = [self count];
    if (count == 0) {
        return;
    }
    NSUInteger distance = index - currentCount;
    for (; count != 0; --count) {
        id object = *objects++;
        [self insertObject:object atIndex:[self count] + distance];
    }
}

// Native -[NSMutableOrderedSet insertObjectsFromArray:range:atIndex:], 0x1c017ef10. Non-external. The NSArray
// check, the range check against the argument's count, the index check against the receiver's count, then a
// temporary buffer of range.length objects filled by -getObjects:range: and -insertObjects:count:atIndex:.
- (void)insertObjectsFromArray:(id)array range:(OCFRange)range atIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: array argument is not an NSArray",
            methodName("insertObjectsFromArray:range:atIndex:")
        );
    }
    NSUInteger otherCount = [array count];
    if (range.location < 0 || range.length < 0 ||
        otherCount < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("insertObjectsFromArray:range:atIndex:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            otherCount
        );
    }
    NSUInteger count = [self count];
    if ((NSInteger)index < 0 || count < index) {
        raiseIndexBeyondBounds(methodName("insertObjectsFromArray:range:atIndex:"), index, count);
    }
    NSUInteger length = (NSUInteger)range.length;
    if (length >> 60 != 0) {
        raiseTemporaryBufferTooLarge(length);
    }
    size_t bufferCount = length > 1 ? length : 1;
    id stackObjects[length < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *objects = stackObjects;
    void *heapObjects = NULL;
    if (length >= 0x101) {
        objects = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = objects;
    }
    [array getObjects:objects range:range];
    [self insertObjects:objects count:length atIndex:index];
    free(heapObjects);
}

// Native -[NSMutableOrderedSet insertObjectsFromArray:atIndex:], 0x1c017f4f0. Non-external. The NSArray
// check, the index check, then the whole argument through a temporary buffer (-getObjects:range: from 0) and
// -insertObjects:count:atIndex:.
- (void)insertObjectsFromArray:(id)array atIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: array argument is not an NSArray",
            methodName("insertObjectsFromArray:atIndex:")
        );
    }
    NSUInteger count = [self count];
    if ((NSInteger)index < 0 || count < index) {
        raiseIndexBeyondBounds(methodName("insertObjectsFromArray:atIndex:"), index, count);
    }
    NSUInteger otherCount = [array count];
    if (otherCount >> 60 != 0) {
        raiseTemporaryBufferTooLarge(otherCount);
    }
    size_t bufferCount = otherCount > 1 ? otherCount : 1;
    id stackObjects[otherCount < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *objects = stackObjects;
    void *heapObjects = NULL;
    if (otherCount >= 0x101) {
        objects = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = objects;
    }
    [array getObjects:objects range:(OCFRange){0, (OCFIndex)otherCount}];
    [self insertObjects:objects count:otherCount atIndex:index];
    free(heapObjects);
}

// Native -[NSMutableOrderedSet insertObjectsFromOrderedSet:range:atIndex:], 0x1c017f914. Non-external. The
// body of -insertObjectsFromArray:range:atIndex: for an NSOrderedSet argument.
- (void)insertObjectsFromOrderedSet:(id)orderedSet range:(OCFRange)range atIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: ordered set argument is not an NSOrderedSet",
            methodName("insertObjectsFromOrderedSet:range:atIndex:")
        );
    }
    NSUInteger otherCount = [orderedSet count];
    if (range.location < 0 || range.length < 0 ||
        otherCount < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("insertObjectsFromOrderedSet:range:atIndex:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            otherCount
        );
    }
    NSUInteger count = [self count];
    if ((NSInteger)index < 0 || count < index) {
        raiseIndexBeyondBounds(methodName("insertObjectsFromOrderedSet:range:atIndex:"), index, count);
    }
    NSUInteger length = (NSUInteger)range.length;
    if (length >> 60 != 0) {
        raiseTemporaryBufferTooLarge(length);
    }
    size_t bufferCount = length > 1 ? length : 1;
    id stackObjects[length < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *objects = stackObjects;
    void *heapObjects = NULL;
    if (length >= 0x101) {
        objects = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = objects;
    }
    [orderedSet getObjects:objects range:range];
    [self insertObjects:objects count:length atIndex:index];
    free(heapObjects);
}

// Native -[NSMutableOrderedSet insertObjectsFromOrderedSet:atIndex:], 0x1c017fef4. Non-external. The body of
// -insertObjectsFromArray:atIndex: for an NSOrderedSet argument.
- (void)insertObjectsFromOrderedSet:(id)orderedSet atIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: ordered set argument is not an NSOrderedSet",
            methodName("insertObjectsFromOrderedSet:atIndex:")
        );
    }
    NSUInteger count = [self count];
    if ((NSInteger)index < 0 || count < index) {
        raiseIndexBeyondBounds(methodName("insertObjectsFromOrderedSet:atIndex:"), index, count);
    }
    NSUInteger otherCount = [orderedSet count];
    if (otherCount >> 60 != 0) {
        raiseTemporaryBufferTooLarge(otherCount);
    }
    size_t bufferCount = otherCount > 1 ? otherCount : 1;
    id stackObjects[otherCount < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *objects = stackObjects;
    void *heapObjects = NULL;
    if (otherCount >= 0x101) {
        objects = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = objects;
    }
    [orderedSet getObjects:objects range:(OCFRange){0, (OCFIndex)otherCount}];
    [self insertObjects:objects count:otherCount atIndex:index];
    free(heapObjects);
}

// Native -[NSMutableOrderedSet insertObjectsFromSet:atIndex:], 0x1c0180318. Non-external. The body of
// -insertObjectsFromArray:atIndex: for an NSSet argument, whose objects are read with -getObjects:count:.
- (void)insertObjectsFromSet:(id)set atIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (set != nil && !_ONSIsNSSet(set)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: set argument is not an NSSet",
            methodName("insertObjectsFromSet:atIndex:")
        );
    }
    NSUInteger count = [self count];
    if ((NSInteger)index < 0 || count < index) {
        raiseIndexBeyondBounds(methodName("insertObjectsFromSet:atIndex:"), index, count);
    }
    NSUInteger otherCount = [set count];
    if (otherCount >> 60 != 0) {
        raiseTemporaryBufferTooLarge(otherCount);
    }
    size_t bufferCount = otherCount > 1 ? otherCount : 1;
    id stackObjects[otherCount < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *objects = stackObjects;
    void *heapObjects = NULL;
    if (otherCount >= 0x101) {
        objects = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = objects;
    }
    [set getObjects:objects count:otherCount];
    [self insertObjects:objects count:otherCount atIndex:index];
    free(heapObjects);
}

// Native -[NSMutableOrderedSet insertObjects:atIndexes:], 0x1c0180738. Non-external. The NSArray check, a nil
// index set and a non-NSIndexSet raise NSInvalidArgumentException; the index set's last index is checked
// against the receiver's count plus the array's count (a negative last index raises; NSNotFound passes; an
// index at or above the bound raises NSRangeException). Differing counts of array and index set raise
// NSInvalidArgumentException through _CFThrowFormattedException. The array's objects are read into a
// temporary buffer and inserted range by range (-rangeCount read before every range, -rangeAtIndex:,
// -insertObjects:count:atIndex: at the range's location with the range's length), consuming the buffer in
// order.
- (void)insertObjects:(id)objects atIndexes:(id)indexes {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (objects != nil && !_ONSIsNSArray(objects)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: array argument is not an NSArray",
            methodName("insertObjects:atIndexes:")
        );
    }
    if (indexes == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: index set cannot be nil",
            methodName("insertObjects:atIndexes:")
        );
    }
    if (!_ONSIsNSIndexSet(indexes)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: index set argument is not an NSIndexSet",
            methodName("insertObjects:atIndexes:")
        );
    }
    NSUInteger count = [self count];
    NSUInteger objectCount = [objects count];
    NSUInteger lastIndex = [indexes lastIndex];
    if ((NSInteger)lastIndex < 0 || (lastIndex != kNotFound && objectCount + count <= lastIndex)) {
        raiseIndexSetBeyondBounds(methodName("insertObjects:atIndexes:"), lastIndex, objectCount + count);
    }
    NSUInteger arrayCount = [objects count];
    NSUInteger indexCount = [indexes count];
    if (arrayCount != indexCount) {
        _CFThrowFormattedException(
            ONSInvalidArgumentException,
            OF_STR("*** %s: count of array (%lu) differs from count of index set (%lu)"),
            methodName("insertObjects:atIndexes:"),
            arrayCount,
            indexCount
        );
    }
    NSUInteger bufferObjectCount = [objects count];
    if (bufferObjectCount >> 60 != 0) {
        raiseTemporaryBufferTooLarge(bufferObjectCount);
    }
    size_t bufferCount = bufferObjectCount > 1 ? bufferObjectCount : 1;
    id stackObjects[bufferObjectCount < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *buffer = stackObjects;
    void *heapObjects = NULL;
    if (bufferObjectCount >= 0x101) {
        buffer = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = buffer;
    }
    [objects getObjects:buffer range:(OCFRange){0, (OCFIndex)bufferObjectCount}];
    NSUInteger consumed = 0;
    for (NSUInteger rangeIndex = 0; rangeIndex < [indexes rangeCount]; ++rangeIndex) {
        OCFRange range = [indexes rangeAtIndex:rangeIndex];
        [self insertObjects:&buffer[consumed]
                      count:(NSUInteger)range.length
                    atIndex:(NSUInteger)range.location];
        consumed += (NSUInteger)range.length;
    }
    free(heapObjects);
}

// Native -[NSMutableOrderedSet intersectOrderedSet:], 0x1c0180db4. Non-external. The NSOrderedSet check
// ("ordered set argument is not an NSOrderedSet"); an argument identical to the receiver does nothing.
// Otherwise the receiver's count passes the temporary buffer check and sizes a buffer (stack below 0x101
// objects, at least one slot, else _CFCreateArrayStorage); the receiver is walked by fast enumeration
// (16-object batches, mutation check on every object) and each object the argument answers -countForObject:
// 0 for is appended to the buffer; the collected objects are then removed in order with -removeObject: and
// the heap buffer is freed. The landing pads after the buffer is made free the heap buffer before resuming
// (0x1c01810d0); the one reached from the buffer check does not (0x1c01810bc).
- (void)intersectOrderedSet:(id)other {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (other != nil && !_ONSIsNSOrderedSet(other)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: ordered set argument is not an NSOrderedSet",
            methodName("intersectOrderedSet:")
        );
    }
    if (self == other) {
        return;
    }
    NSUInteger count = [self count];
    if (count >> 60 != 0) {
        raiseTemporaryBufferTooLarge(count);
    }
    size_t bufferCount = count > 1 ? count : 1;
    id stackObjects[count < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *objects = stackObjects;
    __attribute__((cleanup(releaseTemporaryStorage))) void *heapObjects = NULL;
    if (count >= 0x101) {
        objects = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = objects;
    }
    id *end = objects;
    for (id object in self) {
        if ([other countForObject:object] == 0) {
            *end++ = object;
        }
    }
    for (id *cursor = objects; cursor < end; ++cursor) {
        [self removeObject:*cursor];
    }
}

// Native -[NSMutableOrderedSet intersectSet:], 0x1c01810e4. Non-external. The body of -intersectOrderedSet:
// for an NSSet ("set argument is not an NSSet"), without the identity test; it has no landing pad, so an
// exception after the heap buffer is made leaves it allocated, as natively.
- (void)intersectSet:(id)set {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (set != nil && !_ONSIsNSSet(set)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: set argument is not an NSSet",
            methodName("intersectSet:")
        );
    }
    NSUInteger count = [self count];
    if (count >> 60 != 0) {
        raiseTemporaryBufferTooLarge(count);
    }
    size_t bufferCount = count > 1 ? count : 1;
    id stackObjects[count < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *objects = stackObjects;
    void *heapObjects = NULL;
    if (count >= 0x101) {
        objects = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = objects;
    }
    id *end = objects;
    for (id object in self) {
        if ([set countForObject:object] == 0) {
            *end++ = object;
        }
    }
    for (id *cursor = objects; cursor < end; ++cursor) {
        [self removeObject:*cursor];
    }
    free(heapObjects);
}

// Native -[NSMutableOrderedSet moveObjectsAtIndexes:toIndex:], 0x1c0181404. Non-external. A nil index set and
// a non-NSIndexSet raise NSInvalidArgumentException. The last-index check is the one of
// -insertObjects:atIndexes:, with the array's count read from nil (the native code sends -count to nil, so
// the bound is the receiver's count). The destination is checked against the count less
// -countOfIndexesInRange:{0, index} (negative or above it raises NSRangeException, "for empty ordered set"
// when it is zero). Then -objectsAtIndexes:, -removeObjectsAtIndexes:, and the moved objects through a
// temporary buffer into -insertObjects:count:atIndex: at the destination.
- (void)moveObjectsAtIndexes:(id)indexes toIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (indexes == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: index set cannot be nil",
            methodName("moveObjectsAtIndexes:toIndex:")
        );
    }
    if (!_ONSIsNSIndexSet(indexes)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: index set argument is not an NSIndexSet",
            methodName("moveObjectsAtIndexes:toIndex:")
        );
    }
    NSUInteger count = [self count];
    NSUInteger objectCount = [(id)nil count];
    NSUInteger lastIndex = [indexes lastIndex];
    if ((NSInteger)lastIndex < 0 || (lastIndex != kNotFound && objectCount + count <= lastIndex)) {
        raiseIndexSetBeyondBounds(
            methodName("moveObjectsAtIndexes:toIndex:"), lastIndex, objectCount + count
        );
    }
    NSUInteger currentCount = [self count];
    NSUInteger movedBefore = [indexes countOfIndexesInRange:(OCFRange){0, (OCFIndex)index}];
    NSUInteger remaining = currentCount - movedBefore;
    if ((NSInteger)index < 0 || remaining < index) {
        raiseIndexBeyondBounds(methodName("moveObjectsAtIndexes:toIndex:"), index, remaining);
    }
    id moved = [self objectsAtIndexes:indexes];
    [self removeObjectsAtIndexes:indexes];
    NSUInteger movedCount = [moved count];
    if (movedCount >> 60 != 0) {
        raiseTemporaryBufferTooLarge(movedCount);
    }
    size_t bufferCount = movedCount > 1 ? movedCount : 1;
    id stackObjects[movedCount < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *objects = stackObjects;
    void *heapObjects = NULL;
    if (movedCount >= 0x101) {
        objects = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = objects;
    }
    [moved getObjects:objects range:(OCFRange){0, (OCFIndex)movedCount}];
    [self insertObjects:objects count:movedCount atIndex:index];
    free(heapObjects);
}

// Native -[NSMutableOrderedSet minusOrderedSet:], 0x1c0181b04. Non-external. The NSOrderedSet check; an
// argument identical to the receiver empties it (-removeAllObjects, a tail call). Otherwise the argument is
// walked by fast enumeration (16-object batches, mutation check on every object) and each object is sent
// -removeObject: to the receiver as many times as the argument's -countForObject: answers.
- (void)minusOrderedSet:(id)other {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (other != nil && !_ONSIsNSOrderedSet(other)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: ordered set argument is not an NSOrderedSet",
            methodName("minusOrderedSet:")
        );
    }
    if (self == other) {
        [self removeAllObjects];
        return;
    }
    for (id object in other) {
        NSUInteger occurrences = [other countForObject:object];
        for (; occurrences != 0; --occurrences) {
            [self removeObject:object];
        }
    }
}

// Native -[NSMutableOrderedSet minusSet:], 0x1c0181d58. Non-external. The body of -minusOrderedSet: for an
// NSSet ("set argument is not an NSSet"), without the identity test.
- (void)minusSet:(id)set {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (set != nil && !_ONSIsNSSet(set)) {
        raiseWithLogPack(
            ONSInvalidArgumentException, 12, "*** %s: set argument is not an NSSet", methodName("minusSet:")
        );
    }
    for (id object in set) {
        NSUInteger occurrences = [set countForObject:object];
        for (; occurrences != 0; --occurrences) {
            [self removeObject:object];
        }
    }
}

// Native -[NSMutableOrderedSet removeAllObjects], 0x1c0181f6c. Non-external. -removeObjectAtIndex: from the
// last index down to 0.
- (void)removeAllObjects {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    NSUInteger count = [self count];
    for (NSUInteger index = count; index != 0; --index) {
        [self removeObjectAtIndex:index - 1];
    }
}

// Native -[NSMutableOrderedSet removeFirstObject], 0x1c0181fe4. Non-external. -removeObjectAtIndex: 0 when
// the ordered set is not empty (a tail call).
- (void)removeFirstObject {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if ([self count] != 0) {
        [self removeObjectAtIndex:0];
    }
}

// Native -[NSMutableOrderedSet removeLastObject], 0x1c0182054. Non-external. -removeObjectAtIndex: with the
// last index when the ordered set is not empty (a tail call); no linked-on-or-after test.
- (void)removeLastObject {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    NSUInteger count = [self count];
    if (count != 0) {
        [self removeObjectAtIndex:count - 1];
    }
}

// Native -[NSMutableOrderedSet removeObjectsInRange:], 0x1c01820c4. Non-external. The range check against the
// count, then range.length sends of -removeObjectAtIndex: at range.location.
- (void)removeObjectsInRange:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    NSUInteger count = [self count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("removeObjectsInRange:"), (NSUInteger)range.location, (NSUInteger)range.length, count
        );
    }
    for (NSUInteger length = (NSUInteger)range.length; length != 0; --length) {
        [self removeObjectAtIndex:(NSUInteger)range.location];
    }
}

// Native -[NSMutableOrderedSet removeObject:inRange:], 0x1c01822ec. Non-external. The range check against
// the count, then -indexOfObject:inRange:; an index other than NSNotFound is removed (-removeObjectAtIndex:).
- (void)removeObject:(id)object inRange:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    NSUInteger count = [self count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("removeObject:inRange:"), (NSUInteger)range.location, (NSUInteger)range.length, count
        );
    }
    NSUInteger index = [self indexOfObject:object inRange:range];
    if (index != kNotFound) {
        [self removeObjectAtIndex:index];
    }
}

// Native -[NSMutableOrderedSet removeObject:], 0x1c018255c. Non-external. -indexOfObject:; an index other
// than NSNotFound is removed (a tail call).
- (void)removeObject:(id)object {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    NSUInteger index = [self indexOfObject:object];
    if (index != kNotFound) {
        [self removeObjectAtIndex:index];
    }
}

// Native -[NSMutableOrderedSet removeObjectsAtIndexes:], 0x1c01825e8. Non-external. A nil index set and a
// non-NSIndexSet raise NSInvalidArgumentException; the last-index check of -moveObjectsAtIndexes:toIndex:
// (with the nil -count); then -removeObjectsInRange: for each range of the index set from the last range
// down.
- (void)removeObjectsAtIndexes:(id)indexes {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (indexes == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: index set cannot be nil",
            methodName("removeObjectsAtIndexes:")
        );
    }
    if (!_ONSIsNSIndexSet(indexes)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: index set argument is not an NSIndexSet",
            methodName("removeObjectsAtIndexes:")
        );
    }
    NSUInteger count = [self count];
    NSUInteger objectCount = [(id)nil count];
    NSUInteger lastIndex = [indexes lastIndex];
    if ((NSInteger)lastIndex < 0 || (lastIndex != kNotFound && objectCount + count <= lastIndex)) {
        raiseIndexSetBeyondBounds(methodName("removeObjectsAtIndexes:"), lastIndex, objectCount + count);
    }
    NSUInteger rangeCount = [indexes rangeCount];
    for (NSUInteger rangeIndex = rangeCount; rangeIndex != 0; --rangeIndex) {
        OCFRange range = [indexes rangeAtIndex:rangeIndex - 1];
        [self removeObjectsInRange:range];
    }
}

// Native -[NSMutableOrderedSet removeObjectsInRange:inArray:range:], 0x1c01829a0. Non-external. The
// receiver's range check, the NSArray check, the array's range check against its count; then, for each
// index of the array's range in order, the array's object is looked up with -indexOfObject:inRange: in the
// receiver's range (the range as passed, not adjusted after removals) and an index other than NSNotFound is
// removed.
- (void)removeObjectsInRange:(OCFRange)range inArray:(id)array range:(OCFRange)otherRange {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    NSUInteger count = [self count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("removeObjectsInRange:inArray:range:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            count
        );
    }
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: array argument is not an NSArray",
            methodName("removeObjectsInRange:inArray:range:")
        );
    }
    NSUInteger otherCount = [array count];
    if (otherRange.location < 0 || otherRange.length < 0 ||
        otherCount < (NSUInteger)otherRange.location + (NSUInteger)otherRange.length) {
        raiseRangeBeyondBounds(
            methodName("removeObjectsInRange:inArray:range:"),
            (NSUInteger)otherRange.location,
            (NSUInteger)otherRange.length,
            otherCount
        );
    }
    NSUInteger otherIndex = (NSUInteger)otherRange.location;
    for (NSUInteger remaining = (NSUInteger)otherRange.length; remaining != 0; --remaining) {
        NSUInteger index = [self indexOfObject:[array objectAtIndex:otherIndex] inRange:range];
        if (index != kNotFound) {
            [self removeObjectAtIndex:index];
        }
        ++otherIndex;
    }
}

// Native -[NSMutableOrderedSet removeObjectsInRange:inArray:], 0x1c0182e34. Non-external. The receiver's
// range check and the NSArray check; then the array is walked by fast enumeration (16-object batches,
// mutation check on every object) and each object found by -indexOfObject:inRange: in the receiver's range
// (as passed) is removed.
- (void)removeObjectsInRange:(OCFRange)range inArray:(id)array {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    NSUInteger count = [self count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("removeObjectsInRange:inArray:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            count
        );
    }
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: array argument is not an NSArray",
            methodName("removeObjectsInRange:inArray:")
        );
    }
    for (id object in array) {
        NSUInteger index = [self indexOfObject:object inRange:range];
        if (index != kNotFound) {
            [self removeObjectAtIndex:index];
        }
    }
}

// Native -[NSMutableOrderedSet removeObjectsInArray:range:], 0x1c01831c0. Non-external. The NSArray check,
// the range check against the array's count; then each object of the array's range, in order, is looked up
// with -indexOfObject: and an index other than NSNotFound is removed.
- (void)removeObjectsInArray:(id)array range:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: array argument is not an NSArray",
            methodName("removeObjectsInArray:range:")
        );
    }
    NSUInteger otherCount = [array count];
    if (range.location < 0 || range.length < 0 ||
        otherCount < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("removeObjectsInArray:range:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            otherCount
        );
    }
    NSUInteger otherIndex = (NSUInteger)range.location;
    for (NSUInteger remaining = (NSUInteger)range.length; remaining != 0; --remaining) {
        NSUInteger index = [self indexOfObject:[array objectAtIndex:otherIndex]];
        if (index != kNotFound) {
            [self removeObjectAtIndex:index];
        }
        ++otherIndex;
    }
}

// Native -[NSMutableOrderedSet removeObjectsInArray:], 0x1c01834e4. Non-external. The NSArray check; then
// the array is walked by fast enumeration (16-object batches, mutation check on every object) and each
// object found by -indexOfObject: is removed.
- (void)removeObjectsInArray:(id)array {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: array argument is not an NSArray",
            methodName("removeObjectsInArray:")
        );
    }
    for (id object in array) {
        NSUInteger index = [self indexOfObject:object];
        if (index != kNotFound) {
            [self removeObjectAtIndex:index];
        }
    }
}

// Native -[NSMutableOrderedSet removeObjectsInRange:inOrderedSet:range:], 0x1c01836f0. Non-external. The
// receiver's range check, the NSOrderedSet check, the argument's range check against its count; then the
// argument's range passes the temporary buffer check, is read into a buffer (stack below 0x101 objects, at
// least one slot, else _CFCreateArrayStorage) with -getObjects:range:, and each buffered object found by
// -indexOfObject:inRange: in the receiver's range (as passed) is removed. The heap buffer is freed at the
// end; there is no landing pad.
- (void)removeObjectsInRange:(OCFRange)range inOrderedSet:(id)orderedSet range:(OCFRange)otherRange {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    NSUInteger count = [self count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("removeObjectsInRange:inOrderedSet:range:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            count
        );
    }
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: ordered set argument is not an NSOrderedSet",
            methodName("removeObjectsInRange:inOrderedSet:range:")
        );
    }
    NSUInteger otherCount = [orderedSet count];
    if (otherRange.location < 0 || otherRange.length < 0 ||
        otherCount < (NSUInteger)otherRange.location + (NSUInteger)otherRange.length) {
        raiseRangeBeyondBounds(
            methodName("removeObjectsInRange:inOrderedSet:range:"),
            (NSUInteger)otherRange.location,
            (NSUInteger)otherRange.length,
            otherCount
        );
    }
    NSUInteger length = (NSUInteger)otherRange.length;
    if (length >> 60 != 0) {
        raiseTemporaryBufferTooLarge(length);
    }
    size_t bufferCount = length > 1 ? length : 1;
    id stackObjects[length < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *objects = stackObjects;
    void *heapObjects = NULL;
    if (length >= 0x101) {
        objects = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = objects;
    }
    [orderedSet getObjects:objects range:otherRange];
    for (; length != 0; --length) {
        id object = *objects++;
        NSUInteger index = [self indexOfObject:object inRange:range];
        if (index != kNotFound) {
            [self removeObjectAtIndex:index];
        }
    }
    free(heapObjects);
}

// Native -[NSMutableOrderedSet removeObjectsInRange:inOrderedSet:], 0x1c0183d00. Non-external. The body of
// -removeObjectsInRange:inArray: for an NSOrderedSet.
- (void)removeObjectsInRange:(OCFRange)range inOrderedSet:(id)orderedSet {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    NSUInteger count = [self count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("removeObjectsInRange:inOrderedSet:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            count
        );
    }
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: ordered set argument is not an NSOrderedSet",
            methodName("removeObjectsInRange:inOrderedSet:")
        );
    }
    [self removeObjectsInRange:range
                  inOrderedSet:orderedSet
                         range:(OCFRange){0, (OCFIndex)[orderedSet count]}];
}

// Native -[NSMutableOrderedSet removeObjectsInOrderedSet:range:], 0x1c0184000. Non-external. The body of
// -removeObjectsInArray:range: for an NSOrderedSet.
- (void)removeObjectsInOrderedSet:(id)orderedSet range:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: ordered set argument is not an NSOrderedSet",
            methodName("removeObjectsInOrderedSet:range:")
        );
    }
    NSUInteger otherCount = [orderedSet count];
    if (range.location < 0 || range.length < 0 ||
        otherCount < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("removeObjectsInOrderedSet:range:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            otherCount
        );
    }
    [self removeObjectsInRange:(OCFRange){0, (OCFIndex)[self count]} inOrderedSet:orderedSet range:range];
}

// Native -[NSMutableOrderedSet removeObjectsInOrderedSet:], 0x1c0184304. Non-external. The body of
// -removeObjectsInArray: for an NSOrderedSet.
- (void)removeObjectsInOrderedSet:(id)orderedSet {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: ordered set argument is not an NSOrderedSet",
            methodName("removeObjectsInOrderedSet:")
        );
    }
    NSUInteger count = [self count];
    NSUInteger otherCount = [orderedSet count];
    [self removeObjectsInRange:(OCFRange){0, (OCFIndex)count}
                  inOrderedSet:orderedSet
                         range:(OCFRange){0, (OCFIndex)otherCount}];
}

// Native -[NSMutableOrderedSet removeObjectsInRange:inSet:], 0x1c0184490. Non-external. The receiver's range
// check, the NSSet check; then the set is walked by fast enumeration (16-object batches, mutation check on
// every object) and each object found by -indexOfObject:inRange: in the receiver's range (as passed) is
// removed.
- (void)removeObjectsInRange:(OCFRange)range inSet:(id)set {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    NSUInteger count = [self count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("removeObjectsInRange:inSet:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            count
        );
    }
    if (set != nil && !_ONSIsNSSet(set)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: set argument is not an NSSet",
            methodName("removeObjectsInRange:inSet:")
        );
    }
    for (id object in set) {
        NSUInteger index = [self indexOfObject:object inRange:range];
        if (index != kNotFound) {
            [self removeObjectAtIndex:index];
        }
    }
}

// Native -[NSMutableOrderedSet removeObjectsInSet:], 0x1c018481c. Non-external. The NSSet check; then the set
// is walked by fast enumeration (16-object batches, mutation check on every object) and each object found
// by -indexOfObject: is removed.
- (void)removeObjectsInSet:(id)set {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (set != nil && !_ONSIsNSSet(set)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: set argument is not an NSSet",
            methodName("removeObjectsInSet:")
        );
    }
    for (id object in set) {
        NSUInteger index = [self indexOfObject:object];
        if (index != kNotFound) {
            [self removeObjectAtIndex:index];
        }
    }
}

// Native -[NSMutableOrderedSet removeObjectsAtIndexes:options:passingTest:], 0x1c0184a28. Non-external. A nil
// index set, a non-NSIndexSet and a nil predicate raise NSInvalidArgumentException in that order. Options
// with bit 1 set (NSEnumerationReverse) lose bit 0 (NSEnumerationConcurrent) and bits 60 and 62; the options
// are then marked with bits 60 and 62 (0x5000000000000000) for __NSOrderedSetParameterCheckIterate and
// __NSOrderedSetGetIndexesPassingTest, and the ranges of the resulting index set are removed from the last
// down.
- (void)removeObjectsAtIndexes:(id)indexes
                       options:(NSUInteger)options
                   passingTest:(BOOL (^)(id, NSUInteger, BOOL *))predicate {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (indexes == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: index set cannot be nil",
            methodName("removeObjectsAtIndexes:options:passingTest:")
        );
    }
    if (!_ONSIsNSIndexSet(indexes)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: index set argument is not an NSIndexSet",
            methodName("removeObjectsAtIndexes:options:passingTest:")
        );
    }
    if (predicate == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: predicate cannot be nil",
            methodName("removeObjectsAtIndexes:options:passingTest:")
        );
    }
    if ((options & 2) != 0) {
        options &= UINT64_C(0xAFFFFFFFFFFFFFFE);
    }
    __NSOrderedSetParameterCheckIterate(
        self, _cmd, predicate, options | UINT64_C(0x5000000000000000), indexes
    );
    id passing =
        __NSOrderedSetGetIndexesPassingTest(self, predicate, options | UINT64_C(0x5000000000000000), indexes);
    NSUInteger rangeCount = [passing rangeCount];
    for (NSUInteger rangeIndex = rangeCount; rangeIndex != 0; --rangeIndex) {
        OCFRange range = [passing rangeAtIndex:rangeIndex - 1];
        [self removeObjectsInRange:range];
    }
}

// Native -[NSMutableOrderedSet removeObjectsWithOptions:passingTest:], 0x1c0184d08. Non-external. A nil
// predicate raises NSInvalidArgumentException. Options with bit 1 set lose bits 0 and 62; the options are
// marked with bit 62 (0x4000000000000000) and the passing indexes of the whole ordered set (nil index set)
// are removed range by range from the last down.
- (void)removeObjectsWithOptions:(NSUInteger)options passingTest:(BOOL (^)(id, NSUInteger, BOOL *))predicate {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (predicate == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: predicate cannot be nil",
            methodName("removeObjectsWithOptions:passingTest:")
        );
    }
    if ((options & 2) != 0) {
        options &= UINT64_C(0xBFFFFFFFFFFFFFFE);
    }
    __NSOrderedSetParameterCheckIterate(self, _cmd, predicate, options | UINT64_C(0x4000000000000000), nil);
    id passing =
        __NSOrderedSetGetIndexesPassingTest(self, predicate, options | UINT64_C(0x4000000000000000), nil);
    NSUInteger rangeCount = [passing rangeCount];
    for (NSUInteger rangeIndex = rangeCount; rangeIndex != 0; --rangeIndex) {
        OCFRange range = [passing rangeAtIndex:rangeIndex - 1];
        [self removeObjectsInRange:range];
    }
}

// Native -[NSMutableOrderedSet removeObjectsPassingTest:], 0x1c0184edc. Non-external. No -_mutate: a nil
// predicate raises NSInvalidArgumentException; then the ThreadSanitizer write hook (with the method's return
// address) and -removeObjectsWithOptions:passingTest: with no options (a tail call).
- (void)removeObjectsPassingTest:(BOOL (^)(id, NSUInteger, BOOL *))predicate {
    void *returnAddress = __builtin_return_address(0);
    if (predicate == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: predicate cannot be nil",
            methodName("removeObjectsPassingTest:")
        );
    }
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    [self removeObjectsWithOptions:0 passingTest:predicate];
}

// Native -[NSMutableOrderedSet replaceObject:inRange:], 0x1c0185030. Non-external. A nil object raises
// NSInvalidArgumentException before the count is read; then the range check, and -indexOfObject:inRange:;
// an index other than NSNotFound receives the object (-replaceObjectAtIndex:withObject:, a tail call).
- (void)replaceObject:(id)object inRange:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (object == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: object cannot be nil",
            methodName("replaceObject:inRange:")
        );
    }
    NSUInteger count = [self count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("replaceObject:inRange:"), (NSUInteger)range.location, (NSUInteger)range.length, count
        );
    }
    NSUInteger index = [self indexOfObject:object inRange:range];
    if (index != kNotFound) {
        [self replaceObjectAtIndex:index withObject:object];
    }
}

// Native -[NSMutableOrderedSet replaceObject:], 0x1c0185364. Non-external. A nil object raises
// NSInvalidArgumentException; then -indexOfObject:, and an index other than NSNotFound receives the object (a
// tail call).
- (void)replaceObject:(id)object {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (object == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException, 12, "*** %s: object cannot be nil", methodName("replaceObject:")
        );
    }
    NSUInteger index = [self indexOfObject:object];
    if (index != kNotFound) {
        [self replaceObjectAtIndex:index withObject:object];
    }
}

// Native -[NSMutableOrderedSet replaceObjectsInRange:withObjects:count:], 0x1c0185504. Non-external. The
// range check against the count, then the objects checks of -addObjects:count:. With no objects the range is
// removed and -insertObjects:count:atIndex: is sent with the (empty) list at range.location (a tail call).
// Otherwise every object is retained (objc_retain, no tagged-pointer test), the range is removed
// (-removeObjectsInRange:), all objects are inserted at range.location (-insertObjects:count:atIndex:), and
// every object is released.
- (void)replaceObjectsInRange:(OCFRange)range withObjects:(const id *)objects count:(NSUInteger)count {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    NSUInteger orderedSetCount = [self count];
    if (range.location < 0 || range.length < 0 ||
        orderedSetCount < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("replaceObjectsInRange:withObjects:count:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            orderedSetCount
        );
    }
    if (objects == NULL && count != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            22,
            "*** %s: pointer to objects array is NULL but length is %lu",
            methodName("replaceObjectsInRange:withObjects:count:"),
            count
        );
    }
    if (count >> 61 != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            22,
            "*** %s: count (%lu) of objects array is ridiculous",
            methodName("replaceObjectsInRange:withObjects:count:"),
            count
        );
    }
    if (count == 0) {
        [self removeObjectsInRange:range];
        [self insertObjects:objects count:count atIndex:(NSUInteger)range.location];
        return;
    }
    for (NSUInteger index = 0; index < count; ++index) {
        if (objects[index] == nil) {
            raiseWithLogPack(
                ONSInvalidArgumentException,
                22,
                "*** %s: attempt to insert nil object from objects[%lu]",
                methodName("replaceObjectsInRange:withObjects:count:"),
                index
            );
        }
    }
    for (NSUInteger index = 0; index < count; ++index) {
        [objects[index] retain];
    }
    [self removeObjectsInRange:range];
    [self insertObjects:objects count:count atIndex:(NSUInteger)range.location];
    for (NSUInteger remaining = count; remaining != 0; --remaining) {
        [*objects++ release];
    }
}

// Native -[NSMutableOrderedSet replaceObjectsAtIndexes:withObjects:], 0x1c01859fc. Non-external. The checks
// of -insertObjects:atIndexes: (NSArray check, nil index set, NSIndexSet check, the last index against the
// receiver's count plus the array's count, the differing-counts raise), then the array's objects through a
// temporary buffer. The ranges of the index set are removed from the last down (-rangeCount read once,
// -removeObjectsInRange:), then the buffered objects are inserted range by range from the first
// (-rangeCount read before every range, -insertObjects:count:atIndex: at the range's location with the
// range's length), consuming the buffer in order.
- (void)replaceObjectsAtIndexes:(id)indexes withObjects:(id)objects {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (objects != nil && !_ONSIsNSArray(objects)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: array argument is not an NSArray",
            methodName("replaceObjectsAtIndexes:withObjects:")
        );
    }
    if (indexes == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: index set cannot be nil",
            methodName("replaceObjectsAtIndexes:withObjects:")
        );
    }
    if (!_ONSIsNSIndexSet(indexes)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: index set argument is not an NSIndexSet",
            methodName("replaceObjectsAtIndexes:withObjects:")
        );
    }
    NSUInteger count = [self count];
    NSUInteger objectCount = [objects count];
    NSUInteger lastIndex = [indexes lastIndex];
    if ((NSInteger)lastIndex < 0 || (lastIndex != kNotFound && objectCount + count <= lastIndex)) {
        raiseIndexSetBeyondBounds(
            methodName("replaceObjectsAtIndexes:withObjects:"), lastIndex, objectCount + count
        );
    }
    NSUInteger arrayCount = [objects count];
    NSUInteger indexCount = [indexes count];
    if (arrayCount != indexCount) {
        _CFThrowFormattedException(
            ONSInvalidArgumentException,
            OF_STR("*** %s: count of array (%lu) differs from count of index set (%lu)"),
            methodName("replaceObjectsAtIndexes:withObjects:"),
            arrayCount,
            indexCount
        );
    }
    NSUInteger bufferObjectCount = [objects count];
    if (bufferObjectCount >> 60 != 0) {
        raiseTemporaryBufferTooLarge(bufferObjectCount);
    }
    size_t bufferCount = bufferObjectCount > 1 ? bufferObjectCount : 1;
    id stackObjects[bufferObjectCount < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *buffer = stackObjects;
    void *heapObjects = NULL;
    if (bufferObjectCount >= 0x101) {
        buffer = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = buffer;
    }
    [objects getObjects:buffer range:(OCFRange){0, (OCFIndex)bufferObjectCount}];
    NSUInteger rangeCount = [indexes rangeCount];
    for (NSUInteger rangeIndex = rangeCount; rangeIndex != 0; --rangeIndex) {
        [self removeObjectsInRange:[indexes rangeAtIndex:rangeIndex - 1]];
    }
    NSUInteger consumed = 0;
    for (NSUInteger rangeIndex = 0; rangeIndex < [indexes rangeCount]; ++rangeIndex) {
        OCFRange range = [indexes rangeAtIndex:rangeIndex];
        [self insertObjects:&buffer[consumed]
                      count:(NSUInteger)range.length
                    atIndex:(NSUInteger)range.location];
        consumed += (NSUInteger)range.length;
    }
    free(heapObjects);
}

// Native -[NSMutableOrderedSet replaceObjectsInRange:withObjectsFromArray:range:], 0x1c01860b0. Non-external.
// The NSArray check, the array's range against its count, the receiver's range against its count, then the
// array's range through a temporary buffer into -replaceObjectsInRange:withObjects:count:.
- (void)replaceObjectsInRange:(OCFRange)range withObjectsFromArray:(id)array range:(OCFRange)otherRange {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: array argument is not an NSArray",
            methodName("replaceObjectsInRange:withObjectsFromArray:range:")
        );
    }
    NSUInteger otherCount = [array count];
    if (otherRange.location < 0 || otherRange.length < 0 ||
        otherCount < (NSUInteger)otherRange.location + (NSUInteger)otherRange.length) {
        raiseRangeBeyondBounds(
            methodName("replaceObjectsInRange:withObjectsFromArray:range:"),
            (NSUInteger)otherRange.location,
            (NSUInteger)otherRange.length,
            otherCount
        );
    }
    NSUInteger count = [self count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("replaceObjectsInRange:withObjectsFromArray:range:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            count
        );
    }
    NSUInteger length = (NSUInteger)otherRange.length;
    if (length >> 60 != 0) {
        raiseTemporaryBufferTooLarge(length);
    }
    size_t bufferCount = length > 1 ? length : 1;
    id stackObjects[length < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *objects = stackObjects;
    void *heapObjects = NULL;
    if (length >= 0x101) {
        objects = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = objects;
    }
    [array getObjects:objects range:otherRange];
    [self replaceObjectsInRange:range withObjects:objects count:length];
    free(heapObjects);
}

// Native -[NSMutableOrderedSet replaceObjectsInRange:withObjectsFromArray:], 0x1c01866a0. Non-external. The
// NSArray check, the receiver's range check, then the whole array through a temporary buffer into
// -replaceObjectsInRange:withObjects:count:.
- (void)replaceObjectsInRange:(OCFRange)range withObjectsFromArray:(id)array {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: array argument is not an NSArray",
            methodName("replaceObjectsInRange:withObjectsFromArray:")
        );
    }
    NSUInteger count = [self count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("replaceObjectsInRange:withObjectsFromArray:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            count
        );
    }
    NSUInteger otherCount = [array count];
    if (otherCount >> 60 != 0) {
        raiseTemporaryBufferTooLarge(otherCount);
    }
    size_t bufferCount = otherCount > 1 ? otherCount : 1;
    id stackObjects[otherCount < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *objects = stackObjects;
    void *heapObjects = NULL;
    if (otherCount >= 0x101) {
        objects = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = objects;
    }
    [array getObjects:objects range:(OCFRange){0, (OCFIndex)otherCount}];
    [self replaceObjectsInRange:range withObjects:objects count:otherCount];
    free(heapObjects);
}

// Native -[NSMutableOrderedSet replaceObjectsInRange:withObjectsFromOrderedSet:range:], 0x1c0186adc.
// Non-external. The body of -replaceObjectsInRange:withObjectsFromArray:range: for an NSOrderedSet.
- (void)replaceObjectsInRange:(OCFRange)range
    withObjectsFromOrderedSet:(id)orderedSet
                        range:(OCFRange)otherRange {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: ordered set argument is not an NSOrderedSet",
            methodName("replaceObjectsInRange:withObjectsFromOrderedSet:range:")
        );
    }
    NSUInteger otherCount = [orderedSet count];
    if (otherRange.location < 0 || otherRange.length < 0 ||
        otherCount < (NSUInteger)otherRange.location + (NSUInteger)otherRange.length) {
        raiseRangeBeyondBounds(
            methodName("replaceObjectsInRange:withObjectsFromOrderedSet:range:"),
            (NSUInteger)otherRange.location,
            (NSUInteger)otherRange.length,
            otherCount
        );
    }
    NSUInteger count = [self count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("replaceObjectsInRange:withObjectsFromOrderedSet:range:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            count
        );
    }
    NSUInteger length = (NSUInteger)otherRange.length;
    if (length >> 60 != 0) {
        raiseTemporaryBufferTooLarge(length);
    }
    size_t bufferCount = length > 1 ? length : 1;
    id stackObjects[length < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *objects = stackObjects;
    void *heapObjects = NULL;
    if (length >= 0x101) {
        objects = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = objects;
    }
    [orderedSet getObjects:objects range:otherRange];
    [self replaceObjectsInRange:range withObjects:objects count:length];
    free(heapObjects);
}

// Native -[NSMutableOrderedSet replaceObjectsInRange:withObjectsFromOrderedSet:], 0x1c01870cc. Non-external.
// The body of -replaceObjectsInRange:withObjectsFromArray: for an NSOrderedSet.
- (void)replaceObjectsInRange:(OCFRange)range withObjectsFromOrderedSet:(id)orderedSet {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: ordered set argument is not an NSOrderedSet",
            methodName("replaceObjectsInRange:withObjectsFromOrderedSet:")
        );
    }
    NSUInteger count = [self count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("replaceObjectsInRange:withObjectsFromOrderedSet:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            count
        );
    }
    NSUInteger otherCount = [orderedSet count];
    if (otherCount >> 60 != 0) {
        raiseTemporaryBufferTooLarge(otherCount);
    }
    size_t bufferCount = otherCount > 1 ? otherCount : 1;
    id stackObjects[otherCount < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *objects = stackObjects;
    void *heapObjects = NULL;
    if (otherCount >= 0x101) {
        objects = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = objects;
    }
    [orderedSet getObjects:objects range:(OCFRange){0, (OCFIndex)otherCount}];
    [self replaceObjectsInRange:range withObjects:objects count:otherCount];
    free(heapObjects);
}

// Native -[NSMutableOrderedSet replaceObjectsInRange:withObjectsFromSet:], 0x1c0187508. Non-external. The
// body of -replaceObjectsInRange:withObjectsFromArray: for an NSSet, read with -getObjects:count:.
- (void)replaceObjectsInRange:(OCFRange)range withObjectsFromSet:(id)set {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (set != nil && !_ONSIsNSSet(set)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: set argument is not an NSSet",
            methodName("replaceObjectsInRange:withObjectsFromSet:")
        );
    }
    NSUInteger count = [self count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("replaceObjectsInRange:withObjectsFromSet:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            count
        );
    }
    NSUInteger otherCount = [set count];
    if (otherCount >> 60 != 0) {
        raiseTemporaryBufferTooLarge(otherCount);
    }
    size_t bufferCount = otherCount > 1 ? otherCount : 1;
    id stackObjects[otherCount < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *objects = stackObjects;
    void *heapObjects = NULL;
    if (otherCount >= 0x101) {
        objects = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = objects;
    }
    [set getObjects:objects count:otherCount];
    [self replaceObjectsInRange:range withObjects:objects count:otherCount];
    free(heapObjects);
}

// Native -[NSMutableOrderedSet setArray:], 0x1c0187940. Non-external. The NSArray check, then
// -removeAllObjects and -insertObjectsFromArray:atIndex: 0 (a tail call); no identity test.
- (void)setArray:(id)array {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: array argument is not an NSArray",
            methodName("setArray:")
        );
    }
    [self removeAllObjects];
    [self insertObjectsFromArray:array atIndex:0];
}

// Native -[NSMutableOrderedSet setObject:atIndex:], 0x1c0187ab4. Non-external. A nil object raises
// NSInvalidArgumentException; an index that is negative or above the count raises NSRangeException. An
// index equal to a fresh count sends -insertObject:atIndex:, any other -replaceObjectAtIndex:withObject:.
- (void)setObject:(id)object atIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (object == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException, 12, "*** %s: object cannot be nil", methodName("setObject:atIndex:")
        );
    }
    NSUInteger count = [self count];
    if ((NSInteger)index < 0 || count < index) {
        raiseIndexBeyondBounds(methodName("setObject:atIndex:"), index, count);
    }
    if (index == [self count]) {
        [self insertObject:object atIndex:index];
    } else {
        [self replaceObjectAtIndex:index withObject:object];
    }
}

// Native -[NSMutableOrderedSet setObject:atIndexedSubscript:], 0x1c0187dd8. Non-external. -setObject:atIndex:
// (a tail call, no prologue).
- (void)setObject:(id)object atIndexedSubscript:(NSUInteger)index {
    [self setObject:object atIndex:index];
}

// Native -[NSMutableOrderedSet setObject:], 0x1c0187ddc. Non-external. A nil object raises
// NSInvalidArgumentException ("object cannot be nil"); then -indexOfObject:. The object already at that index
// (nil for NSNotFound, without a send) identical to the argument does nothing; otherwise the index receives
// the object (-replaceObjectAtIndex:withObject:, a tail call, also for NSNotFound).
- (void)setObject:(id)object {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (object == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException, 12, "*** %s: object cannot be nil", methodName("setObject:")
        );
    }
    NSUInteger index = [self indexOfObject:object];
    id existing = index == kNotFound ? nil : [self objectAtIndex:index];
    if (existing == object) {
        return;
    }
    [self replaceObjectAtIndex:index withObject:object];
}

// Native -[NSMutableOrderedSet setOrderedSet:], 0x1c0187f9c. Non-external. The NSOrderedSet check; an
// argument identical to the receiver does nothing; otherwise -removeAllObjects and
// -insertObjectsFromOrderedSet:atIndex: 0 (a tail call).
- (void)setOrderedSet:(id)orderedSet {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: ordered set argument is not an NSOrderedSet",
            methodName("setOrderedSet:")
        );
    }
    if (self == orderedSet) {
        return;
    }
    [self removeAllObjects];
    [self insertObjectsFromOrderedSet:orderedSet atIndex:0];
}

// Native -[NSMutableOrderedSet setSet:], 0x1c0188144. Non-external. The NSSet check, then -removeAllObjects
// and -insertObjectsFromSet:atIndex: 0 (a tail call).
- (void)setSet:(id)set {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (set != nil && !_ONSIsNSSet(set)) {
        raiseWithLogPack(
            ONSInvalidArgumentException, 12, "*** %s: set argument is not an NSSet", methodName("setSet:")
        );
    }
    [self removeAllObjects];
    [self insertObjectsFromSet:set atIndex:0];
}

// Native -[NSMutableOrderedSet sortedArrayFromRange:options:usingComparator:], 0x1c01882b8. Non-external. The
// superclass's implementation (objc_msgSendSuper2 with the class NSMutableOrderedSet and the method's own
// selector, the selector 0x2387b2f96 named from the image's method lists; the range, options and comparator
// pass through untouched), whose result is sent -copy and autoreleased.
- (id)sortedArrayFromRange:(OCFRange)range
                   options:(NSUInteger)options
           usingComparator:(NSInteger (^)(id, id))comparator {
    struct objc_super superclass = {self, (Class)mutableOrderedSetClass};
    id sorted =
        ((id (*)(struct objc_super *, SEL, OCFRange, NSUInteger, NSInteger (^)(id, id)))objc_msgSendSuper2)(
            &superclass, _cmd, range, options, comparator
        );
    return [[sorted copy] autorelease];
}

// Native -[NSMutableOrderedSet sortRange:options:usingComparator:], 0x1c0188320, with its block
// ___57-[NSMutableOrderedSet sortRange:options:usingComparator:]_block_invoke (0x1c018886c). Non-external. A
// nil comparator raises NSInvalidArgumentException, then the range check. Ranges shorter than 2 do nothing.
// Otherwise (new_id_array check on the length): the range's objects are read into a buffer (stack below
// 0x101 objects, else _CFCreateArrayStorage of the length); an index buffer (stack up to 0x1000 entries, else
// malloc_type_malloc of 8 per entry, type 0x100004000313F17) is sorted by CFSortIndexes with the options and
// a block comparing the objects at two indexes with the comparator (the block captures the comparator and
// the object buffer, in that order); a second object buffer of the same size class receives the objects in
// sorted order and replaces the range through -replaceObjectsInRange:withObjects:count:. The heap index
// buffer, the second buffer and the first buffer are freed in that order. On unwinding the landing pads
// free the second and the first buffer, not the index buffer (0x1c018883c, 0x1c0188850); the heap index
// buffer then leaks, as natively. The block literal's flags are the compiler's (natively 0xc0000000, no
// copy or dispose helpers; descriptor 0x22279d5b0).
- (void)sortRange:(OCFRange)range
            options:(NSUInteger)options
    usingComparator:(NSInteger (^)(id, id))comparator {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (comparator == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: comparator cannot be nil",
            methodName("sortRange:options:usingComparator:")
        );
    }
    NSUInteger count = [self count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("sortRange:options:usingComparator:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            count
        );
    }
    NSUInteger length = (NSUInteger)range.length;
    if (length < 2) {
        return;
    }
    if (length >> 60 != 0) {
        raiseTemporaryBufferTooLarge(length);
    }
    id stackObjects[length < 0x101 ? length : 1];
    size_t allocatedSize = 0;
    id *objects = stackObjects;
    __attribute__((cleanup(releaseTemporaryStorage))) void *heapObjects = NULL;
    if (length >= 0x101) {
        objects = _OCFCreateArrayStorage(length, false, &allocatedSize);
        heapObjects = objects;
    }
    [self getObjects:objects range:range];
    OCFIndex stackIndexes[length > 0x1000 ? 1 : length];
    OCFIndex *indexes = stackIndexes;
    if (length > 0x1000) {
        indexes = malloc_type_malloc(length << 3, 0x100004000313F17ULL);
    }
    OCFSortIndexes(
        indexes, (OCFIndex)length, (OCFOptionFlags)options, ^OCFComparisonResult(OCFIndex i1, OCFIndex i2) {
          return (OCFComparisonResult)comparator(objects[i1], objects[i2]);
        }
    );
    id stackSorted[length < 0x101 ? length : 1];
    size_t sortedAllocatedSize = 0;
    __attribute__((cleanup(releaseTemporaryStorage))) void *heapSorted = NULL;
    if (length >= 0x101) {
        heapSorted = _OCFCreateArrayStorage(length, false, &sortedAllocatedSize);
    }
    id *sorted = length < 0x101 ? stackSorted : (id *)heapSorted;
    for (NSUInteger position = 0; position < length; ++position) {
        sorted[position] = objects[indexes[position]];
    }
    [self replaceObjectsInRange:range withObjects:sorted count:length];
    if (stackIndexes != indexes) {
        free(indexes);
    }
}

// Native -[NSMutableOrderedSet sortWithOptions:usingComparator:], 0x1c0188884. Non-external. A nil comparator
// raises NSInvalidArgumentException; ordered sets of 2 or more objects send
// -sortRange:options:usingComparator: of the whole ordered set.
- (void)sortWithOptions:(NSUInteger)options usingComparator:(NSInteger (^)(id, id))comparator {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (comparator == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: comparator cannot be nil",
            methodName("sortWithOptions:usingComparator:")
        );
    }
    NSUInteger count = [self count];
    if (count >= 2) {
        [self sortRange:(OCFRange){0, (OCFIndex)count} options:options usingComparator:comparator];
    }
}

// Native -[NSMutableOrderedSet sortUsingComparator:], 0x1c0188a28. Non-external.
// -sortWithOptions:usingComparator: inlined with no options.
- (void)sortUsingComparator:(NSInteger (^)(id, id))comparator {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (comparator == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: comparator cannot be nil",
            methodName("sortUsingComparator:")
        );
    }
    NSUInteger count = [self count];
    if (count >= 2) {
        [self sortRange:(OCFRange){0, (OCFIndex)count} options:0 usingComparator:comparator];
    }
}

// Native -[NSMutableOrderedSet unionOrderedSet:], 0x1c0188bc8. Non-external. The NSOrderedSet check (no
// identity test); the argument's count passes the temporary buffer check and sizes a buffer (stack below
// 0x101 objects, at least one slot, else _CFCreateArrayStorage) that -getObjects:range: fills; each buffered
// object is sent -addObject: to the receiver as many times as the argument's -countForObject: answers (the
// object read from the buffer again for every send). The heap buffer is freed; there is no landing pad.
- (void)unionOrderedSet:(id)other {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (other != nil && !_ONSIsNSOrderedSet(other)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: ordered set argument is not an NSOrderedSet",
            methodName("unionOrderedSet:")
        );
    }
    NSUInteger otherCount = [other count];
    if (otherCount >> 60 != 0) {
        raiseTemporaryBufferTooLarge(otherCount);
    }
    size_t bufferCount = otherCount > 1 ? otherCount : 1;
    id stackObjects[otherCount < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *objects = stackObjects;
    void *heapObjects = NULL;
    if (otherCount >= 0x101) {
        objects = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = objects;
    }
    [other getObjects:objects range:(OCFRange){0, (OCFIndex)otherCount}];
    for (NSUInteger index = 0; index < otherCount; ++index) {
        NSUInteger occurrences = [other countForObject:objects[index]];
        for (; occurrences != 0; --occurrences) {
            [self addObject:objects[index]];
        }
    }
    free(heapObjects);
}

// Native -[NSMutableOrderedSet unionSet:], 0x1c0188e70. Non-external. The NSSet check; then the set is
// walked by fast enumeration (16-object batches, mutation check on every object) and each object is sent
// -addObject: to the receiver as many times as the set's -countForObject: answers.
- (void)unionSet:(id)set {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableOrderedSet);
    if (set != nil && !_ONSIsNSSet(set)) {
        raiseWithLogPack(
            ONSInvalidArgumentException, 12, "*** %s: set argument is not an NSSet", methodName("unionSet:")
        );
    }
    for (id object in set) {
        NSUInteger occurrences = [set countForObject:object];
        for (; occurrences != 0; --occurrences) {
            [self addObject:object];
        }
    }
}

// Native +[NSMutableOrderedSet orderedSetWithCapacity:], 0x1c0189084. Non-external. objc_alloc of the
// receiver, -initWithCapacity:, autoreleased (a tail call).
+ (id)orderedSetWithCapacity:(NSUInteger)capacity {
    return [[[self alloc] initWithCapacity:capacity] autorelease];
}

// Native -[NSMutableOrderedSet initWithObjects:count:], 0x1c01890ac. Non-external. No mutation prologue: the
// objects checks of -addObjects:count:, then -initWithCapacity: with the count and
// -insertObjects:count:atIndex: 0 on its result, which is answered.
- (id)initWithObjects:(const id *)objects count:(NSUInteger)count {
    if (objects == NULL && count != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            22,
            "*** %s: pointer to objects array is NULL but length is %lu",
            methodName("initWithObjects:count:"),
            count
        );
    }
    if (count >> 61 != 0) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            22,
            "*** %s: count (%lu) of objects array is ridiculous",
            methodName("initWithObjects:count:"),
            count
        );
    }
    for (NSUInteger index = 0; index < count; ++index) {
        if (objects[index] == nil) {
            raiseWithLogPack(
                ONSInvalidArgumentException,
                22,
                "*** %s: attempt to insert nil object from objects[%lu]",
                methodName("initWithObjects:count:"),
                index
            );
        }
    }
    id orderedSet = [self initWithCapacity:count];
    [orderedSet insertObjects:objects count:count atIndex:0];
    return orderedSet;
}

// Native -[NSMutableOrderedSet classForCoder], 0x1c0189368. Non-external. The class reference of
// NSMutableOrderedSet (off_2227CE388) sent +self (objc_opt_self, a tail call).
- (Class)classForCoder {
    return [ONSMutableOrderedSet self];
}

// Native -[NSMutableOrderedSet insertObject:atIndex:], 0x1c03079e0, with OUTLINED_FUNCTION_0_4 (0x1c0189374:
// __CFLookUpClass("NSMutableOrderedSet")), _2_2 (0x1c0189390: the receiver and selector back into X1 and X2)
// and _1_3 (0x1c0189388: __CFRequireConcreteImplementation as a tail call). Non-external. The abstract
// method: raises through __CFRequireConcreteImplementation with the NSMutableOrderedSet class.
- (void)insertObject:(id)object atIndex:(NSUInteger)index {
    Class abstractClass = (Class)__CFLookUpClass(OF_NAME_STRING(ONSMutableOrderedSet));
    __CFRequireConcreteImplementation(abstractClass, self, _cmd);
}

// Native -[NSMutableOrderedSet removeObjectAtIndex:], 0x1c03079fc, with OUTLINED_FUNCTION_0_4, _2_2 and _1_3.
// Non-external. The abstract method, as -insertObject:atIndex:.
- (void)removeObjectAtIndex:(NSUInteger)index {
    Class abstractClass = (Class)__CFLookUpClass(OF_NAME_STRING(ONSMutableOrderedSet));
    __CFRequireConcreteImplementation(abstractClass, self, _cmd);
}

// Native -[NSMutableOrderedSet replaceObjectAtIndex:withObject:], 0x1c0307a18, with OUTLINED_FUNCTION_0_4,
// _2_2 and _1_3. Non-external. The abstract method, as -insertObject:atIndex:.
- (void)replaceObjectAtIndex:(NSUInteger)index withObject:(id)object {
    Class abstractClass = (Class)__CFLookUpClass(OF_NAME_STRING(ONSMutableOrderedSet));
    __CFRequireConcreteImplementation(abstractClass, self, _cmd);
}

// Native -[NSMutableOrderedSet initWithCapacity:], 0x1c0307a34, with OUTLINED_FUNCTION_0_4 and _2_2.
// Non-external. The abstract method: __CFRequireConcreteImplementation (not a tail call), then nil.
- (id)initWithCapacity:(NSUInteger)capacity {
    Class abstractClass = (Class)__CFLookUpClass(OF_NAME_STRING(ONSMutableOrderedSet));
    __CFRequireConcreteImplementation(abstractClass, self, _cmd);
    return nil;
}

@end

//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSMutableArray.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFSortFunctions.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <alloca.h>
#include <mach-o/loader.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdlib.h>

// Inferred organization: the NSMutableArray methods (0x1c0291dcc-0x1c029d434, after the NSNull methods) lie
// together in the iOS Simulator __text section with their blocks and the unit's OUTLINED_FUNCTION_0_42
// (0x1c029d420), _1_30 (0x1c029d434) and _2_26 (0x1c029d43c); the four abstract methods
// -insertObject:atIndex:, -removeObjectAtIndex:, -replaceObjectAtIndex:withObject: and -initWithCapacity: lie
// in the cold region (0x1c030f9c0-0x1c030fa38). Following the owner's decision, the class has its own
// translation unit; the methods follow in native address order, the outlined fragments inside the methods
// that call them. The image defines no category on NSMutableArray. Objective-C compiled with manual reference
// counting (objc_retain, objc_release and objc_autorelease calls, no ARC entry points). Every method of the
// class that the image holds is restored here; the evidence is the IDA capture
// iphone-simulator-corpus-delta8.json (family B-mutable-array, 74 entries). Unless a definition says
// otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly
// read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).
//
// Shared shape of the mutation methods: each sends -_mutate to the receiver first, then calls the
// ThreadSanitizer write hook (when set) with the receiver, the method's return address (X30 at entry) and
// __CFTSANTagMutableArray, then checks its arguments in the order written. Argument errors raise through
// +[NSException exceptionWithName:reason:userInfo:osLogPack:size:] with an os_log pack built on the stack:
// _os_log_pack_size of the format's buffer size (12 for "%s", 22 for "%s %lu", 32 for three arguments, 42
// for four), the pack filled through _os_log_pack_fill (errno 0, the image's mach header, the format), the
// arguments encoded by the compiler's __builtin_os_log_format (natively inline for one or two arguments,
// __os_log_helper_1_2_3_8_32_8_0_8_0 and __os_log_helper_1_2_4_8_32_8_0_8_0_8_0 for three and four), and the
// reason made with kCFAllocatorSystemDefault and the same format as a constant string, autoreleased through
// _CFAutoreleasePoolAddObject, with no user info. The first format argument is the method's name as a
// literal C string ("-[NSMutableArray addObject:]"), spelled here with the class's selected name so that the
// system-name build carries the native bytes. The temporary object buffers follow the native new_id_array
// pattern: a count with any of the top four bits set throws NSGenericException with a CFRelease'd reason;
// otherwise up to 256 objects live on the stack (at least one slot) and larger counts use
// _CFCreateArrayStorage, freed (free(NULL) for the stack case) after use.

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// libobjc's super send with the current class (the entry point [super ...] compiles to); the SDK installs
// no declaration. Supplied, never reimplemented.
extern id objc_msgSendSuper2(struct objc_super *super, SEL op, ...);

// Class symbols the methods use directly (_OBJC_CLASS_$_NSException, _OBJC_CLASS_$_NSSet,
// _OBJC_CLASS_$_NSMutableArray).
extern char exceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char setClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSSet));
extern char mutableArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableArray));

// Selectors the methods send to their arguments, to classes and to the receiver that the restored classes of
// this unit do not declare (NSArray's, NSIndexSet's, NSOrderedSet's, NSSet's and NSException's); declared for
// the message sends only. The category below adopts it so that the receiver's own sends resolve; neither a
// protocol object nor a category is emitted.
@protocol MutableArrayMessages

+ (id)array;
+ (id)setWithArray:(id)array range:(OCFRange)range copyItems:(BOOL)copyItems;
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
- (id)objectsAtIndexes:(id)indexes;
- (id)sortedArrayFromRange:(OCFRange)range
                   options:(NSUInteger)options
           usingComparator:(NSInteger (^)(id, id))comparator;
- (BOOL)containsObject:(id)object;
- (BOOL)containsObject:(id)object inRange:(OCFRange)range;
- (NSUInteger)lastIndex;
- (NSUInteger)rangeCount;
- (OCFRange)rangeAtIndex:(NSUInteger)index;
- (NSUInteger)countOfIndexesInRange:(OCFRange)range;
- (id)initWithObjects:(const id *)objects count:(NSUInteger)count;
- (id)_initByAdoptingBuffer:(id *)buffer count:(NSUInteger)count size:(NSUInteger)size;

@end

@interface ONSMutableArray (MutableArrayMessages) <MutableArrayMessages>
@end

// The ThreadSanitizer hooks every method inlines: load the hook, and when it is set, call it with the
// receiver, the method's return address and the tag. tsanReadCheck and tsanWriteCheck are local descriptive
// names.
static inline void tsanReadCheck(const void *object, void *returnAddress, void *tag) {
    void (*hook)(const void *, void *, void *) =
        (void (*)(const void *, void *, void *))__cf_tsanReadFunction;
    if (hook != NULL) {
        hook(object, returnAddress, tag);
    }
}

static inline void tsanWriteCheck(const void *object, void *returnAddress, void *tag) {
    void (*hook)(const void *, void *, void *) =
        (void (*)(const void *, void *, void *))__cf_tsanWriteFunction;
    if (hook != NULL) {
        hook(object, returnAddress, tag);
    }
}

// The method name every reason starts with, "-[NSMutableArray selector]" in the system-name build.
#define methodName(selectorText) "-[" OF_NAME_STRING(ONSMutableArray) " " selectorText "]"

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

// The inlined index check failure: "beyond bounds [0 .. count - 1]" for a nonempty array, "beyond bounds
// for empty array" otherwise (NSRangeException).
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
            ONSRangeException, 22, "*** %s: index %lu beyond bounds for empty array", method, index          \
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
            "*** %s: range {%lu, %lu} extends beyond bounds for empty array",                                \
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
            "*** %s: index %lu in index set beyond bounds for empty array",                                  \
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

// The cleanup of the temporary buffers whose landing pads free them before resuming (-setArray: and
// -sortRange:options:usingComparator:). releaseTemporaryStorage is a local descriptive name.
static inline void releaseTemporaryStorage(void **storage) {
    free(*storage);
}

@implementation ONSMutableArray

// Native -[NSMutableArray _mutate], 0x1c0291dcc. Non-external. Does nothing.
- (void)_mutate {
}

// Native -[NSMutableArray addObject:], 0x1c0291dd0. Non-external. A nil object raises
// NSInvalidArgumentException; otherwise -insertObject:atIndex: at the count (a tail call).
- (void)addObject:(id)object {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    if (object == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException, 12, "*** %s: object cannot be nil", methodName("addObject:")
        );
    }
    [self insertObject:object atIndex:[self count]];
}

// Native -[NSMutableArray addObjects:count:], 0x1c0291f34. Non-external. A NULL objects pointer with a
// nonzero count, a count with any of the top three bits set and a nil object (the first, by index) raise
// NSInvalidArgumentException in that order; then -insertObjects:count:atIndex: at the count.
- (void)addObjects:(const id *)objects count:(NSUInteger)count {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray addObjectsFromArray:range:], 0x1c0292224. Non-external. A non-nil argument that is
// not an NSArray raises NSInvalidArgumentException; a range outside the argument's count (negative location
// or length, or an end above the count) raises NSRangeException; then -insertObjectsFromArray:range:atIndex:
// at the receiver's count (a tail call).
- (void)addObjectsFromArray:(id)array range:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray addObjectsFromArray:], 0x1c0292524. Non-external. Before anything else, a non-nil
// argument that is not an NSArray is logged (CFLog level 3, "%@" with the autoreleased reason "*** %s: array
// argument is not an NSArray"); when it responds to -countByEnumeratingWithState:objects:count: (the
// selector 0x2387adf6e, named from the image's method lists), its objects are collected by fast enumeration
// (16-object buffer, mutation check on every object) into a new +[NSMutableArray array] that replaces the
// argument. Then the mutation prologue, the NSArray check (raising for an argument still not an NSArray),
// and -insertObjectsFromArray:range:atIndex: of the whole argument at the receiver's count; the receiver's
// count is read before the argument's.
- (void)addObjectsFromArray:(id)array {
    void *returnAddress = __builtin_return_address(0);
    if (array != nil && !_ONSIsNSArray(array)) {
        OCFStringRef reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("*** %s: array argument is not an NSArray"),
            methodName("addObjectsFromArray:")
        );
        OCFLog(3, OF_STR("%@"), _OCFAutoreleasePoolAddObject(NULL, reason));
        if ([array respondsToSelector:@selector(countByEnumeratingWithState:objects:count:)]) {
            id collected = [(id)mutableArrayClass array];
            for (id object in array) {
                [collected addObject:object];
            }
            array = collected;
        }
    }
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray addObjectsFromOrderedSet:range:], 0x1c02927d0. Non-external. The body of
// -addObjectsFromArray:range: for an NSOrderedSet argument ("ordered set argument is not an NSOrderedSet"),
// ending in -insertObjectsFromOrderedSet:range:atIndex: (a tail call).
- (void)addObjectsFromOrderedSet:(id)orderedSet range:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray addObjectsFromOrderedSet:], 0x1c0292ad0. Non-external. The NSOrderedSet check,
// then -insertObjectsFromOrderedSet:range:atIndex: of the whole argument at the receiver's count (a tail
// call); the receiver's count is read before the argument's.
- (void)addObjectsFromOrderedSet:(id)orderedSet {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray addObjectsFromSet:], 0x1c0292c58. Non-external. The NSSet check ("set argument is
// not an NSSet"), then -insertObjectsFromSet:atIndex: at the count (a tail call).
- (void)addObjectsFromSet:(id)set {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray exchangeObjectAtIndex:withObjectAtIndex:], 0x1c0292dcc. Non-external. Each index is
// checked against a fresh count (negative or at/above the count raises NSRangeException). Equal indexes do
// nothing; otherwise the objects at the lower and the higher index are read (lower first), the lower one is
// retained, the lower index receives the higher object and the higher index the lower object, and the lower
// object is released (a tail call).
- (void)exchangeObjectAtIndex:(NSUInteger)index1 withObjectAtIndex:(NSUInteger)index2 {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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
    [self replaceObjectAtIndex:lower withObject:higherObject];
    [self replaceObjectAtIndex:higher withObject:lowerObject];
    [lowerObject release];
}

// Native -[NSMutableArray insertObjects:count:atIndex:], 0x1c02931d0. Non-external. The argument checks of
// -addObjects:count:, then the index check against the count (negative or above the count raises
// NSRangeException), then -insertObject:atIndex: for each object at consecutive indexes.
- (void)insertObjects:(const id *)objects count:(NSUInteger)count atIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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
    NSUInteger arrayCount = [self count];
    if ((NSInteger)index < 0 || arrayCount < index) {
        raiseIndexBeyondBounds(methodName("insertObjects:count:atIndex:"), index, arrayCount);
    }
    for (; count != 0; --count) {
        [self insertObject:*objects++ atIndex:index++];
    }
}

// Native -[NSMutableArray insertObjectsFromArray:range:atIndex:], 0x1c0293644. Non-external. The NSArray
// check, the range check against the argument's count, the index check against the receiver's count, then a
// temporary buffer of range.length objects filled by -getObjects:range: and -insertObjects:count:atIndex:.
- (void)insertObjectsFromArray:(id)array range:(OCFRange)range atIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray insertObjectsFromArray:atIndex:], 0x1c0293c24. Non-external. The NSArray check,
// the index check, then the whole argument through a temporary buffer (-getObjects:range: from 0) and
// -insertObjects:count:atIndex:.
- (void)insertObjectsFromArray:(id)array atIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray insertObjectsFromOrderedSet:range:atIndex:], 0x1c0294048. Non-external. The body
// of -insertObjectsFromArray:range:atIndex: for an NSOrderedSet argument.
- (void)insertObjectsFromOrderedSet:(id)orderedSet range:(OCFRange)range atIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray insertObjectsFromOrderedSet:atIndex:], 0x1c0294628. Non-external. The body of
// -insertObjectsFromArray:atIndex: for an NSOrderedSet argument.
- (void)insertObjectsFromOrderedSet:(id)orderedSet atIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray insertObjectsFromSet:atIndex:], 0x1c0294a4c. Non-external. The body of
// -insertObjectsFromArray:atIndex: for an NSSet argument, whose objects are read with -getObjects:count:.
- (void)insertObjectsFromSet:(id)set atIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray insertObjects:atIndexes:], 0x1c0294e6c. Non-external. The NSArray check, a nil
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
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray moveObjectsAtIndexes:toIndex:], 0x1c02954e8. Non-external. A nil index set and a
// non-NSIndexSet raise NSInvalidArgumentException. The last-index check is the one of
// -insertObjects:atIndexes:, with the array's count read from nil (the native code sends -count to nil, so
// the bound is the receiver's count). The destination is checked against the count less
// -countOfIndexesInRange:{0, index} (negative or above it raises NSRangeException, "for empty array" when it
// is zero). Then -objectsAtIndexes:, -removeObjectsAtIndexes:, and the moved objects through a temporary
// buffer into -insertObjects:count:atIndex: at the destination.
- (void)moveObjectsAtIndexes:(id)indexes toIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray removeAllObjects], 0x1c0295be8. Non-external. -removeObjectAtIndex: from the last
// index down to 0.
- (void)removeAllObjects {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    NSUInteger count = [self count];
    for (NSUInteger index = count; index != 0; --index) {
        [self removeObjectAtIndex:index - 1];
    }
}

// Native -[NSMutableArray removeFirstObject], 0x1c0295c60. Non-external. -removeObjectAtIndex: 0 when the
// array is not empty (a tail call).
- (void)removeFirstObject {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    if ([self count] != 0) {
        [self removeObjectAtIndex:0];
    }
}

// Native -[NSMutableArray removeLastObject], 0x1c0295cd0. Non-external. The count is read before
// _CFExecutableLinkedOnOrAfter(7). Apps linked on or after version 7 remove the last object of a nonempty
// array and do nothing for an empty one. Older apps send -removeObjectAtIndex: with the last index (0 for an
// empty array) and then, for a nonempty array, -removeObjectAtIndex: with the same index again (the branch
// after the first send rejoins the new-app path); this second send is native behavior and is kept.
- (void)removeLastObject {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    NSUInteger count = [self count];
    if (!_OCFExecutableLinkedOnOrAfter(7)) {
        [self removeObjectAtIndex:count == 0 ? 0 : count - 1];
    }
    if (count != 0) {
        [self removeObjectAtIndex:count - 1];
    }
}

// Native -[NSMutableArray removeObjectsInRange:], 0x1c0295d64. Non-external. The range check against the
// count, then range.length sends of -removeObjectAtIndex: at range.location.
- (void)removeObjectsInRange:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray removeObject:inRange:], 0x1c0295f8c. Non-external. The range check, then, with the
// object retained, the range from its last index down: an element identical to the object, or one the
// object answers -isEqual: (object as receiver) YES for, is removed. The object is released after the loop.
- (void)removeObject:(id)object inRange:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    NSUInteger count = [self count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("removeObject:inRange:"), (NSUInteger)range.location, (NSUInteger)range.length, count
        );
    }
    [object retain];
    NSUInteger index = (NSUInteger)range.length + (NSUInteger)range.location - 1;
    for (NSUInteger remaining = (NSUInteger)range.length; remaining != 0; --remaining) {
        id element = [self objectAtIndex:index];
        if (element == object || [object isEqual:element]) {
            [self removeObjectAtIndex:index];
        }
        --index;
    }
    [object release];
}

// Native -[NSMutableArray removeObject:], 0x1c02961fc. Non-external. As -removeObject:inRange: over the
// whole array (count read before the retain), without a range check; the release is a tail call.
- (void)removeObject:(id)object {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    NSUInteger count = [self count];
    [object retain];
    for (NSUInteger index = count; index != 0; --index) {
        id element = [self objectAtIndex:index - 1];
        if (element == object || [object isEqual:element]) {
            [self removeObjectAtIndex:index - 1];
        }
    }
    [object release];
}

// Native -[NSMutableArray removeObjectIdenticalTo:inRange:], 0x1c02962b8. Non-external. As
// -removeObject:inRange:, matching identical elements only.
- (void)removeObjectIdenticalTo:(id)object inRange:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    NSUInteger count = [self count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("removeObjectIdenticalTo:inRange:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            count
        );
    }
    [object retain];
    NSUInteger index = (NSUInteger)range.length + (NSUInteger)range.location - 1;
    for (NSUInteger remaining = (NSUInteger)range.length; remaining != 0; --remaining) {
        if ([self objectAtIndex:index] == object) {
            [self removeObjectAtIndex:index];
        }
        --index;
    }
    [object release];
}

// Native -[NSMutableArray removeObjectIdenticalTo:], 0x1c0296518. Non-external. As -removeObject:, matching
// identical elements only.
- (void)removeObjectIdenticalTo:(id)object {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    NSUInteger count = [self count];
    [object retain];
    for (NSUInteger index = count; index != 0; --index) {
        if ([self objectAtIndex:index - 1] == object) {
            [self removeObjectAtIndex:index - 1];
        }
    }
    [object release];
}

// Native -[NSMutableArray removeObjectsAtIndexes:], 0x1c02965c4. Non-external. A nil index set and a
// non-NSIndexSet raise NSInvalidArgumentException; the last-index check of -moveObjectsAtIndexes:toIndex:
// (with the nil -count); then -removeObjectsInRange: for each range of the index set from the last range
// down.
- (void)removeObjectsAtIndexes:(id)indexes {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray removeObjectsInRange:inArray:range:], 0x1c029697c. Non-external. The receiver's
// range is checked first, then the NSArray check, then the array's range. Inside an autorelease pool
// (_CFAutoreleasePoolPush/Pop), +[NSSet setWithArray:range:copyItems:NO] of the array's range is made, and
// the receiver's range is walked from its last offset down: when the set contains the object at
// range.location + offset, -removeObjectAtIndex: is sent with the offset alone.
// CoreFoundation defect (kept): the removal index omits range.location, so for a nonzero location the
// object removed is not the one tested (assembly 0x1c0296a58-0x1c0296a7c: X2 = X19 + X20 for
// -objectAtIndex:, X2 = X20 for -removeObjectAtIndex:; confidence: verified). The removal index stays below
// the count, so the behavior is well defined.
- (void)removeObjectsInRange:(OCFRange)range inArray:(id)array range:(OCFRange)otherRange {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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
    void *pool = _OCFAutoreleasePoolPush();
    id set = [(id)setClass setWithArray:array range:otherRange copyItems:NO];
    for (NSUInteger offset = (NSUInteger)range.length; offset != 0; --offset) {
        if ([set containsObject:[self objectAtIndex:(NSUInteger)range.location + offset - 1]]) {
            [self removeObjectAtIndex:offset - 1];
        }
    }
    _OCFAutoreleasePoolPop(pool);
}

// Native -[NSMutableArray removeObjectsInRange:inArray:], 0x1c0296e30. Non-external. The receiver's range
// check, the NSArray check, then -removeObjectsInRange:inArray:range: with the whole array (a tail call).
- (void)removeObjectsInRange:(OCFRange)range inArray:(id)array {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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
    [self removeObjectsInRange:range inArray:array range:(OCFRange){0, (OCFIndex)[array count]}];
}

// Native -[NSMutableArray removeObjectsInArray:range:], 0x1c0297130. Non-external. The NSArray check, the
// range check against the array's count, then -removeObjectsInRange:inArray:range: over the whole receiver
// (a tail call).
- (void)removeObjectsInArray:(id)array range:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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
    [self removeObjectsInRange:(OCFRange){0, (OCFIndex)[self count]} inArray:array range:range];
}

// Native -[NSMutableArray removeObjectsInArray:], 0x1c0297434. Non-external. The NSArray check, then
// -removeObjectsInRange:inArray:range: over the whole receiver and the whole array (a tail call); the
// receiver's count is read before the array's.
- (void)removeObjectsInArray:(id)array {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: array argument is not an NSArray",
            methodName("removeObjectsInArray:")
        );
    }
    NSUInteger count = [self count];
    NSUInteger otherCount = [array count];
    [self removeObjectsInRange:(OCFRange){0, (OCFIndex)count}
                       inArray:array
                         range:(OCFRange){0, (OCFIndex)otherCount}];
}

// Native -[NSMutableArray removeObjectsInRange:inOrderedSet:range:], 0x1c02975c0. Non-external. As
// -removeObjectsInRange:inArray:range: for an NSOrderedSet, without an autorelease pool or a set: each
// object at range.location + offset (from the last offset down) is tested with -containsObject:inRange: of
// the ordered set's range, and a match sends -removeObjectAtIndex: with the offset alone.
// CoreFoundation defect (kept): the removal index omits range.location, as in
// -removeObjectsInRange:inArray:range: (assembly 0x1c029766c-0x1c0297698; confidence: verified).
- (void)removeObjectsInRange:(OCFRange)range inOrderedSet:(id)orderedSet range:(OCFRange)otherRange {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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
    for (NSUInteger offset = (NSUInteger)range.length; offset != 0; --offset) {
        id element = [self objectAtIndex:(NSUInteger)range.location + offset - 1];
        if ([orderedSet containsObject:element inRange:otherRange]) {
            [self removeObjectAtIndex:offset - 1];
        }
    }
}

// Native -[NSMutableArray removeObjectsInRange:inOrderedSet:], 0x1c0297a50. Non-external. The body of
// -removeObjectsInRange:inArray: for an NSOrderedSet.
- (void)removeObjectsInRange:(OCFRange)range inOrderedSet:(id)orderedSet {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray removeObjectsInOrderedSet:range:], 0x1c0297d50. Non-external. The body of
// -removeObjectsInArray:range: for an NSOrderedSet.
- (void)removeObjectsInOrderedSet:(id)orderedSet range:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray removeObjectsInOrderedSet:], 0x1c0298054. Non-external. The body of
// -removeObjectsInArray: for an NSOrderedSet.
- (void)removeObjectsInOrderedSet:(id)orderedSet {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray removeObjectsInRange:inSet:], 0x1c02981e0. Non-external. The receiver's range
// check, the NSSet check, then each object at range.location + offset (from the last offset down) tested
// with the set's -containsObject:, a match sending -removeObjectAtIndex: with the offset alone.
// CoreFoundation defect (kept): the removal index omits range.location, as in
// -removeObjectsInRange:inArray:range: (assembly 0x1c0298264-0x1c029828c; confidence: verified).
- (void)removeObjectsInRange:(OCFRange)range inSet:(id)set {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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
    for (NSUInteger offset = (NSUInteger)range.length; offset != 0; --offset) {
        if ([set containsObject:[self objectAtIndex:(NSUInteger)range.location + offset - 1]]) {
            [self removeObjectAtIndex:offset - 1];
        }
    }
}

// Native -[NSMutableArray removeObjectsInSet:], 0x1c02984fc. Non-external. The NSSet check, then
// -removeObjectsInRange:inSet: over the whole receiver (a tail call).
- (void)removeObjectsInSet:(id)set {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    if (set != nil && !_ONSIsNSSet(set)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: set argument is not an NSSet",
            methodName("removeObjectsInSet:")
        );
    }
    [self removeObjectsInRange:(OCFRange){0, (OCFIndex)[self count]} inSet:set];
}

// Native -[NSMutableArray removeObjectsAtIndexes:options:passingTest:], 0x1c0298674. Non-external. A nil
// index set, a non-NSIndexSet and a nil predicate raise NSInvalidArgumentException in that order. Options
// with bit 1 set (NSEnumerationReverse) lose bit 0 (NSEnumerationConcurrent) and bits 60 and 62; the options
// are then marked with bits 60 and 62 (0x5000000000000000) for __NSArrayParameterCheckIterate and
// __NSArrayGetIndexesPassingTest, and the ranges of the resulting index set are removed from the last down.
- (void)removeObjectsAtIndexes:(id)indexes
                       options:(NSUInteger)options
                   passingTest:(BOOL (^)(id, NSUInteger, BOOL *))predicate {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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
    __NSArrayParameterCheckIterate(self, _cmd, predicate, options | UINT64_C(0x5000000000000000), indexes);
    id passing =
        __NSArrayGetIndexesPassingTest(self, predicate, options | UINT64_C(0x5000000000000000), indexes);
    NSUInteger rangeCount = [passing rangeCount];
    for (NSUInteger rangeIndex = rangeCount; rangeIndex != 0; --rangeIndex) {
        OCFRange range = [passing rangeAtIndex:rangeIndex - 1];
        [self removeObjectsInRange:range];
    }
}

// Native -[NSMutableArray removeObjectsWithOptions:passingTest:], 0x1c0298954. Non-external. A nil predicate
// raises NSInvalidArgumentException. Options with bit 1 set lose bits 0 and 62; the options are marked with
// bit 62 (0x4000000000000000) and the passing indexes of the whole array (nil index set) are removed range by
// range from the last down.
- (void)removeObjectsWithOptions:(NSUInteger)options passingTest:(BOOL (^)(id, NSUInteger, BOOL *))predicate {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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
    __NSArrayParameterCheckIterate(self, _cmd, predicate, options | UINT64_C(0x4000000000000000), nil);
    id passing = __NSArrayGetIndexesPassingTest(self, predicate, options | UINT64_C(0x4000000000000000), nil);
    NSUInteger rangeCount = [passing rangeCount];
    for (NSUInteger rangeIndex = rangeCount; rangeIndex != 0; --rangeIndex) {
        OCFRange range = [passing rangeAtIndex:rangeIndex - 1];
        [self removeObjectsInRange:range];
    }
}

// Native -[NSMutableArray removeObjectsPassingTest:], 0x1c0298b28. Non-external. No mutation prologue: a nil
// predicate raises NSInvalidArgumentException; otherwise -removeObjectsWithOptions:passingTest: with no
// options (a tail call).
- (void)removeObjectsPassingTest:(BOOL (^)(id, NSUInteger, BOOL *))predicate {
    if (predicate == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: predicate cannot be nil",
            methodName("removeObjectsPassingTest:")
        );
    }
    [self removeObjectsWithOptions:0 passingTest:predicate];
}

// Native -[NSMutableArray replaceObject:inRange:], 0x1c0298c44. Non-external. A nil object raises
// NSInvalidArgumentException before the count is read; then the range check, and, with the object retained,
// the range from its last index down: an element not identical to the object that the object answers
// -isEqual: YES for is replaced by the object.
- (void)replaceObject:(id)object inRange:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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
    [object retain];
    NSUInteger index = (NSUInteger)range.length + (NSUInteger)range.location - 1;
    for (NSUInteger remaining = (NSUInteger)range.length; remaining != 0; --remaining) {
        id element = [self objectAtIndex:index];
        if (element != object && [object isEqual:element]) {
            [self replaceObjectAtIndex:index withObject:object];
        }
        --index;
    }
    [object release];
}

// Native -[NSMutableArray replaceObject:], 0x1c0298f78. Non-external. As -replaceObject:inRange: over the
// whole array, without a range check.
- (void)replaceObject:(id)object {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    if (object == nil) {
        raiseWithLogPack(
            ONSInvalidArgumentException, 12, "*** %s: object cannot be nil", methodName("replaceObject:")
        );
    }
    NSUInteger count = [self count];
    [object retain];
    for (NSUInteger index = count; index != 0; --index) {
        id element = [self objectAtIndex:index - 1];
        if (element != object && [object isEqual:element]) {
            [self replaceObjectAtIndex:index - 1 withObject:object];
        }
    }
    [object release];
}

// Native -[NSMutableArray replaceObjectsInRange:withObjects:count:], 0x1c029912c. Non-external. The range
// check against the count, then the objects checks of -addObjects:count:. For a nonzero count every object
// whose pointer value is not negative (tagged pointers are skipped) is retained first. The first
// min(range.length, count) indexes of the range are replaced one by one (-replaceObjectAtIndex:withObject:);
// a longer range has its remainder removed (-removeObjectsInRange:), a longer object list has its remainder
// inserted after the replaced indexes (-insertObjects:count:atIndex:). The retained objects are released
// afterwards, again skipping negative pointer values.
- (void)replaceObjectsInRange:(OCFRange)range withObjects:(const id *)objects count:(NSUInteger)count {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    NSUInteger arrayCount = [self count];
    if (range.location < 0 || range.length < 0 ||
        arrayCount < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("replaceObjectsInRange:withObjects:count:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            arrayCount
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
    if (count != 0) {
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
            id object = objects[index];
            if ((intptr_t)object >= 0) {
                [object retain];
            }
        }
    }
    NSUInteger length = (NSUInteger)range.length;
    NSUInteger common = length < count ? length : count;
    NSUInteger index = (NSUInteger)range.location;
    for (NSUInteger position = 0; position < common; ++position) {
        [self replaceObjectAtIndex:index++ withObject:objects[position]];
    }
    if (length > count) {
        [self removeObjectsInRange:(OCFRange){
                                       (OCFIndex)(common + (NSUInteger)range.location),
                                       (OCFIndex)(length - common)
                                   }];
    } else if (count > length) {
        [self insertObjects:&objects[common]
                      count:count - common
                    atIndex:common + (NSUInteger)range.location];
    }
    for (NSUInteger position = 0; position < count; ++position) {
        id object = objects[position];
        if ((intptr_t)object >= 0) {
            [object release];
        }
    }
}

// Native -[NSMutableArray replaceObjectsAtIndexes:withObjects:], 0x1c0299630. Non-external. The checks of
// -insertObjects:atIndexes: (NSArray check, nil index set, NSIndexSet check, the last index against the
// receiver's count plus the array's count, the differing-counts raise), then the array's objects through a
// temporary buffer, each range of the index set replaced in order by
// -replaceObjectsInRange:withObjects:count: with the range's length of buffered objects.
- (void)replaceObjectsAtIndexes:(id)indexes withObjects:(id)objects {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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
    NSUInteger consumed = 0;
    for (NSUInteger rangeIndex = 0; rangeIndex < [indexes rangeCount]; ++rangeIndex) {
        OCFRange range = [indexes rangeAtIndex:rangeIndex];
        [self replaceObjectsInRange:range withObjects:&buffer[consumed] count:(NSUInteger)range.length];
        consumed += (NSUInteger)range.length;
    }
    free(heapObjects);
}

// Native -[NSMutableArray replaceObjectsInRange:withObjectsFromArray:range:], 0x1c0299cb0. Non-external. The
// NSArray check, the array's range against its count, the receiver's range against its count, then the
// array's range through a temporary buffer into -replaceObjectsInRange:withObjects:count:.
- (void)replaceObjectsInRange:(OCFRange)range withObjectsFromArray:(id)array range:(OCFRange)otherRange {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray replaceObjectsInRange:withObjectsFromArray:], 0x1c029a2a0. Non-external. The
// NSArray check, the receiver's range check, then the whole array through a temporary buffer into
// -replaceObjectsInRange:withObjects:count:.
- (void)replaceObjectsInRange:(OCFRange)range withObjectsFromArray:(id)array {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray replaceObjectsInRange:withObjectsFromOrderedSet:range:], 0x1c029a6dc. Non-external.
// The body of -replaceObjectsInRange:withObjectsFromArray:range: for an NSOrderedSet.
- (void)replaceObjectsInRange:(OCFRange)range
    withObjectsFromOrderedSet:(id)orderedSet
                        range:(OCFRange)otherRange {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray replaceObjectsInRange:withObjectsFromOrderedSet:], 0x1c029accc. Non-external. The
// body of -replaceObjectsInRange:withObjectsFromArray: for an NSOrderedSet.
- (void)replaceObjectsInRange:(OCFRange)range withObjectsFromOrderedSet:(id)orderedSet {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray replaceObjectsInRange:withObjectsFromSet:], 0x1c029b108. Non-external. The body of
// -replaceObjectsInRange:withObjectsFromArray: for an NSSet, read with -getObjects:count:.
- (void)replaceObjectsInRange:(OCFRange)range withObjectsFromSet:(id)set {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray setObject:atIndex:], 0x1c029b540. Non-external. A nil object raises
// NSInvalidArgumentException; an index that is negative or above the count raises NSRangeException. An
// index equal to a fresh count sends -insertObject:atIndex:, any other -replaceObjectAtIndex:withObject:.
- (void)setObject:(id)object atIndex:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray setObject:atIndexedSubscript:], 0x1c029b864. Non-external. The mutation prologue,
// then -setObject:atIndex: (a tail call).
- (void)setObject:(id)object atIndexedSubscript:(NSUInteger)index {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    [self setObject:object atIndex:index];
}

// Native -[NSMutableArray setArray:], 0x1c029b8d0. Non-external. The NSArray check; an argument identical to
// the receiver does nothing. Otherwise the receiver's count is read, then the argument's objects through a
// temporary buffer into -replaceObjectsInRange:withObjects:count: over the whole receiver. The heap buffer
// is freed on unwinding too (landing pad 0x1c029bb54).
- (void)setArray:(id)array {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: array argument is not an NSArray",
            methodName("setArray:")
        );
    }
    if (self == array) {
        return;
    }
    NSUInteger count = [self count];
    NSUInteger otherCount = [array count];
    if (otherCount >> 60 != 0) {
        raiseTemporaryBufferTooLarge(otherCount);
    }
    size_t bufferCount = otherCount > 1 ? otherCount : 1;
    id stackObjects[otherCount < 0x101 ? bufferCount : 1];
    size_t allocatedSize = 0;
    id *objects = stackObjects;
    __attribute__((cleanup(releaseTemporaryStorage))) void *heapObjects = NULL;
    if (otherCount >= 0x101) {
        objects = _OCFCreateArrayStorage(bufferCount, false, &allocatedSize);
        heapObjects = objects;
    }
    [array getObjects:objects range:(OCFRange){0, (OCFIndex)otherCount}];
    [self replaceObjectsInRange:(OCFRange){0, (OCFIndex)count} withObjects:objects count:otherCount];
}

// Native -[NSMutableArray sortedArrayFromRange:options:usingComparator:], 0x1c029bb68. Non-external. The
// superclass's implementation (objc_msgSendSuper2 with the class NSMutableArray and the method's own
// selector, the selector 0x2387b2f96 named from the image's method lists; the range, options and comparator
// pass through untouched), whose result is sent -copy and autoreleased.
- (id)sortedArrayFromRange:(OCFRange)range
                   options:(NSUInteger)options
           usingComparator:(NSInteger (^)(id, id))comparator {
    struct objc_super superclass = {self, (Class)mutableArrayClass};
    id sorted =
        ((id (*)(struct objc_super *, SEL, OCFRange, NSUInteger, NSInteger (^)(id, id)))objc_msgSendSuper2)(
            &superclass, _cmd, range, options, comparator
        );
    return [[sorted copy] autorelease];
}

// Native -[NSMutableArray setOrderedSet:], 0x1c029bbd0. Non-external. The NSOrderedSet check, then, without
// an identity test, the receiver's count and the ordered set's objects through a temporary buffer into
// -replaceObjectsInRange:withObjects:count: over the whole receiver.
- (void)setOrderedSet:(id)orderedSet {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    if (orderedSet != nil && !_ONSIsNSOrderedSet(orderedSet)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: ordered set argument is not an NSOrderedSet",
            methodName("setOrderedSet:")
        );
    }
    NSUInteger count = [self count];
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
    [self replaceObjectsInRange:(OCFRange){0, (OCFIndex)count} withObjects:objects count:otherCount];
    free(heapObjects);
}

// Native -[NSMutableArray setSet:], 0x1c029be50. Non-external. The body of -setOrderedSet: for an NSSet,
// read with -getObjects:count:.
- (void)setSet:(id)set {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    if (set != nil && !_ONSIsNSSet(set)) {
        raiseWithLogPack(
            ONSInvalidArgumentException, 12, "*** %s: set argument is not an NSSet", methodName("setSet:")
        );
    }
    NSUInteger count = [self count];
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
    [self replaceObjectsInRange:(OCFRange){0, (OCFIndex)count} withObjects:objects count:otherCount];
    free(heapObjects);
}

// Native -[NSMutableArray sortRange:options:usingComparator:], 0x1c029c0cc, with its block
// ___52-[NSMutableArray sortRange:options:usingComparator:]_block_invoke (0x1c029c618). Non-external. A nil
// comparator raises NSInvalidArgumentException, then the range check. Ranges shorter than 2 do nothing.
// Otherwise (new_id_array check on the length): the range's objects are read into a buffer (stack below
// 0x101 objects, else _CFCreateArrayStorage of the length); an index buffer (stack up to 0x1000 entries, else
// malloc_type_malloc of 8 per entry, type 0x100004000313F17) is sorted by CFSortIndexes with the options and
// a block comparing the objects at two indexes with the comparator (the block captures the comparator and
// the object buffer, in that order); a second object buffer of the same size class receives the objects in
// sorted order and replaces the range through -replaceObjectsInRange:withObjects:count:. The heap index
// buffer, the second buffer and the first buffer are freed in that order. On unwinding the landing pads
// free the second and the first buffer, not the index buffer (0x1c029c5e8, 0x1c029c5fc); the heap index
// buffer then leaks, as natively. The block literal's flags are the compiler's (natively 0xc0000000, no
// copy or dispose helpers; descriptor 0x22279d5b0).
- (void)sortRange:(OCFRange)range
            options:(NSUInteger)options
    usingComparator:(NSInteger (^)(id, id))comparator {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray sortWithOptions:usingComparator:], 0x1c029c630. Non-external. A nil comparator
// raises NSInvalidArgumentException; arrays of 2 or more objects send -sortRange:options:usingComparator: of
// the whole array.
- (void)sortWithOptions:(NSUInteger)options usingComparator:(NSInteger (^)(id, id))comparator {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray sortUsingComparator:], 0x1c029c7d4. Non-external. -sortWithOptions:usingComparator:
// inlined with no options.
- (void)sortUsingComparator:(NSInteger (^)(id, id))comparator {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
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

// Native -[NSMutableArray sortUsingFunction:context:], 0x1c029c974, with its block
// ___44-[NSMutableArray sortUsingFunction:context:]_block_invoke (0x1c029caa4). Non-external. A NULL
// function raises NSInvalidArgumentException through +exceptionWithName:reason:userInfo: (no os_log pack;
// the reason made with kCFAllocatorSystemDefault and autoreleased). Otherwise
// -sortWithOptions:usingComparator: with no options and a stack block (capturing the function, then the
// context) that calls the function with the two objects and the context.
- (void)sortUsingFunction:(NSInteger (*)(id, id, void *))compare context:(void *)context {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    if (compare == NULL) {
        OCFStringRef reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("*** %s: function pointer cannot be NULL"),
            methodName("sortUsingFunction:context:")
        );
        id exception = [(id)exceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                      reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                                                    userInfo:nil];
        objc_exception_throw(exception);
    }
    [self sortWithOptions:0
          usingComparator:^NSInteger(id object1, id object2) {
            return compare(object1, object2, context);
          }];
}

// Native -[NSMutableArray sortUsingFunction:context:range:], 0x1c029cab8, with its block
// ___50-[NSMutableArray sortUsingFunction:context:range:]_block_invoke (0x1c029cd80). Non-external. The NULL
// function raise of -sortUsingFunction:context:, the range check, then, unless the range's length is 1,
// -sortRange:options:usingComparator: of the range with no options and the function block.
- (void)sortUsingFunction:(NSInteger (*)(id, id, void *))compare
                  context:(void *)context
                    range:(OCFRange)range {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    if (compare == NULL) {
        OCFStringRef reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("*** %s: function pointer cannot be NULL"),
            methodName("sortUsingFunction:context:range:")
        );
        id exception = [(id)exceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                      reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                                                    userInfo:nil];
        objc_exception_throw(exception);
    }
    NSUInteger count = [self count];
    if (range.location < 0 || range.length < 0 ||
        count < (NSUInteger)range.location + (NSUInteger)range.length) {
        raiseRangeBeyondBounds(
            methodName("sortUsingFunction:context:range:"),
            (NSUInteger)range.location,
            (NSUInteger)range.length,
            count
        );
    }
    if (range.length != 1) {
        [self sortRange:range
                    options:0
            usingComparator:^NSInteger(id object1, id object2) {
              return compare(object1, object2, context);
            }];
    }
}

// Native -[NSMutableArray sortUsingSelector:], 0x1c029cd94, with its block
// ___36-[NSMutableArray sortUsingSelector:]_block_invoke (0x1c029ce78). Non-external. A NULL selector sends
// -doesNotRecognizeSelector: NULL to the receiver and, should that return, the sort proceeds. Then
// -sortWithOptions:usingComparator: with no options and a stack block (capturing the selector) that sends
// the selector to the first object with the second as argument (objc_msgSend as a tail call).
- (void)sortUsingSelector:(SEL)comparator {
    void *returnAddress = __builtin_return_address(0);
    [self _mutate];
    tsanWriteCheck(self, returnAddress, __OCFTSANTagMutableArray);
    if (comparator == NULL) {
        [self doesNotRecognizeSelector:NULL];
    }
    [self sortWithOptions:0
          usingComparator:^NSInteger(id object1, id object2) {
            return ((NSInteger (*)(id, SEL, id))objc_msgSend)(object1, comparator, object2);
          }];
}

// Native -[NSMutableArray initWithObjects:count:], 0x1c029ce88. Non-external. No mutation prologue: the
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
    id array = [self initWithCapacity:count];
    [array insertObjects:objects count:count atIndex:0];
    return array;
}

// Native -[NSMutableArray arrayByAddingObjectsFromArray:], 0x1c029d144. Non-external. The NSArray check
// comes before the ThreadSanitizer read hook. The argument's count is read before the receiver's; their sum
// passes the new_id_array check and sizes a buffer that is always allocated by _CFCreateArrayStorage (at
// least one pointer). The receiver's objects (when it has any) and then the argument's (when it has any) are
// copied in, every object is retained, and a new NSArray adopts the buffer
// (-_initByAdoptingBuffer:count:size: with the total as both count and size); a NULL buffer instead makes
// -initWithObjects:count: with NULL and the total. The array is answered autoreleased.
- (id)arrayByAddingObjectsFromArray:(id)array {
    void *returnAddress = __builtin_return_address(0);
    if (array != nil && !_ONSIsNSArray(array)) {
        raiseWithLogPack(
            ONSInvalidArgumentException,
            12,
            "*** %s: array argument is not an NSArray",
            methodName("arrayByAddingObjectsFromArray:")
        );
    }
    tsanReadCheck(self, returnAddress, __OCFTSANTagMutableArray);
    NSUInteger otherCount = [array count];
    NSUInteger count = [self count];
    NSUInteger total = count + otherCount;
    if (total >> 60 != 0) {
        raiseTemporaryBufferTooLarge(total);
    }
    size_t allocatedSize = 0;
    id *objects = _OCFCreateArrayStorage(total > 1 ? total : 1, false, &allocatedSize);
    if (count != 0) {
        [self getObjects:objects range:(OCFRange){0, (OCFIndex)count}];
    }
    if (otherCount != 0) {
        [array getObjects:&objects[count] range:(OCFRange){0, (OCFIndex)otherCount}];
    }
    id result;
    if (objects != NULL) {
        for (NSUInteger index = 0; index < total; ++index) {
            [objects[index] retain];
        }
        result = [(id)[ONSArray alloc] _initByAdoptingBuffer:objects count:total size:total];
    } else {
        result = [(id)[ONSArray alloc] initWithObjects:NULL count:total];
    }
    return [result autorelease];
}

// Native +[NSMutableArray arrayWithCapacity:], 0x1c029d3f8. Non-external. objc_alloc of the receiver,
// -initWithCapacity:, autoreleased.
+ (id)arrayWithCapacity:(NSUInteger)capacity {
    return [[[self alloc] initWithCapacity:capacity] autorelease];
}

// Native -[NSMutableArray insertObject:atIndex:], 0x1c030f9c0, with OUTLINED_FUNCTION_0_42 (0x1c029d420:
// __CFLookUpClass("NSMutableArray")), _2_26 (0x1c029d43c: the receiver and selector back into X1 and X2) and
// _1_30 (0x1c029d434: __CFRequireConcreteImplementation as a tail call). Non-external. The abstract method:
// raises through __CFRequireConcreteImplementation with the NSMutableArray class.
- (void)insertObject:(id)object atIndex:(NSUInteger)index {
    Class abstractClass = (Class)__CFLookUpClass(OF_NAME_STRING(ONSMutableArray));
    __CFRequireConcreteImplementation(abstractClass, self, _cmd);
}

// Native -[NSMutableArray removeObjectAtIndex:], 0x1c030f9dc, with OUTLINED_FUNCTION_0_42, _2_26 and _1_30.
// Non-external. The abstract method, as -insertObject:atIndex:.
- (void)removeObjectAtIndex:(NSUInteger)index {
    Class abstractClass = (Class)__CFLookUpClass(OF_NAME_STRING(ONSMutableArray));
    __CFRequireConcreteImplementation(abstractClass, self, _cmd);
}

// Native -[NSMutableArray replaceObjectAtIndex:withObject:], 0x1c030f9f8, with OUTLINED_FUNCTION_0_42, _2_26
// and _1_30. Non-external. The abstract method, as -insertObject:atIndex:.
- (void)replaceObjectAtIndex:(NSUInteger)index withObject:(id)object {
    Class abstractClass = (Class)__CFLookUpClass(OF_NAME_STRING(ONSMutableArray));
    __CFRequireConcreteImplementation(abstractClass, self, _cmd);
}

// Native -[NSMutableArray initWithCapacity:], 0x1c030fa14, with OUTLINED_FUNCTION_0_42 and _2_26.
// Non-external. The abstract method: __CFRequireConcreteImplementation (not a tail call), then nil.
- (id)initWithCapacity:(NSUInteger)capacity {
    Class abstractClass = (Class)__CFLookUpClass(OF_NAME_STRING(ONSMutableArray));
    __CFRequireConcreteImplementation(abstractClass, self, _cmd);
    return nil;
}

@end

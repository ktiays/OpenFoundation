//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Collections/ONSMutableIndexSet.h"
#include "CoreFoundation/OFPortHooks.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <alloca.h>
#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: the NSIndexSet unit spans 0x1c0527ddc-0x1c052f08f of the iOS Simulator Foundation
// __text section: the NSIndexSet methods (+_alloc ... -enumerateRangesUsingBlock:) with the C functions
// _isEmpty (0x1c0528740), _bitfieldSetRangesCount (0x1c052ab00), __NSIndexSetEnumerateBitfield
// (0x1c052b890), __NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__ (0x1c052bbf4),
// __NSIndexSetParameterCheckIterate (0x1c052bc50) and __NSIndexSetEnumerate (0x1c052bd5c) at their native
// places, then the NSMutableIndexSet methods (0x1c052ca5c-0x1c052e347), the block invoke of
// __NSIndexSetEnumerate (0x1c052ed40) and __NSIndexSetChunkIterate (0x1c052edb8). The methods of
// _NSIndexSetEnumerator and the remaining NSIndexSet and NSMutableIndexSet methods of the unit lie
// outside the captured groups. The placeholder and constant subclasses precede NSIndexSet. Two pieces of
// other native units follow the unit's code here because only this unit uses them: the category method
// -[NSIndexSet(NSIndexSetPortCoding) replacementObjectForPortCoder:] (0x1c064e664, in the port coder
// cluster) and _NSGetUnsignedInt (0x1c0fdcbd0, in the cold region; the varint reader of -initWithCoder:).
// The C functions are defined inside the @implementation that holds their native neighbours and reach the
// public instance variables, as the native functions do. Blocks are written as block literals at their
// native place; their invoke functions keep the native names in the comments, and the compiler emits their
// byref helpers (___Block_byref_object_copy_/dispose_, outside this family's list). Objective-C compiled with
// manual reference counting (objc_retain, objc_release; no ARC entry points). Evidence:
// Local/Evidence/cf-core/iphone-simulator-foundation-link.json (family FC-sets-attributed); selector names of
// the super sends (allocWithZone:, init, dealloc, description, replacementObjectForPortCoder:) resolved
// through the image's objc_msgSend stubs. Unless a definition says otherwise: iOS Simulator 24A434,
// Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full). iOS
// Device 24A446: unread. Reading: Source/Foundation/Collections/FoundationSetsAttributedReading.md.
//
// Shared-cache bindings: the methods load several class objects and constants through shared-cache GOT
// slots outside the extracted image; they are identified from their use (confidence: medium): 0x22227FAE0
// the NSException class (receiver of +exceptionWithName:reason:userInfo:), 0x22227F5E0 NSRangeException and
// 0x22227F500 NSInvalidArgumentException (exception names of range and argument errors), 0x22227FAB8 the
// NSData class (the class of the decoded "NSRangeData"), 0x22227FAD0 the NSDictionary class (receiver of
// +dictionaryWithObjects:forKeys:count:), 0x22227FB20 the NSMutableData class (receiver of -initWithCapacity:
// whose result receives -appendBytes:length:), 0x222409420 libobjc's objc_debug_taggedpointer_obfuscator and
// 0x222409408 libobjc's objc_debug_tag60_permutations (the inlined tagged-pointer encoding and decoding).
// The byte at 0xFFFFFC034 that the concurrent enumeration reads is the commpage's active processor count
// (_COMM_PAGE_ACTIVE_CPUS; name from xnu's cpu_capabilities.h, confidence: medium).

// libobjc's tagged-pointer obfuscation state (private exports of libobjc.A.dylib, supplied through
// libobjc; the SDK installs no declarations).
extern uintptr_t objc_debug_taggedpointer_obfuscator;
extern const uint8_t objc_debug_tag60_permutations[];

extern char indexSetClassObject[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSIndexSet));
extern char mutableIndexSetClassObject[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableIndexSet));
extern char constantIndexSetClassObject[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                    OF_NAME_STRING(_ONSConstantIndexSet));
extern char placeholderIndexSetClassObject[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                       OF_NAME_STRING(_ONSPlaceholderIndexSet));
extern char indexSetStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSString));
extern char indexSetMutableStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableString));
extern char indexSetErrorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSError));
extern char indexSetExceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));
extern char indexSetDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSDictionary));
extern char indexSetDataClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSData));
extern char indexSetMutableDataClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableData));

// The messages the methods send to objects of other classes (strings, coders, data, errors, exceptions),
// declared for the sends only; no protocol object is emitted.
@protocol IndexSetMessages

+ (id)stringWithFormat:(id)format, ...;
+ (id)stringWithCapacity:(NSUInteger)capacity;
+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
+ (id)errorWithDomain:(id)domain code:(NSInteger)code userInfo:(id)userInfo;
+ (id)dictionaryWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count;
- (id)initWithCapacity:(NSUInteger)capacity;
- (void)appendBytes:(const void *)bytes length:(NSUInteger)length;
- (const void *)bytes;
- (NSUInteger)length;
- (void)appendString:(id)string;
- (void)appendFormat:(id)format, ...;
- (BOOL)allowsKeyedCoding;
- (BOOL)requiresSecureCoding;
- (void)encodeInteger:(NSInteger)value forKey:(id)key;
- (void)encodeObject:(id)object forKey:(id)key;
- (void)encodeValueOfObjCType:(const char *)type at:(const void *)address;
- (void)decodeValueOfObjCType:(const char *)type at:(void *)address size:(NSUInteger)size;
- (NSInteger)decodeIntegerForKey:(id)key;
- (BOOL)containsValueForKey:(id)key;
- (id)decodeObjectOfClass:(Class)cls forKey:(id)key;
- (void)failWithError:(id)error;
- (BOOL)isByref;

@end

// The super sends of the port coding category; NSObject's -replacementObjectForPortCoder: is declared by
// Foundation's NSPortCoder.h, outside libobjc's NSObject.h. Declaration only; nothing is added to NSObject.

// The methods the unit sends to index sets (its own classes), and the objc_direct
// -_indexClosestToIndex:equalAllowed:following: (no selector argument and a nil check on the receiver
// natively: the BL from -indexGreaterThanIndex: and its siblings passes the index in X1).

// NSNotFound (NSIntegerMax), the "no index" answer of the unit's lookups.
static const NSUInteger kIndexNotFound = LONG_MAX;

// NSEnumerationConcurrent and NSEnumerationReverse, and the private option bits the enumeration functions
// test: the block takes a range (bit 62), the block is a passing test whose YES answer ends the enumeration
// and is returned (bit 61), and the range argument is checked (bit 59). The names are local descriptive
// names.
static const NSUInteger kEnumerationConcurrent = 1;
static const NSUInteger kEnumerationReverse = 2;
static const NSUInteger kEnumerateRanges = OF_WORD_BIT(NSUInteger, 62);
static const NSUInteger kEnumeratePassingTest = OF_WORD_BIT(NSUInteger, 61);
static const NSUInteger kEnumerateCheckRange = OF_WORD_BIT(NSUInteger, 59);

// The malloc type IDs of the range storage and of -__getContainmentVector:inRange:'s index buffer.
static const malloc_type_id_t kIndexSetRangeStorageTypeID = 0x1080040E45EEDC5;
static const malloc_type_id_t kIndexSetIndexBufferTypeID = 0x100004000313F17;

// Native _NSTaggedIndexSetAllowed, 0x22e6fcb5a, non-external (__bss), set by +[NSIndexSet initialize].
bool indexSetTaggedAllowed;

// libobjc's tagged-pointer class registration (private export; no SDK declaration).
extern void _objc_registerTaggedPointerClass(uint16_t tag, Class cls);

// Native __placeholderIndexSet (__DATA_CONST,__const 0x2227d48b8, non-external): the static instance
// +allocWithZone: answers for NSIndexSet itself, 16 bytes: the _NSPlaceholderIndexSet class pointer and a
// zero word (the layout of the second word is unread beyond its value; confidence: medium for its type).
static const struct {
    const void *isa;
    uintptr_t reserved;
} __placeholderIndexSet = {placeholderIndexSetClassObject, 0};

// The layout every block literal starts with; the callout reads its invoke pointer at 16.
typedef struct {
    void *isa;
    int flags;
    int reserved;
    void *invoke;
} IndexSetBlockLiteral;

// The inlined pieces the native functions repeat. They are not native functions of their own; the names
// are local descriptive names and each body is the instruction sequence the callers inline.

// Whether the object is a tagged pointer (bit 63).
static inline bool indexSetIsTagged(const void *set) {
    return (intptr_t)set < 0;
}

// The membership mask of a tagged index set: libobjc's decoding (no obfuscation when bits 62-63 and 0-2
// are all set), then the payload above the three tag bits, 52 bits for an extended tag and 60 otherwise.
static inline NSUInteger indexSetTaggedBitfield(const void *set) {
    uintptr_t value = (uintptr_t)set;
    uintptr_t obfuscator = objc_debug_taggedpointer_obfuscator;
    if ((~value & 0xC000000000000007) == 0) {
        obfuscator = 0;
    }
    uintptr_t decoded = obfuscator ^ value;
    uintptr_t mask = 0xFFFFFFFFFFFFFFF;
    if ((~decoded & 7) == 0) {
        mask = 0xFFFFFFFFFFFFF;
    }
    return mask & (decoded >> 3);
}

// libobjc's encoding of a tagged pointer value: obfuscated and its low three bits permuted, unless the
// obfuscated value has bits 62-63 and 0-2 all set, when the value is answered as it is.
static inline id indexSetEncodeTagged(uintptr_t value) {
    uintptr_t obfuscated = objc_debug_taggedpointer_obfuscator ^ value;
    if ((~obfuscated & 0xC000000000000007) == 0) {
        return (id)value;
    }
    return (id)((obfuscated & 0xFFFFFFFFFFFFFFF8) | objc_debug_tag60_permutations[obfuscated & 7]);
}

// The out-of-line storage of a set holding several ranges.
static inline IndexSetRangeStorage *indexSetStorage(ONSIndexSet *set) {
    return (IndexSetRangeStorage *)set->_internal._multipleRanges._data;
}

// The first range in use: the inline range, or the storage's range at firstRange.
static inline ONSRange *indexSetRanges(ONSIndexSet *set) {
    if (set->_indexSetFlags._hasSingleRange) {
        return &set->_internal._singleRange._range;
    }
    IndexSetRangeStorage *storage = indexSetStorage(set);
    return &storage->ranges[storage->firstRange];
}

// The range count the loops over ranges compute: the word tested is the bitfield or the inline length (the
// storage's rangeCount when neither flag is set); when it is nonzero, 1 for an inline set and the storage's
// rangeCount otherwise.
static inline NSUInteger indexSetLoopRangeCount(ONSIndexSet *set) {
    const NSUInteger *word = set->_indexSetFlags._hasBitfield ? &set->_internal._singleBitfield._bitfield
                                                              : &set->_internal._singleRange._range.length;
    if (!set->_indexSetFlags._hasSingleRange && !set->_indexSetFlags._hasBitfield) {
        word = &indexSetStorage(set)->rangeCount;
    }
    if (*word == 0) {
        return 0;
    }
    return set->_indexSetFlags._hasSingleRange ? 1 : indexSetStorage(set)->rangeCount;
}

// The range count the bounds checks of the mutable methods compute: the tagged payload, the bitfield, the
// inline length or the storage's rangeCount; when that is nonzero, 1 for an inline set and the storage's
// rangeCount otherwise (the flags are read from the receiver even when it is tagged, as natively).
static inline NSUInteger indexSetRangeBound(ONSIndexSet *set) {
    NSUInteger value;
    if (indexSetIsTagged(set)) {
        value = indexSetTaggedBitfield(set);
    } else if (set->_indexSetFlags._hasBitfield) {
        value = set->_internal._singleBitfield._bitfield;
    } else if (set->_indexSetFlags._hasSingleRange) {
        value = set->_internal._singleRange._range.length;
    } else {
        value = indexSetStorage(set)->rangeCount;
    }
    if (value != 0) {
        value = set->_indexSetFlags._hasSingleRange ? 1 : indexSetStorage(set)->rangeCount;
    }
    return value;
}

// The mask of a range of bits: length ones shifted to location, with the shift amounts taken modulo 64 as
// the LSR/LSL register forms do; 0 for an empty range.
static inline NSUInteger indexSetRangeMask(NSUInteger location, NSUInteger length) {
    if (length == 0) {
        return 0;
    }
    return (~(NSUInteger)0 >> ((0 - length) & 63)) << (location & 63);
}

// The membership mask of a set's ranges (the loop the callers inline when every index is below 64).
static inline NSUInteger indexSetBitfieldFromRanges(ONSIndexSet *set) {
    NSUInteger bitfield = 0;
    for (NSUInteger rangeIndex = 0; rangeIndex < indexSetLoopRangeCount(set); rangeIndex++) {
        ONSRange range = indexSetRanges(set)[rangeIndex];
        bitfield |= indexSetRangeMask(range.location, range.length);
    }
    return bitfield;
}

// Count trailing zeros as RBIT and CLZ compute it: 64 for zero.
static inline NSUInteger indexSetTrailingZeros(NSUInteger value) {
    return value != 0 ? (NSUInteger)__builtin_ctzll(value) : 64;
}

static __attribute__((noinline)) BOOL
__NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__(id block, uintptr_t first, uintptr_t second, uintptr_t third);

static BOOL _isEmpty(ONSIndexSet *set);
static NSUInteger _bitfieldSetRangesCount(NSUInteger bitfield);
static BOOL _NSGetUnsignedInt(NSUInteger *value, const uint8_t **cursor, const uint8_t *end);
static BOOL __NSIndexSetChunkIterate(
    ONSIndexSet *set,
    NSUInteger options,
    NSUInteger firstIndex,
    NSUInteger lastIndex,
    NSUInteger start,
    NSUInteger length,
    id block,
    NSUInteger *result
);

// Native __NSEmptyIndexSet_struct, 0x2227d4968, and __NSConstantIndexSets, 0x2227d4988. Non-external.
// These instances contain the constant class pointer, the single-range flag and an inline range.
static const struct {
    const void *isa;
    NSUInteger flags;
    ONSRange range;
} _NSEmptyIndexSet_struct = {constantIndexSetClassObject, 1, {0, 0}},
  _NSConstantIndexSets[] = {
      {constantIndexSetClassObject, 1, {0, 1}},
      {constantIndexSetClassObject, 1, {1, 1}},
      {constantIndexSetClassObject, 1, {2, 1}},
      {constantIndexSetClassObject, 1, {3, 1}},
      {constantIndexSetClassObject, 1, {4, 1}},
      {constantIndexSetClassObject, 1, {5, 1}},
      {constantIndexSetClassObject, 1, {6, 1}},
      {constantIndexSetClassObject, 1, {7, 1}},
      {constantIndexSetClassObject, 1, {8, 1}},
      {constantIndexSetClassObject, 1, {9, 1}},
  };

@implementation _ONSPlaceholderIndexSet

// Native -[_NSPlaceholderIndexSet retainCount], 0x1c05279a4. Non-external.
- (NSUInteger)retainCount {
    return NSUIntegerMax;
}

// Native -[_NSPlaceholderIndexSet init], 0x1c05279ac. Non-external.
- (id)init {
    if (indexSetTaggedAllowed != 1) {
        return [self initWithIndexesInRange:(ONSRange){0, 0}];
    }
    return indexSetEncodeTagged(0x8580000000000007);
}

// Native -[_NSPlaceholderIndexSet initWithIndex:], 0x1c0527a08. Non-external.
// Hex-Rays omits the range length of 1 supplied in X3 on the large-index path.
- (id)initWithIndex:(NSUInteger)index {
    if (index > 63) {
        return [self initWithIndexesInRange:(ONSRange){index, 1}];
    }
    return [self initWithBitfield:(NSUInteger)1 << index];
}

// Native -[_NSPlaceholderIndexSet initWithIndexesInRange:], 0x1c0527a24. Non-external.
// The exception class and name come from the same medium-confidence shared-cache bindings as NSIndexSet.
- (id)initWithIndexesInRange:(ONSRange)range {
    NSUInteger end = range.location + range.length;
    if (range.length != 0 && (NSInteger)end < 0) {
        id proem = _ONSMethodExceptionProem(self, _cmd);
        id rangeString = ONSStringFromRange(range);
        id reason = [(id<IndexSetMessages>)(id)indexSetStringClass
            stringWithFormat:(id)OF_STR("%@: Range %@ exceeds maximum index value of NSNotFound - 1"),
                             proem,
                             rangeString];
        objc_exception_throw([(id<IndexSetMessages>)(id)indexSetExceptionClass
            exceptionWithName:(id)ONSRangeException
                       reason:reason
                     userInfo:nil]);
    }
    if (range.length != 0 && (range.location > 63 || end > 64 || end < range.location)) {
        objc_opt_self((id)constantIndexSetClassObject);
        if (range.length == 1 && range.location < 10) {
            return (id)&_NSConstantIndexSets[range.location];
        }
        return [[(id)indexSetClassObject _alloc] initWithIndexesInRange:range];
    }
    NSUInteger bits = indexSetRangeMask(range.location, range.length);
    return [self initWithBitfield:bits];
}

// Native -[_NSPlaceholderIndexSet initWithBitfield:], 0x1c0527b4c. Non-external.
- (id)initWithBitfield:(NSUInteger)bits {
    if (indexSetTaggedAllowed != 1 || (bits != 0 && ((NSUInteger)__builtin_clzll(bits) ^ 0x3C) > 0x33)) {
        return [[(id)indexSetClassObject _alloc] initWithBitfield:bits];
    }
    return indexSetEncodeTagged((8 * (bits & 0xFFFFFFFFFFFFF)) | 0x8580000000000007);
}

// Native -[_NSPlaceholderIndexSet initWithIndexSet:], 0x1c0527be8. Non-external.
- (id)initWithIndexSet:(id)source {
    NSUInteger count = [(ONSIndexSet *)source count];
    if (indexSetTaggedAllowed == 1 && count == 0) {
        return indexSetEncodeTagged(0x8580000000000007);
    }
    if (count == 1) {
        return [self initWithIndex:[(ONSIndexSet *)source firstIndex]];
    }
    Class cls = objc_opt_class(source);
    if (cls == (Class)indexSetClassObject || cls == (Class)constantIndexSetClassObject ||
        cls == (Class)mutableIndexSetClassObject) {
        NSUInteger bits = 0;
        if ([source getBitfield:&bits]) {
            return [self initWithBitfield:bits];
        }
    }
    cls = objc_opt_class(source);
    if (cls != (Class)constantIndexSetClassObject && count == 0) {
        objc_opt_self((id)constantIndexSetClassObject);
        return (id)&_NSEmptyIndexSet_struct;
    }
    if (source != nil && cls == (Class)constantIndexSetClassObject) {
        return source;
    }
    return [[(id)indexSetClassObject _alloc] initWithIndexSet:source];
}

// Native -[_NSPlaceholderIndexSet initWithCoder:], 0x1c0527d9c. Non-external.
- (id)initWithCoder:(id)coder {
    return [[(id)indexSetClassObject _alloc] initWithCoder:coder];
}

@end

@implementation _ONSConstantIndexSet

// Native -[_NSConstantIndexSet retainCount], 0x1c0527dc8. Non-external.
- (NSUInteger)retainCount {
    return NSUIntegerMax;
}

// Native -[_NSConstantIndexSet retain], 0x1c0527dd0. Non-external. A constant set is never retained; the
// receiver is answered.
- (id)retain {
    return self;
}

// Native -[_NSConstantIndexSet release], 0x1c0527dd4. Non-external.
- (oneway void)release {
}

// Native -[_NSConstantIndexSet autorelease], 0x1c0527dd8. Non-external. The receiver.
- (id)autorelease {
    return self;
}

@end

@implementation ONSIndexSet

// Native +[NSIndexSet _alloc], 0x1c0527ddc. Non-external. NSObject's +allocWithZone: with no zone.
+ (id)_alloc {
    return [super allocWithZone:NULL];
}

// Native +[NSIndexSet allocWithZone:], 0x1c0527e40. Non-external. NSIndexSet itself answers the static
// placeholder; subclasses are allocated with no extra bytes.
+ (id)allocWithZone:(struct _NSZone *)zone {
    Class cls = objc_opt_class((id)(self));
    if (cls == (Class)indexSetClassObject) {
        return (id)&__placeholderIndexSet;
    }
    return ONSAllocateObject(cls, 0, zone);
}

// Native +[NSIndexSet initialize], 0x1c0527e8c. Non-external. NSIndexSet itself registers as the class of
// tagged pointer tag 19 and allows tagged index sets.
+ (void)initialize {
    if (self == (Class)indexSetClassObject) {
        _objc_registerTaggedPointerClass(19, self);
        indexSetTaggedAllowed = true;
    }
}

// Native +[NSIndexSet indexSet], 0x1c0527ec8. Non-external. When tagged index sets are allowed and the
// receiver is NSIndexSet itself, the empty tagged set (value 0x8580000000000007 encoded); otherwise a new
// autoreleased instance.
+ (id)indexSet {
    if (indexSetTaggedAllowed != 1 || self != (Class)indexSetClassObject) {
        return objc_autorelease((id)(objc_alloc_init((Class)(self))));
    }
    return indexSetEncodeTagged(0x8580000000000007);
}

// Native -[NSIndexSet _init], 0x1c0528264. Non-external. A new set is an empty bitfield (both flag bits).
- (id)_init {
    self = [super init];
    if (self != nil) {
        self->_indexSetFlags._hasSingleRange = 1;
        self->_indexSetFlags._hasBitfield = 1;
    }
    return self;
}

// Native -[NSIndexSet init], 0x1c05282d4. Non-external.
- (id)init {
    return [self _init];
}

// Native -[NSIndexSet initWithIndex:], 0x1c05282d8. Non-external. Indexes below 64 are a one-bit bitfield.
- (id)initWithIndex:(NSUInteger)value {
    if (value > 63) {
        return [self initWithIndexesInRange:(ONSRange){value, 1}];
    }
    self = [self _init];
    if (self != nil) {
        self->_internal._singleBitfield._bitfield = (NSUInteger)1 << value;
    }
    return self;
}

// Native -[NSIndexSet initWithIndexesInRange:], 0x1c0528324. Non-external. A range that reaches past
// NSNotFound - 1 releases the new set and raises NSRangeException; a range inside 0-63 becomes a bitfield,
// any other nonempty range the inline range.
- (id)initWithIndexesInRange:(ONSRange)range {
    self = [self _init];
    if (self == nil) {
        return nil;
    }
    NSUInteger end = range.location + range.length;
    if (range.length != 0 && (NSInteger)end < 0) {
        id proem = _ONSMethodExceptionProem(self, _cmd);
        id reason = [(id<IndexSetMessages>)(id)indexSetStringClass
            stringWithFormat:(id)OF_STR("%@: Range %@ exceeds maximum index value of NSNotFound - 1"),
                             proem,
                             ONSStringFromRange(range)];
        objc_release((id)(self));
        objc_exception_throw([(id<IndexSetMessages>)(id)indexSetExceptionClass
            exceptionWithName:(id)ONSRangeException
                       reason:reason
                     userInfo:nil]);
    }
    if (range.length != 0 && (range.location > 63 || end > 64 || end < range.location)) {
        self->_indexSetFlags._hasBitfield = 0;
        self->_internal._singleRange._range.location = range.location;
        self->_internal._singleRange._range.length = range.length;
    } else {
        self->_internal._singleBitfield._bitfield = indexSetRangeMask(range.location, range.length);
    }
    return self;
}

// Native -[NSIndexSet _setContentToContentFromIndexSet:], 0x1c052842c. Non-external. A source of another
// class is first copied into a temporary NSIndexSet. Tagged and bitfield sources and sources whose indexes
// all lie below 64 become a bitfield; a single range is copied inline; several ranges get storage of exactly
// their count (firstRange 0) with indexCount summed from the lengths.
- (void)_setContentToContentFromIndexSet:(id)indexSet {
    if (indexSet == nil) {
        return;
    }
    Class cls = objc_opt_class((id)(indexSet));
    if (cls != (Class)indexSetClassObject && cls != (Class)constantIndexSetClassObject &&
        cls != (Class)mutableIndexSetClassObject) {
        id copy = [objc_alloc((Class)((Class)indexSetClassObject)) initWithIndexSet:indexSet];
        [self _setContentToContentFromIndexSet:copy];
        objc_release((id)(copy));
        return;
    }
    ONSIndexSet *source = indexSet;
    self->_indexSetFlags._hasSingleRange = 1;
    NSUInteger bitfield;
    if (indexSetIsTagged(source)) {
        bitfield = indexSetTaggedBitfield(source);
    } else if (source->_indexSetFlags._hasBitfield) {
        bitfield = source->_internal._singleBitfield._bitfield;
    } else {
        NSUInteger lastIndex = [source lastIndex];
        if (lastIndex == kIndexNotFound || lastIndex <= 63) {
            bitfield = indexSetBitfieldFromRanges(source);
        } else {
            if (source->_indexSetFlags._hasSingleRange) {
                self->_indexSetFlags._hasBitfield = 0;
                self->_internal._singleRange._range = *indexSetRanges(source);
            } else {
                self->_indexSetFlags._hasSingleRange = 0;
                self->_indexSetFlags._hasBitfield = 0;
                NSUInteger count = indexSetLoopRangeCount(source);
                IndexSetRangeStorage *storage =
                    malloc_type_malloc(16 * count + 64, kIndexSetRangeStorageTypeID);
                self->_internal._multipleRanges._data = storage;
                __atomic_store_n(&storage->cacheOwner, 0, __ATOMIC_RELEASE);
                storage->capacity = count;
                storage->rangeCount = count;
                storage->indexCount = 0;
                storage->firstRange = 0;
                ONSRange *ranges = indexSetRanges(self);
                memcpy(ranges, indexSetRanges(source), 16 * count);
                for (NSUInteger rangeIndex = 0; rangeIndex < count; rangeIndex++) {
                    indexSetStorage(self)->indexCount += ranges[rangeIndex].length;
                }
            }
            return;
        }
    }
    self->_indexSetFlags._hasBitfield = 1;
    self->_internal._singleBitfield._bitfield = bitfield;
}

// Native _isEmpty, 0x1c0528740. Non-external. Whether the tagged payload, the bitfield, the inline length
// or the storage's rangeCount is zero.
static BOOL _isEmpty(ONSIndexSet *set) {
    NSUInteger value;
    if (indexSetIsTagged(set)) {
        value = indexSetTaggedBitfield(set);
    } else if (set->_indexSetFlags._hasBitfield) {
        value = set->_internal._singleBitfield._bitfield;
    } else if (set->_indexSetFlags._hasSingleRange) {
        value = set->_internal._singleRange._range.length;
    } else {
        value = indexSetStorage(set)->rangeCount;
    }
    return value == 0;
}

// Native -[NSIndexSet initWithIndexSet:], 0x1c05287ac. Non-external.
- (id)initWithIndexSet:(id)indexSet {
    self = [self _init];
    if (self != nil) {
        [self _setContentToContentFromIndexSet:indexSet];
    }
    return self;
}

// Native -[NSIndexSet initWithBitfield:], 0x1c0528a44. Non-external. When tagged index sets are allowed, the
// receiver is an NSIndexSet and the mask fits the 52-bit payload (CLZ ^ 0x3C at most 0x33), the receiver is
// released and the tagged set answered; otherwise the mask is stored in a new set.
- (id)initWithBitfield:(NSUInteger)bitfield {
    if (indexSetTaggedAllowed == 1 && objc_opt_class((id)(self)) == (Class)indexSetClassObject &&
        (bitfield == 0 || ((NSUInteger)__builtin_clzll(bitfield) ^ 0x3C) <= 0x33)) {
        objc_release((id)(self));
        return indexSetEncodeTagged((8 * (bitfield & 0xFFFFFFFFFFFFF)) | 0x8580000000000007);
    }
    self = [self _init];
    if (self != nil) {
        self->_internal._singleBitfield._bitfield = bitfield;
    }
    return self;
}

// Native -[NSIndexSet dealloc], 0x1c0528af8. Non-external. Frees the range storage of a set without the
// single-range flag.
- (void)dealloc {
    if (!self->_indexSetFlags._hasSingleRange) {
        free(self->_internal._multipleRanges._data);
    }
    [super dealloc];
}

// Native +[NSIndexSet supportsSecureCoding], 0x1c0528b74. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSIndexSet encodeWithCoder:], 0x1c0528b7c. Non-external. Keyed: "NSRangeCount", then for one
// range "NSLocation" and "NSLength", for several an NSMutableData ("NSRangeData", capacity 2 * count + 16)
// of each range's location and length as little-endian base-128 varints (seven bits per byte, the high bit
// marking a continuation), built range by range in a 40-byte stack buffer. Unkeyed: the count and each
// range's location and length as "I" (32-bit) values.
- (void)encodeWithCoder:(id)coder {
    NSUInteger count = [self rangeCount];
    if ([(id<IndexSetMessages>)coder allowsKeyedCoding]) {
        [(id<IndexSetMessages>)coder encodeInteger:(NSInteger)count forKey:(id)OF_STR("NSRangeCount")];
        if (count == 0) {
            return;
        }
        if (count == 1) {
            ONSRange range = [self rangeAtIndex:0];
            [(id<IndexSetMessages>)coder encodeInteger:(NSInteger)range.location
                                                forKey:(id)OF_STR("NSLocation")];
            [(id<IndexSetMessages>)coder encodeInteger:(NSInteger)range.length forKey:(id)OF_STR("NSLength")];
            return;
        }
        id data = [(id<IndexSetMessages>)objc_alloc((Class)((Class)indexSetMutableDataClass))
            initWithCapacity:2 * count + 16];
        for (NSUInteger rangeIndex = 0; rangeIndex != count; rangeIndex++) {
            ONSRange range = [self rangeAtIndex:rangeIndex];
            uint8_t buffer[40];
            uint8_t *cursor = buffer;
            NSUInteger value = range.location;
            while (value >= 0x80) {
                *cursor++ = (uint8_t)(value | 0x80);
                value >>= 7;
            }
            *cursor++ = (uint8_t)value;
            value = range.length;
            while (value >= 0x80) {
                *cursor++ = (uint8_t)(value | 0x80);
                value >>= 7;
            }
            *cursor = (uint8_t)value;
            [(id<IndexSetMessages>)data appendBytes:buffer length:(NSUInteger)(cursor - buffer + 1)];
        }
        [(id<IndexSetMessages>)coder encodeObject:data forKey:(id)OF_STR("NSRangeData")];
        objc_release((id)(data));
    } else {
        unsigned int encodedCount = (unsigned int)count;
        [(id<IndexSetMessages>)coder encodeValueOfObjCType:"I" at:&encodedCount];
        for (NSUInteger rangeIndex = 0; rangeIndex != count; rangeIndex++) {
            ONSRange range = [self rangeAtIndex:rangeIndex];
            unsigned int length = (unsigned int)range.length;
            unsigned int location = (unsigned int)range.location;
            [(id<IndexSetMessages>)coder encodeValueOfObjCType:"I" at:&location];
            [(id<IndexSetMessages>)coder encodeValueOfObjCType:"I" at:&length];
        }
    }
}

// Native -[NSIndexSet initWithCoder:], 0x1c0528da0. Non-external. The ranges are collected in a temporary
// NSMutableIndexSet and the receiver initialized from it. A keyed archive with several ranges but no
// "NSRangeData", missing range bytes, a truncated varint, or (when the coder requires secure coding) bytes
// left over fail the coder with NSCocoaErrorDomain code 4864 and a localized description, release the
// receiver and answer nil.
//
// Foundation defect: for the missing "NSRangeData" case the native code releases the temporary set and the
// receiver, then continues at the common exit, which sends -initWithIndexSet: to nil and releases the
// temporary set a second time (objc_release_x20 at 0x1c0529034 after the release at 0x1c0528fb0), a release
// of a deallocated object. The restoration answers nil after failing the coder, as the other failure paths
// do; the nil message and the second release are dropped.
- (id)initWithCoder:(id)coder {
    id description;
    id indexes = objc_alloc_init((Class)((Class)mutableIndexSetClassObject));
    if (![(id<IndexSetMessages>)coder allowsKeyedCoding]) {
        unsigned int count = 0;
        [(id<IndexSetMessages>)coder decodeValueOfObjCType:"I" at:&count size:4];
        for (NSUInteger remaining = count; remaining != 0; remaining--) {
            unsigned int location = 0;
            unsigned int length = 0;
            [(id<IndexSetMessages>)coder decodeValueOfObjCType:"I" at:&location size:4];
            [(id<IndexSetMessages>)coder decodeValueOfObjCType:"I" at:&length size:4];
            [indexes addIndexesInRange:(ONSRange){location, length}];
        }
    } else {
        NSUInteger count =
            (NSUInteger)[(id<IndexSetMessages>)coder decodeIntegerForKey:(id)OF_STR("NSRangeCount")];
        if (count == 1) {
            NSUInteger location =
                (NSUInteger)[(id<IndexSetMessages>)coder decodeIntegerForKey:(id)OF_STR("NSLocation")];
            NSUInteger length =
                (NSUInteger)[(id<IndexSetMessages>)coder decodeIntegerForKey:(id)OF_STR("NSLength")];
            [indexes addIndexesInRange:(ONSRange){location, length}];
        } else if (count != 0) {
            if (![(id<IndexSetMessages>)coder containsValueForKey:(id)OF_STR("NSRangeData")]) {
                objc_release((id)(indexes));
                objc_release((id)(self));
                description = (id)OF_STR("-[NSIndexSet initWithCoder:] decoder did not provide range data");
                goto fail;
            }
            id data = [(id<IndexSetMessages>)coder
                decodeObjectOfClass:objc_opt_class((id)((Class)indexSetDataClass))
                             forKey:(id)OF_STR("NSRangeData")];
            const uint8_t *cursor = [(id<IndexSetMessages>)data bytes];
            if (cursor == NULL) {
                objc_release((id)(indexes));
                objc_release((id)(self));
                description = (id)OF_STR("Range data missing.");
                goto fail;
            }
            const uint8_t *end = cursor + [(id<IndexSetMessages>)data length];
            do {
                NSUInteger location = 0;
                NSUInteger length = 0;
                if (!_NSGetUnsignedInt(&location, &cursor, end) ||
                    !_NSGetUnsignedInt(&length, &cursor, end)) {
                    objc_release((id)(indexes));
                    objc_release((id)(self));
                    description = (id)OF_STR("Range data did not match expected length.");
                    goto fail;
                }
                [indexes addIndexesInRange:(ONSRange){location, length}];
                count--;
            } while (count != 0);
            if (cursor != end && [(id<IndexSetMessages>)coder requiresSecureCoding]) {
                objc_release((id)(indexes));
                objc_release((id)(self));
                description = (id)OF_STR("Range data did not match expected length.");
                goto fail;
            }
        }
    }
    {
        id result = [self initWithIndexSet:indexes];
        objc_release((id)(indexes));
        return result;
    }
fail:
    {
        id key = ONSLocalizedDescriptionKey;
        id userInfo = [(id<IndexSetMessages>)(id)indexSetDictionaryClass dictionaryWithObjects:&description
                                                                                       forKeys:&key
                                                                                         count:1];
        [(id<IndexSetMessages>)coder
            failWithError:[(id<IndexSetMessages>)(id)indexSetErrorClass errorWithDomain:ONSCocoaErrorDomain
                                                                                   code:4864
                                                                               userInfo:userInfo]];
        return nil;
    }
}

// Native -[NSIndexSet classForCoder], 0x1c052912c. Non-external.
- (Class)classForCoder {
    return (Class)indexSetClassObject;
}

// Native +[NSIndexSet _subclassesMustBeExplicitlyMentionedWhenDecoded], 0x1c0529138. Non-external.
+ (BOOL)_subclassesMustBeExplicitlyMentionedWhenDecoded {
    return YES;
}

// Native -[NSIndexSet _indexOfRangeBeforeOrContainingIndex:], 0x1c052934c. Non-external. The position of the
// last range starting at or before the index (NSNotFound when none). Tagged and bitfield sets enumerate
// their ranges with a block (native ___51-[NSIndexSet _indexOfRangeBeforeOrContainingIndex:]_block_invoke,
// 0x1c0529544: a range containing the index answers its position and stops; a range ending before the index
// answers its position and continues; a range starting after it stops). Range sets answer the last range
// when the index is at or after lastIndex, NSNotFound before firstIndex, and otherwise binary-search the
// ranges.
- (NSUInteger)_indexOfRangeBeforeOrContainingIndex:(NSUInteger)index {
    NSUInteger high;
    if (indexSetIsTagged(self) || self->_indexSetFlags._hasBitfield) {
        __block NSUInteger result = kIndexNotFound;
        __block NSUInteger position = 0;
        [self enumerateRangesUsingBlock:^(ONSRange range, BOOL *stop) {
          if (index >= range.location && index - range.location < range.length) {
              result = position;
              *stop = YES;
          } else if (range.location + range.length < index) {
              result = position;
          } else if (index < range.location) {
              *stop = YES;
          }
          position++;
        }];
        return result;
    }
    if (self->_indexSetFlags._hasSingleRange) {
        if (self->_internal._singleRange._range.length == 0) {
            return kIndexNotFound;
        }
        high = 0;
    } else {
        NSUInteger count = indexSetStorage(self)->rangeCount;
        if (count == 0) {
            return kIndexNotFound;
        }
        high = count - 1;
    }
    if (index >= [self lastIndex]) {
        return high;
    }
    if (index < [self firstIndex]) {
        return kIndexNotFound;
    }
    ONSRange *ranges = indexSetRanges(self);
    NSUInteger low = 0;
    while (low < high) {
        NSUInteger middle = (low + high) >> 1;
        ONSRange *range = &ranges[middle];
        if (range->location > index) {
            high = middle;
        } else {
            if (range->location + range->length - 1 >= index) {
                return middle;
            }
            low = middle + 1;
        }
    }
    NSUInteger before = low == 0 ? kIndexNotFound : low - 1;
    return ranges[low].location <= index ? low : before;
}

// Native -[NSIndexSet _indexOfRangeAfterOrContainingIndex:], 0x1c05295b8. Non-external. The position of the
// first range ending after or containing the index (NSNotFound when none). Tagged and bitfield sets
// enumerate their ranges with a block (native ___50-[NSIndexSet _indexOfRangeAfterOrContainingIndex:]
// _block_invoke, 0x1c05297b8: the first range that does not end at or before the index answers its position
// and stops). Range sets answer 0 at or before firstIndex, NSNotFound after lastIndex, and otherwise
// binary-search the ranges.
- (NSUInteger)_indexOfRangeAfterOrContainingIndex:(NSUInteger)index {
    NSUInteger last;
    if (indexSetIsTagged(self) || self->_indexSetFlags._hasBitfield) {
        __block NSUInteger result = kIndexNotFound;
        __block NSUInteger position = 0;
        [self enumerateRangesUsingBlock:^(ONSRange range, BOOL *stop) {
          if (!(index >= range.location && index - range.location >= range.length)) {
              result = position;
              *stop = YES;
          }
          position++;
        }];
        return result;
    }
    if (self->_indexSetFlags._hasSingleRange) {
        if (self->_internal._singleRange._range.length == 0) {
            return kIndexNotFound;
        }
        last = 0;
    } else {
        NSUInteger count = indexSetStorage(self)->rangeCount;
        if (count == 0) {
            return kIndexNotFound;
        }
        last = count - 1;
    }
    if (index <= [self firstIndex]) {
        return 0;
    }
    if (index > [self lastIndex]) {
        return kIndexNotFound;
    }
    ONSRange *ranges = indexSetRanges(self);
    NSUInteger low = 0;
    if (last != 0) {
        NSUInteger high = last;
        do {
            NSUInteger middle = (low + high) >> 1;
            ONSRange *range = &ranges[middle];
            if (range->location <= index) {
                if (range->location + range->length - 1 >= index) {
                    return middle;
                }
                low = middle + 1;
            } else {
                high = middle;
            }
        } while (low < high);
    }
    NSUInteger rangeLast = ranges[low].location + ranges[low].length - 1;
    NSUInteger after = low < last ? low + 1 : kIndexNotFound;
    return rangeLast < index ? after : low;
}

// Native -[NSIndexSet hash], 0x1c05297fc. Non-external. count + firstIndex + lastIndex, 0 when empty.
- (NSUInteger)hash {
    NSUInteger count = [self count];
    if (count == 0) {
        return 0;
    }
    NSUInteger firstIndex = [self firstIndex];
    return firstIndex + count + [self lastIndex];
}

// Native -[NSIndexSet isEqualToIndexSet:], 0x1c0529848. Non-external. Identity answers YES and nil NO. When
// either set is not of the unit's own classes (NSIndexSet, _NSConstantIndexSet, NSMutableIndexSet), the
// counts and then the indexes are compared through the public methods (both lookups pass the other set's
// index, as natively). Otherwise bitfields are compared, a range set converted to a bitfield when every
// index is below 64 (a larger range set is unequal to a bitfield); two range sets compare range counts,
// index counts and then the ranges.
- (BOOL)isEqualToIndexSet:(id)indexSet {
    if (indexSet == self) {
        return YES;
    }
    if (indexSet == nil) {
        return NO;
    }
    Class selfClass = objc_opt_class((id)(self));
    Class otherClass;
    if ((selfClass != (Class)indexSetClassObject && selfClass != (Class)constantIndexSetClassObject &&
         selfClass != (Class)mutableIndexSetClassObject) ||
        ((otherClass = objc_opt_class((id)(indexSet))) != (Class)indexSetClassObject &&
         otherClass != (Class)constantIndexSetClassObject &&
         otherClass != (Class)mutableIndexSetClassObject)) {
        NSUInteger count = [self count];
        if (count != [(ONSIndexSet *)indexSet count]) {
            return NO;
        }
        if (count == 0) {
            return YES;
        }
        NSUInteger selfIndex = [self firstIndex];
        NSUInteger otherIndex = [(ONSIndexSet *)indexSet firstIndex];
        BOOL equal;
        do {
            equal = selfIndex == otherIndex;
            if (!equal) {
                break;
            }
            selfIndex = [self indexGreaterThanIndex:otherIndex];
            otherIndex = [(ONSIndexSet *)indexSet indexGreaterThanIndex:otherIndex];
            count--;
        } while (count != 0);
        return equal;
    }
    ONSIndexSet *other = indexSet;
    if (indexSetIsTagged(self) || self->_indexSetFlags._hasBitfield) {
        NSUInteger selfBits =
            indexSetIsTagged(self) ? indexSetTaggedBitfield(self) : self->_internal._singleBitfield._bitfield;
        NSUInteger otherBits;
        BOOL comparable = YES;
        if (indexSetIsTagged(other)) {
            otherBits = indexSetTaggedBitfield(other);
        } else if (other->_indexSetFlags._hasBitfield) {
            otherBits = other->_internal._singleBitfield._bitfield;
        } else {
            NSUInteger lastIndex = [other lastIndex];
            if (lastIndex != kIndexNotFound && lastIndex > 63) {
                otherBits = 0;
                comparable = NO;
            } else {
                otherBits = indexSetBitfieldFromRanges(other);
            }
        }
        return selfBits == otherBits ? comparable : NO;
    }
    if (indexSetIsTagged(other) || other->_indexSetFlags._hasBitfield) {
        NSUInteger otherBits = indexSetIsTagged(other) ? indexSetTaggedBitfield(other)
                                                       : other->_internal._singleBitfield._bitfield;
        NSUInteger lastIndex = [self lastIndex];
        BOOL small = !(lastIndex != kIndexNotFound && lastIndex >= 64);
        NSUInteger selfBits = small ? indexSetBitfieldFromRanges(self) : 0;
        return selfBits == otherBits ? small : NO;
    }
    NSUInteger selfCount = self->_indexSetFlags._hasSingleRange
                               ? self->_internal._singleRange._range.length != 0
                               : indexSetStorage(self)->rangeCount;
    NSUInteger otherCount = other->_indexSetFlags._hasSingleRange
                                ? other->_internal._singleRange._range.length != 0
                                : indexSetStorage(other)->rangeCount;
    if (selfCount != otherCount) {
        return NO;
    }
    NSUInteger selfIndexes = 0;
    if (!_isEmpty(self)) {
        selfIndexes = self->_indexSetFlags._hasSingleRange ? self->_internal._singleRange._range.length
                                                           : indexSetStorage(self)->indexCount;
    }
    NSUInteger otherIndexes = 0;
    if (!_isEmpty(other)) {
        otherIndexes = other->_indexSetFlags._hasSingleRange ? other->_internal._singleRange._range.length
                                                             : indexSetStorage(other)->indexCount;
    }
    if (selfIndexes != otherIndexes) {
        return NO;
    }
    ONSRange *selfRanges = indexSetRanges(self);
    ONSRange *otherRanges = indexSetRanges(other);
    if (selfCount == 0) {
        return YES;
    }
    BOOL equal;
    NSUInteger rangeIndex = 0;
    do {
        equal = selfRanges[rangeIndex].location == otherRanges[rangeIndex].location &&
                selfRanges[rangeIndex].length == otherRanges[rangeIndex].length;
        if (!equal) {
            break;
        }
        rangeIndex++;
    } while (rangeIndex != selfCount);
    return equal;
}

// Native -[NSIndexSet isEqual:], 0x1c0529cf8. Non-external.
- (BOOL)isEqual:(id)object {
    if (object == self) {
        return YES;
    }
    if (object != nil && objc_opt_isKindOfClass((id)(object), (id)((Class)indexSetClassObject))) {
        return [self isEqualToIndexSet:object];
    }
    return NO;
}

// Native -[NSIndexSet copyWithZone:], 0x1c0529d58. Non-external. A tagged set answers itself; an NSIndexSet
// is retained; any other class is copied into a new NSIndexSet in the zone.
- (id)copyWithZone:(struct _NSZone *)zone {
    if (indexSetIsTagged(self)) {
        return self;
    }
    if (objc_opt_class((id)(self)) == (Class)indexSetClassObject) {
        return objc_retain((id)(self));
    }
    return [[(Class)indexSetClassObject allocWithZone:zone] initWithIndexSet:self];
}

// Native -[NSIndexSet mutableCopyWithZone:], 0x1c0529dd0. Non-external.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return [[(Class)mutableIndexSetClassObject allocWithZone:zone] initWithIndexSet:self];
}

// Native -[NSIndexSet count], 0x1c0529dfc. Non-external. The population count of a bitfield (CNT and ADDV,
// the low byte), the inline length, or the storage's indexCount when it holds ranges.
- (NSUInteger)count {
    NSUInteger bitfield;
    if (indexSetIsTagged(self)) {
        bitfield = indexSetTaggedBitfield(self);
    } else if (self->_indexSetFlags._hasBitfield) {
        bitfield = self->_internal._singleBitfield._bitfield;
    } else {
        if (self->_indexSetFlags._hasSingleRange) {
            return self->_internal._singleRange._range.length;
        }
        IndexSetRangeStorage *storage = indexSetStorage(self);
        return storage->rangeCount != 0 ? storage->indexCount : 0;
    }
    return (uint8_t)__builtin_popcountll(bitfield);
}

// Native -[NSIndexSet firstIndex], 0x1c0529e80. Non-external.
- (NSUInteger)firstIndex {
    NSUInteger bitfield;
    if (indexSetIsTagged(self)) {
        bitfield = indexSetTaggedBitfield(self);
    } else if (self->_indexSetFlags._hasBitfield) {
        bitfield = self->_internal._singleBitfield._bitfield;
    } else {
        if (self->_indexSetFlags._hasSingleRange) {
            if (self->_internal._singleRange._range.length != 0) {
                return self->_internal._singleRange._range.location;
            }
        } else {
            IndexSetRangeStorage *storage = indexSetStorage(self);
            if (storage->rangeCount != 0) {
                return storage->ranges[storage->firstRange].location;
            }
        }
        return kIndexNotFound;
    }
    int lowest = bitfield != 0 ? __builtin_ctzll(bitfield) : -1;
    return lowest <= -1 ? kIndexNotFound : (NSUInteger)lowest;
}

// Native -[NSIndexSet lastIndex], 0x1c0529f28. Non-external.
- (NSUInteger)lastIndex {
    NSUInteger bitfield;
    if (indexSetIsTagged(self)) {
        bitfield = indexSetTaggedBitfield(self);
        if (bitfield == 0) {
            return kIndexNotFound;
        }
        return (NSUInteger)__builtin_clzll(bitfield) ^ 0x3F;
    }
    if (self->_indexSetFlags._hasBitfield) {
        bitfield = self->_internal._singleBitfield._bitfield;
        if (bitfield != 0) {
            return (NSUInteger)__builtin_clzll(bitfield) ^ 0x3F;
        }
    } else if (self->_indexSetFlags._hasSingleRange) {
        if (self->_internal._singleRange._range.length != 0) {
            ONSRange *range = &self->_internal._singleRange._range;
            return range->location + range->length - 1;
        }
    } else {
        IndexSetRangeStorage *storage = indexSetStorage(self);
        NSUInteger count = storage->rangeCount;
        if (count != 0) {
            ONSRange *range = &storage->ranges[storage->firstRange + count - 1];
            return range->location + range->length - 1;
        }
    }
    return kIndexNotFound;
}

// Native -[NSIndexSet _indexClosestToIndex:equalAllowed:following:], 0x1c0529fd4. Non-external, an
// objc_direct method (nil receiver answers 0). The index nearest the given one that is greater (following)
// or less, or equal when allowed. Bitfields mask and count bits (the index moved by +1, -1 or 0 first, an
// index past 63 following or below 0 preceding answering NSNotFound). A single range answers from its ends
// or falls through to the range search. A storage set first consults the sequential-lookup cache when this
// thread owns it and the index is the cached one, stepping within or across ranges (releasing the cache with
// a store-release of 0 at either end); otherwise the range search finds the range and, when the cache is
// owned by this thread or can be claimed with compare-and-swap, records the answer in it.
- (NSUInteger)_indexClosestToIndex:(NSUInteger)index
                      equalAllowed:(BOOL)equalAllowed
                         following:(BOOL)following {
    NSUInteger bitfield;
    NSUInteger answer;
    NSUInteger rangeIndex;
    NSUInteger rangeLocation;
    ONSRange *ranges;
    if (indexSetIsTagged(self)) {
        bitfield = indexSetTaggedBitfield(self);
        if (bitfield == 0) {
            return kIndexNotFound;
        }
        goto bits;
    }
    if (self->_indexSetFlags._hasBitfield) {
        bitfield = self->_internal._singleBitfield._bitfield;
        if (bitfield == 0) {
            return kIndexNotFound;
        }
        goto bits;
    }
    if (self->_indexSetFlags._hasSingleRange) {
        NSUInteger length = self->_internal._singleRange._range.length;
        if (length == 0) {
            return kIndexNotFound;
        }
        NSUInteger location = self->_internal._singleRange._range.location;
        NSUInteger last = location + length - 1;
        if (following) {
            if (equalAllowed) {
                if (location >= index) {
                    return location;
                }
                if (last >= index) {
                    return index;
                }
                goto after;
            }
            if (location > index) {
                return location;
            }
            if (last > index) {
                return index + 1;
            }
            goto afterNext;
        }
        if (equalAllowed) {
            if (last <= index) {
                return last;
            }
            if (location <= index) {
                return index;
            }
            goto before;
        }
        if (last < index) {
            return last;
        }
        if (location < index) {
            return index - 1;
        }
        goto beforePrevious;
    }
    {
        if (indexSetStorage(self)->rangeCount == 0) {
            return kIndexNotFound;
        }
        uintptr_t thread = (uintptr_t)pthread_self();
        if (__atomic_load_n(&indexSetStorage(self)->cacheOwner, __ATOMIC_ACQUIRE) == thread) {
            IndexSetRangeStorage *storage = indexSetStorage(self);
            if (storage->cachedIndex == index) {
                if (equalAllowed) {
                    return index;
                }
                ONSRange *cachedRanges = self->_indexSetFlags._hasSingleRange
                                             ? &self->_internal._singleRange._range
                                             : &storage->ranges[storage->firstRange];
                if (following) {
                    NSUInteger cachedRange = storage->cachedRange;
                    if (cachedRanges[cachedRange].length - 1 > storage->cachedOffset) {
                        storage->cachedIndex = index + 1;
                        indexSetStorage(self)->cachedOffset = indexSetStorage(self)->cachedOffset + 1;
                        return indexSetStorage(self)->cachedIndex;
                    }
                    if (!_isEmpty(self)) {
                        NSUInteger count = self->_indexSetFlags._hasSingleRange ? 1 : storage->rangeCount;
                        if (cachedRange + 1 < count) {
                            storage->cachedRange = cachedRange + 1;
                            NSUInteger location = cachedRanges[indexSetStorage(self)->cachedRange].location;
                            indexSetStorage(self)->cachedOffset = 0;
                            indexSetStorage(self)->cachedIndex = location;
                            return indexSetStorage(self)->cachedIndex;
                        }
                    }
                } else {
                    if (storage->cachedOffset != 0) {
                        storage->cachedIndex = index - 1;
                        indexSetStorage(self)->cachedOffset = indexSetStorage(self)->cachedOffset - 1;
                        return indexSetStorage(self)->cachedIndex;
                    }
                    NSUInteger cachedRange = storage->cachedRange;
                    if (cachedRange != 0) {
                        storage->cachedRange = cachedRange - 1;
                        ONSRange *range = &cachedRanges[indexSetStorage(self)->cachedRange];
                        NSUInteger location = range->location;
                        indexSetStorage(self)->cachedOffset = range->length - 1;
                        indexSetStorage(self)->cachedIndex = indexSetStorage(self)->cachedOffset + location;
                        return indexSetStorage(self)->cachedIndex;
                    }
                }
                __atomic_store_n(&storage->cacheOwner, 0, __ATOMIC_RELEASE);
                return kIndexNotFound;
            }
        }
        if (following) {
            if (equalAllowed) {
                goto after;
            }
            goto afterNext;
        }
        if (equalAllowed) {
            goto before;
        }
        goto beforePrevious;
    }

bits:
    {
        NSInteger delta = following ? 1 : -1;
        if (equalAllowed) {
            delta = 0;
        }
        // Native ADD wraps before the signed tests. Avoid signed C overflow for NSNotFound/large inputs.
        NSInteger target = (NSInteger)(index + (NSUInteger)delta);
        NSUInteger masked;
        NSUInteger found;
        if (following) {
            if (target > 63) {
                return kIndexNotFound;
            }
            NSUInteger shift = (NSUInteger)(target & ~(OF_WORD_SHIFT(target, 63)));
            masked = bitfield & (~(NSUInteger)0 << shift);
            found = masked != 0 ? (NSUInteger)__builtin_ctzll(masked) : 0;
        } else {
            if (target < 0) {
                return kIndexNotFound;
            }
            NSUInteger top = (NSUInteger)target < 63 ? (NSUInteger)target : 63;
            masked = bitfield & (~(NSUInteger)0 >> (top ^ 63));
            found = masked != 0 ? (NSUInteger)__builtin_clzll(masked) ^ 63 : 0;
        }
        answer = masked != 0 ? found : ~(NSUInteger)0;
        if (answer >= 64) {
            return kIndexNotFound;
        }
        return answer;
    }

afterNext:
    if (index > (LONG_MAX - 1)) {
        return kIndexNotFound;
    }
    index++;
after:
    ranges = indexSetRanges(self);
    rangeIndex = [self _indexOfRangeAfterOrContainingIndex:index];
    if (rangeIndex == kIndexNotFound) {
        return kIndexNotFound;
    }
    rangeLocation = ranges[rangeIndex].location;
    answer = index > rangeLocation ? index : rangeLocation;
    goto cache;

beforePrevious:
    if (index == 0) {
        return kIndexNotFound;
    }
    index--;
before:
    {
        ranges = indexSetRanges(self);
        rangeIndex = [self _indexOfRangeBeforeOrContainingIndex:index];
        if (rangeIndex == kIndexNotFound) {
            return kIndexNotFound;
        }
        ONSRange *range = &ranges[rangeIndex];
        rangeLocation = range->location;
        NSUInteger rangeLast = range->location + range->length - 1;
        answer = index < rangeLast ? index : rangeLast;
    }

cache:
    {
        uintptr_t thread = (uintptr_t)pthread_self();
        uintptr_t expected = 0;
        if (__atomic_load_n(&indexSetStorage(self)->cacheOwner, __ATOMIC_ACQUIRE) == thread ||
            __atomic_compare_exchange_n(
                &indexSetStorage(self)->cacheOwner,
                &expected,
                thread,
                false,
                __ATOMIC_ACQ_REL,
                __ATOMIC_ACQUIRE
            )) {
            indexSetStorage(self)->cachedRange = rangeIndex;
            indexSetStorage(self)->cachedOffset = answer - rangeLocation;
            indexSetStorage(self)->cachedIndex = answer;
        }
        return answer;
    }
}

// Native -[NSIndexSet indexGreaterThanIndex:], 0x1c052a3d8. Non-external.
- (NSUInteger)indexGreaterThanIndex:(NSUInteger)value {
    return [self _indexClosestToIndex:value equalAllowed:NO following:YES];
}

// Native -[NSIndexSet indexLessThanIndex:], 0x1c052a3e8. Non-external.
- (NSUInteger)indexLessThanIndex:(NSUInteger)value {
    return [self _indexClosestToIndex:value equalAllowed:NO following:NO];
}

// Native -[NSIndexSet indexGreaterThanOrEqualToIndex:], 0x1c052a3f8. Non-external.
- (NSUInteger)indexGreaterThanOrEqualToIndex:(NSUInteger)value {
    return [self _indexClosestToIndex:value equalAllowed:YES following:YES];
}

// Native -[NSIndexSet getIndexes:maxCount:inIndexRange:], 0x1c052a418. Non-external. Copies up to
// bufferSize indexes inside the range (the whole set when range is NULL) and advances the range past the
// last one copied. A storage set starts from the cached range when this thread owns the cache and the first
// index is the cached one, and afterwards records the last index copied in the cache (owned or claimed with
// compare-and-swap) when it lies inside the next range to visit.
- (NSUInteger)getIndexes:(NSUInteger *)indexBuffer
                maxCount:(NSUInteger)bufferSize
            inIndexRange:(ONSRange *)range {
    if (bufferSize == 0 || [self count] == 0) {
        return 0;
    }
    NSUInteger first;
    NSUInteger last;
    if (range != NULL) {
        if (range->length == 0) {
            return 0;
        }
        first = range->location;
        last = range->length + range->location - 1;
    } else {
        first = [self firstIndex];
        last = [self lastIndex];
    }
    if (last < first) {
        return 0;
    }
    NSUInteger bitfield;
    if (indexSetIsTagged(self)) {
        bitfield = indexSetTaggedBitfield(self);
    } else if (self->_indexSetFlags._hasBitfield) {
        bitfield = self->_internal._singleBitfield._bitfield;
    } else {
        NSUInteger rangeIndex;
        if (self->_indexSetFlags._hasSingleRange ||
            __atomic_load_n(&indexSetStorage(self)->cacheOwner, __ATOMIC_ACQUIRE) !=
                (uintptr_t)pthread_self() ||
            indexSetStorage(self)->cachedIndex != first ||
            (rangeIndex = indexSetStorage(self)->cachedRange) == kIndexNotFound) {
            rangeIndex = [self _indexOfRangeAfterOrContainingIndex:first];
            if (rangeIndex == kIndexNotFound) {
                return 0;
            }
        }
        NSUInteger count = indexSetLoopRangeCount(self);
        ONSRange *ranges = indexSetRanges(self);
        if (rangeIndex > count ||
            (rangeIndex == count && (!self->_indexSetFlags._hasSingleRange || count != 0))) {
            return 0;
        }
        NSUInteger offset = 0;
        NSUInteger written = 0;
        NSUInteger current;
        do {
            ONSRange *rangeAt = &ranges[rangeIndex];
            if (first < rangeAt->location) {
                current = rangeAt->location;
            } else {
                current = first;
                offset = first - rangeAt->location;
            }
            BOOL inRange = offset < rangeAt->length;
            if (current <= last && offset < rangeAt->length) {
                do {
                    indexBuffer[written++] = current++;
                    offset++;
                    inRange = offset < rangeAt->length;
                } while (current <= last && written < bufferSize && offset < rangeAt->length);
            }
            if (!inRange) {
                offset = 0;
            }
            rangeIndex += !inRange;
        } while (current <= last && rangeIndex < count && written < bufferSize);
        if (written == 0) {
            return 0;
        }
        if (range != NULL) {
            NSUInteger consumed = indexBuffer[written - 1] - first + 1;
            range->location = consumed + range->location;
            range->length = range->length - consumed;
        }
        if (rangeIndex < count && !self->_indexSetFlags._hasSingleRange) {
            ONSRange *next = &ranges[rangeIndex];
            NSUInteger lastWritten = indexBuffer[written - 1];
            NSUInteger cachedOffset = lastWritten - next->location;
            if (cachedOffset < next->length) {
                uintptr_t thread = (uintptr_t)pthread_self();
                uintptr_t expected = 0;
                if (__atomic_load_n(&indexSetStorage(self)->cacheOwner, __ATOMIC_ACQUIRE) == thread ||
                    __atomic_compare_exchange_n(
                        &indexSetStorage(self)->cacheOwner,
                        &expected,
                        thread,
                        false,
                        __ATOMIC_ACQ_REL,
                        __ATOMIC_ACQUIRE
                    )) {
                    indexSetStorage(self)->cachedRange = rangeIndex;
                    indexSetStorage(self)->cachedOffset = cachedOffset;
                    indexSetStorage(self)->cachedIndex = lastWritten;
                }
            }
        }
        return written;
    }
    if (first > 63) {
        return 0;
    }
    NSUInteger written = 0;
    NSUInteger position = first;
    while (position <= 63) {
        NSUInteger masked = (~(NSUInteger)0 << position) & bitfield;
        NSUInteger index = indexSetTrailingZeros(masked);
        if (masked == 0 || last < index) {
            break;
        }
        position = index + 1;
        indexBuffer[written++] = index;
        if (bufferSize == written) {
            written = bufferSize;
            break;
        }
    }
    if (range != NULL && written != 0) {
        NSUInteger consumed = indexBuffer[written - 1] - first + 1;
        range->location = consumed + range->location;
        range->length = range->length - consumed;
    }
    return written;
}

// Native -[NSIndexSet __getContainmentVector:inRange:], 0x1c052a760. Non-external. Sets vector[i] to 1 for
// each member range.location + i (after zeroing range.length bytes) and answers the member count. The
// indexes are gathered into a 256-entry stack buffer, or for ranges longer than 256 into a heap buffer of
// range.length entries (the stack reservation then shrinks to one entry).
- (NSUInteger)__getContainmentVector:(BOOL *)vector inRange:(ONSRange)range {
    NSUInteger entries = range.length >= 257 ? 1 : 256;
    NSUInteger *stackBuffer = alloca((8 * entries + 15) & ~(NSUInteger)15);
    NSUInteger *buffer = stackBuffer;
    if (range.length >= 257) {
        buffer = malloc_type_malloc(8 * range.length, kIndexSetIndexBufferTypeID);
    }
    ONSRange searched = range;
    NSUInteger count = [self getIndexes:buffer maxCount:range.length inIndexRange:&searched];
    bzero(vector, range.length);
    for (NSUInteger slot = 0; slot != count; slot++) {
        vector[buffer[slot] - range.location] = 1;
    }
    if (buffer != stackBuffer) {
        free(buffer);
    }
    return count;
}

// Native -[NSIndexSet rangeCount], 0x1c052aa48. Non-external. Bitfields count their runs of set bits
// (_bitfieldSetRangesCount's loop, inlined natively).
- (NSUInteger)rangeCount {
    NSUInteger bitfield;
    if (indexSetIsTagged(self)) {
        bitfield = indexSetTaggedBitfield(self);
    } else if (self->_indexSetFlags._hasBitfield) {
        bitfield = self->_internal._singleBitfield._bitfield;
    } else if (self->_indexSetFlags._hasSingleRange) {
        return self->_internal._singleRange._range.length != 0;
    } else {
        return indexSetStorage(self)->rangeCount;
    }
    return _bitfieldSetRangesCount(bitfield);
}

// Native _bitfieldSetRangesCount, 0x1c052ab00. Non-external. The number of runs of set bits: alternately
// skip to the next clear bit and the next set bit, counting the runs (0 for an empty mask).
static NSUInteger _bitfieldSetRangesCount(NSUInteger bitfield) {
    if (bitfield == 0) {
        return 0;
    }
    NSUInteger clear = ~bitfield;
    NSUInteger count = 1;
    NSUInteger remaining = bitfield;
    for (;;) {
        NSUInteger gap = (~(NSUInteger)0 << __builtin_ctzll(remaining)) & clear;
        if (gap == 0) {
            return count;
        }
        count++;
        remaining = (~(NSUInteger)0 << __builtin_ctzll(gap)) & bitfield;
        if (remaining == 0) {
            return count - 1;
        }
    }
}

// Native -[NSIndexSet rangeAtIndex:], 0x1c052ab4c. Non-external. The range at the position; out-of-bounds
// positions raise NSInvalidArgumentException ("Index %ld out of bounds [0...%ld]" with the range count).
//
// Foundation defect: 0x1c052abb4/0x1c052ac64 accept index == rangeCount. For nonempty inline
// storage this reads past the 32-byte instance; for heap storage it reads outside initialized ranges and
// may cross the allocation. Raise for these unsafe cases; confidence: verified. An empty inline set at
// index 0 reads its valid inline range and remains allowed (kept).
- (ONSRange)rangeAtIndex:(NSUInteger)rangeIndex {
    NSUInteger bitfield;
    if (indexSetIsTagged(self)) {
        bitfield = indexSetTaggedBitfield(self);
    } else if (self->_indexSetFlags._hasBitfield) {
        bitfield = self->_internal._singleBitfield._bitfield;
    } else {
        NSUInteger count = self->_indexSetFlags._hasSingleRange
                               ? self->_internal._singleRange._range.length != 0
                               : indexSetStorage(self)->rangeCount;
        if (rangeIndex >= count) {
            id reason = [(id<IndexSetMessages>)(id)indexSetStringClass
                stringWithFormat:(id)OF_STR("Index %ld out of bounds [0...%ld]"), rangeIndex, count];
            objc_exception_throw([(id<IndexSetMessages>)(id)indexSetExceptionClass
                exceptionWithName:(id)ONSInvalidArgumentException
                           reason:reason
                         userInfo:nil]);
        }
        return indexSetRanges(self)[rangeIndex];
    }
    NSUInteger clear = ~bitfield;
    NSUInteger end = 0;
    NSUInteger countdown = rangeIndex + 1;
    for (;;) {
        NSUInteger members = (~(NSUInteger)0 << end) & bitfield;
        if (members == 0) {
            break;
        }
        NSUInteger start = (NSUInteger)__builtin_ctzll(members);
        end = indexSetTrailingZeros((~(NSUInteger)0 << start) & clear);
        if (--countdown == 0) {
            return (ONSRange){start, end - start};
        }
        if (end >= 64) {
            break;
        }
    }
    id reason = [(id<IndexSetMessages>)(id)indexSetStringClass
        stringWithFormat:(id)OF_STR("Index %ld out of bounds [0...%ld]"),
                         rangeIndex,
                         _bitfieldSetRangesCount(bitfield)];
    objc_exception_throw([(id<IndexSetMessages>)(id)indexSetExceptionClass
        exceptionWithName:(id)ONSInvalidArgumentException
                   reason:reason
                 userInfo:nil]);
}

// Native -[NSIndexSet getBitfield:], 0x1c052acb8. Non-external. Tagged and bitfield sets store their mask
// (when the pointer is not NULL) and answer YES; range sets answer NO.
- (BOOL)getBitfield:(NSUInteger *)bitfield {
    NSUInteger value;
    if (indexSetIsTagged(self)) {
        value = indexSetTaggedBitfield(self);
    } else if (self->_indexSetFlags._hasBitfield) {
        value = self->_internal._singleBitfield._bitfield;
    } else {
        return NO;
    }
    if (bitfield != NULL) {
        *bitfield = value;
    }
    return YES;
}

// Native -[NSIndexSet description], 0x1c052b53c. Non-external. NSObject's description followed by
// "[number of indexes: %lu (in %lu ranges), indexes: (" and the ranges ("%lu" or "%lu-%lu", separated by
// spaces) and ")]", or "(no indexes)", in an NSMutableString of capacity 128.
- (id)description {
    id string = [(id<IndexSetMessages>)(id)indexSetMutableStringClass stringWithCapacity:128];
    [(id<IndexSetMessages>)string appendString:[super description]];
    NSUInteger count = [self count];
    if (count == 0) {
        [(id<IndexSetMessages>)string appendFormat:(id)OF_STR("(no indexes)")];
        return string;
    }
    NSUInteger rangeCount = [self rangeCount];
    [(id<IndexSetMessages>)string
        appendFormat:(id)OF_STR("[number of indexes: %lu (in %lu ranges), indexes: ("), count, rangeCount];
    for (NSUInteger rangeIndex = 0; rangeIndex != rangeCount; rangeIndex++) {
        if (rangeIndex != 0) {
            [(id<IndexSetMessages>)string appendString:(id)OF_STR(" ")];
        }
        ONSRange range = [self rangeAtIndex:rangeIndex];
        [(id<IndexSetMessages>)string appendFormat:(id)OF_STR("%lu"), range.location];
        if (range.length >= 2) {
            [(id<IndexSetMessages>)string appendFormat:(id)OF_STR("-%lu"), range.location + range.length - 1];
        }
    }
    [(id<IndexSetMessages>)string appendString:(id)OF_STR(")]")];
    return string;
}

// Native __NSIndexSetEnumerateBitfield, 0x1c052b890. Non-external. Enumerates a membership mask over the
// window [start, start + length) clipped to 64 (NSNotFound for a start past 63 or an empty window). Forward,
// the runs are found one after the other from a moving cursor (the native skip counter always selects the
// next run); reverse, the run count is taken first and each run is found again from bit 0 by its position.
// Each run is clipped to the window; range blocks receive the clipped run, index blocks each index (backward
// for reverse). A passing-test block's YES stops and becomes the answer (NSNotFound otherwise).
static NSUInteger __NSIndexSetEnumerateBitfield(
    NSUInteger bitfield, NSUInteger options, NSUInteger start, NSUInteger length, id block
) {
    NSUInteger end = start + length;
    NSUInteger limit = end < 64 ? end : 64;
    NSUInteger clampedStart = start > 63 ? 0 : start;
    if (start > 63 || limit == start) {
        return kIndexNotFound;
    }
    BOOL stop = NO;
    NSUInteger found;
    if (!(options & kEnumerationReverse)) {
        NSUInteger clear = ~bitfield;
        NSUInteger cursor = 0;
        NSUInteger visited = 0;
        NSUInteger skip = ~(NSUInteger)0;
        found = kIndexNotFound;
        do {
            NSUInteger counter = (cursor != 0 ? visited : 0) + skip;
            NSUInteger runStart;
            for (;;) {
                NSUInteger members = (~(NSUInteger)0 << cursor) & bitfield;
                if (members == 0) {
                    return found;
                }
                runStart = (NSUInteger)__builtin_ctzll(members);
                cursor = indexSetTrailingZeros((~(NSUInteger)0 << runStart) & clear);
                bool reached = counter == ~(NSUInteger)0;
                counter++;
                if (reached) {
                    break;
                }
                if (cursor > 63) {
                    return found;
                }
            }
            NSUInteger clippedEnd = limit < cursor ? limit : cursor;
            NSUInteger location = 0;
            NSUInteger runLength = 0;
            if (start <= runStart && end > runStart) {
                location = runStart;
                runLength = clippedEnd - runStart;
            }
            if (runStart <= start && start < cursor) {
                location = start;
                runLength = clippedEnd - start;
            }
            if (runLength != 0) {
                if (options & kEnumerateRanges) {
                    __NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__(
                        block, location, runLength, (uintptr_t)&stop
                    );
                    if (stop) {
                        return found;
                    }
                } else {
                    NSUInteger stopAt = location + runLength;
                    if (location < stopAt) {
                        do {
                            if (options & kEnumeratePassingTest) {
                                if (__NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__(
                                        block, location, (uintptr_t)&stop, 0
                                    )) {
                                    stop = YES;
                                    found = location;
                                }
                            } else {
                                __NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__(
                                    block, location, (uintptr_t)&stop, 0
                                );
                            }
                            if (stop) {
                                break;
                            }
                            location++;
                        } while (location < stopAt);
                        if (stop) {
                            return found;
                        }
                    }
                }
            }
            skip--;
            visited++;
        } while (cursor < 64);
        return found;
    }
    if (bitfield == 0) {
        return kIndexNotFound;
    }
    NSUInteger clear = ~bitfield;
    NSUInteger remaining = _bitfieldSetRangesCount(bitfield);
    if (remaining == 0) {
        return kIndexNotFound;
    }
    found = kIndexNotFound;
    do {
        NSUInteger cursor = 0;
        NSUInteger countdown = remaining;
        remaining--;
        NSUInteger runStart;
        NSUInteger runLength;
        for (;;) {
            NSUInteger members = (~(NSUInteger)0 << cursor) & bitfield;
            if (members == 0) {
                runLength = 0;
                runStart = kIndexNotFound;
                break;
            }
            runStart = (NSUInteger)__builtin_ctzll(members);
            cursor = indexSetTrailingZeros((~(NSUInteger)0 << runStart) & clear);
            if (--countdown == 0) {
                runLength = cursor - runStart;
                break;
            }
            if (cursor > 63) {
                runLength = 0;
                runStart = kIndexNotFound;
                break;
            }
        }
        NSUInteger runEnd = runLength + runStart;
        NSUInteger clippedEnd = limit < runEnd ? limit : runEnd;
        NSUInteger location;
        NSUInteger clippedLength;
        if (runStart <= start && start < runEnd) {
            clippedLength = clippedEnd - start;
            location = clampedStart;
        } else {
            NSUInteger candidateLength = clippedEnd - runStart;
            NSUInteger candidate = runStart;
            if (runStart >= limit) {
                candidate = 0;
                candidateLength = 0;
            }
            location = start > runStart ? 0 : candidate;
            clippedLength = start > runStart ? 0 : candidateLength;
        }
        if (clippedLength == 0) {
            continue;
        }
        if (options & kEnumerateRanges) {
            __NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__(block, location, clippedLength, (uintptr_t)&stop);
            if (stop) {
                return found;
            }
            continue;
        }
        NSUInteger stopAt = clippedLength + location;
        if ((NSInteger)(stopAt - 1) < (NSInteger)location) {
            continue;
        }
        NSInteger position = (NSInteger)stopAt - 2;
        for (;;) {
            NSUInteger index = (NSUInteger)(position + 1);
            if (options & kEnumeratePassingTest) {
                if (__NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__(block, index, (uintptr_t)&stop, 0)) {
                    stop = YES;
                    found = index;
                }
            } else {
                __NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__(block, index, (uintptr_t)&stop, 0);
            }
            if (stop) {
                break;
            }
            bool more = position >= (NSInteger)location;
            position--;
            if (!more) {
                break;
            }
        }
        if (stop) {
            return found;
        }
    } while (remaining != 0);
    return found;
}

// Native __NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__, 0x1c052bbf4. Non-external. Calls the block's invoke
// function with the block and the three argument registers it was given (index and stop for index blocks;
// location, length and stop for range blocks) and answers its result. Kept out of line, as natively, so that
// it names the callout in backtraces.
static __attribute__((noinline)) BOOL
__NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__(id block, uintptr_t first, uintptr_t second, uintptr_t third) {
    IndexSetBlockLiteral *literal = (IndexSetBlockLiteral *)block;
    return ((BOOL (*)(id, uintptr_t, uintptr_t, uintptr_t))literal->invoke)(block, first, second, third);
}

// Native __NSIndexSetParameterCheckIterate, 0x1c052bc50. Non-external. A nil block raises
// NSInvalidArgumentException ("%@: block is nil"); with the range check option and a nonempty range, a
// location past NSNotFound - 1 or a length of NSNotFound or more raises NSRangeException ("%@: a range field
// {%ld, %ld} is NSNotFound or beyond bounds (%ld)"), and so does a range whose end is not positive as a
// signed value ("%@: last range index (%ld) beyond bounds (%ld)").
static void __NSIndexSetParameterCheckIterate(
    id object, SEL selector, id block, NSUInteger options, NSUInteger start, NSUInteger length
) {
    id reason;
    id name;
    if (block == nil) {
        reason = [(id<IndexSetMessages>)(id)indexSetStringClass
            stringWithFormat:(id)OF_STR("%@: block is nil"), _ONSMethodExceptionProem(object, selector)];
        name = (id)ONSInvalidArgumentException;
    } else {
        if (!(options & kEnumerateCheckRange) || length == 0) {
            return;
        }
        if (start > (LONG_MAX - 1) || length >= kIndexNotFound) {
            reason = [(id<IndexSetMessages>)(id)indexSetStringClass
                stringWithFormat:(id)OF_STR(
                                     "%@: a range field {%ld, %ld} is NSNotFound or beyond bounds (%ld)"
                                 ),
                                 _ONSMethodExceptionProem(object, selector),
                                 start,
                                 length,
                                 kIndexNotFound];
        } else {
            if ((NSInteger)(start + length) > 0) {
                return;
            }
            reason = [(id<IndexSetMessages>)(id)indexSetStringClass
                stringWithFormat:(id)OF_STR("%@: last range index (%ld) beyond bounds (%ld)"),
                                 _ONSMethodExceptionProem(object, selector),
                                 start + length - 1,
                                 (NSUInteger)0x7FFFFFFFFFFFFFFE];
        }
        name = (id)ONSRangeException;
    }
    objc_exception_throw([(id<IndexSetMessages>)(id)indexSetExceptionClass exceptionWithName:name
                                                                                      reason:reason
                                                                                    userInfo:nil]);
}

// Native __NSIndexSetEnumerate, 0x1c052bd5c. Non-external. Enumerates the set over the window
// [start, start + length) and answers the index a passing-test block selected (NSNotFound otherwise; also
// for an empty window). Tagged and bitfield sets go to __NSIndexSetEnumerateBitfield. Concurrent
// enumeration (only with at least two active processors, the commpage byte at 0xFFFFFC034) resolves the
// first and last range positions (range blocks) or indexes, picks a chunk of 64, 48, 32, 16, 4 or 1 from
// their distance against multiples of the processor count, and runs dispatch_apply over the chunks with
// __NSIndexSetChunkIterate (native _____NSIndexSetEnumerate_block_invoke, 0x1c052ed40: a chunk is skipped
// once the shared stop byte, read with load-acquire, is set, and its result is stored back with
// store-release); afterwards a storage set releases the lookup cache this thread owns (compare-and-swap of
// this thread to 0) and the shared result is read with load-acquire. Serial enumeration runs inside an
// autorelease pool over the ranges from the range search, clipping each range to the window and stopping at
// the first range outside it, then releases the lookup cache the same way.
static NSUInteger
__NSIndexSetEnumerate(ONSIndexSet *set, NSUInteger options, NSUInteger start, NSUInteger length, id block) {
    if (length == 0) {
        return kIndexNotFound;
    }
    if (indexSetIsTagged(set)) {
        return __NSIndexSetEnumerateBitfield(indexSetTaggedBitfield(set), options, start, length, block);
    }
    if (set->_indexSetFlags._hasBitfield) {
        return __NSIndexSetEnumerateBitfield(
            set->_internal._singleBitfield._bitfield, options, start, length, block
        );
    }
    NSUInteger processors = 1;
    if (options & kEnumerationConcurrent) {
        processors = OF_PORT_ANSWER(commpageActiveCPUs, *(volatile const uint8_t *)(uintptr_t)0xFFFFFC034, );
        if (processors < 2) {
            options &= ~(NSUInteger)1;
        }
    }
    NSUInteger answer;
    if (options & kEnumerationConcurrent) {
        NSUInteger end = start + length;
        NSUInteger first;
        NSUInteger last;
        if (options & kEnumerateRanges) {
            first = [set _indexOfRangeAfterOrContainingIndex:start];
            last = [set _indexOfRangeBeforeOrContainingIndex:end - 1];
        } else {
            first = [set indexGreaterThanOrEqualToIndex:start];
            last = [set indexLessThanIndex:end];
        }
        if (first == kIndexNotFound || last == kIndexNotFound) {
            return kIndexNotFound;
        }
        NSUInteger distance = last - first;
        NSUInteger span = distance + 1;
        NSUInteger chunk = span >= 4 * processors ? 4 : 1;
        if (span >= 16 * processors) {
            chunk = 16;
        }
        if (span >= 32 * processors) {
            chunk = 32;
        }
        if (((processors * 3) << 4) <= span) {
            chunk = 48;
        }
        if (span >= processors << 6) {
            chunk = 64;
        }
        NSUInteger iterations = (chunk + distance) / chunk;
        __block NSUInteger result = kIndexNotFound;
        __block BOOL stop = NO;
        dispatch_apply(iterations, NULL, ^(size_t iteration) {
          if (__atomic_load_n(&stop, __ATOMIC_ACQUIRE) & 1) {
              return;
          }
          NSUInteger chunkFirst = chunk * iteration + first;
          NSUInteger chunkLast = chunk + chunkFirst - 1;
          if (!(chunkLast < last)) {
              chunkLast = last;
          }
          BOOL stopped =
              __NSIndexSetChunkIterate(set, options, chunkFirst, chunkLast, start, length, block, &result);
          __atomic_store_n(&stop, stopped, __ATOMIC_RELEASE);
        });
        if (!set->_indexSetFlags._hasSingleRange) {
            uintptr_t expected = (uintptr_t)pthread_self();
            __atomic_compare_exchange_n(
                &indexSetStorage(set)->cacheOwner, &expected, 0, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            );
        }
        return __atomic_load_n(&result, __ATOMIC_ACQUIRE);
    }
    void *pool = _OCFAutoreleasePoolPush();
    NSUInteger count = indexSetLoopRangeCount(set);
    ONSRange *ranges = indexSetRanges(set);
    BOOL stop = NO;
    NSUInteger end = start + length;
    if (options & kEnumerationReverse) {
        NSUInteger rangeIndex = [set _indexOfRangeBeforeOrContainingIndex:end - 1];
        if (rangeIndex > (LONG_MAX - 1)) {
            goto done;
        }
        for (;;) {
            ONSRange *range = &ranges[rangeIndex];
            NSUInteger location = range->location;
            NSUInteger rangeEnd = range->length + location;
            NSUInteger clippedEnd = rangeEnd < end ? rangeEnd : end;
            NSUInteger clippedLength;
            if (start <= location && location < end) {
                clippedLength = clippedEnd - location;
            } else if (location <= start && start < rangeEnd) {
                clippedLength = clippedEnd - start;
                location = start;
            } else {
                goto done;
            }
            if (clippedLength == 0) {
                goto done;
            }
            if (options & kEnumerateRanges) {
                __NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__(
                    block, location, clippedLength, (uintptr_t)&stop
                );
                if ((NSInteger)rangeIndex < 1 || stop) {
                    goto done;
                }
            } else {
                NSUInteger index = location + clippedLength;
                while (location < index) {
                    index--;
                    if (options & kEnumeratePassingTest) {
                        if (__NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__(
                                block, index, (uintptr_t)&stop, 0
                            )) {
                            answer = index;
                            goto release;
                        }
                    } else {
                        __NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__(block, index, (uintptr_t)&stop, 0);
                    }
                    if (stop) {
                        goto done;
                    }
                }
                if ((NSInteger)rangeIndex < 1) {
                    goto done;
                }
            }
            rangeIndex--;
        }
    }
    {
        NSUInteger rangeIndex = [set _indexOfRangeAfterOrContainingIndex:start];
        if (rangeIndex == kIndexNotFound || rangeIndex >= count) {
            goto done;
        }
        do {
            ONSRange *range = &ranges[rangeIndex];
            NSUInteger location = range->location;
            NSUInteger rangeEnd = range->length + location;
            NSUInteger clippedEnd = rangeEnd < end ? rangeEnd : end;
            NSUInteger clippedLength;
            if (start <= location && location < end) {
                clippedLength = clippedEnd - location;
            } else if (location <= start && start < rangeEnd) {
                clippedLength = clippedEnd - start;
                location = start;
            } else {
                goto done;
            }
            if (clippedLength == 0) {
                goto done;
            }
            if (options & kEnumerateRanges) {
                __NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__(
                    block, location, clippedLength, (uintptr_t)&stop
                );
                if (stop) {
                    goto done;
                }
            } else if (location < location + clippedLength) {
                do {
                    if (options & kEnumeratePassingTest) {
                        if (__NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__(
                                block, location, (uintptr_t)&stop, 0
                            )) {
                            answer = location;
                            goto release;
                        }
                    } else {
                        __NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__(block, location, (uintptr_t)&stop, 0);
                    }
                    if (stop) {
                        goto done;
                    }
                    location++;
                    clippedLength--;
                } while (clippedLength != 0);
            }
            rangeIndex++;
        } while (rangeIndex < count);
    }
done:
    answer = kIndexNotFound;
release:
    if (!set->_indexSetFlags._hasSingleRange) {
        uintptr_t expected = (uintptr_t)pthread_self();
        __atomic_compare_exchange_n(
            &indexSetStorage(set)->cacheOwner, &expected, 0, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
        );
    }
    _OCFAutoreleasePoolPop(pool);
    return answer;
}

// Native -[NSIndexSet enumerateRangesUsingBlock:], 0x1c052c948. Non-external. The whole set, forward, with
// a range block.
- (void)enumerateRangesUsingBlock:(void (^)(ONSRange range, BOOL *stop))block {
    __NSIndexSetParameterCheckIterate(self, _cmd, block, kEnumerateRanges, 0, kIndexNotFound);
    __NSIndexSetEnumerate(self, kEnumerateRanges, 0, kIndexNotFound, block);
}

@end

@implementation ONSMutableIndexSet

// Native -[NSMutableIndexSet classForCoder], 0x1c052ca5c. Non-external.
- (Class)classForCoder {
    return (Class)mutableIndexSetClassObject;
}

// Native -[NSMutableIndexSet _ensureRangeCapacity:], 0x1c052ca68. Non-external. Grows the room for ranges
// to at least the capacity: a capacity of 1 only sets the single-range flag; otherwise the new capacity is
// the larger of twice the current one and the request (at least 4). Storage is reallocated
// (__CFSafelyReallocateTyped, no failure block) and its ranges re-centred (firstRange (capacity - count) / 2
// once the capacity is 5 or more and covers the count, else 0); an inline set moves its range into new
// storage (with the range count 1 or 0 and the index count from the inline length).
- (void)_ensureRangeCapacity:(NSUInteger)capacity {
    NSUInteger current = self->_indexSetFlags._hasSingleRange ? 1 : indexSetStorage(self)->capacity;
    if (capacity <= current) {
        return;
    }
    if (capacity == 1) {
        self->_indexSetFlags._hasSingleRange = 1;
        return;
    }
    NSUInteger doubled = current << 1;
    NSUInteger grown = doubled > capacity ? doubled : capacity;
    NSUInteger newCapacity = capacity < 4 ? 4 : grown;
    if (!self->_indexSetFlags._hasSingleRange) {
        IndexSetRangeStorage *storage = indexSetStorage(self);
        NSUInteger oldFirst = storage->firstRange;
        storage =
            __OCFSafelyReallocateTyped(storage, 16 * newCapacity + 64, kIndexSetRangeStorageTypeID, NULL);
        self->_internal._multipleRanges._data = storage;
        storage->capacity = newCapacity;
        storage = indexSetStorage(self);
        NSUInteger count = storage->rangeCount;
        storage->firstRange = (newCapacity >= count && newCapacity >= 5) ? (newCapacity - count) >> 1 : 0;
        storage = indexSetStorage(self);
        NSUInteger newFirst = storage->firstRange;
        if (oldFirst != newFirst) {
            memmove(&storage->ranges[newFirst], &storage->ranges[oldFirst], 16 * storage->rangeCount);
        }
        return;
    }
    NSUInteger value;
    NSUInteger tested;
    if (indexSetIsTagged(self)) {
        value = indexSetTaggedBitfield(self);
        tested = value;
    } else {
        value = self->_indexSetFlags._hasBitfield ? self->_internal._singleBitfield._bitfield
                                                  : self->_internal._singleRange._range.length;
        tested = self->_indexSetFlags._hasBitfield ? self->_internal._singleBitfield._bitfield
                                                   : self->_internal._singleRange._range.length;
    }
    NSUInteger hasRange = value != 0;
    NSUInteger indexCount = tested != 0 ? self->_internal._singleRange._range.length : 0;
    ONSRange saved = self->_internal._singleRange._range;
    self->_indexSetFlags._hasSingleRange = 0;
    IndexSetRangeStorage *storage = malloc_type_malloc(16 * newCapacity + 64, kIndexSetRangeStorageTypeID);
    self->_internal._multipleRanges._data = storage;
    __atomic_store_n(&storage->cacheOwner, 0, __ATOMIC_RELEASE);
    indexSetStorage(self)->capacity = newCapacity;
    indexSetStorage(self)->rangeCount = hasRange;
    indexSetStorage(self)->indexCount = indexCount;
    indexSetStorage(self)->firstRange = newCapacity < 5 ? 0 : (newCapacity - hasRange) >> 1;
    *indexSetRanges(self) = saved;
}

// Native -[NSMutableIndexSet _addRangeToArray:], 0x1c052ccb8. Non-external.
- (void)_addRangeToArray:(ONSRange)range {
    [self _insertRange:range inArrayAtIndex:[self rangeCount]];
}

// Native -[NSMutableIndexSet _insertRange:inArrayAtIndex:], 0x1c052ccf8. Non-external. A position past the
// range count raises NSInvalidArgumentException ("Index %ld out of range bounds [0...%ld]"). After making
// room for one more range, an inline set stores the range in place; storage shifts the ranges before the
// position down (when the position lies in the front half and there is room in front, or there is no room
// behind; at the end with no room behind, by half the front gap, at least 1) or those after it up (at the
// front, re-centring by half the back gap, at least 1), stores the range, releases the lookup cache with a
// store-release of 0 and updates the range and index counts.
- (void)_insertRange:(ONSRange)range inArrayAtIndex:(NSUInteger)rangeIndex {
    NSUInteger bound = indexSetRangeBound(self);
    if (rangeIndex > bound) {
        id reason = [(id<IndexSetMessages>)(id)indexSetStringClass
            stringWithFormat:(id)OF_STR("Index %ld out of range bounds [0...%ld]"), rangeIndex, bound];
        objc_exception_throw([(id<IndexSetMessages>)(id)indexSetExceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:reason
                     userInfo:nil]);
    }
    [self _ensureRangeCapacity:bound + 1];
    if (self->_indexSetFlags._hasSingleRange) {
        (&self->_internal._singleRange._range)[rangeIndex] = range;
        return;
    }
    IndexSetRangeStorage *storage = indexSetStorage(self);
    NSUInteger first = storage->firstRange;
    NSUInteger count = storage->rangeCount;
    NSUInteger capacity = storage->capacity;
    NSUInteger end = first + count;
    NSUInteger tail = count - rangeIndex;
    if ((rangeIndex < tail && first != 0) || capacity == end) {
        if (count == rangeIndex) {
            NSUInteger gap = first > 2 ? first : 2;
            ONSRange *source = &storage->ranges[first];
            memmove(source - (gap >> 1), source, 16 * rangeIndex);
            storage = indexSetStorage(self);
            storage->firstRange = storage->firstRange - (gap >> 1);
        } else {
            if (rangeIndex != 0) {
                memmove(&storage->ranges[first - 1], &storage->ranges[first], 16 * rangeIndex);
                storage = indexSetStorage(self);
            }
            storage->firstRange = storage->firstRange - 1;
        }
    } else if (rangeIndex != 0) {
        if (count != rangeIndex) {
            memmove(
                &storage->ranges[first + rangeIndex + 1], &storage->ranges[first + rangeIndex], 16 * tail
            );
        }
    } else {
        NSUInteger backGap = capacity - end;
        NSUInteger gap = backGap > 2 ? backGap : 2;
        NSUInteger shift = gap >> 1;
        memmove(&storage->ranges[first + shift], &storage->ranges[first], 16 * count);
        indexSetStorage(self)->firstRange = shift - 1;
    }
    indexSetRanges(self)[rangeIndex] = range;
    __atomic_store_n(&indexSetStorage(self)->cacheOwner, 0, __ATOMIC_RELEASE);
    indexSetStorage(self)->rangeCount = indexSetStorage(self)->rangeCount + 1;
    indexSetStorage(self)->indexCount = indexSetStorage(self)->indexCount + range.length;
}

// Native -[NSMutableIndexSet _removeRangeInArrayAtIndex:], 0x1c052cf60. Non-external. An inline set clears
// its length; storage closes the gap from the shorter side (shifting the front ranges up and advancing
// firstRange when the position lies in the front half), releases the lookup cache with a store-release of 0
// and updates the range and index counts.
//
// Foundation defect: B.HI at 0x1c052d000 accepts index == rangeCount. In heap storage this reads
// an unused range and underflows the following-range count, potentially passing a huge length to memmove.
// Reject equality only for heap storage; confidence: verified. The inline path at 0x1c052d05c ignores
// the accepted index and clears its length, including index 1 of a nonempty inline set (kept).
- (void)_removeRangeInArrayAtIndex:(NSUInteger)rangeIndex {
    NSUInteger bound = indexSetRangeBound(self);
    if (rangeIndex > bound || (rangeIndex == bound && !self->_indexSetFlags._hasSingleRange)) {
        id reason = [(id<IndexSetMessages>)(id)indexSetStringClass
            stringWithFormat:(id)OF_STR("Index %ld out of range bounds [0...%ld]"), rangeIndex, bound];
        objc_exception_throw([(id<IndexSetMessages>)(id)indexSetExceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:reason
                     userInfo:nil]);
    }
    if (self->_indexSetFlags._hasSingleRange) {
        self->_internal._singleRange._range.length = 0;
        return;
    }
    IndexSetRangeStorage *storage = indexSetStorage(self);
    ONSRange *ranges = &storage->ranges[storage->firstRange];
    ONSRange *removed = &ranges[rangeIndex];
    NSUInteger removedLength = removed->length;
    NSUInteger after = storage->rangeCount + ~rangeIndex;
    if (rangeIndex < after) {
        if (rangeIndex != 0) {
            memmove(ranges + 1, ranges, 16 * rangeIndex);
            storage = indexSetStorage(self);
        }
        storage->firstRange = storage->firstRange + 1;
    } else if (after != 0) {
        memmove(removed, removed + 1, 16 * after);
    }
    __atomic_store_n(&indexSetStorage(self)->cacheOwner, 0, __ATOMIC_RELEASE);
    indexSetStorage(self)->rangeCount = indexSetStorage(self)->rangeCount - 1;
    indexSetStorage(self)->indexCount = indexSetStorage(self)->indexCount - removedLength;
}

// Native -[NSMutableIndexSet _replaceRangeInArrayAtIndex:withRange:], 0x1c052d0ec. Non-external. An empty
// range removes the range at the position instead. Storage releases the lookup cache with a store-release
// of 0 and adjusts the index count by the length difference.
//
// Foundation defect: B.HI at 0x1c052d174 accepts index == rangeCount and writes beyond live
// heap ranges or beyond the inline instance for a nonempty inline set. Reject these unsafe equal-bound
// writes; confidence: verified. Index 0 of an empty inline set writes its valid inline range (kept).
- (void)_replaceRangeInArrayAtIndex:(NSUInteger)rangeIndex withRange:(ONSRange)range {
    NSUInteger bound = indexSetRangeBound(self);
    if (range.length == 0) {
        [self _removeRangeInArrayAtIndex:rangeIndex];
        return;
    }
    if (rangeIndex > bound ||
        (rangeIndex == bound && (!self->_indexSetFlags._hasSingleRange || bound != 0))) {
        id reason = [(id<IndexSetMessages>)(id)indexSetStringClass
            stringWithFormat:(id)OF_STR("Index %ld out of range bounds [0...%ld]"), rangeIndex, bound];
        objc_exception_throw([(id<IndexSetMessages>)(id)indexSetExceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:reason
                     userInfo:nil]);
    }
    ONSRange *replaced = &indexSetRanges(self)[rangeIndex];
    NSUInteger oldLength = replaced->length;
    *replaced = range;
    if (!self->_indexSetFlags._hasSingleRange) {
        __atomic_store_n(&indexSetStorage(self)->cacheOwner, 0, __ATOMIC_RELEASE);
        indexSetStorage(self)->indexCount = indexSetStorage(self)->indexCount - oldLength;
        indexSetStorage(self)->indexCount = indexSetStorage(self)->indexCount + range.length;
    }
}

// Native -[NSMutableIndexSet _mergeOverlappingRangesStartingAtIndex:], 0x1c052dc8c. Non-external. While the
// range at the position is not the last and reaches the next one, the next range is absorbed: when it
// extends further, the range at the position is widened and the next removed, and the position then
// advances by one (as natively, so a range overlapping two successors is merged with the first only);
// otherwise the next range is removed and the position kept.
- (void)_mergeOverlappingRangesStartingAtIndex:(NSUInteger)rangeIndex {
    for (;;) {
        NSUInteger value;
        if (indexSetIsTagged(self)) {
            value = indexSetTaggedBitfield(self);
        } else if (self->_indexSetFlags._hasBitfield) {
            value = self->_internal._singleBitfield._bitfield;
        } else if (self->_indexSetFlags._hasSingleRange) {
            value = self->_internal._singleRange._range.length;
        } else {
            value = indexSetStorage(self)->rangeCount;
        }
        if (value == 0) {
            return;
        }
        if (!self->_indexSetFlags._hasSingleRange && indexSetStorage(self)->rangeCount == 0) {
            return;
        }
        NSUInteger lastRange;
        if (indexSetIsTagged(self) || self->_indexSetFlags._hasBitfield) {
            NSUInteger bitfield = indexSetIsTagged(self) ? indexSetTaggedBitfield(self)
                                                         : self->_internal._singleBitfield._bitfield;
            lastRange = bitfield == 0 ? ~(NSUInteger)0 : 0;
            if (!self->_indexSetFlags._hasSingleRange && bitfield != 0) {
                lastRange = indexSetStorage(self)->rangeCount - 1;
            }
        } else if (self->_indexSetFlags._hasSingleRange) {
            lastRange = self->_internal._singleRange._range.length == 0 ? ~(NSUInteger)0 : 0;
        } else {
            lastRange = indexSetStorage(self)->rangeCount - 1;
        }
        if (rangeIndex >= lastRange) {
            return;
        }
        ONSRange *ranges = indexSetRanges(self);
        ONSRange current = ranges[rangeIndex];
        ONSRange *next = &ranges[rangeIndex + 1];
        NSUInteger currentEnd = current.length + current.location;
        if (currentEnd < next->location) {
            return;
        }
        NSUInteger nextEnd = next->length + next->location;
        if (currentEnd < nextEnd) {
            [self _replaceRangeInArrayAtIndex:rangeIndex
                                    withRange:(ONSRange){current.location, nextEnd - current.location}];
            [self _removeRangeInArrayAtIndex:rangeIndex + 1];
            rangeIndex = rangeIndex + 1;
        } else {
            [self _removeRangeInArrayAtIndex:rangeIndex + 1];
        }
    }
}

// Native -[NSMutableIndexSet _convertToUsingRanges], 0x1c052de64. Non-external. Turns a bitfield into
// ranges: the flags become single-range only and the inline length 0 (an empty bitfield ends there, and so
// does a mask without runs); one run is stored inline; several runs get storage for one more range than
// their count, with firstRange 0, filled run by run (the native skip counter selects the next run each time)
// while the index count accumulates, and the range count set at the end.
- (void)_convertToUsingRanges {
    NSUInteger bitfield = self->_internal._singleBitfield._bitfield;
    if (bitfield == 0) {
        self->_indexSetFlags._hasBitfield = 0;
        self->_indexSetFlags._hasSingleRange = 1;
        self->_internal._singleRange._range.length = 0;
        return;
    }
    NSUInteger clear = ~bitfield;
    NSUInteger count = _bitfieldSetRangesCount(bitfield);
    self->_indexSetFlags._hasBitfield = 0;
    self->_indexSetFlags._hasSingleRange = 1;
    self->_internal._singleRange._range.length = 0;
    if (count == 0) {
        return;
    }
    if (count == 1) {
        NSUInteger start = (NSUInteger)__builtin_ctzll(bitfield);
        self->_internal._singleRange._range.location = start;
        self->_internal._singleRange._range.length =
            indexSetTrailingZeros((~(NSUInteger)0 << start) & clear) - start;
        return;
    }
    [self _ensureRangeCapacity:count + 1];
    indexSetStorage(self)->firstRange = 0;
    NSUInteger written = 0;
    NSUInteger cursor = 0;
    NSUInteger skip = ~(NSUInteger)0;
    for (;;) {
        NSUInteger counter = (cursor != 0 ? written : 0) + skip;
        bool stored = false;
        do {
            NSUInteger members = (~(NSUInteger)0 << cursor) & bitfield;
            if (members == 0) {
                break;
            }
            NSUInteger start = (NSUInteger)__builtin_ctzll(members);
            cursor = indexSetTrailingZeros((~(NSUInteger)0 << start) & clear);
            bool reached = counter == ~(NSUInteger)0;
            counter++;
            if (reached) {
                ONSRange *slot = &indexSetRanges(self)[written++];
                slot->location = start;
                slot->length = cursor - start;
                indexSetStorage(self)->indexCount = indexSetStorage(self)->indexCount + (cursor - start);
                skip--;
                stored = true;
                break;
            }
        } while (cursor <= 63);
        if (!stored || cursor >= 64) {
            break;
        }
    }
    indexSetStorage(self)->rangeCount = count;
}

// Native -[NSMutableIndexSet addIndexesInRange:], 0x1c052dff4. Non-external. A range reaching past
// NSNotFound - 1 raises NSRangeException; an empty range does nothing. A bitfield ORs a range ending at or
// below 64 and is converted to ranges otherwise. From the range before or containing the location (0 when
// none), each range is examined: the new range is inserted before a range it ends ahead of, widens a range
// it overlaps or touches (extending it leftwards when it starts earlier), or is contained; after a widening
// or when all ranges were passed (then appended) the ranges from that position are merged.
- (void)addIndexesInRange:(ONSRange)range {
    NSUInteger location = range.location;
    NSUInteger length = range.length;
    NSUInteger end = location + length;
    if (length != 0 && (NSInteger)end < 0) {
        id proem = _ONSMethodExceptionProem(self, _cmd);
        id reason = [(id<IndexSetMessages>)(id)indexSetStringClass
            stringWithFormat:(id)OF_STR("%@: Range %@ exceeds maximum index value of NSNotFound - 1"),
                             proem,
                             ONSStringFromRange(range)];
        objc_exception_throw([(id<IndexSetMessages>)(id)indexSetExceptionClass
            exceptionWithName:(id)ONSRangeException
                       reason:reason
                     userInfo:nil]);
    }
    if (length == 0) {
        return;
    }
    if (self->_indexSetFlags._hasBitfield) {
        if (end <= 64) {
            self->_internal._singleBitfield._bitfield |= (~(NSUInteger)0 >> ((0 - length) & 63))
                                                         << (location & 63);
            return;
        }
        [self _convertToUsingRanges];
    }
    NSUInteger rangeIndex = [self _indexOfRangeBeforeOrContainingIndex:location];
    if (rangeIndex == kIndexNotFound) {
        rangeIndex = 0;
    }
    for (;;) {
        if (rangeIndex >= indexSetRangeBound(self)) {
            break;
        }
        ONSRange existing = indexSetRanges(self)[rangeIndex];
        if (end < existing.location) {
            [self _insertRange:(ONSRange){location, length} inArrayAtIndex:rangeIndex];
            return;
        }
        NSUInteger existingEnd = existing.length + existing.location;
        if (location < existing.location) {
            ONSRange widened;
            if (end > existingEnd) {
                widened = (ONSRange){location, length};
            } else {
                widened = (ONSRange){location, existingEnd - location};
                length = existingEnd - location;
            }
            [self _replaceRangeInArrayAtIndex:rangeIndex withRange:widened];
            break;
        }
        if (end <= existingEnd) {
            return;
        }
        if (existingEnd >= location) {
            [self _replaceRangeInArrayAtIndex:rangeIndex
                                    withRange:(ONSRange){existing.location, end - existing.location}];
            break;
        }
        rangeIndex++;
    }
    if (indexSetRangeBound(self) == rangeIndex) {
        [self _addRangeToArray:(ONSRange){location, length}];
    }
    [self _mergeOverlappingRangesStartingAtIndex:rangeIndex];
}

// Native -[NSMutableIndexSet addIndex:], 0x1c052e2e4. Non-external. A bitfield sets the bit of an index
// below 64 and is converted to ranges otherwise.
- (void)addIndex:(NSUInteger)value {
    if (self->_indexSetFlags._hasBitfield) {
        if (value <= 63) {
            self->_internal._singleBitfield._bitfield |= (NSUInteger)1 << value;
            return;
        }
        [self _convertToUsingRanges];
    }
    [self addIndexesInRange:(ONSRange){value, 1}];
}

@end

// Native __NSIndexSetChunkIterate, 0x1c052edb8. Non-external. One chunk of a concurrent enumeration
// (positions or indexes first through last); answers YES when the block stopped. With a passing test, a
// chunk entirely past the answer found so far (read with load-acquire; before it for reverse) is skipped.
// Range chunks push an autorelease pool and call the block with each range clipped to the window
// [start, start + length). Index chunks gather the chunk's membership with -__getContainmentVector:inRange:
// into a stack vector (nothing to do when it finds no member), push an autorelease pool and call the block
// for each member (from the end for reverse), resetting the stop byte before each call; a passing test's YES
// stores the index into the shared answer when it improves on it (compare-and-swap loop, relaxed) and ends
// the chunk without stopping the others, and a chunk past the answer ends early.
//
// Native behavior kept: the pool pushed for a chunk is popped only when the chunk ends because the block
// stopped or answered; a chunk that runs to its end (or a range chunk with last < first) returns without
// popping it, leaving the objects autoreleased during the chunk to the thread's enclosing pool.
static BOOL __NSIndexSetChunkIterate(
    ONSIndexSet *set,
    NSUInteger options,
    NSUInteger firstIndex,
    NSUInteger lastIndex,
    NSUInteger start,
    NSUInteger length,
    id block,
    NSUInteger *result
) {
    if (options & kEnumeratePassingTest) {
        NSUInteger current = __atomic_load_n(result, __ATOMIC_ACQUIRE);
        if (current != kIndexNotFound) {
            bool keep = (options & kEnumerationReverse) ? lastIndex >= current : current >= firstIndex;
            if (!keep) {
                return NO;
            }
        }
    }
    if (options & kEnumerateRanges) {
        void *pool = _OCFAutoreleasePoolPush();
        BOOL stop = NO;
        if (lastIndex < firstIndex) {
            return NO;
        }
        NSUInteger windowEnd = start + length;
        NSUInteger position = firstIndex;
        do {
            ONSRange range = [set rangeAtIndex:position];
            NSUInteger rangeEnd = range.location + range.length;
            NSUInteger clippedEnd = rangeEnd < windowEnd ? rangeEnd : windowEnd;
            NSUInteger location;
            NSUInteger clippedLength;
            if (start <= range.location && range.location < windowEnd) {
                location = range.location;
                clippedLength = clippedEnd - range.location;
            } else {
                location = start;
                clippedLength = clippedEnd - start;
                if (start >= rangeEnd) {
                    location = 0;
                    clippedLength = 0;
                }
                if (range.location > start) {
                    location = 0;
                    clippedLength = 0;
                }
            }
            if (clippedLength != 0) {
                __NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__(
                    block, location, clippedLength, (uintptr_t)&stop
                );
            }
            if (stop == 1) {
                _OCFAutoreleasePoolPop(pool);
                return YES;
            }
            position++;
        } while (position <= lastIndex);
        return NO;
    }
    NSUInteger remaining = lastIndex - firstIndex;
    NSUInteger count = remaining + 1;
    BOOL *vector = alloca((remaining + 16) & ~(NSUInteger)15);
    if ([set __getContainmentVector:vector inRange:(ONSRange){firstIndex, count}] == 0) {
        return NO;
    }
    void *pool = _OCFAutoreleasePoolPush();
    if (count == 0) {
        return NO;
    }
    NSUInteger offset = 0;
    NSUInteger backward = lastIndex;
    do {
        NSUInteger slot = (options & kEnumerationReverse) ? remaining : offset;
        if (vector[slot] == 1) {
            NSUInteger index = (options & kEnumerationReverse) ? backward : firstIndex + offset;
            BOOL stop = NO;
            if (!(options & kEnumeratePassingTest)) {
                __NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__(block, index, (uintptr_t)&stop, 0);
                if (stop) {
                    _OCFAutoreleasePoolPop(pool);
                    return YES;
                }
            } else {
                if (__NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__(block, index, (uintptr_t)&stop, 0)) {
                    for (;;) {
                        NSUInteger current = __atomic_load_n(result, __ATOMIC_ACQUIRE);
                        if (current != kIndexNotFound) {
                            bool better = (options & kEnumerationReverse) ? current < index : index < current;
                            if (!better) {
                                break;
                            }
                        }
                        NSUInteger expected = current;
                        __atomic_compare_exchange_n(
                            result, &expected, index, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED
                        );
                        if (expected == current) {
                            break;
                        }
                    }
                    _OCFAutoreleasePoolPop(pool);
                    return NO;
                }
                if (stop) {
                    _OCFAutoreleasePoolPop(pool);
                    return YES;
                }
                NSUInteger current = __atomic_load_n(result, __ATOMIC_ACQUIRE);
                if (current != kIndexNotFound) {
                    bool keep =
                        (options & kEnumerationReverse) ? lastIndex >= current : current >= firstIndex;
                    if (!keep) {
                        _OCFAutoreleasePoolPop(pool);
                        return NO;
                    }
                }
            }
        }
        offset++;
        remaining--;
        backward--;
    } while (remaining != ~(NSUInteger)0);
    return NO;
}

@implementation ONSIndexSet (NSIndexSetPortCoding)

// Native -[NSIndexSet(NSIndexSetPortCoding) replacementObjectForPortCoder:], 0x1c064e664. Non-external, in
// the port coder cluster of the image. By-reference coders get NSObject's replacement; others the set
// itself.
- (id)replacementObjectForPortCoder:(id)coder {
    if ([(id<IndexSetMessages>)coder isByref]) {
        return [super replacementObjectForPortCoder:coder];
    }
    return self;
}

@end

// Native _NSGetUnsignedInt, 0x1c0fdcbd0. Non-external, in the cold region of the image. Reads one
// little-endian base-128 varint at the cursor (bounded by end), advancing it; a byte with the high bit set
// (loaded sign-extended) keeps its low seven bits and takes the rest from the recursively read remainder
// shifted by 7 (BFI). Answers NO at the end of the bytes.
static BOOL _NSGetUnsignedInt(NSUInteger *value, const uint8_t **cursor, const uint8_t *end) {
    const uint8_t *byte = *cursor;
    if (byte >= end) {
        return NO;
    }
    *cursor = byte + 1;
    NSInteger read = (int8_t)*byte;
    if (read < 0) {
        NSUInteger rest = 0;
        if (!_NSGetUnsignedInt(&rest, cursor, end)) {
            return NO;
        }
        read = (NSInteger)(((NSUInteger)read & 0x7F) | (rest << 7));
    }
    *value = (NSUInteger)read;
    return YES;
}

//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSOrderedSetI.h"
#include "CoreFoundation/Objective-C/OCFNSArray.h"
#include "CoreFoundation/Objective-C/OCFNSDictionaryConcurrentEnumeration.h"
#include "CoreFoundation/Objective-C/OCFNSFastEnumerationEnumerator.h"
#include "CoreFoundation/Objective-C/OCFNSOrderedSetHelpers.h"
#include "CoreFoundation/Objective-C/OCFNSOrderedSetPlaceholder.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#import <objc/NSObject.h>

#include <alloca.h>
#include <mach-o/loader.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: the __NSOrderedSetI methods lie together in the iOS Simulator __text section
// (0x1c0150f98-0x1c0151fc3) with the block of -enumerateObjectsWithOptions:usingBlock: and
// __NSOrderedSetI_new (0x1c0151b5c) among them, and the tables __NSOrderedSetSizes (0x1c0321300) and
// __NSOrderedSetCapacities (0x1c0321508) and the unit's countByEnumeratingWithState:objects:count:.const_mu
// (0x1c0321500, between the two) in __TEXT,__const. Following the owner's decision, the class has this
// translation unit; the methods follow in native address order, __NSOrderedSetI_new at its native position
// inside the @implementation. Objective-C compiled with manual reference counting (objc_retain,
// objc_release). Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta11.json (family
// K-mutable-ordered-set; __NSOrderedSetI_new from iphone-simulator-corpus-delta10.json). Unless a definition
// says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified
// (assembly read in full; data read with Local/Scratch/cf-core/macho.py). iOS Device 24A446: unread.
// Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).
//
// Argument errors raise as the other bridged collection classes do: an os_log pack of the format's buffer
// size built on the stack (_os_log_pack_size, _os_log_pack_fill with errno 0 and the image's header, the
// arguments encoded as __builtin_os_log_format does), the reason made with kCFAllocatorSystemDefault and
// autoreleased through _CFAutoreleasePoolAddObject, +[NSException
// exceptionWithName:reason:userInfo:osLogPack:size:] with no user info thrown. The first format argument is
// the method's name as a literal C string, spelled with the class's selected name.

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// libobjc's objc_retain, which the native image imports; the SDK installs no declaration for it.
extern id objc_retain(id object);

// The NSException class symbol (off_2227CE300), sent to directly.
extern char exceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));

// The __NSOrderedSetI class symbol (loaded through the __SCR___NSOrderedSetI GOT slot) and the offsets of its
// ivars (from their _OBJC_IVAR_$ symbols), which __NSOrderedSetI_new reads as the native code does: _used (8:
// a 56-bit count written as a word, a halfword and a byte), _szidx (15: the top six bits of that byte),
// _stride (15: its low two bits) and _list (16: the objects in order, followed by the index table). All are
// defined by the @implementation below.
extern char orderedSetIClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSOrderedSetI));
extern const int32_t orderedSetIUsedOffset __asm("_OBJC_IVAR_$_" OF_NAME_STRING(__ONSOrderedSetI) "._used");
extern const int32_t
    orderedSetISizeIndexOffset __asm("_OBJC_IVAR_$_" OF_NAME_STRING(__ONSOrderedSetI) "._szidx");
extern const int32_t
    orderedSetIStrideOffset __asm("_OBJC_IVAR_$_" OF_NAME_STRING(__ONSOrderedSetI) "._stride");
extern const int32_t orderedSetIListOffset __asm("_OBJC_IVAR_$_" OF_NAME_STRING(__ONSOrderedSetI) "._list");

// The message this unit sends that the shared declarations do not carry; declared for the send only.
@protocol OrderedSetIMessages

+ (id)exceptionWithName:(id)name
                 reason:(id)reason
               userInfo:(id)userInfo
              osLogPack:(void *)pack
                   size:(size_t)size;

@end

#define methodName(selectorText) "-[" OF_NAME_STRING(__ONSOrderedSetI) " " selectorText "]"

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

// The objects-argument checks (buffer 0x16 each).
#define RAISE_OBJECTS_EXCEPTIONS(method, objects, count)                                                     \
    do {                                                                                                     \
        if ((objects) == NULL && (count) != 0) {                                                             \
            RAISE_LOGGED_EXCEPTION(                                                                          \
                ONSInvalidArgumentException,                                                                 \
                0x16,                                                                                        \
                "*** %s: pointer to objects array is NULL but length is %lu",                                \
                method,                                                                                      \
                (unsigned long)(count)                                                                       \
            );                                                                                               \
        }                                                                                                    \
        if (((count) >> 61) != 0) {                                                                          \
            RAISE_LOGGED_EXCEPTION(                                                                          \
                ONSInvalidArgumentException,                                                                 \
                0x16,                                                                                        \
                "*** %s: count (%lu) of objects array is ridiculous",                                        \
                method,                                                                                      \
                (unsigned long)(count)                                                                       \
            );                                                                                               \
        }                                                                                                    \
    } while (0)

// Native __NSOrderedSetSizes (0x1c0321300) and __NSOrderedSetCapacities (0x1c0321508), __TEXT,__const,
// non-external: 64 entries each (0x200 bytes, up to the next symbol), the prime slot counts of the index
// table and the capacities they serve, both ending in a zero entry (read with Local/Scratch/cf-core/macho.py;
// confidence: verified).
static const uint64_t __NSOrderedSetSizes[64] = {
    0,
    3,
    7,
    13,
    23,
    41,
    71,
    127,
    191,
    251,
    383,
    631,
    1087,
    1723,
    2803,
    4523,
    7351,
    11959,
    19447,
    31231,
    50683,
    81919,
    132607,
    214519,
    346607,
    561109,
    907759,
    1468927,
    2376191,
    3845119,
    6221311,
    10066421,
    16287743,
    26354171,
    42641881,
    68996069,
    111638519,
    180634607,
    292272623,
    472907251,
    765180413,
    1238087663,
    2003267557,
    3241355263,
    5244622819,
    8485977589,
    13730600407,
    22216578047,
    35947178479,
    58163756537,
    94110934997,
    152274691561,
    246385626107,
    398660317687,
    645045943807,
    1043706260983,
    1688752204787,
    2732458465769,
    4421210670577,
    7153669136377,
    11574879807461,
    18728548943849,
    30303428750843,
    0,
};

static const uint64_t __NSOrderedSetCapacities[64] = {
    0,
    3,
    6,
    11,
    19,
    32,
    52,
    85,
    118,
    155,
    237,
    390,
    672,
    1065,
    1732,
    2795,
    4543,
    7391,
    12019,
    19302,
    31324,
    50629,
    81956,
    132580,
    214215,
    346784,
    561026,
    907847,
    1468567,
    2376414,
    3844982,
    6221390,
    10066379,
    16287773,
    26354132,
    42641916,
    68996399,
    111638327,
    180634415,
    292272755,
    472907503,
    765180257,
    1238087439,
    2003267722,
    3241355160,
    5244622578,
    8485977737,
    13730600347,
    22216578100,
    35947178453,
    58163756541,
    94110935011,
    152274691274,
    246385626296,
    398660317578,
    645045943559,
    1043706261135,
    1688752204693,
    2732458465840,
    4421210670552,
    7153669136706,
    11574879807265,
    18728548943682,
    0,
};

@implementation __ONSOrderedSetI

// Native -[__NSOrderedSetI count], 0x1c0150f98. Non-external. The _used bit-field.
- (NSUInteger)count {
    return _used;
}

// Native -[__NSOrderedSetI indexOfObject:], 0x1c0150fbc. Non-external. nil, or a zero _szidx (the byte at 15
// below 4), answers NSNotFound before anything else. Otherwise _used, the slot count
// __NSOrderedSetSizes[_szidx] and _stride are read and the object is sent -hash; a stride code of 3 then
// answers NSNotFound. The index table follows the _used objects of _list, with entries of 4 bytes (code 2), 2
// bytes (code 1) or 1 byte (any other code). The lookup starts at -hash modulo the slot count and probes
// linearly (wrapping) at most slot-count times: a zero entry answers NSNotFound; an entry n names
// _list[n - 1], which answers n - 1 when it is the object itself or answers -isEqual: YES (bit 0) to it (the
// kept object as receiver).
// CoreFoundation defect (kept): __NSOrderedSetI_new stores the code 8 of an eight-byte table (counts of
// 0xFFFFFFFF or more) as its low two bits, 0, which this lookup reads as one-byte entries; such a set needs
// more than four billion objects, so the divergence is not reachable in ordinary use (assembly
// 0x1c015108c-0x1c01510ac; confidence: verified).
- (NSUInteger)indexOfObject:(id)object {
    if (object == nil) {
        return kNotFound;
    }
    NSUInteger sizeIndex = _szidx;
    if (sizeIndex == 0) {
        return kNotFound;
    }
    NSUInteger used = _used;
    NSUInteger size = __NSOrderedSetSizes[sizeIndex];
    unsigned int stride = _stride;
    NSUInteger hash = [object hash];
    if (stride == 3) {
        return kNotFound;
    }
    const uint8_t *table = (const uint8_t *)_list + used * sizeof(id);
    NSUInteger slot = hash % size;
    for (NSUInteger remaining = size; remaining != 0; remaining--) {
        uint64_t entry;
        if (stride == 2) {
            entry = ((const uint32_t *)table)[slot];
        } else if (stride == 1) {
            entry = ((const uint16_t *)table)[slot];
        } else {
            entry = table[slot];
        }
        if (entry == 0) {
            return kNotFound;
        }
        id existing = _list[entry - 1];
        if (existing == object || ([existing isEqual:object] & 1)) {
            return (NSUInteger)entry - 1;
        }
        slot = slot + 1 - (size > slot + 1 ? 0 : size);
    }
    return kNotFound;
}

// Native -[__NSOrderedSetI objectAtIndex:], 0x1c01510f0. Non-external. An index below _used (unsigned)
// answers the object; otherwise NSRangeException "*** %s: index %lu beyond bounds for empty ordered set"
// (buffer 0x16) for a count of 0, else "*** %s: index %lu beyond bounds [0 .. %lu]" (buffer 0x20) with the
// count minus one.
- (id)objectAtIndex:(NSUInteger)index {
    NSUInteger count = _used;
    if (index >= count) {
        if (count == 0) {
            RAISE_LOGGED_EXCEPTION(
                ONSRangeException,
                0x16,
                "*** %s: index %lu beyond bounds for empty ordered set",
                methodName("objectAtIndex:"),
                (unsigned long)index
            );
        }
        RAISE_LOGGED_EXCEPTION(
            ONSRangeException,
            0x20,
            "*** %s: index %lu beyond bounds [0 .. %lu]",
            methodName("objectAtIndex:"),
            (unsigned long)index,
            (unsigned long)count - 1
        );
    }
    return _list[index];
}

// Native +[__NSOrderedSetI automaticallyNotifiesObserversForKey:], 0x1c0151308. Non-external. Answers NO.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSOrderedSetI getObjects:range:], 0x1c0151310. Non-external. Order: -count (a message), the
// range check (location or length with bit 63 set, or past the count; "for empty ordered set", buffer 0x20,
// or "[0
// .. %lu]", buffer 0x2A), then the buffer checks (a NULL buffer with a nonzero length, a length of 2^61 or
// more); the objects are copied from _list with memmove (a tail call).
- (void)getObjects:(id *)objects range:(OCFRange)range {
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    NSUInteger count = [self count];
    if ((NSInteger)location < 0 || (NSInteger)length < 0 || count < location + length) {
        if (count == 0) {
            RAISE_LOGGED_EXCEPTION(
                ONSRangeException,
                0x20,
                "*** %s: range {%lu, %lu} extends beyond bounds for empty ordered set",
                methodName("getObjects:range:"),
                (unsigned long)location,
                (unsigned long)length
            );
        }
        RAISE_LOGGED_EXCEPTION(
            ONSRangeException,
            0x2A,
            "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",
            methodName("getObjects:range:"),
            (unsigned long)location,
            (unsigned long)length,
            (unsigned long)count - 1
        );
    }
    RAISE_OBJECTS_EXCEPTIONS(methodName("getObjects:range:"), objects, length);
    memmove(objects, &_list[location], length << 3);
}

// Native -[__NSOrderedSetI countByEnumeratingWithState:objects:count:], 0x1c0151690. Non-external. The buffer
// checks on the objects buffer and its length; then a state other than 0 answers 0, and a first call sets the
// state to -1 and the items and mutations pointers (one pair store) to _list and the unit's const_mu, and
// answers _used. The const_mu is countByEnumeratingWithState:objects:count:.const_mu at 0x1c0321500
// (__TEXT,__const, non-external; the one between this unit's two tables, confidence: medium for the
// attribution), holding 1 (read with Local/Scratch/cf-core/macho.py).
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    static const unsigned long const_mu = 1;
    RAISE_OBJECTS_EXCEPTIONS(methodName("countByEnumeratingWithState:objects:count:"), buffer, length);
    FastEnumerationState *enumerationState = (FastEnumerationState *)state;
    if (enumerationState->state != 0) {
        return 0;
    }
    enumerationState->state = (unsigned long)-1;
    enumerationState->itemsPtr = _list;
    enumerationState->mutationsPtr = (unsigned long *)&const_mu;
    return _used;
}

// Native -[__NSOrderedSetI enumerateObjectsWithOptions:usingBlock:], 0x1c0151898, with its block
// ___58-[__NSOrderedSetI enumerateObjectsWithOptions:usingBlock:]_block_invoke (0x1c0151b30, descriptor
// unk_2224297E0: the receiver, then the block; the callout with _list[index]). A nil block raises ("block
// cannot be nil", buffer 0xC). The block goes to __NSCollectionHandleConcurrentEnumerationIfSpecified with an
// autorelease pool per call and _used; when that does not handle it, the objects are walked forward (_used
// reread after every call) or, with bit 1 of the options, backward from _used read once, each object read
// before its own autorelease pool is pushed around the callout, until the stop flag byte is 1. The block
// literal's flags are the compiler's (natively 0xc0000000).
- (void)enumerateObjectsWithOptions:(NSUInteger)options
                         usingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block {
    if (block == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: block cannot be nil",
            methodName("enumerateObjectsWithOptions:usingBlock:")
        );
    }
    BOOL handled = __NSCollectionHandleConcurrentEnumerationIfSpecified(
        options, YES, _used, ^(size_t index, BOOL *stop) {
          __NSORDEREDSET_IS_CALLING_OUT_TO_A_BLOCK__(block, self->_list[index], index, stop);
        }
    );
    if (handled) {
        return;
    }
    BOOL stop = NO;
    if (!(options & 2)) {
        if (_used == 0) {
            return;
        }
        NSUInteger index = 0;
        do {
            id object = _list[index];
            void *pool = _OCFAutoreleasePoolPush();
            __NSORDEREDSET_IS_CALLING_OUT_TO_A_BLOCK__(block, object, index, &stop);
            _OCFAutoreleasePoolPop(pool);
            if (stop == YES) {
                return;
            }
            index++;
        } while (index < _used);
    } else {
        NSUInteger used = _used;
        if (used == 0) {
            return;
        }
        NSUInteger index = used - 1;
        do {
            id object = _list[index];
            void *pool = _OCFAutoreleasePoolPush();
            __NSORDEREDSET_IS_CALLING_OUT_TO_A_BLOCK__(block, object, index, &stop);
            _OCFAutoreleasePoolPop(pool);
            if (stop == YES) {
                return;
            }
            index--;
        } while (index != (NSUInteger)-1);
    }
}

// Native -[__NSOrderedSetI copyWithZone:], 0x1c0151b54. Non-external. The receiver retained (objc_retain, a
// tail call).
- (id)copyWithZone:(struct _NSZone *)zone {
    return objc_retain(self);
}

// Native +[__NSOrderedSetI allocWithZone:], 0x1c0151b58. Non-external. The immutable placeholder (a tail
// call).
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSOrderedSetImmutablePlaceholder();
}

// Native __NSOrderedSetI_new, 0x1c0151b5c. Hidden (was a private external). The index-table entry width and
// stride code follow the count: below 0xFF one byte (code 0), below 0xFFFF two (code 1), below 0xFFFFFFFF
// four (code 2), else eight (code 8, stored as its low two bits, 0). The size index is the first of the 64
// capacities (compared two at a time natively) not below the count; none traps (BRK #1), which the zero last
// entry makes the answer for a count above the largest capacity. An empty ordered set is __CFAllocateObject
// of +[__NSOrderedSetI self] with no extra bytes, a zero _used (the byte, halfword and word stores) and the
// _szidx and _stride bits cleared. Otherwise a malloc_type_calloc(1, size * width + 8 * count,
// 0x80040B8603338) buffer holds the list of kept objects followed by the index table; each object is looked
// up by open addressing from -hash modulo the slot count, probing linearly (wrapping) at most max(size, 1)
// times: a zero entry is a free slot, an entry n names list[n - 1], which is skipped when it is not the
// object itself and not -isEqual: to it (sent to the kept object) and ends the lookup otherwise (the object
// is a duplicate and dropped). A kept object is appended to the list (read again from objects), retained
// unless bit 0 of noRetain is set or it is a tagged pointer or nil, and its 1-based position is stored in the
// slot. The object is then __CFAllocateObject with the table and the list as extra bytes; _used takes the
// kept count (its byte 6, halfword 4 and word 0, in that order), _szidx the size index (above the byte's low
// two bits) and _stride the code's low two bits; the list and then the table are copied after _list (memmove)
// and the buffer is freed. An exhausted probe would select slot index size (past the table); the capacities
// keep a free slot, so that path is unreachable and kept.
id __NSOrderedSetI_new(const id *objects, NSUInteger count, BOOL noRetain) {
    size_t width;
    unsigned int code;
    if (count < 0xFF) {
        width = 1;
        code = 0;
    } else if (count < 0xFFFF) {
        width = 2;
        code = 1;
    } else if (count < 0xFFFFFFFF) {
        width = 4;
        code = 2;
    } else {
        width = 8;
        code = 8;
    }
    NSUInteger sizeIndex = 0;
    while (__NSOrderedSetCapacities[sizeIndex] < count) {
        if (++sizeIndex == 64) {
            __builtin_trap();
        }
    }
    if (count == 0) {
        id set = __CFAllocateObject([(Class)orderedSetIClass self], 0);
        uint8_t *used = (uint8_t *)set + orderedSetIUsedOffset;
        used[6] = 0;
        *(uint16_t *)(used + 4) = 0;
        *(uint32_t *)used = 0;
        uint8_t *sizeIndexByte = (uint8_t *)set + orderedSetISizeIndexOffset;
        *sizeIndexByte &= 3;
        uint8_t *strideByte = (uint8_t *)set + orderedSetIStrideOffset;
        *strideByte &= 0xFC;
        return set;
    }
    NSUInteger size = __NSOrderedSetSizes[sizeIndex];
    size_t tableSize = size * width;
    uint8_t *buffer = (uint8_t *)malloc_type_calloc(1, tableSize + count * sizeof(id), 0x80040B8603338);
    id *list = (id *)buffer;
    uint8_t *table = buffer + count * sizeof(id);
    NSUInteger tries = size > 1 ? size : 1;
    NSUInteger kept = 0;
    for (NSUInteger objectIndex = 0; objectIndex != count; objectIndex++) {
        id object = objects[objectIndex];
        NSUInteger slot = (NSUInteger)[object hash] % size;
        NSUInteger remaining = tries;
        BOOL duplicate = NO;
        while (1) {
            uint64_t entry;
            if (code == 0) {
                entry = table[slot];
            } else if (code == 1) {
                entry = ((uint16_t *)table)[slot];
            } else if (code == 2) {
                entry = ((uint32_t *)table)[slot];
            } else {
                entry = ((uint64_t *)table)[slot];
            }
            if (entry == 0) {
                break;
            }
            id existing = list[entry - 1];
            if (existing == object || [existing isEqual:object]) {
                duplicate = YES;
                break;
            }
            slot = slot + 1 - (size > slot + 1 ? 0 : size);
            if (--remaining == 0) {
                slot = size;
                break;
            }
        }
        if (duplicate) {
            continue;
        }
        id keptObject = objects[objectIndex];
        list[kept] = keptObject;
        if (!(noRetain & 1) && (intptr_t)keptObject >= 1) {
            objc_retain(keptObject);
        }
        kept++;
        if (code == 0) {
            table[slot] = (uint8_t)kept;
        } else if (code == 1) {
            ((uint16_t *)table)[slot] = (uint16_t)kept;
        } else if (code == 2) {
            ((uint32_t *)table)[slot] = (uint32_t)kept;
        } else {
            ((uint64_t *)table)[slot] = kept;
        }
    }
    id set = __CFAllocateObject([(Class)orderedSetIClass self], tableSize + kept * sizeof(id));
    uint8_t *used = (uint8_t *)set + orderedSetIUsedOffset;
    used[6] = (uint8_t)(kept >> 48);
    *(uint16_t *)(used + 4) = (uint16_t)(kept >> 32);
    *(uint32_t *)used = (uint32_t)kept;
    uint8_t *sizeIndexByte = (uint8_t *)set + orderedSetISizeIndexOffset;
    *sizeIndexByte = (uint8_t)((*sizeIndexByte & 3) | (sizeIndex << 2));
    uint8_t *strideByte = (uint8_t *)set + orderedSetIStrideOffset;
    *strideByte = (uint8_t)((*strideByte & 0xFC) | (code & 3));
    memmove((uint8_t *)set + orderedSetIListOffset, list, kept * sizeof(id));
    memmove((uint8_t *)set + orderedSetIListOffset + kept * sizeof(id), table, tableSize);
    free(buffer);
    return set;
}

// Native -[__NSOrderedSetI dealloc], 0x1c0151eec. Non-external. Each object of _list is released (nil and
// tagged pointers, at or below 0 as signed, skipped), _used reread after every release; then super.
- (void)dealloc {
    NSUInteger used = _used;
    for (NSUInteger index = 0; index < used; index++) {
        id object = _list[index];
        if ((intptr_t)object >= 1) {
            [object release];
            used = _used;
        }
    }
    [super dealloc];
}

@end

//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/CharacterSet/OCFCharacterSet.h"

#include "CoreFoundation/Base/OCFHashBytes.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/String/OCFStringFormat.h"
#include "CoreFoundation/Unicode/OCFUniChar.h"

#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <xlocale.h>

// Inferred organization: ____foundation_swift_characterSet_enabled_block_invoke (0x1c017326c) through
// _____CFCharacterSetLongCharacterMemberIMPForSet_block_invoke (0x1c017ba8c), their .cold fragments
// (0x1c0306898-0x1c03079dc) and the OUTLINED_FUNCTION fragments they share (0x1c017baf0-0x1c017bd38) are
// contiguous in the iOS Simulator __text section and are kept in one translation unit, in native order; the
// native source path is not in the evidence. .cold and OUTLINED_FUNCTION fragments are folded into the
// function that branches to them. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read; the decompiler output was used
// as a guide only); iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationCharacterSet.md (to be written from this unit's comments).

// The object (48 bytes, _CFRuntimeCreateInstance with 32 extra bytes): the hash at 16, the variant at 24 and
// 32, the annex at 40. Info bits: 0 mutable, 2 hash valid, 3 inverted, 4-6 the class (0 builtin, 1 range, 2
// string, 3 bitmap, 4 compact bitmap). Range: the first character as 32 bits at 24 (LDR W), the length at 32.
// String: the sorted UTF-16 buffer at 24 (64 units, 0x80 bytes) and its length at 32. Bitmap and compact
// bitmap: the bytes at
// 24. Builtin: the predefined-set number as 64 bits at 24. Type and field names are local descriptive
// spellings.
typedef struct {
    OCFCharacterSetRef *planes;
    uint32_t validPlanes;
    uint8_t planeCount;
    uint8_t isInverted;
} CharacterSetAnnex;

_Static_assert(sizeof(CharacterSetAnnex) == 16, "The annex is a 16-byte allocation.");

struct __OCFCharacterSet {
    OCFRuntimeBase base;
    OCFHashCode hashValue;

    union {
        struct {
            uint32_t firstChar;
            OCFIndex length;
        } range;

        struct {
            UniChar *buffer;
            OCFIndex length;
        } string;

        uint8_t *bitmap;
        OCFIndex builtinType;
    } variants;

    CharacterSetAnnex *annex;
};

_Static_assert(sizeof(struct __OCFCharacterSet) == 48, "The character set object is 48 bytes.");
_Static_assert(offsetof(struct __OCFCharacterSet, annex) == 40, "The annex pointer is at 40.");

typedef struct __OCFCharacterSet CharacterSet;

enum {
    kInfoMutable = 0x1,
    kInfoHasHash = 0x4,
    kInfoInverted = 0x8,
    kInfoClassMask = 0x70,
};

enum {
    kClassBuiltin = 0,
    kClassRange = 1,
    kClassString = 2,
    kClassBitmap = 3,
    kClassCompactBitmap = 4,
};

enum {
    kBitmapSize = 0x2000,
    kStringCapacityBytes = 0x80,
    kCharacterSetTypeID = 25,
};

// malloc type descriptors as the native call sites pass them.
static const OCFAllocatorTypeID kStringBufferType = 0x1000040BDFB0063;
static const OCFAllocatorTypeID kBitmapType = 0x100004077774924;
static const OCFAllocatorTypeID kAnnexType = 0x1080040FC6463CF;
static const OCFAllocatorTypeID kPlanesType = 0x6004044C4A2DF;
static const OCFAllocatorTypeID kInvertBitmapType = 0x100004052888210;

// __CFCSetIsEqualBitmap compares against the sentinels NULL (all clear) and (UInt32 *)-1 (all set).
#define kFullBitmap ((const uint8_t *)(uintptr_t)-1)

// dyld's SPI (libdyld, re-exported by libSystem); the native mutation checks pass 0x07E50901FFFFFFFF in X0.
extern bool dyld_program_sdk_at_least(OCFBuildVersion version);
static const OCFBuildVersion kImmutableCrashVersion = {0xFFFFFFFF, 0x07E50901};

// libobjc's respondsToSelector and retain entries (objc-internal.h), as the native code imports them.
extern BOOL objc_opt_respondsToSelector(id object, SEL selector);
extern id objc_retain(id object);

// libsystem's feature-flag SPI (os/feature_private.h).
extern bool _os_feature_enabled_impl(const char *domain, const char *feature);

// Classes outside this image. __NSCFCharacterSet is the isa of the builtin static instances. The Swift path
// messages the classes behind the out-of-image class references 0x22229c238 and 0x22229c498: no image in the
// evidence covers those slots; they are bound to NSCharacterSet and NSMutableCharacterSet from the selectors
// (+_newCharacterSetWithCharactersInString:, +_newMutableCharacterSet, +_swiftWhitespaces) and from the
// alphabetical order of the neighboring identified slots 0x22229c598 (NSProcessInfo, +processInfo) and
// 0x22229c678 (NSThread, +callStackReturnAddresses); confidence: medium for that binding.
extern char bridgedCharacterSetClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSCFCharacterSet));
extern char swiftCharacterSetClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSCharacterSet));
extern char swiftMutableCharacterSetClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableCharacterSet));

static void __CFCharacterSetDeallocate(OCFTypeRef cf);
static Boolean __CFCharacterSetEqual(OCFTypeRef cf1, OCFTypeRef cf2);
static OCFHashCode __CFCharacterSetHash(OCFTypeRef cf);
static OCFStringRef __CFCharacterSetCopyDescription(OCFTypeRef cf);
static void __CFCSetMakeCompact(CharacterSet *cset);
static OCFMutableCharacterSetRef
__CFCharacterSetCreateCopy(OCFAllocatorRef alloc, OCFCharacterSetRef theSet, Boolean isMutable);
static void __CFCSetGetBitmap(OCFCharacterSetRef cset, uint8_t *bitmap);
static Boolean __CFCSetIsEqualBitmap(const uint8_t *bits1, const uint8_t *bits2);
static void __CFCSetMakeBitmap(CharacterSet *cset);
static void __CFCSetAddNonBMPPlanesInRange(CharacterSet *cset, OCFRange range);
static void __CFCheckForExpandedSet(OCFCharacterSetRef cset);
static void __CFApplySurrogatesInString(
    OCFMutableCharacterSetRef cset,
    OCFStringRef string,
    void (*applyFunction)(OCFMutableCharacterSetRef, OCFRange)
);
static Boolean __CFCSetIsEqualAnnex(OCFCharacterSetRef cf1, OCFCharacterSetRef cf2);
static Boolean
__CFCSetIsBitmapEqualToRange(const uint8_t *bits, UniChar firstChar, UniChar lastChar, Boolean isInverted);
static uint8_t *__CFCreateCompactBitmap(OCFAllocatorRef allocator, const uint8_t *bitmap);

// Native __CFCheckForExapendedSet (sic), 0x22e6d2790 (__DATA,__bss). Non-external; set by
// __CFCharacterSetInitialize when __CF_DEBUG_EXPANDED_SET starts with 'Y'.
static uint8_t __CFCheckForExapendedSet;

// Native _foundation_swift_characterSet_enabled.onceToken and .featureEnabled, 0x22e6d2778 and 0x22e6d2780
// (__DATA,__bss), and ____foundation_swift_characterSet_enabled_block_invoke, 0x1c017326c (the block of
// ___block_literal_global_0; the dispatch_once call is OUTLINED_FUNCTION_0_3, 0x1c017baf0, reached through
// every .cold.1 fragment of this unit). Non-external; this unit's copy of the inline feature check (a second
// copy with its own statics is at 0x1c02715a4). The flag is the Foundation/SwiftNSCharacterSet feature, else
// whether the SwiftNSCharacterSet environment variable is present. Callers compare the flag with 1 (most) or
// test bit 0 (CFCharacterSetInitInlineBuffer, Union, Intersect); the stored value is 0 or 1.
static dispatch_once_t foundationSwiftCharacterSetOnceToken;
static uint8_t foundationSwiftCharacterSetFeatureEnabled;

static bool foundationSwiftCharacterSetEnabled(void) {
    dispatch_once(&foundationSwiftCharacterSetOnceToken, ^{
      uint8_t enabled;
      if (_os_feature_enabled_impl("Foundation", "SwiftNSCharacterSet")) {
          enabled = 1;
      } else {
          enabled = getenv("SwiftNSCharacterSet") != NULL;
      }
      foundationSwiftCharacterSetFeatureEnabled = enabled;
    });
    return foundationSwiftCharacterSetFeatureEnabled == 1;
}

// The native prelude of the public entry points: the Swift feature, a tagged pointer, or an Objective-C
// object of type 25 sends the message instead.
static bool characterSetIsObjC(OCFTypeRef cset) {
    if (foundationSwiftCharacterSetEnabled()) {
        return true;
    }
    if (((uintptr_t)cset >> 63) != 0) {
        return true;
    }
    return __CF_IS_OBJC(kCharacterSetTypeID, cset) != 0;
}

// Inline helpers recovered from their repeated native expansions (no native symbols).
static uint64_t loadInfo(const CharacterSet *cset) {
    return __atomic_load_n(&cset->base.info, __ATOMIC_ACQUIRE);
}

static uint32_t classOf(const CharacterSet *cset) {
    return (uint32_t)(loadInfo(cset) >> 4) & 7;
}

// One LDAR/CASAL loop: clear the given bits, then set the given bits.
static void updateInfo(CharacterSet *cset, uint64_t clearBits, uint64_t setBits) {
    uint64_t info = __atomic_load_n(&cset->base.info, __ATOMIC_ACQUIRE);
    uint64_t updated;
    do {
        updated = (info & ~clearBits) | setBits;
    } while (!__atomic_compare_exchange_n(
        &cset->base.info, &info, updated, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
    ));
}

static void setClass(CharacterSet *cset, uint32_t cls) {
    updateInfo(cset, kInfoClassMask, (uint64_t)cls << 4);
}

static void clearHashValid(CharacterSet *cset) {
    updateInfo(cset, kInfoHasHash, 0);
}

static bool isInverted(const CharacterSet *cset) {
    return (loadInfo(cset) & kInfoInverted) != 0;
}

// The empty test: no annex planes and no annex inversion, and a range or string of length 0 or a bitmap or
// compact bitmap pointer of NULL; a builtin set (and an unknown class) is never empty.
static bool isEmpty(const CharacterSet *cset) {
    const CharacterSetAnnex *annex = cset->annex;
    if (annex != NULL && (annex->validPlanes != 0 || annex->isInverted != 0)) {
        return false;
    }
    switch (classOf(cset)) {
        case kClassRange:
        case kClassString:
            return cset->variants.range.length == 0;
        case kClassBitmap:
        case kClassCompactBitmap:
            return cset->variants.bitmap == NULL;
        default:
            return false;
    }
}

static bool annexIsInverted(const CharacterSet *cset) {
    return cset->annex != NULL && cset->annex->isInverted != 0;
}

// The test every mutation and the deallocation make first: a builtin, immutable, non-inverted set that is the
// static predefined instance of its type.
static bool isStaticPredefinedSet(const CharacterSet *cset) {
    if ((loadInfo(cset) & kInfoClassMask) != 0) {
        return false;
    }
    if ((loadInfo(cset) & kInfoMutable) != 0) {
        return false;
    }
    if ((loadInfo(cset) & kInfoInverted) != 0) {
        return false;
    }
    return OCFCharacterSetGetPredefined((OCFCharacterSetPredefinedSet)cset->variants.builtinType) == cset;
}

// The mutation guard: an immutable set logs, and crashes for programs linked on or after the 2023 releases
// (the .cold.3 fragments: OUTLINED_FUNCTION_28 stores the message, then BRK #1).
static void checkMutable(const CharacterSet *cset, const char *function) {
    if ((loadInfo(cset) & kInfoMutable) == 0) {
        OCFLog(3, OF_STR("%s: Immutable character set passed to mutable function"), function);
        if (dyld_program_sdk_at_least(kImmutableCrashVersion)) {
            crashWithMessage("Immutable character set passed to mutable function");
        }
    }
}

// A new annex with no planes (the STR XZR [annex] / STUR XZR [annex+6] pair).
static CharacterSetAnnex *annexAllocateEmpty(CharacterSet *cset) {
    CharacterSetAnnex *annex =
        (CharacterSetAnnex *)OCFAllocatorAllocateTyped(OCFGetAllocator(cset), 16, kAnnexType, 0);
    cset->annex = annex;
    annex->planes = NULL;
    annex->validPlanes = 0;
    annex->planeCount = 0;
    annex->isInverted = 0;
    return annex;
}

// The plane set if the annex holds one: its valid bit, a plane count of at least the plane, and a non-NULL
// slot.
static OCFCharacterSetRef annexGetPlane(const CharacterSet *cset, uint8_t plane) {
    const CharacterSetAnnex *annex = cset->annex;
    if (annex == NULL || ((annex->validPlanes >> (plane & 31)) & 1) == 0 || annex->planeCount < plane) {
        return NULL;
    }
    return annex->planes[plane - 1];
}

// Grows the annex (creating it if needed) to hold the plane: a new annex records the plane count, clears the
// inversion and the valid bits and allocates plane slots; a short annex records the count and reallocates its
// slots (the allocator is fetched before the slot pointer is tested).
static void annexEnsurePlaneCapacity(CharacterSet *cset, uint8_t plane) {
    CharacterSetAnnex *annex = cset->annex;
    if (annex == NULL) {
        annex = (CharacterSetAnnex *)OCFAllocatorAllocateTyped(OCFGetAllocator(cset), 16, kAnnexType, 0);
        cset->annex = annex;
        annex->planeCount = plane;
        annex->isInverted = 0;
        annex->validPlanes = 0;
        void *planes = OCFAllocatorAllocateTyped(OCFGetAllocator(cset), (OCFIndex)plane * 8, kPlanesType, 0);
        cset->annex->planes = (OCFCharacterSetRef *)planes;
        return;
    }
    if (annex->planeCount < plane) {
        annex->planeCount = plane;
        OCFCharacterSetRef *previous = annex->planes;
        OCFAllocatorRef allocator = OCFGetAllocator(cset);
        void *planes;
        if (previous != NULL) {
            // The type descriptor travels in the fourth argument register and 0 in the fifth, as at every
            // native call site of __CFSafelyReallocateWithAllocatorTyped.
            planes = __OCFSafelyReallocateWithAllocatorTyped(
                allocator, cset->annex->planes, (OCFIndex)plane * 8, kPlanesType, 0, NULL
            );
        } else {
            planes = OCFAllocatorAllocateTyped(allocator, (OCFIndex)plane * 8, kPlanesType, 0);
        }
        cset->annex->planes = (OCFCharacterSetRef *)planes;
    }
}

// The plane set, created empty and mutable (with the owner's allocator) when the valid bit is clear.
static CharacterSet *annexGetOrCreatePlane(CharacterSet *cset, uint8_t plane) {
    annexEnsurePlaneCapacity(cset, plane);
    uint32_t bit = 1u << plane;
    if ((cset->annex->validPlanes & bit) == 0) {
        OCFMutableCharacterSetRef created = OCFCharacterSetCreateMutable(OCFGetAllocator(cset));
        CharacterSetAnnex *annex = cset->annex;
        annex->planes[plane - 1] = created;
        annex->validPlanes |= bit;
    }
    return (CharacterSet *)cset->annex->planes[plane - 1];
}

// Stores a plane set (retained) or clears the plane, releasing the previous set.
static void annexPutPlane(CharacterSet *cset, uint8_t plane, OCFCharacterSetRef planeSet) {
    annexEnsurePlaneCapacity(cset, plane);
    uint32_t bit = 1u << plane;
    if ((cset->annex->validPlanes & bit) != 0) {
        OCFRelease(cset->annex->planes[plane - 1]);
    }
    if (planeSet != NULL) {
        OCFTypeRef retained = OCFRetain(planeSet);
        CharacterSetAnnex *annex = cset->annex;
        annex->planes[plane - 1] = (OCFCharacterSetRef)retained;
        annex->validPlanes |= bit;
    } else {
        cset->annex->validPlanes &= ~bit;
    }
}

// Releases the valid planes (bits 1-16), the slot array (even when NULL) and the annex.
static void annexDeallocate(CharacterSet *cset) {
    for (uint32_t index = 0; index != 16; ++index) {
        CharacterSetAnnex *annex = cset->annex;
        if (((annex->validPlanes >> index) & 2) != 0) {
            OCFRelease(annex->planes[index]);
        }
    }
    OCFAllocatorDeallocate(OCFGetAllocator(cset), cset->annex->planes);
    OCFAllocatorDeallocate(OCFGetAllocator(cset), cset->annex);
    cset->annex = NULL;
}

// The bitmap range operations, on a 0x2000-byte plane bitmap. Adding special-cases a single character;
// removing does not.
static void bitmapAddCharactersInRange(uint8_t *bitmap, UniChar firstChar, UniChar lastChar) {
    if (firstChar == lastChar) {
        bitmap[firstChar >> 3] |= (uint8_t)(1u << (firstChar & 7));
        return;
    }
    uint32_t firstIndex = firstChar >> 3;
    uint32_t lastIndex = lastChar >> 3;
    uint32_t firstMask = 0xFFu << (firstChar & 7);
    uint32_t lastMask = 0xFFu >> (7 & ~(uint32_t)lastChar);
    if (firstIndex == lastIndex) {
        bitmap[firstIndex] |= (uint8_t)(lastMask & firstMask);
        return;
    }
    bitmap[firstIndex] |= (uint8_t)firstMask;
    bitmap[lastIndex] |= (uint8_t)lastMask;
    if (firstIndex + 1 < lastIndex) {
        memset(bitmap + firstIndex + 1, 0xFF, (size_t)(uint32_t)(lastIndex - firstIndex - 2) + 1);
    }
}

static void bitmapRemoveCharactersInRange(uint8_t *bitmap, UniChar firstChar, UniChar lastChar) {
    uint32_t firstIndex = firstChar >> 3;
    uint32_t lastIndex = lastChar >> 3;
    uint32_t firstMask = 0xFFu << (firstChar & 7);
    if (firstIndex == lastIndex) {
        bitmap[firstIndex] &= (uint8_t)~((0xFFu >> (7 & ~(uint32_t)lastChar)) & firstMask);
        return;
    }
    bitmap[firstIndex] &= (uint8_t)~firstMask;
    bitmap[lastIndex] &= (uint8_t)~(0xFFu >> (7 & ~(uint32_t)lastChar));
    if (firstIndex + 1 < lastIndex) {
        memset(bitmap + firstIndex + 1, 0, (size_t)(uint32_t)(lastIndex - firstIndex - 2) + 1);
    }
}

static uint32_t loadWord(const uint8_t *bytes, uint32_t index) {
    uint32_t word;
    memcpy(&word, bytes + (size_t)index * 4, 4);
    return word;
}

static void storeWord(uint8_t *bytes, uint32_t index, uint32_t word) {
    memcpy(bytes + (size_t)index * 4, &word, 4);
}

// Whether two bitmaps are each other's complement (an inlined comparison loop; EOR, CMN #1).
static bool bitmapsAreInverse(const uint8_t *bits1, const uint8_t *bits2) {
    for (uint32_t index = 0; index != kBitmapSize / 4; ++index) {
        if ((loadWord(bits2, index) ^ loadWord(bits1, index)) != 0xFFFFFFFF) {
            return false;
        }
    }
    return true;
}

// The SDK's CFStringInlineBuffer accessors (CFString.h interface; the record is OCFStringInlineBuffer,
// String/OCFString.h), as this unit expands them: 64 buffered units, a four-unit look-behind, the ASCII
// C-string pointer read as signed char. They differ from the string family's stringInitInlineBuffer in one
// respect, so this unit keeps its own copies under local names: the initialization clears the 128-byte
// character buffer first (memset) before setting the other fields.
static void
characterSetInitStringInlineBuffer(OCFStringRef string, OCFStringInlineBuffer *buffer, OCFRange range) {
    memset(buffer->buffer, 0, sizeof(buffer->buffer));
    buffer->theString = string;
    buffer->rangeToBuffer = range;
    buffer->directUniCharBuffer = OCFStringGetCharactersPtr(string);
    buffer->directCStringBuffer =
        buffer->directUniCharBuffer != NULL ? NULL : OCFStringGetCStringPtr(string, 0x600);
    buffer->bufferedRangeStart = 0;
    buffer->bufferedRangeEnd = 0;
}

static UniChar characterSetGetCharacterFromStringInlineBuffer(OCFStringInlineBuffer *buffer, OCFIndex index) {
    if (index < 0 || index >= buffer->rangeToBuffer.length) {
        return 0;
    }
    if (buffer->directUniCharBuffer != NULL) {
        return buffer->directUniCharBuffer[index + buffer->rangeToBuffer.location];
    }
    if (buffer->directCStringBuffer != NULL) {
        return (UniChar)buffer->directCStringBuffer[index + buffer->rangeToBuffer.location];
    }
    if (index >= buffer->bufferedRangeEnd || index < buffer->bufferedRangeStart) {
        buffer->bufferedRangeStart = index - 4;
        if (buffer->bufferedRangeStart < 0) {
            buffer->bufferedRangeStart = 0;
        }
        buffer->bufferedRangeEnd = buffer->bufferedRangeStart + 64;
        if (buffer->bufferedRangeEnd > buffer->rangeToBuffer.length) {
            buffer->bufferedRangeEnd = buffer->rangeToBuffer.length;
        }
        OCFRange fill = {
            buffer->rangeToBuffer.location + buffer->bufferedRangeStart,
            buffer->bufferedRangeEnd - buffer->bufferedRangeStart
        };
        OCFStringGetCharacters(buffer->theString, fill, buffer->buffer);
    }
    return buffer->buffer[index - buffer->bufferedRangeStart];
}

// Sorts a string set's buffer and removes duplicates in place (the qsort/chcompar and compaction sequence of
// the string paths); returns the new length.
static OCFIndex sortAndUniqueString(UniChar *buffer, OCFIndex length) {
    qsort(buffer, (size_t)length, 2, chcompar);
    if (length < 2) {
        return length;
    }
    OCFIndex last = 0;
    for (OCFIndex index = 1; index < length; ++index) {
        UniChar character = buffer[index];
        if (buffer[last] != character) {
            ++last;
            buffer[last] = character;
        }
    }
    return last + 1;
}

// Native __CFCharacterSetClass, 0x22279d088 (__DATA_CONST,__const). Hidden (was private external); version 0,
// name "CFCharacterSet", finalize, equal, hash and the debug description.
const OCFRuntimeClass __CFCharacterSetClass = {
    .version = 0,
    .className = "CFCharacterSet",
    .init = NULL,
    .copy = NULL,
    .finalize = __CFCharacterSetDeallocate,
    .equal = __CFCharacterSetEqual,
    .hash = __CFCharacterSetHash,
    .copyFormattingDesc = NULL,
    .copyDebugDesc = __CFCharacterSetCopyDescription,
};

// Native __CFBuiltinSets, 0x22279d108 (__DATA_CONST,__const, 15 x 48 bytes). Non-external. Static instances:
// isa __NSCFCharacterSet, info 0x1984 (type 25, static, hash valid, builtin class), the precomputed hash, the
// predefined-set number, no annex.
#define BuiltinSet(hash, type)                                                                               \
    {.base = {(uintptr_t)bridgedCharacterSetClass, 0x1984},                                                  \
     .hashValue = hash,                                                                                      \
     .variants = {.builtinType = type},                                                                      \
     .annex = NULL}

static const CharacterSet __CFBuiltinSets[15] = {
    BuiltinSet(0xc4abae, 1),
    BuiltinSet(0x21fef00, 2),
    BuiltinSet(0xe1eef30, 3),
    BuiltinSet(0x1f3f1f0, 4),
    BuiltinSet(0x5784e30, 5),
    BuiltinSet(0xfdc1360, 6),
    BuiltinSet(0x9929bc0, 7),
    BuiltinSet(0xe245890, 8),
    BuiltinSet(0x92f3200, 9),
    BuiltinSet(0xbd67560, 10),
    BuiltinSet(0x989b3e0, 11),
    BuiltinSet(0xf2fad51, 12),
    BuiltinSet(0xdb6ffe0, 13),
    BuiltinSet(0x7c8fe0, 14),
    BuiltinSet(0xc010030, 15),
};

// Native __CFCharacterSetIsMutable, 0x1c01732b8 (corpus delta capture 7). Hidden (was private external).
// Info bit 0 (acquire load), with no type check.
Boolean __CFCharacterSetIsMutable(OCFCharacterSetRef theSet) {
    return (Boolean)(loadInfo(theSet) & kInfoMutable);
}

// Native _CFCharacterSetIsValidRange, 0x1c01732c8. External. iOS Device divergence: unread.
// The location and sum comparisons are unsigned. The length comparison is signed.
Boolean _OCFCharacterSetIsValidRange(OCFIndex location, OCFIndex length) {
    return (uint64_t)location >> 16 <= 0x10 && length < 0x110001 &&
           (uint64_t)location + (uint64_t)length < 0x110001;
}

// Native __CFCharacterSetDeallocate, 0x1c01732f0. Non-external (the class finalize). The allocator is fetched
// first; a static predefined set is left alone; a string, bitmap or compact bitmap frees its non-NULL buffer;
// the annex releases its planes and is freed.
static void __CFCharacterSetDeallocate(OCFTypeRef cf) {
    CharacterSet *cset = (CharacterSet *)cf;
    OCFAllocatorRef allocator = OCFGetAllocator(cset);
    if (isStaticPredefinedSet(cset)) {
        return;
    }
    if (((loadInfo(cset) & kInfoClassMask) == 0x20 && cset->variants.string.buffer != NULL) ||
        ((loadInfo(cset) & kInfoClassMask) == 0x30 && cset->variants.bitmap != NULL) ||
        ((loadInfo(cset) & kInfoClassMask) == 0x40 && cset->variants.bitmap != NULL)) {
        OCFAllocatorDeallocate(allocator, cset->variants.bitmap);
    }
    if (cset->annex != NULL) {
        annexDeallocate(cset);
    }
}

// Native __CFCharacterSetEqual, 0x1c0173400. Non-external (the class equal). Two 0x2000-byte stack bitmaps.
// Range-against-bitmap comparisons check planes 1-15 only (the loops end at 15). CoreFoundation defect
// (kept): when one set is empty and the other is a bitmap set with an inverted annex, each plane compares the
// never-written stack bitmap with NULL (the native code reads the buffer it filled only for other classes),
// so the result depends on stack contents; the defect is not memory-unsafe and is reproduced.
static Boolean __CFCharacterSetEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    CharacterSet *set1 = (CharacterSet *)cf1;
    CharacterSet *set2 = (CharacterSet *)cf2;
    uint8_t bitsBuffer[kBitmapSize];
    uint8_t otherBitsBuffer[kBitmapSize];
    uint64_t info1 = loadInfo(set1);
    uint64_t info2 = loadInfo(set2);
    if ((loadInfo(set1) & kInfoHasHash) != 0 && (loadInfo(set2) & kInfoHasHash) != 0 &&
        set1->hashValue != set2->hashValue) {
        return false;
    }
    uint64_t differ = info2 ^ info1;
    if (isEmpty(set1) && isEmpty(set2) && (differ & kInfoInverted) != 0) {
        return false;
    }
    if (((loadInfo(set1) ^ loadInfo(set2)) & kInfoClassMask) == 0 &&
        (loadInfo(set1) & kInfoClassMask) != 0x40) {
        switch (classOf(set1)) {
            case kClassBuiltin:
                return (differ & kInfoInverted) == 0 &&
                       set1->variants.builtinType == set2->variants.builtinType;
            case kClassRange:
                if (set1->variants.range.firstChar != set2->variants.range.firstChar) {
                    return false;
                }
                return (differ & kInfoInverted) == 0 &&
                       set1->variants.range.length == set2->variants.range.length;
            case kClassString: {
                if ((differ & kInfoInverted) != 0) {
                    return false;
                }
                OCFIndex length = set1->variants.string.length;
                if (set2->variants.string.length != length ||
                    memcmp(
                        set1->variants.string.buffer, set2->variants.string.buffer, (size_t)(2 * length)
                    ) != 0) {
                    return false;
                }
                break;
            }
            case kClassBitmap:
                if (!__CFCSetIsEqualBitmap(set1->variants.bitmap, set2->variants.bitmap)) {
                    return false;
                }
                break;
            default:
                break;
        }
        return __CFCSetIsEqualAnnex(set1, set2);
    }
    if (!isEmpty(set1) && !isEmpty(set2)) {
        if ((loadInfo(set1) & kInfoClassMask) != 0 && (loadInfo(set2) & kInfoClassMask) != 0) {
            if ((loadInfo(set1) & kInfoClassMask) == 0x10 || (loadInfo(set2) & kInfoClassMask) == 0x10) {
                CharacterSet *rangeSet = (loadInfo(set1) & kInfoClassMask) == 0x10 ? set1 : set2;
                CharacterSet *otherSet = rangeSet == set1 ? set2 : set1;
                uint32_t firstChar = rangeSet->variants.range.firstChar;
                uint32_t endChar = firstChar + (uint32_t)rangeSet->variants.range.length;
                uint64_t rangeInfo = loadInfo(rangeSet);
                const uint8_t *bits;
                if ((loadInfo(otherSet) & kInfoClassMask) == 0x30) {
                    bits = otherSet->variants.bitmap;
                } else {
                    bits = bitsBuffer;
                    __CFCSetGetBitmap(otherSet, bitsBuffer);
                }
                Boolean rangeInverted = (Boolean)((rangeInfo >> 3) & 1);
                uint32_t lastChar = endChar - 1;
                UniChar firstLow;
                uint32_t firstPlane;
                if ((firstChar & 0xFF0000) != 0) {
                    if (!__CFCSetIsEqualBitmap(bits, rangeInverted ? kFullBitmap : NULL)) {
                        return false;
                    }
                    firstLow = (UniChar)firstChar;
                    firstPlane = (firstChar >> 16) & 0xFF;
                } else {
                    uint32_t bmpLast = (lastChar & 0xFF0000) != 0 ? 0xFFFFFFFF : lastChar;
                    if (!__CFCSetIsBitmapEqualToRange(
                            bits, (UniChar)firstChar, (UniChar)bmpLast, rangeInverted
                        )) {
                        return false;
                    }
                    firstLow = 0;
                    firstPlane = 1;
                }
                UniChar lastLow = (UniChar)lastChar;
                uint32_t otherAnnexInverted = annexIsInverted(otherSet) ? 1 : 0;
                uint32_t planeState = otherAnnexInverted ^ ((uint32_t)(rangeInfo & 0xF8) >> 3);
                Boolean planeInverted = (Boolean)(planeState & 1);
                const uint8_t *outsideFill = (planeState & 1) != 0 ? kFullBitmap : NULL;
                const uint8_t *insideFill = (planeState & 1) != 0 ? NULL : kFullBitmap;
                uint32_t lastPlane = (lastChar >> 16) & 0xFF;
                uint32_t firstPlaneLast = firstPlane == lastPlane ? lastChar : 0xFFFFFFFF;
                uint32_t firstPlaneMismatch = (planeState ^ 1) | (firstLow != 0 ? 1 : 0) |
                                              (lastLow != 0xFFFF && firstPlane == lastPlane ? 1 : 0);
                uint32_t lastPlaneMatch = planeState & (lastLow == 0xFFFF ? 1 : 0);
                for (uint32_t plane = 1; plane != 16; ++plane) {
                    OCFCharacterSetRef planeSet = annexGetPlane(otherSet, (uint8_t)plane);
                    if (planeSet != NULL) {
                        const uint8_t *planeBits;
                        if ((loadInfo(planeSet) & kInfoClassMask) == 0x30) {
                            planeBits = planeSet->variants.bitmap;
                        } else {
                            planeBits = bitsBuffer;
                            __CFCSetGetBitmap(planeSet, bitsBuffer);
                        }
                        if (plane < firstPlane || plane > lastPlane) {
                            if (!__CFCSetIsEqualBitmap(planeBits, outsideFill)) {
                                return false;
                            }
                        } else if (plane > firstPlane && plane < lastPlane) {
                            if (!__CFCSetIsEqualBitmap(planeBits, insideFill)) {
                                return false;
                            }
                        } else if (plane == firstPlane) {
                            if (!__CFCSetIsBitmapEqualToRange(
                                    planeBits, firstLow, (UniChar)firstPlaneLast, planeInverted
                                )) {
                                return false;
                            }
                        } else if (plane == lastPlane) {
                            if (!__CFCSetIsBitmapEqualToRange(planeBits, 0, lastLow, planeInverted)) {
                                return false;
                            }
                        }
                    } else if (plane < firstPlane || plane > lastPlane) {
                        if ((planeState & 1) != 0) {
                            return false;
                        }
                    } else if (plane <= firstPlane || plane >= lastPlane) {
                        if (plane == firstPlane) {
                            if ((firstPlaneMismatch & 1) != 0) {
                                return false;
                            }
                        } else if (plane == lastPlane && (lastPlaneMatch & 1) == 0) {
                            return false;
                        }
                    } else if ((planeState & 1) == 0) {
                        return false;
                    }
                }
                return true;
            }
            uint64_t class1 = loadInfo(set1) & kInfoClassMask;
            uint64_t class2 = loadInfo(set2) & kInfoClassMask;
            const uint8_t *bits1;
            const uint8_t *bits2;
            if (class1 == 0x30 && class2 == 0x30) {
                bits1 = set1->variants.bitmap;
                bits2 = set2->variants.bitmap;
            } else if (class1 == 0x30 || class2 == 0x30) {
                CharacterSet *nonBitmapSet = class2 == 0x30 ? set1 : set2;
                CharacterSet *bitmapSet = class2 == 0x30 ? set2 : set1;
                __CFCSetGetBitmap(nonBitmapSet, bitsBuffer);
                if (!__CFCSetIsEqualBitmap(bitmapSet->variants.bitmap, bitsBuffer)) {
                    return false;
                }
                return __CFCSetIsEqualAnnex(bitmapSet, nonBitmapSet);
            } else {
                __CFCSetGetBitmap(set1, bitsBuffer);
                __CFCSetGetBitmap(set2, otherBitsBuffer);
                bits1 = bitsBuffer;
                bits2 = otherBitsBuffer;
            }
            if (!__CFCSetIsEqualBitmap(bits1, bits2)) {
                return false;
            }
            return __CFCSetIsEqualAnnex(set1, set2);
        }
        CharacterSet *builtinSet = (loadInfo(set1) & kInfoClassMask) != 0 ? set2 : set1;
        CharacterSet *otherSet = builtinSet == set1 ? set2 : set1;
        if ((loadInfo(otherSet) & kInfoClassMask) == 0x10) {
            uint32_t firstChar = otherSet->variants.range.firstChar;
            uint32_t lastChar = firstChar + (uint32_t)otherSet->variants.range.length - 1;
            uint32_t firstPlane = (firstChar >> 16) & 0xFF;
            uint32_t lastPlane = (lastChar >> 16) & 0xFF;
            bool samePlane = firstPlane == lastPlane;
            uint32_t lastPlaneFirst = samePlane ? firstChar : 0;
            uint32_t firstPlaneLast = samePlane ? lastChar : 0xFFFFFFFF;
            bool lastPlanePartial = (UniChar)lastChar != 0xFFFF || (samePlane && (UniChar)firstChar != 0);
            bool firstPlanePartial = (UniChar)firstChar != 0 || (samePlane && (UniChar)lastChar != 0xFFFF);
            for (uint32_t plane = 0; plane != 16; ++plane) {
                uint8_t result = CFUniCharGetBitmapForPlane(
                    (uint32_t)builtinSet->variants.builtinType,
                    plane,
                    bitsBuffer,
                    (differ & kInfoInverted) == 0
                );
                if (plane < firstPlane || plane > lastPlane) {
                    if (result != 0) {
                        if (result == 1) {
                            return false;
                        }
                    } else if (!__CFCSetIsEqualBitmap(bitsBuffer, NULL)) {
                        return false;
                    }
                } else if (plane <= firstPlane || plane >= lastPlane) {
                    if (result == 1) {
                        if (plane == firstPlane) {
                            if (firstPlanePartial) {
                                return false;
                            }
                        } else if (lastPlanePartial) {
                            return false;
                        }
                    } else {
                        if (result == 0xFF) {
                            return false;
                        }
                        UniChar rangeFirst;
                        UniChar rangeLast;
                        if (plane == firstPlane) {
                            rangeFirst = (UniChar)firstChar;
                            rangeLast = (UniChar)firstPlaneLast;
                        } else {
                            rangeFirst = (UniChar)lastPlaneFirst;
                            rangeLast = (UniChar)lastChar;
                        }
                        if (!__CFCSetIsBitmapEqualToRange(bitsBuffer, rangeFirst, rangeLast, false)) {
                            return false;
                        }
                    }
                } else if (result != 0) {
                    if (result == 0xFF) {
                        return false;
                    }
                } else if (!__CFCSetIsEqualBitmap(bitsBuffer, kFullBitmap)) {
                    return false;
                }
            }
            return true;
        }
        uint32_t builtinType = (uint32_t)builtinSet->variants.builtinType;
        uint8_t result = CFUniCharGetBitmapForPlane(builtinType, 0, bitsBuffer, isInverted(builtinSet));
        const uint8_t *compared1;
        const uint8_t *compared2;
        if (result != 0) {
            if ((loadInfo(otherSet) & kInfoClassMask) == 0x30) {
                compared2 = otherSet->variants.bitmap;
            } else {
                __CFCSetGetBitmap(otherSet, bitsBuffer);
                compared2 = bitsBuffer;
            }
            compared1 = result == 1 ? kFullBitmap : NULL;
        } else if ((loadInfo(otherSet) & kInfoClassMask) == 0x30) {
            compared2 = otherSet->variants.bitmap;
            compared1 = bitsBuffer;
        } else {
            __CFCSetGetBitmap(otherSet, otherBitsBuffer);
            compared1 = bitsBuffer;
            compared2 = otherBitsBuffer;
        }
        if (!__CFCSetIsEqualBitmap(compared1, compared2)) {
            return false;
        }
        bool builtinInverted = isInverted(builtinSet);
        bool otherAnnexInverted = annexIsInverted(otherSet);
        for (uint32_t plane = 1; plane != 16; ++plane) {
            uint8_t planeResult = CFUniCharGetBitmapForPlane(
                builtinType, plane, bitsBuffer, builtinInverted ^ otherAnnexInverted
            );
            OCFCharacterSetRef planeSet = annexGetPlane(otherSet, (uint8_t)plane);
            if (planeResult == 0) {
                if (planeSet == NULL) {
                    return false;
                }
                const uint8_t *planeBits;
                if ((loadInfo(planeSet) & kInfoClassMask) == 0x30) {
                    planeBits = planeSet->variants.bitmap;
                } else {
                    __CFCSetGetBitmap(planeSet, otherBitsBuffer);
                    planeBits = otherBitsBuffer;
                }
                if (!__CFCSetIsEqualBitmap(bitsBuffer, planeBits)) {
                    return false;
                }
            } else if (planeSet != NULL) {
                const uint8_t *planeBits;
                if ((loadInfo(planeSet) & kInfoClassMask) == 0x30) {
                    planeBits = planeSet->variants.bitmap;
                } else {
                    __CFCSetGetBitmap(planeSet, bitsBuffer);
                    planeBits = bitsBuffer;
                }
                if (!__CFCSetIsEqualBitmap(planeResult == 1 ? kFullBitmap : NULL, planeBits)) {
                    return false;
                }
            } else if (planeResult == 1) {
                return false;
            }
        }
        return true;
    }
    bool firstIsEmpty = isEmpty(set1);
    if (set2 == set1) {
        firstIsEmpty = true;
    }
    CharacterSet *cset = firstIsEmpty ? set2 : set1;
    if ((loadInfo(cset) & kInfoClassMask) == 0) {
        return false;
    }
    if ((loadInfo(cset) & kInfoClassMask) == 0x10) {
        OCFIndex length = cset->variants.range.length;
        if ((differ & kInfoInverted) != 0) {
            return length == 0x110000;
        }
        return length == 0;
    }
    CharacterSetAnnex *annex = cset->annex;
    if (annex != NULL) {
        if (annex->isInverted != 0) {
            if (annex->validPlanes != 0x1FFFE) {
                return false;
            }
        } else if (annex->validPlanes != 0) {
            return false;
        }
    }
    const uint8_t *bits;
    bool hasBits = true;
    if ((loadInfo(cset) & kInfoClassMask) == 0x30) {
        bits = cset->variants.bitmap;
        hasBits = bits != NULL;
    } else {
        bits = bitsBuffer;
        __CFCSetGetBitmap(cset, bitsBuffer);
    }
    if (hasBits && !__CFCSetIsEqualBitmap(bits, NULL)) {
        return false;
    }
    if (cset->annex == NULL || cset->annex->isInverted == 0) {
        return true;
    }
    for (uint32_t plane = 1; plane != 16; ++plane) {
        if ((loadInfo(cset) & kInfoClassMask) == 0x30) {
            const uint8_t *fill = annexIsInverted(cset) ? NULL : kFullBitmap;
            if (!__CFCSetIsEqualBitmap(fill, bitsBuffer)) {
                return false;
            }
        } else {
            __CFCSetGetBitmap(annexGetPlane(cset, (uint8_t)plane), bitsBuffer);
            if (!__CFCSetIsEqualBitmap(bitsBuffer, kFullBitmap)) {
                return false;
            }
        }
    }
    return true;
}

// Native __CFCharacterSetHash, 0x1c0174054. Non-external (the class hash). A valid hash is returned; an empty
// set hashes to 0, or 0xFFFFFFFF when inverted; otherwise CFHashBytes over the bitmap (the stored bitmap, or
// the generated one in a 0x2000-byte stack buffer); the hash is stored and marked valid, then reloaded.
static OCFHashCode __CFCharacterSetHash(OCFTypeRef cf) {
    CharacterSet *cset = (CharacterSet *)cf;
    uint8_t bitsBuffer[kBitmapSize];
    if ((loadInfo(cset) & kInfoHasHash) == 0) {
        if (isEmpty(cset)) {
            cset->hashValue = (uint32_t)(-(int64_t)((loadInfo(cset) >> 3) & 1));
        } else if ((loadInfo(cset) & kInfoClassMask) == 0x30) {
            cset->hashValue = OCFHashBytes(cset->variants.bitmap, kBitmapSize);
        } else {
            __CFCSetGetBitmap(cset, bitsBuffer);
            cset->hashValue = OCFHashBytes(bitsBuffer, kBitmapSize);
        }
        updateInfo(cset, 0, kInfoHasHash);
    }
    return cset->hashValue;
}

// Native __CFCharacterSetCopyDescription, 0x1c017418c. Non-external (the class debug description). Empty sets
// describe as All or Empty by the inverted bit; builtin sets by name (an unknown number answers NULL); a
// range as "<CFCharacterSet Range(%u, %ld)>"; a string set lists its items into a mutable string of capacity
// prefix length - n + 8n + 2; bitmap and compact bitmap sets as "<CFCharacterSet Bitmap>"; other classes
// NULL.
static OCFStringRef __CFCharacterSetCopyDescription(OCFTypeRef cf) {
    CharacterSet *cset = (CharacterSet *)cf;
    if (isEmpty(cset)) {
        return (OCFStringRef)OCFRetain(
            isInverted(cset) ? OF_STR("<CFCharacterSet All>") : OF_STR("<CFCharacterSet Empty>")
        );
    }
    switch (classOf(cset)) {
        case kClassBuiltin: {
            OCFStringRef description;
            switch (cset->variants.builtinType) {
                case 1:
                    description = OF_STR("<CFCharacterSet Predefined Control Set>");
                    break;
                case 2:
                    description = OF_STR("<CFCharacterSet Predefined Whitespace Set>");
                    break;
                case 3:
                    description = OF_STR("<CFCharacterSet Predefined WhitespaceAndNewline Set>");
                    break;
                case 4:
                    description = OF_STR("<CFCharacterSet Predefined DecimalDigit Set>");
                    break;
                case 5:
                    description = OF_STR("<CFCharacterSet Predefined Letter Set>");
                    break;
                case 6:
                    description = OF_STR("<CFCharacterSet Predefined LowercaseLetter Set>");
                    break;
                case 7:
                    description = OF_STR("<CFCharacterSet Predefined UppercaseLetter Set>");
                    break;
                case 8:
                    description = OF_STR("<CFCharacterSet Predefined NonBase Set>");
                    break;
                case 9:
                    description = OF_STR("<CFCharacterSet Predefined Decomposable Set>");
                    break;
                case 10:
                    description = OF_STR("<CFCharacterSet Predefined AlphaNumeric Set>");
                    break;
                case 11:
                    description = OF_STR("<CFCharacterSet Predefined Punctuation Set>");
                    break;
                case 12:
                    description = OF_STR("<CFCharacterSet Predefined Illegal Set>");
                    break;
                case 13:
                    description = OF_STR("<CFCharacterSet Predefined CapitalizedLetter Set>");
                    break;
                case 14:
                    description = OF_STR("<CFCharacterSet Predefined Symbol Set>");
                    break;
                case 15:
                    description = OF_STR("<CFCharacterSet Predefined Newline Set>");
                    break;
                default:
                    return NULL;
            }
            return (OCFStringRef)OCFRetain(description);
        }
        case kClassRange:
            return OCFStringCreateWithFormat(
                OCFGetAllocator(cset),
                NULL,
                OF_STR("<CFCharacterSet Range(%u, %ld)>"),
                cset->variants.range.firstChar,
                cset->variants.range.length
            );
        case kClassString: {
            OCFIndex length = cset->variants.string.length;
            OCFAllocatorRef allocator = OCFGetAllocator(cset);
            OCFStringRef prefix = OF_STR("<CFCharacterSet Items(");
            OCFMutableStringRef description =
                OCFStringCreateMutable(allocator, OCFStringGetLength(prefix) - length + length * 8 + 2);
            OCFStringAppend(description, prefix);
            for (OCFIndex index = 0; index < length; ++index) {
                OCFStringAppendFormat(
                    description,
                    NULL,
                    OF_STR("%sU+%04X"),
                    index == 0 ? "" : " ",
                    cset->variants.string.buffer[index]
                );
            }
            OCFStringAppend(description, OF_STR(")>"));
            return description;
        }
        case kClassBitmap:
        case kClassCompactBitmap:
            return (OCFStringRef)OCFRetain(OF_STR("<CFCharacterSet Bitmap>"));
        default:
            return NULL;
    }
}

// Native __CFCharacterSetInitialize, 0x1c017445c, with its dispatch_once (.cold.1, 0x1c0306898, on
// __CFCharacterSetInitialize.initOnce, 0x22e6d2788) and _____CFCharacterSetInitialize_block_invoke
// (0x1c0174484, the block of ___block_literal_global.4). Hidden (was private external). The block sets
// __CFCheckForExapendedSet when __CF_DEBUG_EXPANDED_SET starts with 'Y'.
void __CFCharacterSetInitialize(void) {
    static dispatch_once_t initOnce;
    dispatch_once(&initOnce, ^{
      const char *value = getenv("__CF_DEBUG_EXPANDED_SET");
      if (value != NULL && *value == 'Y') {
          __CFCheckForExapendedSet = 1;
      }
    });
}

// Native CFCharacterSetGetTypeID, 0x1c01744bc. External. The constant 25 (the builtin class number).
OCFTypeID OCFCharacterSetGetTypeID(void) {
    return kCharacterSetTypeID;
}

// Native CFCharacterSetGetPredefined, 0x1c01744c4. External. A number outside 1-15 is .cold.2 (0x1c03068b0:
// CFLog level 3, then the crash annotation and BRK #1). With the Swift feature the NSCharacterSet class
// answers the matching +_swift... selector; otherwise the static instance.
// Folds the compiler-outlined piece _CFCharacterSetGetPredefined.cold.1 (0x1c03068ac).
OCFCharacterSetRef OCFCharacterSetGetPredefined(OCFCharacterSetPredefinedSet theSetIdentifier) {
    if ((uint64_t)(theSetIdentifier - 16) <= 0xFFFFFFFFFFFFFFF0ull) {
        OCFLog(
            3,
            OF_STR("%s: Unknown builtin type %ld"),
            "CFCharacterSetRef CFCharacterSetGetPredefined(CFCharacterSetPredefinedSet)",
            (long)theSetIdentifier
        );
        crashWithMessage("Unknown builtin CFCharacterSet type");
    }
    if (foundationSwiftCharacterSetEnabled()) {
        static const char *const selectors[15] = {
            "_swiftControlCharacters",
            "_swiftWhitespaces",
            "_swiftWhitespacesAndNewlines",
            "_swiftDecimalDigits",
            "_swiftLetters",
            "_swiftLowercaseLetters",
            "_swiftUppercaseLetters",
            "_swiftNonBaseCharacters",
            "_swiftDecomposables",
            "_swiftAlphanumerics",
            "_swiftPunctuationCharacters",
            "_swiftIllegalCharacters",
            "_swiftCapitalizedLetters",
            "_swiftSymbols",
            "_swiftNewlines",
        };
        return ((OCFCharacterSetRef (*)(id, SEL))objc_msgSend)(
            (id)swiftCharacterSetClass, sel_getUid(selectors[theSetIdentifier - 1])
        );
    }
    return &__CFBuiltinSets[theSetIdentifier - 1];
}

// Native CFCharacterSetCreateWithCharactersInRange, 0x1c0174644, with .cold.1 at 0x1c03068ec
// (the feature dispatch_once). External. iOS Device divergence: unread.
// The assembly supplies both range arguments to the diagnostic.
OCFCharacterSetRef OCFCharacterSetCreateWithCharactersInRange(OCFAllocatorRef alloc, OCFRange range) {
    if (!_OCFCharacterSetIsValidRange(range.location, range.length)) {
        OCFLog(
            3,
            OF_STR("%s: Range (location: %ld, length: %ld) outside of valid Unicode range (0x0 - 0x10FFFF)"),
            "CFCharacterSetRef CFCharacterSetCreateWithCharactersInRange(CFAllocatorRef, CFRange)",
            range.location,
            range.length
        );
        crashWithMessage("CFCharacterSet range is outside of valid Unicode range (0x0 - 0x10FFFF)");
    }
    if (foundationSwiftCharacterSetEnabled()) {
        return ((OCFCharacterSetRef (*)(id, SEL, OCFRange))objc_msgSend)(
            (id)swiftCharacterSetClass, sel_getUid("_newCharacterSetWithRange:"), range
        );
    }
    CharacterSet *set = (CharacterSet *)_OCFRuntimeCreateInstance(alloc, kCharacterSetTypeID, 32, NULL);
    if (set != NULL) {
        updateInfo(set, kInfoMutable, 0);
        if (range.length != 0) {
            setClass(set, kClassRange);
            set->hashValue = 0;
            set->variants.range.firstChar = (uint32_t)range.location;
            set->variants.range.length = range.length;
            set->annex = NULL;
        } else {
            setClass(set, kClassBitmap);
            set->annex = NULL;
            set->hashValue = 0;
            set->variants.bitmap = NULL;
            updateInfo(set, 0, kInfoHasHash);
        }
    }
    return set;
}

// Native CFCharacterSetCreateWithCharactersInString, 0x1c01747f8. External. With the Swift feature,
// +[NSCharacterSet _newCharacterSetWithCharactersInString:]. A string of at most 63 units becomes an
// immutable string set: a 0x80-byte buffer, the characters sorted and deduplicated, the hash marked valid (0)
// when empty. If its first unit is below 0xDC00, the unit at the original last index is at least 0xDC00, and
// a surrogate occurs within the original length, the set is released and the long path runs: a mutable set,
// the string added, compacted, and the mutable bit cleared.
// Folds the compiler-outlined piece _CFCharacterSetCreateWithCharactersInString.cold.1 (0x1c03068f0).
OCFCharacterSetRef
OCFCharacterSetCreateWithCharactersInString(OCFAllocatorRef alloc, OCFStringRef theString) {
    if (foundationSwiftCharacterSetEnabled()) {
        return ((OCFCharacterSetRef (*)(id, SEL, OCFStringRef))objc_msgSend)(
            (id)swiftCharacterSetClass, sel_getUid("_newCharacterSetWithCharactersInString:"), theString
        );
    }
    OCFIndex length = OCFStringGetLength(theString);
    if (length <= 63) {
        CharacterSet *cset = (CharacterSet *)_OCFRuntimeCreateInstance(alloc, kCharacterSetTypeID, 32, NULL);
        if (cset == NULL) {
            return NULL;
        }
        updateInfo(cset, kInfoMutable, 0);
        setClass(cset, kClassString);
        cset->hashValue = 0;
        cset->annex = NULL;
        UniChar *buffer = (UniChar *)OCFAllocatorAllocateTyped(
            OCFGetAllocator(cset), kStringCapacityBytes, kStringBufferType, 0
        );
        cset->variants.string.buffer = buffer;
        cset->variants.string.length = length;
        OCFStringGetCharacters(theString, (OCFRange){0, length}, buffer);
        if (length < 2) {
            qsort(cset->variants.string.buffer, (size_t)length, 2, chcompar);
            cset->variants.string.length = length;
            if (length == 0) {
                updateInfo(cset, 0, kInfoHasHash);
            }
            return cset;
        }
        cset->variants.string.length = sortAndUniqueString(cset->variants.string.buffer, length);
        const UniChar *characters = cset->variants.string.buffer;
        if ((characters[0] >> 10) > 0x36) {
            return cset;
        }
        const UniChar *end = characters + length;
        if ((end[-1] >> 10) < 0x37) {
            return cset;
        }
        bool hasSurrogate = false;
        do {
            UniChar character = *characters++;
            if (((character | 0x400) & 0xFC00) == 0xDC00) {
                hasSurrogate = true;
                break;
            }
        } while (characters < end);
        if (!hasSurrogate) {
            return cset;
        }
        OCFRelease(cset);
    }
    CharacterSet *cset = (CharacterSet *)OCFCharacterSetCreateMutable(alloc);
    OCFCharacterSetAddCharactersInString(cset, theString);
    __CFCSetMakeCompact(cset);
    updateInfo(cset, kInfoMutable, 0);
    return cset;
}

// Native chcompar, iOS Simulator 24A434, 0x1c0174a34, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external; no direct caller (an address-taken comparator between
// CFCharacterSetCreateWithCharactersInString and CFCharacterSetCreateMutable).
// iOS Device 24A446: 0x180718e44, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// identical instructions.
// Returns the first UTF-16 unit minus the second as a 32-bit int (LDRH, LDRH, SUB W0).
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int chcompar(const void *first, const void *second) {
    return (int)*(const uint16_t *)first - (int)*(const uint16_t *)second;
}

// Native CFCharacterSetCreateMutable, 0x1c0174a44. External. With the Swift feature,
// +[NSMutableCharacterSet _newMutableCharacterSet]. Otherwise a type-25 instance with 32 extra bytes:
// mutable, bitmap class, no annex, hash 0 and NULL bits, then the hash marked valid.
// Folds the compiler-outlined piece _CFCharacterSetCreateMutable.cold.1 (0x1c03068f4).
OCFMutableCharacterSetRef OCFCharacterSetCreateMutable(OCFAllocatorRef alloc) {
    if (foundationSwiftCharacterSetEnabled()) {
        return ((OCFMutableCharacterSetRef (*)(id, SEL))objc_msgSend)(
            (id)swiftMutableCharacterSetClass, sel_getUid("_newMutableCharacterSet")
        );
    }
    CharacterSet *cset = (CharacterSet *)_OCFRuntimeCreateInstance(alloc, kCharacterSetTypeID, 32, NULL);
    if (cset != NULL) {
        updateInfo(cset, 0, kInfoMutable);
        setClass(cset, kClassBitmap);
        cset->annex = NULL;
        cset->hashValue = 0;
        cset->variants.bitmap = NULL;
        updateInfo(cset, 0, kInfoHasHash);
    }
    return cset;
}

// Native CFCharacterSetAddCharactersInString, 0x1c0174b20, with .cold.2 (0x1c03068fc, the body) and .cold.3
// (0x1c0306c90, the immutable crash). External. A static predefined set, or an empty inverted set, is left as
// is; an empty string adds nothing. A non-inverted set whose prospective length (0 when empty, the string
// length for a string set, 64 otherwise) plus the string length stays below 64 appends to its 0x80-byte
// buffer: surrogates are squeezed out with memmove when the string has at least two units; a zero total frees
// a buffer this call allocated; otherwise the string class is set (for a new buffer), the length stored, and
// the buffer sorted and deduplicated. Every other set becomes a bitmap filled through the inline buffer.
// Either way the hash is cleared; the bitmap path checks the expanded-set warning; surrogate pairs are
// applied as ranges afterwards.
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_1_2 (0x1c017bb04), _OUTLINED_FUNCTION_2_1
// (0x1c017bb20), _OUTLINED_FUNCTION_3_0 (0x1c017bb38), _OUTLINED_FUNCTION_5_0 (0x1c017bb58),
// _OUTLINED_FUNCTION_7_0 (0x1c017bb94), _OUTLINED_FUNCTION_8_0 (0x1c017bbb0), _OUTLINED_FUNCTION_9
// (0x1c017bbcc), _OUTLINED_FUNCTION_10 (0x1c017bbdc), _OUTLINED_FUNCTION_11 (0x1c017bbf4),
// _OUTLINED_FUNCTION_12 (0x1c017bc18), _OUTLINED_FUNCTION_15 (0x1c017bc50), _OUTLINED_FUNCTION_16
// (0x1c017bc5c), _OUTLINED_FUNCTION_17 (0x1c017bc6c), _OUTLINED_FUNCTION_18 (0x1c017bc7c),
// _OUTLINED_FUNCTION_19 (0x1c017bc8c), _OUTLINED_FUNCTION_20 (0x1c017bc9c), _OUTLINED_FUNCTION_21
// (0x1c017bcb4), _OUTLINED_FUNCTION_23 (0x1c017bcd4), _OUTLINED_FUNCTION_24 (0x1c017bcdc),
// _OUTLINED_FUNCTION_25 (0x1c017bce8), _OUTLINED_FUNCTION_26 (0x1c017bcfc), _OUTLINED_FUNCTION_27
// (0x1c017bd10).
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_28 (0x1c017bd24).
// Folds the compiler-outlined piece _CFCharacterSetAddCharactersInString.cold.1 (0x1c03068f8).
void OCFCharacterSetAddCharactersInString(OCFMutableCharacterSetRef theSet, OCFStringRef theString) {
    if (characterSetIsObjC(theSet)) {
        ((void (*)(id, SEL, OCFStringRef))objc_msgSend)(
            (id)theSet, sel_getUid("addCharactersInString:"), theString
        );
        return;
    }
    CharacterSet *cset = theSet;
    checkMutable(cset, "void CFCharacterSetAddCharactersInString(CFMutableCharacterSetRef, CFStringRef)");
    if (isStaticPredefinedSet(cset)) {
        return;
    }
    if (isEmpty(cset) && isInverted(cset)) {
        return;
    }
    OCFIndex length = OCFStringGetLength(theString);
    if (length == 0) {
        return;
    }
    OCFIndex previousLength = 0;
    OCFIndex newLength = 0;
    bool useString = false;
    if (!isInverted(cset)) {
        if (isEmpty(cset)) {
            previousLength = 0;
        } else if ((loadInfo(cset) & kInfoClassMask) == 0x20) {
            previousLength = cset->variants.string.length;
        } else {
            previousLength = 64;
        }
        newLength = previousLength + length;
        useString = newLength < 64;
    }
    if (!useString) {
        __CFCSetMakeBitmap(cset);
        OCFStringInlineBuffer inlineBuffer;
        characterSetInitStringInlineBuffer(theString, &inlineBuffer, (OCFRange){0, length});
        bool hasSurrogate = false;
        for (OCFIndex index = 0; index < length; ++index) {
            UniChar character = characterSetGetCharacterFromStringInlineBuffer(&inlineBuffer, index);
            if (((character & 0xF800) | 0x400) == 0xDC00) {
                hasSurrogate = true;
            } else {
                cset->variants.bitmap[(character >> 3) & 0x1FFF] |= (uint8_t)(1u << (character & 7));
            }
        }
        clearHashValid(cset);
        if (__CFCheckForExapendedSet == 1) {
            __CFCheckForExpandedSet(cset);
        }
        if (hasSurrogate) {
            __CFApplySurrogatesInString(cset, theString, OCFCharacterSetAddCharactersInRange);
        }
        return;
    }
    UniChar *append;
    if (cset->variants.string.buffer != NULL) {
        append = cset->variants.string.buffer + cset->variants.string.length;
    } else {
        append = (UniChar *)OCFAllocatorAllocateTyped(
            OCFGetAllocator(cset), kStringCapacityBytes, kStringBufferType, 0
        );
    }
    OCFStringGetCharacters(theString, (OCFRange){0, length}, append);
    bool hasSurrogate = false;
    if (length >= 2) {
        UniChar *cursor = append;
        UniChar *end = append + length;
        do {
            if (((*cursor | 0x400) & 0xFC00) == 0xDC00) {
                memmove(cursor, cursor + 1, (size_t)((uint8_t *)end - (uint8_t *)(cursor + 1)));
                --end;
                hasSurrogate = true;
            } else {
                ++cursor;
            }
        } while (cursor < end);
        newLength = previousLength + (end - append);
    }
    if (newLength == 0) {
        if (cset->variants.string.buffer == NULL) {
            OCFAllocatorDeallocate(OCFGetAllocator(cset), append);
        }
    } else {
        UniChar *buffer = cset->variants.string.buffer;
        if (buffer == NULL) {
            setClass(cset, kClassString);
            cset->variants.string.buffer = append;
            buffer = append;
        }
        cset->variants.string.length = newLength;
        cset->variants.string.length = sortAndUniqueString(buffer, newLength);
    }
    clearHashValid(cset);
    if (hasSurrogate) {
        __CFApplySurrogatesInString(cset, theString, OCFCharacterSetAddCharactersInRange);
    }
}

// Native __CFCSetMakeCompact, 0x1c0174bf4. Non-external. A bitmap set with bits trades them for a compact
// bitmap when __CFCreateCompactBitmap succeeds.
static void __CFCSetMakeCompact(CharacterSet *cset) {
    if ((loadInfo(cset) & kInfoClassMask) != 0x30) {
        return;
    }
    uint8_t *bitmap = cset->variants.bitmap;
    if (bitmap == NULL) {
        return;
    }
    uint8_t *compact = __CFCreateCompactBitmap(OCFGetAllocator(cset), bitmap);
    if (compact == NULL) {
        return;
    }
    OCFAllocatorDeallocate(OCFGetAllocator(cset), bitmap);
    setClass(cset, kClassCompactBitmap);
    cset->variants.bitmap = compact;
}

// Native CFCharacterSetCreateWithBitmapRepresentation, 0x1c0174c84, with .cold.1 at 0x1c0306ca0.
// External. iOS Device divergence: unread. The Swift class binding at 0x22229c238 has medium confidence.
// CoreFoundation defect: at 0x1c0175044, 0x1c01750a0, 0x1c017513c and 0x1c0175168, the verified native
// code overwrites a duplicate plane's bitmap without freeing the old storage. Release it at replacement.
OCFCharacterSetRef OCFCharacterSetCreateWithBitmapRepresentation(OCFAllocatorRef alloc, OCFDataRef data) {
    if (foundationSwiftCharacterSetEnabled()) {
        return ((OCFCharacterSetRef (*)(id, SEL, OCFDataRef))objc_msgSend)(
            (id)swiftCharacterSetClass, sel_getUid("_newCharacterSetWithBitmapRepresentation:"), data
        );
    }
    CharacterSet *set = (CharacterSet *)_OCFRuntimeCreateInstance(alloc, kCharacterSetTypeID, 32, NULL);
    if (set == NULL) {
        return NULL;
    }
    updateInfo(set, kInfoMutable, 0);
    setClass(set, kClassBitmap);
    set->hashValue = 0;
    set->annex = NULL;
    OCFIndex length = data != NULL ? OCFDataGetLength(data) : 0;
    if (length < 1) {
        set->variants.bitmap = NULL;
        updateInfo(set, 0, kInfoHasHash);
        return set;
    }
    if (length < kBitmapSize) {
        uint8_t *bitmap = OCFAllocatorAllocateTyped(alloc, kBitmapSize, kBitmapType, 0);
        const uint8_t *bytes = OCFDataGetBytePtr(data);
        memmove(bitmap, bytes, (size_t)length);
        bzero(bitmap + length, (size_t)(kBitmapSize - length));
        uint8_t *compact = __CFCreateCompactBitmap(alloc, bitmap);
        if (compact != NULL) {
            OCFAllocatorDeallocate(alloc, bitmap);
            set->variants.bitmap = compact;
            setClass(set, kClassCompactBitmap);
        } else {
            set->variants.bitmap = bitmap;
        }
        return set;
    }

    uint8_t *compact = __CFCreateCompactBitmap(alloc, OCFDataGetBytePtr(data));
    if (compact != NULL) {
        set->variants.bitmap = compact;
        setClass(set, kClassCompactBitmap);
    } else {
        uint8_t *bitmap = OCFAllocatorAllocateTyped(alloc, kBitmapSize, kBitmapType, 0);
        memmove(bitmap, OCFDataGetBytePtr(data), kBitmapSize);
        set->variants.bitmap = bitmap;
    }
    if (length == kBitmapSize) {
        return set;
    }

    const uint8_t *planeBytes = OCFDataGetBytePtr(data) + kBitmapSize;
    OCFIndex remaining = length - kBitmapSize;
    while (remaining >= 2) {
        uint8_t plane = *planeBytes++;
        if (plane == 0) {
            __builtin_trap();
        }
        if (plane <= 16) {
            CharacterSet *planeSet = annexGetOrCreatePlane(set, plane);
            if (planeSet != NULL) {
                uint8_t *bitmap = NULL;
                if (remaining <= kBitmapSize) {
                    OCFIndex bytes = remaining - 1;
                    bitmap = OCFAllocatorAllocateTyped(alloc, kBitmapSize, kBitmapType, 0);
                    memmove(bitmap, planeBytes, (size_t)bytes);
                    bzero(bitmap + bytes, (size_t)(kBitmapSize - bytes));
                    compact = __CFCreateCompactBitmap(alloc, bitmap);
                    if (compact != NULL) {
                        OCFAllocatorDeallocate(alloc, bitmap);
                    }
                } else {
                    compact = __CFCreateCompactBitmap(alloc, planeBytes);
                    if (compact == NULL) {
                        bitmap = OCFAllocatorAllocateTyped(alloc, kBitmapSize, kBitmapType, 0);
                        memmove(bitmap, planeBytes, kBitmapSize);
                    }
                }
                if (planeSet->variants.bitmap != NULL) {
                    OCFAllocatorDeallocate(alloc, planeSet->variants.bitmap);
                }
                planeSet->variants.bitmap = compact != NULL ? compact : bitmap;
                setClass(planeSet, compact != NULL ? kClassCompactBitmap : kClassBitmap);
            }
        }
        if (remaining <= kBitmapSize + 2) {
            break;
        }
        planeBytes += kBitmapSize;
        remaining -= kBitmapSize + 1;
    }
    return set;
}

// Native CFCharacterSetCreateInvertedSet, 0x1c01751a0. External. Only a tagged pointer or an Objective-C
// object of type 25 (no Swift feature test) answers -invertedSet, retained (objc_retain, a tail call).
// Otherwise a mutable copy is inverted and its mutable bit (info bit 0) cleared with a compare-and-swap loop.
OCFCharacterSetRef OCFCharacterSetCreateInvertedSet(OCFAllocatorRef alloc, OCFCharacterSetRef theSet) {
    if (((uintptr_t)theSet >> 63) != 0 || __CF_IS_OBJC(kCharacterSetTypeID, theSet) != 0) {
        return (OCFCharacterSetRef)objc_retain(
            ((id (*)(id, SEL))objc_msgSend)((id)theSet, sel_getUid("invertedSet"))
        );
    }
    OCFMutableCharacterSetRef copy = __CFCharacterSetCreateCopy(alloc, theSet, true);
    OCFCharacterSetInvert(copy);
    updateInfo(copy, kInfoMutable, 0);
    return copy;
}

// Native CFCharacterSetCreateMutableCopy, 0x1c0175224. External; __CFCharacterSetCreateCopy with true.
OCFMutableCharacterSetRef OCFCharacterSetCreateMutableCopy(OCFAllocatorRef alloc, OCFCharacterSetRef theSet) {
    return __CFCharacterSetCreateCopy(alloc, theSet, true);
}

// Native CFCharacterSetInvert, 0x1c017522c, with .cold.2 (0x1c0306ca8, the body) and .cold.3 (0x1c0306e90).
// External. A static predefined set is left alone; the hash is cleared. A bitmap set complements its bits (or
// allocates 0x2000 set bytes, type 0x100004052888210); a compact bitmap swaps its uniform blocks and
// complements its stored blocks; other classes toggle the inverted bit (from the bit read before the loop)
// and, without an annex, get a new one marked inverted. Bitmap classes then toggle the annex inversion,
// creating the annex.
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_6_0 (0x1c017bb8c), _OUTLINED_FUNCTION_22
// (0x1c017bccc).
// Folds the compiler-outlined piece _CFCharacterSetInvert.cold.1 (0x1c0306ca4).
void OCFCharacterSetInvert(OCFMutableCharacterSetRef theSet) {
    if (characterSetIsObjC(theSet)) {
        ((void (*)(id, SEL))objc_msgSend)((id)theSet, sel_getUid("invert"));
        return;
    }
    CharacterSet *cset = theSet;
    checkMutable(cset, "void CFCharacterSetInvert(CFMutableCharacterSetRef)");
    if (isStaticPredefinedSet(cset)) {
        return;
    }
    clearHashValid(cset);
    CharacterSetAnnex *annex;
    if ((loadInfo(cset) & kInfoClassMask) == 0x30) {
        uint8_t *bitmap = cset->variants.bitmap;
        if (bitmap != NULL) {
            for (uint32_t index = 0; index != kBitmapSize; ++index) {
                bitmap[index] = (uint8_t)~bitmap[index];
            }
        } else {
            bitmap = (uint8_t *)OCFAllocatorAllocateTyped(
                OCFGetAllocator(cset), kBitmapSize, kInvertBitmapType, 0
            );
            cset->variants.bitmap = bitmap;
            memset(bitmap, 0xFF, kBitmapSize);
        }
    } else if ((loadInfo(cset) & kInfoClassMask) == 0x40) {
        uint32_t storedBytes = 0;
        uint8_t *compact = cset->variants.bitmap;
        for (uint32_t index = 0; index != 0x100; ++index) {
            uint8_t header = compact[index];
            if (header == 0) {
                compact[index] = 0xFF;
            } else if (header == 0xFF) {
                compact[index] = 0;
            } else {
                storedBytes += 32;
            }
        }
        if ((int32_t)storedBytes >= 1) {
            uint8_t *stored = compact + 0x100;
            for (uint32_t index = 0; index != storedBytes; ++index) {
                stored[index] = (uint8_t)~stored[index];
            }
        }
    } else {
        uint64_t inverted = loadInfo(cset) & kInfoInverted;
        uint64_t info = __atomic_load_n(&cset->base.info, __ATOMIC_ACQUIRE);
        uint64_t updated;
        do {
            updated = ((info & ~(uint64_t)kInfoInverted) | inverted) ^ kInfoInverted;
        } while (!__atomic_compare_exchange_n(
            &cset->base.info, &info, updated, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
        ));
        if (cset->annex == NULL) {
            annex = annexAllocateEmpty(cset);
            annex->isInverted = 1;
            return;
        }
        annex = cset->annex;
        annex->isInverted = annex->isInverted == 0;
        return;
    }
    annex = cset->annex;
    if (annex == NULL) {
        annex = annexAllocateEmpty(cset);
    }
    annex->isInverted = annex->isInverted == 0;
}

// Native CFCharacterSetCreateCopy, 0x1c01752e8. External; __CFCharacterSetCreateCopy with false.
OCFCharacterSetRef OCFCharacterSetCreateCopy(OCFAllocatorRef alloc, OCFCharacterSetRef theSet) {
    return __CFCharacterSetCreateCopy(alloc, theSet, false);
}

// Native __CFCharacterSetCreateCopy, 0x1c01752f0. Non-external. Objective-C sets answer -mutableCopy; an
// immutable source copied immutably is retained. Otherwise a new mutable set (its mutable bit stays set, also
// for an immutable copy) takes the class, hash-valid and inverted bits and the hash; the variant is copied
// with the given allocator (an immutable bitmap copy is compacted when possible; a compact copy is 256 + 32
// bytes per stored block); valid annex planes are copied recursively and stored; the annex inversion is
// copied.
// Folds the compiler-outlined piece ___CFCharacterSetCreateCopy.cold.1 (0x1c0306ea0).
static OCFMutableCharacterSetRef
__CFCharacterSetCreateCopy(OCFAllocatorRef alloc, OCFCharacterSetRef theSet, Boolean isMutable) {
    if (characterSetIsObjC(theSet)) {
        return ((OCFMutableCharacterSetRef (*)(id, SEL))objc_msgSend)((id)theSet, sel_getUid("mutableCopy"));
    }
    const CharacterSet *source = theSet;
    if (!isMutable && (loadInfo(source) & kInfoMutable) == 0) {
        return (OCFMutableCharacterSetRef)OCFRetain(source);
    }
    CharacterSet *copy = OCFCharacterSetCreateMutable(alloc);
    updateInfo(copy, kInfoClassMask, loadInfo(source) & kInfoClassMask);
    updateInfo(copy, kInfoHasHash, loadInfo(source) & kInfoHasHash);
    updateInfo(copy, kInfoInverted, loadInfo(source) & kInfoInverted);
    copy->hashValue = source->hashValue;
    switch (classOf(source)) {
        case kClassBuiltin:
            copy->variants.builtinType = source->variants.builtinType;
            break;
        case kClassRange:
            copy->variants.range.firstChar = source->variants.range.firstChar;
            copy->variants.range.length = source->variants.range.length;
            break;
        case kClassString: {
            UniChar *buffer =
                (UniChar *)OCFAllocatorAllocateTyped(alloc, kStringCapacityBytes, kStringBufferType, 0);
            copy->variants.string.buffer = buffer;
            OCFIndex length = source->variants.string.length;
            copy->variants.string.length = length;
            memmove(buffer, source->variants.string.buffer, (size_t)(length << 1));
            break;
        }
        case kClassBitmap:
            if (source->variants.bitmap == NULL) {
                copy->variants.bitmap = NULL;
                break;
            }
            if (!isMutable) {
                uint8_t *compact = __CFCreateCompactBitmap(alloc, source->variants.bitmap);
                if (compact != NULL) {
                    copy->variants.bitmap = compact;
                    setClass(copy, kClassCompactBitmap);
                    break;
                }
            }
            {
                uint8_t *bitmap = (uint8_t *)OCFAllocatorAllocateTyped(alloc, kBitmapSize, kBitmapType, 0);
                memmove(bitmap, source->variants.bitmap, kBitmapSize);
                copy->variants.bitmap = bitmap;
            }
            break;
        case kClassCompactBitmap: {
            const uint8_t *compact = source->variants.bitmap;
            if (compact == NULL) {
                break;
            }
            uint32_t size = 0x100;
            for (uint32_t index = 0; index != 0x100; ++index) {
                if ((uint8_t)(compact[index] - 1) < 0xFE) {
                    size += 0x20;
                }
            }
            uint8_t *bytes = (uint8_t *)OCFAllocatorAllocateTyped(alloc, size, kBitmapType, 0);
            memmove(bytes, compact, size);
            copy->variants.bitmap = bytes;
            break;
        }
        default:
            break;
    }
    const CharacterSetAnnex *annex = source->annex;
    if (annex == NULL) {
        return copy;
    }
    if (annex->validPlanes == 0) {
        if (annex->isInverted == 0) {
            return copy;
        }
        CharacterSetAnnex *copyAnnex = copy->annex;
        if (copyAnnex == NULL) {
            copyAnnex = annexAllocateEmpty(copy);
        }
        copyAnnex->isInverted = 1;
        return copy;
    }
    for (uint32_t plane = 1; plane != 17; ++plane) {
        OCFCharacterSetRef planeSet = annexGetPlane(source, (uint8_t)plane);
        if (planeSet == NULL) {
            continue;
        }
        OCFMutableCharacterSetRef planeCopy = __CFCharacterSetCreateCopy(alloc, planeSet, isMutable);
        annexPutPlane(copy, (uint8_t)plane, planeCopy);
        OCFRelease(planeCopy);
    }
    if (source->annex != NULL && source->annex->isInverted != 0) {
        CharacterSetAnnex *copyAnnex = copy->annex;
        if (copyAnnex == NULL) {
            copyAnnex = annexAllocateEmpty(copy);
        }
        copyAnnex->isInverted = 1;
    } else if (copy->annex != NULL) {
        copy->annex->isInverted = 0;
    }
    return copy;
}

// Native CFCharacterSetIsCharacterMember, 0x1c0175864. External. Objective-C sets answer
// -longCharacterIsMember:. Builtin: CFUniCharIsMemberOf, inverted by the inverted bit. Range: the inverted
// bit unless the length is nonzero and the first character is at most the character, then inverted against
// first + length > character. String: a binary search of the sorted buffer. Bitmap and compact bitmap: the
// bit (without the inverted bit; a uniform compact block of 0 or 0xFF answers directly); NULL bits answer the
// inverted bit. Other classes answer false.
// Folds the compiler-outlined piece _CFCharacterSetIsCharacterMember.cold.1 (0x1c0306ea4).
Boolean OCFCharacterSetIsCharacterMember(OCFCharacterSetRef theSet, UniChar theChar) {
    if (characterSetIsObjC(theSet)) {
        return ((Boolean (*)(id, SEL, UTF32Char))objc_msgSend)(
            (id)theSet, sel_getUid("longCharacterIsMember:"), theChar
        );
    }
    const CharacterSet *cset = theSet;
    Boolean inverted = (Boolean)((loadInfo(cset) >> 3) & 1);
    switch (classOf(cset)) {
        case kClassBuiltin:
            return inverted ^ OCFUniCharIsMemberOf(theChar, (uint32_t)cset->variants.builtinType);
        case kClassRange: {
            OCFIndex length = cset->variants.range.length;
            if (length == 0) {
                return inverted;
            }
            uint32_t firstChar = cset->variants.range.firstChar;
            if (firstChar > theChar) {
                return inverted;
            }
            return inverted ^ (length + (OCFIndex)firstChar > (OCFIndex)theChar);
        }
        case kClassString: {
            OCFIndex length = cset->variants.string.length;
            if (length == 0) {
                return inverted;
            }
            const UniChar *begin = cset->variants.string.buffer;
            if (theChar < *begin) {
                return inverted;
            }
            const UniChar *end = begin + length - 1;
            if (theChar > *end) {
                return inverted;
            }
            while (begin <= end) {
                const UniChar *middle = begin + ((end - begin) >> 1);
                if (theChar < *middle) {
                    end = middle - 1;
                } else if (theChar > *middle) {
                    begin = middle + 1;
                } else {
                    return inverted ^ 1;
                }
            }
            return inverted;
        }
        case kClassBitmap: {
            const uint8_t *bitmap = cset->variants.bitmap;
            if (bitmap == NULL) {
                return inverted;
            }
            return (bitmap[theChar >> 3] >> (theChar & 7)) & 1;
        }
        case kClassCompactBitmap: {
            const uint8_t *compact = cset->variants.bitmap;
            if (compact == NULL) {
                return inverted;
            }
            uint8_t header = compact[theChar >> 8];
            if (header == 0) {
                return false;
            }
            if (header == 0xFF) {
                return true;
            }
            return (compact[(uint32_t)header * 32 + ((theChar & 0xF8) >> 3) + 0xE0] >> (theChar & 7)) & 1;
        }
        default:
            return false;
    }
}

// Native __CFCharacterSetLongCharacterMemberIMPForSet.onceToken, .swiftIMP and .swiftImmortalIMP,
// 0x22e6d27a8, 0x22e6d2798 and 0x22e6d27a0 (__DATA,__bss), and
// _____CFCharacterSetLongCharacterMemberIMPForSet_block_invoke (0x1c017ba8c, the block of
// ___block_literal_global.61; dispatch_once in _CFCharacterSetIsLongCharacterMemberForInline.cold.1,
// 0x1c0306ea8). Non-external. The block resolves -longCharacterIsMember: (the selector 0x2387b0a56, which the
// image's objc_msgSend$longCharacterIsMember: stub loads) on _NSSwiftCharacterSet and on the class of
// +[NSCharacterSet _swiftWhitespaces].
static dispatch_once_t longCharacterMemberIMPOnceToken;
static IMP longCharacterMemberSwiftIMP;
static IMP longCharacterMemberSwiftImmortalIMP;

// Native _CFCharacterSetIsLongCharacterMemberForInline, 0x1c0175a3c. External. Without flag bits 3-4 (tested
// on the flag byte), CFCharacterSetIsLongCharacterMember on the buffer's set; otherwise the resolved Swift
// method for bit 3, else for bit 4 (the flag word is reloaded; neither bit is BRK #1).
Boolean
_OCFCharacterSetIsLongCharacterMemberForInline(OCFCharacterSetInlineBuffer *buffer, UTF32Char character) {
    if ((*(const uint8_t *)&buffer->flags & 0x18) == 0) {
        return OCFCharacterSetIsLongCharacterMember(buffer->cset, character);
    }
    dispatch_once(&longCharacterMemberIMPOnceToken, ^{
      SEL selector = sel_getUid("longCharacterIsMember:");
      Class swiftClass = objc_getClass(OF_NAME_STRING(_ONSSwiftCharacterSet));
      if (swiftClass != Nil) {
          longCharacterMemberSwiftIMP = class_getMethodImplementation(swiftClass, selector);
      }
      id whitespaces =
          ((id (*)(id, SEL))objc_msgSend)((id)swiftCharacterSetClass, sel_getUid("_swiftWhitespaces"));
      Class immortalClass = object_getClass(whitespaces);
      if (immortalClass != Nil) {
          longCharacterMemberSwiftImmortalIMP = class_getMethodImplementation(immortalClass, selector);
      }
    });
    uint32_t flags = buffer->flags;
    IMP implementation;
    if ((flags & 0x8) != 0) {
        implementation = longCharacterMemberSwiftIMP;
    } else if ((flags & 0x10) != 0) {
        implementation = longCharacterMemberSwiftImmortalIMP;
    } else {
        __builtin_trap();
    }
    return ((Boolean (*)(OCFCharacterSetRef, SEL, UTF32Char))implementation)(
        buffer->cset, sel_getUid("longCharacterIsMember:"), character
    );
}

// Native CFCharacterSetIsLongCharacterMember, 0x1c0175acc. External. A character outside plane 0 is answered
// by the builtin table (inverted by the inverted bit), else by the annex: past plane 16 or without a plane
// set, the annex inversion if any plane is valid, otherwise (and without an annex, false) a range set's range
// test; with a plane set, the low 16 bits are tested in the plane set and inverted by the annex inversion.
// Plane 0 and plane sets use the class tests of CFCharacterSetIsCharacterMember; other classes answer false.
// Folds the compiler-outlined piece _CFCharacterSetIsLongCharacterMember.cold.1 (0x1c0306ebc).
Boolean OCFCharacterSetIsLongCharacterMember(OCFCharacterSetRef theSet, UTF32Char theChar) {
    if (characterSetIsObjC(theSet)) {
        return ((Boolean (*)(id, SEL, UTF32Char))objc_msgSend)(
            (id)theSet, sel_getUid("longCharacterIsMember:"), theChar
        );
    }
    const CharacterSet *cset = theSet;
    uint32_t plane = (theChar >> 16) & 0xFF;
    Boolean annexInverted = false;
    if (plane != 0) {
        if ((loadInfo(cset) & kInfoClassMask) == 0) {
            Boolean inverted = (Boolean)((loadInfo(cset) >> 3) & 1);
            return inverted ^ OCFUniCharIsMemberOf(theChar, (uint32_t)cset->variants.builtinType);
        }
        const CharacterSetAnnex *annex = cset->annex;
        Boolean result;
        uint32_t validPlanes;
        if (annex == NULL) {
            result = false;
            validPlanes = 0;
        } else {
            result = annex->isInverted != 0;
            validPlanes = annex->validPlanes;
            if (plane < 0x11 && ((validPlanes >> ((theChar >> 16) & 31)) & 1) != 0 &&
                annex->planeCount >= plane) {
                OCFCharacterSetRef planeSet = annex->planes[((theChar >> 16) & 0x1F) - 1];
                if (planeSet != NULL) {
                    theChar &= 0xFFFF;
                    cset = planeSet;
                    annexInverted = result;
                    goto testPlane;
                }
            }
        }
        if (validPlanes != 0) {
            return result;
        }
        if ((loadInfo(cset) & kInfoClassMask) != 0x10) {
            return result;
        }
        result = (Boolean)((loadInfo(cset) >> 3) & 1);
        OCFIndex length = cset->variants.range.length;
        if (length == 0) {
            return result;
        }
        uint32_t firstChar = cset->variants.range.firstChar;
        if (firstChar > theChar) {
            return result;
        }
        return result ^ (length + (OCFIndex)firstChar > (OCFIndex)theChar);
    }
testPlane:
    {
        Boolean member = (Boolean)((loadInfo(cset) >> 3) & 1);
        switch (classOf(cset)) {
            case kClassBuiltin:
                member ^= OCFUniCharIsMemberOf(theChar, (uint32_t)cset->variants.builtinType);
                break;
            case kClassRange: {
                OCFIndex length = cset->variants.range.length;
                if (length != 0) {
                    uint32_t firstChar = cset->variants.range.firstChar;
                    if (firstChar <= theChar) {
                        member ^= (length + (OCFIndex)firstChar > (OCFIndex)theChar);
                    }
                }
                break;
            }
            case kClassString: {
                OCFIndex length = cset->variants.string.length;
                if (length == 0) {
                    break;
                }
                const UniChar *begin = cset->variants.string.buffer;
                UniChar character = (UniChar)theChar;
                if (*begin > character) {
                    break;
                }
                const UniChar *end = begin + length - 1;
                if (begin > end || *end < character) {
                    break;
                }
                while (begin <= end) {
                    const UniChar *middle = begin + ((end - begin) >> 1);
                    if (*middle > character) {
                        end = middle - 1;
                    } else if (*middle < character) {
                        begin = middle + 1;
                    } else {
                        member ^= 1;
                        break;
                    }
                }
                break;
            }
            case kClassBitmap: {
                const uint8_t *bitmap = cset->variants.bitmap;
                if (bitmap != NULL) {
                    member = (bitmap[(theChar >> 3) & 0x1FFF] >> (theChar & 7)) & 1;
                }
                break;
            }
            case kClassCompactBitmap: {
                const uint8_t *compact = cset->variants.bitmap;
                if (compact == NULL) {
                    break;
                }
                uint8_t header = compact[(theChar >> 8) & 0xFF];
                if (header == 0) {
                    member = false;
                } else if (header == 0xFF) {
                    member = true;
                } else {
                    member =
                        (compact[(uint32_t)header * 32 + ((theChar >> 3) & 0x1F) + 0xE0] >> (theChar & 7)) &
                        1;
                }
                break;
            }
            default:
                return false;
        }
        return annexInverted ^ member;
    }
}

// Native CFCharacterSetIsSupersetOfSet, 0x1c0175d9c, with .cold.1-.cold.3 (0x1c0306ec0-0x1c0306ec8, each
// the dispatch_once of the Swift feature check). External. Whether each set is Objective-C (a tagged pointer,
// or __CF_IS_OBJC for type 25) is decided first, the set before the other set. An Objective-C set is expanded
// through -_expandedCFCharacterSet when it is tagged, the Swift feature bit 0 is set or __CF_IS_OBJC holds
// again (the feature check first); otherwise, or without an expansion, the general path runs. The fast paths
// then use the expanded sets: an empty set is a superset when inverted, and of an empty non-inverted other
// set only through the general path (any other other set: false); an empty non-inverted other set is a subset
// of any nonempty set. Builtin sets of the same class and type, neither inverted unless the other set is
// inverted (then the general path), and ranges of the same class compare directly (first characters as
// unsigned 32-bit values, ends as signed 64-bit sums; 0x110000 is the end of Unicode). Otherwise plane 0
// bitmaps (two 0x2000-byte stack buffers) must cover the other set's bits; the annex plane masks (bits 1-16,
// complemented within 0x1FFFE for an inverted annex) must cover the other set's; each plane present in both
// annexes must satisfy (set ^ setMask) | (other ^ otherMask) == ~0 word by word, and a plane only in the set
// is compared with the full or empty bitmap when the other set's annex is inverted. The general path asks an
// Objective-C pair (both sets Objective-C, and the set Objective-C again by the Swift feature value 1, the
// tag or __CF_IS_OBJC) -isSupersetOfSet:; otherwise a mutable copy of the set (kCFAllocatorSystemDefault) is
// intersected with the other set and compared with it by __CFCharacterSetEqual.
// Folds the compiler-outlined piece _CFCharacterSetIsSupersetOfSet.cold.2 (0x1c0306ec4).
Boolean OCFCharacterSetIsSupersetOfSet(OCFCharacterSetRef theSet, OCFCharacterSetRef theOtherset) {
    bool setIsObjC = ((uintptr_t)theSet >> 63) != 0 || __CF_IS_OBJC(kCharacterSetTypeID, theSet) != 0;
    bool otherIsObjC =
        ((uintptr_t)theOtherset >> 63) != 0 || __CF_IS_OBJC(kCharacterSetTypeID, theOtherset) != 0;
    OCFCharacterSetRef expandedSet = NULL;
    OCFCharacterSetRef expandedOtherSet = NULL;
    if (setIsObjC) {
        (void)foundationSwiftCharacterSetEnabled();
        if (((uintptr_t)theSet >> 63) == 0 && (foundationSwiftCharacterSetFeatureEnabled & 1) == 0 &&
            __CF_IS_OBJC(kCharacterSetTypeID, theSet) == 0) {
            goto general;
        }
        expandedSet = ((OCFCharacterSetRef (*)(id, SEL))objc_msgSend)(
            (id)theSet, sel_getUid("_expandedCFCharacterSet")
        );
        if (expandedSet == NULL) {
            goto general;
        }
    }
    if (otherIsObjC) {
        (void)foundationSwiftCharacterSetEnabled();
        if (((uintptr_t)theOtherset >> 63) == 0 && (foundationSwiftCharacterSetFeatureEnabled & 1) == 0 &&
            __CF_IS_OBJC(kCharacterSetTypeID, theOtherset) == 0) {
            goto general;
        }
        expandedOtherSet = ((OCFCharacterSetRef (*)(id, SEL))objc_msgSend)(
            (id)theOtherset, sel_getUid("_expandedCFCharacterSet")
        );
        if (expandedOtherSet == NULL) {
            goto general;
        }
    }
    if (expandedSet != NULL) {
        theSet = expandedSet;
    }
    if (expandedOtherSet != NULL) {
        theOtherset = expandedOtherSet;
    }
    {
        const CharacterSet *cset = theSet;
        const CharacterSet *other = theOtherset;
        if (isEmpty(cset)) {
            if (isInverted(cset)) {
                return true;
            }
            if (!isEmpty(other)) {
                return false;
            }
            if (!isInverted(other)) {
                goto general;
            }
            return false;
        }
        if (isEmpty(other) && !isInverted(other)) {
            return true;
        }
        if ((loadInfo(cset) & kInfoClassMask) == 0 || (loadInfo(other) & kInfoClassMask) == 0) {
            if (((loadInfo(cset) ^ loadInfo(other)) & kInfoClassMask) != 0) {
                goto general;
            }
            if (cset->variants.builtinType != other->variants.builtinType) {
                goto general;
            }
            if (isInverted(cset)) {
                goto general;
            }
            if (!isInverted(other)) {
                return true;
            }
            goto general;
        }
        if ((loadInfo(cset) & kInfoClassMask) == 0x10 || (loadInfo(other) & kInfoClassMask) == 0x10) {
            if (((loadInfo(cset) ^ loadInfo(other)) & kInfoClassMask) != 0) {
                goto general;
            }
            uint64_t setInfo = loadInfo(cset);
            uint64_t otherInfo = loadInfo(other);
            uint32_t setFirst = cset->variants.range.firstChar;
            uint32_t otherFirst = other->variants.range.firstChar;
            if ((setInfo & kInfoInverted) != 0) {
                if ((otherInfo & kInfoInverted) != 0) {
                    if (otherFirst > setFirst) {
                        return false;
                    }
                    return (OCFIndex)(cset->variants.range.length + setFirst) <=
                           (OCFIndex)(other->variants.range.length + otherFirst);
                }
                if ((OCFIndex)(other->variants.range.length + otherFirst) <= (OCFIndex)setFirst) {
                    return true;
                }
                return (OCFIndex)(cset->variants.range.length + setFirst) <= (OCFIndex)otherFirst;
            }
            if ((otherInfo & kInfoInverted) != 0) {
                if (setFirst == 0 && cset->variants.range.length == 0x110000) {
                    return true;
                }
                if (otherFirst == 0 && setFirst >= (uint32_t)other->variants.range.length) {
                    return true;
                }
                if ((OCFIndex)(cset->variants.range.length + setFirst) > (OCFIndex)otherFirst) {
                    return false;
                }
                return (OCFIndex)(other->variants.range.length + otherFirst) == 0x110000;
            }
            if (otherFirst < setFirst) {
                return false;
            }
            return (OCFIndex)(cset->variants.range.length + setFirst) >=
                   (OCFIndex)(other->variants.range.length + otherFirst);
        }
        const CharacterSetAnnex *setAnnex = cset->annex;
        uint32_t setValidPlanes = setAnnex != NULL ? setAnnex->validPlanes : 0;
        const CharacterSetAnnex *otherAnnex = other->annex;
        uint32_t otherPlanes = otherAnnex != NULL ? otherAnnex->validPlanes : 0;
        bool setAnnexNotInverted = setAnnex == NULL || setAnnex->isInverted == 0;
        uint32_t setPlanes;
        bool otherAnnexNotInverted;
        if (otherAnnex != NULL) {
            uint8_t otherAnnexInverted = otherAnnex->isInverted;
            setPlanes = setAnnexNotInverted ? setValidPlanes : 0x1FFFE & ~setValidPlanes;
            otherAnnexNotInverted = otherAnnexInverted == 0;
            otherPlanes = otherAnnexNotInverted ? otherPlanes : 0x1FFFE & ~otherPlanes;
        } else {
            setPlanes = setAnnexNotInverted ? setValidPlanes : 0x1FFFE & ~setValidPlanes;
            otherAnnexNotInverted = true;
        }
        uint8_t setBits[kBitmapSize];
        uint8_t otherBits[kBitmapSize];
        __CFCSetGetBitmap(cset, setBits);
        __CFCSetGetBitmap(other, otherBits);
        for (uint32_t index = 0; index != kBitmapSize / 4; ++index) {
            if ((loadWord(otherBits, index) & ~loadWord(setBits, index)) != 0) {
                return false;
            }
        }
        if (otherPlanes == 0) {
            return true;
        }
        if ((otherPlanes & ~setPlanes) != 0) {
            return false;
        }
        const uint8_t *uniformBitmap = setAnnexNotInverted ? kFullBitmap : NULL;
        uint32_t setMask = setAnnexNotInverted ? 0 : 0xFFFFFFFF;
        uint32_t otherMask = otherAnnexNotInverted ? 0xFFFFFFFF : 0;
        for (uint32_t plane = 1; plane != 17; ++plane) {
            OCFCharacterSetRef setPlane = annexGetPlane(cset, (uint8_t)plane);
            if (setPlane == NULL) {
                continue;
            }
            OCFCharacterSetRef otherPlane = annexGetPlane(other, (uint8_t)plane);
            if (otherPlane != NULL) {
                __CFCSetGetBitmap(setPlane, setBits);
                __CFCSetGetBitmap(otherPlane, otherBits);
                for (uint32_t index = 0; index != kBitmapSize / 4; ++index) {
                    if (((loadWord(setBits, index) ^ setMask) | (loadWord(otherBits, index) ^ otherMask)) !=
                        0xFFFFFFFF) {
                        return false;
                    }
                }
                continue;
            }
            if (!otherAnnexNotInverted) {
                __CFCSetGetBitmap(setPlane, setBits);
                if (!__CFCSetIsEqualBitmap(setBits, uniformBitmap)) {
                    return false;
                }
            }
        }
        return true;
    }
general:
    if (setIsObjC && otherIsObjC) {
        if (foundationSwiftCharacterSetEnabled() || ((uintptr_t)theSet >> 63) != 0 ||
            __CF_IS_OBJC(kCharacterSetTypeID, theSet) != 0) {
            return ((Boolean (*)(id, SEL, OCFCharacterSetRef))objc_msgSend)(
                (id)theSet, sel_getUid("isSupersetOfSet:"), theOtherset
            );
        }
    }
    OCFMutableCharacterSetRef copy = __CFCharacterSetCreateCopy(kOCFAllocatorSystemDefault, theSet, true);
    OCFCharacterSetIntersect(copy, theOtherset);
    Boolean result = __CFCharacterSetEqual(copy, theOtherset);
    OCFRelease(copy);
    return result;
}

// Native __CFCSetGetBitmap, 0x1c0176458. Non-external. Fills a 0x2000-byte plane-0 bitmap: a bitmap set's
// bits are copied; otherwise the buffer is set to the inverted bit, an empty set stops there, and the class
// adds its characters: builtin through CFUniCharGetBitmapForPlane (an empty answer for an inverted set clears
// the buffer, a full answer for a non-inverted one fills it), a BMP range bit by bit (removed when inverted),
// string characters (cleared when inverted), compact blocks expanded.
static void __CFCSetGetBitmap(OCFCharacterSetRef theSet, uint8_t *bitmap) {
    const CharacterSet *cset = theSet;
    if ((loadInfo(cset) & kInfoClassMask) == 0x30 && cset->variants.bitmap != NULL) {
        memmove(bitmap, cset->variants.bitmap, kBitmapSize);
        return;
    }
    uint64_t info = loadInfo(cset);
    bool inverted = (info & kInfoInverted) != 0;
    memset(bitmap, inverted ? -1 : 0, kBitmapSize);
    if (isEmpty(cset)) {
        return;
    }
    switch (classOf(cset)) {
        case kClassBuiltin: {
            uint8_t result =
                CFUniCharGetBitmapForPlane((uint32_t)cset->variants.builtinType, 0, bitmap, inverted);
            if (result == 0xFF && inverted) {
                memset(bitmap, 0, kBitmapSize);
            } else if (result == 1 && !inverted) {
                memset(bitmap, 0xFF, kBitmapSize);
            }
            return;
        }
        case kClassRange: {
            uint32_t firstChar = cset->variants.range.firstChar;
            if ((firstChar >> 16) != 0) {
                return;
            }
            OCFIndex length = cset->variants.range.length;
            uint32_t extent =
                length + (OCFIndex)firstChar < 0x10000 ? (uint32_t)length : (uint32_t)-firstChar;
            uint32_t lastChar = firstChar + extent - 1;
            if (inverted) {
                bitmapRemoveCharactersInRange(bitmap, (UniChar)firstChar, (UniChar)lastChar);
            } else {
                bitmapAddCharactersInRange(bitmap, (UniChar)firstChar, (UniChar)lastChar);
            }
            return;
        }
        case kClassString: {
            OCFIndex length = cset->variants.string.length;
            const UniChar *characters = cset->variants.string.buffer;
            while (length != 0) {
                UniChar character = *characters++;
                uint8_t mask = (uint8_t)(1u << (character & 7));
                uint8_t byte = bitmap[character >> 3];
                bitmap[character >> 3] = inverted ? (uint8_t)(byte & ~mask) : (uint8_t)(byte | mask);
                --length;
            }
            return;
        }
        case kClassCompactBitmap: {
            const uint8_t *compact = cset->variants.bitmap;
            const uint8_t *stored = compact + 0x100;
            for (uint32_t index = 0; index != 0x100; ++index) {
                uint8_t header = compact[index];
                if ((uint8_t)(header + 1) > 1) {
                    memcpy(bitmap, stored, 32);
                    stored += 32;
                } else {
                    memset(bitmap, header, 32);
                }
                bitmap += 32;
            }
            return;
        }
        default:
            return;
    }
}

// Native __CFCSetIsEqualBitmap, 0x1c01767bc. Non-external. NULL stands for an all-clear bitmap and
// (UInt32 *)-1 for an all-set one; equal pointers are equal; otherwise word-by-word.
static Boolean __CFCSetIsEqualBitmap(const uint8_t *bits1, const uint8_t *bits2) {
    if (bits1 == bits2) {
        return true;
    }
    if (bits1 == NULL || bits2 == NULL) {
        if (((uintptr_t)bits1 | (uintptr_t)bits2) == 0) {
            return true;
        }
        const uint8_t *bits = bits2 != NULL ? bits2 : bits1;
        if (bits == kFullBitmap) {
            return false;
        }
        for (uint32_t index = 0; index != kBitmapSize / 4; ++index) {
            if (loadWord(bits, index) != 0) {
                return false;
            }
        }
        return true;
    }
    if (bits1 == kFullBitmap) {
        for (uint32_t index = 0; index != kBitmapSize / 4; ++index) {
            if (loadWord(bits2, index) != 0xFFFFFFFF) {
                return false;
            }
        }
        return true;
    }
    if (bits2 == kFullBitmap) {
        for (uint32_t index = 0; index != kBitmapSize / 4; ++index) {
            if (loadWord(bits1, index) != 0xFFFFFFFF) {
                return false;
            }
        }
        return true;
    }
    for (uint32_t index = 0; index != kBitmapSize / 4; ++index) {
        if (loadWord(bits1, index) != loadWord(bits2, index)) {
            return false;
        }
    }
    return true;
}

// The Objective-C other-set prelude of Union and Intersect: a tagged or Objective-C set is expanded through
// -_expandedCFCharacterSet (the Swift feature or __CF_IS_OBJC is tested again first); NULL means the caller
// combines with the set's bitmap representation instead. A CF set is returned as is.
static OCFCharacterSetRef expandedOtherSet(OCFCharacterSetRef theOtherSet, bool *useRepresentation) {
    *useRepresentation = false;
    if (((uintptr_t)theOtherSet >> 63) == 0 && __CF_IS_OBJC(kCharacterSetTypeID, theOtherSet) == 0) {
        return theOtherSet;
    }
    bool expand = ((uintptr_t)theOtherSet >> 63) != 0;
    if (!expand) {
        (void)foundationSwiftCharacterSetEnabled();
        expand = (foundationSwiftCharacterSetFeatureEnabled & 1) != 0 ||
                 __CF_IS_OBJC(kCharacterSetTypeID, theOtherSet) != 0;
    } else {
        (void)foundationSwiftCharacterSetEnabled();
    }
    if (expand) {
        OCFCharacterSetRef expanded = ((OCFCharacterSetRef (*)(id, SEL))objc_msgSend)(
            (id)theOtherSet, sel_getUid("_expandedCFCharacterSet")
        );
        if (expanded != NULL) {
            return expanded;
        }
    }
    *useRepresentation = true;
    return theOtherSet;
}

// Native CFCharacterSetIntersect, 0x1c0176880, with .cold.1-.cold.3 (0x1c0306ecc-0x1c0306ed4). External.
// Control flow below follows the assembly blocks; the plane loops store, invert and intersect annex planes as
// noted inline. An Objective-C other set without an expansion intersects plane 0 with its bitmap
// representation (kCFAllocatorSystemDefault) and returns without the expanded-set check.
// Folds the compiler-outlined piece _CFCharacterSetIntersect.cold.2 (0x1c0306ed0).
void OCFCharacterSetIntersect(OCFMutableCharacterSetRef theSet, OCFCharacterSetRef theOtherSet) {
    if (characterSetIsObjC(theSet)) {
        ((void (*)(id, SEL, OCFCharacterSetRef))objc_msgSend)(
            (id)theSet, sel_getUid("formIntersectionWithCharacterSet:"), theOtherSet
        );
        return;
    }
    CharacterSet *cset = theSet;
    uint8_t bitsBuffer[kBitmapSize];
    checkMutable(cset, "void CFCharacterSetIntersect(CFMutableCharacterSetRef, CFCharacterSetRef)");
    if (isStaticPredefinedSet(cset)) {
        return;
    }
    if (isEmpty(cset) && !isInverted(cset)) {
        return;
    }
    bool useRepresentation;
    const CharacterSet *other = expandedOtherSet(theOtherSet, &useRepresentation);
    if (useRepresentation) {
        OCFDataRef representation =
            OCFCharacterSetCreateBitmapRepresentation(kOCFAllocatorSystemDefault, other);
        if (representation != NULL) {
            if (OCFDataGetLength(representation) != 0) {
                const uint8_t *bytes = OCFDataGetBytePtr(representation);
                if (bytes != NULL) {
                    __CFCSetMakeBitmap(cset);
                    uint8_t *bitmap = cset->variants.bitmap;
                    for (uint32_t index = 0; index != kBitmapSize / 4; ++index) {
                        storeWord(bitmap, index, loadWord(bitmap, index) & loadWord(bytes, index));
                    }
                    clearHashValid(cset);
                }
            }
            OCFRelease(representation);
        }
        return;
    }
    if (isEmpty(other)) {
        if (isInverted(other)) {
            goto done;
        }
        if (((loadInfo(cset) & kInfoClassMask) == 0x20 && cset->variants.string.buffer != NULL) ||
            ((loadInfo(cset) & kInfoClassMask) == 0x30 && cset->variants.bitmap != NULL) ||
            ((loadInfo(cset) & kInfoClassMask) == 0x40 && cset->variants.bitmap != NULL)) {
            OCFAllocatorDeallocate(OCFGetAllocator(cset), cset->variants.bitmap);
        }
        setClass(cset, kClassBitmap);
        cset->variants.bitmap = NULL;
        updateInfo(cset, kInfoInverted, 0);
        cset->hashValue = 0;
        updateInfo(cset, 0, kInfoHasHash);
        if (cset->annex != NULL) {
            annexDeallocate(cset);
        }
        goto done;
    }
    if (isEmpty(cset)) {
        // The set is empty and inverted: it becomes a copy of the other set.
        updateInfo(cset, kInfoClassMask, loadInfo(other) & kInfoClassMask);
        updateInfo(cset, kInfoHasHash, loadInfo(other) & kInfoHasHash);
        updateInfo(cset, kInfoInverted, loadInfo(other) & kInfoInverted);
        cset->hashValue = other->hashValue;
        if (other->annex != NULL && other->annex->validPlanes != 0) {
            for (uint32_t plane = 1; plane != 17; ++plane) {
                OCFCharacterSetRef planeSet = annexGetPlane(other, (uint8_t)plane);
                if (planeSet == NULL) {
                    continue;
                }
                OCFMutableCharacterSetRef planeCopy =
                    __CFCharacterSetCreateCopy(OCFGetAllocator(cset), planeSet, true);
                annexPutPlane(cset, (uint8_t)plane, planeCopy);
                OCFRelease(planeCopy);
            }
            if (other->annex != NULL && other->annex->isInverted != 0) {
                CharacterSetAnnex *annex = cset->annex;
                if (annex == NULL) {
                    annex = annexAllocateEmpty(cset);
                }
                annex->isInverted = 1;
            } else if (cset->annex != NULL) {
                cset->annex->isInverted = 0;
            }
        }
        switch (classOf(other)) {
            case kClassBuiltin:
                cset->variants.builtinType = other->variants.builtinType;
                break;
            case kClassRange:
                cset->variants.range.firstChar = other->variants.range.firstChar;
                cset->variants.range.length = other->variants.range.length;
                break;
            case kClassString: {
                OCFIndex length = other->variants.string.length;
                cset->variants.string.length = length;
                UniChar *buffer = cset->variants.string.buffer;
                if (buffer == NULL) {
                    buffer = (UniChar *)OCFAllocatorAllocateTyped(
                        OCFGetAllocator(cset), kStringCapacityBytes, kStringBufferType, 0
                    );
                    cset->variants.string.buffer = buffer;
                    length = cset->variants.string.length;
                }
                memmove(buffer, other->variants.string.buffer, (size_t)(2 * length));
                break;
            }
            case kClassBitmap: {
                uint8_t *bitmap =
                    (uint8_t *)OCFAllocatorAllocateTyped(OCFGetAllocator(cset), kBitmapSize, kBitmapType, 0);
                cset->variants.bitmap = bitmap;
                memmove(bitmap, other->variants.bitmap, kBitmapSize);
                break;
            }
            case kClassCompactBitmap: {
                const uint8_t *compact = other->variants.bitmap;
                uint32_t size = 0x100;
                for (uint32_t index = 0; index != 0x100; ++index) {
                    if ((uint8_t)(compact[index] - 1) < 0xFE) {
                        size += 0x20;
                    }
                }
                uint8_t *bytes =
                    (uint8_t *)OCFAllocatorAllocateTyped(OCFGetAllocator(cset), size, kBitmapType, 0);
                cset->variants.bitmap = bytes;
                memmove(bytes, compact, size);
                break;
            }
            default:
                break;
        }
        goto done;
    }
    __CFCSetMakeBitmap(cset);
    {
        uint8_t *bitmap = cset->variants.bitmap;
        if ((loadInfo(other) & kInfoClassMask) == 0x30) {
            const uint8_t *otherBits = other->variants.bitmap;
            for (uint32_t index = 0; index != kBitmapSize / 4; ++index) {
                storeWord(bitmap, index, loadWord(bitmap, index) & loadWord(otherBits, index));
            }
        } else {
            __CFCSetGetBitmap(other, bitsBuffer);
            for (uint32_t index = 0; index != kBitmapSize / 4; ++index) {
                storeWord(bitmap, index, loadWord(bitmap, index) & loadWord(bitsBuffer, index));
            }
        }
    }
    clearHashValid(cset);
    if (other->annex != NULL && other->annex->validPlanes != 0) {
        OCFMutableCharacterSetRef emptySet = OCFCharacterSetCreateMutable(NULL);
        for (uint32_t plane = 1; plane != 17; ++plane) {
            uint32_t bit = 1u << plane;
            OCFMutableCharacterSetRef otherPlane =
                (OCFMutableCharacterSetRef)annexGetPlane(other, (uint8_t)plane);
            if (otherPlane != NULL) {
                if (other->annex->isInverted != 0) {
                    OCFCharacterSetInvert(otherPlane);
                }
                CharacterSet *target = annexGetOrCreatePlane(cset, (uint8_t)plane);
                if (cset->annex->isInverted != 0) {
                    OCFCharacterSetInvert(target);
                }
                OCFCharacterSetIntersect(target, otherPlane);
                if (annexIsInverted(cset)) {
                    OCFCharacterSetInvert(target);
                }
                if (annexIsInverted(other)) {
                    OCFCharacterSetInvert(otherPlane);
                }
                if (isEmpty(target) && !isInverted(target)) {
                    annexPutPlane(cset, (uint8_t)plane, NULL);
                }
                continue;
            }
            CharacterSetAnnex *annex = cset->annex;
            if (annex == NULL) {
                continue;
            }
            if ((annex->validPlanes & bit) != 0 && annex->planeCount >= plane &&
                annex->planes[plane - 1] != NULL) {
                CharacterSet *target = (CharacterSet *)annex->planes[plane - 1];
                if (annex->isInverted == 0) {
                    OCFRelease(target);
                    cset->annex->validPlanes &= ~bit;
                    continue;
                }
                OCFCharacterSetInvert(target);
                OCFCharacterSetIntersect(target, emptySet);
                OCFCharacterSetInvert(target);
                continue;
            }
            if (annex->isInverted == 0) {
                continue;
            }
            CharacterSet *target = annexGetOrCreatePlane(cset, (uint8_t)plane);
            if (cset->annex->isInverted != 0) {
                OCFCharacterSetInvert(target);
            }
            OCFCharacterSetIntersect(target, emptySet);
            if (annexIsInverted(cset)) {
                OCFCharacterSetInvert(target);
            }
        }
        OCFRelease(emptySet);
        if (cset->annex != NULL && cset->annex->validPlanes == 0) {
            annexDeallocate(cset);
        }
        goto done;
    }
    if ((loadInfo(other) & kInfoClassMask) == 0 && !annexIsInverted(other)) {
        for (uint32_t plane = 1; plane != 17; ++plane) {
            CharacterSet *planeSet = (CharacterSet *)annexGetPlane(cset, (uint8_t)plane);
            if (planeSet == NULL) {
                continue;
            }
            uint8_t result =
                CFUniCharGetBitmapForPlane((uint32_t)other->variants.builtinType, plane, bitsBuffer, false);
            if (result == 0) {
                __CFCSetMakeBitmap(planeSet);
                bool allClear = true;
                uint8_t *bitmap = planeSet->variants.bitmap;
                for (uint32_t index = 0; index != kBitmapSize / 4; ++index) {
                    uint32_t word = loadWord(bitmap, index) & loadWord(bitsBuffer, index);
                    storeWord(bitmap, index, word);
                    if (word != 0) {
                        allClear = false;
                    }
                }
                if (allClear) {
                    annexPutPlane(cset, (uint8_t)plane, NULL);
                }
            } else if (result == 0xFF) {
                annexPutPlane(cset, (uint8_t)plane, NULL);
            }
        }
        if (cset->annex != NULL && cset->annex->validPlanes == 0) {
            annexDeallocate(cset);
        }
        goto done;
    }
    if ((loadInfo(other) & kInfoClassMask) != 0x10) {
        CharacterSetAnnex *annex = cset->annex;
        if (annex == NULL || (annex->validPlanes == 0 && annex->isInverted == 0)) {
            goto done;
        }
        if (annexIsInverted(other)) {
            goto done;
        }
        annexDeallocate(cset);
        goto done;
    }
    {
        CharacterSet *rangePlanes = OCFCharacterSetCreateMutable(OCFGetAllocator(cset));
        OCFMutableCharacterSetRef emptySet = OCFCharacterSetCreateMutable(NULL);
        __CFCSetAddNonBMPPlanesInRange(
            rangePlanes, (OCFRange){other->variants.range.firstChar, other->variants.range.length}
        );
        bool selfInverted = annexIsInverted(cset);
        bool rangeNotInverted = !annexIsInverted(rangePlanes);
        for (uint32_t plane = 1; plane != 17; ++plane) {
            uint32_t bit = 1u << plane;
            OCFMutableCharacterSetRef rangePlane =
                (OCFMutableCharacterSetRef)annexGetPlane(rangePlanes, (uint8_t)plane);
            if (rangePlane == NULL) {
                CharacterSetAnnex *annex = cset->annex;
                if (annex == NULL) {
                    continue;
                }
                CharacterSet *target;
                if ((annex->validPlanes & bit) != 0 && annex->planeCount >= plane &&
                    annex->planes[plane - 1] != NULL) {
                    target = (CharacterSet *)annex->planes[plane - 1];
                    if (!selfInverted) {
                        OCFRelease(target);
                        cset->annex->validPlanes &= ~bit;
                        continue;
                    }
                } else if (selfInverted) {
                    target = annexGetOrCreatePlane(cset, (uint8_t)plane);
                } else {
                    continue;
                }
                OCFCharacterSetInvert(target);
                OCFCharacterSetIntersect(target, emptySet);
                OCFCharacterSetInvert(target);
                continue;
            }
            CharacterSet *target = annexGetOrCreatePlane(cset, (uint8_t)plane);
            if (!rangeNotInverted) {
                OCFCharacterSetInvert(rangePlane);
            }
            if (selfInverted) {
                OCFCharacterSetInvert(target);
                OCFCharacterSetIntersect(target, rangePlane);
                OCFCharacterSetInvert(target);
            } else {
                OCFCharacterSetIntersect(target, rangePlane);
            }
            if (!rangeNotInverted) {
                OCFCharacterSetInvert(rangePlane);
            }
            if (isEmpty(target) && !isInverted(target)) {
                annexPutPlane(cset, (uint8_t)plane, NULL);
            }
        }
        if (cset->annex != NULL && cset->annex->validPlanes == 0) {
            annexDeallocate(cset);
        }
        OCFRelease(rangePlanes);
        OCFRelease(emptySet);
    }
done:
    if (__CFCheckForExapendedSet == 1) {
        __CFCheckForExpandedSet(cset);
    }
}

// Native CFCharacterSetHasMemberInPlane, 0x1c0177ea8, with .cold.1 (0x1c0306ee4, the dispatch_once of the
// Swift feature through OUTLINED_FUNCTION_0_3) (corpus delta capture 7). External. The info word is loaded
// (acquire) first; a plane above 16 (unsigned) answers false before the dispatch. The Swift feature (byte
// compared with 1), a tagged pointer or an Objective-C object of type 25 is sent -hasMemberInPlane:.
// Otherwise the answer starts as the inverted bit of that first load; an empty set answers it. Builtin sets:
// type 1 answers true for plane 14 or when inverted, else whether CFUniCharGetBitmapPtrForPlane(1, plane) is
// not NULL; types below 4 and type 15 answer true for plane 0 and the inverted bit otherwise; type 12 answers
// true unless inverted with a plane from 3 to 13 ((plane - 14) unsigned at least -11); any other type answers
// true when inverted, else whether CFUniCharGetBitmapPtrForPlane(type, plane) is not NULL. Range sets (first
// and length read as 32 bits; the last character's plane from their 32-bit sum minus one): not inverted,
// whether the plane lies between the first and last characters' planes; inverted, true outside them, false
// strictly inside, and at the boundaries as the assembly computes below. Other classes: plane 0 answers
// whether a bitmap or compact bitmap pointer is set, the inverted bit for an empty string set, true
// otherwise; a nonzero plane answers false without an annex, the annex inversion when the plane is absent,
// and for a present plane set with an inverted annex: a range plane whether it is not exactly the whole
// plane (first 0, length 0x10000), a bitmap plane whether its bitmap is not all set (__CFCSetIsEqualBitmap
// with -1), any other plane whether the 0x2000-byte __CFCSetGetBitmap result has a word that is not all set;
// a present plane with an annex that is not inverted answers true (the annex is reread after
// __CFCSetGetBitmap). The native code tests the stack buffer's address against -1 before the word loop
// (always false); the test is omitted.
Boolean OCFCharacterSetHasMemberInPlane(OCFCharacterSetRef theSet, OCFIndex thePlane) {
    const CharacterSet *cset = theSet;
    uint64_t info = loadInfo(cset);
    if ((uint64_t)thePlane > 16) {
        return false;
    }
    if (foundationSwiftCharacterSetEnabled() || ((uintptr_t)theSet >> 63) != 0 ||
        __CF_IS_OBJC(kCharacterSetTypeID, theSet) != 0) {
        return ((Boolean (*)(id, SEL, OCFIndex))objc_msgSend)(
            (id)theSet, sel_getUid("hasMemberInPlane:"), thePlane
        );
    }
    bool inverted = (info & kInfoInverted) != 0;
    if (isEmpty(cset)) {
        return inverted;
    }
    if ((loadInfo(cset) & kInfoClassMask) == 0) {
        OCFIndex type = cset->variants.builtinType;
        if (type == 1) {
            if (thePlane == 14 || inverted) {
                return true;
            }
        } else if (type < 4 || type == 15) {
            return thePlane == 0 || inverted;
        } else if (type == 12) {
            return !inverted || (uint64_t)(thePlane - 14) < UINT64_C(0xFFFFFFFFFFFFFFF5);
        } else if (inverted) {
            return true;
        }
        return OCFUniCharGetBitmapPtrForPlane((uint32_t)type, (uint32_t)thePlane) != NULL;
    }
    if ((loadInfo(cset) & kInfoClassMask) == 0x10) {
        uint32_t firstChar = cset->variants.range.firstChar;
        uint32_t lastChar = firstChar + (uint32_t)cset->variants.range.length - 1;
        uint64_t firstPlane = firstChar >> 16;
        uint64_t lastPlane = lastChar >> 16;
        uint64_t plane = (uint64_t)thePlane;
        if (!inverted) {
            return plane >= firstPlane && plane <= lastPlane;
        }
        if (plane < firstPlane || plane > lastPlane) {
            return true;
        }
        if (plane > firstPlane && plane < lastPlane) {
            return false;
        }
        uint32_t firstLow = firstChar & 0xFFFF;
        uint32_t lastLow = lastChar & 0xFFFF;
        if (plane == firstPlane) {
            if (firstLow != 0) {
                return true;
            }
            if ((uint32_t)firstPlane != (uint32_t)lastPlane) {
                return false;
            }
            return lastLow != 0xFFFF;
        }
        if (lastLow != 0xFFFF) {
            return true;
        }
        return firstLow != 0 && (uint32_t)firstPlane == (uint32_t)lastPlane;
    }
    if (thePlane == 0) {
        uint32_t cls = classOf(cset);
        if (cls == kClassCompactBitmap || cls == kClassBitmap) {
            return cset->variants.bitmap != NULL;
        }
        if (cls == kClassString && cset->variants.string.length == 0) {
            return inverted;
        }
        return true;
    }
    const CharacterSetAnnex *annex = cset->annex;
    if (annex == NULL) {
        return false;
    }
    OCFCharacterSetRef planeSet = NULL;
    if (((annex->validPlanes >> (uint32_t)thePlane) & 1) != 0 && annex->planeCount >= (uint8_t)thePlane) {
        planeSet = annex->planes[thePlane - 1];
    }
    if (planeSet == NULL) {
        return annex->isInverted != 0;
    }
    if ((loadInfo(planeSet) & kInfoClassMask) == 0x10) {
        annex = cset->annex;
        if (annex == NULL || annex->isInverted == 0 || planeSet->variants.range.firstChar != 0) {
            return true;
        }
        return planeSet->variants.range.length != 0x10000;
    }
    if ((loadInfo(planeSet) & kInfoClassMask) == 0x30) {
        annex = cset->annex;
        if (annex == NULL || annex->isInverted == 0) {
            return true;
        }
        return !__CFCSetIsEqualBitmap(planeSet->variants.bitmap, kFullBitmap);
    }
    uint8_t bitmap[kBitmapSize];
    __CFCSetGetBitmap(planeSet, bitmap);
    annex = cset->annex;
    if (annex == NULL || annex->isInverted == 0) {
        return true;
    }
    for (uint32_t index = 0; index != kBitmapSize / 4; ++index) {
        if (loadWord(bitmap, index) != 0xFFFFFFFF) {
            return true;
        }
    }
    return false;
}

// Native CFCharacterSetCreateBitmapRepresentation, 0x1c0178234, with .cold.1 (0x1c0306ee8). External. The
// data holds the plane-0 bitmap, then for each further plane a plane-number byte and 0x2000 bitmap bytes.
// With annex planes: the listed planes are those present (all 16 when the annex is inverted); each is the
// plane set's bitmap (complemented when the annex is inverted) or uniform. Builtin sets list the non-empty
// planes of CFUniCharGetBitmapForPlane (16 slots when inverted, otherwise the table's plane count - 1) and
// shrink the data to what was written. Range sets write their planes directly; an inverted annex without
// planes writes 16 full planes. CoreFoundation defects (kept, well-defined): for an inverted range spanning
// planes, the last partial plane is labeled with the first partial plane's number, and its added range starts
// at the range's last character instead of the one after it.
OCFDataRef OCFCharacterSetCreateBitmapRepresentation(OCFAllocatorRef alloc, OCFCharacterSetRef theSet) {
    if (characterSetIsObjC(theSet)) {
        return ((OCFDataRef (*)(id, SEL))objc_msgSend)(
            (id)theSet, sel_getUid("_retainedBitmapRepresentation")
        );
    }
    const CharacterSet *cset = theSet;
    const CharacterSetAnnex *annex = cset->annex;
    bool annexInverted = false;
    if (annex != NULL) {
        uint8_t invertedByte = annex->isInverted;
        annexInverted = invertedByte != 0;
        uint32_t validPlanes = annex->validPlanes;
        if (validPlanes != 0) {
            uint8_t planes[16];
            int32_t count = 0;
            for (uint32_t index = 0; index != 16; ++index) {
                uint32_t plane = index + 1;
                if (invertedByte != 0 || (((validPlanes >> plane) & 1) != 0 && plane <= annex->planeCount &&
                                          annex->planes[index] != NULL)) {
                    planes[count++] = (uint8_t)plane;
                }
            }
            OCFIndex length = (int32_t)(count + (count << 13) + 0x2000);
            OCFMutableDataRef data = OCFDataCreateMutable(alloc, length);
            OCFDataSetLength(data, length);
            __CFCSetGetBitmap(cset, OCFDataGetMutableBytePtr(data));
            if (count > 0) {
                uint8_t *bytes = OCFDataGetMutableBytePtr(data);
                if (cset->annex != NULL && cset->annex->validPlanes != 0) {
                    uint8_t fill = invertedByte != 0 ? 0xFF : 0;
                    for (int32_t index = 0; index != count; ++index) {
                        uint8_t plane = planes[index];
                        bytes[0x2000] = plane;
                        if (plane == 0) {
                            __builtin_trap();
                        }
                        bytes += 0x2001;
                        OCFCharacterSetRef planeSet = plane <= 16 ? annexGetPlane(cset, plane) : NULL;
                        if (planeSet != NULL) {
                            __CFCSetGetBitmap(planeSet, bytes);
                            if (invertedByte != 0) {
                                for (uint32_t offset = 0; offset != kBitmapSize; ++offset) {
                                    bytes[offset] = (uint8_t)~bytes[offset];
                                }
                            }
                        } else {
                            memset(bytes, fill, kBitmapSize);
                        }
                    }
                }
            }
            return data;
        }
    }
    uint64_t info = loadInfo(cset);
    uint64_t classInfo = loadInfo(cset);
    if ((info & kInfoClassMask) == 0) {
        int32_t count = (classInfo & kInfoInverted) != 0
                            ? 16
                            : (int32_t)(CFUniCharGetNumberOfPlanes((uint32_t)cset->variants.builtinType) - 1);
        OCFIndex length = (int32_t)(count + (count << 13) + 0x2000);
        OCFMutableDataRef data = OCFDataCreateMutable(alloc, length);
        OCFDataSetLength(data, length);
        __CFCSetGetBitmap(cset, OCFDataGetMutableBytePtr(data));
        if (count >= 1) {
            uint8_t *bytes = OCFDataGetMutableBytePtr(data) + 0x2000;
            bool inverted = isInverted(cset);
            uint32_t plane = 1;
            do {
                uint8_t result = CFUniCharGetBitmapForPlane(
                    (uint32_t)cset->variants.builtinType, plane, bytes + 1, inverted
                );
                if (result != 0xFF) {
                    bytes[0] = (uint8_t)plane;
                    if (result == 1) {
                        memset(bytes + 1, 0xFF, kBitmapSize);
                    }
                    bytes += 0x2001;
                }
                ++plane;
            } while (--count != 0);
            OCFIndex written = bytes - OCFDataGetBytePtr(data);
            if (written < length) {
                OCFDataSetLength(data, written);
            }
        }
        return data;
    }
    if ((classInfo & kInfoClassMask) == 0x10) {
        uint32_t firstChar = cset->variants.range.firstChar;
        uint32_t lastChar = firstChar + (uint32_t)cset->variants.range.length - 1;
        int32_t count;
        if (lastChar >= 0x10000) {
            uint32_t lastPlane = lastChar >> 16;
            uint32_t clampedFirst = firstChar > 0x10000 ? firstChar : 0x10000;
            uint32_t firstPlane = clampedFirst >> 16;
            uint32_t span = lastPlane - firstPlane;
            uint32_t fewer = 15 - span;
            uint32_t more = 16 - span;
            uint32_t partialCount = (clampedFirst & 0xFFFF) == 0 ? fewer : more;
            if ((0xFFFF & ~lastChar) != 0) {
                ++partialCount;
            }
            uint32_t samePlaneCount = ((clampedFirst & 0xFFFF) | (0xFFFF & ~lastChar)) == 0 ? fewer : more;
            uint32_t invertedCount = lastPlane != firstPlane ? partialCount : samePlaneCount;
            count = (int32_t)((loadInfo(cset) & kInfoInverted) != 0 ? invertedCount : span + 1);
        } else {
            count = (loadInfo(cset) & kInfoInverted) != 0 ? 16 : 0;
        }
        OCFIndex length = (int32_t)(count + (count << 13) + 0x2000);
        OCFMutableDataRef data = OCFDataCreateMutable(alloc, length);
        OCFDataSetLength(data, length);
        __CFCSetGetBitmap(cset, OCFDataGetMutableBytePtr(data));
        if (count < 1) {
            return data;
        }
        uint8_t *bytes = OCFDataGetMutableBytePtr(data);
        firstChar = cset->variants.range.firstChar;
        lastChar = firstChar + (uint32_t)cset->variants.range.length - 1;
        uint32_t lastPlane = lastChar >> 16;
        uint32_t clampedFirst = firstChar > 0x10000 ? firstChar : 0x10000;
        uint32_t firstPlane = clampedFirst >> 16;
        if ((loadInfo(cset) & kInfoInverted) == 0) {
            if (firstPlane > lastPlane) {
                return data;
            }
            uint8_t *planeBytes = bytes + 0x2001;
            uint32_t planeCount = lastPlane - firstPlane + 1;
            uint32_t plane = firstPlane;
            for (uint32_t index = 0; index != planeCount; ++index) {
                planeBytes[-1] = (uint8_t)plane;
                uint32_t rangeFirst = index == 0 ? clampedFirst : 0;
                uint32_t rangeLast = index == lastPlane - firstPlane ? lastChar : 0xFFFFFFFF;
                bitmapAddCharactersInRange(planeBytes, (UniChar)rangeFirst, (UniChar)rangeLast);
                ++plane;
                planeBytes += 0x2001;
            }
            return data;
        }
        uint8_t *header = bytes + 0x2000;
        UniChar firstLow = (UniChar)clampedFirst;
        uint32_t plane = 1;
        if (firstChar >= 0x20000) {
            for (uint32_t remaining = firstPlane - 1; remaining != 0; --remaining) {
                header[0] = (uint8_t)plane;
                memset(header + 1, 0xFF, kBitmapSize);
                header += 0x2001;
                ++plane;
            }
        }
        UniChar lastLow = (UniChar)lastChar;
        if (firstPlane == lastPlane) {
            if (firstLow != 0 || lastLow != 0xFFFF) {
                header[0] = (uint8_t)plane;
                memset(header + 1, 0xFF, kBitmapSize);
                bitmapRemoveCharactersInRange(header + 1, (UniChar)clampedFirst, (UniChar)lastChar);
                header += 0x2001;
            }
        } else if (firstPlane < lastPlane) {
            if (firstLow != 0) {
                header[0] = (uint8_t)plane;
                memset(header + 1, 0, kBitmapSize);
                bitmapAddCharactersInRange(header + 1, 0, (UniChar)(clampedFirst - 1));
                header += 0x2001;
            }
            if (lastLow != 0xFFFF) {
                header[0] = (uint8_t)plane;
                memset(header + 1, 0, kBitmapSize);
                bitmapAddCharactersInRange(header + 1, lastLow, 0xFFFF);
                header += 0x2001;
            }
        }
        if ((lastChar >> 20) == 0) {
            uint32_t trailingPlane = lastPlane + 1;
            for (uint32_t remaining = 16 - lastPlane; remaining != 0; --remaining) {
                header[0] = (uint8_t)trailingPlane;
                memset(header + 1, 0xFF, kBitmapSize);
                header += 0x2001;
                ++trailingPlane;
            }
        }
        return data;
    }
    if (annexInverted) {
        OCFMutableDataRef data = OCFDataCreateMutable(alloc, 0x22010);
        OCFDataSetLength(data, 0x22010);
        __CFCSetGetBitmap(cset, OCFDataGetMutableBytePtr(data));
        uint8_t *header = OCFDataGetMutableBytePtr(data) + 0x2000;
        for (uint32_t plane = 1; plane != 17; ++plane) {
            header[0] = (uint8_t)plane;
            memset(header + 1, 0xFF, kBitmapSize);
            header += 0x2001;
        }
        return data;
    }
    OCFMutableDataRef data = OCFDataCreateMutable(alloc, 0x2000);
    OCFDataSetLength(data, 0x2000);
    __CFCSetGetBitmap(cset, OCFDataGetMutableBytePtr(data));
    return data;
}

// The range validation of the range mutations: location >> 16 at most 16, length below 0x110001 (signed), end
// below 0x110001 (unsigned); otherwise CFLog level 3 and the crash annotation (BRK #1).
static void checkRange(OCFRange theRange, const char *function) {
    uint64_t end = (uint64_t)theRange.location + (uint64_t)theRange.length;
    if (((uint64_t)theRange.location >> 16) > 0x10 || theRange.length >= 0x110001 || end >= 0x110001) {
        OCFLog(
            3,
            OF_STR("%s: Range (location: %ld, length: %ld) outside of valid Unicode range (0x0 - 0x10FFFF)"),
            function,
            (long)theRange.location,
            (long)theRange.length
        );
        crashWithMessage("CFCharacterSet range is outside of valid Unicode range (0x0 - 0x10FFFF)");
    }
}

// Native CFCharacterSetAddCharactersInRange, 0x1c0178b00, with .cold.1 and .cold.2 (0x1c0306eec,
// 0x1c0306ef0). External. After the range check: a static predefined set or an empty range is ignored, as is
// an empty inverted set. A non-inverted empty set becomes the range. A range set merges a range starting at
// its first character (the longer length), one starting inside it or adjacent to its end (extended to the new
// end), or one starting before it and reaching it (first moved, length to the farther end); a string set with
// room for the characters appends them (truncated to 16 bits), sorts and deduplicates. Everything else
// becomes a bitmap: non-BMP planes through __CFCSetAddNonBMPPlanesInRange, then the BMP part bit by bit, then
// the expanded-set check. Every change clears the hash.
void OCFCharacterSetAddCharactersInRange(OCFMutableCharacterSetRef theSet, OCFRange theRange) {
    if (characterSetIsObjC(theSet)) {
        ((void (*)(id, SEL, OCFRange))objc_msgSend)(
            (id)theSet, sel_getUid("addCharactersInRange:"), theRange
        );
        return;
    }
    CharacterSet *cset = theSet;
    const char *function = "void CFCharacterSetAddCharactersInRange(CFMutableCharacterSetRef, CFRange)";
    checkMutable(cset, function);
    OCFIndex location = theRange.location;
    OCFIndex length = theRange.length;
    checkRange(theRange, function);
    uint64_t end = (uint64_t)location + (uint64_t)length;
    if (isStaticPredefinedSet(cset)) {
        return;
    }
    if (length == 0) {
        return;
    }
    if (isInverted(cset) && isEmpty(cset)) {
        return;
    }
    if (!isInverted(cset)) {
        if (isEmpty(cset)) {
            setClass(cset, kClassRange);
            cset->variants.range.firstChar = (uint32_t)location;
            cset->variants.range.length = length;
            clearHashValid(cset);
            return;
        }
        if ((loadInfo(cset) & kInfoClassMask) == 0x10) {
            uint64_t firstChar = cset->variants.range.firstChar;
            OCFIndex rangeLength = cset->variants.range.length;
            if ((uint64_t)location == firstChar) {
                cset->variants.range.length = rangeLength > length ? rangeLength : length;
                clearHashValid(cset);
                return;
            }
            OCFIndex rangeEnd = rangeLength + (OCFIndex)firstChar;
            if ((uint64_t)location > firstChar && location <= rangeEnd) {
                if ((uint64_t)rangeEnd < end) {
                    cset->variants.range.length = (OCFIndex)(end - firstChar);
                }
                clearHashValid(cset);
                return;
            }
            if ((uint64_t)location < firstChar && end >= firstChar) {
                cset->variants.range.firstChar = (uint32_t)location;
                OCFIndex farther = rangeEnd > (OCFIndex)end ? rangeEnd : (OCFIndex)end;
                cset->variants.range.length = farther - location;
                clearHashValid(cset);
                return;
            }
        } else if ((loadInfo(cset) & kInfoClassMask) == 0x20) {
            OCFIndex newLength = cset->variants.string.length + length;
            if (newLength <= 0x3F) {
                UniChar *buffer = cset->variants.string.buffer;
                OCFIndex previous = cset->variants.string.length;
                if (buffer == NULL) {
                    buffer = (UniChar *)OCFAllocatorAllocateTyped(
                        OCFGetAllocator(cset), kStringCapacityBytes, kStringBufferType, 0
                    );
                    cset->variants.string.buffer = buffer;
                    previous = cset->variants.string.length;
                    newLength = previous + length;
                }
                cset->variants.string.length = newLength;
                UniChar *cursor = buffer + previous;
                OCFIndex character = location;
                for (OCFIndex remaining = length; remaining != 0; --remaining) {
                    *cursor++ = (UniChar)character++;
                }
                cset->variants.string.length =
                    sortAndUniqueString(cset->variants.string.buffer, cset->variants.string.length);
                clearHashValid(cset);
                return;
            }
        }
    }
    __CFCSetMakeBitmap(cset);
    __CFCSetAddNonBMPPlanesInRange(cset, theRange);
    if (((uint64_t)location >> 16) == 0) {
        uint32_t extent = (end >> 16) == 0 ? (uint32_t)length : (uint32_t)-location;
        uint32_t lastChar = extent + (uint32_t)location - 1;
        bitmapAddCharactersInRange(cset->variants.bitmap, (UniChar)location, (UniChar)lastChar);
    }
    clearHashValid(cset);
    if (__CFCheckForExapendedSet == 1) {
        __CFCheckForExpandedSet(cset);
    }
}

// Native __CFCSetMakeBitmap, 0x1c017909c. Non-external. A bitmap set with bits is left alone. Otherwise a new
// 0x2000-byte bitmap is filled by __CFCSetGetBitmap; a compact or string buffer is freed; a range adds its
// non-BMP planes (and, without annex planes, moves its inverted bit to the annex); a builtin set grows its
// annex to the table's plane count - 1 and gives each non-empty plane a bitmap plane set (a plane above 16
// drops its buffer; a plane number of 0 is BRK #1); an unused plane buffer is freed. The class becomes bitmap
// with the new bits and the inverted bit is cleared. A replaced plane set's previous bits are not freed
// (native behavior).
static void __CFCSetMakeBitmap(CharacterSet *cset) {
    if ((loadInfo(cset) & kInfoClassMask) == 0x30 && cset->variants.bitmap != NULL) {
        return;
    }
    OCFAllocatorRef allocator = OCFGetAllocator(cset);
    uint8_t *bitmap = (uint8_t *)OCFAllocatorAllocateTyped(allocator, kBitmapSize, kBitmapType, 0);
    __CFCSetGetBitmap(cset, bitmap);
    if ((loadInfo(cset) & kInfoClassMask) != 0) {
        if (((loadInfo(cset) & kInfoClassMask) == 0x40 && cset->variants.bitmap != NULL) ||
            ((loadInfo(cset) & kInfoClassMask) == 0x20 && cset->variants.string.buffer != NULL)) {
            OCFAllocatorDeallocate(allocator, cset->variants.bitmap);
            cset->variants.bitmap = NULL;
        } else if ((loadInfo(cset) & kInfoClassMask) == 0x10) {
            if (cset->annex != NULL && cset->annex->validPlanes != 0) {
                __CFCSetAddNonBMPPlanesInRange(
                    cset, (OCFRange){cset->variants.range.firstChar, cset->variants.range.length}
                );
            } else {
                uint64_t info = loadInfo(cset);
                __CFCSetAddNonBMPPlanesInRange(
                    cset, (OCFRange){cset->variants.range.firstChar, cset->variants.range.length}
                );
                if ((info & kInfoInverted) != 0) {
                    CharacterSetAnnex *annex = cset->annex;
                    if (annex == NULL) {
                        annex = annexAllocateEmpty(cset);
                    }
                    annex->isInverted = 1;
                }
            }
        }
    } else {
        uint32_t numberOfPlanes = CFUniCharGetNumberOfPlanes((uint32_t)cset->variants.builtinType);
        if (numberOfPlanes >= 2) {
            uint32_t planeCount = numberOfPlanes - 1;
            CharacterSetAnnex *annex = cset->annex;
            if (annex == NULL) {
                annex =
                    (CharacterSetAnnex *)OCFAllocatorAllocateTyped(OCFGetAllocator(cset), 16, kAnnexType, 0);
                cset->annex = annex;
                annex->planeCount = (uint8_t)planeCount;
                annex->isInverted = 0;
                annex->validPlanes = 0;
                void *planes = NULL;
                if ((planeCount & 0xFF) != 0) {
                    planes = OCFAllocatorAllocateTyped(
                        OCFGetAllocator(cset), (OCFIndex)((planeCount & 0xFF) * 8), kPlanesType, 0
                    );
                    annex = cset->annex;
                }
                annex->planes = (OCFCharacterSetRef *)planes;
            } else if (annex->planeCount < (uint8_t)planeCount) {
                annex->planeCount = (uint8_t)planeCount;
                OCFCharacterSetRef *previous = annex->planes;
                OCFAllocatorRef planeAllocator = OCFGetAllocator(cset);
                void *planes;
                if (previous != NULL) {
                    planes = __OCFSafelyReallocateWithAllocatorTyped(
                        planeAllocator,
                        cset->annex->planes,
                        (OCFIndex)((planeCount & 0xFF) * 8),
                        kPlanesType,
                        0,
                        NULL
                    );
                } else {
                    planes = OCFAllocatorAllocateTyped(
                        planeAllocator, (OCFIndex)((planeCount & 0xFF) * 8), kPlanesType, 0
                    );
                }
                cset->annex->planes = (OCFCharacterSetRef *)planes;
            }
            uint8_t *planeBitmap = NULL;
            for (uint32_t plane = 1; plane != numberOfPlanes; ++plane) {
                if (planeBitmap == NULL) {
                    planeBitmap =
                        (uint8_t *)OCFAllocatorAllocateTyped(allocator, kBitmapSize, kBitmapType, 0);
                }
                uint8_t result = CFUniCharGetBitmapForPlane(
                    (uint32_t)cset->variants.builtinType, plane, planeBitmap, false
                );
                if (result == 0xFF) {
                    continue;
                }
                if (result == 1) {
                    memset(planeBitmap, 0xFF, kBitmapSize);
                }
                uint8_t planeNumber = (uint8_t)plane;
                if (planeNumber == 0) {
                    __builtin_trap();
                }
                if (planeNumber <= 16) {
                    CharacterSet *planeSet = annexGetOrCreatePlane(cset, planeNumber);
                    if (planeSet != NULL) {
                        setClass(planeSet, kClassBitmap);
                        planeSet->variants.bitmap = planeBitmap;
                        updateInfo(planeSet, kInfoInverted, 0);
                        clearHashValid(planeSet);
                    }
                }
                planeBitmap = NULL;
            }
            if (planeBitmap != NULL) {
                OCFAllocatorDeallocate(allocator, planeBitmap);
            }
        }
    }
    setClass(cset, kClassBitmap);
    cset->variants.bitmap = bitmap;
    updateInfo(cset, kInfoInverted, 0);
}

// Native __CFCSetAddNonBMPPlanesInRange, 0x1c017958c. Non-external. For planes max(1, location >> 16) through
// min(16, (end - 1) >> 16): without an inverted annex the range part is added to the plane set (created as
// needed); with one it is removed from an existing plane set, which is released when it becomes empty and is
// not inverted, or a new plane set holding the range part is stored. A plane number of 0 is BRK #1.
// Afterwards an annex without planes and without inversion is freed. CoreFoundation defect (kept,
// well-defined): every plane uses the range's low 16 bits as its first character (the start is not reset to 0
// after the first plane), so a range crossing planes omits the characters below that offset in its later
// planes.
static void __CFCSetAddNonBMPPlanesInRange(CharacterSet *cset, OCFRange range) {
    uint64_t end = (uint64_t)range.location + (uint64_t)range.length;
    uint32_t firstPlane = (uint32_t)((uint64_t)range.location >> 16);
    int32_t lastPlane = (int32_t)((uint32_t)end - 1) >> 16;
    if (lastPlane >= 0x10) {
        lastPlane = 0x10;
    }
    if ((int32_t)firstPlane > 0x10) {
        return;
    }
    bool annexNotInverted = !annexIsInverted(cset);
    uint32_t plane = firstPlane > 1 ? firstPlane : 1;
    if ((int32_t)plane <= lastPlane) {
        OCFIndex rangeFirst = (OCFIndex)((uint64_t)range.location & 0xFFFF);
        uint32_t endLow = (uint32_t)end & 0xFFFF;
        uint64_t endLowWide = end & 0xFFFF;
        uint32_t limit = (uint32_t)lastPlane + 1;
        do {
            uint64_t planeEnd = (endLow != 0 && (uint32_t)lastPlane == plane) ? endLowWide : 0x10000;
            OCFRange planeRange = {rangeFirst, (OCFIndex)(planeEnd - (uint64_t)rangeFirst)};
            uint8_t planeNumber = (uint8_t)plane;
            if (planeNumber == 0) {
                __builtin_trap();
            }
            if (annexNotInverted) {
                if (planeNumber <= 16) {
                    CharacterSet *planeSet = annexGetOrCreatePlane(cset, planeNumber);
                    if (planeSet != NULL) {
                        OCFCharacterSetAddCharactersInRange(planeSet, planeRange);
                    }
                }
            } else {
                CharacterSet *planeSet =
                    planeNumber <= 16 ? (CharacterSet *)annexGetPlane(cset, planeNumber) : NULL;
                if (planeSet != NULL) {
                    OCFCharacterSetRemoveCharactersInRange(planeSet, planeRange);
                    if (isEmpty(planeSet) && !isInverted(planeSet)) {
                        OCFRelease(planeSet);
                        cset->annex->validPlanes &= ~(1u << plane);
                    }
                } else {
                    OCFMutableCharacterSetRef newPlane = OCFCharacterSetCreateMutable(OCFGetAllocator(cset));
                    OCFCharacterSetAddCharactersInRange(newPlane, planeRange);
                    annexPutPlane(cset, planeNumber, newPlane);
                    OCFRelease(newPlane);
                }
            }
            ++plane;
        } while (limit != plane);
    }
    CharacterSetAnnex *annex = cset->annex;
    if (annex != NULL && annex->validPlanes == 0 && annex->isInverted == 0) {
        annexDeallocate(cset);
    }
}

// Native __CFCheckForExpandedSet, 0x1c0179a20, with __CFCheckForExpandedSet.__CFNumberOfPlanesForLogging
// (0x22e6cf4d1, __DATA,__data, initially -1) and .warnedOnce (0x22e6d27b0). Non-external. The plane limit is
// read once from CFCharacterSetCheckForExpandedSet (strtol_l base 0, kept when 1-16, else 0); a set whose
// annex has at least that many valid bits logs a level-4 warning once.
static int8_t __CFNumberOfPlanesForLogging = -1;
static uint8_t warnedOnce;

static void __CFCheckForExpandedSet(OCFCharacterSetRef theSet) {
    const CharacterSet *cset = theSet;
    int8_t limit = __CFNumberOfPlanesForLogging;
    if (limit < 0) {
        const char *value = getenv("CFCharacterSetCheckForExpandedSet");
        long planes = 0;
        if (value != NULL) {
            planes = strtol_l(value, NULL, 0, NULL);
        }
        limit = (uint64_t)(planes - 1) < 0x10 ? (int8_t)planes : 0;
        __CFNumberOfPlanesForLogging = limit;
    }
    if ((uint8_t)limit == 0 || cset->annex == NULL) {
        return;
    }
    uint32_t validPlanes = cset->annex->validPlanes;
    if (validPlanes == 0) {
        return;
    }
    int32_t count = 0;
    for (;;) {
        if ((validPlanes & 1) != 0) {
            ++count;
            if (count >= limit) {
                if ((warnedOnce & 1) == 0) {
                    OCFLog(
                        4,
                        OF_STR(
                            "An expanded CFMutableCharacter has been detected.  "
                            "Recommend to compact with CFCharacterSetCreateCopy"
                        )
                    );
                    warnedOnce = 1;
                }
                return;
            }
        }
        if (validPlanes <= 1) {
            return;
        }
        validPlanes >>= 1;
    }
}

// Native CFCharacterSetRemoveCharactersInRange, 0x1c0179adc, with .cold.2 (0x1c0306f04, the body) and
// .cold.1, .cold.3. External. Mirrors CFCharacterSetAddCharactersInRange: an empty non-inverted set is
// unchanged; an inverted set adds the range to its stored complement (an empty one becomes the range; a range
// set merges; a string set appends); otherwise the set becomes a bitmap and planes and BMP bits are removed;
// an emptied annex without inversion is freed and a fully removed BMP frees the bits. CoreFoundation defects
// (kept, well-defined): an inverted range set merges a range at its first character to the shorter length, a
// range before it to (first - location) + length regardless of the new end, and the planes repeat the
// low-16-bit start (as in
// __CFCSetAddNonBMPPlanesInRange).
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_4_0 (0x1c017bb48), _OUTLINED_FUNCTION_13
// (0x1c017bc3c), _OUTLINED_FUNCTION_14 (0x1c017bc44).
// Folds the compiler-outlined pieces _CFCharacterSetRemoveCharactersInRange.cold.1 (0x1c0306f00),
// _CFCharacterSetRemoveCharactersInRange.cold.3 (0x1c0307604).
void OCFCharacterSetRemoveCharactersInRange(OCFMutableCharacterSetRef theSet, OCFRange theRange) {
    if (characterSetIsObjC(theSet)) {
        ((void (*)(id, SEL, OCFRange))objc_msgSend)(
            (id)theSet, sel_getUid("removeCharactersInRange:"), theRange
        );
        return;
    }
    CharacterSet *cset = theSet;
    const char *function = "void CFCharacterSetRemoveCharactersInRange(CFMutableCharacterSetRef, CFRange)";
    checkMutable(cset, function);
    OCFIndex location = theRange.location;
    OCFIndex length = theRange.length;
    checkRange(theRange, function);
    uint64_t end = (uint64_t)location + (uint64_t)length;
    uint64_t firstPlaneWide = (uint64_t)location >> 16;
    if (isStaticPredefinedSet(cset)) {
        return;
    }
    if (length == 0) {
        return;
    }
    if (!isInverted(cset) && isEmpty(cset)) {
        return;
    }
    if (isInverted(cset)) {
        if (isEmpty(cset)) {
            setClass(cset, kClassRange);
            cset->variants.range.firstChar = (uint32_t)location;
            cset->variants.range.length = length;
            clearHashValid(cset);
            return;
        }
        if ((loadInfo(cset) & kInfoClassMask) == 0x10) {
            uint64_t firstChar = cset->variants.range.firstChar;
            OCFIndex rangeLength = cset->variants.range.length;
            if (firstChar == (uint64_t)location) {
                cset->variants.range.length = rangeLength < length ? rangeLength : length;
                clearHashValid(cset);
                return;
            }
            OCFIndex rangeEnd = rangeLength + (OCFIndex)firstChar;
            if (firstChar < (uint64_t)location && location <= rangeEnd) {
                if ((uint64_t)rangeEnd < end) {
                    cset->variants.range.length = (OCFIndex)(end - firstChar);
                }
                clearHashValid(cset);
                return;
            }
            if (firstChar > (uint64_t)location && end >= firstChar) {
                cset->variants.range.firstChar = (uint32_t)location;
                cset->variants.range.length = (OCFIndex)(firstChar - (uint64_t)location) + rangeLength;
                clearHashValid(cset);
                return;
            }
        } else if ((loadInfo(cset) & kInfoClassMask) == 0x20) {
            OCFIndex newLength = cset->variants.string.length + length;
            if (newLength <= 0x3F) {
                UniChar *buffer = cset->variants.string.buffer;
                OCFIndex previous = cset->variants.string.length;
                if (buffer == NULL) {
                    buffer = (UniChar *)OCFAllocatorAllocateTyped(
                        OCFGetAllocator(cset), kStringCapacityBytes, kStringBufferType, 0
                    );
                    cset->variants.string.buffer = buffer;
                    previous = cset->variants.string.length;
                    newLength = previous + length;
                }
                cset->variants.string.length = newLength;
                UniChar *cursor = buffer + previous;
                OCFIndex character = location;
                for (OCFIndex remaining = length; remaining != 0; --remaining) {
                    *cursor++ = (UniChar)character++;
                }
                cset->variants.string.length =
                    sortAndUniqueString(cset->variants.string.buffer, cset->variants.string.length);
                clearHashValid(cset);
                return;
            }
        }
    }
    __CFCSetMakeBitmap(cset);
    uint32_t plane = (uint32_t)firstPlaneWide > 1 ? (uint32_t)firstPlaneWide : 1;
    int32_t lastPlane = (int32_t)((uint32_t)end - 1) >> 16;
    if ((int32_t)plane <= lastPlane) {
        OCFIndex rangeFirst = (OCFIndex)((uint64_t)location & 0xFFFF);
        uint32_t endLow = (uint32_t)end & 0xFFFF;
        uint64_t endLowWide = end & 0xFFFF;
        uint32_t limit = (uint32_t)lastPlane + 1;
        do {
            uint64_t planeEnd = ((uint32_t)lastPlane == plane && endLow != 0) ? endLowWide : 0x10000;
            OCFRange planeRange = {rangeFirst, (OCFIndex)(planeEnd - (uint64_t)rangeFirst)};
            uint8_t planeNumber = (uint8_t)plane;
            if (annexIsInverted(cset)) {
                if (planeNumber == 0) {
                    __builtin_trap();
                }
                if (planeNumber <= 16) {
                    CharacterSet *planeSet = annexGetOrCreatePlane(cset, planeNumber);
                    if (planeSet != NULL) {
                        OCFCharacterSetAddCharactersInRange(planeSet, planeRange);
                    }
                }
            } else {
                if (planeNumber == 0) {
                    __builtin_trap();
                }
                CharacterSet *planeSet =
                    planeNumber <= 16 ? (CharacterSet *)annexGetPlane(cset, planeNumber) : NULL;
                if (planeSet != NULL) {
                    OCFCharacterSetRemoveCharactersInRange(planeSet, planeRange);
                    if (isEmpty(planeSet) && !isInverted(planeSet)) {
                        OCFRelease(planeSet);
                        cset->annex->validPlanes &= ~(1u << plane);
                    }
                }
            }
            ++plane;
        } while (limit != plane);
    }
    CharacterSetAnnex *annex = cset->annex;
    if (annex != NULL && annex->validPlanes == 0 && annex->isInverted == 0) {
        annexDeallocate(cset);
    }
    if (location < 0x10000) {
        OCFIndex extent = (OCFIndex)end > 0x10000 ? 0x10000 - location : length;
        if (location == 0 && extent == 0x10000) {
            OCFAllocatorDeallocate(OCFGetAllocator(cset), cset->variants.bitmap);
            cset->variants.bitmap = NULL;
        } else {
            uint32_t lastChar = (uint32_t)extent + (uint32_t)location - 1;
            bitmapRemoveCharactersInRange(cset->variants.bitmap, (UniChar)location, (UniChar)lastChar);
        }
    }
    clearHashValid(cset);
    if (__CFCheckForExapendedSet == 1) {
        __CFCheckForExpandedSet(cset);
    }
}

// Native __CFApplySurrogatesInString, 0x1c0179bbc. Non-external. Walks the string through the inline buffer;
// a high surrogate followed (within the length) by a low surrogate yields a scalar, consecutive scalars are
// batched into ranges and each batch is passed to the function (CFCharacterSetAddCharactersInRange or
// CFCharacterSetRemoveCharactersInRange). A high surrogate also skips a following unit that is not a low
// surrogate.
static void __CFApplySurrogatesInString(
    OCFMutableCharacterSetRef cset,
    OCFStringRef string,
    void (*applyFunction)(OCFMutableCharacterSetRef, OCFRange)
) {
    OCFIndex length = OCFStringGetLength(string);
    OCFStringInlineBuffer inlineBuffer;
    characterSetInitStringInlineBuffer(string, &inlineBuffer, (OCFRange){0, length});
    if (length < 1) {
        return;
    }
    OCFIndex start = 0;
    OCFIndex count = 0;
    OCFIndex index = 0;
    do {
        UniChar high = characterSetGetCharacterFromStringInlineBuffer(&inlineBuffer, index);
        OCFIndex next = index + 1;
        if ((high >> 10) == 0x36 && next < length) {
            UniChar low = characterSetGetCharacterFromStringInlineBuffer(&inlineBuffer, next);
            if ((low >> 10) == 0x37) {
                OCFIndex scalar = (OCFIndex)((((uint32_t)high << 10) + low + 0x202400) & 0x3FFFFF);
                if (start + count == scalar) {
                    ++count;
                } else {
                    if (count >= 1) {
                        applyFunction(cset, (OCFRange){start, count});
                    }
                    count = 1;
                    start = scalar;
                }
            }
        } else {
            next = index;
        }
        index = next + 1;
    } while (index < length);
    if (count >= 1) {
        applyFunction(cset, (OCFRange){start, count});
    }
}

// Native CFCharacterSetRemoveCharactersInString, 0x1c0179e4c, with .cold.2 (0x1c0307618, the body) and
// .cold.1, .cold.3. External. Mirrors CFCharacterSetAddCharactersInString: an empty non-inverted set is
// unchanged; a non-inverted set becomes a bitmap and loses the characters; an inverted set with room appends
// the characters to its stored complement (setting the string class for a new buffer, with no zero-length
// check), otherwise the bitmap path. Surrogate pairs are removed as ranges afterwards. CoreFoundation defect
// (kept, well-defined): the string path's surrogate squeeze moves half of the remaining bytes and advances
// past the moved unit.
// Folds the compiler-outlined pieces _CFCharacterSetRemoveCharactersInString.cold.1 (0x1c0307614),
// _CFCharacterSetRemoveCharactersInString.cold.3 (0x1c03079a0).
void OCFCharacterSetRemoveCharactersInString(OCFMutableCharacterSetRef theSet, OCFStringRef theString) {
    if (characterSetIsObjC(theSet)) {
        ((void (*)(id, SEL, OCFStringRef))objc_msgSend)(
            (id)theSet, sel_getUid("removeCharactersInString:"), theString
        );
        return;
    }
    CharacterSet *cset = theSet;
    checkMutable(cset, "void CFCharacterSetRemoveCharactersInString(CFMutableCharacterSetRef, CFStringRef)");
    if (isStaticPredefinedSet(cset)) {
        return;
    }
    if (isEmpty(cset) && !isInverted(cset)) {
        return;
    }
    OCFIndex length = OCFStringGetLength(theString);
    if (length == 0) {
        return;
    }
    OCFIndex previousLength = 0;
    OCFIndex newLength = 0;
    bool useString = false;
    if (isInverted(cset)) {
        if (isEmpty(cset)) {
            previousLength = 0;
        } else if ((loadInfo(cset) & kInfoClassMask) == 0x20) {
            previousLength = cset->variants.string.length;
        } else {
            previousLength = 64;
        }
        newLength = previousLength + length;
        useString = newLength <= 0x3F;
    }
    if (!useString) {
        __CFCSetMakeBitmap(cset);
        OCFStringInlineBuffer inlineBuffer;
        characterSetInitStringInlineBuffer(theString, &inlineBuffer, (OCFRange){0, length});
        bool hasSurrogate = false;
        for (OCFIndex index = 0; index < length; ++index) {
            UniChar character = characterSetGetCharacterFromStringInlineBuffer(&inlineBuffer, index);
            if (((character & 0xF800) | 0x400) == 0xDC00) {
                hasSurrogate = true;
            } else {
                cset->variants.bitmap[(character >> 3) & 0x1FFF] &= (uint8_t)~(1u << (character & 7));
            }
        }
        clearHashValid(cset);
        if (__CFCheckForExapendedSet == 1) {
            __CFCheckForExpandedSet(cset);
        }
        if (hasSurrogate) {
            __CFApplySurrogatesInString(cset, theString, OCFCharacterSetRemoveCharactersInRange);
        }
        return;
    }
    UniChar *append;
    if (cset->variants.string.buffer != NULL) {
        append = cset->variants.string.buffer + cset->variants.string.length;
    } else {
        append = (UniChar *)OCFAllocatorAllocateTyped(
            OCFGetAllocator(cset), kStringCapacityBytes, kStringBufferType, 0
        );
    }
    OCFStringGetCharacters(theString, (OCFRange){0, length}, append);
    bool hasSurrogate = false;
    if (length >= 2) {
        UniChar *cursor = append;
        UniChar *end = append + length;
        do {
            if (((*cursor | 0x400) & 0xFC00) == 0xDC00) {
                intptr_t remainingBytes = (uint8_t *)end - (uint8_t *)cursor - 2;
                memmove(cursor, cursor + 1, (size_t)(remainingBytes >> 1));
                --end;
                hasSurrogate = true;
            }
            ++cursor;
        } while (cursor < end);
        newLength = previousLength + (end - append);
    }
    UniChar *buffer = cset->variants.string.buffer;
    if (buffer == NULL) {
        setClass(cset, kClassString);
        cset->variants.string.buffer = append;
        buffer = append;
    }
    cset->variants.string.length = newLength;
    cset->variants.string.length = sortAndUniqueString(buffer, newLength);
    clearHashValid(cset);
    if (hasSurrogate) {
        __CFApplySurrogatesInString(cset, theString, OCFCharacterSetRemoveCharactersInRange);
    }
}

// Native CFCharacterSetUnion, 0x1c0179f20, with .cold.1-.cold.3 (0x1c03079b0-0x1c03079b8). External. A static
// predefined set or an empty inverted set is left alone. An Objective-C other set without an expansion ORs
// its bitmap representation into plane 0 and returns without the expanded-set check. An empty other set makes
// the set full (range class, length 0, inverted, no annex) when inverted. A builtin other set is copied into
// an empty set (class, number, inverted bit, annex inversion). Otherwise a range is added (an inverted one as
// the two complementary ranges, 32-bit arithmetic), a string through a no-copy CFString, other classes ORed
// into the bitmap; then annex planes are united (intersected when the other annex is inverted) or the annex
// inversion is taken over; a builtin other set adds its non-empty planes.
// Folds the compiler-outlined piece _CFCharacterSetUnion.cold.2 (0x1c03079b4).
void OCFCharacterSetUnion(OCFMutableCharacterSetRef theSet, OCFCharacterSetRef theOtherSet) {
    if (characterSetIsObjC(theSet)) {
        ((void (*)(id, SEL, OCFCharacterSetRef))objc_msgSend)(
            (id)theSet, sel_getUid("formUnionWithCharacterSet:"), theOtherSet
        );
        return;
    }
    CharacterSet *cset = theSet;
    uint8_t bitsBuffer[kBitmapSize];
    checkMutable(cset, "void CFCharacterSetUnion(CFMutableCharacterSetRef, CFCharacterSetRef)");
    if (isStaticPredefinedSet(cset)) {
        return;
    }
    if (isEmpty(cset) && isInverted(cset)) {
        return;
    }
    bool useRepresentation;
    const CharacterSet *other = expandedOtherSet(theOtherSet, &useRepresentation);
    if (useRepresentation) {
        OCFDataRef representation =
            OCFCharacterSetCreateBitmapRepresentation(kOCFAllocatorSystemDefault, other);
        if (representation != NULL) {
            if (OCFDataGetLength(representation) != 0) {
                const uint8_t *bytes = OCFDataGetBytePtr(representation);
                if (bytes != NULL) {
                    __CFCSetMakeBitmap(cset);
                    uint8_t *bitmap = cset->variants.bitmap;
                    for (uint32_t index = 0; index != kBitmapSize / 4; ++index) {
                        storeWord(bitmap, index, loadWord(bitmap, index) | loadWord(bytes, index));
                    }
                    clearHashValid(cset);
                }
            }
            OCFRelease(representation);
        }
        return;
    }
    if (isEmpty(other)) {
        if (isInverted(other)) {
            if (((loadInfo(cset) & kInfoClassMask) == 0x20 && cset->variants.string.buffer != NULL) ||
                ((loadInfo(cset) & kInfoClassMask) == 0x30 && cset->variants.bitmap != NULL) ||
                ((loadInfo(cset) & kInfoClassMask) == 0x40 && cset->variants.bitmap != NULL)) {
                OCFAllocatorDeallocate(OCFGetAllocator(cset), cset->variants.bitmap);
            }
            setClass(cset, kClassRange);
            cset->variants.range.length = 0;
            updateInfo(cset, 0, kInfoInverted);
            clearHashValid(cset);
            if (cset->annex != NULL) {
                annexDeallocate(cset);
            }
        }
        goto done;
    }
    if ((loadInfo(other) & kInfoClassMask) == 0 && isEmpty(cset)) {
        setClass(cset, kClassBuiltin);
        cset->variants.builtinType = other->variants.builtinType;
        if (isInverted(other)) {
            updateInfo(cset, 0, kInfoInverted);
        }
        if (annexIsInverted(other)) {
            CharacterSetAnnex *annex = cset->annex;
            if (annex == NULL) {
                annex = annexAllocateEmpty(cset);
            }
            annex->isInverted = 1;
        }
        clearHashValid(cset);
        goto done;
    }
    {
        uint64_t otherClass = loadInfo(other) & kInfoClassMask;
        uint64_t otherInfo = loadInfo(other);
        if (otherClass == 0x10) {
            uint32_t firstChar = other->variants.range.firstChar;
            if ((otherInfo & kInfoInverted) != 0) {
                OCFIndex rangeLength = other->variants.range.length;
                if (firstChar != 0) {
                    OCFCharacterSetAddCharactersInRange(cset, (OCFRange){0, firstChar});
                }
                uint32_t afterRange = firstChar + (uint32_t)rangeLength;
                OCFCharacterSetAddCharactersInRange(
                    cset, (OCFRange){afterRange, (uint32_t)(0x110000 - afterRange)}
                );
            } else {
                OCFCharacterSetAddCharactersInRange(
                    cset, (OCFRange){firstChar, other->variants.range.length}
                );
            }
        } else if ((otherInfo & kInfoClassMask) == 0x20) {
            OCFStringRef string = OCFStringCreateWithCharactersNoCopy(
                OCFGetAllocator(cset),
                other->variants.string.buffer,
                other->variants.string.length,
                kOCFAllocatorNull
            );
            OCFCharacterSetAddCharactersInString(cset, string);
            OCFRelease(string);
        } else {
            __CFCSetMakeBitmap(cset);
            uint8_t *bitmap = cset->variants.bitmap;
            if ((loadInfo(other) & kInfoClassMask) == 0x30) {
                const uint8_t *otherBits = other->variants.bitmap;
                for (uint32_t index = 0; index != kBitmapSize / 4; ++index) {
                    storeWord(bitmap, index, loadWord(bitmap, index) | loadWord(otherBits, index));
                }
            } else {
                __CFCSetGetBitmap(other, bitsBuffer);
                for (uint32_t index = 0; index != kBitmapSize / 4; ++index) {
                    storeWord(bitmap, index, loadWord(bitmap, index) | loadWord(bitsBuffer, index));
                }
            }
            clearHashValid(cset);
        }
    }
    if (other->annex != NULL) {
        uint8_t otherAnnexInverted = other->annex->isInverted;
        if (other->annex->validPlanes != 0) {
            for (uint32_t plane = 1; plane != 17; ++plane) {
                OCFCharacterSetRef otherPlane = annexGetPlane(other, (uint8_t)plane);
                if (otherPlane == NULL) {
                    continue;
                }
                CharacterSet *target = annexGetOrCreatePlane(cset, (uint8_t)plane);
                if (otherAnnexInverted == 0) {
                    OCFCharacterSetUnion(target, otherPlane);
                } else {
                    OCFCharacterSetIntersect(target, otherPlane);
                }
            }
            goto done;
        }
        if (other->annex->isInverted != 0) {
            CharacterSetAnnex *annex = cset->annex;
            if (annex != NULL) {
                if (annex->validPlanes == 0) {
                    annex->isInverted = 1;
                    goto done;
                }
                annexDeallocate(cset);
            }
            annex = annexAllocateEmpty(cset);
            annex->isInverted = 1;
            goto done;
        }
    }
    if ((loadInfo(other) & kInfoClassMask) == 0) {
        bool otherAnnexInverted = annexIsInverted(other);
        for (uint32_t plane = 1; plane != 17; ++plane) {
            uint8_t result = CFUniCharGetBitmapForPlane(
                (uint32_t)other->variants.builtinType, plane, bitsBuffer, otherAnnexInverted
            );
            if (result == 0xFF) {
                continue;
            }
            CharacterSet *target = annexGetOrCreatePlane(cset, (uint8_t)plane);
            if (result == 1) {
                OCFCharacterSetAddCharactersInRange(target, (OCFRange){0, 0x10000});
            } else {
                __CFCSetMakeBitmap(target);
                uint8_t *bitmap = target->variants.bitmap;
                for (uint32_t index = 0; index != kBitmapSize / 4; ++index) {
                    storeWord(bitmap, index, loadWord(bitmap, index) | loadWord(bitsBuffer, index));
                }
            }
        }
    }
done:
    if (__CFCheckForExapendedSet == 1) {
        __CFCheckForExpandedSet(cset);
    }
}

// Native _CFCharacterSetCompact, 0x1c017aac0, with .cold.1 (0x1c03079c8, the dispatch_once of the Swift
// feature through OUTLINED_FUNCTION_0_3) (corpus delta capture 7). External. A tagged pointer, the Swift
// feature (bit 0) or an Objective-C object of type 25 is sent -makeCharacterSetCompact. Otherwise a bitmap
// set (class 3) with bits is made compact (__CFCSetMakeCompact), then each of the 16 annex planes that is
// present (valid bit plane + 1, a plane count of at least plane + 1 as a byte, a non-NULL slot; the annex
// pointer is reread each time) and is such a bitmap set; nothing happens without an annex or valid planes.
void _OCFCharacterSetCompact(OCFMutableCharacterSetRef theSet) {
    if (characterSetIsObjC(theSet)) {
        ((void (*)(id, SEL))objc_msgSend)((id)theSet, sel_getUid("makeCharacterSetCompact"));
        return;
    }
    CharacterSet *cset = theSet;
    if ((loadInfo(cset) & kInfoClassMask) == 0x30 && cset->variants.bitmap != NULL) {
        __CFCSetMakeCompact(cset);
    }
    if (cset->annex == NULL || cset->annex->validPlanes == 0) {
        return;
    }
    for (OCFIndex index = 0; index != 16; ++index) {
        const CharacterSetAnnex *annex = cset->annex;
        if (annex == NULL) {
            continue;
        }
        uint32_t plane = (uint32_t)index + 1;
        if (((annex->validPlanes >> plane) & 1) == 0 || annex->planeCount < (uint8_t)plane) {
            continue;
        }
        CharacterSet *planeSet = (CharacterSet *)annex->planes[index];
        if (planeSet != NULL && (loadInfo(planeSet) & kInfoClassMask) == 0x30 &&
            planeSet->variants.bitmap != NULL) {
            __CFCSetMakeCompact(planeSet);
        }
    }
}

// Native _CFCharacterSetFast, 0x1c017abc0, with .cold.1 (0x1c03079cc, the dispatch_once of the Swift feature
// through OUTLINED_FUNCTION_0_3) (corpus delta capture 7). External. As _CFCharacterSetCompact with
// -makeCharacterSetFast, compact bitmap sets (class 4) with bits and __CFCSetMakeBitmap.
void _OCFCharacterSetFast(OCFMutableCharacterSetRef theSet) {
    if (characterSetIsObjC(theSet)) {
        ((void (*)(id, SEL))objc_msgSend)((id)theSet, sel_getUid("makeCharacterSetFast"));
        return;
    }
    CharacterSet *cset = theSet;
    if ((loadInfo(cset) & kInfoClassMask) == 0x40 && cset->variants.bitmap != NULL) {
        __CFCSetMakeBitmap(cset);
    }
    if (cset->annex == NULL || cset->annex->validPlanes == 0) {
        return;
    }
    for (OCFIndex index = 0; index != 16; ++index) {
        const CharacterSetAnnex *annex = cset->annex;
        if (annex == NULL) {
            continue;
        }
        uint32_t plane = (uint32_t)index + 1;
        if (((annex->validPlanes >> plane) & 1) == 0 || annex->planeCount < (uint8_t)plane) {
            continue;
        }
        CharacterSet *planeSet = (CharacterSet *)annex->planes[index];
        if (planeSet != NULL && (loadInfo(planeSet) & kInfoClassMask) == 0x40 &&
            planeSet->variants.bitmap != NULL) {
            __CFCSetMakeBitmap(planeSet);
        }
    }
}

// Native _CFCharacterSetGetKeyedCodingType, 0x1c017acc0. External. 1 for a tagged pointer or an Objective-C
// object of type 25; otherwise by class: builtin 2 (type below 14) or 5, range 3, string 4 without annex
// planes or annex inversion (else 1), any other class 1.
OCFIndex _OCFCharacterSetGetKeyedCodingType(OCFCharacterSetRef theSet) {
    if (((uintptr_t)theSet >> 63) != 0 || __CF_IS_OBJC(kCharacterSetTypeID, theSet) != 0) {
        return 1;
    }
    const CharacterSet *cset = theSet;
    switch (classOf(cset)) {
        case kClassString: {
            const CharacterSetAnnex *annex = cset->annex;
            if (annex == NULL || (annex->validPlanes == 0 && annex->isInverted == 0)) {
                return 4;
            }
            return 1;
        }
        case kClassRange:
            return 3;
        case kClassBuiltin:
            return cset->variants.builtinType < 14 ? 2 : 5;
        default:
            return 1;
    }
}

// Native _CFCharacterSetGetKeyedCodingBuiltinType, 0x1c017add0. External. The word at 24, unchecked.
OCFIndex _OCFCharacterSetGetKeyedCodingBuiltinType(OCFCharacterSetRef theSet) {
    return theSet->variants.builtinType;
}

// Native _CFCharacterSetGetKeyedCodingRange, 0x1c017add8. External. The range variant (the first character
// as 32 bits), unchecked.
OCFRange _OCFCharacterSetGetKeyedCodingRange(OCFCharacterSetRef theSet) {
    return (OCFRange){theSet->variants.range.firstChar, theSet->variants.range.length};
}

// Native _CFCharacterSetCreateKeyedCodingString, 0x1c017ade8. External. A string of the string variant's
// characters (kCFAllocatorSystemDefault; a tail call), unchecked.
OCFStringRef _OCFCharacterSetCreateKeyedCodingString(OCFCharacterSetRef theSet) {
    return OCFStringCreateWithCharacters(
        kOCFAllocatorSystemDefault, theSet->variants.string.buffer, theSet->variants.string.length
    );
}

// Native _CFCharacterSetIsInverted, 0x1c017ae00, with .cold.1 (0x1c03079d4, the dispatch_once of the Swift
// feature check). External. Objective-C sets (the usual prelude) answer -_isInverted; otherwise info bit 3.
Boolean _OCFCharacterSetIsInverted(OCFCharacterSetRef theSet) {
    if (characterSetIsObjC(theSet)) {
        return ((Boolean (*)(id, SEL))objc_msgSend)((id)theSet, sel_getUid("_isInverted"));
    }
    return (loadInfo(theSet) >> 3) & 1;
}

// Native _CFCharacterSetSetIsInverted, 0x1c017ae74, with .cold.1 at 0x1c03079d8 (the feature
// dispatch_once). External. iOS Device divergence: unread.
void _OCFCharacterSetSetIsInverted(OCFMutableCharacterSetRef set, Boolean flag) {
    (void)foundationSwiftCharacterSetEnabled();
    if ((uintptr_t)set >> 63 || (foundationSwiftCharacterSetFeatureEnabled & 1) != 0 ||
        __CF_IS_OBJC(kCharacterSetTypeID, set) != 0) {
        ((void (*)(id, SEL, Boolean))objc_msgSend)((id)set, sel_getUid("_setIsInverted:"), flag);
        return;
    }
    updateInfo(set, kInfoInverted, flag != 0 ? kInfoInverted : 0);
}

// Native CFCharacterSetInitInlineBuffer, 0x1c017af14, with .cold.1 (0x1c03079dc). External. The buffer is
// cleared with the set and a range limit of 0x10000. An Objective-C set fills it through -_fillInlineBuffer:
// when it responds (selector 0x2387abcf7, the image's objc_msgSend$_fillInlineBuffer: stub target), else
// through its
// -_expandedCFCharacterSet (none: no bitmap, limit 0x110000). Builtin: the table's plane-0 bitmap (none: no
// bitmap) with limit 0x110000, inverted flag; range: start and limit, inverted flag, nothing else; string: no
// bitmap with the first and last+1 characters (an inverted string widens to the complement's bounds); bitmap
// and compact: the bytes (compact flag; NULL bits: compact flag plus inverted). An inverted annex sets the
// limit to 0x110000; otherwise the highest present plane + 1 does.
void OCFCharacterSetInitInlineBuffer(OCFCharacterSetRef cset, OCFCharacterSetInlineBuffer *buffer) {
    memset(buffer, 0, sizeof(*buffer));
    buffer->cset = cset;
    buffer->rangeLimit = 0x10000;
    const CharacterSet *set = cset;
    if (((uintptr_t)cset >> 63) != 0 || __CF_IS_OBJC(kCharacterSetTypeID, cset) != 0) {
        if (objc_opt_respondsToSelector((id)cset, sel_getUid("_fillInlineBuffer:"))) {
            ((void (*)(id, SEL, OCFCharacterSetInlineBuffer *))objc_msgSend)(
                (id)cset, sel_getUid("_fillInlineBuffer:"), buffer
            );
            return;
        }
        (void)foundationSwiftCharacterSetEnabled();
        bool expand = ((uintptr_t)cset >> 63) != 0 || (foundationSwiftCharacterSetFeatureEnabled & 1) != 0 ||
                      __CF_IS_OBJC(kCharacterSetTypeID, cset) != 0;
        set = NULL;
        if (expand) {
            set = ((OCFCharacterSetRef (*)(id, SEL))objc_msgSend)(
                (id)cset, sel_getUid("_expandedCFCharacterSet")
            );
        }
        if (set == NULL) {
            buffer->flags = 2;
            buffer->rangeLimit = 0x110000;
            return;
        }
    }
    switch (classOf(set)) {
        case kClassBuiltin: {
            const uint8_t *bitmap = OCFUniCharGetBitmapPtrForPlane((uint32_t)set->variants.builtinType, 0);
            buffer->bitmap = bitmap;
            buffer->rangeLimit = 0x110000;
            if (bitmap == NULL) {
                buffer->flags = 2;
            } else if (isInverted(set)) {
                buffer->flags = 4;
            }
            break;
        }
        case kClassRange:
            buffer->rangeStart = set->variants.range.firstChar;
            buffer->rangeLimit = set->variants.range.firstChar + (uint32_t)set->variants.range.length;
            if (isInverted(set)) {
                buffer->flags = 4;
            }
            return;
        case kClassString:
            buffer->flags = 2;
            if (set->variants.string.length >= 1) {
                buffer->rangeStart = set->variants.string.buffer[0];
                buffer->rangeLimit =
                    (uint32_t)set->variants.string.buffer[set->variants.string.length - 1] + 1;
                if (isInverted(set)) {
                    uint32_t start = buffer->rangeStart;
                    uint32_t limit = buffer->rangeLimit;
                    if (start == 0) {
                        buffer->rangeStart = limit;
                        buffer->rangeLimit = 0x10000;
                    } else if (limit == 0x10000) {
                        buffer->rangeStart = 0;
                        buffer->rangeLimit = start;
                    } else {
                        buffer->rangeStart = 0;
                        buffer->rangeLimit = 0x10000;
                    }
                }
            }
            break;
        case kClassBitmap:
        case kClassCompactBitmap: {
            const uint8_t *bitmap = set->variants.bitmap;
            buffer->bitmap = bitmap;
            if (bitmap != NULL) {
                if ((loadInfo(set) & kInfoClassMask) == 0x40) {
                    buffer->flags = 1;
                }
            } else {
                buffer->flags = 1;
                if (isInverted(set)) {
                    buffer->flags |= 4;
                }
            }
            break;
        }
        default:
            return;
    }
    const CharacterSetAnnex *annex = set->annex;
    if (annex == NULL) {
        return;
    }
    if (annex->isInverted != 0) {
        buffer->rangeLimit = 0x110000;
        return;
    }
    uint32_t validPlanes = annex->validPlanes;
    if (validPlanes == 0) {
        return;
    }
    uint32_t limit = 0x110000;
    for (int64_t index = 15;; --index) {
        uint32_t plane = (uint32_t)(index + 1) & 0xFF;
        if (plane <= 0x10 && ((validPlanes >> ((uint32_t)(index + 1) & 31)) & 1) != 0 &&
            annex->planeCount >= plane && annex->planes[index] != NULL) {
            buffer->rangeLimit = limit;
            return;
        }
        limit -= 0x10000;
        if (index + 1 <= 1) {
            return;
        }
    }
}

// ICU (libicucore, linked by the native image); supplied, never reimplemented. Declared here because the SDK
// installs no uset.h. USet is opaque; UErrorCode is 32-bit.
extern int32_t uset_getItemCount(const void *set);
extern int32_t uset_getItem(
    const void *set,
    int32_t itemIndex,
    int32_t *start,
    int32_t *end,
    UniChar *string,
    int32_t stringCapacity,
    int32_t *status
);

// Native _CFCreateCharacterSetFromUSet, 0x1c017b1a8. External. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta2.json. A mutable set (default allocator) or NULL. The
// ICU status is cleared once, before the loop, and only again before a retry. Each item is read into a
// 2,048-unit stack buffer; status 15 (U_BUFFER_OVERFLOW_ERROR) retries into a malloc block of (length + 1)
// units (type 0x1000040BDFB0063; a failed allocation releases the set and answers NULL). A failure status (1
// or more) frees a heap buffer, releases the set and answers NULL. A string item (length above 0) is added
// through a no-copy string (kCFAllocatorSystemDefault, kCFAllocatorNull deallocator) released at once; a
// range item adds start ... end (32-bit arithmetic, sign-extended). The answer is an immutable copy
// (__CFCharacterSetCreateCopy, kCFAllocatorSystemDefault) and the mutable set is released.
OCFMutableCharacterSetRef _OCFCreateCharacterSetFromUSet(void *set) {
    UniChar stackBuffer[2048];
    int32_t status = 0;
    OCFMutableCharacterSetRef result = OCFCharacterSetCreateMutable(NULL);
    if (result == NULL) {
        return NULL;
    }
    int32_t count = uset_getItemCount(set);
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    for (int32_t index = 0; index < count; ++index) {
        int32_t end = 0;
        int32_t start = 0;
        UniChar *buffer = stackBuffer;
        int32_t length = uset_getItem(set, index, &start, &end, stackBuffer, 0x800, &status);
        if (status == 15) {
            int32_t capacity = length + 1;
            UniChar *heapBuffer = malloc_type_malloc((size_t)(int64_t)capacity * 2, 0x1000040BDFB0063ULL);
            if (heapBuffer == NULL) {
                OCFRelease(result);
                return NULL;
            }
            buffer = heapBuffer;
            status = 0;
            uset_getItem(set, index, &start, &end, buffer, capacity, &status);
        }
        if (status >= 1) {
            if (buffer != stackBuffer) {
                free(buffer);
            }
            OCFRelease(result);
            return NULL;
        }
        if (length > 0) {
            OCFStringRef string =
                OCFStringCreateWithCharactersNoCopy(allocator, buffer, (uint32_t)length, kOCFAllocatorNull);
            OCFCharacterSetAddCharactersInString(result, string);
            OCFRelease(string);
        } else {
            OCFCharacterSetAddCharactersInRange(
                result, (OCFRange){(OCFIndex)start, (OCFIndex)(int32_t)(end - start + 1)}
            );
        }
        if (buffer != stackBuffer) {
            free(buffer);
        }
    }
    OCFMutableCharacterSetRef copy = __CFCharacterSetCreateCopy(allocator, result, false);
    OCFRelease(result);
    return copy;
}

// Native __CFCSetIsEqualAnnex, 0x1c017b3a8. Non-external. Two 0x2000-byte stack bitmaps. When exactly one
// annex is inverted, every plane 1-16 must be the other's complement (a plane absent from both is unequal; a
// plane on one side only must be full). Otherwise the valid bits must match and each plane of the first set
// must equal the second's (__CFCharacterSetEqual, possibly against NULL).
static Boolean __CFCSetIsEqualAnnex(OCFCharacterSetRef cf1, OCFCharacterSetRef cf2) {
    const CharacterSet *set1 = cf1;
    const CharacterSet *set2 = cf2;
    uint8_t bitsBuffer1[kBitmapSize];
    uint8_t bitsBuffer2[kBitmapSize];
    const CharacterSetAnnex *annex1 = set1->annex;
    const CharacterSetAnnex *annex2 = set2->annex;
    bool inverted1 = annex1 != NULL && annex1->isInverted != 0;
    bool inverted2 = annex2 != NULL && annex2->isInverted != 0;
    if (inverted1 != inverted2) {
        for (uint32_t plane = 1; plane != 17; ++plane) {
            OCFCharacterSetRef planeSet1 = annexGetPlane(set1, (uint8_t)plane);
            OCFCharacterSetRef planeSet2 = annexGetPlane(set2, (uint8_t)plane);
            if (planeSet1 == NULL && planeSet2 == NULL) {
                return false;
            }
            if (planeSet1 != NULL && planeSet2 != NULL) {
                uint64_t class1 = loadInfo(planeSet1) & kInfoClassMask;
                uint64_t class2 = loadInfo(planeSet2) & kInfoClassMask;
                if (class1 == 0x30 && class2 == 0x30) {
                    if (!bitmapsAreInverse(planeSet1->variants.bitmap, planeSet2->variants.bitmap)) {
                        return false;
                    }
                } else if (class1 != 0x30 && class2 != 0x30) {
                    __CFCSetGetBitmap(planeSet1, bitsBuffer1);
                    __CFCSetGetBitmap(planeSet2, bitsBuffer2);
                    if (!bitmapsAreInverse(bitsBuffer1, bitsBuffer2)) {
                        return false;
                    }
                } else {
                    OCFCharacterSetRef nonBitmapSet = class2 == 0x30 ? planeSet1 : planeSet2;
                    OCFCharacterSetRef bitmapSet = class2 == 0x30 ? planeSet2 : planeSet1;
                    __CFCSetGetBitmap(nonBitmapSet, bitsBuffer1);
                    if (!bitmapsAreInverse(bitmapSet->variants.bitmap, bitsBuffer1)) {
                        return false;
                    }
                }
                continue;
            }
            OCFCharacterSetRef planeSet = planeSet1 != NULL ? planeSet1 : planeSet2;
            if ((loadInfo(planeSet) & kInfoClassMask) == 0x30) {
                if (!__CFCSetIsEqualBitmap(planeSet->variants.bitmap, kFullBitmap)) {
                    return false;
                }
            } else {
                __CFCSetGetBitmap(planeSet, bitsBuffer1);
                if (!__CFCSetIsEqualBitmap(bitsBuffer1, kFullBitmap)) {
                    return false;
                }
            }
        }
        return true;
    }
    uint32_t valid1 = annex1 != NULL ? annex1->validPlanes : 0;
    uint32_t valid2 = annex2 != NULL ? annex2->validPlanes : 0;
    if (annex1 != NULL || annex2 != NULL) {
        if (valid1 != valid2) {
            return false;
        }
    }
    for (uint32_t index = 0; index != 16; ++index) {
        OCFCharacterSetRef planeSet1 = annexGetPlane(set1, (uint8_t)(index + 1));
        OCFCharacterSetRef planeSet2 = annexGetPlane(set2, (uint8_t)(index + 1));
        if (planeSet1 != NULL && !__CFCharacterSetEqual(planeSet1, planeSet2)) {
            return false;
        }
    }
    return true;
}

// Native __CFCSetIsBitmapEqualToRange, 0x1c017b750. Non-external. Whether the bitmap holds exactly the range
// (complemented when inverted): the boundary words are compared with big-endian masks (REV) XOR the
// sign-extended negated flag, the words before and after with the clear fill, the words between with the set
// fill.
static Boolean
__CFCSetIsBitmapEqualToRange(const uint8_t *bits, UniChar firstChar, UniChar lastChar, Boolean isInverted) {
    uint32_t first = firstChar;
    uint32_t last = lastChar;
    uint32_t flip = (uint32_t)(int32_t)(int8_t)(uint8_t)(-(int32_t)isInverted);
    uint32_t firstMask = 0xFFu << (first & 7);
    uint32_t firstWord;
    uint32_t lastWord;
    if ((last ^ first) <= 7) {
        uint32_t mask = (0xFFu >> (7 & ~last)) & firstMask;
        uint32_t expected = (mask << 24) >> (0x18 & ~first);
        firstWord = first >> 5;
        if (loadWord(bits, firstWord) != (expected ^ flip)) {
            return false;
        }
        lastWord = firstWord;
    } else {
        uint32_t firstShift = first & 0x18;
        uint32_t firstWordMask = ((firstMask & 0xFF) << (firstShift ^ 0x18)) | (0xFFFFFFu >> firstShift);
        uint32_t lastShift = last & 0x18;
        uint32_t lastWordMask =
            ((0xFFu >> (7 & ~last)) << (lastShift ^ 0x18)) | (uint32_t)(0xFFFFFFFFull << (32 - lastShift));
        firstWord = first >> 5;
        lastWord = last >> 5;
        if (firstWord == lastWord) {
            if (loadWord(bits, firstWord) != (__builtin_bswap32(lastWordMask & firstWordMask) ^ flip)) {
                return false;
            }
        } else {
            if (loadWord(bits, firstWord) != (__builtin_bswap32(firstWordMask) ^ flip)) {
                return false;
            }
            if (loadWord(bits, lastWord) != (__builtin_bswap32(lastWordMask) ^ flip)) {
                return false;
            }
        }
    }
    uint32_t outsideFill = isInverted != 0 ? 0xFFFFFFFF : 0;
    for (uint32_t index = 0; index < firstWord; ++index) {
        if (loadWord(bits, index) != outsideFill) {
            return false;
        }
    }
    uint32_t insideFill = isInverted == 0 ? 0xFFFFFFFF : 0;
    for (uint32_t index = firstWord + 1; index < lastWord; ++index) {
        if (loadWord(bits, index) != insideFill) {
            return false;
        }
    }
    for (uint32_t index = lastWord + 1; index < kBitmapSize / 4; ++index) {
        if (loadWord(bits, index) != outsideFill) {
            return false;
        }
    }
    return true;
}

// Native __CFCreateCompactBitmap, 0x1c017b8f8. Non-external. Each 32-byte block that is uniformly 0 or 0xFF
// is recorded by that byte in a 256-byte header; other blocks get numbers 1, 2, ... and are stored after the
// header. More than 128 stored blocks answers NULL. The result is header + 32 bytes per stored block,
// allocated with the given allocator (type 0x100004077774924).
static uint8_t *__CFCreateCompactBitmap(OCFAllocatorRef allocator, const uint8_t *bitmap) {
    uint8_t header[256];
    int32_t count = 0;
    const uint8_t *block = bitmap;
    for (uint32_t index = 0; index != 256; ++index) {
        uint8_t value = block[0];
        bool uniform = false;
        if (value == 0 || value == 0xFF) {
            uniform = true;
            for (uint32_t offset = 1; offset != 32; ++offset) {
                if (block[offset] != value) {
                    uniform = false;
                    break;
                }
            }
        }
        if (uniform) {
            header[index] = value;
        } else {
            bool tooMany = count > 0x7F;
            ++count;
            header[index] = (uint8_t)count;
            if (tooMany) {
                return NULL;
            }
        }
        block += 32;
    }
    uint8_t *compact = (uint8_t *)OCFAllocatorAllocateTyped(
        allocator, (OCFIndex)(int32_t)((count << 5) + 0x100), kBitmapType, 0
    );
    if (count >= 1) {
        uint8_t *stored = compact + 0x100;
        for (uint32_t index = 0; index != 256; ++index) {
            uint8_t value = header[index];
            compact[index] = value;
            if (value != 0 && value != 0xFF) {
                memcpy(stored, bitmap, 32);
                stored += 32;
            }
            bitmap += 32;
        }
    } else {
        memcpy(compact, header, 256);
    }
    return compact;
}

// Native _CFCharacterSetIsMutable, 0x1c017ad5c, with .cold.1. External. The Swift feature, a tagged pointer
// or an Objective-C set answers -_isMutable; a CF set answers info bit 0. Reading:
// Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3; iOS Device divergence: unread).
// Folds the compiler-outlined piece __CFCharacterSetIsMutable.cold.1 (0x1c03079d0).
Boolean _OCFCharacterSetIsMutable(OCFCharacterSetRef cset) {
    if (characterSetIsObjC(cset)) {
        return ((BOOL (*)(OCFTypeRef, SEL))objc_msgSend)(cset, sel_getUid("_isMutable"));
    }
    return loadInfo((const CharacterSet *)cset) & 1;
}

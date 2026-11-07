//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Number/OCFNumber.h"

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/String/OCFStringFormat.h"

#include <dispatch/dispatch.h>
#include <float.h>
#include <math.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Native powersOf10, iOS Simulator 24A434, 0x1c03752c0 (624 bytes: 39 high/low pairs, 10^38 down to
// 10^0), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x1808314e0, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// the 624 bytes agree. The length is the 39-iteration, 16-byte-stride loop of emit128.
// Reading: Documentation/Reference/Research/CoreFoundationBigNum.md.
static const Int128Parts powersOf10[39] = {
    {(int64_t)UINT64_C(0x4b3b4ca85a86c47a), UINT64_C(0x098a224000000000)},
    {(int64_t)UINT64_C(0x0785ee10d5da46d9), UINT64_C(0x00f436a000000000)},
    {(int64_t)UINT64_C(0x00c097ce7bc90715), UINT64_C(0xb34b9f1000000000)},
    {(int64_t)UINT64_C(0x0013426172c74d82), UINT64_C(0x2b878fe800000000)},
    {(int64_t)UINT64_C(0x0001ed09bead87c0), UINT64_C(0x378d8e6400000000)},
    {(int64_t)UINT64_C(0x0000314dc6448d93), UINT64_C(0x38c15b0a00000000)},
    {(int64_t)UINT64_C(0x000004ee2d6d415b), UINT64_C(0x85acef8100000000)},
    {(int64_t)UINT64_C(0x0000007e37be2022), UINT64_C(0xc0914b2680000000)},
    {(int64_t)UINT64_C(0x0000000c9f2c9cd0), UINT64_C(0x4674edea40000000)},
    {(int64_t)UINT64_C(0x00000001431e0fae), UINT64_C(0x6d7217caa0000000)},
    {(int64_t)UINT64_C(0x00000000204fce5e), UINT64_C(0x3e25026110000000)},
    {(int64_t)UINT64_C(0x00000000033b2e3c), UINT64_C(0x9fd0803ce8000000)},
    {(int64_t)UINT64_C(0x000000000052b7d2), UINT64_C(0xdcc80cd2e4000000)},
    {(int64_t)UINT64_C(0x0000000000084595), UINT64_C(0x161401484a000000)},
    {(int64_t)UINT64_C(0x000000000000d3c2), UINT64_C(0x1bcecceda1000000)},
    {(int64_t)UINT64_C(0x000000000000152d), UINT64_C(0x02c7e14af6800000)},
    {(int64_t)UINT64_C(0x000000000000021e), UINT64_C(0x19e0c9bab2400000)},
    {(int64_t)UINT64_C(0x0000000000000036), UINT64_C(0x35c9adc5dea00000)},
    {(int64_t)UINT64_C(0x0000000000000005), UINT64_C(0x6bc75e2d63100000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x8ac7230489e80000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x0de0b6b3a7640000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x016345785d8a0000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x002386f26fc10000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x00038d7ea4c68000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x00005af3107a4000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x000009184e72a000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x000000e8d4a51000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x000000174876e800)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x00000002540be400)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x000000003b9aca00)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x0000000005f5e100)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x0000000000989680)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x00000000000f4240)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x00000000000186a0)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x0000000000002710)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x00000000000003e8)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000064)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x000000000000000a)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000001)},
};

// Native neg_powersOf10, iOS Simulator 24A434, 0x1c0375530 (624 bytes: the 39 negated powers of ten,
// -10^38 up to -10^0), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x180831750, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// the 624 bytes agree.
// Reading: Documentation/Reference/Research/CoreFoundationBigNum.md.
static const Int128Parts neg_powersOf10[39] = {
    {(int64_t)UINT64_C(0xb4c4b357a5793b85), UINT64_C(0xf675ddc000000000)},
    {(int64_t)UINT64_C(0xf87a11ef2a25b926), UINT64_C(0xff0bc96000000000)},
    {(int64_t)UINT64_C(0xff3f68318436f8ea), UINT64_C(0x4cb460f000000000)},
    {(int64_t)UINT64_C(0xffecbd9e8d38b27d), UINT64_C(0xd478701800000000)},
    {(int64_t)UINT64_C(0xfffe12f64152783f), UINT64_C(0xc872719c00000000)},
    {(int64_t)UINT64_C(0xffffceb239bb726c), UINT64_C(0xc73ea4f600000000)},
    {(int64_t)UINT64_C(0xfffffb11d292bea4), UINT64_C(0x7a53107f00000000)},
    {(int64_t)UINT64_C(0xffffff81c841dfdd), UINT64_C(0x3f6eb4d980000000)},
    {(int64_t)UINT64_C(0xfffffff360d3632f), UINT64_C(0xb98b1215c0000000)},
    {(int64_t)UINT64_C(0xfffffffebce1f051), UINT64_C(0x928de83560000000)},
    {(int64_t)UINT64_C(0xffffffffdfb031a1), UINT64_C(0xc1dafd9ef0000000)},
    {(int64_t)UINT64_C(0xfffffffffcc4d1c3), UINT64_C(0x602f7fc318000000)},
    {(int64_t)UINT64_C(0xffffffffffad482d), UINT64_C(0x2337f32d1c000000)},
    {(int64_t)UINT64_C(0xfffffffffff7ba6a), UINT64_C(0xe9ebfeb7b6000000)},
    {(int64_t)UINT64_C(0xffffffffffff2c3d), UINT64_C(0xe43133125f000000)},
    {(int64_t)UINT64_C(0xffffffffffffead2), UINT64_C(0xfd381eb509800000)},
    {(int64_t)UINT64_C(0xfffffffffffffde1), UINT64_C(0xe61f36454dc00000)},
    {(int64_t)UINT64_C(0xffffffffffffffc9), UINT64_C(0xca36523a21600000)},
    {(int64_t)UINT64_C(0xfffffffffffffffa), UINT64_C(0x9438a1d29cf00000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0x7538dcfb76180000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xf21f494c589c0000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfe9cba87a2760000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffdc790d903f0000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfffc72815b398000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffffa50cef85c000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfffff6e7b18d6000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffffff172b5af000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffe8b7891800)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfffffffdabf41c00)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffc4653600)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfffffffffa0a1f00)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffff676980)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfffffffffff0bdc0)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfffffffffffe7960)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffd8f0)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfffffffffffffc18)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffff9c)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfffffffffffffff6)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff)},
};

// Native emit128, iOS Simulator 24A434, 0x1c01ac500,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x18069c78c, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: emit128 at 0x18069c78c moves the negation block after the return and
// allocates other scratch registers; comparisons, table reads, stores and order agree.
// The value is a high/low pair read from memory. A negative value is negated in 128 bits and
// prefixed with '-'; forcePlus prefixes '+' otherwise. Each of the 39 decimal positions repeatedly
// adds the negated power while the power is not greater than the remainder (signed high, unsigned low
// comparison). Leading zero positions are skipped, a lone '0' is written when every position was
// skipped, and a NUL ends the string. The 128-bit minimum stays negative after negation and is written
// as "-0", as in the native code. The three native callers ignore the result register; the
// restoration returns void.
// Reading: Documentation/Reference/Research/CoreFoundationBigNum.md.
void emit128(char *buffer, const Int128Parts *value, uint8_t forcePlus) {
    uint64_t high = (uint64_t)value->high;
    uint64_t low = value->low;
    if ((int64_t)high < 0) {
        high = low == 0 ? -high : ~high;
        low = -low;
        *buffer++ = '-';
    } else if (forcePlus) {
        *buffer++ = '+';
    }
    bool leading = true;
    for (size_t index = 0; index != 39; ++index) {
        uint8_t digit = 0;
        const Int128Parts *power = &powersOf10[index];
        const Int128Parts *negated = &neg_powersOf10[index];
        int64_t powerHigh = power->high;
        while (powerHigh < (int64_t)high || (powerHigh <= (int64_t)high && power->low <= low)) {
            uint64_t addend = negated->low;
            high += (uint64_t)negated->high + (low + addend < low);
            low += addend;
            ++digit;
        }
        if (digit != 0 || !leading) {
            leading = false;
            *buffer++ = (char)(digit + '0');
        }
    }
    if (leading) {
        *buffer++ = '0';
    }
    *buffer = 0;
}

// Inferred organization: CFBoolean (0x1c01abab8-0x1c01abbcf) and CFNumber (0x1c01abbd0-0x1c01ae047, around
// emit128) are contiguous in the iOS Simulator __text section and share one translation unit, with the cold
// fragments CFNumberGetTypeID.cold.1 and CFNumberCreate.cold.1 (0x1c0308c04, 0x1c0308c08, both
// _OUTLINED_FUNCTION_0_16, the dispatch_once of CFNumberGetTypeID); the native source path is not in the
// evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446: unread. Reading: not yet
// written; the evidence is the assembly in Local/Evidence/cf-core/iphone-simulator-core-corpus.json.
// Folds the compiler-outlined piece _OUTLINED_FUNCTION_0_16 (0x1c01ae048).

// libobjc interfaces the native image uses (re-exported libobjc.A.dylib); supplied, never reimplemented. The
// tagged-pointer variables are read for each decode, as natively, through the GOT slots 0x222409408
// (objc_debug_tag60_permutations), 0x222409418 (objc_debug_taggedpointer_mask) and 0x222409420
// (objc_debug_taggedpointer_obfuscator); the slot-to-symbol binding is by use and slot order (confidence:
// medium), as in Runtime/OCFRuntime.c.
extern uintptr_t objc_debug_taggedpointer_obfuscator;
extern const uint8_t objc_debug_tag60_permutations[8];
extern uintptr_t objc_debug_taggedpointer_mask;
extern void _objc_registerTaggedPointerClass(uint16_t tag, Class cls);

// Bridged classes stored in the static objects and registered for tag 3.
extern char bridgedBooleanClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSCFBoolean));
extern char bridgedNumberClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSCFNumber));

// The CFNumber object: the value at offset 16, 8 bytes (16 for kCFNumberSInt128Type). Info bits 0-2 (of the
// six bits 0-5 the native code writes) index __CFNumberCanonicalTypes. A tagged number (tag 3) carries the
// same index in payload bits 3-5 and the value from bit 7 (see taggedNumberPayload).
enum {
    kNumberSInt128Type = 17,
};

static void __CFBooleanDeallocate(OCFTypeRef cf);
static OCFHashCode __CFBooleanHash(OCFTypeRef cf);
static OCFStringRef __CFBooleanCopyDescription(OCFTypeRef cf);
static Boolean __CFNumberEqual(OCFTypeRef cf1, OCFTypeRef cf2);
static void __CFNumberGetValue(OCFNumberRef number, OCFNumberType type, void *valuePtr);
static OCFStringRef
__CFNumberCreateFormattingDescriptionAsFloat64(OCFAllocatorRef allocator, OCFNumberRef number);
static Boolean __CFNumberGetValueCompat(OCFNumberRef number, OCFNumberType type, void *valuePtr);

// Native __CFNumberCanonicalTypes, 0x1c0375260 (__TEXT,__const, seven 8-byte types: kCFNumberSInt8Type to
// kCFNumberFloat64Type and 17, kCFNumberSInt128Type). Non-external. The native reads index the table with
// three bits; index 7 reads the first eight bytes of __CFNumberTypeTable, which follows at 0x1c0375298, and
// the restoration stores those bytes in an eighth slot. No number this unit creates has index 7.
static const OCFNumberType __CFNumberCanonicalTypes[8] = {
    1, 2, 3, 4, 5, 6, kNumberSInt128Type, (OCFNumberType)UINT64_C(0x0103008200010000)
};

// Native __CFNumberTypeTable, 0x1c0375298 (__TEXT,__const, twenty 16-bit entries indexed by CFNumberType).
// Non-external. Bits 0-4 the canonical type, bit 5 float, bit 6 the wide storage (Float64, SInt128), bits 7-9
// log2 of the byte size. Types 0, 18 and 19 are zero; the native readers do not check the type, and a type
// above 19 reads past the table (into powersOf10) natively and is undefined here.
static const uint16_t __CFNumberTypeTable[20] = {
    0x0000, 0x0001, 0x0082, 0x0103, 0x0184, 0x0125, 0x01e6, 0x0001, 0x0082, 0x0103,
    0x0184, 0x0184, 0x0125, 0x01e6, 0x0184, 0x0184, 0x01e6, 0x0251, 0x0000, 0x0000,
};

// Native __CFNumberCanonicalTypeIndex, 0x1c03757a0 (__TEXT,__const). Non-external. The index of each
// canonical type in __CFNumberCanonicalTypes; CFNumberCreate indexes it with the canonical types 0-6 and 17
// only, so the eighteen bytes read are restored.
static const uint8_t __CFNumberCanonicalTypeIndex[18] = {
    0,
    0,
    1,
    2,
    3,
    4,
    5,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    6,
};

// Native __CFNumberTaggedPointersDisabled (0x22e6d2c60), __CFNumberCaching (0x22e6d2c61; 1 disables the
// integer cache, 2 also tagged numbers) and __CFNumberCache (0x22e6d2c68, 112 bytes: the numbers -1 to 12),
// __DATA,__bss. Non-external.
static uint8_t __CFNumberTaggedPointersDisabled;
static uint8_t __CFNumberCaching;
static OCFNumberRef __CFNumberCache[14];

// The static numbers, iOS Simulator 24A434 (__DATA_CONST,__const, 24 bytes each; isa __NSCFNumber, info
// 0x1685 for Float64 or 0x1684 for Float32: type 22, static, canonical index 5 or 4): __kCFNumberNaN
// 0x2227a2578,
// __kCFNumberNegativeInfinity 0x2227a2590, __kCFNumberPositiveInfinity 0x2227a25a8, __kCFNumberFloat64Zero
// 0x2227a25c0, __kCFNumberFloat64One 0x2227a25d8, __kCFNumberFloat32Zero 0x2227a25f0 and
// __kCFNumberFloat32One 0x2227a2608. Non-external. The public kCFNumberNaN, kCFNumberNegativeInfinity and
// kCFNumberPositiveInfinity pointers (0x2227a24e0-0x2227a24f0) follow the static numbers.
typedef struct {
    OCFRuntimeBase base;
    uint64_t bits;
} StaticNumber;

static const StaticNumber __kCFNumberNaN = {
    {(uintptr_t)bridgedNumberClass, 0x1685}, UINT64_C(0x7FF8000000000000)
};
static const StaticNumber __kCFNumberNegativeInfinity = {
    {(uintptr_t)bridgedNumberClass, 0x1685}, UINT64_C(0xFFF0000000000000)
};
static const StaticNumber __kCFNumberPositiveInfinity = {
    {(uintptr_t)bridgedNumberClass, 0x1685}, UINT64_C(0x7FF0000000000000)
};
static const StaticNumber __kCFNumberFloat64Zero = {{(uintptr_t)bridgedNumberClass, 0x1685}, 0};
static const StaticNumber __kCFNumberFloat64One = {
    {(uintptr_t)bridgedNumberClass, 0x1685}, UINT64_C(0x3FF0000000000000)
};
static const StaticNumber __kCFNumberFloat32Zero = {{(uintptr_t)bridgedNumberClass, 0x1684}, 0};
static const StaticNumber __kCFNumberFloat32One = {
    {(uintptr_t)bridgedNumberClass, 0x1684}, UINT64_C(0x3F800000)
};

// Native kCFNumberNaN (0x2227a24e0), kCFNumberNegativeInfinity (0x2227a24e8) and kCFNumberPositiveInfinity
// (0x2227a24f0), __DATA_CONST,__const. External. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta6.json ("data"); each pointer, read with
// Local/Scratch/cf-core/macho.py, is the address of the static number above (0x2227a2578, 0x2227a2590 and
// 0x2227a25a8).
const OCFNumberRef kOCFNumberNaN = (OCFNumberRef)&__kCFNumberNaN;
const OCFNumberRef kOCFNumberNegativeInfinity = (OCFNumberRef)&__kCFNumberNegativeInfinity;
const OCFNumberRef kOCFNumberPositiveInfinity = (OCFNumberRef)&__kCFNumberPositiveInfinity;

// Native __kCFBooleanTrue and __kCFBooleanFalse, 0x2227cc260 and 0x2227cc270 (__DATA_CONST,__const_cfobj2, 16
// bytes each; isa __NSCFBoolean, info 0x1580: type 21, static), and kCFBooleanTrue and kCFBooleanFalse,
// 0x2227a8568 and 0x2227a8570 (__DATA_CONST,__const). External. The restoration does not place the objects in
// a __const_cfobj2 section.
const OCFRuntimeBase __kOCFBooleanTrue = {(uintptr_t)bridgedBooleanClass, 0x1580};
const OCFRuntimeBase __kOCFBooleanFalse = {(uintptr_t)bridgedBooleanClass, 0x1580};
const OCFBooleanRef kOCFBooleanTrue = (OCFBooleanRef)&__kOCFBooleanTrue;
const OCFBooleanRef kOCFBooleanFalse = (OCFBooleanRef)&__kOCFBooleanFalse;

// Native __CFBooleanClass, 0x2227a2480 (__DATA_CONST,__const). Hidden (was private external); version 0, name
// "CFBoolean", finalize (slot 4), hash (6), formatting description (7) and debug description (8); the
// record's bytes read from the pinned image, the callbacks from IDA (corpus delta capture).
const OCFRuntimeClass __CFBooleanClass = {
    0,
    "CFBoolean",
    NULL,
    NULL,
    __CFBooleanDeallocate,
    NULL,
    __CFBooleanHash,
    __CFBooleanCopyFormattingDescription,
    __CFBooleanCopyDescription,
    NULL,
    NULL,
    0,
};

// Native __CFNumberClass, 0x2227a24f8 (__DATA_CONST,__const). Hidden (was private external); version 0, name
// "CFNumber", equal, hash and both description callbacks.
const OCFRuntimeClass __CFNumberClass = {
    0,
    "CFNumber",
    NULL,
    NULL,
    NULL,
    __CFNumberEqual,
    __CFNumberHash,
    __CFNumberCopyFormattingDescription,
    __CFNumberCopyDescription,
    NULL,
    NULL,
    0,
};

static bool isTagged(OCFTypeRef cf) {
    return (OF_WORD_SHIFT((uintptr_t)cf, 63)) != 0;
}

static uint64_t infoOf(OCFTypeRef cf) {
    return __atomic_load_n(&((const OCFRuntimeBase *)cf)->info, __ATOMIC_ACQUIRE);
}

// One native CASAL loop: the bits in clear are replaced by set.
static void updateInfo(OCFTypeRef cf, uint64_t clear, uint64_t set) {
    uint64_t *info = &((OCFRuntimeBase *)cf)->info;
    uint64_t current = __atomic_load_n(info, __ATOMIC_ACQUIRE);
    uint64_t updated;
    do {
        updated = (current & ~clear) | set;
    } while (
        !__atomic_compare_exchange_n(info, &current, updated, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
}

// The deobfuscated value of a tagged pointer, decoded inline as the native functions do: the obfuscator is
// skipped for pointers with bits 63, 62 and 0-2 set.
static uintptr_t taggedValue(OCFTypeRef cf) {
    uintptr_t pointer = (uintptr_t)cf;
    uintptr_t obfuscator = objc_debug_taggedpointer_obfuscator;
    if ((pointer & UINT64_C(0xC000000000000007)) == UINT64_C(0xC000000000000007)) {
        obfuscator = 0;
    }
    return obfuscator ^ pointer;
}

// The position of the value's low three bits in the seven-entry permutation table (the basic tag), or 7 when
// they are absent (an extended tag). The native readers accept a number for position 3 only.
static uintptr_t taggedSlot(uintptr_t value) {
    uintptr_t index = 0;
    while (index != 7 && (value & 7) != objc_debug_tag60_permutations[index]) {
        ++index;
    }
    return index;
}

// The signed value of a tagged number: bits 3-54 (low three bits 7, an extended tag) or bits 3-62, sign
// extended, shifted right arithmetically by 6 when payload bit 6 is set and by 4 otherwise.
static int64_t taggedNumberPayload(uintptr_t value) {
    int64_t payload;
    if ((value & 7) == 7) {
        payload = (int64_t)(value << 9) >> 12;
    } else {
        payload = (int64_t)(value << 1) >> 4;
    }
    return payload >> ((value & 0x40) != 0 ? 6 : 4);
}

// The word whose low three bits index __CFNumberCanonicalTypes: value >> 3 for a tagged number (position 3),
// else the info word (also read, as natively, for a tagged pointer of another class).
static uint64_t numberTypeWord(OCFTypeRef number) {
    if (isTagged(number)) {
        uintptr_t value = taggedValue(number);
        if (taggedSlot(value) == 3) {
            return value >> 3;
        }
    }
    return infoOf(number);
}

static const uint8_t *numberValue(OCFTypeRef number) {
    return (const uint8_t *)number + 16;
}

static int64_t readInt64(const void *bytes) {
    int64_t value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

static uint64_t readUInt64(const void *bytes) {
    uint64_t value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

static uint32_t readUInt32(const void *bytes) {
    uint32_t value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

static double readDouble(const void *bytes) {
    double value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

static float readFloat(const void *bytes) {
    float value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

static void writeBits64(void *valuePtr, uint64_t bits) {
    memcpy(valuePtr, &bits, sizeof(bits));
}

static void writeBits32(void *valuePtr, uint32_t bits) {
    memcpy(valuePtr, &bits, sizeof(bits));
}

static void writeInt128(void *valuePtr, int64_t high, uint64_t low) {
    Int128Parts parts = {high, low};
    memcpy(valuePtr, &parts, sizeof(parts));
}

// The arm64 FCVTZS, FCVTZU and FCVTMS conversions the native code uses: NaN converts to 0 and values out of
// range saturate (a C conversion would be undefined there). A float operand converts exactly through double.
static int32_t convertToInt32(double value) {
    if (value != value) {
        return 0;
    }
    if (value >= 2147483648.0) {
        return INT32_MAX;
    }
    if (value < -2147483648.0) {
        return INT32_MIN;
    }
    return (int32_t)value;
}

static int64_t convertToInt64(double value) {
    if (value != value) {
        return 0;
    }
    if (value >= 9223372036854775808.0) {
        return INT64_MAX;
    }
    if (value < -9223372036854775808.0) {
        return INT64_MIN;
    }
    return (int64_t)value;
}

static uint64_t convertToUInt64(double value) {
    if (!(value > -1.0)) {
        return 0;
    }
    if (value >= 18446744073709551616.0) {
        return UINT64_MAX;
    }
    return (uint64_t)value;
}

static int64_t convertToInt64RoundingDown(double value) {
    return convertToInt64(floor(value));
}

// The 128-bit value high:low as a double, as the native code computes it: the two's-complement magnitude in
// two unsigned halves (high negated, or complemented when the low half is nonzero), one fused multiply-add of
// the high half by 2^64 and the low half, negated (FNMADD) for a negative value.
static double doubleFromInt128(int64_t high, uint64_t low) {
    uint64_t magnitudeHigh = (uint64_t)high;
    uint64_t magnitudeLow = low;
    if (high < 0) {
        magnitudeHigh = low != 0 ? ~(uint64_t)high : 0 - (uint64_t)high;
        magnitudeLow = 0 - low;
    }
    double upper = (double)magnitudeHigh;
    double lower = (double)magnitudeLow;
    if (high > -1) {
        return fma(upper, 0x1p64, lower);
    }
    return -fma(upper, 0x1p64, lower);
}

// The in-range double to 128-bit conversion: the high half is value * 2^-64 rounded down (FCVTMS), the low
// half the unsigned truncation (FCVTZU) of fma(floor(value * 2^-64), -2^64, value).
static void int128FromDouble(double value, int64_t *high, uint64_t *low) {
    double scaled = value * 0x1p-64;
    double upper = floor(scaled);
    *high = convertToInt64RoundingDown(scaled);
    *low = convertToUInt64(fma(upper, -0x1p64, value));
}

// Native __CFBooleanCopyFormattingDescription, 0x1c01abab8. Hidden (was private external). The format
// options are not read; the constant "true" for kCFBooleanTrue (the pointer variable is loaded and
// compared), else "false", retained through a tail call to CFRetain. IDA (corpus delta capture).
OCFStringRef __CFBooleanCopyFormattingDescription(OCFTypeRef cf, const void *formatOptions) {
    (void)formatOptions;
    return OCFRetain(cf == kOCFBooleanTrue ? OF_STR("true") : OF_STR("false"));
}

// Native __CFBooleanDeallocate, 0x1c01abae0. Non-external; a bare RET. IDA (corpus delta capture).
static void __CFBooleanDeallocate(OCFTypeRef cf) {
    (void)cf;
}

// Native __CFBooleanHash, 0x1c01abae4. Non-external. 0x9E3779B1 (zero-extended) for kCFBooleanTrue, else 0.
// IDA (corpus delta capture).
static OCFHashCode __CFBooleanHash(OCFTypeRef cf) {
    return cf == kOCFBooleanTrue ? 0x9E3779B1 : 0;
}

// Native __CFBooleanCopyDescription, 0x1c01abb04. Non-external. kCFAllocatorSystemDefault is loaded before
// CFGetAllocator is called; the string is created in the system default allocator, with the object, its
// allocator and the C string "true" or "false". IDA (corpus delta capture).
static OCFStringRef __CFBooleanCopyDescription(OCFTypeRef cf) {
    OCFAllocatorRef systemDefault = kOCFAllocatorSystemDefault;
    OCFAllocatorRef allocator = OCFGetAllocator(cf);
    return OCFStringCreateWithFormat(
        systemDefault,
        NULL,
        OF_STR("<CFBoolean %p [%p]>{value = %s}"),
        cf,
        allocator,
        cf == kOCFBooleanTrue ? "true" : "false"
    );
}

// Native CFBooleanGetTypeID, 0x1c01abb78. External.
OCFTypeID OCFBooleanGetTypeID(void) {
    return 21;
}

// Native CFBooleanGetValue, 0x1c01abb80. External. Tagged pointers and Objective-C objects answer -boolValue;
// a CF boolean is true when it is kCFBooleanTrue.
Boolean OCFBooleanGetValue(OCFBooleanRef boolean) {
    if (isTagged(boolean) || __CF_IS_OBJC(21, boolean) != 0) {
        return ((Boolean (*)(id, SEL))objc_msgSend)((id)boolean, sel_getUid("boolValue"));
    }
    return boolean == kOCFBooleanTrue;
}

// Native __CFNumberCopyFormattingDescriptionAsFloat64, 0x1c01abbd0. Hidden (was private external).
OCFStringRef __CFNumberCopyFormattingDescriptionAsFloat64(OCFTypeRef cf) {
    return __CFNumberCreateFormattingDescriptionAsFloat64(kOCFAllocatorSystemDefault, (OCFNumberRef)cf);
}

// Native __CFNumberCreateFormattingDescriptionAsFloat64, 0x1c01abbe4. Non-external. The Float64 value through
// CFNumberGetValue: "nan", "+infinity", "-infinity" and "0.0" (either zero) are retained constants, other
// values "%.*g" with 17 digits.
static OCFStringRef
__CFNumberCreateFormattingDescriptionAsFloat64(OCFAllocatorRef allocator, OCFNumberRef number) {
    double value = 0.0;
    OCFNumberGetValue(number, kOCFNumberFloat64Type, &value);
    OCFStringRef constant;
    if (value != value) {
        constant = OF_STR("nan");
    } else if ((readUInt64(&value) & UINT64_C(0x7FFFFFFFFFFFFFFF)) == UINT64_C(0x7FF0000000000000)) {
        constant = value > 0.0 ? OF_STR("+infinity") : OF_STR("-infinity");
    } else if (value == 0.0) {
        constant = OF_STR("0.0");
    } else {
        return OCFStringCreateWithFormat(allocator, NULL, OF_STR("%.*g"), 17, value);
    }
    return OCFRetain(constant);
}

// Native __CFNumberCreateFormattingDescription, 0x1c01abcc8 (corpus delta capture 7). Hidden (was private
// external). The formatting options (X2) are not read. As __CFNumberCopyFormattingDescription with the given
// allocator: a floating-point number (type table bit 5 of its canonical type, the type word read through the
// tagged-pointer decode inline) is a tail call of __CFNumberCreateFormattingDescriptionAsFloat64; any other
// its 128-bit value (zeroed, then __CFNumberGetValue with kCFNumberSInt128Type) through emit128 (no forced
// sign) and "%s".
OCFStringRef
__CFNumberCreateFormattingDescription(OCFAllocatorRef allocator, OCFTypeRef cf, const void *formatOptions) {
    (void)formatOptions;
    OCFNumberRef number = (OCFNumberRef)cf;
    if ((__CFNumberTypeTable[__CFNumberCanonicalTypes[numberTypeWord(number) & 7]] & 0x20) != 0) {
        return __CFNumberCreateFormattingDescriptionAsFloat64(allocator, number);
    }
    Int128Parts value = {0, 0};
    __CFNumberGetValue(number, kNumberSInt128Type, &value);
    char buffer[128];
    emit128(buffer, &value, 0);
    return OCFStringCreateWithFormat(allocator, NULL, OF_STR("%s"), buffer);
}

// Native __CFNumberGetValue, 0x1c01abe08. Non-external; the result register is not written (callers ignore
// it) and the restoration returns void. The stored value (a tagged number reads as its signed 64-bit payload)
// is converted to the canonical type of type, saturating:
// - SInt8, SInt16, SInt32, SInt64 from floating point: the value is clamped above to the type's maximum
// (127.0,
//   32767.0, 2147483647.0 or 2^63 for doubles; 127.0f, 32767.0f, 2^31 or 2^63 for floats), converted with
//   FCVTZS, and replaced by the minimum when the value is below -2^(n-1) (NaN converts to 0);
// - from SInt64 storage: clamped to the type's range; from SInt128 storage (high:low): the maximum for a
// positive
//   high half or a zero high half with a low half above the maximum, the minimum for a high half below -1 or
//   -1 with a low half at or below the minimum's complement, else the low half truncated;
// - Float32 from Float64: NaN 0x7fc00000, infinities kept, else clamped to +-FLT_MAX; Float64 from Float32:
//   NaN 0x7ff8000000000000, infinities kept, else the exact value clamped to +-DBL_MAX; integers convert
//   directly (SCVTF) or, for SInt128, through doubleFromInt128;
// - SInt128 from floating point: below -2^127 the minimum, at or above 2^127 the maximum (NaN converts
// through
//   int128FromDouble), else int128FromDouble; from SInt64 the sign-extended value.
// Other requested types store nothing.
static void __CFNumberGetValue(OCFNumberRef number, OCFNumberType type, void *valuePtr) {
    uint16_t requested = __CFNumberTypeTable[type];
    uint64_t word = numberTypeWord(number);
    unsigned requestedType = requested & 0x1f;
    const uint8_t *data = numberValue(number);
    uint16_t storage = __CFNumberTypeTable[__CFNumberCanonicalTypes[word & 7]];
    bool isFloat = (storage & 0x20) != 0;
    bool isWide = (storage & 0x40) != 0;
    int64_t payload = 0;
    if (isTagged(number)) {
        uintptr_t value = taggedValue(number);
        if (taggedSlot(value) == 3) {
            isWide = false;
            isFloat = false;
            payload = taggedNumberPayload(value);
            data = (const uint8_t *)&payload;
        }
    }
    switch (requestedType) {
        case kOCFNumberSInt8Type: {
            int8_t result;
            if (isFloat) {
                int32_t converted;
                bool below;
                if (isWide) {
                    double value = readDouble(data);
                    converted = convertToInt32(value > 127.0 ? 127.0 : value);
                    below = value < -128.0;
                } else {
                    float value = readFloat(data);
                    converted = convertToInt32((double)(value > 127.0f ? 127.0f : value));
                    below = value < -128.0f;
                }
                result = below ? INT8_MIN : (int8_t)converted;
            } else {
                int64_t high = readInt64(data);
                if (!isWide) {
                    int64_t clamped = high > -128 ? high : -128;
                    result = (int8_t)(clamped < 127 ? clamped : 127);
                } else if (high > 0) {
                    result = INT8_MAX;
                } else {
                    uint64_t low = readUInt64(data + 8);
                    if (high == 0 && low > 0x7f) {
                        result = INT8_MAX;
                    } else {
                        result = low <= UINT64_C(0xFFFFFFFFFFFFFF7F) && high == -1 ? INT8_MIN : (int8_t)low;
                        if (high < -1) {
                            result = INT8_MIN;
                        }
                    }
                }
            }
            *(int8_t *)valuePtr = result;
            break;
        }
        case kOCFNumberSInt16Type: {
            int16_t result;
            if (isFloat) {
                int32_t converted;
                bool below;
                if (isWide) {
                    double value = readDouble(data);
                    converted = convertToInt32(value > 32767.0 ? 32767.0 : value);
                    below = value < -32768.0;
                } else {
                    float value = readFloat(data);
                    converted = convertToInt32((double)(value > 32767.0f ? 32767.0f : value));
                    below = value < -32768.0f;
                }
                result = below ? INT16_MIN : (int16_t)converted;
            } else {
                int64_t high = readInt64(data);
                if (!isWide) {
                    int64_t clamped = high > -32768 ? high : -32768;
                    result = (int16_t)(clamped < 32767 ? clamped : 32767);
                } else if (high > 0) {
                    result = INT16_MAX;
                } else {
                    uint64_t low = readUInt64(data + 8);
                    if (high == 0 && (low >> 15) != 0) {
                        result = INT16_MAX;
                    } else {
                        result = low <= UINT64_C(0xFFFFFFFFFFFF7FFF) && high == -1 ? INT16_MIN : (int16_t)low;
                        if (high < -1) {
                            result = INT16_MIN;
                        }
                    }
                }
            }
            *(int16_t *)valuePtr = result;
            break;
        }
        case kOCFNumberSInt32Type: {
            int32_t result;
            if (isFloat) {
                int32_t converted;
                bool below;
                if (isWide) {
                    double value = readDouble(data);
                    converted = convertToInt32(value > 2147483647.0 ? 2147483647.0 : value);
                    below = value < -2147483648.0;
                } else {
                    float value = readFloat(data);
                    converted = convertToInt32((double)(value > 0x1p31f ? 0x1p31f : value));
                    below = value < -0x1p31f;
                }
                result = below ? INT32_MIN : converted;
            } else {
                int64_t high = readInt64(data);
                if (!isWide) {
                    int64_t clamped = high > INT32_MIN ? high : INT32_MIN;
                    result = (int32_t)(clamped < INT32_MAX ? clamped : INT32_MAX);
                } else if (high > 0) {
                    result = INT32_MAX;
                } else {
                    uint64_t low = readUInt64(data + 8);
                    if (high == 0 && (low >> 31) != 0) {
                        result = INT32_MAX;
                    } else {
                        result = low <= UINT64_C(0xFFFFFFFF7FFFFFFF) && high == -1 ? INT32_MIN : (int32_t)low;
                        if (high < -1) {
                            result = INT32_MIN;
                        }
                    }
                }
            }
            *(int32_t *)valuePtr = result;
            break;
        }
        case kOCFNumberSInt64Type: {
            int64_t result;
            if (isFloat) {
                int64_t converted;
                bool below;
                if (isWide) {
                    double value = readDouble(data);
                    converted = convertToInt64(value > 0x1p63 ? 0x1p63 : value);
                    below = value < -0x1p63;
                } else {
                    float value = readFloat(data);
                    converted = convertToInt64((double)(value > 0x1p63f ? 0x1p63f : value));
                    below = value < -0x1p63f;
                }
                result = below ? INT64_MIN : converted;
            } else {
                int64_t high = readInt64(data);
                if (!isWide) {
                    result = high;
                } else if (high > 0) {
                    result = INT64_MAX;
                } else {
                    int64_t low = readInt64(data + 8);
                    if (high == 0 && low < 0) {
                        result = INT64_MAX;
                    } else {
                        result = low >= 0 && high == -1 ? INT64_MIN : low;
                        if (high < -1) {
                            result = INT64_MIN;
                        }
                    }
                }
            }
            *(int64_t *)valuePtr = result;
            break;
        }
        case kOCFNumberFloat32Type: {
            if (!isFloat) {
                int64_t high = readInt64(data);
                float result;
                if (!isWide) {
                    result = (float)high;
                    if (result > FLT_MAX) {
                        result = FLT_MAX;
                    }
                } else {
                    result = (float)doubleFromInt128(high, readUInt64(data + 8));
                }
                *(float *)valuePtr = result;
            } else if (!isWide) {
                writeBits32(valuePtr, readUInt32(data));
            } else {
                double value = readDouble(data);
                if (value != value) {
                    writeBits32(valuePtr, 0x7FC00000);
                } else if (
                    (readUInt64(&value) & UINT64_C(0x7FFFFFFFFFFFFFFF)) == UINT64_C(0x7FF0000000000000)
                ) {
                    writeBits32(valuePtr, value < 0.0 ? 0xFF800000 : 0x7F800000);
                } else {
                    float clamped = (float)(value > (double)FLT_MAX ? (double)FLT_MAX : value);
                    *(float *)valuePtr = value < -(double)FLT_MAX ? -FLT_MAX : clamped;
                }
            }
            break;
        }
        case kOCFNumberFloat64Type: {
            if (!isFloat) {
                int64_t high = readInt64(data);
                double result;
                if (!isWide) {
                    result = (double)high;
                    if (result > DBL_MAX) {
                        result = DBL_MAX;
                    }
                } else {
                    result = doubleFromInt128(high, readUInt64(data + 8));
                }
                *(double *)valuePtr = result;
            } else if (isWide) {
                writeBits64(valuePtr, readUInt64(data));
            } else {
                float value = readFloat(data);
                if (value != value) {
                    writeBits64(valuePtr, UINT64_C(0x7FF8000000000000));
                } else if ((readUInt32(&value) & 0x7FFFFFFF) == 0x7F800000) {
                    writeBits64(
                        valuePtr, value < 0.0f ? UINT64_C(0xFFF0000000000000) : UINT64_C(0x7FF0000000000000)
                    );
                } else {
                    double widened = (double)value;
                    double clamped = widened > DBL_MAX ? DBL_MAX : widened;
                    *(double *)valuePtr = widened < -DBL_MAX ? -DBL_MAX : clamped;
                }
            }
            break;
        }
        case kNumberSInt128Type: {
            if (!isFloat) {
                if (isWide) {
                    memcpy(valuePtr, data, 16);
                } else {
                    int64_t value = readInt64(data);
                    writeInt128(valuePtr, value >> 63, (uint64_t)value);
                }
                break;
            }
            double value;
            bool below;
            bool above;
            if (isWide) {
                value = readDouble(data);
                below = value < -0x1p127;
                above = !below && value >= 0x1p127;
            } else {
                float single = readFloat(data);
                below = single < -0x1p127f;
                above = !below && single >= 0x1p127f;
                value = (double)single;
            }
            int64_t high;
            uint64_t low;
            if (below) {
                high = INT64_MIN;
                low = 0;
            } else if (above) {
                high = INT64_MAX;
                low = UINT64_MAX;
            } else {
                int128FromDouble(value, &high, &low);
            }
            writeInt128(valuePtr, high, low);
            break;
        }
        default:
            break;
    }
}

// Native __CFNumberCopyFormattingDescription, 0x1c01ac5d4. Hidden (was private external). Floating-point
// numbers use __CFNumberCreateFormattingDescriptionAsFloat64; integers their 128-bit value through emit128
// (no forced sign) and "%s".
OCFStringRef __CFNumberCopyFormattingDescription(OCFTypeRef cf, const void *formatOptions) {
    (void)formatOptions;
    OCFNumberRef number = (OCFNumberRef)cf;
    OCFNumberType type = __CFNumberCanonicalTypes[numberTypeWord(number) & 7];
    if ((__CFNumberTypeTable[type] & 0x20) != 0) {
        return __CFNumberCreateFormattingDescriptionAsFloat64(kOCFAllocatorSystemDefault, number);
    }
    Int128Parts value = {0, 0};
    __CFNumberGetValue(number, kNumberSInt128Type, &value);
    char buffer[128];
    emit128(buffer, &value, 0);
    return OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR("%s"), buffer);
}

// Native __CFNumberEqual, 0x1c01ac724. Non-external.
static Boolean __CFNumberEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    return OCFNumberCompare((OCFNumberRef)cf1, (OCFNumberRef)cf2, NULL) == kOCFCompareEqualTo;
}

// Native __CFNumberHash, 0x1c01ac740. Hidden here (non-external natively; the runtime hashes tagged numbers
// with it). SInt8, SInt16 and SInt32 numbers hash their 32-bit magnitude times 2654435761; others the Float64
// value d: with a = |d|, i = floor(a + 0.5), the integral part fma(-trunc(i * 2^-64), 2^64, i) converted
// (FCVTZU) times 2654435761, plus (FCVTZU) or minus (FCVTZU of the magnitude) (a - i) * 2^64 by its sign.
OCFHashCode __CFNumberHash(OCFTypeRef cf) {
    OCFNumberRef number = (OCFNumberRef)cf;
    OCFNumberType type = __CFNumberCanonicalTypes[numberTypeWord(number) & 7];
    if ((uint64_t)(type - 1) <= 2) {
        int32_t value = 0;
        __CFNumberGetValue(number, kOCFNumberSInt32Type, &value);
        uint32_t magnitude = value < 0 ? 0 - (uint32_t)value : (uint32_t)value;
        return (uint64_t)2654435761U * magnitude;
    }
    double value = 0.0;
    __CFNumberGetValue(number, kOCFNumberFloat64Type, &value);
    double magnitude = value < 0.0 ? -value : value;
    double integral = floor(magnitude + 0.5);
    double fraction = magnitude - integral;
    double reduced = fma(-trunc(integral * 0x1p-64), 0x1p64, integral);
    uint64_t integralHash = convertToUInt64(reduced) * 2654435761U;
    double scaled = fraction * 0x1p64;
    uint64_t positive = scaled > 0.0 ? integralHash + convertToUInt64(scaled) : integralHash;
    uint64_t negative = integralHash - convertToUInt64(fabs(scaled));
    return scaled < 0.0 ? negative : positive;
}

// Native __CFNumberCopyDescription, 0x1c01ac8b4. Hidden here (non-external natively; the runtime describes
// tagged numbers with it). Integers write their 128-bit value with a forced sign (emit128) and the canonical
// type's name; floating-point numbers write "nan", "+infinity", "-infinity", "+0.0", "-0.0" or "%+.*f" with
// 10 (Float32) or 20 (Float64) digits, and the name of the type index (4 Float32, 5 Float64). The type names
// are split literals so that they stay native text in both naming modes.
OCFStringRef __CFNumberCopyDescription(OCFTypeRef cf) {
    OCFNumberRef number = (OCFNumberRef)cf;
    uint64_t index = numberTypeWord(number) & 7;
    OCFNumberType type = __CFNumberCanonicalTypes[index];
    OCFMutableStringRef result = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
    OCFStringAppendFormat(result, NULL, OF_STR("<CFNumber %p [%p]>{value = "), cf, OCFGetAllocator(cf));
    uint16_t entry = __CFNumberTypeTable[type];
    if ((entry & 0x20) == 0) {
        Int128Parts value = {0, 0};
        __CFNumberGetValue(number, kNumberSInt128Type, &value);
        char buffer[128];
        emit128(buffer, &value, 1);
        const char *name = "unknown integer";
        if (type <= 2) {
            if (type == 2) {
                name = OF_NAME_STRING(kOCFNumberSInt16Type);
            }
            if (type == 1) {
                name = OF_NAME_STRING(kOCFNumberSInt8Type);
            }
        } else if (type == 3) {
            name = OF_NAME_STRING(kOCFNumberSInt32Type);
        } else if (type == 4) {
            name = OF_NAME_STRING(kOCFNumberSInt64Type);
        } else if (type == kNumberSInt128Type) {
            name = "kCFNumberSInt128Type";
        }
        OCFStringAppendFormat(result, NULL, OF_STR("%s, type = %s}"), buffer, name);
        return result;
    }
    double value = 0.0;
    __CFNumberGetValue(number, kOCFNumberFloat64Type, &value);
    if (value != value) {
        OCFStringAppend(result, OF_STR("nan"));
    } else if ((readUInt64(&value) & UINT64_C(0x7FFFFFFFFFFFFFFF)) == UINT64_C(0x7FF0000000000000)) {
        OCFStringAppend(result, value > 0.0 ? OF_STR("+infinity") : OF_STR("-infinity"));
    } else if (value == 0.0) {
        OCFStringAppend(result, readInt64(&value) < 0 ? OF_STR("-0.0") : OF_STR("+0.0"));
    } else {
        OCFStringAppendFormat(result, NULL, OF_STR("%+.*f"), (entry & 0x40) == 0 ? 10 : 20, value);
    }
    const char *name = "unknown float";
    if (index == 5) {
        name = OF_NAME_STRING(kOCFNumberFloat64Type);
    }
    if (index == 4) {
        name = OF_NAME_STRING(kOCFNumberFloat32Type);
    }
    OCFStringAppendFormat(result, NULL, OF_STR(", type = %s}"), name);
    return result;
}

// Native CFNumberGetTypeID, 0x1c01acb6c, with CFNumberGetTypeID.initOnce (0x22e6d2c58, __DATA,__bss),
// CFNumberGetTypeID.cold.1 (0x1c0308c04) and ___CFNumberGetTypeID_block_invoke (0x1c01acb98). External. Once:
// executables linked on or after system version 7 with tagged pointers enabled (objc_debug_taggedpointer_mask
// nonzero) register __NSCFNumber for tag 3, otherwise tagged numbers are disabled; CFNumberDisableCache set
// to "all" disables caching and tagged numbers (2), any other value caching only (1).
OCFTypeID OCFNumberGetTypeID(void) {
    static dispatch_once_t initOnce;
    dispatch_once(&initOnce, ^{
      if (_OCFExecutableLinkedOnOrAfter(7) && objc_debug_taggedpointer_mask != 0) {
          _objc_registerTaggedPointerClass(3, (Class)bridgedNumberClass);
      } else {
          __CFNumberTaggedPointersDisabled = 1;
      }
      const char *value = getenv("CFNumberDisableCache");
      if (value != NULL) {
          __CFNumberCaching = strcmp(value, "all") == 0 ? 2 : 1;
      }
    });
    return 22;
}

// The inlined system-default test of CFNumberCreate (_CFAllocatorIsSystemDefault): the system default itself,
// or NULL or kCFAllocatorDefault while the current default is the system default.
static bool allocatorIsSystemDefault(OCFAllocatorRef allocator) {
    OCFAllocatorRef systemDefault = kOCFAllocatorSystemDefault;
    if (allocator == systemDefault) {
        return true;
    }
    if (allocator != NULL && kOCFAllocatorDefault != allocator) {
        return false;
    }
    return systemDefault == OCFAllocatorGetDefault();
}

// Whether a 64-bit value fits a tagged number: value - 2^55, unsigned, at least 2^64 - 2^56 + 1 (the open
// interval (-2^55, 2^55 - 1]).
static bool fitsTaggedNumber(int64_t value) {
    return (uint64_t)value + UINT64_C(0xFF80000000000000) >= UINT64_C(0xFF00000000000001);
}

// The inlined _objc_encodeTaggedPointer: the value XOR the obfuscator, returned unchanged when that has bits
// 63, 62 and 0-2 set, else with its low three bits replaced by their permutation.
static OCFNumberRef encodeTaggedNumber(uintptr_t value) {
    uintptr_t encoded = objc_debug_taggedpointer_obfuscator ^ value;
    if ((encoded & UINT64_C(0xC000000000000007)) == UINT64_C(0xC000000000000007)) {
        return (OCFNumberRef)value;
    }
    return (OCFNumberRef)((encoded & ~(uintptr_t)7) | objc_debug_tag60_permutations[encoded & 7]);
}

// Native CFNumberCreate, 0x1c01acc14, with CFNumberCreate.cold.1 (0x1c0308c08, the inlined
// CFNumberGetTypeID's dispatch_once). External. A NULL allocator is the thread's default (or the system
// default).
// - Tagged numbers (unless disabled, for a system-default allocator and caching mode other than 2): SInt8,
//   SInt16 and SInt32 always (value << 7 | 0x8000000000000003, 0x0b or 0x13), SInt64 values that fit (|
//   0x1b), and integral Float32 (| 0x23) and Float64 (| 0x2b) values that fit, except -0.0.
// - Float32 and Float64 zero (bit pattern 0), one, NaN and the infinities answer the retained static numbers,
//   whatever the allocator.
// - Integers -1 to 12 for a system-default allocator with caching enabled answer the retained cached number;
// a
//   new one is published in the cache (compare-and-swap) with type index 2 (SInt32) and retained once more,
//   or gets its original index back when another thread published first.
// Otherwise a new number of 8 extra bytes (16 for SInt128) stores the value.
OCFNumberRef OCFNumberCreate(OCFAllocatorRef allocator, OCFNumberType theType, const void *valuePtr) {
    if (allocator == NULL) {
        OCFAllocatorRef current = _OCFGetTSD(1);
        allocator = current != NULL ? current : kOCFAllocatorSystemDefault;
    }
    if ((__CFNumberTaggedPointersDisabled & 1) == 0 && allocatorIsSystemDefault(allocator) &&
        __CFNumberCaching != 2) {
        uintptr_t tagged = 0;
        bool useTagged = false;
        switch (__CFNumberTypeTable[theType] & 0x1f) {
            case kOCFNumberSInt8Type:
                tagged = ((uint64_t)(int64_t)*(const int8_t *)valuePtr << 7) | UINT64_C(0x8000000000000003);
                useTagged = true;
                break;
            case kOCFNumberSInt16Type:
                tagged = ((uint64_t)(int64_t)*(const int16_t *)valuePtr << 7) | UINT64_C(0x800000000000000B);
                useTagged = true;
                break;
            case kOCFNumberSInt32Type:
                tagged = ((uint64_t)(int64_t)*(const int32_t *)valuePtr << 7) | UINT64_C(0x8000000000000013);
                useTagged = true;
                break;
            case kOCFNumberSInt64Type: {
                int64_t value = readInt64(valuePtr);
                if (fitsTaggedNumber(value)) {
                    tagged = UINT64_C(0x800000000000000B) | (uint64_t)value << 7 | 0x10;
                    useTagged = true;
                }
                break;
            }
            case kOCFNumberFloat32Type: {
                float value = readFloat(valuePtr);
                int64_t integral = convertToInt64((double)value);
                if (value == (float)integral) {
                    if (!((int32_t)readUInt32(&value) < 0 && integral == 0) && fitsTaggedNumber(integral)) {
                        tagged = UINT64_C(0x8000000000000023) | (uint64_t)integral << 7;
                        useTagged = true;
                    }
                }
                break;
            }
            case kOCFNumberFloat64Type: {
                double value = readDouble(valuePtr);
                if (value == (double)convertToInt64(value)) {
                    int64_t integral = convertToInt64(value);
                    if (!(readInt64(&value) < 0 && integral == 0) && fitsTaggedNumber(integral)) {
                        tagged = UINT64_C(0x800000000000000B) | (uint64_t)integral << 7 | 0x20;
                        useTagged = true;
                    }
                }
                break;
            }
            default:
                break;
        }
        if (useTagged) {
            return encodeTaggedNumber(tagged);
        }
    }
    uint16_t entry = __CFNumberTypeTable[theType];
    int64_t cacheValue = -2;
    if ((entry & 0x20) != 0) {
        const StaticNumber *shared = NULL;
        if ((entry & 0x40) == 0) {
            uint32_t bits = readUInt32(valuePtr);
            float value = readFloat(valuePtr);
            if (bits == 0) {
                shared = &__kCFNumberFloat32Zero;
            } else if (bits == 0x3F800000) {
                shared = &__kCFNumberFloat32One;
            } else if (value != value) {
                shared = &__kCFNumberNaN;
            } else if ((bits & 0x7FFFFFFF) == 0x7F800000) {
                shared = value < 0.0f ? &__kCFNumberNegativeInfinity : &__kCFNumberPositiveInfinity;
            }
        } else {
            uint64_t bits = readUInt64(valuePtr);
            double value = readDouble(valuePtr);
            if (bits == 0) {
                shared = &__kCFNumberFloat64Zero;
            } else if (bits == UINT64_C(0x3FF0000000000000)) {
                shared = &__kCFNumberFloat64One;
            } else if (value != value) {
                shared = &__kCFNumberNaN;
            } else if ((bits & UINT64_C(0x7FFFFFFFFFFFFFFF)) == UINT64_C(0x7FF0000000000000)) {
                shared = value < 0.0 ? &__kCFNumberNegativeInfinity : &__kCFNumberPositiveInfinity;
            }
        }
        if (shared != NULL) {
            return OCFRetain(shared);
        }
    } else if (allocatorIsSystemDefault(allocator) && __CFNumberCaching == 0) {
        bool cacheable = false;
        switch (entry & 0x1f) {
            case kOCFNumberSInt8Type:
                cacheValue = *(const int8_t *)valuePtr;
                cacheable = (uint8_t)(cacheValue + 1) <= 13;
                break;
            case kOCFNumberSInt16Type:
                cacheValue = *(const int16_t *)valuePtr;
                cacheable = (uint16_t)(cacheValue + 1) <= 13;
                break;
            case kOCFNumberSInt32Type:
                cacheValue = *(const int32_t *)valuePtr;
                cacheable = (uint32_t)(cacheValue + 1) <= 13;
                break;
            case kOCFNumberSInt64Type:
                cacheValue = readInt64(valuePtr);
                cacheable = (uint64_t)cacheValue + 1 <= 13;
                break;
            default:
                break;
        }
        if (!cacheable) {
            cacheValue = -2;
        } else {
            OCFNumberRef cached = __atomic_load_n(&__CFNumberCache[cacheValue + 1], __ATOMIC_RELAXED);
            if (cached != NULL) {
                return OCFRetain(cached);
            }
        }
    }
    OCFIndex extraBytes = (entry & 0x20) == 0 ? (OCFIndex)(((entry >> 3) & 8) + 8) : 8;
    OCFTypeID typeID = OCFNumberGetTypeID();
    uint8_t *number = (uint8_t *)_OCFRuntimeCreateInstance(allocator, typeID, extraBytes, NULL);
    if (number == NULL) {
        return NULL;
    }
    unsigned canonicalType = entry & 0x1f;
    updateInfo(number, 0x3f, __CFNumberCanonicalTypeIndex[canonicalType] & 7);
    switch (canonicalType) {
        case kOCFNumberSInt8Type: {
            int64_t value = *(const int8_t *)valuePtr;
            memcpy(number + 16, &value, 8);
            break;
        }
        case kOCFNumberSInt16Type: {
            int64_t value = *(const int16_t *)valuePtr;
            memcpy(number + 16, &value, 8);
            break;
        }
        case kOCFNumberSInt32Type: {
            int64_t value = *(const int32_t *)valuePtr;
            memcpy(number + 16, &value, 8);
            break;
        }
        case kOCFNumberSInt64Type:
        case kOCFNumberFloat64Type:
            memcpy(number + 16, valuePtr, 8);
            break;
        case kOCFNumberFloat32Type:
            memcpy(number + 16, valuePtr, 4);
            break;
        case kNumberSInt128Type:
            memcpy(number + 16, valuePtr, 16);
            break;
        default:
            break;
    }
    if (cacheValue == -2) {
        return (OCFNumberRef)number;
    }
    memcpy(number + 16, &cacheValue, 8);
    OCFNumberType originalType = __CFNumberCanonicalTypes[numberTypeWord(number) & 7];
    updateInfo(number, 0x3f, 2);
    OCFNumberRef expected = NULL;
    if (__atomic_compare_exchange_n(
            &__CFNumberCache[cacheValue + 1],
            &expected,
            (OCFNumberRef)number,
            false,
            __ATOMIC_SEQ_CST,
            __ATOMIC_SEQ_CST
        )) {
        OCFRetain(number);
    } else {
        updateInfo(number, 0x3f, __CFNumberCanonicalTypeIndex[originalType] & 7);
    }
    return (OCFNumberRef)number;
}

// Native CFNumberGetType, 0x1c01ad25c. External. Tagged numbers (position 3) answer the type of their payload
// index unmapped (the inlined type read decodes the pointer a second time; its info-word fallback is
// unreachable there); other tagged pointers and Objective-C objects answer -_cfNumberType; CF numbers the
// canonical type of their info index. kCFNumberSInt128Type (17) from the message or the info word is reported
// as SInt64.
OCFNumberType OCFNumberGetType(OCFNumberRef number) {
    OCFNumberType type;
    if (isTagged(number)) {
        uintptr_t value = taggedValue(number);
        if (taggedSlot(value) == 3) {
            if (taggedSlot(value) == 3) {
                return __CFNumberCanonicalTypes[(value >> 3) & 7];
            }
            return __CFNumberCanonicalTypes[infoOf(number) & 7];
        }
        type = ((OCFNumberType (*)(id, SEL))objc_msgSend)((id)number, sel_getUid("_cfNumberType"));
    } else if (__CF_IS_OBJC(22, number) != 0) {
        type = ((OCFNumberType (*)(id, SEL))objc_msgSend)((id)number, sel_getUid("_cfNumberType"));
    } else {
        type = __CFNumberCanonicalTypes[infoOf(number) & 7];
    }
    return type == kNumberSInt128Type ? kOCFNumberSInt64Type : type;
}

// Native _CFNumberGetType2, 0x1c01ad358. External. CFNumberGetType without the SInt128 mapping.
OCFNumberType _OCFNumberGetType2(OCFNumberRef number) {
    if (isTagged(number)) {
        uintptr_t value = taggedValue(number);
        if (taggedSlot(value) != 3) {
            return ((OCFNumberType (*)(id, SEL))objc_msgSend)((id)number, sel_getUid("_cfNumberType"));
        }
        if (taggedSlot(value) == 3) {
            return __CFNumberCanonicalTypes[(value >> 3) & 7];
        }
    } else if (__CF_IS_OBJC(22, number) != 0) {
        return ((OCFNumberType (*)(id, SEL))objc_msgSend)((id)number, sel_getUid("_cfNumberType"));
    }
    return __CFNumberCanonicalTypes[infoOf(number) & 7];
}

// Native CFNumberGetByteSize, 0x1c01ad434. External. 2 to the power of the type's size field.
OCFIndex OCFNumberGetByteSize(OCFNumberRef number) {
    return (OCFIndex)1 << ((__CFNumberTypeTable[OCFNumberGetType(number)] >> 7) & 7);
}

// Native CFNumberIsFloatType, 0x1c01ad460. External.
Boolean OCFNumberIsFloatType(OCFNumberRef number) {
    return (__CFNumberTypeTable[OCFNumberGetType(number)] >> 5) & 1;
}

// Native CFNumberGetValue, 0x1c01ad484. External. Tagged pointers of another class and Objective-C objects
// answer -_getValue:forType: with the canonical type (and the original valuePtr). CF numbers use
// __CFNumberGetValueCompat; tagged numbers store their payload truncated to SInt8, SInt16, SInt32 or SInt64
// or converted to Float32 or Float64 and answer true, other types going through __CFNumberGetValueCompat. A
// NULL valuePtr is replaced by a 128-byte scratch buffer.
Boolean OCFNumberGetValue(OCFNumberRef number, OCFNumberType theType, void *valuePtr) {
    uint8_t scratch[128];
    if (!isTagged(number)) {
        if (__CF_IS_OBJC(22, number) == 0) {
            return __CFNumberGetValueCompat(number, theType, valuePtr == NULL ? scratch : valuePtr);
        }
    } else {
        uintptr_t value = taggedValue(number);
        if (taggedSlot(value) == 3) {
            void *destination = valuePtr == NULL ? scratch : valuePtr;
            int64_t payload = taggedNumberPayload(value);
            switch (__CFNumberTypeTable[theType] & 0x1f) {
                case kOCFNumberSInt8Type:
                    *(int8_t *)destination = (int8_t)payload;
                    return true;
                case kOCFNumberSInt16Type:
                    *(int16_t *)destination = (int16_t)payload;
                    return true;
                case kOCFNumberSInt32Type:
                    *(int32_t *)destination = (int32_t)payload;
                    return true;
                case kOCFNumberSInt64Type:
                    *(int64_t *)destination = payload;
                    return true;
                case kOCFNumberFloat32Type:
                    *(float *)destination = (float)payload;
                    return true;
                case kOCFNumberFloat64Type:
                    *(double *)destination = (double)payload;
                    return true;
                default:
                    return __CFNumberGetValueCompat(number, theType, destination);
            }
        }
    }
    return ((Boolean (*)(id, SEL, void *, OCFIndex))objc_msgSend)(
        (id)number, sel_getUid("_getValue:forType:"), valuePtr, __CFNumberTypeTable[theType] & 0x1f
    );
}

// Native __CFNumberGetValueCompat, 0x1c01ad658. Non-external. Stores the value converted to the canonical
// type of type and answers whether it was represented exactly (false for other types, which store nothing). A
// tagged number (position 3) reads as its signed 64-bit payload, with a Float32 or Float64 payload type read
// as SInt64.
// - SInt8 and SInt16 from SInt64 storage store the truncated value; the answer is true for executables linked
//   before system version 6 and otherwise when the value fits the type or is below 2^8 (2^16) as unsigned.
//   SInt32 and the SInt128 low halves answer whether the truncation is exact. SInt64 from SInt128 stores the
//   low half and answers true.
// - Floating point to integers: FCVTZS (32 bits for SInt8, SInt16 and SInt32, truncated to the size; 64 bits
// for
//   SInt64), exact when the value equals its truncation.
// - Float32 from Float64 is exact when it converts back unchanged; Float64 from Float32 unless NaN; integers
//   convert with SCVTF (exact when FCVTZS converts back) or, for SInt128, through doubleFromInt128 (exact
//   when the 128-bit conversion back, with the bounds of __CFNumberGetValue, equals the value).
// - SInt128 from floating point uses the bounds and conversion of __CFNumberGetValue and is exact when
//   doubleFromInt128 (rounded to Float32 for a Float32 source) equals the value; from integers it is exact.
static Boolean __CFNumberGetValueCompat(OCFNumberRef number, OCFNumberType type, void *valuePtr) {
    unsigned requestedType = __CFNumberTypeTable[type] & 0x1f;
    OCFNumberType sourceType = __CFNumberCanonicalTypes[numberTypeWord(number) & 7];
    const uint8_t *data = numberValue(number);
    int64_t payload = 0;
    if (isTagged(number)) {
        uintptr_t value = taggedValue(number);
        if (taggedSlot(value) == 3) {
            payload = taggedNumberPayload(value);
            if ((uint64_t)(sourceType - 5) < 2) {
                sourceType = kOCFNumberSInt64Type;
            }
            data = (const uint8_t *)&payload;
        }
    }
    uint16_t storage;
    bool isFloat;
    bool isWide;
    switch (requestedType) {
        case kOCFNumberSInt8Type:
        case kOCFNumberSInt16Type:
        case kOCFNumberSInt32Type:
        case kOCFNumberSInt64Type:
        case kOCFNumberFloat32Type:
        case kOCFNumberFloat64Type:
        case kNumberSInt128Type:
            storage = __CFNumberTypeTable[sourceType];
            isFloat = (storage & 0x20) != 0;
            isWide = (storage & 0x40) != 0;
            break;
        default:
            return false;
    }
    switch (requestedType) {
        case kOCFNumberSInt8Type:
        case kOCFNumberSInt16Type: {
            bool sixteen = requestedType == kOCFNumberSInt16Type;
            if (isFloat) {
                if (isWide) {
                    double value = readDouble(data);
                    int32_t converted = convertToInt32(value);
                    if (sixteen) {
                        *(int16_t *)valuePtr = (int16_t)converted;
                    } else {
                        *(int8_t *)valuePtr = (int8_t)converted;
                    }
                    return value == trunc(value);
                }
                float value = readFloat(data);
                int32_t converted = convertToInt32((double)value);
                if (sixteen) {
                    *(int16_t *)valuePtr = (int16_t)converted;
                } else {
                    *(int8_t *)valuePtr = (int8_t)converted;
                }
                return value == truncf(value);
            }
            if (isWide) {
                int64_t low = readInt64(data + 8);
                if (sixteen) {
                    *(int16_t *)valuePtr = (int16_t)low;
                    return low == (int16_t)low;
                }
                *(int8_t *)valuePtr = (int8_t)low;
                return low == (int8_t)low;
            }
            int64_t value = readInt64(data);
            bool fits;
            if (sixteen) {
                *(int16_t *)valuePtr = (int16_t)value;
            } else {
                *(int8_t *)valuePtr = (int8_t)value;
            }
            bool linked = _OCFExecutableLinkedOnOrAfter(6);
            if (sixteen) {
                fits = value == (int16_t)value;
                if ((uint64_t)value < 0x10000) {
                    fits = true;
                }
            } else {
                fits = value == (int8_t)value;
                if ((uint64_t)value < 0x100) {
                    fits = true;
                }
            }
            return !linked ? true : fits;
        }
        case kOCFNumberSInt32Type: {
            if (isFloat) {
                if (isWide) {
                    double value = readDouble(data);
                    *(int32_t *)valuePtr = convertToInt32(value);
                    return value == trunc(value);
                }
                float value = readFloat(data);
                *(int32_t *)valuePtr = convertToInt32((double)value);
                return value == truncf(value);
            }
            int64_t value = isWide ? readInt64(data + 8) : readInt64(data);
            *(int32_t *)valuePtr = (int32_t)value;
            return value == (int32_t)value;
        }
        case kOCFNumberSInt64Type: {
            if (isFloat) {
                if (isWide) {
                    double value = readDouble(data);
                    *(int64_t *)valuePtr = convertToInt64(value);
                    return value == trunc(value);
                }
                float value = readFloat(data);
                *(int64_t *)valuePtr = convertToInt64((double)value);
                return value == truncf(value);
            }
            *(int64_t *)valuePtr = isWide ? readInt64(data + 8) : readInt64(data);
            return true;
        }
        case kOCFNumberFloat32Type: {
            if (isFloat) {
                if (isWide) {
                    double value = readDouble(data);
                    float converted = (float)value;
                    *(float *)valuePtr = converted;
                    return value == (double)converted;
                }
                writeBits32(valuePtr, readUInt32(data));
                return true;
            }
            int64_t high = readInt64(data);
            if (!isWide) {
                float converted = (float)high;
                *(float *)valuePtr = converted;
                return high == convertToInt64((double)converted);
            }
            uint64_t low = readUInt64(data + 8);
            float converted = (float)doubleFromInt128(high, low);
            *(float *)valuePtr = converted;
            int64_t backHigh;
            uint64_t backLow;
            if (converted < -0x1p127f) {
                backHigh = INT64_MIN;
                backLow = 0;
            } else if (converted >= 0x1p127f) {
                backHigh = INT64_MAX;
                backLow = UINT64_MAX;
            } else {
                int128FromDouble((double)converted, &backHigh, &backLow);
            }
            return backHigh == high && backLow == low;
        }
        case kOCFNumberFloat64Type: {
            if (isFloat) {
                if (isWide) {
                    writeBits64(valuePtr, readUInt64(data));
                    return true;
                }
                float value = readFloat(data);
                *(double *)valuePtr = (double)value;
                return value == value;
            }
            int64_t high = readInt64(data);
            if (!isWide) {
                double converted = (double)high;
                *(double *)valuePtr = converted;
                return high == convertToInt64(converted);
            }
            uint64_t low = readUInt64(data + 8);
            double converted = doubleFromInt128(high, low);
            *(double *)valuePtr = converted;
            int64_t backHigh;
            uint64_t backLow;
            if (converted < -0x1p127) {
                backHigh = INT64_MIN;
                backLow = 0;
            } else if (converted < 0x1p127) {
                int128FromDouble(converted, &backHigh, &backLow);
            } else {
                backHigh = INT64_MAX;
                backLow = UINT64_MAX;
            }
            return backHigh == high && backLow == low;
        }
        case kNumberSInt128Type: {
            if (!isFloat) {
                if (isWide) {
                    memcpy(valuePtr, data, 16);
                } else {
                    int64_t value = readInt64(data);
                    writeInt128(valuePtr, value >> 63, (uint64_t)value);
                }
                return true;
            }
            int64_t high;
            uint64_t low;
            if (isWide) {
                double value = readDouble(data);
                if (value < -0x1p127) {
                    high = INT64_MIN;
                    low = 0;
                } else if (value >= 0x1p127) {
                    high = INT64_MAX;
                    low = UINT64_MAX;
                } else {
                    int128FromDouble(value, &high, &low);
                }
                writeInt128(valuePtr, high, low);
                return doubleFromInt128(high, low) == value;
            }
            float value = readFloat(data);
            if (value < -0x1p127f) {
                high = INT64_MIN;
                low = 0;
            } else if (value >= 0x1p127f) {
                high = INT64_MAX;
                low = UINT64_MAX;
            } else {
                int128FromDouble((double)value, &high, &low);
            }
            writeInt128(valuePtr, high, low);
            return value == (float)doubleFromInt128(high, low);
        }
        default:
            return false;
    }
}

// Native CFNumberCompare, 0x1c01adc98. External. A tagged or Objective-C first number answers -compare:, a
// tagged or Objective-C second number -_reverseCompare: with the first. Two integers compare as 128-bit
// values (signed high, unsigned low halves). Two floating-point numbers compare their Float64 values: a NaN
// is below a value with a positive sign and above one with a negative sign, two NaNs are equal, the signs
// decide first (so -0.0 is below +0.0), then the values. An integer and a floating-point number: a value
// below -2^127 or at or above 2^127 decides alone; otherwise the 128-bit values compare, and when they are
// equal a NaN is below a nonnegative and above a negative integer, then the signs (+-1.0 against the
// integer's high half), then the Float64 values decide. Results are from the first number's side.
OCFComparisonResult OCFNumberCompare(OCFNumberRef number, OCFNumberRef otherNumber, void *context) {
    (void)context;
    if (isTagged(number) || __CF_IS_OBJC(22, number) != 0) {
        return ((OCFComparisonResult (*)(id, SEL, id))objc_msgSend)(
            (id)number, sel_getUid("compare:"), (id)otherNumber
        );
    }
    if (isTagged(otherNumber) || __CF_IS_OBJC(22, otherNumber) != 0) {
        return ((OCFComparisonResult (*)(id, SEL, id))objc_msgSend)(
            (id)otherNumber, sel_getUid("_reverseCompare:"), (id)number
        );
    }
    OCFNumberType type1 = __CFNumberCanonicalTypes[infoOf(number) & 7];
    OCFNumberType type2 = __CFNumberCanonicalTypes[infoOf(otherNumber) & 7];
    uint16_t entry1 = __CFNumberTypeTable[type1];
    uint16_t entry2 = __CFNumberTypeTable[type2];
    bool isFloat1 = (entry1 & 0x20) != 0;
    bool isFloat2 = (entry2 & 0x20) != 0;
    if (!isFloat1 && !isFloat2) {
        Int128Parts value1 = {0, 0};
        Int128Parts value2 = {0, 0};
        __CFNumberGetValue(number, kNumberSInt128Type, &value1);
        __CFNumberGetValue(otherNumber, kNumberSInt128Type, &value2);
        if (value1.high < value2.high) {
            return kOCFCompareLessThan;
        }
        if (value1.high > value2.high) {
            return kOCFCompareGreaterThan;
        }
        if (value1.low < value2.low) {
            return kOCFCompareLessThan;
        }
        return value1.low > value2.low ? kOCFCompareGreaterThan : kOCFCompareEqualTo;
    }
    if (isFloat1 && isFloat2) {
        double value1 = 0.0;
        double value2 = 0.0;
        __CFNumberGetValue(number, kOCFNumberFloat64Type, &value1);
        __CFNumberGetValue(otherNumber, kOCFNumberFloat64Type, &value2);
        double sign2 = copysign(1.0, value2);
        if (value1 != value1) {
            if (value2 != value2) {
                return kOCFCompareEqualTo;
            }
            return sign2 >= 0.0 ? kOCFCompareLessThan : kOCFCompareGreaterThan;
        }
        double sign1 = copysign(1.0, value1);
        if (value2 != value2) {
            return sign1 < 0.0 ? kOCFCompareLessThan : kOCFCompareGreaterThan;
        }
        if (sign1 < sign2) {
            return kOCFCompareLessThan;
        }
        if (sign2 < sign1) {
            return kOCFCompareGreaterThan;
        }
        if (value1 < value2) {
            return kOCFCompareLessThan;
        }
        return value2 < value1 ? kOCFCompareGreaterThan : kOCFCompareEqualTo;
    }
    OCFNumberRef floatNumber = isFloat1 ? number : otherNumber;
    OCFNumberRef integerNumber = isFloat1 ? otherNumber : number;
    OCFComparisonResult floatBelow = isFloat1 ? kOCFCompareLessThan : kOCFCompareGreaterThan;
    OCFComparisonResult floatAbove = isFloat1 ? kOCFCompareGreaterThan : kOCFCompareLessThan;
    double integerValue = 0.0;
    double floatValue = 0.0;
    __CFNumberGetValue(floatNumber, kOCFNumberFloat64Type, &floatValue);
    if (floatValue < -0x1p127) {
        return floatBelow;
    }
    if (floatValue >= 0x1p127) {
        return floatAbove;
    }
    Int128Parts value1 = {0, 0};
    Int128Parts value2 = {0, 0};
    __CFNumberGetValue(floatNumber, kNumberSInt128Type, &value1);
    __CFNumberGetValue(integerNumber, kNumberSInt128Type, &value2);
    if (value1.high < value2.high) {
        return floatBelow;
    }
    if (value1.high > value2.high) {
        return floatAbove;
    }
    if (value1.low < value2.low) {
        return floatBelow;
    }
    if (value1.low > value2.low) {
        return floatAbove;
    }
    if (floatValue != floatValue) {
        return value2.high >= 0 ? floatBelow : floatAbove;
    }
    double floatSign = copysign(1.0, floatValue);
    double integerSign = value2.high > -1 ? 1.0 : -1.0;
    if (floatSign < integerSign) {
        return floatBelow;
    }
    if (integerSign < floatSign) {
        return floatAbove;
    }
    __CFNumberGetValue(integerNumber, kOCFNumberFloat64Type, &integerValue);
    if (floatValue < integerValue) {
        return floatBelow;
    }
    return integerValue < floatValue ? floatAbove : kOCFCompareEqualTo;
}

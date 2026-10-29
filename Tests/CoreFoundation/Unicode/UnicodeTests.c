//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Unicode/UnicodeTests.h"
#include "CoreFoundation/Unicode/OCFUniChar.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

// Native CFUniCharIsMemberOf 0x1c02512c4, GetBitmapPtrForPlane 0x1c0251524,
// DecomposeCharacter 0x1c0252910, FillDestinationBuffer 0x1c0252754.
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x1806d523c, 0x1806400ec, 0x1806fc7b4, 0x18077483c;
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
int runCoreFoundationUnicodeDifferentialTests(void) {
    const char *nativeNames[] = {
        OF_NAME_STRING(CFUniCharIsMemberOf),
        OF_NAME_STRING(CFUniCharGetBitmapPtrForPlane),
        OF_NAME_STRING(CFUniCharDecomposeCharacter),
        OF_NAME_STRING(CFUniCharFillDestinationBuffer)
    };
    const char *replacementNames[] = {
        OF_NAME_STRING(OCFUniCharIsMemberOf),
        OF_NAME_STRING(OCFUniCharGetBitmapPtrForPlane),
        OF_NAME_STRING(OCFUniCharDecomposeCharacter),
        OF_NAME_STRING(OCFUniCharFillDestinationBuffer)
    };
    void *native[4];
    void *replacement[4];
    for (size_t index = 0; index < 4; ++index) {
        int status = resolveReferenceFunction(
            "CoreFoundation", OF_REFERENCE_CF_UUID, nativeNames[index], &native[index]
        );
        if (status != 0) {
            return status;
        }
        status =
            resolveReplacementFunction("OpenCoreFoundation", replacementNames[index], &replacement[index]);
        if (status != 0) {
            return status;
        }
        if (native[index] == replacement[index]) {
            fputs("Setup failure: Unicode reference resolved to the replacement.\n", stderr);
            return 77;
        }
    }
    typedef uint8_t (*MemberFunction)(uint32_t, uint32_t);
    typedef const uint8_t *(*BitmapFunction)(uint32_t, uint32_t);
    typedef OCFIndex (*DecomposeFunction)(uint32_t, uint32_t *, OCFIndex);
    typedef uint8_t (*FillFunction)(const uint32_t *, OCFIndex, void **, OCFIndex, OCFIndex *, uint32_t);
    MemberFunction nativeMember = (MemberFunction)native[0];
    MemberFunction replacementMember = (MemberFunction)replacement[0];
    BitmapFunction nativeBitmap = (BitmapFunction)native[1];
    BitmapFunction replacementBitmap = (BitmapFunction)replacement[1];
    DecomposeFunction nativeDecompose = (DecomposeFunction)native[2];
    DecomposeFunction replacementDecompose = (DecomposeFunction)replacement[2];
    FillFunction nativeFill = (FillFunction)native[3];
    FillFunction replacementFill = (FillFunction)replacement[3];
    size_t cases = 0;
    const uint32_t sets[] = {1, 2, 3, 4, 5, 8, 9, 12, 15, 100, 101, 108};
    for (uint32_t character = 0; character <= 0x10ffff; ++character) {
        for (size_t set = 0; set < sizeof(sets) / sizeof(sets[0]); ++set) {
            if (nativeMember(character, sets[set]) != replacementMember(character, sets[set])) {
                fprintf(
                    stderr, "Unicode membership mismatch: character 0x%x, set %u.\n", character, sets[set]
                );
                return 1;
            }
            ++cases;
        }
        uint32_t expected[40];
        uint32_t actual[40];
        memset(expected, 0xa5, sizeof(expected));
        memset(actual, 0xa5, sizeof(actual));
        OCFIndex expectedLength = nativeDecompose(character, expected + 2, 32);
        OCFIndex actualLength = replacementDecompose(character, actual + 2, 32);
        if (expectedLength != actualLength || memcmp(expected, actual, sizeof(expected)) != 0) {
            fprintf(stderr, "Unicode character decomposition mismatch: 0x%x.\n", character);
            return 1;
        }
        ++cases;
    }
    uint32_t rawState = UINT32_C(0x9e3779b9);
    for (size_t rawCase = 0; rawCase < 4096; ++rawCase) {
        rawState = rawState * UINT32_C(1664525) + UINT32_C(1013904223);
        for (size_t set = 0; set < sizeof(sets) / sizeof(sets[0]); ++set) {
            if (nativeMember(rawState, sets[set]) != replacementMember(rawState, sets[set])) {
                fprintf(stderr, "Unicode raw-bit membership mismatch: 0x%x, set %u.\n", rawState, sets[set]);
                return 1;
            }
            ++cases;
        }
        uint32_t expected[40];
        uint32_t actual[40];
        memset(expected, 0xa5, sizeof(expected));
        memset(actual, 0xa5, sizeof(actual));
        OCFIndex expectedLength = nativeDecompose(rawState, expected + 2, 32);
        OCFIndex actualLength = replacementDecompose(rawState, actual + 2, 32);
        if (expectedLength != actualLength || memcmp(expected, actual, sizeof(expected)) != 0) {
            fprintf(stderr, "Unicode raw-bit decomposition mismatch: 0x%x.\n", rawState);
            return 1;
        }
        ++cases;
    }
    for (uint32_t character = 0; character <= 0xffff; ++character) {
        for (OCFIndex capacity = 0; capacity <= 5; ++capacity) {
            uint32_t expected[12];
            uint32_t actual[12];
            memset(expected, 0xa5, sizeof(expected));
            memset(actual, 0xa5, sizeof(actual));
            OCFIndex expectedLength = nativeDecompose(character, expected + 2, capacity);
            OCFIndex actualLength = replacementDecompose(character, actual + 2, capacity);
            if (expectedLength != actualLength || memcmp(expected, actual, sizeof(expected)) != 0) {
                fprintf(stderr, "Unicode decomposition capacity mismatch: 0x%x, %ld.\n", character, capacity);
                return 1;
            }
            ++cases;
        }
    }
    for (uint32_t set = 0; set < 150; ++set) {
        for (uint32_t plane = 0; plane < 256; ++plane) {
            const uint8_t *expected = nativeBitmap(set, plane);
            const uint8_t *actual = replacementBitmap(set, plane);
            if ((expected == 0) != (actual == 0) || (expected && memcmp(expected, actual, 8192) != 0)) {
                fprintf(stderr, "Unicode bitmap mismatch: set %u, plane %u.\n", set, plane);
                return 1;
            }
            ++cases;
        }
    }
    const uint32_t scalars[] = {
        0, 0x7f, 0x80, 0x7ff, 0x800, 0xd800, 0xdc00, 0xffff, 0x10000, 0x10ffff, 0x1fffff, 0x200000, 0xffffffff
    };
    const uint32_t formats[] = {0, 1, 2, 3, 4, UINT32_MAX};
    const OCFIndex usedValues[] = {-1, 0, 1, LONG_MAX};
    for (size_t scalar = 0; scalar < sizeof(scalars) / sizeof(scalars[0]); ++scalar) {
        for (size_t format = 0; format < sizeof(formats) / sizeof(formats[0]); ++format) {
            for (OCFIndex capacity = -1; capacity < 9; ++capacity) {
                for (size_t used = 0; used < sizeof(usedValues) / sizeof(usedValues[0]); ++used) {
                    for (size_t alignment = 0; alignment < 4; ++alignment) {
                        uint8_t expected[64];
                        uint8_t actual[64];
                        memset(expected, 0xa5, sizeof(expected));
                        memset(actual, 0xa5, sizeof(actual));
                        void *expectedDestination = expected + 8 + alignment;
                        void *actualDestination = actual + 8 + alignment;
                        OCFIndex expectedUsed = usedValues[used];
                        OCFIndex actualUsed = usedValues[used];
                        uint8_t expectedResult = nativeFill(
                            scalars + scalar,
                            1,
                            &expectedDestination,
                            capacity,
                            &expectedUsed,
                            formats[format]
                        );
                        uint8_t actualResult = replacementFill(
                            scalars + scalar, 1, &actualDestination, capacity, &actualUsed, formats[format]
                        );
                        if (expectedResult != actualResult || expectedUsed != actualUsed ||
                            (uint8_t *)expectedDestination - expected !=
                                (uint8_t *)actualDestination - actual ||
                            memcmp(expected, actual, sizeof(expected)) != 0) {
                            fprintf(
                                stderr,
                                "Unicode destination mismatch: scalar 0x%x, format %u, capacity %ld.\n",
                                scalars[scalar],
                                formats[format],
                                capacity
                            );
                            return 1;
                        }
                        ++cases;
                    }
                }
            }
        }
    }
    printf("Unicode differential cases passed: %zu (four raw APIs; finite coverage).\n", cases);
    return 0;
}

typedef uint32_t (*PlaneCountFunction)(uint32_t);

typedef const uint8_t *(*PropertyFunction)(uint32_t, uint32_t);

typedef void (*SortFunction)(uint32_t *, OCFIndex);

typedef uint8_t (*BitmapCopyFunction)(uint32_t, uint32_t, void *, bool);

// Native CFUniCharGetNumberOfPlanes 0x1c0251870, CFUniCharGetBitmapForPlane 0x1c02515a0 (hidden; resolved
// by IDA offset), CFUniCharGetUnicodePropertyDataForPlane 0x1c025268c, CFUniCharPrioritySort 0x1c0253320.
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x180715de4, 0x180715aa0, 0x18065673c, 0x1806fcc28;
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// Reading: Documentation/Reference/Research/CoreFoundationUnicode.md.
int runCoreFoundationUnicodePlaneDifferentialTests(void) {
    void *native[4];
    const uintptr_t hiddenAddresses[] = {UINT64_C(0x1c0251870), UINT64_C(0x1c02515a0)};
    for (size_t index = 0; index < 2; ++index) {
        int status = resolveReferenceAddress(
            "CoreFoundation",
            OF_REFERENCE_CF_UUID,
            hiddenAddresses[index],
            UINT64_C(0x1c014a000),
            &native[index]
        );
        if (status != 0) {
            return status;
        }
    }
    int status = resolveReferenceFunction(
        "CoreFoundation",
        OF_REFERENCE_CF_UUID,
        OF_NAME_STRING(CFUniCharGetUnicodePropertyDataForPlane),
        &native[2]
    );
    if (status == 0) {
        status = resolveReferenceFunction(
            "CoreFoundation", OF_REFERENCE_CF_UUID, OF_NAME_STRING(CFUniCharPrioritySort), &native[3]
        );
    }
    if (status != 0) {
        return status;
    }
    void *replacement[4] = {(void *)CFUniCharGetNumberOfPlanes, (void *)CFUniCharGetBitmapForPlane, 0, 0};
    status = checkInternalImplementation(replacement[0]);
    if (status == 0) {
        status = checkInternalImplementation(replacement[1]);
    }
    if (status == 0) {
        status = resolveReplacementFunction(
            "OpenCoreFoundation", OF_NAME_STRING(OCFUniCharGetUnicodePropertyDataForPlane), &replacement[2]
        );
    }
    if (status == 0) {
        status = resolveReplacementFunction(
            "OpenCoreFoundation", OF_NAME_STRING(OCFUniCharPrioritySort), &replacement[3]
        );
    }
    if (status != 0) {
        return status;
    }
    for (size_t index = 0; index < 4; ++index) {
        if (native[index] == replacement[index]) {
            fputs("Setup failure: Unicode plane reference resolved to the replacement.\n", stderr);
            return 77;
        }
    }
    PlaneCountFunction nativeCount = (PlaneCountFunction)native[0];
    PlaneCountFunction replacementCount = (PlaneCountFunction)replacement[0];
    BitmapCopyFunction nativeCopy = (BitmapCopyFunction)native[1];
    BitmapCopyFunction replacementCopy = (BitmapCopyFunction)replacement[1];
    PropertyFunction nativeProperty = (PropertyFunction)native[2];
    PropertyFunction replacementProperty = (PropertyFunction)replacement[2];
    SortFunction nativeSort = (SortFunction)native[3];
    SortFunction replacementSort = (SortFunction)replacement[3];
    size_t cases = 0;
    // Plane counts: only identifiers that map to the 22 read records; others read past the descriptor.
    for (uint32_t set = 0; set <= 110; ++set) {
        if (set > 25 && set < 100) {
            continue;
        }
        if (nativeCount(set) != replacementCount(set)) {
            fprintf(stderr, "Unicode plane count mismatch: set %u.\n", set);
            return 1;
        }
        ++cases;
    }
    // Restored plane payload sizes: combining priority 8704/9472, bidi 14080/15360/768.
    for (uint32_t type = 0; type < 2; ++type) {
        for (uint32_t plane = 0; plane < 300; ++plane) {
            const uint8_t *expected = nativeProperty(type, plane);
            const uint8_t *actual = replacementProperty(type, plane);
            size_t size = type == 0 ? (plane == 0 ? 8704 : 9472)
                                    : (plane == 0   ? 14080
                                       : plane == 1 ? 15360
                                                    : 768);
            if ((expected == 0) != (actual == 0) || (expected && memcmp(expected, actual, size) != 0)) {
                fprintf(stderr, "Unicode property plane mismatch: type %u, plane %u.\n", type, plane);
                return 1;
            }
            ++cases;
        }
    }
    static uint8_t expectedBitmap[8200];
    static uint8_t actualBitmap[8200];
    for (uint32_t set = 0; set < 140; ++set) {
        for (uint32_t plane = 0; plane < 20; ++plane) {
            for (int inverted = 0; inverted < 2; ++inverted) {
                memset(expectedBitmap, 0x5a, sizeof(expectedBitmap));
                memset(actualBitmap, 0x5a, sizeof(actualBitmap));
                uint8_t expected = nativeCopy(set, plane, expectedBitmap + 4, inverted);
                uint8_t actual = replacementCopy(set, plane, actualBitmap + 4, inverted);
                if (expected != actual || memcmp(expectedBitmap, actualBitmap, sizeof(expectedBitmap)) != 0) {
                    fprintf(
                        stderr,
                        "Unicode bitmap copy mismatch: set %u, plane %u, inverted %d.\n",
                        set,
                        plane,
                        inverted
                    );
                    return 1;
                }
                ++cases;
            }
        }
    }
    const uint32_t alphabet[] = {0x41,    0x61,    0x300,   0x301,   0x316,   0x327,    0x334,   0x345,
                                 0x5b0,   0x5c4,   0x93c,   0x94d,   0x302d,  0x3099,   0x1d165, 0x1d167,
                                 0x1d16e, 0x1e944, 0x1f600, 0x20000, 0x2fa1d, 0x10ffff, 0xfeff,  0xffffffff};
    const size_t alphabetCount = sizeof(alphabet) / sizeof(alphabet[0]);
    uint32_t state = UINT32_C(0x2545f491);
    for (size_t sortCase = 0; sortCase < 200000; ++sortCase) {
        uint32_t expected[20];
        uint32_t actual[20];
        state = state * UINT32_C(1664525) + UINT32_C(1013904223);
        OCFIndex length = (OCFIndex)(state >> 28) - 1;
        for (size_t index = 0; index < 20; ++index) {
            state = state * UINT32_C(1664525) + UINT32_C(1013904223);
            expected[index] =
                (state & 0x100) ? alphabet[(state >> 16) % alphabetCount] : (state >> 12) & 0x3ffff;
        }
        memcpy(actual, expected, sizeof(expected));
        nativeSort(expected + 2, length);
        replacementSort(actual + 2, length);
        if (memcmp(expected, actual, sizeof(expected)) != 0) {
            fprintf(stderr, "Unicode priority sort mismatch: case %zu, length %ld.\n", sortCase, length);
            return 1;
        }
        ++cases;
    }
    printf(
        "Unicode plane differential cases passed: %zu (four plane/property/sort APIs; finite coverage).\n",
        cases
    );
    return 0;
}

typedef OCFIndex (*MapCaseFunction)(uint32_t, uint16_t *, OCFIndex, uint32_t, uint32_t, const uint8_t *);

typedef uint32_t (*ConditionalFlagsFunction)(
    uint32_t, const uint16_t *, OCFIndex, OCFIndex, uint32_t, const uint8_t *, uint32_t
);

typedef OCFIndex (*CompatibilityFunction)(uint32_t *, OCFIndex, OCFIndex);

static MapCaseFunction nativeMapCase;

static MapCaseFunction replacementMapCase;

static ConditionalFlagsFunction nativeConditionalFlags;

static ConditionalFlagsFunction replacementConditionalFlags;

static CompatibilityFunction nativeCompatibility;

static CompatibilityFunction replacementCompatibility;

// Two-byte language codes read by one 16-bit load; "en" reaches only the language-independent branches.
static const uint8_t *const kLanguages[7] = {
    0,
    (const uint8_t *)"en",
    (const uint8_t *)"tr",
    (const uint8_t *)"az",
    (const uint8_t *)"lt",
    (const uint8_t *)"nl",
    (const uint8_t *)"el",
};

static int compareMapCase(
    uint32_t character, OCFIndex maxLength, uint32_t context, uint32_t flags, const uint8_t *language
) {
    uint16_t expected[48];
    uint16_t actual[48];
    for (size_t index = 0; index < 48; ++index) {
        expected[index] = 0xa5c3;
        actual[index] = 0xa5c3;
    }
    OCFIndex expectedLength = nativeMapCase(character, expected + 4, maxLength, context, flags, language);
    OCFIndex actualLength = replacementMapCase(character, actual + 4, maxLength, context, flags, language);
    if (expectedLength != actualLength || memcmp(expected, actual, sizeof(expected)) != 0) {
        fprintf(
            stderr,
            "Unicode case mapping mismatch: U+%04X, maxLength %ld, context %u, flags 0x%x, language %s: "
            "%ld/%ld.\n",
            character,
            (long)maxLength,
            context,
            flags,
            language ? (const char *)language : "null",
            (long)expectedLength,
            (long)actualLength
        );
        return 1;
    }
    return 0;
}

static int compareConditionalFlags(
    uint32_t character,
    const uint16_t *buffer,
    OCFIndex currentIndex,
    OCFIndex length,
    uint32_t type,
    const uint8_t *language,
    uint32_t lastFlags
) {
    uint32_t expected =
        nativeConditionalFlags(character, buffer, currentIndex, length, type, language, lastFlags);
    uint32_t actual =
        replacementConditionalFlags(character, buffer, currentIndex, length, type, language, lastFlags);
    if (expected != actual) {
        fprintf(
            stderr,
            "Unicode conditional case flags mismatch: U+%04X, index %ld, length %ld, type %u, language %s, "
            "last 0x%x: 0x%x/0x%x.\n",
            character,
            (long)currentIndex,
            (long)length,
            type,
            language ? (const char *)language : "null",
            lastFlags,
            expected,
            actual
        );
        return 1;
    }
    return 0;
}

static int compareCompatibility(const uint32_t *input, OCFIndex length, OCFIndex maxLength) {
    uint32_t expected[72];
    uint32_t actual[72];
    for (size_t index = 0; index < 72; ++index) {
        expected[index] = UINT32_C(0xa5c3e1f7);
        actual[index] = UINT32_C(0xa5c3e1f7);
    }
    if (length > 0) {
        memcpy(expected + 4, input, (size_t)length * sizeof(uint32_t));
        memcpy(actual + 4, input, (size_t)length * sizeof(uint32_t));
    }
    OCFIndex expectedLength = nativeCompatibility(expected + 4, length, maxLength);
    OCFIndex actualLength = replacementCompatibility(actual + 4, length, maxLength);
    if (expectedLength != actualLength || memcmp(expected, actual, sizeof(expected)) != 0) {
        fprintf(
            stderr,
            "Unicode compatibility decomposition mismatch: first U+%04X, length %ld, maxLength %ld: "
            "%ld/%ld.\n",
            length > 0 ? input[0] : 0,
            (long)length,
            (long)maxLength,
            (long)expectedLength,
            (long)actualLength
        );
        return 1;
    }
    return 0;
}

static uint32_t nextRandom(uint32_t *state) {
    *state = *state * UINT32_C(1664525) + UINT32_C(1013904223);
    return *state;
}

// Native CFUniCharMapCaseTo 0x1c02518e0, CFUniCharGetConditionalCaseMappingFlags 0x1c0252064 (hidden;
// resolved by IDA offset), CFUniCharCompatibilityDecompose 0x1c0253068 and its non-external helper
// __CFUniCharRecursivelyCompatibilityDecomposeCharacter 0x1c0253188 (reached through it).
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446: 0x1806cb400 (with cold bodies 0x1807c7c18, 0x1807c7cdc), 0x180774138, 0x180700fac,
// 0x180700d14; UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified. The conditional-flags corpus
// keeps a non-surrogate unit after each logical buffer end, so the native out-of-range read the restoration
// fixes sees the same answer the restoration assumes. Reading:
// Documentation/Reference/Research/CoreFoundationUnicode.md.
int runCoreFoundationUnicodeCaseDifferentialTests(void) {
    void *native[3];
    void *replacement[3];
    int status = resolveReferenceFunction(
        "CoreFoundation", OF_REFERENCE_CF_UUID, OF_NAME_STRING(CFUniCharMapCaseTo), &native[0]
    );
    if (status == 0) {
        status = resolveReferenceAddress(
            "CoreFoundation", OF_REFERENCE_CF_UUID, UINT64_C(0x1c0252064), UINT64_C(0x1c014a000), &native[1]
        );
    }
    if (status == 0) {
        status = resolveReferenceFunction(
            "CoreFoundation",
            OF_REFERENCE_CF_UUID,
            OF_NAME_STRING(CFUniCharCompatibilityDecompose),
            &native[2]
        );
    }
    if (status == 0) {
        status = resolveReplacementFunction(
            "OpenCoreFoundation", OF_NAME_STRING(OCFUniCharMapCaseTo), &replacement[0]
        );
    }
    if (status == 0) {
        replacement[1] = (void *)CFUniCharGetConditionalCaseMappingFlags;
        status = checkInternalImplementation(replacement[1]);
    }
    if (status == 0) {
        status = resolveReplacementFunction(
            "OpenCoreFoundation", OF_NAME_STRING(OCFUniCharCompatibilityDecompose), &replacement[2]
        );
    }
    if (status != 0) {
        return status;
    }
    for (size_t index = 0; index < 3; ++index) {
        if (native[index] == replacement[index]) {
            fputs("Setup failure: Unicode case reference resolved to the replacement.\n", stderr);
            return 77;
        }
    }
    nativeMapCase = (MapCaseFunction)native[0];
    replacementMapCase = (MapCaseFunction)replacement[0];
    nativeConditionalFlags = (ConditionalFlagsFunction)native[1];
    replacementConditionalFlags = (ConditionalFlagsFunction)replacement[1];
    nativeCompatibility = (CompatibilityFunction)native[2];
    replacementCompatibility = (CompatibilityFunction)replacement[2];
    size_t mapCases = 0;
    size_t flagCases = 0;
    size_t compatibilityCases = 0;

    // Case mapping: every scalar in the four read contexts, without and with the accent-decomposition flag.
    for (uint32_t character = 0; character <= 0x10ffff; ++character) {
        for (uint32_t context = 0; context < 4; ++context) {
            if (compareMapCase(character, 8, context, 0, 0) != 0 ||
                compareMapCase(character, 8, context, 0x10, kLanguages[1]) != 0) {
                return 1;
            }
            mapCases += 2;
        }
    }
    // Capacities around every extra-table count, through the last case-table key (U+1E943).
    const OCFIndex capacities[] = {-1, 0, 1, 2, 3, 4};
    for (uint32_t character = 0; character < 0x20000; ++character) {
        for (uint32_t context = 0; context < 4; ++context) {
            for (size_t index = 0; index < sizeof(capacities) / sizeof(capacities[0]); ++index) {
                if (compareMapCase(character, capacities[index], context, 0, 0) != 0) {
                    return 1;
                }
                ++mapCases;
            }
        }
    }
    // Language/flag matrix over the scalars the language branches name and their neighborhoods.
    uint32_t characters[2048];
    size_t characterCount = 0;
    const uint32_t ranges[][2] = {
        {0x0, 0x24f},
        {0x300, 0x3ff},
        {0x1e00, 0x1fff},
        {0xfb00, 0xfb1f},
        {0x10400, 0x1041f},
        {0x1e900, 0x1e94f}
    };
    for (size_t range = 0; range < sizeof(ranges) / sizeof(ranges[0]); ++range) {
        for (uint32_t character = ranges[range][0]; character <= ranges[range][1]; ++character) {
            characters[characterCount++] = character;
        }
    }
    uint32_t state = UINT32_C(0x6b8b4567);
    while (characterCount < 2048) {
        characters[characterCount++] = nextRandom(&state) % 0x110000;
    }
    const OCFIndex matrixCapacities[] = {1, 3, 8};
    for (size_t index = 0; index < characterCount; ++index) {
        for (uint32_t context = 0; context < 4; ++context) {
            for (uint32_t flags = 0; flags < 32; ++flags) {
                for (size_t language = 0; language < 7; ++language) {
                    for (size_t capacity = 0; capacity < 3; ++capacity) {
                        if (compareMapCase(
                                characters[index],
                                matrixCapacities[capacity],
                                context,
                                flags,
                                kLanguages[language]
                            ) != 0) {
                            return 1;
                        }
                        ++mapCases;
                    }
                }
            }
        }
    }

    // Conditional flags: every scalar in a fixed context, for the language-independent and Lithuanian paths.
    static const uint16_t context[12] = {
        0x61, 0x301, 0x3a3, 0x69, 0x307, 0x301, 0x4a, 0xd834, 0xdd85, 0x41, 0x27, 0x41
    };
    for (uint32_t character = 0; character <= 0x10ffff; ++character) {
        for (uint32_t type = 0; type < 4; ++type) {
            if (compareConditionalFlags(character, context, 3, 11, type, kLanguages[4], 0) != 0 ||
                compareConditionalFlags(character, context, 3, 11, type, kLanguages[4], 0x16) != 0) {
                return 1;
            }
            flagCases += 2;
        }
    }
    // Random buffers over cased, case-ignorable, combining (priorities 230, 220, 216, 1), surrogate and
    // lone-surrogate units, at every index and with every language, type and low flag combination.
    static const uint16_t alphabet[][2] = {
        {0x61, 0},   {0x41, 0},        {0x49, 0},        {0x69, 0},        {0x4a, 0},        {0x6a, 0},
        {0x3a3, 0},  {0x3c3, 0},       {0x391, 0},       {0x27, 0},        {0xad, 0},        {0x2e, 0},
        {0x20, 0},   {0x31, 0},        {0x300, 0},       {0x301, 0},       {0x307, 0},       {0x316, 0},
        {0x334, 0},  {0x345, 0},       {0x5b0, 0},       {0x130, 0},       {0xcc, 0},        {0xcd, 0},
        {0x128, 0},  {0x12e, 0},       {0x1f08, 0},      {0xfb01, 0},      {0xe9, 0},        {0x1e2d, 0},
        {0x2019, 0}, {0xd834, 0xdd85}, {0xd834, 0xdd65}, {0xd801, 0xdc00}, {0xd83a, 0xdd00}, {0xd800, 0},
        {0xdc00, 0}, {0xd83a, 0xdd44},
    };
    const size_t alphabetCount = sizeof(alphabet) / sizeof(alphabet[0]);
    static const uint32_t subjects[] = {
        0x3a3,
        0x49,
        0x69,
        0x4a,
        0x6a,
        0x307,
        0x12e,
        0x130,
        0x131,
        0xcc,
        0x391,
        0x3b1,
        0x1f08,
        0x41,
        0x301,
        0x345,
        0x1e943,
        0x10400
    };
    const size_t subjectCount = sizeof(subjects) / sizeof(subjects[0]);
    for (size_t sample = 0; sample < 120000; ++sample) {
        uint16_t buffer[32];
        OCFIndex length = 0;
        OCFIndex target = (OCFIndex)(nextRandom(&state) % 13);
        while (length < target) {
            const uint16_t *unit = alphabet[(nextRandom(&state) >> 8) % alphabetCount];
            buffer[length++] = unit[0];
            if (unit[1] != 0 && length < 28) {
                buffer[length++] = unit[1];
            }
        }
        for (OCFIndex index = length; index < 32; ++index) {
            buffer[index] = 0x41;
        }
        for (OCFIndex index = 0; index <= length; ++index) {
            uint32_t character = index < length && (nextRandom(&state) & 1) != 0
                                     ? buffer[index]
                                     : subjects[(nextRandom(&state) >> 8) % subjectCount];
            for (size_t language = 0; language < 7; ++language) {
                uint32_t type = nextRandom(&state) >> 30;
                uint32_t lastFlags = (nextRandom(&state) >> 12) & 0x1e;
                if (compareConditionalFlags(
                        character, buffer, index, length, type, kLanguages[language], lastFlags
                    ) != 0) {
                    return 1;
                }
                ++flagCases;
            }
        }
    }

    // Compatibility decomposition: every scalar alone, every member at small capacities, and mixed arrays.
    for (uint32_t character = 0; character <= 0x10ffff; ++character) {
        if (compareCompatibility(&character, 1, 32) != 0) {
            return 1;
        }
        ++compatibilityCases;
        if (OCFUniCharIsMemberOf(character, 100) != 0) {
            for (OCFIndex capacity = -1; capacity <= 20; ++capacity) {
                if (compareCompatibility(&character, 1, capacity) != 0) {
                    return 1;
                }
                ++compatibilityCases;
            }
        }
    }
    static const uint32_t pieces[] = {0x41,    0x61,    0x301,  0xa0,    0xa8,   0xbd,    0xfb01,  0xfdfa,
                                      0x3392,  0x1e9b,  0x1fc1, 0x2474,  0xff21, 0x1d400, 0x1f100, 0xc0,
                                      0x1d15e, 0x2fa1d, 0xffe3, 0x1079c, 0x33ff, 0x3250,  0x2003,  0xfb05};
    const size_t pieceCount = sizeof(pieces) / sizeof(pieces[0]);
    for (size_t sample = 0; sample < 200000; ++sample) {
        uint32_t input[8];
        OCFIndex length = (OCFIndex)(nextRandom(&state) % 8) - 1;
        for (size_t index = 0; index < 8; ++index) {
            input[index] = pieces[(nextRandom(&state) >> 8) % pieceCount];
        }
        OCFIndex capacity = (OCFIndex)((nextRandom(&state) >> 8) % 48) - 1;
        if (compareCompatibility(input, length, capacity) != 0) {
            return 1;
        }
        ++compatibilityCases;
    }
    printf(
        "Unicode case differential cases passed: %zu (%zu case mapping, %zu conditional flags, %zu "
        "compatibility; "
        "finite coverage).\n",
        mapCases + flagCases + compatibilityCases,
        mapCases,
        flagCases,
        compatibilityCases
    );
    return 0;
}

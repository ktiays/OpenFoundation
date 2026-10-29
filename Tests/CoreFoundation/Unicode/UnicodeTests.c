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

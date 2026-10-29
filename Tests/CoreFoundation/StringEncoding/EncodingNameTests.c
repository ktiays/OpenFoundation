//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/StringEncoding/EncodingNameTests.h"
#include "CoreFoundation/StringEncoding/OCFPlatformConverter.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodingConverter.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodingDatabase.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodings.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

typedef unsigned long (*ToNSStringEncoding)(uint32_t encoding);

typedef uint32_t (*WindowsCodePage)(uint32_t encoding);

typedef bool (*CanonicalName)(uint32_t encoding, char *buffer, OCFIndex bufferSize);

typedef const char *(*ICUName)(uint32_t encoding);

typedef OCFIndex (*FallbackProc)(const uint8_t *, OCFIndex, uint16_t *, OCFIndex, OCFIndex *);

typedef uint32_t (*PlatformConversion)(
    uint32_t, uint32_t, const void *, OCFIndex, OCFIndex *, void *, OCFIndex, OCFIndex *
);

typedef OCFIndex (*PlatformLength)(uint32_t, uint32_t, const uint8_t *, OCFIndex);

typedef OCFIndex (*ToBytesWrapper)(
    const StringEncodingConverterRecord *,
    uint32_t,
    const uint16_t *,
    OCFIndex,
    uint8_t *,
    OCFIndex,
    OCFIndex *
);

typedef OCFIndex (*ToUnicodeWrapper)(
    const StringEncodingConverterRecord *,
    uint32_t,
    const uint8_t *,
    OCFIndex,
    uint16_t *,
    OCFIndex,
    OCFIndex *
);

enum {
    kBufferLength = 192,
    kBufferOffset = 4,
    kInputLength = 40,
};

// UTF-16 units the deterministic test converters produce: ASCII, Latin and Greek canonical
// decompositions (one to four values), Hangul, CJK compatibility ideographs that decompose to
// supplementary values (U+FA6C -> U+242EE, U+FACF -> U+2284A, U+FAD0 -> U+22844), surrogates and
// noncharacters.
static const uint16_t kAlphabet[] = {
    0x0041, 0x007f, 0x0080, 0x00c0, 0x00c5, 0x00e9, 0x0344, 0x0390, 0x1e69, 0x1e9b,
    0x1f82, 0x1fc1, 0x1fd3, 0x2126, 0x212b, 0x2adc, 0x30ac, 0xac00, 0xd55c, 0xf900,
    0xfa6c, 0xfacf, 0xfad0, 0xfb1d, 0xd800, 0xdc00, 0xfffd, 0xffff, 0x0301, 0x00a0,
};

static const size_t kAlphabetCount = sizeof(kAlphabet) / sizeof(kAlphabet[0]);

// Deterministic converter callbacks shared by the native and replacement wrappers. Several return a
// nonzero 32-bit value whose low byte or halfword is zero, so a narrower test than the wrappers' CBZ W0
// would be detected, and some succeed without writing, so the wrappers' scratch reuse is observable.
static uint32_t testCheapEightBitToBytes(uint32_t flags, uint16_t character, uint8_t *byte) {
    if (character == 0xffff) {
        return 0;
    }
    if (character % 5 == 0) {
        return 1;
    }
    *byte = (uint8_t)(character * 7 + flags);
    return (character & 1) ? 1 : 0x10000;
}

static uint32_t
testStandardEightBitToBytes(uint32_t flags, const uint16_t *characters, OCFIndex numChars, uint8_t *byte) {
    if (characters[0] == 0xfffe) {
        return 0;
    }
    uint32_t count = 1 + characters[0] % 3;
    if ((OCFIndex)count > numChars) {
        count = (uint32_t)numChars;
    }
    *byte = (uint8_t)(characters[0] ^ (count << 4) ^ flags);
    return count;
}

static uint32_t testCheapMultiByteToBytes(uint32_t flags, uint16_t character, uint8_t *bytes) {
    if (character == 0xfffd) {
        return 0;
    }
    uint32_t count = 1 + character % 4;
    for (uint32_t index = 0; index < count; ++index) {
        bytes[index] = (uint8_t)((character >> index) + index + flags);
    }
    return count;
}

static uint32_t testCheapEightBitToUnicode(uint32_t flags, uint8_t byte, uint16_t *character) {
    if (byte == 0xff) {
        return 0;
    }
    if ((byte & 0x0f) == 0x0e) {
        return 1;
    }
    *character = (flags & 2) ? byte : kAlphabet[byte % kAlphabetCount];
    return (byte & 0x30) ? 1 : 0x100;
}

static uint32_t testStandardEightBitToUnicode(uint32_t flags, uint8_t byte, uint16_t *characters) {
    if (byte == 0xfe) {
        return 0;
    }
    uint32_t count = 1 + byte % 4;
    if (byte % 11 == 0) {
        count = 6;
    }
    for (uint32_t index = 0; index < count; ++index) {
        characters[index] = kAlphabet[(byte * 7 + index * 5 + (flags & 1)) % kAlphabetCount];
    }
    return count;
}

static uint32_t
testCheapMultiByteToUnicode(uint32_t flags, const uint8_t *bytes, OCFIndex numBytes, uint16_t *character) {
    if (bytes[0] == 0xff) {
        return 0;
    }
    uint32_t count = 1 + bytes[0] % 3;
    if ((OCFIndex)count > numBytes) {
        count = (uint32_t)numBytes;
    }
    if ((bytes[0] & 0x0f) != 0x0e) {
        *character = kAlphabet[(bytes[0] + (numBytes > 1 ? bytes[1] : 0) + flags) % kAlphabetCount];
    }
    return count;
}

static uint64_t nextBits(uint64_t *state) {
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return *state;
}

static int resolveHidden(uintptr_t address, void **function) {
    return resolveReferenceAddress(
        "CoreFoundation", OF_REFERENCE_CF_UUID, address, UINT64_C(0x1c014a000), function
    );
}

static int resolvePair(uintptr_t address, void *replacement, void **native) {
    int status = resolveHidden(address, native);
    if (status == 0) {
        status = checkInternalImplementation(replacement);
    }
    if (status == 0 && *native == replacement) {
        fputs("Setup failure: an encoding-name oracle resolved to its replacement.\n", stderr);
        status = 77;
    }
    return status;
}

// Encodings the group distinguishes: every value below 0x20000 (all twelve-bit fields with the low
// upper bits), every twelve-bit field under upper-bit patterns, and every Unicode rotation index.
static size_t encodingCases(uint32_t *values, size_t capacity) {
    static const uint32_t kUpper[] = {
        0x00100000,
        0x04000000,
        0x08000000,
        0x0c000000,
        0x10000000,
        0x14000000,
        0x18000000,
        0x1c000000,
        0x7ffff000,
        0x80000000,
        0xfffff000,
        0x12345000,
        0xa5a5a000,
    };
    size_t count = 0;
    for (uint32_t value = 0; value < 0x20000 && count < capacity; ++value) {
        values[count++] = value;
    }
    for (size_t upper = 0; upper < sizeof(kUpper) / sizeof(kUpper[0]); ++upper) {
        for (uint32_t low = 0; low < 0x1000 && count < capacity; ++low) {
            values[count++] = kUpper[upper] | low;
        }
    }
    for (uint32_t rotation = 0; rotation < 64 && count + 4 <= capacity; ++rotation) {
        values[count++] = (rotation << 26) | 0x100;
        values[count++] = (rotation << 26) | 0x1ff;
        values[count++] = (rotation << 26) | 0x101;
        values[count++] = ((rotation << 26) | 0x100) + 0x100000;
    }
    return count;
}

static int compareScalarFunctions(const uint32_t *encodings, size_t encodingCount, size_t *cases) {
    void *native = NULL;
    void *replacement = NULL;
    int status = resolveReferenceFunction(
        "CoreFoundation",
        OF_REFERENCE_CF_UUID,
        OF_NAME_STRING(CFStringConvertEncodingToNSStringEncoding),
        &native
    );
    if (status == 0) {
        status = resolveReplacementFunction(
            "OpenCoreFoundation", OF_NAME_STRING(OCFStringConvertEncodingToNSStringEncoding), &replacement
        );
    }
    if (status != 0) {
        return status;
    }
    if (native == replacement) {
        fputs("Setup failure: an exported encoding oracle resolved to its replacement.\n", stderr);
        return 77;
    }
    ToNSStringEncoding nativeConvert = (ToNSStringEncoding)native;
    ToNSStringEncoding replacementConvert = (ToNSStringEncoding)replacement;
    for (uint32_t value = 0; value < 0x100000; ++value) {
        if (nativeConvert(value) != replacementConvert(value)) {
            fprintf(stderr, "NSStringEncoding mismatch: %08" PRIx32 ".\n", value);
            return 1;
        }
        ++*cases;
    }
    for (size_t index = 0; index < encodingCount; ++index) {
        if (nativeConvert(encodings[index]) != replacementConvert(encodings[index])) {
            fprintf(stderr, "NSStringEncoding mismatch: %08" PRIx32 ".\n", encodings[index]);
            return 1;
        }
        ++*cases;
    }
    return 0;
}

static int compareDatabase(const uint32_t *encodings, size_t encodingCount, size_t *cases) {
    void *native[3];
    void *replacement[3] = {
        (void *)__CFStringEncodingGetWindowsCodePage,
        (void *)__CFStringEncodingGetCanonicalName,
        (void *)__CFStringEncodingGetICUName,
    };
    const uintptr_t addresses[] = {UINT64_C(0x1c02ae7c4), UINT64_C(0x1c02aea2c), UINT64_C(0x1c02ad994)};
    for (size_t index = 0; index < 3; ++index) {
        int status = resolvePair(addresses[index], replacement[index], &native[index]);
        if (status != 0) {
            return status;
        }
    }
    WindowsCodePage nativeCodePage = (WindowsCodePage)native[0];
    CanonicalName nativeName = (CanonicalName)native[1];
    ICUName nativeICU = (ICUName)native[2];
    const OCFIndex sizes[] = {0, 1, 4, 7, 10, 12, 16, 60, 100};
    for (size_t index = 0; index < encodingCount; ++index) {
        uint32_t encoding = encodings[index];
        if (nativeCodePage(encoding) != __CFStringEncodingGetWindowsCodePage(encoding)) {
            fprintf(stderr, "Windows code page mismatch: %08" PRIx32 ".\n", encoding);
            return 1;
        }
        ++*cases;
        for (size_t size = 0; size < sizeof(sizes) / sizeof(sizes[0]); ++size) {
            char expected[120];
            char actual[120];
            memset(expected, 0x5a, sizeof(expected));
            memset(actual, 0x5a, sizeof(actual));
            bool expectedResult = nativeName(encoding, expected + 8, sizes[size]);
            bool actualResult = __CFStringEncodingGetCanonicalName(encoding, actual + 8, sizes[size]);
            if (expectedResult != actualResult || memcmp(expected, actual, sizeof(expected)) != 0) {
                fprintf(stderr, "Canonical name mismatch: %08" PRIx32 ", size %ld.\n", encoding, sizes[size]);
                return 1;
            }
            ++*cases;
        }
        const char *expectedICU = nativeICU(encoding);
        const char *actualICU = __CFStringEncodingGetICUName(encoding);
        if ((expectedICU == NULL) != (actualICU == NULL) ||
            (expectedICU != NULL && strcmp(expectedICU, actualICU) != 0) ||
            (expectedICU != NULL && encoding != 0xa10 && expectedICU != actualICU)) {
            fprintf(stderr, "ICU name mismatch: %08" PRIx32 ".\n", encoding);
            return 1;
        }
        ++*cases;
    }
    return 0;
}

static int compareFallbackAndPlatform(size_t *cases) {
    void *native[4];
    void *replacement[4] = {
        (void *)__CFDefaultToUnicodeFallbackProc,
        (void *)__CFStringEncodingPlatformUnicodeToBytes,
        (void *)__CFStringEncodingPlatformBytesToUnicode,
        (void *)__CFStringEncodingPlatformCharLengthForBytes,
    };
    const uintptr_t addresses[] = {
        UINT64_C(0x1c022c174), UINT64_C(0x1c01cecc4), UINT64_C(0x1c01ceccc), UINT64_C(0x1c01cecd4)
    };
    for (size_t index = 0; index < 4; ++index) {
        int status = resolvePair(addresses[index], replacement[index], &native[index]);
        if (status != 0) {
            return status;
        }
    }
    FallbackProc nativeFallback = (FallbackProc)native[0];
    const OCFIndex capacities[] = {0, 1, 2, 5, -1, INT64_MIN};
    const uint8_t bytes[4] = {0x80, 0x81, 0x82, 0x83};
    for (size_t capacity = 0; capacity < sizeof(capacities) / sizeof(capacities[0]); ++capacity) {
        for (OCFIndex length = -1; length < 4; ++length) {
            uint16_t expected[6] = {0xa55a, 0xa55a, 0xa55a, 0xa55a, 0xa55a, 0xa55a};
            uint16_t actual[6] = {0xa55a, 0xa55a, 0xa55a, 0xa55a, 0xa55a, 0xa55a};
            OCFIndex expectedUsed = -77;
            OCFIndex actualUsed = -77;
            OCFIndex expectedResult =
                nativeFallback(bytes, length, expected + 2, capacities[capacity], &expectedUsed);
            OCFIndex actualResult = __CFDefaultToUnicodeFallbackProc(
                bytes, length, actual + 2, capacities[capacity], &actualUsed
            );
            if (expectedResult != actualResult || expectedUsed != actualUsed ||
                memcmp(expected, actual, sizeof(expected)) != 0) {
                fprintf(
                    stderr, "Fallback mismatch: capacity %ld, length %ld.\n", capacities[capacity], length
                );
                return 1;
            }
            ++*cases;
        }
    }
    PlatformConversion nativeToBytes = (PlatformConversion)native[1];
    PlatformConversion nativeToUnicode = (PlatformConversion)native[2];
    PlatformConversion replacementToBytes = (PlatformConversion)replacement[1];
    PlatformConversion replacementToUnicode = (PlatformConversion)replacement[2];
    PlatformLength nativeLength = (PlatformLength)native[3];
    uint64_t state = UINT64_C(0x2545f4914f6cdd1d);
    for (size_t index = 0; index < 4096; ++index) {
        uint32_t encoding = (uint32_t)nextBits(&state);
        uint32_t flags = (uint32_t)nextBits(&state);
        OCFIndex length = (OCFIndex)nextBits(&state);
        OCFIndex sentinel[4] = {-1, -2, -3, -4};
        OCFIndex copy[4] = {-1, -2, -3, -4};
        uint8_t buffer[4] = {1, 2, 3, 4};
        if (nativeToBytes(encoding, flags, buffer, length, &sentinel[0], buffer, length, &sentinel[1]) !=
                replacementToBytes(encoding, flags, buffer, length, &copy[0], buffer, length, &copy[1]) ||
            nativeToUnicode(encoding, flags, buffer, length, &sentinel[2], buffer, length, &sentinel[3]) !=
                replacementToUnicode(encoding, flags, buffer, length, &copy[2], buffer, length, &copy[3]) ||
            nativeLength(encoding, flags, buffer, length) !=
                __CFStringEncodingPlatformCharLengthForBytes(encoding, flags, buffer, length) ||
            memcmp(sentinel, copy, sizeof(sentinel)) != 0) {
            fprintf(stderr, "Platform converter mismatch: case %zu.\n", index);
            return 1;
        }
        *cases += 3;
    }
    return 0;
}

static int compareToBytes(
    const char *name,
    ToBytesWrapper native,
    ToBytesWrapper replacement,
    const StringEncodingConverterRecord *converter,
    int allowNegative,
    size_t *cases
) {
    const OCFIndex capacities[] = {0, 1, 2, 3, 4, 7, 16, 40, 64, -1};
    const uint32_t flags[] = {0, 1, 2, UINT32_MAX};
    uint64_t state = UINT64_C(0x853c49e6748fea9b) ^ (uint64_t)(uintptr_t)name[0];
    for (size_t test = 0; test < 24000; ++test) {
        uint16_t input[kInputLength];
        for (size_t index = 0; index < kInputLength; ++index) {
            uint64_t bits = nextBits(&state);
            input[index] = (bits & 3) ? kAlphabet[(bits >> 8) % kAlphabetCount] : (uint16_t)(bits >> 24);
            if ((bits & 0xff0) == 0x120) {
                input[index] = 0xfffe;
            }
        }
        OCFIndex length = (OCFIndex)(nextBits(&state) % (kInputLength + 1));
        if (allowNegative && (test % 97) == 0) {
            length = -(OCFIndex)(test % 3) - 1;
        }
        OCFIndex capacity = capacities[test % (sizeof(capacities) / sizeof(capacities[0]))];
        uint32_t flag = flags[(test / 10) % (sizeof(flags) / sizeof(flags[0]))];
        uint8_t expected[kBufferLength];
        uint8_t actual[kBufferLength];
        memset(expected, 0xa5, sizeof(expected));
        memset(actual, 0xa5, sizeof(actual));
        OCFIndex expectedUsed = -77;
        OCFIndex actualUsed = -77;
        OCFIndex expectedResult =
            native(converter, flag, input, length, expected + kBufferOffset, capacity, &expectedUsed);
        OCFIndex actualResult =
            replacement(converter, flag, input, length, actual + kBufferOffset, capacity, &actualUsed);
        if (expectedResult != actualResult || expectedUsed != actualUsed ||
            memcmp(expected, actual, sizeof(expected)) != 0) {
            fprintf(
                stderr,
                "%s mismatch: case %zu, length %ld, capacity %ld: %ld/%ld, used %ld/%ld.\n",
                name,
                test,
                length,
                capacity,
                expectedResult,
                actualResult,
                expectedUsed,
                actualUsed
            );
            return 1;
        }
        ++*cases;
    }
    return 0;
}

static int compareToUnicode(
    const char *name,
    ToUnicodeWrapper native,
    ToUnicodeWrapper replacement,
    const StringEncodingConverterRecord *converter,
    int allowNegative,
    size_t *cases
) {
    const OCFIndex capacities[] = {0, 1, 2, 3, 4, 7, 16, 40, 64, -1};
    const uint32_t flags[] = {0, 1, 2, UINT32_MAX};
    uint64_t state = UINT64_C(0xda3e39cb94b95bdb) ^ (uint64_t)(uintptr_t)name[0];
    for (size_t test = 0; test < 24000; ++test) {
        uint8_t input[kInputLength];
        for (size_t index = 0; index < kInputLength; ++index) {
            input[index] = (uint8_t)nextBits(&state);
        }
        OCFIndex length = (OCFIndex)(nextBits(&state) % (kInputLength + 1));
        if (allowNegative && (test % 97) == 0) {
            length = -(OCFIndex)(test % 3) - 1;
        }
        OCFIndex capacity = capacities[test % (sizeof(capacities) / sizeof(capacities[0]))];
        uint32_t flag = flags[(test / 10) % (sizeof(flags) / sizeof(flags[0]))];
        uint16_t expected[kBufferLength];
        uint16_t actual[kBufferLength];
        memset(expected, 0xa5, sizeof(expected));
        memset(actual, 0xa5, sizeof(actual));
        OCFIndex expectedUsed = -77;
        OCFIndex actualUsed = -77;
        OCFIndex expectedResult =
            native(converter, flag, input, length, expected + kBufferOffset, capacity, &expectedUsed);
        OCFIndex actualResult =
            replacement(converter, flag, input, length, actual + kBufferOffset, capacity, &actualUsed);
        if (expectedResult != actualResult || expectedUsed != actualUsed ||
            memcmp(expected, actual, sizeof(expected)) != 0) {
            fprintf(
                stderr,
                "%s mismatch: case %zu, length %ld, capacity %ld: %ld/%ld, used %ld/%ld.\n",
                name,
                test,
                length,
                capacity,
                expectedResult,
                actualResult,
                expectedUsed,
                actualUsed
            );
            return 1;
        }
        ++*cases;
    }
    return 0;
}

static int compareWrappers(size_t *cases) {
    const StringEncodingConverterRecord cheapEightBit = {
        .toBytes.cheapEightBit = testCheapEightBitToBytes,
        .toUnicode.cheapEightBit = testCheapEightBitToUnicode,
    };
    const StringEncodingConverterRecord standardEightBit = {
        .toBytes.standardEightBit = testStandardEightBitToBytes,
        .toUnicode.standardEightBit = testStandardEightBitToUnicode,
    };
    const StringEncodingConverterRecord cheapMultiByte = {
        .toBytes.cheapMultiByte = testCheapMultiByteToBytes,
        .toUnicode.cheapMultiByte = testCheapMultiByteToUnicode,
    };

    struct {
        const char *name;
        uintptr_t address;
        void *replacement;
        const StringEncodingConverterRecord *converter;
        int toBytes;
        int allowNegative;
    } wrappers[] = {
        {"ToBytesCheapEightBit",
         UINT64_C(0x1c022b484),
         (void *)__CFToBytesCheapEightBitWrapper,
         &cheapEightBit,
         1,
         1},
        {"ToUnicodeCheapEightBit",
         UINT64_C(0x1c022b564),
         (void *)__CFToUnicodeCheapEightBitWrapper,
         &cheapEightBit,
         0,
         1},
        {"ToCanonicalUnicodeCheapEightBit",
         UINT64_C(0x1c022b644),
         (void *)__CFToCanonicalUnicodeCheapEightBitWrapper,
         &cheapEightBit,
         0,
         1},
        {"ToBytesStandardEightBit",
         UINT64_C(0x1c022b7d0),
         (void *)__CFToBytesStandardEightBitWrapper,
         &standardEightBit,
         1,
         0},
        {"ToUnicodeStandardEightBit",
         UINT64_C(0x1c022b8e8),
         (void *)__CFToUnicodeStandardEightBitWrapper,
         &standardEightBit,
         0,
         1},
        {"ToCanonicalUnicodeStandardEightBit",
         UINT64_C(0x1c022b9f0),
         (void *)__CFToCanonicalUnicodeStandardEightBitWrapper,
         &standardEightBit,
         0,
         1},
        {"ToBytesCheapMultiByte",
         UINT64_C(0x1c022bba4),
         (void *)__CFToBytesCheapMultiByteWrapper,
         &cheapMultiByte,
         1,
         1},
        {"ToUnicodeCheapMultiByte",
         UINT64_C(0x1c022bcc0),
         (void *)__CFToUnicodeCheapMultiByteWrapper,
         &cheapMultiByte,
         0,
         0},
        {"ToCanonicalUnicodeCheapMultiByte",
         UINT64_C(0x1c022bdd4),
         (void *)__CFToCanonicalUnicodeCheapMultiByteWrapper,
         &cheapMultiByte,
         0,
         0},
    };

    for (size_t index = 0; index < sizeof(wrappers) / sizeof(wrappers[0]); ++index) {
        void *native = NULL;
        int status = resolvePair(wrappers[index].address, wrappers[index].replacement, &native);
        if (status != 0) {
            return status;
        }
        if (wrappers[index].toBytes) {
            status = compareToBytes(
                wrappers[index].name,
                (ToBytesWrapper)native,
                (ToBytesWrapper)wrappers[index].replacement,
                wrappers[index].converter,
                wrappers[index].allowNegative,
                cases
            );
        } else {
            status = compareToUnicode(
                wrappers[index].name,
                (ToUnicodeWrapper)native,
                (ToUnicodeWrapper)wrappers[index].replacement,
                wrappers[index].converter,
                wrappers[index].allowNegative,
                cases
            );
        }
        if (status != 0) {
            return status;
        }
    }
    return 0;
}

// Native CFStringConvertEncodingToNSStringEncoding 0x1c02340fc (exported); hidden functions resolved by IDA
// offset: the nine converter wrappers 0x1c022b484-0x1c022bdd4, __CFDefaultToUnicodeFallbackProc 0x1c022c174,
// the platform converter functions 0x1c01cecc4/0x1c01ceccc/0x1c01cecd4, __CFStringEncodingGetWindowsCodePage
// 0x1c02ae7c4,
// __CFStringEncodingGetCanonicalName 0x1c02aea2c and __CFStringEncodingGetICUName 0x1c02ad994.
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
// iOS Device 24A446 addresses and divergences: the definitions and
// Documentation/Reference/Research/CoreFoundationStringEncoding.md.
int runStringEncodingNameDifferentialTests(void) {
    static uint32_t encodings[0x20000 + 13 * 0x1000 + 256];
    size_t encodingCount = encodingCases(encodings, sizeof(encodings) / sizeof(encodings[0]));
    size_t cases = 0;
    int status = compareScalarFunctions(encodings, encodingCount, &cases);
    if (status == 0) {
        status = compareDatabase(encodings, encodingCount, &cases);
    }
    if (status == 0) {
        status = compareFallbackAndPlatform(&cases);
    }
    if (status == 0) {
        status = compareWrappers(&cases);
    }
    if (status != 0) {
        return status;
    }
    printf(
        "String-encoding name/converter differential cases passed: %zu (seventeen functions; finite "
        "coverage).\n",
        cases
    );
    return 0;
}

//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PropertyList/PropertyListHelperTests.h"
#include "CoreFoundation/PropertyList/OCFBinaryPList.h"
#include "CoreFoundation/PropertyList/OCFOldStylePList.h"
#include "CoreFoundation/PropertyList/OCFPropertyList.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef bool (*OffsetFunction)(
    const uint8_t *, uint64_t, uint64_t, const BinaryPlistTrailer *, OCFIndex, uint64_t *, const void *
);

typedef bool (*StringFunction)(const uint8_t *, uint64_t, const uint8_t *, BinaryPlistStringBlock);

typedef bool (*FilterFunction)(const char *, OCFIndex, const char *, OCFIndex);

typedef uint32_t (*LineNumberFunction)(const XMLPlistParseContext *);

typedef uint32_t (*LineNumberStringsFunction)(const OldStylePlistParseContext *);

typedef bool (*AdvanceFunction)(OldStylePlistParseContext *);

typedef struct {
    OffsetFunction offset;
    StringFunction ascii;
    StringFunction unicode16;
    FilterFunction filter;
    LineNumberFunction lineNumber;
    LineNumberStringsFunction lineNumberStrings;
    AdvanceFunction advance;
} PropertyListFunctions;

typedef struct {
    size_t calls;
    const uint8_t *characters;
    uint64_t count;
} BlockRecord;

// Accepted (true) results, reported so that coverage of the success paths stays visible.
static size_t acceptedOffsets = 0;
static size_t acceptedStrings = 0;

enum {
    kOffsetBufferSize = 65536,
    kOffsetTableLimit = 4096,
    kStringBufferSize = 4096,
};

static uint64_t nextRandom(uint64_t *state) {
    uint64_t value = (*state += UINT64_C(0x9e3779b97f4a7c15));
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31);
}

static int resolveHidden(uintptr_t address, void *replacement, void **native) {
    int status = resolveReferenceAddress(
        "CoreFoundation", OF_REFERENCE_CF_UUID, address, UINT64_C(0x1c014a000), native
    );
    if (status == 0) {
        status = checkInternalImplementation(replacement);
    }
    if (status == 0 && *native == replacement) {
        fputs("Setup failure: property-list helper reference resolved to the replacement.\n", stderr);
        status = 77;
    }
    return status;
}

static int
resolveExported(const char *nativeName, const char *replacementName, void **native, void **replacement) {
    int status = resolveReferenceFunction("CoreFoundation", OF_REFERENCE_CF_UUID, nativeName, native);
    if (status == 0) {
        status = resolveReplacementFunction("OpenCoreFoundation", replacementName, replacement);
    }
    if (status == 0 && *native == *replacement) {
        fputs("Setup failure: property-list reference resolved to the replacement.\n", stderr);
        status = 77;
    }
    return status;
}

static int resolveFunctions(PropertyListFunctions *native, PropertyListFunctions *replacement) {
    void *nativeFunction = NULL;
    void *replacementFunction = NULL;
    int status = resolveExported(
        OF_NAME_STRING(__CFBinaryPlistGetOffsetForValueFromArray2),
        OF_NAME_STRING(__OCFBinaryPlistGetOffsetForValueFromArray2),
        &nativeFunction,
        &replacementFunction
    );
    if (status != 0) {
        return status;
    }
    native->offset = (OffsetFunction)nativeFunction;
    replacement->offset = (OffsetFunction)replacementFunction;
    status = resolveExported(
        OF_NAME_STRING(__CFBinaryPlistParseASCIIString),
        OF_NAME_STRING(__OCFBinaryPlistParseASCIIString),
        &nativeFunction,
        &replacementFunction
    );
    if (status != 0) {
        return status;
    }
    native->ascii = (StringFunction)nativeFunction;
    replacement->ascii = (StringFunction)replacementFunction;
    status = resolveExported(
        OF_NAME_STRING(__CFBinaryPlistParseUnicode16String),
        OF_NAME_STRING(__OCFBinaryPlistParseUnicode16String),
        &nativeFunction,
        &replacementFunction
    );
    if (status != 0) {
        return status;
    }
    native->unicode16 = (StringFunction)nativeFunction;
    replacement->unicode16 = (StringFunction)replacementFunction;
    status = resolveHidden(
        UINT64_C(0x1c01d446c), (void *)__CFPropertyListKeyBufferMatchesFilterBuffer, &nativeFunction
    );
    if (status != 0) {
        return status;
    }
    native->filter = (FilterFunction)nativeFunction;
    replacement->filter = __CFPropertyListKeyBufferMatchesFilterBuffer;
    status = resolveHidden(UINT64_C(0x1c01d84dc), (void *)lineNumber, &nativeFunction);
    if (status != 0) {
        return status;
    }
    native->lineNumber = (LineNumberFunction)nativeFunction;
    replacement->lineNumber = lineNumber;
    status = resolveHidden(UINT64_C(0x1c02d54e0), (void *)lineNumberStrings, &nativeFunction);
    if (status != 0) {
        return status;
    }
    native->lineNumberStrings = (LineNumberStringsFunction)nativeFunction;
    replacement->lineNumberStrings = lineNumberStrings;
    status = resolveHidden(UINT64_C(0x1c02d4e94), (void *)advanceToNonSpace, &nativeFunction);
    if (status != 0) {
        return status;
    }
    native->advance = (AdvanceFunction)nativeFunction;
    replacement->advance = advanceToNonSpace;
    return 0;
}

static void writeBigEndian(uint8_t *bytes, uint64_t value, size_t size) {
    for (size_t index = 0; index < size; ++index) {
        bytes[size - 1 - index] = index < 8 ? (uint8_t)(value >> (8 * index)) : 0;
    }
}

static const uint8_t kIntegerSizes[] = {0, 1, 2, 3, 4, 5, 8, 16, 255};

// One binary array case inside a 64 KiB buffer: every read the native function can make, for the
// parameters chosen here, stays inside the buffer (see the reading).
static int compareOffsetCase(
    const PropertyListFunctions *native,
    const PropertyListFunctions *replacement,
    uint8_t *buffer,
    uint64_t *state,
    size_t *cases
) {
    uint64_t choice = nextRandom(state);
    for (size_t index = 0; index < 512; ++index) {
        uint64_t bits = nextRandom(state);
        memcpy(buffer + index * 8, &bits, 8);
    }
    BinaryPlistTrailer trailer;
    memset(&trailer, 0xa5, sizeof(trailer));
    trailer.offsetTableOffset = (choice & 0x3f) == 0 ? 0 : 8 + nextRandom(state) % (kOffsetTableLimit - 8);
    trailer.offsetIntSize = kIntegerSizes[nextRandom(state) % sizeof(kIntegerSizes)];
    trailer.objectRefSize = kIntegerSizes[nextRandom(state) % sizeof(kIntegerSizes)];
    uint64_t room =
        (kOffsetBufferSize - kOffsetTableLimit) / (trailer.offsetIntSize ? trailer.offsetIntSize : 1);
    uint64_t numObjects = 1 + nextRandom(state) % (room < 256 ? room : 256);
    if (trailer.offsetIntSize == 0 && (choice & 0x40)) {
        numObjects = nextRandom(state);
    }
    trailer.numObjects = (choice & 0x180) == 0 ? 0 : numObjects;
    uint64_t tableOffset = trailer.offsetTableOffset ? trailer.offsetTableOffset : kOffsetTableLimit;
    uint64_t startOffset = nextRandom(state) % (tableOffset + 4);
    if ((choice & 0x600) != 0 && tableOffset > 16) {
        startOffset = 8 + nextRandom(state) % (tableOffset - 16);
    }
    uint64_t count = nextRandom(state) % 15;
    uint8_t *cursor = buffer + startOffset;
    *cursor++ = (uint8_t)(((choice >> 11) & 7) == 0 ? nextRandom(state) : 0xa0 | count);
    if ((choice >> 14) & 1) {
        cursor[-1] |= 0x0f;
        uint64_t exponent = nextRandom(state) % 16;
        if (exponent > 8 && ((choice >> 15) & 3) != 0) {
            exponent %= 5;
        }
        *cursor++ = (uint8_t)(((choice >> 17) & 15) == 0 ? nextRandom(state) : 0x10 | exponent);
        count = nextRandom(state) % 40;
        if (((choice >> 21) & 15) == 0) {
            count = nextRandom(state) >> (nextRandom(state) % 64);
        }
        uint64_t byteCount = UINT64_C(1) << exponent;
        if (startOffset + 2 + byteCount < kOffsetBufferSize) {
            writeBigEndian(cursor, count, (size_t)byteCount);
            cursor += byteCount;
        }
    }
    uint64_t referencesStart = (uint64_t)(cursor - buffer);
    for (uint64_t reference = 0; reference < count && reference < 64; ++reference) {
        uint64_t offset = referencesStart + reference * trailer.objectRefSize;
        if (offset + trailer.objectRefSize >= kOffsetBufferSize) {
            break;
        }
        uint64_t value = nextRandom(state) % (trailer.numObjects + 2);
        writeBigEndian(buffer + offset, value, trailer.objectRefSize);
    }
    OCFIndex arrayIndex = (OCFIndex)(nextRandom(state) % (count + 3));
    if (((choice >> 25) & 15) == 0) {
        arrayIndex = (OCFIndex)nextRandom(state);
    }
    int withOffset = ((choice >> 29) & 3) != 0;
    uint64_t expected[3] = {
        UINT64_C(0xa5a5a5a5a5a5a5a5), UINT64_C(0xa5a5a5a5a5a5a5a5), UINT64_C(0xa5a5a5a5a5a5a5a5)
    };
    uint64_t actual[3] = {
        UINT64_C(0xa5a5a5a5a5a5a5a5), UINT64_C(0xa5a5a5a5a5a5a5a5), UINT64_C(0xa5a5a5a5a5a5a5a5)
    };
    bool expectedResult = native->offset(
        buffer, kOffsetBufferSize, startOffset, &trailer, arrayIndex, withOffset ? &expected[1] : NULL, NULL
    );
    bool actualResult = replacement->offset(
        buffer, kOffsetBufferSize, startOffset, &trailer, arrayIndex, withOffset ? &actual[1] : NULL, NULL
    );
    if (expectedResult != actualResult || memcmp(expected, actual, sizeof(expected)) != 0) {
        fprintf(
            stderr,
            "__CFBinaryPlistGetOffsetForValueFromArray2 mismatch: start %llu, table %llu, sizes %u/%u, "
            "objects %llu, index %lld.\n",
            (unsigned long long)startOffset,
            (unsigned long long)trailer.offsetTableOffset,
            trailer.offsetIntSize,
            trailer.objectRefSize,
            (unsigned long long)trailer.numObjects,
            (long long)arrayIndex
        );
        return 1;
    }
    acceptedOffsets += expectedResult;
    ++*cases;
    return 0;
}

static bool callString(
    StringFunction function, const uint8_t *base, uint64_t limit, const uint8_t *marker, BlockRecord *record
) {
    memset(record, 0, sizeof(*record));
    return function(base, limit, marker, ^bool(const uint8_t *characters, uint64_t count) {
      ++record->calls;
      record->characters = characters;
      record->count = count;
      return ((count ^ (uintptr_t)characters) & 1) == 0;
    });
}

static int compareStringCase(
    const PropertyListFunctions *native,
    const PropertyListFunctions *replacement,
    uint8_t *buffer,
    uint64_t markerOffset,
    uint64_t baseOffset,
    uint64_t endOffset,
    size_t *cases
) {
    const uint8_t *marker = buffer + markerOffset;
    const uint8_t *base = buffer + baseOffset;
    uint64_t limit = endOffset - baseOffset;
    StringFunction functions[2][2] = {
        {native->ascii, replacement->ascii}, {native->unicode16, replacement->unicode16}
    };
    for (size_t kind = 0; kind < 2; ++kind) {
        BlockRecord expected;
        BlockRecord actual;
        bool expectedResult = callString(functions[kind][0], base, limit, marker, &expected);
        bool actualResult = callString(functions[kind][1], base, limit, marker, &actual);
        if (expectedResult != actualResult || memcmp(&expected, &actual, sizeof(expected)) != 0) {
            fprintf(
                stderr,
                "%s mismatch: marker %llu, base %llu, end %llu, bytes %02x %02x.\n",
                kind ? "__CFBinaryPlistParseUnicode16String" : "__CFBinaryPlistParseASCIIString",
                (unsigned long long)markerOffset,
                (unsigned long long)baseOffset,
                (unsigned long long)endOffset,
                marker[0],
                marker[1]
            );
            return 1;
        }
        acceptedStrings += expectedResult;
        ++*cases;
    }
    return 0;
}

static int compareStrings(
    const PropertyListFunctions *native,
    const PropertyListFunctions *replacement,
    uint64_t *state,
    size_t *cases
) {
    uint8_t *buffer = malloc(kStringBufferSize);
    if (!buffer) {
        fputs("Setup failure: string buffer.\n", stderr);
        return 77;
    }
    int status = 0;
    // Every marker byte and every following integer-marker byte, at three ends.
    for (uint32_t bytes = 0; bytes < 0x10000 && status == 0; ++bytes) {
        for (size_t index = 0; index < kStringBufferSize; ++index) {
            buffer[index] = (uint8_t)(index * 7 + bytes);
        }
        buffer[100] = (uint8_t)bytes;
        buffer[101] = (uint8_t)(bytes >> 8);
        static const uint64_t kEnds[] = {99, 100, 101, 102, 103, 109, 117, 400, 4000};
        for (size_t end = 0; end < sizeof(kEnds) / sizeof(kEnds[0]) && status == 0; ++end) {
            status = compareStringCase(native, replacement, buffer, 100, 40, kEnds[end], cases);
        }
    }
    // Random markers, count integers of every width (including bit 63) and ends around the data.
    for (size_t round = 0; round < 1000000 && status == 0; ++round) {
        uint64_t choice = nextRandom(state);
        uint64_t markerOffset = nextRandom(state) % 300;
        uint8_t *cursor = buffer + markerOffset;
        *cursor++ = (uint8_t)(nextRandom(state) | ((choice & 3) ? 0x0f : 0));
        uint64_t exponent = nextRandom(state) % 16;
        if (exponent > 7 && ((choice >> 2) & 3) != 0) {
            exponent %= 8;
        }
        *cursor++ = (uint8_t)(((choice >> 4) & 15) == 0 ? nextRandom(state) : 0x10 | exponent);
        uint64_t count = nextRandom(state) % 600;
        if (((choice >> 8) & 7) == 0) {
            count = nextRandom(state) >> (nextRandom(state) % 64);
        }
        uint64_t byteCount = UINT64_C(1) << exponent;
        if (byteCount <= 128) {
            writeBigEndian(cursor, count, (size_t)byteCount);
        }
        uint64_t endOffset = markerOffset + nextRandom(state) % 1400;
        if (markerOffset > 0 && ((choice >> 11) & 7) == 0) {
            endOffset = markerOffset - 1 + nextRandom(state) % 3;
        }
        uint64_t baseOffset = nextRandom(state) % (endOffset + 1);
        status = compareStringCase(native, replacement, buffer, markerOffset, baseOffset, endOffset, cases);
    }
    free(buffer);
    if (status != 0) {
        return status;
    }
    // The last address as marker fails before any read.
    const uint8_t *last = (const uint8_t *)UINTPTR_MAX;
    BlockRecord expected;
    BlockRecord actual;
    StringFunction functions[2][2] = {
        {native->ascii, replacement->ascii}, {native->unicode16, replacement->unicode16}
    };
    for (size_t kind = 0; kind < 2; ++kind) {
        bool expectedResult = callString(functions[kind][0], NULL, 0, last, &expected);
        bool actualResult = callString(functions[kind][1], NULL, 0, last, &actual);
        if (expectedResult != actualResult || memcmp(&expected, &actual, sizeof(expected)) != 0) {
            fputs("Binary plist string last-address mismatch.\n", stderr);
            return 1;
        }
        ++*cases;
    }
    return 0;
}

static int
compareFilters(const PropertyListFunctions *native, const PropertyListFunctions *replacement, size_t *cases) {
    // Every string of length 0..4 over "a", "b" and "*" as key and filter, with lengths from -2 to the
    // buffer length + 1; equal negative lengths and a negative prefix length pass a huge size to
    // memcmp natively and are not called.
    static const char kAlphabet[3] = {'a', 'b', '*'};
    char keys[121][6];
    size_t count = 0;
    for (size_t length = 0; length <= 4; ++length) {
        size_t total = 1;
        for (size_t index = 0; index < length; ++index) {
            total *= 3;
        }
        for (size_t value = 0; value < total; ++value) {
            size_t digits = value;
            memset(keys[count], 'z', sizeof(keys[count]));
            for (size_t index = 0; index < length; ++index) {
                keys[count][index] = kAlphabet[digits % 3];
                digits /= 3;
            }
            keys[count][5] = (char)length;
            ++count;
        }
    }
    for (size_t key = 0; key < count; ++key) {
        for (size_t filter = 0; filter < count; ++filter) {
            OCFIndex keyBase = keys[key][5];
            OCFIndex filterBase = keys[filter][5];
            for (OCFIndex keyLength = -2; keyLength <= keyBase + 1; ++keyLength) {
                for (OCFIndex filterLength = -2; filterLength <= filterBase + 1; ++filterLength) {
                    if (filterLength < 0 && keyLength == filterLength) {
                        continue;
                    }
                    bool expected = native->filter(keys[key], keyLength, keys[filter], filterLength);
                    bool actual = replacement->filter(keys[key], keyLength, keys[filter], filterLength);
                    if (expected != actual) {
                        fprintf(
                            stderr,
                            "__CFPropertyListKeyBufferMatchesFilterBuffer mismatch: lengths %ld/%ld.\n",
                            (long)keyLength,
                            (long)filterLength
                        );
                        return 1;
                    }
                    ++*cases;
                }
            }
        }
    }
    return 0;
}

static int compareLineNumbers(
    const PropertyListFunctions *native, const PropertyListFunctions *replacement, size_t *cases
) {
    // Every byte string of length 0..10 over LF, CR and 'a', with curr at every position and three ends.
    static const uint8_t kBytes[3] = {'\n', '\r', 'a'};
    uint8_t buffer[12];
    for (size_t length = 0; length <= 10; ++length) {
        size_t total = 1;
        for (size_t index = 0; index < length; ++index) {
            total *= 3;
        }
        for (size_t value = 0; value < total; ++value) {
            size_t digits = value;
            for (size_t index = 0; index < length; ++index) {
                buffer[index] = kBytes[digits % 3];
                digits /= 3;
            }
            buffer[length] = '\n';
            for (size_t curr = 0; curr <= length + 1; ++curr) {
                for (size_t endChoice = 0; endChoice < 3; ++endChoice) {
                    size_t end = endChoice == 0 ? length : (endChoice == 1 ? (curr ? curr - 1 : 0) : curr);
                    if (end > length) {
                        end = length;
                    }
                    XMLPlistParseContext context = {buffer, buffer + curr, buffer + end};
                    uint32_t expected = native->lineNumber(&context);
                    uint32_t actual = replacement->lineNumber(&context);
                    if (expected != actual) {
                        fprintf(
                            stderr, "lineNumber mismatch: length %zu, curr %zu, end %zu.\n", length, curr, end
                        );
                        return 1;
                    }
                    ++*cases;
                }
            }
        }
    }
    // The UTF-16 form over LF, CR, 'a', U+0D0A and U+2028, length 0..8.
    static const uint16_t kUnits[5] = {'\n', '\r', 'a', 0x0d0a, 0x2028};
    uint16_t units[10];
    for (size_t length = 0; length <= 8; ++length) {
        size_t total = 1;
        for (size_t index = 0; index < length; ++index) {
            total *= 5;
        }
        for (size_t value = 0; value < total; ++value) {
            size_t digits = value;
            for (size_t index = 0; index < length; ++index) {
                units[index] = kUnits[digits % 5];
                digits /= 5;
            }
            units[length] = '\n';
            for (size_t curr = 0; curr <= length + 1; ++curr) {
                for (size_t endChoice = 0; endChoice < 3; ++endChoice) {
                    size_t end = endChoice == 0 ? length : (endChoice == 1 ? (curr ? curr - 1 : 0) : curr);
                    if (end > length) {
                        end = length;
                    }
                    OldStylePlistParseContext context = {units, units + curr, units + end};
                    uint32_t expected = native->lineNumberStrings(&context);
                    uint32_t actual = replacement->lineNumberStrings(&context);
                    if (expected != actual) {
                        fprintf(
                            stderr,
                            "lineNumberStrings mismatch: length %zu, curr %zu, end %zu.\n",
                            length,
                            curr,
                            end
                        );
                        return 1;
                    }
                    ++*cases;
                }
            }
        }
    }
    return 0;
}

static int
compareAdvance(const PropertyListFunctions *native, const PropertyListFunctions *replacement, size_t *cases) {
    // Every unit string of length 0..7 over space, tab, LF, CR, '/', '*', 'a' and U+2028, starting at
    // every position (and one past the end), with the end at the length. The context is compared whole.
    static const uint16_t kUnits[8] = {' ', '\t', '\n', '\r', '/', '*', 'a', 0x2028};
    uint16_t units[10];
    for (size_t length = 0; length <= 7; ++length) {
        size_t total = (size_t)1 << (3 * length);
        for (size_t value = 0; value < total; ++value) {
            for (size_t index = 0; index < length; ++index) {
                units[index] = kUnits[(value >> (3 * index)) & 7];
            }
            units[length] = '/';
            units[length + 1] = '/';
            for (size_t start = 0; start <= length + 1; ++start) {
                OldStylePlistParseContext expected = {units, units + start, units + length};
                OldStylePlistParseContext actual = expected;
                bool expectedResult = native->advance(&expected);
                bool actualResult = replacement->advance(&actual);
                if (expectedResult != actualResult || memcmp(&expected, &actual, sizeof(expected)) != 0) {
                    fprintf(
                        stderr,
                        "advanceToNonSpace mismatch: length %zu, value %zu, start %zu.\n",
                        length,
                        value,
                        start
                    );
                    return 1;
                }
                ++*cases;
            }
        }
    }
    return 0;
}

// Native __CFBinaryPlistGetOffsetForValueFromArray2 0x1c0163e48, __CFBinaryPlistParseASCIIString
// 0x1c01667dc and __CFBinaryPlistParseUnicode16String 0x1c01668f4 (external), and
// __CFPropertyListKeyBufferMatchesFilterBuffer 0x1c01d446c, lineNumber 0x1c01d84dc, lineNumberStrings
// 0x1c02d54e0 and advanceToNonSpace 0x1c02d4e94 (non-external; resolved by IDA offset), iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. iOS Device 24A446: 0x180733140,
// 0x18061f684, 0x18068035c, 0x18068f8c4, 0x1806f6734, 0x18070d188, 0x180702bac;
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified for the inline bodies.
// Results, the offset out parameter between canaries, every block call (count, pointer, argument) and
// the parser context after advanceToNonSpace are compared exactly.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationPropertyListHelperDifferentialTests(void) {
    PropertyListFunctions native;
    PropertyListFunctions replacement;
    int status = resolveFunctions(&native, &replacement);
    if (status != 0) {
        return status;
    }
    size_t cases = 0;
    uint64_t state = UINT64_C(0x5eed0f0b16a0201);
    uint8_t *buffer = malloc(kOffsetBufferSize);
    if (!buffer) {
        fputs("Setup failure: binary plist buffer.\n", stderr);
        return 77;
    }
    memset(buffer, 0, kOffsetBufferSize);
    for (size_t round = 0; round < 2000000 && status == 0; ++round) {
        status = compareOffsetCase(&native, &replacement, buffer, &state, &cases);
    }
    free(buffer);
    if (status == 0) {
        status = compareStrings(&native, &replacement, &state, &cases);
    }
    if (status == 0) {
        status = compareFilters(&native, &replacement, &cases);
    }
    if (status == 0) {
        status = compareLineNumbers(&native, &replacement, &cases);
    }
    if (status == 0) {
        status = compareAdvance(&native, &replacement, &cases);
    }
    if (status != 0) {
        return status;
    }
    printf(
        "Property-list helper differential cases passed: %zu (seven functions; %zu array and %zu string "
        "calls accepted; finite coverage).\n",
        cases,
        acceptedOffsets,
        acceptedStrings
    );
    return 0;
}

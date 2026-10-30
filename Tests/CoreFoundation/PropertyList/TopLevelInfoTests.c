//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PropertyList/TopLevelInfoTests.h"
#include "CoreFoundation/PropertyList/OCFBinaryPList.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef bool (*TopLevelInfoFunction)(const uint8_t *, uint64_t, uint8_t *, uint64_t *, BinaryPlistTrailer *);

enum {
    kBufferSize = 65536,
};

static size_t acceptedCases = 0;

static uint64_t nextRandom(uint64_t *state) {
    uint64_t value = (*state += UINT64_C(0x9e3779b97f4a7c15));
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31);
}

static void storeBigEndian(uint8_t *bytes, uint64_t value, uint32_t width) {
    for (uint32_t index = width; index > 0; --index) {
        bytes[index - 1] = (uint8_t)value;
        value >>= 8;
    }
}

// Builds a well-formed binary property list of random shape (entry widths, object count, object-area
// size, entry values, top object), then optionally perturbs it. Returns the length used.
static uint64_t buildCandidate(uint8_t *buffer, uint64_t *state) {
    uint64_t bits = nextRandom(state);
    static const uint8_t kWidths[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 16};
    uint32_t offsetIntSize = kWidths[(bits & 0xff) % sizeof(kWidths)];
    uint32_t objectRefSize = kWidths[((bits >> 8) & 0xff) % sizeof(kWidths)];
    if ((bits >> 16) % 13 == 0) {
        objectRefSize = (uint32_t)((bits >> 24) & 0xff);
    }
    uint64_t numObjects = 1 + nextRandom(state) % 300;
    uint64_t objectArea = 1 + nextRandom(state) % 2000;
    uint64_t offsetTableOffset = 8 + objectArea;
    if (offsetIntSize == 1 && offsetTableOffset > 0xff) {
        offsetTableOffset = 8 + nextRandom(state) % 0xf8;
    }
    if (offsetIntSize == 1 && numObjects > 0xff) {
        numObjects = 1 + numObjects % 0xff;
    }
    uint64_t length = offsetTableOffset + numObjects * offsetIntSize + 32;
    memcpy(buffer, "bplist00", 8);
    for (uint64_t index = 8; index < offsetTableOffset; ++index) {
        buffer[index] = (uint8_t)nextRandom(state);
    }
    for (uint64_t index = 0; index < numObjects; ++index) {
        uint64_t value = 8 + nextRandom(state) % (offsetTableOffset - 8);
        storeBigEndian(buffer + offsetTableOffset + index * offsetIntSize, value, offsetIntSize);
    }
    uint8_t *trailer = buffer + length - 32;
    for (uint32_t index = 0; index < 6; ++index) {
        trailer[index] = (uint8_t)nextRandom(state);
    }
    trailer[6] = (uint8_t)offsetIntSize;
    trailer[7] = (uint8_t)objectRefSize;
    storeBigEndian(trailer + 8, numObjects, 8);
    storeBigEndian(trailer + 16, nextRandom(state) % numObjects, 8);
    storeBigEndian(trailer + 24, offsetTableOffset, 8);
    uint64_t mutation = nextRandom(state);
    switch (mutation % 16) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            break;
        case 6:
            buffer[nextRandom(state) % length] ^= (uint8_t)(1 + (mutation >> 8) % 255);
            break;
        case 7:
            trailer[8 + (mutation >> 8) % 24] = (uint8_t)nextRandom(state);
            break;
        case 8:
            trailer[6 + (mutation >> 8) % 2] = (uint8_t)nextRandom(state);
            break;
        case 9: {
            static const uint64_t kFieldValues[] = {
                0,
                1,
                7,
                8,
                9,
                UINT64_C(0x7fffffffffffffff),
                UINT64_C(0x8000000000000000),
                UINT64_C(0xffffffffffffffff),
                UINT64_C(0x0100000000000000)
            };
            uint64_t value = kFieldValues[(mutation >> 8) % (sizeof(kFieldValues) / sizeof(kFieldValues[0]))];
            storeBigEndian(trailer + 8 + 8 * ((mutation >> 16) % 3), value, 8);
            break;
        }
        case 10: {
            uint64_t index = nextRandom(state) % numObjects;
            uint64_t value = offsetTableOffset - 1 + (mutation >> 8) % 3;
            if ((mutation >> 20) & 1) {
                value = (mutation >> 24) % 8;
            }
            storeBigEndian(buffer + offsetTableOffset + index * offsetIntSize, value, offsetIntSize);
            break;
        }
        case 11:
            length -= 1 + (mutation >> 8) % 4;
            break;
        case 12:
            length += 1 + (mutation >> 8) % 4;
            break;
        case 13:
            buffer[(mutation >> 8) % 8] = (uint8_t)nextRandom(state);
            break;
        case 14:
            storeBigEndian(trailer + 16, numObjects - 1 + (mutation >> 8) % 2, 8);
            break;
        default:
            length = 0x20 + (mutation >> 8) % 16;
            break;
    }
    if (length > kBufferSize) {
        length = kBufferSize;
    }
    return length;
}

static int compareCase(
    TopLevelInfoFunction native,
    TopLevelInfoFunction replacement,
    const uint8_t *data,
    uint64_t length,
    uint32_t outputs
) {
    uint8_t markers[2] = {0x5a, 0x5a};
    uint64_t offsets[2] = {UINT64_C(0xa5a5a5a5a5a5a5a5), UINT64_C(0xa5a5a5a5a5a5a5a5)};
    BinaryPlistTrailer trailers[2];
    memset(trailers, 0xa5, sizeof(trailers));
    bool expected = native(
        data,
        length,
        (outputs & 1) ? &markers[0] : NULL,
        (outputs & 2) ? &offsets[0] : NULL,
        (outputs & 4) ? &trailers[0] : NULL
    );
    bool actual = replacement(
        data,
        length,
        (outputs & 1) ? &markers[1] : NULL,
        (outputs & 2) ? &offsets[1] : NULL,
        (outputs & 4) ? &trailers[1] : NULL
    );
    if (expected != actual || markers[0] != markers[1] || offsets[0] != offsets[1] ||
        memcmp(&trailers[0], &trailers[1], sizeof(BinaryPlistTrailer)) != 0) {
        fprintf(
            stderr,
            "__CFBinaryPlistGetTopLevelInfo mismatch: length %llu, outputs %u, result %d/%d.\n",
            (unsigned long long)length,
            outputs,
            expected,
            actual
        );
        return 1;
    }
    if (expected) {
        ++acceptedCases;
    }
    return 0;
}

// Native __CFBinaryPlistGetTopLevelInfo 0x1c0163b94 (external), iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. iOS Device 24A446: 0x18061ce78,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// The result, the marker byte, the offset and all 32 trailer bytes (prefilled) are compared for each
// combination of null and nonnull outputs. Inputs are random well-formed lists with entry widths
// 1..9, 12 and 16 and arbitrary reference widths, and their perturbations: a flipped byte, header bytes,
// trailer widths and 64-bit fields (including bit 63, 0, 8 and 9), offset-table entries at and past the
// object-area end or below 8, lengths off by one to four and lengths below the 41-byte minimum. A null
// buffer is checked with a nonzero length. Every native read stays inside the buffer once its checks
// pass (the table and the top object's byte lie before the trailer).
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationTopLevelInfoDifferentialTests(void) {
    void *nativeFunction = NULL;
    void *replacementFunction = NULL;
    int status = resolveReferenceFunction(
        "CoreFoundation",
        OF_REFERENCE_CF_UUID,
        OF_NAME_STRING(__CFBinaryPlistGetTopLevelInfo),
        &nativeFunction
    );
    if (status == 0) {
        status = resolveReplacementFunction(
            "OpenCoreFoundation", OF_NAME_STRING(__OCFBinaryPlistGetTopLevelInfo), &replacementFunction
        );
    }
    if (status == 0 && nativeFunction == replacementFunction) {
        fputs("Setup failure: top-level-info reference resolved to the replacement.\n", stderr);
        status = 77;
    }
    if (status != 0) {
        return status;
    }
    TopLevelInfoFunction native = (TopLevelInfoFunction)nativeFunction;
    TopLevelInfoFunction replacement = (TopLevelInfoFunction)replacementFunction;
    uint8_t *buffer = calloc(1, kBufferSize);
    if (buffer == NULL) {
        fputs("Setup failure: top-level-info buffer allocation failed.\n", stderr);
        return 77;
    }
    size_t cases = 0;
    acceptedCases = 0;
    for (uint32_t outputs = 0; outputs < 8; ++outputs) {
        if (compareCase(native, replacement, NULL, 100, outputs) != 0) {
            free(buffer);
            return 1;
        }
        ++cases;
    }
    uint64_t state = UINT64_C(0xb9115700f0c0ffee);
    for (uint32_t round = 0; round < 400000; ++round) {
        uint64_t length = buildCandidate(buffer, &state);
        for (uint32_t outputs = 0; outputs < 8; outputs += (round & 1) ? 7 : 1) {
            if (compareCase(native, replacement, buffer, length, outputs) != 0) {
                free(buffer);
                return 1;
            }
            ++cases;
        }
    }
    free(buffer);
    printf(
        "Binary-plist top-level-info differential cases passed: %zu, %zu accepted (one function; finite "
        "coverage).\n",
        cases,
        acceptedCases
    );
    return 0;
}

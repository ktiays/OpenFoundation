//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "KernelTests.h"

#include "CoreFoundation/StringEncoding/KernelCases.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <dlfcn.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t (*ToByte)(uint32_t flags, uint16_t character, uint8_t *byte);

typedef uint8_t (*FromByte)(uint32_t flags, uint8_t byte, uint16_t *character);

typedef OCFIndex (*ToUTF8)(
    uint32_t flags,
    const uint16_t *characters,
    OCFIndex count,
    uint8_t *bytes,
    OCFIndex capacity,
    OCFIndex *usedBytes
);

typedef OCFIndex (*UTF8Length)(uint32_t flags, const uint16_t *characters, OCFIndex count);

typedef struct {
    const char *name;
    uintptr_t nativeTo;
    uintptr_t nativeFrom;
    ToByte replacementTo;
    FromByte replacementFrom;
} SingleByteKernel;

// All native addresses below are read IDA function boundaries in iOS Simulator
// CoreFoundation 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Independent iOS Device comparisons and table provenance:
// Documentation/Reference/Research/CoreFoundationStringEncoding.md.
static const SingleByteKernel kSingleByteKernels[] = {
    {"ASCII", UINT64_C(0x1c01689f4), UINT64_C(0x1c0168a0c), __CFToASCII, __CFFromASCII},
    {"ISOLatin1", UINT64_C(0x1c0168a20), UINT64_C(0x1c0168a38), __CFToISOLatin1, __CFFromISOLatin1},
    {"WinLatin1", UINT64_C(0x1c0168c80), UINT64_C(0x1c0168d04), __CFToWinLatin1, __CFFromWinLatin1},
    {"NextStepLatin",
     UINT64_C(0x1c0168e4c),
     UINT64_C(0x1c0168ebc),
     __CFToNextStepLatin,
     __CFFromNextStepLatin},
    {"MacRomanLatin1",
     UINT64_C(0x1c019fbc0),
     UINT64_C(0x1c019fc30),
     __CFToMacRomanLatin1,
     __CFFromMacRomanLatin1},
    {"DOSLatin1", UINT64_C(0x1c019fe78), UINT64_C(0x1c019fef0), __CFToDOSLatin1, __CFFromDOSLatin1},
};

static int checkKernelOwner(void *function) {
    Dl_info actual;
    Dl_info expected;
    if (!dladdr(function, &actual) || !dladdr((void *)runStringEncodingKernelDifferentialTests, &expected) ||
        actual.dli_fbase != expected.dli_fbase) {
        fprintf(stderr, "Setup failure: replacement encoding kernel is not in the current test image.\n");
        return 77;
    }
    return 0;
}

static int resolveKernel(uintptr_t address, void **function) {
    return resolveReferenceAddress(
        "CoreFoundation", OF_REFERENCE_CF_UUID, address, UINT64_C(0x1c014a000), function
    );
}

static int compareSingleByteKernels(size_t *comparisons) {
    const uint32_t flags[] = {0, 1, 0x80, 0xc0, 0x800, 0x10000, 0xff000000, UINT32_MAX};
    for (size_t kernel = 0; kernel < sizeof(kSingleByteKernels) / sizeof(kSingleByteKernels[0]); ++kernel) {
        const SingleByteKernel *test = &kSingleByteKernels[kernel];
        void *nativeToAddress = NULL;
        void *nativeFromAddress = NULL;
        int status = resolveKernel(test->nativeTo, &nativeToAddress);
        if (status == 0) {
            status = resolveKernel(test->nativeFrom, &nativeFromAddress);
        }
        if (status == 0) {
            status = checkKernelOwner((void *)test->replacementTo);
        }
        if (status == 0) {
            status = checkKernelOwner((void *)test->replacementFrom);
        }
        if (status != 0) {
            return status;
        }
        if (nativeToAddress == (void *)test->replacementTo ||
            nativeFromAddress == (void *)test->replacementFrom) {
            fprintf(stderr, "Setup failure: a single-byte encoding oracle equals its replacement.\n");
            return 77;
        }
        ToByte nativeTo = (ToByte)nativeToAddress;
        FromByte nativeFrom = (FromByte)nativeFromAddress;
        for (size_t flag = 0; flag < sizeof(flags) / sizeof(flags[0]); ++flag) {
            for (uint32_t character = 0; character <= UINT16_MAX; ++character) {
                uint8_t nativeBytes[5] = {0xa5, 0xa5, 0xa5, 0xa5, 0xa5};
                uint8_t replacementBytes[5] = {0xa5, 0xa5, 0xa5, 0xa5, 0xa5};
                uint8_t expected = nativeTo(flags[flag], (uint16_t)character, nativeBytes + 2);
                uint8_t actual = test->replacementTo(flags[flag], (uint16_t)character, replacementBytes + 2);
                if (expected != actual || memcmp(nativeBytes, replacementBytes, sizeof(nativeBytes)) != 0) {
                    fprintf(
                        stderr,
                        "%s To-byte mismatch: U+%04" PRIx32 ", flags %08" PRIx32 ".\n",
                        test->name,
                        character,
                        flags[flag]
                    );
                    return 1;
                }
                ++*comparisons;
            }
            for (uint32_t byte = 0; byte <= UINT8_MAX; ++byte) {
                uint16_t nativeCharacters[3] = {0xa55a, 0xa55a, 0xa55a};
                uint16_t replacementCharacters[3] = {0xa55a, 0xa55a, 0xa55a};
                uint8_t expected = nativeFrom(flags[flag], (uint8_t)byte, nativeCharacters + 1);
                uint8_t actual = test->replacementFrom(flags[flag], (uint8_t)byte, replacementCharacters + 1);
                if (expected != actual ||
                    memcmp(nativeCharacters, replacementCharacters, sizeof(nativeCharacters)) != 0) {
                    fprintf(
                        stderr,
                        "%s From-byte mismatch: %02" PRIx32 ", flags %08" PRIx32 ".\n",
                        test->name,
                        byte,
                        flags[flag]
                    );
                    return 1;
                }
                ++*comparisons;
            }
        }
    }
    return 0;
}

static int compareUTF8(
    ToUTF8 native,
    UTF8Length nativeLength,
    const uint16_t *input,
    OCFIndex count,
    uint32_t flags,
    OCFIndex capacity,
    int withUsed,
    size_t *comparisons
) {
    uint16_t nativeInput[16];
    uint16_t replacementInput[16];
    memcpy(nativeInput, input, (size_t)count * sizeof(*input));
    memcpy(replacementInput, input, (size_t)count * sizeof(*input));
    uint8_t nativeBytes[64];
    uint8_t replacementBytes[64];
    memset(nativeBytes, 0xa5, sizeof(nativeBytes));
    memset(replacementBytes, 0xa5, sizeof(replacementBytes));
    OCFIndex nativeUsed = -123456789;
    OCFIndex replacementUsed = -123456789;
    OCFIndex expected =
        native(flags, nativeInput, count, nativeBytes + 8, capacity, withUsed ? &nativeUsed : NULL);
    OCFIndex actual = __CFToUTF8(
        flags, replacementInput, count, replacementBytes + 8, capacity, withUsed ? &replacementUsed : NULL
    );
    if (expected != actual || nativeUsed != replacementUsed ||
        memcmp(nativeBytes, replacementBytes, sizeof(nativeBytes)) != 0 ||
        memcmp(nativeInput, input, (size_t)count * sizeof(*input)) != 0 ||
        memcmp(replacementInput, input, (size_t)count * sizeof(*input)) != 0) {
        fprintf(
            stderr,
            "UTF8 encoder mismatch at case %zu, count %ld, flags %08" PRIx32
            ", capacity %ld: consumed %ld/%ld, used %ld/%ld.\n",
            *comparisons,
            count,
            flags,
            capacity,
            expected,
            actual,
            nativeUsed,
            replacementUsed
        );
        return 1;
    }
    ++*comparisons;
    OCFIndex expectedLength = nativeLength(flags, nativeInput, count);
    OCFIndex actualLength = __CFToUTF8Len(flags, replacementInput, count);
    if (expectedLength != actualLength) {
        fprintf(
            stderr, "UTF8 length mismatch at case %zu: %ld/%ld.\n", *comparisons, expectedLength, actualLength
        );
        return 1;
    }
    ++*comparisons;
    return 0;
}

static uint64_t nextEncodingBits(uint64_t *state) {
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return *state;
}

static int compareUTF8Kernels(size_t *comparisons) {
    void *nativeAddress = NULL;
    void *nativeLengthAddress = NULL;
    int status = resolveKernel(UINT64_C(0x1c0168fe4), &nativeAddress);
    if (status == 0) {
        status = resolveKernel(UINT64_C(0x1c0169564), &nativeLengthAddress);
    }
    if (status == 0) {
        status = checkKernelOwner((void *)__CFToUTF8);
    }
    if (status == 0) {
        status = checkKernelOwner((void *)__CFToUTF8Len);
    }
    if (status != 0) {
        return status;
    }
    if (nativeAddress == (void *)__CFToUTF8 || nativeLengthAddress == (void *)__CFToUTF8Len) {
        fprintf(stderr, "Setup failure: UTF8 oracle equals its replacement.\n");
        return 77;
    }
    ToUTF8 native = (ToUTF8)nativeAddress;
    UTF8Length nativeLength = (UTF8Length)nativeLengthAddress;
    for (size_t index = 0; index < sizeof(kUTF8KernelCases) / sizeof(kUTF8KernelCases[0]); ++index) {
        const UTF8KernelCase *test = &kUTF8KernelCases[index];
        status = compareUTF8(
            native, nativeLength, test->input, test->count, test->flags, test->capacity, 1, comparisons
        );
        if (status != 0) {
            return status;
        }
    }
    const uint32_t flags[] = {0, 1, 0x80, 0xc0, 0x800, UINT32_MAX};
    const OCFIndex capacities[] = {0, 1, 2, 3, 4, 5};
    for (uint32_t character = 0; character <= UINT16_MAX; ++character) {
        uint16_t input[] = {(uint16_t)character};
        for (size_t flag = 0; flag < sizeof(flags) / sizeof(flags[0]); ++flag) {
            for (size_t capacity = 0; capacity < sizeof(capacities) / sizeof(capacities[0]); ++capacity) {
                status = compareUTF8(
                    native,
                    nativeLength,
                    input,
                    1,
                    flags[flag],
                    capacities[capacity],
                    (int)(capacity & 1),
                    comparisons
                );
                if (status != 0) {
                    return status;
                }
            }
        }
    }
    const uint16_t boundaries[] = {
        0, 0x41, 0x7f, 0x80, 0x7ff, 0x800, 0xd7ff, 0xd800, 0xdbff, 0xdc00, 0xdfff, 0xe000, 0xffff
    };
    for (size_t first = 0; first < sizeof(boundaries) / sizeof(boundaries[0]); ++first) {
        for (size_t second = 0; second < sizeof(boundaries) / sizeof(boundaries[0]); ++second) {
            uint16_t input[] = {0x41, boundaries[first], boundaries[second], 0x20ac};
            for (size_t flag = 0; flag < sizeof(flags) / sizeof(flags[0]); ++flag) {
                for (OCFIndex capacity = 0; capacity <= 12; ++capacity) {
                    status =
                        compareUTF8(native, nativeLength, input, 4, flags[flag], capacity, 1, comparisons);
                    if (status != 0) {
                        return status;
                    }
                }
            }
        }
    }
    uint64_t state = UINT64_C(0x869f44ecb372ab15);
    for (unsigned index = 0; index < 4096; ++index) {
        uint16_t input[8];
        for (size_t unit = 0; unit < 8; ++unit) {
            input[unit] = (uint16_t)nextEncodingBits(&state);
        }
        uint32_t selectedFlags = (uint32_t)nextEncodingBits(&state);
        OCFIndex count = (OCFIndex)(nextEncodingBits(&state) % 9);
        OCFIndex capacity = (OCFIndex)(nextEncodingBits(&state) % 25);
        status = compareUTF8(
            native, nativeLength, input, count, selectedFlags, capacity, (int)(index & 1), comparisons
        );
        if (status != 0) {
            return status;
        }
    }
    return 0;
}

int runStringEncodingKernelDifferentialTests(void) {
    size_t comparisons = 0;
    int status = compareSingleByteKernels(&comparisons);
    if (status == 0) {
        status = compareUTF8Kernels(&comparisons);
    }
    if (status == 0) {
        printf("String encoding kernel differential comparisons passed: %zu.\n", comparisons);
    }
    return status;
}

//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Locale/LocaleIdentifierTests.h"
#include "CoreFoundation/Locale/OCFLocaleIdentifier.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef int (*EntryComparator)(const void *testEntry, const void *tableEntry);

typedef void (*UpdateFullLocaleString)(char *, char **, char **, char *);

typedef void (*RemoveSubstrings)(char *, const char *);

typedef void (*AppendKeyValue)(char *, const char *);

typedef bool (*CheckForTag)(const char *, const char *);

enum {
    kGuard = 16,
    kCapacity = 0x101,
    kBufferSize = kGuard + kCapacity + kGuard,
    kPrefixCount = 0x130,
    kMaximumCases = 64,
    kMaximumTestKeys = 8192,
};

static const unsigned char kCanary = 0x5a;

static uint64_t nextBits(uint64_t *state) {
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return *state;
}

static int resolvePair(uintptr_t address, void *replacement, void **native) {
    int status = resolveReferenceAddress(
        "CoreFoundation", OF_REFERENCE_CF_UUID, address, UINT64_C(0x1c014a000), native
    );
    if (status == 0) {
        status = checkInternalImplementation(replacement);
    }
    if (status == 0 && *native == replacement) {
        fputs("Setup failure: a locale-identifier oracle resolved to its replacement.\n", stderr);
        status = 77;
    }
    return status;
}

// Decodes the ADRP/ADD pair (ADRL) at a native instruction address, so the native tables are read from
// the pinned image itself rather than from a copy of the restoration's tables.
static int resolveNativeTable(uintptr_t instructionAddress, unsigned int reg, const void **table) {
    void *pointer = NULL;
    int status = resolveReferenceAddress(
        "CoreFoundation", OF_REFERENCE_CF_UUID, instructionAddress, UINT64_C(0x1c014a000), &pointer
    );
    if (status != 0) {
        return status;
    }
    uint32_t adrp;
    uint32_t add;
    memcpy(&adrp, pointer, 4);
    memcpy(&add, (const char *)pointer + 4, 4);
    if ((adrp & 0x9f000000) != 0x90000000 || (adrp & 0x1f) != reg || (add & 0xffc00000) != 0x91000000 ||
        (add & 0x1f) != reg || ((add >> 5) & 0x1f) != reg) {
        fputs("Setup failure: unexpected native table reference instructions.\n", stderr);
        return 77;
    }
    int64_t immediate = (int64_t)((((adrp >> 5) & 0x7ffff) << 2) | ((adrp >> 29) & 3));
    if (immediate & (INT64_C(1) << 20)) {
        immediate -= INT64_C(1) << 21;
    }
    uintptr_t page = ((uintptr_t)pointer & ~(uintptr_t)0xfff) + (uintptr_t)(immediate << 12);
    *table = (const void *)(page + ((add >> 10) & 0xfff));
    return 0;
}

static void fillBuffer(unsigned char *buffer, const char *text, unsigned char before) {
    memset(buffer, kCanary, kBufferSize);
    buffer[kGuard - 1] = before;
    size_t length = strlen(text);
    if (length > kCapacity - 1) {
        length = kCapacity - 1;
    }
    memcpy(buffer + kGuard, text, length);
    buffer[kGuard + length] = 0;
}

static long pointerOffset(const char *pointer, const unsigned char *buffer) {
    return pointer == NULL ? -1 : (long)(pointer - (const char *)buffer);
}

typedef struct {
    UpdateFullLocaleString native;
    size_t cases;
} UpdateContext;

static int compareUpdate(UpdateContext *context, const char *text, unsigned char before) {
    static unsigned char expectedLocale[kBufferSize];
    static unsigned char actualLocale[kBufferSize];
    static unsigned char expectedKeys[kBufferSize];
    static unsigned char actualKeys[kBufferSize];
    fillBuffer(expectedLocale, text, before);
    fillBuffer(actualLocale, text, before);
    fillBuffer(expectedKeys, "", kCanary);
    fillBuffer(actualKeys, "", kCanary);
    char *expectedHyphen = (char *)expectedLocale;
    char *expectedUnderscore = (char *)expectedLocale;
    char *actualHyphen = (char *)actualLocale;
    char *actualUnderscore = (char *)actualLocale;
    context->native(
        (char *)expectedLocale + kGuard, &expectedHyphen, &expectedUnderscore, (char *)expectedKeys + kGuard
    );
    _UpdateFullLocaleString(
        (char *)actualLocale + kGuard, &actualHyphen, &actualUnderscore, (char *)actualKeys + kGuard
    );
    if (memcmp(expectedLocale, actualLocale, kBufferSize) != 0 ||
        memcmp(expectedKeys, actualKeys, kBufferSize) != 0 ||
        pointerOffset(expectedHyphen, expectedLocale) != pointerOffset(actualHyphen, actualLocale) ||
        pointerOffset(expectedUnderscore, expectedLocale) != pointerOffset(actualUnderscore, actualLocale)) {
        fprintf(
            stderr,
            "_UpdateFullLocaleString mismatch: \"%s\" -> native \"%s\" \"%s\", replacement \"%s\" \"%s\".\n",
            text,
            (char *)expectedLocale + kGuard,
            (char *)expectedKeys + kGuard,
            (char *)actualLocale + kGuard,
            (char *)actualKeys + kGuard
        );
        return 1;
    }
    ++context->cases;
    return 0;
}

static const char *const kRegionPieces[] = {
    "UK",   "uk",   "Uk",   "TP",   "tp",    "CS",    "cs",  "YU",    "yu",     "HR",
    "hr",   "RS",   "rs",   "US",   "us",    "GB",    "419", "001",   "1",      "12",
    "Latn", "latn", "Cyrl", "Hant", "POSIX", "posix", "x",   "abcde", "abcdef", "",
};

static const char *const kSeparators[] = {"_", "-", "__", "_-", "-_", "--", "@", ".", " ", ";", "="};

static const char *const kKeywordTails[] = {
    "",
    "@calendar=japanese",
    "@collation=phonebook;currency=EUR",
    "@a=b",
    "@calendar",
    "@",
    "@=x",
    "@numbers=arab;calendar=islamic",
};

static void appendText(char *buffer, size_t capacity, const char *text) {
    size_t length = strlen(buffer);
    size_t extra = strlen(text);
    if (length + extra >= capacity) {
        extra = capacity - 1 - length;
    }
    memcpy(buffer + length, text, extra);
    buffer[length + extra] = 0;
}

static void
randomCase(char *output, size_t capacity, uint64_t *state, const char *const *keys, size_t keyCount) {
    static const char kLetters[] = "aAbBcCdDeEfFhHiIkKlLmMnNoOrRsStTuUxXyYzZ0123456789_-@=;. ";
    output[0] = 0;
    unsigned int pieces = 1 + (unsigned int)(nextBits(state) % 6);
    for (unsigned int piece = 0; piece < pieces; ++piece) {
        uint64_t choice = nextBits(state) % 10;
        if (choice < 3) {
            appendText(output, capacity, keys[nextBits(state) % keyCount]);
        } else if (choice < 6) {
            appendText(
                output, capacity, kSeparators[nextBits(state) % (sizeof(kSeparators) / sizeof(*kSeparators))]
            );
            appendText(
                output,
                capacity,
                kRegionPieces[nextBits(state) % (sizeof(kRegionPieces) / sizeof(*kRegionPieces))]
            );
        } else if (choice < 7) {
            appendText(
                output,
                capacity,
                kKeywordTails[nextBits(state) % (sizeof(kKeywordTails) / sizeof(*kKeywordTails))]
            );
        } else {
            unsigned int count = 1 + (unsigned int)(nextBits(state) % 4);
            for (unsigned int index = 0; index < count; ++index) {
                char letter[2] = {kLetters[nextBits(state) % (sizeof(kLetters) - 1)], 0};
                appendText(output, capacity, letter);
            }
        }
    }
}

static void upperCopy(char *output, const char *text, size_t capacity) {
    size_t index = 0;
    for (; text[index] != 0 && index + 1 < capacity; ++index) {
        char character = text[index];
        output[index] = (character >= 'a' && character <= 'z') ? (char)(character - 'a' + 'A') : character;
    }
    output[index] = 0;
}

static int compareUpdates(
    UpdateFullLocaleString native,
    const LocaleStringMapping *table,
    const LocaleSpecialCase *special,
    size_t *cases
) {
    UpdateContext context = {native, 0};
    static const char *const kSuffixes[] = {
        "",
        "_US",
        "-us",
        "_us_POSIX",
        "-Latn",
        "-latn-rs",
        "_419",
        "-419",
        "_CS",
        "-UK",
        "@calendar=japanese",
        "_US@currency=EUR",
        "_us__POSIX@collation=phonebook",
        "x",
        "1",
        "_",
        "-",
        "--",
        ".utf8",
    };
    const char *keys[kPrefixCount];
    for (size_t index = 0; index < kPrefixCount; ++index) {
        keys[index] = table[index].key;
    }
    char text[kCapacity + 64];
    char upper[kCapacity + 64];
    for (size_t index = 0; index < kPrefixCount; ++index) {
        for (size_t suffix = 0; suffix < sizeof(kSuffixes) / sizeof(*kSuffixes); ++suffix) {
            snprintf(text, sizeof(text), "%s%s", table[index].key, kSuffixes[suffix]);
            if (compareUpdate(&context, text, kCanary) != 0) {
                return 1;
            }
            upperCopy(upper, text, sizeof(upper));
            if (compareUpdate(&context, upper, kCanary) != 0) {
                return 1;
            }
            snprintf(text, sizeof(text), "%s%s", table[index].value, kSuffixes[suffix]);
            if (compareUpdate(&context, text, kCanary) != 0) {
                return 1;
            }
        }
        // A key whose next byte is alphanumeric does not match; a shortened key never matches.
        size_t length = strlen(table[index].key);
        snprintf(text, sizeof(text), "%.*s", (int)(length - 1), table[index].key);
        if (compareUpdate(&context, text, kCanary) != 0) {
            return 1;
        }
    }
    // Every special case with and without its language tag, region spellings and separators.
    static const char *const kLanguages[] = {"", "en", "cs", "sk", "sh", "sr", "hr", "CS", "SH", "csx", "e"};
    for (size_t record = 0; special[record].regionTag != NULL; ++record) {
        const char *tags[2] = {special[record].regionTag, special[record].alternateRegionTag};
        for (size_t tag = 0; tag < 2; ++tag) {
            if (tags[tag] == NULL) {
                continue;
            }
            for (size_t language = 0; language < sizeof(kLanguages) / sizeof(*kLanguages); ++language) {
                static const char *const kForms[] = {
                    "%s%s", "%s_%s", "%s%sx", "%s%s1", "%s%s_POSIX", "%s%s-x", "%s%s@a=b", "%s-Latn%s"
                };
                for (size_t form = 0; form < sizeof(kForms) / sizeof(*kForms); ++form) {
                    const char *region = form == 1 ? tags[tag] + 1 : tags[tag];
                    snprintf(text, sizeof(text), kForms[form], kLanguages[language], region);
                    if (compareUpdate(&context, text, kCanary) != 0) {
                        return 1;
                    }
                    upperCopy(upper, text, sizeof(upper));
                    if (compareUpdate(&context, upper, kCanary) != 0) {
                        return 1;
                    }
                }
            }
        }
    }
    // Lengths around the 256-byte growth limit for the two growing records ("hbs", "tl").
    for (size_t padding = 240; padding < kCapacity - 1; ++padding) {
        static const char *const kGrowing[] = {"hbs", "tl", "HBS", "Tl", "art-lojban"};
        for (size_t grow = 0; grow < sizeof(kGrowing) / sizeof(*kGrowing); ++grow) {
            size_t length = strlen(kGrowing[grow]);
            if (padding <= length) {
                continue;
            }
            memset(text, 'a', padding);
            memcpy(text, kGrowing[grow], length);
            text[length] = '_';
            text[padding] = 0;
            if (compareUpdate(&context, text, kCanary) != 0) {
                return 1;
            }
        }
    }
    // Leading underscores: the byte before the buffer is not '_', so the native unbounded walk stops there.
    static const char *const kLeading[] = {
        "__X@a=b", "_X@a=b", "___x_y@calendar=japanese", "__x_POSIX@a=b", "a__B@c=d", "__", "_", "@", ""
    };
    for (size_t index = 0; index < sizeof(kLeading) / sizeof(*kLeading); ++index) {
        if (compareUpdate(&context, kLeading[index], kCanary) != 0 ||
            compareUpdate(&context, kLeading[index], 0) != 0) {
            return 1;
        }
    }
    uint64_t state = UINT64_C(0x9e3779b97f4a7c15);
    for (size_t index = 0; index < 400000; ++index) {
        randomCase(text, 200, &state, keys, kPrefixCount);
        if (compareUpdate(&context, text, kCanary) != 0) {
            return 1;
        }
    }
    *cases += context.cases;
    return 0;
}

// Establishes the native unbounded '_' walk of _UpdateFullLocaleString on bytes this test owns: with '_'
// before the buffer, the native function clears it; the replacement stops at the buffer.
static int probeUnderscoreWalk(UpdateFullLocaleString native, size_t *cases) {
    static unsigned char expected[kBufferSize];
    static unsigned char actual[kBufferSize];
    unsigned char keysExpected[kBufferSize];
    unsigned char keysActual[kBufferSize];
    fillBuffer(expected, "__X@a=b", '_');
    fillBuffer(actual, "__X@a=b", '_');
    fillBuffer(keysExpected, "", kCanary);
    fillBuffer(keysActual, "", kCanary);
    char *hyphen = NULL;
    char *underscore = NULL;
    native((char *)expected + kGuard, &hyphen, &underscore, (char *)keysExpected + kGuard);
    _UpdateFullLocaleString((char *)actual + kGuard, &hyphen, &underscore, (char *)keysActual + kGuard);
    if (keysExpected[kGuard] != '@') {
        fputs("Setup failure: the canonicalized probe identifier has no key/value string.\n", stderr);
        return 77;
    }
    if (expected[kGuard - 1] != 0 || expected[kGuard - 2] != kCanary || actual[kGuard - 1] != '_' ||
        memcmp(expected + kGuard, actual + kGuard, kBufferSize - kGuard) != 0 ||
        memcmp(keysExpected, keysActual, kBufferSize) != 0) {
        fputs("_UpdateFullLocaleString underscore-walk probe mismatch.\n", stderr);
        return 1;
    }
    ++*cases;
    return 0;
}

static size_t comparatorKeys(const LocaleStringMapping *table, char storage[][40], const char **keys) {
    static const char *const kSuffixes[] = {"", "_", "-", "x", "1", "_US", "-us", "@", " ", "\xe9", "\x80"};
    size_t count = 0;
    for (size_t index = 0; index < kPrefixCount; ++index) {
        for (size_t suffix = 0; suffix < sizeof(kSuffixes) / sizeof(*kSuffixes); ++suffix) {
            snprintf(storage[count], 40, "%s%s", table[index].key, kSuffixes[suffix]);
            keys[count] = storage[count];
            ++count;
            upperCopy(storage[count], storage[count - 1], 40);
            keys[count] = storage[count];
            ++count;
        }
        size_t length = strlen(table[index].key);
        snprintf(storage[count], 40, "%.*s", (int)(length - 1), table[index].key);
        keys[count] = storage[count];
        ++count;
    }
    static const char kBytes[] = "aZ_-0@\xe9\xff\x80 ~";
    uint64_t state = UINT64_C(0x243f6a8885a308d3);
    while (count < kMaximumTestKeys) {
        size_t length = nextBits(&state) % 6;
        for (size_t index = 0; index < length; ++index) {
            storage[count][index] = kBytes[nextBits(&state) % (sizeof(kBytes) - 1)];
        }
        storage[count][length] = 0;
        keys[count] = storage[count];
        ++count;
    }
    return count;
}

static int compareComparators(const LocaleStringMapping *table, size_t *cases) {
    void *native[3];
    void *replacement[3] = {
        (void *)_CompareTestEntryToTableEntryKey,
        (void *)_CompareTestEntryPrefixToTableEntryKey,
        (void *)_CompareLowerTestEntryPrefixToTableEntryKey,
    };
    const uintptr_t addresses[] = {UINT64_C(0x1c01a0324), UINT64_C(0x1c01a0888), UINT64_C(0x1c01a1810)};
    static const char *const names[] = {
        "_CompareTestEntryToTableEntryKey",
        "_CompareTestEntryPrefixToTableEntryKey",
        "_CompareLowerTestEntryPrefixToTableEntryKey",
    };
    for (size_t index = 0; index < 3; ++index) {
        int status = resolvePair(addresses[index], replacement[index], &native[index]);
        if (status != 0) {
            return status;
        }
    }
    static char storage[kMaximumTestKeys][40];
    static const char *keys[kMaximumTestKeys];
    size_t count = comparatorKeys(table, storage, keys);
    // Table side: every native key, then the first 600 generated keys.
    for (size_t side = 0; side < kPrefixCount + 600; ++side) {
        LocaleStringMapping tableEntry = {
            side < kPrefixCount ? table[side].key : keys[side - kPrefixCount], NULL
        };
        for (size_t index = 0; index < count; ++index) {
            LocaleStringMapping testEntry = {keys[index], NULL};
            for (size_t function = 0; function < 3; ++function) {
                int expected = ((EntryComparator)native[function])(&testEntry, &tableEntry);
                int actual = ((EntryComparator)replacement[function])(&testEntry, &tableEntry);
                if (expected != actual) {
                    fprintf(
                        stderr,
                        "%s mismatch: \"%s\" vs \"%s\": %d, %d.\n",
                        names[function],
                        testEntry.key,
                        tableEntry.key,
                        expected,
                        actual
                    );
                    return 1;
                }
                ++*cases;
            }
        }
    }
    return 0;
}

static int compareCheckForTag(size_t *cases) {
    void *native = NULL;
    int status = resolvePair(UINT64_C(0x1c01a18fc), (void *)_CheckForTag, &native);
    if (status != 0) {
        return status;
    }
    static const char kBytes[] = {
        0, 'a', 'e', 'n', 'U', 'K', 'z', '-', '_', '0', '9', ' ', '@', '\xe9', '\x80', '\xff'
    };
    const size_t count = sizeof(kBytes);
    for (size_t first = 0; first < count; ++first) {
        for (size_t second = 0; second < count; ++second) {
            for (size_t third = 0; third < count; ++third) {
                const char text[4] = {kBytes[first], kBytes[second], kBytes[third], 0};
                for (size_t tagFirst = 0; tagFirst < count; ++tagFirst) {
                    for (size_t tagSecond = 0; tagSecond < count; ++tagSecond) {
                        const char tag[3] = {kBytes[tagFirst], kBytes[tagSecond], 0};
                        if (((CheckForTag)native)(text, tag) != _CheckForTag(text, tag)) {
                            fputs("_CheckForTag mismatch.\n", stderr);
                            return 1;
                        }
                        ++*cases;
                    }
                }
            }
        }
    }
    return 0;
}

static int compareRemoveSubstrings(const LocaleStringMapping *table, size_t *cases) {
    void *native = NULL;
    int status = resolvePair(UINT64_C(0x1c01a08e4), (void *)_RemoveSubstringsIfPresent, &native);
    if (status != 0) {
        return status;
    }
    static const char *const kLists[] = {
        "",
        " ",
        "_POSIX",
        "-Latn _POSIX",
        "\t-x\n",
        "b  c",
        "a",
        "zz",
        "_ -",
        "en_US en",
        "  -us ",
        "@calendar=",
        "=;",
        "x\vy\fz\r",
        "~!",
        "aaaa",
    };
    static const char kGraph[] = "abcAB_-@=;.~!0123456789";
    static const char kSpace[] = " \t\n\v\f\r";
    static unsigned char expected[kBufferSize];
    static unsigned char actual[kBufferSize];
    const char *keys[kPrefixCount];
    for (size_t index = 0; index < kPrefixCount; ++index) {
        keys[index] = table[index].key;
    }
    uint64_t state = UINT64_C(0x13198a2e03707344);
    char text[200];
    char list[200];
    for (size_t index = 0; index < 300000; ++index) {
        randomCase(text, 120, &state, keys, kPrefixCount);
        size_t fixed = sizeof(kLists) / sizeof(*kLists);
        uint64_t pick = nextBits(&state) % (fixed + 4);
        if (pick < fixed) {
            snprintf(list, sizeof(list), "%s", kLists[pick]);
        } else {
            size_t length = 0;
            unsigned int tokens = (unsigned int)(nextBits(&state) % 5);
            for (unsigned int token = 0; token < tokens && length < 150; ++token) {
                unsigned int spaces = (unsigned int)(nextBits(&state) % 3);
                for (unsigned int space = 0; space < spaces; ++space) {
                    list[length++] = kSpace[nextBits(&state) % (sizeof(kSpace) - 1)];
                }
                if (nextBits(&state) % 3 == 0) {
                    // A substring taken from the locale string, so removals happen.
                    size_t textLength = strlen(text);
                    if (textLength > 0) {
                        size_t start = nextBits(&state) % textLength;
                        size_t take = 1 + nextBits(&state) % 6;
                        for (size_t offset = 0; offset < take && text[start + offset] > ' ' &&
                                                text[start + offset] < 0x7f && length < 180;
                             ++offset) {
                            list[length++] = text[start + offset];
                        }
                    }
                } else {
                    unsigned int letters = 1 + (unsigned int)(nextBits(&state) % 5);
                    for (unsigned int letter = 0; letter < letters; ++letter) {
                        list[length++] = kGraph[nextBits(&state) % (sizeof(kGraph) - 1)];
                    }
                }
                list[length++] = ' ';
            }
            list[length] = 0;
        }
        fillBuffer(expected, text, kCanary);
        fillBuffer(actual, text, kCanary);
        ((RemoveSubstrings)native)((char *)expected + kGuard, list);
        _RemoveSubstringsIfPresent((char *)actual + kGuard, list);
        if (memcmp(expected, actual, kBufferSize) != 0) {
            fprintf(stderr, "_RemoveSubstringsIfPresent mismatch: \"%s\", \"%s\".\n", text, list);
            return 1;
        }
        ++*cases;
    }
    return 0;
}

static int compareAppendKeyValue(const LocaleStringMapping *table, size_t *cases) {
    void *native = NULL;
    int status = resolvePair(UINT64_C(0x1c01a0a14), (void *)_AppendKeyValueString, &native);
    if (status != 0) {
        return status;
    }
    static const char *const kLocales[] = {
        "",
        "en",
        "en_US",
        "de_DE@collation=phonebook",
        "ja_JP@calendar=gregorian",
        "zh-Hant",
        "x@",
        "@a=b",
    };
    static const char *const kKeys[] = {
        "",
        "@calendar=japanese",
        "@collation=phonebook;currency=EUR",
        "@a=b",
        "@",
        "@=",
        "@calendar",
        "calendar=japanese",
        "@calendar=japanese;calendar=buddhist",
        "@numbers=arab;;calendar=islamic",
        "@x="
        "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrst"
        "uvwxyzabcdefghij",
        "en_US@calendar=japanese",
    };
    static const char kKeyLetters[] = "abckmnz019";
    static const char kValueLetters[] = "abcdefXYZ0189-_";
    static unsigned char expected[kBufferSize];
    static unsigned char actual[kBufferSize];
    const char *keys[kPrefixCount];
    for (size_t index = 0; index < kPrefixCount; ++index) {
        keys[index] = table[index].key;
    }
    for (size_t locale = 0; locale < sizeof(kLocales) / sizeof(*kLocales); ++locale) {
        for (size_t key = 0; key < sizeof(kKeys) / sizeof(*kKeys); ++key) {
            fillBuffer(expected, kLocales[locale], kCanary);
            fillBuffer(actual, kLocales[locale], kCanary);
            ((AppendKeyValue)native)((char *)expected + kGuard, kKeys[key]);
            _AppendKeyValueString((char *)actual + kGuard, kKeys[key]);
            if (memcmp(expected, actual, kBufferSize) != 0) {
                fprintf(
                    stderr, "_AppendKeyValueString mismatch: \"%s\", \"%s\".\n", kLocales[locale], kKeys[key]
                );
                return 1;
            }
            ++*cases;
        }
    }
    uint64_t state = UINT64_C(0xa4093822299f31d0);
    char locale[200];
    char keyValue[200];
    for (size_t index = 0; index < 60000; ++index) {
        randomCase(locale, 80, &state, keys, kPrefixCount);
        size_t length = 0;
        if (nextBits(&state) % 8 != 0) {
            keyValue[length++] = '@';
        }
        // Values stay at most 40 bytes, far from the 96-byte native value buffer.
        unsigned int pairs = (unsigned int)(nextBits(&state) % 4);
        for (unsigned int pair = 0; pair < pairs; ++pair) {
            unsigned int keyLength = (unsigned int)(nextBits(&state) % 9);
            for (unsigned int letter = 0; letter < keyLength; ++letter) {
                keyValue[length++] = kKeyLetters[nextBits(&state) % (sizeof(kKeyLetters) - 1)];
            }
            if (nextBits(&state) % 6 != 0) {
                keyValue[length++] = '=';
            }
            unsigned int valueLength = (unsigned int)(nextBits(&state) % 41);
            for (unsigned int letter = 0; letter < valueLength; ++letter) {
                keyValue[length++] = kValueLetters[nextBits(&state) % (sizeof(kValueLetters) - 1)];
            }
            if (pair + 1 < pairs) {
                keyValue[length++] = ';';
            }
        }
        keyValue[length] = 0;
        fillBuffer(expected, locale, kCanary);
        fillBuffer(actual, locale, kCanary);
        ((AppendKeyValue)native)((char *)expected + kGuard, keyValue);
        _AppendKeyValueString((char *)actual + kGuard, keyValue);
        if (memcmp(expected, actual, kBufferSize) != 0) {
            fprintf(stderr, "_AppendKeyValueString mismatch: \"%s\", \"%s\".\n", locale, keyValue);
            return 1;
        }
        ++*cases;
    }
    return 0;
}

// Native hidden functions resolved by IDA offset: _CompareTestEntryToTableEntryKey 0x1c01a0324,
// _UpdateFullLocaleString 0x1c01a0330, _CompareTestEntryPrefixToTableEntryKey 0x1c01a0888,
// _RemoveSubstringsIfPresent 0x1c01a08e4, _AppendKeyValueString 0x1c01a0a14,
// _CompareLowerTestEntryPrefixToTableEntryKey 0x1c01a1810 and _CheckForTag 0x1c01a18fc; the native
// tables _localeStringPrefixToCanonical and _specialCases are read through the ADRL instructions at
// 0x1c01a0370 and 0x1c01a0678. iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. iOS Device 24A446 addresses and divergences: the definitions and
// Documentation/Reference/Research/CoreFoundationLocaleIdentifier.md.
int runCoreFoundationLocaleIdentifierDifferentialTests(void) {
    const void *prefixTable = NULL;
    const void *specialTable = NULL;
    int status = resolveNativeTable(UINT64_C(0x1c01a0370), 1, &prefixTable);
    if (status == 0) {
        status = resolveNativeTable(UINT64_C(0x1c01a0678), 19, &specialTable);
    }
    void *update = NULL;
    if (status == 0) {
        status = resolvePair(UINT64_C(0x1c01a0330), (void *)_UpdateFullLocaleString, &update);
    }
    if (status != 0) {
        return status;
    }
    const LocaleStringMapping *table = prefixTable;
    const LocaleSpecialCase *special = specialTable;
    size_t specialCount = 0;
    while (special[specialCount].regionTag != NULL && specialCount < kMaximumCases) {
        ++specialCount;
    }
    if (specialCount != 7 || strcmp(table[0].key, "aar") != 0 ||
        strcmp(table[kPrefixCount - 1].key, "zyb") != 0) {
        fputs("Setup failure: the native locale tables do not have the read shape.\n", stderr);
        return 77;
    }
    size_t cases = 0;
    status = compareComparators(table, &cases);
    if (status == 0) {
        status = compareCheckForTag(&cases);
    }
    if (status == 0) {
        status = compareUpdates((UpdateFullLocaleString)update, table, special, &cases);
    }
    if (status == 0) {
        status = probeUnderscoreWalk((UpdateFullLocaleString)update, &cases);
    }
    if (status == 0) {
        status = compareRemoveSubstrings(table, &cases);
    }
    if (status == 0) {
        status = compareAppendKeyValue(table, &cases);
    }
    if (status != 0) {
        return status;
    }
    printf(
        "Locale-identifier differential cases passed: %zu (seven functions over the native tables; finite "
        "coverage).\n",
        cases
    );
    return 0;
}

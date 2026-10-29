//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Locale/LocaleIdentifierStandaloneTests.h"
#include "CoreFoundation/Locale/OCFLocaleIdentifier.h"

#include <stdio.h>
#include <string.h>

static int fail(int line) {
    fprintf(stderr, "Locale-identifier standalone mismatch at source line %d.\n", line);
    return 1;
}

static int compare(const char *test, const char *table) {
    LocaleStringMapping testEntry = {test, NULL};
    LocaleStringMapping tableEntry = {table, NULL};
    return _CompareLowerTestEntryPrefixToTableEntryKey(&testEntry, &tableEntry);
}

static int comparePrefix(const char *test, const char *table) {
    LocaleStringMapping testEntry = {test, NULL};
    LocaleStringMapping tableEntry = {table, NULL};
    return _CompareTestEntryPrefixToTableEntryKey(&testEntry, &tableEntry);
}

typedef struct {
    const char *input;
    const char *locale;
    const char *keys;
    long hyphen;
    long underscore;
} UpdateCase;

// Expected values follow the reading of _UpdateFullLocaleString and its tables.
static const UpdateCase kUpdateCases[] = {
    {"EN_us", "en_US", "", -1, 2},
    {"en-uk", "en-GB", "", 2, -1},
    {"en_UK", "en_GB", "", -1, 2},
    {"zh-guoyu", "zh", "", -1, -1},
    {"ZH-GUOYU", "zh", "", -1, -1},
    {"tl", "fil", "", -1, -1},
    {"hbs", "sr-Latn", "", -1, -1},
    {"cs_CS", "cs_CZ", "", -1, 2},
    {"sk-cs", "sk-SK", "", 2, -1},
    {"de_CS", "de_RS", "", -1, 2},
    {"sh_HR", "hr_HR", "", -1, 2},
    {"sh-RS", "sr-RS", "", 2, -1},
    {"en-TP", "en-TL", "", 2, -1},
    {"en-419", "en-419", "", 2, -1},
    {"en_419", "en_419", "", -1, 2},
    {"en_us_posix", "en_US_POSIX", "", -1, 2},
    {"123", "123", "", -1, -1},
};

static int checkUpdate(const UpdateCase *test, int line) {
    char buffer[0x101];
    char keys[0x101];
    memset(buffer, 0x5a, sizeof(buffer));
    memset(keys, 0x5a, sizeof(keys));
    strcpy(buffer, test->input);
    char *hyphen = buffer;
    char *underscore = buffer;
    _UpdateFullLocaleString(buffer, &hyphen, &underscore, keys);
    long hyphenOffset = hyphen == NULL ? -1 : (long)(hyphen - buffer);
    long underscoreOffset = underscore == NULL ? -1 : (long)(underscore - buffer);
    if (strcmp(buffer, test->locale) != 0 || strcmp(keys, test->keys) != 0 || hyphenOffset != test->hyphen ||
        underscoreOffset != test->underscore) {
        fprintf(
            stderr,
            "\"%s\" -> \"%s\" \"%s\" %ld %ld.\n",
            test->input,
            buffer,
            keys,
            hyphenOffset,
            underscoreOffset
        );
        return fail(line);
    }
    return 0;
}

// Native _CompareTestEntryToTableEntryKey 0x1c01a0324, _UpdateFullLocaleString 0x1c01a0330,
// _CompareTestEntryPrefixToTableEntryKey 0x1c01a0888, _RemoveSubstringsIfPresent 0x1c01a08e4,
// _AppendKeyValueString 0x1c01a0a14, _CompareLowerTestEntryPrefixToTableEntryKey 0x1c01a1810,
// _CheckForTag 0x1c01a18fc. iOS Simulator 24A434 UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified. Device comparison: 0x1806a63f0, 0x1806a6a84, 0x1806a7388, 0x1806ca710, 0x1806a751c,
// 0x1806a6fbc, 0x1806a7118, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B, including the outlined blocks.
// Reading: Documentation/Reference/Research/CoreFoundationLocaleIdentifier.md.
int runCoreFoundationLocaleIdentifierStandaloneTests(void) {
    size_t cases = 0;
    if (!_CheckForTag("en-US", "en") || !_CheckForTag("en", "en") || _CheckForTag("eng", "en") ||
        _CheckForTag("de", "en") || !_CheckForTag("UK", "UK") || _CheckForTag("UK1", "UK")) {
        return fail(__LINE__);
    }
    cases += 6;
    if (compare("EN_us", "en") != 0 || compare("eng", "en") != 1 || compare("e_", "en") != -1 ||
        compare("a", "b") != -1 || compare("c", "b") != 1 || compare("en-", "en") != 0 ||
        compare("en", "en-gb") != -1 || compare("ART-LOJBAN", "art-lojban") != 0) {
        return fail(__LINE__);
    }
    cases += 8;
    if (comparePrefix("en_US", "en") != 0 || comparePrefix("de", "en") != -1 ||
        comparePrefix("fr", "en") != 1 || comparePrefix("e", "en") != -1 || comparePrefix("", "") != 0 ||
        comparePrefix("\xe9", "a") != -1) {
        return fail(__LINE__);
    }
    cases += 6;
    LocaleStringMapping left = {"abc", NULL};
    LocaleStringMapping right = {"abd", NULL};
    if (_CompareTestEntryToTableEntryKey(&left, &right) != strcmp("abc", "abd") ||
        _CompareTestEntryToTableEntryKey(&left, &left) != 0) {
        return fail(__LINE__);
    }
    cases += 2;
    for (size_t index = 0; index < sizeof(kUpdateCases) / sizeof(*kUpdateCases); ++index) {
        if (checkUpdate(&kUpdateCases[index], __LINE__) != 0) {
            return 1;
        }
        ++cases;
    }
    // CoreFoundation defect fix: the '_' walk stops at the first byte of the buffer.
    char guarded[1 + 0x101];
    char keys[0x101];
    memset(guarded, 0x5a, sizeof(guarded));
    guarded[0] = '_';
    strcpy(guarded + 1, "__X@a=b");
    char *hyphen = NULL;
    char *underscore = NULL;
    _UpdateFullLocaleString(guarded + 1, &hyphen, &underscore, keys);
    if (guarded[0] != '_' || guarded[1] != 0 || guarded[2] != 0 || keys[0] != '@') {
        return fail(__LINE__);
    }
    ++cases;
    char text[0x101];
    strcpy(text, "en_US_POSIX");
    _RemoveSubstringsIfPresent(text, "_POSIX");
    if (strcmp(text, "en_US") != 0) {
        return fail(__LINE__);
    }
    strcpy(text, "abcabc");
    _RemoveSubstringsIfPresent(text, " b \t c\n");
    if (strcmp(text, "aabc") != 0) {
        return fail(__LINE__);
    }
    strcpy(text, "abc");
    _RemoveSubstringsIfPresent(text, "");
    _RemoveSubstringsIfPresent(text, "x");
    if (strcmp(text, "abc") != 0) {
        return fail(__LINE__);
    }
    cases += 3;
    strcpy(text, "en_US");
    _AppendKeyValueString(text, "");
    if (strcmp(text, "en_US") != 0) {
        return fail(__LINE__);
    }
    _AppendKeyValueString(text, "@calendar=japanese");
    if (strcmp(text, "en_US@calendar=japanese") != 0) {
        return fail(__LINE__);
    }
    cases += 2;
    printf("Locale-identifier standalone cases passed: %zu.\n", cases);
    return 0;
}

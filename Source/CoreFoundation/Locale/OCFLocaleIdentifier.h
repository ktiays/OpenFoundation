//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdbool.h>

// Sixteen-byte record of _localeStringPrefixToCanonical and of the bsearch keys the comparators
// receive, iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified:
// the key string pointer at offset 0 (the only field the comparators read) and the result string
// pointer at offset 8 (read by __UpdateFullLocaleString after a match). The native caller builds its
// key as {string, NULL}. LocaleStringMapping and its field names are local descriptive spellings.
// Reading: Documentation/Reference/Research/CoreFoundationLocaleIdentifier.md.
typedef struct {
    const char *key;
    const char *value;
} LocaleStringMapping;

// Forty-byte record of _specialCases, iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified: five string pointers at offsets 0, 8, 16, 24 and 32; a record whose regionTag
// is NULL ends the table. LocaleSpecialCase and its field names are local descriptive spellings.
// Reading: Documentation/Reference/Research/CoreFoundationLocaleIdentifier.md.
typedef struct {
    const char *languageTag;
    const char *regionTag;
    const char *replacement;
    const char *alternateRegionTag;
    const char *alternateReplacement;
} LocaleSpecialCase;

// Original non-external names are preserved; no process-shared symbol is exported.
OF_EXTERN int _CompareTestEntryToTableEntryKey(const void *testEntry, const void *tableEntry);

OF_EXTERN void _UpdateFullLocaleString(
    char *inLocaleString, char **hyphenRegion, char **underscoreRegion, char *keyValueString
);

OF_EXTERN int _CompareTestEntryPrefixToTableEntryKey(const void *testEntry, const void *tableEntry);

OF_EXTERN void _RemoveSubstringsIfPresent(char *localeString, const char *substrings);

OF_EXTERN void _AppendKeyValueString(char *localeString, const char *keyValueString);

OF_EXTERN int _CompareLowerTestEntryPrefixToTableEntryKey(const void *testEntry, const void *tableEntry);

OF_EXTERN bool _CheckForTag(const char *localeString, const char *tag);

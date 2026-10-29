//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Locale/OCFLocaleIdentifier.h"

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// ICU (libicucore) is a library the native image links (/usr/lib/libicucore.A.dylib); it is supplied,
// never reimplemented. The SDK installs no uloc.h or uenum.h, so the called entry points are declared
// here. UErrorCode is a 32-bit enumeration; the native callers pass 32-bit stack slots. UEnumeration is
// opaque.
typedef struct UEnumeration UEnumeration;

extern int32_t uloc_canonicalize(const char *localeID, char *name, int32_t nameCapacity, int32_t *err);

extern UEnumeration *uloc_openKeywords(const char *localeID, int32_t *status);

extern const char *uenum_next(UEnumeration *en, int32_t *resultLength, int32_t *status);

extern int32_t uloc_getKeywordValue(
    const char *localeID, const char *keywordName, char *buffer, int32_t bufferCapacity, int32_t *status
);

extern int32_t uloc_setKeywordValue(
    const char *keywordName, const char *keywordValue, char *buffer, int32_t bufferCapacity, int32_t *status
);

extern void uenum_close(UEnumeration *en);

// Inferred organization: the seven functions lie in one iOS Simulator __text region
// (0x1c01a0324-0x1c01a1960) with _CFLocaleCreateCanonicalLanguageIdentifierFromString and
// _CFLocaleCreateCanonicalLocaleIdentifierFromString, their callers; their tables are in
// __DATA_CONST,__const beside _localeStringPrefixToDefaults. The native source path is not in the
// evidence; Source/CoreFoundation/Locale/OCFLocaleIdentifier.c is a local spelling.
// The tables below were decoded from the capture by Local/Evidence/cf-locale-identifier/generate-tables.py,
// which also checks that the iOS Simulator and iOS Device images agree record for record.
// Character classes: the native code expands the <ctype.h> inlines against _DefaultRuneLocale (a
// sign-tested index into __runetype, otherwise __maskrune) with masks 0x100 (alpha), 0x500 (alnum),
// 0x800 (graph), 0x4000 (space) and 0x8000 (upper); the same inlines are used here. Case conversion
// calls __tolower and __toupper.

// Native _localeStringPrefixToCanonical, iOS Simulator 24A434, 0x2227a0b98 (__DATA_CONST,__const, 304
// sixteen-byte records: the bsearch count 0x130 and width 0x10 of __UpdateFullLocaleString), UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external. Sorted by key in byte order.
// iOS Device 24A446: 0x1e04ead30, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// chained-fixup targets decode to the same 304 key/value strings; fixup metadata bits remain unread.
// Reading: Documentation/Reference/Research/CoreFoundationLocaleIdentifier.md.
static const LocaleStringMapping localeStringPrefixToCanonical[304] = {
    {"aar", "aa"},         {"abk", "ab"},         {"afr", "af"},        {"aju", "jrb"},
    {"aka", "ak"},         {"alb", "sq"},         {"als", "sq"},        {"amh", "am"},
    {"ara", "ar"},         {"arb", "ar"},         {"arg", "an"},        {"arm", "hy"},
    {"art-lojban", "jbo"}, {"asm", "as"},         {"ava", "av"},        {"ave", "ae"},
    {"aym", "ay"},         {"ayr", "ay"},         {"aze", "az"},        {"azj", "az"},
    {"bak", "ba"},         {"bam", "bm"},         {"baq", "eu"},        {"bcc", "bal"},
    {"bcl", "bik"},        {"bel", "be"},         {"ben", "bn"},        {"bih", "bh"},
    {"bis", "bi"},         {"bod", "bo"},         {"bos", "bs"},        {"bre", "br"},
    {"bul", "bg"},         {"bur", "my"},         {"bxk", "luy"},       {"bxr", "bua"},
    {"cat", "ca"},         {"ces", "cs"},         {"cha", "ch"},        {"che", "ce"},
    {"chi", "zh"},         {"chu", "cu"},         {"chv", "cv"},        {"cld", "syr"},
    {"cmn", "zh"},         {"cor", "kw"},         {"cos", "co"},        {"cre", "cr"},
    {"cym", "cy"},         {"cze", "cs"},         {"dan", "da"},        {"deu", "de"},
    {"dgo", "doi"},        {"dhd", "mwr"},        {"dik", "din"},       {"diq", "zza"},
    {"div", "dv"},         {"dut", "nl"},         {"dzo", "dz"},        {"ekk", "et"},
    {"ell", "el"},         {"emk", "man"},        {"eng", "en"},        {"epo", "eo"},
    {"esk", "ik"},         {"est", "et"},         {"eus", "eu"},        {"ewe", "ee"},
    {"fao", "fo"},         {"fas", "fa"},         {"fat", "ak"},        {"fij", "fj"},
    {"fin", "fi"},         {"fra", "fr"},         {"fre", "fr"},        {"fry", "fy"},
    {"fuc", "ff"},         {"ful", "ff"},         {"gaz", "om"},        {"gbo", "grb"},
    {"geo", "ka"},         {"ger", "de"},         {"gla", "gd"},        {"gle", "ga"},
    {"glg", "gl"},         {"glv", "gv"},         {"gno", "gon"},       {"gre", "el"},
    {"grn", "gn"},         {"gug", "gn"},         {"guj", "gu"},        {"gya", "gba"},
    {"hat", "ht"},         {"hau", "ha"},         {"hbs", "sr_Latn"},   {"hdn", "hai"},
    {"hea", "hmn"},        {"heb", "he"},         {"her", "hz"},        {"him", "srx"},
    {"hin", "hi"},         {"hmo", "ho"},         {"hrv", "hr"},        {"hun", "hu"},
    {"hye", "hy"},         {"i-ami", "ami"},      {"i-bnn", "bnn"},     {"i-hak", "hak"},
    {"i-klingon", "tlh"},  {"i-lux", "lb"},       {"i-navajo", "nv"},   {"i-pwn", "pwn"},
    {"i-tao", "tao"},      {"i-tay", "tay"},      {"i-tsu", "tsu"},     {"ibo", "ig"},
    {"ice", "is"},         {"ido", "io"},         {"iii", "ii"},        {"ike", "iu"},
    {"iku", "iu"},         {"ile", "ie"},         {"in", "id"},         {"ina", "ia"},
    {"ind", "id"},         {"ipk", "ik"},         {"isl", "is"},        {"ita", "it"},
    {"iw", "he"},          {"jav", "jv"},         {"jaw", "jv"},        {"ji", "yi"},
    {"jpn", "ja"},         {"jw", "jv"},          {"kal", "kl"},        {"kan", "kn"},
    {"kas", "ks"},         {"kat", "ka"},         {"kau", "kr"},        {"kaz", "kk"},
    {"khk", "mn"},         {"khm", "km"},         {"kik", "ki"},        {"kin", "rw"},
    {"kir", "ky"},         {"kmr", "ku"},         {"knc", "kr"},        {"kng", "kg"},
    {"knn", "kok"},        {"kom", "kv"},         {"kon", "kg"},        {"kor", "ko"},
    {"kpv", "kv"},         {"kua", "kj"},         {"kur", "ku"},        {"lao", "lo"},
    {"lat", "la"},         {"lav", "lv"},         {"lbk", "bnc"},       {"lim", "li"},
    {"lin", "ln"},         {"lit", "lt"},         {"ltz", "lb"},        {"lub", "lu"},
    {"lug", "lg"},         {"lvs", "lv"},         {"mac", "mk"},        {"mal", "ml"},
    {"mar", "mr"},         {"may", "ms"},         {"mhr", "chm"},       {"mkd", "mk"},
    {"mlg", "mg"},         {"mlt", "mt"},         {"mol", "mo"},        {"mon", "mn"},
    {"msa", "ms"},         {"mup", "raj"},        {"mya", "my"},        {"nau", "na"},
    {"nav", "nv"},         {"nbl", "nr"},         {"nde", "nd"},        {"ndo", "ng"},
    {"nep", "ne"},         {"nld", "nl"},         {"nno", "nn"},        {"no", "nb"},
    {"no-bok", "nb"},      {"no-nyn", "nn"},      {"nob", "nb"},        {"nor", "nb"},
    {"nya", "ny"},         {"oci", "oc"},         {"ojg", "oj"},        {"oji", "oj"},
    {"ori", "or"},         {"orm", "om"},         {"oss", "os"},        {"pan", "pa"},
    {"pbu", "ps"},         {"per", "fa"},         {"pes", "fa"},        {"pli", "pi"},
    {"plt", "mg"},         {"pnb", "lah"},        {"pol", "pl"},        {"por", "pt"},
    {"pus", "ps"},         {"que", "qu"},         {"qxp", "qu"},        {"rmy", "rom"},
    {"roh", "rm"},         {"ron", "ro"},         {"rum", "ro"},        {"run", "rn"},
    {"rus", "ru"},         {"sag", "sg"},         {"san", "sa"},        {"scc", "sr"},
    {"scr", "hr"},         {"sgn-be-fr", "sfb"},  {"sgn-be-nl", "vgt"}, {"sgn-ch-de", "sgg"},
    {"sin", "si"},         {"slk", "sk"},         {"slo", "sk"},        {"slv", "sl"},
    {"sme", "se"},         {"smo", "sm"},         {"sna", "sn"},        {"snd", "sd"},
    {"som", "so"},         {"sot", "st"},         {"spa", "es"},        {"spy", "kln"},
    {"sqi", "sq"},         {"src", "sc"},         {"srd", "sc"},        {"srp", "sr"},
    {"ssw", "ss"},         {"sun", "su"},         {"swa", "sw"},        {"swe", "sv"},
    {"swh", "sw"},         {"tah", "ty"},         {"tam", "ta"},        {"tat", "tt"},
    {"tel", "te"},         {"tgk", "tg"},         {"tgl", "fil"},       {"tha", "th"},
    {"tib", "bo"},         {"tir", "ti"},         {"tl", "fil"},        {"ton", "to"},
    {"tsn", "tn"},         {"tso", "ts"},         {"ttq", "tmh"},       {"tuk", "tk"},
    {"tur", "tr"},         {"tw", "ak"},          {"twi", "ak"},        {"uig", "ug"},
    {"ukr", "uk"},         {"umu", "del"},        {"urd", "ur"},        {"uzb", "uz"},
    {"uzn", "uz"},         {"ven", "ve"},         {"vie", "vi"},        {"vol", "vo"},
    {"wel", "cy"},         {"wln", "wa"},         {"wol", "wo"},        {"xho", "xh"},
    {"xpe", "kpe"},        {"xsl", "den"},        {"ydd", "yi"},        {"yid", "yi"},
    {"yor", "yo"},         {"zai", "zap"},        {"zh-cdo", "cdo"},    {"zh-cjy", "cjy"},
    {"zh-cmn", "zh"},      {"zh-cpx", "cpx"},     {"zh-czh", "czh"},    {"zh-czo", "czo"},
    {"zh-gan", "gan"},     {"zh-guoyu", "zh"},    {"zh-hak", "hak"},    {"zh-hakka", "hak"},
    {"zh-hsn", "hsn"},     {"zh-min-nan", "nan"}, {"zh-mnp", "mnp"},    {"zh-nan", "nan"},
    {"zh-wuu", "wuu"},     {"zh-xiang", "hsn"},   {"zh-yue", "yue"},    {"zha", "za"},
    {"zho", "zh"},         {"zsm", "ms"},         {"zul", "zu"},        {"zyb", "za"},
};

// Native _specialCases, iOS Simulator 24A434, 0x2227a1e98 (__DATA_CONST,__const, seven forty-byte
// records and an all-NULL terminator at +0x118: the loop of __UpdateFullLocaleString advances by 0x28 and
// stops when the next record's regionTag is NULL), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified. Non-external. The native loop hoists the first regionTag, "-UK" (0x1c04820dc), as a literal.
// iOS Device 24A446: 0x1e04ec030, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// chained-fixup targets decode to the same strings and NULL slots; fixup metadata bits remain unread.
// Reading: Documentation/Reference/Research/CoreFoundationLocaleIdentifier.md.
static const LocaleSpecialCase specialCases[8] = {
    {NULL, "-UK", "GB", NULL, NULL},
    {NULL, "-TP", "TL", NULL, NULL},
    {"cs", "-CS", "CZ", NULL, NULL},
    {"sk", "-CS", "SK", NULL, NULL},
    {NULL, "-CS", "RS", NULL, NULL},
    {NULL, "-YU", "RS", NULL, NULL},
    {"sh", "-HR", "hr", "-RS", "sr"},
    {NULL, NULL, NULL, NULL, NULL},
};

// Native _CompareTestEntryToTableEntryKey, iOS Simulator 24A434, 0x1c01a0324, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external bsearch callback: loads the
// key pointer at offset 0 of both records and tail-calls strcmp, so the strcmp result is returned
// unchanged. iOS Device 24A446: 0x1806a63f0, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence:
// verified; identical instructions. Reading:
// Documentation/Reference/Research/CoreFoundationLocaleIdentifier.md.
int _CompareTestEntryToTableEntryKey(const void *testEntry, const void *tableEntry) {
    return strcmp(
        ((const LocaleStringMapping *)testEntry)->key, ((const LocaleStringMapping *)tableEntry)->key
    );
}

// Native _UpdateFullLocaleString, iOS Simulator 24A434, 0x1c01a0330, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external. The callers pass a 257-byte
// locale buffer and a 257-byte key/value buffer (uloc_canonicalize capacity 0x101); the two region
// pointers are stored through the second and third arguments last.
// CoreFoundation defect: after a canonicalized key/value string is found, the native loop clears the
// variant separator and walks back over '_' bytes with no lower bound (0x1c01a0644: STRB WZR, [X25];
// LDRB W9, [X25,#-1]!), so an identifier whose leading bytes up to the variant separator are all '_'
// (for example "__X@a=b") reads the byte before the buffer and clears it when it is '_'; confidence:
// verified. The restoration stops at the first byte of the buffer; every result whose native walk stays
// inside the buffer is unchanged. The iOS Device image has the same unbounded walk (0x1806a6e8c).
// iOS Device divergence: _UpdateFullLocaleString 0x1806a6a84, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified. The language-tag test of _CheckForTag is inlined at 0x1806a6c48, and the
// lower-case special-case path is split into two outlined blocks: __UpdateFullLocaleString.cold.1
// (0x1807bd6b0, called at 0x1806a6f90) returns !isalnum(found[3]) for the first regionTag occurrence, and
// __UpdateFullLocaleString.cold.2 (0x1807bd6f0, called at 0x1806a6f38 with the record, a NULL-region
// flag, the buffer and region + 1) tests the alternate tag against the '_' region, then by strstr and a
// non-alphanumeric fourth byte, and copies alternateReplacement over the buffer. The table search, scan,
// canonicalization step, both special-case paths and their order of tests agree with the iOS Simulator.
// Reading: Documentation/Reference/Research/CoreFoundationLocaleIdentifier.md.
void _UpdateFullLocaleString(
    char *inLocaleString, char **hyphenRegion, char **underscoreRegion, char *keyValueString
) {
    LocaleStringMapping testEntry = {inLocaleString, NULL};
    const LocaleStringMapping *foundEntry = bsearch(
        &testEntry, localeStringPrefixToCanonical, 0x130, 0x10, _CompareLowerTestEntryPrefixToTableEntryKey
    );
    if (foundEntry != NULL) {
        int keyLength = (int)strlen(foundEntry->key);
        const char *value = foundEntry->value;
        int valueLength = (int)strlen(value);
        int delta = valueLength - keyLength;
        bool replace = true;
        if (delta < 0) {
            char *cursor = inLocaleString + valueLength;
            unsigned int distance = (unsigned int)-delta;
            unsigned char byte;
            do {
                byte = (unsigned char)cursor[distance];
                *cursor++ = (char)byte;
            } while (byte != 0);
        } else if (delta > 0) {
            int length = (int)strlen(inLocaleString);
            if (delta + length > 256) {
                replace = false;
            } else if ((long)length >= (long)keyLength) {
                char *stop = inLocaleString + keyLength;
                char *cursor = inLocaleString + length;
                do {
                    cursor[(unsigned int)delta] = *cursor;
                    --cursor;
                } while (cursor >= stop);
            }
        }
        if (replace) {
            char byte = *value;
            if (byte != 0) {
                const char *source = value + 1;
                char *destination = inLocaleString;
                do {
                    *destination++ = byte;
                    byte = *source++;
                } while (byte != 0);
            }
        }
    }

    char *underscoreRegionStart = NULL;
    char *hyphenRegionStart = NULL;
    char *variantSeparator = NULL;
    bool sawDigit = false;
    bool sawSeparator = false;
    bool upperCase = false;
    char *segment = inLocaleString;
    for (uintptr_t index = 0;; ++index) {
        char *current = inLocaleString + index;
        char character = *current;
        if (isalpha(character)) {
            *current = (char)(upperCase ? __toupper(character) : __tolower(character));
            continue;
        }
        if ((unsigned int)(character - '0') < 10) {
            sawDigit = true;
            continue;
        }
        if (!sawSeparator) {
            if (sawDigit) {
                break;
            }
        } else if (upperCase) {
            upperCase = true;
            variantSeparator = segment;
        } else {
            // 32-bit segment offset and index, as the native W-register comparisons.
            uint32_t offset = (uint32_t)((uintptr_t)segment - (uintptr_t)inLocaleString);
            uint32_t position = (uint32_t)index;
            bool hyphenate = true;
            if (offset + 3 == position && !sawDigit) {
                if (*segment == '_') {
                    segment[1] = (char)__toupper(segment[1]);
                    segment[2] = (char)__toupper(segment[2]);
                    upperCase = true;
                    underscoreRegionStart = segment;
                    hyphenate = false;
                } else if (hyphenRegionStart == NULL) {
                    segment[1] = (char)__toupper(segment[1]);
                    segment[2] = (char)__toupper(segment[2]);
                    hyphenRegionStart = segment;
                }
            } else if (offset + 4 == position && sawDigit) {
                if (*segment == '_') {
                    upperCase = true;
                    underscoreRegionStart = segment;
                    hyphenate = false;
                } else if (hyphenRegionStart == NULL) {
                    hyphenRegionStart = segment;
                }
            } else if (offset + 5 == position && !sawDigit) {
                segment[1] = (char)__toupper(segment[1]);
            } else if (offset + 1 == position && *segment == '_') {
                upperCase = true;
                hyphenate = false;
            }
            if (hyphenate) {
                upperCase = false;
                *segment = '-';
            }
        }
        sawDigit = false;
        sawSeparator = true;
        segment = current;
        if (*current != '_' && *current != '-') {
            break;
        }
    }

    *keyValueString = 0;
    if (variantSeparator != NULL) {
        int32_t status = 0;
        int32_t length = uloc_canonicalize(inLocaleString, keyValueString, 0x101, &status);
        if (status <= 0) {
            keyValueString[length < 0x100 ? length : 0x100] = 0;
            char *destination = keyValueString;
            for (const char *cursor = keyValueString; *cursor != 0; ++cursor) {
                if (*cursor == '@') {
                    // CoreFoundation defect fix (see above): the native walk has no lower bound.
                    do {
                        *variantSeparator = 0;
                        if (variantSeparator == inLocaleString) {
                            break;
                        }
                        --variantSeparator;
                    } while (*variantSeparator == '_');
                    char byte = *cursor;
                    if (byte != 0) {
                        size_t offset = 1;
                        do {
                            *destination++ = byte;
                            byte = cursor[offset++];
                        } while (byte != 0);
                    }
                    break;
                }
            }
            *destination = 0;
        }
    }

    const char *regionTag = specialCases[0].regionTag;
    for (const LocaleSpecialCase *entry = specialCases;; ++entry) {
        if (entry->languageTag == NULL || _CheckForTag(inLocaleString, entry->languageTag)) {
            const char *replacement = entry->replacement;
            const char *source = NULL;
            char *destination = NULL;
            if (isupper(*replacement)) {
                char *found = strstr(inLocaleString, regionTag);
                if (found != NULL && !isalnum(found[3])) {
                    for (const char *byte = replacement; *byte != 0; ++byte) {
                        *++found = *byte;
                    }
                }
                if (underscoreRegionStart != NULL && _CheckForTag(underscoreRegionStart + 1, regionTag + 1)) {
                    source = replacement;
                    destination = underscoreRegionStart + 1;
                }
            } else if (
                underscoreRegionStart != NULL && _CheckForTag(underscoreRegionStart + 1, regionTag + 1)
            ) {
                source = replacement;
                destination = inLocaleString;
            } else {
                char *found = strstr(inLocaleString, regionTag);
                if (found != NULL && !isalnum(found[3])) {
                    source = replacement;
                    destination = inLocaleString;
                } else {
                    const char *alternateRegionTag = entry->alternateRegionTag;
                    if ((underscoreRegionStart != NULL &&
                         _CheckForTag(underscoreRegionStart + 1, alternateRegionTag + 1)) ||
                        ((found = strstr(inLocaleString, alternateRegionTag)) != NULL &&
                         !isalnum(found[3]))) {
                        source = entry->alternateReplacement;
                        destination = inLocaleString;
                    }
                }
            }
            if (source != NULL) {
                for (; *source != 0; ++source) {
                    *destination++ = *source;
                }
            }
        }
        regionTag = entry[1].regionTag;
        if (regionTag == NULL) {
            break;
        }
    }
    *hyphenRegion = hyphenRegionStart;
    *underscoreRegion = underscoreRegionStart;
}

// Native _CompareTestEntryPrefixToTableEntryKey, iOS Simulator 24A434, 0x1c01a0888, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external bsearch callback: 0 when the
// table key is a byte prefix of the test key, otherwise -1 or 1 by a signed comparison of the first
// differing bytes. iOS Device 24A446: 0x1806a7388, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence:
// verified; the loop is rotated (pre-indexed loads), with the same comparisons and results.
// Reading: Documentation/Reference/Research/CoreFoundationLocaleIdentifier.md.
int _CompareTestEntryPrefixToTableEntryKey(const void *testEntry, const void *tableEntry) {
    const unsigned char *test = (const unsigned char *)((const LocaleStringMapping *)testEntry)->key;
    const unsigned char *table = (const unsigned char *)((const LocaleStringMapping *)tableEntry)->key;
    unsigned char testByte = *test++;
    unsigned char tableByte = *table++;
    bool tableEnded = tableByte == 0;
    while (testByte == tableByte && tableByte != 0) {
        testByte = *test++;
        tableByte = *table++;
        tableEnded = tableByte == 0;
    }
    int result = (signed char)testByte < (signed char)tableByte ? -1 : 1;
    return tableEnded ? 0 : result;
}

// Native _RemoveSubstringsIfPresent, iOS Simulator 24A434, 0x1c01a08e4, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external; the callers ignore X0, so
// the result is void. Each run of graph bytes of the substring list is copied to a 257-byte stack
// buffer, the following space bytes are skipped, and the first occurrence of the run in the locale
// string is removed. Native preconditions kept as read: a run longer than 256 bytes overflows the
// stack buffer, and a byte that is neither graph, space nor NUL never advances the list (the loop does
// not terminate); every native caller passes a table string and neither case is exercised.
// iOS Device 24A446: 0x1806ca710, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; one
// counter serves as both index and removal length, and the space skip starts at the byte after the
// run; the copies, searches and removals agree.
// Reading: Documentation/Reference/Research/CoreFoundationLocaleIdentifier.md.
void _RemoveSubstringsIfPresent(char *localeString, const char *substrings) {
    char character = *substrings;
    if (character == 0) {
        return;
    }
    const char *cursor = substrings;
    do {
        char substring[257];
        size_t length = 0;
        while (isgraph(character)) {
            substring[length] = character;
            character = cursor[length + 1];
            ++length;
        }
        cursor += length;
        while (isspace(*cursor)) {
            ++cursor;
        }
        if (length != 0) {
            substring[(uint32_t)length] = 0;
            char *found = strstr(localeString, substring);
            if (found != NULL) {
                char byte;
                do {
                    byte = found[(uint32_t)length];
                    *found++ = byte;
                } while (byte != 0);
            }
        }
        character = *cursor;
    } while (character != 0);
}

// Native _AppendKeyValueString, iOS Simulator 24A434, 0x1c01a0a14, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external; the callers ignore X0, so
// the result is void. The locale buffer capacity is 0x101 and the keyword value buffer is 96 bytes;
// the same 32-bit slot receives the uenum_next length and both keyword-call results. A value that
// fills the 96 bytes exactly is passed to uloc_setKeywordValue without a terminator when libicucore
// reports only a warning; that libicucore behavior is unread, the native code is kept, and the case
// is not exercised. iOS Device 24A446: 0x1806a751c, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B;
// confidence: verified; the early return is placed first and the loop rotated; calls, arguments,
// capacities and status tests agree.
// Reading: Documentation/Reference/Research/CoreFoundationLocaleIdentifier.md.
void _AppendKeyValueString(char *localeString, const char *keyValueString) {
    if (*keyValueString == 0) {
        return;
    }
    int32_t status = 0;
    UEnumeration *keywords = uloc_openKeywords(keyValueString, &status);
    if (keywords == NULL) {
        return;
    }
    int32_t length = 0;
    if (status <= 0) {
        do {
            const char *keyword = uenum_next(keywords, &length, &status);
            if (keyword == NULL) {
                break;
            }
            char value[96];
            length = uloc_getKeywordValue(keyValueString, keyword, value, 96, &status);
            length = uloc_setKeywordValue(keyword, value, localeString, 0x101, &status);
        } while (status < 1);
    }
    uenum_close(keywords);
}

// Native _CompareLowerTestEntryPrefixToTableEntryKey, iOS Simulator 24A434, 0x1c01a1810, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external bsearch callback. Bytes of
// the test key pass through __tolower and are compared, as signed chars, with the table key until
// they differ, the table key ends or the lowered byte is '_'. When the table key has ended the result
// is 1 if the lowered test byte is alphanumeric, otherwise 0 (a match); when it has not, an '_' test
// byte orders first (-1), otherwise the signed comparison decides. iOS Device 24A446: 0x1806a6fbc,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; pre-indexed loads and a shared exit,
// with the same comparisons and results.
// Reading: Documentation/Reference/Research/CoreFoundationLocaleIdentifier.md.
int _CompareLowerTestEntryPrefixToTableEntryKey(const void *testEntry, const void *tableEntry) {
    const char *test = ((const LocaleStringMapping *)testEntry)->key;
    const char *table = ((const LocaleStringMapping *)tableEntry)->key;
    char lowered = (char)__tolower(*test);
    char tableByte = *table;
    while (lowered == tableByte && tableByte != 0 && lowered != '_') {
        lowered = (char)__tolower(*++test);
        tableByte = *++table;
    }
    if (tableByte != 0) {
        if (lowered == '_') {
            return -1;
        }
        return lowered < tableByte ? -1 : 1;
    }
    return isalnum(lowered) ? 1 : 0;
}

// Native _CheckForTag, iOS Simulator 24A434, 0x1c01a18fc, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external: true (W0 = 1) when the first two bytes equal the tag's and the
// third byte is not alphanumeric. iOS Device divergence: _CheckForTag 0x1806a7118, UUID
// 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified. The non-negative __runetype lookup is the
// shared outlined _OUTLINED_FUNCTION_0_21 (0x18065c6b0, called at 0x1806a7150): __runetype[c] & 0x500
// through the same rune-locale GOT slot (0x1e016af20) as the inline lookups; the strncmp, sign test and
// __maskrune path agree. Reading: Documentation/Reference/Research/CoreFoundationLocaleIdentifier.md.
bool _CheckForTag(const char *localeString, const char *tag) {
    if (strncmp(localeString, tag, 2) != 0) {
        return false;
    }
    return !isalnum(localeString[2]);
}

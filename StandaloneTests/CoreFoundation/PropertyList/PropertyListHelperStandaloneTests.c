//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PropertyList/PropertyListHelperStandaloneTests.h"
#include "CoreFoundation/PropertyList/OCFBinaryPList.h"
#include "CoreFoundation/PropertyList/OCFOldStylePList.h"
#include "CoreFoundation/PropertyList/OCFPropertyList.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

static int fail(int line) {
    fprintf(stderr, "Property-list helper standalone mismatch at source line %d.\n", line);
    return 1;
}

// Native __CFBinaryPlistGetOffsetForValueFromArray2 0x1c0163e48, __CFBinaryPlistParseASCIIString
// 0x1c01667dc, __CFBinaryPlistParseUnicode16String 0x1c01668f4, __CFPropertyListKeyBufferMatchesFilterBuffer
// 0x1c01d446c, lineNumber 0x1c01d84dc, lineNumberStrings 0x1c02d54e0 and advanceToNonSpace 0x1c02d4e94,
// iOS Simulator 24A434 UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Device comparison:
// 0x180733140, 0x18061f684, 0x18068035c, 0x18068f8c4, 0x1806f6734, 0x18070d188, 0x180702bac,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified for the inline bodies.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationPropertyListHelperStandaloneTests(void) {
    size_t cases = 0;
    if (sizeof(BinaryPlistTrailer) != 32 || offsetof(BinaryPlistTrailer, numObjects) != 8 ||
        offsetof(BinaryPlistTrailer, offsetTableOffset) != 24) {
        return fail(__LINE__);
    }
    ++cases;
    // "bplist00", a two-element array (0xA2) with one-byte references 1 and 0 at offset 8, then the
    // offset table at offset 11 with one-byte entries 8 and 10.
    uint8_t data[16] = {'b', 'p', 'l', 'i', 's', 't', '0', '0', 0xa2, 0x01, 0x00, 0x08, 0x0a, 0, 0, 0};
    BinaryPlistTrailer trailer;
    memset(&trailer, 0, sizeof(trailer));
    trailer.offsetIntSize = 1;
    trailer.objectRefSize = 1;
    trailer.numObjects = 2;
    trailer.offsetTableOffset = 11;
    uint64_t offset = 0;
    if (!__OCFBinaryPlistGetOffsetForValueFromArray2(data, sizeof(data), 8, &trailer, 0, &offset, NULL) ||
        offset != 10) {
        return fail(__LINE__);
    }
    ++cases;
    if (!__OCFBinaryPlistGetOffsetForValueFromArray2(data, sizeof(data), 8, &trailer, 1, &offset, NULL) ||
        offset != 8) {
        return fail(__LINE__);
    }
    ++cases;
    offset = 77;
    if (__OCFBinaryPlistGetOffsetForValueFromArray2(data, sizeof(data), 8, &trailer, 2, &offset, NULL) ||
        offset != 77 ||
        __OCFBinaryPlistGetOffsetForValueFromArray2(data, sizeof(data), 7, &trailer, 0, NULL, NULL)) {
        return fail(__LINE__);
    }
    ++cases;
    // A reference at or above numObjects fails.
    data[9] = 0x02;
    if (__OCFBinaryPlistGetOffsetForValueFromArray2(data, sizeof(data), 8, &trailer, 0, NULL, NULL)) {
        return fail(__LINE__);
    }
    ++cases;
    // Strings: a five-character inline count and a 0xF count followed by a one-byte integer of 3.
    const uint8_t ascii[8] = {0x55, 'h', 'e', 'l', 'l', 'o', 0, 0};
    __block uint64_t seen = 0;
    __block const uint8_t *seenCharacters = NULL;
    BinaryPlistStringBlock block = ^bool(const uint8_t *characters, uint64_t count) {
      seen = count;
      seenCharacters = characters;
      return true;
    };
    if (!__OCFBinaryPlistParseASCIIString(ascii, 5, ascii, block) || seen != 5 ||
        seenCharacters != ascii + 1) {
        return fail(__LINE__);
    }
    ++cases;
    seen = 0;
    if (__OCFBinaryPlistParseASCIIString(ascii, 4, ascii, block) || seen != 0) {
        return fail(__LINE__);
    }
    ++cases;
    const uint8_t unicode[9] = {0x6f, 0x10, 0x03, 0, 'a', 0, 'b', 0, 'c'};
    if (!__OCFBinaryPlistParseUnicode16String(unicode, 8, unicode, block) || seen != 3 ||
        seenCharacters != unicode + 3) {
        return fail(__LINE__);
    }
    ++cases;
    seen = 0;
    if (__OCFBinaryPlistParseUnicode16String(unicode, 7, unicode, block) || seen != 0) {
        return fail(__LINE__);
    }
    ++cases;
    // Filters.
    if (!__CFPropertyListKeyBufferMatchesFilterBuffer("anything", 8, "*", 1) ||
        !__CFPropertyListKeyBufferMatchesFilterBuffer("CFBundleName", 12, "CFBundle*", 9) ||
        __CFPropertyListKeyBufferMatchesFilterBuffer("CFBund", 6, "CFBundle*", 9) ||
        !__CFPropertyListKeyBufferMatchesFilterBuffer("key", 3, "key", 3) ||
        __CFPropertyListKeyBufferMatchesFilterBuffer("key", 3, "ke", 2) ||
        !__CFPropertyListKeyBufferMatchesFilterBuffer("k", 1, "k", 1)) {
        return fail(__LINE__);
    }
    ++cases;
    // Line numbers: CR LF counts once, lone CR and LF count each.
    const uint8_t text[] = "a\r\nb\rc\nd";
    XMLPlistParseContext xml = {text, text + 7, text + 8};
    if (lineNumber(&xml) != 4) {
        return fail(__LINE__);
    }
    ++cases;
    xml.curr = text + 2;
    if (lineNumber(&xml) != 2) {
        return fail(__LINE__);
    }
    ++cases;
    const uint16_t units[] = {'a', '\r', '\n', 'b', '\n', ' ', '/', '/', 'x', '\n', '/', '*', '*', '/', 'k'};
    OldStylePlistParseContext oldStyle = {units, units + 5, units + 15};
    if (lineNumberStrings(&oldStyle) != 3) {
        return fail(__LINE__);
    }
    ++cases;
    if (!advanceToNonSpace(&oldStyle) || oldStyle.curr != units + 14) {
        return fail(__LINE__);
    }
    ++cases;
    oldStyle.curr = units + 15;
    if (advanceToNonSpace(&oldStyle) || oldStyle.curr != units + 15) {
        return fail(__LINE__);
    }
    ++cases;
    printf("Property-list helper standalone cases passed: %zu.\n", cases);
    return 0;
}

//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PropertyList/OCFOldStylePList.h"

#include <ctype.h>
#include <runetype.h>

// Inferred organization: the old-style parser lies contiguously in the iOS Simulator __text section, from
// __CFCreateOldStylePropertyListOrStringsFile (0x1c02d4b0c) through parsePlistString (0x1c02d5fc0), and is
// kept in one translation unit in native address order; the native source path is not in the evidence.
// _uniqueStringForCharacters (0x1c02d5dbc) belongs to this unit and is restored by another sub-track. Unless
// a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (disassembly); iOS Device 24A446: unread for the parse functions.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md (advanceToNonSpace,
// lineNumberStrings); the other functions: Local/Scratch/cf-core/tasks/plist.txt (sub-track reading; no
// versioned reading yet).

static OCFTypeRef parsePlistObject(OldStylePlistParseContext *context, bool requireObject, uint32_t depth);
static OCFMutableDictionaryRef parsePlistDictContent(OldStylePlistParseContext *context, uint32_t depth);
static OCFStringRef parseQuotedPlistString(OldStylePlistParseContext *context, UniChar quote);
static OCFStringRef parseUnquotedPlistString(OldStylePlistParseContext *context);
static OCFStringRef parsePlistString(OldStylePlistParseContext *context);

// The helpers below reproduce code the native functions inline at each use (no native symbols). The parse
// functions replace the pending error after creating the new one: the new error is created first, then the
// old one released, then the new one stored.
static void replaceParseError(OldStylePlistParseContext *context, OCFErrorRef error) {
    if (context->error != NULL) {
        OCFRelease(context->error);
    }
    context->error = error;
}

static void clearParseError(OldStylePlistParseContext *context) {
    if (context->error != NULL) {
        OCFRelease(context->error);
        context->error = NULL;
    }
}

// The characters of an unquoted string, as parsePlistObject, parseUnquotedPlistString and parsePlistString
// test them: ASCII digits and letters (SUB #0x30 / CMP #10 and AND #0xffffffdf / SUB #0x41 / CCMP #0x1a, both
// compared as 16-bit values) and '$', '-', '.', '/', ':' and '_' (bit mask 0x0800000000400e01 over character
// - 0x24 for differences up to 0x3b).
static bool isValidUnquotedStringCharacter(UniChar character) {
    if ((uint16_t)(character - '0') < 10 || (uint16_t)((character & ~0x20u) - 'A') < 26) {
        return true;
    }
    uint32_t bit = (uint32_t)character - 0x24;
    return bit <= 0x3b && ((UINT64_C(1) << bit) & UINT64_C(0x0800000000400e01)) != 0;
}

// Native __CFCreateOldStylePropertyListOrStringsFile, 0x1c02d4b0c. Non-external (was a private external).
// Retains originalString when it is nonnull; otherwise creates a no-copy string over xmlData's bytes in
// guessedEncoding (kCFAllocatorSystemDefault, no external representation, kCFAllocatorNull deallocator). An
// empty string fails with "...The string is empty."; a string without a direct UTF-16 buffer is copied into a
// buffer from allocator (malloc type 0x01000040bdfb0063) and released at once. The overflow test adds the
// first-unit address and the unit count (not the byte count) as signed 64-bit values (CMN / B.VC). The two
// resource failures store their message in the crash annotation (gCRAnnotations[1]) without trapping and
// return NULL with *outError untouched. Without a first object the result is an empty mutable dictionary; a
// top-level string followed by more input reparses the whole input as dictionary content (a strings file).
// Every NULL result after parsing sets *outError, creating "Unknown error..." when no parse error is pending.
// *format receives kCFPropertyListOpenStepFormat only on success.
OCFTypeRef __CFCreateOldStylePropertyListOrStringsFile(
    OCFAllocatorRef allocator,
    OCFDataRef xmlData,
    OCFStringRef originalString,
    OCFStringEncoding guessedEncoding,
    OCFOptionFlags option,
    OCFErrorRef *outError,
    OCFPropertyListFormat *format
) {
    OCFStringRef string = NULL;
    if (originalString != NULL) {
        string = OCFRetain(originalString);
    } else {
        string = OCFStringCreateWithBytesNoCopy(
            kOCFAllocatorSystemDefault,
            OCFDataGetBytePtr(xmlData),
            OCFDataGetLength(xmlData),
            guessedEncoding,
            false,
            kOCFAllocatorNull
        );
        if (string == NULL) {
            if (outError != NULL) {
                *outError = __CFPropertyListCreateError(
                    kOCFPropertyListReadCorruptError, OF_STR("Conversion of string failed.")
                );
            }
            return NULL;
        }
    }
    OCFIndex length = OCFStringGetLength(string);
    if (length == 0) {
        if (outError != NULL) {
            *outError = __CFPropertyListCreateError(
                kOCFPropertyListReadCorruptError, OF_STR("Conversion of string failed. The string is empty.")
            );
        }
        OCFRelease(string);
        return NULL;
    }
    const UniChar *characters = OCFStringGetCharactersPtr(string);
    UniChar *buffer = NULL;
    OldStylePlistParseContext context;
    intptr_t unusedSum = 0;
    if (characters != NULL) {
        context.mutabilityOption = 0;
        context.unused = 0;
        context.begin = characters;
        if (__builtin_add_overflow((intptr_t)characters, (intptr_t)length, &unusedSum)) {
            gCRAnnotations[1] = (uint64_t)(uintptr_t)"Unable to address entirety of CFPropertyList";
            OCFRelease(string);
            return NULL;
        }
        buffer = (UniChar *)characters;
    } else {
        buffer = OCFAllocatorAllocateTyped(
            allocator, length * (OCFIndex)sizeof(UniChar), UINT64_C(0x01000040bdfb0063), 0
        );
        if (buffer == NULL) {
            gCRAnnotations[1] =
                (uint64_t)(uintptr_t)"CFPropertyList ran out of memory while attempting to allocate "
                                     "temporary storage.";
            OCFRelease(string);
            return NULL;
        }
        OCFStringGetCharacters(string, (OCFRange){0, length}, buffer);
        OCFRelease(string);
        context.mutabilityOption = 0;
        context.unused = 0;
        context.begin = buffer;
        if (__builtin_add_overflow((intptr_t)buffer, (intptr_t)length, &unusedSum)) {
            gCRAnnotations[1] = (uint64_t)(uintptr_t)"Unable to address entirety of CFPropertyList";
            OCFAllocatorDeallocate(allocator, buffer);
            return NULL;
        }
        string = NULL;
    }
    context.curr = buffer;
    context.end = buffer + length;
    context.allocator = allocator;
    context.mutabilityOption = (uint32_t)option;
    context.stringSet = OCFSetCreateMutable(allocator, 0, &kOCFTypeSetCallBacks);
    if (context.stringSet == NULL) {
        gCRAnnotations[1] =
            (uint64_t)(uintptr_t)"CFPropertyList ran out of memory while attempting to allocate "
                                 "temporary storage.";
        if (characters != NULL) {
            OCFRelease(string);
        } else {
            OCFAllocatorDeallocate(allocator, buffer);
        }
        return NULL;
    }
    context.error = NULL;
    OCFTypeRef result = NULL;
    if (!advanceToNonSpace(&context)) {
        result = OCFDictionaryCreateMutable(
            allocator, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
    } else {
        result = parsePlistObject(&context, true, 0);
        if (result != NULL && advanceToNonSpace(&context)) {
            OCFTypeID typeID = OCFGetTypeID(result);
            OCFTypeID stringTypeID = OCFStringGetTypeID();
            OCFRelease(result);
            if (typeID == stringTypeID) {
                clearParseError(&context);
                context.curr = buffer;
                result = parsePlistDictContent(&context, 0);
            } else {
                if (context.error != NULL) {
                    OCFRelease(context.error);
                }
                context.error = __CFPropertyListCreateError(
                    kOCFPropertyListReadCorruptError,
                    OF_STR("Junk after plist at line %d"),
                    lineNumberStrings(&context)
                );
                result = NULL;
            }
        }
    }
    if (result != NULL) {
        if (format != NULL) {
            *format = kOCFPropertyListOpenStepFormat;
        }
    } else {
        OCFErrorRef error = context.error;
        if (outError != NULL) {
            if (error == NULL) {
                error = __CFPropertyListCreateError(
                    kOCFPropertyListReadCorruptError,
                    OF_STR("Unknown error parsing property list around line %d"),
                    lineNumberStrings(&context)
                );
            }
            *outError = error;
        } else if (error != NULL) {
            OCFRelease(error);
        }
    }
    if (characters != NULL) {
        OCFRelease(string);
    } else {
        OCFAllocatorDeallocate(allocator, buffer);
    }
    if (context.stringSet != NULL) {
        OCFRelease(context.stringSet);
    }
    return result;
}

// Native advanceToNonSpace, iOS Simulator 24A434, 0x1c02d4e94, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external; callers test bit 0 or the whole of W0.
// iOS Device 24A446: 0x180702bac, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// reorders the comment blocks; the comparisons, bounds and stores agree.
// Skips U+0009..U+000D, U+2028 and U+2029 (SUB #9 / CMP #5, ADD #-0x2028 / CCMP #2), the space U+0020,
// "//" comments up to (not past) LF, CR, U+2028 or U+2029, and "/*" comments
// past the closing "*/". Returns true with curr at the first other unit, including a final lone '/'.
// Returns false with curr at the stop position when the end is reached; when curr is already at or
// beyond end it returns false without a store. An unterminated "/*" stops at end.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
bool advanceToNonSpace(OldStylePlistParseContext *context) {
    const uint16_t *p = context->curr;
    const uint16_t *end = context->end;
    if (p >= end) {
        return false;
    }
    do {
        uint16_t character = *p;
        if ((unsigned)(character - 9) < 5 || (unsigned)(character - 0x2028) < 2 || character == ' ') {
            ++p;
            continue;
        }
        if (character != '/' || p + 1 >= end) {
            context->curr = p;
            return true;
        }
        if (p[1] == '*') {
            const uint16_t *scan = p + 2;
            p = scan;
            if (scan >= end) {
                continue;
            }
            const uint16_t *next = scan;
            do {
                uint16_t commentCharacter = *next++;
                if (commentCharacter == '*' && next < end && *next == '/') {
                    p = scan + 2;
                    break;
                }
                scan = next;
                p = next;
            } while (next < end);
        } else if (p[1] == '/') {
            p += 2;
            while (p < end && !((unsigned)(*p - 0x2028) < 2 || *p == '\n' || *p == '\r')) {
                ++p;
            }
        } else {
            context->curr = p;
            return true;
        }
    } while (p < end);
    context->curr = p;
    return false;
}

// Native parsePlistObject, 0x1c02d4f8c. Non-external.
// A depth above 512 (CMP W2, #0x201 / B.LO, unsigned) fails before any input is read. At the end of input it
// returns NULL, with "Unexpected EOF while parsing plist" only when requireObject is set. The first unit is
// consumed: '{' parses dictionary content at the same depth and requires '}'; '"' and '\'' tail-call
// parseQuotedPlistString with that quote; '(' parses elements at depth + 1 (the first optional, later ones
// after
// ','; a NULL element ends the list) and requires ')'; '<' parses hex data. Any other unit is unconsumed
// again: a valid unquoted-string character tail-calls parseUnquotedPlistString; otherwise the result is NULL,
// with "Unexpected character '0x%x' at line %d" only when requireObject is set. A completed array or data
// object clears the pending error. The data loop collects up to 400 bytes in a stack buffer before each
// CFDataAppendBytes; it skips TAB, LF, CR, space, U+2028 and U+2029 between byte pairs, accepts ASCII hex
// digits of either case (the high digit through SUB/AND #0xff range tests, as below), and stops at '>' or the
// end; a pair cut by '>', the end or a non-hex unit fails as uneven, any other unit as invalid hex. The
// native flush also tests the byte count for the inlined helper's negative results (-2 uneven, otherwise
// invalid hex); the count is never negative on these paths, so that test is not reproduced.
static OCFTypeRef parsePlistObject(OldStylePlistParseContext *context, bool requireObject, uint32_t depth) {
    if (depth > 0x200) {
        replaceParseError(
            context,
            __CFPropertyListCreateError(
                kOCFPropertyListReadCorruptError,
                OF_STR("Too many nested arrays or dictionaries at line %d"),
                lineNumberStrings(context)
            )
        );
        return NULL;
    }
    if (!advanceToNonSpace(context)) {
        if (requireObject) {
            replaceParseError(
                context,
                __CFPropertyListCreateError(
                    kOCFPropertyListReadCorruptError, OF_STR("Unexpected EOF while parsing plist")
                )
            );
        }
        return NULL;
    }
    const uint16_t *start = context->curr;
    UniChar character = *start;
    context->curr = start + 1;
    switch (character) {
        case '{': {
            OCFMutableDictionaryRef dictionary = parsePlistDictContent(context, depth);
            if (dictionary == NULL) {
                return NULL;
            }
            if (advanceToNonSpace(context) && *context->curr == '}') {
                context->curr += 1;
                return dictionary;
            }
            OCFRelease(dictionary);
            replaceParseError(
                context,
                __CFPropertyListCreateError(
                    kOCFPropertyListReadCorruptError,
                    OF_STR("Expected terminating '}' for dictionary at line %d"),
                    lineNumberStrings(context)
                )
            );
            return NULL;
        }
        case '"':
        case '\'':
            return parseQuotedPlistString(context, character);
        case '(': {
            OCFMutableArrayRef array = OCFArrayCreateMutable(context->allocator, 0, &kOCFTypeArrayCallBacks);
            if (array == NULL) {
                replaceParseError(
                    context,
                    __CFPropertyListCreateError(
                        kOCFPropertyListReadCorruptError,
                        OF_STR("Unable to allocate array string while parsing plist")
                    )
                );
                return NULL;
            }
            OCFTypeRef element = parsePlistObject(context, false, depth + 1);
            while (element != NULL) {
                OCFArrayAppendValue(array, element);
                OCFRelease(element);
                if (!advanceToNonSpace(context)) {
                    OCFRelease(array);
                    replaceParseError(
                        context,
                        __CFPropertyListCreateError(
                            kOCFPropertyListReadCorruptError,
                            OF_STR("Expected ',' for array at line %d"),
                            lineNumberStrings(context)
                        )
                    );
                    return NULL;
                }
                if (*context->curr != ',') {
                    break;
                }
                context->curr += 1;
                element = parsePlistObject(context, false, depth + 1);
            }
            if (!advanceToNonSpace(context) || *context->curr != ')') {
                OCFRelease(array);
                replaceParseError(
                    context,
                    __CFPropertyListCreateError(
                        kOCFPropertyListReadCorruptError,
                        OF_STR("Expected terminating ')' for array at line %d"),
                        lineNumberStrings(context)
                    )
                );
                return NULL;
            }
            clearParseError(context);
            context->curr += 1;
            return array;
        }
        case '<': {
            OCFMutableDataRef data = OCFDataCreateMutable(context->allocator, 0);
            if (data == NULL) {
                replaceParseError(
                    context,
                    __CFPropertyListCreateError(
                        kOCFPropertyListReadCorruptError,
                        OF_STR("Unable to allocate data while parsing property list at line %d"),
                        lineNumberStrings(context)
                    )
                );
                return NULL;
            }
            uint8_t bytes[400];
            while (context->curr < context->end) {
                int32_t count = 0;
                const uint16_t *p = context->curr;
                const uint16_t *end = context->end;
                do {
                    UniChar unit = *p;
                    if (unit == '>') {
                        break;
                    }
                    uint32_t high = 0;
                    bool isDigit = true;
                    if ((uint8_t)(unit - '0') < 10) {
                        high = (uint32_t)unit - '0';
                    } else if ((uint8_t)(unit - 'a') <= 5) {
                        high = (uint32_t)unit - 0x57;
                    } else if ((uint8_t)(unit - 'G') >= 0xfa) {
                        high = (uint32_t)unit - 0x37;
                    } else {
                        isDigit = false;
                        bool isSpace = unit <= 0x20 && ((UINT64_C(1) << unit) & UINT64_C(0x100002600)) != 0;
                        if (!isSpace && (uint32_t)(unit - 0x2028) >= 2) {
                            OCFRelease(data);
                            replaceParseError(
                                context,
                                __CFPropertyListCreateError(
                                    kOCFPropertyListReadCorruptError,
                                    OF_STR("Malformed data byte group at line %d; invalid hex"),
                                    lineNumberStrings(context)
                                )
                            );
                            return NULL;
                        }
                    }
                    if (isDigit) {
                        p += 1;
                        context->curr = p;
                        uint32_t low = 0;
                        bool lowIsDigit = false;
                        if (p < end) {
                            UniChar lowUnit = *p;
                            lowIsDigit = true;
                            if ((uint8_t)(lowUnit - '0') < 10) {
                                low = (uint32_t)lowUnit - '0';
                            } else if ((uint8_t)(lowUnit - 'a') <= 5) {
                                low = (uint32_t)lowUnit - 0x57;
                            } else if ((uint8_t)(lowUnit - 'G') >= 0xfa) {
                                low = (uint32_t)lowUnit - 0x37;
                            } else {
                                lowIsDigit = false;
                            }
                        }
                        if (!lowIsDigit) {
                            OCFRelease(data);
                            replaceParseError(
                                context,
                                __CFPropertyListCreateError(
                                    kOCFPropertyListReadCorruptError,
                                    OF_STR("Malformed data byte group at line %d; uneven length"),
                                    lineNumberStrings(context)
                                )
                            );
                            return NULL;
                        }
                        bytes[count] = (uint8_t)(low + (high << 4));
                        count += 1;
                    }
                    p += 1;
                    context->curr = p;
                } while (p < end && count < 400);
                if (count == 0) {
                    break;
                }
                OCFDataAppendBytes(data, bytes, count);
            }
            clearParseError(context);
            if (context->curr < context->end && *context->curr == '>') {
                context->curr += 1;
                return data;
            }
            OCFRelease(data);
            replaceParseError(
                context,
                __CFPropertyListCreateError(
                    kOCFPropertyListReadCorruptError,
                    OF_STR("Expected terminating '>' for data at line %d"),
                    lineNumberStrings(context)
                )
            );
            return NULL;
        }
        default:
            break;
    }
    context->curr = start;
    if (isValidUnquotedStringCharacter(character)) {
        return parseUnquotedPlistString(context);
    }
    if (requireObject) {
        replaceParseError(
            context,
            __CFPropertyListCreateError(
                kOCFPropertyListReadCorruptError,
                OF_STR("Unexpected character '0x%x' at line %d"),
                character,
                lineNumberStrings(context)
            )
        );
    }
    return NULL;
}

// Native lineNumberStrings, iOS Simulator 24A434, 0x1c02d54e0, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. Non-external; callers pass the context to error-message formats with %d.
// iOS Device 24A446: 0x18070d188, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// merges the two CR-LF bound tests into one CCMP; the results agree.
// The UTF-16 form of lineNumber: counts lines from begin up to (not including) curr within end; LF and
// CR each start a line and CR followed by LF inside both bounds counts once. The count starts at 1 and is
// computed in 32 bits.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
uint32_t lineNumberStrings(const OldStylePlistParseContext *context) {
    const uint16_t *p = context->begin;
    const uint16_t *end = context->end;
    uint32_t count = 1;
    while (p < end && p < context->curr) {
        if (*p == '\n') {
            ++count;
        } else if (*p == '\r') {
            ++count;
            if (p + 1 < end && p + 1 < context->curr && p[1] == '\n') {
                ++p;
            }
        }
        ++p;
    }
    return count;
}

// Native parsePlistDictContent, 0x1c02d5558. Non-external.
// Reads key-value pairs until parsePlistString finds no key, then clears the pending error. After a key, '='
// consumes the unit and parses a required value at depth + 1; ';' is left in place and the value is the key
// itself (retained). Every pair must be followed by ';'. A missing value releases the key and the dictionary
// and returns NULL without a new error; the other failures replace the pending error.
static OCFMutableDictionaryRef parsePlistDictContent(OldStylePlistParseContext *context, uint32_t depth) {
    OCFMutableDictionaryRef dictionary = OCFDictionaryCreateMutable(
        context->allocator, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    if (dictionary == NULL) {
        replaceParseError(
            context,
            __CFPropertyListCreateError(
                kOCFPropertyListReadCorruptError,
                OF_STR("Unable to allocate dictionary while parsing plist at line %d"),
                lineNumberStrings(context)
            )
        );
        return NULL;
    }
    OCFStringRef key = parsePlistString(context);
    while (key != NULL) {
        if (!advanceToNonSpace(context)) {
            replaceParseError(
                context,
                __CFPropertyListCreateError(
                    kOCFPropertyListReadCorruptError,
                    OF_STR("Missing ';' on line %d"),
                    lineNumberStrings(context)
                )
            );
            OCFRelease(key);
            OCFRelease(dictionary);
            return NULL;
        }
        const uint16_t *p = context->curr;
        OCFTypeRef value = NULL;
        if (*p == '=') {
            context->curr = p + 1;
            value = parsePlistObject(context, true, depth + 1);
            if (value == NULL) {
                OCFRelease(key);
                OCFRelease(dictionary);
                return NULL;
            }
        } else if (*p == ';') {
            value = OCFRetain(key);
        } else {
            replaceParseError(
                context,
                __CFPropertyListCreateError(
                    kOCFPropertyListReadCorruptError,
                    OF_STR("Expected ';' or '=' after key at line %d"),
                    lineNumberStrings(context)
                )
            );
            OCFRelease(key);
            OCFRelease(dictionary);
            return NULL;
        }
        OCFDictionarySetValue(dictionary, key, value);
        OCFRelease(key);
        if (value != NULL) {
            OCFRelease(value);
        }
        if (!advanceToNonSpace(context) || *context->curr != ';') {
            replaceParseError(
                context,
                __CFPropertyListCreateError(
                    kOCFPropertyListReadCorruptError,
                    OF_STR("Missing ';' on line %d"),
                    lineNumberStrings(context)
                )
            );
            OCFRelease(dictionary);
            return NULL;
        }
        context->curr += 1;
        key = parsePlistString(context);
    }
    clearParseError(context);
    return dictionary;
}

// Native parseQuotedPlistString, 0x1c02d5748. Non-external. Called with curr after the opening quote.
// Runs of plain units are appended lazily: the mutable string is created at the first backslash. Escapes: \a
// \b \f \n \r \t \v, an escaped LF or '"' (themselves), up to three octal digits converted from NeXTSTEP
// Latin (CFStringEncodingBytesToUnicode with 0xb01, flags 0, one byte, one unit; any other result fails), \U
// with up to four hex digits, and any other unit taken literally. The hex test is the ctype inline for units
// below 0x80: _DefaultRuneLocale.__runetype[unit] & _CTYPE_X (offset 0x3c, bit 0x10000); the digit value
// subtracts 0x30 below ':', 0x37 below 'G' and 0x57 otherwise; a \U without a first hex digit fails, any
// other value (including 0 from \U0000) is appended as one unit. A backslash as the last unit (curr + 1 ==
// end, an equality test) fails. Reaching the end without the quote restores curr to the opening position and
// fails. With mutabilityOption 2 the result is a new mutable string; otherwise a string with escapes is
// uniqued through the context's set (CFSetGetValue, else __CFStringCollectionCopy, CFSetAddValue and
// __CFTypeCollectionRelease, then CFRetain) and a string without escapes comes from
// _uniqueStringForCharacters. The closing quote is consumed and the pending error cleared on every path that
// reaches the end, including a failed __CFStringCollectionCopy, whose new error is therefore cleared and
// whose result is NULL (native behavior).
static OCFStringRef parseQuotedPlistString(OldStylePlistParseContext *context, UniChar quote) {
    const uint16_t *start = context->curr;
    const uint16_t *end = context->end;
    if (start >= end) {
        context->curr = start;
        replaceParseError(
            context,
            __CFPropertyListCreateError(
                kOCFPropertyListReadCorruptError,
                OF_STR("Unterminated quoted string starting on line %d"),
                lineNumberStrings(context)
            )
        );
        return NULL;
    }
    OCFMutableStringRef string = NULL;
    const uint16_t *mark = start;
    const uint16_t *p = start;
    do {
        UniChar unit = *p;
        if (unit == quote) {
            break;
        }
        if (unit != '\\') {
            p += 1;
            context->curr = p;
            continue;
        }
        if (string == NULL) {
            string = OCFStringCreateMutable(context->allocator, 0);
            if (string == NULL) {
                replaceParseError(
                    context,
                    __CFPropertyListCreateError(
                        kOCFPropertyListReadCorruptError,
                        OF_STR("Unable to allocate quoted string while parsing plist")
                    )
                );
                return NULL;
            }
            p = context->curr;
        }
        OCFStringAppendCharacters(string, mark, (p - mark));
        p = context->curr;
        end = context->end;
        context->curr = p + 1;
        if (p + 1 == end) {
            OCFRelease(string);
            replaceParseError(
                context,
                __CFPropertyListCreateError(
                    kOCFPropertyListReadCorruptError,
                    OF_STR("Unterminated backslash sequence on line %d"),
                    lineNumberStrings(context)
                )
            );
            return NULL;
        }
        UniChar escaped = p[1];
        p += 2;
        context->curr = p;
        UniChar character = escaped;
        switch (escaped) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7': {
                uint32_t number = (uint32_t)escaped - '0';
                UniChar converted = 0;
                OCFIndex usedCharLen = 0;
                for (int digit = 0; digit < 2 && p < end; ++digit) {
                    UniChar octal = *p++;
                    if ((octal & 0xfff8) != 0x30) {
                        break;
                    }
                    number = (number << 3) + octal - 0x30;
                    context->curr = p;
                }
                uint8_t byte = (uint8_t)number;
                uint32_t status =
                    OCFStringEncodingBytesToUnicode(0xb01, 0, &byte, 1, NULL, &converted, 1, &usedCharLen);
                if (status != 0 || usedCharLen != 1) {
                    replaceParseError(
                        context,
                        __CFPropertyListCreateError(
                            kOCFPropertyListReadCorruptError,
                            OF_STR("Unable to convert octet-stream while parsing plist")
                        )
                    );
                    OCFRelease(string);
                    return NULL;
                }
                character = converted;
                break;
            }
            case '\n':
                character = '\n';
                break;
            case '"':
                character = '"';
                break;
            case 'U': {
                if (p >= end) {
                    replaceParseError(
                        context,
                        __CFPropertyListCreateError(
                            kOCFPropertyListReadCorruptError,
                            OF_STR(
                                "Unexpected end of file while parsing unicode character escape sequence on "
                                "line %d"
                            ),
                            lineNumberStrings(context)
                        )
                    );
                    OCFRelease(string);
                    return NULL;
                }
                uint32_t number = 0;
                uint32_t remaining = 4;
                uint32_t left = 4;
                bool more = true;
                for (;;) {
                    UniChar hex = *p;
                    if (hex > 0x7f || (_DefaultRuneLocale.__runetype[hex] & _CTYPE_X) == 0) {
                        more = true;
                        left = remaining;
                        break;
                    }
                    p += 1;
                    context->curr = p;
                    int32_t adjustment = hex < 0x3a ? -0x30 : (hex < 0x47 ? -0x37 : -0x57);
                    number = hex + (number << 4) + (uint32_t)adjustment;
                    left = remaining - 1;
                    more = p < end;
                    if (!more || remaining == 1) {
                        break;
                    }
                    remaining = left;
                }
                if (number == 0 && left == 4) {
                    if (more) {
                        replaceParseError(
                            context,
                            __CFPropertyListCreateError(
                                kOCFPropertyListReadCorruptError,
                                OF_STR(
                                    "Unexpected character `%c` while parsing unicode character escape "
                                    "sequence on "
                                    "line %d"
                                ),
                                *p,
                                lineNumberStrings(context)
                            )
                        );
                    } else {
                        replaceParseError(
                            context,
                            __CFPropertyListCreateError(
                                kOCFPropertyListReadCorruptError,
                                OF_STR(
                                    "Unexpected end of file while parsing unicode character escape sequence "
                                    "on "
                                    "line %d"
                                ),
                                lineNumberStrings(context)
                            )
                        );
                    }
                    OCFRelease(string);
                    return NULL;
                }
                character = (UniChar)number;
                break;
            }
            case 'a':
                character = 7;
                break;
            case 'b':
                character = 8;
                break;
            case 'f':
                character = 12;
                break;
            case 'n':
                character = 10;
                break;
            case 'r':
                character = 13;
                break;
            case 't':
                character = 9;
                break;
            case 'v':
                character = 11;
                break;
            default:
                break;
        }
        OCFStringAppendCharacters(string, &character, 1);
        p = context->curr;
        end = context->end;
        mark = p;
    } while (p < end);
    if (end <= p) {
        if (string != NULL) {
            OCFRelease(string);
        }
        context->curr = start;
        replaceParseError(
            context,
            __CFPropertyListCreateError(
                kOCFPropertyListReadCorruptError,
                OF_STR("Unterminated quoted string starting on line %d"),
                lineNumberStrings(context)
            )
        );
        return NULL;
    }
    OCFStringRef result = NULL;
    if (string == NULL) {
        if (context->mutabilityOption == kOCFPropertyListMutableContainersAndLeaves) {
            string = OCFStringCreateMutable(context->allocator, 0);
            if (string == NULL) {
                replaceParseError(
                    context,
                    __CFPropertyListCreateError(
                        kOCFPropertyListReadCorruptError,
                        OF_STR("Unable to allocate mutable string while parsing plist")
                    )
                );
                return NULL;
            }
            if (mark != context->curr) {
                OCFStringAppendCharacters(string, mark, (context->curr - mark));
            }
            result = string;
        } else {
            result = _uniqueStringForCharacters(context, mark, (p - mark));
        }
    } else {
        if (mark != p) {
            OCFStringAppendCharacters(string, mark, (p - mark));
        }
        if (context->mutabilityOption == kOCFPropertyListMutableContainersAndLeaves) {
            result = string;
        } else {
            OCFStringRef unique = OCFSetGetValue(context->stringSet, string);
            bool retain = true;
            if (unique == NULL) {
                unique = __CFStringCollectionCopy(context->allocator, string);
                if (unique == NULL) {
                    replaceParseError(
                        context,
                        __CFPropertyListCreateError(
                            kOCFPropertyListReadCorruptError,
                            OF_STR("Unable to copy string while parsing plist")
                        )
                    );
                    retain = false;
                } else {
                    OCFSetAddValue(context->stringSet, unique);
                    __CFTypeCollectionRelease(context->allocator, unique);
                }
            }
            if (retain) {
                OCFRetain(unique);
            }
            OCFRelease(string);
            result = unique;
        }
    }
    context->curr += 1;
    clearParseError(context);
    return result;
}

// Native parseUnquotedPlistString, 0x1c02d5c80. Non-external.
// Consumes the valid unquoted-string characters from curr (storing curr after each). None at all, or curr at
// the end, fails with "Unexpected EOF". With mutabilityOption 2 the result is a new mutable string; otherwise
// _uniqueStringForCharacters is tail-called with the run.
static OCFStringRef parseUnquotedPlistString(OldStylePlistParseContext *context) {
    const uint16_t *mark = context->curr;
    const uint16_t *end = context->end;
    if (mark < end) {
        const uint16_t *p = mark;
        while (isValidUnquotedStringCharacter(*p)) {
            p += 1;
            context->curr = p;
            if (p >= end) {
                break;
            }
        }
        if (p != mark) {
            if (context->mutabilityOption != kOCFPropertyListMutableContainersAndLeaves) {
                return _uniqueStringForCharacters(context, mark, (p - mark));
            }
            OCFMutableStringRef string = OCFStringCreateMutable(context->allocator, 0);
            if (string != NULL) {
                OCFStringAppendCharacters(string, mark, (context->curr - mark));
                return string;
            }
            replaceParseError(
                context,
                __CFPropertyListCreateError(
                    kOCFPropertyListReadCorruptError,
                    OF_STR("Unable to allocate unquoted string while parsing plist")
                )
            );
            return NULL;
        }
    }
    replaceParseError(
        context, __CFPropertyListCreateError(kOCFPropertyListReadCorruptError, OF_STR("Unexpected EOF"))
    );
    return NULL;
}

// Native _uniqueStringForCharacters, 0x1c02d5dbc. Non-external. An empty run answers the retained constant
// empty string. Otherwise ASCII units are narrowed into a buffer of length + 1 bytes (on the stack below 2048
// units, else from the system default allocator with type 0x100004077774924) and become a string through
// CFStringCreateWithBytes (ASCII, the context's allocator); the first non-ASCII unit makes the string from
// the units instead (CFStringCreateWithCharacters). A heap buffer is freed after a successful creation. The
// string is uniqued through the context's string set (the equal member when there is one, else it is added)
// and the result is retained. Failures release any earlier error and store kCFPropertyListReadCorruptError
// "Unable to allocate string while parsing plist" (no buffer), "... pre-unique string ..." or "... ascii
// string ..."; the answer is then NULL.
// CoreFoundation defect: a heap buffer is not freed when the string creation fails (iOS Simulator 24A434,
// 0x1c02d5f5c and 0x1c02d5ed8 branch past the deallocation; confidence: verified). The leak is kept: it is
// well-defined and limited to allocation failure.
OCFStringRef
_uniqueStringForCharacters(OldStylePlistParseContext *context, const UniChar *base, OCFIndex length) {
    if (length == 0) {
        return OCFRetain(kOCFEmptyString);
    }
    size_t stackLength = length < 0x800 ? (size_t)length + 1 : 1;
    char stackBuffer[stackLength];
    char *buffer = stackBuffer;
    OCFStringRef message;
    OCFStringRef string;
    if (length >= 0x800) {
        buffer = OCFAllocatorAllocateTyped(kOCFAllocatorSystemDefault, length + 1, 0x100004077774924, 0);
        if (buffer == NULL) {
            message = OF_STR("Unable to allocate string while parsing plist");
            goto failure;
        }
    }
    for (OCFIndex idx = 0; idx != length; ++idx) {
        UniChar character = base[idx];
        if (character >= 0x80) {
            string = OCFStringCreateWithCharacters(context->allocator, base, length);
            if (string == NULL) {
                message = OF_STR("Unable to allocate pre-unique string while parsing plist");
                goto failure;
            }
            goto created;
        }
        buffer[idx] = (char)character;
    }
    buffer[length] = 0;
    string = OCFStringCreateWithBytes(context->allocator, (const UInt8 *)buffer, length, 0x600, false);
    if (string == NULL) {
        message = OF_STR("Unable to allocate ascii string while parsing plist");
        goto failure;
    }
created:
    if (buffer != stackBuffer) {
        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, buffer);
    }
    OCFStringRef unique = OCFSetGetValue(context->stringSet, string);
    if (unique == NULL) {
        OCFSetAddValue(context->stringSet, string);
        unique = string;
    }
    OCFRelease(string);
    return OCFRetain(unique);
failure:
    {
        OCFErrorRef error = __CFPropertyListCreateError(0xF00, message);
        if (context->error != NULL) {
            OCFRelease(context->error);
        }
        context->error = error;
        return NULL;
    }
}

// Native parsePlistString, 0x1c02d5fc0. Non-external.
// Skips space; a quote is consumed and parseQuotedPlistString tail-called with it; a valid unquoted-string
// character tail-calls parseUnquotedPlistString; anything else, or the end of input, returns NULL without an
// error.
static OCFStringRef parsePlistString(OldStylePlistParseContext *context) {
    if (!advanceToNonSpace(context)) {
        return NULL;
    }
    UniChar character = *context->curr;
    if (character == '\'' || character == '"') {
        context->curr += 1;
        return parseQuotedPlistString(context, character);
    }
    if (isValidUnquotedStringCharacter(character)) {
        return parseUnquotedPlistString(context);
    }
    return NULL;
}

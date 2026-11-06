//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/String/OCFStringScanner.h"

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"

#include <stdlib.h>
#include <string.h>
#include <xlocale.h>

// Inferred organization: __CFStringScanInteger (0x1c022e34c) and __CFStringScanDouble (0x1c022ea4c) follow
// the byte-stream unit in the iOS Simulator __text section and form the native string-scanner unit; the
// native source path is not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationString.md.

// Native __CFNumberSet, 0x1c03a34d4 (__TEXT,__const, non-external): a bitmap of the characters a double may
// contain ('+', '-', '.', '0'-'9', 'E', 'e'), indexed by character >> 3 and bit character & 7. Sixteen bytes
// are the table; the four zero bytes before the next symbol (0x1c03a34e8) are padding and are not reached
// (only characters below 0x80 are looked up).
static const uint8_t __CFNumberSet[16] = {
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x68,
    0xff,
    0x03,
    0x20,
    0x00,
    0x00,
    0x00,
    0x20,
    0x00,
    0x00,
    0x00,
};

// The character accessor both scanners inline (the SDK inline-buffer accessor with 0xffff, not 0, outside the
// range): a negative index or one at or beyond rangeToBuffer.length answers 0xffff; a refill starts four
// characters back (zero when the index is below 4) and ends at most 64 characters later.
static inline UniChar scannerCharacterAtIndex(OCFStringInlineBuffer *buf, OCFIndex idx) {
    if (idx < 0 || buf->rangeToBuffer.length <= idx) {
        return 0xffff;
    }
    if (buf->directUniCharBuffer != NULL) {
        return buf->directUniCharBuffer[idx + buf->rangeToBuffer.location];
    }
    if (buf->directCStringBuffer != NULL) {
        return (UniChar)buf->directCStringBuffer[idx + buf->rangeToBuffer.location];
    }
    if (buf->bufferedRangeEnd <= idx || buf->bufferedRangeStart > idx) {
        OCFIndex start = (uint32_t)idx < 4 ? 0 : idx - 4;
        OCFIndex end = (uint64_t)(start + 64) < (uint64_t)buf->rangeToBuffer.length
                           ? start + 64
                           : buf->rangeToBuffer.length;
        buf->bufferedRangeStart = start;
        buf->bufferedRangeEnd = end;
        OCFStringGetCharacters(
            buf->theString, (OCFRange){buf->rangeToBuffer.location + start, end - start}, buf->buffer
        );
    }
    return buf->buffer[idx - buf->bufferedRangeStart];
}

// The scanners' whitespace: below 0x21, 0x7f-0xa0, 0x2000-0x200b and 0x3000 (0xffff is not whitespace).
static inline bool scannerIsWhitespace(UniChar ch) {
    return ch == 0x3000 || (UniChar)(ch - 0x200c) > 0xfff3 || ch < 0x21 || (UniChar)(ch - 0xa1) >= 0xffde;
}

static inline bool scannerIsDigit(UniChar ch) {
    return (UniChar)(ch - ':') >= 0xfff6;
}

// ++(*indexPtr) as the native code computes it: the sign-extended index plus one is both stored (low 32 bits)
// and used, 64-bit, to read the next character.
static inline UniChar scannerAdvance(OCFStringInlineBuffer *buf, SInt32 *indexPtr) {
    OCFIndex next = (OCFIndex)*indexPtr + 1;
    *indexPtr = (SInt32)next;
    return scannerCharacterAtIndex(buf, next);
}

// Native __CFStringScanInteger, 0x1c022e34c. Hidden (was private external). locale is not read. Skips
// whitespace (each step reloads *indexPtr and adds one in 32 bits), then an optional sign followed by more
// whitespace, and needs a decimal digit. Digits accumulate in 32 bits; past 214748364 (or at it with a final
// digit, less one when negative, of 7 or more) a 64-bit scan continues when doLonglong is set and otherwise
// the result saturates to INT_MAX or INT_MIN while the remaining digits are skipped; the 64-bit accumulation
// saturates the same way at 0x0ccccccccccccccc. A saturated result is stored without negation. The 64-bit
// accumulation adds the character minus '0' directly; the 32-bit one adds -1 for a non-digit (never reached).
Boolean __CFStringScanInteger(
    OCFStringInlineBuffer *buf, OCFTypeRef locale, SInt32 *indexPtr, Boolean doLonglong, void *result
) {
    (void)locale;
    SInt32 index = *indexPtr;
    UniChar ch;
    for (;;) {
        ch = scannerCharacterAtIndex(buf, index);
        if (!scannerIsWhitespace(ch)) {
            break;
        }
        index = (SInt32)((uint32_t)*indexPtr + 1);
        *indexPtr = index;
    }
    bool neg = false;
    if (ch == '-' || ch == '+') {
        UniChar next;
        do {
            next = scannerAdvance(buf, indexPtr);
        } while (scannerIsWhitespace(next));
        neg = ch == '-';
        ch = next;
    }
    if (!scannerIsDigit(ch)) {
        return false;
    }
    bool doingLonglong = false;
    int32_t intResult = 0;
    int64_t longlongResult = 0;
    bool useInt = true;
    for (;;) {
        if (doingLonglong) {
            if (longlongResult > 0x0ccccccccccccccb &&
                (longlongResult != 0x0ccccccccccccccc || (int32_t)ch - (int32_t)neg - '0' >= 7)) {
                longlongResult = neg ? INT64_MIN : INT64_MAX;
                do {
                    ch = scannerAdvance(buf, indexPtr);
                } while (scannerIsDigit(ch));
                neg = false;
                useInt = false;
                break;
            }
            longlongResult = (int64_t)((uint64_t)longlongResult * 10 + ch - '0');
            ch = scannerAdvance(buf, indexPtr);
            useInt = false;
        } else {
            int32_t digit = (UniChar)(ch - '0') < 10 ? (int32_t)ch - '0' : -1;
            if (intResult > 214748363 && (intResult != 214748364 || digit - (int32_t)neg >= 7)) {
                if (doLonglong) {
                    longlongResult = (uint32_t)intResult;
                    doingLonglong = true;
                    continue;
                }
                intResult = neg ? INT32_MIN : INT32_MAX;
                do {
                    ch = scannerAdvance(buf, indexPtr);
                } while (scannerIsDigit(ch));
                neg = false;
                useInt = true;
                break;
            }
            intResult = (int32_t)((uint32_t)intResult * 10 + (uint32_t)digit);
            ch = scannerAdvance(buf, indexPtr);
            useInt = true;
        }
        if (!scannerIsDigit(ch)) {
            break;
        }
    }
    if (result != NULL) {
        if (doLonglong) {
            int64_t value = useInt ? (int64_t)intResult : longlongResult;
            if (neg) {
                value = (int64_t)(0 - (uint64_t)value);
            }
            *(int64_t *)result = value;
        } else {
            *(int32_t *)result = neg ? (int32_t)(0 - (uint32_t)intResult) : intResult;
        }
    }
    return true;
}

// Native __CFStringScanDouble, 0x1c022ea4c. Hidden (was private external). locale is not read. Skips
// whitespace as __CFStringScanInteger does, copies the following characters below 0x80 that are in
// __CFNumberSet into a 256-byte stack buffer (grown by 256 bytes at a time from the default allocator, malloc
// type ID 0x100004077774924; the growth result is not checked, as natively), and parses them with strtod_l
// (NULL locale). *indexPtr is not advanced while copying; it advances by the parsed length, and nothing is
// stored when strtod_l parses nothing. A first character of 0x80 or above parses an empty string. The fourth
// argument of __CFSafelyReallocateWithAllocatorTyped receives the malloc type ID and the fifth 0, as at every
// native call site (OCFRuntime.h names them hint and typeID).
Boolean
__CFStringScanDouble(OCFStringInlineBuffer *buf, OCFTypeRef locale, SInt32 *indexPtr, double *result) {
    (void)locale;
    char localCharacters[256];
    char *charPtr = localCharacters;
    char *endCharPtr = NULL;
    OCFAllocatorRef tmpAlloc = NULL;
    double value;
    SInt32 index = *indexPtr;
    UniChar ch;
    for (;;) {
        ch = scannerCharacterAtIndex(buf, index);
        if (!scannerIsWhitespace(ch)) {
            break;
        }
        index = (SInt32)((uint32_t)*indexPtr + 1);
        *indexPtr = index;
    }
    if (ch >= 0x80) {
        localCharacters[0] = 0;
        value = strtod_l(localCharacters, &endCharPtr, NULL);
    } else {
        int32_t capacity = 0x100;
        OCFIndex numChars = 0;
        for (;;) {
            if (((__CFNumberSet[(ch >> 3) & 0x1fff] >> (ch & 7)) & 1) == 0) {
                break;
            }
            if (numChars >= (OCFIndex)(capacity - 1)) {
                if (tmpAlloc == NULL) {
                    tmpAlloc = _OCFGetTSD(1);
                    if (tmpAlloc == NULL) {
                        tmpAlloc = kOCFAllocatorSystemDefault;
                    }
                }
                capacity += 0x100;
                if (charPtr != localCharacters) {
                    charPtr = __OCFSafelyReallocateWithAllocatorTyped(
                        tmpAlloc, charPtr, capacity, 0x100004077774924, 0, NULL
                    );
                } else {
                    charPtr = OCFAllocatorAllocateTyped(tmpAlloc, capacity, 0x100004077774924, 0);
                    memmove(charPtr, localCharacters, numChars);
                }
            }
            charPtr[numChars] = (char)ch;
            SInt32 next = (SInt32)((uint32_t)*indexPtr + (uint32_t)numChars + 1);
            ch = scannerCharacterAtIndex(buf, next);
            ++numChars;
            if (ch >= 0x80) {
                break;
            }
        }
        charPtr[(uint32_t)numChars] = 0;
        value = strtod_l(charPtr, &endCharPtr, NULL);
        if (tmpAlloc != NULL) {
            OCFAllocatorDeallocate(tmpAlloc, charPtr);
        }
    }
    if (endCharPtr == charPtr) {
        return false;
    }
    *indexPtr = (SInt32)((uint32_t)*indexPtr + (uint32_t)(endCharPtr - charPtr));
    if (result != NULL) {
        *result = value;
    }
    return true;
}

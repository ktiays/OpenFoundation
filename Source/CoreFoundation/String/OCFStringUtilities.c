//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/String/OCFStringUtilities.h"

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/Unicode/OCFUniChar.h"

#include <string.h>

// Inferred organization: _CFCompareStringsWithLocale (0x1c02342d0), __extendLocationBackward (0x1c0234ee4),
// __CFStringCreateCollator (0x1c0235140), __CompareTextDefault (0x1c0235234), __extendLocationForward
// (0x1c0235744) and __collatorFinalize (0x1c0235a08) form the native string-utilities unit after the
// encoding-name functions (CFStringGetMostCompatibleMacStringEncoding, 0x1c02342cc) in the iOS Simulator
// __text section; the native source path is not in the evidence. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationString.md.
// Folds the compiler-outlined piece __CFCompareStringsWithLocale.cold.1 (0x1c030cd38).

// libsystem_platform's os_unfair_lock_lock_with_options (private SPI); every native call passes 0x10000.
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);

// ICU (libicucore), supplied. UCollator is opaque; UErrorCode, UColAttribute and UColAttributeValue are
// int-sized enumerations.
extern void *ucol_open(const char *loc, int32_t *status);
extern void ucol_setAttribute(void *coll, int32_t attr, int32_t value, int32_t *status);
extern void ucol_close(void *coll);
extern int32_t ucol_strcoll(
    const void *coll, const UniChar *source, int32_t sourceLength, const UniChar *target, int32_t targetLength
);
extern void *ucol_openElements(const void *coll, const UniChar *text, int32_t textLength, int32_t *status);
extern void ucol_closeElements(void *elements);
extern int32_t ucol_next(void *elements, int32_t *status);
extern int32_t ucol_getOffset(const void *elements);

// The unit's static functions, defined below in native order.
static OCFIndex __extendLocationBackward(
    OCFIndex location, OCFStringInlineBuffer *buffer, const uint8_t *nonBaseBMP, const uint8_t *punctBMP
);
static void *__CFStringCreateCollator(OCFLocaleRef compareLocale);
static int32_t __CompareTextDefault(
    void *collator,
    Boolean collatorKind,
    OCFOptionFlags options,
    const UniChar *text1,
    OCFIndex text1Length,
    const UniChar *text2,
    OCFIndex text2Length,
    Boolean *equivalent,
    int32_t *order
);
static OCFIndex __extendLocationForward(
    OCFIndex location,
    OCFStringInlineBuffer *buffer,
    const uint8_t *alnumBMP,
    const uint8_t *punctBMP,
    const uint8_t *controlBMP,
    OCFIndex strMax
);
static void __collatorFinalize(void *collator);

// Per-thread slots: 8 the cached collator's locale, 9 the cached collator (destructor __collatorFinalize), 17
// the options of the last comparison.
enum {
    kTSDCollatorLocaleSlot = 8,
    kTSDCollatorSlot = 9,
    kTSDCollatorOptionsSlot = 17,
};

// Native default-collator pool, 0x22e6d3268-0x22e6d32c0 (__DATA,__bss). Non-external:
// __CFDefaultCollatorLock,
// __CFDefaultCollatorLocale, __CFDefaultCollatorsCount and __CFDefaultCollators (eight entries;
// __collatorFinalize returns a collator to the pool while the count is at most 7).
static os_unfair_lock __CFDefaultCollatorLock = OS_UNFAIR_LOCK_INIT;
static OCFLocaleRef __CFDefaultCollatorLocale;
static OCFIndex __CFDefaultCollatorsCount;
static void *__CFDefaultCollators[8];

// Native _CFCompareStringsWithLocale.alnumBMP, nonBaseBMP, punctBMP and controlBMP, 0x22e6d3248-0x22e6d3260
// (__DATA,__bss). Non-external; filled on the first call (character sets 0xa, 8, 0xb and 0x6c, plane 0).
static const uint8_t *alnumBMP;
static const uint8_t *nonBaseBMP;
static const uint8_t *punctBMP;
static const uint8_t *controlBMP;

// The inlined refill of an inline buffer at start: the buffered range becomes [start, min(start + 64,
// length)), filled from the C-string pointer (sign-extended bytes) or CFStringGetCharacters; answers the
// buffered start.
static OCFIndex refillInlineBuffer(OCFStringInlineBuffer *buffer, OCFIndex start) {
    OCFIndex end = start + 64 < buffer->rangeToBuffer.length ? start + 64 : buffer->rangeToBuffer.length;
    buffer->bufferedRangeStart = start;
    buffer->bufferedRangeEnd = end;
    const char *direct = buffer->directCStringBuffer;
    OCFRange range = {buffer->rangeToBuffer.location + start, end - start};
    if (direct == NULL) {
        OCFStringGetCharacters(buffer->theString, range, buffer->buffer);
        return buffer->bufferedRangeStart;
    }
    if (range.length != 0) {
        const char *source = direct + range.location;
        UniChar *destination = buffer->buffer;
        for (OCFIndex count = start - end; count != 0; ++count) {
            *destination++ = (UniChar)(int8_t)*source++;
        }
    }
    return start;
}

// The inlined direct access to [start, end) of an inline buffer: the Unicode pointer when there is one, else
// the inline buffer for at most 64 characters (refilled when the range is not buffered); NULL when the range
// leaves the buffered string or is longer than 64 characters without a Unicode pointer.
static const UniChar *
inlineBufferRange(OCFStringInlineBuffer *buffer, OCFIndex start, OCFIndex end, OCFIndex length) {
    if (start < 0 || end > buffer->rangeToBuffer.length) {
        return NULL;
    }
    if (buffer->directUniCharBuffer != NULL) {
        return buffer->directUniCharBuffer + buffer->rangeToBuffer.location + start;
    }
    if (length > 64) {
        return NULL;
    }
    OCFIndex bufferedStart = buffer->bufferedRangeStart;
    if (end > buffer->bufferedRangeEnd || start < bufferedStart) {
        bufferedStart = refillInlineBuffer(buffer, start);
    }
    return buffer->buffer + (start - bufferedStart);
}

// The inlined copy of a cluster [start, end) into the 128-character stack buffer or the growing heap buffer
// (system default allocator, type ID 0x1000040BDFB0063, capacity rounded up to a multiple of 128 plus 128):
// from the Unicode pointer, else from the buffered part of the inline buffer and the string.
// CoreFoundation defect: _CFCompareStringsWithLocale, 0x1c0234804 and 0x1c02348cc (and 0x1c0234a54,
// 0x1c0234b18, 0x1c0234b30 for the second string); confidence: high. The native code stores the copy
// destination into the heap-buffer variable even when it is the stack buffer, so a cluster of 65-128
// characters after a heap buffer was allocated makes the final CFAllocatorDeallocate free the stack buffer
// and leak the heap buffer. The restored code records only heap destinations.
static UniChar *copyCluster(
    OCFStringInlineBuffer *buffer,
    OCFIndex start,
    OCFIndex end,
    OCFIndex length,
    UniChar *localBuffer,
    UniChar **heapBuffer,
    OCFIndex *heapCapacity
) {
    UniChar *characters;
    if (length < 0x81) {
        characters = localBuffer;
    } else if (*heapCapacity >= length) {
        characters = *heapBuffer;
    } else {
        OCFIndex capacity = (length & 0x7FFFFFFFFFFFFF80) + 0x80;
        if (*heapCapacity != 0) {
            characters = __OCFSafelyReallocateWithAllocatorTyped(
                kOCFAllocatorSystemDefault, *heapBuffer, capacity * 2, 0x1000040BDFB0063, 0, NULL
            );
        } else {
            characters =
                OCFAllocatorAllocateTyped(kOCFAllocatorSystemDefault, capacity * 2, 0x1000040BDFB0063, 0);
            if (characters == NULL) {
                __CFStringHandleOutOfMemory();
            }
        }
        *heapCapacity = capacity;
    }
    if (buffer->directUniCharBuffer != NULL) {
        memmove(
            characters,
            buffer->directUniCharBuffer + buffer->rangeToBuffer.location + start,
            (size_t)(length * 2)
        );
    } else {
        OCFIndex bufferedStart = buffer->bufferedRangeStart;
        OCFIndex offset = start - bufferedStart;
        UniChar *output = characters;
        OCFIndex next = start;
        OCFIndex remaining = length;
        OCFIndex available = buffer->bufferedRangeEnd - start;
        if (offset >= 0 && available > 0) {
            OCFIndex count = length < available ? length : available;
            memmove(characters, buffer->buffer + offset, (size_t)(count * 2));
            output = characters + count;
            next = count + start;
            remaining = length - count;
        } else {
            OCFIndex tail = end - bufferedStart;
            if (tail > 0 && end < buffer->bufferedRangeEnd) {
                remaining = bufferedStart - start;
                memmove(characters + remaining, buffer->buffer, (size_t)(tail * 2));
            }
        }
        if (remaining >= 1) {
            OCFIndex location = buffer->rangeToBuffer.location + next;
            const char *direct = buffer->directCStringBuffer;
            if (direct != NULL) {
                const char *source = direct + location;
                do {
                    *output++ = (UniChar)(int8_t)*source++;
                } while (--remaining != 0);
            } else {
                OCFRange range = {location, remaining};
                OCFStringGetCharacters(buffer->theString, range, output);
            }
        }
    }
    if (characters != localBuffer) {
        *heapBuffer = characters;
    }
    return characters;
}

// Native _CFCompareStringsWithLocale, 0x1c02342d0. Hidden (was private external). Facts read from the
// assembly:
// - Both starts are first moved back with __extendLocationBackward (location - 1, when the location is at
// least 1).
// - The collator: the per-thread one when its locale (slot 8) is compareLocale (collator kind 1); else, under
//   __CFDefaultCollatorLock, a pooled default collator when compareLocale is the default-collator locale or
//   the current locale (which then replaces the pool's locale, closing the pooled collators) (kind 0, or a
//   new collator when the pool is empty); else a new collator (kind 1) and slot 17 is cleared first.
// - When both whole ranges are directly accessible (see inlineBufferRange) they are compared at once:
//   __CompareTextDefault's order when it succeeds (0 for equivalent strings unless kCFCompareForcedOrdering),
//   else memcmp, whose zero answer gives kCFCompareGreaterThan regardless of the lengths (kept).
// - Otherwise the strings are compared cluster by cluster (__extendLocationForward); a memcmp answer is
// widened
//   away from zero, a shorter equal cluster ends the comparison, and an equivalent cluster records the forced
//   ordering once (kCFCompareForcedOrdering). A NULL cluster pointer is .cold.1 ("Invalid string range
//   produced for character buffer in _CFCompareStringsWithLocale.").
// - A collator other than the per-thread one becomes the per-thread collator (the previous one finalized
// through
//   __collatorFinalize when a cached locale existed), with a retained locale; slot 17 then records options.
OCFComparisonResult _CFCompareStringsWithLocale(
    OCFStringInlineBuffer *str1,
    OCFRange str1Range,
    OCFStringInlineBuffer *str2,
    OCFRange str2Range,
    OCFOptionFlags options,
    const void *compareLocale
) {
    OCFIndex location1 = str1Range.location;
    OCFIndex length1 = str1Range.length;
    OCFIndex location2 = str2Range.location;
    OCFIndex length2 = str2Range.length;
    OCFLocaleRef locale = compareLocale;
    int32_t order = 0;
    Boolean equivalent = false;
    if (alnumBMP == NULL) {
        alnumBMP = OCFUniCharGetBitmapPtrForPlane(0xa, 0);
        nonBaseBMP = OCFUniCharGetBitmapPtrForPlane(8, 0);
        punctBMP = OCFUniCharGetBitmapPtrForPlane(0xb, 0);
        controlBMP = OCFUniCharGetBitmapPtrForPlane(0x6c, 0);
    }
    OCFIndex start1 = location1;
    if (location1 - 1 >= 0) {
        start1 = __extendLocationBackward(location1 - 1, str1, nonBaseBMP, punctBMP);
    }
    OCFIndex start2 = location2;
    if (location2 - 1 >= 0) {
        start2 = __extendLocationBackward(location2 - 1, str2, nonBaseBMP, punctBMP);
    }
    void *cachedCollator = _OCFGetTSD(kTSDCollatorSlot);
    OCFLocaleRef cachedLocale = _OCFGetTSD(kTSDCollatorLocaleSlot);
    void *collator;
    Boolean collatorKind;
    if (cachedLocale == locale) {
        collatorKind = 1;
        collator = cachedCollator;
    } else {
        os_unfair_lock_lock_with_options(&__CFDefaultCollatorLock, 0x10000);
        bool useNewCollator = false;
        OCFLocaleRef current;
        bool keepCurrent;
        if (locale == __CFDefaultCollatorLocale) {
            current = NULL;
            keepCurrent = true;
        } else {
            current = OCFLocaleCopyCurrent();
            if (locale != current) {
                OCFRelease(current);
                os_unfair_lock_unlock(&__CFDefaultCollatorLock);
                useNewCollator = true;
            } else if (locale == NULL || __CFDefaultCollatorLocale == locale) {
                keepCurrent = locale == NULL;
            } else {
                OCFIndex count = __CFDefaultCollatorsCount;
                if (count >= 1) {
                    do {
                        __CFDefaultCollatorsCount = count - 1;
                        ucol_close(__CFDefaultCollators[count - 1]);
                        count = __CFDefaultCollatorsCount;
                    } while (count > 0);
                }
                if (__CFDefaultCollatorLocale != NULL) {
                    OCFRelease(__CFDefaultCollatorLocale);
                }
                keepCurrent = false;
                __CFDefaultCollatorLocale = OCFRetain(current);
            }
        }
        collator = NULL;
        if (!useNewCollator) {
            OCFIndex count = __CFDefaultCollatorsCount;
            if (count > 0) {
                --count;
                __CFDefaultCollatorsCount = count;
                collator = __CFDefaultCollators[count];
                os_unfair_lock_unlock(&__CFDefaultCollatorLock);
            } else {
                os_unfair_lock_unlock(&__CFDefaultCollatorLock);
            }
            if (collator == NULL) {
                collator = __CFStringCreateCollator(locale);
            }
            if (!keepCurrent) {
                OCFRelease(current);
            }
            useNewCollator = collator == NULL;
            collatorKind = 0;
        }
        if (useNewCollator) {
            collator = __CFStringCreateCollator(locale);
            _OCFSetTSD(kTSDCollatorOptionsSlot, NULL, NULL);
            collatorKind = 1;
        }
    }

    OCFComparisonResult result;
    const UniChar *characters1 = inlineBufferRange(str1, start1, start1 + length1, length1);
    const UniChar *characters2 = NULL;
    bool compared = false;
    if (start2 >= 0 && start2 + length2 <= str2->rangeToBuffer.length) {
        characters2 = inlineBufferRange(str2, start2, start2 + length2, length2);
        if (characters1 != NULL && characters2 != NULL) {
            OCFIndex count1 = location1 + length1 - start1;
            OCFIndex count2 = location2 + length2 - start2;
            if (collator != NULL && __CompareTextDefault(
                                        collator,
                                        collatorKind,
                                        options,
                                        characters1,
                                        count1,
                                        characters2,
                                        count2,
                                        &equivalent,
                                        &order
                                    ) == 0) {
                bool ordered = equivalent ? ((options >> 9) & 1) != 0 : true;
                result = ordered ? (order <= -1 ? kOCFCompareLessThan : kOCFCompareGreaterThan)
                                 : kOCFCompareEqualTo;
            } else {
                OCFIndex count = count1 < count2 ? count1 : count2;
                int difference = memcmp(characters1, characters2, (size_t)(count * 2));
                result = difference <= -1 ? kOCFCompareLessThan : kOCFCompareGreaterThan;
            }
            compared = true;
        }
    }
    if (!compared) {
        OCFIndex forcedResult = 0;
        OCFIndex heapCapacity1 = 0;
        UniChar *heapBuffer1 = NULL;
        UniChar *heapBuffer2 = NULL;
        OCFIndex heapCapacity2 = 0;
        UniChar localBuffer1[128];
        UniChar localBuffer2[128];
        OCFIndex end1 = location1 + length1;
        OCFIndex end2 = location2 + length2;
        result = kOCFCompareGreaterThan;
        const UniChar *clusterCharacters1 = characters1;
        const UniChar *clusterCharacters2 = characters2;
        bool finished = false;
        do {
            if (location1 < end1) {
                location1 = __extendLocationForward(location1, str1, alnumBMP, punctBMP, controlBMP, end1);
                length1 = location1 - start1;
                clusterCharacters1 = inlineBufferRange(str1, start1, location1, length1);
                if (clusterCharacters1 == NULL) {
                    clusterCharacters1 = copyCluster(
                        str1, start1, location1, length1, localBuffer1, &heapBuffer1, &heapCapacity1
                    );
                }
            }
            if (location2 < end2) {
                location2 = __extendLocationForward(location2, str2, alnumBMP, punctBMP, controlBMP, end2);
                length2 = location2 - start2;
                clusterCharacters2 = inlineBufferRange(str2, start2, location2, length2);
                if (clusterCharacters2 == NULL) {
                    clusterCharacters2 = copyCluster(
                        str2, start2, location2, length2, localBuffer2, &heapBuffer2, &heapCapacity2
                    );
                }
            }
            if (collator != NULL && __CompareTextDefault(
                                        collator,
                                        collatorKind,
                                        options,
                                        clusterCharacters1,
                                        length1,
                                        clusterCharacters2,
                                        length2,
                                        &equivalent,
                                        &order
                                    ) == 0) {
                if (equivalent) {
                    OCFIndex sign = order <= -1 ? -1 : 1;
                    if ((order != 0 && ((options >> 9) & 1) != 0) && forcedResult == 0) {
                        forcedResult = sign;
                    }
                    order = 0;
                }
            } else {
                if (clusterCharacters1 == NULL || clusterCharacters2 == NULL) {
                    crashWithMessage(
                        "Invalid string range produced for character buffer in _CFCompareStringsWithLocale."
                    );
                }
                OCFIndex count = length1 < length2 ? length1 : length2;
                order = memcmp(clusterCharacters1, clusterCharacters2, (size_t)(count * 2));
                if (order != 0) {
                    order = order + ((order >> 31) | 1);
                } else if (length1 < length2) {
                    result = kOCFCompareLessThan;
                    order = -2;
                    finished = true;
                } else if (length2 < length1) {
                    order = 2;
                    finished = true;
                }
            }
            if (finished) {
                break;
            }
            if ((uint32_t)(order - 2) < 0xFFFFFFFD) {
                break;
            }
            if (order == 0) {
                start1 = location1;
                start2 = location2;
            }
        } while (location1 < end1 || location2 < end2);
        if (!finished) {
            result = order == 0 ? forcedResult : (order <= -1 ? kOCFCompareLessThan : kOCFCompareGreaterThan);
        }
        if (heapCapacity1 >= 1) {
            OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, heapBuffer1);
        }
        if (heapCapacity2 >= 1) {
            OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, heapBuffer2);
        }
    }
    if (collator != cachedCollator) {
        if (cachedLocale != NULL) {
            __collatorFinalize(_OCFGetTSD(kTSDCollatorSlot));
        }
        _OCFSetTSD(kTSDCollatorSlot, collator, __collatorFinalize);
        _OCFSetTSD(kTSDCollatorLocaleSlot, (void *)OCFRetain(locale), NULL);
    }
    _OCFSetTSD(kTSDCollatorOptionsSlot, (void *)options, NULL);
    return result;
}

// Native __extendLocationBackward, 0x1c0234ee4. Non-external. Moves location back over the characters that
// continue a cluster; location 0 answers 0. The character at location is read from the inline buffer without
// a lower bound (0 at or past the buffered length; C-string bytes sign-extended; a refill starts at location
// - min(location, 4)). A low surrogate whose predecessor (refilled from location - min(location, 5), only
// while location is at most the length) is a high surrogate answers location. Otherwise: a character in
// nonBaseBMP continues unless it lies in U+2E80-U+ABFF ((unit - 0x2e80) >> 7 below 0xfb, 16-bit); a character
// outside it continues only when punctBMP is set, the character is outside that range and in punctBMP. Every
// other character answers location; moving past index 1 answers 0.
static OCFIndex __extendLocationBackward(
    OCFIndex location, OCFStringInlineBuffer *buffer, const uint8_t *nonBaseBMP, const uint8_t *punctBMP
) {
    if (location == 0) {
        return 0;
    }
    do {
        uint16_t character;
        if (location >= buffer->rangeToBuffer.length) {
            character = 0;
        } else {
            if (buffer->directUniCharBuffer != NULL) {
                character = buffer->directUniCharBuffer[buffer->rangeToBuffer.location + location];
            } else if (buffer->directCStringBuffer != NULL) {
                character =
                    (uint16_t)(int8_t)buffer->directCStringBuffer[buffer->rangeToBuffer.location + location];
            } else {
                if (location >= buffer->bufferedRangeEnd || location < buffer->bufferedRangeStart) {
                    OCFIndex back = (uint64_t)location < 4 ? location : 4;
                    OCFIndex start = location - back;
                    OCFIndex length = buffer->rangeToBuffer.length;
                    OCFIndex end = (uint64_t)(start + 64) < (uint64_t)length ? start + 64 : length;
                    buffer->bufferedRangeStart = start;
                    buffer->bufferedRangeEnd = end;
                    OCFStringGetCharacters(
                        buffer->theString,
                        __OCFRangeMake(buffer->rangeToBuffer.location + start, end - start),
                        buffer->buffer
                    );
                }
                character = buffer->buffer[location - buffer->bufferedRangeStart];
            }
            if ((character >> 10) == 0x37 && location <= buffer->rangeToBuffer.length) {
                uint16_t previous;
                if (buffer->directUniCharBuffer != NULL) {
                    previous = buffer->directUniCharBuffer[buffer->rangeToBuffer.location + location - 1];
                } else if (buffer->directCStringBuffer != NULL) {
                    previous = (uint16_t)(int8_t)
                                   buffer->directCStringBuffer[buffer->rangeToBuffer.location + location - 1];
                } else {
                    if (location > buffer->bufferedRangeEnd || location <= buffer->bufferedRangeStart) {
                        OCFIndex back = (uint64_t)location < 5 ? location : 5;
                        OCFIndex start = location - back;
                        OCFIndex length = buffer->rangeToBuffer.length;
                        OCFIndex end = (uint64_t)(start + 64) < (uint64_t)length ? start + 64 : length;
                        buffer->bufferedRangeStart = start;
                        buffer->bufferedRangeEnd = end;
                        OCFStringGetCharacters(
                            buffer->theString,
                            __OCFRangeMake(buffer->rangeToBuffer.location + start, end - start),
                            buffer->buffer
                        );
                    }
                    previous = buffer->buffer[location - buffer->bufferedRangeStart - 1];
                }
                if ((previous >> 10) == 0x36) {
                    return location;
                }
            }
        }
        bool ideographic = (uint16_t)((uint16_t)(character - 0x2e80) >> 7) < 0xfb;
        if (nonBaseBMP != NULL && ((nonBaseBMP[character >> 3] >> (character & 7)) & 1) != 0) {
            if (ideographic) {
                return location;
            }
        } else if (
            punctBMP == NULL || ideographic || ((1u << (character & 7)) & punctBMP[character >> 3]) == 0
        ) {
            return location;
        }
        --location;
    } while (location > 0);
    return 0;
}

// Native __CFStringCreateCollator, 0x1c0235140. Non-external. Opens an ICU collator for the locale's collator
// identifier (CFStringGetCString into a zeroed 128-byte buffer, ASCII; the result is not checked) and sets,
// with one status word that is never checked: normalization mode off (4, 0x10), alternate handling
// non-ignorable (1, 0x15), strength primary (5, 0), case level off (3, 0x10) and numeric collation off (7,
// 0x10).
static void *__CFStringCreateCollator(OCFLocaleRef compareLocale) {
    OCFStringRef identifier = OCFLocaleGetValue(compareLocale, __kCFLocaleCollatorID);
    char buffer[128];
    memset(buffer, 0, sizeof(buffer));
    OCFStringGetCString(identifier, buffer, 128, 0x600);
    int32_t status = 0;
    void *collator = ucol_open(buffer, &status);
    ucol_setAttribute(collator, 4, 0x10, &status);
    ucol_setAttribute(collator, 1, 0x15, &status);
    ucol_setAttribute(collator, 5, 0, &status);
    ucol_setAttribute(collator, 3, 0x10, &status);
    ucol_setAttribute(collator, 7, 0x10, &status);
    return collator;
}

// Native __CompareTextDefault, 0x1c0235234. Non-external. ICU attributes: 3 case level, 4 normalization
// mode, 5 strength, 7 numeric collation; values 0x10 off, 0x11 on. *equivalent starts true and *order 0.
// Numeric collation follows options bit 6 (kCFCompareNumerically): it is set unless collatorKind is set and
// bit 6 of per-thread slot 17 already matches. A primary-strength ucol_strcoll difference stores -2 or 2.
// Otherwise normalization is turned on, strength becomes secondary (1) when options bit 7
// (kCFCompareDiacriticInsensitive) is clear and primary (0) when it is set, and case level is on when bit 0
// (kCFCompareCaseInsensitive) is clear; an ICU error resets the attributes (4, 5, 3 and 7 off/0) and answers
// 666. A difference stores -1 or 1; the attributes are reset (4 off, 5 primary, 3 off). Unless options bit 4
// (kCFCompareNonliteral) is set or an order was found, both texts' collation elements are walked: the
// element streams end at -1 or an ICU error; elements differing in more than the low byte stop nothing;
// equal-high elements below 0x10000 record, once and only from 0x100 up, the comparison of the two offset
// advances (the second text's against the first's); elements from 0x10000 compare their low bytes, with
// 0x8f-0x97 mapped down by 0x8a, and a difference stops the walk with -1 or 1. The walk's result (or the
// recorded advance comparison) becomes the order; an ICU error while opening stores 0 and leaves the element
// iterators open (native). Then *equivalent = (order == 0); a nonzero order answers 0. Otherwise a tertiary
// comparison (4 on, 5 tertiary (2), 3 on) runs: an ICU error resets and answers 666; a difference stores -1
// or 1; equal collation compares the UTF-16 units over the shorter length (a 32-bit counter), then the
// lengths. The attributes are reset (4 off, 5 primary, 3 off) and 0 is answered.
static int32_t __CompareTextDefault(
    void *collator,
    Boolean collatorKind,
    OCFOptionFlags options,
    const UniChar *text1,
    OCFIndex text1Length,
    const UniChar *text2,
    OCFIndex text2Length,
    Boolean *equivalent,
    int32_t *order
) {
    *equivalent = true;
    *order = 0;
    uint32_t numeric = (uint32_t)(options >> 6) & 1;
    int32_t status;
    if (collatorKind == 0 ||
        (((uint32_t)(uintptr_t)_OCFGetTSD(kTSDCollatorOptionsSlot) >> 6) & 1) != numeric) {
        status = 0;
        ucol_setAttribute(collator, 7, numeric != 0 ? 0x11 : 0x10, &status);
    }
    int32_t result;
    int32_t comparison = ucol_strcoll(collator, text1, (int32_t)text1Length, text2, (int32_t)text2Length);
    if (comparison != 0) {
        result = comparison == -1 ? -2 : 2;
        goto storeOrder;
    }
    result = *order;
    if (result != 0) {
        goto checkOrder;
    }
    status = 0;
    ucol_setAttribute(collator, 4, 0x11, &status);
    ucol_setAttribute(collator, 5, (options & 0x80) == 0 ? 1 : 0, &status);
    ucol_setAttribute(collator, 3, (options & 1) == 0 ? 0x11 : 0x10, &status);
    if (status > 0) {
        goto failure;
    }
    comparison = ucol_strcoll(collator, text1, (int32_t)text1Length, text2, (int32_t)text2Length);
    if (comparison != 0) {
        *order = comparison == -1 ? -1 : 1;
    }
    status = 0;
    ucol_setAttribute(collator, 4, 0x10, &status);
    ucol_setAttribute(collator, 5, 0, &status);
    ucol_setAttribute(collator, 3, 0x10, &status);
    result = *order;
    if ((options & 0x10) != 0 || result != 0) {
        goto checkOrder;
    }
    status = 0;
    void *elements1 = ucol_openElements(collator, text1, (int32_t)text1Length, &status);
    void *elements2 = ucol_openElements(collator, text2, (int32_t)text2Length, &status);
    result = 0;
    if (status > 0) {
        goto storeOrder;
    }
    {
        int32_t element1 = ucol_next(elements1, &status);
        int32_t element2 = ucol_next(elements2, &status);
        int32_t walk = 0;
        int32_t advance = 0;
        if (status <= 0 && element1 != -1 && element2 != -1) {
            int32_t previousOffset1 = 0;
            int32_t previousOffset2 = 0;
            for (;;) {
                int32_t offset1 = ucol_getOffset(elements1);
                int32_t offset2 = ucol_getOffset(elements2);
                if ((uint32_t)(element2 ^ element1) <= 0xFF) {
                    if ((uint32_t)element1 < 0x10000) {
                        int32_t advance1 = offset1 - previousOffset1;
                        int32_t advance2 = offset2 - previousOffset2;
                        int32_t difference = advance2 > advance1 ? 1 : (advance2 >= advance1 ? 0 : -1);
                        if (advance != 0) {
                            difference = advance;
                        }
                        if ((uint32_t)element1 >= 0x100) {
                            advance = difference;
                        }
                    } else {
                        uint32_t low1 = (uint32_t)element1 & 0xFF;
                        uint32_t low2 = (uint32_t)element2 & 0xFF;
                        if (low1 - 0x8F < 9) {
                            low1 -= 0x8A;
                        }
                        if (low2 - 0x8F < 9) {
                            low2 -= 0x8A;
                        }
                        if (low1 != low2) {
                            walk = (int32_t)low1 >= (int32_t)low2 ? 1 : -1;
                            break;
                        }
                    }
                }
                element1 = ucol_next(elements1, &status);
                element2 = ucol_next(elements2, &status);
                previousOffset1 = offset1;
                previousOffset2 = offset2;
                if (status > 0 || element1 == -1 || element2 == -1) {
                    walk = 0;
                    break;
                }
            }
        }
        ucol_closeElements(elements1);
        ucol_closeElements(elements2);
        result = walk == 0 ? advance : walk;
    }
storeOrder:
    *order = result;
checkOrder:
    *equivalent = result == 0;
    if (*order != 0) {
        return 0;
    }
    status = 0;
    ucol_setAttribute(collator, 4, 0x11, &status);
    ucol_setAttribute(collator, 5, 2, &status);
    ucol_setAttribute(collator, 3, 0x11, &status);
    if (status >= 1) {
        goto failure;
    }
    comparison = ucol_strcoll(collator, text1, (int32_t)text1Length, text2, (int32_t)text2Length);
    int32_t tertiary;
    if (comparison != 0) {
        tertiary = comparison == -1 ? -1 : 1;
    } else {
        uint32_t count =
            (uint32_t)((uint64_t)text1Length < (uint64_t)text2Length ? text1Length : text2Length);
        const UniChar *characters1 = text1;
        const UniChar *characters2 = text2;
        tertiary = 2;
        while (count != 0) {
            UniChar character1 = *characters1++;
            UniChar character2 = *characters2++;
            if (character1 != character2) {
                tertiary = character1 >= character2 ? 1 : -1;
                break;
            }
            --count;
        }
        if (tertiary == 2) {
            if (text1Length == text2Length) {
                tertiary = 0;
            } else {
                tertiary = (uint64_t)text1Length >= (uint64_t)text2Length ? 1 : -1;
            }
        }
    }
    *order = tertiary;
    status = 0;
    ucol_setAttribute(collator, 4, 0x10, &status);
    ucol_setAttribute(collator, 5, 0, &status);
    ucol_setAttribute(collator, 3, 0x10, &status);
    return 0;
failure:
    status = 0;
    ucol_setAttribute(collator, 4, 0x10, &status);
    ucol_setAttribute(collator, 5, 0, &status);
    ucol_setAttribute(collator, 3, 0x10, &status);
    ucol_setAttribute(collator, 7, 0x10, &status);
    return 666;
}

// Native __extendLocationForward, 0x1c0235744. Non-external. Moves location forward over the characters that
// continue a cluster, at least one step, while it stays below strMax. A character outside [0, length) reads
// as 0 (C-string bytes sign-extended; a refill starts at location - 4, or 0, and its follower's at location -
// 3, or 0). A high surrogate followed (below the length) by a low surrogate moves two places and continues
// when the supplementary character is in the plane bitmap of character set 0xa (alphanumeric), 0xb
// (punctuation) or 0x6c, else answers. Any other character moves one place: one in alnumBMP or punctBMP
// continues unless it lies in U+2E80-U+ABFF; any other continues only when controlBMP is set, the character
// lies outside that range and is in controlBMP.
static OCFIndex __extendLocationForward(
    OCFIndex location,
    OCFStringInlineBuffer *buffer,
    const uint8_t *alnumBMP,
    const uint8_t *punctBMP,
    const uint8_t *controlBMP,
    OCFIndex strMax
) {
    for (;;) {
        uint16_t character = 0;
        if (location >= 0 && location < buffer->rangeToBuffer.length) {
            if (buffer->directUniCharBuffer != NULL) {
                character = buffer->directUniCharBuffer[location + buffer->rangeToBuffer.location];
            } else if (buffer->directCStringBuffer != NULL) {
                character =
                    (uint16_t)(int8_t)buffer->directCStringBuffer[location + buffer->rangeToBuffer.location];
            } else {
                if (location >= buffer->bufferedRangeEnd || location < buffer->bufferedRangeStart) {
                    OCFIndex start = (uint64_t)location < 4 ? 0 : location - 4;
                    OCFIndex length = buffer->rangeToBuffer.length;
                    OCFIndex end = (uint64_t)(start + 64) < (uint64_t)length ? start + 64 : length;
                    buffer->bufferedRangeStart = start;
                    buffer->bufferedRangeEnd = end;
                    OCFStringGetCharacters(
                        buffer->theString,
                        __OCFRangeMake(buffer->rangeToBuffer.location + start, end - start),
                        buffer->buffer
                    );
                }
                character = buffer->buffer[location - buffer->bufferedRangeStart];
            }
            OCFIndex next = location + 1;
            if ((character >> 10) == 0x36 && next < buffer->rangeToBuffer.length) {
                uint16_t following;
                if (buffer->directUniCharBuffer != NULL) {
                    following = buffer->directUniCharBuffer[next + buffer->rangeToBuffer.location];
                } else if (buffer->directCStringBuffer != NULL) {
                    following =
                        (uint16_t)(int8_t)buffer->directCStringBuffer[next + buffer->rangeToBuffer.location];
                } else {
                    if (next >= buffer->bufferedRangeEnd || next < buffer->bufferedRangeStart) {
                        OCFIndex start = (uint64_t)location < 3 ? 0 : location - 3;
                        OCFIndex length = buffer->rangeToBuffer.length;
                        OCFIndex end = (uint64_t)(start + 64) < (uint64_t)length ? start + 64 : length;
                        buffer->bufferedRangeStart = start;
                        buffer->bufferedRangeEnd = end;
                        OCFStringGetCharacters(
                            buffer->theString,
                            __OCFRangeMake(buffer->rangeToBuffer.location + start, end - start),
                            buffer->buffer
                        );
                    }
                    following = buffer->buffer[next - buffer->bufferedRangeStart];
                }
                if ((following >> 10) == 0x37) {
                    uint32_t longCharacter = ((uint32_t)character << 10) + following + 0xFCA02400;
                    location += 2;
                    uint32_t plane = longCharacter >> 16;
                    const uint8_t *bitmap = OCFUniCharGetBitmapPtrForPlane(0xa, plane);
                    if (bitmap == NULL ||
                        ((bitmap[(longCharacter >> 3) & 0x1FFF] >> (following & 7)) & 1) == 0) {
                        bitmap = OCFUniCharGetBitmapPtrForPlane(0xb, plane);
                        if (bitmap == NULL ||
                            ((bitmap[(longCharacter >> 3) & 0x1FFF] >> (following & 7)) & 1) == 0) {
                            bitmap = OCFUniCharGetBitmapPtrForPlane(0x6c, plane);
                            if (bitmap == NULL ||
                                ((bitmap[(longCharacter >> 3) & 0x1FFF] >> (following & 7)) & 1) == 0) {
                                return location;
                            }
                        }
                    }
                    if (location >= strMax) {
                        return location;
                    }
                    continue;
                }
            }
        }
        ++location;
        bool ideographic = (uint16_t)((uint16_t)(character - 0x2e80) >> 7) < 0xfb;
        if ((alnumBMP != NULL && ((alnumBMP[character >> 3] >> (character & 7)) & 1) != 0) ||
            (punctBMP != NULL && ((punctBMP[character >> 3] >> (character & 7)) & 1) != 0)) {
            if (ideographic) {
                return location;
            }
        } else if (
            controlBMP == NULL || ideographic || ((1u << (character & 7)) & controlBMP[character >> 3]) == 0
        ) {
            return location;
        }
        if (location >= strMax) {
            return location;
        }
    }
}

// Native __collatorFinalize, 0x1c0235a08. Non-external; the destructor of per-thread slot 9. The cached
// locale (slot 8) is read, then slots 9, 8 and 17 are cleared (no destructors). Under __CFDefaultCollatorLock
// the collator joins the default pool when the pool's locale is the cached locale and fewer than eight are
// pooled; otherwise, after unlocking, a non-NULL collator is closed. The cached locale is released when set.
static void __collatorFinalize(void *collator) {
    OCFLocaleRef locale = (OCFLocaleRef)_OCFGetTSD(kTSDCollatorLocaleSlot);
    _OCFSetTSD(kTSDCollatorSlot, NULL, NULL);
    _OCFSetTSD(kTSDCollatorLocaleSlot, NULL, NULL);
    _OCFSetTSD(kTSDCollatorOptionsSlot, NULL, NULL);
    os_unfair_lock_lock_with_options(&__CFDefaultCollatorLock, 0x10000);
    OCFIndex count = __CFDefaultCollatorsCount;
    if (__CFDefaultCollatorLocale == locale && count <= 7) {
        __CFDefaultCollatorsCount = count + 1;
        __CFDefaultCollators[count] = collator;
        os_unfair_lock_unlock(&__CFDefaultCollatorLock);
    } else {
        os_unfair_lock_unlock(&__CFDefaultCollatorLock);
        if (collator != NULL) {
            ucol_close(collator);
        }
    }
    if (locale != NULL) {
        OCFRelease(locale);
    }
}

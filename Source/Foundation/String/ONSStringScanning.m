//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSStringScanning.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"

#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/Locale/OCFLocaleKeys.h"

#include <ctype.h>
#include <malloc/malloc.h>
#include <stdlib.h>
#include <xlocale.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <stdint.h>
#include <string.h>
#include <unicode/uchar.h>

// Inferred organization: the scanning functions lie together in the Foundation image's NSScanner region
// (0x1c05c0c74-0x1c05c1ddc; __NSScanHexDoubleFromString at 0x1c05c1e0c is outside the scope). Each function
// inlines the same character-buffer steps; they are spelled here as the static inline helpers
// scanBufferStart, scanBufferAdvance and scanBufferSkip, and the digit test as isADigit (its static `set` is
// the native _isADigit.set, 0x22e6fcc48, __DATA,__bss). Objective-C compiled with manual reference counting.
// Unless a definition says
// otherwise: iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence:
// verified (assembly read in full, iphone-simulator-foundation-link.json). iOS Device divergence: iOS Device
// 24A446 remains unread. Reading: Foundation/String/ONSStringReview.md.
//
// The characters are fetched with objc_msgSend and sel_getUid("getCharacters:range:") (not a selector
// reference), 32 at a time.

// Fetches count characters of the buffer's string at location into the buffer.
static inline void scanBufferFetch(StringScanBuffer *buffer, ONSUInteger location, ONSUInteger count) {
    ((void (*)(id, SEL, unsigned short *, ONSUInteger, ONSUInteger))objc_msgSend)(
        buffer->string, sel_getUid("getCharacters:range:"), buffer->characters, location, count
    );
}

// Starts the zeroed buffer at location: the string's -length, up to 32 characters, the current character
// (0xFFFF at or past the end) and index 1.
static inline void scanBufferStart(StringScanBuffer *buffer, id string, ONSUInteger location) {
    ONSUInteger length = [string length];
    buffer->length = length;
    buffer->location = location;
    buffer->string = string;
    if (length > location) {
        ONSUInteger count = length - location;
        if (count >= 0x20) {
            count = 0x20;
        }
        buffer->count = count;
        scanBufferFetch(buffer, location, count);
        buffer->current = buffer->characters[0];
    } else {
        buffer->count = 0;
        buffer->current = 0xFFFF;
    }
    buffer->index = 1;
}

// Moves to the next character: from the fetched run, else the next run of up to 32 characters, else past the
// end (index count + 1, current 0xFFFF).
static inline unsigned short scanBufferAdvance(StringScanBuffer *buffer) {
    ONSUInteger index = buffer->index;
    if (index < buffer->count) {
        buffer->index = index + 1;
        buffer->current = buffer->characters[index];
        return buffer->current;
    }
    ONSUInteger location = buffer->location + buffer->count;
    if (buffer->length > location) {
        ONSUInteger count = buffer->length - location;
        if (count >= 0x20) {
            count = 0x20;
        }
        buffer->location = location;
        buffer->count = count;
        scanBufferFetch(buffer, location, count);
        buffer->current = buffer->characters[0];
        buffer->index = 1;
        return buffer->current;
    }
    buffer->index = buffer->count + 1;
    buffer->current = 0xFFFF;
    return buffer->current;
}

// Skips the skip set's characters while the buffer has not run past the end (index - 1 < count, unsigned).
static inline void scanBufferSkip(StringScanBuffer *buffer, id skipSet) {
    if (skipSet == nil || ![skipSet characterIsMember:buffer->current]) {
        return;
    }
    do {
        if (buffer->index - 1 >= buffer->count) {
            break;
        }
        scanBufferAdvance(buffer);
    } while ([skipSet characterIsMember:buffer->current]);
}

// Whether the character is in +[NSCharacterSet decimalDigitCharacterSet], fetched once into the native
// isADigit.set (not retained).
static inline BOOL isADigit(unsigned short character) {
    static id set;
    if (set == nil) {
        set = [(id)characterSetClassSymbol decimalDigitCharacterSet];
    }
    return [set characterIsMember:character];
}

// The digit's value: c - '0' for ASCII digits, otherwise u_charDigitValue (sign-extended).
static inline int64_t digitValue(unsigned short character) {
    uint32_t value = (uint32_t)character - '0';
    if (value <= 9) {
        return value;
    }
    return u_charDigitValue(character);
}

// Native __NSScanLongLongFromString, 0x1c05c0c74. Non-external (was a private external). Below
// 0x0CCCCCCCCCCCCCCC (signed) every digit accumulates (value * 10 + digit, wrapping); at exactly
// 0x0CCCCCCCCCCCCCCC a digit accumulates while digit - negative < 7; otherwise the remaining digits are
// consumed and the value becomes INT64_MIN (negative) or INT64_MAX, with the sign cleared. The stored value
// is negated for a leading '-' (wrapping). *location becomes the run's location + index - 1.
BOOL _NSScanLongLongFromString(id string, id skipSet, long long *value, ONSUInteger *location) {
    StringScanBuffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    scanBufferStart(&buffer, string, *location);
    scanBufferSkip(&buffer, skipSet);
    BOOL negative = buffer.current == '-';
    if (negative || buffer.current == '+') {
        scanBufferAdvance(&buffer);
        scanBufferSkip(&buffer, skipSet);
    }
    BOOL result = isADigit(buffer.current);
    if (!result) {
        return result;
    }
    uint64_t accumulated = 0;
    unsigned short character;
    do {
        int64_t signedValue = (int64_t)accumulated;
        if (signedValue > 0x0CCCCCCCCCCCCCCBLL &&
            (signedValue != 0x0CCCCCCCCCCCCCCCLL || digitValue(buffer.current) - (negative & 1) >= 7)) {
            do {
                scanBufferAdvance(&buffer);
            } while (isADigit(buffer.current));
            accumulated = (negative & 1) != 0 ? (uint64_t)INT64_MIN : (uint64_t)INT64_MAX;
            character = buffer.current;
            negative = NO;
        } else {
            accumulated = accumulated * 10 + (uint64_t)digitValue(buffer.current);
            character = scanBufferAdvance(&buffer);
        }
    } while (isADigit(character));
    if (value != NULL) {
        *value = (long long)((negative & 1) != 0 ? 0 - accumulated : accumulated);
    }
    *location = buffer.location + buffer.index - 1;
    return result;
}

// Native __NSScanIntFromString, 0x1c05c11f8. Non-external (was a private external). The steps of
// __NSScanLongLongFromString (the first 0x1c8 bytes are instruction-for-instruction the same) in 32 bits: the
// thresholds are 0x0CCCCCCB and 0x0CCCCCCC (signed) and the overflow values INT32_MIN and INT32_MAX; the
// accumulation wraps in 32 bits.
BOOL _NSScanIntFromString(id string, id skipSet, int *value, ONSUInteger *location) {
    StringScanBuffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    scanBufferStart(&buffer, string, *location);
    scanBufferSkip(&buffer, skipSet);
    BOOL negative = buffer.current == '-';
    if (negative || buffer.current == '+') {
        scanBufferAdvance(&buffer);
        scanBufferSkip(&buffer, skipSet);
    }
    BOOL result = isADigit(buffer.current);
    if (!result) {
        return result;
    }
    uint32_t accumulated = 0;
    unsigned short character;
    do {
        int32_t signedValue = (int32_t)accumulated;
        if (signedValue > 0x0CCCCCCB &&
            (signedValue != 0x0CCCCCCC || digitValue(buffer.current) - (negative & 1) >= 7)) {
            do {
                scanBufferAdvance(&buffer);
            } while (isADigit(buffer.current));
            accumulated = (negative & 1) != 0 ? (uint32_t)INT32_MIN : (uint32_t)INT32_MAX;
            character = buffer.current;
            negative = NO;
        } else {
            accumulated = accumulated * 10 + (uint32_t)digitValue(buffer.current);
            character = scanBufferAdvance(&buffer);
        }
    } while (isADigit(character));
    if (value != NULL) {
        *value = (int)((negative & 1) != 0 ? 0 - accumulated : accumulated);
    }
    *location = buffer.location + buffer.index - 1;
    return result;
}

// Native __NSCheckDecimalSeparator, 0x1c05c177c. Non-external (was a private external). With the current
// character equal to separatorCharacter (compared with all 32 bits of the argument) and a one-character
// separator (a nil separator counts as one), YES. Otherwise NO unless the separator is non-nil and the
// character matches; then each further separator character (from index 1, while length - 2 decrements without
// borrowing) is compared with the next buffer character, which is also stored into *character: a mismatch
// answers NO, a full match (or a separator shorter than two characters) YES.
BOOL _NSCheckDecimalSeparator(
    StringScanBuffer *buffer, unsigned short *character, unsigned short separatorCharacter, id separator
) {
    ONSUInteger length = separator != nil ? [separator length] : 1;
    unsigned short current = *character;
    if (current == separatorCharacter && length == 1) {
        return YES;
    }
    if (separator == nil || current != separatorCharacter) {
        return NO;
    }
    if (length < 2) {
        return YES;
    }
    ONSUInteger remaining = length - 2;
    ONSUInteger separatorIndex = 1;
    while (YES) {
        unsigned short next = scanBufferAdvance(buffer);
        *character = next;
        unsigned short expected = [separator characterAtIndex:separatorIndex];
        BOOL more = remaining != 0;
        remaining--;
        if (next != expected) {
            return NO;
        }
        separatorIndex++;
        if (!more) {
            return YES;
        }
    }
}

// Native __NSScanDoubleFromString, 0x1c05c18d0. Hidden (was a private external). The native __NSNumberSet
// (0x1c100e0b0, __TEXT,__const) admits ASCII +, -, digits, E and e; other characters must be Unicode digits
// or match the locale's separator. strtod_l, not the scan loop, decides how many normalized bytes belong to
// the number. The resulting location counts those bytes even for multi-character separators (kept).
// GOT 0x22227F550 is bound as NSLocaleDecimalSeparator (confidence: medium); the CoreFoundation reading
// establishes that it aliases kCFLocaleDecimalSeparatorKey. The fallback key is the literal
// NSDecimalSeparator. Foundation defect: at 0x1c05c1c2c a failed initial malloc_type_malloc is followed by
// four unguarded 16-byte stores at 0x1c05c1c40-0x1c05c1c44. Return NO on allocation failure, like the native
// reallocation-failure path, leaving the caller's output/location untouched. Confidence: verified. iOS Device
// divergence: unread.
BOOL _NSScanDoubleFromString(id string, id skipSet, double *value, ONSUInteger *location, id locale) {
    id separator = [locale objectForKey:(id)kOCFLocaleDecimalSeparatorKey];
    if (separator == nil) {
        separator = [locale objectForKey:(id)OF_STR("NSDecimalSeparator")];
    }
    if (separator == nil && [locale isKindOfClass:[(Class)localeClassSymbol class]]) {
        separator = [locale decimalSeparator];
    }
    unsigned short separatorCharacter =
        separator != nil && [separator length] != 0 ? [separator characterAtIndex:0] : '.';
    StringScanBuffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    scanBufferStart(&buffer, string, *location);
    scanBufferSkip(&buffer, skipSet);
    ONSUInteger originalLocation = *location;
    ONSUInteger initialLocation = buffer.location;
    ONSUInteger initialIndex = buffer.index;
    unsigned short character = buffer.current;
    if (character <= 0x7F && isspace(character)) {
        return NO;
    }
    static const uint8_t kNumberSet[16] = {0, 0, 0, 0, 0, 0x28, 0xFF, 0x03, 0x20, 0, 0, 0, 0x20, 0, 0, 0};
    char localBytes[64];
    char *bytes = localBytes;
    char *heapBytes = NULL;
    size_t capacity = 0;
    size_t used = 0;
    size_t limit = sizeof(localBytes) - 1;
    for (;;) {
        unsigned short output = character;
        if (character <= 0x7F) {
            if (((kNumberSet[character >> 3] >> (character & 7)) & 1) == 0) {
                if (!_NSCheckDecimalSeparator(&buffer, &character, separatorCharacter, separator)) {
                    break;
                }
                output = '.';
            }
        } else {
            int digit = u_charDigitValue(character);
            if (digit != -1) {
                output = (unsigned short)(digit + '0');
            } else {
                if (!_NSCheckDecimalSeparator(&buffer, &character, separatorCharacter, separator)) {
                    break;
                }
                output = '.';
            }
        }
        if (used >= limit) {
            capacity += 128;
            if (heapBytes != NULL) {
                // Native ____NSScanDoubleFromString_block_invoke, 0x1c05c1de0. Non-external.
                heapBytes = __OCFSafelyReallocateTyped(
                    heapBytes, capacity, 0x100004077774924, ^(void *pointer, bool *stop) {
                      free(pointer);
                      *stop = true;
                    }
                );
                if (heapBytes == NULL) {
                    return NO;
                }
            } else {
                heapBytes = malloc_type_malloc(capacity, 0x100004077774924);
                if (heapBytes == NULL) {
                    return NO;
                }
                memcpy(heapBytes, localBytes, sizeof(localBytes));
            }
            bytes = heapBytes;
            limit = capacity - 1;
        }
        bytes[used++] = (char)output;
        character = scanBufferAdvance(&buffer);
    }
    bytes[used] = 0;
    char *end;
    double result = strtod_l(bytes, &end, NULL);
    // Compute the byte count before free; the native code subtracts the saved addresses after free.
    size_t parsed = (size_t)(end - bytes);
    if (heapBytes != NULL) {
        free(heapBytes);
    }
    if (parsed == 0) {
        return NO;
    }
    *location += initialLocation + initialIndex - 1 - originalLocation + parsed;
    if (value != NULL) {
        *value = result;
    }
    return YES;
}

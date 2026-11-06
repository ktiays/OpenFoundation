//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/String/OCFStringPseudoLanguage.h"

#include "CoreFoundation/CharacterSet/OCFCharacterSet.h"
#include "CoreFoundation/String/OCFStringFormat.h"

#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: _CFDoubledStringCreate (0x1c01966d0) through ___CFTallStringCreate_block_invoke
// (0x1c0196c70), with _CFAccentuatedStringCreate.cold.1 (0x1c0307f98), are contiguous in the iOS Simulator
// __text section between the copy-on-write helpers and the __NSPlaceholderSet methods, and form the native
// pseudo-language unit; the native source path is not in the evidence. Unless a definition says otherwise:
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationString.md.

// The transform callback: appends to the output buffer after a single-unit cluster outside the skipped set
// (the buffer, the count of units written, and the unit just written).
typedef void (^PseudoLanguageTransform)(UniChar *buffer, OCFIndex *count, UniChar character);

static OCFStringRef __CFPseudoLanguageStringCreateWithTransform(
    OCFStringRef string, OCFIndex expansion, OCFCharacterSetRef charsToSkip, PseudoLanguageTransform transform
);

// Native _CFDoubledStringCreate, 0x1c01966d0. External. A mutable copy (kCFAllocatorSystemDefault, no
// capacity limit) of string with every "%" removed (CFStringFindAndReplace over the whole length, no
// options), followed by " %@" formatted with the original string.
OCFMutableStringRef _OCFDoubledStringCreate(OCFStringRef string) {
    OCFMutableStringRef result = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
    OCFStringAppend(result, string);
    OCFStringFindAndReplace(
        result, OF_STR("%"), OF_STR(""), __OCFRangeMake(0, OCFStringGetLength(result)), 0
    );
    OCFStringAppendFormat(result, NULL, OF_STR(" %@"), string);
    return result;
}

// Native _CFAffixedStringCreate, 0x1c019675c. External. "%@%@%@" (kCFAllocatorDefault, no format options)
// with the second argument, the string and the third argument.
OCFStringRef _OCFAffixedStringCreate(OCFStringRef string, OCFStringRef prefix, OCFStringRef suffix) {
    return OCFStringCreateWithFormat(kOCFAllocatorDefault, NULL, OF_STR("%@%@%@"), prefix, string, suffix);
}

// Native _CFRLORightToLeftStringCreate, 0x1c019679c. External. The string between U+202E (right-to-left
// override) and U+202C (pop directional formatting): "%C%@%C" with kCFAllocatorDefault.
OCFStringRef _OCFRLORightToLeftStringCreate(OCFStringRef string) {
    return OCFStringCreateWithFormat(
        kOCFAllocatorDefault, NULL, OF_STR("%C%@%C"), (UniChar)0x202E, string, (UniChar)0x202C
    );
}

// Native _CFAccentuatedStringCreate.charsToSkip and .onceToken (__DATA,__bss), and .acceptableAccentChars,
// 0x1c03735fc (__TEXT,__const, six UTF-16 units). Non-external.
static OCFCharacterSetRef accentuatedStringCharsToSkip;
static dispatch_once_t accentuatedStringOnceToken;
static const UniChar acceptableAccentChars[6] = {0x0300, 0x0327, 0x0303, 0x0325, 0x030a, 0x0308};

// Native _CFAccentuatedStringCreate, 0x1c01967e4, with .cold.1 (0x1c0307f98, the dispatch_once call reached
// while the once token is not -1), ___CFAccentuatedStringCreate_block_invoke (0x1c0196830) and
// ___CFAccentuatedStringCreate_block_invoke_2 (0x1c0196b94). External. The skipped set is built once: a
// mutable set (kCFAllocatorDefault) united with the predefined sets 1 (control), 3 (whitespace and newline),
// 0xb (punctuation), 0xe (symbol), 4 (decimal digit) and 8 (non-base), copied and released. The transform
// (expansion 2) appends acceptableAccentChars[unit % 6] after each transformed unit (a tail call).
OCFStringRef _OCFAccentuatedStringCreate(OCFStringRef string) {
    dispatch_once(&accentuatedStringOnceToken, ^{
      OCFAllocatorRef allocator = kOCFAllocatorDefault;
      OCFMutableCharacterSetRef set = OCFCharacterSetCreateMutable(allocator);
      OCFCharacterSetUnion(set, OCFCharacterSetGetPredefined(1));
      OCFCharacterSetUnion(set, OCFCharacterSetGetPredefined(3));
      OCFCharacterSetUnion(set, OCFCharacterSetGetPredefined(0xb));
      OCFCharacterSetUnion(set, OCFCharacterSetGetPredefined(0xe));
      OCFCharacterSetUnion(set, OCFCharacterSetGetPredefined(4));
      OCFCharacterSetUnion(set, OCFCharacterSetGetPredefined(8));
      accentuatedStringCharsToSkip = OCFCharacterSetCreateCopy(allocator, set);
      OCFRelease(set);
    });
    return __CFPseudoLanguageStringCreateWithTransform(
        string, 2, accentuatedStringCharsToSkip, ^(UniChar *buffer, OCFIndex *count, UniChar character) {
          UniChar accent = acceptableAccentChars[(uint16_t)(character % 6)];
          OCFIndex index = *count;
          buffer[index] = accent;
          *count = index + 1;
        }
    );
}

// Native __CFPseudoLanguageStringCreateWithTransform, 0x1c01968f0. Non-external. A mutable copy of string
// (kCFAllocatorDefault, capacity its length) is read through an inline buffer (CFStringGetCharactersPtr, else
// the ASCII C-string pointer, else 64-unit refills from index - 4). The output buffer holds expansion *
// length
// + 1 units (malloc_type_malloc, type 0x1000040BDFB0063). Composed-character clusters are copied unit by unit
// from the current index for the cluster's length (the cluster's location is not used); a cluster of one unit
// that is not in charsToSkip then passes the output buffer, the count and the unit to the transform. A
// cluster length below 1 leaves the index unchanged (native). The output is terminated, wrapped without a
// copy (CFStringCreateMutableWithExternalCharactersNoCopy, NULL allocator, kCFAllocatorNull), copied with
// kCFAllocatorDefault; the mutable copy and the wrapper are released and the buffer freed.
static OCFStringRef __CFPseudoLanguageStringCreateWithTransform(
    OCFStringRef string, OCFIndex expansion, OCFCharacterSetRef charsToSkip, PseudoLanguageTransform transform
) {
    OCFAllocatorRef allocator = kOCFAllocatorDefault;
    OCFMutableStringRef copy = OCFStringCreateMutableCopy(allocator, OCFStringGetLength(string), string);
    OCFIndex length = OCFStringGetLength(copy);
    OCFStringInlineBuffer inlineBuffer;
    inlineBuffer.theString = copy;
    inlineBuffer.rangeToBuffer = __OCFRangeMake(0, length);
    inlineBuffer.directUniCharBuffer = OCFStringGetCharactersPtr(copy);
    inlineBuffer.directCStringBuffer =
        inlineBuffer.directUniCharBuffer != NULL ? NULL : OCFStringGetCStringPtr(copy, 0x600);
    memset(inlineBuffer.buffer, 0, sizeof(inlineBuffer.buffer));
    inlineBuffer.bufferedRangeStart = 0;
    inlineBuffer.bufferedRangeEnd = 0;
    UniChar *buffer = malloc_type_malloc((size_t)(expansion * length * 2 + 2), 0x1000040BDFB0063);
    OCFIndex count = 0;
    if (length >= 1) {
        OCFIndex index = 0;
        OCFIndex end;
        do {
            OCFRange range = OCFStringGetRangeOfComposedCharactersAtIndex(copy, index);
            if (range.length < 1) {
                end = index;
            } else {
                end = range.length + index;
                uint16_t character;
                do {
                    if (index < 0 || index >= inlineBuffer.rangeToBuffer.length) {
                        character = 0;
                    } else if (inlineBuffer.directUniCharBuffer != NULL) {
                        character =
                            inlineBuffer.directUniCharBuffer[index + inlineBuffer.rangeToBuffer.location];
                    } else if (inlineBuffer.directCStringBuffer != NULL) {
                        character =
                            (uint16_t)(int8_t)
                                inlineBuffer.directCStringBuffer[index + inlineBuffer.rangeToBuffer.location];
                    } else {
                        if (index >= inlineBuffer.bufferedRangeEnd ||
                            index < inlineBuffer.bufferedRangeStart) {
                            OCFIndex start = (uint64_t)index < 4 ? 0 : index - 4;
                            OCFIndex bufferLength = inlineBuffer.rangeToBuffer.length;
                            OCFIndex stop =
                                (uint64_t)(start + 64) < (uint64_t)bufferLength ? start + 64 : bufferLength;
                            inlineBuffer.bufferedRangeStart = start;
                            inlineBuffer.bufferedRangeEnd = stop;
                            OCFStringGetCharacters(
                                inlineBuffer.theString,
                                __OCFRangeMake(inlineBuffer.rangeToBuffer.location + start, stop - start),
                                inlineBuffer.buffer
                            );
                        }
                        character = inlineBuffer.buffer[index - inlineBuffer.bufferedRangeStart];
                    }
                    ++index;
                    buffer[count++] = character;
                } while (index != end);
                if (range.length == 1 && !OCFCharacterSetIsCharacterMember(charsToSkip, character)) {
                    transform(buffer, &count, character);
                }
            }
            index = end;
        } while (end < length);
    }
    buffer[count] = 0;
    OCFMutableStringRef wrapper =
        OCFStringCreateMutableWithExternalCharactersNoCopy(NULL, buffer, count, count, kOCFAllocatorNull);
    OCFStringRef result = OCFStringCreateCopy(allocator, wrapper);
    OCFRelease(copy);
    OCFRelease(wrapper);
    free(buffer);
    return result;
}

// Native _CFTallStringCreate.tallCharacters, 0x1c0373608 (__TEXT,__const, seven UTF-16 units). Non-external.
static const UniChar tallCharacters[7] = {0x0921, 0x094d, 0x0921, 0x0942, 0x0e01, 0x0e36, 0x0e4a};

// Native _CFTallStringCreate, 0x1c0196bcc, with ___CFTallStringCreate_block_invoke (0x1c0196c70). External.
// The string between two copies of the tall characters (CFStringCreateWithCharacters with a NULL allocator;
// "%@%@%@" with kCFAllocatorDefault) is transformed with expansion 7, skipping the predefined set 8
// (non-base): after each whitespace or newline unit (predefined set 3) the transform writes the seven tall
// characters and a space (count + 8). Both intermediate strings are released.
OCFStringRef _OCFTallStringCreate(OCFStringRef string) {
    OCFStringRef tall = OCFStringCreateWithCharacters(NULL, tallCharacters, 7);
    OCFStringRef framed =
        OCFStringCreateWithFormat(kOCFAllocatorDefault, NULL, OF_STR("%@%@%@"), tall, string, tall);
    OCFStringRef result = __CFPseudoLanguageStringCreateWithTransform(
        framed, 7, OCFCharacterSetGetPredefined(8), ^(UniChar *buffer, OCFIndex *count, UniChar character) {
          OCFIndex index = *count;
          if (OCFCharacterSetIsCharacterMember(OCFCharacterSetGetPredefined(3), character)) {
              UniChar *destination = buffer + index;
              ++index;
              for (OCFIndex offset = 0; offset != 7; ++offset) {
                  destination[offset] = tallCharacters[offset];
                  ++index;
              }
              destination[7] = 0x20;
          }
          *count = index;
        }
    );
    OCFRelease(tall);
    OCFRelease(framed);
    return result;
}

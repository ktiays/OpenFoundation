//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/URL/OCFURLPathExtension.h"

// Inferred organization: path component extension parsing. iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for the UniChar input path.
// iOS Device divergence: unread. Reading: Local/Evidence/cf-core/iphone-simulator-corpus-delta13.json.

// Native _CFGetPathExtensionRangesFromPathComponentUniChars, 0x1c025832c. External.
// The inline buffer starts with only the direct UTF-16 pointer set. For NULL input the native path
// attempts to fill from a NULL CFString (kept). Range locations exclude their preceding dot.
void _OCFGetPathExtensionRangesFromPathComponentUniChars(
    const UniChar *characters, OCFIndex length, OCFRange *primary, OCFRange *secondary
) {
    if (primary == NULL && secondary == NULL) {
        return;
    }
    OCFRange last = {-1, 0};
    OCFRange prior = {-1, 0};
    if (length >= 2) {
        UniChar buffer[64] = {0};
        OCFIndex bufferStart = 0;
        for (OCFIndex dot = length - 1; dot >= 1; --dot) {
            if (characters == NULL && (dot == length - 1 || dot < bufferStart)) {
                bufferStart = dot >= 63 ? dot - 63 : 0;
                OCFIndex count = length - bufferStart;
                if (count > 64) {
                    count = 64;
                }
                OCFStringGetCharacters(NULL, (OCFRange){bufferStart, count}, buffer);
            }
            UniChar value = characters != NULL ? characters[dot] : buffer[dot - bufferStart];
            if (value == ' ' || value == '/' || value == 0x061c || (value >= 0x200e && value <= 0x200f) ||
                (value >= 0x202a && value <= 0x202e) || (value >= 0x2066 && value <= 0x2069)) {
                break;
            }
            if (value != '.') {
                continue;
            }
            if (dot == length - 1 || (dot <= 2 && characters != NULL && characters[0] == '.' &&
                                      (dot == 1 || characters[1] == '.'))) {
                break;
            }
            last = (OCFRange){dot + 1, length - dot - 1};
            if (secondary != NULL && dot >= 2 && characters != NULL) {
                OCFIndex end = dot - 1;
                while (end >= 1 && characters[end] == ' ') {
                    --end;
                }
                OCFIndex separator = end;
                while (separator >= 1 && characters[separator] != '.' && characters[separator] != ' ') {
                    --separator;
                }
                OCFIndex priorDot = separator;
                while (priorDot >= 1 && characters[priorDot] == ' ') {
                    --priorDot;
                }
                if (priorDot >= 1 && characters[priorDot] == '.' && end > separator) {
                    prior = (OCFRange){separator + 1, end - separator};
                }
            }
            break;
        }
    }
    if (primary != NULL) {
        *primary = last;
    }
    if (secondary != NULL) {
        *secondary = prior;
    }
}

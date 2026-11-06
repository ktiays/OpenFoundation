//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/String/OCFStringPropertyListSupport.h"

#include <string.h>

// Inferred organization: __CFStringContainsNullCharacter (0x1c01d8110) lies in the property-list unit of the
// iOS Simulator __text section, between __plistAppendString and __CFPropertyListIsArrayPlistAux, and only
// __CFPropertyListIsValidAux and __CFPropertyListIsDictPlistAux call it. It is kept in this translation unit,
// with hidden linkage, until the property-list unit is restored; there it becomes static again. The native
// source path is not in the evidence. iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified; iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationString.md.

// Native __CFStringContainsNullCharacter, 0x1c01d8110. Non-external (hidden here; see above). An empty string
// answers false. The direct UTF-16 contents are scanned first, then the UTF-8 pointer from
// _CFStringGetUTF8PtrWithOptions(string, &length, false, true), and otherwise every character through an
// inline buffer over the whole string (its 128-byte character buffer zeroed first; the index never leaves the
// range, so the native code omits the range test, which the shared accessor keeps).
Boolean __CFStringContainsNullCharacter(OCFStringRef string) {
    OCFIndex length = OCFStringGetLength(string);
    if (length == 0) {
        return false;
    }
    const UniChar *characters = OCFStringGetCharactersPtr(string);
    if (characters != NULL) {
        for (OCFIndex index = 0; index < length; ++index) {
            if (characters[index] == 0) {
                return true;
            }
        }
        return false;
    }
    OCFIndex utf8Length = 0;
    const char *utf8 = _CFStringGetUTF8PtrWithOptions(string, &utf8Length, false, true);
    if (utf8 != NULL) {
        for (OCFIndex index = 0; index < utf8Length; ++index) {
            if (utf8[index] == 0) {
                return true;
            }
        }
        return false;
    }
    OCFStringInlineBuffer buffer;
    memset(buffer.buffer, 0, sizeof(buffer.buffer));
    stringInitInlineBuffer(string, &buffer, (OCFRange){0, length});
    for (OCFIndex index = 0; index < length; ++index) {
        if (stringGetCharacterFromInlineBuffer(&buffer, index) == 0) {
            return true;
        }
    }
    return false;
}

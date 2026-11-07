//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/String/OCFTaggedPointerString.h"

#include <objc/message.h>
#include <objc/runtime.h>
#include <string.h>

// Inferred organization: the static helpers isEqualToTaggedPointer (0x1c0159f88) and isEqualToString
// (0x1c015a180), __NSInlineTaggedPointerStringIsEqualToTaggedPointer (0x1c015a344) and
// __NSTaggedPointerStringGetBytes and
// __NSTaggedPointerStringGetLength (0x1c015ad48-0x1c015ae14) sit among the NSTaggedPointerString methods in
// the iOS Simulator __text section (after -[NSTaggedPointerString isEqualToString:] and before
// -[NSTaggedPointerString lowercaseStringWithLocale:]); they are the C helpers of that Objective-C unit and
// are kept in this translation unit until the unit itself is restored. The native source path is not in the
// evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationString.md.

// An inline tagged string (tag index 2) holds its length in bits 3-6 of the decoded pointer and its
// characters above bit 7: up to seven bytes verbatim, or eight to eleven characters in six-bit (8 and 9
// characters) or five-bit (10 and 11 characters) codes of sixBitToCharLookup.

// Native sixBitToCharLookup, 0x1c0372600 (__TEXT,__const, 64 bytes, unterminated; charToSixBitLookup
// follows at 0x1c0372640). Non-external.
static const char sixBitToCharLookup[64] __attribute__((nonstring)) =
    "eilotrm.apdnsIc ufkMShjTRxgC4013bDNvwyUL2O856P-B79AFKEWV_zGJ/HYX";

// Native isEqualToString, 0x1c015a180 (defined below in native order).
static bool isEqualToString(OCFTypeRef string, OCFTypeRef other);

// Native isEqualToTaggedPointer, 0x1c0159f88. Non-external. string is an inline tagged string, other another
// tagged pointer. other's tag is searched in the first seven permutation entries (an extended tag is 8 plus
// bits 55-62 of the decoded pointer): tag 22 is answered by isEqualToString (a tail call), an indirect tagged
// string (tag 0) is compared by length (bits 3-6 of string, decoded with the obfuscator's low 32 bits or 7)
// and then memcmp of the decoded characters against _CFIndirectTaggedPointerStringGetContents; any other tag
// is unequal. The characters are decoded into a zeroed 16-byte buffer as _NSTaggedPointerStringGetBytes does.
static bool isEqualToTaggedPointer(OCFTypeRef string, OCFTypeRef other) {
    uintptr_t obfuscator = objc_debug_taggedpointer_obfuscator;
    uintptr_t otherPointer = (uintptr_t)other;
    uintptr_t otherDecoded = ((~otherPointer & 0xC000000000000007) == 0 ? 0 : obfuscator) ^ otherPointer;
    uintptr_t index = 0;
    do {
        if ((otherDecoded & 7) == objc_debug_tag60_permutations[index]) {
            break;
        }
        ++index;
    } while (index != 7);
    uint32_t tag = (uint32_t)(index & 7);
    if (tag == 7) {
        tag = (uint32_t)((OF_WORD_SHIFT((index | otherDecoded), 55)) & 0xFF) + 8;
    }
    if (tag == 22) {
        return isEqualToString(string, other);
    }
    if (tag != 0) {
        return false;
    }
    uintptr_t pointer = (uintptr_t)string;
    uintptr_t flags = pointer & 0xC000000000000007;
    uint32_t lengthObfuscator = flags == 0xC000000000000007 ? 7 : (uint32_t)obfuscator;
    uintptr_t length = ((lengthObfuscator ^ (uint32_t)pointer) >> 3) & 0xF;
    uint16_t otherLength = 0;
    const UInt8 *contents = _OCFIndirectTaggedPointerStringGetContents((OCFStringRef)other, &otherLength);
    if (length != otherLength) {
        return false;
    }
    uint64_t buffer[2] = {0, 0};
    uintptr_t decoded = (flags == 0xC000000000000007 ? 0 : objc_debug_taggedpointer_obfuscator) ^ pointer;
    uintptr_t mask = (~(uint32_t)decoded & 7) == 0 ? UINT64_C(0xFFFFFFFFFFFFF) : UINT64_C(0xFFFFFFFFFFFFFFF);
    uint64_t payload = (mask & (decoded >> 3)) >> 4;
    size_t count = (decoded >> 3) & 0xF;
    if (count < 8) {
        buffer[0] = payload;
    } else {
        UInt8 *last = (UInt8 *)buffer - 1;
        size_t position = count;
        if (count < 10) {
            do {
                last[position] = (UInt8)sixBitToCharLookup[payload & 0x3F];
                payload >>= 6;
                --position;
            } while (position != 0);
        } else {
            do {
                last[position] = (UInt8)sixBitToCharLookup[payload & 0x1F];
                payload >>= 5;
                --position;
            } while (position != 0);
        }
    }
    return memcmp(buffer, contents, count) == 0;
}

// Native isEqualToString, 0x1c015a180. Non-external. string is an inline tagged string, other an object
// answering -length and -getCharacters:range:. The lengths must match ([other length] against bits 3-6 of
// string); the characters are decoded into a zeroed 16-byte buffer, other's first length units are fetched
// into a 16-unit buffer with getCharacters:range: {0, length}, and every unit is compared with the
// zero-extended byte. An empty string is equal after the fetch.
static bool isEqualToString(OCFTypeRef string, OCFTypeRef other) {
    uintptr_t pointer = (uintptr_t)string;
    uintptr_t flags = pointer & 0xC000000000000007;
    uint32_t lengthObfuscator =
        flags == 0xC000000000000007 ? 7 : (uint32_t)objc_debug_taggedpointer_obfuscator;
    uintptr_t length = ((lengthObfuscator ^ (uint32_t)pointer) >> 3) & 0xF;
    if (length != ((uintptr_t (*)(id, SEL))objc_msgSend)((id)other, sel_getUid("length"))) {
        return false;
    }
    uint64_t buffer[2] = {0, 0};
    UniChar characters[16];
    uintptr_t decoded = (flags == 0xC000000000000007 ? 0 : objc_debug_taggedpointer_obfuscator) ^ pointer;
    uintptr_t mask = (~(uint32_t)decoded & 7) == 0 ? UINT64_C(0xFFFFFFFFFFFFF) : UINT64_C(0xFFFFFFFFFFFFFFF);
    uint64_t payload = (mask & (decoded >> 3)) >> 4;
    uintptr_t count = (decoded >> 3) & 0xF;
    if (count < 8) {
        buffer[0] = payload;
        ((void (*)(id, SEL, UniChar *, OCFRange))objc_msgSend)(
            (id)other, sel_getUid("getCharacters:range:"), characters, __OCFRangeMake(0, (OCFIndex)length)
        );
        if (count == 0) {
            return true;
        }
    } else {
        UInt8 *last = (UInt8 *)buffer - 1;
        uintptr_t position = count;
        if (count < 10) {
            do {
                last[position] = (UInt8)sixBitToCharLookup[payload & 0x3F];
                payload >>= 6;
                --position;
            } while (position != 0);
        } else {
            do {
                last[position] = (UInt8)sixBitToCharLookup[payload & 0x1F];
                payload >>= 5;
                --position;
            } while (position != 0);
        }
        ((void (*)(id, SEL, UniChar *, OCFRange))objc_msgSend)(
            (id)other, sel_getUid("getCharacters:range:"), characters, __OCFRangeMake(0, (OCFIndex)length)
        );
    }
    const UInt8 *bytes = (const UInt8 *)buffer;
    bool equal;
    uintptr_t index = 0;
    do {
        equal = characters[index] == bytes[index];
        ++index;
    } while (equal && index < count);
    return equal;
}

// Native _NSTaggedPointerStringEqualCFString, 0x1c015a340 (corpus delta capture 7). Hidden (was private
// external). A tail call of the unit's static isEqualToString with the inline tagged string and the CF
// string.
Boolean _NSTaggedPointerStringEqualCFString(OCFTypeRef taggedString, OCFStringRef string) {
    return isEqualToString(taggedString, string);
}

// Native __NSInlineTaggedPointerStringIsEqualToTaggedPointer, 0x1c015a344. Hidden (was private external).
// Identical pointers are equal; otherwise the unit's static isEqualToTaggedPointer decides (a tail call).
Boolean __NSInlineTaggedPointerStringIsEqualToTaggedPointer(OCFTypeRef string, OCFTypeRef other) {
    if (string == other) {
        return true;
    }
    return isEqualToTaggedPointer(string, other);
}

// Native __NSTaggedPointerStringGetBytes, 0x1c015ad48. External. The pointer is decoded with the obfuscator
// (skipped when bits 63, 62 and 0-2 are set); the payload is the decoded value shifted right by 7, masked to
// 52 bits when the decoded low three bits are all set (an extended tag) and to 56 bits otherwise. Up to seven
// characters the eight payload bytes are stored as one 64-bit word; longer strings are written backwards, one
// lookup byte per six-bit (fewer than ten characters) or five-bit code. Returns the length; buffer is not
// terminated.
OCFIndex _ONSTaggedPointerStringGetBytes(OCFStringRef string, UInt8 *buffer) {
    uintptr_t pointer = (uintptr_t)string;
    uintptr_t obfuscator = (~pointer & 0xC000000000000007) == 0 ? 0 : objc_debug_taggedpointer_obfuscator;
    uintptr_t decoded = obfuscator ^ pointer;
    uintptr_t mask = (~(uint32_t)decoded & 7) == 0 ? UINT64_C(0xFFFFFFFFFFFFF) : UINT64_C(0xFFFFFFFFFFFFFFF);
    uint64_t payload = (mask & (decoded >> 3)) >> 4;
    OCFIndex length = (OCFIndex)((decoded >> 3) & 0xF);
    if (length < 8) {
        memcpy(buffer, &payload, sizeof(payload));
        return length;
    }
    UInt8 *last = buffer - 1;
    OCFIndex index = length;
    if (length < 10) {
        do {
            last[index] = (UInt8)sixBitToCharLookup[payload & 0x3F];
            payload >>= 6;
            --index;
        } while (index != 0);
    } else {
        do {
            last[index] = (UInt8)sixBitToCharLookup[payload & 0x1F];
            payload >>= 5;
            --index;
        } while (index != 0);
    }
    return length;
}

// Native __NSTaggedPointerStringGetLength, 0x1c015adec. External. Bits 3-6 of the decoded pointer. Only the
// obfuscator's low 32 bits are loaded (LDR W8) and the no-obfuscation case uses 7 instead of 0; neither
// changes bits 3-6.
OCFIndex _ONSTaggedPointerStringGetLength(OCFStringRef string) {
    uintptr_t pointer = (uintptr_t)string;
    uint32_t obfuscator =
        (~pointer & 0xC000000000000007) == 0 ? 7 : (uint32_t)objc_debug_taggedpointer_obfuscator;
    return (OCFIndex)(((obfuscator ^ (uint32_t)pointer) >> 3) & 0xF);
}

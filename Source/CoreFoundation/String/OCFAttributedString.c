//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/String/OCFAttributedString.h"

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/String/OCFStringFormat.h"

#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: __CFAttributedStringDeallocate (0x1c014da40) through
// __CFAttributedStringCheckAndSetAttribute (0x1c014f80c) form the CFAttributedString cluster of the iOS
// Simulator __text section, between CFArray and CFBag, with CFAttributedStringSetAttributes.cold.1 at
// 0x1c0305f68; this unit keeps the restored functions in native order under String/ (the native source path
// is not in the evidence). The class callbacks __CFAttributedStringDeallocate, __CFAttributedStringEqual,
// __CFAttributedStringHash and __CFAttributedStringCopyDescription (with its .cold.1, 0x1c0305f54) and the
// class record of type 62 are restored from iphone-simulator-corpus-delta5.json. Not restored (not in the
// evidence): CFAttributedStringCreate (0x1c014e6d0), _CFAttributedStringCreateWithRuns (0x1c014e9b0),
// _CFAttributedStringGetNumberOfRuns (0x1c014ef0c), _CFAttributedStringGetRuns (0x1c014efd4),
// CFAttributedStringGetMutableString (0x1c014f0b4), CFAttributedStringBeginEditing and EndEditing
// (0x1c014f5b8, 0x1c014f5f8), _CFAttributedStringIsMutable (0x1c014f638) and __CFAttributedStringMtbl
// (0x1c014f64c). Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full; pseudocode used as a
// guide only); iOS Device 24A446: unread. Reading: not yet written; evidence
// Local/Evidence/cf-core/iphone-simulator-corpus-delta4.json (the type ID function:
// iphone-simulator-core-corpus.json).

enum {
    kAttributedStringTypeID = 62,
    kInfoImmutable = 0x1,
};

static OCFMutableDictionaryRef
__CFAttributedStringCreateAttributesDictionary(OCFAllocatorRef alloc, OCFDictionaryRef attributes);

// The helpers below reproduce code the native functions inline at each use (no native symbols): the
// Objective-C test (a tagged pointer, or __CF_IS_OBJC for type 62), the info word (acquire loads) and the
// immutable bit (set with a compare-and-swap loop), and __CFGetAllocator.
static bool isObjCAttributedString(OCFTypeRef cf) {
    return ((uintptr_t)cf >> 63) != 0 || __CF_IS_OBJC(kAttributedStringTypeID, cf) != 0;
}

static uint64_t infoOf(OCFTypeRef cf) {
    return __atomic_load_n(&((const OCFRuntimeBase *)cf)->info, __ATOMIC_ACQUIRE);
}

static void setImmutable(OCFTypeRef cf) {
    uint64_t *info = &((OCFRuntimeBase *)cf)->info;
    uint64_t expected = __atomic_load_n(info, __ATOMIC_ACQUIRE);
    while (!__atomic_compare_exchange_n(
        info, &expected, expected | kInfoImmutable, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
    )) {
    }
}

static OCFAllocatorRef storedAllocator(OCFTypeRef cf) {
    if ((infoOf(cf) & 0x80) != 0) {
        return kOCFAllocatorSystemDefault;
    }
    return *(const OCFAllocatorRef *)((const uint8_t *)cf - 16);
}

static void __CFAttributedStringDeallocate(OCFTypeRef cf);
static Boolean __CFAttributedStringEqual(OCFTypeRef cf1, OCFTypeRef cf2);
static OCFHashCode __CFAttributedStringHash(OCFTypeRef cf);
static OCFStringRef __CFAttributedStringCopyDescription(OCFTypeRef cf);

// Native __CFAttributedStringClass, 0x222429110 (__DATA_CONST,__const). Hidden (was a private external);
// version 0, name "CFAttributedString", finalize, equal, hash and the debug description (the record's words,
// read with macho.py).
const OCFRuntimeClass __CFAttributedStringClass = {
    0,
    "CFAttributedString",
    NULL,
    NULL,
    __CFAttributedStringDeallocate,
    __CFAttributedStringEqual,
    __CFAttributedStringHash,
    NULL,
    __CFAttributedStringCopyDescription,
    NULL,
    NULL,
    0,
};

// Native __CFAttributedStringDeallocate, 0x1c014da40. Non-external. Releases the string, then the run array
// (a tail call).
static void __CFAttributedStringDeallocate(OCFTypeRef cf) {
    OCFAttributedStringRef aStr = (OCFAttributedStringRef)cf;
    OCFRelease(aStr->string);
    OCFRelease(aStr->attributeArray);
}

// Native __CFAttributedStringEqual, 0x1c014da68. Non-external. The second object's string is -string when it
// is an Objective-C object (a tagged pointer, or __CF_IS_OBJC for type 62), otherwise read directly; unequal
// strings answer false. Over the first string's length (read again from the object), the attribute
// dictionaries are compared run by run: each side's attributes are fetched (CFAttributedStringGetAttributes)
// when the location reaches the end of its last range (both ranges start empty), and the location advances to
// the nearer range end. The first unequal pair answers false; an empty string answers true.
static Boolean __CFAttributedStringEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    OCFAttributedStringRef aStr1 = (OCFAttributedStringRef)cf1;
    OCFAttributedStringRef aStr2 = (OCFAttributedStringRef)cf2;
    OCFStringRef string1 = aStr1->string;
    OCFStringRef string2;
    if (isObjCAttributedString(cf2)) {
        string2 = ((OCFStringRef (*)(id, SEL))objc_msgSend)((id)cf2, sel_getUid("string"));
    } else {
        string2 = aStr2->string;
    }
    if (!OCFEqual(string1, string2)) {
        return false;
    }
    OCFIndex length = OCFStringGetLength(aStr1->string);
    OCFRange range1 = {0, 0};
    OCFRange range2 = {0, 0};
    if (length < 1) {
        return true;
    }
    OCFDictionaryRef attributes1 = NULL;
    OCFDictionaryRef attributes2 = NULL;
    OCFIndex location = 0;
    do {
        if (location >= range1.location + range1.length) {
            attributes1 = OCFAttributedStringGetAttributes(aStr1, location, &range1);
        }
        if (location >= range2.location + range2.length) {
            attributes2 = OCFAttributedStringGetAttributes(aStr2, location, &range2);
        }
        if (!OCFEqual(attributes1, attributes2)) {
            return false;
        }
        OCFIndex end2 = range2.location + range2.length;
        OCFIndex end1 = range1.location + range1.length;
        location = end1 < end2 ? end1 : end2;
    } while (location < length);
    return true;
}

// Native __CFAttributedStringHash, 0x1c014dbb4. Non-external. The string's hash (a tail call).
static OCFHashCode __CFAttributedStringHash(OCFTypeRef cf) {
    return OCFHash(((OCFAttributedStringRef)cf)->string);
}

// Native __CFAttributedStringCopyDescription, 0x1c014dbbc, with __CFAttributedStringCopyDescription.cold.1
// (0x1c0305f54). Non-external. A mutable string of kCFAllocatorSystemDefault (no maximum) receives, for each
// run from location 0 while the location is below the string's length (read once): the run's substring
// (created with kCFAllocatorSystemDefault, appended and released), " %p {" with the attribute dictionary,
// then its pairs, last first, as "%@=%@%s" with the key, the value and ", " (or "" after the first pair),
// then "} Len %ld\n" with the run's length; the location becomes the range's end. The keys and values are
// gathered into two 256-entry stack buffers (of which the native code zeroes the first 64 entries, 512 bytes
// each, on every run) when the count is at most 256, otherwise into two malloc_type_malloc buffers (8 bytes
// per entry, type 0xC0040B8AA526D) that are freed after the run. A count of 0x0FFFFFFFFFFFFFFF or more
// (unsigned) crashes with "Ridiculous size count" (.cold.1). The strings are the __cfstring constants
// 0x2227ac1a0 (" %p {", named cfstr_P by the capture; its binding by the section's order is medium
// confidence), 0x2227ac1c0 and 0x2227ac1e0.
static OCFStringRef __CFAttributedStringCopyDescription(OCFTypeRef cf) {
    OCFAttributedStringRef aStr = (OCFAttributedStringRef)cf;
    OCFAllocatorRef alloc = kOCFAllocatorSystemDefault;
    OCFMutableStringRef description = OCFStringCreateMutable(alloc, 0);
    OCFIndex length = OCFStringGetLength(aStr->string);
    OCFRange range = {0, 0};
    if (length < 1) {
        return description;
    }
    do {
        OCFDictionaryRef attributes = OCFAttributedStringGetAttributes(aStr, range.location, &range);
        OCFStringRef substring = OCFStringCreateWithSubstring(alloc, aStr->string, range);
        OCFStringAppend(description, substring);
        OCFRelease(substring);
        OCFStringAppendFormat(description, NULL, OF_STR(" %p {"), attributes);
        OCFIndex count = OCFDictionaryGetCount(attributes);
        const void *keysBuffer[256];
        const void *valuesBuffer[256];
        memset(keysBuffer, 0, 512);
        if ((uint64_t)count >= 0x0FFFFFFFFFFFFFFF) {
            crashWithMessage("Ridiculous size count");
        }
        const void **keys;
        const void **values;
        if ((uint64_t)count < 0x101) {
            memset(valuesBuffer, 0, 512);
            keys = keysBuffer;
            values = valuesBuffer;
            OCFDictionaryGetKeysAndValues(attributes, keys, values);
        } else {
            keys = malloc_type_malloc((size_t)count << 3, 0xC0040B8AA526D);
            memset(valuesBuffer, 0, 512);
            values = malloc_type_malloc((size_t)count << 3, 0xC0040B8AA526D);
            OCFDictionaryGetKeysAndValues(attributes, keys, values);
        }
        for (OCFIndex idx = count; idx != 0;) {
            idx--;
            OCFStringAppendFormat(
                description, NULL, OF_STR("%@=%@%s"), keys[idx], values[idx], idx == 0 ? "" : ", "
            );
        }
        OCFStringAppendFormat(description, NULL, OF_STR("} Len %ld\n"), range.length);
        range.location = range.location + range.length;
        if (keys != keysBuffer) {
            free(keys);
        }
        if (values != valuesBuffer) {
            free(values);
        }
    } while (range.location < length);
    return description;
}

// Native CFAttributedStringGetTypeID, 0x1c014df14. External; always 62 (moved here from Runtime/OCFRuntime.c,
// whose __CFInitialize bridges type 62; declared by Runtime/OCFRuntime.h).
OCFTypeID OCFAttributedStringGetTypeID(void) {
    return kAttributedStringTypeID;
}

// Native __CFAttributedStringCreateMutableWithSubstring, 0x1c014df1c. External. A new mutable attributed
// string (maximum length 0) receives the substring of aStr's string (read from the object directly) at 0,
// then each run of aStr's attributes from the range's start to its end, clipped to the range, is set with
// clearing at its offset from the range's start; the walk advances by the clipped length.
OCFMutableAttributedStringRef __OCFAttributedStringCreateMutableWithSubstring(
    OCFAllocatorRef alloc, OCFAttributedStringRef aStr, OCFRange range
) {
    OCFMutableAttributedStringRef result = OCFAttributedStringCreateMutable(alloc, 0);
    OCFStringRef substring = OCFStringCreateWithSubstring(alloc, aStr->string, range);
    OCFAttributedStringReplaceString(result, (OCFRange){0, 0}, substring);
    OCFRelease(substring);
    if (range.length < 1) {
        return result;
    }
    OCFIndex end = range.location + range.length;
    OCFIndex location = range.location;
    do {
        OCFRange effectiveRange = {0, 0};
        OCFDictionaryRef attributes = OCFAttributedStringGetAttributes(aStr, location, &effectiveRange);
        OCFIndex effectiveEnd = effectiveRange.location + effectiveRange.length;
        OCFIndex length =
            effectiveEnd > end ? end - location : effectiveRange.location - location + effectiveRange.length;
        if (effectiveRange.location != location || effectiveEnd > end) {
            effectiveRange.length = length;
        }
        OCFAttributedStringSetAttributes(
            result, (OCFRange){location - range.location, length}, attributes, true
        );
        location = effectiveRange.length + location;
    } while (location < end);
    return result;
}

// Native CFAttributedStringCreateMutable, 0x1c014e040. External. A type 62 object (16 extra bytes) with an
// empty mutable string of the maximum length and an empty run array, both on the given allocator.
OCFMutableAttributedStringRef OCFAttributedStringCreateMutable(OCFAllocatorRef alloc, OCFIndex maxLength) {
    OCFMutableAttributedStringRef aStr =
        (OCFMutableAttributedStringRef)_OCFRuntimeCreateInstance(alloc, kAttributedStringTypeID, 16, NULL);
    if (aStr == NULL) {
        return NULL;
    }
    aStr->string = OCFStringCreateMutable(alloc, maxLength);
    aStr->attributeArray = OCFRunArrayCreate(alloc);
    return aStr;
}

// Native CFAttributedStringReplaceString, 0x1c014e0a0. External. Objective-C:
// -replaceCharactersInRange:withString:. A nonempty replacement takes the attributes at the range's start
// (a nonempty range: replaced over the range) or of the character before it (an insertion; at location 0 the
// first character's, or a new empty dictionary with the copy-string key and CFType value callbacks when the
// string is empty), retained for the run array change and released after it; an empty replacement deletes
// the range's attributes (an empty range with an empty replacement changes no attributes). The string is
// then replaced (a tail call).
void OCFAttributedStringReplaceString(
    OCFMutableAttributedStringRef aStr, OCFRange range, OCFStringRef replacement
) {
    if (isObjCAttributedString(aStr)) {
        ((void (*)(id, SEL, OCFRange, OCFStringRef))objc_msgSend)(
            (id)aStr, sel_getUid("replaceCharactersInRange:withString:"), range, replacement
        );
        return;
    }
    OCFIndex newLength = OCFStringGetLength(replacement);
    OCFTypeRef attributes;
    if (newLength >= 1) {
        if (range.length >= 1) {
            attributes = OCFRunArrayGetValueAtIndex(aStr->attributeArray, range.location, NULL, NULL);
            OCFRetain(attributes);
            goto replace;
        }
        if (range.location - 1 >= 0) {
            attributes = OCFRunArrayGetValueAtIndex(aStr->attributeArray, range.location - 1, NULL, NULL);
            OCFRetain(attributes);
        } else if (OCFStringGetLength(aStr->string) >= 1) {
            attributes = OCFRunArrayGetValueAtIndex(aStr->attributeArray, 0, NULL, NULL);
            OCFRetain(attributes);
        } else {
            attributes = OCFDictionaryCreateMutable(
                OCFGetAllocator(aStr),
                0,
                &kOCFCopyStringDictionaryKeyCallBacks,
                &kOCFTypeDictionaryValueCallBacks
            );
        }
        goto insert;
    }
    if (range.length >= 1) {
        attributes = NULL;
        goto replace;
    }
    if (newLength == 0) {
        goto replaceString;
    }
    attributes = NULL;
    goto insert;
replace:
    OCFRunArrayReplace(aStr->attributeArray, range, attributes, newLength);
    if (attributes != NULL) {
        OCFRelease(attributes);
    }
    goto replaceString;
insert:
    OCFRunArrayInsert(aStr->attributeArray, (OCFRange){range.location, newLength}, attributes);
    if (attributes != NULL) {
        OCFRelease(attributes);
    }
replaceString:
    OCFStringReplace(aStr->string, range, replacement);
}

// Native CFAttributedStringGetAttributes, 0x1c014e218. External. Objective-C:
// -attributesAtIndex:effectiveRange:; otherwise the run array's value (a tail call).
OCFDictionaryRef
OCFAttributedStringGetAttributes(OCFAttributedStringRef aStr, OCFIndex loc, OCFRange *effectiveRange) {
    if (isObjCAttributedString(aStr)) {
        return ((OCFDictionaryRef (*)(id, SEL, OCFIndex, OCFRange *))objc_msgSend)(
            (id)aStr, sel_getUid("attributesAtIndex:effectiveRange:"), loc, effectiveRange
        );
    }
    return (OCFDictionaryRef)OCFRunArrayGetValueAtIndex(aStr->attributeArray, loc, effectiveRange, NULL);
}

// Native CFAttributedStringSetAttributes, 0x1c014e284, with CFAttributedStringSetAttributes.cold.1
// (0x1c0305f68). External. Clearing: Objective-C -setAttributes:range:; otherwise a nonempty range gets a
// mutable copy of the attributes (a new empty dictionary for NULL) over the whole range. Adding: Objective-C
// -addAttributes:range:; otherwise the attributes' keys and values (none: nothing) are read into two
// 256-entry zeroed stack buffers, or two malloc_type_malloc buffers (type 0xC0040B8AA526D) above 256 entries;
// a count of 0x0FFFFFFFFFFFFFFF or more stores "Ridiculous size count" and traps. Each run of the range then
// takes the pairs: in place when the run lies within the range from its start and its dictionary has a retain
// count of 1, otherwise in a mutable copy that replaces the run clipped to the range. The buffers are freed
// when allocated.
void OCFAttributedStringSetAttributes(
    OCFMutableAttributedStringRef aStr,
    OCFRange range,
    OCFDictionaryRef replacement,
    Boolean clearOtherAttributes
) {
    if (clearOtherAttributes) {
        if (isObjCAttributedString(aStr)) {
            ((void (*)(id, SEL, OCFDictionaryRef, OCFRange))objc_msgSend)(
                (id)aStr, sel_getUid("setAttributes:range:"), replacement, range
            );
            return;
        }
        if (range.length == 0) {
            return;
        }
        OCFMutableDictionaryRef attributes =
            __CFAttributedStringCreateAttributesDictionary(OCFGetAllocator(aStr), replacement);
        OCFRunArrayReplace(aStr->attributeArray, range, attributes, range.length);
        OCFRelease(attributes);
        return;
    }
    if (isObjCAttributedString(aStr)) {
        ((void (*)(id, SEL, OCFDictionaryRef, OCFRange))objc_msgSend)(
            (id)aStr, sel_getUid("addAttributes:range:"), replacement, range
        );
        return;
    }
    OCFIndex count = OCFDictionaryGetCount(replacement);
    if (count == 0) {
        return;
    }
    const void *keysBuffer[256] = {0};
    if ((uint64_t)count >= 0x0FFFFFFFFFFFFFFF) {
        crashWithMessage("Ridiculous size count");
    }
    const void *valuesBuffer[256] = {0};
    const void **keys;
    const void **values;
    if (count <= 256) {
        values = valuesBuffer;
        keys = keysBuffer;
    } else {
        keys = malloc_type_malloc((size_t)count * 8, 0xC0040B8AA526D);
        values = malloc_type_malloc((size_t)count * 8, 0xC0040B8AA526D);
    }
    OCFDictionaryGetKeysAndValues(replacement, keys, values);
    OCFIndex location = range.location;
    OCFIndex length = range.length;
    while (length != 0) {
        OCFRange effectiveRange = {0, 0};
        OCFMutableDictionaryRef attributes = (OCFMutableDictionaryRef)OCFRunArrayGetValueAtIndex(
            aStr->attributeArray, location, &effectiveRange, NULL
        );
        if (effectiveRange.location == location && effectiveRange.length <= length &&
            OCFGetRetainCount(attributes) == 1) {
            for (OCFIndex idx = 0; idx < count; idx++) {
                OCFDictionarySetValue(attributes, keys[idx], values[idx]);
            }
        } else {
            OCFIndex offset = effectiveRange.location - location;
            if (offset < 0) {
                effectiveRange.location = location;
                effectiveRange.length = offset + effectiveRange.length;
            }
            if (effectiveRange.length > length) {
                effectiveRange.length = length;
            }
            OCFMutableDictionaryRef copy =
                __CFAttributedStringCreateAttributesDictionary(OCFGetAllocator(aStr), attributes);
            for (OCFIndex idx = 0; idx < count; idx++) {
                OCFDictionarySetValue(copy, keys[idx], values[idx]);
            }
            OCFRunArrayReplace(aStr->attributeArray, effectiveRange, copy, effectiveRange.length);
            OCFRelease(copy);
        }
        location = effectiveRange.length + location;
        length -= effectiveRange.length;
    }
    if (keys != keysBuffer) {
        free(keys);
    }
    if (values != valuesBuffer) {
        free(values);
    }
}

// Native __CFAttributedStringCreateAttributesDictionary, 0x1c014e790. Non-external. A mutable copy of the
// attributes (capacity 0), or for NULL a new empty dictionary with the copy-string key and CFType value
// callbacks (tail calls).
static OCFMutableDictionaryRef
__CFAttributedStringCreateAttributesDictionary(OCFAllocatorRef alloc, OCFDictionaryRef attributes) {
    if (attributes != NULL) {
        return OCFDictionaryCreateMutableCopy(alloc, 0, attributes);
    }
    return OCFDictionaryCreateMutable(
        alloc, 0, &kOCFCopyStringDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
}

// Native CFAttributedStringCreateWithSubstring, 0x1c014e7b4. External. Objective-C:
// -_createAttributedSubstringWithRange:. The whole string (location 0 and the full length, read first) is a
// copy (a tail call); otherwise a mutable substring marked immutable (info bit 0).
OCFAttributedStringRef
OCFAttributedStringCreateWithSubstring(OCFAllocatorRef alloc, OCFAttributedStringRef aStr, OCFRange range) {
    if (isObjCAttributedString(aStr)) {
        return ((OCFAttributedStringRef (*)(id, SEL, OCFRange))objc_msgSend)(
            (id)aStr, sel_getUid("_createAttributedSubstringWithRange:"), range
        );
    }
    OCFIndex length = OCFAttributedStringGetLength(aStr);
    if (range.location == 0 && range.length == length) {
        return OCFAttributedStringCreateCopy(alloc, aStr);
    }
    OCFMutableAttributedStringRef result =
        __OCFAttributedStringCreateMutableWithSubstring(alloc, aStr, range);
    setImmutable(result);
    return result;
}

// Native CFAttributedStringGetLength, 0x1c014e874. External. Objective-C: -length; otherwise the string's
// length (a tail call).
OCFIndex OCFAttributedStringGetLength(OCFAttributedStringRef aStr) {
    if (isObjCAttributedString(aStr)) {
        return ((OCFIndex (*)(id, SEL))objc_msgSend)((id)aStr, sel_getUid("length"));
    }
    return OCFStringGetLength(aStr->string);
}

// Native CFAttributedStringCreateCopy, 0x1c014e8b8. External. Objective-C: -copy. An immutable attributed
// string whose allocator (__CFGetAllocator) is the requested one (NULL: the default allocator, the
// thread's slot 1 or the system default) is retained; otherwise a mutable copy of the whole string is marked
// immutable.
OCFAttributedStringRef OCFAttributedStringCreateCopy(OCFAllocatorRef alloc, OCFAttributedStringRef aStr) {
    if (isObjCAttributedString(aStr)) {
        return ((OCFAttributedStringRef (*)(id, SEL))objc_msgSend)((id)aStr, sel_getUid("copy"));
    }
    if ((infoOf(aStr) & kInfoImmutable) != 0) {
        OCFAllocatorRef requested = alloc;
        if (requested == NULL) {
            OCFAllocatorRef current = _OCFGetTSD(1);
            requested = current == NULL ? kOCFAllocatorSystemDefault : current;
        }
        if (requested == storedAllocator(aStr)) {
            return (OCFAttributedStringRef)OCFRetain(aStr);
        }
    }
    OCFMutableAttributedStringRef result = __OCFAttributedStringCreateMutableWithSubstring(
        alloc, aStr, (OCFRange){0, OCFAttributedStringGetLength(aStr)}
    );
    setImmutable(result);
    return result;
}

// Native CFAttributedStringCreateMutableCopy, 0x1c014eadc. External. Objective-C: -mutableCopy; otherwise a
// mutable substring over the whole string (the maximum length is ignored; a tail call).
OCFMutableAttributedStringRef
OCFAttributedStringCreateMutableCopy(OCFAllocatorRef alloc, OCFIndex maxLength, OCFAttributedStringRef aStr) {
    (void)maxLength;
    if (isObjCAttributedString(aStr)) {
        return ((OCFMutableAttributedStringRef (*)(id, SEL))objc_msgSend)(
            (id)aStr, sel_getUid("mutableCopy")
        );
    }
    return __OCFAttributedStringCreateMutableWithSubstring(
        alloc, aStr, (OCFRange){0, OCFStringGetLength(aStr->string)}
    );
}

// Native CFAttributedStringGetString, 0x1c014eb38. External. Objective-C: -string; otherwise the string.
OCFStringRef OCFAttributedStringGetString(OCFAttributedStringRef aStr) {
    if (isObjCAttributedString(aStr)) {
        return ((OCFStringRef (*)(id, SEL))objc_msgSend)((id)aStr, sel_getUid("string"));
    }
    return aStr->string;
}

// Native CFAttributedStringGetAttribute, 0x1c014eb7c. External. Objective-C:
// -attribute:atIndex:effectiveRange:; otherwise the named value of the run's dictionary (a tail call).
OCFTypeRef OCFAttributedStringGetAttribute(
    OCFAttributedStringRef aStr, OCFIndex loc, OCFStringRef attrName, OCFRange *effectiveRange
) {
    if (isObjCAttributedString(aStr)) {
        return ((OCFTypeRef (*)(id, SEL, OCFStringRef, OCFIndex, OCFRange *))objc_msgSend)(
            (id)aStr, sel_getUid("attribute:atIndex:effectiveRange:"), attrName, loc, effectiveRange
        );
    }
    OCFDictionaryRef attributes =
        (OCFDictionaryRef)OCFRunArrayGetValueAtIndex(aStr->attributeArray, loc, effectiveRange, NULL);
    return OCFDictionaryGetValue(attributes, attrName);
}

// Native CFAttributedStringGetAttributesAndLongestEffectiveRange, 0x1c014ebf8. External. Objective-C:
// -attributesAtIndex:longestEffectiveRange:inRange:. Otherwise the attributes at the location; with a range
// pointer, the run is extended forward while the next run's attributes are CFEqual (up to the end of inRange)
// and backward from the run's start while the previous run's are (down to inRange's start), and the result
// is clipped to inRange.
OCFDictionaryRef OCFAttributedStringGetAttributesAndLongestEffectiveRange(
    OCFAttributedStringRef aStr, OCFIndex loc, OCFRange inRange, OCFRange *longestEffectiveRange
) {
    if (isObjCAttributedString(aStr)) {
        return ((OCFDictionaryRef (*)(id, SEL, OCFIndex, OCFRange *, OCFRange))objc_msgSend)(
            (id)aStr,
            sel_getUid("attributesAtIndex:longestEffectiveRange:inRange:"),
            loc,
            longestEffectiveRange,
            inRange
        );
    }
    OCFDictionaryRef attributes = OCFAttributedStringGetAttributes(aStr, loc, longestEffectiveRange);
    if (longestEffectiveRange == NULL) {
        return attributes;
    }
    OCFRange range = *longestEffectiveRange;
    OCFIndex inEnd = inRange.location + inRange.length;
    OCFIndex end;
    while (true) {
        end = range.length + range.location;
        if (end >= inEnd) {
            break;
        }
        if (!OCFEqual(OCFAttributedStringGetAttributes(aStr, end, &range), attributes)) {
            break;
        }
    }
    range = *longestEffectiveRange;
    OCFIndex start;
    while (true) {
        start = range.location;
        if (start <= inRange.location) {
            break;
        }
        if (!OCFEqual(OCFAttributedStringGetAttributes(aStr, start - 1, &range), attributes)) {
            break;
        }
    }
    OCFIndex resultStart = start > inRange.location ? start : inRange.location;
    OCFIndex resultEnd = inEnd < end ? inEnd : end;
    longestEffectiveRange->location = resultStart;
    longestEffectiveRange->length = resultEnd - resultStart;
    return attributes;
}

// Native CFAttributedStringGetAttributeAndLongestEffectiveRange, 0x1c014ed68. External. Objective-C:
// -attribute:atIndex:longestEffectiveRange:inRange:. As the attributes variant for one attribute: runs
// extend while the value is identical, or both are non-NULL and CFEqual.
OCFTypeRef OCFAttributedStringGetAttributeAndLongestEffectiveRange(
    OCFAttributedStringRef aStr,
    OCFIndex loc,
    OCFStringRef attrName,
    OCFRange inRange,
    OCFRange *longestEffectiveRange
) {
    if (isObjCAttributedString(aStr)) {
        return ((OCFTypeRef (*)(id, SEL, OCFStringRef, OCFIndex, OCFRange *, OCFRange))objc_msgSend)(
            (id)aStr,
            sel_getUid("attribute:atIndex:longestEffectiveRange:inRange:"),
            attrName,
            loc,
            longestEffectiveRange,
            inRange
        );
    }
    OCFTypeRef value = OCFAttributedStringGetAttribute(aStr, loc, attrName, longestEffectiveRange);
    if (longestEffectiveRange == NULL) {
        return value;
    }
    OCFRange range = *longestEffectiveRange;
    OCFIndex inEnd = inRange.location + inRange.length;
    OCFIndex end;
    while (true) {
        end = range.length + range.location;
        if (end >= inEnd) {
            break;
        }
        OCFTypeRef next = OCFAttributedStringGetAttribute(aStr, end, attrName, &range);
        if (next == value) {
            continue;
        }
        if (next == NULL || value == NULL || !OCFEqual(next, value)) {
            break;
        }
    }
    range = *longestEffectiveRange;
    OCFIndex start;
    while (true) {
        start = range.location;
        if (start <= inRange.location) {
            break;
        }
        OCFTypeRef previous = OCFAttributedStringGetAttribute(aStr, start - 1, attrName, &range);
        if (previous == value) {
            continue;
        }
        if (previous == NULL || value == NULL || !OCFEqual(previous, value)) {
            break;
        }
    }
    OCFIndex resultStart = start > inRange.location ? start : inRange.location;
    OCFIndex resultEnd = inEnd < end ? inEnd : end;
    longestEffectiveRange->location = resultStart;
    longestEffectiveRange->length = resultEnd - resultStart;
    return value;
}

// Native CFAttributedStringSetAttribute, 0x1c014f0f8. External. Objective-C: -addAttribute:value:range:.
// Otherwise each run of the range takes the value: in place when the run lies within the range from its start
// and its dictionary has a retain count of 1; else, unless the run already holds a CFEqual value, in a
// mutable copy that replaces the run clipped to the range.
void OCFAttributedStringSetAttribute(
    OCFMutableAttributedStringRef aStr, OCFRange range, OCFStringRef attrName, OCFTypeRef value
) {
    if (isObjCAttributedString(aStr)) {
        ((void (*)(id, SEL, OCFStringRef, OCFTypeRef, OCFRange))objc_msgSend)(
            (id)aStr, sel_getUid("addAttribute:value:range:"), attrName, value, range
        );
        return;
    }
    OCFIndex location = range.location;
    OCFIndex length = range.length;
    while (length != 0) {
        OCFRange effectiveRange = {0, 0};
        OCFMutableDictionaryRef attributes = (OCFMutableDictionaryRef)OCFRunArrayGetValueAtIndex(
            aStr->attributeArray, location, &effectiveRange, NULL
        );
        if (effectiveRange.location == location && effectiveRange.length <= length &&
            OCFGetRetainCount(attributes) == 1) {
            OCFDictionarySetValue(attributes, attrName, value);
        } else {
            OCFIndex offset = effectiveRange.location - location;
            if (offset < 0) {
                effectiveRange.location = location;
                effectiveRange.length = offset + effectiveRange.length;
            }
            if (effectiveRange.length > length) {
                effectiveRange.length = length;
            }
            OCFTypeRef existing = OCFDictionaryGetValue(attributes, attrName);
            if (existing == NULL || !OCFEqual(existing, value)) {
                OCFMutableDictionaryRef copy =
                    __CFAttributedStringCreateAttributesDictionary(OCFGetAllocator(aStr), attributes);
                OCFDictionarySetValue(copy, attrName, value);
                OCFRunArrayReplace(aStr->attributeArray, effectiveRange, copy, effectiveRange.length);
                OCFRelease(copy);
            }
        }
        location = effectiveRange.length + location;
        length -= effectiveRange.length;
    }
}

// Native CFAttributedStringRemoveAttribute, 0x1c014f29c. External. Objective-C: -removeAttribute:range:. As
// CFAttributedStringSetAttribute with removal: a copy is made only for a run whose dictionary contains the
// name.
void OCFAttributedStringRemoveAttribute(
    OCFMutableAttributedStringRef aStr, OCFRange range, OCFStringRef attrName
) {
    if (isObjCAttributedString(aStr)) {
        ((void (*)(id, SEL, OCFStringRef, OCFRange))objc_msgSend)(
            (id)aStr, sel_getUid("removeAttribute:range:"), attrName, range
        );
        return;
    }
    OCFIndex location = range.location;
    OCFIndex length = range.length;
    while (length != 0) {
        OCFRange effectiveRange = {0, 0};
        OCFMutableDictionaryRef attributes = (OCFMutableDictionaryRef)OCFRunArrayGetValueAtIndex(
            aStr->attributeArray, location, &effectiveRange, NULL
        );
        if (effectiveRange.location == location && effectiveRange.length <= length &&
            OCFGetRetainCount(attributes) == 1) {
            OCFDictionaryRemoveValue(attributes, attrName);
        } else {
            OCFIndex offset = effectiveRange.location - location;
            if (offset < 0) {
                effectiveRange.location = location;
                effectiveRange.length = offset + effectiveRange.length;
            }
            if (effectiveRange.length > length) {
                effectiveRange.length = length;
            }
            if (OCFDictionaryContainsKey(attributes, attrName)) {
                OCFMutableDictionaryRef copy =
                    __CFAttributedStringCreateAttributesDictionary(OCFGetAllocator(aStr), attributes);
                OCFDictionaryRemoveValue(copy, attrName);
                OCFRunArrayReplace(aStr->attributeArray, effectiveRange, copy, effectiveRange.length);
                OCFRelease(copy);
            }
        }
        location = effectiveRange.length + location;
        length -= effectiveRange.length;
    }
}

// Native CFAttributedStringReplaceAttributedString, 0x1c014f424. External. Objective-C:
// -replaceCharactersInRange:withAttributedString:. The replacement's string is its -string when it is
// Objective-C, otherwise its string field. Each run of the replacement (one effective range, zeroed once, its
// location set to the next run's start) is inserted at its offset from the range's start as a mutable copy of
// its attributes (allocator of aStr, read once); the replaced range's runs are then deleted after the
// inserted length, and the string replaced.
void OCFAttributedStringReplaceAttributedString(
    OCFMutableAttributedStringRef aStr, OCFRange range, OCFAttributedStringRef replacement
) {
    if (isObjCAttributedString(aStr)) {
        ((void (*)(id, SEL, OCFRange, OCFAttributedStringRef))objc_msgSend)(
            (id)aStr, sel_getUid("replaceCharactersInRange:withAttributedString:"), range, replacement
        );
        return;
    }
    OCFStringRef string;
    if (isObjCAttributedString(replacement)) {
        string = ((OCFStringRef (*)(id, SEL))objc_msgSend)((id)replacement, sel_getUid("string"));
    } else {
        string = replacement->string;
    }
    OCFIndex newLength = OCFStringGetLength(string);
    if (newLength >= 1) {
        OCFAllocatorRef alloc = OCFGetAllocator(aStr);
        OCFIndex location = 0;
        OCFRange effectiveRange = {0, 0};
        do {
            OCFDictionaryRef attributes =
                OCFAttributedStringGetAttributes(replacement, location, &effectiveRange);
            OCFMutableDictionaryRef copy = __CFAttributedStringCreateAttributesDictionary(alloc, attributes);
            OCFRunArrayInsert(
                aStr->attributeArray,
                (OCFRange){effectiveRange.location + range.location, effectiveRange.length},
                copy
            );
            OCFRelease(copy);
            location = effectiveRange.location + effectiveRange.length;
            effectiveRange.location = location;
        } while (location < newLength);
    }
    if (range.length >= 1) {
        OCFRunArrayDelete(aStr->attributeArray, (OCFRange){newLength + range.location, range.length});
    }
    OCFStringReplace(aStr->string, range, string);
}

// Native CFAttributedStringBeginEditing, 0x1c014f5b8. External. Objective-C: -beginEditing; a CF attributed
// string does nothing.
void OCFAttributedStringBeginEditing(OCFMutableAttributedStringRef aStr) {
    if (isObjCAttributedString(aStr)) {
        ((void (*)(id, SEL))objc_msgSend)((id)aStr, sel_getUid("beginEditing"));
    }
}

// Native CFAttributedStringEndEditing, 0x1c014f5f8. External. Objective-C: -endEditing; a CF attributed
// string does nothing.
void OCFAttributedStringEndEditing(OCFMutableAttributedStringRef aStr) {
    if (isObjCAttributedString(aStr)) {
        ((void (*)(id, SEL))objc_msgSend)((id)aStr, sel_getUid("endEditing"));
    }
}

// Native __CFAttributedStringMtbl, 0x1c014f64c. External. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta6.json. True when info bit 0 of an acquire load of the
// info word is clear (BIC of the word from 1).
Boolean __OCFAttributedStringMtbl(OCFAttributedStringRef aStr) {
    return 1 & ~(uint32_t)__atomic_load_n(&((const OCFRuntimeBase *)aStr)->info, __ATOMIC_ACQUIRE);
}

// Native _CFAttributedStringGetLength, 0x1c014f660. External. The string's length, without dispatch (a tail
// call).
OCFIndex _OCFAttributedStringGetLength(OCFAttributedStringRef aStr) {
    return OCFStringGetLength(aStr->string);
}

// The range check of the Check entry points: 3 when the location or the end lies beyond the string's length.
static bool rangeExceedsString(OCFAttributedStringRef aStr, OCFIndex location, OCFIndex length) {
    OCFIndex stringLength = OCFStringGetLength(aStr->string);
    return location > stringLength || location + length > stringLength;
}

// Native _CFAttributedStringCheckAndReplace, 0x1c014f668. External. 3 for a range beyond the string, 1 for an
// immutable attributed string, otherwise 0 after CFAttributedStringReplaceString.
int _OCFAttributedStringCheckAndReplace(
    OCFMutableAttributedStringRef aStr, OCFIndex location, OCFIndex length, OCFStringRef replacement
) {
    if (rangeExceedsString(aStr, location, length)) {
        return 3;
    }
    if ((infoOf(aStr) & kInfoImmutable) != 0) {
        return 1;
    }
    OCFAttributedStringReplaceString(aStr, (OCFRange){location, length}, replacement);
    return 0;
}

// Native _CFAttributedStringCheckAndReplaceAttributed, 0x1c014f6e4. External. As
// _CFAttributedStringCheckAndReplace with CFAttributedStringReplaceAttributedString; a NULL replacement
// replaces the range with the constant empty string (CFAttributedStringReplaceString).
int _OCFAttributedStringCheckAndReplaceAttributed(
    OCFMutableAttributedStringRef aStr, OCFIndex location, OCFIndex length, OCFAttributedStringRef replacement
) {
    if (rangeExceedsString(aStr, location, length)) {
        return 3;
    }
    if ((infoOf(aStr) & kInfoImmutable) != 0) {
        return 1;
    }
    if (replacement != NULL) {
        OCFAttributedStringReplaceAttributedString(aStr, (OCFRange){location, length}, replacement);
    } else {
        OCFAttributedStringReplaceString(aStr, (OCFRange){location, length}, OF_STR(""));
    }
    return 0;
}

// Native _CFAttributedStringCheckAndSetAttributes, 0x1c014f780. External. As
// _CFAttributedStringCheckAndReplace with CFAttributedStringSetAttributes.
int _OCFAttributedStringCheckAndSetAttributes(
    OCFMutableAttributedStringRef aStr,
    OCFIndex location,
    OCFIndex length,
    OCFDictionaryRef attributes,
    Boolean clearOtherAttributes
) {
    if (rangeExceedsString(aStr, location, length)) {
        return 3;
    }
    if ((infoOf(aStr) & kInfoImmutable) != 0) {
        return 1;
    }
    OCFAttributedStringSetAttributes(aStr, (OCFRange){location, length}, attributes, clearOtherAttributes);
    return 0;
}

// Native _CFAttributedStringCheckAndSetAttribute, 0x1c014f80c. External. As
// _CFAttributedStringCheckAndReplace; a NULL name answers 2, a NULL value removes the attribute.
int _OCFAttributedStringCheckAndSetAttribute(
    OCFMutableAttributedStringRef aStr,
    OCFIndex location,
    OCFIndex length,
    OCFStringRef name,
    OCFTypeRef value
) {
    if (rangeExceedsString(aStr, location, length)) {
        return 3;
    }
    if ((infoOf(aStr) & kInfoImmutable) != 0) {
        return 1;
    }
    if (name == NULL) {
        return 2;
    }
    if (value != NULL) {
        OCFAttributedStringSetAttribute(aStr, (OCFRange){location, length}, name, value);
    } else {
        OCFAttributedStringRemoveAttribute(aStr, (OCFRange){location, length}, name);
    }
    return 0;
}

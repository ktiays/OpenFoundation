//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Collections/OCFRunArray.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

// Public CFAttributedString interface, iOS Simulator SDK CFAttributedString.h, spelled with the OCF prefix.
typedef const struct __OCFAttributedString *OCFAttributedStringRef;
typedef struct __OCFAttributedString *OCFMutableAttributedStringRef;

// The CFAttributedString object (type 62, 16 extra bytes), iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for every offset named here (the accesses of
// String/OCFAttributedString.c): the mutable string and the run array of attribute dictionaries. Info bit 0
// marks an immutable attributed string. The field names are local descriptive spellings.
struct __OCFAttributedString {
    OCFRuntimeBase base;
    OCFMutableStringRef string;
    OCFRunArrayRef attributeArray;
};

OF_EXPORT OCFMutableAttributedStringRef
OCFAttributedStringCreateMutable(OCFAllocatorRef alloc, OCFIndex maxLength);
OF_EXPORT void OCFAttributedStringReplaceString(
    OCFMutableAttributedStringRef aStr, OCFRange range, OCFStringRef replacement
);
OF_EXPORT OCFDictionaryRef
OCFAttributedStringGetAttributes(OCFAttributedStringRef aStr, OCFIndex loc, OCFRange *effectiveRange);
OF_EXPORT void OCFAttributedStringSetAttributes(
    OCFMutableAttributedStringRef aStr,
    OCFRange range,
    OCFDictionaryRef replacement,
    Boolean clearOtherAttributes
);
OF_EXPORT OCFAttributedStringRef
OCFAttributedStringCreateWithSubstring(OCFAllocatorRef alloc, OCFAttributedStringRef aStr, OCFRange range);
OF_EXPORT OCFIndex OCFAttributedStringGetLength(OCFAttributedStringRef aStr);
OF_EXPORT OCFAttributedStringRef
OCFAttributedStringCreateCopy(OCFAllocatorRef alloc, OCFAttributedStringRef aStr);
OF_EXPORT OCFMutableAttributedStringRef
OCFAttributedStringCreateMutableCopy(OCFAllocatorRef alloc, OCFIndex maxLength, OCFAttributedStringRef aStr);
OF_EXPORT OCFStringRef OCFAttributedStringGetString(OCFAttributedStringRef aStr);
OF_EXPORT OCFTypeRef OCFAttributedStringGetAttribute(
    OCFAttributedStringRef aStr, OCFIndex loc, OCFStringRef attrName, OCFRange *effectiveRange
);
OF_EXPORT OCFDictionaryRef OCFAttributedStringGetAttributesAndLongestEffectiveRange(
    OCFAttributedStringRef aStr, OCFIndex loc, OCFRange inRange, OCFRange *longestEffectiveRange
);
OF_EXPORT OCFTypeRef OCFAttributedStringGetAttributeAndLongestEffectiveRange(
    OCFAttributedStringRef aStr,
    OCFIndex loc,
    OCFStringRef attrName,
    OCFRange inRange,
    OCFRange *longestEffectiveRange
);
OF_EXPORT void OCFAttributedStringSetAttribute(
    OCFMutableAttributedStringRef aStr, OCFRange range, OCFStringRef attrName, OCFTypeRef value
);
OF_EXPORT void
OCFAttributedStringRemoveAttribute(OCFMutableAttributedStringRef aStr, OCFRange range, OCFStringRef attrName);
OF_EXPORT void OCFAttributedStringReplaceAttributedString(
    OCFMutableAttributedStringRef aStr, OCFRange range, OCFAttributedStringRef replacement
);
OF_EXPORT void OCFAttributedStringBeginEditing(OCFMutableAttributedStringRef aStr);
OF_EXPORT void OCFAttributedStringEndEditing(OCFMutableAttributedStringRef aStr);

// Exported natively (no SDK declaration). The Check entry points answer 3 for a range beyond the string, 1
// for an immutable attributed string, 2 for a NULL attribute name, and 0 after the change.
OF_EXPORT OCFMutableAttributedStringRef __OCFAttributedStringCreateMutableWithSubstring(
    OCFAllocatorRef alloc, OCFAttributedStringRef aStr, OCFRange range
);
OF_EXPORT OCFIndex _OCFAttributedStringGetLength(OCFAttributedStringRef aStr);
// Native __CFAttributedStringMtbl, 0x1c014f64c (external): true when the mutable bit is clear.
OF_EXPORT Boolean __OCFAttributedStringMtbl(OCFAttributedStringRef aStr);
OF_EXPORT int _OCFAttributedStringCheckAndReplace(
    OCFMutableAttributedStringRef aStr, OCFIndex location, OCFIndex length, OCFStringRef replacement
);
OF_EXPORT int _OCFAttributedStringCheckAndReplaceAttributed(
    OCFMutableAttributedStringRef aStr, OCFIndex location, OCFIndex length, OCFAttributedStringRef replacement
);
OF_EXPORT int _OCFAttributedStringCheckAndSetAttributes(
    OCFMutableAttributedStringRef aStr,
    OCFIndex location,
    OCFIndex length,
    OCFDictionaryRef attributes,
    Boolean clearOtherAttributes
);
OF_EXPORT int _OCFAttributedStringCheckAndSetAttribute(
    OCFMutableAttributedStringRef aStr,
    OCFIndex location,
    OCFIndex length,
    OCFStringRef name,
    OCFTypeRef value
);

// The class record (type 62), defined by String/OCFAttributedString.c for the builtin table.
OF_EXTERN const OCFRuntimeClass __CFAttributedStringClass;

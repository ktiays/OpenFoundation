//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"

#include <stdint.h>

// Public interface of CFCharacterSet.h (iOS Simulator SDK), spelled OCF.
typedef const struct __OCFCharacterSet *OCFCharacterSetRef;
typedef struct __OCFCharacterSet *OCFMutableCharacterSetRef;

typedef enum : OCFIndex {
    kOCFCharacterSetControl = 1,
    kOCFCharacterSetWhitespace = 2,
    kOCFCharacterSetWhitespaceAndNewline = 3,
    kOCFCharacterSetDecimalDigit = 4,
    kOCFCharacterSetLetter = 5,
    kOCFCharacterSetLowercaseLetter = 6,
    kOCFCharacterSetUppercaseLetter = 7,
    kOCFCharacterSetNonBase = 8,
    kOCFCharacterSetDecomposable = 9,
    kOCFCharacterSetAlphaNumeric = 10,
    kOCFCharacterSetPunctuation = 11,
    kOCFCharacterSetIllegal = 12,
    kOCFCharacterSetCapitalizedLetter = 13,
    kOCFCharacterSetSymbol = 14,
    kOCFCharacterSetNewline = 15,
} OCFCharacterSetPredefinedSet;

// The inline-buffer record CFCharacterSetInitInlineBuffer fills and
// _CFCharacterSetIsLongCharacterMemberForInline reads (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for the offsets 0, 8, 12, 16 and 24 and the
// 32-byte size, which CFCharacterSetInitInlineBuffer clears with two Q stores). Flags: bit 0 compact bitmap,
// bit 1 no bitmap, bit 2 inverted; bits 3 and 4 select the Swift implementations
// _CFCharacterSetIsLongCharacterMemberForInline calls (set by -_fillInlineBuffer:, outside this image).
// OCFCharacterSetInlineBuffer is the CFCharacterSetPriv.h name spelled OCF; field names are descriptive.
typedef struct {
    OCFCharacterSetRef cset;
    uint32_t flags;
    uint32_t rangeStart;
    uint32_t rangeLimit;
    const uint8_t *bitmap;
} OCFCharacterSetInlineBuffer;

// Original non-external name is preserved; no process-shared symbol is exported.
OF_EXTERN int chcompar(const void *first, const void *second);

// The class record (type 25 in the builtin class table). Hidden (was private external).
OF_EXTERN const OCFRuntimeClass __CFCharacterSetClass;
OF_EXPORT Boolean _OCFCharacterSetIsValidRange(OCFIndex location, OCFIndex length);

// Native __CFCharacterSetIsMutable, 0x1c01732b8. Hidden (was private external).
OF_EXTERN Boolean __CFCharacterSetIsMutable(OCFCharacterSetRef theSet);

OF_EXPORT OCFTypeID OCFCharacterSetGetTypeID(void);
OF_EXTERN void __CFCharacterSetInitialize(void);

OF_EXPORT OCFCharacterSetRef OCFCharacterSetGetPredefined(OCFCharacterSetPredefinedSet theSetIdentifier);
OF_EXPORT OCFCharacterSetRef
OCFCharacterSetCreateWithCharactersInString(OCFAllocatorRef alloc, OCFStringRef theString);
OF_EXPORT OCFCharacterSetRef
OCFCharacterSetCreateWithCharactersInRange(OCFAllocatorRef alloc, OCFRange range);
OF_EXPORT OCFMutableCharacterSetRef OCFCharacterSetCreateMutable(OCFAllocatorRef alloc);
OF_EXPORT OCFCharacterSetRef OCFCharacterSetCreateCopy(OCFAllocatorRef alloc, OCFCharacterSetRef theSet);
OF_EXPORT OCFMutableCharacterSetRef
OCFCharacterSetCreateMutableCopy(OCFAllocatorRef alloc, OCFCharacterSetRef theSet);
OF_EXPORT Boolean OCFCharacterSetIsCharacterMember(OCFCharacterSetRef theSet, UniChar theChar);
OF_EXPORT Boolean OCFCharacterSetIsLongCharacterMember(OCFCharacterSetRef theSet, UTF32Char theChar);
OF_EXPORT OCFDataRef
OCFCharacterSetCreateBitmapRepresentation(OCFAllocatorRef alloc, OCFCharacterSetRef theSet);
OF_EXPORT void OCFCharacterSetAddCharactersInRange(OCFMutableCharacterSetRef theSet, OCFRange theRange);
OF_EXPORT void OCFCharacterSetRemoveCharactersInRange(OCFMutableCharacterSetRef theSet, OCFRange theRange);
OF_EXPORT void OCFCharacterSetAddCharactersInString(OCFMutableCharacterSetRef theSet, OCFStringRef theString);
OF_EXPORT void
OCFCharacterSetRemoveCharactersInString(OCFMutableCharacterSetRef theSet, OCFStringRef theString);
OF_EXPORT void OCFCharacterSetUnion(OCFMutableCharacterSetRef theSet, OCFCharacterSetRef theOtherSet);
OF_EXPORT void OCFCharacterSetIntersect(OCFMutableCharacterSetRef theSet, OCFCharacterSetRef theOtherSet);
OF_EXPORT void OCFCharacterSetInvert(OCFMutableCharacterSetRef theSet);
OF_EXPORT OCFCharacterSetRef
OCFCharacterSetCreateWithBitmapRepresentation(OCFAllocatorRef alloc, OCFDataRef data);
OF_EXPORT OCFCharacterSetRef
OCFCharacterSetCreateInvertedSet(OCFAllocatorRef alloc, OCFCharacterSetRef theSet);
OF_EXPORT Boolean OCFCharacterSetIsSupersetOfSet(OCFCharacterSetRef theSet, OCFCharacterSetRef theOtherset);
OF_EXPORT Boolean OCFCharacterSetHasMemberInPlane(OCFCharacterSetRef theSet, OCFIndex thePlane);

// CFCharacterSetPriv.h interfaces (exported natively).
OF_EXPORT void OCFCharacterSetInitInlineBuffer(OCFCharacterSetRef cset, OCFCharacterSetInlineBuffer *buffer);
OF_EXPORT Boolean
_OCFCharacterSetIsLongCharacterMemberForInline(OCFCharacterSetInlineBuffer *buffer, UTF32Char character);

// Exported natively (no SDK declaration): the keyed-coding accessors and the inverted test.
OF_EXPORT Boolean _OCFCharacterSetIsInverted(OCFCharacterSetRef theSet);
OF_EXPORT void _OCFCharacterSetSetIsInverted(OCFMutableCharacterSetRef set, Boolean flag);
OF_EXPORT void _OCFCharacterSetCompact(OCFMutableCharacterSetRef theSet);
OF_EXPORT void _OCFCharacterSetFast(OCFMutableCharacterSetRef theSet);
OF_EXPORT OCFIndex _OCFCharacterSetGetKeyedCodingType(OCFCharacterSetRef theSet);
OF_EXPORT OCFIndex _OCFCharacterSetGetKeyedCodingBuiltinType(OCFCharacterSetRef theSet);
OF_EXPORT OCFRange _OCFCharacterSetGetKeyedCodingRange(OCFCharacterSetRef theSet);
OF_EXPORT OCFStringRef _OCFCharacterSetCreateKeyedCodingString(OCFCharacterSetRef theSet);

// Native _CFCreateCharacterSetFromUSet, 0x1c017b1a8 (external), in this unit's range (called by the string
// formatting units with an ICU USet); answers an immutable set despite the declared type.
OF_EXPORT OCFMutableCharacterSetRef _OCFCreateCharacterSetFromUSet(void *set);

// Native _CFCharacterSetIsMutable, 0x1c017ad5c. External.
OF_EXPORT Boolean _OCFCharacterSetIsMutable(OCFCharacterSetRef cset);

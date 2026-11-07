//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFData.h>

CF_EXTERN_C_BEGIN

typedef const struct CF_BRIDGED_TYPE(NSCharacterSet) __CFCharacterSet *CFCharacterSetRef;
typedef struct CF_BRIDGED_MUTABLE_TYPE(NSMutableCharacterSet) __CFCharacterSet *CFMutableCharacterSetRef;

typedef CF_ENUM(CFIndex, CFCharacterSetPredefinedSet) {
    kCFCharacterSetControl = 1,
    kCFCharacterSetWhitespace,
    kCFCharacterSetWhitespaceAndNewline,
    kCFCharacterSetDecimalDigit,
    kCFCharacterSetLetter,
    kCFCharacterSetLowercaseLetter,
    kCFCharacterSetUppercaseLetter,
    kCFCharacterSetNonBase,
    kCFCharacterSetDecomposable,
    kCFCharacterSetAlphaNumeric,
    kCFCharacterSetPunctuation,
    kCFCharacterSetCapitalizedLetter = 13,
    kCFCharacterSetSymbol = 14,
    kCFCharacterSetNewline = 15,
    kCFCharacterSetIllegal = 12
};

CF_EXPORT CFTypeID CFCharacterSetGetTypeID(void);
CF_EXPORT CFCharacterSetRef CFCharacterSetGetPredefined(CFCharacterSetPredefinedSet theSetIdentifier);
CF_EXPORT CFCharacterSetRef CFCharacterSetCreateWithCharactersInRange(CFAllocatorRef alloc, CFRange theRange);
CF_EXPORT CFCharacterSetRef CFCharacterSetCreateWithCharactersInString(CFAllocatorRef alloc, CFStringRef theString);
CF_EXPORT CFCharacterSetRef CFCharacterSetCreateWithBitmapRepresentation(CFAllocatorRef alloc, CFDataRef theData);
CF_EXPORT CFCharacterSetRef CFCharacterSetCreateInvertedSet(CFAllocatorRef alloc, CFCharacterSetRef theSet);
CF_EXPORT Boolean CFCharacterSetIsSupersetOfSet(CFCharacterSetRef theSet, CFCharacterSetRef theOtherset);
CF_EXPORT Boolean CFCharacterSetHasMemberInPlane(CFCharacterSetRef theSet, CFIndex thePlane);
CF_EXPORT CFMutableCharacterSetRef CFCharacterSetCreateMutable(CFAllocatorRef alloc);
CF_EXPORT CFCharacterSetRef CFCharacterSetCreateCopy(CFAllocatorRef alloc, CFCharacterSetRef theSet);
CF_EXPORT CFMutableCharacterSetRef CFCharacterSetCreateMutableCopy(CFAllocatorRef alloc, CFCharacterSetRef theSet);
CF_EXPORT Boolean CFCharacterSetIsCharacterMember(CFCharacterSetRef theSet, UniChar theChar);
CF_EXPORT Boolean CFCharacterSetIsLongCharacterMember(CFCharacterSetRef theSet, UTF32Char theChar);
CF_EXPORT CFDataRef CFCharacterSetCreateBitmapRepresentation(CFAllocatorRef alloc, CFCharacterSetRef theSet);
CF_EXPORT void CFCharacterSetAddCharactersInRange(CFMutableCharacterSetRef theSet, CFRange theRange);
CF_EXPORT void CFCharacterSetRemoveCharactersInRange(CFMutableCharacterSetRef theSet, CFRange theRange);
CF_EXPORT void CFCharacterSetAddCharactersInString(CFMutableCharacterSetRef theSet, CFStringRef theString);
CF_EXPORT void CFCharacterSetRemoveCharactersInString(CFMutableCharacterSetRef theSet, CFStringRef theString);
CF_EXPORT void CFCharacterSetUnion(CFMutableCharacterSetRef theSet, CFCharacterSetRef theOtherSet);
CF_EXPORT void CFCharacterSetIntersect(CFMutableCharacterSetRef theSet, CFCharacterSetRef theOtherSet);
CF_EXPORT void CFCharacterSetInvert(CFMutableCharacterSetRef theSet);

CF_EXTERN_C_END

//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Locale/OCFLocaleKeys.h"

// CFLocale.h (iOS Simulator SDK interface), spelled OCF. The CFLocale functions restored on this branch are
// defined in Locale/OCFLocale.c (the NSLocale-backed entry points and the reset and preferences paths) and in
// Locale/OCFLocaleIdentifier.c (CFLocaleCreateCanonicalLanguageIdentifierFromString); the other interfaces
// stay undefined until their units are restored. This header cannot include String/OCFString.h, which
// includes it.
typedef const struct __OCFLocale *OCFLocaleRef;
typedef OCFStringRef OCFLocaleIdentifier;
typedef OCFStringRef OCFLocaleKey;

// CFLocaleLanguageDirection (SDK CFLocale.h values).
typedef enum : OCFIndex {
    kOCFLocaleLanguageDirectionUnknown = 0,
    kOCFLocaleLanguageDirectionLeftToRight = 1,
    kOCFLocaleLanguageDirectionRightToLeft = 2,
    kOCFLocaleLanguageDirectionTopToBottom = 3,
    kOCFLocaleLanguageDirectionBottomToTop = 4,
} OCFLocaleLanguageDirection;

OF_EXPORT OCFTypeID OCFLocaleGetTypeID(void);
OF_EXPORT OCFLocaleRef OCFLocaleGetSystem(void);
OF_EXPORT OCFLocaleRef OCFLocaleCopyCurrent(void);
OF_EXPORT OCFLocaleRef OCFLocaleCreate(OCFAllocatorRef allocator, OCFLocaleIdentifier localeIdentifier);
OF_EXPORT OCFLocaleIdentifier OCFLocaleGetIdentifier(OCFLocaleRef locale);
OF_EXPORT OCFTypeRef OCFLocaleGetValue(OCFLocaleRef locale, OCFLocaleKey key);
OF_EXPORT OCFStringRef
OCFLocaleCopyDisplayNameForPropertyValue(OCFLocaleRef displayLocale, OCFStringRef key, OCFStringRef value);
OF_EXPORT OCFLocaleLanguageDirection OCFLocaleGetLanguageCharacterDirection(OCFStringRef isoLangCode);
OF_EXPORT OCFLocaleIdentifier OCFLocaleCreateCanonicalLanguageIdentifierFromString(
    OCFAllocatorRef allocator, OCFStringRef localeIdentifier
);

// Native _CFLocaleResetCurrent (0x1c01dbff0), external.
OF_EXPORT void _OCFLocaleResetCurrent(void);

// Native __kCFLocaleCollatorID (0x2227a3b08, __DATA_CONST,__const; defined in Locale/OCFLocale.c) and
// __CFLocaleGetDoesNotRequireSpecialCaseHandling (0x1c01dbef8), both were private externals.
OF_EXTERN const OCFStringRef __kCFLocaleCollatorID;
OF_EXTERN Boolean __CFLocaleGetDoesNotRequireSpecialCaseHandling(OCFLocaleRef locale);

// Native private externals of the CFLocale unit (hidden, native names). The preference getters answer the
// object -[NSLocale _prefForKey:] returns (their callers test it for a dictionary); __CFLocaleResetCurrent
// tests its argument as a 32-bit value; __CFLocalePrefsChanged reads no argument (its callers are outside the
// corpus; confidence: medium for that prototype).
OF_EXTERN OCFTypeRef _CFLocaleGetAppleICUNumberFormatStringsPref(OCFLocaleRef locale);
OF_EXTERN OCFTypeRef _CFLocaleGetAppleICUNumberSymbolsPref(OCFLocaleRef locale);
OF_EXTERN void __CFLocaleResetCurrent(Boolean resetLocale);
OF_EXTERN void __CFLocalePrefsChanged(void);

// _CFLocaleCopyLocaleIdentifierByAddingLikelySubtags (0x1c01dbf0c, was a private external) is defined in
// Locale/OCFLocale.c; PlugIn/OCFBundleLocalization.h declares it, and Locale/OCFLocale.c includes that header
// for the declaration.

// The Foundation class the CFLocale entry points message (NSLocale); declared for the naming switch.
#ifdef __OBJC__
@class ONSLocale;
#endif

// Native kCFLocaleCurrentLocaleDidChangeNotification, 0x2227a3b90. External.
OF_EXPORT const OCFStringRef kOCFLocaleCurrentLocaleDidChangeNotification;

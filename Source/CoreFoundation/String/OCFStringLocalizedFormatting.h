//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/String/OCFString.h"

#include <stdbool.h>

// Locale-aware formatting helpers of the iOS Simulator CoreFoundation image (24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC): the formatting-family character sets, locale-summary lookup,
// conditional formatting and the vocative case of given names.

// Native __CFStringLocalizedFormattingInfo (was a private external): the constant dictionary with the
// formatting-family patterns (NSStringFormatFamilyInfoKey) and the built-in entity rules
// (NSStringFormatRuleInfoKey: TechnicalTerm, VocativeName).
OF_EXTERN const OCFDictionaryRef _CFStringLocalizedFormattingInfo;

// Hidden (were private externals).
OF_EXTERN OCFMutableDictionaryRef
_CFStringGetInputIdentifierFormatterMappingFromDescriptor(OCFDictionaryRef descriptor);

OF_EXTERN const void *
_CFStringGetRelevantLocaleInfoFromLocaleSummary(OCFDictionaryRef localeSummary, OCFLocaleRef locale);

OF_EXTERN bool _CFStringContentsInCharacterSet(OCFStringRef string, OCFCharacterSetRef characterSet);

OF_EXTERN bool _CFStringConditionalFormattingMeetsCondition(OCFStringRef string, OCFLocaleRef locale);

// External.
OF_EXPORT OCFStringRef _OCFStringCopyVocativeCaseOfGivenName(OCFStringRef givenName, OCFLocaleRef locale);

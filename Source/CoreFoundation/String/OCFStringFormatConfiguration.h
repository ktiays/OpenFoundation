//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/PlugIn/OCFBundleMain.h"

// Original hidden names are preserved (natively private external); no process-shared symbol is exported.
OF_EXTERN OCFStringRef
_CFStringCopyWithFomatStringConfiguration(OCFStringRef string, const void *configuration);
OF_EXTERN OCFStringRef _CFStringCreateWithWidthContexts(OCFDictionaryRef widthContexts);
OF_EXTERN OCFStringRef _CFStringCreateWithMarkdownAndConfiguration(
    OCFStringRef markdown, const void *configuration, OCFBundleRef bundle, OCFStringRef localization
);

#include <stdarg.h>
#include <stdbool.h>

// Format-string configuration of the iOS Simulator CoreFoundation image (24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC): the stringsdict-style keys and rule types, the configuration of a
// format string, resolution of a configured format string and the inflection hand-off.

// Exported keys and rule types (__DATA_CONST,__const pointers to the image's literals). Only the six that the
// restored functions reach are defined; the unit's other keys (0x2227aa1a0-0x2227aa2a8) are not restored.
OF_EXPORT const OCFStringRef kOCFStringFormatSpecTypeKey;
OF_EXPORT const OCFStringRef kOCFStringPluralRuleType;
OF_EXPORT const OCFStringRef kOCFStringFormatFamilyInfoKey;
OF_EXPORT const OCFStringRef kOCFStringFormatDefaultValueKey;
OF_EXPORT const OCFStringRef kOCFStringFormatRuleInfoKey;
OF_EXPORT const OCFStringRef kOCFStringFormatValueTypeKey;

// External: the configuration a format string carries (-formatConfiguration), or NULL.
OF_EXPORT OCFDictionaryRef _OCFStringGetFormatSpecifierConfiguration(OCFStringRef format);

// Hidden (were private externals).
OF_EXTERN OCFStringRef _CFStringFindAffixRuleFormat(OCFStringRef languageCode, OCFStringRef string);

OF_EXTERN OCFTypeRef _CFCopyResolvedFormatStringWithConfiguration(
    OCFTypeRef argument, OCFDictionaryRef configuration, OCFTypeRef locale
);

OF_EXTERN bool _CFStringObjCFormatRequiresInflection(OCFStringRef format);

OF_EXTERN OCFStringRef _CFStringCreateFormatWithInflectionAndArguments(
    OCFAllocatorRef alloc, OCFDictionaryRef formatOptions, OCFStringRef format, va_list arguments
);

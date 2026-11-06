//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Base/OCFFileUtilities.h"
#include "CoreFoundation/PlugIn/OCFBundle.h"
#include "CoreFoundation/PlugIn/OCFBundleInfoPlist.h"
#include "CoreFoundation/Preferences/OCFPreferences.h"
#include "CoreFoundation/PropertyList/OCFPropertyList.h"
#include "CoreFoundation/String/OCFStringFormat.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/Locale/OCFLocale.h"

#include <stdint.h>

// The bundle-localization unit, CFBundle_Locale.c (the source path its functions log), iOS Simulator
// 0x1c01f3b1c-0x1c01f7130, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC. Prototypes follow the native register
// use; parameter names are local descriptive spellings.

// Public interfaces (SDK CFBundle.h, spelled OCF).
OF_EXPORT OCFArrayRef OCFBundleCopyBundleLocalizations(OCFBundleRef bundle);
OF_EXPORT OCFArrayRef OCFBundleCopyLocalizationsForPreferences(OCFArrayRef locArray, OCFArrayRef prefArray);
OF_EXPORT OCFArrayRef OCFBundleCopyPreferredLocalizationsFromArray(OCFArrayRef locArray);

// Hidden interfaces (were private externals; original names are preserved).
OF_EXTERN Boolean CFBundleAllowMixedLocalizations(void);
OF_EXTERN OCFArrayRef _CFBundleCopyLanguageSearchListInBundle(OCFBundleRef bundle);
OF_EXTERN OCFArrayRef _CFBundleCopyUserLanguages(void);
OF_EXTERN OCFMutableArrayRef _CFBundleCopyLanguageSearchListInDirectory(OCFURLRef url, uint8_t *version);

// Interfaces of families not yet restored on this branch (SDK or native signatures; their owners' headers
// replace these declarations).

OF_EXTERN OCFStringRef _CFLocaleCopyLocaleIdentifierByAddingLikelySubtags(OCFStringRef localeIdentifier);

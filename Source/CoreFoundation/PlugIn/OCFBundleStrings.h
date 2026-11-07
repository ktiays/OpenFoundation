//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/PlugIn/OCFBundle.h"
#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/String/OCFStringPseudoLanguage.h"

#include <stdint.h>

// The bundle-strings unit (CFBundle_Strings.m, the source path its functions log and assert with), iOS
// Simulator 0x1c02bf0c4-0x1c02c3d7c, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC. Prototypes follow the native
// register use; parameter and field names are local descriptive spellings.

// The strings sources of one table: 72 bytes, passed by value (a pointer to a copy) and answered through X8.
// Offsets are those of the native loads and stores; StringsSources is a local name.
typedef struct {
    OCFURLRef stringsURL;             // +0
    OCFStringRef stringsLanguage;     // +8
    OCFDataRef stringsData;           // +16
    OCFURLRef stringsDictURL;         // +24
    OCFStringRef stringsDictLanguage; // +32
    OCFDataRef stringsDictData;       // +40
    OCFURLRef locTableURL;            // +48
    OCFDataRef locTableData;          // +56
    uint8_t stringsMapped;            // +64
    uint8_t stringsDictMapped;        // +65
    uint8_t locTableMapped;           // +66
    uint8_t loadStringsDictFirst; // +67: 1 unless the strings language is preferred to the stringsdict one.
    uint8_t locTableUnused;       // +68: no loctable variant was chosen; the loctable data is dropped.
    uint8_t _padding69[3];        // +69
} StringsSources;

OF_ABI_ASSERT(sizeof(StringsSources) == 72, "The strings sources are 72 bytes.");

// Public interfaces (SDK CFBundle.h, spelled OCF).
OF_EXPORT OCFDataRef _OCFBundleGetMappedStringsFile(OCFIndex index);
OF_EXPORT OCFDictionaryRef OCFBundleCopyLocalizedStringTableForLocalization(
    OCFBundleRef bundle, OCFStringRef tableName, OCFStringRef localizationName
);
OF_EXPORT OCFStringRef OCFBundleCopyLocalizedString(
    OCFBundleRef bundle, OCFStringRef key, OCFStringRef value, OCFStringRef tableName
);

// Natively exported private interfaces. localizations is an array of localization names; outLocalization
// receives the localization taken from the strings file URL (retained) when not NULL.
OF_EXPORT OCFStringRef _OCFBundleCopyLocalizedStringForLocalizationTableURLAndMarkdownOption(
    OCFBundleRef bundle,
    OCFStringRef key,
    OCFStringRef value,
    OCFStringRef tableName,
    OCFArrayRef localizations,
    Boolean preventMarkdownParsing,
    OCFTypeRef *outLocalization
);
OF_EXPORT OCFStringRef _OCFBundleCopyLocalizedStringForLocalizationAndTableURL(
    OCFBundleRef bundle,
    OCFStringRef key,
    OCFStringRef value,
    OCFStringRef tableName,
    OCFStringRef localizationName,
    OCFTypeRef *outLocalization
);

// Was a private external (hidden; native name). The second argument is tested as a 32-bit flag.
OF_EXTERN StringsSources _CFBundleGetStringsSources(
    OCFBundleRef bundle, Boolean allowMapping, OCFStringRef tableName, OCFArrayRef localizations
);

// Foundation's __NSCreateBPlistMappedDataFromURL (imported from Foundation natively): X0 the URL, X1 a block
// answering the index of a mapped data, X2 the address of an error it may store. Default visibility; the
// Foundation product supplies it.
extern OCFDataRef __ONSCreateBPlistMappedDataFromURL(
    OCFURLRef url, OCFIndex (^registerMappedData)(OCFDataRef data), OCFErrorRef *error
);

// The Objective-C class this unit allocates (NSSet, an Objective-C class of this image).
#ifdef __OBJC__
@class ONSSet;
#endif

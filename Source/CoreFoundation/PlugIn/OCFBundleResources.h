//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/PlugIn/OCFBundle.h"

#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Base/OCFFileUtilities.h"
#include "CoreFoundation/URL/OCFURL.h"
#include "CoreFoundation/String/OCFStringFormat.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/PlugIn/OCFBundleLocalization.h"

// The bundle-resource unit (CFBundle_Resources.c, iOS Simulator 0x1c016f9b8-0x1c0173240), iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC. Parameter widths follow the native register use;
// parameter names are local descriptive spellings.

// The resource predicate that _CFBundleFindResourcesWithPredicate invokes with a query-table key and the stop
// flag (invoke at block + 16; the result is tested as a 32-bit register). ResourcePredicate is a local name.
typedef Boolean (^ResourcePredicate)(OCFStringRef fileName, Boolean *stop);

OF_EXPORT OCFURLRef OCFBundleCopyResourceURL(
    OCFBundleRef bundle, OCFStringRef resourceName, OCFStringRef resourceType, OCFStringRef subDirName
);

OF_EXPORT OCFArrayRef
OCFBundleCopyResourceURLsOfType(OCFBundleRef bundle, OCFStringRef resourceType, OCFStringRef subDirName);
OF_EXPORT OCFURLRef OCFBundleCopyResourceURLForLocalization(
    OCFBundleRef bundle,
    OCFStringRef resourceName,
    OCFStringRef resourceType,
    OCFStringRef subDirName,
    OCFStringRef localizationName
);

// The third argument (X2) is never read by the native function; both native callers leave X2 unset.
OF_EXPORT OCFTypeRef _OCFBundleCopyFindResources(
    OCFBundleRef bundle,
    OCFURLRef bundleURL,
    OCFArrayRef unused,
    OCFStringRef resourceName,
    OCFStringRef resourceType,
    OCFStringRef subPath,
    OCFStringRef localization,
    Boolean returnArray,
    Boolean localized,
    ResourcePredicate predicate
);

OF_EXTERN uint8_t _CFBundleGetBundleVersionForURL(OCFURLRef url);

OF_EXTERN OCFStringRef _CFBundleGetPlatformExecutablesSubdirectoryName(void);

// Was a private external (hidden; native name).
OF_EXTERN Boolean _CFIsResourceAtURL(OCFURLRef url, Boolean *isDirectory);

// Interfaces of families not yet restored on this branch (path helpers of the file-utilities unit; were
// private externals, hidden with native names). Their results, if any, are not read by the restored callers.

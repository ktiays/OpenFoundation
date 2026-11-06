//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/URL/OCFURL.h"

// The CFURL bookmark entry points that forward to CoreServicesInternal (SDK CFURL.h signatures, OCF
// spellings), defined in URL/OCFURLCoreServicesForwarding.c. External.
OF_EXPORT OCFDataRef OCFURLCreateBookmarkData(
    OCFAllocatorRef allocator,
    OCFURLRef url,
    OCFOptionFlags options,
    OCFArrayRef resourcePropertiesToInclude,
    OCFURLRef relativeToURL,
    OCFErrorRef *error
);
OF_EXPORT OCFDictionaryRef OCFURLCreateResourcePropertiesForKeysFromBookmarkData(
    OCFAllocatorRef allocator, OCFArrayRef resourcePropertiesToReturn, OCFDataRef bookmark
);
OF_EXPORT OCFDataRef
OCFURLCreateBookmarkDataFromFile(OCFAllocatorRef allocator, OCFURLRef fileURL, OCFErrorRef *errorRef);
OF_EXPORT Boolean OCFURLWriteBookmarkDataToFile(
    OCFDataRef bookmarkRef, OCFURLRef fileURL, OCFOptionFlags options, OCFErrorRef *errorRef
);

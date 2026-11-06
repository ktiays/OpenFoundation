//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/URL/OCFURL.h"

// Original private-external name is preserved (hidden; no process-shared symbol is exported).
OF_EXTERN Boolean _CFURLExists(OCFURLRef url);

OF_EXPORT Boolean OCFURLWriteDataAndPropertiesToResource(
    OCFURLRef url, OCFDataRef data, OCFDictionaryRef properties, SInt32 *errorCode
);

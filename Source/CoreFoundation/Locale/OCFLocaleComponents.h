//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Locale/OCFLocale.h"

// CFLocale.h (iOS Simulator SDK interface), spelled OCF: the locale-identifier component functions restored
// in Locale/OCFLocaleComponents.c.
OF_EXPORT OCFDictionaryRef
OCFLocaleCreateComponentsFromLocaleIdentifier(OCFAllocatorRef allocator, OCFLocaleIdentifier localeID);
OF_EXPORT OCFLocaleIdentifier
OCFLocaleCreateLocaleIdentifierFromComponents(OCFAllocatorRef allocator, OCFDictionaryRef dictionary);

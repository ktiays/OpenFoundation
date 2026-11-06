//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/String/OCFString.h"

OF_EXPORT OCFCharacterSetRef _OCFURLComponentsGetURLPathAllowedCharacterSet(void);
OF_EXPORT OCFStringRef _OCFStringCreateByAddingPercentEncodingWithAllowedCharacters(
    OCFAllocatorRef allocator, OCFStringRef string, OCFCharacterSetRef allowedCharacters
);

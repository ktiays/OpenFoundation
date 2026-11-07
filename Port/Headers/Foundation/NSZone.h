//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <Foundation/NSObjCRuntime.h>

#if defined(__OBJC__)
@class NSString;

typedef struct _NSZone NSZone;

FOUNDATION_EXPORT void *NSZoneMalloc(NSZone *zone, NSUInteger size);
#endif

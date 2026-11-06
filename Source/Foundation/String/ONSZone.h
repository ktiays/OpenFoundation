//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSString.h"

// Native NSZoneMalloc, 0x1c0593e6c, external (SDK NSZone.h) (String/ONSZone.m).
OF_EXPORT void *ONSZoneMalloc(struct _NSZone *zone, ONSUInteger size);

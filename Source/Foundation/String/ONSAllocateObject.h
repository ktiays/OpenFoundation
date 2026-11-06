//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSString.h"

// Native NSAllocateObject, 0x1c0582f54, external (SDK NSObject.h) (String/ONSAllocateObject.m). The zone is
// not read.
OF_EXPORT id ONSAllocateObject(Class aClass, ONSUInteger extraBytes, struct _NSZone *zone);

// Native NSDeallocateObject, 0x1c0582ff8, external (SDK NSObject.h) (String/ONSAllocateObject.m).
OF_EXPORT void ONSDeallocateObject(id object);

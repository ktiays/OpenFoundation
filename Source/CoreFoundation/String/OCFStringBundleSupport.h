//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/String/OCFString.h"

// Non-external natively (statics of the bundle-strings unit); hidden here with their native names and static
// when merged with that unit.
OF_EXTERN OCFMutableStringRef
_CFStringCreateAppend3(OCFStringRef first, OCFStringRef second, OCFStringRef third);
OF_EXTERN OCFMutableStringRef _CFStringCreateAppend2(OCFStringRef first, OCFStringRef second);
OF_EXTERN void __CFStringsDictAddFunction(const void *key, const void *value, void *context);

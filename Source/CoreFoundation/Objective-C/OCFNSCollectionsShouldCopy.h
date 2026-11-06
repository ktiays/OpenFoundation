//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/objc.h>

// Native __NSCollectionsShouldCopy, 0x1c0287374 (was a private external;
// Objective-C/OCFNSCollectionsShouldCopy.m): whether the collections' -copyWithZone: copies their items.
OF_EXTERN BOOL __NSCollectionsShouldCopy(void);

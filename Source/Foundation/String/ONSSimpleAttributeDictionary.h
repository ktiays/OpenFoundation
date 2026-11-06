//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSDictionary.h"
#include "CoreFoundation/Objective-C/OCFNSEnumerator.h"

#include <stdint.h>

// Native _NSSimpleAttributeDictionaryElement, recovered from the elements ivar encoding.
typedef struct _NSSimpleAttributeDictionaryElement {
    NSUInteger hash;
    id key;
    id value;
} NSSimpleAttributeDictionaryElement;

// Native NSSimpleAttributeDictionary, 0x22e6d6368, non-external. class_ro_t 0x222848570 gives
// instance start 8 and size 40. The superclass is NSDictionary (shared-cache pointer 0x22e6cde50).
__attribute__((visibility("hidden")))
@interface ONSSimpleAttributeDictionary : ONSDictionary {
    @package
    uint32_t numElements;
    uint32_t refCount;
    NSSimpleAttributeDictionaryElement elements[1];
}
+ (id)emptyAttributeDictionary;
+ (id)newWithDictionary:(id)dictionary;
- (id)newWithKey:(id)key object:(id)object;
- (NSUInteger)slotForKey:(id)key;
@end

// Native NSSimpleAttributeDictionaryEnumerator, 0x22e6d6390, non-external. class_ro_t 0x222848648
// gives instance start 8 and size 24. The superclass is NSEnumerator (shared-cache pointer 0x22e6cdec8).
__attribute__((visibility("hidden")))
@interface ONSSimpleAttributeDictionaryEnumerator : ONSEnumerator {
    @private
    ONSSimpleAttributeDictionary *dictionary;
    NSUInteger nextElement;
}
- (id)initWithAttributeDictionary:(ONSSimpleAttributeDictionary *)attributes;
@end

// Native __NSAttributeDictionaryClass, 0x1c04c4b50, non-external. The hidden name crosses the two
// source units that recover this part of the attributed-string cluster.
OF_EXTERN Class _NSAttributeDictionaryClass(void);

@class ONSAttributeDictionary;

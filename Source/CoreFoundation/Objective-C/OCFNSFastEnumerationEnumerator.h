//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Objective-C/OCFNSEnumerator.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

// The fast enumeration state (SDK NSFastEnumerationState, 64 bytes: the state, the items pointer, the
// mutations pointer and five extra words). FastEnumerationState and its field names are local descriptive
// spellings of the SDK's.
typedef struct {
    unsigned long state;
    id *itemsPtr;
    unsigned long *mutationsPtr;
    unsigned long extra[5];
} FastEnumerationState;

OF_ABI_ASSERT(sizeof(FastEnumerationState) == 64, "The fast enumeration state is 64 bytes.");

// Native class __NSFastEnumerationEnumerator (__DATA,__objc_data 0x22e6cdef0, external), an NSEnumerator
// subclass; class_ro_t 0x2227c3eb8: instance start 8, size 240; ivars _obj (offset 8), _origObj (16), _index
// (24), _count (32), _mut (40), _objects (48, 128 bytes) and _state (176, 64 bytes), all with eight-byte
// alignment, their offset symbols _OBJC_IVAR_$___NSFastEnumerationEnumerator._obj ... (were private
// externals) (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The
// ivar type encodings lie outside the extracted image (unread); the types follow their sizes and uses.
// Exported, as the native image exports the class.
__attribute__((visibility("default")))
@interface __ONSFastEnumerationEnumerator : ONSEnumerator {
    id _obj;
    id _origObj;
    unsigned long _index;
    unsigned long _count;
    unsigned long _mut;
    id _objects[16];
    FastEnumerationState _state;
}

- (id)initWithObject:(id)object;

@end
#endif

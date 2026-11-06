//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "CoreFoundation/Objective-C/OCFNSData.h"
#include "CoreFoundation/PropertyList/OCFBinaryPList.h"
#include <OpenFoundation/Base/ONSObjCRuntime.h>

#ifdef __OBJC__
// Native _NSBPlistMappedData, 0x22e6d80c8. Was a private external. class_ro_t 0x22284ed78:
// NSData superclass, instance start 8 and size 73.
@interface _ONSBPlistMappedData : ONSData {
    @private
    void *ptr;
    ONSUInteger size;
    ONSUInteger mappingIndex;
    BinaryPlistTrailer bplistTrailer;
    ONSUInteger bplistOffset;
    uint8_t bplistMarker;
}

- (id)initWithFileURL:(id)url error:(id *)error;
- (void)setMappingIndex:(ONSUInteger)index;
- (ONSUInteger)mappingIndex;
- (const void *)bytes;
- (ONSUInteger)length;
- (ONSUInteger)_bplistObjectsRangeEnd;

@end
#endif

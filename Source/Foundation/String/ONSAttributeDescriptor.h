//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <objc/NSObject.h>

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified unless noted. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.

@class ONSString;

// Native _NSAttributeDescriptor, 0x22e6dbc00, was private external. class_ro_t 0x222859fb8
// gives instance start 8, size 16. Superclass 0x22e6b6088 is NSObject (confidence: medium).
__attribute__((visibility("hidden")))
@interface _ONSAttributeDescriptor : NSObject {
    @private
    ONSString *_name;
}
- (ONSString *)name;
- (id)attributeValueFromValue:(id)value;
@end

// Native _NSURLAttributeDescriptor (0x22e6dbb88), _NSInflectionAttributeDescriptor (0x22e6dbbb0) and
// _NSMorphologyAttributeDescriptor (0x22e6dbbd8), non-external: subclasses with instance start and size 16
// and no ivars, used only as the static descriptors of the attribute table (never retained, released or
// deallocated).
__attribute__((visibility("hidden")))
@interface _ONSURLAttributeDescriptor : _ONSAttributeDescriptor
@end

__attribute__((visibility("hidden")))
@interface _ONSInflectionAttributeDescriptor : _ONSAttributeDescriptor
@end

__attribute__((visibility("hidden")))
@interface _ONSMorphologyAttributeDescriptor : _ONSAttributeDescriptor
@end

@interface _ONSAttributeDescriptor (NSAttributedDescriptorForAttributeKey)
+ (_ONSAttributeDescriptor *)descriptorForAttributeKey:(ONSString *)key;
@end

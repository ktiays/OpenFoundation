//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>

#if defined(__OBJC__)
@class NSData, NSString, NSError, NSInputStream, NSOutputStream;

typedef NS_OPTIONS(NSUInteger, NSPropertyListMutabilityOptions) {
    NSPropertyListImmutable = kCFPropertyListImmutable,
    NSPropertyListMutableContainers = kCFPropertyListMutableContainers,
    NSPropertyListMutableContainersAndLeaves = kCFPropertyListMutableContainersAndLeaves
};

typedef NS_ENUM(NSUInteger, NSPropertyListFormat) {
    NSPropertyListOpenStepFormat = kCFPropertyListOpenStepFormat,
    NSPropertyListXMLFormat_v1_0 = kCFPropertyListXMLFormat_v1_0,
    NSPropertyListBinaryFormat_v1_0 = kCFPropertyListBinaryFormat_v1_0
};

typedef NSPropertyListMutabilityOptions NSPropertyListReadOptions;
typedef NSUInteger NSPropertyListWriteOptions;

@interface NSPropertyListSerialization : NSObject

+ (NSData *)dataWithPropertyList:(id)plist
                          format:(NSPropertyListFormat)format
                         options:(NSPropertyListWriteOptions)opt
                           error:(NSError **)error;
+ (id)propertyListWithData:(NSData *)data
                   options:(NSPropertyListReadOptions)opt
                    format:(NSPropertyListFormat *)format
                     error:(NSError **)error;

@end
#endif

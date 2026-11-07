//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>
#import <Foundation/NSArray.h>
#import <Foundation/NSSet.h>

#if defined(__OBJC__)
@class NSDictionary<KeyType, ObjectType>, NSString;

@interface NSPredicate : NSObject <NSSecureCoding, NSCopying>

+ (NSPredicate *)predicateWithBlock:(BOOL (^)(id evaluatedObject, NSDictionary<NSString *, id> *bindings))block;

@end

@interface NSArray<ObjectType> (NSPredicateSupport)

- (NSArray<ObjectType> *)filteredArrayUsingPredicate:(NSPredicate *)predicate;

@end
#endif

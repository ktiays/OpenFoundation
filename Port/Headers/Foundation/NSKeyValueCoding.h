//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>
#import <Foundation/NSException.h>

#if defined(__OBJC__)
@class NSArray<ObjectType>, NSDictionary<KeyType, ObjectType>, NSError, NSMutableArray<ObjectType>,
    NSMutableOrderedSet<ObjectType>, NSMutableSet<ObjectType>, NSString;

@interface NSObject (NSKeyValueCoding)

- (id)valueForKey:(NSString *)key;
- (void)setValue:(id)value forKey:(NSString *)key;
- (id)valueForKeyPath:(NSString *)keyPath;
- (void)setValue:(id)value forKeyPath:(NSString *)keyPath;
- (id)valueForUndefinedKey:(NSString *)key;

@end
#endif

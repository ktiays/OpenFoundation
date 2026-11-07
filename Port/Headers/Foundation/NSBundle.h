//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>

#if defined(__OBJC__)
@class NSString, NSURL, NSArray<ObjectType>, NSDictionary<KeyType, ObjectType>, NSError;

@interface NSBundle : NSObject

@property (class, readonly, strong) NSBundle *mainBundle;
+ (instancetype)bundleWithPath:(NSString *)path;
- (instancetype)initWithPath:(NSString *)path NS_DESIGNATED_INITIALIZER;
+ (NSBundle *)bundleForClass:(Class)aClass;

- (BOOL)load;
@property (readonly, getter=isLoaded) BOOL loaded;
- (BOOL)unload;

@property (readonly, copy) NSURL *bundleURL;
@property (readonly, copy) NSURL *resourceURL;
@property (readonly, copy) NSURL *executableURL;
@property (readonly, copy) NSString *bundlePath;

- (NSURL *)URLForResource:(NSString *)name withExtension:(NSString *)ext;
- (NSURL *)URLForResource:(NSString *)name withExtension:(NSString *)ext subdirectory:(NSString *)subpath;
- (NSString *)localizedStringForKey:(NSString *)key value:(NSString *)value table:(NSString *)tableName;

@property (readonly, copy) NSString *bundleIdentifier;
@property (readonly, copy) NSDictionary<NSString *, id> *infoDictionary;
@property (readonly, copy) NSDictionary<NSString *, id> *localizedInfoDictionary;
- (id)objectForInfoDictionaryKey:(NSString *)key;
- (Class)classNamed:(NSString *)className;

@end
#endif

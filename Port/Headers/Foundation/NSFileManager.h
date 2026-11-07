//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>
#import <Foundation/NSPathUtilities.h>

#if defined(__OBJC__)
@class NSArray<ObjectType>, NSData, NSDictionary<KeyType, ObjectType>, NSError, NSString, NSURL;

typedef NSString *NSFileAttributeKey NS_TYPED_EXTENSIBLE_ENUM;

typedef NS_OPTIONS(NSUInteger, NSDirectoryEnumerationOptions) {
    NSDirectoryEnumerationSkipsSubdirectoryDescendants = 1UL << 0,
    NSDirectoryEnumerationSkipsPackageDescendants = 1UL << 1,
    NSDirectoryEnumerationSkipsHiddenFiles = 1UL << 2,
    NSDirectoryEnumerationIncludesDirectoriesPostOrder = 1UL << 3,
    NSDirectoryEnumerationProducesRelativePathURLs = 1UL << 4
};

@interface NSFileManager : NSObject

@property (class, readonly, strong) NSFileManager *defaultManager;

- (BOOL)createDirectoryAtPath:(NSString *)path
    withIntermediateDirectories:(BOOL)createIntermediates
                     attributes:(NSDictionary<NSFileAttributeKey, id> *)attributes
                          error:(NSError **)error;
- (BOOL)moveItemAtPath:(NSString *)srcPath toPath:(NSString *)dstPath error:(NSError **)error;
- (BOOL)removeItemAtPath:(NSString *)path error:(NSError **)error;
@property (readonly, copy) NSString *currentDirectoryPath;
- (BOOL)fileExistsAtPath:(NSString *)path;
- (NSString *)stringWithFileSystemRepresentation:(const char *)str length:(NSUInteger)len;
@property (readonly, copy) NSURL *temporaryDirectory;

@end
#endif

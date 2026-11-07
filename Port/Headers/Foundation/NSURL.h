//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>
#import <Foundation/NSString.h>
#import <Foundation/NSCharacterSet.h>

#if defined(__OBJC__)
@class NSArray<ObjectType>, NSData, NSError, NSNumber;

@interface NSURL : NSObject <NSSecureCoding, NSCopying>

+ (NSURL *)fileURLWithPath:(NSString *)path isDirectory:(BOOL)isDir;
+ (NSURL *)fileURLWithPath:(NSString *)path;
- (instancetype)initWithString:(NSString *)URLString relativeToURL:(NSURL *)baseURL NS_DESIGNATED_INITIALIZER;
+ (instancetype)URLWithString:(NSString *)URLString;
+ (instancetype)URLWithString:(NSString *)URLString relativeToURL:(NSURL *)baseURL;

@property (readonly, copy) NSString *absoluteString;
@property (readonly, copy) NSString *path;
@property (readonly, copy) NSString *relativePath;
@property (readonly, getter=isFileURL) BOOL fileURL;

@end

@interface NSURL (NSURLPathUtilities)

@property (readonly, copy) NSURL *URLByDeletingLastPathComponent;

@end

@interface NSCharacterSet (NSURLUtilities)

@property (class, readonly, copy) NSCharacterSet *URLPathAllowedCharacterSet;

@end

@interface NSString (NSURLUtilities)

- (NSString *)stringByAddingPercentEncodingWithAllowedCharacters:(NSCharacterSet *)allowedCharacters;

@end
#endif

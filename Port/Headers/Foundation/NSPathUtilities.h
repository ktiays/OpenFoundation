//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSString.h>
#import <Foundation/NSArray.h>

#if defined(__OBJC__)
@interface NSString (NSStringPathExtensions)

+ (NSString *)pathWithComponents:(NSArray<NSString *> *)components;
@property (readonly, copy) NSArray<NSString *> *pathComponents;
@property (readonly, copy) NSString *lastPathComponent;
@property (readonly, copy) NSString *stringByDeletingLastPathComponent;
- (NSString *)stringByAppendingPathComponent:(NSString *)str;
@property (readonly, copy) NSString *pathExtension;
@property (readonly, copy) NSString *stringByDeletingPathExtension;
@property (readonly, copy) NSString *stringByStandardizingPath;
@property (readonly) const char *fileSystemRepresentation NS_RETURNS_INNER_POINTER;
- (BOOL)getFileSystemRepresentation:(char *)cname maxLength:(NSUInteger)max;

@end

FOUNDATION_EXPORT NSString *NSHomeDirectoryForUser(NSString *userName);
FOUNDATION_EXPORT NSString *NSTemporaryDirectory(void);

typedef NS_ENUM(NSUInteger, NSSearchPathDirectory) {
    NSApplicationDirectory = 1,
    NSDemoApplicationDirectory,
    NSDeveloperApplicationDirectory,
    NSAdminApplicationDirectory,
    NSLibraryDirectory,
    NSDeveloperDirectory,
    NSUserDirectory,
    NSDocumentationDirectory,
    NSDocumentDirectory,
    NSCoreServiceDirectory,
    NSAutosavedInformationDirectory = 11,
    NSDesktopDirectory = 12,
    NSCachesDirectory = 13,
    NSApplicationSupportDirectory = 14,
    NSDownloadsDirectory = 15,
    NSInputMethodsDirectory = 16,
    NSMoviesDirectory = 17,
    NSMusicDirectory = 18,
    NSPicturesDirectory = 19,
    NSPrinterDescriptionDirectory = 20,
    NSSharedPublicDirectory = 21,
    NSPreferencePanesDirectory = 22,
    NSApplicationScriptsDirectory = 23,
    NSItemReplacementDirectory = 99,
    NSAllApplicationsDirectory = 100,
    NSAllLibrariesDirectory = 101,
    NSTrashDirectory = 102
};

typedef NS_OPTIONS(NSUInteger, NSSearchPathDomainMask) {
    NSUserDomainMask = 1,
    NSLocalDomainMask = 2,
    NSNetworkDomainMask = 4,
    NSSystemDomainMask = 8,
    NSAllDomainsMask = 0x0ffff
};

FOUNDATION_EXPORT NSArray<NSString *> *NSSearchPathForDirectoriesInDomains(
    NSSearchPathDirectory directory,
    NSSearchPathDomainMask domainMask,
    BOOL expandTilde
);
#endif

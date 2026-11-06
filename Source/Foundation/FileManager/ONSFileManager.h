//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenFoundation/Base/ONSObjCRuntime.h>

#ifdef __OBJC__
#import <objc/NSObject.h>

@class ONSValue;
@class _ONSFileManagerBridge;

// Native NSFileManager, 0x22e6d7cb8. External. class_ro_t 0x22284e2d8:
// NSObject superclass (shared-cache binding, medium confidence), instance start 8 and size 32.
__attribute__((visibility("default")))
@interface ONSFileManager : NSObject {
    @private
    id _delegate;
    ONSValue *_weakDelegateValue;
    _ONSFileManagerBridge *swiftBridge;
}

+ (id)defaultManager;
- (BOOL)fileExistsAtPath:(id)path;
- (BOOL)getFileSystemRepresentation:(char *)buffer maxLength:(ONSUInteger)length withPath:(id)path;
- (id)stringWithFileSystemRepresentation:(const char *)bytes length:(ONSUInteger)length;
- (id)delegate;
- (void)setDelegate:(id)delegate;
- (BOOL)moveItemAtPath:(id)source toPath:(id)destination error:(id *)error;
- (BOOL)removeItemAtPath:(id)path error:(id *)error;
- (BOOL)createDirectoryAtPath:(id)path
    withIntermediateDirectories:(BOOL)intermediates
                     attributes:(id)attributes
                          error:(id *)error;

@end

@interface ONSFileManager (NSUserInformation)
- (id)temporaryDirectory;
@end
#endif

// Native __initUbiquityIdentityDidChange (0x1c0511dfc) and __postUbqiuityAccountChangeNotification
// (0x1c0511ebc, the native spelling), hidden (were private externals) (FileManager/ONSFileManager.m).
OF_EXTERN void _initUbiquityIdentityDidChange(void);
OF_EXTERN void _postUbqiuityAccountChangeNotification(void);

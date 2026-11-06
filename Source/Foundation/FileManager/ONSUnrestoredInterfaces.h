//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "CoreFoundation/OCFBuild.h"

#import "Foundation/FileManager/ONSFileManager.h"

// Native _NSSearchPathsForDirectoryInDomain, 0x1c08d5f38. Was a private external.
OF_EXTERN id __attribute__((swiftcall))
_NSSearchPathsForDirectoryInDomain(ONSUInteger directory, ONSUInteger domains, BOOL expandTilde);

#ifdef __OBJC__
// Native _NSFileManagerBridge, 0x22e6e2840. Its Swift implementation is outside the current scope.
// Selector boundaries are read from the NSFileManager sends and the Swift entry-point capture.
@interface _ONSFileManagerBridge : NSObject
- (id)initWithFileManager:(ONSFileManager *)manager;
- (BOOL)fileExistsAtPath:(id)path;
- (BOOL)moveItemAtPath:(id)source toPath:(id)destination options:(ONSUInteger)options error:(id *)error;
- (BOOL)removeItemAtPath:(id)path error:(id *)error;
- (BOOL)createDirectoryAtPath:(id)path
    withIntermediateDirectories:(BOOL)intermediates
                     attributes:(id)attributes
                          error:(id *)error;
- (id)temporaryDirectory;
- (id)currentDirectoryPath;
- (BOOL)setAttributes:(id)attributes ofItemAtPath:(id)path error:(id *)error;
@end
#endif

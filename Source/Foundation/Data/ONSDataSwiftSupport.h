//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>
#include <stddef.h>

#include "Foundation/Base/ONSPlatformShims.h"
#import "CoreFoundation/Objective-C/OCFNSData.h"
#import "CoreFoundation/Objective-C/OCFNSMutableData.h"

@class ONSCoder;
@class ONSString;

// The NSData messages that the Swift Data code (Data/Data+Bridging.swift) overrides or sends. NSData's own
// methods are Foundation's NSData(NSData) category, outside the restored scope: each of these is an indirect
// edge that stays open, and the category declares and defines nothing. iOS Simulator Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (the selectors of the captured thunks and super
// sends).
@interface ONSData (DataSwiftMessages)
- (NSUInteger)length;
- (const void *)bytes;
- (id)init;
- (id)initWithCoder:(ONSCoder *)coder;
- (id)initWithBytes:(const void *)bytes length:(NSUInteger)length;
- (id)initWithBytesNoCopy:(void *)bytes length:(NSUInteger)length freeWhenDone:(BOOL)freeWhenDone;
- (id)initWithContentsOfFile:(ONSString *)path;
- (id)initWithContentsOfMappedFile:(ONSString *)path;
- (id)copyWithZone:(struct _NSZone *)zone;
- (id)mutableCopyWithZone:(struct _NSZone *)zone;
- (ONSData *)_bridgingCopy:(const void **)bytes length:(NSUInteger *)length;
@end

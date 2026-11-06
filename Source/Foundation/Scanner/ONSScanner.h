//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "Foundation/Base/ONSFoundationSupport.h"
#include <OpenFoundation/Decimal/ONSDecimal.h>

#ifdef __OBJC__
#import <objc/NSObject.h>

@class ONSConcreteScanner;

// Native NSScanner class 0x22e6dcee8, class_ro_t 0x22285de78: instance start/size 8/8, no ivars.
// Superclass 0x22e6b6088 is external: NSObject (SDK hierarchy, confidence: medium).
// iOS Simulator Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified ivar data.
__attribute__((visibility("default")))
@interface ONSScanner : NSObject
- (id)initWithString:(id)string;
+ (id)scannerWithString:(id)string;
- (id)copyWithZone:(struct _NSZone *)zone;
- (BOOL)scanDouble:(double *)result;
- (BOOL)scanInt:(int *)result;
- (id)charactersToBeSkipped;
- (BOOL)caseSensitive;
- (id)locale;
- (id)string;
- (void)setScanLocation:(NSUInteger)location;
- (NSUInteger)scanLocation;
- (void)setCharactersToBeSkipped:(id)set;
- (void)setCaseSensitive:(BOOL)value;
- (void)setLocale:(id)locale;
@end
#endif

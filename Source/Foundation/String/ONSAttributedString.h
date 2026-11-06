//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#include <OpenFoundation/Range/ONSRange.h>

#include <OpenCoreFoundation/OCFBuild.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

// iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md.

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class NSAttributedString (__DATA,__objc_data 0x22e6d6138, external), the root of the attributed
// string cluster; class_ro_t 0x222847ff8: flags 0, instance start and size 8, no ivars. Its superclass
// pointer (0x22e6b6088) lies outside the image: NSObject, libobjc's root class (iOS Simulator 24A434,
// Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified). Its restored methods are in
// String/ONSAttributedString.m; its categories NSInflection, NSAttributedStringFormattingSPI and
// _NSAttributedStringFromMarkdown are restored in String/ONSAttributedStringInflection.m,
// String/ONSAttributedStringFormatting.m and String/ONSAttributedStringMarkdown.m.
__attribute__((visibility("default")))
@interface ONSAttributedString : NSObject
+ (id)allocWithZone:(struct _NSZone *)zone;
- (id)attribute:(id)name atIndex:(NSUInteger)location effectiveRange:(ONSRange *)range;
- (id)attributedSubstringFromRange:(ONSRange)range;
- (NSUInteger)length;
- (NSUInteger)hash;
- (BOOL)isEqual:(id)object;
- (BOOL)isEqualToAttributedString:(id)other;
- (id)mutableCopyWithZone:(struct _NSZone *)zone;
- (id)copyWithZone:(struct _NSZone *)zone;
- (id)description;
- (id)attributesAtIndex:(NSUInteger)location longestEffectiveRange:(ONSRange *)range inRange:(ONSRange)limit;
- (id)attribute:(id)name
                  atIndex:(NSUInteger)location
    longestEffectiveRange:(ONSRange *)range
                  inRange:(ONSRange)limit;
- (void)enumerateAttributesInRange:(ONSRange)range
                           options:(NSUInteger)options
                        usingBlock:(void (^)(id attributes, ONSRange range, BOOL *stop))block;
- (void)enumerateAttribute:(id)name
                   inRange:(ONSRange)range
                   options:(NSUInteger)options
                usingBlock:(void (^)(id value, ONSRange range, BOOL *stop))block;
+ (BOOL)_subclassesMustBeExplicitlyMentionedWhenDecoded;
- (Class)classForCoder;
- (void)encodeWithCoder:(id)coder;
- (id)initWithCoder:(id)coder;
+ (BOOL)supportsSecureCoding;
- (id)string;
- (id)attributesAtIndex:(NSUInteger)location effectiveRange:(ONSRange *)range;
@end

#endif

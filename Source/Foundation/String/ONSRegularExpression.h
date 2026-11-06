//
//  Created by ktiays on 2026/11/05.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenFoundation/Range/ONSRange.h>

#include <OpenCoreFoundation/OCFBuild.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

// Shared declarations of the NSRegularExpression unit (String/ONSRegularExpression.m), iOS Simulator 24A434,
// arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified. iOS Device 24A446:
// unread. Reading: Source/Foundation/String/RegularExpressionReading.md.

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native NSRegularExpression, 0x22e6e0598 (external); class_ro_t 0x222869be0: flags 0, instance start 8, size
// 36. Ivars: _pattern at 8, _options at 16, _internal (the ICU URegularExpression) at 24 and _checkout (int,
// 1 while one enumeration uses _internal) at 32; offset variables 0x22e6d51b0-0x22e6d51bc. The superclass is
// libobjc's NSObject. Only the methods the captured groups reach are declared.
__attribute__((visibility("default")))
@interface ONSRegularExpression : NSObject {
    id _pattern;
    NSUInteger _options;
    void *_internal;
    int _checkout;
}

+ (void)initialize;
- (id)initWithPattern:(id)pattern options:(NSUInteger)options error:(id *)error;
- (id)copyWithZone:(struct _NSZone *)zone;
- (void)encodeWithCoder:(id)coder;
- (id)initWithCoder:(id)coder;
+ (BOOL)supportsSecureCoding;
- (NSUInteger)hash;
- (BOOL)isEqual:(id)object;
- (void)dealloc;
- (id)description;
- (id)pattern;
- (NSUInteger)options;
- (NSUInteger)numberOfCaptureGroups;
+ (id)escapedPatternForString:(id)string;
@end

@interface ONSRegularExpression (NSMatching)
- (void)enumerateMatchesInString:(id)string
                         options:(NSUInteger)options
                           range:(ONSRange)range
                      usingBlock:(void (^)(id result, NSUInteger flags, BOOL *stop))block;
- (id)matchesInString:(id)string options:(NSUInteger)options range:(ONSRange)range;
- (ONSRange)rangeOfFirstMatchInString:(id)string options:(NSUInteger)options range:(ONSRange)range;
@end

@interface ONSRegularExpression (NSReplacement)
- (id)stringByReplacingMatchesInString:(id)string
                               options:(NSUInteger)options
                                 range:(ONSRange)range
                          withTemplate:(id)replacementTemplate;
- (NSUInteger)replaceMatchesInString:(id)string
                             options:(NSUInteger)options
                               range:(ONSRange)range
                        withTemplate:(id)replacementTemplate;
- (id)replacementStringForResult:(id)result inString:(id)string offset:(NSInteger)offset template:(id)templ;
@end

#endif

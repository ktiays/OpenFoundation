//
//  Created by ktiays on 2026/11/05.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenFoundation/Range/ONSRange.h>

#include <OpenCoreFoundation/OCFBuild.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

// Shared declarations of the regular-expression checking results (String/ONSTextCheckingResult.m), iOS
// Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified. iOS
// Device 24A446: unread. Reading: Source/Foundation/String/RegularExpressionReading.md.

#ifdef __OBJC__
#import <objc/NSObject.h>

@class ONSRegularExpression;

// Native NSTextCheckingResult, 0x22e6df238 (external); class_ro_t 0x2228654d8: flags 0, instance start and
// size 8, no ivars; the superclass is libobjc's NSObject. None of its own methods is reached.
__attribute__((visibility("default")))
@interface ONSTextCheckingResult : NSObject
@end

// Native NSRegularExpressionCheckingResult, 0x22e6df760 (external); class_ro_t 0x2228660b0: flags 0, instance
// start and size 8, no ivars. None of its own methods is reached.
__attribute__((visibility("default")))
@interface ONSRegularExpressionCheckingResult : ONSTextCheckingResult
@end

// Native NSSimpleRegularExpressionCheckingResult, 0x22e6df418 (external); class_ro_t 0x222866188: instance
// start 8, size 64. Ivars: _regularExpression at 8, _ranges ([3] NSRange) at 16; unused ranges hold
// NSNotFound in both words.
__attribute__((visibility("default")))
@interface ONSSimpleRegularExpressionCheckingResult : ONSRegularExpressionCheckingResult {
    ONSRegularExpression *_regularExpression;
    ONSRange _ranges[3];
}

- (id)initWithRanges:(const ONSRange *)ranges count:(NSUInteger)count regularExpression:(id)regularExpression;
- (id)initWithRangeArray:(id)rangeArray regularExpression:(id)regularExpression;
- (ONSRange)range;
- (NSUInteger)numberOfRanges;
- (ONSRange)rangeAtIndex:(NSUInteger)index;
@end

// Native NSExtendedRegularExpressionCheckingResult, 0x22e6df440 (external); class_ro_t 0x222866260: instance
// start 8, size 128. Ivars: _regularExpression at 8, _ranges ([7] NSRange) at 16.
__attribute__((visibility("default")))
@interface ONSExtendedRegularExpressionCheckingResult : ONSRegularExpressionCheckingResult {
    ONSRegularExpression *_regularExpression;
    ONSRange _ranges[7];
}

- (id)initWithRanges:(const ONSRange *)ranges count:(NSUInteger)count regularExpression:(id)regularExpression;
- (id)initWithRangeArray:(id)rangeArray regularExpression:(id)regularExpression;
- (ONSRange)range;
- (NSUInteger)numberOfRanges;
- (ONSRange)rangeAtIndex:(NSUInteger)index;
@end

// Native NSComplexRegularExpressionCheckingResult, 0x22e6df468 (external); class_ro_t 0x222866338: instance
// start 8, size 24. Ivars: _regularExpression at 8, _rangeArray (NSArray of NSValue ranges) at 16.
__attribute__((visibility("default")))
@interface ONSComplexRegularExpressionCheckingResult : ONSRegularExpressionCheckingResult {
    ONSRegularExpression *_regularExpression;
    id _rangeArray;
}

- (id)initWithRanges:(const ONSRange *)ranges count:(NSUInteger)count regularExpression:(id)regularExpression;
- (id)initWithRangeArray:(id)rangeArray regularExpression:(id)regularExpression;
- (ONSRange)range;
- (NSUInteger)numberOfRanges;
- (ONSRange)rangeAtIndex:(NSUInteger)index;
@end

#endif

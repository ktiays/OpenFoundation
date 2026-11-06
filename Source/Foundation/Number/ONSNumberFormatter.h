//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Locale/OCFNumberFormatter.h"
#import "Foundation/Number/ONSFormatter.h"
#include <OpenFoundation/Range/ONSRange.h>

@class ONSMutableDictionary;
@class ONSRecursiveLock;

// Native NSNumberFormatter, 0x22e6dbea8. External. class_ro_t 0x22285a6c0:
// NSFormatter superclass, instance start 8, size 64, seven pointer-width ivars.
__attribute__((visibility("default")))
@interface ONSNumberFormatter : ONSFormatter {
    @package
    ONSMutableDictionary *_attributes;
    OCFNumberFormatterRef _formatter;
    NSUInteger _counter;
    NSUInteger _behavior;
    ONSRecursiveLock *_lock;
    NSUInteger _stateBitMask;
    NSInteger _cacheGeneration;
}

+ (NSUInteger)defaultFormatterBehavior;
+ (void)setDefaultFormatterBehavior:(NSUInteger)behavior;
- (id)stringForObjectValue:(id)object;
- (BOOL)getObjectValue:(id *)object forString:(id)string errorDescription:(id *)errorDescription;
- (BOOL)getObjectValue:(id *)object forString:(id)string range:(ONSRange *)range error:(id *)error;
- (id)stringFromNumber:(id)number;
- (id)numberFromString:(id)string;
- (void)setFormattingContext:(NSInteger)context;
- (id)locale;
- (void)setLocale:(id)locale;
- (BOOL)generatesDecimalNumbers;
- (BOOL)allowsFloats;
- (void)setAllowsFloats:(BOOL)allowsFloats;
- (id)decimalSeparator;
- (void)setDecimalSeparator:(id)separator;
- (void)setUsesGroupingSeparator:(BOOL)grouping;
- (id)groupingSeparator;
- (void)setGroupingSeparator:(id)separator;
- (id)zeroSymbol;
- (id)nilSymbol;
- (void)setNilSymbol:(id)symbol;
- (id)notANumberSymbol;
- (id)positiveInfinitySymbol;
- (void)setPositiveInfinitySymbol:(id)symbol;
- (id)negativeInfinitySymbol;
- (void)setNegativeInfinitySymbol:(id)symbol;
- (NSUInteger)roundingMode;
- (void)setMinimumFractionDigits:(NSUInteger)digits;
- (id)minimum;
- (id)maximum;
- (void)_clearFormatter;
- (void)_regenerateFormatter;

@end

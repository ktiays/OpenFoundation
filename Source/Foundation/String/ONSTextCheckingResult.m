//
//  Created by ktiays on 2026/11/05.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSTextCheckingResult.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/objc-exception.h>

// Inferred organization: the regular-expression checking results lie in the NSTextCheckingResult unit of the
// iOS Simulator Foundation __text section (0x1c061e508-0x1c061f23b for the restored methods); only the
// methods the captured groups reach are restored, and NSTextCheckingResult and
// NSRegularExpressionCheckingResult have none. Objective-C compiled with manual reference counting. Unless a
// definition says otherwise: iOS Simulator 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified (assembly read in full). iOS Device 24A446: unread. Shared-cache bindings (confidence:
// medium): 0x22227FAE0 the NSException class, 0x22227F500 NSInvalidArgumentException, 0x22227FB18 the
// NSMutableArray class.

extern void objc_release(id object);

extern char checkingResultStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSString));
extern char checkingResultExceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));
extern char checkingResultMutableArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                        OF_NAME_STRING(ONSMutableArray));
extern char checkingResultValueClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSValue));

// The messages the unit sends to objects of other classes, declared for the sends only.
@protocol TextCheckingResultMessages
+ (id)stringWithFormat:(id)format, ...;
+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
+ (id)array;
+ (id)valueWithRange:(ONSRange)range;
- (void)addObject:(id)object;
- (NSUInteger)count;
- (id)objectAtIndex:(NSUInteger)index;
- (ONSRange)rangeValue;
- (id)copy;
@end

// NSNotFound, the filler of unused range words.
static const NSUInteger kNotFound = LONG_MAX;

static void raiseInvalidArgument(id format, id object, SEL selector) __attribute__((noreturn));

// The raise of the unit's argument checks: an NSInvalidArgumentException whose reason formats the full method
// name (the native methods inline this sequence).
static void raiseInvalidArgument(id format, id object, SEL selector) {
    id reason = [(id)checkingResultStringClass stringWithFormat:format, _ONSFullMethodName(object, selector)];
    objc_exception_throw([(id)checkingResultExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                                      reason:reason
                                                                    userInfo:nil]);
}

@implementation ONSTextCheckingResult
@end

@implementation ONSRegularExpressionCheckingResult
@end

@implementation ONSSimpleRegularExpressionCheckingResult

// Native -[NSSimpleRegularExpressionCheckingResult initWithRanges:count:regularExpression:], 0x1c061e508.
// One to three ranges; the unused words are NSNotFound.
- (id)initWithRanges:(const ONSRange *)ranges
                count:(NSUInteger)count
    regularExpression:(id)regularExpression {
    if (count - 4 <= (NSUInteger)-4) {
        raiseInvalidArgument((id)OF_STR("%@: must have 1-3 ranges"), self, _cmd);
    }
    self = [super init];
    if (self != nil) {
        _regularExpression = [regularExpression copy];
        for (NSUInteger index = 0; index < count; ++index) {
            _ranges[index] = ranges[index];
        }
        if (count <= 2 && 2 * count != 6) {
            NSUInteger *words = (NSUInteger *)&_ranges[count];
            for (NSUInteger index = 0; index < 6 - 2 * count; ++index) {
                words[index] = kNotFound;
            }
        }
    }
    return self;
}

// Native -[NSSimpleRegularExpressionCheckingResult initWithRangeArray:regularExpression:], 0x1c061e63c. At
// most three ranges are read; the array's count is passed on (and checked there).
- (id)initWithRangeArray:(id)rangeArray regularExpression:(id)regularExpression {
    ONSRange ranges[3];
    NSUInteger count = [rangeArray count];
    NSUInteger read = count >= 3 ? 3 : count;
    for (NSUInteger index = 0; index != read; ++index) {
        ranges[index] = [[rangeArray objectAtIndex:index] rangeValue];
    }
    return [self initWithRanges:ranges count:count regularExpression:regularExpression];
}

// Native -[NSSimpleRegularExpressionCheckingResult dealloc], 0x1c061e70c.
- (void)dealloc {
    objc_release(_regularExpression);
    [super dealloc];
}

// Native -[NSSimpleRegularExpressionCheckingResult range], 0x1c061e788.
- (ONSRange)range {
    return _ranges[0];
}

// Native -[NSSimpleRegularExpressionCheckingResult numberOfRanges], 0x1c061e878. The index of the first range
// holding NSNotFound in both words, else 3.
- (NSUInteger)numberOfRanges {
    for (NSUInteger index = 1; index != 3; ++index) {
        if (_ranges[index].location == kNotFound && _ranges[index].length == kNotFound) {
            return index;
        }
    }
    return 3;
}

// Native -[NSSimpleRegularExpressionCheckingResult rangeAtIndex:], 0x1c061e8e8. An index past the ranges, or
// an unused range, raises.
- (ONSRange)rangeAtIndex:(NSUInteger)index {
    ONSRange range = {kNotFound, kNotFound};
    if (index <= 2) {
        range = _ranges[index];
    }
    if (range.location != kNotFound || range.length != kNotFound) {
        return range;
    }
    id reason = [(id)checkingResultStringClass
        stringWithFormat:(id)OF_STR("%@: index %lu out of range"), _ONSFullMethodName(self, _cmd), index];
    objc_exception_throw([(id)checkingResultExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                                      reason:reason
                                                                    userInfo:nil]);
}

@end

@implementation ONSExtendedRegularExpressionCheckingResult

// Native -[NSExtendedRegularExpressionCheckingResult initWithRanges:count:regularExpression:], 0x1c061e9f8.
// One to seven ranges; the unused words are NSNotFound.
- (id)initWithRanges:(const ONSRange *)ranges
                count:(NSUInteger)count
    regularExpression:(id)regularExpression {
    if (count - 8 <= (NSUInteger)-8) {
        raiseInvalidArgument((id)OF_STR("%@: must have 1-7 ranges"), self, _cmd);
    }
    self = [super init];
    if (self != nil) {
        _regularExpression = [regularExpression copy];
        for (NSUInteger index = 0; index < count; ++index) {
            _ranges[index] = ranges[index];
        }
        if (count <= 6 && 2 * count != 14) {
            NSUInteger *words = (NSUInteger *)&_ranges[count];
            for (NSUInteger index = 0; index < 14 - 2 * count; ++index) {
                words[index] = kNotFound;
            }
        }
    }
    return self;
}

// Native -[NSExtendedRegularExpressionCheckingResult initWithRangeArray:regularExpression:], 0x1c061eb2c. At
// most seven ranges are read; the array's count is passed on.
- (id)initWithRangeArray:(id)rangeArray regularExpression:(id)regularExpression {
    ONSRange ranges[7];
    NSUInteger count = [rangeArray count];
    NSUInteger read = count >= 7 ? 7 : count;
    for (NSUInteger index = 0; index != read; ++index) {
        ranges[index] = [[rangeArray objectAtIndex:index] rangeValue];
    }
    return [self initWithRanges:ranges count:count regularExpression:regularExpression];
}

// Native -[NSExtendedRegularExpressionCheckingResult dealloc], 0x1c061ebfc.
- (void)dealloc {
    objc_release(_regularExpression);
    [super dealloc];
}

// Native -[NSExtendedRegularExpressionCheckingResult range], 0x1c061ec78.
- (ONSRange)range {
    return _ranges[0];
}

// Native -[NSExtendedRegularExpressionCheckingResult numberOfRanges], 0x1c061ed68. The index of the first
// range holding NSNotFound in both words, else 7.
- (NSUInteger)numberOfRanges {
    NSUInteger index;
    for (index = 1; index != 7; ++index) {
        if (_ranges[index].location == kNotFound && _ranges[index].length == kNotFound) {
            break;
        }
    }
    return index;
}

// Native -[NSExtendedRegularExpressionCheckingResult rangeAtIndex:], 0x1c061eda0.
- (ONSRange)rangeAtIndex:(NSUInteger)index {
    ONSRange range = {kNotFound, kNotFound};
    if (index <= 6) {
        range = _ranges[index];
    }
    if (range.location != kNotFound || range.length != kNotFound) {
        return range;
    }
    id reason = [(id)checkingResultStringClass
        stringWithFormat:(id)OF_STR("%@: index %lu out of range"), _ONSFullMethodName(self, _cmd), index];
    objc_exception_throw([(id)checkingResultExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                                      reason:reason
                                                                    userInfo:nil]);
}

@end

@implementation ONSComplexRegularExpressionCheckingResult

// Native -[NSComplexRegularExpressionCheckingResult initWithRanges:count:regularExpression:], 0x1c061eeb0.
// The ranges are boxed into an array.
- (id)initWithRanges:(const ONSRange *)ranges
                count:(NSUInteger)count
    regularExpression:(id)regularExpression {
    id rangeArray = [(id)checkingResultMutableArrayClass array];
    for (NSUInteger index = 0; index != count; ++index) {
        [rangeArray addObject:[(id)checkingResultValueClass valueWithRange:ranges[index]]];
    }
    return [self initWithRangeArray:rangeArray regularExpression:regularExpression];
}

// Native -[NSComplexRegularExpressionCheckingResult initWithRangeArray:regularExpression:], 0x1c061ef34. At
// least one range.
- (id)initWithRangeArray:(id)rangeArray regularExpression:(id)regularExpression {
    if ([rangeArray count] == 0) {
        raiseInvalidArgument((id)OF_STR("%@: must have at least one range"), self, _cmd);
    }
    self = [super init];
    if (self != nil) {
        _regularExpression = [regularExpression copy];
        _rangeArray = [rangeArray copy];
    }
    return self;
}

// Native -[NSComplexRegularExpressionCheckingResult dealloc], 0x1c061f034.
- (void)dealloc {
    objc_release(_regularExpression);
    objc_release(_rangeArray);
    [super dealloc];
}

// Native -[NSComplexRegularExpressionCheckingResult range], 0x1c061f0b8.
- (ONSRange)range {
    return [[_rangeArray objectAtIndex:0] rangeValue];
}

// Native -[NSComplexRegularExpressionCheckingResult numberOfRanges], 0x1c061f21c.
- (NSUInteger)numberOfRanges {
    return [_rangeArray count];
}

// Native -[NSComplexRegularExpressionCheckingResult rangeAtIndex:], 0x1c061f224.
- (ONSRange)rangeAtIndex:(NSUInteger)index {
    return [[_rangeArray objectAtIndex:index] rangeValue];
}

@end

//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSConcreteAttributedString.h"
#import "Foundation/Archiving/ONSObjCRuntimeNames.h"
#import "Foundation/String/ONSAttributedStringInternal.h"
#import "Foundation/String/ONSLog.h"
#import "Foundation/String/ONSMutableString.h"
#import "Foundation/String/ONSRLEArray.h"
#import "Foundation/String/ONSSimpleAttributeDictionary.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Objective-C/OCFNSException.h"
#include "CoreFoundation/String/OCFString.h"

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified unless noted. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.

extern char concreteMutableRLEArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableRLEArray));

@protocol ConcreteAttributedStringMessages
- (ONSRLEArray *)_runArrayHoldingAttributes;
- (struct _NSZone *)zone;
@end

// Inlined in the native string initializers and attribute setter. The three initializers use
// independent warning bytes. The shared-cache exception class and name bindings have medium confidence.
static inline __attribute__((noreturn)) void throwNilAttributedStringValue(id self, SEL cmd) {
    const char *name = _NSNameOfClass(objc_opt_class(self));
    const char *selector = sel_getName(cmd);
    id reason =
        [(id)attributedStringStringClass stringWithFormat:(id)OF_STR("%s %s: nil value"), name, selector];
    objc_exception_throw([(id)attributedStringExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                                        reason:reason
                                                                      userInfo:nil]);
}

static inline void warnForNilAttributedString(id self, SEL cmd, bool *warned) {
    if (_OCFExecutableLinkedOnOrAfter(6)) {
        throwNilAttributedStringValue(self, cmd);
    }
    if (!__atomic_load_n(warned, __ATOMIC_RELAXED)) {
        Class cls = objc_opt_class(self);
        id selector = ONSStringFromSelector(cmd);
        ONSLog(
            (id)OF_STR(
                "-[%@ %@] called with nil string argument. This has undefined behavior and will "
                "raise an exception in post-Leopard linked apps. This warning is displayed only once."
            ),
            cls,
            selector
        );
        __atomic_store_n(warned, true, __ATOMIC_RELAXED);
    }
}

@implementation ONSConcreteAttributedString

// Native -[NSConcreteAttributedString initWithString:], 0x1c04c4948. Non-external.
// Foundation defect: use relaxed atomic accesses for the unsynchronized warning byte.
- (id)initWithString:(id)value {
    if (value == nil) {
        static bool warned;
        warnForNilAttributedString(self, _cmd, &warned);
    }
    self = [super init];
    if (self != nil) {
        string = [value copyWithZone:[(id<ConcreteAttributedStringMessages>)self zone]];
        if (value != nil && string == nil) {
            objc_release(self);
            return nil;
        }
        attributes = nil;
    }
    return self;
}

// Native -[NSConcreteAttributedString initWithString:attributes:], 0x1c04c4aa8. Non-external.
- (id)initWithString:(id)value attributes:(id)dictionary {
    self = [self initWithString:value];
    if (dictionary != nil && self != nil) {
        ONSMutableRLEArray *runs = [[(id)concreteMutableRLEArrayClass
            allocWithZone:[(id<ConcreteAttributedStringMessages>)self zone]] init];
        id converted = [(id)_NSAttributeDictionaryClass() newWithDictionary:dictionary];
        [runs insertObject:converted range:(ONSRange){0, [self length]}];
        if ([self _willRequireIntentResolutionWhenContainingAttributes:converted]) {
            [self _markRequiringIntentResolution];
        }
        objc_release(converted);
        attributes = runs;
    }
    return self;
}

// Native -[NSConcreteAttributedString initWithAttributedString:], 0x1c04c4b98. Non-external.
- (id)initWithAttributedString:(id)value {
    self = [self initWithString:[(ONSAttributedString *)value string]];
    if (self != nil) {
        if (objc_opt_respondsToSelector(value, @selector(_runArrayHoldingAttributes))) {
            ONSRLEArray *runs = [(id<ConcreteAttributedStringMessages>)value _runArrayHoldingAttributes];
            if (runs != nil) {
                attributes = [runs copyWithZone:[(id<ConcreteAttributedStringMessages>)self zone]];
            }
        }
        if (attributes == nil) {
            NSUInteger length = [string length];
            ONSMutableRLEArray *runs = [[(id)concreteMutableRLEArrayClass
                allocWithZone:[(id<ConcreteAttributedStringMessages>)self zone]] init];
            for (NSUInteger index = 0; index < length;) {
                ONSRange range = {0, 0};
                Class dictionaryClass = _NSAttributeDictionaryClass();
                id dictionary = [(ONSAttributedString *)value attributesAtIndex:index effectiveRange:&range];
                id converted = [(id)dictionaryClass newWithDictionary:dictionary];
                NSUInteger end = range.location + range.length;
                NSUInteger count = (end <= length ? end : length) - index;
                [runs insertObject:converted range:(ONSRange){index, count}];
                objc_release(converted);
                index += count;
            }
            attributes = runs;
        }
        if ([(ONSAttributedString *)value _mayRequireIntentResolution]) {
            [self _markRequiringIntentResolution];
        }
    }
    return self;
}

// Native -[NSConcreteAttributedString init], 0x1c04c4d2c. Non-external.
- (id)init {
    return [self initWithString:(id)OF_STR("")];
}

// Native -[NSConcreteAttributedString dealloc], 0x1c04c4d38. Non-external.
- (void)dealloc {
    objc_release(string);
    objc_release(attributes);
    [super dealloc];
}

// Native -[NSConcreteAttributedString copyWithZone:], 0x1c04c4db4. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    if (_OCFExecutableLinkedOnOrAfter(9)) {
        return objc_retain(self);
    }
    return [super copyWithZone:zone];
}

// Native -[NSConcreteAttributedString attributesAtIndex:effectiveRange:], 0x1c04c4e64. Non-external.
- (id)attributesAtIndex:(NSUInteger)index effectiveRange:(ONSRange *)range {
    if (attributes != nil) {
        return [attributes objectAtIndex:index effectiveRange:range];
    }
    if (range != NULL) {
        *range = (ONSRange){0, [string length]};
    }
    return [(id)_NSAttributeDictionaryClass() emptyAttributeDictionary];
}

// Native -[NSConcreteAttributedString attribute:atIndex:effectiveRange:], 0x1c04c4eb0. Non-external.
- (id)attribute:(id)key atIndex:(NSUInteger)index effectiveRange:(ONSRange *)range {
    if (attributes != nil) {
        return [[attributes objectAtIndex:index effectiveRange:range] objectForKey:key];
    }
    if (range != NULL) {
        *range = (ONSRange){0, [string length]};
    }
    return nil;
}

// Native -[NSConcreteAttributedString length], 0x1c04c4f0c. Non-external.
- (NSUInteger)length {
    return [string length];
}

// Native -[NSConcreteAttributedString string], 0x1c04c4f14. Non-external.
- (id)string {
    return string;
}

// Native -[NSConcreteAttributedString _runArrayHoldingAttributes], 0x1c04c4f1c. Non-external.
- (ONSRLEArray *)_runArrayHoldingAttributes {
    return attributes;
}

@end

@implementation ONSConcreteMutableAttributedString

// Native +[NSConcreteMutableAttributedString _mutableStringClass], 0x1c04c56ec. Non-external.
+ (Class)_mutableStringClass {
    return __NSMutableStringClass;
}

// Native -[NSConcreteMutableAttributedString initWithAttributedString:], 0x1c04c56f8. Non-external.
- (id)initWithAttributedString:(id)value {
    self = [super init];
    if (self != nil) {
        struct _NSZone *zone = [(id<ConcreteAttributedStringMessages>)self zone];
        Class cls = [objc_opt_class(self) _mutableStringClass];
        if (value != nil) {
            if (objc_opt_respondsToSelector(value, @selector(_runArrayHoldingAttributes))) {
                ONSRLEArray *runs = [(id<ConcreteAttributedStringMessages>)value _runArrayHoldingAttributes];
                if (runs != nil) {
                    mutableAttributes = [runs mutableCopyWithZone:zone];
                }
            }
            if (mutableAttributes == nil) {
                NSUInteger length = [(ONSString *)[(ONSAttributedString *)value string] length];
                mutableAttributes = [[(id)concreteMutableRLEArrayClass allocWithZone:zone] init];
                for (NSUInteger index = 0; index < length;) {
                    ONSRange range = {0, 0};
                    Class dictionaryClass = _NSAttributeDictionaryClass();
                    id dictionary = [(ONSAttributedString *)value attributesAtIndex:index
                                                                     effectiveRange:&range];
                    id converted = [(id)dictionaryClass newWithDictionary:dictionary];
                    NSUInteger end = range.location + range.length;
                    NSUInteger count = (end <= length ? end : length) - index;
                    [mutableAttributes insertObject:converted range:(ONSRange){index, count}];
                    objc_release(converted);
                    index += count;
                }
            }
            if (cls == __NSMutableStringClass) {
                mutableString = [[(ONSAttributedString *)value string] mutableCopyWithZone:zone];
            } else {
                id allocated = [cls allocWithZone:zone];
                mutableString = [allocated initWithString:[(ONSAttributedString *)value string]];
            }
            if ([(ONSAttributedString *)value _mayRequireIntentResolution]) {
                [self _markRequiringIntentResolution];
            }
        } else {
            mutableAttributes = [[(id)concreteMutableRLEArrayClass allocWithZone:zone] init];
            mutableString = [[cls allocWithZone:zone] init];
        }
    }
    return self;
}

// Native -[NSConcreteMutableAttributedString initWithString:attributes:], 0x1c04c5918. Non-external.
// Foundation defect: use relaxed atomic accesses for the unsynchronized warning byte.
- (id)initWithString:(id)value attributes:(id)dictionary {
    if (value == nil) {
        static bool warned;
        warnForNilAttributedString(self, _cmd, &warned);
    }
    self = [self initWithAttributedString:nil];
    if (self != nil) {
        [self replaceCharactersInRange:(ONSRange){0, 0} withString:value];
        id converted = [(id)_NSAttributeDictionaryClass() newWithDictionary:dictionary];
        NSUInteger length = [self length];
        [mutableAttributes replaceObjectsInRange:(ONSRange){0, length} withObject:converted length:length];
        if ([self _willRequireIntentResolutionWhenContainingAttributes:converted]) {
            [self _markRequiringIntentResolution];
        }
        objc_release(converted);
    }
    return self;
}

// Native -[NSConcreteMutableAttributedString initWithString:], 0x1c04c5a70. Non-external.
// Foundation defect: use relaxed atomic accesses for the unsynchronized warning byte.
- (id)initWithString:(id)value {
    if (value == nil) {
        static bool warned;
        warnForNilAttributedString(self, _cmd, &warned);
    }
    self = [self initWithAttributedString:nil];
    if (self != nil) {
        [self replaceCharactersInRange:(ONSRange){0, 0} withString:value];
    }
    return self;
}

// Native -[NSConcreteMutableAttributedString init], 0x1c04c5b78. Non-external.
- (id)init {
    return [self initWithString:(id)OF_STR("")];
}

// Native -[NSConcreteMutableAttributedString dealloc], 0x1c04c5b84. Non-external.
- (void)dealloc {
    objc_release(mutableString);
    objc_release(mutableAttributes);
    [super dealloc];
}

// Native -[NSConcreteMutableAttributedString attributesAtIndex:effectiveRange:], 0x1c04c5c00. Non-external.
- (id)attributesAtIndex:(NSUInteger)index effectiveRange:(ONSRange *)range {
    return [mutableAttributes objectAtIndex:index effectiveRange:range];
}

// Native -[NSConcreteMutableAttributedString attribute:atIndex:effectiveRange:], 0x1c04c5c08. Non-external.
- (id)attribute:(id)key atIndex:(NSUInteger)index effectiveRange:(ONSRange *)range {
    return [[mutableAttributes objectAtIndex:index effectiveRange:range] objectForKey:key];
}

// Native -[NSConcreteMutableAttributedString length], 0x1c04c5c38. Non-external.
- (NSUInteger)length {
    return [mutableString length];
}

// Native -[NSConcreteMutableAttributedString string], 0x1c04c5c40. Non-external.
- (id)string {
    return mutableString;
}

// Native -[NSConcreteMutableAttributedString _runArrayHoldingAttributes], 0x1c04c5c48. Non-external.
- (ONSRLEArray *)_runArrayHoldingAttributes {
    return mutableAttributes;
}

// Native -[NSConcreteMutableAttributedString replaceCharactersInRange:withAttributedString:],
// 0x1c04c5c50. Non-external.
- (void)replaceCharactersInRange:(ONSRange)range withAttributedString:(id)value {
    ONSString *string = [(ONSAttributedString *)value string];
    NSUInteger length = [string length];
    if (length != 0) {
        ONSRange source = {0, 0};
        do {
            Class dictionaryClass = _NSAttributeDictionaryClass();
            id dictionary = [(ONSAttributedString *)value attributesAtIndex:source.location
                                                             effectiveRange:&source];
            id converted = [(id)dictionaryClass newWithDictionary:dictionary];
            [mutableAttributes insertObject:converted
                                      range:(ONSRange){range.location + source.location, source.length}];
            objc_release(converted);
            source.location += source.length;
        } while (source.location < length);
    }
    if (range.length != 0) {
        [mutableAttributes deleteObjectsInRange:(ONSRange){range.location + length, range.length}];
    }
    [mutableString replaceCharactersInRange:range withString:string];
    if ([(ONSAttributedString *)value _mayRequireIntentResolution]) {
        [self _markRequiringIntentResolution];
    }
    [self edited:3 range:range changeInLength:(NSInteger)(length - range.length)];
}

// Native -[NSConcreteMutableAttributedString addAttribute:value:range:], 0x1c04c5d88. Non-external.
- (void)addAttribute:(id)key value:(id)value range:(ONSRange)range {
    if (range.length == 0) {
        return;
    }
    if (value == nil) {
        throwNilAttributedStringValue(self, _cmd);
    }
    ONSRange remaining = range;
    do {
        ONSRange effective = {0, 0};
        id current = [mutableAttributes objectAtIndex:remaining.location effectiveRange:&effective];
        effective = attributedStringIntersectRanges(effective, remaining);
        id changed = [current newWithKey:key object:value];
        if (changed != current) {
            [mutableAttributes replaceObjectsInRange:effective withObject:changed length:effective.length];
        }
        objc_release(changed);
        remaining.location += effective.length;
        remaining.length -= effective.length;
    } while (remaining.length != 0);
    if ([self _willRequireIntentResolutionWhenContainingAttribute:key value:value]) {
        [self _markRequiringIntentResolution];
    }
    [self edited:1 range:range changeInLength:0];
}

// Native -[NSConcreteMutableAttributedString attribute:atIndex:longestEffectiveRange:inRange:],
// 0x1c04c5f40. Non-external. Selector at 0x2387cf54a is objectAtRunIndex:length: (confidence: medium).
// The method it invokes is absent from this capture.
- (id)attribute:(id)key
                  atIndex:(NSUInteger)index
    longestEffectiveRange:(ONSRange *)range
                  inRange:(ONSRange)limit {
    ONSRange effective = {0, 0};
    NSUInteger run = 0;
    id value = [[mutableAttributes objectAtIndex:index effectiveRange:&effective
                                        runIndex:&run] objectForKey:key];
    if (range == NULL) {
        return value;
    }
    NSUInteger runLength = 0;
    NSUInteger start = effective.location;
    NSUInteger end = start + effective.length;
    NSUInteger limitEnd = limit.location + limit.length;
    SEL runSelector = sel_registerName("objectAtRunIndex:length:");
    id (*getRun)(id, SEL, NSUInteger, NSUInteger *) =
        (void *)class_getMethodImplementation(objc_opt_class(mutableAttributes), runSelector);
    SEL equalSelector = @selector(isEqual:);
    BOOL (*equal)(id, SEL, id) = NULL;
    if (value != nil) {
        equal = (void *)class_getMethodImplementation(objc_opt_class(value), equalSelector);
    }
    for (NSUInteger next = run + 1; end < limitEnd; ++next) {
        id dictionary = getRun(mutableAttributes, runSelector, next, &runLength);
        if (dictionary == nil) {
            break;
        }
        id other = [dictionary objectForKey:key];
        if (value != other && (value == nil || !equal(value, equalSelector, other))) {
            break;
        }
        end += runLength;
    }
    for (NSUInteger previous = run - 1; start > limit.location; --previous) {
        id other = [getRun(mutableAttributes, runSelector, previous, &runLength) objectForKey:key];
        if (value != other && (value == nil || !equal(value, equalSelector, other))) {
            break;
        }
        start -= runLength;
    }
    *range = attributedStringIntersectRanges(limit, (ONSRange){start, end - start});
    return value;
}

// Native -[NSConcreteMutableAttributedString setAttributes:range:], 0x1c04c616c. Non-external.
- (void)setAttributes:(id)dictionary range:(ONSRange)range {
    if (range.length != 0) {
        id converted = [(id)_NSAttributeDictionaryClass() newWithDictionary:dictionary];
        [mutableAttributes replaceObjectsInRange:range withObject:converted length:range.length];
        if ([self _willRequireIntentResolutionWhenContainingAttributes:converted]) {
            [self _markRequiringIntentResolution];
        }
        objc_release(converted);
        [self edited:1 range:range changeInLength:0];
    }
}

// Native -[NSConcreteMutableAttributedString replaceCharactersInRange:withString:], 0x1c04c61fc.
// Non-external.
- (void)replaceCharactersInRange:(ONSRange)range withString:(id)value {
    NSUInteger length = [(ONSString *)value length];
    id dictionary = nil;
    if (length != 0) {
        if (range.length != 0) {
            dictionary = [mutableAttributes objectAtIndex:range.location effectiveRange:NULL];
            objc_retain(dictionary);
            [mutableAttributes replaceObjectsInRange:range withObject:dictionary length:length];
        } else {
            if (range.location != 0) {
                dictionary = [mutableAttributes objectAtIndex:range.location - 1 effectiveRange:NULL];
            } else if ([mutableString length] != 0) {
                dictionary = [mutableAttributes objectAtIndex:0 effectiveRange:NULL];
            } else {
                dictionary = [(id)_NSAttributeDictionaryClass() emptyAttributeDictionary];
            }
            objc_retain(dictionary);
            [mutableAttributes insertObject:dictionary range:(ONSRange){range.location, length}];
        }
    } else if (range.length != 0) {
        [mutableAttributes replaceObjectsInRange:range withObject:nil length:0];
    }
    if (dictionary != nil) {
        objc_release(dictionary);
    }
    [mutableString replaceCharactersInRange:range withString:value];
    [self edited:2 range:range changeInLength:(NSInteger)(length - range.length)];
}

// Native -[NSConcreteMutableAttributedString edited:range:changeInLength:], 0x1c04c6310.
// Non-external. The complete native body is a RET instruction. The selector is a subclass hook.
- (void)edited:(NSUInteger)mask range:(ONSRange)range changeInLength:(NSInteger)delta {
    (void)mask;
    (void)range;
    (void)delta;
}

// Native -[NSConcreteMutableAttributedString initWithCoder:], 0x1c04c6314. Non-external.
- (id)initWithCoder:(id)coder {
    self = [self init];
    if (!_NSReadMutableAttributedStringWithCoder(coder, self)) {
        objc_release(self);
        return nil;
    }
    return self;
}

// Native +[NSConcreteMutableAttributedString supportsSecureCoding], 0x1c04c6354. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSConcreteMutableAttributedString _mayRequireIntentResolution], 0x1c04c635c. Non-external.
- (BOOL)_mayRequireIntentResolution {
    return fields.mayNeedIntentResolution != 0;
}

// Native -[NSConcreteMutableAttributedString _markRequiringIntentResolution], 0x1c04c636c. Non-external.
- (void)_markRequiringIntentResolution {
    fields.mayNeedIntentResolution = 1;
}

@end

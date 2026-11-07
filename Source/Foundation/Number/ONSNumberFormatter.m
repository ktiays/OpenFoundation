//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Number/ONSNumberFormatter.h"

#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Locale/OCFLocale.h"
#include "CoreFoundation/Number/OCFNumber.h"
#import "CoreFoundation/Objective-C/OCFNSArray.h"
#import "CoreFoundation/Objective-C/OCFNSMutableDictionary.h"
#import "CoreFoundation/Objective-C/OCFNSLocale.h"
#import "Foundation/Archiving/ONSArchivingPrivate.h"
#import "Foundation/Base/ONSUnrestoredInterfaces.h"
#import "Foundation/Error/ONSErrorConstants.h"
#import "Foundation/Locale/ONSLocaleNotifications.h"
#import "Foundation/Lock/ONSLock.h"
#import "Foundation/Number/ONSNumberDecimalNumberExtensions.h"
#import "Foundation/Number/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSCharacterSet.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"

#include <math.h>
#include <objc/objc-exception.h>

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified except the shared-cache GOT bindings, identified by property order and use.
// iOS Device divergence: iOS Device 24A446 remains unread. Reading: Foundation/Number/Reading.md, round 2.
// Inferred organization follows the contiguous NSNumberFormatter native methods. Manual reference counting.

extern char numberFormatterClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSNumberFormatter));
extern char numberFormatterStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSString));
extern char numberFormatterLockClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSRecursiveLock));
extern char numberFormatterDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                       OF_NAME_STRING(ONSMutableDictionary));

// Native __NSNumberFormatterDefaultBehavior, 0x22e6fcbc0. Non-external, zero-initialized.
static _Atomic(NSUInteger) defaultFormatterBehavior;
static _Atomic(IMP) rangeParsingBaseImplementation;

static BOOL getObjectValue_0(
    ONSNumberFormatter *self, id *object, ONSString *string, ONSRange *range, BOOL checkTail, id *error
);

@implementation ONSNumberFormatter

// Native +[NSNumberFormatter defaultFormatterBehavior], 0x1c0576718. Non-external.
// Foundation defect: the native read at 0x1c057671c races with the setter's store at 0x1c0576734.
// Preserve the value and store semantics with a relaxed atomic.
+ (NSUInteger)defaultFormatterBehavior {
    return atomic_load_explicit(&defaultFormatterBehavior, memory_order_relaxed);
}

// Native +[NSNumberFormatter setDefaultFormatterBehavior:], 0x1c0576724. Non-external.
+ (void)setDefaultFormatterBehavior:(NSUInteger)behavior {
    atomic_store_explicit(&defaultFormatterBehavior, behavior != 0 ? behavior : 1040, memory_order_relaxed);
}

// Native +[NSNumberFormatter initialize], 0x1c057673c. Non-external.
+ (void)initialize {
    if (atomic_load_explicit(&defaultFormatterBehavior, memory_order_relaxed) == 0) {
        [self setDefaultFormatterBehavior:0];
    }
}

// Native -[NSNumberFormatter _clearFormatter], 0x1c057676c. Non-external.
- (void)_clearFormatter {
    if (_formatter != NULL) {
        OCFRelease(_formatter);
    }
    _formatter = NULL;
    _cacheGeneration = (NSInteger)((NSUInteger)_cacheGeneration + 1);
}

// Native -[NSNumberFormatter _regenerateFormatter], 0x1c05767b4. Non-external.
- (void)_regenerateFormatter {
    if (_formatter != NULL) {
        [self _clearFormatter];
    }
    _counter = (NSUInteger)_localeNotificationCount();
    id locale = [_attributes objectForKey:(id)OF_STR("locale")];
    id style = [_attributes objectForKey:(id)OF_STR("numberStyle")];
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    if (locale == nil) {
        locale = [ONSLocale currentLocale];
    }
    _formatter = OCFNumberFormatterCreate(allocator, (OCFLocaleRef)locale, [style integerValue]);
    if (_formatter == NULL) {
        return;
    }
    id positive = [_attributes objectForKey:(id)OF_STR("positiveFormat")];
    id negative = [_attributes objectForKey:(id)OF_STR("negativeFormat")];
    if (positive != nil) {
        if (negative == nil || [negative isEqualToString:positive]) {
            OCFNumberFormatterSetFormat(_formatter, (OCFStringRef)positive);
        } else {
            id combined = [objc_alloc((Class)numberFormatterStringClass)
                initWithFormat:(id)OF_STR("%@;%@"), positive, negative];
            OCFNumberFormatterSetFormat(_formatter, (OCFStringRef)combined);
            objc_release(combined);
        }
    } else if (negative != nil) {
        OCFStringRef format = OCFNumberFormatterGetFormat(_formatter);
        id parts = objc_autorelease([[(id)format componentsSeparatedByString:(id)OF_STR(";")] mutableCopy]);
        if ([parts count] > 1) {
            [parts replaceObjectAtIndex:1 withObject:negative];
        } else {
            [parts addObject:negative];
        }
        OCFStringRef updated = (OCFStringRef)[parts componentsJoinedByString:(id)OF_STR(";")];
        OCFNumberFormatterSetFormat(_formatter, updated);
    }
    id value;

    value = [_attributes objectForKey:(id)OF_STR("alwaysShowsDecimalSeparator")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterAlwaysShowDecimalSeparatorKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("decimalSeparator")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterDecimalSeparatorKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("currencyDecimalSeparator")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterCurrencyDecimalSeparatorKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("usesGroupingSeparator")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterUseGroupingSeparatorKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("zeroSymbol")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterZeroSymbolKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("notANumberSymbol")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterNaNSymbolKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("groupingSeparator")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterGroupingSeparatorKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("positivePrefix")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterPositivePrefixKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("positiveSuffix")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterPositiveSuffixKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("negativePrefix")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterNegativePrefixKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("negativeSuffix")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterNegativeSuffixKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("currencyCode")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterCurrencyCodeKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("currencySymbol")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterCurrencySymbolKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("internationalCurrencySymbol")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterInternationalCurrencySymbolKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("percentSymbol")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterPercentSymbolKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("perMillSymbol")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterPerMillSymbolKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("minusSign")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterMinusSignKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("plusSign")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterPlusSignKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("exponentSymbol")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterExponentSymbolKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("groupingSize")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterGroupingSizeKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("secondaryGroupingSize")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterSecondaryGroupingSizeKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("formatWidth")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterFormatWidthKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("multiplier")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterMultiplierKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("paddingCharacter")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterPaddingCharacterKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("paddingPosition")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterPaddingPositionKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("roundingMode")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterRoundingModeKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("roundingIncrement")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterRoundingIncrementKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("minimumIntegerDigits")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterMinIntegerDigitsKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("maximumIntegerDigits")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterMaxIntegerDigitsKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("minimumFractionDigits")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterMinFractionDigitsKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("maximumFractionDigits")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterMaxFractionDigitsKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("currencyGroupingSeparator")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterCurrencyGroupingSeparatorKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("lenient")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterIsLenientKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("usesSignificantDigits")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterUseSignificantDigitsKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("minimumSignificantDigits")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterMinSignificantDigitsKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("maximumSignificantDigits")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterMaxSignificantDigitsKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("usesCharacterDirection")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterUsesCharacterDirectionKey, value);
    }

    value = [_attributes objectForKey:(id)OF_STR("formattingContext")];
    if (value != nil && [value intValue] != -1) {
        OCFNumberFormatterSetProperty(_formatter, OF_STR("kCFNumberFormatterFormattingContextKey"), value);
    }
    value = [_attributes objectForKey:(id)OF_STR("minimumGroupingDigits")];
    if (value != nil) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterMinGroupingDigitsKey, value);
    }
}

// Native -[NSNumberFormatter __Keynote_NOOP], 0x1c0577050. Non-external.
- (id)__Keynote_NOOP {
    return nil;
}

// Native -[NSNumberFormatter init], 0x1c0577058. Non-external.
- (id)init {
    if (objc_lookUpClass("ChartNumberFormatter") != Nil &&
        [(id)OF_STR("com.apple.Keynote") isEqual:[[ONSBundle mainBundle] bundleIdentifier]]) {
        Class cls = (Class)objc_opt_self((id)numberFormatterClass);
        IMP noOperation = class_getMethodImplementation(cls, sel_registerName("__Keynote_NOOP"));
        const char *const selectors[] = {
            "_getNegative", "_setNegativeFormat:", "_getPositive", "_setPositiveFormat:"
        };
        for (size_t i = 0; i < sizeof(selectors) / sizeof(selectors[0]); ++i) {
            cls = (Class)objc_opt_self((id)numberFormatterClass);
            Method method = class_getInstanceMethod(cls, sel_registerName(selectors[i]));
            if (method != NULL) {
                method_setImplementation(method, noOperation);
            }
        }
        const char *const setters[] = {"setNegativeFormat:", "setPositiveFormat:"};
        for (size_t i = 0; i < sizeof(setters) / sizeof(setters[0]); ++i) {
            cls = (Class)objc_opt_self((id)numberFormatterClass);
            IMP implementation = class_getMethodImplementation(cls, sel_registerName(setters[i]));
            Class chartClass = objc_lookUpClass("ChartNumberFormatter");
            Method method = class_getInstanceMethod(chartClass, sel_registerName(setters[i]));
            if (method != NULL) {
                method_setImplementation(method, implementation);
            }
        }
    }
    self = [super init];
    if (self != nil) {
        _lock = objc_alloc_init((Class)numberFormatterLockClass);
        _attributes = [objc_allocWithZone((Class)numberFormatterDictionaryClass) initWithCapacity:20];
        NSUInteger behavior = [(id)objc_opt_class(self) defaultFormatterBehavior];
        [_attributes setValue:[ONSNumber numberWithInteger:(NSInteger)behavior]
                       forKey:(id)OF_STR("formatterBehavior")];
        [self setNilSymbol:(id)OF_STR("")];
        const UniChar positiveInfinity[] = {'+', 0x221e};
        const UniChar negativeInfinity[] = {'-', 0x221e};
        [self setPositiveInfinitySymbol:[ONSString stringWithCharacters:positiveInfinity length:2]];
        [self setNegativeInfinitySymbol:[ONSString stringWithCharacters:negativeInfinity length:2]];
        [self setAllowsFloats:YES];
        [self _clearFormatter];
    }
    return self;
}

// Native -[NSNumberFormatter dealloc], 0x1c05774d0. Non-external.
- (void)dealloc {
    objc_release(_attributes);
    if (_formatter != NULL) {
        OCFRelease(_formatter);
    }
    objc_release(_lock);
    [super dealloc];
}

// Native -[NSNumberFormatter copyWithZone:], 0x1c0577570. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    [_lock lock];
    ONSNumberFormatter *copy = [[(id)objc_opt_class(self) allocWithZone:zone] init];
    [copy->_attributes setDictionary:_attributes];
    [_lock unlock];
    [copy _clearFormatter];
    return copy;
}

// Native -[NSNumberFormatter encodeWithCoder:], 0x1c05775e8. Non-external.
- (void)encodeWithCoder:(id)coder {
    [super encodeWithCoder:coder];
    if (![coder allowsKeyedCoding]) {
        objc_exception_throw([ONSException
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:(id)OF_STR("NSNumberFormatters cannot be encoded by non-keyed archivers")
                     userInfo:nil]);
    }
    [_lock lock];
    [coder encodeObject:_attributes forKey:(id)OF_STR("NS.attributes")];
    [_lock unlock];
}

// Native -[NSNumberFormatter initWithCoder:], 0x1c05776c8. Non-external.
- (id)initWithCoder:(id)coder {
    self = [super initWithCoder:coder];
    if (self == nil) {
        return nil;
    }
    _lock = objc_alloc_init((Class)numberFormatterLockClass);
    if (![coder allowsKeyedCoding]) {
        objc_release(self);
        objc_exception_throw([ONSException
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:(id)OF_STR("NSNumberFormatters cannot be decoded by non-keyed archivers")
                     userInfo:nil]);
    }
    _attributes = objc_retain([coder decodeObjectForKey:(id)OF_STR("NS.attributes")]);
    NSInteger behavior = [[_attributes objectForKey:(id)OF_STR("formatterBehavior")] integerValue];
    if (behavior != 1040) {
        if (behavior == 0) {
            behavior = (NSInteger)[(id)objc_opt_class(self) defaultFormatterBehavior];
        } else {
            behavior = 1040;
        }
        [_attributes setValue:[ONSNumber numberWithInteger:behavior] forKey:(id)OF_STR("formatterBehavior")];
    }
    _behavior = (NSUInteger)[[_attributes objectForKey:(id)OF_STR("formatterBehavior")] integerValue];
    [self _clearFormatter];
    return self;
}

// Native -[NSNumberFormatter stringForObjectValue:], 0x1c0577848. Non-external.
- (id)stringForObjectValue:(id)object {
    if (object == nil) {
        return [self nilSymbol];
    }
    if (!_ONSIsNSNumber(object)) {
        return nil;
    }
    [_lock lock];
    double value = [object doubleValue];
    id string = nil;
    switch (fpclassify(value)) {
        case FP_ZERO:
        case FP_SUBNORMAL:
            string = [self zeroSymbol];
            break;
        case FP_NAN:
            string = [self notANumberSymbol];
            break;
        case FP_INFINITE:
            string = signbit(value) ? [self negativeInfinitySymbol] : [self positiveInfinitySymbol];
            break;
    }
    if (string == nil) {
        if ((NSUInteger)_localeNotificationCount() != _counter) {
            [self _clearFormatter];
        }
        if (_formatter == NULL) {
            [self _regenerateFormatter];
        }
        if (_formatter != NULL) {
            string = objc_autorelease((id)OCFNumberFormatterCreateStringWithNumber(
                kOCFAllocatorSystemDefault, _formatter, (OCFNumberRef)object
            ));
        }
    }
    if ([[_attributes objectForKey:(id)OF_STR("formattingContext")] intValue] == -1) {
        id<NumberFormattingContextProxy> proxy = objc_alloc_init((Class)numberFormatterProxyClass);
        [proxy setString:string];
        [proxy setItem:object];
        ONSNumberFormatter *formatter = [objc_allocWithZone(objc_opt_class(self)) init];
        [formatter->_attributes setDictionary:_attributes];
        [proxy _retainFormatter:formatter];
        [formatter _clearFormatter];
        objc_release(formatter);
        string = objc_autorelease(proxy);
    }
    [_lock unlock];
    return string;
}

// Native -[NSNumberFormatter getObjectValue:forString:errorDescription:], 0x1c0577c28. Non-external.
// Foundation defect: the first-call IMP cache load and store at 0x1c0577c60–0x1c0577c8c race.
// Preserve publication of the first discovered IMP through an atomic compare-exchange.
- (BOOL)getObjectValue:(id *)object forString:(id)string errorDescription:(id *)description {
    SEL selector = @selector(getObjectValue:forString:range:error:);
    IMP base = atomic_load_explicit(&rangeParsingBaseImplementation, memory_order_relaxed);
    if (base == NULL) {
        Class cls = objc_lookUpClass(OF_NAME_STRING(ONSNumberFormatter));
        IMP implementation = class_getMethodImplementation(cls, selector);
        if (atomic_compare_exchange_strong_explicit(
                &rangeParsingBaseImplementation,
                &base,
                implementation,
                memory_order_relaxed,
                memory_order_relaxed
            )) {
            base = implementation;
        }
    }
    IMP current = class_getMethodImplementation(objc_opt_class(self), selector);
    id error = nil;
    id *errorOut = description != NULL ? &error : NULL;
    BOOL success;
    if (current == base) {
        success = getObjectValue_0(self, object, string, NULL, YES, errorOut);
    } else {
        success = [self getObjectValue:object forString:string range:NULL error:errorOut];
    }
    if (description != NULL) {
        *description = [error localizedDescription];
    }
    return success;
}

// Native -[NSNumberFormatter getObjectValue:forString:range:error:], 0x1c0577d34. Non-external.
- (BOOL)getObjectValue:(id *)object forString:(id)string range:(ONSRange *)range error:(id *)error {
    return getObjectValue_0(self, object, string, range, NO, error);
}

// Native -[NSNumberFormatter stringFromNumber:], 0x1c0577d48. Non-external.
- (id)stringFromNumber:(id)number {
    if (number == nil) {
        return nil;
    }
    return [self stringForObjectValue:number];
}

// Native -[NSNumberFormatter numberFromString:], 0x1c0577d58. Non-external.
- (id)numberFromString:(id)string {
    if (string == nil) {
        return nil;
    }
    id number = nil;
    return [self getObjectValue:&number forString:string errorDescription:NULL] ? number : nil;
}

// Native -[NSNumberFormatter setFormattingContext:], 0x1c0577f20. Non-external.
- (void)setFormattingContext:(NSInteger)context {
    int value;
    switch (context) {
        case 0:
            value = 256;
            break;
        case 1:
            value = -1;
            break;
        case 2:
            value = 260;
            break;
        case 3:
            value = 259;
            break;
        case 4:
            value = 258;
            break;
        case 5:
            value = 257;
            break;
        default:
            value = 0;
            break;
    }
    id number = [ONSNumber numberWithInt:value];
    [_lock lock];
    _cacheGeneration = (NSInteger)((NSUInteger)_cacheGeneration + 1);
    [_attributes setValue:number forKey:(id)OF_STR("formattingContext")];
    if (context != 1 && _formatter != NULL) {
        OCFNumberFormatterSetProperty(_formatter, OF_STR("kCFNumberFormatterFormattingContextKey"), number);
    }
    _stateBitMask |= 1;
    [_lock unlock];
}

// Native -[NSNumberFormatter locale], 0x1c05781a8. Non-external.
- (id)locale {
    [_lock lock];
    if ((NSUInteger)_localeNotificationCount() != _counter) {
        [self _clearFormatter];
    }
    id locale = [_attributes objectForKey:(id)OF_STR("locale")];
    if (locale == nil) {
        if (_formatter == NULL) {
            [self _regenerateFormatter];
        }
        locale = _formatter != NULL ? (id)OCFNumberFormatterGetLocale(_formatter) : [ONSLocale currentLocale];
    }
    objc_retain(locale);
    [_lock unlock];
    return objc_autorelease(locale);
}

// Native -[NSNumberFormatter setLocale:], 0x1c0578264. Non-external.
- (void)setLocale:(id)locale {
    [_lock lock];
    if (![[_attributes objectForKey:(id)OF_STR("locale")] isEqual:locale]) {
        [_attributes setValue:objc_autorelease([locale copyWithZone:NULL]) forKey:(id)OF_STR("locale")];
        [self _clearFormatter];
        _stateBitMask |= 5;
    }
    [_lock unlock];
}

// Native -[NSNumberFormatter generatesDecimalNumbers], 0x1c057830c. Non-external.
- (BOOL)generatesDecimalNumbers {
    [_lock lock];
    id value = [_attributes objectForKey:(id)OF_STR("generatesDecimalNumbers")];
    BOOL generates = value != nil ? [value boolValue] : NO;
    [_lock unlock];
    return generates;
}

// Native -[NSNumberFormatter allowsFloats], 0x1c0578b04. Non-external.
- (BOOL)allowsFloats {
    [_lock lock];
    BOOL allows = [[_attributes objectForKey:(id)OF_STR("allowsFloats")] boolValue];
    [_lock unlock];
    return allows;
}

// Native -[NSNumberFormatter setAllowsFloats:], 0x1c0578b64. Non-external.
- (void)setAllowsFloats:(BOOL)allowsFloats {
    [_lock lock];
    _cacheGeneration = (NSInteger)((NSUInteger)_cacheGeneration + 1);
    id value = (id)(allowsFloats ? kOCFBooleanTrue : kOCFBooleanFalse);
    [_attributes setValue:value forKey:(id)OF_STR("allowsFloats")];
    [_lock unlock];
}

// Native -[NSNumberFormatter decimalSeparator], 0x1c0578be4. Non-external.
- (id)decimalSeparator {
    [_lock lock];
    id value = [_attributes objectForKey:(id)OF_STR("decimalSeparator")];
    if (value == nil) {
        if ((NSUInteger)_localeNotificationCount() != _counter) {
            [self _clearFormatter];
        }
        OCFStringRef key = kOCFNumberFormatterDecimalSeparatorKey;
        if (_formatter == NULL) {
            [self _regenerateFormatter];
        }
        if (key != NULL && _formatter != NULL) {
            value = objc_autorelease((id)OCFNumberFormatterCopyProperty(_formatter, key));
        }
    }
    objc_retain(value);
    [_lock unlock];
    return objc_autorelease(value);
}

// Native -[NSNumberFormatter setDecimalSeparator:], 0x1c0578cc0. Non-external.
- (void)setDecimalSeparator:(id)separator {
    id value = objc_autorelease([separator copyWithZone:NULL]);
    [_lock lock];
    _cacheGeneration = (NSInteger)((NSUInteger)_cacheGeneration + 1);
    [_attributes setValue:value forKey:(id)OF_STR("decimalSeparator")];
    if (value != nil) {
        if (_formatter == NULL) {
            [self _regenerateFormatter];
        }
        if (_formatter != NULL) {
            OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterDecimalSeparatorKey, value);
        }
    } else {
        [self _clearFormatter];
    }
    _stateBitMask |= 1;
    [_lock unlock];
}

// Native -[NSNumberFormatter setUsesGroupingSeparator:], 0x1c05791e8. Non-external.
- (void)setUsesGroupingSeparator:(BOOL)grouping {
    id value = (id)(grouping ? kOCFBooleanTrue : kOCFBooleanFalse);
    [_lock lock];
    _cacheGeneration = (NSInteger)((NSUInteger)_cacheGeneration + 1);
    [_attributes setValue:value forKey:(id)OF_STR("usesGroupingSeparator")];
    if (_formatter != NULL) {
        OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterUseGroupingSeparatorKey, value);
    }
    _stateBitMask |= 1;
    [_lock unlock];
}

// Native -[NSNumberFormatter groupingSeparator], 0x1c057929c. Non-external.
- (id)groupingSeparator {
    [_lock lock];
    id value = [_attributes objectForKey:(id)OF_STR("groupingSeparator")];
    if (value == nil) {
        if ((NSUInteger)_localeNotificationCount() != _counter) {
            [self _clearFormatter];
        }
        OCFStringRef key = kOCFNumberFormatterGroupingSeparatorKey;
        if (_formatter == NULL) {
            [self _regenerateFormatter];
        }
        if (key != NULL && _formatter != NULL) {
            value = objc_autorelease((id)OCFNumberFormatterCopyProperty(_formatter, key));
        }
    }
    objc_retain(value);
    [_lock unlock];
    return objc_autorelease(value);
}

// Native -[NSNumberFormatter setGroupingSeparator:], 0x1c0579378. Non-external.
- (void)setGroupingSeparator:(id)separator {
    [_lock lock];
    _cacheGeneration = (NSInteger)((NSUInteger)_cacheGeneration + 1);
    id value = objc_autorelease([separator copyWithZone:NULL]);
    [_attributes setValue:value forKey:(id)OF_STR("groupingSeparator")];
    if (value != nil) {
        if (_formatter == NULL) {
            [self _regenerateFormatter];
        }
        if (_formatter != NULL) {
            OCFNumberFormatterSetProperty(_formatter, kOCFNumberFormatterGroupingSeparatorKey, value);
        }
    } else {
        [self _clearFormatter];
    }
    _stateBitMask |= 1;
    [_lock unlock];
}

// Native -[NSNumberFormatter zeroSymbol], 0x1c057944c. Non-external.
- (id)zeroSymbol {
    [_lock lock];
    id value = [_attributes objectForKey:(id)OF_STR("zeroSymbol")];
    if (value == nil) {
        if ((NSUInteger)_localeNotificationCount() != _counter) {
            [self _clearFormatter];
        }
        OCFStringRef key = kOCFNumberFormatterZeroSymbolKey;
        if (_formatter == NULL) {
            [self _regenerateFormatter];
        }
        if (key != NULL && _formatter != NULL) {
            value = objc_autorelease((id)OCFNumberFormatterCopyProperty(_formatter, key));
        }
    }
    objc_retain(value);
    [_lock unlock];
    return objc_autorelease(value);
}

// Native -[NSNumberFormatter nilSymbol], 0x1c057979c. Non-external.
- (id)nilSymbol {
    [_lock lock];
    id value = [_attributes objectForKey:(id)OF_STR("nilSymbol")];
    if (value == nil) {
        if ((NSUInteger)_localeNotificationCount() != _counter) {
            [self _clearFormatter];
        }
        if (_formatter == NULL) {
            [self _regenerateFormatter];
        }
    }
    objc_retain(value);
    [_lock unlock];
    return objc_autorelease(value);
}

// Native -[NSNumberFormatter setNilSymbol:], 0x1c0579838. Non-external.
- (void)setNilSymbol:(id)symbol {
    [_lock lock];
    id previous = [_attributes objectForKey:(id)OF_STR("nilSymbol")];
    if ((symbol != nil) == (previous == nil) || ![previous isEqual:symbol]) {
        id value = objc_autorelease([symbol copyWithZone:NULL]);
        [_attributes setValue:value forKey:(id)OF_STR("nilSymbol")];
        _cacheGeneration = (NSInteger)((NSUInteger)_cacheGeneration + 1);
        _stateBitMask |= 1;
    }
    [_lock unlock];
}

// Native -[NSNumberFormatter notANumberSymbol], 0x1c0579a64. Non-external.
- (id)notANumberSymbol {
    [_lock lock];
    id value = [_attributes objectForKey:(id)OF_STR("notANumberSymbol")];
    if (value == nil) {
        if ((NSUInteger)_localeNotificationCount() != _counter) {
            [self _clearFormatter];
        }
        OCFStringRef key = kOCFNumberFormatterNaNSymbolKey;
        if (_formatter == NULL) {
            [self _regenerateFormatter];
        }
        if (key != NULL && _formatter != NULL) {
            value = objc_autorelease((id)OCFNumberFormatterCopyProperty(_formatter, key));
        }
    }
    objc_retain(value);
    [_lock unlock];
    return objc_autorelease(value);
}

// Native -[NSNumberFormatter positiveInfinitySymbol], 0x1c0579db4. Non-external.
- (id)positiveInfinitySymbol {
    [_lock lock];
    id value = [_attributes objectForKey:(id)OF_STR("positiveInfinitySymbol")];
    if (value == nil) {
        if ((NSUInteger)_localeNotificationCount() != _counter) {
            [self _clearFormatter];
        }
        if (_formatter == NULL) {
            [self _regenerateFormatter];
        }
    }
    objc_retain(value);
    [_lock unlock];
    return objc_autorelease(value);
}

// Native -[NSNumberFormatter setPositiveInfinitySymbol:], 0x1c0579e50. Non-external.
- (void)setPositiveInfinitySymbol:(id)symbol {
    [_lock lock];
    id previous = [_attributes objectForKey:(id)OF_STR("positiveInfinitySymbol")];
    if ((symbol != nil) == (previous == nil) || ![previous isEqual:symbol]) {
        id value = objc_autorelease([symbol copyWithZone:NULL]);
        [_attributes setValue:value forKey:(id)OF_STR("positiveInfinitySymbol")];
        _cacheGeneration = (NSInteger)((NSUInteger)_cacheGeneration + 1);
        _stateBitMask |= 1;
    }
    [_lock unlock];
}

// Native -[NSNumberFormatter negativeInfinitySymbol], 0x1c057a07c. Non-external.
- (id)negativeInfinitySymbol {
    [_lock lock];
    id value = [_attributes objectForKey:(id)OF_STR("negativeInfinitySymbol")];
    if (value == nil) {
        if ((NSUInteger)_localeNotificationCount() != _counter) {
            [self _clearFormatter];
        }
        if (_formatter == NULL) {
            [self _regenerateFormatter];
        }
    }
    objc_retain(value);
    [_lock unlock];
    return objc_autorelease(value);
}

// Native -[NSNumberFormatter setNegativeInfinitySymbol:], 0x1c057a118. Non-external.
- (void)setNegativeInfinitySymbol:(id)symbol {
    [_lock lock];
    id previous = [_attributes objectForKey:(id)OF_STR("negativeInfinitySymbol")];
    if ((symbol != nil) == (previous == nil) || ![previous isEqual:symbol]) {
        id value = objc_autorelease([symbol copyWithZone:NULL]);
        [_attributes setValue:value forKey:(id)OF_STR("negativeInfinitySymbol")];
        _cacheGeneration = (NSInteger)((NSUInteger)_cacheGeneration + 1);
        _stateBitMask |= 1;
    }
    [_lock unlock];
}

// Native -[NSNumberFormatter currencyCode], 0x1c057aaf4. Non-external. The set value; otherwise the
// formatter's kCFNumberFormatterCurrencyCodeKey property (the formatter regenerated when the locale changed
// or it is missing), nil without a formatter.
- (id)currencyCode {
    [_lock lock];
    id code = [_attributes objectForKey:(id)OF_STR("currencyCode")];
    if (code == nil) {
        if ((NSUInteger)_localeNotificationCount() != _counter) {
            [self _clearFormatter];
        }
        OCFStringRef key = kOCFNumberFormatterCurrencyCodeKey;
        if (_formatter == NULL) {
            [self _regenerateFormatter];
        }
        code = (id)key;
        if (key != NULL) {
            code = _formatter != NULL ? objc_autorelease((id)OCFNumberFormatterCopyProperty(_formatter, key))
                                      : nil;
        }
    }
    objc_retain(code);
    [_lock unlock];
    return objc_autorelease(code);
}

// Native -[NSNumberFormatter currencySymbol], 0x1c057ad44. Non-external. As -currencyCode, with
// kCFNumberFormatterCurrencySymbolKey.
- (id)currencySymbol {
    [_lock lock];
    id symbol = [_attributes objectForKey:(id)OF_STR("currencySymbol")];
    if (symbol == nil) {
        if ((NSUInteger)_localeNotificationCount() != _counter) {
            [self _clearFormatter];
        }
        OCFStringRef key = kOCFNumberFormatterCurrencySymbolKey;
        if (_formatter == NULL) {
            [self _regenerateFormatter];
        }
        symbol = (id)key;
        if (key != NULL) {
            symbol = _formatter != NULL
                         ? objc_autorelease((id)OCFNumberFormatterCopyProperty(_formatter, key))
                         : nil;
        }
    }
    objc_retain(symbol);
    [_lock unlock];
    return objc_autorelease(symbol);
}

// Native -[NSNumberFormatter roundingMode], 0x1c057c608. Non-external.
- (NSUInteger)roundingMode {
    [_lock lock];
    id value = [_attributes objectForKey:(id)OF_STR("roundingMode")];
    if (value == nil) {
        if ((NSUInteger)_localeNotificationCount() != _counter) {
            [self _clearFormatter];
        }
        OCFStringRef key = kOCFNumberFormatterRoundingModeKey;
        if (_formatter == NULL) {
            [self _regenerateFormatter];
        }
        if (key != NULL && _formatter != NULL) {
            value = objc_autorelease((id)OCFNumberFormatterCopyProperty(_formatter, key));
        }
    }
    NSUInteger mode = (NSUInteger)[value integerValue];
    [_lock unlock];
    return mode;
}

// Native -[NSNumberFormatter setMinimumFractionDigits:], 0x1c057d004. Non-external.
- (void)setMinimumFractionDigits:(NSUInteger)digits {
    [_lock lock];
    id previous = [_attributes objectForKey:(id)OF_STR("minimumFractionDigits")];
    if (previous == nil || (NSUInteger)[previous integerValue] != digits) {
        OCFStringRef key = kOCFNumberFormatterMinFractionDigitsKey;
        id value = [ONSNumber numberWithInteger:(NSInteger)digits];
        [_attributes setValue:value forKey:(id)OF_STR("minimumFractionDigits")];
        _cacheGeneration = (NSInteger)((NSUInteger)_cacheGeneration + 1);
        if (key != NULL && _formatter != NULL) {
            OCFNumberFormatterSetProperty(_formatter, key, value);
        }
    }

    OCFStringRef key = kOCFNumberFormatterMaxFractionDigitsKey;
    id maximum = [_attributes objectForKey:(id)OF_STR("maximumFractionDigits")];
    if (maximum == nil) {
        if ((NSUInteger)_localeNotificationCount() != _counter) {
            [self _clearFormatter];
        }
        if (_formatter == NULL) {
            [self _regenerateFormatter];
        }
        if (key != NULL && _formatter != NULL) {
            maximum = objc_autorelease((id)OCFNumberFormatterCopyProperty(_formatter, key));
        }
    }
    if (digits > (NSUInteger)[maximum integerValue]) {
        previous = [_attributes objectForKey:(id)OF_STR("maximumFractionDigits")];
        if (previous == nil || (NSUInteger)[previous integerValue] != digits) {
            id value = [ONSNumber numberWithInteger:(NSInteger)digits];
            [_attributes setValue:value forKey:(id)OF_STR("maximumFractionDigits")];
            _cacheGeneration = (NSInteger)((NSUInteger)_cacheGeneration + 1);
            if (key != NULL && _formatter != NULL) {
                OCFNumberFormatterSetProperty(_formatter, key, value);
            }
        }
    }
    [_lock unlock];
}

// Native -[NSNumberFormatter minimum], 0x1c057d4b8. Non-external.
- (id)minimum {
    [_lock lock];
    id value = [_attributes objectForKey:(id)OF_STR("minimum")];
    objc_retain(value);
    [_lock unlock];
    return objc_autorelease(value);
}

// Native -[NSNumberFormatter maximum], 0x1c057d600. Non-external.
- (id)maximum {
    [_lock lock];
    id value = [_attributes objectForKey:(id)OF_STR("maximum")];
    objc_retain(value);
    [_lock unlock];
    return objc_autorelease(value);
}

@end

// Native getObjectValue_0, 0x1c0fdde2c. Non-external.
static BOOL getObjectValue_0(
    ONSNumberFormatter *self, id *object, ONSString *string, ONSRange *range, BOOL checkTail, id *error
) {
    [self->_lock lock];
    if ([string isEqualToString:[self nilSymbol]]) {
        if (object != NULL) {
            *object = nil;
        }
        [self->_lock unlock];
        return YES;
    }
    if ([string isEqualToString:[self negativeInfinitySymbol]]) {
        if (object != NULL) {
            *object = [self generatesDecimalNumbers] ? [ONSDecimalNumber minimumDecimalNumber]
                                                     : (id)kOCFNumberNegativeInfinity;
        }
        [self->_lock unlock];
        return YES;
    }
    if ([string isEqualToString:[self positiveInfinitySymbol]]) {
        if (object != NULL) {
            *object = [self generatesDecimalNumbers] ? [ONSDecimalNumber maximumDecimalNumber]
                                                     : (id)kOCFNumberPositiveInfinity;
        }
        [self->_lock unlock];
        return YES;
    }

    ONSString *decimal = [self decimalSeparator];
    ONSString *grouping = [self groupingSeparator];
    if (([grouping length] != 0 && [grouping characterAtIndex:0] == 160) ||
        ([decimal length] != 0 && [decimal characterAtIndex:0] == 160)) {
        id digits = [ONSCharacterSet characterSetWithCharactersInString:(id)OF_STR("0123456789")];
        ONSRange first = [string rangeOfCharacterFromSet:digits options:2];
        ONSRange last = [string rangeOfCharacterFromSet:digits options:6];
        if (first.location != (NSUInteger)INT64_MAX && last.location != (NSUInteger)INT64_MAX) {
            UniChar space = 160;
            id replacement = [ONSString stringWithCharacters:&space length:1];
            ONSRange span = {first.location, last.location - first.location + 1};
            string = [string stringByReplacingOccurrencesOfString:(id)OF_STR(" ")
                                                       withString:replacement
                                                          options:2
                                                            range:span];
        }
    }

    if ((NSUInteger)_localeNotificationCount() != self->_counter) {
        [self _clearFormatter];
    }
    BOOL skipTailCheck = !checkTail || !_OCFExecutableLinkedOnOrAfter(6);
    NSUInteger length = [string length];
    ONSRange input = range != NULL ? *range : (ONSRange){0, length};
    OCFRange parsed = {(OCFIndex)input.location, (OCFIndex)input.length};

    if (self->_formatter == NULL) {
        [self _regenerateFormatter];
    }
    id number = nil;
    if (self->_formatter != NULL) {
        number = (id)OCFNumberFormatterCreateNumberFromString(
            kOCFAllocatorSystemDefault, self->_formatter, (OCFStringRef)string, &parsed, ![self allowsFloats]
        );
    }
    objc_autorelease(number);
    id tail = [string substringFromIndex:(NSUInteger)parsed.location + (NSUInteger)parsed.length];
    BOOL valid = number != nil;
    if (valid && !skipTailCheck) {
        ONSCharacterSet *whitespace = [ONSCharacterSet whitespaceCharacterSet];
        valid = [whitespace isSupersetOfSet:[ONSCharacterSet characterSetWithCharactersInString:tail]];
    }
    if (!valid && self->_formatter != NULL) {
        OCFStringRef format = OCFNumberFormatterGetFormat(self->_formatter);
        if (format != NULL) {
            OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
            OCFMutableStringRef filtered = OCFStringCreateMutable(allocator, 0);
            OCFIndex count = OCFStringGetLength(format);
            BOOL quoted = NO;
            for (OCFIndex i = 0; i < count; ++i) {
                UniChar ch = OCFStringGetCharacterAtIndex(format, i);
                unsigned int bit = (unsigned int)ch - 35;
                BOOL keep = (bit <= 0x1d && ((UINT32_C(1) << bit) & UINT32_C(0x207fed05)) != 0) ||
                            ch == 164 || ch == 8240;
                if (keep) {
                    if (!quoted) {
                        OCFStringAppendCharacters(filtered, &ch, 1);
                    }
                } else if (ch == 39) {
                    quoted = !quoted;
                }
            }
            if (!OCFEqual(filtered, format)) {
                OCFRetain(format);
                OCFNumberFormatterSetFormat(self->_formatter, filtered);
                input = range != NULL ? *range : (ONSRange){0, length};
                parsed = (OCFRange){(OCFIndex)input.location, (OCFIndex)input.length};
                number = (id)OCFNumberFormatterCreateNumberFromString(
                    allocator, self->_formatter, (OCFStringRef)string, &parsed, ![self allowsFloats]
                );
                objc_autorelease(number);
                tail = [string substringFromIndex:(NSUInteger)parsed.location + (NSUInteger)parsed.length];
                valid = number != nil;
                if (valid && !skipTailCheck) {
                    ONSCharacterSet *whitespace = [ONSCharacterSet whitespaceCharacterSet];
                    valid = [whitespace
                        isSupersetOfSet:[ONSCharacterSet characterSetWithCharactersInString:tail]];
                }
                OCFNumberFormatterSetFormat(self->_formatter, format);
                OCFRelease(format);
            }
            OCFRelease(filtered);
        }
    }

    if (!valid) {
        if (error != NULL) {
            id info = [ONSDictionary dictionaryWithObjectsAndKeys:string, (id)OF_STR("NSInvalidValue"), nil];
            *error = [ONSError errorWithDomain:ONSCocoaErrorDomain code:2048 userInfo:info];
        }
        [self->_lock unlock];
        return NO;
    }
    if (range != NULL) {
        *range = (ONSRange){(NSUInteger)parsed.location, (NSUInteger)parsed.length};
    }

    id bound = [self minimum];
    id boundKey = nil;
    if (bound != nil && [number compare:bound] == -1) {
        boundKey = ONSMinimumValidValueErrorKey;
    } else {
        bound = [self maximum];
        if (bound != nil && [number compare:bound] == 1) {
            boundKey = ONSMaximumValidValueErrorKey;
        }
    }
    if (boundKey != nil) {
        if (error != NULL) {
            id info = [ONSDictionary
                dictionaryWithObjectsAndKeys:bound, boundKey, number, (id)OF_STR("NSInvalidValue"), nil];
            *error = [ONSError errorWithDomain:ONSCocoaErrorDomain code:2048 userInfo:info];
        }
        [self->_lock unlock];
        return NO;
    }

    if (object != NULL) {
        if ([self generatesDecimalNumbers]) {
            ONSDecimal decimal = {0};
            if (number != nil) {
                decimal = [number decimalValue];
            }
            *object = [ONSDecimalNumber decimalNumberWithDecimal:decimal];
        } else {
            *object = number;
        }
    }
    [self->_lock unlock];
    return YES;
}

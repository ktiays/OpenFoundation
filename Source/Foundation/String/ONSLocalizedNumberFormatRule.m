//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSLocalizedNumberFormatRule.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/String/OCFString.h"

#include <dispatch/dispatch.h>
#include <os/lock.h>

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified except shared-cache GOT bindings (medium). iOS Device divergence: iOS Device 24A446
// remains unread. Reading: Local/Scratch/cf-core-r2-fg/Reading.md. The native retains/releases use MRC.

@implementation ONSLocalizedNumberFormatRule

// Native -[NSLocalizedNumberFormatRule _init], 0x1c0613be4. Non-external.
- (instancetype)_init {
    return [super init];
}

// Native +[NSLocalizedNumberFormatRule automatic], 0x1c0613c44. Non-external.
+ (id)automatic {
    static dispatch_once_t once;
    static _ONSLocalizedNumberFormatRuleAutomatic *rule;
    // Native ___40+[NSLocalizedNumberFormatRule automatic]_block_invoke, 0x1c0613c84. Non-external.
    dispatch_once(&once, ^{
      rule = [[_ONSLocalizedNumberFormatRuleAutomatic alloc] _init];
    });
    return rule;
}

// Native -[NSLocalizedNumberFormatRule copyWithZone:], 0x1c0fe7778. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    ONSRequestConcreteImplementation(self, _cmd, objc_opt_class(self));
    return nil;
}

// Native -[NSLocalizedNumberFormatRule encodeWithCoder:], 0x1c0fe77b0. Non-external.
- (void)encodeWithCoder:(id)coder {
    ONSRequestConcreteImplementation(self, _cmd, objc_opt_class(self));
}

// Native -[NSLocalizedNumberFormatRule initWithCoder:], 0x1c0fe77e0. Non-external.
- (instancetype)initWithCoder:(id)coder {
    ONSRequestConcreteImplementation(self, _cmd, objc_opt_class(self));
    objc_release(self);
    return nil;
}

// Native +[NSLocalizedNumberFormatRule supportsSecureCoding], 0x1c0fe781c. Non-external.
+ (BOOL)supportsSecureCoding {
    ONSRequestConcreteImplementation(self, _cmd, objc_opt_class(self));
    return NO;
}

// Native -[NSLocalizedNumberFormatRule _formattedString:withStringLocale:andDisplayLocale:],
// 0x1c0fe7854. Non-external.
- (id)_formattedString:(id)string withStringLocale:(id)stringLocale andDisplayLocale:(id)displayLocale {
    ONSRequestConcreteImplementation(self, _cmd, objc_opt_class(self));
    return nil;
}

@end

// The formatter-cache key callbacks: the keys are locales, equal when they and their decimal and grouping
// separators are equal.

// Native FormatterCacheKeyCopy, 0x1c0614040. Non-external. X1 holds the dictionary key.
static const void *FormatterCacheKeyCopy(OCFAllocatorRef allocator, const void *key) {
    (void)allocator;
    return [(id)key copy];
}

// Native FormatterCacheKeyRelease, 0x1c0614048. Non-external. X1 holds the dictionary key.
// Restored by FD-decimal-number-formatting; one of the formatter-cache callbacks of this unit.
static void FormatterCacheKeyRelease(OCFAllocatorRef allocator, const void *key) {
    (void)allocator;
    objc_release((id)key);
}

// Native FormatterCacheKeyEqual, 0x1c061404c. Non-external.
static Boolean FormatterCacheKeyEqual(const void *first, const void *second) {
    if (![(id)first isEqual:(id)second]) {
        return false;
    }
    if (![[(id)first decimalSeparator] isEqual:[(id)second decimalSeparator]]) {
        return false;
    }
    return [[(id)first groupingSeparator] isEqual:[(id)second groupingSeparator]];
}

// Native FormatterCacheKeyHash, 0x1c06140c4. Non-external.
static OCFHashCode FormatterCacheKeyHash(const void *key) {
    NSUInteger hash = [(id)key hash] ^ [[(id)key decimalSeparator] hash];
    return hash ^ [[(id)key groupingSeparator] hash];
}

@implementation _ONSLocalizedNumberFormatRuleAutomatic

// Native -[_NSLocalizedNumberFormatRuleAutomatic copyWithZone:], 0x1c0613cac. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return objc_retain(self);
}

// Native -[_NSLocalizedNumberFormatRuleAutomatic encodeWithCoder:], 0x1c0613cb8. Non-external. Body is RET.
- (void)encodeWithCoder:(id)coder {
}

// Native -[_NSLocalizedNumberFormatRuleAutomatic initWithCoder:], 0x1c0613cbc. Non-external.
- (instancetype)initWithCoder:(id)coder {
    objc_release(self);
    return objc_retain([ONSLocalizedNumberFormatRule automatic]);
}

// Native -[_NSLocalizedNumberFormatRuleAutomatic description], 0x1c0613cdc. Non-external.
- (id)description {
    return [ONSString stringWithFormat:(id)OF_STR("%@ (format number rule)"), [super description]];
}

// Native -[_NSLocalizedNumberFormatRuleAutomatic _locked_formatterForLocale:], 0x1c0613d60. Non-external.
- (ONSNumberFormatter *)_locked_formatterForLocale:(id)locale {
    static OCFMutableDictionaryRef formatters;
    static const OCFDictionaryKeyCallBacks callbacks = {
        0,
        FormatterCacheKeyCopy,
        FormatterCacheKeyRelease,
        NULL,
        FormatterCacheKeyEqual,
        FormatterCacheKeyHash
    };
    if (!formatters) {
        formatters = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, &callbacks, &kOCFTypeDictionaryValueCallBacks
        );
    }

    ONSNumberFormatter *formatter = (ONSNumberFormatter *)OCFDictionaryGetValue(formatters, locale);
    if (!formatter) {
        formatter = objc_autorelease([[ONSNumberFormatter alloc] init]);
        [formatter setLocale:locale];
        [formatter setDecimalSeparator:[locale decimalSeparator]];
        [formatter setGroupingSeparator:[locale groupingSeparator]];
        if (OCFDictionaryGetCount(formatters) >= 101) {
            OCFDictionaryRemoveAllValues(formatters);
        }
        OCFDictionarySetValue(formatters, locale, formatter);
    }
    return formatter;
}

// Native -[_NSLocalizedNumberFormatRuleAutomatic _formattedString:withStringLocale:andDisplayLocale:],
// 0x1c0613e38. Non-external.
- (id)_formattedString:(ONSString *)string
      withStringLocale:(id)stringLocale
      andDisplayLocale:(id)displayLocale {
    static os_unfair_lock lock = OS_UNFAIR_LOCK_INIT;
    os_unfair_lock_lock(&lock);

    ONSNumberFormatter *parser = [self _locked_formatterForLocale:stringLocale];
    id separator = [stringLocale groupingSeparator];
    BOOL groups = separator ? [string containsString:separator] : NO;
    [parser setUsesGroupingSeparator:groups];
    id number = [parser numberFromString:string];
    if (!number) {
        os_unfair_lock_unlock(&lock);
        return nil;
    }

    __block NSUInteger fractionDigits = 0;
    ONSRange decimalRange = [string rangeOfString:[stringLocale decimalSeparator]];
    if (decimalRange.location != NSIntegerMax) {
        NSUInteger start = decimalRange.location + decimalRange.length;
        ONSRange fractionRange = {start, [string length] - start};
        // Native ___92-[_NSLocalizedNumberFormatRuleAutomatic
        // _formattedString:withStringLocale:andDisplayLocale:]_block_invoke, 0x1c0614028. Non-external.
        [string enumerateSubstringsInRange:fractionRange
                                   options:2
                                usingBlock:^(
                                    __unused id substring,
                                    __unused ONSRange substringRange,
                                    __unused ONSRange enclosingRange,
                                    __unused BOOL *stop
                                ) {
                                  ++fractionDigits;
                                }];
    }

    ONSNumberFormatter *formatter = [self _locked_formatterForLocale:displayLocale];
    [formatter setMinimumFractionDigits:fractionDigits];
    [formatter setUsesGroupingSeparator:groups];
    id result = [formatter stringFromNumber:number];
    os_unfair_lock_unlock(&lock);
    return result;
}

@end

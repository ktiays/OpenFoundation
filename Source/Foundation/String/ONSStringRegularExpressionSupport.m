//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSStringRegularExpressionSupport.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"

#include <dispatch/dispatch.h>

// Inferred organization: the NSRegularExpressionSupport categories share this unit and their native cache.
// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified
// except the NSCache GOT binding (0x22227FAA8, identified by its selectors; confidence: medium).
// iOS Device divergence: all definitions in this unit remain unread on iOS Device 24A446.
// Reading: Foundation/String/ONSStringReview.md; original IDA capture iphone-simulator-foundation-link.json;
// initializer: Local/Scratch/cf-core-fd-string-review/supplemental-ida.json.

// Native __MergedGlobals_137 (0x22e76d158) and qword_22E76D160 (0x22e76d160), __bss: once token and cache.
static dispatch_once_t regexCacheOnce;
static id regexCache;

// Shared spelling of the native inlined once path. The block owns no captured objects.
static inline void initializeRegexCache(void) {
    // Native ____createRegexForPattern_block_invoke, 0x1c063fe88. Non-external. The +1 cache is kept alive.
    dispatch_once(&regexCacheOnce, ^{
      regexCache = [[ONSCache alloc] init];
      [regexCache setName:(id)OF_STR("NSRegularExpressionCache")];
      [regexCache setCountLimit:10];
    });
}

@implementation ONSString (NSRegularExpressionSupport)

// Native -[NSString(NSRegularExpressionSupport) _rangeOfRegularExpressionPattern:options:range:locale:],
// 0x1c063fb14. Non-external. The locale is not read. Compilation uses options & 1; matching uses (options >>
// 1) & 4. The range result is returned in X0/X1, including {NSNotFound, 0} on an invalid expression.
- (ONSRange)_rangeOfRegularExpressionPattern:(id)pattern
                                     options:(ONSUInteger)options
                                       range:(ONSRange)range
                                      locale:(id)locale {
    (void)locale;
    ONSUInteger compileOptions = options & 1;
    initializeRegexCache();
    id key = [ONSString stringWithFormat:(id)OF_STR("%lx:%@"), compileOptions, pattern];
    id expression = [[regexCache objectForKey:key] retain];
    if (expression == nil) {
        expression = [[ONSRegularExpression alloc] initWithPattern:pattern options:compileOptions error:NULL];
        if (expression == nil) {
            return (ONSRange){LONG_MAX, 0};
        }
        [regexCache setObject:expression forKey:key];
    }
    ONSRange result = [expression rangeOfFirstMatchInString:self options:(options >> 1) & 4 range:range];
    [expression release];
    return result;
}

// Native -[NSString(NSRegularExpressionSupport)
// _stringByReplacingOccurrencesOfRegularExpressionPattern:withTemplate:options:range:], 0x1c063fc3c.
// Non-external. A compilation failure returns self without retaining or autoreleasing it.
- (id)_stringByReplacingOccurrencesOfRegularExpressionPattern:(id)pattern
                                                 withTemplate:(id)replacementTemplate
                                                      options:(ONSUInteger)options
                                                        range:(ONSRange)range {
    ONSUInteger compileOptions = options & 1;
    initializeRegexCache();
    id key = [ONSString stringWithFormat:(id)OF_STR("%lx:%@"), compileOptions, pattern];
    id expression = [[regexCache objectForKey:key] retain];
    if (expression == nil) {
        expression = [[ONSRegularExpression alloc] initWithPattern:pattern options:compileOptions error:NULL];
        if (expression == nil) {
            return self;
        }
        [regexCache setObject:expression forKey:key];
    }
    id result = [expression stringByReplacingMatchesInString:self
                                                     options:(options >> 1) & 4
                                                       range:range
                                                withTemplate:replacementTemplate];
    [expression release];
    return result;
}

@end

@implementation ONSMutableString (NSRegularExpressionSupport)

// Native -[NSMutableString(NSRegularExpressionSupport)
// _replaceOccurrencesOfRegularExpressionPattern:withTemplate:options:range:], 0x1c063fd6c. Non-external.
// A failed compilation returns zero; the count is saved before releasing the expression.
- (ONSUInteger)_replaceOccurrencesOfRegularExpressionPattern:(id)pattern
                                                withTemplate:(id)replacementTemplate
                                                     options:(ONSUInteger)options
                                                       range:(ONSRange)range {
    ONSUInteger compileOptions = options & 1;
    initializeRegexCache();
    id key = [ONSString stringWithFormat:(id)OF_STR("%lx:%@"), compileOptions, pattern];
    id expression = [[regexCache objectForKey:key] retain];
    if (expression == nil) {
        expression = [[ONSRegularExpression alloc] initWithPattern:pattern options:compileOptions error:NULL];
        if (expression == nil) {
            return 0;
        }
        [regexCache setObject:expression forKey:key];
    }
    ONSUInteger result = [expression replaceMatchesInString:self
                                                    options:(options >> 1) & 4
                                                      range:range
                                               withTemplate:replacementTemplate];
    [expression release];
    return result;
}

@end

//
//  Created by ktiays on 2026/11/05.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSRegularExpression.h"
#import "Foundation/String/ONSLog.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSTextCheckingResult.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/String/OCFStringUText.h"

#include <malloc/malloc.h>
#include <objc/objc-exception.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: the NSRegularExpression unit spans 0x1c0650f0c-0x1c0653e7b of the iOS Simulator
// Foundation __text section: the class methods, escapedString, the NSMatching and NSReplacement categories
// with their block invokes, and the ICU callbacks regexMatchCallback and regexFindProgressCallback.
// NSDataDetector, which follows the categories, lies outside the scope. Objective-C compiled with manual
// reference counting. Unless a definition says otherwise: iOS Simulator 24A434, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full). iOS Device 24A446:
// unread.
//
// Shared-cache bindings, identified from their use (confidence: medium): 0x22227FAE0 the NSException class,
// 0x22227F500 NSInvalidArgumentException, 0x22227F5E0 NSRangeException, 0x22227FAD0 the NSDictionary class
// (+dictionaryWithObjectsAndKeys:), 0x22227FB18 the NSMutableArray class (+array, objc_alloc_init).

extern id objc_retain(id object);
extern void objc_release(id object);
extern id objc_autorelease(id object);
extern Class objc_opt_class(id object);

extern char regexStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSString));
extern char regexMutableStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableString));
extern char regexCharacterSetClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSCharacterSet));
extern char regexErrorClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSError));
extern char regexExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char regexDictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDictionary));
extern char regexMutableArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableArray));
extern char regexValueClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSValue));

// libicucore (supplied; the SDK installs no ICU headers). URegularExpression is opaque; UParseError is ICU's
// 72-byte parse error (line, offset, 16 characters of pre- and post-context).
typedef struct URegularExpression URegularExpression;

typedef struct {
    int32_t line;
    int32_t offset;
    uint16_t preContext[16];
    uint16_t postContext[16];
} RegexParseError;

typedef int8_t (*RegexMatchCallback)(const void *context, int32_t steps);
typedef int8_t (*RegexFindProgressCallback)(const void *context, int64_t matchIndex);

extern URegularExpression *
uregex_open(const uint16_t *pattern, int32_t length, uint32_t flags, RegexParseError *error, int32_t *status);
extern void uregex_close(URegularExpression *regex);
extern URegularExpression *uregex_clone(const URegularExpression *regex, int32_t *status);
extern int32_t uregex_groupCount(URegularExpression *regex, int32_t *status);
extern void uregex_setText(URegularExpression *regex, const uint16_t *text, int32_t length, int32_t *status);
extern void uregex_setUText(URegularExpression *regex, ProviderUText *text, int32_t *status);
extern void uregex_setRegion64(URegularExpression *regex, int64_t start, int64_t limit, int32_t *status);
extern void uregex_setMatchCallback(
    URegularExpression *regex, RegexMatchCallback callback, const void *context, int32_t *status
);
extern void uregex_setFindProgressCallback(
    URegularExpression *regex, RegexFindProgressCallback callback, const void *context, int32_t *status
);
extern void uregex_useTransparentBounds(URegularExpression *regex, int8_t value, int32_t *status);
extern void uregex_useAnchoringBounds(URegularExpression *regex, int8_t value, int32_t *status);
extern int8_t uregex_findNext(URegularExpression *regex, int32_t *status);
extern int64_t uregex_start64(URegularExpression *regex, int32_t group, int32_t *status);
extern int64_t uregex_end64(URegularExpression *regex, int32_t group, int32_t *status);
extern int8_t uregex_hitEnd(URegularExpression *regex, int32_t *status);
extern int8_t uregex_requireEnd(URegularExpression *regex, int32_t *status);
extern void utext_close(ProviderUText *text);

// The messages the unit sends to objects of other classes, declared for the sends only.
@protocol RegularExpressionMessages
+ (id)stringWithFormat:(id)format, ...;
+ (id)stringWithString:(id)string;
+ (id)string;
+ (id)characterSetWithCharactersInString:(id)string;
+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
+ (id)errorWithDomain:(id)domain code:(NSInteger)code userInfo:(id)userInfo;
+ (id)dictionaryWithObjectsAndKeys:(id)firstObject, ...;
+ (id)array;
+ (id)valueWithRange:(ONSRange)range;
+ (void)setVersion:(NSInteger)version;
- (NSUInteger)length;
- (void)getCharacters:(uint16_t *)buffer range:(ONSRange)range;
- (uint16_t)characterAtIndex:(NSUInteger)index;
- (id)substringWithRange:(ONSRange)range;
- (ONSRange)rangeOfCharacterFromSet:(id)set;
- (ONSRange)rangeOfCharacterFromSet:(id)set options:(NSUInteger)options range:(ONSRange)range;
- (void)insertString:(id)string atIndex:(NSUInteger)index;
- (void)deleteCharactersInRange:(ONSRange)range;
- (void)replaceCharactersInRange:(ONSRange)range withString:(id)string;
- (void)appendString:(id)string;
- (void)addObject:(id)object;
- (BOOL)allowsKeyedCoding;
- (void)encodeObject:(id)object forKey:(id)key;
- (void)encodeInt64:(int64_t)value forKey:(id)key;
- (void)encodeObject:(id)object;
- (void)encodeValueOfObjCType:(const char *)type at:(const void *)address;
- (id)decodeObjectOfClass:(Class)cls forKey:(id)key;
- (int64_t)decodeInt64ForKey:(id)key;
- (NSInteger)versionForClassName:(id)className;
- (id)decodeObject;
- (void)decodeValueOfObjCType:(const char *)type at:(void *)address size:(NSUInteger)size;
- (ONSRange)range;
- (NSUInteger)numberOfRanges;
- (ONSRange)rangeAtIndex:(NSUInteger)index;
- (id)copy;
@end

// NSNotFound.
static const NSUInteger kNotFound = 0x7FFFFFFFFFFFFFFF;

// The context the enumeration hands its ICU callbacks: the block, the anchored limit (the end of the previous
// match, or NSNotFound), and two flags set by the callbacks: the block asked to stop, and a find passed the
// limit. The struct and member names are local.
typedef struct {
    void (^block)(id result, NSUInteger flags, BOOL *stop);
    NSUInteger limit;
    BOOL stopped;
    BOOL passedLimit;
} RegexCallbackContext;

static id escapedString(id string, id characters);
static int8_t regexMatchCallback(const void *context, int32_t steps);
static int8_t regexFindProgressCallback(const void *context, int64_t matchIndex);

@implementation ONSRegularExpression

// Native +[NSRegularExpression initialize], 0x1c0650f0c.
+ (void)initialize {
    if (self == [ONSRegularExpression class]) {
        [(id)self setVersion:1];
    }
}

// Native -[NSRegularExpression initWithPattern:options:error:], 0x1c0650f90. Compiles the pattern (escaped
// for NSRegularExpressionIgnoreMetacharacters) with the options mapped to ICU flags; its characters are read
// in place, into a 256-character stack buffer, or into a malloc'd one. A failure answers nil with
// NSCocoaErrorDomain 2048 (NSInvalidValue: the compiled pattern); a pattern of 0x7FFFFFFF characters or more
// fails the same way.
- (id)initWithPattern:(id)pattern options:(NSUInteger)options error:(id *)error {
    if (pattern == nil) {
        id reason = [(id)regexStringClass
            stringWithFormat:(id)OF_STR("%@: nil argument"), _ONSMethodExceptionProem(self, _cmd)];
        objc_release(self);
        objc_exception_throw([(id)regexExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                                 reason:reason
                                                               userInfo:nil]);
    }
    self = [super init];
    if (self == nil) {
        return nil;
    }
    int32_t status = 0;
    RegexParseError parseError;
    memset(&parseError, 0, sizeof(parseError));
    id compiled = pattern;
    if ((options & 4) != 0) {
        compiled = [ONSRegularExpression escapedPatternForString:pattern];
    }
    NSUInteger length = [compiled length];
    uint16_t stackBuffer[256];
    const uint16_t *characters = OCFStringGetCharactersPtr((OCFStringRef)compiled);
    bool allocated = characters == NULL;
    if (characters == NULL) {
        if (length <= 0x100) {
            [compiled getCharacters:stackBuffer range:(ONSRange){0, length}];
            characters = stackBuffer;
            allocated = false;
        } else {
            uint16_t *buffer = malloc_type_malloc(2 * length, 0x1000040BDFB0063);
            if (buffer == NULL) {
                goto finish;
            }
            [compiled getCharacters:buffer range:(ONSRange){0, length}];
            characters = buffer;
        }
    }
    bool failed;
    if (length >= 0x7FFFFFFF) {
        failed = false;
    } else {
        uint32_t flags = ((4 * (uint32_t)options) & 0x20) | (2 * ((uint32_t)options & 3)) |
                         (((uint32_t)options >> 1) & 8) | ((options & 0x20) != 0) |
                         (((options & 0x40) != 0) << 8);
        _internal = uregex_open(characters, (int32_t)length, flags, &parseError, &status);
        failed = status > 0;
    }
    if (_internal == NULL || failed) {
        if (error != NULL) {
            *error = [(id)regexErrorClass
                errorWithDomain:(id)OF_STR(OF_NAME_STRING(ONSCocoaErrorDomain))
                           code:2048
                       userInfo:[(id)regexDictionaryClass
                                    dictionaryWithObjectsAndKeys:compiled,
                                                                 (id)OF_STR("NSInvalidValue"),
                                                                 nil]];
        }
        if (_internal != NULL) {
            uregex_close(_internal);
            _internal = NULL;
        }
    }
    if (allocated) {
        free((void *)characters);
    }
finish:
    if (_internal == NULL) {
        objc_release(self);
        return nil;
    }
    _pattern = [pattern copy];
    _options = options;
    return self;
}

// Native -[NSRegularExpression copyWithZone:], 0x1c0651230. Immutable: the receiver, retained.
- (id)copyWithZone:(struct _NSZone *)zone {
    return objc_retain(self);
}

// Native -[NSRegularExpression encodeWithCoder:], 0x1c0651234. NSPattern and NSOptions when keyed; otherwise
// the pattern and the options as "Q".
- (void)encodeWithCoder:(id)coder {
    id pattern = [self pattern];
    NSUInteger options = [self options];
    if ([coder allowsKeyedCoding]) {
        [coder encodeObject:pattern forKey:(id)OF_STR("NSPattern")];
        [coder encodeInt64:(int64_t)options forKey:(id)OF_STR("NSOptions")];
        return;
    }
    [coder encodeObject:pattern];
    [coder encodeValueOfObjCType:"Q" at:&options];
}

// Native -[NSRegularExpression initWithCoder:], 0x1c0651324. Unkeyed archives must carry class version 1;
// another version is logged and the receiver deallocated.
- (id)initWithCoder:(id)coder {
    if ([coder allowsKeyedCoding]) {
        id pattern = [coder decodeObjectOfClass:[(id)regexStringClass class] forKey:(id)OF_STR("NSPattern")];
        int64_t options = [coder decodeInt64ForKey:(id)OF_STR("NSOptions")];
        return [self initWithPattern:pattern options:(NSUInteger)options error:NULL];
    }
    NSUInteger options = 0;
    NSInteger version = [coder versionForClassName:(id)OF_STR(OF_NAME_STRING(ONSRegularExpression))];
    if (version == 1) {
        id pattern = [coder decodeObject];
        [coder decodeValueOfObjCType:"Q" at:&options size:8];
        return [self initWithPattern:pattern options:options error:NULL];
    }
    ONSLog(
        (id)OF_STR("%@: class version %ld cannot read instances archived with version %ld"),
        _ONSFullMethodName(self, _cmd),
        (long)1,
        (long)version
    );
    [self dealloc];
    return nil;
}

// Native +[NSRegularExpression supportsSecureCoding], 0x1c0651458.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSRegularExpression hash], 0x1c0651460.
- (NSUInteger)hash {
    return [[self pattern] hash] ^ [self options];
}

// Native -[NSRegularExpression isEqual:], 0x1c0651494.
- (BOOL)isEqual:(id)object {
    if (object == self) {
        return YES;
    }
    if (object == nil || ![object isKindOfClass:[ONSRegularExpression class]]) {
        return NO;
    }
    if (![[self pattern] isEqual:[object pattern]]) {
        return NO;
    }
    return [self options] == [object options];
}

// Native -[NSRegularExpression dealloc], 0x1c065152c.
- (void)dealloc {
    objc_release(_pattern);
    if (_internal != NULL) {
        uregex_close(_internal);
        _internal = NULL;
    }
    [super dealloc];
}

// Native -[NSRegularExpression description], 0x1c06515b0.
- (id)description {
    return [(id)regexStringClass
        stringWithFormat:(id)OF_STR("%@ %@ 0x%lx"), [super description], _pattern, _options];
}

// Native -[NSRegularExpression pattern], 0x1c0651640.
- (id)pattern {
    return objc_autorelease(objc_retain(_pattern));
}

// Native -[NSRegularExpression options], 0x1c0651658.
- (NSUInteger)options {
    return _options;
}

// Native -[NSRegularExpression numberOfCaptureGroups], 0x1c0651660.
- (NSUInteger)numberOfCaptureGroups {
    int32_t status = 0;
    if (_internal == NULL) {
        return 0;
    }
    return (NSUInteger)(NSInteger)uregex_groupCount(_internal, &status);
}

// Native +[NSRegularExpression escapedPatternForString:], 0x1c06517f8.
+ (id)escapedPatternForString:(id)string {
    return escapedString(string, (id)OF_STR("*?+[(){}^$|\\./"));
}

@end

// Native escapedString, 0x1c0651808. Non-external. A backslash before every character of the set; the string
// itself when none occurs.
static id escapedString(id string, id characters) {
    id set = [(id)regexCharacterSetClass characterSetWithCharactersInString:characters];
    ONSRange range = [string rangeOfCharacterFromSet:set];
    if (string == nil || range.length == 0) {
        return string;
    }
    string = [(id)regexMutableStringClass stringWithString:string];
    do {
        [string insertString:(id)OF_STR("\\") atIndex:range.location];
        NSUInteger length = [string length];
        NSUInteger next = range.location + range.length + 1;
        if (next >= length) {
            break;
        }
        range = [string rangeOfCharacterFromSet:set options:0 range:(ONSRange){next, length - next}];
    } while (range.length != 0);
    return string;
}

@implementation ONSRegularExpression (NSMatching)

// Native -[NSRegularExpression(NSMatching) enumerateMatchesInString:options:range:usingBlock:], 0x1c06518d0.
// Matches over a CFString UText in the range (clamped to 0x7FFFFFFF), on _internal when the checkout is free
// and on a clone otherwise. Each match's ranges (up to the capture groups) make a simple (1-3), extended
// (4-7) or complex result (none with the private option 0x2000); the block receives it with the hit-end and
// require-end flags. NSMatchingAnchored keeps each match at or before the previous end; the progress callback
// enforces it. NSMatchingReportCompletion ends with a nil result (Completed, or Completed | InternalError on
// an ICU error). When the UText setup fails the regex is left with its callbacks and bounds reset but the
// checkout taken and a clone open (native).
- (void)enumerateMatchesInString:(id)string
                         options:(NSUInteger)options
                           range:(ONSRange)range
                      usingBlock:(void (^)(id result, NSUInteger flags, BOOL *stop))block {
    NSUInteger stringLength = [string length];
    NSUInteger groups = [self numberOfCaptureGroups];
    BOOL stop = NO;
    int32_t findStatus = 0;
    if (string == nil) {
        id reason = [(id)regexStringClass
            stringWithFormat:(id)OF_STR("%@: nil argument"), _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([(id)regexExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                                 reason:reason
                                                               userInfo:nil]);
    }
    if (stringLength < range.length || range.location > stringLength - range.length) {
        id reason = [(id)regexStringClass stringWithFormat:(id)OF_STR("%@: Range or index out of bounds"),
                                                           _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([(id)regexExceptionClass exceptionWithName:(id)ONSRangeException
                                                                 reason:reason
                                                               userInfo:nil]);
    }
    RegexCallbackContext context;
    context.block = block;
    context.limit = (options & 4) != 0 ? range.location : kNotFound;
    context.stopped = NO;
    context.passedLimit = NO;
    URegularExpression *regex = _internal;
    NSUInteger length = [string length];
    ProviderUText *text = NULL;
    int32_t status = 0;
    uint16_t empty = 0;
    bool owned = false;
    bool progress = false;
    if (range.location > 0x7FFFFFFE || range.location + range.length > length) {
        goto completion;
    }
    text = OCFStringOpenUText(NULL, (OCFStringRef)string);
    if (text == NULL) {
        goto completion;
    }
    NSUInteger regionLength =
        (range.location + range.length) >> 31 != 0 ? 0x7FFFFFFF - range.location : range.length;
    int32_t cloneStatus = 0;
    int expected = 0;
    atomic_compare_exchange_strong((_Atomic int *)&_checkout, &expected, 1);
    owned = expected == 0;
    if (!owned) {
        regex = uregex_clone(regex, &cloneStatus);
    }
    uregex_setUText(regex, text, &status);
    if (regex == NULL) {
        goto completion;
    }
    uregex_setRegion64(regex, (int64_t)range.location, (int64_t)(regionLength + range.location), &status);
    if ((options & 1) != 0) {
        uregex_setMatchCallback(regex, regexMatchCallback, &context, &status);
    }
    if ((options & 5) != 0) {
        uregex_setFindProgressCallback(regex, regexFindProgressCallback, &context, &status);
        progress = true;
    }
    if ((options & 8) != 0) {
        uregex_useTransparentBounds(regex, 1, &status);
    }
    if ((options & 0x10) != 0) {
        uregex_useAnchoringBounds(regex, 0, &status);
    }
    if (status > 0) {
        uregex_setText(regex, &empty, 0, &status);
        if ((options & 1) != 0) {
            uregex_setMatchCallback(regex, NULL, NULL, &status);
        }
        if (progress) {
            uregex_setFindProgressCallback(regex, NULL, NULL, &status);
        }
        if ((options & 8) != 0) {
            uregex_useTransparentBounds(regex, 0, &status);
        }
        if ((options & 0x10) != 0) {
            uregex_useAnchoringBounds(regex, 1, &status);
        }
        goto completion;
    }
    {
        ONSRange ranges[7];
        while (1) {
            int8_t found = uregex_findNext(regex, &findStatus);
            if (!found || findStatus > 0 || stop || context.stopped || context.passedLimit) {
                break;
            }
            if ((options & 4) != 0) {
                int64_t start = uregex_start64(regex, 0, &findStatus);
                if ((NSUInteger)start > context.limit) {
                    break;
                }
                context.limit = (NSUInteger)uregex_end64(regex, 0, &findStatus);
            }
            id result = nil;
            if ((options & 0x2000) == 0) {
                id rangeArray = groups >= 7 ? [[(id)regexMutableArrayClass alloc] init] : nil;
                for (NSUInteger group = 0; group <= groups; ++group) {
                    status = 0;
                    int64_t start = uregex_start64(regex, (int32_t)group, &status);
                    int64_t end = uregex_end64(regex, (int32_t)group, &status);
                    bool invalid = start < 0 || end < start || status > 0;
                    if (groups > 6) {
                        ONSRange groupRange = invalid
                                                  ? (ONSRange){kNotFound, 0}
                                                  : (ONSRange){(NSUInteger)start, (NSUInteger)(end - start)};
                        [rangeArray addObject:[(id)regexValueClass valueWithRange:groupRange]];
                    } else {
                        ranges[group] = invalid ? (ONSRange){kNotFound, 0}
                                                : (ONSRange){(NSUInteger)start, (NSUInteger)(end - start)};
                    }
                }
                if (groups > 6) {
                    result = [[ONSComplexRegularExpressionCheckingResult alloc] initWithRangeArray:rangeArray
                                                                                 regularExpression:self];
                    objc_release(rangeArray);
                } else if (groups > 2) {
                    result = [[ONSExtendedRegularExpressionCheckingResult alloc] initWithRanges:ranges
                                                                                          count:groups + 1
                                                                              regularExpression:self];
                } else {
                    result = [[ONSSimpleRegularExpressionCheckingResult alloc] initWithRanges:ranges
                                                                                        count:groups + 1
                                                                            regularExpression:self];
                }
            }
            int32_t endStatus = 0;
            int8_t hitEnd = uregex_hitEnd(regex, &endStatus);
            int8_t requireEnd = uregex_requireEnd(regex, &endStatus);
            NSUInteger flags = hitEnd ? 4 : 0;
            if (requireEnd) {
                flags |= 8;
            }
            if (endStatus > 0) {
                flags = 0;
            }
            block(result, flags, &stop);
            objc_release(result);
            if (stop) {
                goto finished;
            }
        }
        if ((options & 2) != 0 && !stop && !context.stopped) {
            NSUInteger flags;
            if (findStatus <= 0 || context.passedLimit) {
                int32_t endStatus = 0;
                int8_t hitEnd = uregex_hitEnd(regex, &endStatus);
                int8_t requireEnd = uregex_requireEnd(regex, &endStatus);
                NSUInteger endFlags = hitEnd ? 4 : 0;
                if (requireEnd) {
                    endFlags |= 8;
                }
                flags = endStatus > 0 ? 2 : (endFlags | 2);
            } else {
                flags = 18;
            }
            block(nil, flags, &stop);
        }
    finished:
        status = 0;
        if (!owned) {
            uregex_close(regex);
            goto close;
        }
        uregex_setText(regex, &empty, 0, &status);
        if ((options & 1) != 0) {
            uregex_setMatchCallback(regex, NULL, NULL, &status);
        }
        if ((options & 5) != 0) {
            uregex_setFindProgressCallback(regex, NULL, NULL, &status);
        }
        if ((options & 8) != 0) {
            uregex_useTransparentBounds(regex, 0, &status);
        }
        if ((options & 0x10) != 0) {
            uregex_useAnchoringBounds(regex, 1, &status);
        }
        atomic_fetch_and((_Atomic int *)&_checkout, 0);
        goto close;
    }

completion:
    if ((options & 2) != 0 && !context.stopped) {
        block(nil, 18, &stop);
    }
    status = 0;
close:
    if (text != NULL) {
        utext_close(text);
    }
}

// Native -[NSRegularExpression(NSMatching) matchesInString:options:range:], 0x1c0651f64. The non-nil results,
// enumerated without progress and completion reports.
- (id)matchesInString:(id)string options:(NSUInteger)options range:(ONSRange)range {
    id matches = [(id)regexMutableArrayClass array];
    [self enumerateMatchesInString:string
                           options:options & ~(NSUInteger)3
                             range:range
                        usingBlock:^(id result, NSUInteger flags, BOOL *stop) {
                          // Native ___65-[NSRegularExpression(NSMatching)
                          // matchesInString:options:range:]_block_invoke, 0x1c0652034. Non-external.
                          (void)flags;
                          (void)stop;
                          if (result != nil) {
                              [matches addObject:result];
                          }
                        }];
    return matches;
}

// Native -[NSRegularExpression(NSMatching) rangeOfFirstMatchInString:options:range:], 0x1c0652260.
// {NSNotFound, 0} when nothing matches.
- (ONSRange)rangeOfFirstMatchInString:(id)string options:(NSUInteger)options range:(ONSRange)range {
    __block ONSRange first = {kNotFound, 0};
    [self enumerateMatchesInString:string
                           options:options & ~(NSUInteger)3
                             range:range
                        usingBlock:^(id result, NSUInteger flags, BOOL *stop) {
                          // Native ___75-[NSRegularExpression(NSMatching)
                          // rangeOfFirstMatchInString:options:range:]_block_invoke, 0x1c0652348.
                          // Non-external.
                          (void)flags;
                          if (result != nil) {
                              first = [result range];
                          }
                          *stop = YES;
                        }];
    return first;
}

@end

@implementation ONSRegularExpression (NSReplacement)

// Native -[NSRegularExpression(NSReplacement) stringByReplacingMatchesInString:options:range:withTemplate:],
// 0x1c0652388. The string with each match replaced by its template expansion; the text between matches and
// after the last one is copied.
- (id)stringByReplacingMatchesInString:(id)string
                               options:(NSUInteger)options
                                 range:(ONSRange)range
                          withTemplate:(id)replacementTemplate {
    id result = [(id)regexMutableStringClass string];
    NSUInteger length = [string length];
    if (replacementTemplate == nil) {
        id reason = [(id)regexStringClass
            stringWithFormat:(id)OF_STR("%@: nil argument"), _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([(id)regexExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                                 reason:reason
                                                               userInfo:nil]);
    }
    NSUInteger previousEnd = 0;
    id matches = [self matchesInString:string options:options & ~(NSUInteger)3 range:range];
    for (id match in matches) {
        ONSRange matchRange = [match range];
        id replacement = [self replacementStringForResult:match
                                                 inString:string
                                                   offset:0
                                                 template:replacementTemplate];
        if (matchRange.location > previousEnd) {
            [result appendString:[string substringWithRange:(ONSRange){
                                                                previousEnd, matchRange.location - previousEnd
                                                            }]];
        }
        [result appendString:replacement];
        previousEnd = matchRange.location + matchRange.length;
    }
    if (length > previousEnd) {
        [result appendString:[string substringWithRange:(ONSRange){previousEnd, length - previousEnd}]];
    }
    return result;
}

// Native -[NSRegularExpression(NSReplacement) replaceMatchesInString:options:range:withTemplate:],
// 0x1c06525bc. Replaces each match in place, shifting later ranges by the accumulated change; returns the
// number of matches.
- (NSUInteger)replaceMatchesInString:(id)string
                             options:(NSUInteger)options
                               range:(ONSRange)range
                        withTemplate:(id)replacementTemplate {
    if (replacementTemplate == nil) {
        id reason = [(id)regexStringClass
            stringWithFormat:(id)OF_STR("%@: nil argument"), _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([(id)regexExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                                 reason:reason
                                                               userInfo:nil]);
    }
    NSUInteger count = 0;
    NSInteger offset = 0;
    id matches = [self matchesInString:string options:options & ~(NSUInteger)3 range:range];
    for (id match in matches) {
        ONSRange matchRange = [match range];
        id replacement = [self replacementStringForResult:match
                                                 inString:string
                                                   offset:offset
                                                 template:replacementTemplate];
        [string replaceCharactersInRange:(ONSRange){matchRange.location + offset, matchRange.length}
                              withString:replacement];
        offset += (NSInteger)[replacement length] - (NSInteger)matchRange.length;
        ++count;
    }
    return count;
}

// Native -[NSRegularExpression(NSReplacement) replacementStringForResult:inString:offset:template:],
// 0x1c0652784. Expands "$n" (n read with at most as many digits as the range count has, a group past the
// ranges giving the empty string) from the result's ranges shifted by offset, and "\c" to c. The character
// set of '\' and '$' is created once (static, retained).
- (id)replacementStringForResult:(id)result inString:(id)string offset:(NSInteger)offset template:(id)templ {
    static id characterSet;
    NSUInteger rangeCount = [result numberOfRanges];
    if (result == nil || string == nil || templ == nil) {
        id reason = [(id)regexStringClass
            stringWithFormat:(id)OF_STR("%@: nil argument"), _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([(id)regexExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                                 reason:reason
                                                               userInfo:nil]);
    }
    if (characterSet == nil) {
        characterSet =
            objc_retain([(id)regexCharacterSetClass characterSetWithCharactersInString:(id)OF_STR("\\$")]);
    }
    ONSRange found = [templ rangeOfCharacterFromSet:characterSet];
    if (found.length == 0) {
        return templ;
    }
    NSUInteger location = found.location;
    NSUInteger foundLength = found.length;
    templ = [(id)regexMutableStringClass stringWithString:templ];
    NSUInteger length = [templ length];
    NSUInteger maximumDigits;
    if (rangeCount < 11) {
        maximumDigits = 1;
    } else {
        NSUInteger power = 10;
        NSUInteger digits = 1;
        do {
            maximumDigits = digits + 1;
            power *= 10;
        } while (power < rangeCount && digits++ < 0x13);
    }
    do {
        uint16_t character = [templ characterAtIndex:location];
        if (character == '$') {
            NSUInteger cursor = location + foundLength;
            NSUInteger end = length >= location + foundLength + maximumDigits
                                 ? location + foundLength + maximumDigits
                                 : length;
            if (cursor < end) {
                NSUInteger group = kNotFound;
                do {
                    uint16_t digit = [templ characterAtIndex:cursor];
                    if ((uint32_t)digit - 58 < 0xFFFFFFF6) {
                        break;
                    }
                    group = (group == kNotFound ? 0 : 10 * group) + (digit & 0xF);
                    ++cursor;
                } while (cursor < end);
                if (group != kNotFound) {
                    NSUInteger groupLocation;
                    if (group >= rangeCount) {
                        foundLength = 0;
                        groupLocation = kNotFound;
                    } else {
                        ONSRange groupRange = [result rangeAtIndex:group];
                        groupLocation = groupRange.location;
                        foundLength = groupRange.length;
                    }
                    NSUInteger shifted = groupLocation == kNotFound ? kNotFound : groupLocation + offset;
                    id replacement = (id)OF_STR("");
                    if (shifted != kNotFound && foundLength != 0) {
                        replacement = [string substringWithRange:(ONSRange){shifted, foundLength}];
                    }
                    NSUInteger replaced = cursor - location;
                    [templ replaceCharactersInRange:(ONSRange){location, replaced} withString:replacement];
                    length = length + foundLength - replaced;
                }
            }
        } else if (character == '\\') {
            [templ deleteCharactersInRange:(ONSRange){location, foundLength}];
            length -= foundLength;
            foundLength = 1;
        }
        if (length <= location + foundLength) {
            break;
        }
        found = [templ
            rangeOfCharacterFromSet:characterSet
                            options:0
                              range:(ONSRange){location + foundLength, length - (location + foundLength)}];
        location = found.location;
        foundLength = found.length;
    } while (foundLength != 0);
    return templ;
}

@end

// Native regexMatchCallback, 0x1c0653d44. Non-external. Reports progress (a nil result with
// NSMatchingProgress) and continues unless the block stops.
static int8_t regexMatchCallback(const void *context, int32_t steps) {
    (void)steps;
    BOOL stop = NO;
    if (context == NULL) {
        return 1;
    }
    RegexCallbackContext *callbackContext = (RegexCallbackContext *)context;
    callbackContext->block(nil, 1, &stop);
    callbackContext->stopped = stop;
    return !stop;
}

// Native regexFindProgressCallback, 0x1c0653dcc. Non-external. Stops a find that passed the anchored limit;
// otherwise reports progress as regexMatchCallback does (continuing when there is no block).
static int8_t regexFindProgressCallback(const void *context, int64_t matchIndex) {
    BOOL stop = NO;
    if (context == NULL) {
        return 1;
    }
    RegexCallbackContext *callbackContext = (RegexCallbackContext *)context;
    NSUInteger limit = callbackContext->limit;
    if (limit != kNotFound && (NSUInteger)matchIndex > limit) {
        callbackContext->passedLimit = YES;
        return 0;
    }
    if (callbackContext->block == nil) {
        return 1;
    }
    callbackContext->block(nil, 1, &stop);
    callbackContext->stopped = stop;
    return !stop;
}

//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSMutableAttributedString.h"
#import "Foundation/String/ONSAttributedStringInternal.h"
#include "CoreFoundation/String/OCFString.h"

// Inferred organization: the mutable half of the native attributed-string base unit, with its cold
// abstract primitives. Evidence: Local/Evidence/cf-core/iphone-simulator-foundation-link.json,
// FC-sets-attributed. iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md. MRC with direct libobjc ownership calls.
// The NSException/NSInvalidArgumentException shared-cache bindings at 0x22227fae0 and 0x22227f500 have medium
// confidence (the slots lie outside the extracted image).

@implementation ONSMutableAttributedString

// Native +[NSMutableAttributedString allocWithZone:], 0x1c04c4f24. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    Class cls = self;
    if (cls == (Class)objc_opt_self((id)attributedStringMutableBaseClass)) {
        cls = (Class)objc_opt_self((id)attributedStringMutableConcreteClass);
    }
    return ONSAllocateObject(cls, 0, zone);
}

// Native -[NSMutableAttributedString mutableString], 0x1c04c4f74. Non-external.
- (id)mutableString {
    return objc_autorelease([objc_allocWithZone((Class)attributedStringProxyClass)
        initWithMutableAttributedString:self]);
}

// Native -[NSMutableAttributedString addAttribute:value:range:], 0x1c04c4fa4. Non-external. Empty ranges
// bypass nil-value validation (kept). The presentation-intent marker is set after all runs, before
// endEditing.
- (void)addAttribute:(id)name value:(id)value range:(ONSRange)range {
    if (range.length == 0) {
        return;
    }
    if (value == nil) {
        id reason = [(id)attributedStringStringClass stringWithFormat:(id)OF_STR("%s %s: nil value"),
                                                                      _NSNameOfClass(objc_opt_class(self)),
                                                                      sel_getName(_cmd)];
        objc_exception_throw([(id)attributedStringExceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:reason
                     userInfo:nil]);
    }
    [self beginEditing];
    do {
        ONSRange effectiveRange = {0, 0};
        id attributes = [[self attributesAtIndex:range.location
                                  effectiveRange:&effectiveRange] mutableCopyWithZone:NULL];
        effectiveRange = attributedStringIntersectRanges(range, effectiveRange);
        [attributes setObject:value forKey:name];
        [self setAttributes:attributes range:effectiveRange];
        objc_release(attributes);
        range.location += effectiveRange.length;
        range.length -= effectiveRange.length;
    } while (range.length != 0);
    if ([self _willRequireIntentResolutionWhenContainingAttribute:name value:value]) {
        [self _markRequiringIntentResolution];
    }
    [self endEditing];
}

// Native -[NSMutableAttributedString addAttributes:range:], 0x1c04c5134. Non-external. The assembly
// always stores the requested start and caps the intersected length to the remaining length. A nil
// attributes dictionary throws only for nonempty ranges (kept); no editing cleanup is added on exceptions.
- (void)addAttributes:(id)attributes range:(ONSRange)range {
    if (range.length == 0) {
        return;
    }
    if (attributes == nil) {
        id reason = [(id)attributedStringStringClass stringWithFormat:(id)OF_STR("%s %s: nil value"),
                                                                      _NSNameOfClass(objc_opt_class(self)),
                                                                      sel_getName(_cmd)];
        objc_exception_throw([(id)attributedStringExceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:reason
                     userInfo:nil]);
    }
    [self beginEditing];
    do {
        ONSRange effectiveRange = {0, 0};
        id merged = [[self attributesAtIndex:range.location
                              effectiveRange:&effectiveRange] mutableCopyWithZone:NULL];
        ONSRange intersection = attributedStringIntersectRanges(range, effectiveRange);
        effectiveRange.location = range.location;
        effectiveRange.length = intersection.length < range.length ? intersection.length : range.length;
        [merged addEntriesFromDictionary:attributes];
        [self setAttributes:merged range:effectiveRange];
        objc_release(merged);
        range.location += effectiveRange.length;
        range.length -= effectiveRange.length;
    } while (range.length != 0);
    if ([self _willRequireIntentResolutionWhenContainingAttributes:attributes]) {
        [self _markRequiringIntentResolution];
    }
    [self endEditing];
}

// Native -[NSMutableAttributedString removeAttribute:range:], 0x1c04c53f0. Non-external.
- (void)removeAttribute:(id)name range:(ONSRange)range {
    if (range.length == 0) {
        return;
    }
    [self beginEditing];
    do {
        ONSRange effectiveRange = {0, 0};
        id attributes = [[self attributesAtIndex:range.location
                                  effectiveRange:&effectiveRange] mutableCopyWithZone:NULL];
        effectiveRange = attributedStringIntersectRanges(range, effectiveRange);
        [attributes removeObjectForKey:name];
        [self setAttributes:attributes range:effectiveRange];
        objc_release(attributes);
        range.location += effectiveRange.length;
        range.length -= effectiveRange.length;
    } while (range.length != 0);
    [self endEditing];
}

// Native -[NSMutableAttributedString replaceCharactersInRange:withAttributedString:], 0x1c04c54f8.
// Non-external. Read the replacement string and length before beginEditing; copy each run after replacing
// text, propagate the intent marker, then endEditing. No self-replacement snapshot is added (kept).
- (void)replaceCharactersInRange:(ONSRange)range withAttributedString:(id)string {
    id replacement = [string string];
    NSUInteger length = [replacement length];
    ONSRange effectiveRange = {0, 0};
    [self beginEditing];
    [self replaceCharactersInRange:range withString:replacement];
    while (effectiveRange.location < length) {
        id attributes = [string attributesAtIndex:effectiveRange.location effectiveRange:&effectiveRange];
        [self setAttributes:attributes
                      range:(ONSRange){effectiveRange.location + range.location, effectiveRange.length}];
        effectiveRange.location += effectiveRange.length;
    }
    if ([string _mayRequireIntentResolution]) {
        [self _markRequiringIntentResolution];
    }
    [self endEditing];
}

// Native -[NSMutableAttributedString appendAttributedString:], 0x1c04c5600. Non-external.
- (void)appendAttributedString:(id)string {
    ONSRange range = {[(id<AttributedStringMessages>)[self string] length], 0};
    [self replaceCharactersInRange:range withAttributedString:string];
}

// Native -[NSMutableAttributedString deleteCharactersInRange:], 0x1c04c5638. Non-external. Allocate and
// initialize an empty NSAttributedString, replace with it, then release it through libobjc.
- (void)deleteCharactersInRange:(ONSRange)range {
    id empty = objc_alloc_init((Class)attributedStringBaseClass);
    [self replaceCharactersInRange:range withAttributedString:empty];
    objc_release(empty);
}

// Native -[NSMutableAttributedString beginEditing], 0x1c04c56cc. Non-external. Native RET, no operation.
- (void)beginEditing {
}

// Native -[NSMutableAttributedString endEditing], 0x1c04c56d0. Non-external. Native RET, no operation.
- (void)endEditing {
}

// Native -[NSMutableAttributedString classForCoder], 0x1c04c56d4. Non-external.
- (Class)classForCoder {
    return (Class)objc_opt_self((id)attributedStringMutableBaseClass);
}

// Native -[NSMutableAttributedString replaceCharactersInRange:withString:], 0x1c0fe2118. Non-external;
// the native abstract method, not a success stub.
- (void)replaceCharactersInRange:(ONSRange)range withString:(id)string {
    (void)range;
    (void)string;
    Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSMutableAttributedString)));
    ONSRequestConcreteImplementation(self, _cmd, cls);
}

// Native -[NSMutableAttributedString setAttributes:range:], 0x1c0fe2150. Non-external; abstract.
- (void)setAttributes:(id)attributes range:(ONSRange)range {
    (void)attributes;
    (void)range;
    Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSMutableAttributedString)));
    ONSRequestConcreteImplementation(self, _cmd, cls);
}

@end

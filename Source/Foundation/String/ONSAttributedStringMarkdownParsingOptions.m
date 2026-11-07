//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSAttributedStringMarkdownParsingOptions.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified unless noted. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.

extern char parsingOptionsClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                            OF_NAME_STRING(ONSAttributedStringMarkdownParsingOptions));

// The copy, equality and destruction paths use the libobjc ownership entry points under MRC.
@implementation ONSAttributedStringMarkdownParsingOptions

// Native -[NSAttributedStringMarkdownParsingOptions init], 0x1c0601228. Non-external.
- (instancetype)init {
    self = [super init];
    if (self != nil) {
        _allowsExtendedAttributes = NO;
        _appliesSourcePositionAttributes = NO;
        _failurePolicy = 0;
        _languageCode = nil;
        _interpretedSyntax = 0;
    }
    return self;
}

// Native -[NSAttributedStringMarkdownParsingOptions dealloc], 0x1c0601298. Non-external.
- (void)dealloc {
    objc_release(_languageCode);
    [super dealloc];
}

// Native -[NSAttributedStringMarkdownParsingOptions copyWithZone:], 0x1c060130c. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    ONSAttributedStringMarkdownParsingOptions *copy = objc_alloc_init(objc_opt_class(self));
    [copy setAllowsExtendedAttributes:[self allowsExtendedAttributes]];
    [copy setInterpretedSyntax:[self interpretedSyntax]];
    [copy setFailurePolicy:[self failurePolicy]];
    [copy setLanguageCode:[self languageCode]];
    [copy setAppliesSourcePositionAttributes:[self appliesSourcePositionAttributes]];
    return copy;
}

// Native -[NSAttributedStringMarkdownParsingOptions hash], 0x1c060139c. Non-external.
- (NSUInteger)hash {
    NSUInteger extended = [self allowsExtendedAttributes];
    NSUInteger syntax = [self interpretedSyntax];
    NSUInteger failure = [self failurePolicy];
    NSUInteger language = [[self languageCode] hash];
    NSUInteger positions = [self appliesSourcePositionAttributes];
    return extended ^ syntax ^ failure ^ language ^ positions;
}

// Native -[NSAttributedStringMarkdownParsingOptions isEqual:], 0x1c0601404. Non-external.
// The assembly tests the BOOL return registers, which Hex-Rays types as object pointers.
- (BOOL)isEqual:(id)object {
    Class cls = objc_opt_class((id)parsingOptionsClass);
    if (!objc_opt_isKindOfClass(object, cls)) {
        return NO;
    }
    ONSAttributedStringMarkdownParsingOptions *other = object;
    BOOL extended = [other allowsExtendedAttributes];
    if (extended != [self allowsExtendedAttributes]) {
        return NO;
    }
    NSInteger syntax = [other interpretedSyntax];
    if (syntax != [self interpretedSyntax]) {
        return NO;
    }
    NSInteger failure = [other failurePolicy];
    if (failure != [self failurePolicy]) {
        return NO;
    }
    id otherLanguage = [other languageCode];
    if (otherLanguage != [self languageCode]) {
        if ([self languageCode] == nil) {
            return NO;
        }
        otherLanguage = [other languageCode];
        if (![otherLanguage isEqual:[self languageCode]]) {
            return NO;
        }
    }
    BOOL positions = [other appliesSourcePositionAttributes];
    return positions == [self appliesSourcePositionAttributes];
}

// Native -[NSAttributedStringMarkdownParsingOptions allowsExtendedAttributes], 0x1c060150c. Non-external.
// Native -[NSAttributedStringMarkdownParsingOptions setAllowsExtendedAttributes:], 0x1c0601518. Non-external.
@synthesize allowsExtendedAttributes = _allowsExtendedAttributes;

// Native -[NSAttributedStringMarkdownParsingOptions interpretedSyntax], 0x1c0601520. Non-external.
// Native -[NSAttributedStringMarkdownParsingOptions setInterpretedSyntax:], 0x1c0601528. Non-external.
@synthesize interpretedSyntax = _interpretedSyntax;

// Native -[NSAttributedStringMarkdownParsingOptions failurePolicy], 0x1c0601530. Non-external.
// Native -[NSAttributedStringMarkdownParsingOptions setFailurePolicy:], 0x1c0601538. Non-external.
@synthesize failurePolicy = _failurePolicy;

// Native -[NSAttributedStringMarkdownParsingOptions languageCode], 0x1c0601540. Non-external.
// Native -[NSAttributedStringMarkdownParsingOptions setLanguageCode:], 0x1c060154c. Non-external.
// Atomic copy accessors use objc_getProperty and objc_setProperty_atomic_copy.
@synthesize languageCode = _languageCode;

// Native -[NSAttributedStringMarkdownParsingOptions appliesSourcePositionAttributes], 0x1c0601554.
// Non-external.
// Native -[NSAttributedStringMarkdownParsingOptions setAppliesSourcePositionAttributes:], 0x1c0601560.
// Non-external.
@synthesize appliesSourcePositionAttributes = _appliesSourcePositionAttributes;

@end

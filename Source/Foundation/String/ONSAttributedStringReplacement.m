//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSAttributedStringReplacement.h"
#import "Foundation/String/ONSAttributedStringFormatting.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#import "CoreFoundation/Objective-C/OCFNSMutableArray.h"
#import "CoreFoundation/Objective-C/OCFNSDictionary.h"
#include <objc/runtime.h>

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified except shared-cache GOT bindings (medium). iOS Device divergence: iOS Device 24A446
// remains unread. Reading: Local/Scratch/cf-core-r2-fg/Reading.md. The native retains/releases use MRC.

extern char replacementMutableArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableArray));
extern char numberClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSNumber));

@implementation _ONSAttributedStringReplacement

// Native -[_NSAttributedStringReplacement _initWithMetadata:], 0x1c0549878. Non-external.
- (instancetype)_initWithMetadata:(id<GrammarCollectionMessages>)metadata {
    self = [super init];
    if (!self) {
        return nil;
    }

    id index = [metadata objectForKeyedSubscript:(id)_kOCFStringFormatMetadataReplacementIndexKey];
    if (!objc_opt_isKindOfClass(index, objc_opt_class((id)numberClass)) || !index) {
        objc_release(self);
        return nil;
    }
    _index = [index integerValue];

    id specifierLengthKey = (id)_kOCFStringFormatMetadataSpecifierRangeLengthInFormatStringKey;
    id location = [metadata
        objectForKeyedSubscript:(id)_kOCFStringFormatMetadataSpecifierRangeLocationInFormatStringKey];
    BOOL locationIsNumber = objc_opt_isKindOfClass(location, objc_opt_class((id)numberClass));
    id length = [metadata objectForKeyedSubscript:specifierLengthKey];
    BOOL lengthIsNumber = objc_opt_isKindOfClass(length, objc_opt_class((id)numberClass));
    NSUInteger rangeLocation = NSIntegerMax;
    NSUInteger rangeLength = NSIntegerMax;
    if (locationIsNumber && location && lengthIsNumber && length) {
        rangeLocation = [location integerValue];
        rangeLength = [length integerValue];
    }
    _specifierRangeInFormatString = (ONSRange){rangeLocation, rangeLength};
    if (rangeLocation == NSIntegerMax) {
        objc_release(self);
        return nil;
    }

    id replacementLengthKey = (id)_kOCFStringFormatMetadataReplacementRangeLengthKey;
    location = [metadata objectForKeyedSubscript:(id)_kOCFStringFormatMetadataReplacementRangeLocationKey];
    locationIsNumber = objc_opt_isKindOfClass(location, objc_opt_class((id)numberClass));
    length = [metadata objectForKeyedSubscript:replacementLengthKey];
    lengthIsNumber = objc_opt_isKindOfClass(length, objc_opt_class((id)numberClass));
    rangeLocation = NSIntegerMax;
    rangeLength = NSIntegerMax;
    if (locationIsNumber && location && lengthIsNumber && length) {
        rangeLocation = [location integerValue];
        rangeLength = [length integerValue];
    }
    _replacementRangeInResult = (ONSRange){rangeLocation, rangeLength};
    if (rangeLocation == NSIntegerMax) {
        objc_release(self);
        return nil;
    }

    _replacementKind = 2;
    id argument = [metadata objectForKeyedSubscript:(id)_kOCFStringFormatMetadataArgumentObjectKey];
    if (argument) {
        _argument = objc_retain(argument);
        _replacementKind = 0;
    } else {
        argument = [metadata objectForKeyedSubscript:(id)_kOCFStringFormatMetadataArgumentNumberKey];
        if (argument) {
            _argument = objc_retain(argument);
            _replacementKind = 1;
        }
    }
    return self;
}

// Native -[_NSAttributedStringReplacement dealloc], 0x1c0549bc4. Non-external.
- (void)dealloc {
    objc_release(_argument);
    [super dealloc];
}

// Native +[_NSAttributedStringReplacement _replacementsFromMetadataArray:], 0x1c0549c38. Non-external.
+ (id)_replacementsFromMetadataArray:(id)metadata {
    id<GrammarCollectionMessages> replacements =
        [(id<GrammarCollectionMessages>)replacementMutableArrayClass array];
    for (id entry in metadata) {
        id replacement = [[_ONSAttributedStringReplacement alloc] _initWithMetadata:entry];
        if (replacement) {
            [replacements addObject:replacement];
        }
        objc_release(replacement);
    }
    return replacements;
}

// Native -[_NSAttributedStringReplacement index], 0x1c0549d5c. Non-external.
- (NSInteger)index {
    return _index;
}

// Native -[_NSAttributedStringReplacement replacementRangeInResult], 0x1c0549dc8. Non-external.
// The assembly returns both range words in x0/x1. Hex-Rays loses the length.
- (ONSRange)replacementRangeInResult {
    ONSRange range;
    objc_copyStruct(&range, &_replacementRangeInResult, sizeof(range), YES, NO);
    return range;
}

// Native -[_NSAttributedStringReplacement argument], 0x1c0549e2c. Non-external.
- (id)argument {
    return objc_getProperty(self, _cmd, 16, YES);
}

// Native -[_NSAttributedStringReplacement replacementKind], 0x1c0549e38. Non-external.
- (NSInteger)replacementKind {
    return _replacementKind;
}

@end

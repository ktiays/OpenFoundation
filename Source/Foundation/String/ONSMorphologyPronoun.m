//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSMorphologyPronoun.h"

#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSAttributedStringInflection.h"

// iOS Simulator 27.0 (24A434), arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured assembly. Manual reference counting.
// iOS Device divergence: see the method addresses and comparisons in Local/Scratch/cf-core-r2-fi/Reading.md.

extern id objc_getProperty(id self, SEL selector, ptrdiff_t offset, BOOL atomic);

@implementation ONSMorphologyPronoun

// Native -[NSMorphologyPronoun dealloc], 0x1c04c9014. Non-external.
- (void)dealloc {
    objc_release(_pronoun);
    objc_release(_morphology);
    objc_release(_dependentMorphology);
    [super dealloc];
}

// Native -[NSMorphologyPronoun hash], 0x1c04c9098. Non-external.
- (NSUInteger)hash {
    NSUInteger hash = [[self pronoun] hash];
    hash ^= [[self morphology] hash];
    return hash ^ [[self dependentMorphology] hash];
}

// Native -[NSMorphologyPronoun isEqual:], 0x1c04c90e0. Non-external.
- (BOOL)isEqual:(id)object {
    if (!objc_opt_isKindOfClass(object, [ONSMorphologyPronoun class])) {
        return NO;
    }
    ONSMorphologyPronoun *other = object;
    id lhs = [self pronoun];
    id rhs = [other pronoun];
    if (![lhs isEqualToString:rhs]) {
        return NO;
    }
    lhs = [self morphology];
    rhs = [other morphology];
    if (![lhs isEqual:rhs]) {
        return NO;
    }
    lhs = [self dependentMorphology];
    rhs = [other dependentMorphology];
    if (lhs == rhs) {
        return YES;
    }
    lhs = [self dependentMorphology];
    rhs = [other dependentMorphology];
    return [lhs isEqual:rhs];
}

// Native -[NSMorphologyPronoun copyWithZone:], 0x1c04c91c0. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    (void)zone;
    return objc_retain(self);
}

// Native +[NSMorphologyPronoun supportsSecureCoding], 0x1c04c91c4. Non-external.
// iOS Device 24A446: 0x180ee8708, verified same result.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSMorphologyPronoun encodeWithCoder:], 0x1c04c91cc. Non-external.
- (void)encodeWithCoder:(ONSCoder *)coder {
    [coder encodeObject:_pronoun forKey:(id)OF_STR("pronoun")];
    [coder encodeObject:_morphology forKey:(id)OF_STR("morphology")];
    if (_dependentMorphology != nil) {
        [coder encodeObject:_dependentMorphology forKey:(id)OF_STR("dependentMorphology")];
    }
}

// Native -[NSMorphologyPronoun initWithCoder:], 0x1c04c9234. Non-external.
- (id)initWithCoder:(ONSCoder *)coder {
    self = [super init];
    if (self != nil) {
        _pronoun = [[coder decodeObjectOfClass:[ONSString class] forKey:(id)OF_STR("pronoun")] copy];
        _morphology = [[coder decodeObjectOfClass:[ONSMorphology class]
                                           forKey:(id)OF_STR("morphology")] copy];
        _dependentMorphology = [[coder decodeObjectOfClass:[ONSMorphology class]
                                                    forKey:(id)OF_STR("dependentMorphology")] copy];
    }
    return self;
}

// Native -[NSMorphologyPronoun pronoun], 0x1c04c932c. Non-external.
- (ONSString *)pronoun {
    return objc_getProperty(self, _cmd, 8, YES);
}

// Native -[NSMorphologyPronoun morphology], 0x1c04c9338. Non-external.
- (ONSMorphology *)morphology {
    return objc_getProperty(self, _cmd, 16, YES);
}

// Native -[NSMorphologyPronoun dependentMorphology], 0x1c04c9344. Non-external.
- (ONSMorphology *)dependentMorphology {
    return objc_getProperty(self, _cmd, 24, YES);
}

@end

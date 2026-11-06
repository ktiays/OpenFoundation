//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSTermOfAddress.h"

#import "CoreFoundation/Objective-C/OCFNSArray.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSMorphologyPronoun.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"

#include <dispatch/dispatch.h>

// iOS Simulator 27.0 (24A434), arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured assembly. Manual reference counting.
// iOS Device divergence: see the method addresses and comparisons in Local/Scratch/cf-core-r2-fi/Reading.md.

extern id objc_getProperty(id self, SEL selector, ptrdiff_t offset, BOOL atomic);

@implementation ONSTermOfAddress

// Native -[NSTermOfAddress _initWithGender:addressingUser:], 0x1c05c4204. Non-external.
- (id)_initWithGender:(NSInteger)gender addressingUser:(BOOL)addressingUser {
    self = [super init];
    if (self != nil) {
        _gender = gender;
        _language = nil;
        _pronouns = nil;
        _addressesCurrentUser = addressingUser;
    }
    return self;
}

// Native +[NSTermOfAddress neutral], 0x1c05c4328. Non-external.
+ (id)neutral {
    static dispatch_once_t once;
    static id term;
    // Native ___26+[NSTermOfAddress neutral]_block_invoke, 0x1c05c43c8. Non-external.
    dispatch_once(&once, ^{
      term = [objc_alloc(self) _initWithGender:3 addressingUser:NO];
    });
    return term;
}

// Native +[NSTermOfAddress feminine], 0x1c05c43f4. Non-external.
+ (id)feminine {
    static dispatch_once_t once;
    static id term;
    // Native ___27+[NSTermOfAddress feminine]_block_invoke, 0x1c05c4494. Non-external.
    dispatch_once(&once, ^{
      term = [objc_alloc(self) _initWithGender:1 addressingUser:NO];
    });
    return term;
}

// Native +[NSTermOfAddress masculine], 0x1c05c44c0. Non-external.
+ (id)masculine {
    static dispatch_once_t once;
    static id term;
    // Native ___28+[NSTermOfAddress masculine]_block_invoke, 0x1c05c4560. Non-external.
    dispatch_once(&once, ^{
      term = [objc_alloc(self) _initWithGender:2 addressingUser:NO];
    });
    return term;
}

// Native +[NSTermOfAddress currentUser], 0x1c05c458c. Non-external.
+ (id)currentUser {
    static dispatch_once_t once;
    static id term;
    // Native ___30+[NSTermOfAddress currentUser]_block_invoke, 0x1c05c462c. Non-external.
    dispatch_once(&once, ^{
      term = [objc_alloc(self) _initWithGender:0 addressingUser:YES];
    });
    return term;
}

// Native -[NSTermOfAddress dealloc], 0x1c05c4690. Non-external.
- (void)dealloc {
    objc_release(_language);
    objc_release(_pronouns);
    [super dealloc];
}

// Native -[NSTermOfAddress copyWithZone:], 0x1c05c470c. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    (void)zone;
    return objc_retain(self);
}

// Native -[NSTermOfAddress debugDescription], 0x1c05c4710. Non-external.
- (id)debugDescription {
    if ([self isEqual:[ONSTermOfAddress neutral]]) {
        return [ONSString stringWithFormat:(id)OF_STR("<%@ %p>{ neutral }"), objc_opt_class(self), self];
    }
    if ([self isEqual:[ONSTermOfAddress feminine]]) {
        return [ONSString stringWithFormat:(id)OF_STR("<%@ %p>{ feminine }"), objc_opt_class(self), self];
    }
    if ([self isEqual:[ONSTermOfAddress masculine]]) {
        return [ONSString stringWithFormat:(id)OF_STR("<%@ %p>{ masculine }"), objc_opt_class(self), self];
    }
    BOOL currentUser = [self isEqual:[ONSTermOfAddress currentUser]];
    Class cls = objc_opt_class(self);
    if (currentUser) {
        return [ONSString stringWithFormat:(id)OF_STR("<%@ %p>{ currentUser }"), cls, self];
    }
    return [ONSString stringWithFormat:(id)OF_STR("<%@ %p>{ languageIdentifier: %@, pronouns: %@ }"),
                                       cls,
                                       self,
                                       _language,
                                       _pronouns];
}

// Native +[NSTermOfAddress supportsSecureCoding], 0x1c05c4840. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSTermOfAddress initWithCoder:], 0x1c05c4848. Non-external.
- (id)initWithCoder:(ONSCoder *)coder {
    self = [super init];
    if (![coder allowsKeyedCoding]) {
        objc_release(self);
        @throw [ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                        reason:(id)OF_STR("Encoder does not allow keyed coding!")
                                      userInfo:nil];
    }
    if (self == nil) {
        return nil;
    }
    _gender = [coder decodeIntegerForKey:(id)OF_STR("termOfAddress")];
    if ((NSUInteger)_gender - 1 <= 2) {
        _addressesCurrentUser = NO;
        _language = nil;
        _pronouns = nil;
        return self;
    }
    _addressesCurrentUser = [coder decodeBoolForKey:(id)OF_STR("addressesUser")];
    if (_addressesCurrentUser) {
        _language = nil;
        _pronouns = nil;
        return self;
    }

    _language = [[coder decodeObjectOfClass:[ONSString class] forKey:(id)OF_STR("language")] copy];
    _pronouns = [[coder decodeArrayOfObjectsOfClass:[ONSMorphologyPronoun class]
                                             forKey:(id)OF_STR("pronouns")] copy];
    if (_language == nil || _pronouns == nil) {
        objc_release(_language);
        objc_release(_pronouns);
        _language = nil;
        _pronouns = nil;
        objc_release(self);
        @throw [ONSException
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:(id)OF_STR("Could not decode language or pronouns for NSTermOfAddress")
                     userInfo:nil];
    }
    return self;
}

// Native -[NSTermOfAddress encodeWithCoder:], 0x1c05c49e0. Non-external.
- (void)encodeWithCoder:(ONSCoder *)coder {
    if (![coder allowsKeyedCoding]) {
        @throw [ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                        reason:(id)OF_STR("Encoder does not allow keyed coding!")
                                      userInfo:nil];
    }
    if (_addressesCurrentUser == YES) {
        [coder encodeBool:YES forKey:(id)OF_STR("addressesUser")];
    }
    if (_gender != 0) {
        [coder encodeInteger:_gender forKey:(id)OF_STR("termOfAddress")];
    }
    if (_language != nil) {
        [coder encodeObject:_language forKey:(id)OF_STR("language")];
    }
    if (_pronouns != nil) {
        [coder encodeObject:_pronouns forKey:(id)OF_STR("pronouns")];
    }
}

// Native -[NSTermOfAddress hash], 0x1c05c4aa4. Non-external.
- (NSUInteger)hash {
    NSUInteger hash = (NSUInteger)_gender ^ (NSUInteger)_addressesCurrentUser;
    if (_language != nil) {
        hash ^= [_language hash];
    }
    if (_pronouns != nil) {
        hash ^= [_pronouns hash];
    }
    return hash;
}

// Native -[NSTermOfAddress isEqual:], 0x1c05c4af0. Non-external.
- (BOOL)isEqual:(id)object {
    if (!objc_opt_isKindOfClass(object, [ONSTermOfAddress class])) {
        return NO;
    }
    ONSTermOfAddress *other = object;
    if (_gender != other->_gender || _addressesCurrentUser != other->_addressesCurrentUser) {
        return NO;
    }
    if (_pronouns != other->_pronouns && ![_pronouns isEqualToArray:other->_pronouns]) {
        return NO;
    }
    if (_language == other->_language) {
        return YES;
    }
    return [_language isEqualToString:other->_language];
}

// Native -[NSTermOfAddress languageIdentifier], 0x1c05c4b8c. Non-external. The atomic, retained property
// getter of _language (offset 24).
- (id)languageIdentifier {
    return objc_getProperty(self, _cmd, 24, YES);
}

// Native -[NSTermOfAddress pronouns], 0x1c05c4b98. Non-external.
- (ONSArray *)pronouns {
    return objc_getProperty(self, _cmd, 32, YES);
}

@end

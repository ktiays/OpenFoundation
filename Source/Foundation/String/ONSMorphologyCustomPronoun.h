//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <objc/NSObject.h>

@class ONSCoder;
@class ONSString;

// NSMorphologyCustomPronoun class object 0x22e6d6430, class_ro_t 0x2228489d0.
// Instance start 8, size 48. All five NSString ivars have 8-byte alignment and size.
__attribute__((visibility("default")))
@interface ONSMorphologyCustomPronoun : NSObject {
    ONSString *_subjectForm;
    ONSString *_objectForm;
    ONSString *_possessiveForm;
    ONSString *_possessiveAdjectiveForm;
    ONSString *_reflexiveForm;
}
@property(nonatomic, copy) ONSString *subjectForm;
@property(nonatomic, copy) ONSString *objectForm;
@property(nonatomic, copy) ONSString *possessiveForm;
@property(nonatomic, copy) ONSString *possessiveAdjectiveForm;
@property(nonatomic, copy) ONSString *reflexiveForm;

+ (BOOL)isSupportedForLanguage:(id)language;
+ (id)requiredKeysForLanguage:(id)language;
+ (BOOL)supportsSecureCoding;
- (void)encodeWithCoder:(ONSCoder *)coder;
- (id)initWithCoder:(ONSCoder *)coder;
@end

@interface ONSMorphologyCustomPronoun (NSExternalRepresentation)
- (id)_initWithExternalRepresentationDictionary:(id)dictionary;
@end

@interface ONSMorphologyCustomPronoun (NSSupportedPronounValidation)
+ (BOOL)_validateCustomPronounWithAccessor:(id (^)(id key))accessor
                               forLanguage:(id)language
                                     error:(id *)error;
@end

//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <objc/NSObject.h>

@class ONSMorphologyCustomPronoun;

// Native class_ro_t 0x222867fb8, instance start 8, size 64.
__attribute__((visibility("default")))
@interface _ONSAttributedStringGrammarInflection : NSObject {
    @package
    NSInteger _number;
    ONSMorphologyCustomPronoun *_englishCustomPronoun;
    NSInteger _context;
    NSInteger _gender;
    NSInteger _grammaticalCase;
    NSInteger _determinationType;
    NSInteger _person;
}
- (instancetype)initWithMorphology:(id)morphology;
- (ONSMorphologyCustomPronoun *)englishCustomPronoun;
- (NSInteger)context;
- (void)setContext:(NSInteger)value;
- (NSInteger)gender;
- (void)setGender:(NSInteger)value;
- (NSInteger)grammaticalCase;
- (void)setGrammaticalCase:(NSInteger)value;
- (NSInteger)determinationType;
- (void)setDeterminationType:(NSInteger)value;
- (NSInteger)number;
- (void)setNumber:(NSInteger)value;
- (NSInteger)person;
- (void)setPerson:(NSInteger)value;
@end

@interface _ONSAttributedStringGrammarInflection (NSExternalRepresentation)
- (instancetype)initWithExternalRepresentation:(id)data error:(id *)error;
- (instancetype)initWithExternalRepresentationDictionary:(id)dictionary error:(id *)error;
- (id)externalRepresentationDictionary;
- (id)externalRepresentation;
@end

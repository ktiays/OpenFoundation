//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSGrammaticalExternalRepresentation.h"
#import "Foundation/String/ONSString.h"
#include "CoreFoundation/String/OCFString.h"

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Scratch/cf-core-r2-fg/Reading.md.

// Native _NSGrammaticalGenderGetFromExternalRepresentation, 0x1c04c9d74. Non-external.
BOOL _NSGrammaticalGenderGetFromExternalRepresentation(id representation, NSInteger *value) {
    NSInteger result;
    if (!representation) {
        result = 0;
    } else if ([representation isEqual:(id)OF_STR("feminine")]) {
        result = 1;
    } else if ([representation isEqual:(id)OF_STR("masculine")]) {
        result = 2;
    } else if ([representation isEqual:(id)OF_STR("neuter")]) {
        result = 3;
    } else {
        return NO;
    }
    *value = result;
    return YES;
}

// Native _NSGrammaticalNumberGetFromExternalRepresentation, 0x1c04c9df4. Non-external.
BOOL _NSGrammaticalNumberGetFromExternalRepresentation(id representation, NSInteger *value) {
    NSInteger result;
    if (!representation) {
        result = 0;
    } else if ([representation isEqual:(id)OF_STR("one")]) {
        result = 1;
    } else if ([representation isEqual:(id)OF_STR("zero")]) {
        result = 2;
    } else if ([representation isEqual:(id)OF_STR("other")]) {
        result = 3;
    } else if ([representation isEqual:(id)OF_STR("two")]) {
        result = 4;
    } else if ([representation isEqual:(id)OF_STR("few")]) {
        result = 5;
    } else if ([representation isEqual:(id)OF_STR("many")]) {
        result = 6;
    } else {
        return NO;
    }
    *value = result;
    return YES;
}

// Native _NSGrammaticalPartOfSpeechGetFromExternalRepresentation, 0x1c04c9ec8. Non-external.
BOOL _NSGrammaticalPartOfSpeechGetFromExternalRepresentation(id representation, NSInteger *value) {
    NSInteger result;
    if (!representation) {
        result = 0;
    } else if ([representation isEqual:(id)OF_STR("determiner")]) {
        result = 1;
    } else if ([representation isEqual:(id)OF_STR("pronoun")]) {
        result = 2;
    } else if ([representation isEqual:(id)OF_STR("letter")]) {
        result = 3;
    } else if ([representation isEqual:(id)OF_STR("adverb")]) {
        result = 4;
    } else if ([representation isEqual:(id)OF_STR("particle")]) {
        result = 5;
    } else if ([representation isEqual:(id)OF_STR("adjective")]) {
        result = 6;
    } else if ([representation isEqual:(id)OF_STR("adposition")]) {
        result = 7;
    } else if ([representation isEqual:(id)OF_STR("verb")]) {
        result = 8;
    } else if ([representation isEqual:(id)OF_STR("noun")]) {
        result = 9;
    } else if ([representation isEqual:(id)OF_STR("conjunction")]) {
        result = 10;
    } else if ([representation isEqual:(id)OF_STR("numeral")]) {
        result = 11;
    } else if ([representation isEqual:(id)OF_STR("interjection")]) {
        result = 12;
    } else if ([representation isEqual:(id)OF_STR("preposition")]) {
        result = 13;
    } else if ([representation isEqual:(id)OF_STR("abbreviation")]) {
        result = 14;
    } else {
        return NO;
    }
    *value = result;
    return YES;
}

// Native _NSGrammaticalCaseGetFromExternalRepresentation, 0x1c04ca07c. Non-external.
BOOL _NSGrammaticalCaseGetFromExternalRepresentation(id representation, NSInteger *value) {
    NSInteger result;
    if (!representation) {
        result = 0;
    } else if ([representation isEqual:(id)OF_STR("nominative")]) {
        result = 1;
    } else if ([representation isEqual:(id)OF_STR("accusative")]) {
        result = 2;
    } else if ([representation isEqual:(id)OF_STR("dative")]) {
        result = 3;
    } else if ([representation isEqual:(id)OF_STR("genitive")]) {
        result = 4;
    } else if ([representation isEqual:(id)OF_STR("prepositional")]) {
        result = 5;
    } else if ([representation isEqual:(id)OF_STR("ablative")]) {
        result = 6;
    } else if ([representation isEqual:(id)OF_STR("adessive")]) {
        result = 7;
    } else if ([representation isEqual:(id)OF_STR("allative")]) {
        result = 8;
    } else if ([representation isEqual:(id)OF_STR("elative")]) {
        result = 9;
    } else if ([representation isEqual:(id)OF_STR("illative")]) {
        result = 10;
    } else if ([representation isEqual:(id)OF_STR("essive")]) {
        result = 11;
    } else if ([representation isEqual:(id)OF_STR("inessive")]) {
        result = 12;
    } else if ([representation isEqual:(id)OF_STR("locative")]) {
        result = 13;
    } else if ([representation isEqual:(id)OF_STR("translative")]) {
        result = 14;
    } else {
        return NO;
    }
    *value = result;
    return YES;
}

// Native _NSGrammaticalPronounTypeGetFromExternalRepresentation, 0x1c04ca230. Non-external.
BOOL _NSGrammaticalPronounTypeGetFromExternalRepresentation(id representation, NSInteger *value) {
    NSInteger result;
    if (!representation) {
        result = 0;
    } else if ([representation isEqual:(id)OF_STR("personal")]) {
        result = 1;
    } else if ([representation isEqual:(id)OF_STR("reflexive")]) {
        result = 2;
    } else if ([representation isEqual:(id)OF_STR("possessive")]) {
        result = 3;
    } else {
        return NO;
    }
    *value = result;
    return YES;
}

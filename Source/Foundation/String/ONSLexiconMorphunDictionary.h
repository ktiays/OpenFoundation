//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <objc/NSObject.h>

@class ONSTermOfAddress;
@class ONSMorphology;
@class ONSString;
@class ONSLocale;

// _NSLexiconMorphunDictionary class object 0x22e6dbef8, class_ro_t 0x22285a810.
// Instance start 8, size 48. The analyzer, tokenizer and dictionary are opaque Morphun handles.
@interface _ONSLexiconMorphunDictionary : NSObject {
    ONSLocale *_appropriateLocale;
    struct MANAConfigurableAnalyzer *_analyzer;
    struct MTokenizer *_tokenizer;
    const struct MDDictionaryMetaData *_dictionary;
    ONSString *_languageCode;
}
- (id)initWithLanguage:(id)language;
- (id)allPossibleWordAttributesForWord:(id)word;
- (id)preferredPronoun:(id)pronoun
      forTermOfAddress:(ONSTermOfAddress *)term
            morphology:(ONSMorphology *)morphology
            dependency:(id)dependency;
- (ONSString *)languageCode;
@end

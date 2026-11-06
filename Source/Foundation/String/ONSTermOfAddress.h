//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <objc/NSObject.h>

@class ONSArray;
@class ONSCoder;
@class ONSString;

// NSTermOfAddress class object 0x22e6dcfd8, class_ro_t 0x22285e2f0. Instance start 8, size 40.
__attribute__((visibility("default")))
@interface ONSTermOfAddress : NSObject {
    NSInteger _gender;
    BOOL _addressesCurrentUser;
    ONSString *_language;
    ONSArray *_pronouns;
}
+ (id)neutral;
+ (id)feminine;
+ (id)masculine;
+ (id)currentUser;
+ (BOOL)supportsSecureCoding;
- (id)initWithCoder:(ONSCoder *)coder;
- (void)encodeWithCoder:(ONSCoder *)coder;
- (ONSArray *)pronouns;
@end

// Swift Objective-C thunks deferred to the coordinator's scope update. Keep the native sends.
@interface ONSTermOfAddress (FoundationSwift)
- (BOOL)_isSupportedInLanguage:(id)language;
- (id)_withExtrapolatedPronouns;
- (id)_morphologyForLanguage:(id)language;
- (BOOL)_containsPronoun:(id)pronoun withGrammemes:(id)grammemes;
@end

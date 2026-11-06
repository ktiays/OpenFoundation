//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSMorphologyCustomPronoun.h"

#import "CoreFoundation/Objective-C/OCFNSConstantArray.h"
#import "CoreFoundation/Objective-C/OCFNSEmptyCollections.h"
#import "CoreFoundation/Objective-C/OCFNSDictionary.h"
#import "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#import "Foundation/Error/ONSError.h"
#import "Foundation/Error/ONSErrorConstants.h"
#import "Foundation/String/ONSString.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"

#include "CoreFoundation/String/OCFString.h"

// iOS Simulator 27.0 (24A434), arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured assembly and supplemental IDA read. Manual reference counting.
// Reading: Local/Scratch/cf-core-r2-fi/Reading.md. Source organization inferred.

OF_EXPORT id const ONSInvalidValueErrorKey;

// Native NSInvalidValueErrorKey, 0x2227d1248, external, pointing to the CFString at 0x222824050.
static const OCFConstantString invalidValueErrorKey = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSInvalidValue", 14
};
id const ONSInvalidValueErrorKey = (id)&invalidValueErrorKey;

extern char nullClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSNull));
extern char constantArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSConstantArray));
@protocol MorphologyNullMessages
+ (id)null;
@end

extern void objc_setProperty_nonatomic_copy(id self, SEL selector, id value, ptrdiff_t offset);

#define PRONOUN_KEY(text) {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, text, sizeof(text) - 1}

// Native constant strings, 0x22281ec30 through 0x22281ecb0. The object identities are shared with the
// required-keys array and the archiving and dictionary methods.
static const OCFConstantString subjectKey = PRONOUN_KEY("subjectForm");
static const OCFConstantString objectKey = PRONOUN_KEY("objectForm");
static const OCFConstantString possessiveKey = PRONOUN_KEY("possessiveForm");
static const OCFConstantString possessiveAdjectiveKey = PRONOUN_KEY("possessiveAdjectiveForm");
static const OCFConstantString reflexiveKey = PRONOUN_KEY("reflexiveForm");

#undef PRONOUN_KEY

// Native NSConstantArray at 0x222899d70; the object table starts at 0x22288bcb8.
static const id requiredKeys[] = {
    (id)&subjectKey, (id)&objectKey, (id)&possessiveKey, (id)&possessiveAdjectiveKey, (id)&reflexiveKey
};

static const struct {
    const void *isa;
    NSUInteger count;
    const id *objects;
} requiredKeysArray = {constantArrayClass, 5, requiredKeys};

@implementation ONSMorphologyCustomPronoun

// Native -[NSMorphologyCustomPronoun dealloc], 0x1c04c89d8. Non-external.
- (void)dealloc {
    objc_release(_subjectForm);
    objc_release(_objectForm);
    objc_release(_possessiveForm);
    objc_release(_possessiveAdjectiveForm);
    objc_release(_reflexiveForm);
    [super dealloc];
}

// Native -[NSMorphologyCustomPronoun hash], 0x1c04c8a6c. Non-external.
- (NSUInteger)hash {
    NSUInteger hash = [[self subjectForm] hash];
    hash ^= [[self objectForm] hash];
    hash ^= [[self possessiveForm] hash];
    hash ^= [[self possessiveAdjectiveForm] hash];
    return hash ^ [[self reflexiveForm] hash];
}

// Native -[NSMorphologyCustomPronoun isEqual:], 0x1c04c8ae0. Non-external.
- (BOOL)isEqual:(id)object {
    if (!objc_opt_isKindOfClass(object, [ONSMorphologyCustomPronoun class])) {
        return NO;
    }

    id lhs = [self subjectForm];
    id rhs = [object subjectForm];
    if (lhs != rhs && ![lhs isEqual:rhs]) {
        return NO;
    }
    lhs = [self objectForm];
    rhs = [object objectForm];
    if (lhs != rhs && ![lhs isEqual:rhs]) {
        return NO;
    }
    lhs = [self possessiveForm];
    rhs = [object possessiveForm];
    if (lhs != rhs && ![lhs isEqual:rhs]) {
        return NO;
    }
    lhs = [self possessiveAdjectiveForm];
    rhs = [object possessiveAdjectiveForm];
    if (lhs != rhs && ![lhs isEqual:rhs]) {
        return NO;
    }
    lhs = [self reflexiveForm];
    rhs = [object reflexiveForm];
    if (lhs == rhs) {
        return YES;
    }
    return [lhs isEqual:rhs];
}

// Native +[NSMorphologyCustomPronoun isSupportedForLanguage:], 0x1c04c8c14. Non-external.
+ (BOOL)isSupportedForLanguage:(id)language {
    if ([language length] < 2 || [[language substringToIndex:2]
                                     caseInsensitiveCompare:(id)OF_STR("en")] != 0) {
        return NO;
    }
    if ([language length] == 2 || [language characterAtIndex:2] == '-') {
        return YES;
    }
    return [language characterAtIndex:2] == '_';
}

// Native +[NSMorphologyCustomPronoun requiredKeysForLanguage:], 0x1c04c8ca4. Non-external.
+ (id)requiredKeysForLanguage:(id)language {
    if ([self isSupportedForLanguage:language]) {
        return (id)&requiredKeysArray;
    }
    return __ONSArray0__;
}

// Native -[NSMorphologyCustomPronoun copyWithZone:], 0x1c04c8cd0. Non-external.
// Foundation defect: verified at 0x1c04c8cf8. The first ivar store dereferences a nil result when a
// subclass initializer fails. Return nil before copying any forms in that case.
- (id)copyWithZone:(struct _NSZone *)zone {
    (void)zone;
    ONSMorphologyCustomPronoun *copy = objc_alloc_init(objc_opt_class(self));
    if (copy == nil) {
        return nil;
    }
    copy->_subjectForm = [[self subjectForm] copy];
    copy->_objectForm = [[self objectForm] copy];
    copy->_possessiveForm = [[self possessiveForm] copy];
    copy->_possessiveAdjectiveForm = [[self possessiveAdjectiveForm] copy];
    copy->_reflexiveForm = [[self reflexiveForm] copy];
    return copy;
}

// Native +[NSMorphologyCustomPronoun supportsSecureCoding], 0x1c04c8d50. Non-external.
// iOS Device 24A446: 0x180ee825c, verified same result.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSMorphologyCustomPronoun encodeWithCoder:], 0x1c04c8d58. Non-external.
- (void)encodeWithCoder:(ONSCoder *)coder {
    [coder encodeObject:_subjectForm forKey:(id)&subjectKey];
    [coder encodeObject:_objectForm forKey:(id)&objectKey];
    [coder encodeObject:_possessiveForm forKey:(id)&possessiveKey];
    [coder encodeObject:_possessiveAdjectiveForm forKey:(id)&possessiveAdjectiveKey];
    [coder encodeObject:_reflexiveForm forKey:(id)&reflexiveKey];
}

// Native -[NSMorphologyCustomPronoun initWithCoder:], 0x1c04c8dd8. Non-external.
- (id)initWithCoder:(ONSCoder *)coder {
    self = [super init];
    if (self != nil) {
        _subjectForm = [[coder decodeObjectOfClass:[ONSString class] forKey:(id)&subjectKey] copy];
        _objectForm = [[coder decodeObjectOfClass:[ONSString class] forKey:(id)&objectKey] copy];
        _possessiveForm = [[coder decodeObjectOfClass:[ONSString class] forKey:(id)&possessiveKey] copy];
        _possessiveAdjectiveForm = [[coder decodeObjectOfClass:[ONSString class]
                                                        forKey:(id)&possessiveAdjectiveKey] copy];
        _reflexiveForm = [[coder decodeObjectOfClass:[ONSString class] forKey:(id)&reflexiveKey] copy];
    }
    return self;
}

// Native -[NSMorphologyCustomPronoun subjectForm], 0x1c04c8f10. Non-external.
- (ONSString *)subjectForm {
    return _subjectForm;
}

// Native -[NSMorphologyCustomPronoun setSubjectForm:], 0x1c04c8f18. Non-external.
- (void)setSubjectForm:(ONSString *)value {
    objc_setProperty_nonatomic_copy(self, _cmd, value, 8);
}

// Native -[NSMorphologyCustomPronoun objectForm], 0x1c04c8f20. Non-external.
- (ONSString *)objectForm {
    return _objectForm;
}

// Native -[NSMorphologyCustomPronoun setObjectForm:], 0x1c04c8f28. Non-external.
- (void)setObjectForm:(ONSString *)value {
    objc_setProperty_nonatomic_copy(self, _cmd, value, 16);
}

// Native -[NSMorphologyCustomPronoun possessiveForm], 0x1c04c8f30. Non-external.
- (ONSString *)possessiveForm {
    return _possessiveForm;
}

// Native -[NSMorphologyCustomPronoun setPossessiveForm:], 0x1c04c8f38. Non-external.
- (void)setPossessiveForm:(ONSString *)value {
    objc_setProperty_nonatomic_copy(self, _cmd, value, 24);
}

// Native -[NSMorphologyCustomPronoun possessiveAdjectiveForm], 0x1c04c8f40. Non-external.
- (ONSString *)possessiveAdjectiveForm {
    return _possessiveAdjectiveForm;
}

// Native -[NSMorphologyCustomPronoun setPossessiveAdjectiveForm:], 0x1c04c8f48. Non-external.
- (void)setPossessiveAdjectiveForm:(ONSString *)value {
    objc_setProperty_nonatomic_copy(self, _cmd, value, 32);
}

// Native -[NSMorphologyCustomPronoun reflexiveForm], 0x1c04c8f50. Non-external.
- (ONSString *)reflexiveForm {
    return _reflexiveForm;
}

// Native -[NSMorphologyCustomPronoun setReflexiveForm:], 0x1c04c8f58. Non-external.
- (void)setReflexiveForm:(ONSString *)value {
    objc_setProperty_nonatomic_copy(self, _cmd, value, 40);
}

@end

@implementation ONSMorphologyCustomPronoun (NSExternalRepresentation)

// Native -[NSMorphologyCustomPronoun(NSExternalRepresentation) _initWithExternalRepresentationDictionary:],
// 0x1c04ca6d0. Non-external. Validate all five values before copying any.
- (id)_initWithExternalRepresentationDictionary:(id)dictionary {
    self = [super init];
    if (self == nil) {
        return nil;
    }

    id subject = [dictionary objectForKeyedSubscript:(id)&subjectKey];
    if (subject != nil && !_ONSIsNSString(subject)) {
        objc_release(self);
        return nil;
    }
    id object = [dictionary objectForKeyedSubscript:(id)&objectKey];
    if (object != nil && !_ONSIsNSString(object)) {
        objc_release(self);
        return nil;
    }
    id possessive = [dictionary objectForKeyedSubscript:(id)&possessiveKey];
    if (possessive != nil && !_ONSIsNSString(possessive)) {
        objc_release(self);
        return nil;
    }
    id possessiveAdjective = [dictionary objectForKeyedSubscript:(id)&possessiveAdjectiveKey];
    if (possessiveAdjective != nil && !_ONSIsNSString(possessiveAdjective)) {
        objc_release(self);
        return nil;
    }
    id reflexive = [dictionary objectForKeyedSubscript:(id)&reflexiveKey];
    if (reflexive != nil && !_ONSIsNSString(reflexive)) {
        objc_release(self);
        return nil;
    }
    _subjectForm = [subject copy];
    _objectForm = [object copy];
    _possessiveForm = [possessive copy];
    _possessiveAdjectiveForm = [possessiveAdjective copy];
    _reflexiveForm = [reflexive copy];
    return self;
}

// Native -[NSMorphologyCustomPronoun(NSExternalRepresentation) description], 0x1c04ca944. Non-external.
- (id)description {
    id description = [super description];
    id subject = [self subjectForm];
    id object = [self objectForm];
    id possessive = [self possessiveForm];
    id possessiveAdjective = [self possessiveAdjectiveForm];
    id reflexive = [self reflexiveForm];
    return [ONSString stringWithFormat:(id)OF_STR(
                                           "%@ { subject: '%@', object: '%@', possessive: '%@', possessive "
                                           "adjective: '%@', reflexive: '%@' }"
                                       ),
                                       description,
                                       subject,
                                       object,
                                       possessive,
                                       possessiveAdjective,
                                       reflexive];
}

@end

@implementation ONSMorphologyCustomPronoun (NSSupportedPronounValidation)

// Native +[NSMorphologyCustomPronoun(NSSupportedPronounValidation)
// _validateCustomPronounWithAccessor:forLanguage:error:], 0x1c04caa28. Non-external.
+ (BOOL)_validateCustomPronounWithAccessor:(id (^)(id key))accessor
                               forLanguage:(id)language
                                     error:(id *)error {
    id invalidValue = nil;
    if ([ONSMorphologyCustomPronoun isSupportedForLanguage:language]) {
        for (id key in [ONSMorphologyCustomPronoun requiredKeysForLanguage:language]) {
            id value = accessor(key);
            if (value == nil || value == [(id<MorphologyNullMessages>)nullClass null]) {
                invalidValue = key;
                break;
            }
        }
        if (invalidValue == nil) {
            return YES;
        }
    } else {
        invalidValue = (id)OF_STR("self");
    }

    if (error != NULL) {
        const id objects[] = {invalidValue};
        const id keys[] = {ONSInvalidValueErrorKey};
        id userInfo = [ONSDictionary dictionaryWithObjects:objects forKeys:keys count:1];
        *error = [ONSError errorWithDomain:ONSCocoaErrorDomain code:1024 userInfo:userInfo];
    }
    return NO;
}

@end

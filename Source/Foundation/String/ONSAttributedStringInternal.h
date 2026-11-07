//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSMutableAttributedString.h"
#import "Foundation/String/ONSAttributedStringAdditions.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Collections/OCFNSCFDictionary.h"

#include <dispatch/dispatch.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdbool.h>
#include <stdint.h>

// Private declarations for the attributed-string base and mutable units. iOS Simulator 24A434,
// Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified unless noted.
// iOS Device 24A446: unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md.

// Class-symbol aliases avoid importing the coordinator's placeholder interfaces. Native class metadata is
// declared in the owners' headers; other families' declarations are collected in ONSUnrestoredInterfaces.h.
extern char attributedStringBaseClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSAttributedString));
extern char attributedStringMutableBaseClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                         OF_NAME_STRING(ONSMutableAttributedString));
extern char attributedStringConcreteClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                      OF_NAME_STRING(ONSConcreteAttributedString));
extern char
    attributedStringMutableConcreteClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                     OF_NAME_STRING(ONSConcreteMutableAttributedString));
extern char attributedStringStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSString));
extern char attributedStringMutableStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                           OF_NAME_STRING(ONSMutableString));
extern char attributedStringExceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));
extern char attributedStringDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSDictionary));
extern char attributedStringMutableArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                          OF_NAME_STRING(ONSMutableArray));
extern char attributedStringDataClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSData));
extern char attributedStringMutableDataClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                         OF_NAME_STRING(ONSMutableData));
extern char attributedStringSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSSet));
extern char attributedStringKeyedUnarchiverClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                             OF_NAME_STRING(ONSKeyedUnarchiver));
extern char attributedStringErrorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSError));
extern char
    attributedStringProxyClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                           OF_NAME_STRING(ONSMutableStringProxyForMutableAttributedString));
extern char attributedStringConstantIntegerClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                             OF_NAME_STRING(ONSConstantIntegerNumber));

extern char attributedStringNumberClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSNumber));
extern char attributedStringArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSArray));
extern char attributedStringMutableDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                               OF_NAME_STRING(ONSMutableDictionary));
extern char attributedStringURLClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSURL));
extern char attributedStringMutableSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableSet));
// Round-2 Foundation families FA (attributed storage), FI (morphology) and FG (inflection rules).
extern char attributedStringPresentationIntentClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                                OF_NAME_STRING(ONSPresentationIntent));
extern char attributedStringMorphologyClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMorphology));
extern char attributedStringInflectionRuleClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                            OF_NAME_STRING(ONSInflectionRule));

// Native __NSReadMutableAttributedStringWithCoder, 0x1c04c4280, non-external; shared with concrete classes
// in the native unit. Hidden here to preserve that boundary when the coordinator restores those classes.
OF_EXTERN bool _NSReadMutableAttributedStringWithCoder(id coder, id result);

// Inlined range intersections, recovered from the base and mutable attributed-string methods. The first
// range's start has priority when both starts are contained; unsigned end wrapping follows the assembly.
static inline ONSRange attributedStringIntersectRanges(ONSRange first, ONSRange second) {
    NSUInteger firstEnd = first.location + first.length;
    NSUInteger secondEnd = second.location + second.length;
    NSUInteger end = firstEnd < secondEnd ? firstEnd : secondEnd;
    if (second.location <= first.location && first.location < secondEnd) {
        return (ONSRange){first.location, end - first.location};
    }
    if (first.location <= second.location && second.location < firstEnd) {
        return (ONSRange){second.location, end - second.location};
    }
    return (ONSRange){0, 0};
}

@protocol AttributedStringMessages <NSObject>
+ (id)stringWithFormat:(id)format, ...;
+ (id)stringWithCapacity:(NSUInteger)capacity;
+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
+ (id)dictionaryWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count;
+ (id)errorWithDomain:(id)domain code:(NSInteger)code userInfo:(id)userInfo;
+ (id)setWithObjects:(id)firstObject, ...;
+ (id)set;
- (id)initWithCapacity:(NSUInteger)capacity;
- (id)initWithMutableAttributedString:(id)string;
- (id)initWithAttributedString:(id)string;
- (id)initWithString:(id)string;
- (NSUInteger)length;
- (NSUInteger)count;
- (id)substringWithRange:(ONSRange)range;
- (id)objectForKey:(id)key;
- (id)objectForKeyedSubscript:(id)key;
- (id)objectAtIndex:(NSUInteger)index;
- (id)objectAtIndexedSubscript:(NSUInteger)index;
- (BOOL)isEqualToDictionary:(id)dictionary;
- (id)mutableCopyWithZone:(struct _NSZone *)zone;
- (id)copyWithZone:(struct _NSZone *)zone;
- (void)setObject:(id)object forKey:(id)key;
- (void)removeObjectForKey:(id)key;
- (void)addEntriesFromDictionary:(id)dictionary;
- (void)addObject:(id)object;
- (void)appendBytes:(const void *)bytes length:(NSUInteger)length;
- (void)replaceCharactersInRange:(ONSRange)range withString:(id)string;
- (BOOL)allowsKeyedCoding;
- (void)encodeObject:(id)object;
- (void)encodeObject:(id)object forKey:(id)key;
- (void)encodeValuesOfObjCTypes:(const char *)types, ...;
- (id)decodeObject;
- (id)decodeObjectOfClasses:(id)classes forKey:(id)key;
- (void)decodeValuesOfObjCTypes:(const char *)types, ...;
- (id)error;
- (id)allowedClasses;
- (BOOL)_strictSecureDecodingEnabled;
- (id)setByAddingObjectsFromSet:(id)set;
- (void)failWithError:(id)error;
- (const uint8_t *)bytes;
@end

// Selectors sent by the capture whose implementations are outside this family's scope. Declaration-only
// category: no replacement behavior or category method is emitted for these uncaptured initializers.
@interface ONSAttributedString (UncapturedInitializers)
- (id)initWithString:(id)string;
- (id)initWithAttributedString:(id)string;
@end

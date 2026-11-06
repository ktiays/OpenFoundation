//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSAttributedString.h"
#import "Foundation/String/ONSAttributedStringInternal.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#include "CoreFoundation/String/OCFAttributedString.h"
#include "CoreFoundation/String/OCFString.h"
#include "Foundation/Collections/ONSObjectArray.h"

#include <Block.h>
#include <dispatch/dispatch.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: NSAttributedString's base unit, including its cold abstract primitives. Read from
// Local/Evidence/cf-core/iphone-simulator-foundation-link.json, family FC-sets-attributed. iOS Simulator
// 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified unless noted.
// iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md. MRC: direct libobjc ownership calls
// follow the capture. The NSString archive key keeps its native spelling in both naming modes; only runtime
// class-name lookups use OF_NAME_STRING. Shared-cache bindings (confidence: medium): NSException 0x22227fae0,
// NSRangeException 0x22227f5e0, NSMutableArray 0x22227fb18, NSMutableData 0x22227fb20, NSSet 0x22227fb68,
// NSData 0x22227fab8, NSDictionary 0x22227fad0 and __NSDictionary0__struct 0x2222807b8. The last binding is
// confirmed as an imported symbol by nm; the extracted image has no bytes at these shared-cache addresses.

// Native __MergedGlobals.19 fields at 0x22e76c0e0, 0x22e76c0e8, 0x22e76c0f0.
static id attributedStringStrictDecodeClasses;
static id attributedStringDecodeClasses;
static dispatch_once_t attributedStringDecodeClassesOnceToken;

@implementation ONSAttributedString

// Native +[NSAttributedString allocWithZone:], 0x1c04c3150. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    Class cls = self;
    if (cls == (Class)objc_opt_self((id)attributedStringBaseClass)) {
        cls = (Class)objc_opt_self((id)attributedStringConcreteClass);
    }
    return ONSAllocateObject(cls, 0, zone);
}

// Native -[NSAttributedString attribute:atIndex:effectiveRange:], 0x1c04c31a0. Non-external.
- (id)attribute:(id)name atIndex:(NSUInteger)location effectiveRange:(ONSRange *)range {
    return [[self attributesAtIndex:location effectiveRange:range] objectForKey:name];
}

// Native -[NSAttributedString attributedSubstringFromRange:], 0x1c04c31cc. Non-external. The sum is
// unsigned, as are the native bound and full-length tests. The full-length path returns an autoreleased
// copy; the shorter path returns an autoreleased mutable concrete attributed string (kept).
- (id)attributedSubstringFromRange:(ONSRange)range {
    NSUInteger end = range.location + range.length;
    id string = [self string];
    NSUInteger length = [string length];
    if (end > length) {
        id reason = [(id)attributedStringStringClass stringWithFormat:(id)OF_STR("%s %s: Out of bounds"),
                                                                      _NSNameOfClass(objc_opt_class(self)),
                                                                      sel_getName(_cmd)];
        objc_exception_throw([(id)attributedStringExceptionClass exceptionWithName:(id)ONSRangeException
                                                                            reason:reason
                                                                          userInfo:nil]);
    }
    if (range.length == length) {
        return objc_autorelease([self copyWithZone:NULL]);
    }
    id substring = [string substringWithRange:range];
    id result = objc_autorelease([objc_allocWithZone((Class)attributedStringMutableConcreteClass)
        initWithString:substring]);
    NSUInteger location = range.location;
    while (location < end) {
        ONSRange effectiveRange = {0, 0};
        id attributes = [self attributesAtIndex:location effectiveRange:&effectiveRange];
        NSUInteger effectiveEnd = effectiveRange.location + effectiveRange.length;
        NSUInteger runLength = effectiveEnd > end ? end - location : effectiveEnd - location;
        if (effectiveRange.location != location || effectiveEnd > end) {
            effectiveRange.length = runLength;
        }
        [result addAttributes:attributes range:(ONSRange){location - range.location, runLength}];
        location += effectiveRange.length;
    }
    return result;
}

// Native -[NSAttributedString length], 0x1c04c34e0. Non-external.
- (NSUInteger)length {
    return [[self string] length];
}

// Native -[NSAttributedString hash], 0x1c04c34f4. Non-external.
- (NSUInteger)hash {
    return [[self string] hash];
}

// Native -[NSAttributedString isEqual:], 0x1c04c3508. Non-external.
- (BOOL)isEqual:(id)object {
    if ((objc_opt_isKindOfClass(object, objc_opt_self((id)attributedStringBaseClass)) & 1) == 0) {
        return NO;
    }
    return [self isEqualToAttributedString:object];
}

// Native -[NSAttributedString isEqualToAttributedString:], 0x1c04c355c. Non-external. Re-read a run only
// after crossing its end; compare dictionaries with isEqualToDictionary:, including a nil receiver.
- (BOOL)isEqualToAttributedString:(id)other {
    if (other == self) {
        return YES;
    }
    id string = [self string];
    id otherString = [other string];
    if (![string isEqual:otherString]) {
        return NO;
    }
    NSUInteger length = [self length];
    ONSRange ownRange = {0, 0};
    ONSRange otherRange = {0, 0};
    id ownAttributes = nil;
    id otherAttributes = nil;
    NSUInteger location = 0;
    while (location < length) {
        if (location >= ownRange.location + ownRange.length) {
            ownAttributes = [self attributesAtIndex:location effectiveRange:&ownRange];
        }
        if (location >= otherRange.location + otherRange.length) {
            otherAttributes = [other attributesAtIndex:location effectiveRange:&otherRange];
        }
        if (![ownAttributes isEqualToDictionary:otherAttributes]) {
            return NO;
        }
        NSUInteger ownEnd = ownRange.location + ownRange.length;
        NSUInteger otherEnd = otherRange.location + otherRange.length;
        location = otherEnd < ownEnd ? otherEnd : ownEnd;
    }
    return YES;
}

// Native -[NSAttributedString mutableCopyWithZone:], 0x1c04c3698. Non-external.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return [[(id)attributedStringMutableConcreteClass allocWithZone:zone] initWithAttributedString:self];
}

// Native -[NSAttributedString copyWithZone:], 0x1c04c36c4. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return [[(id)attributedStringConcreteClass allocWithZone:zone] initWithAttributedString:self];
}

// Native -[NSAttributedString description], 0x1c04c36f0. Non-external. Append each substring, then the
// description of its attributes, with two replaceCharactersInRange:withString: sends per run.
- (id)description {
    NSUInteger length = [self length];
    id result = [(id)attributedStringMutableStringClass stringWithCapacity:2 * length];
    ONSRange range = {0, 0};
    while (range.location < length) {
        id attributes = [self attributesAtIndex:range.location effectiveRange:&range];
        NSUInteger insertion = [result length];
        id substring = [[self string] substringWithRange:range];
        [result replaceCharactersInRange:(ONSRange){insertion, 0} withString:substring];
        insertion = [result length];
        [result replaceCharactersInRange:(ONSRange){insertion, 0} withString:[attributes description]];
        range.location += range.length;
    }
    return result;
}

// Native -[NSAttributedString attributesAtIndex:longestEffectiveRange:inRange:], 0x1c04c37f8.
// Non-external. Pass the caller's output pointer to the first lookup, then walk each side with a local
// range. Identity does not bypass the dictionary equality message here (kept).
- (id)attributesAtIndex:(NSUInteger)location longestEffectiveRange:(ONSRange *)range inRange:(ONSRange)limit {
    id attributes = [self attributesAtIndex:location effectiveRange:range];
    if (range != NULL) {
        ONSRange candidate = *range;
        NSUInteger limitEnd = limit.location + limit.length;
        NSUInteger end;
        do {
            end = candidate.location + candidate.length;
            if (end >= limitEnd) {
                break;
            }
        } while ([[self attributesAtIndex:end effectiveRange:&candidate] isEqualToDictionary:attributes]);
        candidate = *range;
        NSUInteger start;
        do {
            start = candidate.location;
            if (start <= limit.location) {
                break;
            }
        } while ([[self attributesAtIndex:start - 1
                           effectiveRange:&candidate] isEqualToDictionary:attributes]);
        *range = attributedStringIntersectRanges(limit, (ONSRange){start, end - start});
    }
    return attributes;
}

// Native -[NSAttributedString attribute:atIndex:longestEffectiveRange:inRange:], 0x1c04c391c.
// Non-external. Unlike dictionary runs, attribute runs accept identity, so adjacent nil values merge.
- (id)attribute:(id)name
                  atIndex:(NSUInteger)location
    longestEffectiveRange:(ONSRange *)range
                  inRange:(ONSRange)limit {
    id value = [self attribute:name atIndex:location effectiveRange:range];
    if (range != NULL) {
        ONSRange candidate = *range;
        NSUInteger limitEnd = limit.location + limit.length;
        NSUInteger end;
        id adjacent;
        do {
            end = candidate.location + candidate.length;
            if (end >= limitEnd) {
                break;
            }
            adjacent = [self attribute:name atIndex:end effectiveRange:&candidate];
        } while (adjacent == value || [adjacent isEqual:value]);
        candidate = *range;
        NSUInteger start;
        do {
            start = candidate.location;
            if (start <= limit.location) {
                break;
            }
            adjacent = [self attribute:name atIndex:start - 1 effectiveRange:&candidate];
        } while (adjacent == value || [adjacent isEqual:value]);
        *range = attributedStringIntersectRanges(limit, (ONSRange){start, end - start});
    }
    return value;
}

// Native -[NSAttributedString enumerateAttributesInRange:options:usingBlock:], 0x1c04c3a64.
// Non-external. Bit 1 reverses enumeration, bit 20 requests effective rather than longest ranges. Retain
// each dictionary across the block, release it on both normal and exception paths, then test stop's bit 0.
// Forward enumeration observes changes in the receiver's length after each callback; reverse does not.
- (void)enumerateAttributesInRange:(ONSRange)range
                           options:(NSUInteger)options
                        usingBlock:(void (^)(id, ONSRange, BOOL *))block {
    if (range.length == 0) {
        return;
    }
    NSUInteger location = range.location + ((options & 2) ? range.length - 1 : 0);
    BOOL stop = NO;
    NSUInteger previousLength = [self length];
    for (;;) {
        ONSRange effectiveRange = {0, 0};
        id attributes;
        if (options & 0x100000) {
            attributes = [self attributesAtIndex:location effectiveRange:&effectiveRange];
            effectiveRange = attributedStringIntersectRanges(effectiveRange, range);
        } else {
            attributes = [self attributesAtIndex:location
                           longestEffectiveRange:&effectiveRange
                                         inRange:range];
        }
        attributes = objc_retain(attributes);
        @try {
            block(attributes, effectiveRange, &stop);
        } @catch (id exception) {
            objc_release(attributes);
            @throw;
        }
        objc_release(attributes);
        if (stop & 1) {
            return;
        }
        if (options & 2) {
            if (effectiveRange.location <= range.location) {
                return;
            }
            location = effectiveRange.location - 1;
        } else {
            NSUInteger length = [self length];
            location = effectiveRange.location + effectiveRange.length;
            if (length != previousLength) {
                location = location - previousLength + length;
                range.length = range.length - previousLength + length;
                previousLength = length;
            }
            if (location >= range.location + range.length) {
                return;
            }
        }
    }
}

// Native -[NSAttributedString enumerateAttribute:inRange:options:usingBlock:], 0x1c04c3c54.
// Non-external. The ownership, exception cleanup, stop bit and mutation adjustment match the dictionary
// enumeration above; this is an independent native method with attribute-specific lookup sends.
- (void)enumerateAttribute:(id)name
                   inRange:(ONSRange)range
                   options:(NSUInteger)options
                usingBlock:(void (^)(id, ONSRange, BOOL *))block {
    if (range.length == 0) {
        return;
    }
    NSUInteger location = range.location + ((options & 2) ? range.length - 1 : 0);
    BOOL stop = NO;
    NSUInteger previousLength = [self length];
    for (;;) {
        ONSRange effectiveRange = {0, 0};
        id value;
        if (options & 0x100000) {
            value = [self attribute:name atIndex:location effectiveRange:&effectiveRange];
            effectiveRange = attributedStringIntersectRanges(effectiveRange, range);
        } else {
            value = [self attribute:name
                              atIndex:location
                longestEffectiveRange:&effectiveRange
                              inRange:range];
        }
        value = objc_retain(value);
        @try {
            block(value, effectiveRange, &stop);
        } @catch (id exception) {
            objc_release(value);
            @throw;
        }
        objc_release(value);
        if (stop & 1) {
            return;
        }
        if (options & 2) {
            if (effectiveRange.location <= range.location) {
                return;
            }
            location = effectiveRange.location - 1;
        } else {
            NSUInteger length = [self length];
            location = effectiveRange.location + effectiveRange.length;
            if (length != previousLength) {
                location = location - previousLength + length;
                range.length = range.length - previousLength + length;
                previousLength = length;
            }
            if (location >= range.location + range.length) {
                return;
            }
        }
    }
}

// Native +[NSAttributedString _subclassesMustBeExplicitlyMentionedWhenDecoded], 0x1c04c3e50.
// Non-external.
+ (BOOL)_subclassesMustBeExplicitlyMentionedWhenDecoded {
    return YES;
}

// Native -[NSAttributedString classForCoder], 0x1c04c3e58. Non-external.
- (Class)classForCoder {
    return (Class)objc_opt_self((id)attributedStringBaseClass);
}

// Native -[NSAttributedString encodeWithCoder:], 0x1c04c3e64. Non-external. Copy the first four callback
// words from kCFTypeDictionaryKeyCallBacks, then clear equal/hash: the dictionary indexes attribute object
// identities. Keyed streams use two unsigned varints per run; unkeyed streams use signed i / unsigned I.
- (void)encodeWithCoder:(id)coder {
    NSUInteger length = [self length];
    OCFDictionaryKeyCallBacks callbacks = kOCFTypeDictionaryKeyCallBacks;
    callbacks.equal = NULL;
    callbacks.hash = NULL;
    BOOL keyed = [coder allowsKeyedCoding];
    id string = [self string];
    if ((keyed & 1) == 0) {
        [coder encodeObject:string];
        if (length == 0) {
            return;
        }
        OCFMutableDictionaryRef indexes = OCFDictionaryCreateMutable(NULL, 0, &callbacks, NULL);
        NSUInteger location = 0;
        uint32_t nextIndex = 1;
        do {
            void *pool = objc_autoreleasePoolPush();
            ONSRange range = {0, 0};
            id attributes = [self attributesAtIndex:location
                              longestEffectiveRange:&range
                                            inRange:(ONSRange){location, length - location}];
            int32_t index = (int32_t)(uintptr_t)OCFDictionaryGetValue(indexes, attributes);
            uint32_t runLength = (uint32_t)range.length;
            if (index != 0) {
                [coder encodeValuesOfObjCTypes:"iI", &index, &runLength];
            } else {
                index = (int32_t)nextIndex;
                OCFDictionaryAddValue(indexes, attributes, (const void *)(intptr_t)index);
                [coder encodeValuesOfObjCTypes:"iI", &index, &runLength];
                [coder encodeObject:attributes];
                nextIndex++;
            }
            location += range.length;
            objc_autoreleasePoolPop(pool);
        } while (location < length);
        if (indexes != NULL) {
            OCFRelease(indexes);
        }
        return;
    }
    [coder encodeObject:string
                 forKey:(id)OF_STR(
                            "NS"
                            "String"
                        )];
    if (length == 0) {
        return;
    }
    ONSRange range = {0, 0};
    id attributes = [self attributesAtIndex:0 longestEffectiveRange:&range inRange:(ONSRange){0, length}];
    if (attributes == nil) {
        attributes = (id)&__ONSDictionary0__struct;
    }
    attributes = objc_retain(attributes);
    OCFMutableDictionaryRef indexes = NULL;
    if (range.length == length) {
        [coder encodeObject:attributes forKey:(id)OF_STR("NSAttributes")];
    } else {
        id array = [objc_alloc((Class)attributedStringMutableArrayClass) initWithCapacity:20];
        id data = [objc_alloc((Class)attributedStringMutableDataClass) initWithCapacity:100];
        indexes = OCFDictionaryCreateMutable(NULL, 0, &callbacks, NULL);
        NSUInteger location = 0;
        for (;;) {
            void *pool = objc_autoreleasePoolPush();
            const void *index = NULL;
            if (!OCFDictionaryGetValueIfPresent(indexes, attributes, &index)) {
                index = (const void *)(uintptr_t)[array count];
                OCFDictionaryAddValue(indexes, attributes, index);
                [array addObject:attributes];
            }
            uint8_t bytes[20];
            uint8_t *cursor = bytes;
            NSUInteger value = range.length;
            while (value >= 0x80) {
                *cursor++ = (uint8_t)(value | 0x80);
                value >>= 7;
            }
            *cursor++ = (uint8_t)value;
            value = (NSUInteger)index;
            while (value >= 0x80) {
                *cursor++ = (uint8_t)(value | 0x80);
                value >>= 7;
            }
            *cursor++ = (uint8_t)value;
            [data appendBytes:bytes length:(NSUInteger)(cursor - bytes)];
            location += range.length;
            if (location == length) {
                objc_autoreleasePoolPop(pool);
                break;
            }
            objc_release(attributes);
            attributes = [self attributesAtIndex:location
                           longestEffectiveRange:&range
                                         inRange:(ONSRange){location, length - location}];
            attributes = objc_retain(attributes != nil ? attributes : (id)&__ONSDictionary0__struct);
            objc_autoreleasePoolPop(pool);
        }
        [coder encodeObject:array forKey:(id)OF_STR("NSAttributes")];
        [coder encodeObject:data forKey:(id)OF_STR("NSAttributeInfo")];
        objc_release(array);
        objc_release(data);
    }
    objc_release(attributes);
    if (indexes != NULL) {
        OCFRelease(indexes);
    }
}

// Native -[NSAttributedString initWithCoder:], 0x1c04c4210. Non-external.
- (id)initWithCoder:(id)coder {
    id temporary = [objc_allocWithZone((Class)attributedStringMutableBaseClass) init];
    id result;
    if (_NSReadMutableAttributedStringWithCoder(coder, temporary)) {
        result = [self initWithAttributedString:temporary];
    } else {
        objc_release(self);
        result = nil;
    }
    objc_release(temporary);
    return result;
}

// Native __NSReadMutableAttributedStringWithCoder, 0x1c04c4280. Non-external (shared hidden boundary for
// the concrete attributed-string classes). The assembly supplies both NSReallocateObjectArray arguments
// and preserves a pointer result; Hex-Rays omits the count argument. The keyed parser checks addition
// carry, not multiplication overflow; its 64-bit MUL/LSL wrapping is retained.
// Foundation defect: at 0x1c04c4490-0x1c04c4498 the native caller overwrites its heap pointer on failed
// reallocation, leaking the old buffer. Free the old allocation before reporting failure; confidence:
// verified.
bool _NSReadMutableAttributedStringWithCoder(id coder, id result) {
    BOOL keyed = [coder allowsKeyedCoding];
    id string;
    if (keyed != 0) {
        Class immutable = objc_opt_class((id)attributedStringStringClass);
        Class mutable = objc_opt_class((id)attributedStringMutableStringClass);
        id classes = [(id)attributedStringSetClass setWithObjects:immutable, mutable, nil];
        string = [coder decodeObjectOfClasses:classes
                                       forKey:(id)OF_STR(
                                                  "NS"
                                                  "String"
                                              )];
    } else {
        string = [coder decodeObject];
    }
    if ([coder error] != nil) {
        return false;
    }
    if (string == nil) {
        string = (id)OF_STR("");
    }
    NSUInteger length = [string length];
    [result replaceCharactersInRange:(ONSRange){0, 0} withString:string];
    if (length == 0) {
        return true;
    }
    if ((keyed & 1) == 0) {
        NSUInteger location = 0;
        NSUInteger capacity = 20;
        id stackObjects[20] = {nil};
        id *objects = stackObjects;
        do {
            int32_t index = 0;
            uint32_t runLength = 0;
            [coder decodeValuesOfObjCTypes:"iI", &index, &runLength];
            if ([coder error] != nil) {
                if (objects != stackObjects) {
                    free(objects);
                }
                return false;
            }
            if (index < 1) {
                break;
            }
            if (capacity <= (uint32_t)index) {
                NSUInteger doubled = 2 * capacity;
                NSUInteger newCapacity = doubled > (uint32_t)index ? doubled : (NSUInteger)index + 1;
                if (objects == stackObjects) {
                    objects = ONSAllocateObjectArray(newCapacity);
                    if (objects == NULL) {
                        break;
                    }
                    memmove(objects, stackObjects, sizeof(id) * capacity);
                } else {
                    id *allocation = ONSReallocateObjectArray(objects, newCapacity);
                    if (allocation == NULL) {
                        free(objects);
                        objects = NULL;
                        break;
                    }
                    objects = allocation;
                    bzero(objects + capacity, sizeof(id) * (newCapacity - capacity));
                }
                capacity = newCapacity;
                objects[index] = [coder decodeObject];
                if ([coder error] != nil) {
                    free(objects);
                    return false;
                }
            } else if (objects[index] == nil) {
                objects[index] = [coder decodeObject];
                if ([coder error] != nil) {
                    if (objects != stackObjects) {
                        free(objects);
                    }
                    return false;
                }
            }
            [result setAttributes:objects[index] range:(ONSRange){location, runLength}];
            location += runLength;
        } while (location < length);
        if (objects != NULL && objects != stackObjects) {
            free(objects);
        }
        return location >= length;
    }
    // Native ____NSReadMutableAttributedStringWithCoder_block_invoke, 0x1c04c7b64. Non-external.
    // Shared-cache GOT class bindings, confidence: medium. NSMutableSet at 0x22227fb38,
    // NSArray at 0x22227fa90, NSMutableArray at 0x22227fb18, NSDictionary at 0x22227fad0,
    // NSMutableDictionary at 0x22227fb28, NSData at 0x22227fab8 and NSURL at 0x22227fb88.
    dispatch_once(&attributedStringDecodeClassesOnceToken, ^{
      Class number = objc_opt_class((id)attributedStringNumberClass);
      Class array = objc_opt_class((id)attributedStringArrayClass);
      Class mutableArray = objc_opt_class((id)attributedStringMutableArrayClass);
      Class dictionary = objc_opt_class((id)attributedStringDictionaryClass);
      Class mutableDictionary = objc_opt_class((id)attributedStringMutableDictionaryClass);
      Class data = objc_opt_class((id)attributedStringDataClass);
      Class url = objc_opt_class((id)attributedStringURLClass);
      Class string = objc_opt_class((id)attributedStringStringClass);
      Class presentationIntent = objc_opt_class((id)attributedStringPresentationIntentClass);
      Class morphology = objc_opt_class((id)attributedStringMorphologyClass);
      Class inflectionRule = objc_opt_class((id)attributedStringInflectionRuleClass);
      attributedStringStrictDecodeClasses =
          objc_retain([(id)attributedStringMutableSetClass setWithObjects:number,
                                                                          array,
                                                                          mutableArray,
                                                                          dictionary,
                                                                          mutableDictionary,
                                                                          data,
                                                                          url,
                                                                          string,
                                                                          presentationIntent,
                                                                          morphology,
                                                                          inflectionRule,
                                                                          nil]);

      const char *const optionalClassNames[] = {
          "NSParagraphStyle",
          "NSGlyphInfo",
          "NSTextAlternatives",
          "NSTextAttachment",
          "NSShadow",
          "NSAdaptiveImageGlyph",
          "NSFont",
          "UIFont",
          "NSColor",
          "UIColor"
      };
      for (size_t index = 0; index < sizeof(optionalClassNames) / sizeof(optionalClassNames[0]); ++index) {
          Class cls = objc_getClass(optionalClassNames[index]);
          if (cls != Nil) {
              [attributedStringStrictDecodeClasses addObject:cls];
          }
      }
      attributedStringDecodeClasses = [attributedStringStrictDecodeClasses mutableCopy];
      Class encapsulation = objc_getClass("NSTextEncapsulation");
      if (encapsulation != Nil) {
          [attributedStringDecodeClasses addObject:encapsulation];
      }
    });
    id classes = [coder allowedClasses];
    if (classes == nil) {
        classes = [(id)attributedStringSetClass set];
    }
    id permitted;
    if ((objc_opt_isKindOfClass(coder, (id)attributedStringKeyedUnarchiverClass) & 1) != 0 &&
        ([coder _strictSecureDecodingEnabled] & 1) != 0) {
        permitted = attributedStringStrictDecodeClasses;
    } else {
        permitted = attributedStringDecodeClasses;
    }
    classes = [classes setByAddingObjectsFromSet:permitted];
    id attributes = [coder decodeObjectOfClasses:classes forKey:(id)OF_STR("NSAttributes")];
    if ([coder error] != nil) {
        return false;
    }
    Class immutableData = objc_opt_class((id)attributedStringDataClass);
    Class mutableData = objc_opt_class((id)attributedStringMutableDataClass);
    classes = [(id)attributedStringSetClass setWithObjects:immutableData, mutableData, nil];
    id info = [coder decodeObjectOfClasses:classes forKey:(id)OF_STR("NSAttributeInfo")];
    if ([coder error] != nil) {
        return false;
    }
    if (attributes == nil) {
        return true;
    }
    BOOL validClass = info == nil ? _ONSIsNSDictionary(attributes) : _ONSIsNSArray(attributes);
    if ((validClass & 1) == 0) {
        id actualClass = ONSStringFromClass(objc_opt_class(attributes));
        id description;
        if (info == nil) {
            description = [(id)attributedStringStringClass
                stringWithFormat:(id)OF_STR(
                                     "-[NSAttributedString initWithCoder:] expected to decode attributes as "
                                     "an NSDictionary "
                                     "but found %@ instead"
                                 ),
                                 actualClass];
        } else {
            description = [(id)attributedStringStringClass
                stringWithFormat:
                    (id)OF_STR(
                        "-[NSAttributedString initWithCoder:] expected to decode attributes as an NSArray "
                        "but found %@ instead"
                    ),
                    actualClass];
        }
        id key = ONSLocalizedDescriptionKey;
        id dictionary = [(id)attributedStringDictionaryClass dictionaryWithObjects:&description
                                                                           forKeys:&key
                                                                             count:1];
        [coder failWithError:[(id)attributedStringErrorClass errorWithDomain:ONSCocoaErrorDomain
                                                                        code:4864
                                                                    userInfo:dictionary]];
        return false;
    }
    if (info == nil) {
        [result setAttributes:attributes range:(ONSRange){0, length}];
        return true;
    }
    const uint8_t *cursor = [info bytes];
    const uint8_t *end = cursor + [info length];
    NSUInteger location = 0;
    do {
        if (cursor == end) {
            return false;
        }
        NSUInteger runLength = 0;
        NSUInteger multiplier = 1;
        for (;;) {
            uint8_t byte = *cursor++;
            NSUInteger component = multiplier * (NSUInteger)(byte & 0x7f);
            if (__builtin_add_overflow(runLength, component, &runLength)) {
                return false;
            }
            if ((byte & 0x80) == 0) {
                break;
            }
            multiplier <<= 7;
            if (cursor == end) {
                return false;
            }
        }
        if (cursor == end) {
            return false;
        }
        NSUInteger index = 0;
        multiplier = 1;
        for (;;) {
            uint8_t byte = *cursor++;
            NSUInteger component = multiplier * (NSUInteger)(byte & 0x7f);
            if (__builtin_add_overflow(index, component, &index)) {
                return false;
            }
            if ((byte & 0x80) == 0) {
                break;
            }
            multiplier <<= 7;
            if (cursor == end) {
                return false;
            }
        }
        [result setAttributes:[attributes objectAtIndex:index] range:(ONSRange){location, runLength}];
        location += runLength;
    } while (location < length);
    return true;
}

// Native +[NSAttributedString supportsSecureCoding], 0x1c04c4814. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSAttributedString string], 0x1c0fe2098. Non-external; the native abstract method, not a stub.
- (id)string {
    Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSAttributedString)));
    ONSRequestConcreteImplementation(self, _cmd, cls);
    return nil;
}

// Native -[NSAttributedString attributesAtIndex:effectiveRange:], 0x1c0fe20d8. Non-external; abstract.
- (id)attributesAtIndex:(NSUInteger)location effectiveRange:(ONSRange *)range {
    (void)location;
    (void)range;
    Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSAttributedString)));
    ONSRequestConcreteImplementation(self, _cmd, cls);
    return nil;
}

@end

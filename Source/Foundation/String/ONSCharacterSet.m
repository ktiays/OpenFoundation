//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSMutableCharacterSet.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <Block.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdint.h>
#include <stdlib.h>

// Class objects referenced by direct libobjc class queries in the native capture.
extern char runtimeONSMutableCharacterSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                           OF_NAME_STRING(ONSMutableCharacterSet));

// Inferred organization: the NSCharacterSet methods lie at 0x1c04d89ec-0x1c04d94ef and the
// NSMutableCharacterSet methods at 0x1c04d9590-0x1c04d9d4f of the iOS Simulator Foundation __text section
// (the methods outside the scope between them are not restored); the categories NSURLUtilities
// (+URLPathAllowedCharacterSet, 0x1c05ebac8) and NSCharacterSetPortCoding (-replacementObjectForPortCoder:,
// 0x1c064e42c) lie in other native units and are restored here after the classes, as categories with their
// native names. The unit also defines the shared statics and block of the inline feature check
// _foundation_swift_characterSet_enabled (String/ONSCharacterSet.h). Objective-C compiled with manual
// reference counting (objc_retain, objc_release, objc_autorelease; no ARC entry points). Evidence:
// Local/Evidence/cf-core/iphone-simulator-foundation-link.json (family FC-sets-attributed). Unless a
// definition says otherwise: iOS Simulator 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md.
//
// Class references through the shared cache's GOT (slots outside the extracted image, identified from their
// use; confidence: medium): 0x22227FAE0 NSException (the receiver of
// +exceptionWithName:reason:userInfo:), 0x22227F4E0 the NSGenericException pointer (the exception name of
// the decoding failures; the slots appear sorted by symbol name, with 0x22227F500 NSInvalidArgumentException
// and 0x22227F5E0 NSRangeException after it), 0x22227FAD0 NSDictionary, 0x22227FAB8 NSData and 0x22227FB20
// NSMutableData. The selectors at 0x2387AD760 (-isEqual:'s respondsToSelector: argument), 0x2387AF76F
// (-[NSMutableCharacterSet initWithCoder:]'s super send) and 0x2387D34A3 (the port coding super send) lie
// outside the image; they are read as bitmapRepresentation, initWithCoder: and replacementObjectForPortCoder:
// from their use (confidence: medium for the first, high for the super sends of the methods' own selectors).

extern char swiftCharacterSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(_ONSSwiftCharacterSet));
extern char placeholderCharacterSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                     OF_NAME_STRING(_ONSPlaceholderCharacterSet));
extern char characterSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSCharacterSet));
extern char keyedArchiverClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSKeyedArchiver));
extern char keyedUnarchiverClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSKeyedUnarchiver));
extern char errorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSError));
extern char stringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSString));
extern char exceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));
extern char dictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSDictionary));
extern char dataClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSData));
extern char mutableDataClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableData));

// The messages the methods send to objects of other classes (the Swift-backed set, the placeholder, coders,
// data, strings, errors, exceptions and dictionaries), declared for the sends only; no protocol object is
// emitted.
@protocol CharacterSetMessages <NSObject>

- (id)initWithCharactersInString:(id)aString mutable:(BOOL)isMutable;
- (id)initWithRange:(ONSRange)range mutable:(BOOL)isMutable;
- (id)initWithSet:(id)set options:(NSUInteger)options;
- (id)initWithLength:(NSUInteger)length;
- (void *)mutableBytes;
- (const void *)bytes;
- (NSUInteger)length;
- (BOOL)allowsKeyedCoding;
- (void)_encodePropertyList:(id)propertyList forKey:(id)key;
- (void)encodeObject:(id)object forKey:(id)key;
- (NSInteger)decodeIntegerForKey:(id)key;
- (void)encodeBool:(BOOL)value forKey:(id)key;
- (void)encodeInt64:(int64_t)value forKey:(id)key;
- (void)encodeInteger:(NSInteger)value forKey:(id)key;
- (BOOL)decodeBoolForKey:(id)key;
- (BOOL)containsValueForKey:(id)key;
- (int64_t)decodeInt64ForKey:(id)key;
- (id)_decodePropertyListForKey:(id)key;
- (id)decodeObjectOfClass:(Class)aClass forKey:(id)key;
- (void)failWithError:(id)error;
- (BOOL)isByref;
+ (id)errorWithDomain:(id)domain code:(NSInteger)code userInfo:(id)userInfo;
+ (id)dictionaryWithObject:(id)object forKey:(id)key;
+ (id)dictionaryWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count;
+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
+ (id)stringWithFormat:(id)format, ...;
+ (id)_swiftURLPathAllowedCharacterSet;

@end

// NSObject's port coding method, Foundation's (NSObject(NSDistributedObjects)), declared for the super send
// of -[NSCharacterSet(NSCharacterSetPortCoding) replacementObjectForPortCoder:] only.

// The layout of a global block literal and of its descriptor (Block ABI; the type names and field names are
// local descriptive spellings, as in CoreFoundation's Locale/OCFDateFormatter.c).
typedef struct {
    uintptr_t reserved;
    uintptr_t size;
    const char *signature;
    const char *layout;
} CharacterSetGlobalBlockDescriptor;

typedef struct {
    void *isa;
    int32_t flags;
    int32_t reserved;
    void (*invoke)(void *block);
    const CharacterSetGlobalBlockDescriptor *descriptor;
} CharacterSetGlobalBlockLiteral;

OF_ABI_ASSERT(sizeof(CharacterSetGlobalBlockLiteral) == 32, "A global block literal is 32 bytes.");

// Native ___block_descriptor_32_e5_v8?0l, 0x22281c8b8 (__DATA_CONST,__const): {0x50000000, 32, "v8@?0"
// (0x1c142970b), no layout}. Static: only the literal below uses it.
static const CharacterSetGlobalBlockDescriptor characterSetGlobalBlockDescriptor = {
    0x50000000, 32, "v8@?0", NULL
};

// Native _foundation_swift_characterSet_enabled.onceToken and .featureEnabled (see the header); hidden
// definitions under their native names.
__attribute__((visibility("hidden"))) dispatch_once_t foundationSwiftCharacterSetOnceToken;
__attribute__((visibility("hidden"))) uint8_t foundationSwiftCharacterSetFeatureEnabled;

// Native ____foundation_swift_characterSet_enabled_block_invoke, 0x1c0fdc9cc.
// Non-external (was a private external).
OF_EXTERN void foundationSwiftCharacterSetEnabledInvoke(void *block) __asm(
    OF_SYMBOL("___foundation_swift_characterSet_enabled_block_invoke")
);

void foundationSwiftCharacterSetEnabledInvoke(void *block) {
    (void)block;
    bool enabled = _os_feature_enabled_impl("Foundation", "SwiftNSCharacterSet");
    if (!enabled) {
        enabled = getenv("SwiftNSCharacterSet") != NULL;
    }
    foundationSwiftCharacterSetFeatureEnabled = enabled;
}

// Native ___block_literal_global (IDA ___block_literal_global_10), 0x22281c8d8.
// Shared by the character set and the NSURLUtilities category.
__attribute__((visibility("hidden"))) const CharacterSetGlobalBlockLiteral
    foundationSwiftCharacterSetEnabledLiteral __asm(OF_SYMBOL("__block_literal_global_10")) = {
        _NSConcreteGlobalBlock,
        0x50000000,
        0,
        foundationSwiftCharacterSetEnabledInvoke,
        &characterSetGlobalBlockDescriptor
};

// The log every abstract method of the cluster makes before -doesNotRecognizeSelector: (the constant string
// cfstr_AbstractClassD, "Abstract class %@ does not implement method %s; it must be implemented in
// subclasses.", read as cfstr_AbstractClassD), with the receiver's class (objc_opt_class) and the selector's
// name. abstractCharacterSetMethod is a local descriptive name for the expansion each method repeats.
static inline void abstractCharacterSetMethod(id object, SEL selector) {
    Class objectClass = objc_opt_class((id)(object));
    const char *name = sel_getName(selector);
    ONSLog(
        (id)OF_STR("Abstract class %@ does not implement method %s; it must be implemented in subclasses."),
        objectClass,
        name
    );
    [object doesNotRecognizeSelector:selector];
}

@implementation ONSCharacterSet

// Native -[NSCharacterSet _cfTypeID], 0x1c04d89ec.
- (unsigned long)_cfTypeID {
    return OCFCharacterSetGetTypeID();
}

// Native -[NSCharacterSet _expandedCFCharacterSet], 0x1c04d89f0.
- (OCFCharacterSetRef)_expandedCFCharacterSet {
    return NULL;
}

// Native +[NSCharacterSet whitespaceCharacterSet], 0x1c04d8a00 (kCFCharacterSetWhitespace, 2).
+ (id)whitespaceCharacterSet {
    return (id)OCFCharacterSetGetPredefined(kOCFCharacterSetWhitespace);
}

// Native +[NSCharacterSet whitespaceAndNewlineCharacterSet], 0x1c04d8a08
// (kCFCharacterSetWhitespaceAndNewline, 3).
+ (id)whitespaceAndNewlineCharacterSet {
    return (id)OCFCharacterSetGetPredefined(kOCFCharacterSetWhitespaceAndNewline);
}

// Native +[NSCharacterSet decimalDigitCharacterSet], 0x1c04d8a10 (kCFCharacterSetDecimalDigit, 4).
+ (id)decimalDigitCharacterSet {
    return (id)OCFCharacterSetGetPredefined(kOCFCharacterSetDecimalDigit);
}

// Native +[NSCharacterSet lowercaseLetterCharacterSet], 0x1c04d8a20. Non-external.
+ (id)lowercaseLetterCharacterSet {
    return (id)OCFCharacterSetGetPredefined(kOCFCharacterSetLowercaseLetter);
}

// Native +[NSCharacterSet alphanumericCharacterSet], 0x1c04d8a30 (kCFCharacterSetAlphaNumeric, 10).
+ (id)alphanumericCharacterSet {
    return (id)OCFCharacterSetGetPredefined(kOCFCharacterSetAlphaNumeric);
}

// Native +[NSCharacterSet characterSetWithCharactersInString:], 0x1c04d8b9c. With the Swift feature, a
// _NSSwiftCharacterSet (objc_alloc) initialized immutable; else CFCharacterSetCreateWithCharactersInString
// with the default allocator. Autoreleased either way.
+ (id)characterSetWithCharactersInString:(id)aString {
    id set;
    if (foundationSwiftCharacterSetEnabled()) {
        set = [(id<CharacterSetMessages>)objc_alloc((Class)((Class)swiftCharacterSetClass))
            initWithCharactersInString:aString
                               mutable:NO];
    } else {
        set = (id)OCFCharacterSetCreateWithCharactersInString(NULL, (OCFStringRef)aString);
    }
    return objc_autorelease((id)(set));
}

// Native -[NSCharacterSet init], 0x1c04d8c94. An instance of exactly NSCharacterSet (the class symbol, not
// objc_opt_class) is released and replaced by an empty set: a _NSSwiftCharacterSet over {0, 0}, immutable,
// with the Swift feature, else CFCharacterSetCreateWithCharactersInRange(NULL, {0, 0}). Subclasses keep self.
- (id)init {
    if ([self isMemberOfClass:(Class)characterSetClass]) {
        id set;
        if (foundationSwiftCharacterSetEnabled()) {
            set = [(id<CharacterSetMessages>)objc_alloc((Class)((Class)swiftCharacterSetClass))
                initWithRange:(ONSRange){0, 0}
                      mutable:NO];
        } else {
            set = (id)OCFCharacterSetCreateWithCharactersInRange(NULL, (OCFRange){0, 0});
        }
        objc_release((id)(self));
        return set;
    }
    return self;
}

// Native -[NSCharacterSet hash], 0x1c04d8d38.
- (NSUInteger)hash {
    return [[self bitmapRepresentation] hash];
}

// Native -[NSCharacterSet copyWithZone:], 0x1c04d8d4c: objc_retain of self.
- (id)copyWithZone:(struct _NSZone *)zone {
    (void)zone;
    return objc_retain((id)(self));
}

// Native -[NSCharacterSet invertedSet], 0x1c04d8d50. A _NSPlaceholderCharacterSet (objc_allocWithZone)
// initialized with self and option 1 (inverted), autoreleased.
- (id)invertedSet {
    return objc_autorelease((id)([(id<CharacterSetMessages>)
            objc_allocWithZone((Class)((Class)placeholderCharacterSetClass)) initWithSet:self
                                                                                 options:1]));
}

// Native -[NSCharacterSet isEmpty], 0x1c04d8d84. YES for no bitmap or a length below 1 (a signed
// comparison); otherwise whether every byte of the bitmap is zero, scanned from the start.
- (BOOL)isEmpty {
    id bitmap = [self bitmapRepresentation];
    if (bitmap == nil) {
        return YES;
    }
    const uint8_t *bytes = [(id<CharacterSetMessages>)bitmap bytes];
    NSInteger length = (NSInteger)[(id<CharacterSetMessages>)bitmap length];
    if (length < 1) {
        return YES;
    }
    for (NSInteger remaining = length - 1;; remaining--) {
        uint8_t byte = *bytes++;
        if (byte != 0) {
            return NO;
        }
        if (remaining == 0) {
            return YES;
        }
    }
}

// Native -[NSCharacterSet count], 0x1c04d8de4. The population count (CNT/ADDV) of the bitmap's first 0x2000
// bytes; 0 when the bitmap has no bytes.
- (NSUInteger)count {
    const uint8_t *bytes = [(id<CharacterSetMessages>)[self bitmapRepresentation] bytes];
    NSUInteger count = 0;
    if (bytes != NULL) {
        for (NSUInteger index = 0; index != 0x2000; index++) {
            NSUInteger bits;
            if (bytes[index] != 0) {
                bits = (NSUInteger)__builtin_popcount(bytes[index]);
            } else {
                bits = 0;
            }
            count += bits;
        }
    }
    return count;
}

// Native -[NSCharacterSet isMutable], 0x1c04d8e3c.
- (BOOL)isMutable {
    return NO;
}

// Native -[NSCharacterSet makeImmutable], 0x1c04d8e44.
- (void)makeImmutable {
}

// Native -[NSCharacterSet characterIsMember:], 0x1c04d8e48. Abstract.
- (BOOL)characterIsMember:(unsigned short)aCharacter {
    (void)aCharacter;
    abstractCharacterSetMethod(self, _cmd);
    return NO;
}

// Native -[NSCharacterSet longCharacterIsMember:], 0x1c04d8ea8. The character truncated to 16 bits.
- (BOOL)longCharacterIsMember:(uint32_t)theLongChar {
    return [self characterIsMember:(unsigned short)theLongChar];
}

// Native -[NSCharacterSet isSupersetOfSet:], 0x1c04d8eb0. Abstract.
- (BOOL)isSupersetOfSet:(id)theOtherSet {
    (void)theOtherSet;
    abstractCharacterSetMethod(self, _cmd);
    return NO;
}

// Native -[NSCharacterSet isEqual:], 0x1c04d8f70. Identity answers YES; an object that does not respond to
// the selector 0x2387AD760 (read as bitmapRepresentation) answers NO; otherwise the bitmaps' -isEqual:.
- (BOOL)isEqual:(id)object {
    if (object == self) {
        return YES;
    }
    if (objc_opt_respondsToSelector((id)(object), (@selector(bitmapRepresentation)))) {
        return [[self bitmapRepresentation] isEqual:[object bitmapRepresentation]];
    }
    return NO;
}

// Native -[NSCharacterSet bitmapRepresentation], 0x1c04d8fe0. An NSMutableData of 0x2000 zero bytes
// (objc_allocWithZone, -initWithLength:) with bit (c & 7) of byte c >> 3 set for each member c in 0..0xFFFE
// (the loop stops before 0xFFFF), autoreleased.
- (id)bitmapRepresentation {
    id data = [(id<CharacterSetMessages>)objc_allocWithZone((Class)((Class)mutableDataClass))
        initWithLength:0x2000];
    uint8_t *bytes = [(id<CharacterSetMessages>)data mutableBytes];
    for (uint16_t character = 0; character != 0xFFFF; character++) {
        if ([self characterIsMember:character]) {
            bytes[character >> 3] |= (uint8_t)(1 << (character & 7));
        }
    }
    return objc_autorelease((id)(data));
}

// Native -[NSCharacterSet mutableCopyWithZone:], 0x1c04d9070. Abstract; answers nil.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    (void)zone;
    abstractCharacterSetMethod(self, _cmd);
    return nil;
}

// Native -[NSCharacterSet encodeWithCoder:], 0x1c04d90d0. The bitmap is taken first; a coder that does not
// allow keyed coding gets nothing. An NSKeyedArchiver (object_getClass compared with the class) stores it as
// the property list "NSBitmap"; another keyed coder as the object "NSBitmapObject".
- (void)encodeWithCoder:(id)coder {
    id bitmap = [self bitmapRepresentation];
    if (![(id<CharacterSetMessages>)coder allowsKeyedCoding]) {
        return;
    }
    if (object_getClass(coder) == (Class)keyedArchiverClass) {
        [(id<CharacterSetMessages>)coder _encodePropertyList:bitmap forKey:(id)OF_STR("NSBitmap")];
    } else {
        [(id<CharacterSetMessages>)coder encodeObject:bitmap forKey:(id)OF_STR("NSBitmapObject")];
    }
}

// Native -[NSCharacterSet initWithCoder:], 0x1c04d9150. Self is always released; the result is a new
// CoreFoundation set (or nil).
// The keys "NSString" and "NSRange" are spelled as two concatenated literals so that the prefix tool does not
// read them as the runtime names of the mapped NSString class and NSRange type: they are archive keys, equal
// in both naming modes.
// - A coder without keyed coding answers nil.
// - "NSBuiltinID" (else "NSBuiltinID2" when 0) in 1...15 selects a predefined set; inverted ("NSIsInverted",
//   else "NSIsInverted2") it is a mutable copy marked inverted, otherwise the predefined set retained
//   (CFRetain before the NULL check, as natively).
// - Otherwise "NSRange" (a 64-bit value: location in the high 32 bits, arithmetic shift, length in the low 32
//   bits), checked with _CFCharacterSetIsValidRange; then a string ("NSString" as a property list when
//   present or when the coder is an NSKeyedUnarchiver, else the object "NSStringObject" of class NSString);
//   then a bitmap ("NSBitmap" as a property list for an NSKeyedUnarchiver or when present, else the object
//   "NSBitmapObject" of class NSData). The decoded set is marked inverted when "NSIsInverted" was set.
// - A string or bitmap that is missing or of the wrong type fails the coder with NSCocoaErrorDomain 4865
//   (NSCoderValueNotFoundError) and the description "Decoder value not found".
// - A failed creation throws NSGenericException (GOT slot 0x22227F4E0, medium confidence) with the native
//   reason, after self is released.
- (id)initWithCoder:(id)coder {
    id<CharacterSetMessages> decoder = (id<CharacterSetMessages>)coder;
    id result;
    id reason;
    if (![decoder allowsKeyedCoding]) {
        result = nil;
        goto done;
    }
    NSInteger builtinID = [decoder decodeIntegerForKey:(id)OF_STR("NSBuiltinID")];
    BOOL inverted = [decoder decodeBoolForKey:(id)OF_STR("NSIsInverted")];
    if (builtinID == 0) {
        builtinID = [decoder decodeIntegerForKey:(id)OF_STR("NSBuiltinID2")];
    }
    Class coderClass = object_getClass(coder);
    BOOL hasString = [decoder containsValueForKey:(id)OF_STR(
                                                      "NS"
                                                      "String"
                                                  )];
    BOOL hasStringObject = [decoder containsValueForKey:(id)OF_STR("NSStringObject")];
    if ((NSUInteger)(builtinID - 1) <= 0xE) {
        OCFCharacterSetRef predefined = OCFCharacterSetGetPredefined((OCFCharacterSetPredefinedSet)builtinID);
        BOOL markInverted;
        if (inverted || [decoder decodeBoolForKey:(id)OF_STR("NSIsInverted2")]) {
            predefined = OCFCharacterSetCreateMutableCopy(NULL, predefined);
            markInverted = YES;
        } else {
            OCFRetain(predefined);
            markInverted = NO;
        }
        if (predefined == NULL) {
            objc_release((id)(self));
            reason = (id)OF_STR("CFCharacterSetGetPredefined -- Predefined Character Set not found");
            goto raise;
        }
        result = (id)predefined;
        if (!markInverted) {
            goto done;
        }
        goto invert;
    }
    OCFCharacterSetRef created;
    if ([decoder containsValueForKey:(id)OF_STR(
                                         "NS"
                                         "Range"
                                     )]) {
        int64_t value = [decoder decodeInt64ForKey:(id)OF_STR(
                                                       "NS"
                                                       "Range"
                                                   )];
        OCFIndex location = (OCFIndex)(value >> 32);
        OCFIndex length = (OCFIndex)(uint32_t)value;
        if (!_OCFCharacterSetIsValidRange(location, length)) {
            objc_release((id)(self));
            reason = (id)OF_STR(
                "CFCharacterSetCreateWithCharactersInRange -- Character Set creation with characters in "
                "range "
                "failed: Invalid range"
            );
            goto raise;
        }
        created = OCFCharacterSetCreateWithCharactersInRange(NULL, (OCFRange){location, length});
        if (created == NULL) {
            objc_release((id)(self));
            reason = (id)OF_STR(
                "CFCharacterSetCreateWithCharactersInRange -- Character Set creation with characters in "
                "range "
                "failed"
            );
            goto raise;
        }
    } else if (hasString || hasStringObject) {
        id string;
        if (coderClass == (Class)keyedUnarchiverClass || hasString) {
            string = [decoder _decodePropertyListForKey:(id)OF_STR(
                                                            "NS"
                                                            "String"
                                                        )];
        } else {
            string = [decoder decodeObjectOfClass:objc_opt_class((id)((Class)stringClass))
                                           forKey:(id)OF_STR("NSStringObject")];
        }
        if (string == nil || !_ONSIsNSString(string)) {
            goto notFound;
        }
        created = OCFCharacterSetCreateWithCharactersInString(NULL, (OCFStringRef)string);
        if (created == NULL) {
            objc_release((id)(self));
            reason = (id)OF_STR(
                "CFCharacterSetCreateWithCharactersInString -- Character Set creation with characters in "
                "string "
                "failed"
            );
            goto raise;
        }
    } else {
        id bitmap;
        if (coderClass == (Class)keyedUnarchiverClass ||
            [decoder containsValueForKey:(id)OF_STR("NSBitmap")]) {
            bitmap = [decoder _decodePropertyListForKey:(id)OF_STR("NSBitmap")];
        } else {
            bitmap = [decoder decodeObjectOfClass:objc_opt_class((id)((Class)dataClass))
                                           forKey:(id)OF_STR("NSBitmapObject")];
        }
        if (bitmap == nil || !_ONSIsNSData(bitmap)) {
            goto notFound;
        }
        created = OCFCharacterSetCreateWithBitmapRepresentation(NULL, (OCFDataRef)bitmap);
        if (created == NULL) {
            objc_release((id)(self));
            reason = (id)OF_STR(
                "CFCharacterSetCreateWithBitmapRepresentation -- Character Set creation with bitmap "
                "representation failed"
            );
            goto raise;
        }
    }
    result = (id)created;
    if (!inverted) {
        goto done;
    }
invert:
    _OCFCharacterSetSetIsInverted((OCFMutableCharacterSetRef)result, true);
    goto done;
notFound:
    [decoder failWithError:[(id<CharacterSetMessages>)(Class)errorClass
                               errorWithDomain:ONSCocoaErrorDomain
                                          code:4865
                                      userInfo:[(id<CharacterSetMessages>)(Class)dictionaryClass
                                                   dictionaryWithObject:(id)OF_STR("Decoder value not found")
                                                                 forKey:ONSLocalizedDescriptionKey]]];
    result = nil;
done:
    objc_release((id)(self));
    return result;
raise:
    objc_exception_throw([(id<CharacterSetMessages>)(Class)exceptionClass
        exceptionWithName:(id)ONSGenericException
                   reason:reason
                 userInfo:nil]);
}

// Native +[NSCharacterSet supportsSecureCoding], 0x1c04d94d4.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSCharacterSet _retainedBitmapRepresentation], 0x1c04d94dc.
- (id)_retainedBitmapRepresentation {
    return objc_retain((id)([self bitmapRepresentation]));
}

@end

@implementation ONSMutableCharacterSet

// Native +[NSMutableCharacterSet characterSetWithCharactersInString:], 0x1c04d9590. With the Swift feature, a
// _NSSwiftCharacterSet initialized mutable; else CFCharacterSetCreateMutable(NULL) followed by
// CFCharacterSetAddCharactersInString. Autoreleased.
+ (id)characterSetWithCharactersInString:(id)aString {
    id set;
    if (foundationSwiftCharacterSetEnabled()) {
        set = [(id<CharacterSetMessages>)objc_alloc((Class)((Class)swiftCharacterSetClass))
            initWithCharactersInString:aString
                               mutable:YES];
    } else {
        set = (id)OCFCharacterSetCreateMutable(NULL);
        OCFCharacterSetAddCharactersInString((OCFMutableCharacterSetRef)set, (OCFStringRef)aString);
    }
    return objc_autorelease((id)(set));
}

// Native init.__NSMutableCharacterSetClass, 0x22e6fca90 (__DATA,__bss, non-external): -init's cache of
// objc_opt_class(NSMutableCharacterSet), filled on first use without synchronization.
static Class mutableCharacterSetClassCache;

// Native -[NSMutableCharacterSet init], 0x1c04d961c. An instance of exactly NSMutableCharacterSet is released
// and replaced by an empty mutable set: a _NSSwiftCharacterSet over {0, 0}, mutable, with the Swift feature,
// else CFCharacterSetCreateMutable(NULL).
- (id)init {
    if (mutableCharacterSetClassCache == Nil) {
        mutableCharacterSetClassCache = objc_opt_class((id)((id)runtimeONSMutableCharacterSetClass));
    }
    if ([self isMemberOfClass:mutableCharacterSetClassCache]) {
        id set;
        if (foundationSwiftCharacterSetEnabled()) {
            set = [(id<CharacterSetMessages>)objc_alloc((Class)((Class)swiftCharacterSetClass))
                initWithRange:(ONSRange){0, 0}
                      mutable:YES];
        } else {
            set = (id)OCFCharacterSetCreateMutable(NULL);
        }
        objc_release((id)(self));
        return set;
    }
    return self;
}

// Native -[NSMutableCharacterSet copyWithZone:], 0x1c04d96d4. The mutable copy (with the same zone),
// made immutable.
- (id)copyWithZone:(struct _NSZone *)zone {
    id copy = [self mutableCopyWithZone:zone];
    [copy makeImmutable];
    return copy;
}

// Native -[NSMutableCharacterSet mutableCopyWithZone:], 0x1c04d96fc. Abstract; answers nil.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    (void)zone;
    abstractCharacterSetMethod(self, _cmd);
    return nil;
}

// Native -[NSMutableCharacterSet addCharactersInRange:], 0x1c04d975c. Abstract.
- (void)addCharactersInRange:(ONSRange)aRange {
    (void)aRange;
    abstractCharacterSetMethod(self, _cmd);
}

// Native -[NSMutableCharacterSet removeCharactersInRange:], 0x1c04d97b4. Abstract.
- (void)removeCharactersInRange:(ONSRange)aRange {
    (void)aRange;
    abstractCharacterSetMethod(self, _cmd);
}

// Native -[NSMutableCharacterSet addCharactersInString:], 0x1c04d980c. Abstract.
- (void)addCharactersInString:(id)aString {
    (void)aString;
    abstractCharacterSetMethod(self, _cmd);
}

// Native -[NSMutableCharacterSet removeCharactersInString:], 0x1c04d9864. Abstract.
- (void)removeCharactersInString:(id)aString {
    (void)aString;
    abstractCharacterSetMethod(self, _cmd);
}

// Native -[NSMutableCharacterSet formUnionWithCharacterSet:], 0x1c04d98bc. Abstract.
- (void)formUnionWithCharacterSet:(id)otherSet {
    (void)otherSet;
    abstractCharacterSetMethod(self, _cmd);
}

// Native -[NSMutableCharacterSet formIntersectionWithCharacterSet:], 0x1c04d9914. Abstract.
- (void)formIntersectionWithCharacterSet:(id)otherSet {
    (void)otherSet;
    abstractCharacterSetMethod(self, _cmd);
}

// Native -[NSMutableCharacterSet invert], 0x1c04d996c. Abstract.
- (void)invert {
    abstractCharacterSetMethod(self, _cmd);
}

// Native -[NSMutableCharacterSet isMutable], 0x1c04d99c4.
- (BOOL)isMutable {
    return YES;
}

// Native +[NSMutableCharacterSet whitespaceCharacterSet], 0x1c04d99f0. NSCharacterSet's set (sent to the
// class, not self) copied mutably with a NULL zone, autoreleased.
+ (id)whitespaceCharacterSet {
    return objc_autorelease((id)([[ONSCharacterSet whitespaceCharacterSet] mutableCopyWithZone:NULL]));
}

// Native +[NSMutableCharacterSet whitespaceAndNewlineCharacterSet], 0x1c04d9a14. As +whitespaceCharacterSet.
+ (id)whitespaceAndNewlineCharacterSet {
    return objc_autorelease((id)([[ONSCharacterSet whitespaceAndNewlineCharacterSet]
        mutableCopyWithZone:NULL]));
}

// Native +[NSMutableCharacterSet decimalDigitCharacterSet], 0x1c04d9a38. As +whitespaceCharacterSet.
+ (id)decimalDigitCharacterSet {
    return objc_autorelease((id)([[ONSCharacterSet decimalDigitCharacterSet] mutableCopyWithZone:NULL]));
}

// Native +[NSMutableCharacterSet alphanumericCharacterSet], 0x1c04d9aec. As +whitespaceCharacterSet.
+ (id)alphanumericCharacterSet {
    return objc_autorelease((id)([[ONSCharacterSet alphanumericCharacterSet] mutableCopyWithZone:NULL]));
}

// Native +[NSMutableCharacterSet lowercaseLetterCharacterSet], 0x1c04d9a80. Non-external.
+ (id)lowercaseLetterCharacterSet {
    ONSCharacterSet *set = [(id)characterSetClass lowercaseLetterCharacterSet];
    return objc_autorelease([set mutableCopyWithZone:NULL]);
}

// Native -[NSMutableCharacterSet initWithCoder:], 0x1c04d9c30. NSCharacterSet's decoding (super, selector
// 0x2387AF76F), then a mutable copy (NULL zone); the decoded set is released either way. A failed copy fails
// the coder with NSCocoaErrorDomain 4864 (NSCoderReadCorruptError) and the description "Unable to create
// mutable copy of the character set %p!" (the decoded set's address, formatted before its release).
- (id)initWithCoder:(id)coder {
    id decoded = [super initWithCoder:coder];
    id copy = [decoded mutableCopyWithZone:NULL];
    if (copy != nil) {
        objc_release((id)(decoded));
    } else {
        id description = [(id<CharacterSetMessages>)(Class)stringClass
            stringWithFormat:(id)OF_STR("Unable to create mutable copy of the character set %p!"), decoded];
        objc_release((id)(decoded));
        id key = ONSLocalizedDescriptionKey;
        id value = description;
        [(id<CharacterSetMessages>)coder
            failWithError:[(id<CharacterSetMessages>)(Class)errorClass
                              errorWithDomain:ONSCocoaErrorDomain
                                         code:4864
                                     userInfo:[(id<CharacterSetMessages>)(Class)dictionaryClass
                                                  dictionaryWithObjects:&value
                                                                forKeys:&key
                                                                  count:1]]];
    }
    return copy;
}

// Native +[NSMutableCharacterSet supportsSecureCoding], 0x1c04d9d48.
+ (BOOL)supportsSecureCoding {
    return YES;
}

@end

// Additional character-set classes from iphone-simulator-foundation-link-delta1.json, FA-attributed-storage.
// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.
extern char foundationCFCharacterSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                      OF_NAME_STRING(ONSCFCharacterSet));

@implementation ONSCFCharacterSet

// Native -[NSCFCharacterSet isEqual:], 0x1c04d9d50. Non-external.
- (BOOL)isEqual:(id)object {
    if (object == nil) {
        return NO;
    }
    return object == self || _OCFNonObjCEqual((OCFTypeRef)self, (OCFTypeRef)object) != 0;
}

// Native -[NSCFCharacterSet hash], 0x1c04d9d8c. Non-external.
- (NSUInteger)hash {
    return _OCFNonObjCHash((OCFTypeRef)self);
}

// Native -[NSCFCharacterSet retain], 0x1c04d9d90. Non-external. A tail call to _CFNonObjCRetain.
- (id)retain {
    return (id)_OCFNonObjCRetain(self);
}

// Native -[NSCFCharacterSet release], 0x1c04d9d94. Non-external.
- (oneway void)release {
    _OCFNonObjCRelease(self);
}

// Native -[NSCFCharacterSet allowsWeakReference], 0x1c04d9d98. Non-external.
- (BOOL)allowsWeakReference {
    return _OCFIsDeallocating(self) == 0;
}

// Native -[NSCFCharacterSet retainWeakReference], 0x1c04d9db4. Non-external.
- (BOOL)retainWeakReference {
    return _OCFTryRetain(self) != NULL;
}

// Native -[NSCFCharacterSet retainCount], 0x1c04d9dd0. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)OCFGetRetainCount(self);
}

// Native -[NSCFCharacterSet copyWithZone:], 0x1c04d9ddc. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return (id)OCFCharacterSetCreateCopy(NULL, (OCFCharacterSetRef)self);
}

// Native -[NSCFCharacterSet mutableCopyWithZone:], 0x1c04d9de8. Non-external.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return (id)OCFCharacterSetCreateMutableCopy(NULL, (OCFCharacterSetRef)self);
}

// Native -[NSCFCharacterSet characterIsMember:], 0x1c04d9df4. Non-external.
- (BOOL)characterIsMember:(unsigned short)character {
    return OCFCharacterSetIsCharacterMember((OCFCharacterSetRef)self, character) != 0;
}

// Native -[NSCFCharacterSet longCharacterIsMember:], 0x1c04d9e14. Non-external.
- (BOOL)longCharacterIsMember:(uint32_t)character {
    return OCFCharacterSetIsLongCharacterMember((OCFCharacterSetRef)self, character) != 0;
}

// Native -[NSCFCharacterSet isSupersetOfSet:], 0x1c04d9e34. Non-external.
- (BOOL)isSupersetOfSet:(id)other {
    return OCFCharacterSetIsSupersetOfSet((OCFCharacterSetRef)self, (OCFCharacterSetRef)other) != 0;
}

// Native -[NSCFCharacterSet bitmapRepresentation], 0x1c04d9e74. Non-external.
- (id)bitmapRepresentation {
    return objc_autorelease((id)OCFCharacterSetCreateBitmapRepresentation(NULL, (OCFCharacterSetRef)self));
}

// Native -[NSCFCharacterSet invertedSet], 0x1c04d9e90. Non-external.
- (id)invertedSet {
    return objc_autorelease((id)OCFCharacterSetCreateInvertedSet(NULL, (OCFCharacterSetRef)self));
}

// Native -[NSCFCharacterSet invert], 0x1c04d9ee4. Non-external.
- (void)invert {
    OCFCharacterSetInvert((OCFMutableCharacterSetRef)self);
}

// Native -[NSCFCharacterSet classForArchiver], 0x1c04d9ef0. Non-external.
- (Class)classForArchiver {
    BOOL mutable = _OCFCharacterSetIsMutable((OCFCharacterSetRef)self) != 0;
    return (Class)objc_opt_self((id)(mutable ? runtimeONSMutableCharacterSetClass : characterSetClass));
}

// Native -[NSCFCharacterSet classForKeyedArchiver], 0x1c04d9f20. Non-external.
- (Class)classForKeyedArchiver {
    return [self classForArchiver];
}

// Native -[NSCFCharacterSet encodeWithCoder:], 0x1c04d9f24. Non-external.
- (void)encodeWithCoder:(id)coder {
    id<CharacterSetMessages> encoder = coder;
    if (![encoder allowsKeyedCoding]) {
        return;
    }
    OCFCharacterSetRef set = (OCFCharacterSetRef)self;
    Boolean inverted = _OCFCharacterSetIsInverted(set);
    OCFIndex type = _OCFCharacterSetGetKeyedCodingType(set);
    switch (type) {
        case 2:
            [encoder encodeInteger:_OCFCharacterSetGetKeyedCodingBuiltinType(set)
                            forKey:(id)OF_STR("NSBuiltinID")];
            break;
        case 3: {
            OCFRange range = _OCFCharacterSetGetKeyedCodingRange(set);
            [encoder encodeInt64:(int64_t)((uint64_t)range.location << 32 | (uint64_t)range.length)
                          forKey:(id)OF_STR(
                                     "NS"
                                     "Range"
                                 )];
            break;
        }
        case 4: {
            OCFStringRef string = _OCFCharacterSetCreateKeyedCodingString(set);
            if (object_getClass(coder) == (Class)keyedArchiverClass) {
                [encoder _encodePropertyList:(id)string
                                      forKey:(id)OF_STR(
                                                 "NS"
                                                 "String"
                                             )];
            } else {
                [encoder encodeObject:(id)string forKey:(id)OF_STR("NSStringObject")];
            }
            OCFRelease(string);
            break;
        }
        case 5:
            [encoder encodeInteger:_OCFCharacterSetGetKeyedCodingBuiltinType(set)
                            forKey:(id)OF_STR("NSBuiltinID2")];
            if (inverted) {
                [encoder encodeBool:YES forKey:(id)OF_STR("NSIsInverted2")];
            }
            __attribute__((fallthrough));
        default: {
            Class cls = object_getClass(coder);
            id bitmap = [self bitmapRepresentation];
            if (cls == (Class)keyedArchiverClass) {
                [encoder _encodePropertyList:bitmap forKey:(id)OF_STR("NSBitmap")];
            } else {
                [encoder encodeObject:bitmap forKey:(id)OF_STR("NSBitmapObject")];
            }
            return;
        }
    }
    if (inverted) {
        [encoder encodeBool:YES forKey:(id)OF_STR("NSIsInverted")];
    }
}

@end

static inline ONSCharacterSet *loadInvertedCharacterSet(ONSCharacterSet **slot) {
    ONSCharacterSet *set;
    __atomic_load(slot, &set, __ATOMIC_ACQUIRE);
    return set;
}

// libobjc's weak-reference support messages of NSObject, declared for the super sends only.
@interface ONSCharacterSet (RuntimeReferenceMessages)
- (BOOL)_isDeallocating;
- (BOOL)_tryRetain;
@end

@implementation _ONSPlaceholderCharacterSet

// Native -[_NSPlaceholderCharacterSet initWithSet:options:], 0x1c04da0e8. Non-external.
- (id)initWithSet:(id)set options:(NSUInteger)options {
    self = [super init];
    _flags._inverted = options & 1;
    _flags._builtin = (options >> 1) & 1;
    _flags._isCF = !_flags._inverted && objc_opt_isKindOfClass(set, (id)foundationCFCharacterSetClass);
    if (_flags._inverted && [(ONSCharacterSet *)set isMutable]) {
        ONSMutableCharacterSet *copy = [(ONSCharacterSet *)set mutableCopyWithZone:NULL];
        [copy invert];
        [copy makeImmutable];
        objc_release(self);
        return (id)copy;
    }
    _original = objc_retain(set);
    return self;
}

// Native -[_NSPlaceholderCharacterSet _expandedCFCharacterSet], 0x1c04da1ec. Non-external.
- (OCFCharacterSetRef)_expandedCFCharacterSet {
    return _flags._isCF ? (OCFCharacterSetRef)_original : NULL;
}

// Native -[_NSPlaceholderCharacterSet _expandInverted], 0x1c04da204. Non-external.
// Foundation defect: the initial ordinary load of _invertedSet races with the CASAL publication.
// Use atomic pointer accesses throughout the placeholder while preserving the CAS winner and ownership.
- (void)_expandInverted {
    if (_flags._inverted && loadInvertedCharacterSet(&_invertedSet) == nil) {
        ONSCharacterSet *copy = [_original mutableCopyWithZone:NULL];
        [(ONSMutableCharacterSet *)copy invert];
        ONSCharacterSet *expected = nil;
        if (!__atomic_compare_exchange(
                &_invertedSet, &expected, &copy, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            )) {
            objc_release(copy);
        }
    }
}

// Native -[_NSPlaceholderCharacterSet mutableCopyWithZone:], 0x1c04da274. Non-external.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    ONSCharacterSet *inverted = loadInvertedCharacterSet(&_invertedSet);
    if (inverted != nil) {
        return [inverted mutableCopyWithZone:NULL];
    }
    ONSMutableCharacterSet *copy = [_original mutableCopyWithZone:NULL];
    if (_flags._inverted) {
        [copy invert];
    }
    return copy;
}

// Native -[_NSPlaceholderCharacterSet characterIsMember:], 0x1c04da2cc. Non-external.
- (BOOL)characterIsMember:(unsigned short)character {
    return [self longCharacterIsMember:character];
}

// Native -[_NSPlaceholderCharacterSet longCharacterIsMember:], 0x1c04da2d0. Non-external.
- (BOOL)longCharacterIsMember:(uint32_t)character {
    return (OCFCharacterSetIsLongCharacterMember((OCFCharacterSetRef)_original, character) != 0) ^
           _flags._inverted;
}

// Native -[_NSPlaceholderCharacterSet isSupersetOfSet:], 0x1c04da30c. Non-external.
- (BOOL)isSupersetOfSet:(id)other {
    ONSCharacterSet *inverted = loadInvertedCharacterSet(&_invertedSet);
    if (inverted != nil) {
        return OCFCharacterSetIsSupersetOfSet((OCFCharacterSetRef)inverted, (OCFCharacterSetRef)other) != 0;
    }
    BOOL contains =
        OCFCharacterSetIsSupersetOfSet((OCFCharacterSetRef)_original, (OCFCharacterSetRef)other) != 0;
    if (!_flags._inverted) {
        return contains;
    }
    if (contains) {
        return NO;
    }
    [self _expandInverted];
    inverted = loadInvertedCharacterSet(&_invertedSet);
    return OCFCharacterSetIsSupersetOfSet((OCFCharacterSetRef)inverted, (OCFCharacterSetRef)other) != 0;
}

// Native -[_NSPlaceholderCharacterSet isEqual:], 0x1c04da3d0. Non-external.
- (BOOL)isEqual:(id)object {
    if (self == object) {
        return YES;
    }
    OCFCharacterSetRef set = (OCFCharacterSetRef)_original;
    if (_flags._isCF && set == [(ONSCharacterSet *)object _expandedCFCharacterSet]) {
        return YES;
    }
    if (_flags._inverted) {
        [self _expandInverted];
        set = (OCFCharacterSetRef)loadInvertedCharacterSet(&_invertedSet);
    }
    return OCFEqual(set, (OCFTypeRef)object) != 0;
}

// Native -[_NSPlaceholderCharacterSet bitmapRepresentation], 0x1c04da454. Non-external.
- (id)bitmapRepresentation {
    ONSCharacterSet *set;
    if (_flags._inverted) {
        [self _expandInverted];
        set = loadInvertedCharacterSet(&_invertedSet);
    } else {
        set = _original;
    }
    return objc_autorelease((id)OCFCharacterSetCreateBitmapRepresentation(NULL, (OCFCharacterSetRef)set));
}

// Native -[_NSPlaceholderCharacterSet isEmpty], 0x1c04da498. Non-external.
// A non-inverted placeholder always answers NO (kept).
- (BOOL)isEmpty {
    if (!_flags._inverted || [_original isEmpty]) {
        return NO;
    }
    [self _expandInverted];
    return [loadInvertedCharacterSet(&_invertedSet) isEmpty];
}

// Native -[_NSPlaceholderCharacterSet invertedSet], 0x1c04da4e4. Non-external.
- (id)invertedSet {
    return _flags._inverted ? _original : [_original invertedSet];
}

// Native -[_NSPlaceholderCharacterSet autorelease], 0x1c04da4fc. Non-external.
- (id)autorelease {
    return _flags._builtin ? self : [super autorelease];
}

// Native -[_NSPlaceholderCharacterSet retain], 0x1c04da564. Non-external.
- (id)retain {
    return _flags._builtin ? self : [super retain];
}

// Native -[_NSPlaceholderCharacterSet retainCount], 0x1c04da5cc. Non-external.
- (NSUInteger)retainCount {
    return _flags._builtin ? NSUIntegerMax : [super retainCount];
}

// Native -[_NSPlaceholderCharacterSet release], 0x1c04da63c. Non-external.
- (oneway void)release {
    if (!_flags._builtin) {
        [super release];
    }
}

// Native -[_NSPlaceholderCharacterSet _isDeallocating], 0x1c04da6a4. Non-external. A builtin set is never
// deallocating.
- (BOOL)_isDeallocating {
    return _flags._builtin ? NO : [super _isDeallocating];
}

// Native -[_NSPlaceholderCharacterSet _tryRetain], 0x1c04da714. Non-external. A builtin set always retains.
- (BOOL)_tryRetain {
    return _flags._builtin ? YES : [super _tryRetain];
}

// Native -[_NSPlaceholderCharacterSet dealloc], 0x1c04da784. Non-external.
- (void)dealloc {
    if (!_flags._builtin) {
        objc_release(_original);
        objc_release(loadInvertedCharacterSet(&_invertedSet));
        [super dealloc];
    }
}

// Native -[_NSPlaceholderCharacterSet replacementObjectForKeyedArchiver:], 0x1c04da808. Non-external.
- (id)replacementObjectForKeyedArchiver:(id)archiver {
    if (_flags._inverted) {
        [self _expandInverted];
        return loadInvertedCharacterSet(&_invertedSet);
    }
    return _original;
}

// Native -[_NSPlaceholderCharacterSet replacementObjectForCoder:], 0x1c04da844. Non-external.
- (id)replacementObjectForCoder:(id)coder {
    if (_flags._inverted) {
        [self _expandInverted];
        return loadInvertedCharacterSet(&_invertedSet);
    }
    return _original;
}

@end

@implementation ONSCharacterSet (NSURLUtilities)

// Native +[NSCharacterSet(NSURLUtilities) URLPathAllowedCharacterSet], 0x1c05ebac8. With the Swift feature,
// +[NSCharacterSet _swiftURLPathAllowedCharacterSet] (sent to the class, not self); else
// _CFURLComponentsGetURLPathAllowedCharacterSet().
+ (id)URLPathAllowedCharacterSet {
    if (foundationSwiftCharacterSetEnabled()) {
        return [(id<CharacterSetMessages>)(id)characterSetClass _swiftURLPathAllowedCharacterSet];
    }
    return (id)_OCFURLComponentsGetURLPathAllowedCharacterSet();
}

@end

@implementation ONSCharacterSet (NSCharacterSetPortCoding)

// Native -[NSCharacterSet(NSCharacterSetPortCoding) replacementObjectForPortCoder:], 0x1c064e42c. Sent by
// reference (-isByref) and mutable: NSObject's replacement (super, selector 0x2387D34A3); otherwise self.
- (id)replacementObjectForPortCoder:(id)coder {
    if ([(id<CharacterSetMessages>)coder isByref] && [self isMutable]) {
        return [super replacementObjectForPortCoder:coder];
    }
    return self;
}

@end

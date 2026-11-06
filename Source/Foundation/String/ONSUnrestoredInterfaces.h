//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSRegularExpression.h"

#import "CoreFoundation/CharacterSet/OCFCharacterSet.h"
#import "CoreFoundation/Objective-C/OCFNSObjectGlue.h"
#import "CoreFoundation/String/OCFString.h"

#import "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#import "CoreFoundation/String/OCFString.h"
#import "CoreFoundation/String/OCFStringTokenizer.h"
#import "CoreFoundation/URL/OCFURLAllowedCharacters.h"
#import "CoreFoundation/URL/OCFURLPathExtension.h"
#import "Foundation/Number/ONSFormatter.h"
#import "Foundation/Number/ONSJSONRoundTrippingNumber.h"
#import "Foundation/Number/ONSNumberFormatter.h"
#import "Foundation/String/ONSGrammaticalExternalRepresentation.h"
#import "Foundation/String/ONSJSONSerialization.h"
#import "Foundation/String/ONSMorphologyCustomPronoun.h"

#import "Foundation/Number/ONSDecimalNumber.h"

#import "Foundation/Archiving/ONSCoder.h"
#import "Foundation/Archiving/ONSKeyedArchiver.h"
#import "Foundation/Error/ONSError.h"
#import "Foundation/Error/ONSErrorConstants.h"
#import "Foundation/FileManager/ONSFileManager.h"
#import "Foundation/Number/ONSNumber.h"
#import "Foundation/Number/ONSValue.h"
#import "Foundation/Scanner/ONSScanner.h"
#import "Foundation/String/ONSAttributedString.h"
#import "Foundation/String/ONSAttributedStringInflection.h"
#import "Foundation/String/ONSJSONSerialization.h"
#import "CoreFoundation/Objective-C/OCFNSOutputStream.h"
#import "Foundation/String/ONSCharacterSet.h"
#import "Foundation/String/ONSCheapMutableString.h"
#import "Foundation/String/ONSLocalizedString.h"
#import "Foundation/String/ONSPathStore2.h"

#include "CoreFoundation/Collections/OCFDictionary.h"
#include "Foundation/Error/ONSErrorConstants.h"

#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/Locale/OCFLocale.h"
#include "CoreFoundation/String/OCFString.h"
#include <OpenFoundation/Base/ONSObjCRuntime.h>

#import "Foundation/String/ONSString.h"
#import "Foundation/String/ONSMutableString.h"
#include <OpenFoundation/Decimal/ONSDecimal.h>
#include <objc/objc.h>
#include <dispatch/dispatch.h>
#include <OpenFoundation/Range/ONSRange.h>
#include "CoreFoundation/CharacterSet/OCFCharacterSet.h"

// Temporary interfaces needed by FS-string from other restoration families. The coordinator replaces these
// declarations with their owners' headers at integration. These are declarations, never replacement bodies.
// Reference: iOS Simulator 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 unless stated.
// CoreFoundation UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC. iOS Device 24A446: unread.

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native NSCache, CoreFoundation class import through Foundation GOT 0x22227FAA8 in the initializer
// ____createRegexForPattern_block_invoke, 0x1c063fe88 (binding confidence: medium). Message-only interface;
// no ivars or class implementation. The CoreFoundation cache family supplies its class object.
@interface ONSCache : NSObject
- (void)setName:(id)name;
- (void)setCountLimit:(ONSUInteger)limit;
- (id)objectForKey:(id)key;
- (void)setObject:(id)object forKey:(id)key;
@end

// Messages into the shared locale and character-set families from 0x1c05c18d0 and 0x1c05ebbc4. These are
// only method declarations; no protocol object is emitted.
@protocol StringSharedDependencyMessages
- (id)decimalSeparator;
- (id)__copySwiftStringByAddingPercentEncodingWithAllowedCharacters:(id)allowedCharacters;
@end
#endif

#ifdef __OBJC__
// Foundation classes outside this family that the restored code references by name (class symbols and runtime
// names), so that the prefix tool maps them. Those not restored on this branch keep undefined class symbols.
@class ONSArray;
@class ONSAttributedString;
@class ONSCharacterSet;
@class ONSCheapMutableString;
@class ONSData;
@class ONSDecimalNumber;
@class ONSDictionary;
@class ONSError;
@class ONSException;
@class ONSFileManager;
@class ONSKeyedArchiver;
@class ONSMutableSet;
@class ONSLocale;
@class ONSMutableArray;
@class ONSMutableData;
@class ONSNumber;
@class ONSPathStore2;
@class ONSRegularExpression;
@class ONSScanner;
@class ONSURL;
@class __ONSLocalizedString;
// CoreFoundation's tagged string class, looked up by name in +[NSString initialize].
@class ONSTaggedPointerString;
// CFNetwork's classes (an external image) that -initWithContentsOfURL:encoding:error: references.
@class ONSHTTPURLResponse;
@class ONSURLConnection;
@class ONSURLRequest;
#endif

#ifdef __OBJC__
extern char fileErrorClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSError));
extern char fileNumberClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSNumber));
extern char fileDictionaryClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDictionary));
extern char fileURLClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSURL));
extern char fileCheapMutableStringClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSCheapMutableString));
extern char mutableStringExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char pathStoreClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSPathStore2));
extern char pathCheapMutableStringClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSCheapMutableString));
extern char pathFileManagerClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSFileManager));
extern char pathMutableArrayClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableArray));
extern char pathMutableSetClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableSet));
extern char pathDataClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSData));
extern char pathExceptionClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char placeholderMutableStringExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char placeholderExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char placeholderAttributedStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSAttributedString));
extern char exceptionClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char arrayClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSArray));
extern char mutableArrayClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableArray));
extern char localeClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSLocale));
extern char placeholderStringClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSPlaceholderString));
extern char characterSetClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSCharacterSet));
extern char dataClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSData));
extern char mutableDataClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableData));
extern char dictionaryClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDictionary));
extern char urlClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSURL));
extern char keyedArchiverClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSKeyedArchiver));
extern char errorClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSError));
extern char httpURLResponseClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSHTTPURLResponse));
extern char urlConnectionClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSURLConnection));
extern char urlRequestClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSURLRequest));
extern char stringCFAdditionsExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char scannerClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSScanner));
extern char decimalNumberClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDecimalNumber));
extern char encodingsLocalizedStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSLocalizedString));
extern char encodingsExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char encodingsDictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDictionary));
extern char encodingsMutableDataClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableData));

@protocol StringForeignMessages

+ (id)defaultManager;
+ (void)_setAsTaggedStringClass;
- (BOOL)fileExistsAtPath:(id)path;
- (const void *)bytes;
- (void *)mutableBytes;
- (void)setLength:(ONSUInteger)length;
+ (id)dataWithLength:(ONSUInteger)length;
+ (id)dataWithBytesNoCopy:(void *)bytes length:(ONSUInteger)length;
- (id)initWithLength:(ONSUInteger)length;
- (id)initWithBytes:(const void *)bytes length:(ONSUInteger)length;
- (BOOL)writeToURL:(id)url options:(ONSUInteger)options error:(id *)error;
- (ONSInteger)code;
+ (id)errorWithDomain:(id)domain code:(ONSInteger)code userInfo:(id)userInfo;
+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
- (id)objectForKey:(id)key;
- (void)setObject:(id)object forKey:(id)key;
+ (id)dictionaryWithObjectsAndKeys:(id)firstObject, ...;
- (id)initWithObjectsAndKeys:(id)firstObject, ...;
+ (id)array;
+ (id)arrayWithObject:(id)object;
- (void)addObject:(id)object;
- (BOOL)containsObject:(id)object;
- (BOOL)isFileURL;
- (id)path;
+ (id)fileURLWithPath:(id)path;
- (BOOL)allowsKeyedCoding;
- (BOOL)containsValueForKey:(id)key;
- (const uint8_t *)decodeBytesForKey:(id)key returnedLength:(ONSUInteger *)length;
- (void *)decodeBytesWithReturnedLength:(ONSUInteger *)length;
- (void)encodeBytes:(const void *)bytes length:(ONSUInteger)length;
- (void)encodeBytes:(const uint8_t *)bytes length:(ONSUInteger)length forKey:(id)key;
- (ONSInteger)versionForClassName:(id)className;
- (void)failWithError:(id)error;
- (id)_decodePropertyListForKey:(id)key;
- (void)_encodePropertyList:(id)propertyList forKey:(id)key;
- (BOOL)isByref;
+ (id)whitespaceCharacterSet;
+ (id)decimalDigitCharacterSet;
- (BOOL)characterIsMember:(unsigned short)character;
+ (id)numberWithUnsignedInteger:(ONSUInteger)value;
- (id)descriptionWithLocale:(id)locale;
- (id)copyWithZone:(struct _NSZone *)zone;

@end

@protocol FileUtilitiesMessages

- (BOOL)getFileSystemRepresentation:(char *)buffer maxLength:(ONSUInteger)maxLength withPath:(id)path;
- (void)setContentsNoCopy:(void *)contents
                   length:(ONSUInteger)length
             freeWhenDone:(BOOL)freeWhenDone
                isUnicode:(BOOL)isUnicode;
+ (id)pathStoreWithCharacters:(const unsigned short *)characters length:(ONSUInteger)length;
- (BOOL)__swiftFillFileSystemRepresentationWithPointer:(char *)buffer maxLength:(ONSInteger)maxLength;

@end

@protocol PlaceholderStringMessages

- (id)_initWithFormat:(id)format options:(ONSUInteger)options locale:(id)locale arguments:(va_list)arguments;
- (id)string;
- (id)_dynamicContextEvaluation:(const void *)context patternString:(const void *)patternString;

@end

@protocol StringOtherEncodingsMessages

- (id)baseString;
- (id)initWithString:(id)string withFormatConfiguration:(id)configuration;

@end

@protocol StringDecimalExtensionMessages

+ (id)scannerWithString:(id)string;
- (BOOL)scanDecimal:(ONSDecimal *)decimal;
+ (id)notANumber;

@end

@protocol StringDistantStringMessages

- (id)_proxyForLocalObject:(id)object;

@end

// Path-store fault reporting sends this selector to the components array. KVC implementation unread.
@protocol PathStoreArrayMessages
- (id)valueForKey:(id)key;
@end

@protocol StringPartBMessages

- (id)initWithContentsOfMappedFile:(id)path;
+ (BOOL)_writeDataToPath:(id)path
                           data:(id)data
                        options:(ONSUInteger)options
    stringEncodingAttributeData:(id)attributeData
                 reportProgress:(BOOL)reportProgress
                          error:(id *)error;
+ (BOOL)_writeDataToPath:(id)path
                    data:(id)data
                 options:(ONSUInteger)options
          reportProgress:(BOOL)reportProgress
                   error:(id *)error;
+ (BOOL)_readBytesFromPath:(id)path
                 maxLength:(ONSUInteger)maxLength
                     bytes:(void **)bytes
                    length:(ONSUInteger *)length
                    didMap:(BOOL *)didMap
                   options:(ONSUInteger)options
            reportProgress:(BOOL)reportProgress
                     error:(id *)error;
- (id)_securePath;
- (id)initWithURL:(id)url cachePolicy:(ONSUInteger)cachePolicy timeoutInterval:(double)timeoutInterval;
+ (id)sendSynchronousRequest:(id)request returningResponse:(id *)response error:(id *)error;
- (ONSInteger)statusCode;
+ (BOOL)isErrorStatusCode:(ONSInteger)statusCode;
+ (id)_readCorruptErrorWithFormat:(id)format, ...;

@end
#endif

#ifdef __OBJC__
// Foreign Foundation class records read through IDA on 2026/11/03. NSObject's shared-cache superclass
// address is 0x22e6b6088 (name binding confidence: medium); all other superclass links below are in-image.
// No ivars are declared: these are message-only interfaces, not restored class layouts.

// Native NSAttributedString, 0x22e6d6138; owner FC-sets-attributed.

// Native NSCharacterSet, 0x22e6d6958; owner FC-sets-attributed.

// Native NSError, 0x22e6d78d0; owner FM-misc.

// Native NSValue (0x22e6de478), NSNumber (0x22e6de400) and NSDecimalNumber (0x22e6d7358);
// owner FN-number-value (decimal-number implementation remains outside this batch).

// Native NSScanner, 0x22e6dcee8; owner FM-misc.

// Native NSCoder (0x22e6d6ae8) and NSKeyedArchiver (0x22e6dadf0); owner FK-keyed-archiving.
@interface ONSCoder (UnrestoredStringMessages)
- (BOOL)allowsKeyedCoding;
- (BOOL)containsValueForKey:(id)key;
- (const uint8_t *)decodeBytesForKey:(id)key returnedLength:(ONSUInteger *)length;
- (void *)decodeBytesWithReturnedLength:(ONSUInteger *)length;
- (void)encodeBytes:(const void *)bytes length:(ONSUInteger)length;
- (void)encodeBytes:(const uint8_t *)bytes length:(ONSUInteger)length forKey:(id)key;
- (ONSInteger)versionForClassName:(id)className;
- (void)failWithError:(id)error;
- (id)_decodePropertyListForKey:(id)key;
- (void)_encodePropertyList:(id)propertyList forKey:(id)key;
@end

#endif

#ifdef __OBJC__
// Round 2 FG-grammar-inflection-rules supplies this parser method, on the class allocated by
// FO-notifications-locks-json-coders.

// The descriptor table refers to static instances of these native subclasses. Their method bodies
// are outside the supplied FA capture and the class definitions remain undefined.
@class _ONSNumericAttributeDescriptor;
@class _ONSAssumedFallbackInflectionAttributeDescriptor;
@class _ONSNumberFormattingAttributeDescriptor;
@class _ONSMorphologyAttributeDescriptor;
@class _ONSInflectionAttributeDescriptor;
@class _ONSMarkdownStringAttributeDescriptor;
@class _ONSStringAttributeDescriptor;
@class _ONSURLAttributeDescriptor;
#endif

OF_EXPORT const OCFStringRef _kOCFStringFormatMetadataArgumentNumberKey;

#ifdef __OBJC__

#endif

// FI supplies the morphology custom-pronoun class.
#ifdef __OBJC__

@interface ONSMorphology (GrammarInflectionMessages)
- (id)_customPronouns;
@end

@interface NSObject (LocalizedNumberFormatLocaleMessages)
- (id)groupingSeparator;
@end
#endif

#ifdef __OBJC__
// Selector declarations for the CoreFoundation collection implementations used by FG.
@protocol GrammarCollectionMessages
+ (id)array;
- (id)initWithCapacity:(NSUInteger)capacity;
- (void)addObject:(id)object;
- (id)objectForKeyedSubscript:(id)key;
- (void)setObject:(id)object forKeyedSubscript:(id)key;
- (id)allValues;
- (NSUInteger)count;
- (id)objectAtIndexedSubscript:(NSUInteger)index;
+ (id)dictionaryWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count;
@end
#endif

#ifdef __OBJC__
// FO owns these methods of _NSJSONWriter. The class layout is shared with FG.
@interface _ONSJSONWriter (WriterMessages)
- (BOOL)appendString:(id)string range:(ONSRange)range;
- (void)setFailure:(ONSError *)error;
@end

// FD owns this class and its numeric representation.

@interface ONSOutputStream (JSONWriterMessages)
- (NSInteger)write:(const uint8_t *)bytes maxLength:(NSUInteger)length;
@end

@protocol JSONCollectionMessages <GrammarCollectionMessages>
+ (id)dataWithBytesNoCopy:(void *)bytes length:(NSUInteger)length freeWhenDone:(BOOL)freeWhenDone;
+ (id)dictionaryWithObjectsAndKeys:(id)firstObject, ...;
- (void)getObjects:(id *)objects andKeys:(id *)keys count:(NSUInteger)count;
- (void)enumerateKeysAndObjectsUsingBlock:(void (^)(id key, id value, BOOL *stop))block;
@end
#endif

#ifdef __OBJC__
#import "CoreFoundation/Objective-C/OCFNSDictionary.h"
#import "Foundation/Archiving/ONSCoder.h"

// NSCoder restoration. Native -[NSCoder decodeArrayOfObjectsOfClass:forKey:], 0x1c04db6b8.
@interface ONSCoder (TermOfAddressDecoding)
- (id)decodeArrayOfObjectsOfClass:(Class)cls forKey:(id)key;
@end

@protocol MorphologyIndexedMessages
- (NSUInteger)count;
- (id)objectAtIndexedSubscript:(NSUInteger)index;
@end

@interface ONSDictionary (MorphologyDictionaryMessages)
+ (id)dictionaryWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count;
- (id)objectForKeyedSubscript:(id)key;
@end

@protocol MorphologyMutableDictionaryMessages
- (void)setObject:(id)object forKeyedSubscript:(id)key;
@end
#endif

//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Exception/ONSAssertionHandler.h"
#import "Foundation/String/ONSAttributedStringGrammarInflection.h"
#import "Foundation/String/ONSAttributedStringReplacement.h"
#import "Foundation/String/ONSInflectionRule.h"
#import "Foundation/String/ONSLexiconMorphunDictionary.h"
#import "Foundation/String/ONSLocalizedNumberFormatRule.h"
#import "Foundation/String/ONSTermOfAddress.h"

#import "Foundation/Exception/ONSAssertionHandler.h"
#import "Foundation/Number/ONSConstantIntegerNumber.h"

#import "Foundation/String/ONSPlaceholderString.h"
#import "Foundation/String/ONSInflectionRule.h"
#import "Foundation/String/ONSLocalizedNumberFormatRule.h"
#import "Foundation/String/ONSAttributedStringReplacement.h"
#import "Foundation/String/ONSAttributedStringGrammarInflection.h"

#import "Foundation/Base/ONSUnrestoredInterfaces.h"

#import "Foundation/Archiving/ONSCoder.h"
#import "Foundation/Archiving/ONSKeyedArchiver.h"
#import "Foundation/Archiving/ONSKeyedUnarchiver.h"
#import "Foundation/Archiving/ONSObjCRuntimeNames.h"
#import "Foundation/Error/ONSError.h"
#import "Foundation/Error/ONSErrorConstants.h"
#import "Foundation/Number/ONSNumber.h"
#import "Foundation/Number/ONSValue.h"
#import "Foundation/String/ONSAllocateObject.h"
#import "Foundation/String/ONSAttributedStringFromMarkdownCreatorConcrete.h"
#import "Foundation/String/ONSAttributedStringMarkdownParsingOptions.h"
#import "Foundation/String/ONSAttributedStringMarkdownSourcePosition.h"
#import "Foundation/String/ONSPresentationIntent.h"
#import "Foundation/String/ONSLog.h"
#import "Foundation/String/ONSMutableString.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSString.h"
#import "Foundation/String/ONSStringFromRange.h"
#import "Foundation/Thread/ONSThread.h"

#include <OpenFoundation/Range/ONSRange.h>

#include <OpenCoreFoundation/OCFBuild.h>
#include <OpenCoreFoundation/Base/OCFBase.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

// Interfaces of families not yet restored on this branch.
//
// Declarations of other batch-8 families used by the NSIndexSet, NSAttributedString and NSCharacterSet
// family (FC-sets-attributed), iOS Simulator 24A434, arm64,
// Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. All other-family declarations are collected here,
// including those used by one unit. The coordinator replaces this header with an include of the owners'
// headers when the families land together. Object types the restored code receives from these functions are
// spelled id: the classes are Foundation's and restored by other families.

// The zone type of Foundation's allocation interfaces (libobjc's objc/NSObject.h declares the same tag).
struct _NSZone;

// libsystem feature configuration interface, called by the native formatting method.
extern bool _os_feature_enabled_impl(const char *domain, const char *feature);

// libobjc calls at this family's captured ownership, allocation and optimized query boundaries.
extern id objc_getProperty(id object, SEL selector, ptrdiff_t offset, BOOL atomic);
extern void
objc_copyStruct(void *destination, const void *source, ptrdiff_t size, BOOL atomic, BOOL hasStrong);
extern id objc_retain(id object);
extern void objc_release(id object);
extern id objc_autorelease(id object);
extern id objc_alloc(Class cls);
extern id objc_alloc_init(Class cls);
extern id objc_allocWithZone(Class cls);
extern id objc_opt_self(id object);
extern Class objc_opt_class(id object);
extern BOOL objc_opt_isKindOfClass(id object, id cls);
extern BOOL objc_opt_respondsToSelector(id object, SEL selector);
extern void *objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void *pool);

#ifdef __OBJC__
#import <objc/NSObject.h>

// Declaration-only contracts of other Foundation families and uncaptured class groups. Each interface
// contains only selectors sent by FC-sets-attributed, with no ivars. The coordinator replaces these with
// owner headers. Superclasses and class addresses were checked in the pinned class_ro_t metadata; the
// declarations do not restore these class objects or methods.

// FS-string: NSString 0x22e6dd528; NSMutableString 0x22e6db6b0.

// FK-keyed-archiving: NSCoder 0x22e6d6ae8; NSKeyedArchiver 0x22e6dadf0;
// NSKeyedUnarchiver 0x22e6dadc8. Selector-only declarations, never their unread ivar layouts.
@interface ONSCoder (UnrestoredCollectionsMessages)
- (BOOL)allowsKeyedCoding;
- (BOOL)requiresSecureCoding;
- (void)encodeInteger:(NSInteger)value forKey:(id)key;
- (void)encodeObject:(id)object;
- (void)encodeObject:(id)object forKey:(id)key;
- (void)encodeValueOfObjCType:(const char *)type at:(const void *)address;
- (void)encodeValuesOfObjCTypes:(const char *)types, ...;
- (void)decodeValueOfObjCType:(const char *)type at:(void *)address size:(NSUInteger)size;
- (void)decodeValuesOfObjCTypes:(const char *)types, ...;
- (NSInteger)decodeIntegerForKey:(id)key;
- (int64_t)decodeInt64ForKey:(id)key;
- (BOOL)decodeBoolForKey:(id)key;
- (BOOL)containsValueForKey:(id)key;
- (id)decodeObject;
- (id)decodeObjectOfClass:(Class)cls forKey:(id)key;
- (void)encodeBool:(BOOL)value forKey:(id)key;
- (id)decodeObjectOfClasses:(id)classes forKey:(id)key;
- (id)allowedClasses;
- (id)error;
- (void)failWithError:(id)error;
- (id)_decodePropertyListForKey:(id)key;
@end

// FM-misc: NSError 0x22e6d78d0; NSThread 0x22e6dda78. NSMorphology's real 96-byte interface is
// separately owned by ONSAttributedStringInflection.h for this family's two direct methods; reconcile it
// with FM at integration. Its class object and its other methods are not defined by this family.

// Remaining Foundation integration dependencies: NSAssertionHandler 0x22e6d79e8,
// NSBundle 0x22e6d66d8, NSTermOfAddress 0x22e6dcfd8 and _NSLexiconMorphunDictionary 0x22e6dbef8.

// Uncaptured Foundation groups, explicit integration dependencies: NSBundle 0x22e6d66d8, NSInflectionRule
// 0x22e6dfaf8, _NSInflectionRuleExplicit 0x22e6dfbe8, NSLocalizedNumberFormatRule 0x22e6def18,
// NSTermOfAddress 0x22e6dcfd8, _NSAttributedStringGrammarInflection 0x22e6dfe18,
// _NSAttributedStringReplacement 0x22e6da918, _NSLexiconMorphunDictionary 0x22e6dbef8. These groups have no
// function bodies in this family's capture.

#import "Foundation/String/ONSTermOfAddress.h"

#import "Foundation/String/ONSLexiconMorphunDictionary.h"

// CoreFoundation class references use that product's owner interfaces in the source files when needed.
@class ONSArray;
@class ONSDictionary;
@class ONSMutableArray;
@class ONSMutableDictionary;
@class ONSData;
@class ONSMutableData;
@class ONSException;
@class ONSLocale;
@class ONSNull;
@class ONSSet;
@class ONSURL;

// Uncaptured cluster classes derive from FC's bases. A forward declaration avoids an include cycle with
// those bases; their missing class objects are explicit entries in the integration allow list.
@class ONSConcreteAttributedString;
@class ONSConcreteMutableAttributedString;
@class ONSMutableStringProxyForMutableAttributedString;
@class _ONSPlaceholderIndexSet;
@class _ONSConstantIndexSet;
@class _ONSPlaceholderCharacterSet;
@class _ONSSwiftCharacterSet;
#endif

#ifdef __OBJC__
// Declaration for the two native super sends; no category implementation is installed on NSObject.
@interface NSObject (FoundationPortCodingDeclarations)
- (id)replacementObjectForPortCoder:(id)coder;
@end
#endif

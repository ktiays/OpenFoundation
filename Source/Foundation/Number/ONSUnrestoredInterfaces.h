//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "CoreFoundation/Objective-C/OCFNSObjectGlue.h"

#import "CoreFoundation/Collections/OCFSet.h"
#import "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#import "CoreFoundation/Objective-C/OCFObjCTypeEncoding.h"
#import "CoreFoundation/String/OCFString.h"
#import "Foundation/Archiving/ONSXPCCoder.h"

#import "Foundation/Number/ONSDecimalNumber.h"

#import "Foundation/Archiving/ONSCoder.h"
#import "Foundation/Archiving/ONSKeyedArchiver.h"
#import "Foundation/Archiving/ONSKeyedUnarchiver.h"
#import "Foundation/Error/ONSError.h"
#import "Foundation/Error/ONSErrorKeyedArchivalErrors.h"
#import "Foundation/Scanner/ONSDecimalNumberScanning.h"
#import "Foundation/Scanner/ONSScanner.h"
#import "Foundation/String/ONSAllocateObject.h"
#import "Foundation/String/ONSLog.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSString.h"

#import "Foundation/Number/ONSConcreteValue.h"
#import "Foundation/Number/ONSNumber.h"
#import "Foundation/Number/ONSWeakObjectValue.h"
#include "CoreFoundation/Collections/OCFSet.h"
#include "CoreFoundation/Locale/OCFNumberFormatter.h"
#import "CoreFoundation/Objective-C/OCFNSMutableDictionary.h"
#import "Foundation/Base/ONSUnrestoredInterfaces.h"
#import "CoreFoundation/Objective-C/OCFNSData.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "CoreFoundation/Objective-C/OCFNSSet.h"
#include <OpenFoundation/Decimal/ONSDecimal.h>

// Integration declarations, replaced by the owners' headers when the families land together.
// Foundation: iOS Simulator 24A434, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// CoreFoundation: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC.
// The addresses below identify native definitions, not replacement addresses.

#ifdef __OBJC__
// FS-string; NSString, class 0x22e6dd528. Only message interfaces, no restored layout.

// CoreFoundation, NSMutableData, class 0x22e6ccd48; superclass NSData. Only the sends used here.
@interface ONSMutableData : ONSData
- (id)initWithLength:(NSUInteger)length;
- (void *)mutableBytes;
@end

// Native ivar offset variables, 0x22e6d4df8 (typeInfo, offset 16, ^v) and 0x22e6d4dfc
// (_specialFlags, offset 8, Q), external. ONSConcreteValue supplies these in ONSValue.m. Using its offset
// variables preserves the native non-fragile ivar access.
extern int32_t
    concreteValueTypeInfoOffset __asm("_OBJC_IVAR_$_" OF_NAME_STRING(ONSConcreteValue) ".typeInfo");
extern int32_t
    concreteValueSpecialFlagsOffset __asm("_OBJC_IVAR_$_" OF_NAME_STRING(ONSConcreteValue) "._specialFlags");

// NSXPCCoder, 0x22e6e1df8, external, unread coder dependency of FD-decimal-number-formatting.
// class_ro_t 0x222870678 names NSCoder as the superclass. No layout or methods are declared.

extern char jsonXPCCoderClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSXPCCoder));

// FA owns _NSStringProxyForContext and all of its methods. Only the formatter's sends are declared.
@class _ONSStringProxyForContext;
extern char numberFormatterProxyClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(_ONSStringProxyForContext));

@protocol NumberFormattingContextProxy
- (void)setString:(id)string;
- (void)setItem:(id)item;
- (void)_retainFormatter:(id)formatter;
@end

@interface NSObject (UnrestoredFormattingCollectionMessages)
- (void)replaceObjectAtIndex:(NSUInteger)index withObject:(id)object;
@end

@interface ONSDictionary (UnrestoredFormattingMessages)
+ (id)dictionaryWithObjectsAndKeys:(id)firstObject, ...;
@end

@interface ONSMutableDictionary (UnrestoredFormattingMessages)
- (void)setDictionary:(id)dictionary;
- (void)setValue:(id)value forKey:(id)key;
@end

@interface ONSBundle (UnrestoredFormattingMessages)
- (id)bundleIdentifier;
@end

// FK-keyed-archiving; NSCoder 0x22e6d6ae8. Only messages sent to the coder are declared.
@interface ONSCoder (UnrestoredNumberMessages)
- (BOOL)allowsKeyedCoding;
- (BOOL)containsValueForKey:(id)key;
- (NSInteger)decodeIntegerForKey:(id)key;
- (int)decodeIntForKey:(id)key;
- (double)decodeDoubleForKey:(id)key;
- (BOOL)decodeBoolForKey:(id)key;
- (int64_t)decodeInt64ForKey:(id)key;
- (ValuePoint)decodePointForKey:(id)key;
- (ValueSize)decodeSizeForKey:(id)key;
- (ValueRect)decodeRectForKey:(id)key;
- (id)decodeObjectOfClass:(Class)aClass forKey:(id)key;
- (void)decodeValueOfObjCType:(const char *)type at:(void *)data size:(NSUInteger)size;
- (void)encodeValueOfObjCType:(const char *)type at:(const void *)address;
- (id)_decodePropertyListForKey:(id)key;
- (void)_encodePropertyList:(id)plist forKey:(id)key;
- (void)encodeInteger:(NSInteger)value forKey:(id)key;
- (void)encodePoint:(ValuePoint)value forKey:(id)key;
- (void)encodeSize:(ValueSize)value forKey:(id)key;
- (void)encodeRect:(ValueRect)value forKey:(id)key;
- (void)encodeBool:(BOOL)value forKey:(id)key;
- (void)encodeInt64:(int64_t)value forKey:(id)key;
- (void)encodeDouble:(double)value forKey:(id)key;
- (void)failWithError:(id)error;
@end

// FK-keyed-archiving; NSKeyedArchiver 0x22e6dadf0, NSKeyedUnarchiver 0x22e6dadc8.

// FM-misc; NSError 0x22e6d78d0. The corrupt-error factory is outside this family's capture.

// FM-misc; NSScanner 0x22e6dcee8. +scannerWithString: is captured at 0x1c05c0b44;
// -scanDecimal: is an unread selector boundary used by -[NSNumber decimalValue].

#endif

// Supplied libobjc SPI, called directly by the captured native unit. objc_retain is already declared by
// CoreFoundation/Objective-C/OCFNSSet.h; the coordinator should move all these to the runtime owner header.
// Examples: +[NSNumber numberWithChar:] 0x1c05fa480; -[NSValue init] 0x1c05f8d64;
// __NSNewValue 0x1c05f77f4. No Foundation/CoreFoundation operation is forwarded to the system.
extern id objc_alloc(Class aClass);
extern id objc_allocWithZone(Class aClass);
extern id objc_autorelease(id object);
extern void objc_release(id object);
extern Class objc_opt_class(id object);
extern id objc_opt_self(id object);

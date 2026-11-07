//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "CoreFoundation/Collections/OCFSet.h"
#import "CoreFoundation/PropertyList/OCFBinaryPList.h"
#import "CoreFoundation/PropertyList/OCFPropertyList.h"

#import "Foundation/Base/ONSUnrestoredInterfaces.h"

#import "Foundation/Error/ONSError.h"
#import "Foundation/Error/ONSErrorConstants.h"
#import "Foundation/Number/ONSNumber.h"
#import "Foundation/Number/ONSValue.h"
#import "Foundation/ProcessInfo/ONSProcessInfo.h"
#import "Foundation/String/ONSLog.h"
#import "Foundation/String/ONSMutableString.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSString.h"

#include "Foundation/Error/ONSErrorConstants.h"

#include <OpenCoreFoundation/OCFBuild.h>

#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFSet.h"
#include "CoreFoundation/PropertyList/OCFBinaryPList.h"

#include <dispatch/dispatch.h>
#include <objc/objc.h>
#include <os/log.h>

// Foreign interfaces needed by FK-keyed-archiving. These declarations do not implement their targets.
// Foundation iOS Simulator 24A434, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Reading: Source/Foundation/Archiving/Reading.md. iOS Device 24A446: unread.

// Shared-cache class bindings (confidence: medium): 0x22227fb30 and 0x22227fb48, CoreFoundation.
extern char archivingMutableOrderedSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                        OF_NAME_STRING(ONSMutableOrderedSet));
extern char archivingOrderedSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSOrderedSet));
// Round-2 Foundation family FR (pointer collections).
@class ONSHashTable;
@class ONSPointerArray;
extern char archivingHashTableClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSHashTable));
extern char archivingPointerArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSPointerArray));
extern char archivingLocalizedStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                      OF_NAME_STRING(__ONSLocalizedString));
extern char archivingAttributedStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                       OF_NAME_STRING(ONSAttributedString));
extern char archivingMutableAttributedStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                              OF_NAME_STRING(ONSMutableAttributedString));
extern char archivingIndexSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSIndexSet));
extern char archivingMutableIndexSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                      OF_NAME_STRING(ONSMutableIndexSet));
// Foundation class at 0x22e6d8528. Its implementation is outside the FX capture.
@class ONSCountedSet;
extern char archivingCountedSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSCountedSet));

#ifdef __OBJC__
#import "Foundation/Archiving/ONSKeyedCoderOldStyleArray.h"
#import "Foundation/Archiving/ONSKeyedUnarchiverHelper.h"
#import "Foundation/Archiving/ONSPerformanceMeter.h"

#import <objc/NSObject.h>

#import "Foundation/Archiving/ONSCoder.h"

// Native NSCoder methods omitted from the captured family. The indirect message edges remain open.
@interface ONSCoder (UnrestoredMessageInterfaces)
// -[NSCoder allowedClasses], 0x1c04dc028.
- (id)allowedClasses;
// -[NSCoder _enforceCollectionTyping], 0x1c04dcaac.
- (BOOL)_enforceCollectionTyping;
// -[NSCoder validateAllowedClass:forKey:], 0x1c04dcf3c.
- (BOOL)validateAllowedClass:(Class)cls forKey:(id)key;
// -[NSCoder validateClassSupportsSecureCoding:], 0x1c04dcf44.
- (void)validateClassSupportsSecureCoding:(Class)cls;
// -[NSCoder decodeTopLevelObjectOfClasses:forKey:error:], 0x1c04dbb84.
- (id)decodeTopLevelObjectOfClasses:(id)classes forKey:(id)key error:(id *)error;
@end

// Native NSCoder(Exceptions), omitted from the capture. The category emits declarations only.
@interface ONSCoder (Exceptions)
// -[NSCoder(Exceptions) __decoderInfoForAllowedClassesWarning], 0x1c04dd8a8.
- (id)__decoderInfoForAllowedClassesWarning;
// -[NSCoder(Exceptions) __decoderEnforceCollectionType], 0x1c04dd8b4.
- (BOOL)__decoderEnforceCollectionType;
// -[NSCoder(Exceptions) __failWithExceptionName:errorCode:format:], 0x1c04ddbd8.
- (void)__failWithExceptionName:(id)name errorCode:(NSInteger)code format:(id)format, ...;
// +[NSCoder(Exceptions) __categorizeException:intoError:], 0x1c04ddd58.
+ (BOOL)__categorizeException:(id)exception intoError:(id *)error;
@end

// Class records and superclasses read in IDA. Only interfaces used by this family are declared; the owning
// families supply the definitions and full interfaces at integration.
// Native NSString 0x22e6dd528 / NSMutableString 0x22e6db6b0; FS-string.

// Native NSValue 0x22e6de478 / NSNumber 0x22e6de400; FN-number-value.

// Native NSError 0x22e6d78d0; FM-misc.

// Native NSProcessInfo 0x22e6dc9c0; FM-misc.

#endif

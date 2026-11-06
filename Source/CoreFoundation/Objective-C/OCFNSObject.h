//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <objc/objc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Bridged classes the C units reference by name (OF_NAME_STRING) before their definitions.
#ifdef __OBJC__
@class __ONSCFType;
@class ONSNull;
@class ONSMutableDictionary;
@class ONSMutableSet;
@class ONSMutableOrderedSet;
@class ONSMutableData;
@class __ONSCFString;
@class __ONSCFSet;
@class __ONSCFDictionary;
@class __ONSCFArray;
@class __ONSCFData;
@class __ONSCFBoolean;
@class __ONSCFNumber;
@class ONSMachPort;
@class ONSURL;
@class __ONSCFTimer;
@class __ONSCFAttributedString;
@class __ONSCFLocale;
@class __ONSCFError;
@class __ONSCFCharacterSet;
@class __ONSCFInputStream;
@class __ONSCFOutputStream;
@class ONSArray;
@class ONSMutableArray;
@class ONSDate;
@class ONSCharacterSet;
@class ONSMutableCharacterSet;
@class _ONSSwiftCharacterSet;
@class __ONSVariableWidthString;
@class __ONSCFLocalizedAttributedString;
@class ONSConstantDictionary;
@class ONSConstantArray;
@class ONSException;
@class ONSBlock;
@class ONSPersonNameComponents;
@class ONSPersonNameComponentsFormatter;
#endif

// The exception name constants (external natively as NSGenericException, NSInvalidArgumentException,
// NSInternalInconsistencyException and NSMallocException), defined by the exception unit
// (Objective-C/OCFException.m).
OF_EXPORT const OCFStringRef ONSGenericException;
OF_EXPORT const OCFStringRef ONSRangeException;
OF_EXPORT const OCFStringRef ONSInvalidArgumentException;
OF_EXPORT const OCFStringRef ONSInternalInconsistencyException;
OF_EXPORT const OCFStringRef ONSMallocException;

// Native _CFThrowFormattedException, 0x1c0308c0c (hidden, was a private external;
// Objective-C/OCFException.m): raises an Objective-C exception with a formatted reason.
OF_EXTERN void _CFThrowFormattedException(OCFStringRef name, OCFStringRef format, ...)
    __attribute__((noreturn));

// Native __CFLookUpClass, 0x1c029ed14 (was a private external), in the Objective-C runtime glue beside
// __CFAllocateObject (Objective-C/OCFNSObject.m).
OF_EXTERN uintptr_t __CFLookUpClass(const char *name);

// Native _CFRaiseMemoryException, 0x1c030f268 (was a private external; Objective-C/OCFException.m): throws an
// NSMallocException with the reason.
OF_EXTERN __attribute__((noreturn)) void _CFRaiseMemoryException(OCFStringRef reason);

// Push and pop an Objective-C autorelease pool; the pop returns its argument.
OF_EXPORT void *_OCFAutoreleasePoolPush(void);

OF_EXPORT void *_OCFAutoreleasePoolPop(void *context);

// Load-time Objective-C setup (exception handlers, forwarding, enumeration mutation, NSBlock classes, zombies
// and allocation statistics), defined by the Objective-C units.
OF_EXTERN void __exceptionInit(void);
// Native name outside the CF/NS prefix rule (CF followed by '_'), exported natively; hidden until the owner
// records a naming decision.
OF_EXTERN void _CF_forwarding_prep_0(void);
OF_EXTERN void __NSFastEnumerationMutationHandler(void *object);
OF_EXTERN void __CFMakeNSBlockClasses(void);
OF_EXTERN void __CFZombifyNSObject(void);
OF_EXTERN void __CFOAInitializeNSObject(void);

// The Objective-C runtime glue of this unit (Objective-C/OCFNSObject.m).
OF_EXPORT id _OCFAutoreleasePoolAddObject(void *pool, OCFTypeRef object);
OF_EXPORT BOOL _ONSIsNSNumber(id object);
OF_EXPORT BOOL _ONSIsNSCFConstantString(id object);
OF_EXTERN id getAtomTarget(id object);
OF_EXTERN id __CFAllocateObject(Class cls, size_t extraBytes);
// Non-external natively (the bad-alloc handler of +[NSObject(NSObject) load], which is not restored).
OF_EXTERN __attribute__((noreturn)) id couldNotInstantiate(Class cls);

// The exception unit (Objective-C/OCFException.m).
OF_EXTERN id __exceptionPreprocess(id exception);
OF_EXTERN int __exceptionMatch(Class catchType, id exception);
OF_EXTERN void __handleUncaughtException(id exception);
OF_EXTERN bool objectIsKindOfClass(id object, Class cls);

// The forwarding trampolines (Objective-C/OCFForwarding.c). __invoking___ calls function with the registers
// and stack arguments of argumentFrame (frameSize bytes: X0-X8 at 0-0x40, Q0-Q7 at 0x50-0xC0, stack arguments
// from 0xE0) and stores the result registers into returnFrame; flags is not read.
OF_EXTERN void __invoking___(
    void (*function)(void), void *returnFrame, void *argumentFrame, size_t frameSize, uint32_t flags
);
// Natively an external alias of _CF_forwarding_prep_0 (the same address); a name outside the CF/NS prefix
// rule, hidden until the owner records a naming decision.
OF_EXTERN void __forwarding_prep_0___(void);

// The type-string set callbacks of the method-signature unit (Objective-C/OCFForwarding.c).
OF_EXTERN const void *typeStringRetain(OCFAllocatorRef allocator, const void *value);
OF_EXTERN void typeStringRelease(OCFAllocatorRef allocator, const void *value);
OF_EXTERN OCFStringRef copyTypeStringDesc(const void *value);
OF_EXTERN Boolean typeStringEqual(const void *value1, const void *value2);
OF_EXTERN OCFHashCode typeStringHash(const void *value);

// Native __CFFullMethodName, 0x1c029eb84 (was a private external; Objective-C/OCFNSObject.m): the
// "-[Class selector]" text for the class, the object and the selector.
OF_EXTERN OCFStringRef __CFFullMethodName(Class cls, const void *object, SEL selector);

// Native __CFExceptionProem, 0x1c029ec78 (was a private external; Objective-C/OCFNSObject.m): the
// "-[Class selector]" prefix of an exception or log reason for the object and selector.
OF_EXTERN OCFStringRef __CFExceptionProem(const void *object, SEL selector);

// Interfaces of families not yet restored on this branch
// Native ___forwarding___, 0x1c028bcc0 (was a private external), with .cold.1-.cold.4
// (0x1c030f334-0x1c030f46c): in the NSInvocation unit, whose -[NSInvocation
// _initWithMethodSignature:frame:buffer:size:] it calls directly. The frame of _CF_forwarding_prep_0 and
// whether the call returns a structure (always 0 there); the result is the frame whose registers are
// returned, or NULL to tail-call objc_msgSend with the original registers. Restored in
// Objective-C/OCFNSInvocation.m.
OF_EXTERN void *___forwarding___(void *frame, intptr_t isStret);

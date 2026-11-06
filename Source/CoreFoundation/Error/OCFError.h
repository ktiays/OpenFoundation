//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Collections/OCFNSCFDictionary.h"

// CFError.h (iOS Simulator SDK interface), spelled OCF. The CFError unit (Error/OCFError.c) restores the
// functions in scope; the remaining entry points of the native unit (CFErrorCopyFailureReason,
// CFErrorCopyRecoverySuggestion, CFErrorCreateWithUserInfoKeysAndValues, the call-back setters and getters
// other than CFErrorCopyCallBackBlockForDomain, _CFErrorCreateRedactedDescription) are outside the scope.
typedef struct __OCFError *OCFErrorRef;
typedef OCFStringRef OCFErrorDomain;

OF_EXPORT const OCFStringRef kOCFErrorDomainPOSIX;
OF_EXPORT const OCFStringRef kOCFErrorDomainOSStatus;
OF_EXPORT const OCFStringRef kOCFErrorDomainCocoa;
OF_EXPORT const OCFStringRef kOCFErrorLocalizedDescriptionKey;
OF_EXPORT const OCFStringRef kOCFErrorLocalizedFailureReasonKey;
OF_EXPORT const OCFStringRef kOCFErrorLocalizedRecoverySuggestionKey;
OF_EXPORT const OCFStringRef kOCFErrorDebugDescriptionKey;
OF_EXPORT const OCFStringRef kOCFErrorUnderlyingErrorKey;

OF_EXPORT OCFTypeID OCFErrorGetTypeID(void);
OF_EXPORT OCFErrorRef
OCFErrorCreate(OCFAllocatorRef allocator, OCFStringRef domain, OCFIndex code, OCFDictionaryRef userInfo);
OF_EXPORT OCFDictionaryRef OCFErrorCopyUserInfo(OCFErrorRef err);
OF_EXPORT OCFIndex OCFErrorGetCode(OCFErrorRef err);
OF_EXPORT OCFErrorDomain OCFErrorGetDomain(OCFErrorRef err);
OF_EXPORT OCFStringRef OCFErrorCopyDescription(OCFErrorRef err);

// Private entry points of the CFError unit (external natively; no SDK declaration). The call-stack value is
// the array of return addresses stored as the error's "CFErrorCallStacks" associated object.
OF_EXPORT OCFStringRef _OCFErrorCreateLocalizedDescription(OCFErrorRef err);
OF_EXPORT OCFStringRef _OCFErrorCreateDebugDescription(OCFErrorRef err);
OF_EXPORT OCFStringRef _OCFErrorCreateUnlocalizedDebugDescription(OCFErrorRef err);
OF_EXPORT OCFStringRef _OCFErrorCreateRedactedDescription(OCFTypeRef err);
OF_EXPORT void _OCFErrorSetCallStackReturnAddresses(OCFErrorRef err, OCFArrayRef callStackReturnAddresses);
OF_EXPORT OCFArrayRef _OCFErrorCopyCallStackReturnAddresses(OCFTypeRef err);

#ifdef __BLOCKS__
// The user-info key call-back block of a domain (local descriptive type name): called with the error and the
// key, it answers an autoreleased value or NULL. CFErrorCopyCallBackBlockForDomain answers it retained.
typedef const void * (^UserInfoKeyCallBackBlock)(OCFErrorRef err, OCFStringRef key);
OF_EXPORT UserInfoKeyCallBackBlock OCFErrorCopyCallBackBlockForDomain(OCFStringRef domainName);
#endif

// The CFError class record (type 27), for the builtin class table; the type is OCFRuntimeClass
// (Runtime/OCFRuntime.h).
OF_EXTERN const struct __OCFRuntimeClass __CFErrorClass;

// Interfaces of families not yet restored on this branch
// Native __NSDictionary0__ (0x2227a4378, __DATA_CONST,__const, external): the pointer to the empty immutable
// dictionary instance (__NSDictionary0__struct, 0x2227cc280).

// Foundation classes this unit names at run time (the class OCFError.c looks up for the Cocoa call-back, and
// the receiver of +callStackReturnAddresses); declared for the naming switch.
#ifdef __OBJC__
@class ONSError;
@class ONSThread;
#endif

// Native CFErrorCreateWithUserInfoKeysAndValues, 0x1c0198608. External.
OF_EXPORT OCFErrorRef OCFErrorCreateWithUserInfoKeysAndValues(
    OCFAllocatorRef allocator,
    OCFStringRef domain,
    OCFIndex code,
    const void *const *userInfoKeys,
    const void *const *userInfoValues,
    OCFIndex numUserInfoValues
);

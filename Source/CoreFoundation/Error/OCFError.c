//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Error/OCFError.h"

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/PlugIn/OCFBundle.h"
#include "CoreFoundation/PlugIn/OCFBundleStrings.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/String/OCFStringFormat.h"
#include "CoreFoundation/URL/OCFURL.h"

#include <Block.h>
#include <mach/mach_error.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <os/lock.h>
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

// Inferred organization: the CFError functions (0x1c01977a0-0x1c0199024) are one contiguous cluster of the
// iOS Simulator __text section, with their .cold pieces at 0x1c0307fd0-0x1c0308030, and are kept in one
// translation unit; the native source path is not in the evidence. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full);
// iOS Device 24A446: unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json.

// The CFError instance (type 27, 24 extra bytes). The layout is the one the native block descriptor encodes
// ({__CFError={__CFRuntimeBase=QAQ}q^{__CFString}^{__CFDictionary}}) and every accessor reads: the code at
// 16, the domain at 24, the user info at 32. Field names are local descriptive spellings.
struct __OCFError {
    OCFRuntimeBase base;
    OCFIndex code;
    OCFStringRef domain;
    OCFDictionaryRef userInfo;
};

enum {
    kErrorTypeID = 27,
};

// The state of one debug-description pass (a 32-byte stack record natively): the string appended to at 0,
// the user-info nesting depth at 8, the localized flag at 16 and the redacted flag at 17. Local descriptive
// names.
typedef struct {
    OCFMutableStringRef string;
    OCFIndex depth;
    uint8_t localized;
    uint8_t redacted;
} DescriptionContext;

// The user-info key call-back of a domain (CFErrorUserInfoKeyCallBack; local descriptive name).
typedef const void *(*UserInfoKeyCallBack)(OCFErrorRef err, OCFStringRef key);

// dyld's SPI (libdyld, re-exported by libSystem); the native call passes 0x07DF0901FFFFFFFF in X0.
extern bool dyld_program_sdk_at_least(OCFBuildVersion version);

// libobjc entries the native code imports directly (objc-internal.h); supplied.
extern BOOL objc_opt_respondsToSelector(id object, SEL selector);
extern void *objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void *context);

// libsystem_platform's os_unfair_lock_lock_with_options (private SPI); every native call here passes 0x10000
// (OS_UNFAIR_LOCK_DATA_SYNCHRONIZATION).
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);

// The receiver of +callStackReturnAddresses in __CFErrorSetCallStackReturnAddresses.cold.1 is the class
// reference at 0x22229C678, outside the extracted image; NSThread, the class answering that class message,
// is the binding (confidence: medium).
extern char callStackClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSThread));

// The key of the call-stack associated object ("CFErrorCallStacks", one C string; the key is its address).
static const char callStacksKey[] = "CFErrorCallStacks";

// Native __CFErrorCallBackTable, __CFErrorLock and __CFErrorCallStackCaptureEnabled, 0x22e6d2818, 0x22e6d2820
// and 0x22e6d2824 (__DATA,__bss). Non-external. No function of this restoration stores the capture flag (its
// writer is outside the corpus); it is read with acquire ordering.
static OCFMutableDictionaryRef __CFErrorCallBackTable;
static os_unfair_lock __CFErrorLock = OS_UNFAIR_LOCK_INIT;
static _Atomic(uint8_t) __CFErrorCallStackCaptureEnabled;

static OCFStringRef _CFErrorCopyUserInfoKeyFromUserInfo(OCFErrorRef err, OCFStringRef key);
static OCFStringRef _CFErrorCopyUserInfoKeyFromCallBack(OCFErrorRef err, OCFStringRef key);
static OCFStringRef _CFErrorCopyUserInfoKey(OCFErrorRef err, OCFStringRef key);
static void _CFErrorFormatDebugDescriptionAux(OCFErrorRef err, DescriptionContext *context);
static OCFDictionaryRef _CFErrorCreateEmptyDictionary(OCFAllocatorRef allocator);
static void userInfoKeyValueShow(const void *key, const void *value, void *context);
static const void *blockCopyValueCallBack(OCFAllocatorRef allocator, const void *value);
static void blockReleaseValueCallBack(OCFAllocatorRef allocator, const void *value);
static const void *_CFErrorPOSIXCallBack(OCFErrorRef err, OCFStringRef key);
static const void *_CFErrorMachCallBack(OCFErrorRef err, OCFStringRef key);
static const void *_CFErrorCocoaCallBack(OCFErrorRef err, OCFStringRef key);

// Every accessor of this unit tests for a tagged pointer (bit 63) first and asks __CF_IS_OBJC(27, err) only
// for an untagged one, once per access.
static inline bool errorIsObjC(OCFTypeRef err) {
    return ((uintptr_t)err >> 63) != 0 || __CF_IS_OBJC(kErrorTypeID, err) != 0;
}

static inline OCFIndex errorCode(OCFErrorRef err) {
    if (errorIsObjC(err)) {
        return ((OCFIndex (*)(id, SEL))objc_msgSend)((id)err, sel_getUid("code"));
    }
    return err->code;
}

static inline OCFStringRef errorDomain(OCFErrorRef err) {
    if (errorIsObjC(err)) {
        return ((OCFStringRef (*)(id, SEL))objc_msgSend)((id)err, sel_getUid("domain"));
    }
    return err->domain;
}

static inline OCFDictionaryRef errorUserInfo(OCFErrorRef err) {
    if (errorIsObjC(err)) {
        return ((OCFDictionaryRef (*)(id, SEL))objc_msgSend)((id)err, sel_getUid("userInfo"));
    }
    return err->userInfo;
}

// Native __CFErrorDeallocate, 0x1c01977a0. Non-external. Releases the domain, then the user info (neither
// tested for NULL), then removes the associated objects.
static void __CFErrorDeallocate(OCFTypeRef cf) {
    OCFErrorRef err = (OCFErrorRef)cf;
    OCFRelease(err->domain);
    OCFRelease(err->userInfo);
    objc_removeAssociatedObjects((id)cf);
}

// Native __CFErrorEqual, 0x1c01977d0. Non-external. Codes compare as integers, domains with CFEqual, then the
// copied user infos: equal pointers (both NULL included) are equal, one NULL is unequal, otherwise CFEqual.
// Both copies are released when not NULL.
static Boolean __CFErrorEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    OCFErrorRef err1 = (OCFErrorRef)cf1;
    OCFErrorRef err2 = (OCFErrorRef)cf2;
    OCFIndex code1 = errorCode(err1);
    if (code1 != errorCode(err2)) {
        return false;
    }
    OCFStringRef domain1 = errorDomain(err1);
    if (!OCFEqual(domain1, errorDomain(err2))) {
        return false;
    }
    OCFDictionaryRef userInfo1 = OCFErrorCopyUserInfo(err1);
    OCFDictionaryRef userInfo2 = OCFErrorCopyUserInfo(err2);
    Boolean result;
    if (userInfo1 == userInfo2) {
        result = true;
    } else if (userInfo1 == NULL || userInfo2 == NULL) {
        result = false;
    } else {
        result = OCFEqual(userInfo1, userInfo2) != 0;
    }
    if (userInfo1 != NULL) {
        OCFRelease(userInfo1);
    }
    if (userInfo2 != NULL) {
        OCFRelease(userInfo2);
    }
    return result;
}

// Native __CFErrorHash, 0x1c0197914. Non-external. The code plus the hash of the domain (ivars read
// directly).
static OCFHashCode __CFErrorHash(OCFTypeRef cf) {
    OCFErrorRef err = (OCFErrorRef)cf;
    OCFHashCode hash = OCFHash(err->domain);
    return (OCFHashCode)err->code + hash;
}

// Native __CFErrorCopyFormattingDescription, 0x1c0197940. Non-external. Programs linked on or after the build
// version 0x07DF0901 (platform 0xFFFFFFFF) get the debug description, others the localized description; the
// format options are not used.
static OCFStringRef __CFErrorCopyFormattingDescription(OCFTypeRef cf, const void *formatOptions) {
    (void)formatOptions;
    static const OCFBuildVersion kDebugDescriptionVersion = {0xFFFFFFFF, 0x07DF0901};
    if (dyld_program_sdk_at_least(kDebugDescriptionVersion)) {
        return _OCFErrorCreateDebugDescription((OCFErrorRef)cf);
    }
    return OCFErrorCopyDescription((OCFErrorRef)cf);
}

// Native __CFErrorCopyDescription, 0x1c0197980. Non-external; a tail call of _CFErrorCreateDebugDescription.
static OCFStringRef __CFErrorCopyDescription(OCFTypeRef cf) {
    return _OCFErrorCreateDebugDescription((OCFErrorRef)cf);
}

// Native __CFErrorClass, 0x22279db00 (__DATA_CONST,__const). Hidden (was private external); version 0, name
// "CFError", finalize, equal, hash, formatting and debug descriptions.
const OCFRuntimeClass __CFErrorClass = {
    0,
    "CFError",
    NULL,
    NULL,
    __CFErrorDeallocate,
    __CFErrorEqual,
    __CFErrorHash,
    __CFErrorCopyFormattingDescription,
    __CFErrorCopyDescription,
    NULL,
    NULL,
    0,
};

// Native kCFErrorLocalizedDescriptionKey, 0x22279da90 (__DATA_CONST,__const), a pointer to the constant
// string "NSLocalizedDescription" at 0x2227ae6c0 (flags 0x7c8; read with Local/Scratch/cf-core/macho.py).
// External; family J of batch 5 (the restored preference sources load it).
static const OCFConstantString localizedDescriptionKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSLocalizedDescription", 22
};
const OCFStringRef kOCFErrorLocalizedDescriptionKey = (OCFStringRef)&localizedDescriptionKeyString;

// Native kCFErrorLocalizedFailureReasonKey (0x22279daa0) and kCFErrorLocalizedRecoverySuggestionKey
// (0x22279daa8) (__DATA_CONST,__const), pointers to the constant strings "NSLocalizedFailureReason"
// (0x2227ae700) and "NSLocalizedRecoverySuggestion" (0x2227ae720), flags 0x7c8 (read in IDA). External; the
// bundle errors (_CFBundleCreateErrorDebug) store them.
static const OCFConstantString localizedFailureReasonKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSLocalizedFailureReason", 24
};
static const OCFConstantString localizedRecoverySuggestionKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSLocalizedRecoverySuggestion", 29
};
const OCFStringRef kOCFErrorLocalizedFailureReasonKey = (OCFStringRef)&localizedFailureReasonKeyString;
const OCFStringRef kOCFErrorLocalizedRecoverySuggestionKey =
    (OCFStringRef)&localizedRecoverySuggestionKeyString;

// Native kCFErrorDebugDescriptionKey, kCFErrorUnderlyingErrorKey, kCFErrorDomainPOSIX, kCFErrorDomainOSStatus
// and kCFErrorDomainCocoa, 0x22279dab8, 0x22279dac0, 0x22279dae0, 0x22279dae8 and 0x22279daf8
// (__DATA_CONST,__const), pointers to the constant strings at 0x2227ae760, 0x2227ae780, 0x2227ae7e0,
// 0x2227ae800 and 0x2227ae840. External. The other keys and domains of the table (0x22279da98-0x22279daf0)
// are not reached by the scope.
static const OCFConstantString debugDescriptionKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSDebugDescription", 18
};
static const OCFConstantString underlyingErrorKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSUnderlyingError", 17
};
static const OCFConstantString domainPOSIXString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(ONSPOSIXErrorDomain),
    sizeof(OF_NAME_STRING(ONSPOSIXErrorDomain)) - 1
};
static const OCFConstantString domainOSStatusString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSOSStatusErrorDomain", 21
};
static const OCFConstantString domainCocoaString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(ONSCocoaErrorDomain),
    sizeof(OF_NAME_STRING(ONSCocoaErrorDomain)) - 1
};
const OCFStringRef kOCFErrorDebugDescriptionKey = (OCFStringRef)&debugDescriptionKeyString;
const OCFStringRef kOCFErrorUnderlyingErrorKey = (OCFStringRef)&underlyingErrorKeyString;
const OCFStringRef kOCFErrorDomainPOSIX = (OCFStringRef)&domainPOSIXString;
const OCFStringRef kOCFErrorDomainOSStatus = (OCFStringRef)&domainOSStatusString;
const OCFStringRef kOCFErrorDomainCocoa = (OCFStringRef)&domainCocoaString;

// Native CFErrorGetTypeID, 0x1c0197984. External. The builtin type ID 27.
OCFTypeID OCFErrorGetTypeID(void) {
    return kErrorTypeID;
}

// Native _CFErrorCreateLocalizedDescription, 0x1c019798c. External. Two passes, the user info first and the
// domain call-back second, each asking NSLocalizedDescription (returned as is) and then NSLocalizedFailure;
// a failure is combined with NSLocalizedFailureReason (user info, then call-back) as "%@ %@", localized from
// the CoreFoundation bundle's "Error" table when the bundle exists and the localized format yields a string.
// Without either, the result is built from NSLocalizedFailureReason, or from the domain (localized from the
// bundle with the domain as key and value), the code and NSDescription. The strings keep the literal
// "\U2019" escape of the native constants. Every format uses the system default allocator.
OCFStringRef _OCFErrorCreateLocalizedDescription(OCFErrorRef err) {
    bool fromUserInfo = true;
    bool firstPass;
    do {
        firstPass = fromUserInfo;
        OCFStringRef (*copyKey)(OCFErrorRef, OCFStringRef) =
            fromUserInfo ? _CFErrorCopyUserInfoKeyFromUserInfo : _CFErrorCopyUserInfoKeyFromCallBack;
        OCFStringRef description = copyKey(err, OF_STR("NSLocalizedDescription"));
        if (description != NULL) {
            return description;
        }
        OCFStringRef failure = copyKey(err, OF_STR("NSLocalizedFailure"));
        if (failure != NULL) {
            OCFStringRef reason = _CFErrorCopyUserInfoKey(err, OF_STR("NSLocalizedFailureReason"));
            if (reason == NULL) {
                return failure;
            }
            OCFBundleRef bundle = OCFBundleGetBundleWithIdentifier(OF_STR("com.apple.CoreFoundation"));
            OCFAllocatorRef systemDefault;
            OCFStringRef result;
            if (bundle != NULL) {
                OCFStringRef format =
                    OCFBundleCopyLocalizedString(bundle, OF_STR("%@ %@"), OF_STR("%@ %@"), OF_STR("Error"));
                systemDefault = kOCFAllocatorSystemDefault;
                result = OCFStringCreateWithFormat(systemDefault, NULL, format, failure, reason);
                OCFRelease(format);
                if (result != NULL) {
                    OCFRelease(failure);
                    OCFRelease(reason);
                    return result;
                }
            } else {
                systemDefault = kOCFAllocatorSystemDefault;
            }
            result = OCFStringCreateWithFormat(systemDefault, NULL, OF_STR("%@ %@"), failure, reason);
            OCFRelease(failure);
            OCFRelease(reason);
            return result;
        }
        fromUserInfo = false;
    } while (firstPass);

    OCFBundleRef bundle = OCFBundleGetBundleWithIdentifier(OF_STR("com.apple.CoreFoundation"));
    OCFStringRef reason = _CFErrorCopyUserInfoKey(err, OF_STR("NSLocalizedFailureReason"));
    if (bundle == NULL) {
        if (reason != NULL) {
            OCFStringRef result = OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault,
                NULL,
                OF_STR("The operation couldn\\U2019t be completed. %@"),
                reason
            );
            OCFRelease(reason);
            return result;
        }
        OCFStringRef description = _CFErrorCopyUserInfoKey(err, OF_STR("NSDescription"));
        OCFAllocatorRef systemDefault = kOCFAllocatorSystemDefault;
        if (description != NULL) {
            OCFStringRef domain = errorDomain(err);
            OCFIndex code = errorCode(err);
            OCFStringRef result = OCFStringCreateWithFormat(
                systemDefault,
                NULL,
                OF_STR("The operation couldn\\U2019t be completed. (%@ error %ld - %@)"),
                domain,
                code,
                description
            );
            OCFRelease(description);
            return result;
        }
        OCFStringRef domain = errorDomain(err);
        OCFIndex code = errorCode(err);
        return OCFStringCreateWithFormat(
            systemDefault,
            NULL,
            OF_STR("The operation couldn\\U2019t be completed. (%@ error %ld.)"),
            domain,
            code
        );
    }
    if (reason != NULL) {
        OCFStringRef format = OCFBundleCopyLocalizedString(
            bundle,
            OF_STR("The operation couldn\\U2019t be completed. %@"),
            OF_STR("The operation couldn\\U2019t be completed. %@"),
            OF_STR("Error")
        );
        OCFStringRef result = OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, format, reason);
        OCFRelease(format);
        OCFRelease(reason);
        return result;
    }
    OCFStringRef description = _CFErrorCopyUserInfoKey(err, OF_STR("NSDescription"));
    OCFStringRef domainKey = errorDomain(err);
    OCFStringRef domainValue = errorDomain(err);
    OCFStringRef localizedDomain =
        OCFBundleCopyLocalizedString(bundle, domainKey, domainValue, OF_STR("Error"));
    OCFStringRef result;
    if (description != NULL) {
        OCFStringRef format = OCFBundleCopyLocalizedString(
            bundle,
            OF_STR("The operation couldn\\U2019t be completed. (%@ error %ld - %@)"),
            OF_STR("The operation couldn\\U2019t be completed. (%@ error %ld - %@)"),
            OF_STR("Error")
        );
        OCFAllocatorRef systemDefault = kOCFAllocatorSystemDefault;
        OCFIndex code = errorCode(err);
        result = OCFStringCreateWithFormat(systemDefault, NULL, format, localizedDomain, code, description);
        OCFRelease(format);
        OCFRelease(description);
    } else {
        OCFStringRef format = OCFBundleCopyLocalizedString(
            bundle,
            OF_STR("The operation couldn\\U2019t be completed. (%@ error %ld.)"),
            OF_STR("The operation couldn\\U2019t be completed. (%@ error %ld.)"),
            OF_STR("Error")
        );
        OCFAllocatorRef systemDefault = kOCFAllocatorSystemDefault;
        OCFIndex code = errorCode(err);
        result = OCFStringCreateWithFormat(systemDefault, NULL, format, localizedDomain, code);
        OCFRelease(format);
    }
    OCFRelease(localizedDomain);
    return result;
}

// Native _CFErrorCopyUserInfoKeyFromUserInfo, 0x1c0197e1c. Non-external. The user-info value for the key,
// retained, or NULL when there is no user info or no value.
static OCFStringRef _CFErrorCopyUserInfoKeyFromUserInfo(OCFErrorRef err, OCFStringRef key) {
    OCFDictionaryRef userInfo = errorUserInfo(err);
    if (userInfo == NULL) {
        return NULL;
    }
    const void *value = OCFDictionaryGetValue(userInfo, key);
    if (value != NULL) {
        OCFRetain(value);
    }
    return value;
}

// Native _CFErrorCopyUserInfoKeyFromCallBack, 0x1c0197e88. Non-external. The domain's call-back block
// (retained by CFErrorCopyCallBackBlockForDomain) is called with the error and the key; its result is
// retained and the block released.
static OCFStringRef _CFErrorCopyUserInfoKeyFromCallBack(OCFErrorRef err, OCFStringRef key) {
    UserInfoKeyCallBackBlock callBack = OCFErrorCopyCallBackBlockForDomain(errorDomain(err));
    if (callBack == NULL) {
        return NULL;
    }
    const void *value = callBack(err, key);
    if (value != NULL) {
        OCFRetain(value);
    }
    OCFRelease(callBack);
    return value;
}

// Native _CFErrorCopyUserInfoKey, 0x1c0197f14. Non-external. The user info first, then the domain call-back.
static OCFStringRef _CFErrorCopyUserInfoKey(OCFErrorRef err, OCFStringRef key) {
    OCFStringRef value = _CFErrorCopyUserInfoKeyFromUserInfo(err, key);
    if (value != NULL) {
        return value;
    }
    return _CFErrorCopyUserInfoKeyFromCallBack(err, key);
}

// Native CFErrorGetDomain, 0x1c0197f50. External.
OCFErrorDomain OCFErrorGetDomain(OCFErrorRef err) {
    if (errorIsObjC(err)) {
        return ((OCFStringRef (*)(id, SEL))objc_msgSend)((id)err, sel_getUid("domain"));
    }
    return err->domain;
}

// Native CFErrorGetCode, 0x1c0197f94. External.
OCFIndex OCFErrorGetCode(OCFErrorRef err) {
    if (errorIsObjC(err)) {
        return ((OCFIndex (*)(id, SEL))objc_msgSend)((id)err, sel_getUid("code"));
    }
    return err->code;
}

// Native _CFErrorCreateDebugDescription, 0x1c0197ff0. External. A mutable string of the system default
// allocator, formatted at depth 0 with the localized flag set and the redacted flag clear.
OCFStringRef _OCFErrorCreateDebugDescription(OCFErrorRef err) {
    DescriptionContext context;
    context.string = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
    context.depth = 0;
    context.localized = 1;
    context.redacted = 0;
    _CFErrorFormatDebugDescriptionAux(err, &context);
    return context.string;
}

// Native _CFErrorFormatDebugDescriptionAux, 0x1c0198074. Non-external. The description is NSDescription,
// else NSDebugDescription (which is then not repeated), else, for a localized pass only,
// NSLocalizedDescription or NSLocalizedFailureReason. Appends "Error Domain=%@ Code=%ld", the description in
// quotes (even when NULL; redacted passes append -redactedDescription only when the object responds), the
// debug description in parentheses when it is nonempty and differs, and up to three levels of user info
// (deeper levels print the dictionary pointer). After the user info, a trailing ", " is replaced with "}" or
// "}" is inserted at the end. The user info, domain and code are read in that order.
static void _CFErrorFormatDebugDescriptionAux(OCFErrorRef err, DescriptionContext *context) {
    OCFStringRef debugDescription = _CFErrorCopyUserInfoKey(err, OF_STR("NSDebugDescription"));
    OCFStringRef description = _CFErrorCopyUserInfoKey(err, OF_STR("NSDescription"));
    if (description == NULL) {
        if (debugDescription != NULL) {
            description = debugDescription;
        } else if (context->localized != 0) {
            description = _CFErrorCopyUserInfoKey(err, OF_STR("NSLocalizedDescription"));
            if (description == NULL) {
                description = _CFErrorCopyUserInfoKey(err, OF_STR("NSLocalizedFailureReason"));
            }
        } else {
            description = NULL;
        }
        debugDescription = NULL;
    }
    OCFDictionaryRef userInfo = errorUserInfo(err);
    OCFMutableStringRef string = context->string;
    OCFStringRef domain = errorDomain(err);
    OCFIndex code = errorCode(err);
    OCFStringAppendFormat(string, NULL, OF_STR("Error Domain=%@ Code=%ld"), domain, code);
    if (context->redacted != 0) {
        if (objc_opt_respondsToSelector((id)description, sel_getUid("redactedDescription"))) {
            OCFMutableStringRef target = context->string;
            OCFStringRef redacted =
                ((OCFStringRef (*)(id, SEL))objc_msgSend)((id)description, sel_getUid("redactedDescription"));
            OCFStringAppendFormat(target, NULL, OF_STR(" \"%@\""), redacted);
        }
    } else {
        OCFStringAppendFormat(context->string, NULL, OF_STR(" \"%@\""), description);
    }
    if (description != NULL && debugDescription != NULL && OCFStringGetLength(debugDescription) >= 1 &&
        !OCFEqual(description, debugDescription)) {
        if (context->redacted != 0) {
            if (objc_opt_respondsToSelector((id)debugDescription, sel_getUid("redactedDescription"))) {
                OCFMutableStringRef target = context->string;
                OCFStringRef redacted = ((OCFStringRef (*)(id, SEL))objc_msgSend)(
                    (id)debugDescription, sel_getUid("redactedDescription")
                );
                OCFStringAppendFormat(target, NULL, OF_STR(" (%@)"), redacted);
            }
        } else {
            OCFStringAppendFormat(context->string, NULL, OF_STR(" (%@)"), debugDescription);
        }
    }
    if (userInfo != NULL && OCFDictionaryGetCount(userInfo) != 0) {
        OCFIndex depth = context->depth;
        if (depth > 2) {
            OCFStringAppendFormat(context->string, NULL, OF_STR(" UserInfo=%p (not displayed)"), userInfo);
        } else {
            context->depth = depth + 1;
            OCFStringAppend(context->string, OF_STR(" UserInfo={"));
            OCFDictionaryApplyFunction(userInfo, userInfoKeyValueShow, context);
            OCFIndex suffix = OCFStringHasSuffix(context->string, OF_STR(", ")) ? 2 : 0;
            OCFMutableStringRef target = context->string;
            OCFIndex length = OCFStringGetLength(target);
            OCFStringReplace(target, (OCFRange){length - suffix, suffix}, OF_STR("}"));
            context->depth = context->depth - 1;
        }
    }
    if (debugDescription != NULL) {
        OCFRelease(debugDescription);
    }
    if (description != NULL) {
        OCFRelease(description);
    }
}

// Native _CFErrorCreateUnlocalizedDebugDescription, 0x1c0198384. External. As the debug description with
// both flags clear.
OCFStringRef _OCFErrorCreateUnlocalizedDebugDescription(OCFErrorRef err) {
    DescriptionContext context;
    context.string = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
    context.depth = 0;
    context.localized = 0;
    context.redacted = 0;
    _CFErrorFormatDebugDescriptionAux(err, &context);
    return context.string;
}

// Native _CFErrorCreateRedactedDescription, 0x1c0198404. External. iOS Device divergence: unread.
OCFStringRef _OCFErrorCreateRedactedDescription(OCFTypeRef err) {
    DescriptionContext context = {
        .string = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0),
        .depth = 0,
        .localized = 1,
        .redacted = 1,
    };
    _CFErrorFormatDebugDescriptionAux((OCFErrorRef)err, &context);
    return context.string;
}

// Native CFErrorCreate, 0x1c0198488. External. A type-27 instance with 24 extra bytes and no category; the
// domain is copied (CFStringCreateCopy) and stored with the code, the user info copied or replaced by an
// empty dictionary, and the call stack recorded when capture is enabled. The arguments are not checked.
OCFErrorRef
OCFErrorCreate(OCFAllocatorRef allocator, OCFStringRef domain, OCFIndex code, OCFDictionaryRef userInfo) {
    struct __OCFError *err =
        (struct __OCFError *)_OCFRuntimeCreateInstance(allocator, kErrorTypeID, 24, NULL);
    if (err != NULL) {
        OCFStringRef domainCopy = OCFStringCreateCopy(allocator, domain);
        err->code = code;
        err->domain = domainCopy;
        if (userInfo != NULL) {
            err->userInfo = OCFDictionaryCreateCopy(allocator, userInfo);
        } else {
            err->userInfo = _CFErrorCreateEmptyDictionary(allocator);
        }
        _OCFErrorSetCallStackReturnAddresses(err, NULL);
    }
    return err;
}

// Native CFErrorCreateWithUserInfoKeysAndValues, 0x1c0198608. External. The keys and values become a
// dictionary (copying string keys, retaining values) that the new error retains; the dictionary is then
// released. Reading: Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3; iOS Device
// divergence: unread).
OCFErrorRef OCFErrorCreateWithUserInfoKeysAndValues(
    OCFAllocatorRef allocator,
    OCFStringRef domain,
    OCFIndex code,
    const void *const *userInfoKeys,
    const void *const *userInfoValues,
    OCFIndex numUserInfoValues
) {
    OCFDictionaryRef userInfo = OCFDictionaryCreate(
        allocator,
        (const void **)userInfoKeys,
        (const void **)userInfoValues,
        numUserInfoValues,
        &kOCFCopyStringDictionaryKeyCallBacks,
        &kOCFTypeDictionaryValueCallBacks
    );
    if (userInfo == NULL) {
        return NULL;
    }
    OCFErrorRef error = OCFErrorCreate(allocator, domain, code, userInfo);
    OCFRelease(userInfo);
    return error;
}

// Native _CFErrorCreateEmptyDictionary, 0x1c0198514. Non-external. A NULL allocator is the thread's default
// (_CFGetTSD(1), else the system default). The system default allocator, or the default allocator when it is
// the system default, gets the retained empty dictionary instance; any other allocator a new empty dictionary
// with the copy-string key and type value call-backs.
static OCFDictionaryRef _CFErrorCreateEmptyDictionary(OCFAllocatorRef allocator) {
    OCFAllocatorRef systemDefault;
    if (allocator == NULL) {
        OCFAllocatorRef current = (OCFAllocatorRef)_OCFGetTSD(1);
        systemDefault = kOCFAllocatorSystemDefault;
        allocator = current == NULL ? systemDefault : current;
    } else {
        systemDefault = kOCFAllocatorSystemDefault;
    }
    if (allocator != systemDefault) {
        if (allocator != NULL && kOCFAllocatorDefault != allocator) {
            return OCFDictionaryCreate(
                allocator,
                NULL,
                NULL,
                0,
                &kOCFCopyStringDictionaryKeyCallBacks,
                &kOCFTypeDictionaryValueCallBacks
            );
        }
        if (systemDefault != OCFAllocatorGetDefault()) {
            return OCFDictionaryCreate(
                allocator,
                NULL,
                NULL,
                0,
                &kOCFCopyStringDictionaryKeyCallBacks,
                &kOCFTypeDictionaryValueCallBacks
            );
        }
    }
    return OCFRetain(__ONSDictionary0__);
}

// Native _CFErrorSetCallStackReturnAddresses, 0x1c01985c4. External. A non-NULL value is set as the
// "CFErrorCallStacks" associated object (policy 0x303, copy). With NULL, when the capture flag reads 1
// (acquire), _CFErrorSetCallStackReturnAddresses.cold.1 (0x1c0307fd0) sets the current call stack
// (+callStackReturnAddresses, policy 0x301, retain) inside an autorelease pool.
void _OCFErrorSetCallStackReturnAddresses(OCFErrorRef err, OCFArrayRef callStackReturnAddresses) {
    if (callStackReturnAddresses != NULL) {
        objc_setAssociatedObject((id)err, callStacksKey, (id)callStackReturnAddresses, 0x303);
        return;
    }
    if (atomic_load_explicit(&__CFErrorCallStackCaptureEnabled, memory_order_acquire) == 1) {
        void *pool = objc_autoreleasePoolPush();
        id addresses =
            ((id (*)(id, SEL))objc_msgSend)((id)callStackClass, sel_getUid("callStackReturnAddresses"));
        objc_setAssociatedObject((id)err, callStacksKey, addresses, 0x301);
        objc_autoreleasePoolPop(pool);
    }
}

// Native CFErrorCopyUserInfo, 0x1c0198688. External. The user info retained, or an empty dictionary of the
// error's allocator when there is none.
OCFDictionaryRef OCFErrorCopyUserInfo(OCFErrorRef err) {
    OCFDictionaryRef userInfo = errorUserInfo(err);
    if (userInfo != NULL) {
        return OCFRetain(userInfo);
    }
    return _CFErrorCreateEmptyDictionary(OCFGetAllocator(err));
}

// Native CFErrorCopyDescription, 0x1c01986e0. External. Objective-C errors answer -localizedDescription,
// retained when not NULL; CF errors the localized description.
OCFStringRef OCFErrorCopyDescription(OCFErrorRef err) {
    if (!errorIsObjC(err)) {
        return _OCFErrorCreateLocalizedDescription(err);
    }
    OCFStringRef description =
        ((OCFStringRef (*)(id, SEL))objc_msgSend)((id)err, sel_getUid("localizedDescription"));
    if (description != NULL) {
        return OCFRetain(description);
    }
    return description;
}

// The body of __CFErrorSetCallBackForDomainNoLock, which the native code inlines into
// __CFErrorInitializeCallBackTable (it has no symbol of its own): a stack block capturing the function
// (descriptor ___block_descriptor_40_e89_..., 40 bytes; the invoke is ___CFErrorSetCallBackForDomainNoLock_
// block_invoke, 0x1c0198fe8, which calls the function with the error and the key and autoreleases a non-NULL
// result) is set as the domain's value; the table's value call-backs copy it.
static inline __attribute__((always_inline)) void
__CFErrorSetCallBackForDomainNoLock(OCFStringRef domainName, UserInfoKeyCallBack callBack) {
    UserInfoKeyCallBackBlock block = ^const void *(OCFErrorRef err, OCFStringRef key) {
        const void *value = callBack(err, key);
        if (value != NULL) {
            OCFAutorelease(value);
        }
        return value;
    };
    OCFDictionarySetValue(__CFErrorCallBackTable, domainName, block);
}

// Native _CFErrorInitializeCallBackTable, 0x1c0198864. Non-external. The table is created before taking the
// lock (copy-string keys; the type value call-backs with Block_copy and Block_release as retain and release)
// and released when another thread installed one first; otherwise the POSIX, Mach and Cocoa call-backs are
// set, in that order, under the lock.
static void _CFErrorInitializeCallBackTable(void) {
    OCFDictionaryValueCallBacks valueCallBacks = kOCFTypeDictionaryValueCallBacks;
    valueCallBacks.retain = blockCopyValueCallBack;
    valueCallBacks.release = blockReleaseValueCallBack;
    OCFMutableDictionaryRef table = OCFDictionaryCreateMutable(
        kOCFAllocatorSystemDefault, 0, &kOCFCopyStringDictionaryKeyCallBacks, &valueCallBacks
    );
    os_unfair_lock_lock_with_options(&__CFErrorLock, 0x10000);
    if (__CFErrorCallBackTable != NULL) {
        os_unfair_lock_unlock(&__CFErrorLock);
        OCFRelease(table);
        return;
    }
    __CFErrorCallBackTable = table;
    __CFErrorSetCallBackForDomainNoLock(OF_STR(OF_NAME_STRING(ONSPOSIXErrorDomain)), _CFErrorPOSIXCallBack);
    __CFErrorSetCallBackForDomainNoLock(OF_STR("NSMachErrorDomain"), _CFErrorMachCallBack);
    __CFErrorSetCallBackForDomainNoLock(OF_STR(OF_NAME_STRING(ONSCocoaErrorDomain)), _CFErrorCocoaCallBack);
    os_unfair_lock_unlock(&__CFErrorLock);
}

// Native CFErrorCopyCallBackBlockForDomain, 0x1c0198a64. External. The table is initialized when unset
// (tested without the lock); the block is looked up and retained under the lock.
UserInfoKeyCallBackBlock OCFErrorCopyCallBackBlockForDomain(OCFStringRef domainName) {
    if (__CFErrorCallBackTable == NULL) {
        _CFErrorInitializeCallBackTable();
    }
    os_unfair_lock_lock_with_options(&__CFErrorLock, 0x10000);
    const void *block = OCFDictionaryGetValue(__CFErrorCallBackTable, domainName);
    if (block != NULL) {
        OCFRetain(block);
    }
    os_unfair_lock_unlock(&__CFErrorLock);
    return (UserInfoKeyCallBackBlock)block;
}

// Native _CFErrorCopyCallStackReturnAddresses, 0x1c0198b90. External. iOS Device divergence: unread.
OCFArrayRef _OCFErrorCopyCallStackReturnAddresses(OCFTypeRef err) {
    OCFArrayRef addresses = (OCFArrayRef)objc_getAssociatedObject((id)err, callStacksKey);
    if (addresses != NULL) {
        return OCFRetain(addresses);
    }
    return NULL;
}

// Native userInfoKeyValueShow, 0x1c0198bb8. Non-external; the user-info applier of the debug description. A
// NULL context is userInfoKeyValueShow.cold.1 (0x1c030801c, crash annotation, BRK #1). An NSUnderlyingError
// value of type 27 is described recursively between "%@=%p {" and "}, " (CFEqual on the key is evaluated
// first, always); otherwise "%@=%@, ", or for a redacted pass the value's -redactedDescription or
// "<private>".
static void userInfoKeyValueShow(const void *key, const void *value, void *context) {
    if (context == NULL) {
        crashWithMessage("*** userInfoKeyValueShow() called with NULL context ***");
    }
    DescriptionContext *description = (DescriptionContext *)context;
    Boolean underlying = OCFEqual(key, OF_STR("NSUnderlyingError"));
    if (value != NULL && underlying && OCFGetTypeID(value) == kErrorTypeID) {
        OCFStringAppendFormat(description->string, NULL, OF_STR("%@=%p {"), key, value);
        _CFErrorFormatDebugDescriptionAux((OCFErrorRef)value, description);
        OCFStringAppend(description->string, OF_STR("}, "));
        return;
    }
    if (description->redacted != 0) {
        BOOL responds = objc_opt_respondsToSelector((id)value, sel_getUid("redactedDescription"));
        OCFMutableStringRef string = description->string;
        if (responds) {
            OCFStringRef redacted =
                ((OCFStringRef (*)(id, SEL))objc_msgSend)((id)value, sel_getUid("redactedDescription"));
            OCFStringAppendFormat(string, NULL, OF_STR("%@=%@, "), key, redacted);
        } else {
            OCFStringAppendFormat(string, NULL, OF_STR("%@=<private>, "), key);
        }
        return;
    }
    OCFStringAppendFormat(description->string, NULL, OF_STR("%@=%@, "), key, value);
}

// Native blockCopyValueCallBack, 0x1c0198cc0. Non-external; a tail call of _Block_copy on the value.
static const void *blockCopyValueCallBack(OCFAllocatorRef allocator, const void *value) {
    (void)allocator;
    return _Block_copy(value);
}

// Native blockReleaseValueCallBack, 0x1c0198cc8. Non-external; a tail call of _Block_release on the value.
static void blockReleaseValueCallBack(OCFAllocatorRef allocator, const void *value) {
    (void)allocator;
    _Block_release(value);
}

// Native _CFErrorPOSIXCallBack, 0x1c0198cd0. Non-external. For NSDescription or NSLocalizedFailureReason:
// strerror of the code (truncated to int) as a UTF-8 string of the system default allocator (NULL for an
// empty or NULL result). For NSLocalizedFailureReason, the string is replaced by its localization from the
// ErrnoErrors table of <first user-domain (8) library (5) directory>/CoreServices/CoreTypes.bundle; a
// missing localization ("%%NOTFOUND%%", compared by identity and released) answers NULL. When a step of the
// bundle lookup fails, the strerror string is answered.
static const void *_CFErrorPOSIXCallBack(OCFErrorRef err, OCFStringRef key) {
    if (!OCFEqual(key, OF_STR("NSDescription")) && !OCFEqual(key, OF_STR("NSLocalizedFailureReason"))) {
        return NULL;
    }
    const char *message = strerror((int)errorCode(err));
    if (message == NULL || message[0] == 0) {
        return NULL;
    }
    OCFAllocatorRef systemDefault = kOCFAllocatorSystemDefault;
    OCFStringRef result = OCFStringCreateWithCString(systemDefault, message, 0x8000100);
    if (result == NULL || OCFEqual(key, OF_STR("NSDescription"))) {
        return result;
    }
    OCFArrayRef paths = OCFCopySearchPathForDirectoriesInDomains(5, 8, 0);
    if (paths == NULL) {
        return result;
    }
    if (OCFArrayGetCount(paths) >= 1) {
        OCFStringRef library =
            OCFURLCopyFileSystemPath((OCFURLRef)OCFArrayGetValueAtIndex(paths, 0), kOCFURLPOSIXPathStyle);
        if (library != NULL) {
            OCFStringRef path = OCFStringCreateWithFormat(
                systemDefault, NULL, OF_STR("%@/CoreServices/CoreTypes.bundle"), library
            );
            OCFURLRef url = OCFURLCreateWithFileSystemPath(systemDefault, path, kOCFURLPOSIXPathStyle, false);
            OCFRelease(library);
            if (url != NULL) {
                OCFBundleRef bundle = OCFBundleCreate(systemDefault, url);
                if (bundle != NULL) {
                    OCFStringRef notFound = OF_STR("%%NOTFOUND%%");
                    OCFStringRef localized =
                        OCFBundleCopyLocalizedString(bundle, result, notFound, OF_STR("ErrnoErrors"));
                    if (localized == notFound) {
                        OCFRelease(notFound);
                        localized = NULL;
                    }
                    OCFRelease(result);
                    OCFRelease(bundle);
                    result = localized;
                }
                OCFRelease(url);
            }
            OCFRelease(path);
        }
    }
    OCFRelease(paths);
    return result;
}

// Native _CFErrorMachCallBack, 0x1c0198eac. Non-external. For NSDescription only: mach_error_string of the
// code (truncated to 32 bits) as a UTF-8 string of the system default allocator, NULL for an empty or NULL
// result.
static const void *_CFErrorMachCallBack(OCFErrorRef err, OCFStringRef key) {
    if (!OCFEqual(key, OF_STR("NSDescription"))) {
        return NULL;
    }
    const char *message = mach_error_string((mach_error_t)errorCode(err));
    if (message == NULL || message[0] == 0) {
        return NULL;
    }
    return OCFStringCreateWithCString(kOCFAllocatorSystemDefault, message, 0x8000100);
}

// Native _CFErrorCocoaCallBack, 0x1c0198f3c. Non-external. The implementation of
// -_retainedUserInfoCallBackForKey: of NSError (objc_lookUpClass, then class_getMethodImplementation) is
// cached in __CFErrorCocoaCallBack.cocoaCallBack (0x22e6d2828, __DATA,__bss) and called with the error, the
// registered selector and the key; NULL when the class or the method is missing. In this restoration the
// class name is the selected NSError spelling (ONSError in prefixed mode), so that a prefixed build never
// reaches the system Foundation class.
static const void *_CFErrorCocoaCallBack(OCFErrorRef err, OCFStringRef key) {
    static IMP cocoaCallBack;
    IMP implementation = cocoaCallBack;
    if (implementation == NULL) {
        Class cls = objc_lookUpClass(OF_NAME_STRING(ONSError));
        if (cls != NULL) {
            implementation =
                class_getMethodImplementation(cls, sel_registerName("_retainedUserInfoCallBackForKey:"));
            cocoaCallBack = implementation;
            if (implementation == NULL) {
                return NULL;
            }
        } else {
            implementation = cocoaCallBack;
            if (implementation == NULL) {
                return NULL;
            }
        }
    }
    SEL selector = sel_registerName("_retainedUserInfoCallBackForKey:");
    return ((const void *(*)(OCFErrorRef, SEL, OCFStringRef))implementation)(err, selector, key);
}

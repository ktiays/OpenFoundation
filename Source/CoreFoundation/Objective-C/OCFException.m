//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/String/OCFStringFormat.h"

#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <os/lock.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

// Inferred organization: __NSFastEnumerationMutationHandler (0x1c028737c, after the fast-enumeration helpers)
// and the exception functions __exceptionInit through objectIsKindOfClass (0x1c0287e48-0x1c0288550, after the
// NSException methods) lie in the Objective-C region of the iOS Simulator __text section; the unit is
// Objective-C. _CFThrowFormattedException (0x1c0308c0c) and _CFRaiseMemoryException (0x1c030f268) lie in the
// image's split-off region after the hot text, where the linker places .cold fragments and functions that
// never return, so their addresses do not establish a unit (their neighbours are _mutateError of the
// __NSCFString methods and the NSEnumerator methods); they join this unit because both only build and throw
// an NSException, as its functions do (confidence: medium for that placement). The NSException methods are
// outside this unit's scope. The native source path is not in the evidence. Unless a definition says
// otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified
// (assembly); iOS Device 24A446: unread.
//
// The NSException object fields read here: name at 8 and the reserved dictionary at 32 (0x20), as the native
// functions address them.

// libobjc entry points the native unit imports whose declarations the SDK does not install (objc-internal.h);
// supplied, never reimplemented.
extern id objc_opt_new(Class cls);
extern BOOL objc_opt_respondsToSelector(id object, SEL selector);
extern BOOL objc_opt_isKindOfClass(id object, Class cls);
extern Class objc_opt_class(id object);

// The bridged exception class (the native __SCR_NSException GOT slot holds _OBJC_CLASS_$_NSException).
extern char exceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));

typedef struct {
    Class isa;
    OCFStringRef name;
    OCFStringRef reason;
    id userInfo;
    id reserved;
} ExceptionFields;

// Native __last_exception_backtrace__ (0x22e6cff79, __DATA,__cf_except_bt, 8,192 bytes),
// __last_exception_class_name__, __last_exception_name__ and __last_exception_os_log_pack__ (0x22e6d1f80,
// 0x22e6d1f88 and 0x22e6d1f90, __DATA,__cf_except_pack; the last 1,024 bytes) and
// __last_exception_backtrace_lock__ (0x22e6d3788, __DATA,__bss). Non-external; crash reporting reads the two
// named sections.
__attribute__((section("__DATA,__cf_except_bt"), used)) static char __last_exception_backtrace__[0x2000];
__attribute__((section("__DATA,__cf_except_pack"), used)) static const char *__last_exception_class_name__;
__attribute__((section("__DATA,__cf_except_pack"), used)) static const char *__last_exception_name__;
__attribute__((section("__DATA,__cf_except_pack"), used)) static char __last_exception_os_log_pack__[0x400];
static os_unfair_lock __last_exception_backtrace_lock__ = OS_UNFAIR_LOCK_INIT;

// Native __uncaughtExceptionHandler, 0x22e6d3780 (__DATA,__bss). Non-external; the handler installed by
// NSSetUncaughtExceptionHandler, which is outside this unit's scope (nothing sets it on this branch).
static void (*__uncaughtExceptionHandler)(id exception);

// Native __handleUncaughtException's buffer sizes, 0x1c046c330 (__TEXT,__const: 0x2000, 0x1000, 0x800, 0x400)
// and the fifth size 0x200 (MOV W8, #0x200), tried in order.
static const size_t kUncaughtBufferSizes[5] = {0x2000, 0x1000, 0x800, 0x400, 0x200};

// Native NSGenericException (0x2227a8440), NSInvalidArgumentException (0x2227a8450),
// NSInternalInconsistencyException (0x2227a8458) and NSMallocException (0x2227a8460), __DATA_CONST,__const.
// External. Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta6.json ("data"); each pointer and
// its constant string (0x2227b8700, 0x2227b8740, 0x2227b8760 and 0x2227b8780, __DATA_CONST,__cfstring: the
// constant-string class, flags 0x7c8, the name's ASCII bytes and its length) read with
// Local/Scratch/cf-core/macho.py. The four lie in one run of exception names after the NSEnumerator unit's
// __NSEnumerator0__, with NSRangeException (0x2227a8448, string 0x2227b8720) between the first two, restored
// because the restored array and set methods raise it; the run is taken to be the NSException unit's, which
// this unit restores in part (confidence: medium for that placement). Each string is the exception's name, a
// runtime name: it is spelled with OF_NAME_STRING, so it reads ONS... in the prefixed build and NS... (the
// native bytes) in the system-name build. The restoration keeps the strings in __DATA_CONST,__const rather
// than __cfstring.
static const OCFConstantString genericExceptionString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(ONSGenericException),
    sizeof(OF_NAME_STRING(ONSGenericException)) - 1
};
const OCFStringRef ONSGenericException = (OCFStringRef)&genericExceptionString;

static const OCFConstantString rangeExceptionString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(ONSRangeException),
    sizeof(OF_NAME_STRING(ONSRangeException)) - 1
};
const OCFStringRef ONSRangeException = (OCFStringRef)&rangeExceptionString;

static const OCFConstantString invalidArgumentExceptionString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(ONSInvalidArgumentException),
    sizeof(OF_NAME_STRING(ONSInvalidArgumentException)) - 1
};
const OCFStringRef ONSInvalidArgumentException = (OCFStringRef)&invalidArgumentExceptionString;

static const OCFConstantString internalInconsistencyExceptionString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(ONSInternalInconsistencyException),
    sizeof(OF_NAME_STRING(ONSInternalInconsistencyException)) - 1
};
const OCFStringRef ONSInternalInconsistencyException = (OCFStringRef)&internalInconsistencyExceptionString;

static const OCFConstantString mallocExceptionString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(ONSMallocException),
    sizeof(OF_NAME_STRING(ONSMallocException)) - 1
};
const OCFStringRef ONSMallocException = (OCFStringRef)&mallocExceptionString;

static id exceptionNamed(OCFStringRef name, id reason) {
    return ((id (*)(id, SEL, OCFStringRef, id, id))objc_msgSend)(
        (id)exceptionClass, sel_getUid("exceptionWithName:reason:userInfo:"), name, reason, nil
    );
}

static id send(id receiver, const char *selector) {
    return ((id (*)(id, SEL))objc_msgSend)(receiver, sel_getUid(selector));
}

static id objectForKey(id dictionary, OCFStringRef key) {
    return ((id (*)(id, SEL, OCFStringRef))objc_msgSend)(dictionary, sel_getUid("objectForKey:"), key);
}

// Native __NSFastEnumerationMutationHandler, 0x1c028737c. Hidden (was a private external). Throws an
// NSGenericException whose reason is the autoreleased "*** Collection <%s: %p> was mutated while being
// enumerated." (the class name from objc_opt_class, kCFAllocatorSystemDefault; the pool argument 0).
void __NSFastEnumerationMutationHandler(void *object) {
    Class cls = objc_opt_class((id)object);
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFStringRef string = OCFStringCreateWithFormat(
        allocator,
        NULL,
        OF_STR("*** Collection <%s: %p> was mutated while being enumerated."),
        class_getName(cls),
        object
    );
    id reason = _OCFAutoreleasePoolAddObject(NULL, string);
    objc_exception_throw(exceptionNamed(ONSGenericException, reason));
}

// Native __exceptionInit, 0x1c0287e48. Hidden (was a private external). Installs the exception preprocessor,
// the matcher and the uncaught-exception handler (the last a tail call).
void __exceptionInit(void) {
    objc_setExceptionPreprocessor((objc_exception_preprocessor)__exceptionPreprocess);
    objc_setExceptionMatcher((objc_exception_matcher)__exceptionMatch);
    objc_setUncaughtExceptionHandler((objc_uncaught_exception_handler)__handleUncaughtException);
}

// Native __exceptionPreprocess, 0x1c0287e78. Non-external; installed by __exceptionInit. For an NSException
// whose reserved dictionary does not yet hold "callStackReturnAddresses" or "callStackSymbols" (a missing
// dictionary is created as an NSMutableDictionary from __CFLookUpClass with objc_opt_new and stored), and
// unless userInfo's "NSExceptionOmitCallstacks" is true (bit 0 of boolValue), NSThread's
// callStackReturnAddresses and callStackSymbols are stored when non-nil. The exception is returned.
id __exceptionPreprocess(id exception) {
    if (objectIsKindOfClass(exception, (Class)exceptionClass)) {
        ExceptionFields *fields = (ExceptionFields *)exception;
        id reserved = fields->reserved;
        if (reserved != nil) {
            if (objectForKey(reserved, OF_STR("callStackReturnAddresses")) != nil) {
                return exception;
            }
            if (objectForKey(reserved, OF_STR("callStackSymbols")) != nil) {
                return exception;
            }
        } else {
            reserved = objc_opt_new((Class)__CFLookUpClass(OF_NAME_STRING(ONSMutableDictionary)));
            fields->reserved = reserved;
        }
        id userInfo = send(exception, "userInfo");
        id omit = objectForKey(userInfo, OF_STR("NSExceptionOmitCallstacks"));
        BOOL omitCallstacks = ((BOOL (*)(id, SEL))objc_msgSend)(omit, sel_getUid("boolValue"));
        if ((omitCallstacks & 1) == 0) {
            id thread = (id)__CFLookUpClass(OF_NAME_STRING(ONSThread));
            id addresses = send(thread, "callStackReturnAddresses");
            id symbols = send(thread, "callStackSymbols");
            SEL setObjectForKey = sel_getUid("setObject:forKey:");
            if (addresses != nil) {
                ((void (*)(id, SEL, id, OCFStringRef))objc_msgSend)(
                    reserved, setObjectForKey, addresses, OF_STR("callStackReturnAddresses")
                );
            }
            if (symbols != nil) {
                ((void (*)(id, SEL, id, OCFStringRef))objc_msgSend)(
                    reserved, setObjectForKey, symbols, OF_STR("callStackSymbols")
                );
            }
        }
    }
    return exception;
}

// Native __exceptionMatch, 0x1c0287f6c. Non-external; installed by __exceptionInit. Whether the exception is
// a kind of the catch type.
int __exceptionMatch(Class catchType, id exception) {
    return objectIsKindOfClass(exception, catchType);
}

// Native __handleUncaughtException, 0x1c0287f8c. Non-external; installed by __exceptionInit.
// An object that is not an NSException logs "*** Terminating app due to uncaught exception of class '%s'".
// For an NSException, under the backtrace lock: the return addresses from the reserved dictionary become
// "(%p %p ...)" in __last_exception_backtrace__ ("()" without them; the last space becomes ')', or ')' is
// appended), the class name and, for a constant-string name, its C string pointer in the system encoding are
// recorded, and the reserved "osLogPack" data is copied into the zeroed 1,024-byte pack. With call stack
// symbols that answer _descriptionWithBuffer:size:, the first calloc of 8,192, 4,096, 2,048, 1,024 or 512
// bytes (malloc type 0x100004077774924) receives "*** Terminating app due to uncaught exception '<name>',
// reason: '<reason>'" (UTF-8, each truncated to the space left), "\n*** First throw call stack:\n", the
// symbols and "\n"; it is logged, cut after the reason and left as the crash message (when every calloc
// fails,
// "*** Terminating app due to uncaught exception, out of memory" is). Otherwise the formatted message with
// the symbols' description (or the return addresses' when there are no symbols) is autoreleased, its UTF-8
// form becomes the crash message and is logged. Logging is _CFLogSimpleCString at level 3. Finally an
// installed
// __uncaughtExceptionHandler is called for an NSException.
void __handleUncaughtException(id exception) {
    id description = nil;
    OCFStringRef message;
    if (!objectIsKindOfClass(exception, (Class)exceptionClass)) {
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        message = OCFStringCreateWithFormat(
            allocator,
            NULL,
            OF_STR("*** Terminating app due to uncaught exception of class '%s'"),
            object_getClassName(exception)
        );
    } else {
        ExceptionFields *fields = (ExceptionFields *)exception;
        OCFStringRef name = (OCFStringRef)send(exception, "name");
        OCFStringRef reason = (OCFStringRef)send(exception, "reason");
        id addresses = objectForKey(fields->reserved, OF_STR("callStackReturnAddresses"));
        os_unfair_lock_lock(&__last_exception_backtrace_lock__);
        if (addresses != nil) {
            OCFIndex count = ((OCFIndex (*)(id, SEL))objc_msgSend)(addresses, sel_getUid("count"));
            __builtin___strlcpy_chk(__last_exception_backtrace__, "(", 0x2000, 0x2000);
            for (OCFIndex index = 0; index != count; index++) {
                id number =
                    ((id (*)(id, SEL, OCFIndex))objc_msgSend)(addresses, sel_getUid("objectAtIndex:"), index);
                uintptr_t address =
                    ((uintptr_t (*)(id, SEL))objc_msgSend)(number, sel_getUid("unsignedIntegerValue"));
                char entry[0x44];
                snprintf(entry, 0x44, "%p ", (void *)address);
                __builtin___strlcat_chk(__last_exception_backtrace__, entry, 0x2000, 0x2000);
            }
            char *space = strrchr(__last_exception_backtrace__, ' ');
            if (space != NULL) {
                space[0] = ')';
                space[1] = 0;
            } else {
                __builtin___strlcat_chk(__last_exception_backtrace__, ")", 0x2000, 0x2000);
            }
        } else {
            __builtin___strlcpy_chk(__last_exception_backtrace__, "()", 0x2000, 0x2000);
        }
        __last_exception_class_name__ = object_getClassName(exception);
        OCFStringRef nameField = fields->name;
        if (_ONSIsNSCFConstantString((id)nameField)) {
            __last_exception_name__ = OCFStringGetCStringPtr(nameField, OCFStringGetSystemEncoding());
        } else {
            __last_exception_name__ = NULL;
        }
        bzero(__last_exception_os_log_pack__, 0x400);
        id pack = objectForKey(fields->reserved, OF_STR("osLogPack"));
        if (pack != nil) {
            ((void (*)(id, SEL, void *, OCFIndex))objc_msgSend)(
                pack, sel_getUid("getBytes:length:"), __last_exception_os_log_pack__, 0x400
            );
        }
        os_unfair_lock_unlock(&__last_exception_backtrace_lock__);
        id symbols = objectForKey(fields->reserved, OF_STR("callStackSymbols"));
        description = addresses;
        if (symbols != nil) {
            if (objc_opt_respondsToSelector(symbols, sel_getUid("_descriptionWithBuffer:size:")) & 1) {
                char *buffer = NULL;
                size_t size = 0;
                for (int index = 0; index != 5; index++) {
                    size = kUncaughtBufferSizes[index];
                    buffer = malloc_type_calloc(size, 1, 0x100004077774924);
                    if (buffer != NULL) {
                        break;
                    }
                }
                if (buffer == NULL) {
                    const char *outOfMemory = "*** Terminating app due to uncaught exception, out of memory";
                    gCRAnnotations[1] = (uint64_t)(uintptr_t)outOfMemory;
                    _CFLogSimpleCString(3, outOfMemory, 0x3C);
                } else {
                    size_t used = strlcpy(buffer, "*** Terminating app due to uncaught exception '", size);
                    OCFIndex converted = 0;
                    OCFStringGetBytes(
                        name,
                        (OCFRange){0, OCFStringGetLength(name)},
                        kOCFStringEncodingUTF8,
                        0,
                        false,
                        (UInt8 *)buffer + used,
                        (OCFIndex)(size + ~used),
                        &converted
                    );
                    buffer[used + converted] = 0;
                    used += converted;
                    used += strlcpy(buffer + used, "', reason: '", size - used);
                    OCFStringGetBytes(
                        reason,
                        (OCFRange){0, OCFStringGetLength(reason)},
                        kOCFStringEncodingUTF8,
                        0,
                        false,
                        (UInt8 *)buffer + used,
                        (OCFIndex)(size + ~used),
                        &converted
                    );
                    buffer[used + converted] = 0;
                    used += converted;
                    used += strlcpy(buffer + used, "'", size - used);
                    size_t reasonEnd = used;
                    used += strlcpy(buffer + used, "\n*** First throw call stack:\n", size - used);
                    size_t written = ((size_t (*)(id, SEL, char *, size_t))objc_msgSend)(
                        symbols, sel_getUid("_descriptionWithBuffer:size:"), buffer + used, size - used
                    );
                    used += written;
                    strlcpy(buffer + used, "\n", size - used);
                    _CFLogSimpleCString(3, buffer, strlen(buffer));
                    buffer[reasonEnd] = 0;
                    gCRAnnotations[1] = (uint64_t)(uintptr_t)buffer;
                }
                goto callHandler;
            }
            description = symbols;
        }
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        message = OCFStringCreateWithFormat(
            allocator,
            NULL,
            OF_STR(
                "*** Terminating app due to uncaught exception '%@', reason: '%@'\n*** First throw call "
                "stack:\n%@\n"
            ),
            name,
            reason,
            send(description, "description")
        );
    }
    {
        id autoreleased = _OCFAutoreleasePoolAddObject(NULL, message);
        const char *text = ((const char *(*)(id, SEL))objc_msgSend)(autoreleased, sel_getUid("UTF8String"));
        gCRAnnotations[1] = (uint64_t)(uintptr_t)text;
        _CFLogSimpleCString(3, text, strlen(text));
    }
callHandler:
    if (__uncaughtExceptionHandler != NULL && objectIsKindOfClass(exception, (Class)exceptionClass)) {
        __uncaughtExceptionHandler(exception);
    }
}

// Native objectIsKindOfClass, 0x1c02884c4. Non-external. nil answers false. When the object's class answers
// isKindOfClass: (the selector at 0x2387aa172, named by use: class_respondsToSelector before
// objc_opt_isKindOfClass; confidence: medium), objc_opt_isKindOfClass decides; otherwise the class chain from
// object_getClass is walked with class_getSuperclass until the class or Nil.
bool objectIsKindOfClass(id object, Class cls) {
    if (object == nil) {
        return false;
    }
    if (class_respondsToSelector(object_getClass(object), sel_getUid("isKindOfClass:"))) {
        return objc_opt_isKindOfClass(object, cls) & 1;
    }
    Class candidate = object_getClass(object);
    while (candidate != cls && candidate != Nil) {
        candidate = class_getSuperclass(candidate);
    }
    return candidate != Nil;
}

// Native _CFThrowFormattedException, 0x1c0308c0c. Hidden (was a private external). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta2.json. The reason is created from the format and the
// variadic arguments (kCFAllocatorSystemDefault, no options) and autoreleased (CFAutorelease of NULL
// included); the format itself is the reason when the creation fails (CSEL). The exception is created with
// the name, the reason and nil user info and thrown.
void _CFThrowFormattedException(OCFStringRef name, OCFStringRef format, ...) {
    va_list arguments;
    va_start(arguments, format);
    OCFStringRef reason =
        OCFStringCreateWithFormatAndArguments(kOCFAllocatorSystemDefault, NULL, format, arguments);
    OCFAutorelease(reason);
    objc_exception_throw(exceptionNamed(name, reason == NULL ? (id)format : (id)reason));
}

// Native _CFRaiseMemoryException, 0x1c030f268. Hidden (was a private external). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta2.json. Throws an exception named by the constant
// string "NSMallocException" (a __cfstring literal, not the exported name constant) with the reason and nil
// user info.
void _CFRaiseMemoryException(OCFStringRef reason) {
    objc_exception_throw(exceptionNamed(OF_STR(OF_NAME_STRING(ONSMallocException)), (id)reason));
}

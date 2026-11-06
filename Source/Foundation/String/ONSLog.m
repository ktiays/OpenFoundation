//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSLog.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSPlaceholderString.h"
#import "Foundation/String/ONSStringOtherEncodings.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"

#include <dispatch/dispatch.h>
#include <objc/runtime.h>

// Inferred organization: NSLog (0x1c0593870) and __NSOSLog (0x1c0592ae4) lie in the Foundation image's NSLog
// functions (0x1c0592ae4-0x1c05938f4, with NSLogv, _NSLogCStringFunction and _NSSetLogCStringFunction, which
// the scope does not reach); __NSLogv (0x1c0fde558) and ___NSLogAdaptor (0x1c0fde5d0) lie in the outlined
// region. The once block was captured separately on 2026/11/03. Objective-C compiled with
// manual reference counting. Unless a definition says otherwise: iOS Simulator 24A434, arm64, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full,
// iphone-simulator-foundation-link.json). iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Foundation/String/ONSStringReview.md.

// Native ___NSLogCString, 0x22e776268 (__DATA,__common). Non-external (was a private external).
void (*__NSLogCString)(const char *message, size_t length, BOOL withSyslogBanner) = NULL;

extern void *objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void *pool);
extern Class objc_opt_class(id object);

// Native Foundation imports the NSException exception type record. This declaration only selects
// that runtime catch type, so Clang references the existing record instead of emitting another one.
__attribute__((objc_exception, objc_runtime_name(OF_NAME_STRING(ONSException))))
@interface LogCallbackException : NSObject
@end

@protocol LogCallbackMessages
- (id)redactedDescription;
- (id)name;
- (id)reason;
- (id)callStackSymbols;
@end

// Native __NS_os_log_callback, 0x1c0592324. External.
// The LSDA has a typed NSException catch followed by a catch-all, and a nested catch-all while
// formatting the exception. The cleanup landing pads end the active catch before resuming unwind.
// The pool is popped after the catches on the normal return path. Hex-Rays omits the exception paths.
// Reading: Local/Scratch/cf-core-r2-fx/ida/ns-os-log-callback-eh.txt.
BOOL _ONS_os_log_callback(id object, char *buffer, size_t capacity, uint8_t *privacy) {
    if (object == nil) {
        return NO;
    }

    void *pool = objc_autoreleasePoolPush();
    BOOL written = NO;
    @try {
        if ([object isProxy] || object_isClass(object)) {
            object = [object description];
        }
        if (_ONSIsNSObject(object)) {
            id description;
            if (privacy != NULL && *privacy != 1) {
                description = [object description];
            } else {
                if (_ONSIsNSNumber(object) && privacy != NULL) {
                    *privacy = 0;
                }
                if (_ONSIsNSCFConstantString(object) && privacy != NULL) {
                    *privacy = 0;
                }
                if (class_respondsToSelector(objc_opt_class(object), sel_getUid("redactedDescription"))) {
                    description = [(id<LogCallbackMessages>)object redactedDescription];
                    if (privacy != NULL) {
                        *privacy = 0;
                    }
                } else {
                    description = (id)OF_STR("");
                }
            }

            NSUInteger length = [description length];
            if (length == 0) {
                if (buffer != NULL && capacity != 0) {
                    buffer[0] = 0;
                }
                written = YES;
            } else if (capacity >= 2) {
                NSUInteger used = 0;
                ONSRange range = {0, length};
                if ([description getBytes:buffer
                                maxLength:capacity - 1
                               usedLength:&used
                                 encoding:4
                                  options:2
                                    range:range
                           remainingRange:NULL] ||
                    [description getBytes:buffer
                                maxLength:capacity - 1
                               usedLength:&used
                                 encoding:7
                                  options:2
                                    range:range
                           remainingRange:NULL]) {
                    buffer[used] = 0;
                    written = YES;
                }
            }
        }
    } @catch (LogCallbackException *exception) {
        @try {
            Class cls = objc_opt_class(object);
            id name = [(id<LogCallbackMessages>)exception name];
            id reason = [(id<LogCallbackMessages>)exception reason];
            id stack = [(id<LogCallbackMessages>)exception callStackSymbols];
            id description = [ONSString
                stringWithFormat:(id)OF_STR(
                                     "-[%@:%p description] raised exception: name:%@ reason:%@ backtrace:%@"
                                 ),
                                 cls,
                                 (void *)object,
                                 name,
                                 reason,
                                 stack];
            if (description != nil) {
                ONSRange range = {0, [description length]};
                NSUInteger used = 0;
                if ([description getBytes:buffer
                                maxLength:capacity - 1
                               usedLength:&used
                                 encoding:4
                                  options:2
                                    range:range
                           remainingRange:NULL] ||
                    [description getBytes:buffer
                                maxLength:capacity - 1
                               usedLength:&used
                                 encoding:7
                                  options:2
                                    range:range
                           remainingRange:NULL]) {
                    buffer[used] = 0;
                    written = YES;
                }
            }
        } @catch (...) {
            written = NO;
        }
    } @catch (...) {
        written = NO;
    }
    objc_autoreleasePoolPop(pool);
    return written;
}

// Native __NSOSLog, 0x1c0592ae4. Hidden (was a private external). The dispatch token at 0x22e76c918 and
// the handle at 0x22e76c910 are zero-initialized __bss storage (not the unbacked IDA segment's 0xFF bytes).
// Supplemental IDA reading: Local/Scratch/cf-core-fd-string-review/supplemental-ida.json.
os_log_t _NSOSLog(void) {
    static dispatch_once_t once;
    static os_log_t log;
    // Native ____NSOSLog_block_invoke, 0x1c05938c8. Non-external. The created handle is kept for process
    // life.
    dispatch_once(&once, ^{
      log = os_log_create("com.apple.Foundation", "general");
    });
    return log;
}

// Native NSLog, 0x1c0593870. External. Passes the variadic arguments and its own return address (X30) to
// __NSLogv.
void ONSLog(id format, ...) {
    va_list arguments;
    va_start(arguments, format);
    _NSLogv(format, arguments, __builtin_return_address(0));
    va_end(arguments);
}

// Native __NSLogv, 0x1c0fde558. Non-external (was a private external). Inside an autorelease pool: the log
// function is ___NSLogAdaptor when __NSLogCString is set and NULL otherwise; the description callbacks are
// __NSDescriptionWithLocaleFunc and __NSDescriptionWithStringProxyFunc; no format options; level 4. The pool
// pop is a tail call.
void _NSLogv(id format, va_list arguments, void *returnAddress) {
    void *pool = _OCFAutoreleasePoolPush();
    _OCFLogvEx3(
        __NSLogCString != NULL ? __NSLogAdaptor : NULL,
        (OCFStringRef (*)(void *, const void *))_ONSDescriptionWithLocaleFunc,
        (OCFStringRef (*)(void *, const void *, const void *, bool, bool *))
            _ONSDescriptionWithStringProxyFunc,
        NULL,
        4,
        (OCFStringRef)format,
        arguments,
        returnAddress
    );
    _OCFAutoreleasePoolPop(pool);
}

// Native ___NSLogAdaptor, 0x1c0fde5d0. Non-external (was a private external). The level is dropped; the
// banner flag becomes 0 or 1 (a 32-bit test); a tail call of __NSLogCString.
void __NSLogAdaptor(int32_t level, const char *message, size_t length, char withBanner) {
    (void)level;
    __NSLogCString(message, length, withBanner != 0);
}

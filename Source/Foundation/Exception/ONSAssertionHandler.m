//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Exception/ONSAssertionHandler.h"

#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "Foundation/Archiving/ONSArchivingPrivate.h"
#import "Foundation/Number/ONSNumber.h"
#import "Foundation/String/ONSLog.h"
#import "Foundation/Thread/ONSThread.h"

#include <objc/objc-exception.h>
#include <stdarg.h>

// Foundation, iOS Simulator 27.0 (24A434), arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured IDA assembly. iOS Device divergence: iOS Device 24A446 is unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json.

extern char assertionHandlerClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSAssertionHandler));
extern char assertionThreadClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSThread));

@protocol AssertionDictionaryMessages
+ (id)dictionary;
- (id)objectForKey:(id)key;
- (void)setObject:(id)object forKey:(id)key;
@end

@implementation ONSAssertionHandler

// Native +[NSAssertionHandler currentHandler], 0x1c0507174. Non-external.
+ (id)currentHandler {
    id<AssertionDictionaryMessages> dictionary = [[(id)assertionThreadClass currentThread] threadDictionary];
    // This string is a thread dictionary key.
    id key = (id)OF_STR(
        "NS"
        "AssertionHandler"
    );
    id handler = [dictionary objectForKey:key];
    if (dictionary != nil && handler == nil) {
        handler = [objc_allocWithZone((Class)assertionHandlerClass) init];
        [dictionary setObject:handler forKey:key];
        objc_release(handler);
    }
    return handler;
}

// Native __userInfoForFileAndLine, 0x1c0507308. Non-external.
static id _userInfoForFileAndLine(id file, NSInteger line) {
    if (file == nil && line == 0) {
        return nil;
    }
    id<AssertionDictionaryMessages> info =
        [(Class<AssertionDictionaryMessages>)archivingMutableDictionaryClass dictionary];
    if (file != nil) {
        [info setObject:file forKey:(id)OF_STR("NSAssertFile")];
    }
    if (line != 0) {
        [info setObject:[ONSNumber numberWithInteger:line] forKey:(id)OF_STR("NSAssertLine")];
    }
    return info;
}

// Native -[NSAssertionHandler handleFailureInFunction:file:lineNumber:description:], 0x1c0507394.
// Non-external.
- (void)handleFailureInFunction:(id)function
                           file:(id)file
                     lineNumber:(NSInteger)line
                    description:(id)format, ... {
    va_list arguments;
    va_start(arguments, format);
    os_log_t log = _NSOSLog();
    if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
        uint8_t buffer[32];
#define ASSERTION_LOG_FORMAT "*** Assertion failure in %@(), %@:%ld"
        __builtin_os_log_format(buffer, ASSERTION_LOG_FORMAT, function, file, line);
        _os_log_error_impl(
            &__dso_handle, log, OS_LOG_TYPE_ERROR, ASSERTION_LOG_FORMAT, buffer, sizeof(buffer)
        );
#undef ASSERTION_LOG_FORMAT
    }
    va_list copied;
    va_copy(copied, arguments);
    id string = objc_alloc((Class)archivingStringClass);
    id reason = objc_autorelease([string initWithFormat:(format != nil ? format : (id)OF_STR(" "))
                                                 locale:nil
                                              arguments:copied]);
    va_end(copied);
    va_end(arguments);
    id info = _userInfoForFileAndLine(file, line);
    objc_exception_throw([ONSException exceptionWithName:(id)ONSInternalInconsistencyException
                                                  reason:reason
                                                userInfo:info]);
}

@end

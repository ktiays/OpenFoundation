//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSException.h"

#import "CoreFoundation/Collections/OCFData.h"
#import "CoreFoundation/String/OCFString.h"
#import "CoreFoundation/String/OCFStringFormat.h"

#include <mach-o/loader.h>
#include <objc/objc-exception.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

// Inferred organization: the NSException methods (0x1c0287870-0x1c0287e40) lie together in the iOS Simulator
// __text section, before the exception handlers that Objective-C/OCFException.m restores (__exceptionInit at
// 0x1c0287e48, another family's file); following the owner's decision, the class has its own translation
// unit. Objective-C compiled with manual reference counting (objc_retain, objc_release, objc_autorelease).
// The native source path is not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full; the os_log pack
// header read with Local/Scratch/cf-core/macho.py); iOS Device 24A446: unread. Reading: not yet written; the
// evidence is the assembly in Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json.
//
// The ivars are read at their fixed offsets (8, 16, 24, 32) natively, not through the ivar offset symbols.

// libsystem_trace's os_log pack interface (private SPI, no SDK declaration); supplied.
extern size_t _os_log_pack_size(size_t os_log_format_buffer_size);
extern uint8_t *_os_log_pack_fill(void *pack, size_t size, int saved_errno, const void *dso, const char *fmt);

// The image's own Mach-O header, the dso argument of _os_log_pack_fill (dword_1C014A000).
extern struct mach_header __dso_handle;

// The NSString class (Foundation's; its class reference, the GOT slot 0x22229c638, lies outside the image and
// is bound by use, the receiver of +stringWithFormat:; confidence: medium).
extern char stringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSString));

// Selectors this unit sends to objects of classes outside it (NSDictionary, NSMutableDictionary, NSArray and
// NSString), declared for the message sends only; no protocol object is emitted.
@protocol ExceptionMessages

- (id)objectForKey:(id)key;
- (void)setObject:(id)object forKey:(id)key;
+ (id)stringWithFormat:(id)format, ...;

@end

@implementation ONSException

// Native -[NSException name], 0x1c0287870. Non-external; retained and autoreleased (a tail call).
- (id)name {
    return [[name retain] autorelease];
}

// Native -[NSException reason], 0x1c0287888. Non-external; retained and autoreleased (a tail call).
- (id)reason {
    return [[reason retain] autorelease];
}

// Native -[NSException userInfo], 0x1c02878a0. Non-external; retained and autoreleased (a tail call).
- (id)userInfo {
    return [[userInfo retain] autorelease];
}

// Native -[NSException callStackReturnAddresses], 0x1c02878b8. Non-external; the reserved dictionary's
// "callStackReturnAddresses" (a tail call).
- (id)callStackReturnAddresses {
    return [reserved objectForKey:(id)OF_STR("callStackReturnAddresses")];
}

// Native -[NSException callStackSymbols], 0x1c02878c8. Non-external; the reserved dictionary's
// "callStackSymbols" (a tail call).
- (id)callStackSymbols {
    return [reserved objectForKey:(id)OF_STR("callStackSymbols")];
}

// Native -[NSException _isUnarchived], 0x1c02878d8. Non-external; whether the reserved dictionary holds
// "_NSExceptionWasUnarchived".
- (BOOL)_isUnarchived {
    return [reserved objectForKey:(id)OF_STR("_NSExceptionWasUnarchived")] != nil;
}

// Native -[NSException _markAsUnarchived], 0x1c0287900. Non-external. Only when there is no reserved
// dictionary: a new NSMutableDictionary (__CFLookUpClass, objc_opt_new) is stored and receives "SENTINEL" for
// "_NSExceptionWasUnarchived" (a tail call); an existing dictionary is left unmarked.
- (void)_markAsUnarchived {
    if (reserved != nil) {
        return;
    }
    reserved = [(id)__CFLookUpClass(OF_NAME_STRING(ONSMutableDictionary)) new];
    [reserved setObject:(id)OF_STR("SENTINEL") forKey:(id)OF_STR("_NSExceptionWasUnarchived")];
}

// Native -[NSException _installStackTraceKeyIfNeeded], 0x1c028794c. Non-external.
// A user info that already holds "NSStackTraceKey" answers NO. Otherwise the user info is replaced by a
// mutable copy of itself (autoreleased first; the copy is not released) or, without one, by a new
// NSMutableDictionary; then the description of the reserved dictionary's "callStackReturnAddresses", when not
// nil, is stored for "NSStackTraceKey" and YES answered; a nil description answers NO.
- (BOOL)_installStackTraceKeyIfNeeded {
    if (userInfo != nil) {
        if ([userInfo objectForKey:(id)OF_STR("NSStackTraceKey")] != nil) {
            return NO;
        }
    }
    if (userInfo != nil) {
        userInfo = [[userInfo autorelease] mutableCopy];
    } else {
        userInfo = [(id)__CFLookUpClass(OF_NAME_STRING(ONSMutableDictionary)) new];
    }
    id description = [[reserved objectForKey:(id)OF_STR("callStackReturnAddresses")] description];
    if (description == nil) {
        return NO;
    }
    [userInfo setObject:description forKey:(id)OF_STR("NSStackTraceKey")];
    return YES;
}

// Native -[NSException isEqual:], 0x1c02879e0. Non-external. nil is not equal; objects of the same class
// (objc_opt_class, the argument's first) compare their names with -isEqual: (a tail call).
- (BOOL)isEqual:(id)other {
    if (other == nil) {
        return NO;
    }
    Class otherClass = [other class];
    if (otherClass != [self class]) {
        return NO;
    }
    return [name isEqual:((ONSException *)other)->name];
}

// Native -[NSException hash], 0x1c0287a44. Non-external; the name's hash (a tail call).
- (NSUInteger)hash {
    return [name hash];
}

// Native -[NSException description], 0x1c0287a4c. Non-external; the reason, or the name without one.
- (id)description {
    if (reason != nil) {
        return reason;
    }
    return name;
}

// Native -[NSException redactedDescription], 0x1c0287a64. Non-external. Without a reason, the name; otherwise
// +[NSString stringWithFormat:] with "%@ <redacted reason>" and the name.
- (id)redactedDescription {
    if (reason == nil) {
        return name;
    }
    return [(id)stringClass stringWithFormat:(id)OF_STR("%@ <redacted reason>"), name];
}

// Native -[NSException raise], 0x1c0287aac. Non-external; objc_exception_throw of the receiver.
- (void)raise {
    objc_exception_throw(self);
}

// Native -[NSException init], 0x1c0287ab8. Non-external; the receiver is released and nil answered.
- (id)init {
    [self release];
    return nil;
}

// Native -[NSException initWithName:reason:userInfo:osLogPack:size:], 0x1c0287ad0. Non-external.
// No super initializer is called. The name and reason are copied (-copy) and the user info retained. A stack
// buffer of _os_log_pack_size(12) bytes, rounded up to 16, is always allocated. Without a pack, a reason that
// is a constant string (_NSIsNSCFConstantString) fills the buffer with _os_log_pack_fill (no errno, this
// image's header, "%s"), writes the 32-bit argument header 0x08200102 (qword_1C0385B30's low word) and the
// reason's C string pointer (CFStringGetCStringPtr in the system encoding) after it, and uses the buffer as
// the pack; any other reason returns at once. With a pack, a new NSMutableDictionary becomes the reserved
// dictionary and receives a CFData (kCFAllocatorSystemDefault) of the pack for "osLogPack", released after.
// The size argument is used unchanged for the stack buffer's data too.
- (id)initWithName:(id)aName
            reason:(id)aReason
          userInfo:(id)aUserInfo
         osLogPack:(void *)osLogPack
              size:(size_t)size {
    name = [aName copy];
    reason = [aReason copy];
    userInfo = [aUserInfo retain];
    size_t packSize = _os_log_pack_size(12);
    uint8_t *buffer = __builtin_alloca((packSize + 15) & ~(size_t)15);
    if (osLogPack == NULL) {
        if (!_ONSIsNSCFConstantString(reason)) {
            return self;
        }
        const char *cString = OCFStringGetCStringPtr((OCFStringRef)reason, OCFStringGetSystemEncoding());
        uint8_t *arguments = _os_log_pack_fill(buffer, packSize, 0, &__dso_handle, "%s");
        uint32_t header = 0x08200102;
        memcpy(arguments, &header, sizeof(header));
        memcpy(arguments + 4, &cString, sizeof(cString));
        osLogPack = buffer;
    }
    reserved = [(id)__CFLookUpClass(OF_NAME_STRING(ONSMutableDictionary)) new];
    OCFDataRef data = OCFDataCreate(kOCFAllocatorSystemDefault, osLogPack, (OCFIndex)size);
    [reserved setObject:(id)data forKey:(id)OF_STR("osLogPack")];
    [(id)data release];
    return self;
}

// Native -[NSException initWithName:reason:userInfo:], 0x1c0287c28. Non-external; no pack and size 0 (a tail
// call).
- (id)initWithName:(id)aName reason:(id)aReason userInfo:(id)aUserInfo {
    return [self initWithName:aName reason:aReason userInfo:aUserInfo osLogPack:NULL size:0];
}

// Native +[NSException exceptionWithName:reason:userInfo:], 0x1c0287c34. Non-external.
+ (id)exceptionWithName:(id)aName reason:(id)aReason userInfo:(id)aUserInfo {
    return [[[self alloc] initWithName:aName reason:aReason userInfo:aUserInfo] autorelease];
}

// Native +[NSException exceptionWithName:reason:userInfo:osLogPack:size:], 0x1c0287c74. Non-external.
+ (id)exceptionWithName:(id)aName
                 reason:(id)aReason
               userInfo:(id)aUserInfo
              osLogPack:(void *)osLogPack
                   size:(size_t)size {
    return [[[self alloc] initWithName:aName reason:aReason userInfo:aUserInfo osLogPack:osLogPack
                                  size:size] autorelease];
}

// Native -[NSException dealloc], 0x1c0287ccc. Non-external; the four ivars are released in order, then
// super's -dealloc.
- (void)dealloc {
    [name release];
    [reason release];
    [userInfo release];
    [reserved release];
    [super dealloc];
}

// Native -[NSException copyWithZone:], 0x1c0287d58. Non-external; the receiver retained (objc_retain).
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self retain];
}

// Native +[NSException raise:format:arguments:], 0x1c0287d5c. Non-external. The reason is the formatted
// string (CFStringCreateWithFormatAndArguments, kCFAllocatorSystemDefault, no options, autoreleased) or, for
// a nil format, the empty string; +exceptionWithName:reason:userInfo: (nil user info) is thrown.
+ (void)raise:(id)aName format:(id)format arguments:(va_list)argList {
    id aReason;
    if (format == nil) {
        aReason = (id)OF_STR("");
    } else {
        aReason = [(id)OCFStringCreateWithFormatAndArguments(
            kOCFAllocatorSystemDefault, NULL, (OCFStringRef)format, argList
        ) autorelease];
    }
    objc_exception_throw([self exceptionWithName:aName reason:aReason userInfo:nil]);
}

// Native +[NSException raise:format:], 0x1c0287db8. Non-external; as +raise:format:arguments: with the
// variadic arguments (no va_end before the throw).
+ (void)raise:(id)aName format:(id)format, ... {
    id aReason;
    if (format == nil) {
        aReason = (id)OF_STR("");
    } else {
        va_list arguments;
        va_start(arguments, format);
        aReason = [(id)OCFStringCreateWithFormatAndArguments(
            kOCFAllocatorSystemDefault, NULL, (OCFStringRef)format, arguments
        ) autorelease];
    }
    objc_exception_throw([self exceptionWithName:aName reason:aReason userInfo:nil]);
}

// Native -[NSException initWithCoder:], 0x1c0287e34. Non-external; nil (the receiver is not released).
- (id)initWithCoder:(id)coder {
    return nil;
}

// Native -[NSException encodeWithCoder:], 0x1c0287e3c. Non-external; does nothing.
- (void)encodeWithCoder:(id)coder {
}

// Native +[NSException supportsSecureCoding], 0x1c0287e40. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

@end

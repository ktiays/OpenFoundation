//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSArray0.h"
#include "CoreFoundation/Objective-C/OCFNSEnumerator0.h"
#include "CoreFoundation/Objective-C/OCFNSPlaceholderArray.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <alloca.h>
#include <mach-o/loader.h>
#include <objc/objc-exception.h>
#include <stdint.h>

// Inferred organization: the __NSArray0 methods +automaticallyNotifiesObserversForKey: (0x1c014b170) through
// -objectEnumerator (0x1c014b290) lie among the NSArray implementation methods of the iOS Simulator __text
// section; its trapping -init and -dealloc (0x1c0305ee4, 0x1c0305ee8) lie in the cold region. Following the
// owner's decision, the class has its own translation unit; the methods follow in native address order. The
// class's only instance is the static empty array __NSArray0__struct (Objective-C/OCFNSEmptyCollections.m).
// Objective-C, manual reference counting (the retain and release methods are the singleton's no-ops; no
// method of the unit calls objc_retain or objc_release). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json (+automaticallyNotifiesObserversForKey:) and
// iphone-simulator-corpus-delta11.json (family L-collection-leftovers: the other methods). iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device
// 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written
// yet).

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// The NSException class symbol (off_2227CE300), sent to directly.
extern char array0ExceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));

// The message this unit sends that the shared declarations do not carry; declared for the send only.
@protocol Array0Messages

+ (id)exceptionWithName:(id)name
                 reason:(id)reason
               userInfo:(id)userInfo
              osLogPack:(void *)pack
                   size:(size_t)size;

@end

@implementation __ONSArray0

// Native +[__NSArray0 automaticallyNotifiesObserversForKey:], 0x1c014b170. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native +[__NSArray0 allocWithZone:], 0x1c014b178. Non-external. The immutable placeholder (a tail call);
// the zone is not read.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSArrayImmutablePlaceholder();
}

// Native -[__NSArray0 copy], 0x1c014b17c. Non-external. The receiver (RET), not retained.
- (id)copy {
    return self;
}

// Native -[__NSArray0 copyWithZone:], 0x1c014b180. Non-external. The receiver (RET), not retained.
- (id)copyWithZone:(struct _NSZone *)zone {
    return self;
}

// Native -[__NSArray0 retainCount], 0x1c014b184. Non-external. Answers NSUIntegerMax.
- (NSUInteger)retainCount {
    return NSUIntegerMax;
}

// Native -[__NSArray0 retain], 0x1c014b18c. Non-external. The receiver (RET).
- (instancetype)retain {
    return self;
}

// Native -[__NSArray0 release], 0x1c014b190. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[__NSArray0 autorelease], 0x1c014b194. Non-external. The receiver (RET).
- (instancetype)autorelease {
    return self;
}

// Native -[__NSArray0 count], 0x1c014b198. Non-external. Answers 0.
- (NSUInteger)count {
    return 0;
}

// Native -[__NSArray0 objectAtIndex:], 0x1c014b1a0. Non-external. Every index raises NSRangeException with
// "*** %s: index %lu beyond bounds for empty array" (the method's name as a literal C string, spelled with
// the class's selected name, and the index): an os_log pack of _os_log_pack_size(0x16) on the stack, filled
// through _os_log_pack_fill (errno 0, the image's mach header, the format) and the two arguments encoded as
// __builtin_os_log_format does; the reason made with kCFAllocatorSystemDefault and the same format as a
// constant string, autoreleased through _CFAutoreleasePoolAddObject; +[NSException
// exceptionWithName:reason:userInfo:osLogPack:size:] with no user info thrown.
- (id)objectAtIndex:(NSUInteger)index {
    size_t packSize = _os_log_pack_size(0x16);
    void *pack = alloca((packSize + 15) & ~(size_t)15);
    uint8_t *packBuffer = _os_log_pack_fill(
        pack, packSize, 0, &__dso_handle, "*** %s: index %lu beyond bounds for empty array"
    );
    __builtin_os_log_format(
        packBuffer,
        "*** %s: index %lu beyond bounds for empty array",
        "-[" OF_NAME_STRING(__ONSArray0) " objectAtIndex:]",
        (unsigned long)index
    );
    OCFStringRef reason = OCFStringCreateWithFormat(
        kOCFAllocatorSystemDefault,
        NULL,
        OF_STR("*** %s: index %lu beyond bounds for empty array"),
        "-[" OF_NAME_STRING(__ONSArray0) " objectAtIndex:]",
        (unsigned long)index
    );
    objc_exception_throw([(id<Array0Messages>)(id)array0ExceptionClass
        exceptionWithName:(id)ONSRangeException
                   reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                 userInfo:nil
                osLogPack:pack
                     size:packSize]);
}

// Native -[__NSArray0 objectEnumerator], 0x1c014b290. Non-external. The __NSEnumerator0 class is sent
// objc_opt_class (its result unused), then the empty enumerator singleton __NSEnumerator0__ is answered, not
// retained.
- (id)objectEnumerator {
    (void)[__ONSEnumerator0 class];
    return __NSEnumerator0__;
}

// Native -[__NSArray0 init], 0x1c0305ee4 (cold region). Non-external. A bare BRK #1.
- (id)init {
    __builtin_trap();
}

// Native -[__NSArray0 dealloc], 0x1c0305ee8 (cold region). Non-external. A bare BRK #1; super is never
// reached, so the missing-super-call diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"

- (void)dealloc {
    __builtin_trap();
}

#pragma clang diagnostic pop

@end

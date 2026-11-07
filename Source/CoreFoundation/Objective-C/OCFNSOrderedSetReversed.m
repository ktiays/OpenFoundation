//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSOrderedSetReversed.h"
#include "CoreFoundation/Objective-C/OCFNSArray.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <alloca.h>
#include <mach-o/loader.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <stdint.h>

// Inferred organization: the __NSOrderedSetReversed methods lie together in the iOS Simulator __text section
// (0x1c01d34dc-0x1c01d37df), after the __NSOrderedSetReverseEnumerator methods; following the owner's
// decision, the class has its own translation unit. The methods follow in native address order. Objective-C
// compiled with manual reference counting (objc_release). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta11.json (family K-mutable-ordered-set). Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).
//
// Argument errors raise as the other bridged collection classes do (an os_log pack built on the stack, the
// reason made with kCFAllocatorSystemDefault and autoreleased, +[NSException
// exceptionWithName:reason:userInfo:osLogPack:size:] thrown); the first format argument is the method's name,
// spelled with the class's selected name.

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// The NSException class symbol (off_2227CE300), sent to directly.
extern char exceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));

// The message this unit sends that the shared declarations do not carry; declared for the send only.
@protocol OrderedSetReversedMessages

+ (id)exceptionWithName:(id)name
                 reason:(id)reason
               userInfo:(id)userInfo
              osLogPack:(void *)pack
                   size:(size_t)size;

@end

#define methodName(selectorText) "-[" OF_NAME_STRING(__ONSOrderedSetReversed) " " selectorText "]"

#define RAISE_LOGGED_EXCEPTION(name, bufferSize, format, ...)                                                \
    do {                                                                                                     \
        size_t packSize = _os_log_pack_size(bufferSize);                                                     \
        void *pack = alloca((packSize + 15) & ~(size_t)15);                                                  \
        uint8_t *packBuffer = _os_log_pack_fill(pack, packSize, 0, &__dso_handle, format);                   \
        __builtin_os_log_format(packBuffer, format, __VA_ARGS__);                                            \
        OCFStringRef reason =                                                                                \
            OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR(format), __VA_ARGS__);        \
        objc_exception_throw([(id)exceptionClass                                                             \
            exceptionWithName:(id)(name)                                                                     \
                       reason:_OCFAutoreleasePoolAddObject(NULL, reason)                                     \
                     userInfo:nil                                                                            \
                    osLogPack:pack                                                                           \
                         size:packSize]);                                                                    \
    } while (0)

@implementation __ONSOrderedSetReversed

// Native -[__NSOrderedSetReversed initWithOrderedSet:], 0x1c01d34dc. Non-external. _orderedSet takes -copy of
// the ordered set and _cnt that copy's -count; super's initializer is not sent. The native init reaches no
// designated initializer of super, so that diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-designated-initializers"

- (id)initWithOrderedSet:(id)orderedSet {
    _orderedSet = [orderedSet copy];
    _cnt = [_orderedSet count];
    return self;
}

#pragma clang diagnostic pop

// Native -[__NSOrderedSetReversed dealloc], 0x1c01d3520. Non-external. _orderedSet released, then super.
- (void)dealloc {
    [_orderedSet release];
    [super dealloc];
}

// Native -[__NSOrderedSetReversed count], 0x1c01d359c. Non-external. The _cnt ivar.
- (NSUInteger)count {
    return _cnt;
}

// Native -[__NSOrderedSetReversed indexOfObject:], 0x1c01d35ac. Non-external. The copy's -indexOfObject:;
// NSNotFound is answered as is, any other index mirrored (_cnt + ~index).
- (NSUInteger)indexOfObject:(id)object {
    NSUInteger index = [_orderedSet indexOfObject:object];
    if (index == kNotFound) {
        return kNotFound;
    }
    return _cnt + ~index;
}

// Native -[__NSOrderedSetReversed objectAtIndex:], 0x1c01d35fc. Non-external. An index with bit 63 clear
// below _cnt answers -objectAtIndex: of the copy at _cnt + ~index, the mirrored index (a tail call);
// otherwise NSRangeException "*** %s: index %lu beyond bounds for empty ordered set" (buffer 0x16) for a
// count of 0, else "*** %s: index %lu beyond bounds [0 .. %lu]" (buffer 0x20) with the count minus one.
- (id)objectAtIndex:(NSUInteger)index {
    NSUInteger count = _cnt;
    if ((NSInteger)index < 0 || count <= index) {
        if (count == 0) {
            RAISE_LOGGED_EXCEPTION(
                ONSRangeException,
                0x16,
                "*** %s: index %lu beyond bounds for empty ordered set",
                methodName("objectAtIndex:"),
                (unsigned long)index
            );
        }
        RAISE_LOGGED_EXCEPTION(
            ONSRangeException,
            0x20,
            "*** %s: index %lu beyond bounds [0 .. %lu]",
            methodName("objectAtIndex:"),
            (unsigned long)index,
            (unsigned long)count - 1
        );
    }
    return [_orderedSet objectAtIndex:count + ~index];
}

@end

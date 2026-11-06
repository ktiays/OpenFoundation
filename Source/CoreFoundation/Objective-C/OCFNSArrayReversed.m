//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSArrayReversed.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <alloca.h>
#include <mach-o/loader.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <stdint.h>

// Inferred organization: the __NSArrayReversed methods lie together in the iOS Simulator __text section
// (0x1c028e898-0x1c028eeb7), followed by the __NSArrayReverseEnumerator methods; following the owner's
// decision, each class has its own translation unit. The methods follow in native address order. Objective-C
// compiled with manual reference counting (objc_release). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json (family H-array-concrete). Unless a definition
// says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified
// (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).
//
// Argument errors raise as the other bridged array classes do (an os_log pack built on the stack, the reason
// made with kCFAllocatorSystemDefault and autoreleased, +[NSException
// exceptionWithName:reason:userInfo:osLogPack:size:] thrown); the first format argument is the method's name,
// spelled with the class's selected name.

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// The NSException class symbol (off_2227CE300), sent to directly.
extern char exceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));

// The message this unit sends that the shared declarations do not carry; declared for the send only.
@protocol ArrayReversedMessages

+ (id)exceptionWithName:(id)name
                 reason:(id)reason
               userInfo:(id)userInfo
              osLogPack:(void *)pack
                   size:(size_t)size;

@end

#define methodName(selectorText) "-[" OF_NAME_STRING(__ONSArrayReversed) " " selectorText "]"

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

@implementation __ONSArrayReversed

// Native -[__NSArrayReversed count], 0x1c028e898. Non-external. The _cnt ivar.
- (NSUInteger)count {
    return _cnt;
}

// Native -[__NSArrayReversed objectAtIndex:], 0x1c028e8a8. Non-external. An index with bit 63 clear below
// _cnt answers -objectAtIndex: of _array at _cnt + ~index, the mirrored index (a tail call); otherwise
// NSRangeException "*** %s: index %lu beyond bounds for empty array" (buffer 0x16) for a count of 0, else
// "*** %s: index %lu beyond bounds [0 .. %lu]" (buffer 0x20) with the count minus one.
- (id)objectAtIndex:(NSUInteger)index {
    NSUInteger count = _cnt;
    if ((NSInteger)index < 0 || count <= index) {
        if (count == 0) {
            RAISE_LOGGED_EXCEPTION(
                ONSRangeException,
                0x16,
                "*** %s: index %lu beyond bounds for empty array",
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
    return [_array objectAtIndex:count + ~index];
}

// Native -[__NSArrayReversed getObjects:range:], 0x1c028ea8c. Non-external. Order: a NULL buffer with a
// nonzero length, a length of 2^61 or more (buffer 0x16 each), then -count and the range check against it
// (location with bit 63 set or past the count with the length; "for empty array", buffer 0x20, or "[0 ..
// %lu]", buffer 0x2A). Each object is -objectAtIndex: of _array at _cnt + ~(location + i), _array and _cnt
// reread per object.
- (void)getObjects:(id *)objects range:(OCFRange)range {
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if (objects == NULL && length != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            methodName("getObjects:range:"),
            (unsigned long)length
        );
    }
    if ((length >> 61) != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            methodName("getObjects:range:"),
            (unsigned long)length
        );
    }
    NSUInteger count = [self count];
    if ((NSInteger)location < 0 || count < location + length) {
        if (count == 0) {
            RAISE_LOGGED_EXCEPTION(
                ONSRangeException,
                0x20,
                "*** %s: range {%lu, %lu} extends beyond bounds for empty array",
                methodName("getObjects:range:"),
                (unsigned long)location,
                (unsigned long)length
            );
        }
        RAISE_LOGGED_EXCEPTION(
            ONSRangeException,
            0x2A,
            "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",
            methodName("getObjects:range:"),
            (unsigned long)location,
            (unsigned long)length,
            (unsigned long)count - 1
        );
    }
    NSUInteger mirrored = ~location;
    for (NSUInteger remaining = length; remaining != 0; remaining--) {
        *objects++ = [_array objectAtIndex:mirrored + _cnt];
        mirrored--;
    }
}

// Native -[__NSArrayReversed initWithArray:], 0x1c028edfc. Non-external. _array takes -copy of the array and
// _cnt that copy's -count; super's initializer is not sent.
- (id)initWithArray:(id)array {
    _array = [array copy];
    _cnt = [_array count];
    return self;
}

// Native -[__NSArrayReversed dealloc], 0x1c028ee40. Non-external. _array released, then super.
- (void)dealloc {
    [_array release];
    [super dealloc];
}

@end

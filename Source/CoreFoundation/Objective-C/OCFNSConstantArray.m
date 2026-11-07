//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSConstantArray.h"
#include "CoreFoundation/Objective-C/OCFNSArrayM.h"
#include "CoreFoundation/Objective-C/OCFNSPlaceholderArray.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <alloca.h>
#include <mach-o/loader.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Inferred organization: the NSConstantArray methods lie together in the iOS Simulator __text section
// (0x1c01a3798-0x1c01a3ffc), with the three that only crash (-initWithObjects:count:, -init, -dealloc;
// 0x1c0308734-0x1c030874c) in the cold region; following the owner's decision, the class has its own
// translation unit. Objective-C, manual reference counting (the memory-management methods are the immortal
// object's). The os_log argument packing of the range raises is __os_log_helper_1_2_3_8_32_8_0_8_0
// (0x1c01512e0) and __os_log_helper_1_2_4_8_32_8_0_8_0_8_0 (0x1c0151660), the compiler's helpers for
// __builtin_os_log_format, or the same stores inline; they are restored as that builtin. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full).
// iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not
// written yet).

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// libobjc entry point without an SDK declaration (objc-internal.h); supplied, never reimplemented.
extern id objc_alloc(Class cls);

// The NSException and __NSConstantArrayEnumerator class symbols, sent to directly.
extern char exceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));
extern char constantArrayEnumeratorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                     OF_NAME_STRING(__ONSConstantArrayEnumerator));

// The message this unit sends to the enumerator it allocates.
@protocol ConstantArrayEnumeratorMessages

- (id)initWithConstantArray:(id)array;

@end

// The raise every bounds check of this unit inlines: an os_log pack for the format's argument buffer
// (bufferSize bytes; _os_log_pack_size, the pack on the stack, filled with errno 0 and the image's header),
// the arguments packed as __builtin_os_log_format does, the reason made with kCFAllocatorSystemDefault and
// autoreleased through _CFAutoreleasePoolAddObject, and +[NSException
// exceptionWithName:reason:userInfo:osLogPack:size:] (no user info) thrown.
#define RAISE_LOGGED_EXCEPTION(name, bufferSize, format, ...)                                                \
    do {                                                                                                     \
        size_t packSize = _os_log_pack_size(bufferSize);                                                     \
        void *pack = alloca((packSize + 15) & ~(size_t)15);                                                  \
        uint8_t *packBuffer = _os_log_pack_fill(pack, packSize, 0, &__dso_handle, format);                   \
        __builtin_os_log_format(packBuffer, format, __VA_ARGS__);                                            \
        OCFStringRef reason =                                                                                \
            OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR(format), __VA_ARGS__);        \
        id exception = ((id (*)(id, SEL, id, id, id, void *, size_t))objc_msgSend)(                          \
            (id)exceptionClass,                                                                              \
            sel_getUid("exceptionWithName:reason:userInfo:osLogPack:size:"),                                 \
            (id)(name),                                                                                      \
            _OCFAutoreleasePoolAddObject(NULL, reason),                                                      \
            nil,                                                                                             \
            pack,                                                                                            \
            packSize                                                                                         \
        );                                                                                                   \
        objc_exception_throw(exception);                                                                     \
    } while (0)

@implementation ONSConstantArray

// Native -[NSConstantArray count], 0x1c01a3798. Non-external. The _count ivar.
- (NSUInteger)count {
    return _count;
}

// Native -[NSConstantArray objectAtIndex:], 0x1c01a37a8. Non-external. An index with bit 63 clear below the
// count answers the object; otherwise NSRangeException: "*** %s: index %lu beyond bounds for empty array"
// (buffer 0x16) for an empty array, else "*** %s: index %lu beyond bounds [0 .. %lu]" (buffer 0x20) with the
// count minus one; %s is the method name literal.
- (id)objectAtIndex:(NSUInteger)index {
    NSUInteger count = _count;
    if ((intptr_t)index >= 0 && count > index) {
        return _objects[index];
    }
    if (count == 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSRangeException,
            0x16,
            "*** %s: index %lu beyond bounds for empty array",
            "-[NSConstantArray objectAtIndex:]",
            index
        );
    }
    RAISE_LOGGED_EXCEPTION(
        ONSRangeException,
        0x20,
        "*** %s: index %lu beyond bounds [0 .. %lu]",
        "-[NSConstantArray objectAtIndex:]",
        index,
        count - 1
    );
}

// Native -[NSConstantArray getObjects:range:], 0x1c01a3988. Non-external. Order: a NULL buffer with a nonzero
// length raises NSInvalidArgumentException "*** %s: pointer to objects array is NULL but length is %lu"; a
// length of 2^61 or more raises "*** %s: count (%lu) of objects array is ridiculous" (both buffer 0x16); a
// location with bit 63 set or a count below location + length (unsigned) raises NSRangeException with
// "*** %s: range {%lu, %lu} extends beyond bounds for empty array" (buffer 0x20) or "... [0 .. %lu]" (buffer
// 0x2A, the count minus one). Lengths above 4 are copied with memmove (a tail call); 1-4 objects one by one.
- (void)getObjects:(id *)objects range:(OCFRange)range {
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    if (objects == NULL && length != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[NSConstantArray getObjects:range:]",
            length
        );
    }
    if ((OF_WORD_SHIFT(length, 61)) != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[NSConstantArray getObjects:range:]",
            length
        );
    }
    NSUInteger count = _count;
    if ((intptr_t)location >= 0 && count >= location + length) {
        const id *source = _objects + location;
        if (length > 4) {
            memmove(objects, source, length * sizeof(id));
            return;
        }
        for (NSUInteger index = 0; index < length; index++) {
            objects[index] = source[index];
        }
        return;
    }
    if (count == 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSRangeException,
            0x20,
            "*** %s: range {%lu, %lu} extends beyond bounds for empty array",
            "-[NSConstantArray getObjects:range:]",
            location,
            length
        );
    }
    RAISE_LOGGED_EXCEPTION(
        ONSRangeException,
        0x2A,
        "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",
        "-[NSConstantArray getObjects:range:]",
        location,
        length,
        count - 1
    );
}

// Native -[NSConstantArray objectEnumerator], 0x1c01a3d64. Non-external. An autoreleased
// __NSConstantArrayEnumerator (objc_alloc of its class reference, -initWithConstantArray: self).
- (id)objectEnumerator {
    id enumerator = objc_alloc((Class)(void *)constantArrayEnumeratorClass);
    return [[enumerator initWithConstantArray:self] autorelease];
}

// Native -[NSConstantArray objectAtIndexedSubscript:], 0x1c01a3d94. Non-external. -objectAtIndex: with this
// method's name in the reasons.
- (id)objectAtIndexedSubscript:(NSUInteger)index {
    NSUInteger count = _count;
    if ((intptr_t)index >= 0 && count > index) {
        return _objects[index];
    }
    if (count == 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSRangeException,
            0x16,
            "*** %s: index %lu beyond bounds for empty array",
            "-[NSConstantArray objectAtIndexedSubscript:]",
            index
        );
    }
    RAISE_LOGGED_EXCEPTION(
        ONSRangeException,
        0x20,
        "*** %s: index %lu beyond bounds [0 .. %lu]",
        "-[NSConstantArray objectAtIndexedSubscript:]",
        index,
        count - 1
    );
}

// Native +[NSConstantArray new], 0x1c01a3f74. Non-external. objc_opt_new of the NSArray class reference (a
// tail call).
+ (id)new {
    return (id)[ONSArray new];
}

// Native +[NSConstantArray allocWithZone:], 0x1c01a3f80. Non-external. A tail call of
// __NSArrayImmutablePlaceholder; the zone is not read.
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSArrayImmutablePlaceholder();
}

// Native -[NSConstantArray mutableCopy], 0x1c01a3f84. Non-external. __NSArrayM_new of the objects and the
// count, retaining them (a tail call).
- (id)mutableCopy {
    return __NSArrayM_new(_objects, _count, NO);
}

// Native -[NSConstantArray mutableCopyWithZone:], 0x1c01a3fa8. Non-external. As -mutableCopy; the zone is not
// read.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return __NSArrayM_new(_objects, _count, NO);
}

// Native -[NSConstantArray autorelease], 0x1c01a3fcc. Non-external. Answers the receiver (RET with X0 kept).
- (id)autorelease {
    return self;
}

// Native -[NSConstantArray retain], 0x1c01a3fd0. Non-external. Answers the receiver.
- (id)retain {
    return self;
}

// Native -[NSConstantArray _tryRetain], 0x1c01a3fd4. Non-external.
- (BOOL)_tryRetain {
    return YES;
}

// Native -[NSConstantArray _isDeallocating], 0x1c01a3fdc. Non-external.
- (BOOL)_isDeallocating {
    return NO;
}

// Native -[NSConstantArray retainCount], 0x1c01a3fe4. Non-external. Answers all ones.
- (NSUInteger)retainCount {
    return (NSUInteger)-1;
}

// Native -[NSConstantArray release], 0x1c01a3fec. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[NSConstantArray copyWithZone:], 0x1c01a3ff0. Non-external. Answers the receiver.
- (id)copyWithZone:(struct _NSZone *)zone {
    return self;
}

// Native -[NSConstantArray copy], 0x1c01a3ff4. Non-external. Answers the receiver.
- (id)copy {
    return self;
}

// Native +[NSConstantArray automaticallyNotifiesObserversForKey:], 0x1c01a3ff8. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[NSConstantArray initWithObjects:count:], 0x1c0308734. Non-external. Crashes "attempting to
// allocate a constant object".
- (id)initWithObjects:(const id *)objects count:(NSUInteger)count {
    crashWithMessage("attempting to allocate a constant object");
}

// Native -[NSConstantArray init], 0x1c0308748. Non-external. BRK #1.
- (id)init {
    __builtin_trap();
}

// Native -[NSConstantArray dealloc], 0x1c030874c. Non-external. BRK #1; it never reaches super's -dealloc, so
// the missing-super-call diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"

- (void)dealloc {
    __builtin_trap();
}

#pragma clang diagnostic pop

@end

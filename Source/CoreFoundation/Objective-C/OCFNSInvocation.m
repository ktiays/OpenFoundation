//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSInvocation.h"

#include "CoreFoundation/Base/OCFUtilities.h"

#include <alloca.h>
#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// Inferred organization: -[NSInvocation _initWithMethodSignature:frame:buffer:size:] (0x1c028a938) lies
// among the forwarding and invocation code of the iOS Simulator __text section, with
// ___magic_cookie_block_invoke at 0x1c028e154 and the .cold.1 fragment (0x1c030f2a4, a jump to
// OUTLINED_FUNCTION_1_28 at 0x1c028e5f8, the dispatch_once call) in the cold region; following the owner's
// decision, the class has its own translation unit. The cookie function is inlined natively and its block is
// a global block (___block_literal_global_33); both are restored here with the method that needs them.
// ___forwarding___ (0x1c028bcc0, with .cold.1-.cold.4 at 0x1c030f334-0x1c030f46c) follows the method in the
// same region; it calls the method directly and reads the class's ivars, so it is restored here.
// Objective-C, manual reference counting (objc_retain for the signature, objc_release of a stack
// invocation). Unless a definition says
// otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified
// (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// Native magic_cookie: no symbol (inlined); its function statics magic_cookie.oGuard (__DATA,__bss
// 0x22e6d37b0) and magic_cookie.oValue (0x22e6d37a8). The predicate is tested inline and the dispatch_once
// call is the cold fragment. Native ___magic_cookie_block_invoke, 0x1c028e154 (non-external): a value already
// set is kept; otherwise arc4random_buf fills the four bytes until they are nonzero.
static uint32_t magic_cookie(void) {
    static dispatch_once_t oGuard;
    static uint32_t oValue;
    dispatch_once(&oGuard, ^{
      if (oValue != 0) {
          return;
      }
      do {
          arc4random_buf(&oValue, 4);
      } while (oValue == 0);
    });
    return oValue;
}

// libdyld's dyld_program_sdk_at_least (no SDK declaration); supplied, never reimplemented.
extern bool dyld_program_sdk_at_least(OCFBuildVersion version);

// libobjc's tagged-pointer variables, read for each decode as natively (the GOT slots 0x222409420 and
// 0x222409408, bound by use as in Number/OCFNumber.c; confidence: medium).
extern uintptr_t objc_debug_taggedpointer_obfuscator;
extern const uint8_t objc_debug_tag60_permutations[8];

// The build version every tag-0 decision asks dyld about (MOV X0, #0x7E30901FFFFFFFF): any platform, version
// 0x07e30901.
static const OCFBuildVersion kForwardingAtomVersion = {0xffffffff, 0x07e30901};

// The class symbols the native code loads through its class references (NSInvocation 0x2227ce328, NSData
// 0x2227ce2c8).
extern char invocationClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSInvocation));
extern char dataClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSData));

// Selectors ___forwarding___ sends to the receiver, the signature and NSData, declared for @selector only; no
// protocol object is emitted.
@protocol ForwardingMessages

- (id)forwardingTargetForSelector:(SEL)selector;
- (id)methodSignatureForSelector:(SEL)selector;
- (void)_forwardStackInvocation:(id)invocation;
- (void)forwardInvocation:(id)invocation;
- (void)doesNotRecognizeSelector:(SEL)selector;
- (const void *)_frameDescriptor;
- (NSUInteger)numberOfArguments;
- (const char *)methodReturnType;
- (id)dataWithBytes:(const void *)bytes length:(NSUInteger)length;
- (const void *)bytes;

@end

// The decode ___forwarding___ inlines twice: true for a tagged pointer (bit 63) whose deobfuscated tag is
// objc_debug_tag60_permutations[0] (an NSAtom). The obfuscator is not applied when the pointer's bits 62-63
// and 0-2 are all set. isAtomPointer is a local descriptive name.
static inline bool isAtomPointer(id object) {
    uintptr_t pointer = (uintptr_t)object;
    if ((intptr_t)pointer >= 0) {
        return false;
    }
    uintptr_t obfuscator = (~pointer & 0xC000000000000007) == 0 ? 0 : objc_debug_taggedpointer_obfuscator;
    uintptr_t tag = (obfuscator ^ pointer) & 7;
    uintptr_t index = 0;
    while (tag != objc_debug_tag60_permutations[index]) {
        ++index;
        if (index == 7) {
            return false;
        }
    }
    return index == 0;
}

extern id objc_alloc(Class cls);
extern id objc_autorelease(id object);
extern id objc_opt_new(Class cls);
extern Class objc_opt_class(id object);
extern id objc_retain(id object);
extern void objc_release(id object);
extern char invocationExceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));
extern char invocationMutableArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableArray));

static void __NSICreateBackingForArgumentIfNeeded(
    ONSInvocation *invocation, const ForwardingArgumentRecord *record, OCFIndex index, bool copyContents
);
static void __NSI3(const ForwardingArgumentRecord *record, void *base, id *container, bool inFrame);

@implementation ONSInvocation

// Native -[NSInvocation _initWithMethodSignature:frame:buffer:size:], 0x1c028a938, with its .cold.1
// (0x1c030f2a4). Was a private external. A nil receiver answers nil. The buffer must hold the signature's
// frame plus 0x140 bytes, else BRK #1. The signature is retained, the word at 32 cleared, the buffer zeroed
// and the frame placed 0x140 bytes into it; the cookie is stored, the caller's frame (when given) copied in,
// and the bytes at 52 and 53 set to 0 and 1 (one 16-bit store of 0x100).
- (id)_initWithMethodSignature:(id)signature
                         frame:(const void *)frame
                        buffer:(void *)buffer
                          size:(size_t)size {
    if (self == nil) {
        return nil;
    }
    if (size < [signature frameLength] + 0x140) {
        __builtin_trap();
    }
    _signature = [signature retain];
    _container = nil;
    _buffer = buffer;
    bzero(buffer, size);
    _frame = (uint8_t *)_buffer + 0x140;
    _magic = magic_cookie();
    if (frame != NULL) {
        void *destination = _frame;
        memmove(destination, frame, [signature frameLength]);
    }
    _byteAt52 = 0;
    _byteAt53 = 1;
    return self;
}

// Native +[NSInvocation _invocationWithMethodSignature:frame:], 0x1c028aa04. Non-external.
// The cookie initializer is .cold.1 at 0x1c030f2a8. iOS Device divergence: unread.
+ (id)_invocationWithMethodSignature:(id)signature frame:(void *)frame {
    size_t size;
    OCFStringRef format = NULL;
    if (signature == nil) {
        format = OF_STR("%@: method signature argument cannot be nil");
    } else if (__builtin_add_overflow([signature frameLength], (size_t)0x140, &size)) {
        format = OF_STR("Overflow while allocating space for method signature %@");
    }
    if (format != NULL) {
        OCFStringRef proem = __CFExceptionProem(self, _cmd);
        OCFStringRef reason = OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, format, proem);
        id autoreleased = _OCFAutoreleasePoolAddObject(NULL, reason);
        id exception = ((id (*)(id, SEL, OCFStringRef, id, id))objc_msgSend)(
            (id)invocationExceptionClass,
            sel_getUid("exceptionWithName:reason:userInfo:"),
            ONSInvalidArgumentException,
            autoreleased,
            nil
        );
        objc_exception_throw(exception);
    }

    ONSInvocation *invocation = objc_alloc(self);
    invocation->_signature = objc_retain(signature);
    void *buffer = malloc_type_calloc(1, size, 0xd20796a1);
    // CoreFoundation defect: at 0x1c028aa70 the unchecked allocation can form an invalid frame.
    // Fail before the frame store or copy when allocation fails.
    if (buffer == NULL) {
        crashWithMessage("Out of memory");
    }
    invocation->_frame = (uint8_t *)buffer + 0x140;
    invocation->_buffer = buffer;
    invocation->_container = nil;
    invocation->_magic = magic_cookie();
    if (frame != NULL) {
        void *destination = invocation->_frame;
        memmove(destination, frame, [signature frameLength]);
    }
    invocation->_byteAt52 = 0;
    return objc_autorelease(invocation);
}

// Native -[NSInvocation init], 0x1c028ab74. Non-external. iOS Device divergence: unread.
- (instancetype)init {
    objc_release(self);
    return nil;
}

// Native -[NSInvocation dealloc], 0x1c028ab8c. Non-external. iOS Device divergence: unread.
// Reading: Local/Scratch/cf-core-r2-c2/invocation-cleanup-ida.txt.
- (void)dealloc {
    if (_byteAt53 == 1) {
        objc_release(_container);
        objc_release(_signature);
        objc_destructInstance(self);
        return;
    }
    if (_buffer != NULL) {
        free(_buffer);
    }
    _frame = NULL;
    _buffer = NULL;
    objc_release(_container);
    objc_release(_signature);
    [super dealloc];
}

// Native -[NSInvocation retainArguments], 0x1c028ac68, with .cold.1 at 0x1c030f2ac and
// .cold.2 at 0x1c030f2b0. Non-external. iOS Device divergence: unread.
// The assembly supplies the return record, the selected storage and the container to __NSI3.
- (void)retainArguments {
    uint32_t cookie = _magic;
    if (cookie != magic_cookie()) {
        crashWithMessage("Corrupt NSInvocation detected.");
    }
    if (_byteAt52 != 0) {
        return;
    }
    _byteAt52 = 1;
    const ForwardingFrameDescriptor *descriptor = [_signature _frameDescriptor];
    __NSICreateBackingForArgumentIfNeeded(self, descriptor->returnRecord, -1, true);
    const ForwardingArgumentRecord *result = descriptor->returnRecord;
    __NSI3(result, (result->flags & 0x80) != 0 ? _frame : _buffer, &_container, true);
    OCFIndex index = 0;
    for (const ForwardingArgumentRecord *argument = descriptor->firstArgument; argument != NULL;
         argument = argument->next, ++index) {
        __NSICreateBackingForArgumentIfNeeded(self, argument, index, true);
        __NSI3(argument, _frame, &_container, true);
    }
}

// Native __NSICreateBackingForArgumentIfNeeded, 0x1c028ad54, with .cold.1 at 0x1c030f2c4 and
// .cold.2 at 0x1c030f2d8. Non-external. iOS Device divergence: unread.
static void __NSICreateBackingForArgumentIfNeeded(
    ONSInvocation *invocation, const ForwardingArgumentRecord *record, OCFIndex index, bool copyContents
) {
    if ((record->flags & 0x80) == 0) {
        return;
    }
    if (invocation->_argumentBackingFlags == NULL) {
        OCFIndex capacity = (OCFIndex)[invocation->_signature numberOfArguments] + 1;
        OCFMutableDataRef flags = OCFDataCreateMutable(kOCFAllocatorSystemDefault, capacity);
        if (flags == NULL) {
            crashWithMessage("Out of memory");
        }
        invocation->_argumentBackingFlags = OCFDataGetMutableBytePtr(flags);
        if (invocation->_container == nil) {
            invocation->_container = objc_opt_new((Class)invocationMutableArrayClass);
            if (invocation->_container == nil) {
                crashWithMessage("Out of memory");
            }
        }
        [invocation->_container addObject:(id)flags];
        OCFRelease(flags);
    }
    uint8_t *flag = invocation->_argumentBackingFlags + (index + 1);
    if ((*flag & 1) != 0) {
        return;
    }
    OCFMutableDataRef backing =
        (OCFMutableDataRef)objc_autorelease((id)OCFDataCreateMutable(kOCFAllocatorSystemDefault, 0));
    const uint8_t *bytes = NULL;
    if (copyContents) {
        uint32_t offset = record->offset + record->extraOffset;
        memcpy(&bytes, (const uint8_t *)invocation->_frame + offset, sizeof(bytes));
    }
    uint32_t length = record->type->size;
    if (bytes != NULL) {
        OCFDataAppendBytes(backing, bytes, length);
    } else {
        OCFDataSetLength(backing, length);
    }
    if (invocation->_container == nil) {
        invocation->_container = objc_opt_new((Class)invocationMutableArrayClass);
    }
    [invocation->_container addObject:(id)backing];
    uint8_t *storage = OCFDataGetMutableBytePtr(backing);
    memcpy((uint8_t *)invocation->_frame + record->offset, &storage, sizeof(storage));
    *flag = 1;
}

// Native __NSI3, 0x1c028aeb0. Non-external. iOS Device divergence: unread.
static void __NSI3(const ForwardingArgumentRecord *record, void *base, id *container, bool inFrame) {
    uint16_t flags;
    uint8_t *slot;
    for (;;) {
        if (record == NULL) {
            return;
        }
        uint32_t offset = inFrame ? record->offset + record->extraOffset : record->memoryOffset;
        slot = (uint8_t *)base + offset;
        flags = record->flags;
        if ((flags & 0x80) == 0) {
            break;
        }
        inFrame = false;
        record = record->type;
        memcpy(&base, slot, sizeof(base));
    }
    if ((flags & 0x1800) == 0x1800) {
        uint8_t *bytes;
        memcpy(&bytes, slot, sizeof(bytes));
        if (bytes == NULL) {
            return;
        }
        bool linkedOnOrAfter = _OCFExecutableLinkedOnOrAfter(6);
        if ((flags & 3) == 1 || !linkedOnOrAfter) {
            OCFMutableDataRef data =
                (OCFMutableDataRef)objc_autorelease((id)OCFDataCreateMutable(kOCFAllocatorSystemDefault, 0));
            OCFDataAppendBytes(data, bytes, record->size);
            if (*container == nil) {
                *container = objc_opt_new((Class)invocationMutableArrayClass);
            }
            [*container addObject:(id)data];
            bytes = OCFDataGetMutableBytePtr(data);
            memcpy(slot, &bytes, sizeof(bytes));
        }
        for (const ForwardingArgumentRecord *child = record->type; child != NULL; child = child->next) {
            __NSI3(child, bytes, container, false);
        }
    } else if ((flags & 0xc00) != 0) {
        for (const ForwardingArgumentRecord *child = record->type; child != NULL; child = child->next) {
            __NSI3(child, base, container, inFrame);
        }
    } else if ((flags & 0x2000) != 0) {
        id object;
        memcpy(&object, slot, sizeof(object));
        if ((flags & 0x8000) != 0) {
            object = objc_autorelease([object copy]);
            memcpy(slot, &object, sizeof(object));
        }
        if (object != nil) {
            if (*container == nil) {
                *container = objc_opt_new((Class)invocationMutableArrayClass);
            }
            [*container addObject:object];
        }
    } else if ((flags & 0x4000) != 0) {
        const char *string;
        memcpy(&string, slot, sizeof(string));
        if (string != NULL) {
            if (*container == nil) {
                *container = objc_opt_new((Class)invocationMutableArrayClass);
            }
            OCFMutableDataRef data =
                (OCFMutableDataRef)objc_autorelease((id)OCFDataCreateMutable(kOCFAllocatorSystemDefault, 0));
            OCFDataAppendBytes(data, (const uint8_t *)string, (OCFIndex)(strlen(string) + 1));
            [*container addObject:(id)data];
            uint8_t *bytes = OCFDataGetMutableBytePtr(data);
            memcpy(slot, &bytes, sizeof(bytes));
        }
    }
}

// Native -[NSInvocation copyWithZone:], 0x1c028b114, with .cold.1 at 0x1c030f2ec and
// .cold.2 at 0x1c030f2f0. Non-external. iOS Device divergence: unread.
// The captured method allocates a cleared frame and leaves it cleared before retaining arguments (kept).
- (id)copyWithZone:(struct _NSZone *)zone {
    (void)zone;
    uint32_t cookie = _magic;
    if (cookie != magic_cookie()) {
        crashWithMessage("Corrupt NSInvocation detected.");
    }
    ONSInvocation *copy = objc_alloc(objc_opt_class(self));
    copy->_signature = objc_retain(_signature);
    size_t size;
    // CoreFoundation defect: 0x1c028b16c and 0x1c028b17c do not check size overflow or allocation failure.
    if (__builtin_add_overflow([_signature frameLength], (size_t)0x140, &size)) {
        crashWithMessage("Out of memory");
    }
    void *buffer = malloc_type_calloc(1, size, 0xed4e1c41);
    if (buffer == NULL) {
        crashWithMessage("Out of memory");
    }
    copy->_frame = (uint8_t *)buffer + 0x140;
    copy->_buffer = buffer;
    copy->_container = nil;
    copy->_magic = _magic;
    if (_byteAt52 != 0) {
        [copy retainArguments];
    }
    return copy;
}

// Native ___forwarding___, 0x1c028bcc0, with its function static ___forwarding___.placeholder (__DATA,__bss
// 0x22e6d37a0, 8 bytes) and .cold.1 (0x1c030f334, the atom path), .cold.2 (0x1c030f39c, no
// forwardInvocation:), .cold.3 (0x1c030f3e0, no doesNotRecognizeSelector:) and .cold.4 (0x1c030f410, the
// zombie). Hidden (was a private external); called by _CF_forwarding_prep_0 with the register frame (X0-X8 at
// 0, Q0-Q7 at 0x50, stack arguments from 0xE0) and whether the call returns a structure. The selector
// references (forwardingTargetForSelector:, methodSignatureForSelector:, _forwardStackInvocation:,
// forwardInvocation:, doesNotRecognizeSelector:) lie outside the extracted image and are bound by the sends
// that follow each class_respondsToSelector check (confidence: medium).
//
// 1. An atom receiver (tag index 0) of a program linked before the 0x07e30901 SDK set is forwarded to its
//    getAtomTarget: the target replaces the receiver, __invoking___ sends the message through objc_msgSend
//    (the GOT slot 0x222409518, bound by use; confidence: medium) with a 0x400-byte frame, the receiver is
//    put back when the frame still holds the target, and the frame is answered.
// 2. -forwardingTargetForSelector: (when the class responds): a target other than nil and the receiver
//    replaces the receiver in the frame and NULL is answered (the trampoline then sends the message), unless
//    the target is itself an atom, which loops back to 1 with the target as receiver.
// 3. A class whose name begins "_NSZombie_" logs "message sent to deallocated instance" (level 3, after an
//    allocation event 21 when __CFOASafe is set: OUTLINED_FUNCTION_2_24, 0x1c028e60c,
//    __CFRecordAllocationEvent(21, receiver, 0, 0, 0), read in iphone-simulator-corpus-delta3.json,
//    confidence: verified) and executes BRK #1.
// 4. -methodSignatureForSelector: (a class without it logs one of two warnings, chosen by whether it has a
//    superclass); a nil signature, or none, reaches the unrecognized-selector path: a warning when
//    sel_getUid of the selector's name differs from it, then -doesNotRecognizeSelector: (BRK #1 after it), or
//    the warning that it is missing and BRK #1.
// 5. A struct-return disagreement between the caller and the signature (flags bit 6) is logged.
// 6. A class answering _forwardStackInvocation: gets an NSInvocation constructed on the stack
//    (objc_constructInstance on the instance size, cleared twice, and a cleared buffer of the frame length
//    plus 0x140 bytes) initialized directly through -_initWithMethodSignature:frame:buffer:size:; otherwise a
//    class without forwardInvocation: stores "*** NSForwarding: object %p of class '%s' does not implement
//    forwardInvocation:" (snprintf, 256 bytes, in the function's stack area) as the crash message and
//    executes BRK #1, and any other gets +_invocationWithMethodSignature:frame: and -forwardInvocation:.
// 7. When the invocation's byte at 52 is set and the return record's flags bit 7 is set, the result is copied
//    (memmove, the type record's length) from the pointer in the invocation's frame to the pointer in the
//    caller's frame at the record's offset.
// 8. A void return type ("v", or "V" then "v") answers the placeholder; otherwise the invocation's buffer
//    (the result registers). A stack invocation's result is first copied into an autoreleased NSData
//    (dataWithBytes:length: with the buffer size), whose bytes are answered, and the invocation is released
//    in both cases.
void *___forwarding___(void *frame, intptr_t isStret) {
    static uint64_t placeholder;
    char message[0x100];
    id *slots = (id *)frame;
    id receiver = slots[0];
    SEL selector = (SEL)slots[1];
    Class cls;
    const char *className;
    id signature;
    const ForwardingFrameDescriptor *descriptor;
    BOOL stackInvocation;
    ONSInvocation *invocation;
    size_t size;
    bool atom = isAtomPointer(receiver);
    for (;;) {
        if (atom && !dyld_program_sdk_at_least(kForwardingAtomVersion)) {
            id target = getAtomTarget(receiver);
            slots[0] = target;
            __invoking___((void (*)(void))objc_msgSend, frame, frame, 0x400, 0);
            if (slots[0] == target) {
                slots[0] = receiver;
            }
            return frame;
        }
        cls = object_getClass(receiver);
        className = class_getName(cls);
        if (!class_respondsToSelector(cls, @selector(forwardingTargetForSelector:))) {
            break;
        }
        id target =
            ((id (*)(id, SEL, SEL))objc_msgSend)(receiver, @selector(forwardingTargetForSelector:), selector);
        if (target == nil || target == receiver) {
            break;
        }
        if (!isAtomPointer(target)) {
            slots[0] = target;
            return NULL;
        }
        receiver = target;
        atom = true;
    }
    if (strncmp(className, "_NSZombie_", 10) == 0) {
        if (__OCFOASafe == 1) {
            __OCFRecordAllocationEvent(21, receiver, 0, 0, NULL);
        }
        OCFLog(
            3,
            OF_STR("*** -[%s %s]: message sent to deallocated instance %p"),
            className + 10,
            sel_getName(selector),
            receiver
        );
        __builtin_trap();
    }
    if (!class_respondsToSelector(cls, @selector(methodSignatureForSelector:))) {
        Class superclass = class_getSuperclass(cls);
        const char *name = object_getClassName(receiver);
        if (superclass == Nil) {
            OCFLog(
                4,
                OF_STR(
                    "*** NSForwarding: warning: object %p of class '%s' does not implement "
                    "methodSignatureForSelector: -- did you forget to declare the superclass of '%s'?"
                ),
                receiver,
                name,
                object_getClassName(receiver)
            );
        } else {
            OCFLog(
                4,
                OF_STR(
                    "*** NSForwarding: warning: object %p of class '%s' does not implement "
                    "methodSignatureForSelector: -- trouble ahead"
                ),
                receiver,
                name
            );
        }
        goto unrecognized;
    }
    signature =
        ((id (*)(id, SEL, SEL))objc_msgSend)(receiver, @selector(methodSignatureForSelector:), selector);
    if (signature == nil) {
        goto unrecognized;
    }
    descriptor =
        ((const ForwardingFrameDescriptor *(*)(id, SEL))objc_msgSend)(signature, @selector(_frameDescriptor));
    if ((uintptr_t)isStret != ((descriptor->returnRecord->flags >> 6) & 1)) {
        const char *selectorName = sel_getName(selector);
        OCFLog(
            4,
            OF_STR(
                "*** NSForwarding: warning: method signature and compiler disagree on struct-return-edness "
                "of "
                "'%s'.  Signature thinks it does%s return a struct, and compiler thinks it does%s."
            ),
            selectorName,
            (descriptor->returnRecord->flags & 0x40) == 0 ? " not" : "",
            isStret == 0 ? " not" : ""
        );
    }
    stackInvocation =
        class_respondsToSelector(object_getClass(receiver), @selector(_forwardStackInvocation:));
    if (stackInvocation) {
        (void)[ONSInvocation self];
        size_t frameLength = ((size_t (*)(id, SEL))objc_msgSend)(signature, @selector(frameLength));
        size = frameLength + 0x140;
        size_t instanceSize = class_getInstanceSize((Class)invocationClass);
        void *storage = alloca((instanceSize + 15) & ~(size_t)15);
        bzero(storage, instanceSize);
        bzero(storage, instanceSize);
        void *buffer = alloca((frameLength + 0x14F) & ~(size_t)15);
        bzero(buffer, size);
        invocation = objc_constructInstance((Class)invocationClass, storage);
        [invocation _initWithMethodSignature:signature frame:frame buffer:buffer size:size];
        ((void (*)(id, SEL, id))objc_msgSend)(receiver, @selector(_forwardStackInvocation:), invocation);
    } else {
        if (!class_respondsToSelector(object_getClass(receiver), @selector(forwardInvocation:))) {
            snprintf(
                message,
                0x100,
                "*** NSForwarding: object %p of class '%s' does not implement forwardInvocation:",
                receiver,
                object_getClassName(receiver)
            );
            crashWithMessage(message);
        }
        invocation = [ONSInvocation _invocationWithMethodSignature:signature frame:frame];
        ((void (*)(id, SEL, id))objc_msgSend)(receiver, @selector(forwardInvocation:), invocation);
        size = 0;
    }
    if (invocation->_byteAt52 != 0 && (descriptor->returnRecord->flags & 0x80) != 0) {
        const ForwardingArgumentRecord *record = descriptor->returnRecord;
        size_t offset = record->offset;
        void *destination = *(void **)((uint8_t *)frame + offset + record->extraOffset);
        void *source = *(void **)((uint8_t *)invocation->_frame + offset + record->extraOffset);
        memmove(destination, source, record->type->size);
    }
    const char *returnType = ((const char *(*)(id, SEL))objc_msgSend)(signature, @selector(methodReturnType));
    if (returnType[0] == 'v' || (returnType[0] == 'V' && returnType[1] == 'v')) {
        if (stackInvocation) {
            [invocation release];
        }
        return &placeholder;
    }
    void *result = invocation->_buffer;
    if (stackInvocation) {
        id data = ((id (*)(id, SEL, const void *, NSUInteger))objc_msgSend)(
            (id)dataClass, @selector(dataWithBytes:length:), result, size
        );
        result = (void *)((const void *(*)(id, SEL))objc_msgSend)(data, @selector(bytes));
        [invocation release];
    }
    return result;

unrecognized:
    {
        const char *selectorName = sel_getName(selector);
        SEL registered = sel_getUid(selectorName);
        if (registered != selector) {
            OCFLog(
                4,
                OF_STR(
                    "*** NSForwarding: warning: selector (%p) for message '%s' does not match selector known "
                    "to "
                    "Objective C runtime (%p)-- abort"
                ),
                selector,
                selectorName,
                registered
            );
        }
        if (class_respondsToSelector(object_getClass(receiver), @selector(doesNotRecognizeSelector:))) {
            ((void (*)(id, SEL, SEL))objc_msgSend)(receiver, @selector(doesNotRecognizeSelector:), selector);
            __builtin_trap();
        }
        OCFLog(
            4,
            OF_STR(
                "*** NSForwarding: warning: object %p of class '%s' does not implement "
                "doesNotRecognizeSelector: -- "
                "abort"
            ),
            receiver,
            object_getClassName(receiver)
        );
        __builtin_trap();
    }
}

@end

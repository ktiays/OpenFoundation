//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Message forwarding for the wasm port, in place of Source/CoreFoundation/Objective-C/OCFForwarding.c
// (trimmed, Port/Trim.tsv). The native file is the arm64 forwarding and invoking trampolines plus the
// type-string set callbacks. wasm has no untyped calls, so objc4 forwards a message through
// objc_wasm_set_forward_handler with the call's lowered argument slots instead of a register frame.
//
// The forward handler follows ___forwarding___'s order: -forwardingTargetForSelector: first (a different
// target receives the original call), then -methodSignatureForSelector:, and -doesNotRecognizeSelector: when
// there is no signature. A message with a signature would be packaged as an NSInvocation for
// -forwardInvocation:; the port does not build one from wasm argument slots yet and stops with a message.

#import "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/String/OCFString.h"

#include <objc/message.h>
#include <objc/runtime.h>
#include <objc/wasm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern bool _dyld_is_memory_immutable(const void *address, size_t length);

// The type-string callbacks, as Source/CoreFoundation/Objective-C/OCFForwarding.c restores them (native
// typeStringRetain 0x1c0289694, typeStringRelease 0x1c02896f4, copyTypeStringDesc 0x1c0289738,
// typeStringEqual 0x1c0289750, typeStringHash 0x1c028976c; iOS Simulator 24A434, CoreFoundation
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC). The port has no immutable image memory, so every string is copied.
const void *typeStringRetain(OCFAllocatorRef allocator, const void *value) {
    (void)allocator;
    size_t length = strlen(value);
    if (_dyld_is_memory_immutable(value, length + 1)) {
        return value;
    }
    void *copy = malloc_type_malloc(length + 1, 0x16488430);
    return memcpy(copy, value, length + 1);
}

void typeStringRelease(OCFAllocatorRef allocator, const void *value) {
    (void)allocator;
    if (_dyld_is_memory_immutable(value, strlen(value) + 1)) {
        return;
    }
    free((void *)value);
}

OCFStringRef copyTypeStringDesc(const void *value) {
    return OCFStringCreateWithCString(kOCFAllocatorSystemDefault, value, 0x600);
}

Boolean typeStringEqual(const void *value1, const void *value2) {
    return strcmp(value1, value2) == 0;
}

OCFHashCode typeStringHash(const void *value) {
    return OCFStringHashCString(value, (OCFIndex)strlen(value));
}

static void unsupported(const char *what) {
    fprintf(stderr, "OpenFoundation (wasm): %s is not supported by the wasm port\n", what);
    abort();
}

// Registered with objc_setForwardHandler by the restored runtime initialization; objc4's wasm dispatch never
// calls it, because it forwards through the wasm forward handler below.
void _CF_forwarding_prep_0(void) {
    unsupported("an untyped forwarding entry");
}

// -[NSInvocation invoke] and ___forwarding___ call a function with a register frame; wasm has none.
void __invoking___(
    void (*function)(void), void *returnFrame, void *argumentFrame, size_t frameSize, uint32_t flags
) {
    (void)function;
    (void)returnFrame;
    (void)argumentFrame;
    (void)frameSize;
    (void)flags;
    unsupported("invoking an NSInvocation");
}

// The first argument slot of the receiver: after the structure-return pointer when the call returns one
// (flag 1 of the ABI signature "return:flags:parameters:resultSize").
static size_t receiverSlot(const char *signature) {
    const char *flags = strchr(signature, ':');
    if (flags == NULL) {
        return 0;
    }
    return (strtoul(flags + 1, NULL, 10) & 1) != 0 ? 1 : 0;
}

static void forwardMessage(objc_wasm_invocation *invocation) {
    const uint64_t *slots = invocation->arguments;
    size_t first = receiverSlot(invocation->abi_signature);
    id receiver = (id)(uintptr_t)slots[first];
    SEL selector = (SEL)(uintptr_t)slots[first + 1];
    Class cls = object_getClass(receiver);
    if (getenv("OF_PORT_TRACE_FORWARDING") != NULL) {
        fprintf(
            stderr,
            "OpenFoundation (wasm): forwarding %c[%s %s]\n",
            class_isMetaClass(cls) ? '+' : '-',
            class_getName(cls),
            sel_getName(selector)
        );
    }

    SEL targetSelector = @selector(forwardingTargetForSelector:);
    if (class_respondsToSelector(cls, targetSelector)) {
        id target = ((id (*)(id, SEL, SEL))objc_msgSend)(receiver, targetSelector, selector);
        if (target != nil && target != receiver) {
            objc_wasm_invocation_invoke(invocation, target, NULL);
            return;
        }
    }
    SEL signatureSelector = @selector(methodSignatureForSelector:);
    id signature = nil;
    if (class_respondsToSelector(cls, signatureSelector)) {
        signature = ((id (*)(id, SEL, SEL))objc_msgSend)(receiver, signatureSelector, selector);
    }
    if (signature == nil) {
        ((void (*)(id, SEL, SEL))objc_msgSend)(receiver, @selector(doesNotRecognizeSelector:), selector);
        // -doesNotRecognizeSelector: raises; a receiver whose override returns gets no result.
        return;
    }
    fprintf(
        stderr,
        "OpenFoundation (wasm): -[%s %s] needs -forwardInvocation:\n",
        class_getName(cls),
        sel_getName(selector)
    );
    unsupported("forwarding a message as an NSInvocation");
}

__attribute__((constructor)) static void installForwardHandler(void) {
    objc_wasm_set_forward_handler(forwardMessage);
}

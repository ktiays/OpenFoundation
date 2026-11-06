//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <malloc/malloc.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: the type-string set callbacks (0x1c0289694-0x1c0289790, in the NSMethodSignature
// region) and the forwarding trampolines _CF_forwarding_prep_0 (0x1c028e630) and __invoking___ (0x1c028e7b0,
// at the end of the NSInvocation region) of the iOS Simulator __text section. ___forwarding___ (0x1c028bcc0)
// and the rest of those units are outside this unit. The native source paths are not in the evidence. Unless
// a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly); iOS Device 24A446: unread.
//
// The two trampolines move argument and result registers and the stack in ways C cannot express (all of
// X0-X8 and Q0-Q7 are saved and reloaded around a call, and __invoking___ builds the callee's stack arguments
// below its own frame). They are written as naked functions whose bodies are the native instruction
// sequences; the build compiles no assembly sources, so a naked function in a C unit takes the place of an
// .s file.

// dyld's _dyld_is_memory_immutable (dyld_priv.h), which the native unit imports; supplied, never
// reimplemented.
extern bool _dyld_is_memory_immutable(const void *address, size_t length);

// Native typeStringRetain, 0x1c0289694. Non-external; the retain callback of the method signatures' type
// string set. A string in immutable memory (its length plus the terminator) is used as is; any other is
// copied into a malloc block of type 0x16488430 (memcpy, a tail call).
const void *typeStringRetain(OCFAllocatorRef allocator, const void *value) {
    (void)allocator;
    size_t length = strlen(value);
    if (_dyld_is_memory_immutable(value, length + 1)) {
        return value;
    }
    void *copy = malloc_type_malloc(length + 1, 0x16488430);
    return memcpy(copy, value, length + 1);
}

// Native typeStringRelease, 0x1c02896f4. Non-external; frees a string not in immutable memory (a tail call).
void typeStringRelease(OCFAllocatorRef allocator, const void *value) {
    (void)allocator;
    if (_dyld_is_memory_immutable(value, strlen(value) + 1)) {
        return;
    }
    free((void *)value);
}

// Native copyTypeStringDesc, 0x1c0289738. Non-external; the string as a kCFStringEncodingASCII (0x600) string
// of kCFAllocatorSystemDefault (a tail call).
OCFStringRef copyTypeStringDesc(const void *value) {
    return OCFStringCreateWithCString(kOCFAllocatorSystemDefault, value, 0x600);
}

// Native typeStringEqual, 0x1c0289750. Non-external; strcmp equality.
Boolean typeStringEqual(const void *value1, const void *value2) {
    return strcmp(value1, value2) == 0;
}

// Native typeStringHash, 0x1c028976c. Non-external; CFStringHashCString over the bytes (a tail call).
OCFHashCode typeStringHash(const void *value) {
    return OCFStringHashCString(value, (OCFIndex)strlen(value));
}

// Native _CF_forwarding_prep_0 (external; also exported as ___forwarding_prep_0___ at the same address),
// 0x1c028e630. A name outside the CF/NS prefix rule, hidden until the owner records a naming decision; so is
// the alias. The forwarding entry: X0-X8 and Q0-Q7 are stored in a 0xD0-byte frame (X0 at 0, Q0 at 0x50)
// under the frame record, so the caller's stack arguments follow at 0xE0, and ___forwarding___(frame, 0)
// runs. A returned frame supplies all result registers; NULL reloads the original registers and tail-calls
// objc_msgSend.
__attribute__((naked)) void _CF_forwarding_prep_0(void) {
    __asm__ volatile(".globl ___forwarding_prep_0___\n"
                     ".private_extern ___forwarding_prep_0___\n"
                     "___forwarding_prep_0___:\n"
                     "stp x29, x30, [sp, #-0x10]!\n"
                     "mov x29, sp\n"
                     "sub sp, sp, #0xd0\n"
                     "str q7, [sp, #0xc0]\n"
                     "str q6, [sp, #0xb0]\n"
                     "str q5, [sp, #0xa0]\n"
                     "str q4, [sp, #0x90]\n"
                     "str q3, [sp, #0x80]\n"
                     "str q2, [sp, #0x70]\n"
                     "str q1, [sp, #0x60]\n"
                     "str q0, [sp, #0x50]\n"
                     "str x8, [sp, #0x40]\n"
                     "str x7, [sp, #0x38]\n"
                     "str x6, [sp, #0x30]\n"
                     "str x5, [sp, #0x28]\n"
                     "str x4, [sp, #0x20]\n"
                     "str x3, [sp, #0x18]\n"
                     "str x2, [sp, #0x10]\n"
                     "str x1, [sp, #0x8]\n"
                     "str x0, [sp]\n"
                     "mov x0, sp\n"
                     "mov x1, #0\n"
                     "bl ____forwarding___\n"
                     "cbz x0, 1f\n"
                     "ldr q7, [x0, #0xc0]\n"
                     "ldr q6, [x0, #0xb0]\n"
                     "ldr q5, [x0, #0xa0]\n"
                     "ldr q4, [x0, #0x90]\n"
                     "ldr q3, [x0, #0x80]\n"
                     "ldr q2, [x0, #0x70]\n"
                     "ldr q1, [x0, #0x60]\n"
                     "ldr q0, [x0, #0x50]\n"
                     "ldr x8, [x0, #0x40]\n"
                     "ldr x7, [x0, #0x38]\n"
                     "ldr x6, [x0, #0x30]\n"
                     "ldr x5, [x0, #0x28]\n"
                     "ldr x4, [x0, #0x20]\n"
                     "ldr x3, [x0, #0x18]\n"
                     "ldr x2, [x0, #0x10]\n"
                     "ldr x1, [x0, #0x8]\n"
                     "ldr x0, [x0]\n"
                     "mov sp, x29\n"
                     "ldp x29, x30, [sp], #0x10\n"
                     "ret\n"
                     "1:\n"
                     "ldr q7, [sp, #0xc0]\n"
                     "ldr q6, [sp, #0xb0]\n"
                     "ldr q5, [sp, #0xa0]\n"
                     "ldr q4, [sp, #0x90]\n"
                     "ldr q3, [sp, #0x80]\n"
                     "ldr q2, [sp, #0x70]\n"
                     "ldr q1, [sp, #0x60]\n"
                     "ldr q0, [sp, #0x50]\n"
                     "ldr x8, [sp, #0x40]\n"
                     "ldr x7, [sp, #0x38]\n"
                     "ldr x6, [sp, #0x30]\n"
                     "ldr x5, [sp, #0x28]\n"
                     "ldr x4, [sp, #0x20]\n"
                     "ldr x3, [sp, #0x18]\n"
                     "ldr x2, [sp, #0x10]\n"
                     "ldr x1, [sp, #0x8]\n"
                     "ldr x0, [sp]\n"
                     "mov sp, x29\n"
                     "ldp x29, x30, [sp], #0x10\n"
                     "b _objc_msgSend\n");
}

// Native __invoking___, 0x1c028e7b0. Hidden (was a private external). The return frame is kept at the frame
// record - 16; frameSize - 0xE0 bytes of stack arguments from argumentFrame + 0xE0 are copied, eight at a
// time, to a 16-byte-aligned area below the frame; X0-X8 and Q0-Q7 are loaded from argumentFrame, the
// function is called, and its X0-X8 and Q0-Q7 are stored into the return frame. The flags (X4) are not read.
__attribute__((naked)) void __invoking___(
    void (*function)(void), void *returnFrame, void *argumentFrame, size_t frameSize, uint32_t flags
) {
    __asm__ volatile("stp x29, x30, [sp, #-0x10]!\n"
                     "mov x29, sp\n"
                     "sub sp, sp, #0x10\n"
                     "str x1, [sp]\n"
                     "sub x12, x3, #0xe0\n"
                     "add x11, x2, #0xe0\n"
                     "sub x10, sp, x12\n"
                     "and x10, x10, #0xfffffffffffffff0\n"
                     "mov sp, x10\n"
                     "b 2f\n"
                     "1:\n"
                     "ldr x9, [x11]\n"
                     "add x11, x11, #8\n"
                     "sub x12, x12, #8\n"
                     "str x9, [x10]\n"
                     "add x10, x10, #8\n"
                     "2:\n"
                     "cbnz x12, 1b\n"
                     "mov x11, x2\n"
                     "mov x10, x0\n"
                     "ldr q7, [x11, #0xc0]\n"
                     "ldr q6, [x11, #0xb0]\n"
                     "ldr q5, [x11, #0xa0]\n"
                     "ldr q4, [x11, #0x90]\n"
                     "ldr q3, [x11, #0x80]\n"
                     "ldr q2, [x11, #0x70]\n"
                     "ldr q1, [x11, #0x60]\n"
                     "ldr q0, [x11, #0x50]\n"
                     "ldr x8, [x11, #0x40]\n"
                     "ldr x7, [x11, #0x38]\n"
                     "ldr x6, [x11, #0x30]\n"
                     "ldr x5, [x11, #0x28]\n"
                     "ldr x4, [x11, #0x20]\n"
                     "ldr x3, [x11, #0x18]\n"
                     "ldr x2, [x11, #0x10]\n"
                     "ldr x1, [x11, #0x8]\n"
                     "ldr x0, [x11]\n"
                     "blr x10\n"
                     "mov x11, #-16\n"
                     "ldr x11, [x29, x11]\n"
                     "str q7, [x11, #0xc0]\n"
                     "str q6, [x11, #0xb0]\n"
                     "str q5, [x11, #0xa0]\n"
                     "str q4, [x11, #0x90]\n"
                     "str q3, [x11, #0x80]\n"
                     "str q2, [x11, #0x70]\n"
                     "str q1, [x11, #0x60]\n"
                     "str q0, [x11, #0x50]\n"
                     "str x8, [x11, #0x40]\n"
                     "str x7, [x11, #0x38]\n"
                     "str x6, [x11, #0x30]\n"
                     "str x5, [x11, #0x28]\n"
                     "str x4, [x11, #0x20]\n"
                     "str x3, [x11, #0x18]\n"
                     "str x2, [x11, #0x10]\n"
                     "str x1, [x11, #0x8]\n"
                     "str x0, [x11]\n"
                     "mov sp, x29\n"
                     "ldp x29, x30, [sp], #0x10\n"
                     "ret\n");
}

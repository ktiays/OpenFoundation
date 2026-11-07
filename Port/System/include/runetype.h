//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// The rune locale of Darwin's ctype (runetype.h) for the wasm port: the C locale's table for the first 256
// characters, with Darwin's class bits.

#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

#define _CTYPE_A 0x00000100L
#define _CTYPE_C 0x00000200L
#define _CTYPE_D 0x00000400L
#define _CTYPE_G 0x00000800L
#define _CTYPE_L 0x00001000L
#define _CTYPE_P 0x00002000L
#define _CTYPE_S 0x00004000L
#define _CTYPE_U 0x00008000L
#define _CTYPE_X 0x00010000L
#define _CTYPE_B 0x00020000L
#define _CTYPE_R 0x00040000L
#define _CTYPE_I 0x00080000L
#define _CTYPE_T 0x00100000L
#define _CTYPE_Q 0x00200000L

typedef struct {
    char __magic[8];
    char __encoding[32];
    void *__sgetrune;
    void *__sputrune;
    int32_t __invalid_rune;
    uint32_t __runetype[256];
    int32_t __maplower[256];
    int32_t __mapupper[256];
} _RuneLocale;

extern _RuneLocale _DefaultRuneLocale;

unsigned long __maskrune(int32_t character, unsigned long mask);

// Darwin's class tests of <_ctype.h>, which its <ctype.h> declares and the restored sources call.
static __inline int __istype(int32_t character, unsigned long mask) {
    return (character >= 0 && character < 0x80) ? !!(_DefaultRuneLocale.__runetype[character] & mask)
                                                : !!__maskrune(character, mask);
}

static __inline int32_t __isctype(int32_t character, unsigned long mask) {
    return (character < 0 || character >= 256) ? 0 : !!(_DefaultRuneLocale.__runetype[character] & mask);
}

__END_DECLS

//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// os_unfair_lock for the wasm port (Port/System/Source/OSLock.c): the lock word holds the owner's thread id
// shifted left by one, with bit 0 set while another thread waits; waiters block on the word with Emscripten's
// futex. The interface is the one the iOS SDK's <os/lock.h> and libplatform's private header declare.

#include <stdbool.h>
#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

typedef struct os_unfair_lock_s {
    uint32_t _os_unfair_lock_opaque;
} os_unfair_lock, *os_unfair_lock_t;

#define OS_UNFAIR_LOCK_INIT ((os_unfair_lock){0})

typedef uint32_t os_unfair_lock_options_t;

enum {
    OS_UNFAIR_LOCK_NONE = 0x00000000,
    OS_UNFAIR_LOCK_DATA_SYNCHRONIZATION = 0x00010000,
    OS_UNFAIR_LOCK_ADAPTIVE_SPIN = 0x00040000,
};

void os_unfair_lock_lock(os_unfair_lock_t lock);

bool os_unfair_lock_trylock(os_unfair_lock_t lock);

void os_unfair_lock_unlock(os_unfair_lock_t lock);

void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, os_unfair_lock_options_t options);

void os_unfair_lock_assert_owner(const os_unfair_lock *lock);

void os_unfair_lock_assert_not_owner(const os_unfair_lock *lock);

__END_DECLS

//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include <os/lock_private.h>

#include <emscripten/threading.h>
#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// The lock word: (owner thread id << 1) | waiters. Thread ids are positive and below 2^31.
static const uint32_t kWaitersBit = 1;

static uint32_t currentOwnerValue(void) {
    static _Thread_local uint32_t value;
    if (value == 0) {
        value = (uint32_t)gettid() << 1;
    }
    return value;
}

static _Atomic uint32_t *lockWord(os_unfair_lock_t lock) {
    return (_Atomic uint32_t *)&lock->_os_unfair_lock_opaque;
}

static void crash(const char *message) {
    fprintf(stderr, "BUG IN CLIENT OF LIBPLATFORM: %s\n", message);
    abort();
}

void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, os_unfair_lock_options_t options) {
    (void)options;
    _Atomic uint32_t *word = lockWord(lock);
    uint32_t self = currentOwnerValue();
    uint32_t expected = 0;
    if (atomic_compare_exchange_strong_explicit(
            word, &expected, self, memory_order_acquire, memory_order_relaxed
        )) {
        return;
    }
    // Once this thread has waited, it takes the lock with the waiters bit so that the next unlock wakes the
    // other waiters it cannot see.
    uint32_t next = self;
    for (;;) {
        if ((expected & ~kWaitersBit) == self) {
            crash("Trying to recursively lock an os_unfair_lock");
        }
        if (expected == 0) {
            if (atomic_compare_exchange_weak_explicit(
                    word, &expected, next, memory_order_acquire, memory_order_relaxed
                )) {
                return;
            }
            continue;
        }
        if ((expected & kWaitersBit) == 0) {
            uint32_t flagged = expected | kWaitersBit;
            if (!atomic_compare_exchange_weak_explicit(
                    word, &expected, flagged, memory_order_relaxed, memory_order_relaxed
                )) {
                continue;
            }
            expected = flagged;
        }
        emscripten_futex_wait((volatile void *)word, expected, INFINITY);
        next = self | kWaitersBit;
        expected = atomic_load_explicit(word, memory_order_relaxed);
    }
}

void os_unfair_lock_lock(os_unfair_lock_t lock) {
    os_unfair_lock_lock_with_options(lock, OS_UNFAIR_LOCK_NONE);
}

bool os_unfair_lock_trylock(os_unfair_lock_t lock) {
    uint32_t expected = 0;
    return atomic_compare_exchange_strong_explicit(
        lockWord(lock), &expected, currentOwnerValue(), memory_order_acquire, memory_order_relaxed
    );
}

void os_unfair_lock_unlock(os_unfair_lock_t lock) {
    _Atomic uint32_t *word = lockWord(lock);
    uint32_t previous = atomic_exchange_explicit(word, 0, memory_order_release);
    if ((previous & ~kWaitersBit) != currentOwnerValue()) {
        crash("Unlock of an os_unfair_lock not owned by current thread");
    }
    if ((previous & kWaitersBit) != 0) {
        emscripten_futex_wake((volatile void *)word, 1);
    }
}

void os_unfair_lock_assert_owner(const os_unfair_lock *lock) {
    uint32_t value =
        atomic_load_explicit((_Atomic uint32_t *)&lock->_os_unfair_lock_opaque, memory_order_relaxed);
    if ((value & ~kWaitersBit) != currentOwnerValue()) {
        crash("Assertion failed: Lock unexpectedly not owned by current thread");
    }
}

void os_unfair_lock_assert_not_owner(const os_unfair_lock *lock) {
    uint32_t value =
        atomic_load_explicit((_Atomic uint32_t *)&lock->_os_unfair_lock_opaque, memory_order_relaxed);
    if ((value & ~kWaitersBit) == currentOwnerValue()) {
        crash("Assertion failed: Lock unexpectedly owned by current thread");
    }
}

static bool ownsLock(os_unfair_recursive_lock_t lock) {
    uint32_t value = atomic_load_explicit(lockWord(&lock->ourl_lock), memory_order_relaxed);
    return (value & ~kWaitersBit) == currentOwnerValue();
}

void os_unfair_recursive_lock_lock_with_options(
    os_unfair_recursive_lock_t lock, os_unfair_lock_options_t options
) {
    if (ownsLock(lock)) {
        lock->ourl_count++;
        return;
    }
    os_unfair_lock_lock_with_options(&lock->ourl_lock, options);
}

bool os_unfair_recursive_lock_trylock(os_unfair_recursive_lock_t lock) {
    if (ownsLock(lock)) {
        lock->ourl_count++;
        return true;
    }
    return os_unfair_lock_trylock(&lock->ourl_lock);
}

void os_unfair_recursive_lock_unlock(os_unfair_recursive_lock_t lock) {
    if (lock->ourl_count != 0) {
        if (!ownsLock(lock)) {
            crash("Unlock of an os_unfair_recursive_lock not owned by current thread");
        }
        lock->ourl_count--;
        return;
    }
    os_unfair_lock_unlock(&lock->ourl_lock);
}

bool os_unfair_recursive_lock_tryunlock4objc(os_unfair_recursive_lock_t lock) {
    if (!ownsLock(lock)) {
        return false;
    }
    os_unfair_recursive_lock_unlock(lock);
    return true;
}

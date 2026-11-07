//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// libplatform's private os_unfair_recursive_lock (Port/System/Source/OSLock.c). The public <os/lock.h> does
// not declare it, as on Darwin, so that sources declaring it themselves keep their own declarations.

#include <os/lock.h>

__BEGIN_DECLS

typedef struct os_unfair_recursive_lock_s {
    os_unfair_lock ourl_lock;
    uint32_t ourl_count;
} os_unfair_recursive_lock, *os_unfair_recursive_lock_t;

#define OS_UNFAIR_RECURSIVE_LOCK_INIT ((os_unfair_recursive_lock){OS_UNFAIR_LOCK_INIT, 0})

void os_unfair_recursive_lock_lock_with_options(
    os_unfair_recursive_lock_t lock, os_unfair_lock_options_t options
);

bool os_unfair_recursive_lock_trylock(os_unfair_recursive_lock_t lock);

void os_unfair_recursive_lock_unlock(os_unfair_recursive_lock_t lock);

bool os_unfair_recursive_lock_tryunlock4objc(os_unfair_recursive_lock_t lock);

#define os_unfair_recursive_lock_lock(lock) os_unfair_recursive_lock_lock_with_options((lock), 0)

__END_DECLS

//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// os_activity for the wasm port: activities exist as values, and scopes record nothing.

#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

typedef struct os_activity_s *os_activity_t;

typedef enum {
    OS_ACTIVITY_FLAG_DEFAULT = 0,
    OS_ACTIVITY_FLAG_DETACHED = 0x1,
    OS_ACTIVITY_FLAG_IF_NONE_PRESENT = 0x2,
} os_activity_flag_t;

struct os_activity_scope_state_s {
    uint64_t opaque[2];
};
typedef struct os_activity_scope_state_s *os_activity_scope_state_t;

extern struct os_activity_s _os_activity_current;
extern struct os_activity_s _os_activity_none;
#define OS_ACTIVITY_CURRENT (&_os_activity_current)
#define OS_ACTIVITY_NONE (&_os_activity_none)

os_activity_t
_os_activity_create(void *dso, const char *description, os_activity_t parent, os_activity_flag_t flags);
#define os_activity_create(description, parent, flags)                                                       \
    _os_activity_create(0, (description), (parent), (flags))

void os_activity_scope_enter(os_activity_t activity, os_activity_scope_state_t state);
void os_activity_scope_leave(os_activity_scope_state_t state);

__END_DECLS

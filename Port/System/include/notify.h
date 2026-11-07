//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// notify(3) for the wasm port (Port/System/Source/Notify.c): Darwin notifications within the process. A post
// runs every handler registered for its name on the handler's queue; each name keeps a 64-bit state.

#include <dispatch/dispatch.h>
#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

#define NOTIFY_STATUS_OK 0
#define NOTIFY_STATUS_INVALID_NAME 1
#define NOTIFY_STATUS_INVALID_TOKEN 2
#define NOTIFY_STATUS_INVALID_PORT 3
#define NOTIFY_STATUS_INVALID_FILE 4
#define NOTIFY_STATUS_INVALID_SIGNAL 5
#define NOTIFY_STATUS_INVALID_REQUEST 6
#define NOTIFY_STATUS_NOT_AUTHORIZED 7
#define NOTIFY_STATUS_FAILED 1000000

#define NOTIFY_REUSE 0x00000001

typedef void (^notify_handler_t)(int token);

uint32_t notify_post(const char *name);
uint32_t
notify_register_dispatch(const char *name, int *outToken, dispatch_queue_t queue, notify_handler_t handler);
uint32_t notify_register_check(const char *name, int *outToken);
uint32_t notify_check(int token, int *check);
uint32_t notify_cancel(int token);
uint32_t notify_get_state(int token, uint64_t *state);
uint32_t notify_set_state(int token, uint64_t state);
uint32_t notify_is_valid_token(int token);

__END_DECLS

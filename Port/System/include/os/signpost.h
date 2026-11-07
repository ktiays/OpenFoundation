//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// os_signpost for the wasm port: signposts are never enabled, and the emitting macros evaluate nothing.

#include <os/log.h>

typedef uint64_t os_signpost_id_t;

typedef enum : uint8_t {
    OS_SIGNPOST_EVENT = 0x00,
    OS_SIGNPOST_INTERVAL_BEGIN = 0x01,
    OS_SIGNPOST_INTERVAL_END = 0x02,
} os_signpost_type_t;

#define OS_SIGNPOST_ID_NULL ((os_signpost_id_t)0)
#define OS_SIGNPOST_ID_INVALID ((os_signpost_id_t)~0)
#define OS_SIGNPOST_ID_EXCLUSIVE ((os_signpost_id_t)0xEEEEB0B5B2B2EEEE)

#define os_signpost_enabled(log) ((void)(log), false)
#define os_signpost_id_make_with_pointer(log, pointer) ((void)(log), (os_signpost_id_t)(uintptr_t)(pointer))
#define os_signpost_id_generate(log) ((void)(log), (os_signpost_id_t)1)
#define os_signpost_event_emit(log, ...) ((void)0)
#define os_signpost_interval_begin(log, ...) ((void)0)
#define os_signpost_interval_end(log, ...) ((void)0)
#define os_signpost_animation_interval_begin(log, ...) ((void)0)
#define os_signpost_id_make_with_id(log, id) ((void)(log), (os_signpost_id_t)(id))
#define _os_signpost_emit_with_type(emitFunction, log, type, spid, name, format, ...) ((void)0)
#define os_signpost_emit_with_type(log, type, spid, name, ...) ((void)0)

__BEGIN_DECLS

// libsystem_trace's emitting entry points, which the restored sources name directly; never reached through
// the macros above, and they record nothing.
void _os_signpost_emit_with_name_impl(
    void *dso,
    os_log_t log,
    os_signpost_type_t type,
    os_signpost_id_t spid,
    const char *name,
    const char *format,
    uint8_t *buffer,
    uint32_t size
);
void _os_signpost_emit_unreliably_with_name_impl(
    void *dso,
    os_log_t log,
    os_signpost_type_t type,
    os_signpost_id_t spid,
    const char *name,
    const char *format,
    uint8_t *buffer,
    uint32_t size
);

__END_DECLS

//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// os_signpost for the wasm port: signposts are never enabled, and the emitting macros evaluate nothing.

#include <os/log.h>

typedef uint64_t os_signpost_id_t;

#define OS_SIGNPOST_ID_NULL ((os_signpost_id_t)0)
#define OS_SIGNPOST_ID_INVALID ((os_signpost_id_t)~0)
#define OS_SIGNPOST_ID_EXCLUSIVE ((os_signpost_id_t)0xEEEEB0B5B2B2EEEE)

#define os_signpost_enabled(log) ((void)(log), false)
#define os_signpost_id_make_with_pointer(log, pointer) ((void)(log), (os_signpost_id_t)(uintptr_t)(pointer))
#define os_signpost_id_generate(log) ((void)(log), (os_signpost_id_t)1)
#define os_signpost_event_emit(log, ...) ((void)0)
#define os_signpost_interval_begin(log, ...) ((void)0)
#define os_signpost_interval_end(log, ...) ((void)0)

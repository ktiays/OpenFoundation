//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// os_log for the wasm port (Port/System/Source/OSLog.c). The interface is the iOS SDK's; the macros neither
// evaluate nor encode their arguments. Error and fault messages print their format string to stderr; other
// types are disabled. The libsystem_trace SPI the restored sources call directly (the _impl entry points and
// the os_log packs exceptions carry) is implemented too.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

typedef struct os_log_s *os_log_t;
typedef uint8_t os_log_type_t;

enum {
    OS_LOG_TYPE_DEFAULT = 0x00,
    OS_LOG_TYPE_INFO = 0x01,
    OS_LOG_TYPE_DEBUG = 0x02,
    OS_LOG_TYPE_ERROR = 0x10,
    OS_LOG_TYPE_FAULT = 0x11,
};

extern struct os_log_s _os_log_default;
extern struct os_log_s _os_log_disabled;
#define OS_LOG_DEFAULT (&_os_log_default)
#define OS_LOG_DISABLED (&_os_log_disabled)

os_log_t os_log_create(const char *subsystem, const char *category);

bool os_log_type_enabled(os_log_t log, os_log_type_t type);

void _os_log_impl(
    void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buffer, uint32_t size
);

void _os_log_debug_impl(
    void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buffer, uint32_t size
);

void _os_log_error_impl(
    void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buffer, uint32_t size
);

void _os_log_fault_impl(
    void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buffer, uint32_t size
);

void _os_log_port_message(os_log_t log, os_log_type_t type, const char *format);

#define os_log_with_type(log, type, format, ...) _os_log_port_message((log), (type), (format))
#define os_log(log, format, ...) os_log_with_type((log), OS_LOG_TYPE_DEFAULT, (format))
#define os_log_info(log, format, ...) os_log_with_type((log), OS_LOG_TYPE_INFO, (format))
#define os_log_debug(log, format, ...) os_log_with_type((log), OS_LOG_TYPE_DEBUG, (format))
#define os_log_error(log, format, ...) os_log_with_type((log), OS_LOG_TYPE_ERROR, (format))
#define os_log_fault(log, format, ...) os_log_with_type((log), OS_LOG_TYPE_FAULT, (format))
#define os_log_info_enabled(log) os_log_type_enabled((log), OS_LOG_TYPE_INFO)
#define os_log_debug_enabled(log) os_log_type_enabled((log), OS_LOG_TYPE_DEBUG)

__END_DECLS

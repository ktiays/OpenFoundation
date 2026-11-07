//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// libsystem_trace and libplatform answers for the wasm port: os_log, os_activity, the os_log packs and the
// feature and variant queries.

#include <os/activity.h>
#include <os/log.h>
#include <os/signpost.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct os_log_s {
    const char *subsystem;
    const char *category;
    struct os_log_s *next;
};

struct os_activity_s {
    const char *description;
};

struct os_log_s _os_log_default = {"", "", NULL};
struct os_log_s _os_log_disabled = {"", "", NULL};
struct os_activity_s _os_activity_current = {""};
struct os_activity_s _os_activity_none = {""};

os_log_t os_log_create(const char *subsystem, const char *category) {
    os_log_t log = calloc(1, sizeof(struct os_log_s));
    log->subsystem = strdup(subsystem != NULL ? subsystem : "");
    log->category = strdup(category != NULL ? category : "");
    return log;
}

// Errors and faults are on; the other types are off. OF_PORT_LOG=all turns every type on.
bool os_log_type_enabled(os_log_t log, os_log_type_t type) {
    if (log == OS_LOG_DISABLED) {
        return false;
    }
    if (type == OS_LOG_TYPE_ERROR || type == OS_LOG_TYPE_FAULT) {
        return true;
    }
    const char *setting = getenv("OF_PORT_LOG");
    return setting != NULL && strcmp(setting, "all") == 0;
}

void _os_log_port_message(os_log_t log, os_log_type_t type, const char *format) {
    if (!os_log_type_enabled(log, type)) {
        return;
    }
    const char *subsystem = log != NULL && log->subsystem[0] != '\0' ? log->subsystem : "os_log";
    fprintf(stderr, "[%s] %s\n", subsystem, format);
}

static void logEncoded(os_log_t log, os_log_type_t type, const char *format) {
    _os_log_port_message(log, type, format);
}

void _os_log_impl(
    void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buffer, uint32_t size
) {
    (void)dso;
    (void)buffer;
    (void)size;
    logEncoded(log, type, format);
}

void _os_log_debug_impl(
    void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buffer, uint32_t size
) {
    _os_log_impl(dso, log, type, format, buffer, size);
}

void _os_log_error_impl(
    void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buffer, uint32_t size
) {
    _os_log_impl(dso, log, type, format, buffer, size);
}

void _os_log_fault_impl(
    void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buffer, uint32_t size
) {
    _os_log_impl(dso, log, type, format, buffer, size);
}

// An os_log pack: the libsystem_trace header, then the compiler-encoded argument buffer.
typedef struct {
    uint64_t continuousTime;
    struct timespec wallTime;
    const void *imageHeader;
    const void *returnAddress;
    const char *format;
    uint8_t data[];
} LogPack;

size_t _os_log_pack_size(size_t formatBufferSize) {
    return sizeof(LogPack) + formatBufferSize;
}

uint8_t *_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format) {
    (void)size;
    (void)savedErrno;
    LogPack *header = pack;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    header->continuousTime = (uint64_t)now.tv_sec * 1000000000ULL + (uint64_t)now.tv_nsec;
    clock_gettime(CLOCK_REALTIME, &header->wallTime);
    header->imageHeader = dso;
    header->returnAddress = __builtin_return_address(0);
    header->format = format;
    return header->data;
}

static void *gNSCFFormatter;

void _os_log_set_nscf_formatter(void *formatter) {
    gNSCFFormatter = formatter;
}

os_activity_t
_os_activity_create(void *dso, const char *description, os_activity_t parent, os_activity_flag_t flags) {
    (void)dso;
    (void)parent;
    (void)flags;
    os_activity_t activity = calloc(1, sizeof(struct os_activity_s));
    activity->description = description;
    return activity;
}

void os_activity_scope_enter(os_activity_t activity, os_activity_scope_state_t state) {
    (void)activity;
    memset(state, 0, sizeof(*state));
}

void os_activity_scope_leave(os_activity_scope_state_t state) {
    (void)state;
}

// Every feature flag is off on the port: the Swift-implemented Foundation paths they select are not built
// (Port/Trim.tsv), and the native code's own path is taken.
bool _os_feature_enabled_impl(const char *domain, const char *feature) {
    (void)domain;
    (void)feature;
    return false;
}

bool os_variant_has_internal_diagnostics(const char *subsystem) {
    (void)subsystem;
    return false;
}

bool os_variant_has_internal_content(const char *subsystem) {
    (void)subsystem;
    return false;
}

bool os_variant_allows_internal_security_policies(const char *subsystem) {
    (void)subsystem;
    return false;
}

void _os_signpost_emit_with_name_impl(
    void *dso,
    os_log_t log,
    os_signpost_type_t type,
    os_signpost_id_t spid,
    const char *name,
    const char *format,
    uint8_t *buffer,
    uint32_t size
) {
    (void)dso;
    (void)log;
    (void)type;
    (void)spid;
    (void)name;
    (void)format;
    (void)buffer;
    (void)size;
}

void _os_signpost_emit_unreliably_with_name_impl(
    void *dso,
    os_log_t log,
    os_signpost_type_t type,
    os_signpost_id_t spid,
    const char *name,
    const char *format,
    uint8_t *buffer,
    uint32_t size
) {
    _os_signpost_emit_with_name_impl(dso, log, type, spid, name, format, buffer, size);
}

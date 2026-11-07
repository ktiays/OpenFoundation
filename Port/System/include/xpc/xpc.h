//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// XPC for the wasm port (Port/System/Source/Xpc.c). There is no other process to talk to: value objects
// (dictionaries, arrays, strings, integers, data, null, Mach send rights) work; connections exist, deliver
// nothing and never call their event handlers; entitlement and bootstrap queries answer NULL, and user
// sessions are disabled.

#include <dispatch/dispatch.h>
#include <mach/mach.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <uuid/uuid.h>

__BEGIN_DECLS

typedef void *xpc_object_t;
typedef xpc_object_t xpc_connection_t;
typedef xpc_object_t xpc_endpoint_t;
typedef const struct _xpc_type_s *xpc_type_t;
typedef void (^xpc_handler_t)(xpc_object_t object);
typedef bool (^xpc_array_applier_t)(size_t index, xpc_object_t value);
typedef bool (^xpc_dictionary_applier_t)(const char *key, xpc_object_t value);

extern const struct _xpc_type_s _xpc_type_dictionary;
extern const struct _xpc_type_s _xpc_type_array;
extern const struct _xpc_type_s _xpc_type_string;
extern const struct _xpc_type_s _xpc_type_uint64;
extern const struct _xpc_type_s _xpc_type_int64;
extern const struct _xpc_type_s _xpc_type_bool;
extern const struct _xpc_type_s _xpc_type_data;
extern const struct _xpc_type_s _xpc_type_null;
extern const struct _xpc_type_s _xpc_type_error;
extern const struct _xpc_type_s _xpc_type_connection;
extern const struct _xpc_type_s _xpc_type_endpoint;
extern const struct _xpc_type_s _xpc_type_mach_send;
#define XPC_TYPE_DICTIONARY (&_xpc_type_dictionary)
#define XPC_TYPE_ARRAY (&_xpc_type_array)
#define XPC_TYPE_STRING (&_xpc_type_string)
#define XPC_TYPE_UINT64 (&_xpc_type_uint64)
#define XPC_TYPE_INT64 (&_xpc_type_int64)
#define XPC_TYPE_BOOL (&_xpc_type_bool)
#define XPC_TYPE_DATA (&_xpc_type_data)
#define XPC_TYPE_NULL (&_xpc_type_null)
#define XPC_TYPE_ERROR (&_xpc_type_error)
#define XPC_TYPE_CONNECTION (&_xpc_type_connection)
#define XPC_TYPE_ENDPOINT (&_xpc_type_endpoint)
#define XPC_TYPE_MACH_SEND (&_xpc_type_mach_send)

extern void *_xpc_error_connection_interrupted;
extern void *_xpc_error_connection_invalid;
extern void *_xpc_error_termination_imminent;
#define XPC_ERROR_CONNECTION_INTERRUPTED ((xpc_object_t) & _xpc_error_connection_interrupted)
#define XPC_ERROR_CONNECTION_INVALID ((xpc_object_t) & _xpc_error_connection_invalid)
#define XPC_ERROR_TERMINATION_IMMINENT ((xpc_object_t) & _xpc_error_termination_imminent)

#define XPC_ARRAY_APPEND ((size_t)(-1))
#define XPC_CONNECTION_MACH_SERVICE_LISTENER (1 << 0)
#define XPC_CONNECTION_MACH_SERVICE_PRIVILEGED (1 << 1)

xpc_object_t xpc_retain(xpc_object_t object);
void xpc_release(xpc_object_t object);
xpc_type_t xpc_get_type(xpc_object_t object);
char *xpc_copy_description(xpc_object_t object);

xpc_object_t xpc_dictionary_create(const char *const *keys, const xpc_object_t *values, size_t count);
void xpc_dictionary_set_value(xpc_object_t dictionary, const char *key, xpc_object_t value);
xpc_object_t xpc_dictionary_get_value(xpc_object_t dictionary, const char *key);
void xpc_dictionary_set_string(xpc_object_t dictionary, const char *key, const char *string);
const char *xpc_dictionary_get_string(xpc_object_t dictionary, const char *key);
void xpc_dictionary_set_uint64(xpc_object_t dictionary, const char *key, uint64_t value);
uint64_t xpc_dictionary_get_uint64(xpc_object_t dictionary, const char *key);
size_t xpc_dictionary_get_count(xpc_object_t dictionary);
bool xpc_dictionary_apply(xpc_object_t dictionary, xpc_dictionary_applier_t applier);
void xpc_dictionary_set_data(xpc_object_t dictionary, const char *key, const void *bytes, size_t length);
const void *xpc_dictionary_get_data(xpc_object_t dictionary, const char *key, size_t *length);
// A send right in a dictionary: setting copies the caller's right into the value, copying answers a new send
// right (the caller owns it) or MACH_PORT_NULL when the key holds none.
void xpc_dictionary_set_mach_send(xpc_object_t dictionary, const char *key, mach_port_t port);
mach_port_t xpc_dictionary_copy_mach_send(xpc_object_t dictionary, const char *key);

xpc_object_t xpc_array_create(const xpc_object_t *objects, size_t count);
void xpc_array_append_value(xpc_object_t array, xpc_object_t value);
void xpc_array_set_value(xpc_object_t array, size_t index, xpc_object_t value);
xpc_object_t xpc_array_get_value(xpc_object_t array, size_t index);
size_t xpc_array_get_count(xpc_object_t array);
const char *xpc_array_get_string(xpc_object_t array, size_t index);
void xpc_array_set_uint64(xpc_object_t array, size_t index, uint64_t value);
bool xpc_array_apply(xpc_object_t array, xpc_array_applier_t applier);

xpc_object_t xpc_string_create(const char *string);
const char *xpc_string_get_string_ptr(xpc_object_t string);
size_t xpc_string_get_length(xpc_object_t string);
xpc_object_t xpc_uint64_create(uint64_t value);
uint64_t xpc_uint64_get_value(xpc_object_t value);
xpc_object_t xpc_null_create(void);
xpc_object_t xpc_data_create(const void *bytes, size_t length);
xpc_object_t xpc_data_create_with_dispatch_data(dispatch_data_t data);
const void *xpc_data_get_bytes_ptr(xpc_object_t data);
size_t xpc_data_get_length(xpc_object_t data);

// Mach send rights as values (libxpc SPI): the object owns one send right and deallocates it when freed.
// create copies the caller's right; create_with_disposition takes it as the disposition says (MOVE_SEND
// consumes the caller's right, COPY_SEND copies it, MAKE_SEND makes one from a receive right).
xpc_object_t xpc_mach_send_create(mach_port_t port);
xpc_object_t xpc_mach_send_create_with_disposition(mach_port_t port, unsigned int disposition);
mach_port_t xpc_mach_send_copy_right(xpc_object_t object);
mach_port_t xpc_mach_send_get_right(xpc_object_t object);

// User sessions (libxpc SPI): the port runs one session, so they are disabled.
int xpc_user_sessions_enabled(void);
uid_t xpc_user_sessions_get_foreground_uid(int *error);
const char *xpc_strerror(int error);

xpc_connection_t
xpc_connection_create_mach_service(const char *name, dispatch_queue_t targetQueue, uint64_t flags);
xpc_connection_t
_xpc_connection_create_mach_service(const char *name, dispatch_queue_t targetQueue, uint64_t flags);
xpc_connection_t xpc_connection_create_from_endpoint(xpc_endpoint_t endpoint);
void xpc_connection_set_target_queue(xpc_connection_t connection, dispatch_queue_t targetQueue);
void xpc_connection_set_event_handler(xpc_connection_t connection, xpc_handler_t handler);
void xpc_connection_activate(xpc_connection_t connection);
void xpc_connection_resume(xpc_connection_t connection);
void xpc_connection_cancel(xpc_connection_t connection);
void xpc_connection_send_message(xpc_connection_t connection, xpc_object_t message);
void xpc_connection_send_barrier(xpc_connection_t connection, dispatch_block_t barrier);

xpc_object_t xpc_copy_entitlement_for_token(const char *entitlement, audit_token_t *auditToken);
xpc_object_t xpc_copy_bootstrap(void);

__END_DECLS

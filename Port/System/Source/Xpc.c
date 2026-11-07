//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// XPC values and inert connections for the wasm port (Port/System/include/xpc/xpc.h).

#include <xpc/xpc.h>

#include <Block.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct _xpc_type_s {
    const char *name;
};

const struct _xpc_type_s _xpc_type_dictionary = {"dictionary"};
const struct _xpc_type_s _xpc_type_array = {"array"};
const struct _xpc_type_s _xpc_type_string = {"string"};
const struct _xpc_type_s _xpc_type_uint64 = {"uint64"};
const struct _xpc_type_s _xpc_type_int64 = {"int64"};
const struct _xpc_type_s _xpc_type_bool = {"bool"};
const struct _xpc_type_s _xpc_type_data = {"data"};
const struct _xpc_type_s _xpc_type_null = {"null"};
const struct _xpc_type_s _xpc_type_error = {"error"};
const struct _xpc_type_s _xpc_type_connection = {"connection"};
const struct _xpc_type_s _xpc_type_endpoint = {"endpoint"};
const struct _xpc_type_s _xpc_type_mach_send = {"mach_send"};

typedef struct XpcObject {
    xpc_type_t type;
    _Atomic int referenceCount;
    // Dictionary keys, or nothing.
    char **keys;
    // Dictionary and array values.
    struct XpcObject **values;
    size_t count;
    size_t capacity;
    // Strings and data.
    char *bytes;
    size_t length;
    uint64_t integer;
    xpc_handler_t handler;
    // A Mach send value's right.
    mach_port_t port;
} XpcObject;

// The error objects are immortal and never compared by content.
void *_xpc_error_connection_interrupted;
void *_xpc_error_connection_invalid;
void *_xpc_error_termination_imminent;

static bool isError(xpc_object_t object) {
    return object == XPC_ERROR_CONNECTION_INTERRUPTED || object == XPC_ERROR_CONNECTION_INVALID ||
           object == XPC_ERROR_TERMINATION_IMMINENT;
}

static XpcObject *createObject(xpc_type_t type) {
    XpcObject *object = calloc(1, sizeof(XpcObject));
    object->type = type;
    object->referenceCount = 1;
    return object;
}

xpc_object_t xpc_retain(xpc_object_t object) {
    if (object != NULL && !isError(object)) {
        atomic_fetch_add(&((XpcObject *)object)->referenceCount, 1);
    }
    return object;
}

void xpc_release(xpc_object_t value) {
    if (value == NULL || isError(value)) {
        return;
    }
    XpcObject *object = value;
    if (atomic_fetch_sub(&object->referenceCount, 1) != 1) {
        return;
    }
    for (size_t index = 0; index < object->count; index++) {
        if (object->keys != NULL) {
            free(object->keys[index]);
        }
        xpc_release(object->values[index]);
    }
    free(object->keys);
    free(object->values);
    free(object->bytes);
    if (object->handler != NULL) {
        Block_release(object->handler);
    }
    if (object->type == XPC_TYPE_MACH_SEND && MACH_PORT_VALID(object->port)) {
        mach_port_deallocate(mach_task_self(), object->port);
    }
    free(object);
}

xpc_type_t xpc_get_type(xpc_object_t object) {
    if (isError(object)) {
        return XPC_TYPE_ERROR;
    }
    return ((XpcObject *)object)->type;
}

char *xpc_copy_description(xpc_object_t object) {
    char *description = NULL;
    asprintf(&description, "<xpc %s %p>", xpc_get_type(object)->name, object);
    return description;
}

static void appendValue(XpcObject *container, char *key, xpc_object_t value) {
    if (container->count == container->capacity) {
        container->capacity = container->capacity == 0 ? 4 : container->capacity * 2;
        container->values = realloc(container->values, container->capacity * sizeof(XpcObject *));
        if (container->type == XPC_TYPE_DICTIONARY) {
            container->keys = realloc(container->keys, container->capacity * sizeof(char *));
        }
    }
    if (key != NULL) {
        container->keys[container->count] = key;
    }
    container->values[container->count++] = xpc_retain(value);
}

xpc_object_t xpc_dictionary_create(const char *const *keys, const xpc_object_t *values, size_t count) {
    XpcObject *dictionary = createObject(XPC_TYPE_DICTIONARY);
    for (size_t index = 0; index < count; index++) {
        xpc_dictionary_set_value(dictionary, keys[index], values[index]);
    }
    return dictionary;
}

static ptrdiff_t dictionaryIndex(XpcObject *dictionary, const char *key) {
    for (size_t index = 0; index < dictionary->count; index++) {
        if (strcmp(dictionary->keys[index], key) == 0) {
            return (ptrdiff_t)index;
        }
    }
    return -1;
}

void xpc_dictionary_set_value(xpc_object_t value, const char *key, xpc_object_t item) {
    XpcObject *dictionary = value;
    ptrdiff_t index = dictionaryIndex(dictionary, key);
    if (index >= 0) {
        xpc_release(dictionary->values[index]);
        if (item == NULL) {
            free(dictionary->keys[index]);
            dictionary->count--;
            memmove(
                &dictionary->keys[index],
                &dictionary->keys[index + 1],
                (dictionary->count - (size_t)index) * sizeof(char *)
            );
            memmove(
                &dictionary->values[index],
                &dictionary->values[index + 1],
                (dictionary->count - (size_t)index) * sizeof(XpcObject *)
            );
        } else {
            dictionary->values[index] = xpc_retain(item);
        }
        return;
    }
    if (item != NULL) {
        appendValue(dictionary, strdup(key), item);
    }
}

xpc_object_t xpc_dictionary_get_value(xpc_object_t value, const char *key) {
    XpcObject *dictionary = value;
    ptrdiff_t index = dictionaryIndex(dictionary, key);
    return index >= 0 ? dictionary->values[index] : NULL;
}

void xpc_dictionary_set_string(xpc_object_t dictionary, const char *key, const char *string) {
    xpc_object_t item = xpc_string_create(string);
    xpc_dictionary_set_value(dictionary, key, item);
    xpc_release(item);
}

const char *xpc_dictionary_get_string(xpc_object_t dictionary, const char *key) {
    xpc_object_t item = xpc_dictionary_get_value(dictionary, key);
    return item != NULL && xpc_get_type(item) == XPC_TYPE_STRING ? xpc_string_get_string_ptr(item) : NULL;
}

void xpc_dictionary_set_uint64(xpc_object_t dictionary, const char *key, uint64_t value) {
    xpc_object_t item = xpc_uint64_create(value);
    xpc_dictionary_set_value(dictionary, key, item);
    xpc_release(item);
}

uint64_t xpc_dictionary_get_uint64(xpc_object_t dictionary, const char *key) {
    xpc_object_t item = xpc_dictionary_get_value(dictionary, key);
    return item != NULL && xpc_get_type(item) == XPC_TYPE_UINT64 ? xpc_uint64_get_value(item) : 0;
}

size_t xpc_dictionary_get_count(xpc_object_t dictionary) {
    return ((XpcObject *)dictionary)->count;
}

bool xpc_dictionary_apply(xpc_object_t value, xpc_dictionary_applier_t applier) {
    XpcObject *dictionary = value;
    for (size_t index = 0; index < dictionary->count; index++) {
        if (!applier(dictionary->keys[index], dictionary->values[index])) {
            return false;
        }
    }
    return true;
}

xpc_object_t xpc_array_create(const xpc_object_t *objects, size_t count) {
    XpcObject *array = createObject(XPC_TYPE_ARRAY);
    for (size_t index = 0; index < count; index++) {
        appendValue(array, NULL, objects[index]);
    }
    return array;
}

void xpc_array_append_value(xpc_object_t array, xpc_object_t value) {
    appendValue(array, NULL, value);
}

void xpc_array_set_value(xpc_object_t value, size_t index, xpc_object_t item) {
    XpcObject *array = value;
    if (index == XPC_ARRAY_APPEND || index == array->count) {
        appendValue(array, NULL, item);
        return;
    }
    if (index < array->count) {
        xpc_object_t old = array->values[index];
        array->values[index] = xpc_retain(item);
        xpc_release(old);
    }
}

xpc_object_t xpc_array_get_value(xpc_object_t value, size_t index) {
    XpcObject *array = value;
    return index < array->count ? array->values[index] : NULL;
}

size_t xpc_array_get_count(xpc_object_t array) {
    return ((XpcObject *)array)->count;
}

const char *xpc_array_get_string(xpc_object_t array, size_t index) {
    xpc_object_t item = xpc_array_get_value(array, index);
    return item != NULL && xpc_get_type(item) == XPC_TYPE_STRING ? xpc_string_get_string_ptr(item) : NULL;
}

void xpc_array_set_uint64(xpc_object_t array, size_t index, uint64_t value) {
    xpc_object_t item = xpc_uint64_create(value);
    xpc_array_set_value(array, index, item);
    xpc_release(item);
}

bool xpc_array_apply(xpc_object_t value, xpc_array_applier_t applier) {
    XpcObject *array = value;
    for (size_t index = 0; index < array->count; index++) {
        if (!applier(index, array->values[index])) {
            return false;
        }
    }
    return true;
}

xpc_object_t xpc_string_create(const char *string) {
    XpcObject *object = createObject(XPC_TYPE_STRING);
    object->length = strlen(string);
    object->bytes = strdup(string);
    return object;
}

const char *xpc_string_get_string_ptr(xpc_object_t string) {
    return ((XpcObject *)string)->bytes;
}

size_t xpc_string_get_length(xpc_object_t string) {
    return ((XpcObject *)string)->length;
}

xpc_object_t xpc_uint64_create(uint64_t value) {
    XpcObject *object = createObject(XPC_TYPE_UINT64);
    object->integer = value;
    return object;
}

uint64_t xpc_uint64_get_value(xpc_object_t value) {
    return ((XpcObject *)value)->integer;
}

xpc_object_t xpc_null_create(void) {
    return createObject(XPC_TYPE_NULL);
}

xpc_object_t xpc_data_create(const void *bytes, size_t length) {
    XpcObject *object = createObject(XPC_TYPE_DATA);
    object->bytes = malloc(length == 0 ? 1 : length);
    if (length > 0) {
        memcpy(object->bytes, bytes, length);
    }
    object->length = length;
    return object;
}

xpc_object_t xpc_data_create_with_dispatch_data(dispatch_data_t data) {
    const void *bytes = NULL;
    size_t length = 0;
    dispatch_data_t contiguous = dispatch_data_create_map(data, &bytes, &length);
    xpc_object_t object = xpc_data_create(bytes, length);
    dispatch_release(contiguous);
    return object;
}

const void *xpc_data_get_bytes_ptr(xpc_object_t data) {
    return ((XpcObject *)data)->bytes;
}

size_t xpc_data_get_length(xpc_object_t data) {
    return ((XpcObject *)data)->length;
}

void xpc_dictionary_set_data(xpc_object_t dictionary, const char *key, const void *bytes, size_t length) {
    xpc_object_t item = xpc_data_create(bytes, length);
    xpc_dictionary_set_value(dictionary, key, item);
    xpc_release(item);
}

const void *xpc_dictionary_get_data(xpc_object_t dictionary, const char *key, size_t *length) {
    xpc_object_t item = xpc_dictionary_get_value(dictionary, key);
    if (item == NULL || xpc_get_type(item) != XPC_TYPE_DATA) {
        if (length != NULL) {
            *length = 0;
        }
        return NULL;
    }
    if (length != NULL) {
        *length = xpc_data_get_length(item);
    }
    return xpc_data_get_bytes_ptr(item);
}

void xpc_dictionary_set_mach_send(xpc_object_t dictionary, const char *key, mach_port_t port) {
    xpc_object_t item = xpc_mach_send_create(port);
    xpc_dictionary_set_value(dictionary, key, item);
    xpc_release(item);
}

mach_port_t xpc_dictionary_copy_mach_send(xpc_object_t dictionary, const char *key) {
    xpc_object_t item = xpc_dictionary_get_value(dictionary, key);
    if (item == NULL || xpc_get_type(item) != XPC_TYPE_MACH_SEND) {
        return MACH_PORT_NULL;
    }
    return xpc_mach_send_copy_right(item);
}

xpc_object_t xpc_mach_send_create(mach_port_t port) {
    return xpc_mach_send_create_with_disposition(port, MACH_MSG_TYPE_COPY_SEND);
}

xpc_object_t xpc_mach_send_create_with_disposition(mach_port_t port, unsigned int disposition) {
    if (MACH_PORT_VALID(port)) {
        kern_return_t result = KERN_SUCCESS;
        switch (disposition) {
            case MACH_MSG_TYPE_MOVE_SEND:
                break;
            case MACH_MSG_TYPE_COPY_SEND:
                result = mach_port_mod_refs(mach_task_self(), port, MACH_PORT_RIGHT_SEND, 1);
                break;
            case MACH_MSG_TYPE_MAKE_SEND:
                result = mach_port_insert_right(mach_task_self(), port, port, MACH_MSG_TYPE_MAKE_SEND);
                break;
            default:
                result = KERN_INVALID_VALUE;
                break;
        }
        // libxpc crashes on a right it cannot take; a value without a right stands in for it here.
        if (result != KERN_SUCCESS) {
            port = MACH_PORT_DEAD;
        }
    }
    XpcObject *object = createObject(XPC_TYPE_MACH_SEND);
    object->port = port;
    return object;
}

mach_port_t xpc_mach_send_copy_right(xpc_object_t value) {
    XpcObject *object = value;
    if (object == NULL || object->type != XPC_TYPE_MACH_SEND || !MACH_PORT_VALID(object->port)) {
        return object != NULL && object->type == XPC_TYPE_MACH_SEND ? object->port : MACH_PORT_NULL;
    }
    if (mach_port_mod_refs(mach_task_self(), object->port, MACH_PORT_RIGHT_SEND, 1) != KERN_SUCCESS) {
        return MACH_PORT_DEAD;
    }
    return object->port;
}

mach_port_t xpc_mach_send_get_right(xpc_object_t value) {
    XpcObject *object = value;
    return object != NULL && object->type == XPC_TYPE_MACH_SEND ? object->port : MACH_PORT_NULL;
}

int xpc_user_sessions_enabled(void) {
    return 0;
}

uid_t xpc_user_sessions_get_foreground_uid(int *error) {
    if (error != NULL) {
        *error = ENOTSUP;
    }
    return (uid_t)-1;
}

const char *xpc_strerror(int error) {
    return strerror(error);
}

xpc_connection_t
xpc_connection_create_mach_service(const char *name, dispatch_queue_t targetQueue, uint64_t flags) {
    (void)name;
    (void)targetQueue;
    (void)flags;
    return createObject(XPC_TYPE_CONNECTION);
}

xpc_connection_t
_xpc_connection_create_mach_service(const char *name, dispatch_queue_t targetQueue, uint64_t flags) {
    return xpc_connection_create_mach_service(name, targetQueue, flags);
}

xpc_connection_t xpc_connection_create_from_endpoint(xpc_endpoint_t endpoint) {
    (void)endpoint;
    return createObject(XPC_TYPE_CONNECTION);
}

void xpc_connection_set_target_queue(xpc_connection_t connection, dispatch_queue_t targetQueue) {
    (void)connection;
    (void)targetQueue;
}

void xpc_connection_set_event_handler(xpc_connection_t value, xpc_handler_t handler) {
    XpcObject *connection = value;
    if (connection->handler != NULL) {
        Block_release(connection->handler);
    }
    connection->handler = Block_copy(handler);
}

void xpc_connection_activate(xpc_connection_t connection) {
    (void)connection;
}

void xpc_connection_resume(xpc_connection_t connection) {
    (void)connection;
}

void xpc_connection_cancel(xpc_connection_t connection) {
    (void)connection;
}

void xpc_connection_send_message(xpc_connection_t connection, xpc_object_t message) {
    (void)connection;
    (void)message;
}

void xpc_connection_send_barrier(xpc_connection_t connection, dispatch_block_t barrier) {
    (void)connection;
    dispatch_async(dispatch_get_global_queue(0, 0), barrier);
}

xpc_object_t xpc_copy_entitlement_for_token(const char *entitlement, audit_token_t *auditToken) {
    (void)entitlement;
    (void)auditToken;
    return NULL;
}

xpc_object_t xpc_copy_bootstrap(void) {
    return NULL;
}

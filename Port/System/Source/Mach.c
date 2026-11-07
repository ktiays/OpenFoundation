//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// In-process Mach for the wasm port (Port/System/include/mach/PortMach.h). One table holds every port and
// port set of the process under one mutex; a receiver waits on one condition variable that every enqueue,
// destruction and membership change broadcasts. mk_timer runs on a timer thread of its own. Message bodies
// are copied as bytes; the header's ports are exchanged as the kernel does (the destination becomes the
// receiver's local port, the reply port its remote port), and no right in a body is transferred.

#include <mach/PortMach.h>

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef enum {
    PortKindFree = 0,
    PortKindReceive,
    PortKindSet,
    PortKindDeadName,
} PortKind;

typedef struct PortMessage {
    struct PortMessage *next;
    mach_msg_size_t size;
    unsigned char bytes[];
} PortMessage;

typedef struct {
    PortKind kind;
    uint32_t generation;
    // A receive right: send rights held by name, the queue and its limit, the set it belongs to, the guard.
    mach_port_urefs_t sendRights;
    mach_port_context_t context;
    bool guarded;
    mach_port_msgcount_t queueLimit;
    mach_port_msgcount_t queueCount;
    PortMessage *head;
    PortMessage *tail;
    mach_port_name_t portSet;
    // A timer port's deadline, or UINT64_MAX when unarmed (and 0 for a port that is no timer).
    bool isTimer;
    uint64_t timerDeadline;
} PortEntry;

mach_port_t mach_task_self_ = 0x103;

static pthread_mutex_t gPortLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gPortCondition;
static pthread_cond_t gTimerCondition;
static pthread_once_t gPortOnce = PTHREAD_ONCE_INIT;
static PortEntry *gPorts;
static uint32_t gPortCapacity;
static uint32_t gFreeSearchStart = 1;

// A name is the table index in the low 24 bits and a generation in the high 8 bits, so a stale name of a
// reused slot does not name the new port. Index 0 is never used (MACH_PORT_NULL).
static mach_port_name_t makeName(uint32_t index, uint32_t generation) {
    return (index & 0x00FFFFFF) | ((generation & 0xFF) << 24);
}

static PortEntry *lookup(mach_port_name_t name) {
    if (!MACH_PORT_VALID(name)) {
        return NULL;
    }
    uint32_t index = name & 0x00FFFFFF;
    if (index == 0 || index >= gPortCapacity) {
        return NULL;
    }
    PortEntry *entry = &gPorts[index];
    if (entry->kind == PortKindFree || (entry->generation & 0xFF) != (name >> 24)) {
        return NULL;
    }
    return entry;
}

static void *timerThread(void *argument);

static void initialize(void) {
    pthread_condattr_t attributes;
    pthread_condattr_init(&attributes);
    pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC);
    pthread_cond_init(&gPortCondition, &attributes);
    pthread_cond_init(&gTimerCondition, &attributes);
    pthread_condattr_destroy(&attributes);
    gPortCapacity = 256;
    gPorts = calloc(gPortCapacity, sizeof(PortEntry));
    pthread_t thread;
    pthread_attr_t threadAttributes;
    pthread_attr_init(&threadAttributes);
    pthread_attr_setdetachstate(&threadAttributes, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread, &threadAttributes, timerThread, NULL);
    pthread_attr_destroy(&threadAttributes);
}

static void lockTable(void) {
    pthread_once(&gPortOnce, initialize);
    pthread_mutex_lock(&gPortLock);
}

static void unlockTable(void) {
    pthread_mutex_unlock(&gPortLock);
}

static mach_port_name_t allocateEntry(PortKind kind) {
    for (uint32_t pass = 0; pass < 2; pass++) {
        for (uint32_t index = gFreeSearchStart; index < gPortCapacity; index++) {
            if (gPorts[index].kind == PortKindFree) {
                uint32_t generation = gPorts[index].generation + 1;
                memset(&gPorts[index], 0, sizeof(PortEntry));
                gPorts[index].kind = kind;
                gPorts[index].generation = generation;
                gPorts[index].queueLimit = MACH_PORT_QLIMIT_DEFAULT;
                gFreeSearchStart = index + 1;
                mach_port_name_t name = makeName(index, generation);
                if (!MACH_PORT_VALID(name)) {
                    continue;
                }
                return name;
            }
        }
        gFreeSearchStart = 1;
    }
    uint32_t oldCapacity = gPortCapacity;
    if (oldCapacity >= 0x00FFFFFF) {
        return MACH_PORT_NULL;
    }
    gPortCapacity = oldCapacity * 2;
    gPorts = realloc(gPorts, gPortCapacity * sizeof(PortEntry));
    memset(&gPorts[oldCapacity], 0, (gPortCapacity - oldCapacity) * sizeof(PortEntry));
    gFreeSearchStart = oldCapacity;
    return allocateEntry(kind);
}

static void freeQueue(PortEntry *entry) {
    PortMessage *message = entry->head;
    while (message != NULL) {
        PortMessage *next = message->next;
        free(message);
        message = next;
    }
    entry->head = NULL;
    entry->tail = NULL;
    entry->queueCount = 0;
}

// Destroys a receive right: its queue goes, members leave their set, and remaining send rights turn into a
// dead name.
static void destroyReceiveRight(PortEntry *entry) {
    freeQueue(entry);
    entry->portSet = MACH_PORT_NULL;
    entry->isTimer = false;
    if (entry->sendRights > 0) {
        entry->kind = PortKindDeadName;
    } else {
        entry->kind = PortKindFree;
    }
    pthread_cond_broadcast(&gPortCondition);
}

static void destroyPortSet(mach_port_name_t name, PortEntry *set) {
    for (uint32_t index = 1; index < gPortCapacity; index++) {
        if (gPorts[index].kind == PortKindReceive && gPorts[index].portSet == name) {
            gPorts[index].portSet = MACH_PORT_NULL;
        }
    }
    set->kind = PortKindFree;
    pthread_cond_broadcast(&gPortCondition);
}

kern_return_t mach_port_allocate(ipc_space_t task, mach_port_right_t right, mach_port_name_t *name) {
    (void)task;
    if (right != MACH_PORT_RIGHT_RECEIVE && right != MACH_PORT_RIGHT_PORT_SET &&
        right != MACH_PORT_RIGHT_DEAD_NAME) {
        return KERN_INVALID_VALUE;
    }
    lockTable();
    PortKind kind = right == MACH_PORT_RIGHT_RECEIVE    ? PortKindReceive
                    : right == MACH_PORT_RIGHT_PORT_SET ? PortKindSet
                                                        : PortKindDeadName;
    mach_port_name_t allocated = allocateEntry(kind);
    if (kind == PortKindDeadName && allocated != MACH_PORT_NULL) {
        lookup(allocated)->sendRights = 1;
    }
    unlockTable();
    if (allocated == MACH_PORT_NULL) {
        return KERN_RESOURCE_SHORTAGE;
    }
    *name = allocated;
    return KERN_SUCCESS;
}

kern_return_t mach_port_construct(
    ipc_space_t task, mach_port_options_ptr_t options, mach_port_context_t context, mach_port_name_t *name
) {
    (void)task;
    lockTable();
    mach_port_name_t allocated = allocateEntry(PortKindReceive);
    if (allocated == MACH_PORT_NULL) {
        unlockTable();
        return KERN_RESOURCE_SHORTAGE;
    }
    PortEntry *entry = lookup(allocated);
    entry->context = context;
    if (options != NULL) {
        entry->guarded = (options->flags & MPO_CONTEXT_AS_GUARD) != 0;
        if ((options->flags & MPO_QLIMIT) != 0) {
            entry->queueLimit = options->mpl.mpl_qlimit;
        }
        if ((options->flags & MPO_INSERT_SEND_RIGHT) != 0) {
            entry->sendRights = 1;
        }
    }
    unlockTable();
    *name = allocated;
    return KERN_SUCCESS;
}

kern_return_t mach_port_destruct(
    ipc_space_t task, mach_port_name_t name, mach_port_delta_t srdelta, mach_port_context_t guard
) {
    (void)task;
    lockTable();
    PortEntry *entry = lookup(name);
    if (entry == NULL || entry->kind != PortKindReceive) {
        unlockTable();
        return entry == NULL ? KERN_INVALID_NAME : KERN_INVALID_RIGHT;
    }
    if (entry->guarded && entry->context != guard) {
        unlockTable();
        return KERN_INVALID_ARGUMENT;
    }
    if (srdelta < 0) {
        mach_port_urefs_t drop = (mach_port_urefs_t)-srdelta;
        entry->sendRights = drop > entry->sendRights ? 0 : entry->sendRights - drop;
    }
    destroyReceiveRight(entry);
    unlockTable();
    return KERN_SUCCESS;
}

kern_return_t mach_port_deallocate(ipc_space_t task, mach_port_name_t name) {
    (void)task;
    lockTable();
    PortEntry *entry = lookup(name);
    if (entry == NULL) {
        unlockTable();
        return MACH_PORT_VALID(name) ? KERN_INVALID_NAME : KERN_SUCCESS;
    }
    if ((entry->kind != PortKindReceive && entry->kind != PortKindDeadName) || entry->sendRights == 0) {
        unlockTable();
        return KERN_INVALID_RIGHT;
    }
    entry->sendRights--;
    if (entry->kind == PortKindDeadName && entry->sendRights == 0) {
        entry->kind = PortKindFree;
    }
    unlockTable();
    return KERN_SUCCESS;
}

kern_return_t mach_port_destroy(ipc_space_t task, mach_port_name_t name) {
    (void)task;
    lockTable();
    PortEntry *entry = lookup(name);
    if (entry == NULL) {
        unlockTable();
        return KERN_INVALID_NAME;
    }
    if (entry->kind == PortKindSet) {
        destroyPortSet(name, entry);
    } else {
        entry->sendRights = 0;
        if (entry->kind == PortKindReceive) {
            destroyReceiveRight(entry);
        } else {
            entry->kind = PortKindFree;
        }
    }
    unlockTable();
    return KERN_SUCCESS;
}

kern_return_t mach_port_mod_refs(
    ipc_space_t task, mach_port_name_t name, mach_port_right_t right, mach_port_delta_t delta
) {
    (void)task;
    lockTable();
    PortEntry *entry = lookup(name);
    if (entry == NULL) {
        unlockTable();
        return KERN_INVALID_NAME;
    }
    kern_return_t result = KERN_SUCCESS;
    switch (right) {
        case MACH_PORT_RIGHT_RECEIVE:
            if (entry->kind != PortKindReceive) {
                result = KERN_INVALID_RIGHT;
            } else if (delta < 0) {
                destroyReceiveRight(entry);
            } else if (delta > 0) {
                result = KERN_INVALID_VALUE;
            }
            break;
        case MACH_PORT_RIGHT_PORT_SET:
            if (entry->kind != PortKindSet) {
                result = KERN_INVALID_RIGHT;
            } else if (delta < 0) {
                destroyPortSet(name, entry);
            } else if (delta > 0) {
                result = KERN_INVALID_VALUE;
            }
            break;
        case MACH_PORT_RIGHT_SEND:
        case MACH_PORT_RIGHT_DEAD_NAME:
            if (entry->kind != PortKindReceive && entry->kind != PortKindDeadName) {
                result = KERN_INVALID_RIGHT;
            } else if (delta < 0 && (mach_port_urefs_t)-delta > entry->sendRights) {
                result = KERN_INVALID_VALUE;
            } else {
                entry->sendRights = (mach_port_urefs_t)((int64_t)entry->sendRights + delta);
                if (entry->kind == PortKindDeadName && entry->sendRights == 0) {
                    entry->kind = PortKindFree;
                }
            }
            break;
        default:
            result = KERN_INVALID_VALUE;
            break;
    }
    unlockTable();
    return result;
}

kern_return_t mach_port_get_refs(
    ipc_space_t task, mach_port_name_t name, mach_port_right_t right, mach_port_urefs_t *refs
) {
    (void)task;
    lockTable();
    PortEntry *entry = lookup(name);
    if (entry == NULL) {
        unlockTable();
        return KERN_INVALID_NAME;
    }
    switch (right) {
        case MACH_PORT_RIGHT_RECEIVE:
            *refs = entry->kind == PortKindReceive ? 1 : 0;
            break;
        case MACH_PORT_RIGHT_PORT_SET:
            *refs = entry->kind == PortKindSet ? 1 : 0;
            break;
        case MACH_PORT_RIGHT_SEND:
            *refs = entry->kind == PortKindReceive ? entry->sendRights : 0;
            break;
        case MACH_PORT_RIGHT_DEAD_NAME:
            *refs = entry->kind == PortKindDeadName ? entry->sendRights : 0;
            break;
        default:
            *refs = 0;
            break;
    }
    unlockTable();
    return KERN_SUCCESS;
}

kern_return_t mach_port_insert_right(
    ipc_space_t task, mach_port_name_t name, mach_port_t poly, mach_msg_type_name_t polyPoly
) {
    (void)task;
    if (name != poly) {
        return KERN_INVALID_VALUE;
    }
    lockTable();
    PortEntry *entry = lookup(name);
    kern_return_t result = KERN_SUCCESS;
    if (entry == NULL) {
        result = KERN_INVALID_NAME;
    } else if (
        polyPoly == MACH_MSG_TYPE_MAKE_SEND || polyPoly == MACH_MSG_TYPE_COPY_SEND ||
        polyPoly == MACH_MSG_TYPE_MOVE_SEND
    ) {
        if (entry->kind != PortKindReceive) {
            result = KERN_INVALID_RIGHT;
        } else if (polyPoly != MACH_MSG_TYPE_MOVE_SEND) {
            entry->sendRights++;
        }
    } else {
        result = KERN_INVALID_VALUE;
    }
    unlockTable();
    return result;
}

kern_return_t mach_port_insert_member(ipc_space_t task, mach_port_name_t name, mach_port_name_t pset) {
    (void)task;
    lockTable();
    PortEntry *entry = lookup(name);
    PortEntry *set = lookup(pset);
    kern_return_t result = KERN_SUCCESS;
    if (entry == NULL || set == NULL) {
        result = KERN_INVALID_NAME;
    } else if (entry->kind != PortKindReceive || set->kind != PortKindSet) {
        result = KERN_INVALID_RIGHT;
    } else if (entry->portSet == pset) {
        result = KERN_ALREADY_IN_SET;
    } else {
        // A Darwin receive right may be in several sets; the restored sources only ever use one at a time.
        entry->portSet = pset;
        pthread_cond_broadcast(&gPortCondition);
    }
    unlockTable();
    return result;
}

kern_return_t mach_port_extract_member(ipc_space_t task, mach_port_name_t name, mach_port_name_t pset) {
    (void)task;
    lockTable();
    PortEntry *entry = lookup(name);
    PortEntry *set = lookup(pset);
    kern_return_t result = KERN_SUCCESS;
    if (entry == NULL || set == NULL) {
        result = KERN_INVALID_NAME;
    } else if (entry->kind != PortKindReceive || set->kind != PortKindSet) {
        result = KERN_INVALID_RIGHT;
    } else if (entry->portSet != pset) {
        result = KERN_NOT_IN_SET;
    } else {
        entry->portSet = MACH_PORT_NULL;
    }
    unlockTable();
    return result;
}

kern_return_t mach_port_move_member(ipc_space_t task, mach_port_name_t member, mach_port_name_t after) {
    (void)task;
    lockTable();
    PortEntry *entry = lookup(member);
    kern_return_t result = KERN_SUCCESS;
    if (entry == NULL || entry->kind != PortKindReceive) {
        result = entry == NULL ? KERN_INVALID_NAME : KERN_INVALID_RIGHT;
    } else if (after == MACH_PORT_NULL) {
        entry->portSet = MACH_PORT_NULL;
    } else {
        PortEntry *set = lookup(after);
        if (set == NULL || set->kind != PortKindSet) {
            result = set == NULL ? KERN_INVALID_NAME : KERN_INVALID_RIGHT;
        } else {
            entry->portSet = after;
            pthread_cond_broadcast(&gPortCondition);
        }
    }
    unlockTable();
    return result;
}

kern_return_t mach_port_type(ipc_space_t task, mach_port_name_t name, mach_port_type_t *ptype) {
    (void)task;
    lockTable();
    PortEntry *entry = lookup(name);
    if (entry == NULL) {
        unlockTable();
        return KERN_INVALID_NAME;
    }
    mach_port_type_t type = MACH_PORT_TYPE_NONE;
    switch (entry->kind) {
        case PortKindReceive:
            type = MACH_PORT_TYPE_RECEIVE | (entry->sendRights > 0 ? MACH_PORT_TYPE_SEND : 0);
            break;
        case PortKindSet:
            type = MACH_PORT_TYPE_PORT_SET;
            break;
        case PortKindDeadName:
            type = MACH_PORT_TYPE_DEAD_NAME;
            break;
        case PortKindFree:
            break;
    }
    unlockTable();
    *ptype = type;
    return KERN_SUCCESS;
}

kern_return_t mach_port_get_context(ipc_space_t task, mach_port_name_t name, mach_port_context_t *context) {
    (void)task;
    lockTable();
    PortEntry *entry = lookup(name);
    if (entry == NULL || entry->kind != PortKindReceive) {
        unlockTable();
        return entry == NULL ? KERN_INVALID_NAME : KERN_INVALID_RIGHT;
    }
    *context = entry->guarded ? 0 : entry->context;
    unlockTable();
    return KERN_SUCCESS;
}

kern_return_t mach_port_set_context(ipc_space_t task, mach_port_name_t name, mach_port_context_t context) {
    (void)task;
    lockTable();
    PortEntry *entry = lookup(name);
    if (entry == NULL || entry->kind != PortKindReceive) {
        unlockTable();
        return entry == NULL ? KERN_INVALID_NAME : KERN_INVALID_RIGHT;
    }
    if (entry->guarded) {
        unlockTable();
        return KERN_INVALID_ARGUMENT;
    }
    entry->context = context;
    unlockTable();
    return KERN_SUCCESS;
}

kern_return_t mach_port_request_notification(
    ipc_space_t task,
    mach_port_name_t name,
    integer_t msgid,
    mach_port_mscount_t sync,
    mach_port_t notify,
    mach_msg_type_name_t notifyPoly,
    mach_port_t *previous
) {
    (void)task;
    (void)msgid;
    (void)sync;
    (void)notify;
    (void)notifyPoly;
    // No notification is ever generated: in one process a port dies only when its owner destroys it.
    if (previous != NULL) {
        *previous = MACH_PORT_NULL;
    }
    lockTable();
    kern_return_t result = lookup(name) == NULL ? KERN_INVALID_NAME : KERN_SUCCESS;
    unlockTable();
    return result;
}

static void deadlineAfter(mach_msg_timeout_t milliseconds, struct timespec *deadline) {
    clock_gettime(CLOCK_MONOTONIC, deadline);
    deadline->tv_sec += milliseconds / 1000;
    deadline->tv_nsec += (long)(milliseconds % 1000) * 1000000L;
    if (deadline->tv_nsec >= 1000000000L) {
        deadline->tv_sec += 1;
        deadline->tv_nsec -= 1000000000L;
    }
}

// Waits on the table's condition; false when the deadline passed.
static bool waitForChange(bool hasDeadline, const struct timespec *deadline) {
    if (!hasDeadline) {
        pthread_cond_wait(&gPortCondition, &gPortLock);
        return true;
    }
    return pthread_cond_timedwait(&gPortCondition, &gPortLock, deadline) != ETIMEDOUT;
}

static void enqueue(PortEntry *entry, PortMessage *message) {
    message->next = NULL;
    if (entry->tail != NULL) {
        entry->tail->next = message;
    } else {
        entry->head = message;
    }
    entry->tail = message;
    entry->queueCount++;
    pthread_cond_broadcast(&gPortCondition);
}

static mach_msg_return_t sendMessage(
    mach_msg_header_t *msg, mach_msg_option_t option, mach_msg_size_t size, mach_msg_timeout_t timeout
) {
    if (size < sizeof(mach_msg_header_t)) {
        return MACH_SEND_MSG_TOO_SMALL;
    }
    mach_port_name_t destination = msg->msgh_remote_port;
    mach_msg_type_name_t remoteDisposition = MACH_MSGH_BITS_REMOTE(msg->msgh_bits);
    mach_msg_type_name_t localDisposition = MACH_MSGH_BITS_LOCAL(msg->msgh_bits);
    bool hasDeadline = (option & MACH_SEND_TIMEOUT) != 0;
    struct timespec deadline;
    if (hasDeadline) {
        deadlineAfter(timeout, &deadline);
    }

    lockTable();
    PortEntry *entry = lookup(destination);
    for (;;) {
        if (entry == NULL || entry->kind != PortKindReceive) {
            unlockTable();
            return MACH_SEND_INVALID_DEST;
        }
        if (entry->queueCount < entry->queueLimit) {
            break;
        }
        if (hasDeadline && timeout == 0) {
            unlockTable();
            return MACH_SEND_TIMED_OUT;
        }
        if (!waitForChange(hasDeadline, &deadline)) {
            unlockTable();
            return MACH_SEND_TIMED_OUT;
        }
        entry = lookup(destination);
    }
    // A moved send right is consumed by the send.
    if (remoteDisposition == MACH_MSG_TYPE_MOVE_SEND && entry->sendRights > 0) {
        entry->sendRights--;
    }
    PortMessage *message = malloc(sizeof(PortMessage) + size);
    if (message == NULL) {
        unlockTable();
        return MACH_SEND_NO_BUFFER;
    }
    message->size = size;
    memcpy(message->bytes, msg, size);
    mach_msg_header_t *copy = (mach_msg_header_t *)message->bytes;
    // The receiver sees the destination as its local port and the reply port as its remote port, with the
    // dispositions the kernel reports for the rights it carries.
    copy->msgh_local_port = destination;
    copy->msgh_remote_port = msg->msgh_local_port;
    mach_msg_type_name_t receivedRemote = 0;
    if (MACH_PORT_VALID(msg->msgh_local_port)) {
        receivedRemote = localDisposition == MACH_MSG_TYPE_MAKE_SEND_ONCE ||
                                 localDisposition == MACH_MSG_TYPE_MOVE_SEND_ONCE
                             ? MACH_MSG_TYPE_MOVE_SEND_ONCE
                             : MACH_MSG_TYPE_MOVE_SEND;
        PortEntry *reply = lookup(msg->msgh_local_port);
        if (reply != NULL && reply->kind == PortKindReceive && localDisposition != MACH_MSG_TYPE_MOVE_SEND &&
            localDisposition != MACH_MSG_TYPE_MOVE_SEND_ONCE) {
            reply->sendRights++;
        }
    }
    copy->msgh_bits =
        MACH_MSGH_BITS_SET(receivedRemote, MACH_MSG_TYPE_MOVE_SEND, 0, MACH_MSGH_BITS_OTHER(msg->msgh_bits));
    copy->msgh_size = size;
    enqueue(entry, message);
    unlockTable();
    return MACH_MSG_SUCCESS;
}

// The queue a receive on `name` takes from: the port's own, or the first nonempty member's for a set.
static PortEntry *readyQueue(mach_port_name_t name, PortEntry *receiver) {
    if (receiver->kind == PortKindReceive) {
        return receiver->head != NULL ? receiver : NULL;
    }
    for (uint32_t index = 1; index < gPortCapacity; index++) {
        PortEntry *member = &gPorts[index];
        if (member->kind == PortKindReceive && member->portSet == name && member->head != NULL) {
            return member;
        }
    }
    return NULL;
}

static mach_msg_return_t receiveMessage(
    mach_msg_header_t *msg,
    mach_msg_option_t option,
    mach_msg_size_t limit,
    mach_port_name_t name,
    mach_msg_timeout_t timeout
) {
    bool hasDeadline = (option & MACH_RCV_TIMEOUT) != 0;
    struct timespec deadline;
    if (hasDeadline) {
        deadlineAfter(timeout, &deadline);
    }
    lockTable();
    for (;;) {
        PortEntry *receiver = lookup(name);
        if (receiver == NULL || (receiver->kind != PortKindReceive && receiver->kind != PortKindSet)) {
            unlockTable();
            return receiver == NULL ? MACH_RCV_INVALID_NAME : MACH_RCV_INVALID_NAME;
        }
        PortEntry *queue = readyQueue(name, receiver);
        if (queue != NULL) {
            PortMessage *message = queue->head;
            if (message->size > limit) {
                ((mach_msg_header_t *)msg)->msgh_size = message->size;
                if ((option & MACH_RCV_LARGE) != 0) {
                    unlockTable();
                    return MACH_RCV_TOO_LARGE;
                }
                queue->head = message->next;
                if (queue->head == NULL) {
                    queue->tail = NULL;
                }
                queue->queueCount--;
                free(message);
                pthread_cond_broadcast(&gPortCondition);
                unlockTable();
                return MACH_RCV_TOO_LARGE;
            }
            queue->head = message->next;
            if (queue->head == NULL) {
                queue->tail = NULL;
            }
            queue->queueCount--;
            memcpy(msg, message->bytes, message->size);
            // A minimal format-0 trailer follows the message when the buffer has room for it.
            if (limit >= message->size + sizeof(mach_msg_trailer_t)) {
                mach_msg_trailer_t trailer = {MACH_MSG_TRAILER_FORMAT_0, sizeof(mach_msg_trailer_t)};
                memcpy((unsigned char *)msg + message->size, &trailer, sizeof(trailer));
            }
            free(message);
            pthread_cond_broadcast(&gPortCondition);
            unlockTable();
            return MACH_MSG_SUCCESS;
        }
        if (hasDeadline && timeout == 0) {
            unlockTable();
            return MACH_RCV_TIMED_OUT;
        }
        if (!waitForChange(hasDeadline, &deadline)) {
            unlockTable();
            return MACH_RCV_TIMED_OUT;
        }
    }
}

mach_msg_return_t mach_msg(
    mach_msg_header_t *msg,
    mach_msg_option_t option,
    mach_msg_size_t sendSize,
    mach_msg_size_t receiveLimit,
    mach_port_name_t receiveName,
    mach_msg_timeout_t timeout,
    mach_port_name_t notify
) {
    (void)notify;
    if ((option & MACH_SEND_MSG) != 0) {
        mach_msg_return_t result = sendMessage(msg, option, sendSize, timeout);
        if (result != MACH_MSG_SUCCESS) {
            return result;
        }
    }
    if ((option & MACH_RCV_MSG) != 0) {
        return receiveMessage(msg, option, receiveLimit, receiveName, timeout);
    }
    return MACH_MSG_SUCCESS;
}

void mach_msg_destroy(mach_msg_header_t *msg) {
    // The reply right a received message carries is the only right the port moves.
    if (MACH_MSGH_BITS_REMOTE(msg->msgh_bits) == MACH_MSG_TYPE_MOVE_SEND &&
        MACH_PORT_VALID(msg->msgh_remote_port)) {
        mach_port_deallocate(mach_task_self_, msg->msgh_remote_port);
    }
}

mach_port_t mach_thread_self(void) {
    return (mach_port_t)gettid();
}

mach_port_t mach_host_self(void) {
    return 0x203;
}

const char *mach_error_string(mach_error_t errorValue) {
    switch (errorValue) {
        case KERN_SUCCESS:
            return "(os/kern) successful";
        case KERN_INVALID_ARGUMENT:
            return "(os/kern) invalid argument";
        case KERN_FAILURE:
            return "(os/kern) failure";
        case KERN_RESOURCE_SHORTAGE:
            return "(os/kern) resource shortage";
        case KERN_INVALID_NAME:
            return "(os/kern) invalid name";
        case KERN_INVALID_RIGHT:
            return "(os/kern) invalid right";
        case KERN_INVALID_VALUE:
            return "(os/kern) invalid value";
        case MACH_SEND_INVALID_DEST:
            return "(ipc/send) invalid destination port";
        case MACH_SEND_TIMED_OUT:
            return "(ipc/send) timed out";
        case MACH_RCV_INVALID_NAME:
            return "(ipc/rcv) invalid name";
        case MACH_RCV_TIMED_OUT:
            return "(ipc/rcv) timed out";
        case MACH_RCV_TOO_LARGE:
            return "(ipc/rcv) msg too large";
        default:
            return "unknown error code";
    }
}

void mach_error(const char *string, mach_error_t errorValue) {
    fprintf(stderr, "%s %s (0x%x)\n", string, mach_error_string(errorValue), (unsigned)errorValue);
}

#pragma mark - Time

// Mach absolute time is CLOCK_MONOTONIC in nanoseconds, so the timebase is 1/1.
kern_return_t mach_timebase_info(mach_timebase_info_t info) {
    info->numer = 1;
    info->denom = 1;
    return KERN_SUCCESS;
}

uint64_t mach_absolute_time(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (uint64_t)now.tv_sec * 1000000000ULL + (uint64_t)now.tv_nsec;
}

uint64_t mach_approximate_time(void) {
    return mach_absolute_time();
}

uint64_t mach_continuous_time(void) {
    return mach_absolute_time();
}

uint64_t mach_continuous_approximate_time(void) {
    return mach_absolute_time();
}

#pragma mark - mk_timer

mach_port_name_t mk_timer_create(void) {
    lockTable();
    mach_port_name_t name = allocateEntry(PortKindReceive);
    if (name != MACH_PORT_NULL) {
        PortEntry *entry = lookup(name);
        entry->isTimer = true;
        entry->timerDeadline = UINT64_MAX;
        entry->queueLimit = 1;
    }
    unlockTable();
    return name;
}

kern_return_t mk_timer_destroy(mach_port_name_t name) {
    lockTable();
    PortEntry *entry = lookup(name);
    if (entry == NULL || !entry->isTimer) {
        unlockTable();
        return KERN_INVALID_ARGUMENT;
    }
    entry->sendRights = 0;
    destroyReceiveRight(entry);
    pthread_cond_signal(&gTimerCondition);
    unlockTable();
    return KERN_SUCCESS;
}

kern_return_t mk_timer_arm(mach_port_name_t name, uint64_t expireTime) {
    lockTable();
    PortEntry *entry = lookup(name);
    if (entry == NULL || !entry->isTimer) {
        unlockTable();
        return KERN_INVALID_ARGUMENT;
    }
    entry->timerDeadline = expireTime;
    pthread_cond_signal(&gTimerCondition);
    unlockTable();
    return KERN_SUCCESS;
}

kern_return_t
mk_timer_arm_leeway(mach_port_name_t name, uint64_t flags, uint64_t expireTime, uint64_t leeway) {
    (void)flags;
    (void)leeway;
    return mk_timer_arm(name, expireTime);
}

kern_return_t mk_timer_cancel(mach_port_name_t name, uint64_t *resultTime) {
    lockTable();
    PortEntry *entry = lookup(name);
    if (entry == NULL || !entry->isTimer) {
        unlockTable();
        return KERN_INVALID_ARGUMENT;
    }
    if (resultTime != NULL) {
        *resultTime = entry->timerDeadline == UINT64_MAX ? 0 : entry->timerDeadline;
    }
    entry->timerDeadline = UINT64_MAX;
    unlockTable();
    return KERN_SUCCESS;
}

static void *timerThread(void *argument) {
    (void)argument;
    pthread_mutex_lock(&gPortLock);
    for (;;) {
        uint64_t now = mach_absolute_time();
        uint64_t earliest = UINT64_MAX;
        for (uint32_t index = 1; index < gPortCapacity; index++) {
            PortEntry *entry = &gPorts[index];
            if (entry->kind != PortKindReceive || !entry->isTimer || entry->timerDeadline == UINT64_MAX) {
                continue;
            }
            if (entry->timerDeadline <= now) {
                entry->timerDeadline = UINT64_MAX;
                if (entry->queueCount < entry->queueLimit) {
                    PortMessage *message = malloc(sizeof(PortMessage) + sizeof(mk_timer_expire_msg_t));
                    if (message != NULL) {
                        message->size = sizeof(mk_timer_expire_msg_t);
                        mk_timer_expire_msg_t *expire = (mk_timer_expire_msg_t *)message->bytes;
                        memset(expire, 0, sizeof(*expire));
                        expire->header.msgh_size = sizeof(mk_timer_expire_msg_t);
                        expire->header.msgh_local_port = makeName(index, entry->generation);
                        enqueue(entry, message);
                    }
                }
            } else if (entry->timerDeadline < earliest) {
                earliest = entry->timerDeadline;
            }
        }
        if (earliest == UINT64_MAX) {
            pthread_cond_wait(&gTimerCondition, &gPortLock);
        } else {
            uint64_t wait = earliest - now;
            struct timespec deadline;
            clock_gettime(CLOCK_MONOTONIC, &deadline);
            uint64_t nanoseconds = (uint64_t)deadline.tv_nsec + wait % 1000000000ULL;
            deadline.tv_sec += (time_t)(wait / 1000000000ULL + nanoseconds / 1000000000ULL);
            deadline.tv_nsec = (long)(nanoseconds % 1000000000ULL);
            pthread_cond_timedwait(&gTimerCondition, &gPortLock, &deadline);
        }
    }
    return NULL;
}

#pragma mark - VM

vm_size_t vm_page_size = 4096;
vm_size_t vm_page_mask = 4095;
int vm_page_shift = 12;

kern_return_t vm_allocate(vm_map_t task, vm_address_t *address, vm_size_t size, int flags) {
    (void)task;
    if ((flags & VM_FLAGS_ANYWHERE) == 0) {
        return KERN_INVALID_ARGUMENT;
    }
    size_t rounded = (size + vm_page_mask) & ~vm_page_mask;
    void *memory = aligned_alloc(vm_page_size, rounded == 0 ? vm_page_size : rounded);
    if (memory == NULL) {
        return KERN_NO_SPACE;
    }
    memset(memory, 0, rounded);
    *address = (vm_address_t)memory;
    return KERN_SUCCESS;
}

kern_return_t vm_deallocate(vm_map_t task, vm_address_t address, vm_size_t size) {
    (void)task;
    (void)size;
    free((void *)address);
    return KERN_SUCCESS;
}

kern_return_t vm_copy(vm_map_t task, vm_address_t source, vm_size_t size, vm_address_t destination) {
    (void)task;
    memmove((void *)destination, (const void *)source, size);
    return KERN_SUCCESS;
}

kern_return_t mach_vm_allocate(vm_map_t task, mach_vm_address_t *address, mach_vm_size_t size, int flags) {
    vm_address_t result = 0;
    kern_return_t kr = vm_allocate(task, &result, (vm_size_t)size, flags);
    *address = result;
    return kr;
}

kern_return_t mach_vm_deallocate(vm_map_t task, mach_vm_address_t address, mach_vm_size_t size) {
    return vm_deallocate(task, (vm_address_t)address, (vm_size_t)size);
}

kern_return_t
task_info(task_name_t task, task_flavor_t flavor, task_info_t info, mach_msg_type_number_t *count) {
    (void)task;
    (void)flavor;
    (void)info;
    (void)count;
    return KERN_NOT_SUPPORTED;
}

#pragma mark - libdispatch's run-loop handles

// libdispatch's Emscripten run-loop handle (Port/libdispatch/patches): a receive right with a send right,
// limited to one queued wake-up for the main queue as on Darwin, that CoreFoundation waits on with mach_msg.
uint32_t _dispatch_emscripten_runloop_handle_create(bool mainQueue) {
    mach_port_options_t options = {0};
    options.flags = MPO_INSERT_SEND_RIGHT | (mainQueue ? MPO_QLIMIT : 0);
    options.mpl.mpl_qlimit = 1;
    mach_port_name_t name = MACH_PORT_NULL;
    if (mach_port_construct(mach_task_self_, &options, 0, &name) != KERN_SUCCESS) {
        fprintf(stderr, "libdispatch: no run-loop port\n");
        abort();
    }
    return name;
}

void _dispatch_emscripten_runloop_handle_dispose(uint32_t handle) {
    mach_port_destruct(mach_task_self_, handle, -1, 0);
}

void _dispatch_emscripten_runloop_handle_poke(uint32_t handle) {
    mach_msg_header_t header = {0};
    header.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
    header.msgh_size = sizeof(header);
    header.msgh_remote_port = handle;
    mach_msg(
        &header, MACH_SEND_MSG | MACH_SEND_TIMEOUT, sizeof(header), 0, MACH_PORT_NULL, 0, MACH_PORT_NULL
    );
}

voucher_mach_msg_state_t voucher_mach_msg_adopt(mach_msg_header_t *msg) {
    (void)msg;
    return VOUCHER_MACH_MSG_STATE_UNCHANGED;
}

void voucher_mach_msg_revert(voucher_mach_msg_state_t state) {
    (void)state;
}

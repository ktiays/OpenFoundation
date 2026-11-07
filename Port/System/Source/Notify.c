//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Darwin notifications within the process (Port/System/include/notify.h).

#include <notify.h>

#include <Block.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    bool used;
    char *name;
    dispatch_queue_t queue;
    notify_handler_t handler;
    bool checked;
} Registration;

typedef struct {
    char *name;
    uint64_t state;
} NameState;

static pthread_mutex_t gNotifyLock = PTHREAD_MUTEX_INITIALIZER;
static Registration *gRegistrations;
static int gRegistrationCount;
static NameState *gStates;
static int gStateCount;

static NameState *stateForName(const char *name) {
    for (int index = 0; index < gStateCount; index++) {
        if (strcmp(gStates[index].name, name) == 0) {
            return &gStates[index];
        }
    }
    gStates = realloc(gStates, (size_t)(gStateCount + 1) * sizeof(NameState));
    gStates[gStateCount].name = strdup(name);
    gStates[gStateCount].state = 0;
    return &gStates[gStateCount++];
}

static int addRegistration(const char *name, dispatch_queue_t queue, notify_handler_t handler) {
    int token = -1;
    for (int index = 0; index < gRegistrationCount; index++) {
        if (!gRegistrations[index].used) {
            token = index;
            break;
        }
    }
    if (token < 0) {
        gRegistrations = realloc(gRegistrations, (size_t)(gRegistrationCount + 1) * sizeof(Registration));
        token = gRegistrationCount++;
    }
    Registration *registration = &gRegistrations[token];
    registration->used = true;
    registration->name = strdup(name);
    registration->queue = queue;
    registration->handler = handler != NULL ? Block_copy(handler) : NULL;
    registration->checked = false;
    if (queue != NULL) {
        dispatch_retain(queue);
    }
    return token;
}

uint32_t notify_post(const char *name) {
    if (name == NULL) {
        return NOTIFY_STATUS_INVALID_NAME;
    }
    pthread_mutex_lock(&gNotifyLock);
    for (int token = 0; token < gRegistrationCount; token++) {
        Registration *registration = &gRegistrations[token];
        if (!registration->used || strcmp(registration->name, name) != 0) {
            continue;
        }
        registration->checked = true;
        if (registration->handler != NULL && registration->queue != NULL) {
            notify_handler_t handler = Block_copy(registration->handler);
            int handlerToken = token;
            dispatch_async(registration->queue, ^{
              handler(handlerToken);
              Block_release(handler);
            });
        }
    }
    pthread_mutex_unlock(&gNotifyLock);
    return NOTIFY_STATUS_OK;
}

uint32_t
notify_register_dispatch(const char *name, int *outToken, dispatch_queue_t queue, notify_handler_t handler) {
    if (name == NULL || outToken == NULL) {
        return NOTIFY_STATUS_INVALID_NAME;
    }
    pthread_mutex_lock(&gNotifyLock);
    *outToken = addRegistration(name, queue, handler);
    pthread_mutex_unlock(&gNotifyLock);
    return NOTIFY_STATUS_OK;
}

uint32_t notify_register_check(const char *name, int *outToken) {
    if (name == NULL || outToken == NULL) {
        return NOTIFY_STATUS_INVALID_NAME;
    }
    pthread_mutex_lock(&gNotifyLock);
    *outToken = addRegistration(name, NULL, NULL);
    gRegistrations[*outToken].checked = true;
    pthread_mutex_unlock(&gNotifyLock);
    return NOTIFY_STATUS_OK;
}

static Registration *registrationForToken(int token) {
    return token >= 0 && token < gRegistrationCount && gRegistrations[token].used ? &gRegistrations[token]
                                                                                  : NULL;
}

uint32_t notify_check(int token, int *check) {
    pthread_mutex_lock(&gNotifyLock);
    Registration *registration = registrationForToken(token);
    if (registration == NULL) {
        pthread_mutex_unlock(&gNotifyLock);
        return NOTIFY_STATUS_INVALID_TOKEN;
    }
    *check = registration->checked ? 1 : 0;
    registration->checked = false;
    pthread_mutex_unlock(&gNotifyLock);
    return NOTIFY_STATUS_OK;
}

uint32_t notify_cancel(int token) {
    pthread_mutex_lock(&gNotifyLock);
    Registration *registration = registrationForToken(token);
    if (registration == NULL) {
        pthread_mutex_unlock(&gNotifyLock);
        return NOTIFY_STATUS_INVALID_TOKEN;
    }
    free(registration->name);
    if (registration->handler != NULL) {
        Block_release(registration->handler);
    }
    if (registration->queue != NULL) {
        dispatch_release(registration->queue);
    }
    memset(registration, 0, sizeof(*registration));
    pthread_mutex_unlock(&gNotifyLock);
    return NOTIFY_STATUS_OK;
}

uint32_t notify_get_state(int token, uint64_t *state) {
    pthread_mutex_lock(&gNotifyLock);
    Registration *registration = registrationForToken(token);
    if (registration == NULL) {
        pthread_mutex_unlock(&gNotifyLock);
        return NOTIFY_STATUS_INVALID_TOKEN;
    }
    *state = stateForName(registration->name)->state;
    pthread_mutex_unlock(&gNotifyLock);
    return NOTIFY_STATUS_OK;
}

uint32_t notify_set_state(int token, uint64_t state) {
    pthread_mutex_lock(&gNotifyLock);
    Registration *registration = registrationForToken(token);
    if (registration == NULL) {
        pthread_mutex_unlock(&gNotifyLock);
        return NOTIFY_STATUS_INVALID_TOKEN;
    }
    stateForName(registration->name)->state = state;
    pthread_mutex_unlock(&gNotifyLock);
    return NOTIFY_STATUS_OK;
}

uint32_t notify_is_valid_token(int token) {
    pthread_mutex_lock(&gNotifyLock);
    bool valid = registrationForToken(token) != NULL;
    pthread_mutex_unlock(&gNotifyLock);
    return valid ? 1 : 0;
}

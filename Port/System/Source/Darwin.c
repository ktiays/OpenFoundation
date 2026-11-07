//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// dyld, crt_externs, sysdir, QoS and sysctl answers of the wasm port: one module without Mach-O images, no
// search-path directories, the main thread's QoS class everywhere, and no sysctl names.

#include <crt_externs.h>
#include <mach-o/dyld.h>
#include <sys/qos.h>
#include <sys/sysctl.h>
#include <sysdir.h>

#include <errno.h>
#include <stdbool.h>
#include <string.h>

extern char **environ;

uint32_t _dyld_image_count(void) {
    return 0;
}

const struct mach_header *_dyld_get_image_header(uint32_t imageIndex) {
    (void)imageIndex;
    return NULL;
}

intptr_t _dyld_get_image_vmaddr_slide(uint32_t imageIndex) {
    (void)imageIndex;
    return 0;
}

const char *_dyld_get_image_name(uint32_t imageIndex) {
    (void)imageIndex;
    return NULL;
}

void _dyld_register_func_for_add_image(void (*callback)(const struct mach_header *header, intptr_t slide)) {
    (void)callback;
}

void _dyld_register_func_for_remove_image(
    void (*callback)(const struct mach_header *header, intptr_t slide)
) {
    (void)callback;
}

void _dyld_register_for_bulk_image_loads(
    void (*callback)(unsigned count, const struct mach_header *headers[], const char *paths[])
) {
    (void)callback;
}

static const char kExecutablePath[] = "/wasm/main";

int _NSGetExecutablePath(char *buffer, uint32_t *bufferSize) {
    uint32_t needed = (uint32_t)sizeof(kExecutablePath);
    if (*bufferSize < needed) {
        *bufferSize = needed;
        return -1;
    }
    memcpy(buffer, kExecutablePath, needed);
    return 0;
}

int32_t NSVersionOfRunTimeLibrary(const char *libraryName) {
    (void)libraryName;
    return -1;
}

int32_t NSVersionOfLinkTimeLibrary(const char *libraryName) {
    (void)libraryName;
    return -1;
}

const char *dyld_image_path_containing_address(const void *address) {
    (void)address;
    return NULL;
}

// The program counts as built with the newest SDK, as an app built for the pinned iOS 27.0 is.
typedef struct {
    uint32_t platform;
    uint32_t version;
} PortBuildVersion;

bool dyld_program_sdk_at_least(PortBuildVersion version) {
    (void)version;
    return true;
}

bool OFPort_programSDKAtLeastPacked(uint64_t version) {
    (void)version;
    return true;
}

// The platform whose restoration runs: the iOS Simulator (PLATFORM_IOSSIMULATOR).
uint32_t dyld_get_active_platform(void) {
    return 7;
}

bool dyld_has_inserted_or_interposing_libraries(void) {
    return false;
}

bool _dyld_shared_cache_contains_path(const char *path) {
    (void)path;
    return false;
}

bool _dyld_is_memory_immutable(const void *address, size_t length) {
    (void)address;
    (void)length;
    return false;
}

static int gArgc = 1;
static char gProgramName[] = "wasm";
static char *gArgv[] = {gProgramName, NULL};
static char **gArgvPointer = gArgv;
static char *gProgname = gProgramName;

int *_NSGetArgc(void) {
    return &gArgc;
}

char ***_NSGetArgv(void) {
    return &gArgvPointer;
}

char ***_NSGetEnviron(void) {
    return &environ;
}

char **_NSGetProgname(void) {
    return &gProgname;
}

const struct mach_header_64 *_NSGetMachExecuteHeader(void) {
    return NULL;
}

sysdir_search_path_enumeration_state sysdir_start_search_path_enumeration(
    sysdir_search_path_directory_t directory, sysdir_search_path_domain_mask_t domainMask
) {
    (void)directory;
    (void)domainMask;
    return 0;
}

sysdir_search_path_enumeration_state sysdir_start_search_path_enumeration_private(
    sysdir_search_path_directory_t directory, sysdir_search_path_domain_mask_t domainMask
) {
    return sysdir_start_search_path_enumeration(directory, domainMask);
}

sysdir_search_path_enumeration_state
sysdir_get_next_search_path_enumeration(sysdir_search_path_enumeration_state state, char *path) {
    (void)state;
    if (path != NULL) {
        path[0] = '\0';
    }
    return 0;
}

qos_class_t qos_class_self(void) {
    return QOS_CLASS_USER_INTERACTIVE;
}

qos_class_t qos_class_main(void) {
    return QOS_CLASS_USER_INTERACTIVE;
}

int pthread_attr_set_qos_class_np(pthread_attr_t *attributes, qos_class_t qosClass, int relativePriority) {
    (void)attributes;
    (void)qosClass;
    (void)relativePriority;
    return 0;
}

int pthread_attr_get_qos_class_np(pthread_attr_t *attributes, qos_class_t *qosClass, int *relativePriority) {
    (void)attributes;
    if (qosClass != NULL) {
        *qosClass = QOS_CLASS_DEFAULT;
    }
    if (relativePriority != NULL) {
        *relativePriority = 0;
    }
    return 0;
}

int pthread_set_qos_class_self_np(qos_class_t qosClass, int relativePriority) {
    (void)qosClass;
    (void)relativePriority;
    return 0;
}

int pthread_get_qos_class_np(pthread_t thread, qos_class_t *qosClass, int *relativePriority) {
    (void)thread;
    if (qosClass != NULL) {
        *qosClass = QOS_CLASS_USER_INTERACTIVE;
    }
    if (relativePriority != NULL) {
        *relativePriority = 0;
    }
    return 0;
}

int sysctl(int *name, unsigned int nameLength, void *old, size_t *oldLength, void *new, size_t newLength) {
    (void)name;
    (void)nameLength;
    (void)old;
    (void)oldLength;
    (void)new;
    (void)newLength;
    errno = ENOENT;
    return -1;
}

int sysctlbyname(const char *name, void *old, size_t *oldLength, void *new, size_t newLength) {
    (void)name;
    (void)old;
    (void)oldLength;
    (void)new;
    (void)newLength;
    errno = ENOENT;
    return -1;
}

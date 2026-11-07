//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// The remaining libSystem, libobjc and libicucore entry points the restored sources call that Emscripten does
// not have. Each answers as the platform would when the facility is absent: no process information, no code
// signature, no sandbox, no containers, no kdebug, no Mach-O sections, no fat-file architectures, and the C
// locale's choices for Apple's ICU additions.

#include <PortDarwin.h>

#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include <sys/select.h>

int pthread_getattr_np(pthread_t thread, pthread_attr_t *attributes);

void *__memmove_chk(void *destination, const void *source, size_t length, size_t destinationLength) {
    if (length > destinationLength) {
        abort();
    }
    return memmove(destination, source, length);
}

size_t __strlcpy_chk(char *destination, const char *source, size_t length, size_t destinationLength) {
    if (length > destinationLength) {
        abort();
    }
    return strlcpy(destination, source, length);
}

size_t __strlcat_chk(char *destination, const char *source, size_t length, size_t destinationLength) {
    if (length > destinationLength) {
        abort();
    }
    return strlcat(destination, source, length);
}

static pthread_mutex_t gEnvironmentLock = PTHREAD_MUTEX_INITIALIZER;

void environ_lock_np(void) {
    pthread_mutex_lock(&gEnvironmentLock);
}

void environ_unlock_np(void) {
    pthread_mutex_unlock(&gEnvironmentLock);
}

int pthread_getname_np(pthread_t thread, char *name, size_t length) {
    (void)thread;
    if (length > 0) {
        name[0] = '\0';
    }
    return 0;
}

// No per-thread credentials: callers fall back to the process's.
int pthread_getugid_np(uid_t *uid, gid_t *gid) {
    (void)uid;
    (void)gid;
    return ESRCH;
}

int pthread_is_threaded_np(void) {
    return 1;
}

static pthread_t gMainThread;

__attribute__((constructor)) static void recordMainThread(void) {
    gMainThread = pthread_self();
}

pthread_t pthread_main_thread_np(void) {
    return gMainThread;
}

// Darwin's static TSD keys: libpthread reserves fixed key numbers for their owners (CoreFoundation's is 55).
// Emscripten's libc hands out keys in increasing order and lets pthread_getspecific read any created key, so
// the port creates placeholder keys (without destructors) until it obtains the requested number, which gets
// the destructor. CoreFoundation then reads and writes its slot by number, as on Darwin.
int pthread_key_init_np(int key, void (*destructor)(void *)) {
    for (;;) {
        pthread_key_t created;
        if (pthread_key_create(&created, NULL) != 0) {
            return EAGAIN;
        }
        if ((int)created == key) {
            pthread_key_delete(created);
            pthread_key_t replacement;
            if (pthread_key_create(&replacement, destructor) != 0 || (int)replacement != key) {
                return EAGAIN;
            }
            return 0;
        }
        if ((int)created > key) {
            pthread_key_delete(created);
            return EINVAL;
        }
    }
}

int _pthread_setspecific_static(unsigned long key, const void *value) {
    return pthread_setspecific((pthread_key_t)key, value);
}

void *OFPort_staticKeyGetSpecific(unsigned long key) {
    return pthread_getspecific((pthread_key_t)key);
}

int sigsuspend(const sigset_t *mask) {
    (void)mask;
    errno = EINTR;
    return -1;
}

int portSelect(int count, fd_set *read, fd_set *write, fd_set *error, struct timeval *timeout) __asm__(
    "select$DARWIN_EXTSN"
);

int portSelect(int count, fd_set *read, fd_set *write, fd_set *error, struct timeval *timeout) {
    return select(count, read, write, error, timeout);
}

int portPselect(
    int count,
    fd_set *read,
    fd_set *write,
    fd_set *error,
    const struct timespec *timeout,
    const sigset_t *mask
) __asm__("pselect$DARWIN_EXTSN");

int portPselect(
    int count,
    fd_set *read,
    fd_set *write,
    fd_set *error,
    const struct timespec *timeout,
    const sigset_t *mask
) {
    return pselect(count, read, write, error, timeout, mask);
}

int proc_pidinfo(int pid, int flavor, uint64_t argument, void *buffer, int bufferSize) {
    (void)pid;
    (void)flavor;
    (void)argument;
    (void)buffer;
    (void)bufferSize;
    return 0;
}

int proc_pidpath(int pid, void *buffer, uint32_t bufferSize) {
    (void)pid;
    (void)buffer;
    (void)bufferSize;
    errno = ESRCH;
    return 0;
}

int csops(pid_t pid, unsigned int operations, void *userAddress, size_t userSize) {
    (void)pid;
    (void)operations;
    (void)userAddress;
    (void)userSize;
    errno = ENOSYS;
    return -1;
}

const int SANDBOX_CHECK_NO_REPORT = 0x40000000;

// No sandbox: every operation is allowed.
int sandbox_check(pid_t pid, const char *operation, int type, ...) {
    (void)pid;
    (void)operation;
    (void)type;
    return 0;
}

const char *container_get_identifier(const void *object) {
    (void)object;
    return NULL;
}

const char *container_get_path(const void *object) {
    (void)object;
    return NULL;
}

bool kdebug_is_enabled(uint32_t code) {
    (void)code;
    return false;
}

int kdebug_trace(uint32_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
    (void)code;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    return 0;
}

uint64_t kdebug_trace_string(uint32_t debugID, uint64_t stringID, const char *string) {
    (void)debugID;
    (void)string;
    return stringID;
}

const struct section_64 *
getsectbynamefromheader_64(const struct mach_header_64 *header, const char *segment, const char *section) {
    (void)header;
    (void)segment;
    (void)section;
    return NULL;
}

const struct segment_command_64 *getsegbyname(const char *segment) {
    (void)segment;
    return NULL;
}

struct fat_arch *NXFindBestFatArch(
    cpu_type_t cpuType, cpu_subtype_t cpuSubtype, struct fat_arch *architectures, uint32_t count
) {
    (void)cpuType;
    (void)cpuSubtype;
    (void)architectures;
    (void)count;
    return NULL;
}

const void *NXGetLocalArchInfo(void) {
    return NULL;
}

void thread_stack_async_pcs(void **frames, unsigned int capacity, unsigned int *count) {
    (void)frames;
    (void)capacity;
    *count = 0;
}

const char *const kOSThermalNotificationName = "com.apple.system.thermalstatus";

// NSLog and CFLog take their stderr path when the os_log shim is off.
bool os_log_shim_enabled(void *returnAddress) {
    (void)returnAddress;
    return false;
}

void os_log_shim_with_CFString_4NSLog(
    void *returnAddress, void *log, uint8_t type, const void *format, va_list arguments, void *context
) {
    (void)returnAddress;
    (void)log;
    (void)type;
    (void)format;
    (void)arguments;
    (void)context;
}

// objc4 has no tagged pointers on wasm32, so there is no tag to register and no permutation to apply.
void _objc_registerTaggedPointerClass(uint16_t tag, void *cls) {
    (void)tag;
    (void)cls;
}

const uint8_t objc_debug_tag60_permutations[8] = {0, 1, 2, 3, 4, 5, 6, 7};

// Apple's ICU additions, for the C locale: the first preferred language that is available (exactly, or by its
// language subtag), else the first available localization.
static size_t languageLength(const char *identifier) {
    size_t length = 0;
    while (identifier[length] != '\0' && identifier[length] != '-' && identifier[length] != '_') {
        length++;
    }
    return length;
}

int32_t ualoc_localizationsToUse(
    const char *const *preferredLanguages,
    int32_t preferredLanguagesCount,
    const char *const *availableLocalizations,
    int32_t availableLocalizationsCount,
    char *localizationsToUse[],
    int32_t localizationsToUseCapacity,
    int *status
) {
    if (status != NULL && *status > 0) {
        return -1;
    }
    if (localizationsToUseCapacity <= 0 || availableLocalizationsCount <= 0) {
        return 0;
    }
    for (int32_t preferred = 0; preferred < preferredLanguagesCount; preferred++) {
        for (int32_t available = 0; available < availableLocalizationsCount; available++) {
            if (strcmp(preferredLanguages[preferred], availableLocalizations[available]) == 0) {
                localizationsToUse[0] = (char *)availableLocalizations[available];
                return 1;
            }
        }
        size_t length = languageLength(preferredLanguages[preferred]);
        for (int32_t available = 0; available < availableLocalizationsCount; available++) {
            if (languageLength(availableLocalizations[available]) == length &&
                strncmp(preferredLanguages[preferred], availableLocalizations[available], length) == 0) {
                localizationsToUse[0] = (char *)availableLocalizations[available];
                return 1;
            }
        }
    }
    localizationsToUse[0] = (char *)availableLocalizations[0];
    return 1;
}

// The pattern is used as is: the C locale has no hour-cycle or width remapping to apply.
int32_t uadatpg_remapPatternWithOptions(
    void **generator,
    const uint16_t *pattern,
    int32_t patternLength,
    uint32_t options,
    uint16_t *result,
    int32_t resultCapacity,
    int32_t *status
) {
    (void)generator;
    (void)options;
    if (status != NULL && *status > 0) {
        return 0;
    }
    if (patternLength < 0) {
        patternLength = 0;
        while (pattern[patternLength] != 0) {
            patternLength++;
        }
    }
    if (patternLength > resultCapacity) {
        if (status != NULL) {
            *status = 15; // U_BUFFER_OVERFLOW_ERROR
        }
        return patternLength;
    }
    memcpy(result, pattern, (size_t)patternLength * sizeof(uint16_t));
    if (patternLength < resultCapacity) {
        result[patternLength] = 0;
    }
    return patternLength;
}

char *__stpcpy_chk(char *destination, const char *source, size_t destinationLength) {
    if (strlen(source) >= destinationLength) {
        abort();
    }
    return stpcpy(destination, source);
}

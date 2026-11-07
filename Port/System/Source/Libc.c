//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// The Darwin libc and libpthread additions of the wasm port (Port/System/include/PortDarwin.h).

#include <PortDarwin.h>

#include <ctype.h>
#include <emscripten/threading.h>
#include <limits.h>
#include <math.h>
#include <sched.h>
#include <sys/random.h>
#include <sys/select.h>
#include <time.h>

// musl declares these GNU additions only under _GNU_SOURCE.
int pthread_getattr_np(pthread_t thread, pthread_attr_t *attributes);

void *malloc_type_malloc(size_t size, malloc_type_id_t typeID) {
    (void)typeID;
    return malloc(size);
}

void *malloc_type_calloc(size_t count, size_t size, malloc_type_id_t typeID) {
    (void)typeID;
    return calloc(count, size);
}

void *malloc_type_realloc(void *pointer, size_t size, malloc_type_id_t typeID) {
    (void)typeID;
    return realloc(pointer, size);
}

void *malloc_type_aligned_alloc(size_t alignment, size_t size, malloc_type_id_t typeID) {
    (void)typeID;
    return aligned_alloc(alignment, size);
}

int malloc_type_posix_memalign(void **memory, size_t alignment, size_t size, malloc_type_id_t typeID) {
    (void)typeID;
    return posix_memalign(memory, alignment, size);
}

void *malloc_type_valloc(size_t size, malloc_type_id_t typeID) {
    (void)typeID;
    return aligned_alloc(4096, (size + 4095) & ~(size_t)4095);
}

void __assert_rtn(const char *function, const char *file, int line, const char *expression) {
    if (function != NULL) {
        fprintf(
            stderr,
            "Assertion failed: (%s), function %s, file %s, line %d.\n",
            expression,
            function,
            file,
            line
        );
    } else {
        fprintf(stderr, "Assertion failed: (%s), file %s, line %d.\n", expression, file, line);
    }
    abort();
}

int fls(int value) {
    return value == 0 ? 0 : (int)(sizeof(int) * CHAR_BIT) - __builtin_clz((unsigned int)value);
}

int flsl(long value) {
    return value == 0 ? 0 : (int)(sizeof(long) * CHAR_BIT) - __builtin_clzl((unsigned long)value);
}

int flsll(long long value) {
    return value == 0 ? 0 : (int)(sizeof(long long) * CHAR_BIT) - __builtin_clzll((unsigned long long)value);
}

int ffsl(long value) {
    return __builtin_ffsl(value);
}

int ffsll(long long value) {
    return __builtin_ffsll(value);
}

int __toupper(int character) {
    return toupper(character);
}

int __tolower(int character) {
    return tolower(character);
}

long strtol_l(const char *string, char **end, int base, locale_t locale) {
    (void)locale;
    return strtol(string, end, base);
}

long long strtoll_l(const char *string, char **end, int base, locale_t locale) {
    (void)locale;
    return strtoll(string, end, base);
}

unsigned long strtoul_l(const char *string, char **end, int base, locale_t locale) {
    (void)locale;
    return strtoul(string, end, base);
}

unsigned long long strtoull_l(const char *string, char **end, int base, locale_t locale) {
    (void)locale;
    return strtoull(string, end, base);
}

int vsnprintf_l(char *buffer, size_t size, locale_t locale, const char *format, va_list arguments) {
    (void)locale;
    return vsnprintf(buffer, size, format, arguments);
}

int snprintf_l(char *buffer, size_t size, locale_t locale, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    int result = vsnprintf_l(buffer, size, locale, format, arguments);
    va_end(arguments);
    return result;
}

int vasprintf_l(char **result, locale_t locale, const char *format, va_list arguments) {
    (void)locale;
    return vasprintf(result, format, arguments);
}

int asprintf_l(char **result, locale_t locale, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    int length = vasprintf_l(result, locale, format, arguments);
    va_end(arguments);
    return length;
}

int sscanf_l(const char *string, locale_t locale, const char *format, ...) {
    (void)locale;
    va_list arguments;
    va_start(arguments, format);
    int result = vsscanf(string, format, arguments);
    va_end(arguments);
    return result;
}

static void fillPattern(void *buffer, const void *pattern, size_t patternLength, size_t length) {
    unsigned char *bytes = buffer;
    const unsigned char *source = pattern;
    for (size_t index = 0; index < length; index++) {
        bytes[index] = source[index % patternLength];
    }
}

void memset_pattern4(void *buffer, const void *pattern, size_t length) {
    fillPattern(buffer, pattern, 4, length);
}

void memset_pattern8(void *buffer, const void *pattern, size_t length) {
    fillPattern(buffer, pattern, 8, length);
}

void memset_pattern16(void *buffer, const void *pattern, size_t length) {
    fillPattern(buffer, pattern, 16, length);
}

int *__error(void) {
    return &errno;
}

void arc4random_buf(void *buffer, size_t size) {
    unsigned char *bytes = buffer;
    while (size > 0) {
        size_t chunk = size > 256 ? 256 : size;
        if (getentropy(bytes, chunk) != 0) {
            abort();
        }
        bytes += chunk;
        size -= chunk;
    }
}

uint32_t arc4random(void) {
    uint32_t value;
    arc4random_buf(&value, sizeof(value));
    return value;
}

uint32_t arc4random_uniform(uint32_t upperBound) {
    if (upperBound < 2) {
        return 0;
    }
    uint32_t minimum = -upperBound % upperBound;
    for (;;) {
        uint32_t value = arc4random();
        if (value >= minimum) {
            return value % upperBound;
        }
    }
}

static const char *gProgramName = "wasm";

const char *getprogname(void) {
    return gProgramName;
}

void setprogname(const char *name) {
    const char *slash = strrchr(name, '/');
    gProgramName = slash != NULL ? slash + 1 : name;
}

int issetugid(void) {
    return 0;
}

int pthread_main_np(void) {
    return emscripten_is_main_runtime_thread() ? 1 : 0;
}

int pthread_threadid_np(pthread_t thread, uint64_t *threadID) {
    if (threadID == NULL) {
        return EINVAL;
    }
    *threadID = thread == NULL || pthread_equal(thread, pthread_self()) ? (uint64_t)gettid()
                                                                        : (uint64_t)(uintptr_t)thread;
    return 0;
}

unsigned int pthread_mach_thread_np(pthread_t thread) {
    uint64_t threadID = 0;
    pthread_threadid_np(thread, &threadID);
    return (unsigned int)threadID;
}

pthread_t pthread_from_mach_thread_np(unsigned int port) {
    return port == (unsigned int)gettid() ? pthread_self() : NULL;
}

size_t pthread_get_stacksize_np(pthread_t thread) {
    pthread_attr_t attributes;
    size_t size = 0;
    if (pthread_getattr_np(thread, &attributes) == 0) {
        pthread_attr_getstacksize(&attributes, &size);
        pthread_attr_destroy(&attributes);
    }
    return size;
}

void *pthread_get_stackaddr_np(pthread_t thread) {
    pthread_attr_t attributes;
    void *address = NULL;
    size_t size = 0;
    if (pthread_getattr_np(thread, &attributes) == 0) {
        pthread_attr_getstack(&attributes, &address, &size);
        pthread_attr_destroy(&attributes);
    }
    // Darwin answers the stack's top.
    return (char *)address + size;
}

int portSetCurrentThreadName(const char *name) {
    (void)name;
    return 0;
}

int pthread_setname_np(const char *name) {
    return portSetCurrentThreadName(name);
}

bool dlopen_preflight(const char *path) {
    (void)path;
    return false;
}

char *strnstr(const char *haystack, const char *needle, size_t length) {
    size_t needleLength = strlen(needle);
    if (needleLength == 0) {
        return (char *)haystack;
    }
    for (size_t index = 0; index + needleLength <= length && haystack[index] != '\0'; index++) {
        if (haystack[index] == needle[0] && strncmp(haystack + index, needle, needleLength) == 0) {
            return (char *)haystack + index;
        }
    }
    return NULL;
}

void *bsearch_b(
    const void *key, const void *base, size_t count, size_t width, int (^compare)(const void *, const void *)
) {
    const char *low = base;
    while (count > 0) {
        const char *middle = low + (count / 2) * width;
        int result = compare(key, middle);
        if (result == 0) {
            return (void *)middle;
        }
        if (result > 0) {
            low = middle + width;
            count = count - count / 2 - 1;
        } else {
            count /= 2;
        }
    }
    return NULL;
}

static _Thread_local int (^gSortComparator)(const void *, const void *);

static int sortTrampoline(const void *left, const void *right) {
    return gSortComparator(left, right);
}

void qsort_b(void *base, size_t count, size_t width, int (^compare)(const void *, const void *)) {
    int (^previous)(const void *, const void *) = gSortComparator;
    gSortComparator = compare;
    qsort(base, count, width, sortTrampoline);
    gSortComparator = previous;
}

void pthread_yield_np(void) {
    sched_yield();
}

double __exp10(double value) {
    return pow(10.0, value);
}

float __exp10f(float value) {
    return powf(10.0f, value);
}

struct __float2 __sincosf_stret(float value) {
    struct __float2 result = {sinf(value), cosf(value)};
    return result;
}

struct __double2 __sincos_stret(double value) {
    struct __double2 result = {sin(value), cos(value)};
    return result;
}

// A top-down merge sort through one scratch buffer: equal elements keep their order.
static void mergeRange(
    unsigned char *base, unsigned char *scratch, size_t count, size_t width, int (*compare)(const void *, const void *)
) {
    if (count < 2) {
        return;
    }
    size_t middle = count / 2;
    mergeRange(base, scratch, middle, width, compare);
    mergeRange(base + middle * width, scratch, count - middle, width, compare);
    if (compare(base + (middle - 1) * width, base + middle * width) <= 0) {
        return;
    }
    memcpy(scratch, base, middle * width);
    size_t left = 0;
    size_t right = middle;
    size_t out = 0;
    while (left < middle && right < count) {
        if (compare(base + right * width, scratch + left * width) < 0) {
            memmove(base + out * width, base + right * width, width);
            right++;
        } else {
            memcpy(base + out * width, scratch + left * width, width);
            left++;
        }
        out++;
    }
    memcpy(base + out * width, scratch + left * width, (middle - left) * width);
}

int mergesort(void *base, size_t count, size_t width, int (*compare)(const void *, const void *)) {
    if (width < sizeof(void *) / 2) {
        errno = EINVAL;
        return -1;
    }
    if (count < 2) {
        return 0;
    }
    unsigned char *scratch = malloc((count / 2) * width);
    if (scratch == NULL) {
        errno = ENOMEM;
        return -1;
    }
    mergeRange(base, scratch, count, width, compare);
    free(scratch);
    return 0;
}

int pthread_cond_timedwait_relative_np(
    pthread_cond_t *condition, pthread_mutex_t *mutex, const struct timespec *relativeTime
) {
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += relativeTime->tv_sec;
    deadline.tv_nsec += relativeTime->tv_nsec;
    while (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec += 1;
        deadline.tv_nsec -= 1000000000L;
    }
    return pthread_cond_timedwait(condition, mutex, &deadline);
}

int __darwin_check_fd_set_overflow(int descriptor, const void *set, int unlimitedSelect) {
    (void)set;
    return unlimitedSelect != 0 || (descriptor >= 0 && descriptor < FD_SETSIZE);
}

int getattrlist(
    const char *path, void *attributeList, void *buffer, size_t bufferSize, unsigned int options
) {
    (void)path;
    (void)attributeList;
    (void)buffer;
    (void)bufferSize;
    (void)options;
    errno = ENOTSUP;
    return -1;
}

int fgetattrlist(int descriptor, void *attributeList, void *buffer, size_t bufferSize, unsigned int options) {
    (void)descriptor;
    return getattrlist(NULL, attributeList, buffer, bufferSize, options);
}

int backtrace(void **addresses, int size) {
    (void)addresses;
    (void)size;
    return 0;
}

char **backtrace_symbols(void *const *addresses, int size) {
    (void)addresses;
    return calloc((size_t)(size > 0 ? size : 1), sizeof(char *));
}

void backtrace_symbols_fd(void *const *addresses, int size, int descriptor) {
    (void)addresses;
    (void)size;
    (void)descriptor;
}

struct statfs;

int statfs(const char *path, struct statfs *buffer) {
    (void)path;
    (void)buffer;
    errno = ENOSYS;
    return -1;
}

int fstatfs(int descriptor, struct statfs *buffer) {
    (void)descriptor;
    (void)buffer;
    errno = ENOSYS;
    return -1;
}

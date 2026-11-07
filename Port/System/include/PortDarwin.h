//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// Force-included in every translation unit of the wasm port (Port/CMakeLists.txt): the Darwin libc and
// libpthread additions the restored sources call by their Darwin names, declared here so that the standard
// headers they include stay Emscripten's own. Implementations: Port/System/Source/Libc.c.

#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>
#include <locale.h>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

// Typed allocation (libmalloc's malloc_type interfaces); the type descriptor is ignored.
typedef unsigned long long malloc_type_id_t;
void *malloc_type_malloc(size_t size, malloc_type_id_t typeID);
void *malloc_type_calloc(size_t count, size_t size, malloc_type_id_t typeID);
void *malloc_type_realloc(void *pointer, size_t size, malloc_type_id_t typeID);
void *malloc_type_aligned_alloc(size_t alignment, size_t size, malloc_type_id_t typeID);
int malloc_type_posix_memalign(void **memory, size_t alignment, size_t size, malloc_type_id_t typeID);
void *malloc_type_valloc(size_t size, malloc_type_id_t typeID);

// Assertions as Darwin's assert() reports them.
void __assert_rtn(const char *function, const char *file, int line, const char *expression)
    __attribute__((__noreturn__));

// Bit scanning.
int fls(int value);
int flsl(long value);
int flsll(long long value);
int ffsl(long value);
int ffsll(long long value);

// Unchecked case conversion.
int __toupper(int character);
int __tolower(int character);

// xlocale variants. The port has only the C locale, so the locale argument is ignored.
long strtol_l(const char *string, char **end, int base, locale_t locale);
long long strtoll_l(const char *string, char **end, int base, locale_t locale);
unsigned long strtoul_l(const char *string, char **end, int base, locale_t locale);
unsigned long long strtoull_l(const char *string, char **end, int base, locale_t locale);
double strtod_l(const char *string, char **end, locale_t locale);
float strtof_l(const char *string, char **end, locale_t locale);
long double strtold_l(const char *string, char **end, locale_t locale);
int snprintf_l(char *buffer, size_t size, locale_t locale, const char *format, ...);
int vsnprintf_l(char *buffer, size_t size, locale_t locale, const char *format, va_list arguments);
int asprintf_l(char **result, locale_t locale, const char *format, ...);
int vasprintf_l(char **result, locale_t locale, const char *format, va_list arguments);
int sscanf_l(const char *string, locale_t locale, const char *format, ...);

// String additions.
size_t strlcpy(char *destination, const char *source, size_t size);
size_t strlcat(char *destination, const char *source, size_t size);
void memset_pattern4(void *buffer, const void *pattern, size_t length);
void memset_pattern8(void *buffer, const void *pattern, size_t length);
void memset_pattern16(void *buffer, const void *pattern, size_t length);

// errno, randomness, process.
int *__error(void);
uint32_t arc4random(void);
void arc4random_buf(void *buffer, size_t size);
uint32_t arc4random_uniform(uint32_t upperBound);
pid_t gettid(void);
const char *getprogname(void);
void setprogname(const char *name);
int issetugid(void);

// libpthread's non-portable additions.
int pthread_main_np(void);
int pthread_threadid_np(pthread_t thread, uint64_t *threadID);
unsigned int pthread_mach_thread_np(pthread_t thread);
pthread_t pthread_from_mach_thread_np(unsigned int port);
size_t pthread_get_stacksize_np(pthread_t thread);
void *pthread_get_stackaddr_np(pthread_t thread);
#if defined(__cplusplus) && defined(_GNU_SOURCE)
// C++ under em++ (which defines _GNU_SOURCE): Emscripten's <pthread.h> declares the glibc form
// pthread_setname_np(pthread_t, const char *), so Darwin's one-argument form is an overload that reaches the
// port's definition under its own name; no C++ object refers to the C symbol with Darwin's signature.
int portSetCurrentThreadName(const char *name);
#else
int pthread_setname_np(const char *name);
int portSetCurrentThreadName(const char *name);
#endif
#ifndef PTHREAD_ERRORCHECK_MUTEX_INITIALIZER
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#endif
#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER                                                                  \
    {                                                                                                        \
        {                                                                                                    \
            {                                                                                                \
                PTHREAD_MUTEX_RECURSIVE                                                                      \
            }                                                                                                \
        }                                                                                                    \
    }
#endif

// fcntl(2) commands Emscripten does not have: the port answers EINVAL.
#ifndef F_GETPATH
#define F_GETPATH 50
#endif
#ifndef F_NOCACHE
#define F_NOCACHE 48
#endif
#ifndef F_FULLFSYNC
#define F_FULLFSYNC 51
#endif
#ifndef O_EVTONLY
#define O_EVTONLY 0
#endif
#ifndef O_SYMLINK
#define O_SYMLINK 0
#endif
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

// confstr(3) names of Darwin's per-user directories.
#ifndef _CS_DARWIN_USER_DIR
#define _CS_DARWIN_USER_DIR 65536
#define _CS_DARWIN_USER_TEMP_DIR 65537
#define _CS_DARWIN_USER_CACHE_DIR 65538
#endif

// Darwin's stat time fields, which musl names st_*tim.
#define st_atimespec st_atim
#define st_mtimespec st_mtim
#define st_ctimespec st_ctim

// arm64's trap builtin, which the restored sources use for their crash paths.
#define __builtin_arm_trap(code) __builtin_trap()

// dlfcn additions: the port loads no images.
#ifndef RTLD_FIRST
#define RTLD_FIRST 0x100
#endif
bool dlopen_preflight(const char *path);

// More libc and libpthread additions.
char *strnstr(const char *haystack, const char *needle, size_t length);
void *bsearch_b(
    const void *key, const void *base, size_t count, size_t width, int (^compare)(const void *, const void *)
);
void qsort_b(void *base, size_t count, size_t width, int (^compare)(const void *, const void *));
// Darwin's stable sort (<stdlib.h>): 0, or -1 with errno EINVAL for an element narrower than
// sizeof(void *) / 2, or ENOMEM.
int mergesort(void *base, size_t count, size_t width, int (*compare)(const void *, const void *));
// A wait on a condition bounded by a time relative to now (libpthread).
struct timespec;
int pthread_cond_timedwait_relative_np(
    pthread_cond_t *condition, pthread_mutex_t *mutex, const struct timespec *relativeTime
);
void pthread_yield_np(void);
int pthread_getname_np(pthread_t thread, char *name, size_t length);
unsigned long __maskrune(int32_t character, unsigned long mask);
double __exp10(double value);
float __exp10f(float value);

// Darwin libm's sine and cosine of one argument in one call (<math.h>). The port computes them as sin() and
// cos() (sinf() and cosf()) of Emscripten's libm, so each value is bit for bit what the separate calls give
// on wasm. Darwin's own __sincos_stret and __sincosf_stret are a separate implementation, whose results
// differ from Darwin's sin() and cos() by up to one ulp for some arguments; those bits are not reproduced
// (no libm the port runs reproduces Darwin's).
struct __float2 {
    float __sinval;
    float __cosval;
};
struct __double2 {
    double __sinval;
    double __cosval;
};
struct __float2 __sincosf_stret(float value);
struct __double2 __sincos_stret(double value);
int __darwin_check_fd_set_overflow(int descriptor, const void *set, int unlimitedSelect);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus) && defined(_GNU_SOURCE)
inline int pthread_setname_np(const char *name) {
    return portSetCurrentThreadName(name);
}
#endif

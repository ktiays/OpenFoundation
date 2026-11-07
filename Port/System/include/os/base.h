//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// <os/base.h> of the wasm port: libdispatch's generic Unix base (os/generic_unix_base.h), plus the
// libplatform macros the supplied libdispatch and libsystem headers use beyond it.

#include <os/generic_unix_base.h>

#ifndef OS_UNSAFE_INDEXABLE
#define OS_UNSAFE_INDEXABLE
#endif
#ifndef OS_ALWAYS_INLINE
#define OS_ALWAYS_INLINE __attribute__((__always_inline__))
#endif
#ifndef OS_INLINE
#define OS_INLINE static __inline__
#endif
#ifndef OS_NOINLINE
#define OS_NOINLINE __attribute__((__noinline__))
#endif
#ifndef OS_NOESCAPE
#define OS_NOESCAPE __attribute__((__noescape__))
#endif
#ifndef OS_FORMAT_PRINTF
#define OS_FORMAT_PRINTF(x, y) __attribute__((__format__(printf, x, y)))
#endif
#ifndef OS_NONNULL1
#define OS_NONNULL1 __attribute__((__nonnull__(1)))
#define OS_NONNULL2 __attribute__((__nonnull__(2)))
#define OS_NONNULL3 __attribute__((__nonnull__(3)))
#define OS_NONNULL_ALL __attribute__((__nonnull__))
#endif
#ifndef OS_NORETURN
#define OS_NORETURN __attribute__((__noreturn__))
#endif
#ifndef OS_CONST
#define OS_CONST __attribute__((__const__))
#endif
#ifndef OS_PURE
#define OS_PURE __attribute__((__pure__))
#endif
#ifndef OS_COLD
#define OS_COLD __attribute__((__cold__))
#endif
#ifndef OS_USED
#define OS_USED __attribute__((__used__))
#endif
#ifndef OS_WEAK
#define OS_WEAK __attribute__((__weak__))
#endif
#ifndef OS_MALLOC
#define OS_MALLOC __attribute__((__malloc__))
#endif
#ifndef OS_PACKED
#define OS_PACKED __attribute__((__packed__))
#endif
#ifndef OS_OPTIONS
#define OS_OPTIONS(_name, _type, ...)                                                                        \
    typedef enum : _type { __VA_ARGS__ } __attribute__((__flag_enum__)) _name##_t
#endif
#ifndef OS_CLOSED_ENUM
#define OS_CLOSED_ENUM(_name, _type, ...) typedef enum : _type { __VA_ARGS__ } _name##_t
#endif
#ifndef OS_CLOSED_OPTIONS
#define OS_CLOSED_OPTIONS(_name, _type, ...) OS_OPTIONS(_name, _type, __VA_ARGS__)
#endif
#ifndef OS_EXPORT
#define OS_EXPORT extern __attribute__((__visibility__("default")))
#endif
#ifndef OS_COUNTED_BY
#define OS_COUNTED_BY(N)
#define OS_SIZED_BY(N)
#endif

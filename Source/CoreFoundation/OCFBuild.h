//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <TargetConditionals.h>

#ifndef OF_SYSTEM_NAMES
#define OF_SYSTEM_NAMES 0
#endif

#if OF_SYSTEM_NAMES != 0 && OF_SYSTEM_NAMES != 1
#error "OF_SYSTEM_NAMES must be 0 or 1 in source code."
#endif

// OF_PORT is 1 only in a port's build (Port/CMakeLists.txt; AGENTS.md, "Porting"), which runs the iOS
// Simulator restoration on another TARGET. Every Apple build leaves it 0.
#ifndef OF_PORT
#define OF_PORT 0
#endif

#if !OF_PORT
#if !defined(__arm64__)
#error "OpenFoundation currently supports arm64 only."
#endif

#if !TARGET_OS_SIMULATOR || !TARGET_OS_IOS
#error "Only iOS Simulator restoration is admitted; the iOS Device stage is not restored."
#endif
#endif

#define OF_STRINGIFY_IMPL(value) #value
#define OF_STRINGIFY(value) OF_STRINGIFY_IMPL(value)
#define OF_NAME_STRING(value) OF_STRINGIFY(value)

// The assembler name of a C-level symbol: the platform's user label prefix ("_" on Darwin, none on wasm)
// followed by `name`, a string literal.
#define OF_SYMBOL(name) OF_STRINGIFY(__USER_LABEL_PREFIX__) name

// A static assertion of a native arm64 (LP64) layout. The wasm32 port has 32-bit pointers and longs, so the
// native sizes and offsets do not hold there and are not asserted; every other build asserts them.
#if OF_PORT && !defined(__LP64__)
#define OF_ABI_ASSERT(condition, message) _Static_assert(1, message)
#else
#define OF_ABI_ASSERT(condition, message) _Static_assert(condition, message)
#endif

// A right shift written for a 64-bit word, applied to `value`'s own width: on arm64 the shift as written; on
// the wasm32 port, whose long and pointer-sized integers have 32 bits, the same distance from the top bit.
// The native code tests the top bits of counts and pointers this way ("capacity (%lu) is ridiculous", the
// tagged-pointer bit); a 32-bit operand shifted by 60 or more is undefined.
#define OF_WORD_SHIFT(value, shift) ((value) >> ((shift) - 64 + (int)(sizeof(value) * 8)))

// Bit `bit` of a 64-bit word, placed at the same distance from the top bit of `type` (bit 62 of an arm64
// NSUInteger is bit 30 of a wasm32 one): the native code keeps private flags in the top bits of option words.
#define OF_WORD_BIT(type, bit) ((type)1 << ((bit) - 64 + (int)(sizeof(type) * 8)))

// The type of a bit-field that natively packs into a 64-bit NSUInteger. The wasm32 port keeps a 64-bit
// container so that the native widths fit; every other build declares the native type.
#if OF_PORT && !defined(__LP64__)
#define OF_UINT64_BITFIELD(nativeType) unsigned long long
#else
#define OF_UINT64_BITFIELD(nativeType) nativeType
#endif

#ifdef __cplusplus
#define OF_EXTERN extern "C"
#else
#define OF_EXTERN extern
#endif

#define OF_EXPORT OF_EXTERN __attribute__((visibility("default")))

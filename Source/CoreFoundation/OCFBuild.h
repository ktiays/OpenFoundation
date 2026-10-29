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

#if !defined(__arm64__)
#error "OpenFoundation currently supports arm64 only."
#endif

#if !TARGET_OS_SIMULATOR || !TARGET_OS_IOS
#error "Only the iOS Simulator bootstrap is available; the iOS Device stage is not restored."
#endif

#define OF_STRINGIFY_IMPL(value) #value
#define OF_STRINGIFY(value) OF_STRINGIFY_IMPL(value)
#define OF_NAME_STRING(value) OF_STRINGIFY(value)

#ifdef __cplusplus
#define OF_EXTERN extern "C"
#else
#define OF_EXTERN extern
#endif

#define OF_EXPORT OF_EXTERN __attribute__((visibility("default")))

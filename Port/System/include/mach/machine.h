//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// cpu_type_t and the CPU constants of <mach/machine.h>.

#include <mach/PortMach.h>

#define CPU_ARCH_ABI64 0x01000000
#define CPU_ARCH_ABI64_32 0x02000000
#define CPU_TYPE_ANY ((cpu_type_t) - 1)
#define CPU_TYPE_X86 ((cpu_type_t)7)
#define CPU_TYPE_I386 CPU_TYPE_X86
#define CPU_TYPE_X86_64 (CPU_TYPE_X86 | CPU_ARCH_ABI64)
#define CPU_TYPE_ARM ((cpu_type_t)12)
#define CPU_TYPE_ARM64 (CPU_TYPE_ARM | CPU_ARCH_ABI64)
#define CPU_TYPE_ARM64_32 (CPU_TYPE_ARM | CPU_ARCH_ABI64_32)
#define CPU_TYPE_POWERPC ((cpu_type_t)18)
#define CPU_TYPE_POWERPC64 (CPU_TYPE_POWERPC | CPU_ARCH_ABI64)
#define CPU_SUBTYPE_MASK 0xff000000
#define CPU_SUBTYPE_LIB64 0x80000000
#define CPU_SUBTYPE_ARM64_ALL ((cpu_subtype_t)0)
#define CPU_SUBTYPE_ARM64E ((cpu_subtype_t)2)
#define CPU_SUBTYPE_X86_64_ALL ((cpu_subtype_t)3)

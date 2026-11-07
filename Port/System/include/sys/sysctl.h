//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// sysctl(3) for the wasm port: no name is known, so every query fails with ENOENT.

#include <stddef.h>
#include <sys/types.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

#define CTL_KERN 1
#define CTL_HW 6
#define KERN_OSRELEASE 2
#define KERN_OSVERSION 65
#define KERN_PROC 14
#define KERN_PROC_PID 1
#define KERN_MAXFILESPERPROC 29
#define HW_MEMSIZE 24
#define HW_NCPU 3

// The fields of Darwin's KERN_PROC record the restored sources read; sysctl fills none (it fails).
struct kinfo_proc {
    struct {
        int p_flag;
        char reserved[392];
    } kp_proc;

    struct {
        struct {
            uid_t p_ruid;
            uid_t p_svuid;
            gid_t p_rgid;
            gid_t p_svgid;
        } e_pcred;

        char reserved[240];
    } kp_eproc;
};

#define P_TRACED 0x00000800

int sysctl(int *name, unsigned int nameLength, void *old, size_t *oldLength, void *new, size_t newLength);
int sysctlbyname(const char *name, void *old, size_t *oldLength, void *new, size_t newLength);

__END_DECLS

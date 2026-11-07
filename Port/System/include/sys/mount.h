//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// <sys/mount.h> for the wasm port: Darwin's statfs with the mount flags the restored sources test. The port
// reports no file system (statfs fails with ENOSYS).

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

#define MNT_RDONLY 0x00000001
#define MNT_LOCAL 0x00001000
#define MNT_DONTBROWSE 0x00100000
#define MFSTYPENAMELEN 16
#define MAXPATHLEN 1024

struct statfs {
    uint32_t f_bsize;
    int32_t f_iosize;
    uint64_t f_blocks;
    uint64_t f_bfree;
    uint64_t f_bavail;
    uint64_t f_files;
    uint64_t f_ffree;
    uint64_t f_fsid;
    uid_t f_owner;
    uint32_t f_type;
    uint32_t f_flags;
    uint32_t f_fssubtype;
    char f_fstypename[MFSTYPENAMELEN];
    char f_mntonname[MAXPATHLEN];
    char f_mntfromname[MAXPATHLEN];
    uint32_t f_flags_ext;
    uint32_t f_reserved[7];
};

int statfs(const char *path, struct statfs *buffer);
int fstatfs(int descriptor, struct statfs *buffer);

__END_DECLS

//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// getattrlist(2) for the wasm port: no attribute is available, so every request fails with ENOTSUP.

#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

typedef uint32_t attrgroup_t;

struct attrlist {
    uint16_t bitmapcount;
    uint16_t reserved;
    attrgroup_t commonattr;
    attrgroup_t volattr;
    attrgroup_t dirattr;
    attrgroup_t fileattr;
    attrgroup_t forkattr;
};

typedef struct attrreference {
    int32_t attr_dataoffset;
    uint32_t attr_length;
} attrreference_t;

#define ATTR_BIT_MAP_COUNT 5
#define ATTR_CMN_NAME 0x00000001
#define ATTR_CMN_OBJTYPE 0x00000008
#define ATTR_CMN_FULLPATH 0x08000000
#define FSOPT_NOFOLLOW 0x00000001

int getattrlist(const char *path, void *attributeList, void *buffer, size_t bufferSize, unsigned int options);
int fgetattrlist(int descriptor, void *attributeList, void *buffer, size_t bufferSize, unsigned int options);

__END_DECLS

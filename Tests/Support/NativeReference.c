//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "Support/NativeReference.h"

#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <stdio.h>
#include <string.h>
#include <uuid/uuid.h>

int checkNativeReference(const char *framework, const char *expectedUUID, int capture) {
    char path[256];
    snprintf(path, sizeof(path), "/System/Library/Frameworks/%s.framework/%s", framework, framework);
    void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        fprintf(stderr, "Setup failure: cannot open %s: %s\n", path, dlerror());
        return 77;
    }

    for (uint32_t index = 0; index < _dyld_image_count(); ++index) {
        const char *imagePath = _dyld_get_image_name(index);
        size_t pathLength = strlen(path);
        size_t imageLength = strlen(imagePath);
        if (imageLength < pathLength || strcmp(imagePath + imageLength - pathLength, path) != 0) {
            continue;
        }
        const struct mach_header_64 *header = (const struct mach_header_64 *)_dyld_get_image_header(index);
        if (header->magic != MH_MAGIC_64) {
            fprintf(stderr, "Setup failure: %s is not a 64-bit Mach-O image.\n", framework);
            dlclose(handle);
            return 77;
        }
        const struct load_command *command = (const struct load_command *)(header + 1);
        for (uint32_t number = 0; number < header->ncmds; ++number) {
            if (command->cmd == LC_UUID) {
                const struct uuid_command *uuidCommand = (const struct uuid_command *)command;
                uuid_string_t actualUUID;
                uuid_unparse_upper(uuidCommand->uuid, actualUUID);
                if (capture) {
                    printf("REFERENCE %s %s\n", framework, actualUUID);
                } else if (!expectedUUID[0] || strcmp(actualUUID, expectedUUID) != 0) {
                    fprintf(
                        stderr,
                        "Setup failure: %s UUID %s does not match pin %s.\n",
                        framework,
                        actualUUID,
                        expectedUUID
                    );
                    dlclose(handle);
                    return 77;
                }
                dlclose(handle);
                return 0;
            }
            command = (const struct load_command *)((const char *)command + command->cmdsize);
        }
    }
    fprintf(stderr, "Setup failure: cannot establish the owning image of %s.\n", framework);
    dlclose(handle);
    return 77;
}

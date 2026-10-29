//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "Support/NativeReference.h"

#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach/vm_prot.h>
#include <stdio.h>
#include <string.h>
#include <uuid/uuid.h>

static int imagePathMatches(const char *actual, const char *expected) {
    size_t actualLength = strlen(actual);
    size_t expectedLength = strlen(expected);
    return actualLength >= expectedLength && strcmp(actual + actualLength - expectedLength, expected) == 0;
}

static int checkImageUUID(
    const struct mach_header_64 *header, const char *framework, const char *expectedUUID, int capture
) {
    if (!header || header->magic != MH_MAGIC_64) {
        fprintf(stderr, "Setup failure: %s is not a 64-bit Mach-O image.\n", framework);
        return 77;
    }
    const char *end = (const char *)(header + 1) + header->sizeofcmds;
    const struct load_command *command = (const struct load_command *)(header + 1);
    for (uint32_t number = 0; number < header->ncmds; ++number) {
        if ((const char *)command + sizeof(*command) > end || command->cmdsize < sizeof(*command) ||
            (const char *)command + command->cmdsize > end) {
            fprintf(stderr, "Setup failure: malformed load commands in %s.\n", framework);
            return 77;
        }
        if (command->cmd == LC_UUID && command->cmdsize >= sizeof(struct uuid_command)) {
            const struct uuid_command *uuidCommand = (const struct uuid_command *)command;
            uuid_string_t actualUUID;
            uuid_unparse_upper(uuidCommand->uuid, actualUUID);
            if (capture) {
                printf("REFERENCE %s %s\n", framework, actualUUID);
            } else if (!expectedUUID || !expectedUUID[0] || strcmp(actualUUID, expectedUUID) != 0) {
                fprintf(
                    stderr,
                    "Setup failure: %s UUID %s does not match pin %s.\n",
                    framework,
                    actualUUID,
                    expectedUUID ? expectedUUID : "<missing>"
                );
                return 77;
            }
            return 0;
        }
        command = (const struct load_command *)((const char *)command + command->cmdsize);
    }
    fprintf(stderr, "Setup failure: missing LC_UUID in %s.\n", framework);
    return 77;
}

int checkNativeReference(const char *framework, const char *expectedUUID, int capture) {
    char path[256];
    snprintf(path, sizeof(path), "/System/Library/Frameworks/%s.framework/%s", framework, framework);
    void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        fprintf(stderr, "Setup failure: cannot open %s: %s\n", path, dlerror());
        return 77;
    }
    for (uint32_t index = 0; index < _dyld_image_count(); ++index) {
        if (imagePathMatches(_dyld_get_image_name(index), path)) {
            int status = checkImageUUID(
                (const struct mach_header_64 *)_dyld_get_image_header(index), framework, expectedUUID, capture
            );
            dlclose(handle);
            return status;
        }
    }
    fprintf(stderr, "Setup failure: cannot establish the owning image of %s.\n", framework);
    dlclose(handle);
    return 77;
}

static int resolveFrameworkFunction(
    const char *path, const char *owner, const char *expectedUUID, const char *symbol, void **function
) {
    *function = NULL;
    void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        fprintf(stderr, "Setup failure: cannot open %s: %s\n", path, dlerror());
        return 77;
    }
    dlerror();
    void *address = dlsym(handle, symbol);
    const char *error = dlerror();
    if (!address || error) {
        fprintf(
            stderr,
            "Setup failure: cannot resolve %s in %s: %s\n",
            symbol,
            path,
            error ? error : "missing address"
        );
        dlclose(handle);
        return 77;
    }
    Dl_info information;
    if (!dladdr(address, &information) || !information.dli_fname ||
        !imagePathMatches(information.dli_fname, owner)) {
        fprintf(stderr, "Setup failure: %s has an unexpected owning image.\n", symbol);
        dlclose(handle);
        return 77;
    }
    if (expectedUUID) {
        int status = checkImageUUID(information.dli_fbase, owner, expectedUUID, 0);
        if (status) {
            dlclose(handle);
            return status;
        }
    }
    // Keep the test-only handle open until process exit so the returned code cannot unload.
    *function = address;
    return 0;
}

int resolveReferenceFunction(
    const char *framework, const char *expectedUUID, const char *symbol, void **function
) {
    char path[256];
    snprintf(path, sizeof(path), "/System/Library/Frameworks/%s.framework/%s", framework, framework);
    return resolveFrameworkFunction(path, path, expectedUUID, symbol, function);
}

int resolveReplacementFunction(const char *framework, const char *symbol, void **function) {
    char path[256];
    char owner[256];
    snprintf(path, sizeof(path), "@rpath/%s.framework/%s", framework, framework);
    snprintf(owner, sizeof(owner), "/%s.framework/%s", framework, framework);
    return resolveFrameworkFunction(path, owner, NULL, symbol, function);
}

int resolveReferenceAddress(
    const char *framework,
    const char *expectedUUID,
    uintptr_t nativeAddress,
    uintptr_t nativeImageBase,
    void **function
) {
    *function = NULL;
    int status = checkNativeReference(framework, expectedUUID, 0);
    if (status != 0) {
        return status;
    }
    char path[256];
    snprintf(path, sizeof(path), "/System/Library/Frameworks/%s.framework/%s", framework, framework);
    void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        fprintf(stderr, "Setup failure: cannot retain native image %s.\n", framework);
        return 77;
    }
    for (uint32_t index = 0; index < _dyld_image_count(); ++index) {
        if (!imagePathMatches(_dyld_get_image_name(index), path)) {
            continue;
        }
        const struct mach_header_64 *header = (const struct mach_header_64 *)_dyld_get_image_header(index);
        const struct load_command *command = (const struct load_command *)(header + 1);
        int baseMatches = 0;
        int executableAddress = 0;
        for (uint32_t number = 0; number < header->ncmds; ++number) {
            if (command->cmd == LC_SEGMENT_64) {
                const struct segment_command_64 *segment = (const struct segment_command_64 *)command;
                if (strcmp(segment->segname, "__TEXT") == 0) {
                    baseMatches = segment->vmaddr == nativeImageBase;
                }
                if ((segment->initprot & VM_PROT_EXECUTE) != 0 && nativeAddress >= segment->vmaddr &&
                    nativeAddress - segment->vmaddr < segment->vmsize) {
                    executableAddress = 1;
                }
            }
            command = (const struct load_command *)((const char *)command + command->cmdsize);
        }
        if (!baseMatches || !executableAddress || nativeAddress < nativeImageBase) {
            fprintf(stderr, "Setup failure: native address/base is outside the pinned executable image.\n");
            dlclose(handle);
            return 77;
        }
        uintptr_t address = (uintptr_t)header + (nativeAddress - nativeImageBase);
        Dl_info information;
        if (!dladdr((const void *)address, &information) || information.dli_fbase != header ||
            !information.dli_fname || !imagePathMatches(information.dli_fname, path)) {
            fprintf(stderr, "Setup failure: native offset resolved into an unexpected image.\n");
            dlclose(handle);
            return 77;
        }
        *function = (void *)address;
        return 0;
    }
    fprintf(stderr, "Setup failure: no loaded owner for native address.\n");
    dlclose(handle);
    return 77;
}

int checkInternalImplementation(const void *function) {
    Dl_info information;
    if (!function || !dladdr(function, &information) || information.dli_fbase != _dyld_get_image_header(0)) {
        fprintf(stderr, "Setup failure: internal replacement is not owned by the current test image.\n");
        return 77;
    }
    return 0;
}

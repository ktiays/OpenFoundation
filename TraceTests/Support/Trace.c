//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "Support/Trace.h"

#include "ReferencePins.h"

#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <uuid/uuid.h>

enum {
    kIdentityCapacity = 4096,
    kDynamicCapacity = 256,
    kBuiltinTypeCount = 72,
};

static const void *gIdentities[kIdentityCapacity];
static size_t gIdentityCount;
static unsigned long gDynamicTypes[kDynamicCapacity];
static size_t gDynamicCount;
static int gSetupFailed;
static char gText[8][32];
static size_t gTextIndex;

static char *nextText(void) {
    gTextIndex = (gTextIndex + 1) % 8;
    return gText[gTextIndex];
}

static int pathEndsWith(const char *path, const char *suffix) {
    size_t pathLength = strlen(path);
    size_t suffixLength = strlen(suffix);
    return pathLength >= suffixLength && strcmp(path + pathLength - suffixLength, suffix) == 0;
}

static int imageUUID(const struct mach_header_64 *header, uuid_string_t text) {
    if (!header || header->magic != MH_MAGIC_64) {
        return 0;
    }
    const struct load_command *command = (const struct load_command *)(header + 1);
    for (uint32_t number = 0; number < header->ncmds; ++number) {
        if (command->cmd == LC_UUID && command->cmdsize >= sizeof(struct uuid_command)) {
            uuid_unparse_upper(((const struct uuid_command *)command)->uuid, text);
            return 1;
        }
        command = (const struct load_command *)((const char *)command + command->cmdsize);
    }
    return 0;
}

int checkTraceProcess(void) {
    int nativeCore = 0;
    int nativeFoundation = 0;
    int restored = 0;
    uuid_string_t coreUUID = "";
    for (uint32_t index = 0; index < _dyld_image_count(); ++index) {
        const char *name = _dyld_get_image_name(index);
        if (pathEndsWith(name, "/CoreFoundation.framework/CoreFoundation")) {
            nativeCore = 1;
            if (!imageUUID((const struct mach_header_64 *)_dyld_get_image_header(index), coreUUID)) {
                fputs("Setup failure: the loaded CoreFoundation has no LC_UUID.\n", stderr);
                return 77;
            }
        } else if (pathEndsWith(name, "/Foundation.framework/Foundation")) {
            nativeFoundation = 1;
        } else if (pathEndsWith(name, "/OpenCoreFoundation.framework/OpenCoreFoundation")) {
            restored = 1;
        }
    }
#if OF_TRACE_REFERENCE
    if (!nativeCore || restored) {
        fputs(
            "Setup failure: the reference trace needs system CoreFoundation and no restored framework.\n",
            stderr
        );
        return 77;
    }
    if (strcmp(coreUUID, OF_REFERENCE_CF_UUID) != 0) {
        fprintf(
            stderr,
            "Setup failure: CoreFoundation UUID %s does not match pin %s.\n",
            coreUUID,
            OF_REFERENCE_CF_UUID
        );
        return 77;
    }
    printf("# side reference\n# CoreFoundation %s\n", coreUUID);
#else
    if (nativeCore || nativeFoundation || !restored) {
        fputs(
            "Setup failure: the replacement trace needs OpenCoreFoundation and no native "
            "Foundation/CoreFoundation.\n",
            stderr
        );
        return 77;
    }
    printf("# side replacement\n# match %s\n", OF_REFERENCE_CF_UUID);
#endif
    (void)nativeFoundation;
    return 0;
}

void traceScenario(const char *name) {
    gIdentityCount = 0;
    gDynamicCount = 0;
    printf("== %s\n", name);
}

void traceLine(const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    fputs("  ", stdout);
    vprintf(format, arguments);
    fputc('\n', stdout);
    va_end(arguments);
}

const char *traceObject(const void *object) {
    if (object == NULL) {
        return "NULL";
    }
    size_t index = 0;
    while (index < gIdentityCount && gIdentities[index] != object) {
        ++index;
    }
    if (index == gIdentityCount) {
        if (gIdentityCount == kIdentityCapacity) {
            gSetupFailed = 1;
            return "@overflow";
        }
        gIdentities[gIdentityCount++] = object;
    }
    char *text = nextText();
    snprintf(text, sizeof(gText[0]), "@%zu", index + 1);
    return text;
}

void traceForget(const void *object) {
    for (size_t index = 0; index < gIdentityCount; ++index) {
        if (gIdentities[index] == object) {
            // Keep later indices stable: the slot stays occupied by an address no object can have.
            gIdentities[index] = (const void *)(uintptr_t)1;
            return;
        }
    }
}

const char *traceTypeID(unsigned long typeID) {
    if (typeID < kBuiltinTypeCount) {
        char *text = nextText();
        snprintf(text, sizeof(gText[0]), "%lu", typeID);
        return text;
    }
    size_t index = 0;
    while (index < gDynamicCount && gDynamicTypes[index] != typeID) {
        ++index;
    }
    if (index == gDynamicCount) {
        if (gDynamicCount == kDynamicCapacity) {
            gSetupFailed = 1;
            return "dynamic#overflow";
        }
        gDynamicTypes[gDynamicCount++] = typeID;
    }
    char *text = nextText();
    snprintf(text, sizeof(gText[0]), "dynamic#%zu", index + 1);
    return text;
}

int traceIsTagged(const void *object) {
    return ((uintptr_t)object >> 63) != 0;
}

int traceSetupFailed(void) {
    return gSetupFailed;
}

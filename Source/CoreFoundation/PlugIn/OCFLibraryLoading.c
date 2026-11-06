//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PlugIn/OCFLibraryLoading.h"

#include <assert.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <stdbool.h>
#include <string.h>

// Inferred organization: _CFIsMatchingLibrary (0x1c02ace1c), _CFGetHandleForLoadedLibrary (0x1c02ace6c) and
// _CFGetHandleForInsertedOrInterposingLibrary (0x1c02acee4) are contiguous in the iOS Simulator __text
// section between the NSCollection helpers and CFTimeZone; their .cold.1 assertions (0x1c031061c,
// 0x1c0310644) name CFLibraryLoading.c, which names this unit. The native directory is not in the evidence;
// the unit is kept beside the bundle loader. iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified; iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBundle.md (pending; the coordinator writes the reading).

// dyld's SPI (libdyld, re-exported by libSystem; no SDK declaration). The native caller tests W0.
extern bool dyld_has_inserted_or_interposing_libraries(void);

// Native _CFIsMatchingLibrary, 0x1c02ace1c. Hidden (was private external). The text after the last "/" of the
// image path (the whole path without one) starts with the prefix (strncmp over strlen(prefix) bytes).
Boolean _CFIsMatchingLibrary(const char *imagePath, const char *namePrefix) {
    const char *slash = strrchr(imagePath, '/');
    const char *name = slash == NULL ? imagePath : slash + 1;
    return strncmp(name, namePrefix, strlen(namePrefix)) == 0;
}

// Native _CFGetHandleForLoadedLibrary, 0x1c02ace6c, with its .cold.1 (0x1c031061c, __assert_rtn at
// CFLibraryLoading.c line 40). Hidden (was private external). The first dyld image (32-bit index) that
// matches is reopened with dlopen mode 0x15 (RTLD_NOLOAD | RTLD_LOCAL | RTLD_LAZY), which must succeed; a
// NULL image name or no match answers NULL.
void *_CFGetHandleForLoadedLibrary(const char *namePrefix) {
    uint32_t count = _dyld_image_count();
    if (count == 0) {
        return NULL;
    }
    for (uint32_t index = 0; index != count; index++) {
        const char *imagePath = _dyld_get_image_name(index);
        if (_CFIsMatchingLibrary(imagePath, namePrefix)) {
            if (imagePath == NULL) {
                return NULL;
            }
            void *handle = dlopen(imagePath, RTLD_NOLOAD | RTLD_LOCAL | RTLD_LAZY);
            if (handle == NULL) {
                __assert_rtn(
                    "_CFGetHandleForLoadedLibrary",
                    "CFLibraryLoading.c",
                    40,
                    "handle && \"Using exact image path to retrieve handle for already-loaded library must "
                    "succeed\""
                );
            }
            return handle;
        }
    }
    return NULL;
}

// Native _CFGetHandleForInsertedOrInterposingLibrary, 0x1c02acee4, with its .cold.1 (0x1c0310644,
// __assert_rtn at CFLibraryLoading.c line 45 for a NULL prefix). External. NULL unless
// dyld_has_inserted_or_interposing_libraries answers true; then a tail call of _CFGetHandleForLoadedLibrary.
void *_OCFGetHandleForInsertedOrInterposingLibrary(const char *namePrefix) {
    if (namePrefix == NULL) {
        __assert_rtn(
            OF_NAME_STRING(_OCFGetHandleForInsertedOrInterposingLibrary),
            "CFLibraryLoading.c",
            45,
            "namePrefix && \"Library name prefix cannot be null\""
        );
    }
    if (!dyld_has_inserted_or_interposing_libraries()) {
        return NULL;
    }
    return _CFGetHandleForLoadedLibrary(namePrefix);
}

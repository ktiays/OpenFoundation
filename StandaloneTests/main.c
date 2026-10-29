//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include <OpenFoundation/OpenFoundation.h>

#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    for (uint32_t index = 0; index < _dyld_image_count(); ++index) {
        const char *path = _dyld_get_image_name(index);
        if (strstr(path, "/System/Library/Frameworks/Foundation.framework/") ||
            strstr(path, "/System/Library/Frameworks/CoreFoundation.framework/")) {
            fprintf(stderr, "Standalone isolation failure: loaded %s\n", path);
            return 1;
        }
    }
    puts("Standalone framework load passed without system Foundation or CoreFoundation.");
    puts("Restored API assertions: 0 (bootstrap only).");
    return 0;
}

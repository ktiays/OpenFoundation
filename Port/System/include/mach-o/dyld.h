//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// <mach-o/dyld.h> for the wasm port: the program is one module with no Mach-O image, so dyld reports no
// image, the executable path is the module's name, and image-load callbacks never fire.

#include <mach-o/loader.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

uint32_t _dyld_image_count(void);
const struct mach_header *_dyld_get_image_header(uint32_t imageIndex);
intptr_t _dyld_get_image_vmaddr_slide(uint32_t imageIndex);
const char *_dyld_get_image_name(uint32_t imageIndex);
void _dyld_register_func_for_add_image(void (*callback)(const struct mach_header *header, intptr_t slide));
void _dyld_register_func_for_remove_image(void (*callback)(const struct mach_header *header, intptr_t slide));
int _NSGetExecutablePath(char *buffer, uint32_t *bufferSize);
int32_t NSVersionOfRunTimeLibrary(const char *libraryName);
int32_t NSVersionOfLinkTimeLibrary(const char *libraryName);
bool _dyld_shared_cache_contains_path(const char *path);

__END_DECLS

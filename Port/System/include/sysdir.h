//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// sysdir(3) for the wasm port: no search path has a directory, so every enumeration ends at once.

#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

typedef unsigned int sysdir_search_path_enumeration_state;
typedef unsigned int sysdir_search_path_directory_t;
typedef unsigned int sysdir_search_path_domain_mask_t;

sysdir_search_path_enumeration_state sysdir_start_search_path_enumeration(
    sysdir_search_path_directory_t directory, sysdir_search_path_domain_mask_t domainMask
);
sysdir_search_path_enumeration_state sysdir_start_search_path_enumeration_private(
    sysdir_search_path_directory_t directory, sysdir_search_path_domain_mask_t domainMask
);
sysdir_search_path_enumeration_state
sysdir_get_next_search_path_enumeration(sysdir_search_path_enumeration_state state, char *path);

__END_DECLS

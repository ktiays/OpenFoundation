//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// backtrace(3) for the wasm port: wasm frames are not walkable from C, so a backtrace is empty.

#include <stddef.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

int backtrace(void **addresses, int size);
char **backtrace_symbols(void *const *addresses, int size);
void backtrace_symbols_fd(void *const *addresses, int size, int descriptor);

__END_DECLS

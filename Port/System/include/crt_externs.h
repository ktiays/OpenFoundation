//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// <crt_externs.h> for the wasm port: the program's arguments as Emscripten does not keep them, a single
// program name; the environment is the process's.

#include <sys/cdefs.h>

__BEGIN_DECLS

int *_NSGetArgc(void);
char ***_NSGetArgv(void);
char ***_NSGetEnviron(void);
char **_NSGetProgname(void);
struct mach_header_64;
// The arm64 declaration (the restoration is LP64); the port has no image and answers NULL.
const struct mach_header_64 *_NSGetMachExecuteHeader(void);

__END_DECLS

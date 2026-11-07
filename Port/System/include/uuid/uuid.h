//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// libuuid for the wasm port (Port/System/Source/Uuid.c).

#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

typedef unsigned char uuid_t[16];
typedef char uuid_string_t[37];

void uuid_clear(uuid_t uu);
int uuid_compare(const uuid_t uu1, const uuid_t uu2);
void uuid_copy(uuid_t destination, const uuid_t source);
void uuid_generate(uuid_t out);
void uuid_generate_random(uuid_t out);
int uuid_is_null(const uuid_t uu);
int uuid_parse(const char *in, uuid_t uu);
void uuid_unparse(const uuid_t uu, char *out);
void uuid_unparse_lower(const uuid_t uu, char *out);
void uuid_unparse_upper(const uuid_t uu, char *out);

__END_DECLS

//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// <ctype.h> of the wasm port: Emscripten's, with what Darwin's <ctype.h> adds through <_ctype.h> (the rune
// locale's class bits and __istype/__isctype, runetype.h).

#include_next <ctype.h>

#ifndef PORT_SYSTEM_CTYPE_H
#define PORT_SYSTEM_CTYPE_H
#include <runetype.h>
#endif

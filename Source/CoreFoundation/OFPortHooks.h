//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// Port hooks (AGENTS.md, "Porting"). A port is a TARGET of its own whose code lives in Port/, outside
// Source/. The restoration's sources reach a port only through the macros below, and only at the sites
// Port/hooks.tsv lists.
//
// `OF_PORT_ANSWER(hook, native, ...)` replaces the value of one expression. On every Apple TARGET it is
// `(native)`, the restored expression as it is; on a port, `OFPort_<hook>(...)`, and `native` is not
// compiled.
//
// `OF_PORT_RETURN(hook, ...)` replaces a whole function and is its first statement. On every Apple TARGET it
// is `((void)0)`, which generates nothing; on a port, `return OFPort_<hook>(...)`.
//
// `OF_PORT_FUNCTION(hook, native)` replaces a function address stored in data, such as a callback in a static
// table, where a call is not a constant expression. On every Apple TARGET it is `(native)`; on a port,
// `(OFPort_<hook>)`, a function the port declares with the slot's exact type.
//
// `OF_PORT_NATIVE(statement)` keeps a native statement that cannot compile on a port (inline assembly of the
// native instruction set) out of a port's build: on every Apple TARGET it is the statement; on a port,
// nothing. It follows an OF_PORT_RETURN of the same function, which supplies the port's behavior.
//
// So on every Apple TARGET the macros leave the restored code alone, and the binary is the one built without
// them.

#if OF_PORT
// The port's declarations of its hook functions, on the port's include path only.
#include <OFPortHookDeclarations.h>
#define OF_PORT_ANSWER(hook, native, ...) (OFPort_##hook(__VA_ARGS__))
#define OF_PORT_RETURN(hook, ...) return OFPort_##hook(__VA_ARGS__)
#define OF_PORT_FUNCTION(hook, native) (OFPort_##hook)
#define OF_PORT_NATIVE(...)
#else
#define OF_PORT_ANSWER(hook, native, ...) (native)
#define OF_PORT_RETURN(hook, ...) ((void)0)
#define OF_PORT_FUNCTION(hook, native) (native)
#define OF_PORT_NATIVE(...) __VA_ARGS__
#endif

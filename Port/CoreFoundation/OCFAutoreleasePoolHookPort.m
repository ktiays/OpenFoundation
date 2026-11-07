//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// The wasm port's libdispatch autorelease-pool pop hook (Port/hooks.tsv, autoreleasePoolPopHook).
// __CFInitialize (Source/CoreFoundation/Runtime/OCFRuntime.c) stores _CFAutoreleasePoolPop, which answers its
// argument, in libdispatch's void (*)(void *) hook. wasm checks indirect call signatures, so the port stores
// this function of the hook's type; it calls the restored _CFAutoreleasePoolPop unchanged.

#import "CoreFoundation/Objective-C/OCFNSObject.h"

#include <OFPortHookDeclarations.h>

static void autoreleasePoolPop(void *context) {
    (void)_OCFAutoreleasePoolPop(context);
}

void (*OFPort_autoreleasePoolPopHook(void))(void *) {
    return autoreleasePoolPop;
}

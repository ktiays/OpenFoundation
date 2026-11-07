//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation's load-time initialization for the wasm port. On Darwin, +[NSObject(NSObject) load] runs
// _NSInitializePlatform when objc maps Foundation, after CoreFoundation's initializer. The restored category
// stays uninstalled (Source/Foundation/Base/ONSInitializePlatform.m explains the libswiftCore boundary on
// Apple builds); in one wasm module objc4 runs every +load before CoreFoundation's constructor, so the port
// initializes Foundation from a constructor of this archive, which links after CoreFoundation's.

#import "Foundation/Base/ONSInitializePlatform.h"
#import <objc/NSObjCRuntime.h>

__attribute__((constructor)) static void initializeFoundation(void) {
    _NSInitializePlatform();
}

#include "CoreFoundation/String/OCFString.h"

// libswiftCore's _SwiftCreateBridgedString, which _NSToDoAtProcessStart calls once to create (and release) a
// bridged string. The port has no Swift runtime; a string of the same bytes and CF encoding stands for it.
id _SwiftCreateBridgedString(const char *bytes, NSUInteger length, uint32_t encoding) {
    return (id)OCFStringCreateWithBytes(
        kOCFAllocatorSystemDefault, (const UInt8 *)bytes, (OCFIndex)length, encoding, false
    );
}

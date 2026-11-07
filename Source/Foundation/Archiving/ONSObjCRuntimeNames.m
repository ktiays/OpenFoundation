//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Archiving/ONSObjCRuntimeNames.h"

#import "Foundation/Archiving/ONSArchivingPrivate.h"
#import "Foundation/String/ONSLog.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#include "CoreFoundation/String/OCFString.h"

#include <dispatch/dispatch.h>

#include <objc/runtime.h>
#include <string.h>

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. MRC: the Copy functions return +1, NSStringFromClass autoreleases its new string.
// iOS Device divergence: these functions' iOS Device 24A446 bodies remain unread.
// Reading: Source/Foundation/Archiving/Reading.md.

// Native NSStringFromSelector, 0x1c057e724. External.
// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. iOS Device 24A446: unread. Reading:
// Source/Foundation/String/FoundationAttributedStorageReading.md.
id ONSStringFromSelector(SEL selector) {
    if (selector == NULL) {
        return nil;
    }
    // The old-SDK path treats the selector register as an object pointer when its sign bit is set.
    uintptr_t selectorBits;
    memcpy(&selectorBits, &selector, sizeof(selectorBits));
    OCFBuildVersion sdk = {UINT32_MAX, 0x07E90000};
    if ((OF_WORD_SHIFT(selectorBits, 63)) != 0 && !dyld_program_sdk_at_least(sdk)) {
        static dispatch_once_t warnOnce;
        // Native ___NSStringFromSelector_block_invoke, 0x1c057e800. Non-external.
        // Clang does not recognize the restored constant object as a fixed format string.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
        dispatch_once(&warnOnce, ^{
          ONSLog((id)OF_STR(
              "Calling NSStringFromSelector with an object as the argument is an error, "
              "and will crash in a future release of the operating system. Call it with a SEL "
              "as the argument instead. This warning is displayed only once."
          ));
        });
#pragma clang diagnostic pop
        return (id)selectorBits;
    }
    const char *name = sel_getName(selector);
    if (name == NULL) {
        return nil;
    }
    size_t length = strlen(name);
    if (_OCFBytesInASCII(name, length)) {
        id tagged = (id)_OCFStringCreateIndirectASCIITaggedPointerString((const UInt8 *)name, length);
        if (tagged != nil) {
            return tagged;
        }
    }
    id string = objc_alloc((Class)archivingStringClass);
    return objc_autorelease([string initWithBytesNoCopy:(void *)name length:length encoding:4
                                           freeWhenDone:NO]);
}

// Native NSCopyStringFromSelector, 0x1c057e81c. Non-external (was a private external).
id NSCopyStringFromSelector(SEL selector) {
    if (selector == NULL) {
        return nil;
    }
    const char *name = sel_getName(selector);
    if (name == NULL) {
        return nil;
    }
    id string = objc_alloc((Class)archivingStringClass);
    size_t length = strlen(name);
    return [string initWithBytesNoCopy:(void *)name length:length encoding:4 freeWhenDone:NO];
}

// Native NSSelectorFromString, 0x1c057e880. External. The fast path compares UTF-8 bytes with UTF-16 length;
// the slow path checks every code unit for NUL before registering the UTF8String (kept).
SEL ONSSelectorFromString(id name) {
    if (name == nil) {
        return NULL;
    }
    NSUInteger length = [name length];
    char buffer[1000];
    if ([name getCString:buffer maxLength:1000 encoding:4] && strlen(buffer) == length) {
        return sel_registerName(buffer);
    }
    for (NSUInteger index = 0; index != length; ++index) {
        if ([name characterAtIndex:index] == 0) {
            return NULL;
        }
    }
    return sel_registerName([name UTF8String]);
}

// Native NSStringFromClass, 0x1c057ead8. External. This copies the C name; NSCopyStringFromClass does not.
id ONSStringFromClass(Class cls) {
    if (cls == Nil) {
        return nil;
    }
    const char *name = class_getName(cls);
    if (name == NULL) {
        return nil;
    }
    id string = objc_alloc((Class)archivingStringClass);
    return objc_autorelease([string initWithCString:name encoding:4]);
}

// Native NSCopyStringFromClass, 0x1c057eb28. Non-external (was a private external).
id NSCopyStringFromClass(Class cls) {
    if (cls == Nil) {
        return nil;
    }
    const char *name = class_getName(cls);
    if (name == NULL) {
        return nil;
    }
    id string = objc_alloc((Class)archivingStringClass);
    size_t length = strlen(name);
    return [string initWithBytesNoCopy:(void *)name length:length encoding:4 freeWhenDone:NO];
}

// Native NSClassFromString, 0x1c057eb8c. External. objc_lookUpClass, not objc_getClass: no missing-class
// hook.
Class ONSClassFromString(id name) {
    if (name == nil) {
        return Nil;
    }
    NSUInteger length = [name length];
    char buffer[1000];
    if ([name getCString:buffer maxLength:1000 encoding:4] && strlen(buffer) == length) {
        return objc_lookUpClass(buffer);
    }
    for (NSUInteger index = 0; index != length; ++index) {
        if ([name characterAtIndex:index] == 0) {
            return Nil;
        }
    }
    return objc_lookUpClass([name UTF8String]);
}

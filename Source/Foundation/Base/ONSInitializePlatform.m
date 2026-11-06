//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Base/ONSInitializePlatform.h"

#import "Foundation/Archiving/ONSArchivingPrivate.h"
#import "Foundation/ProcessInfo/ONSProcessInfo.h"
#import "Foundation/String/ONSLog.h"
#import "Foundation/String/ONSStringOtherEncodings.h"
#import "Foundation/Thread/ONSThread.h"

#include <dlfcn.h>
#include <objc/runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

// Foundation, iOS Simulator 27.0 (24A434), arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured IDA assembly. iOS Device divergence: iOS Device 24A446 is unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json.

extern int kdebug_trace(uint32_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);

// Native imports from libswiftCore and libSystem, respectively.
extern id _SwiftCreateBridgedString(const char *bytes, NSUInteger length, uint32_t encoding);
typedef bool (*CFLogFormatter)(id object, char *buffer, size_t capacity, uint8_t *privateFlag);
extern void _os_log_set_nscf_formatter(CFLogFormatter formatter);

extern char startupThreadClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSThread));
extern char startupProcessInfoClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSProcessInfo));

static uint8_t initializePlatformInited;
uint8_t ONSDebugEnabled;
uint8_t ONSZombieEnabled;
uint8_t ONSDeallocateZombies;
uint8_t _NSDoAPCache = 1;
uint8_t _NSDOLoggingEnabled;
uint8_t _NSDOLoggingEnabled2;

// Native __NSSetupDispatchDataBridge.onceToken.
static dispatch_once_t dispatchDataBridgeToken;

// Native +[NSObject(NSObject) load], 0x1c0582e6c. Non-external.
// Only the isolated naming mode may install the category on libobjc's shared root class, and it does not yet:
// _NSToDoAtProcessStart calls libswiftCore's _SwiftCreateBridgedString, whose bridging classes make
// libswiftCore lazily load the system CoreFoundation, which collides with the restored one (its
// tagged-pointer classes). Open issue, 2026-11-06 (Documentation/Reference/Scope.md, "Landing on main"): the
// load-time entry stays uninstalled until that libswiftCore boundary is resolved; the definitions below are
// built in both modes.
#define OF_FOUNDATION_LOAD_INITIALIZER 0
#if OF_SYSTEM_NAMES && OF_FOUNDATION_LOAD_INITIALIZER
@interface NSObject (NSObject)
@end

@implementation NSObject (NSObject)

+ (void)load {
    _NSInitializePlatform();
}

@end
#endif

// Native __NSInitializePlatform, 0x1c059265c. Non-external (was a private external).
void _NSInitializePlatform(void) {
    if ((initializePlatformInited & 1) != 0) {
        return;
    }
    initializePlatformInited = 1;
    __OCFInitialize();
    const char *value = getenv(
        "NS"
        "DebugEnabled"
    );
    if (value != NULL && ((uint8_t)*value | 0x20) == 'y') {
        ONSDebugEnabled = 1;
    }
    value = getenv(
        "NS"
        "ZombieEnabled"
    );
    if (value != NULL && ((uint8_t)*value | 0x20) == 'y') {
        ONSZombieEnabled = 1;
    }
    value = getenv(
        "NS"
        "DeallocateZombies"
    );
    if (value != NULL && ((uint8_t)*value | 0x20) == 'y') {
        ONSDeallocateZombies = 1;
    }
    value = getenv("NSDisableAutoreleasePoolCache");
    if (value != NULL && ((uint8_t)*value | 0x20) == 'y') {
        _NSDoAPCache = 0;
    }
    __NSSetCStringCharToUnichar(NULL);
    _NSToDoAtProcessStart();
    _os_log_set_nscf_formatter(_ONS_os_log_callback);
    value = getenv("NSDOLoggingEnabled");
    if (value != NULL && ((uint8_t)*value | 0x20) == 'y') {
        _NSDOLoggingEnabled = 1;
        _NSDOLoggingEnabled2 = 1;
    }
    value = getenv("NSUnbufferedIO");
    if (value != NULL && ((uint8_t)*value | 0x20) == 'y') {
        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);
    }
    [(id)startupThreadClass currentThread];
    _NSSetupDispatchDataBridge();
    [[(id)startupProcessInfoClass processInfo] arguments];
    if (*(const volatile uint8_t *)UINT64_C(0xfffffc342) != 0) {
        kdebug_trace(0x1e030005, 0, 0, 0, 0);
        void *extension = dlopen(
            "/System/Cryptexes/Supplemental/Frameworks/ERFoundationExtension.dylib", RTLD_LAZY | RTLD_LOCAL
        );
        if (extension != NULL) {
            dlclose(extension);
        }
        kdebug_trace(0x1e030006, 0, 0, 0, 0);
    }
}

// Native __NSToDoAtProcessStart, 0x1c05a1888. Non-external (was a private external).
void _NSToDoAtProcessStart(void) {
    __NSCFStringClass = objc_lookUpClass(OF_NAME_STRING(__ONSCFString));
    __NSMutableStringClass = objc_lookUpClass(OF_NAME_STRING(ONSMutableString));
    __NSStringClass = objc_lookUpClass(OF_NAME_STRING(ONSString));
    atomic_store_explicit(
        &__NSConcreteValueClass,
        (uintptr_t)objc_lookUpClass(OF_NAME_STRING(ONSConcreteValue)),
        memory_order_relaxed
    );
    __NSMutableStringClass = objc_lookUpClass(OF_NAME_STRING(ONSMutableString));
    class_setVersion(__NSStringClass, 1);
    class_setVersion(__NSMutableStringClass, 1);
    class_setVersion(objc_lookUpClass(OF_NAME_STRING(ONSTimeZone)), 0);
    class_setVersion(objc_lookUpClass(OF_NAME_STRING(ONSCountedSet)), 1);
    class_setVersion(objc_lookUpClass(OF_NAME_STRING(ONSNumberFormatter)), 4);
    class_setVersion(objc_lookUpClass(OF_NAME_STRING(ONSDateFormatter)), 41);
    class_setVersion(objc_lookUpClass(OF_NAME_STRING(ONSAffineTransform)), 1);
    objc_release(_SwiftCreateBridgedString("a", 1, 0x600));
    Class errorClass = objc_lookUpClass(OF_NAME_STRING(ONSError));
    Class nativeSwiftError = objc_lookUpClass("__SwiftNativeNSError");
    if (nativeSwiftError != Nil) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        class_setSuperclass(nativeSwiftError, errorClass);
#pragma clang diagnostic pop
    }
}

// Native __NSSetupDispatchDataBridge, 0x1c06be574. Non-external (was a private external).
void _NSSetupDispatchDataBridge(void) {
    if (dispatchDataBridgeToken != -1) {
        // Native ____NSSetupDispatchDataBridge_block_invoke, 0x1c06be59c. Non-external. Global block literal
        // 0x22281e0d0. Restored by the coordinator from the FX-swift-and-functions capture.
        dispatch_once(&dispatchDataBridgeToken, ^{
          Class dispatchData = objc_getClass("OS_dispatch_data");
          if (dispatchData != Nil) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
              class_setSuperclass(dispatchData, objc_getClass("_NSDispatchData"));
#pragma clang diagnostic pop
          } else {
              syslog(
                  3,
                  "Foundation could not locate dispatch_data_t's Objective-C class. Bridging will not be in "
                  "effect, and attempts to use it will crash"
              );
          }
        });
    }
}

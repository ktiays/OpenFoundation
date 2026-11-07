//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// _NSSwiftProcessInfo for the wasm port: Source/Foundation/ProcessInfo/ProcessInfo.swift translated to
// Objective-C (manual reference counting). The Swift restoration cites the native functions (Foundation
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03, iOS Simulator 24A434); each member below names the Swift member it
// translates. Mutex<State> becomes an os_unfair_lock over the same fields. The class methods the Objective-C
// side sends (+updateThermalState:, +updatePowerState:) take NSInteger as their callers declare them: arm64
// passes NSInteger and UInt64 in one register, wasm's typed calls need the caller's type.

#import "CoreFoundation/Base/OCFPlatform.h"
#import "Foundation/Base/ONSPlatformShims.h"
#import "Foundation/ProcessInfo/ONSProcessInfo.h"
#import "Foundation/String/ONSPathUtilities.h"
#import "Foundation/String/ONSString.h"

#include <crt_externs.h>
#include <objc/runtime.h>
#include <notify.h>
#include <os/lock.h>
#include <stdlib.h>
#include <string.h>

@protocol SwiftProcessInfoMessages
+ (id)arrayWithObjects:(const id *)objects count:(NSUInteger)count;
+ (id)dictionaryWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count;
- (id)copy;
@end

// _ProcessInfo's state: the process name and arguments, computed once.
static os_unfair_lock gProcessInfoLock = OS_UNFAIR_LOCK_INIT;
static id gProcessName;
static id gArguments;

// _ONSSwiftProcessInfo's GlobalState: the cached thermal and power states and their notify tokens.
static os_unfair_lock gGlobalStateLock = OS_UNFAIR_LOCK_INIT;
static bool gHasPowerState;
static bool gPowerState;
static int gPowerToken;
static bool gPowerNotificationCancelled = true;
static int gThermalToken;
static bool gThermalNotificationCancelled = true;
static bool gHasThermalState;
static NSInteger gThermalState;

static id standardizedProcessPath(void) {
    const char *path = _OCFProcessPath();
    if (path == NULL) {
        return nil;
    }
    return [[ONSString stringWithUTF8String:path] stringByStandardizingPath];
}

@interface _ONSSwiftProcessInfo : ONSProcessInfo
@end

@implementation _ONSSwiftProcessInfo

// static let _shared, and the @objc processInfo().
+ (id)processInfo {
    static _ONSSwiftProcessInfo *shared;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
      shared = [[_ONSSwiftProcessInfo allocWithZone:NULL] init];
    });
    return shared;
}

// _ProcessInfo.arguments: CommandLine.arguments with the first replaced by the standardized process path.
- (id)arguments {
    os_unfair_lock_lock(&gProcessInfoLock);
    if (gArguments == nil) {
        int count = *_NSGetArgc();
        char **argv = *_NSGetArgv();
        id standardized = standardizedProcessPath();
        NSUInteger total = count > 0 ? (NSUInteger)count : (standardized != nil ? 1 : 0);
        id *objects = calloc(total == 0 ? 1 : total, sizeof(id));
        for (NSUInteger index = 0; index < total; index++) {
            objects[index] = index < (NSUInteger)count ? [ONSString stringWithUTF8String:argv[index]] : nil;
        }
        if (standardized != nil) {
            objects[0] = standardized;
        }
        gArguments = [[(id)objc_getClass(OF_NAME_STRING(ONSArray)) arrayWithObjects:objects
                                                                              count:total] retain];
        free(objects);
    }
    id arguments = gArguments;
    os_unfair_lock_unlock(&gProcessInfoLock);
    return arguments;
}

// _ProcessInfo.processName: the standardized process path's last component, or the empty string.
- (id)processName {
    os_unfair_lock_lock(&gProcessInfoLock);
    if (gProcessName == nil) {
        id standardized = standardizedProcessPath();
        gProcessName = [(standardized != nil ? [standardized lastPathComponent] : @"") copy];
    }
    id processName = gProcessName;
    os_unfair_lock_unlock(&gProcessInfoLock);
    return processName;
}

// _ProcessInfo.environment: every "name=value" entry of environ, copied under the environment lock.
- (id)environment {
    _platform_shims_lock_environ();
    char **environment = _platform_shims_get_environ();
    if (environment == NULL) {
        _platform_shims_unlock_environ();
        return [(id)objc_getClass(OF_NAME_STRING(ONSDictionary)) dictionaryWithObjects:NULL
                                                                               forKeys:NULL
                                                                                 count:0];
    }
    NSUInteger count = 0;
    while (environment[count] != NULL) {
        count++;
    }
    char **copies = calloc(count == 0 ? 1 : count, sizeof(char *));
    for (NSUInteger index = 0; index < count; index++) {
        copies[index] = strdup(environment[index]);
    }
    _platform_shims_unlock_environ();
    id *keys = calloc(count == 0 ? 1 : count, sizeof(id));
    id *values = calloc(count == 0 ? 1 : count, sizeof(id));
    NSUInteger used = 0;
    for (NSUInteger index = 0; index < count; index++) {
        char *separator = strchr(copies[index], '=');
        if (separator == NULL) {
            continue;
        }
        *separator = '\0';
        keys[used] = [ONSString stringWithUTF8String:copies[index]];
        values[used] = [ONSString stringWithUTF8String:separator + 1];
        used++;
    }
    id result = [(id)objc_getClass(OF_NAME_STRING(ONSDictionary)) dictionaryWithObjects:values
                                                                                forKeys:keys
                                                                                  count:used];
    for (NSUInteger index = 0; index < count; index++) {
        free(copies[index]);
    }
    free(copies);
    free(keys);
    free(values);
    return result;
}

// static cancelThermalStateNotifyNotification().
+ (void)cancelThermalStateNotifyNotification {
    os_unfair_lock_lock(&gGlobalStateLock);
    if (!gThermalNotificationCancelled) {
        notify_cancel(gThermalToken);
        gThermalToken = 0;
        gThermalNotificationCancelled = true;
    }
    os_unfair_lock_unlock(&gGlobalStateLock);
}

// static updateThermalState(_:): 10 fair, 20 serious, 30-50 critical, anything else nominal.
+ (void)updateThermalState:(NSInteger)state {
    NSInteger thermalState;
    switch ((uint32_t)state) {
        case 10:
            thermalState = 1;
            break;
        case 20:
            thermalState = 2;
            break;
        case 30:
        case 40:
        case 50:
            thermalState = 3;
            break;
        default:
            thermalState = 0;
            break;
    }
    os_unfair_lock_lock(&gGlobalStateLock);
    gHasThermalState = true;
    gThermalState = thermalState;
    os_unfair_lock_unlock(&gGlobalStateLock);
}

// isLowPowerModeEnabled(): the cached power state, or the state of com.apple.system.lowpowermode after
// registering for its changes on the generic queue matching the main thread's QoS.
- (BOOL)isLowPowerModeEnabled {
    os_unfair_lock_lock(&gGlobalStateLock);
    if (gHasPowerState) {
        bool enabled = gPowerState;
        os_unfair_lock_unlock(&gGlobalStateLock);
        return enabled;
    }
    int token = 0;
    uint32_t status = notify_register_dispatch(
        "com.apple.system.lowpowermode", &token, __NSDispatchQueueGetGenericMatchingMain(), ^(int changed) {
          uint64_t value = 0;
          if (notify_get_state(changed, &value) != 0) {
              return;
          }
          os_unfair_lock_lock(&gGlobalStateLock);
          gPowerToken = changed;
          gPowerNotificationCancelled = false;
          gHasPowerState = true;
          gPowerState = value != 0;
          os_unfair_lock_unlock(&gGlobalStateLock);
        }
    );
    uint64_t value = 0;
    if (status != 0 || notify_get_state(token, &value) != 0) {
        os_unfair_lock_unlock(&gGlobalStateLock);
        return NO;
    }
    gHasPowerState = true;
    gPowerState = value != 0;
    bool enabled = gPowerState;
    os_unfair_lock_unlock(&gGlobalStateLock);
    return enabled;
}

// static cancelPowerStateNotifyNotification().
+ (void)cancelPowerStateNotifyNotification {
    os_unfair_lock_lock(&gGlobalStateLock);
    if (!gPowerNotificationCancelled) {
        notify_cancel(gPowerToken);
        gPowerToken = 0;
        gPowerNotificationCancelled = true;
    }
    os_unfair_lock_unlock(&gGlobalStateLock);
}

// static updatePowerState(_:).
+ (void)updatePowerState:(NSInteger)state {
    os_unfair_lock_lock(&gGlobalStateLock);
    gHasPowerState = true;
    gPowerState = state != 0;
    os_unfair_lock_unlock(&gGlobalStateLock);
}

@end

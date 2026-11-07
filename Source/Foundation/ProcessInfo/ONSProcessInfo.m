//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/ProcessInfo/ONSProcessInfo.h"

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/String/OCFString.h"
#import "Foundation/String/ONSStringOtherEncodings.h"

#include <crt_externs.h>
#include <malloc/malloc.h>
#include <notify.h>
#include <stdatomic.h>
#include <dispatch/dispatch.h>
#include <os/lock.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

// Inferred organization: the NSProcessInfo methods (0x1c05a19f8-0x1c05a2bc0, the NSProcessInfoHardwareState
// category among them) lie together in the iOS Simulator __text section; the native source path is not in the
// evidence. Objective-C compiled with manual reference counting natively. Unless a definition says otherwise:
// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified
// (assembly read in full); iOS Device 24A446: unread. Reading: Local/Scratch/cf-core-fd-misc/Reading.md;
// original assembly in Local/Evidence/cf-core/iphone-simulator-foundation-link.json (family FM-misc).

// The classes this unit references by symbol: NSProcessInfo itself where the native code compares its class
// reference (___SCR_NSProcessInfo), _NSSwiftProcessInfo, NSString (Foundation's, the GOT slot 0x22289f3d8)
// and NSMutableArray (CoreFoundation's; the uniqued GOT slot 0x22227fb18 lies outside the image and is bound
// by the selectors sent and the slots' alphabetical order; confidence: medium).
extern char processInfoClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSProcessInfo));
extern char swiftProcessInfoClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(_ONSSwiftProcessInfo));
extern char stringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSString));
extern char mutableArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableArray));
// CoreFoundation's NSDictionary and NSMutableDictionary (uniqued GOT slots 0x22227fad0 and 0x22227fb28, bound
// by their selectors; confidence: medium) and Foundation's NSNotificationCenter.
extern char processInfoDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSDictionary));
extern char processInfoMutableDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                          OF_NAME_STRING(ONSMutableDictionary));
extern char processInfoNotificationCenterClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                           OF_NAME_STRING(ONSNotificationCenter));

// Selectors this unit sends to objects of classes outside it (NSString, NSMutableArray and
// _NSSwiftProcessInfo), declared for the message sends only; no protocol object is emitted.
@protocol ProcessInfoMessages

+ (id)array;
+ (id)stringWithCString:(const char *)cString encoding:(NSUInteger)encoding;
- (id)initWithBytes:(const void *)bytes length:(NSUInteger)length encoding:(NSUInteger)encoding;
- (id)stringByStandardizingPath;
- (void)addObject:(id)object;
- (NSUInteger)count;
- (void)replaceObjectAtIndex:(NSUInteger)index withObject:(id)object;
- (id)copy;
- (BOOL)isLowPowerModeEnabled;
+ (id)dictionary;
+ (id)dictionaryWithDictionary:(id)dictionary;
- (id)initWithCapacity:(NSUInteger)capacity;
- (id)objectForKey:(id)key;
- (void)setObject:(id)object forKey:(id)key;
+ (id)stringWithBytes:(const void *)bytes length:(NSUInteger)length encoding:(NSUInteger)encoding;
+ (void)cancelThermalStateNotifyNotification;
+ (void)cancelPowerStateNotifyNotification;
+ (void)updateThermalState:(NSInteger)state;
+ (void)updatePowerState:(NSInteger)state;
+ (id)defaultCenter;
- (void)postNotificationName:(id)name object:(id)object;

@end

// libsystem's thermal notification name (Libc's OSThermalNotification; the simulator SDK installs no header).
extern const char *const kOSThermalNotificationName;

// libsystem's environ lock (private; no SDK declaration).
extern void environ_lock_np(void);
extern void environ_unlock_np(void);
extern void objc_release(id object);

static void freeenv(char **copies, NSUInteger count);

// Native once predicates/values at 0x22e76c9a8/0x22e76c989 and 0x22e76c990/0x22e76c998.
// Their block invokes were independently captured through IDA on 2026/11/03 (additional-ida.json).
static dispatch_once_t swiftProcessInfoEnabledOnce;
static uint8_t swiftProcessInfoEnabled;
static dispatch_once_t processInfoOnce;
static id sharedProcessInfo;
extern bool _os_feature_enabled_impl(const char *domain, const char *feature);

// libsystem_platform's os_unfair_lock_lock_with_options (private SPI, no SDK declaration); supplied. The
// native callers pass 0x10000 (OS_UNFAIR_LOCK_DATA_SYNCHRONIZATION).
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);

// The lock of the lazily computed ivars (_processLock, __DATA,__bss 0x22e6fcc20, non-external).
static os_unfair_lock processLock = OS_UNFAIR_LOCK_INIT;

@implementation ONSProcessInfo

// Native +[NSProcessInfo allocWithZone:], 0x1c05a19f8. External class, non-external method. When the Swift
// process info is enabled (the once flag equals 1) and the receiver is NSProcessInfo itself, +allocWithZone:
// of _NSSwiftProcessInfo (a tail call); otherwise the superclass's (NSObject's) +allocWithZone:.
+ (id)allocWithZone:(struct _NSZone *)zone {
    dispatch_once(&swiftProcessInfoEnabledOnce, ^{
      // Native ____foundation_swift_process_info_enabled_block_invoke, 0x1c05a2c24. Non-external.
      if ((_os_feature_enabled_impl("Foundation", "NSSwiftProcessInfo") & 1) != 0) {
          swiftProcessInfoEnabled = 1;
      } else {
          swiftProcessInfoEnabled = getenv("NSSwiftProcessInfo") != NULL;
      }
    });
    if (swiftProcessInfoEnabled == 1 && self == (Class)processInfoClass) {
        return [(id)swiftProcessInfoClass allocWithZone:zone];
    }
    return [super allocWithZone:zone];
}

// Native +[NSProcessInfo processInfo], 0x1c05a1bcc. NSProcessInfo itself answers +processInfo of
// _NSSwiftProcessInfo (a tail call); any other receiver the instance its once block creates.
+ (id)processInfo {
    if (self == (Class)processInfoClass) {
        return [(id)swiftProcessInfoClass processInfo];
    }
    dispatch_once(&processInfoOnce, ^{
      // Native ___28+[NSProcessInfo processInfo]_block_invoke, 0x1c05a1c28. Non-external.
      sharedProcessInfo = [[(id)processInfoClass allocWithZone:NULL] init];
    });
    return sharedProcessInfo;
}

// Native -[NSProcessInfo dealloc], 0x1c05a1c50. Releases environment, arguments, hostName and name (offsets 8
// to 32), then [super dealloc].
- (void)dealloc {
    [environment release];
    [arguments release];
    [hostName release];
    [name release];
    [super dealloc];
}

// Native -[NSProcessInfo environment], 0x1c05a1d2c. External class, non-external method. The cached
// dictionary when there is one (never set by this method); otherwise a new immutable dictionary from a copy
// of environ taken under environ_lock_np: each "name=value" entry (entries without '=' are skipped) decoded
// as UTF-8 or, failing that, the default C string encoding, the first occurrence of a name winning. The copy
// starts at 32 entries and doubles; an overflowing capacity answers an empty dictionary.
- (id)environment {
    if (environment != nil) {
        return environment;
    }
    environ_lock_np();
    char **variables = *_NSGetEnviron();
    if (variables == NULL) {
        environ_unlock_np();
        return [(id)processInfoDictionaryClass dictionary];
    }
    char **copies = malloc_type_malloc(0x100, 0x10040436913F5);
    NSUInteger count = 0;
    char *variable = variables[0];
    if (variable != NULL) {
        NSUInteger capacity = 32;
        while (1) {
            if (count >= capacity) {
                if ((NSInteger)capacity < 0) {
                    environ_unlock_np();
                    freeenv(copies, count);
                    return [(id)processInfoDictionaryClass dictionary];
                }
                copies = malloc_type_realloc(copies, 16 * capacity, 0x10040436913F5);
                capacity *= 2;
                variable = variables[count];
            }
            copies[count] = strdup(variable);
            variable = variables[++count];
            if (variable == NULL) {
                environ_unlock_np();
                break;
            }
        }
    } else {
        environ_unlock_np();
        if (copies == NULL) {
            return [(id)processInfoDictionaryClass dictionary];
        }
    }
    id dictionary = [[(id)processInfoMutableDictionaryClass alloc] initWithCapacity:count];
    for (NSUInteger index = 0; index != count; ++index) {
        const char *entry = copies[index];
        NSUInteger nameLength = 0;
        while (entry[nameLength] != 0 && entry[nameLength] != '=') {
            ++nameLength;
        }
        size_t rest = strlen(entry + nameLength);
        if (rest == 0) {
            continue;
        }
        id key = [(id)stringClass stringWithBytes:entry length:nameLength encoding:4];
        if (key == nil) {
            NSUInteger encoding = __ONSDefaultStringEncodingFullyInited == 1 ? _ONSDefaultStringEncoding
                                                                             : _ONSDefaultCStringEncoding();
            key = [(id)stringClass stringWithBytes:entry length:nameLength encoding:encoding];
        }
        id value = [(id)stringClass stringWithBytes:entry + nameLength + 1 length:rest - 1 encoding:4];
        if (value == nil) {
            NSUInteger encoding = __ONSDefaultStringEncodingFullyInited == 1 ? _ONSDefaultStringEncoding
                                                                             : _ONSDefaultCStringEncoding();
            value = [(id)stringClass stringWithBytes:entry + nameLength + 1
                                              length:rest - 1
                                            encoding:encoding];
        }
        if (key != nil && value != nil && [dictionary objectForKey:key] == nil) {
            [dictionary setObject:value forKey:key];
        }
    }
    id result = [(id)processInfoDictionaryClass dictionaryWithDictionary:dictionary];
    objc_release(dictionary);
    freeenv(copies, count);
    return result;
}

// Native freeenv, 0x1c05a1fc8. Non-external. Frees the count copied entries, then the list.
static void freeenv(char **copies, NSUInteger count) {
    for (NSUInteger index = 0; index != count; ++index) {
        free(copies[index]);
    }
    free(copies);
}

// Native -[NSProcessInfo arguments], 0x1c05a200c. Under the process lock (taken with option 0x10000),
// computed once: the process path (_CFProcessPath) as a string (CFStringCreateWithFileSystemRepresentation,
// autoreleased; failing that NSString +allocWithZone: nil and -initWithBytes:length:encoding: with the
// default C-string encoding, autoreleased; failing that ""), standardized (-stringByStandardizingPath); then
// every non-NULL argv entry (_NSGetArgv, _NSGetArgc, argc compared signed with 1) as UTF-8 (4) or else ISO
// Latin 1 (5) is added to +[NSMutableArray array]. The standardized path replaces element 0, or is added to
// an empty array; the array's copy is stored. The ivar is returned unretained.
- (id)arguments {
    os_unfair_lock_lock_with_options(&processLock, 0x10000);
    id result = arguments;
    if (result == nil) {
        const char *path = _OCFProcessPath();
        id string = [(id)OCFStringCreateWithFileSystemRepresentation(NULL, path) autorelease];
        if (string == nil) {
            id allocated = [(id)stringClass allocWithZone:nil];
            size_t length = strlen(path);
            NSUInteger encoding;
            if (__ONSDefaultStringEncodingFullyInited == 1) {
                encoding = _ONSDefaultStringEncoding;
            } else {
                encoding = _ONSDefaultCStringEncoding();
            }
            string = [[allocated initWithBytes:path length:length encoding:encoding] autorelease];
        }
        if (string == nil) {
            string = (id)OF_STR("");
        }
        id standardizedPath = [string stringByStandardizingPath];
        id array = [(id)mutableArrayClass array];
        char **argv = *_NSGetArgv();
        int argc = *_NSGetArgc();
        if (argc >= 1) {
            uint64_t remaining = (uint32_t)argc;
            do {
                if (*argv != NULL) {
                    id argument = [(id)stringClass stringWithCString:*argv encoding:4];
                    if (argument == nil) {
                        argument = [(id)stringClass stringWithCString:*argv encoding:5];
                    }
                    if (argument != nil) {
                        [array addObject:argument];
                    }
                }
                argv++;
                remaining--;
            } while (remaining != 0);
        }
        if ([array count] != 0) {
            [array replaceObjectAtIndex:0 withObject:standardizedPath];
        } else {
            [array addObject:standardizedPath];
        }
        result = [array copy];
        arguments = result;
    }
    os_unfair_lock_unlock(&processLock);
    return result;
}

// Native -[NSProcessInfo processName], 0x1c05a224c. Under the process lock: the name ivar, computed once from
// _CFGetProgname's string (CFStringCreateWithFileSystemRepresentation, retained by the ivar), copied; after
// the unlock the copy is autoreleased (a tail call).
- (id)processName {
    os_unfair_lock_lock_with_options(&processLock, 0x10000);
    id current = name;
    if (current == nil) {
        current = (id)OCFStringCreateWithFileSystemRepresentation(NULL, *_OCFGetProgname());
        name = current;
    }
    id copy = [current copy];
    os_unfair_lock_unlock(&processLock);
    return [copy autorelease];
}

@end

// Native _NSProcessInfoGetPowerState.once and .powerState (__DATA,__bss): the registration once and the
// low power mode state (0 or 1; -1 after a notify failure).
static dispatch_once_t powerStateOnce;
static _Atomic NSInteger powerState;

// Native _NSProcessInfoGetThermalState.once and .currentState (__DATA,__bss).
static dispatch_once_t thermalStateOnce;
static _Atomic NSInteger thermalState;

// Native __NSProcessInfoPowerStateNotificationName, 0x1c100dd00 (__TEXT,__const, non-external).
static const char _NSProcessInfoPowerStateNotificationName[] = "com.apple.system.lowpowermode";

static void NSProcessInfoNotifyThermalState(int token);
static void NSProcessInfoNotifyPowerState(int token);

// Native __NSProcessInfoGetPowerState, 0x1c05a2b7c. Hidden (was a private external). Registers for the low
// power mode notification once (the global block ____NSProcessInfoGetPowerState_block_invoke, 0x1c05a2e68),
// then answers the last state.
NSInteger _NSProcessInfoGetPowerState(void) {
    dispatch_once(&powerStateOnce, ^{
      // Native ____NSProcessInfoGetPowerState_block_invoke, 0x1c05a2e68. Non-external. On a successful
      // registration Swift's own notification is cancelled and the state read now.
      int token = 0;
      dispatch_queue_t queue = dispatch_get_global_queue(qos_class_main(), 2);
      uint32_t status =
          notify_register_dispatch(_NSProcessInfoPowerStateNotificationName, &token, queue, ^(int changed) {
            // Native ____NSProcessInfoGetPowerState_block_invoke_2, 0x1c05a2ef4. Non-external.
            NSProcessInfoNotifyPowerState(changed);
          });
      if (status == 0) {
          [(id)swiftProcessInfoClass cancelPowerStateNotifyNotification];
          NSProcessInfoNotifyPowerState(token);
      }
    });
    return atomic_load(&powerState);
}

@implementation ONSProcessInfo (NSProcessInfoHardwareState)

// Native -[NSProcessInfo(NSProcessInfoHardwareState) isLowPowerModeEnabled], 0x1c05a2bc0. External class,
// non-external method. Forwarded to +processInfo of _NSSwiftProcessInfo (a tail call).
- (BOOL)isLowPowerModeEnabled {
    return [[(id)swiftProcessInfoClass processInfo] isLowPowerModeEnabled];
}

@end

// Native __NSProcessInfoGetThermalState, 0x1c05a2c70. Hidden (was a private external). Registers for the
// thermal notification once, then answers the last state.
NSInteger _NSProcessInfoGetThermalState(void) {
    dispatch_once(&thermalStateOnce, ^{
      // Native ____NSProcessInfoGetThermalState_block_invoke, 0x1c05a2cb4. Non-external. The notification
      // name is libsystem's kOSThermalNotificationName (GOT slot 0x222418540; confidence: medium).
      int token = 0;
      dispatch_queue_t queue = dispatch_get_global_queue(qos_class_main(), 2);
      uint32_t status = notify_register_dispatch(kOSThermalNotificationName, &token, queue, ^(int changed) {
        // Native ____NSProcessInfoGetThermalState_block_invoke_2, 0x1c05a2d50. Non-external.
        NSProcessInfoNotifyThermalState(changed);
      });
      if (status == 0) {
          [(id)swiftProcessInfoClass cancelThermalStateNotifyNotification];
          NSProcessInfoNotifyThermalState(token);
      }
    });
    return atomic_load(&thermalState);
}

// Native NSProcessInfoNotifyThermalState, 0x1c05a2d58. Non-external. Maps the notify state (10 fair, 20
// serious, 30/40/50 critical, otherwise nominal) to NSProcessInfoThermalState, tells _NSSwiftProcessInfo, and
// posts NSProcessInfoThermalStateDidChangeNotification when the state changed; a failed read stores -1.
static void NSProcessInfoNotifyThermalState(int token) {
    uint64_t value = 0;
    if (notify_get_state(token, &value) != 0) {
        atomic_exchange(&thermalState, -1);
        return;
    }
    NSInteger state;
    if ((int64_t)value == 10) {
        state = 1;
    } else if ((int64_t)value == 20) {
        state = 2;
    } else if ((int64_t)value == 30 || (int64_t)value == 40 || (int64_t)value == 50) {
        state = 3;
    } else {
        state = 0;
    }
    [(id)swiftProcessInfoClass updateThermalState:state];
    if (atomic_exchange(&thermalState, state) != state) {
        [[(id)processInfoNotificationCenterClass defaultCenter]
            postNotificationName:(id)OF_STR("NSProcessInfoThermalStateDidChangeNotification")
                          object:[ONSProcessInfo processInfo]];
    }
}

// Native NSProcessInfoNotifyPowerState, 0x1c05a2efc. Non-external. Low power mode is a non-zero notify state;
// _NSSwiftProcessInfo is told and NSProcessInfoPowerStateDidChangeNotification posted when the state changed;
// a failed read stores -1.
static void NSProcessInfoNotifyPowerState(int token) {
    uint64_t value = 0;
    if (notify_get_state(token, &value) != 0) {
        atomic_exchange(&powerState, -1);
        return;
    }
    NSInteger state = value != 0;
    [(id)swiftProcessInfoClass updatePowerState:state];
    if (atomic_exchange(&powerState, state) != state) {
        [[(id)processInfoNotificationCenterClass defaultCenter]
            postNotificationName:(id)OF_STR("NSProcessInfoPowerStateDidChangeNotification")
                          object:[ONSProcessInfo processInfo]];
    }
}

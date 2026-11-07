//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/FileManager/ONSFileManager.h"
#import "Foundation/FileManager/ONSUnrestoredInterfaces.h"
#import "Foundation/Archiving/ONSArchivingPrivate.h"
#import "Foundation/Number/ONSValue.h"
#import "Foundation/String/ONSFileUtilities.h"
#import "Foundation/String/ONSPathUtilities.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"

#import "Foundation/String/ONSLog.h"
#include "CoreFoundation/RunLoop/OCFRunLoop.h"
#include "CoreFoundation/URL/OCFURLPromises.h"

#include <dispatch/dispatch.h>
#include <objc/runtime.h>
#include <os/log.h>
#include <dlfcn.h>
#include <mach/mach.h>
#include <objc/objc-exception.h>
#include <unistd.h>
#include <xpc/xpc.h>

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. iOS Device 24A446: comparison pending.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FB-bundle-paths-plist;
// Swift bridge entry points: Local/Scratch/cf-core-r2-fb/ida-parents-bridges.json.
// Uniqued GOT bindings (medium): NSException 0x22227fae0, NSInvalidArgumentException 0x22227f500,
// mach_task_self_ 0x222419e80, XPC_TYPE_ARRAY 0x22241b968, XPC_TYPE_STRING 0x22241ba28 and
// SANDBOX_CHECK_NO_REPORT 0x22241b290.

extern xpc_object_t xpc_copy_entitlement_for_token(const char *entitlement, audit_token_t *token);
extern void *objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void *pool);

extern char fileManagerClassSymbol[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSFileManager));
extern char fileManagerDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSDictionary));
extern char fileManagerErrorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSError));
extern char fileManagerNotificationCenterClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                           OF_NAME_STRING(ONSNotificationCenter));

// The messages the unit sends to objects of other classes (URLs, errors, dictionaries, weak values and the
// CloudDocs category of NSFileManager), declared for the sends only.
@protocol FileManagerMessages
+ (id)dictionaryWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count;
- (id)initWithDomain:(id)domain code:(NSInteger)code userInfo:(id)userInfo;
- (BOOL)checkResourceIsReachableAndReturnError:(id *)error;
- (BOOL)br_movePromisedItemAtURL:(id)source toURL:(id)destination error:(id *)error;
- (NSInteger)code;
- (id)domain;
- (id)path;
- (BOOL)isEqualToString:(id)string;
- (id)weakObjectValue;
+ (id)defaultCenter;
- (void)postNotificationName:(id)name object:(id)object;
@end

// Native __DATA,__crash_info 0x22e6fc8c8, size 0x148, version 7. The methods write the message at +8.
__attribute__((
    section("__DATA,__crash_info"), used
)) static volatile uint64_t fileManagerCrashAnnotations[41] = {7};

// Native qword_22E76C300 and qword_22E76C308.
static dispatch_once_t defaultManagerOnce;
static ONSFileManager *defaultManager;

// Native process entitlement state, __DATA,__bss.
static dispatch_once_t containerEntitlementOnce;
static BOOL containerEntitlement;
static dispatch_once_t cloudServicesOnce;
static BOOL cloudServices;
static dispatch_once_t ubiquityIdentityOnce;
static BOOL ubiquityIdentity;

static dispatch_once_t librarianFunctionsOnce;
static dispatch_once_t librarianHandleOnce;
static void *librarianHandle;
static void *librarianEvictItemAtURL;
static void *librarianStartSyncForItem;
static void *librarianSetSyncEnabledForItem;
static void *librarianGetURLForPublishedItem;
static void *librarianGetMobileContainerForID;
static void *librarianCopyUbiquityAccountToken;
static void *librarianRefreshUbiquityAccountToken;
static BOOL (*librarianCurrentProcessHasUbiquityContainer)(void);

// The once-initializers natively inlined at each user: _NSFrameworkHandleForLibrarianSymbols (the CloudDocs
// handle, whose global block ____NSFrameworkHandleForLibrarianSymbols_block_invoke is 0x1c050bf48) and
// _initializeLibrarianFunctions (the librarian symbols, global block
// ____initializeLibrarianFunctions_block_invoke 0x1c051419c, which inlines the handle's once check). Each
// native expansion passes the same global block; the restored helpers are local descriptive names for the
// expansions.
static inline void *frameworkHandleForLibrarianSymbols(void) {
    dispatch_once(&librarianHandleOnce, ^{
      // Native ____NSFrameworkHandleForLibrarianSymbols_block_invoke, 0x1c050bf48. Non-external.
      librarianHandle = dlopen("/System/Library/PrivateFrameworks/CloudDocs.framework/CloudDocs", RTLD_FIRST);
    });
    return librarianHandle;
}

static inline void initializeLibrarianFunctions(void) {
    dispatch_once(&librarianFunctionsOnce, ^{
      // Native ____initializeLibrarianFunctions_block_invoke, 0x1c051419c. Non-external.
      void *handle = frameworkHandleForLibrarianSymbols();
      if (handle != NULL) {
          librarianEvictItemAtURL = dlsym(handle, "LBEvictItemAtURL");
          librarianStartSyncForItem = dlsym(handle, "LBStartSyncForItem");
          librarianSetSyncEnabledForItem = dlsym(handle, "LBSetSyncEnabledForItem");
          librarianGetURLForPublishedItem = dlsym(handle, "LBGetURLForPublishedItem");
          librarianGetMobileContainerForID = dlsym(handle, "LBGetMobileContainerForID");
          librarianCopyUbiquityAccountToken = dlsym(handle, "LBCopyUbiquityAccountToken");
          librarianRefreshUbiquityAccountToken = dlsym(handle, "LBRefreshUbiquityAccountToken");
          librarianCurrentProcessHasUbiquityContainer = dlsym(handle, "LBCurrentProcessHasUbiquityContainer");
      }
    });
}

@interface ONSFileManager ()
- (BOOL)_processUsesCloudServices;
@end

@implementation ONSFileManager

// Native -[NSFileManager init], 0x1c050dee4. Non-external.
- (id)init {
    self = [super init];
    if (self != nil) {
        swiftBridge = [[_ONSFileManagerBridge alloc] initWithFileManager:self];
    }
    return self;
}

// Native +[NSFileManager defaultManager], 0x1c050f060. Non-external.
+ (id)defaultManager {
    dispatch_once(&defaultManagerOnce, ^{
      // Native ___31+[NSFileManager defaultManager]_block_invoke, 0x1c050f100. Non-external.
      defaultManager = [objc_allocWithZone((Class)self) init];
    });
    return defaultManager;
}

// Native -[NSFileManager currentDirectoryPath], 0x1c050f124. Non-external. The Swift bridge's answer.
- (id)currentDirectoryPath {
    return [swiftBridge currentDirectoryPath];
}

// Native -[NSFileManager fileExistsAtPath:], 0x1c050f154. Non-external.
- (BOOL)fileExistsAtPath:(id)path {
    return path != nil && [swiftBridge fileExistsAtPath:path];
}

// Native -[NSFileManager getFileSystemRepresentation:maxLength:withPath:], 0x1c050f708. Non-external.
- (BOOL)getFileSystemRepresentation:(char *)buffer maxLength:(ONSUInteger)length withPath:(id)path {
    if (path != nil) {
        return [path getFileSystemRepresentation:buffer maxLength:length];
    }
    *buffer = '\0';
    return NO;
}

// Native -[NSFileManager stringWithFileSystemRepresentation:length:], 0x1c050f728. Non-external.
- (id)stringWithFileSystemRepresentation:(const char *)bytes length:(ONSUInteger)length {
    return _NSFileSystemRepresentationString(bytes, length);
}

// Native -[NSFileManager retain], 0x1c050f8d8. Non-external.
- (id)retain {
    return self == defaultManager ? self : [super retain];
}

// Native -[NSFileManager release], 0x1c050f948. Non-external.
- (oneway void)release {
    if (self != defaultManager) {
        [super release];
    }
}

// Native -[NSFileManager dealloc], 0x1c050f9b8. Non-external.
- (void)dealloc {
    if (self == defaultManager) {
        fileManagerCrashAnnotations[1] =
            (uintptr_t)"You cannot deallocate the default file manager singleton";
        __builtin_arm_trap(1);
    }
    objc_release(_weakDelegateValue);
    objc_release(swiftBridge);
    [super dealloc];
}

// Native -[NSFileManager delegate], 0x1c050fa58. Non-external.
- (id)delegate {
    return _delegate;
}

// Native -[NSFileManager setDelegate:], 0x1c050fa60. Non-external.
- (void)setDelegate:(id)delegate {
    if (delegate != nil && self == defaultManager &&
        dyld_program_sdk_at_least((OCFBuildVersion){UINT32_MAX, 0x07ea0000})) {
        fileManagerCrashAnnotations[1] = (uintptr_t)"You cannot set a delegate on the default file manager. "
                                                    "To use a delegate, create a non-global "
                                                    "instance of a FileManager.";
        __builtin_arm_trap(1);
    }

    ONSValue *old = _weakDelegateValue;
    _delegate = delegate;
    objc_release(old);
    _weakDelegateValue = delegate != nil ? objc_retain([ONSValue valueWithWeakObject:delegate]) : nil;
}

// Native -[NSFileManager _safeDelegate], 0x1c050faf8. Non-external. For programs built with the SDK
// 0x07E40901 or later, when the receiver is exactly NSFileManager or its subclass does not override
// -delegate, the weakly held delegate; otherwise -delegate.
- (id)_safeDelegate {
    if (dyld_program_sdk_at_least((OCFBuildVersion){UINT32_MAX, 0x07e40901})) {
        Class cls = [self class];
        if (cls == (Class)fileManagerClassSymbol ||
            class_getInstanceMethod(cls, @selector(delegate)) ==
                class_getInstanceMethod((Class)fileManagerClassSymbol, @selector(delegate))) {
            return [(id)_weakDelegateValue weakObjectValue];
        }
    }
    return [self delegate];
}

// Native -[NSFileManager _handleFaultedOutCloudDocFromSource:toDestination:handled:error:], 0x1c050fd34.
// Non-external. A source that is a promise for another physical item that is not reachable is moved by
// CloudDocs (-br_movePromisedItemAtURL:toURL:error:, when the default manager responds to it); handled tells
// whether CloudDocs moved it. A failed move answers NO with an NSCocoaErrorDomain error (4 when CloudDocs
// reported Cocoa error 4, else 512) whose user info names both paths, the variant "Move" and the underlying
// error.
- (BOOL)_handleFaultedOutCloudDocFromSource:(id)source
                              toDestination:(id)destination
                                    handled:(BOOL *)handled
                                      error:(id *)error {
    if (handled != NULL) {
        *handled = NO;
    }
    OCFURLRef physical = _OCFURLPromiseCopyPhysicalURL((OCFURLRef)source);
    if (physical == NULL) {
        return YES;
    }
    if ((id)physical == source || [source checkResourceIsReachableAndReturnError:NULL]) {
        OCFRelease(physical);
        return YES;
    }
    frameworkHandleForLibrarianSymbols();
    if (![[ONSFileManager defaultManager]
            respondsToSelector:@selector(br_movePromisedItemAtURL:toURL:error:)]) {
        OCFRelease(physical);
        return YES;
    }
    id underlying = nil;
    if ([[ONSFileManager defaultManager] br_movePromisedItemAtURL:source toURL:destination
                                                            error:&underlying]) {
        if (handled != NULL) {
            *handled = YES;
        }
        OCFRelease(physical);
        return YES;
    }
    NSInteger code = 512;
    if ([underlying code] == 4 &&
        [[underlying domain] isEqualToString:(id)OF_STR(OF_NAME_STRING(ONSCocoaErrorDomain))]) {
        code = 4;
    }
    id keys[4] = {
        (id)OF_STR("NSFilePath"),
        (id)OF_STR("NSDestinationFilePath"),
        (id)OF_STR("NSUserStringVariant"),
        (id)OF_STR("NSUnderlyingError")
    };
    id objects[4] = {[source path], [destination path], (id)OF_STR("Move"), underlying};
    id userInfo = [(id)fileManagerDictionaryClass dictionaryWithObjects:objects forKeys:keys count:4];
    if (error != NULL) {
        *error = objc_autorelease([[(id)fileManagerErrorClass alloc]
            initWithDomain:(id)OF_STR(OF_NAME_STRING(ONSCocoaErrorDomain))
                      code:code
                  userInfo:userInfo]);
    }
    OCFRelease(physical);
    return NO;
}

// Native -[NSFileManager moveItemAtPath:toPath:error:], 0x1c050ff4c. Non-external.
// The final throw does not fall through into moveItemAtURL:toURL:options:error: as Hex-Rays displays.
- (BOOL)moveItemAtPath:(id)source toPath:(id)destination error:(id *)error {
    id reason;
    if (source == nil && _OCFExecutableLinkedOnOrAfter(6)) {
        reason = [ONSString
            stringWithFormat:(id)OF_STR("%@: source path is nil"), _ONSMethodExceptionProem(self, _cmd)];
    } else if (destination == nil) {
        reason = [ONSString
            stringWithFormat:(id)OF_STR("%@: destination path is nil"), _ONSMethodExceptionProem(self, _cmd)];
    } else {
        return [swiftBridge moveItemAtPath:source toPath:destination options:0 error:error];
    }
    objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                  reason:reason
                                                userInfo:nil]);
}

// Native -[NSFileManager removeItemAtPath:error:], 0x1c05102b0. Non-external.
- (BOOL)removeItemAtPath:(id)path error:(id *)error {
    return path == nil || [swiftBridge removeItemAtPath:path error:error];
}

// Native -[NSFileManager setAttributes:ofItemAtPath:error:], 0x1c05104d0. Non-external. The Swift bridge's
// answer.
- (BOOL)setAttributes:(id)attributes ofItemAtPath:(id)path error:(id *)error {
    return [swiftBridge setAttributes:attributes ofItemAtPath:path error:error];
}

// Native -[NSFileManager createDirectoryAtPath:withIntermediateDirectories:attributes:error:], 0x1c05104d8.
// Non-external.
- (BOOL)createDirectoryAtPath:(id)path
    withIntermediateDirectories:(BOOL)intermediates
                     attributes:(id)attributes
                          error:(id *)error {
    return [swiftBridge createDirectoryAtPath:path
                  withIntermediateDirectories:intermediates
                                   attributes:attributes
                                        error:error];
}

// Native -[NSFileManager _processHasUbiquityContainerEntitlement], 0x1c05119a8. Non-external.
- (BOOL)_processHasUbiquityContainerEntitlement {
    dispatch_once(&containerEntitlementOnce, ^{
      // Native ___56-[NSFileManager _processHasUbiquityContainerEntitlement]_block_invoke, 0x1c0511a48.
      if (![self _processUsesCloudServices]) {
          audit_token_t token = {0};
          mach_msg_type_number_t count = TASK_AUDIT_TOKEN_COUNT;
          task_info(mach_task_self(), TASK_AUDIT_TOKEN, (task_info_t)&token, &count);
          xpc_object_t identifiers =
              xpc_copy_entitlement_for_token("com.apple.developer.ubiquity-container-identifiers", &token);
          if (identifiers == NULL) {
              return;
          }
          const char *identifier = NULL;
          if (xpc_get_type(identifiers) == XPC_TYPE_ARRAY && xpc_array_get_count(identifiers) != 0) {
              identifier = xpc_array_get_string(identifiers, 0);
          }
          xpc_release(identifiers);
          if (identifier == NULL) {
              return;
          }
      }

      initializeLibrarianFunctions();

      if (librarianCurrentProcessHasUbiquityContainer != NULL) {
          containerEntitlement = librarianCurrentProcessHasUbiquityContainer();
      } else {
          containerEntitlement = NO;
      }
    });
    return containerEntitlement;
}

// Native -[NSFileManager _processUsesCloudServices], 0x1c0511b78. Non-external.
- (BOOL)_processUsesCloudServices {
    dispatch_once(&cloudServicesOnce, ^{
      // Native ___42-[NSFileManager _processUsesCloudServices]_block_invoke, 0x1c0511bb8. Non-external.
      audit_token_t token = {0};
      mach_msg_type_number_t count = TASK_AUDIT_TOKEN_COUNT;
      task_info(mach_task_self(), TASK_AUDIT_TOKEN, (task_info_t)&token, &count);
      xpc_object_t services = xpc_copy_entitlement_for_token("com.apple.developer.icloud-services", &token);
      if (services != NULL) {
          if (xpc_get_type(services) == XPC_TYPE_ARRAY && xpc_array_get_count(services) != 0 &&
              xpc_array_get_string(services, 0) != NULL) {
              cloudServices = YES;
          }
          xpc_release(services);
      }
      if (!cloudServices) {
          xpc_object_t identifier =
              xpc_copy_entitlement_for_token("com.apple.developer.ubiquity-kvstore-identifier", &token);
          if (identifier != NULL) {
              cloudServices = xpc_get_type(identifier) == XPC_TYPE_STRING;
              xpc_release(identifier);
          }
      }
    });
    return cloudServices;
}

// Native -[NSFileManager _processCanAccessUbiquityIdentityToken], 0x1c0511cd0. Non-external.
- (BOOL)_processCanAccessUbiquityIdentityToken {
    dispatch_once(&ubiquityIdentityOnce, ^{
      // Native ___55-[NSFileManager _processCanAccessUbiquityIdentityToken]_block_invoke, 0x1c0511d10.
      ubiquityIdentity =
          sandbox_check(getpid(), "mach-lookup", SANDBOX_CHECK_NO_REPORT | 2, "com.apple.bird.token") == 0;
    });
    return ubiquityIdentity;
}

@end

// Native __initUbiquityIdentityDidChange, 0x1c0511dfc. Hidden (was a private external). With the ubiquity
// container entitlement or cloud services, and access to the identity token, the librarian functions are
// initialized; otherwise an info message is logged.
void _initUbiquityIdentityDidChange(void) {
    ONSFileManager *manager = [ONSFileManager defaultManager];
    if (([manager _processHasUbiquityContainerEntitlement] || [manager _processUsesCloudServices]) &&
        [manager _processCanAccessUbiquityIdentityToken]) {
        initializeLibrarianFunctions();
    } else {
        os_log_t log = _NSOSLog();
        os_log_info(
            log, "Registering for NSUbiquityIdentityDidChangeNotification without the necessary entitlements"
        );
    }
}

// Native __postUbqiuityAccountChangeNotification (the native spelling), 0x1c0511ebc. Hidden (was a private
// external). When LBRefreshUbiquityAccountToken (stored at 0x22e76c3b8) answers true, the main run loop posts
// NSUbiquityIdentityDidChangeNotification in its common modes (kCFRunLoopCommonModes, GOT slot 0x222280f80,
// confidence: medium) and is woken.
void _postUbqiuityAccountChangeNotification(void) {
    BOOL (*refresh)(void) = (BOOL (*)(void))librarianRefreshUbiquityAccountToken;
    if (refresh == NULL || !refresh()) {
        return;
    }
    OCFRunLoopPerformBlock(OCFRunLoopGetMain(), kOCFRunLoopCommonModes, ^{
      // Native ____postUbqiuityAccountChangeNotification_block_invoke, 0x1c0514290. Non-external.
      void *pool = objc_autoreleasePoolPush();
      [[(id)fileManagerNotificationCenterClass defaultCenter]
          postNotificationName:(id)OF_STR("NSUbiquityIdentityDidChangeNotification")
                        object:nil];
      objc_autoreleasePoolPop(pool);
    });
    OCFRunLoopWakeUp(OCFRunLoopGetMain());
}

@implementation ONSFileManager (NSUserInformation)

// Native -[NSFileManager(NSUserInformation) temporaryDirectory], 0x1c0513018. Non-external.
- (id)temporaryDirectory {
    return [swiftBridge temporaryDirectory];
}

@end

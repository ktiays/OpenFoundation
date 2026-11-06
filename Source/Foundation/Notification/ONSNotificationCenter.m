//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Notification/ONSNotificationCenter.h"

#import "Foundation/Archiving/ONSRuntimeIssuesLog.h"
#import "Foundation/Notification/ONSNotification.h"
#import "Foundation/Notification/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSMutableString.h"
#import "Foundation/String/ONSString.h"
#import "Foundation/Thread/ONSThread.h"

#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Collections/OCFSet.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <Block.h>
#include <dispatch/dispatch.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <stdatomic.h>
#include <string.h>
#include <unistd.h>

#include <objc/objc-exception.h>
#include <os/lock.h>

// Foundation, iOS Simulator 27.0 (24A434), arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured IDA assembly. iOS Device divergence: iOS Device 24A446 is unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json.

extern Class objc_opt_class(id object);
extern id objc_opt_self(id object);
extern id objc_opt_new(id cls);
extern struct mach_header __dso_handle;
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);
extern int proc_pidinfo(int pid, int flavor, uint64_t argument, void *buffer, int bufferSize);

extern char notificationClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSNotification));
extern char notificationCenterClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSNotificationCenter));
extern char observerClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSObserver));

// Native __nsDependentNotifications, 0x2227d5ae8: four addresses followed by NULL.
static const DependentNotification *const _nsDependentNotifications[] = {
    &_NSUbiquityIdentityDidChangeDependentNotification,
    &_NSBundleDidLoadDependentNotification,
    &_NSProcessInfoThermalStateDidChangeDependentNotification,
    &_NSProcessInfoPowerStateDidChangeDependentNotification,
    NULL
};

static os_unfair_lock _MergedGlobals_97 = OS_UNFAIR_LOCK_INIT;
static dispatch_once_t qword_22E76C6B0;
static _Atomic(OCFMutableSetRef) qword_22E76C6B8;
static _Atomic(ONSNotificationCenter *) qword_22E76C6D0;
static dispatch_once_t qword_22E76C6D8;
// Native subclass default-center cache qword_22E76C6C0 and its lock stru_22E76C6AC.
static OCFMutableDictionaryRef subclassDefaultCenters;
static os_unfair_lock subclassDefaultCenterLock = OS_UNFAIR_LOCK_INIT;
static dispatch_once_t addObserverOnce;

@interface ONSNotificationCenter (RemovingObserver)
- (void)_removeObserver:(uint64_t)token performSubclassCheck:(BOOL)check;
@end

@implementation __ONSObserver

// Native +[__NSObserver observerWithCenter:], 0x1c0572430. Non-external; objc_direct.
+ (id)observerWithCenter:(ONSNotificationCenter *)center {
    // Clang emits objc_opt_self at the entry to this direct class method.
    __ONSObserver *observer = objc_opt_new((id)observerClassSymbol);
    observer->nc = objc_retain(center);
    // Native ___35+[__NSObserver observerWithCenter:]_block_invoke, 0x1c05724b8. Non-external.
    dispatch_once(&qword_22E76C6B0, ^{
      OCFSetCallBacks callbacks = {0};
      // GOT 0x222281080 is bound to kCFTypeSetCallBacks (confidence: medium).
      callbacks.retain = kOCFTypeSetCallBacks.retain;
      callbacks.release = kOCFTypeSetCallBacks.release;
      // Foundation defect: the store at 0x1c057250c is outside the unfair lock used by the readers
      // (e.g. 0x1c0572cd8). Publish the initialized set with release/acquire ordering.
      OCFMutableSetRef observers = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, &callbacks);
      atomic_store_explicit(&qword_22E76C6B8, observers, memory_order_release);
    });
    os_unfair_lock_lock(&_MergedGlobals_97);
    OCFSetAddValue(atomic_load_explicit(&qword_22E76C6B8, memory_order_acquire), observer);
    os_unfair_lock_unlock(&_MergedGlobals_97);
    return observer;
}

// Native -[__NSObserver dealloc], 0x1c0572538. Non-external.
- (void)dealloc {
    os_unfair_lock_lock(&_MergedGlobals_97);
    ONSNotificationCenter *center = nc;
    nc = nil;
    os_unfair_lock_unlock(&_MergedGlobals_97);
    if (_block != NULL) {
        Block_release(_block);
    }
    [center removeObserver:self];
    objc_release(center);
    [super dealloc];
}

// Native -[__NSObserver token], 0x1c05725e8. Non-external.
- (uint64_t)token {
    return _token;
}

@end

@implementation ONSNotificationCenter

// Native standardDefaultCenter, inlined into +defaultCenter (0x1c0572618) and +_defaultCenterWithoutCreating
// (0x1c0572708). Both share one once token (0x22e76c6d8), one center (0x22e76c6d0) and one global block
// literal (0x2227d5b30).
static inline ONSNotificationCenter *standardDefaultCenter(void) {
    // Native ___standardDefaultCenter_block_invoke, 0x1c0573a40. Non-external.
    dispatch_once(&qword_22E76C6D8, ^{
      ONSNotificationCenter *center = objc_alloc((Class)notificationCenterClassSymbol);
      center->_impl = (OCFNotificationCenterRef)OCFRetain(_OCFXNotificationGetTaskCenter());
      atomic_store_explicit(&qword_22E76C6D0, center, memory_order_release);
    });
    return atomic_load_explicit(&qword_22E76C6D0, memory_order_acquire);
}

// Native +[NSNotificationCenter load], 0x1c057260c. Non-external.
+ (void)load {
    _OCFNotificationCenterRegisterDependentNotificationList(_nsDependentNotifications);
}

// Native +[NSNotificationCenter defaultCenter], 0x1c0572618. Non-external.
// GOT bindings (medium): kCFAllocatorSystemDefault 0x222280b70,
// kCFTypeDictionaryKeyCallBacks 0x222281070, kCFTypeDictionaryValueCallBacks 0x222281078.
// Reading: Local/Scratch/cf-core-r2-fb/ida-parents-bridges.json (FB-bundle-paths-plist).
+ (id)defaultCenter {
    if ((Class)self == (Class)notificationCenterClassSymbol) {
        return standardDefaultCenter();
    }

    os_unfair_lock_lock_with_options(&subclassDefaultCenterLock, 0x10000);
    if (subclassDefaultCenters == NULL) {
        subclassDefaultCenters = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
    }
    id center = (id)OCFDictionaryGetValue(subclassDefaultCenters, self);
    if (center == nil) {
        center = objc_alloc_init((Class)self);
        OCFDictionarySetValue(subclassDefaultCenters, self, center);
        objc_release(center);
    }
    os_unfair_lock_unlock(&subclassDefaultCenterLock);
    return center;
}

// Native +[NSNotificationCenter _defaultCenterWithoutCreating], 0x1c0572708. Non-external.
+ (id)_defaultCenterWithoutCreating {
    if (atomic_load_explicit(&qword_22E76C6D0, memory_order_acquire) == nil) {
        return nil;
    }
    return standardDefaultCenter();
}

// Native -[NSNotificationCenter postNotification:], 0x1c0572e1c. Non-external.
// The throw at 0x1c0572e80 does not return into the following method as shown by Hex-Rays.
- (void)postNotification:(id)notification {
    if (notification != nil) {
        _OCFXNotificationPost(_impl, notification, 0);
        return;
    }
    id reason = [ONSString
        stringWithFormat:(id)OF_STR("%@: notification is nil"), _ONSMethodExceptionProem(self, _cmd)];
    objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                  reason:reason
                                                userInfo:nil]);
}

// Native -[NSNotificationCenter init], 0x1c05727ec. Non-external.
// The shared-cache GOT binding at 0x222280b38 is the custom local configuration (confidence: medium).
- (id)init {
    _impl = _OCFXNotificationCenterCreate(
        kOCFAllocatorDefault, &_kOCFXNotificationConfigurationCustomLocalConfiguration
    );
    actorQueueManagerLock = OS_UNFAIR_LOCK_INIT;
    _actorQueueManager = nil;
    return self;
}

// Native -[NSNotificationCenter dealloc], 0x1c0572830. Non-external.
- (void)dealloc {
    if (_impl != NULL) {
        OCFRelease(_impl);
    }
    objc_release(_actorQueueManager);
    [super dealloc];
}

// Native -[NSNotificationCenter description], 0x1c05728b0. Non-external.
- (id)description {
    return objc_autorelease((id)OCFCopyDescription(_impl));
}

// Native -[NSNotificationCenter _removeObserver:performSubclassCheck:], 0x1c05729f0. Non-external.
- (void)_removeObserver:(uint64_t)token performSubclassCheck:(BOOL)check {
    if (check) {
        Class actualClass = object_getClass(self);
        if (actualClass != objc_opt_class((id)notificationCenterClassSymbol)) {
            SEL selector = sel_getUid("_removeObserver:performSubclassCheck:");
            IMP actual = method_getImplementation(class_getInstanceMethod(actualClass, selector));
            Class baseClass = objc_opt_class((id)notificationCenterClassSymbol);
            if (actual == method_getImplementation(class_getInstanceMethod(baseClass, selector))) {
                objc_opt_self((id)observerClassSymbol);
                __ONSObserver *observer = [__ONSObserver observerWithCenter:self];
                observer->_token = token;
                [self removeObserver:observer];
                return;
            }
        }
    }
    _OCFXNotificationCancelToken(_impl, token);
}

// Native -[NSNotificationCenter addObserver:selector:name:object:], 0x1c0572ac4. Non-external.
- (void)addObserver:(id)observer selector:(SEL)selector name:(id)name object:(id)object {
    if (observer == nil || selector == NULL) {
        return;
    }
    if (object != nil && strcmp(class_getName(objc_opt_class(object)), "__SwiftValue") == 0) {
        os_log_t log = _NSRuntimeIssuesLog();
        if (os_log_type_enabled(log, OS_LOG_TYPE_FAULT)) {
            uint8_t buffer[2] = {0, 0};
            _os_log_fault_impl(
                &__dso_handle,
                log,
                OS_LOG_TYPE_FAULT,
                "Object cannot be a Swift value type.",
                buffer,
                sizeof(buffer)
            );
        }
        return;
    }
    if (name == ONSWillBecomeMultiThreadedNotification &&
        (atomic_load_explicit(&_NSIsMultiThreaded, memory_order_relaxed) & 1) == 0) {
        atomic_store_explicit(&_NSIsMultiThreaded, 1, memory_order_relaxed);
        id notification = [objc_alloc((Class)notificationClassSymbol) initWithName:name
                                                                            object:nil
                                                                          userInfo:nil];
        ((void (*)(id, SEL, id))objc_msgSend)(observer, selector, notification);
        objc_release(notification);
        return;
    }
    // Native ___57-[NSNotificationCenter addObserver:selector:name:object:]_block_invoke,
    // 0x1c0572c7c. Non-external; the captured invoke consists solely of RET (kept).
    dispatch_once(&addObserverOnce, ^{
    });
    OCFOptionFlags options = class_isMetaClass(object_getClass(observer)) ? 0x40000 : 0x40400;
    // GOT 0x222280de0 and 0x222280de8 are the any-name and any-object imports (confidence: medium).
    OCFStringRef notificationName = name != nil ? (OCFStringRef)name : kOCFNotificationAnyName;
    const void *notificationObject = object != nil ? (const void *)object : kOCFNotificationAnyObject;
    _OCFXNotificationRegisterObserver(
        _impl, notificationName, notificationObject, options, observer, selector, NULL
    );
}

// Native -[NSNotificationCenter removeObserver:], 0x1c0572c80. Non-external.
- (void)removeObserver:(id)observer {
    [self removeObserver:observer name:nil object:nil];
}

// Native -[NSNotificationCenter removeObserver:name:object:], 0x1c0572c8c. Non-external.
- (void)removeObserver:(id)observer name:(id)name object:(id)object {
    if (observer == nil) {
        return;
    }
    objc_opt_self((id)observerClassSymbol);
    os_unfair_lock_lock(&_MergedGlobals_97);
    OCFMutableSetRef observers = atomic_load_explicit(&qword_22E76C6B8, memory_order_acquire);
    BOOL registered = observers != NULL && OCFSetContainsValue(observers, observer);
    os_unfair_lock_unlock(&_MergedGlobals_97);
    if (registered) {
        objc_opt_self((id)observerClassSymbol);
        os_unfair_lock_lock(&_MergedGlobals_97);
        observers = atomic_load_explicit(&qword_22E76C6B8, memory_order_acquire);
        if (observers != NULL && OCFSetContainsValue(observers, observer)) {
            OCFSetRemoveValue(observers, observer);
            __ONSObserver *tokenObserver = observer;
            ONSNotificationCenter *center = tokenObserver->nc;
            tokenObserver->nc = nil;
            uint64_t token = [tokenObserver token];
            os_unfair_lock_unlock(&_MergedGlobals_97);
            if (center != nil) {
                [center _removeObserver:token performSubclassCheck:NO];
                objc_release(observer);
                objc_release(center);
            }
        } else {
            os_unfair_lock_unlock(&_MergedGlobals_97);
        }
        return;
    }
    OCFStringRef notificationName = name != nil ? (OCFStringRef)name : kOCFNotificationAnyName;
    const void *notificationObject = object != nil ? (const void *)object : kOCFNotificationAnyObject;
    _OCFXNotificationRemoveObservers(_impl, notificationName, notificationObject, observer);
}

// Native -[NSNotificationCenter postNotificationName:object:], 0x1c0572e84. Non-external.
- (void)postNotificationName:(id)name object:(id)object {
    [self postNotificationName:name object:object userInfo:nil];
}

// Native -[NSNotificationCenter postNotificationName:object:userInfo:], 0x1c0572e8c. Non-external.
- (void)postNotificationName:(id)name object:(id)object userInfo:(id)userInfo {
    if (name != nil) {
        id notification = [objc_alloc((Class)notificationClassSymbol) initWithName:name
                                                                            object:object
                                                                          userInfo:userInfo];
        _OCFXNotificationPost(_impl, notification, 0);
        objc_release(notification);
    }
}

// Native -[NSNotificationCenter isEmpty], 0x1c05730b8. Non-external.
- (BOOL)isEmpty {
    return _OCFXNotificationCenterIsEmpty(_impl) != 0;
}

// Native -[NSNotificationCenter debugDescription], 0x1c05730d8. Non-external.
- (id)debugDescription {
    uint32_t info[16] = {0};
    int pid = getpid();
    if (proc_pidinfo(pid, 13, 0, info, sizeof(info)) != sizeof(info) || (info[8] & 2) == 0) {
        return [ONSString stringWithFormat:(id)OF_STR("<%@:%p>\n"), objc_opt_class(self), self];
    }
    ONSMutableString *description =
        [ONSMutableString stringWithFormat:(id)OF_STR("<%@:%p>\n"), objc_opt_class(self), self];
    OCFStringRef debugInfo = _OCFXNotificationCenterCopyDebugInfo(_impl);
    [description appendString:(id)debugInfo];
    OCFRelease(debugInfo);
    return description;
}

@end

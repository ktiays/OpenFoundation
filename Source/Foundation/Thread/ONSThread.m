//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Thread/ONSThread.h"

#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/RunLoop/OCFRunLoop.h"
#include "CoreFoundation/String/OCFString.h"
#include <Block.h>
#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <objc/objc-sync.h>
#include <objc/runtime.h>
#include <stdatomic.h>
#include <stdlib.h>

// iOS Simulator 27.0 (24A434), arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured assembly, except explicitly marked bindings.
// iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Scratch/cf-core-fd-misc/Reading.md; native source paths unread, organization inferred.
// MRC: retains, releases and autorelease-pool operations are explicit in the capture.

extern Class objc_opt_class(id object);
extern id objc_opt_new(Class cls);
extern BOOL objc_opt_respondsToSelector(id object, SEL selector);
extern pthread_t pthread_main_thread_np(void);
extern void thread_stack_async_pcs(void **frames, unsigned int capacity, unsigned int *count);

extern char threadClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSThread));
extern char threadDataClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(_ONSThreadData));
extern char callStackArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(_ONSCallStackArray));
extern char notificationCenterClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSNotificationCenter));
extern char mutableDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableDictionary));
extern char exceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));

@protocol ThreadMessages
+ (id)arrayWithFrames:(void **)frames count:(unsigned int)count symbols:(BOOL)symbols;
+ (id)_defaultCenterWithoutCreating;
- (void)postNotificationName:(id)name object:(id)object userInfo:(id)info;
+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)info;
- (id)copy;
- (const char *)UTF8String;
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)objects count:(NSUInteger)count;
- (void)signal:(BOOL)flag;
- (id)objectForKey:(id)key;
@end

// Native _NSIsMultiThreaded, 0x22e775b48. Was a private external; one-byte ABI preserved.
// Foundation defect: +mainThread 0x1c05e31f0-0x1c05e3208 and +currentThread
// 0x1c05e3348-0x1c05e3360 access this flag without synchronization, racing ordinary concurrent callers.
// Use relaxed atomics and exchange to publish the one-time notification exactly once.
_Atomic(uint8_t) _NSIsMultiThreaded;

// Native _init.seqNum, 0x22e6fcca0. Non-external; LDADDAL uses acquire/release ordering.
static _Atomic(unsigned int) threadSequenceNumber;

// Native __MergedGlobals.7_2 / qword_22e76ccb8: main-thread once predicate / instance.
// Invoke 0x1c05e5254 was independently read through IDA on 2026/11/03 (additional-ida.json).
static dispatch_once_t mainThreadOnce;
__attribute__((visibility("hidden"))) id sharedMainThread;
extern char mainThreadClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(_ONSMainThread));

static id _NSThreadGet0(int current);
static void __NSFinalizeThreadData(void *value);

@implementation ONSThread

// Native +[NSThread isMainThread], 0x1c05e2c00. External class, non-external method.
+ (BOOL)isMainThread {
    return pthread_main_np() == 1;
}

// Native +[NSThread callStackSymbols], 0x1c05e3094. Non-external method.
// Ownership of the final allocation passes to _NSCallStackArray, including zero-frame failures.
+ (id)callStackSymbols {
    unsigned int capacity = 256;
    void **frames = malloc_type_malloc(0x800, UINT64_C(0x80040b8603338));
    unsigned int count = 0;
    if (frames != NULL) {
        thread_stack_async_pcs(frames, capacity, &count);
        while (count != 0) {
            while (frames[count - 1] == NULL) {
                count--;
                if (count == 0) {
                    break;
                }
            }
            if (count == 0 || capacity > count || (capacity >> 20) != 0) {
                break;
            }
            void **larger = malloc_type_malloc((uint32_t)(capacity << 5), UINT64_C(0x80040b8603338));
            if (larger == NULL) {
                break;
            }
            capacity <<= 2;
            free(frames);
            frames = larger;
            count = 0;
            thread_stack_async_pcs(frames, capacity, &count);
        }
    }
    return [(id)callStackArrayClass arrayWithFrames:frames count:count symbols:YES];
}

// Native +[NSThread mainThread], 0x1c05e31e0. Non-external method.
// Foundation defect: the native shared-flag race is repaired as documented at _NSIsMultiThreaded.
+ (id)mainThread {
    if ((atomic_load_explicit(&_NSIsMultiThreaded, memory_order_relaxed) & 1) == 0 &&
        pthread_main_np() != 1 &&
        (atomic_exchange_explicit(&_NSIsMultiThreaded, 1, memory_order_relaxed) & 1) == 0) {
        // The observer registration at 0x1c0572ac4 checks the identity of the exported thread notification.
        [[(id)notificationCenterClass _defaultCenterWithoutCreating]
            postNotificationName:ONSWillBecomeMultiThreadedNotification
                          object:nil
                        userInfo:nil];
    }
    return _NSThreadGet0(0);
}

// Native _NSThreadGet0, 0x1c05e323c. Non-external (file-local).
// The CCMP/CSET sequence selects the shared thread when current == 0 OR pthread_equal != 0.
static id _NSThreadGet0(int current) {
    pthread_t thread = pthread_self();
    int isMain = pthread_equal(thread, pthread_main_thread_np());
    BOOL cache = (current | isMain) != 0;
    BOOL useMain = current == 0 || isMain != 0;
    ONSThread *result;
    if (!cache || (result = _OCFGetTSD(28)) == nil) {
        if (useMain) {
            dispatch_once(&mainThreadOnce, ^{
              // Native _____mainNSThread_block_invoke, 0x1c05e5254. Non-external.
              ONSThread *main = objc_opt_new((Class)mainThreadClass);
              atomic_store_explicit(&((_ONSThreadData *)main->_private)->status, 14, memory_order_release);
              pthread_t tid = pthread_main_thread_np();
              sharedMainThread = main;
              ((_ONSThreadData *)main->_private)->tid = tid;
            });
            result = sharedMainThread;
        } else {
            result = objc_opt_new((Class)threadClass);
            atomic_store_explicit(&((_ONSThreadData *)result->_private)->status, 14, memory_order_release);
            ((_ONSThreadData *)result->_private)->tid = pthread_self();
        }
        _OCFSetTSD(29, (void *)(uintptr_t)12345678, __NSFinalizeThreadData);
        if (cache) {
            _OCFSetTSD(28, result, NULL);
        }
    }
    return result;
}

// Native +[NSThread currentThread], 0x1c05e3338. Non-external method.
// Foundation defect: the native shared-flag race is repaired as documented at _NSIsMultiThreaded.
+ (id)currentThread {
    if ((atomic_load_explicit(&_NSIsMultiThreaded, memory_order_relaxed) & 1) == 0 &&
        pthread_main_np() != 1 &&
        (atomic_exchange_explicit(&_NSIsMultiThreaded, 1, memory_order_relaxed) & 1) == 0) {
        [[(id)notificationCenterClass _defaultCenterWithoutCreating]
            postNotificationName:ONSWillBecomeMultiThreadedNotification
                          object:nil
                        userInfo:nil];
    }
    return _NSThreadGet0(1);
}

// Native -[NSThread initWithBlock:], 0x1c05e3620. Non-external method.
// The selector at cache address 0x2387b00a5 is "invoke", independently read through IDA MCP from the
// pinned 24A434 shared cache (UUID 5827BB56-52E1-3D0B-AC02-7A3201246563); thread-selector.json.
- (id)initWithBlock:(void (^)(void))block {
    if (block != nil) {
        void *copied = _Block_copy(block);
        id result = [self initWithTarget:(id)copied selector:@selector(invoke) object:nil];
        _Block_release(copied);
        return result;
    }
    id reason = [ONSString stringWithFormat:(id)OF_STR("%@: block targets for threads cannot be nil"),
                                            _ONSMethodExceptionProem(self, _cmd)];
    [self release];
    @throw [(id)exceptionClass exceptionWithName:(id)ONSInvalidArgumentException reason:reason userInfo:nil];
}

// Native -[NSThread init], 0x1c05e36cc. Non-external method. Mutable dictionary GOT 0x22227fb28 (medium).
- (id)init {
    _ONSThreadData *data = objc_opt_new((Class)threadDataClass);
    data->dict = objc_opt_new((Class)mutableDictionaryClass);
    data->name = nil;
    data->qstate = 0;
    data->qos = 0;
    data->_selector = NULL;
    data->_argument = nil;
    data->_target = nil;
    data->seqNum = (int)(atomic_fetch_add_explicit(&threadSequenceNumber, 1, memory_order_acq_rel) + 1);
    data->cancel = 0;
    atomic_store_explicit(&data->status, 0, memory_order_release);
    data->performQ = nil;
    data->performD = nil;
    data->pri = -1.0;
    data->defpri = -1.0;
    pthread_attr_init(&data->attr);
    pthread_attr_setscope(&data->attr, 1);
    pthread_attr_setdetachstate(&data->attr, 2);
    data->tid = NULL;
    _private = data;
    struct sched_param parameters = {0};
    double priority;
    if (pthread_attr_getschedparam(&data->attr, &parameters) != 0) {
        priority = data->defpri;
    } else {
        priority = (double)parameters.sched_priority / 62.0;
        data->defpri = priority;
    }
    data->pri = priority;
    return self;
}

// Native -[NSThread initWithTarget:selector:object:], 0x1c05e37dc. Non-external method.
- (id)initWithTarget:(id)target selector:(SEL)selector object:(id)argument {
    if (target != nil && (objc_opt_respondsToSelector(target, selector) & 1) == 0) {
        id proem = _ONSMethodExceptionProem(self, _cmd);
        id targetProem = _ONSMethodExceptionProem(target, selector);
        id reason = [ONSString
            stringWithFormat:(id)OF_STR("%@: target does not implement selector (%@)"), proem, targetProem];
        [self release];
        @throw [(id)exceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                              reason:reason
                                            userInfo:nil];
    }
    self = [self init];
    id retainedTarget = [target retain];
    if (_private != nil) {
        ((_ONSThreadData *)_private)->_target = retainedTarget;
        if (_private != nil) {
            ((_ONSThreadData *)_private)->_selector = selector;
        }
    }
    id retainedArgument = [argument retain];
    if (_private != nil) {
        ((_ONSThreadData *)_private)->_argument = retainedArgument;
    }
    return self;
}

// Native -[NSThread dealloc], 0x1c05e38d4. Non-external method.
- (void)dealloc {
    if (_private != nil) {
        [((_ONSThreadData *)_private)->name release];
        ((_ONSThreadData *)_private)->name = nil;
        [_private release];
    }
    [super dealloc];
}

// Native -[NSThread threadDictionary], 0x1c05e3960. Non-external method.
- (id)threadDictionary {
    return ((_ONSThreadData *)_private)->dict;
}

// Native -[NSThread setName:], 0x1c05e396c. Non-external method; release precedes copy (kept).
- (void)setName:(id)value {
    if (((_ONSThreadData *)_private)->name != value) {
        [((_ONSThreadData *)_private)->name release];
        ((_ONSThreadData *)_private)->name = [value copy];
        if (self == [ONSThread currentThread]) {
            id stored = ((_ONSThreadData *)_private)->name;
            pthread_setname_np(stored != nil ? [stored UTF8String] : "");
        }
    }
}

// Native -[NSThread name], 0x1c05e39f0. Non-external method.
- (id)name {
    id result = ((_ONSThreadData *)_private)->name;
    if (result == nil) {
        char bytes[300];
        if (pthread_getname_np(((_ONSThreadData *)_private)->tid, bytes, sizeof(bytes)) != 0) {
            return ((_ONSThreadData *)_private)->name;
        }
        return [ONSString stringWithUTF8String:bytes];
    }
    return result;
}

// Native -[NSThread isMainThread], 0x1c05e3f64. Non-external method.
- (BOOL)isMainThread {
    return [ONSThread mainThread] == self;
}

// Native -[NSThread description], 0x1c05e4014. Non-external method.
// The fallback name follows the calling thread's pthread_main_np, not the described thread (kept).
- (id)description {
    Class cls = objc_opt_class(self);
    unsigned int sequence = ((_ONSThreadData *)_private)->seqNum;
    id value = ((_ONSThreadData *)_private)->name;
    if (value == nil) {
        value = pthread_main_np() == 1 ? (id)OF_STR("main") : nil;
    }
    return [ONSString
        stringWithFormat:(id)OF_STR("<%@: %p>{number = %d, name = %@}"), cls, self, sequence, value];
}

@end

struct ThreadEnumerationState {
    unsigned long state;
    id *itemsPtr;
    unsigned long *mutationsPtr;
    unsigned long extra[5];
};

// Native __NSFinalizeThreadData, 0x1c05e4eac. Non-external (file-local).
// Three destructor rounds retain the native qstate 0 -> 1 -> 2 -> 3 transitions and callback/release order.
static void __NSFinalizeThreadData(void *value) {
    _OCFSetTSD(29, value, __NSFinalizeThreadData);
    ONSThread *thread = _NSThreadGet0(1);
    _ONSThreadData *data = thread->_private;
    if (data->qstate == 2) {
        data->qstate = 3;
        pthread_attr_destroy(&((_ONSThreadData *)thread->_private)->attr);
        [thread release];
        _OCFSetTSD(29, NULL, NULL);
    } else if (data->qstate == 1) {
        data->qstate = 2;
        void *pool = _OCFAutoreleasePoolPush();
        [((_ONSThreadData *)thread->_private)->dict release];
        ((_ONSThreadData *)thread->_private)->dict = nil;
        _OCFAutoreleasePoolPop(pool);
    } else if (data->qstate == 0) {
        data->qstate = 1;
        void *pool = _OCFAutoreleasePoolPush();
        [[(id)notificationCenterClass _defaultCenterWithoutCreating]
            postNotificationName:(id)OF_STR("NSThreadWillExitNotification")
                          object:thread
                        userInfo:nil];
        atomic_store_explicit(&((_ONSThreadData *)thread->_private)->status, 15, memory_order_release);
        objc_sync_enter(thread);
        struct ThreadEnumerationState state = {0};
        id objects[16];
        id queue = ((_ONSThreadData *)thread->_private)->performQ;
        NSUInteger count = [queue countByEnumeratingWithState:&state objects:objects count:16];
        if (count != 0) {
            unsigned long mutation = *state.mutationsPtr;
            do {
                for (NSUInteger index = 0; index != count; index++) {
                    if (*state.mutationsPtr != mutation) {
                        objc_enumerationMutation(queue);
                    }
                    [state.itemsPtr[index] signal:YES];
                }
                count = [queue countByEnumeratingWithState:&state objects:objects count:16];
            } while (count != 0);
        }
        id dictionary = ((_ONSThreadData *)thread->_private)->performD;
        if (dictionary != nil) {
            struct ThreadEnumerationState keys = {0};
            id keyObjects[16];
            count = [dictionary countByEnumeratingWithState:&keys objects:keyObjects count:16];
            if (count != 0) {
                unsigned long mutation = *keys.mutationsPtr;
                do {
                    for (NSUInteger index = 0; index != count; index++) {
                        if (*keys.mutationsPtr != mutation) {
                            objc_enumerationMutation(dictionary);
                        }
                        OCFRunLoopSourceRef source =
                            (OCFRunLoopSourceRef)[(id)((_ONSThreadData *)thread->_private)->performD
                                objectForKey:keys.itemsPtr[index]];
                        OCFRunLoopSourceContext context = {0};
                        OCFRunLoopSourceGetContext(source, &context);
                        OCFRunLoopSourceInvalidate(source);
                        free(context.info);
                    }
                    count = [dictionary countByEnumeratingWithState:&keys objects:keyObjects count:16];
                } while (count != 0);
            }
            [((_ONSThreadData *)thread->_private)->performD release];
            ((_ONSThreadData *)thread->_private)->performD = nil;
        }
        if (((_ONSThreadData *)thread->_private)->performQ != nil) {
            [((_ONSThreadData *)thread->_private)->performQ release];
            ((_ONSThreadData *)thread->_private)->performQ = nil;
        }
        objc_sync_exit(thread);
        data = thread->_private;
        [data != nil ? data->_argument : nil release];
        data = thread->_private;
        if (data != nil) {
            data->_argument = nil;
            data = thread->_private;
            if (data != nil) {
                data = (id)data->_target;
            }
        }
        [(id)data release];
        ((_ONSThreadData *)thread->_private)->_target = nil;
        [((_ONSThreadData *)thread->_private)->dict release];
        ((_ONSThreadData *)thread->_private)->dict = nil;
        _OCFAutoreleasePoolPop(pool);
    }
}

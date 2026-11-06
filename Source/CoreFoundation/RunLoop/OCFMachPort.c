//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/RunLoop/OCFMachPort.h"
#include "CoreFoundation/RunLoop/OCFRunLoop.h"
#include "CoreFoundation/String/OCFString.h"

#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

// Inferred organization: CFMachPortGetTypeID (0x1c01a4498) and __CFMachPortPerform through
// ____CFDescribeCFMachPortPerformCallout_block_invoke (0x1c01a51cc-0x1c01a53f0) lie in the CFMachPort cluster
// of the iOS Simulator __text section, with __CFDescribeCFMachPortPerformCallout.cold.1 at 0x1c03088f4, and
// so do the class callbacks (0x1c01a41f0-0x1c01a4494), __CFMachPortClass (0x2227a1fd8) and
// __CFMachMessageCheckForAndDestroyUnsentMessage (0x1c01a55d4); the rest of the cluster (creation and
// invalidation) is outside this unit's scope. The native source path is not in the evidence. Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (disassembly); iOS Device 24A446: unread. Reading: Local/Scratch/cf-core/tasks/runloop-streams.txt
// (sub-track reading; no versioned reading yet).

// The fork check inlined at each use; __CF_FORK_STATE is read as a volatile byte.
static inline void checkForFork(void) {
    if ((*(volatile uint8_t *)&__CF_FORK_STATE & 2) == 0) {
        __CF_USED();
    }
    if ((*(volatile uint8_t *)&__CF_FORK_STATE & 1) != 0) {
        __THE_PROCESS_HAS_FORKED_AND_YOU_CANNOT_USE_THIS_COREFOUNDATION_FUNCTIONALITY___YOU_MUST_EXEC__();
    }
}

// Native __CFMachPortDeallocate, 0x1c01a41f0 (the delta capture). Non-external; the class's finalize. After
// the fork check a forked process returns. Under the lock a valid port (state 0) is invalidated: the state
// becomes 1 (a store followed by DMB ISH), a dispatch source is cancelled and cleared, the invalidation
// callback, the run loop source (cleared) and the context information (cleared) are taken with the release
// callback at 112; the invalidation callback runs with the port and the information, the source is
// invalidated and released, and the information released when both exist, each with the lock dropped; then
// the state becomes 2 (again with DMB ISH). The state becomes 3, the port name and bits 2 and 1 of the info
// word (separate acquire loads) are read, the lock is dropped and _cfmp_record_deallocation(0, ...) records
// them (a tail call).
static void __CFMachPortDeallocate(OCFTypeRef cf) {
    OCFMachPortRef mp = (OCFMachPortRef)cf;
    checkForFork();
    if ((*(volatile uint8_t *)&__CF_FORK_STATE & 1) != 0) {
        return;
    }
    os_unfair_lock_lock(&mp->lock);
    if (mp->state == 0) {
        mp->state = 1;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
        if (mp->dispatchSource != NULL) {
            dispatch_source_cancel(mp->dispatchSource);
            mp->dispatchSource = NULL;
        }
        OCFMachPortInvalidationCallBack invalidationCallBack = mp->invalidationCallBack;
        OCFRunLoopSourceRef source = mp->runLoopSource;
        mp->runLoopSource = NULL;
        void *info = mp->context.info;
        mp->context.info = NULL;
        void (*release)(const void *) = mp->release;
        if (invalidationCallBack != NULL) {
            os_unfair_lock_unlock(&mp->lock);
            invalidationCallBack(mp, info);
            os_unfair_lock_lock(&mp->lock);
        }
        if (source != NULL) {
            os_unfair_lock_unlock(&mp->lock);
            OCFRunLoopSourceInvalidate(source);
            OCFRelease(source);
            os_unfair_lock_lock(&mp->lock);
        }
        if (release != NULL && info != NULL) {
            os_unfair_lock_unlock(&mp->lock);
            release(info);
            os_unfair_lock_lock(&mp->lock);
        }
        mp->state = 2;
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
    }
    mp->state = 3;
    mach_port_t port = mp->port;
    bool bit2 = ((__atomic_load_n(&mp->base.info, __ATOMIC_ACQUIRE) >> 2) & 1) != 0;
    bool bit1 = ((__atomic_load_n(&mp->base.info, __ATOMIC_ACQUIRE) >> 1) & 1) != 0;
    os_unfair_lock_unlock(&mp->lock);
    _cfmp_record_deallocation(0, port, bit2, bit1);
}

// Native __CFMachPortEqual, 0x1c01a433c (the delta capture). Non-external; equal port names.
static Boolean __CFMachPortEqual(OCFTypeRef cf1, OCFTypeRef cf2) {
    return ((OCFMachPortRef)cf1)->port == ((OCFMachPortRef)cf2)->port;
}

// Native __CFMachPortHash, 0x1c01a4350 (the delta capture). Non-external; the port name.
static OCFHashCode __CFMachPortHash(OCFTypeRef cf) {
    return ((OCFMachPortRef)cf)->port;
}

// Native __CFMachPortCopyDescription, 0x1c01a4358 (the delta capture). Non-external; the class's debug
// description. With information and a copyDescription callback its answer is the context description;
// otherwise, or when it answers NULL, "<CFMachPort context %p>" with the information (re-read) of
// kCFAllocatorSystemDefault. The callout is named by dladdr when the symbol starts exactly at it, else "???";
// valid is state 0. The context description is released when non-NULL.
static OCFStringRef __CFMachPortCopyDescription(OCFTypeRef cf) {
    OCFMachPortRef mp = (OCFMachPortRef)cf;
    OCFStringRef contextDesc = NULL;
    if (mp->context.info != NULL && mp->context.copyDescription != NULL) {
        contextDesc = mp->context.copyDescription(mp->context.info);
    }
    if (contextDesc == NULL) {
        contextDesc = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault, NULL, OF_STR("<CFMachPort context %p>"), mp->context.info
        );
    }
    void *callout = (void *)mp->callout;
    Dl_info dlInfo;
    memset(&dlInfo, 0, sizeof(dlInfo));
    const char *name = "???";
    if (dladdr(callout, &dlInfo) != 0 && dlInfo.dli_sname != NULL && dlInfo.dli_saddr == callout) {
        name = dlInfo.dli_sname;
    }
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFAllocatorRef objectAllocator = OCFGetAllocator(mp);
    OCFStringRef result = OCFStringCreateWithFormat(
        allocator,
        NULL,
        OF_STR("<CFMachPort %p [%p]>{valid = %s, port = %x, source = %p, callout = %s (%p), context = %@}"),
        mp,
        objectAllocator,
        mp->state == 0 ? "Yes" : "No",
        mp->port,
        mp->runLoopSource,
        name,
        callout,
        contextDesc
    );
    if (contextDesc != NULL) {
        OCFRelease(contextDesc);
    }
    return result;
}

// Native __CFMachPortClass, 0x2227a1fd8 (__DATA_CONST,__const). Hidden (the builtin table's record for type
// 36); version 0, name "CFMachPort", finalize, equal, hash and the debug description.
const OCFRuntimeClass __CFMachPortClass = {
    0,
    "CFMachPort",
    NULL,
    NULL,
    __CFMachPortDeallocate,
    __CFMachPortEqual,
    __CFMachPortHash,
    NULL,
    __CFMachPortCopyDescription,
    NULL,
    NULL,
    0,
};

// Native CFMachPortGetTypeID, 0x1c01a4498. External.
OCFTypeID OCFMachPortGetTypeID(void) {
    return 36;
}

// Native __CFMachPortPerform, 0x1c01a51cc. Hidden (was private external). After the fork check a forked
// process returns NULL. Under the port's lock an invalid port (nonzero state) returns NULL; otherwise the
// information is retained with the port's retain callback (the release callback is then remembered), the
// callout runs unlocked and the information is released; a second fork check reads the state once more (a
// discarded volatile load).
void *__CFMachPortPerform(void *msg, OCFIndex size, OCFAllocatorRef allocator, void *info) {
    OCFMachPortRef mp = info;
    (void)allocator;
    checkForFork();
    if ((*(volatile uint8_t *)&__CF_FORK_STATE & 1) != 0) {
        return NULL;
    }
    os_unfair_lock_lock(&mp->lock);
    if (mp->state != 0) {
        os_unfair_lock_unlock(&mp->lock);
        return NULL;
    }
    void *contextInfo = mp->context.info;
    void (*contextRelease)(const void *) = NULL;
    if (mp->retain != NULL) {
        contextInfo = (void *)mp->retain(mp->context.info);
        contextRelease = mp->release;
    }
    os_unfair_lock_unlock(&mp->lock);
    mp->callout(mp, msg, size, contextInfo);
    if (contextRelease != NULL) {
        contextRelease(contextInfo);
    }
    checkForFork();
    (void)*(volatile uint8_t *)&__CF_FORK_STATE;
    return NULL;
}

// Native __CFDescribeCFMachPortPerformCallout, 0x1c01a52b0, with .cold.1 (0x1c03088f4, the dispatch_once
// call) and
// ____CFDescribeCFMachPortPerformCallout_block_invoke (0x1c01a53f0). Hidden (was private external).
// Foundation's NSMachPort callout is described by Foundation's describer (a tail call with the context's
// address); any other callout by its dladdr symbol ("(CFMachPort) %s") and image path. The once block looks
// both up in Foundation.
void __CFDescribeCFMachPortPerformCallout(
    void *info, char *libraryName, size_t libraryNameSize, char *functionName, size_t functionNameSize
) {
    OCFMachPortRef mp = info;
    static dispatch_once_t onceToken;
    static void *foundationFireMachPort;
    static void (*foundationDescribeNSMachPortCallout)(
        OCFMachPortRef, OCFMachPortContext *, char *, size_t, char *, size_t
    );
    dispatch_once(&onceToken, ^{
      void *handle = dlopen("/System/Library/Frameworks/Foundation.framework/Foundation", RTLD_NOW);
      if (handle != NULL) {
          foundationFireMachPort = dlsym(handle, "__NSFireMachPort");
          foundationDescribeNSMachPortCallout = dlsym(handle, "__NSDescribeNSMachPortCallout");
          dlclose(handle);
      }
    });
    void *callout = (void *)mp->callout;
    if (foundationFireMachPort != NULL && foundationDescribeNSMachPortCallout != NULL &&
        callout == foundationFireMachPort) {
        foundationDescribeNSMachPortCallout(
            mp, &mp->context, libraryName, libraryNameSize, functionName, functionNameSize
        );
        return;
    }
    Dl_info dlInfo;
    memset(&dlInfo, 0, sizeof(dlInfo));
    if (dladdr(callout, &dlInfo) != 0) {
        snprintf(functionName, functionNameSize, "(CFMachPort) %s", dlInfo.dli_sname);
        snprintf(libraryName, libraryNameSize, "%s", dlInfo.dli_fname);
    }
}

// Native __CFMachMessageCheckForAndDestroyUnsentMessage, 0x1c01a55d4. External. For MACH_SEND_INTERRUPTED
// (0x10000007) and 0x10, a valid local port name (1 to 0xFFFFFFFD) whose disposition (bits 8-12) is 17 or 18
// is deallocated from mach_task_self_, and the local disposition bits are then cleared for any valid name.
// For MACH_SEND_INVALID_DEST (0x10000003) and 0x10 the message is destroyed (a tail call of
// mach_msg_destroy).
void __OCFMachMessageCheckForAndDestroyUnsentMessage(kern_return_t ret, mach_msg_header_t *msg) {
    if (ret == 0x10000007 || ret == 0x10) {
        mach_port_name_t name = msg->msgh_local_port;
        if ((uint32_t)(name - 1) <= 0xFFFFFFFC) {
            uint32_t bits = msg->msgh_bits;
            if ((uint32_t)(((bits >> 8) & 0x1F) - 17) <= 1) {
                mach_port_deallocate(mach_task_self_, name);
                bits = msg->msgh_bits;
            }
            msg->msgh_bits = bits & 0xFFFFE0FF;
        }
    }
    if (ret == 0x10000003 || ret == 0x10) {
        mach_msg_destroy(msg);
    }
}

//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "Foundation/Base/ONSPlatformShims.h"
#include "CoreFoundation/OFPortHooks.h"

#include <crt_externs.h>
#include <dispatch/dispatch.h>
#include <mach/mach.h>
#include <pthread/qos.h>

// Inferred organization: these helpers come from several native units (the platform shims at 0x1c05922c8,
// the URL support at 0x1c05e65a4, the file manager shims at 0x1c0c44068, a Swift-compiled dispatch helper at
// 0x1c0ce7134 and the Swift overlays' compatibility checks at 0x1c0f6be9c); they are grouped here because
// only the Swift code calls them. Unless a definition says otherwise: iOS Simulator 24A434, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Bindings: 0x22241A5B0 vm_page_size and 0x222419E80 mach_task_self_ (libsystem; confidence: medium).

// libsystem's environ lock (private; no SDK declaration) and dyld's SDK check.
extern void environ_lock_np(void);
extern void environ_unlock_np(void);
extern bool dyld_program_sdk_at_least(uint64_t version);
extern void *objc_autoreleaseReturnValue(void *object);
extern void *objc_claimAutoreleasedReturnValue(void *object);

// Native __platform_shims_lock_environ, 0x1c05922c8. A tail call.
void _platform_shims_lock_environ(void) {
    environ_lock_np();
}

// Native __platform_shims_unlock_environ, 0x1c05922cc. A tail call.
void _platform_shims_unlock_environ(void) {
    environ_unlock_np();
}

// Native __platform_shims_get_environ, 0x1c05922d0.
char **_platform_shims_get_environ(void) {
    return *_NSGetEnviron();
}

// Native __platform_shims_vm_size, 0x1c05922f8. vm_page_size.
uintptr_t _platform_shims_vm_size(void) {
    return vm_page_size;
}

// Native __platform_mach_task_self, 0x1c0592308. mach_task_self_.
uint32_t _platform_mach_task_self(void) {
    return mach_task_self_;
}

// Native __foundation_uuid_generate_random, 0x1c0592318. A tail call.
void _foundation_uuid_generate_random(uuid_t out) {
    uuid_generate_random(out);
}

// Native __foundation_uuid_parse, 0x1c059231c. A tail call.
int _foundation_uuid_parse(const char *in, uuid_t uu) {
    return uuid_parse(in, uu);
}

// Native __foundation_uuid_unparse_upper, 0x1c0592320. A tail call.
void _foundation_uuid_unparse_upper(const uuid_t uuid, uuid_string_t out) {
    uuid_unparse_upper(uuid, out);
}

// Native ___NSURLSupportDeprecatedParameterComponent, 0x1c05e65a4.
bool __NSURLSupportDeprecatedParameterComponent(void) {
    return false;
}

// Native __filemanager_shims_getgrnam_r, 0x1c0c44068. A tail call.
int _filemanager_shims_getgrnam_r(
    const char *name, struct group *group, char *buffer, size_t size, struct group **result
) {
    return getgrnam_r(name, group, buffer, size, result);
}

// Native ___NSDispatchQueueGetGenericMatchingMain, 0x1c0ce7134. Swift-compiled: the queue is claimed and
// returned autoreleased (objc_claimAutoreleasedReturnValue, objc_autoreleaseReturnValue).
dispatch_queue_t __NSDispatchQueueGetGenericMatchingMain(void) {
    dispatch_queue_t queue = dispatch_get_global_queue(qos_class_main(), 2);
    return (dispatch_queue_t)objc_autoreleaseReturnValue(objc_claimAutoreleasedReturnValue((void *)queue));
}

// Native __NSLinkedOnOrAfterLuckier, 0x1c0f6be9c.
bool _NSLinkedOnOrAfterLuckier(void) {
    return OF_PORT_ANSWER(
        programSDKAtLeastPacked, dyld_program_sdk_at_least(0x7E90000FFFFFFFFULL), 0x7E90000FFFFFFFFULL
    );
}

// Native __NSLinkedOnOrAfterCrystalglow, 0x1c0f6bff4.
bool _NSLinkedOnOrAfterCrystalglow(void) {
    return OF_PORT_ANSWER(
        programSDKAtLeastPacked, dyld_program_sdk_at_least(0x7E80000FFFFFFFFULL), 0x7E80000FFFFFFFFULL
    );
}

//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>
#include <dispatch/dispatch.h>
#include <grp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uuid/uuid.h>

// The C shims and compatibility checks of the Foundation image that its Swift code calls
// (Base/ONSPlatformShims.c), iOS Simulator 24A434, arm64, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified. iOS Device 24A446: unread. All hidden (were
// private externals) under their native names, which lie outside the CF/NS prefix rule (the _NS ones are
// internal helpers, as _NSNameOfClass).

// Native __platform_shims_lock_environ (0x1c05922c8), __platform_shims_unlock_environ (0x1c05922cc),
// __platform_shims_get_environ (0x1c05922d0), __platform_shims_vm_size (0x1c05922f8) and
// __platform_mach_task_self (0x1c0592308).
OF_EXTERN void _platform_shims_lock_environ(void);
OF_EXTERN void _platform_shims_unlock_environ(void);
OF_EXTERN char **_platform_shims_get_environ(void);
OF_EXTERN uintptr_t _platform_shims_vm_size(void);
OF_EXTERN uint32_t _platform_mach_task_self(void);

// Native __foundation_uuid_generate_random (0x1c0592318), __foundation_uuid_parse (0x1c059231c) and
// __foundation_uuid_unparse_upper (0x1c0592320).
OF_EXTERN void _foundation_uuid_generate_random(uuid_t out);
OF_EXTERN int _foundation_uuid_parse(const char *in, uuid_t uu);
OF_EXTERN void _foundation_uuid_unparse_upper(const uuid_t uuid, uuid_string_t out);

// Native ___NSURLSupportDeprecatedParameterComponent, 0x1c05e65a4: always false.
OF_EXTERN bool __NSURLSupportDeprecatedParameterComponent(void);

// Native __filemanager_shims_getgrnam_r, 0x1c0c44068.
OF_EXTERN int _filemanager_shims_getgrnam_r(
    const char *name, struct group *group, char *buffer, size_t size, struct group **result
);

// Native ___NSDispatchQueueGetGenericMatchingMain, 0x1c0ce7134: the global queue of the main thread's QoS.
OF_EXTERN dispatch_queue_t __NSDispatchQueueGetGenericMatchingMain(void);

// Native __NSLinkedOnOrAfterLuckier (0x1c0f6be9c) and __NSLinkedOnOrAfterCrystalglow (0x1c0f6bff4).
OF_EXTERN bool _NSLinkedOnOrAfterLuckier(void);
OF_EXTERN bool _NSLinkedOnOrAfterCrystalglow(void);

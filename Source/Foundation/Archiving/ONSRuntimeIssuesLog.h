//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>

#include <os/log.h>

// Native __NSRuntimeIssuesLog, 0x1c0593960 (was a private external; defined in
// Archiving/ONSRuntimeIssuesLog.m): the os_log handle for runtime issue reports, created once.
OF_EXTERN os_log_t _NSRuntimeIssuesLog(void);

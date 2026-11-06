//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSString.h"

#include <os/log.h>

// The NSLog functions (String/ONSLog.m), iOS Simulator 24A434, arm64, UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified unless a declaration says otherwise. iOS Device
// 24A446: unread.

// Native ___NSLogCString (0x22e776268, __DATA,__common, hidden, was a private external): the C-string log
// function _NSSetLogCStringFunction installs (not in the scope), or NULL. The adaptor passes the message, its
// length (all of X2) and the banner flag.
typedef void (*LogCStringFunction)(const char *message, size_t length, BOOL withSyslogBanner);
OF_EXTERN LogCStringFunction __NSLogCString;

// Native NSLog, 0x1c0593870, external (SDK NSObjCRuntime.h).
OF_EXPORT void ONSLog(id format, ...);

// Native __NS_os_log_callback, 0x1c0592324. External.
OF_EXPORT BOOL _ONS_os_log_callback(id object, char *buffer, size_t capacity, uint8_t *privacy);

// Native __NSOSLog, 0x1c0592ae4 (hidden, was a private external): the Foundation os_log handle, created once
// by the global block ____NSOSLog_block_invoke (0x1c05938c8) and kept in __MergedGlobals_100.
// The block was read in the supplemental IDA capture on 2026/11/03; confidence: verified.
OF_EXTERN os_log_t _NSOSLog(void);

// Native __NSLogv, 0x1c0fde558 (hidden, was a private external): logs the format and arguments through
// _CFLogvEx3 at level 4 with the caller's return address.
OF_EXTERN void _NSLogv(id format, va_list arguments, void *returnAddress);

// Native ___NSLogAdaptor, 0x1c0fde5d0 (hidden, was a private external): the CFLog function that forwards to
// __NSLogCString.
OF_EXTERN void __NSLogAdaptor(int32_t level, const char *message, size_t length, char withBanner);

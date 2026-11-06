//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Collections/OCFDictionary.h"

#include <os/log.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// The 8-byte result of _CFBuildVersionForCFSystemVersion, returned in X0: the platform in the low 32
// bits and the packed version in the high 32 bits (ORR X0, X8, X0, LSL #32), the layout of dyld's
// build-version pair. OCFBuildVersion and its field names are local descriptive spellings; iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified.
typedef struct {
    uint32_t platform;
    uint32_t version;
} OCFBuildVersion;

// systemVersion is compared as an unsigned 64-bit value; platform is a 32-bit dyld platform number.
OF_EXPORT OCFBuildVersion _OCFBuildVersionForCFSystemVersion(uint64_t systemVersion, uint32_t platform);

// Both return the dyld answer for the mapped build version unchanged.
OF_EXPORT bool _OCFPlatformExecutableLinkedOnOrAfter(uint64_t systemVersion, uint32_t platform);

OF_EXPORT bool _OCFExecutableLinkedOnOrAfter(uint64_t systemVersion);

// Log levels as the native callers pass them (3 error, 4 warning); the level indexes the "0".."7" table.
OF_EXTERN void _CFLogSimple(int32_t level, const char *format, ...) __attribute__((format(printf, 2, 3)));

OF_EXTERN void __CFLogCStringLegacy(int32_t level, const char *message, size_t length, bool withBanner);

// The effective user and group IDs of the process (or of the thread when they cannot change).
OF_EXTERN void __CFGetUGIDs(uint32_t *euid, uint32_t *egid);

OF_EXPORT uint32_t _OCFGetEUID(void);

// CFLog: formats with the string formatter and logs at the given level.
OF_EXPORT void OCFLog(int32_t level, OCFStringRef format, ...);

// The log function receives the level, the formatted UTF-8 message, its length and the banner flag.
typedef void (*OCFLogFunc)(int32_t level, const char *message, size_t length, char withBanner);

OF_EXPORT void _OCFLogvEx3(
    OCFLogFunc logit,
    OCFStringRef (*copyDescFunc)(void *, const void *),
    OCFStringRef (*contextDescFunc)(void *, const void *, const void *, bool, bool *),
    OCFDictionaryRef formatOptions,
    int32_t level,
    OCFStringRef format,
    va_list arguments,
    void *returnAddress
);

// Hidden (was a private external).
OF_EXTERN void _CFLogSimpleCString(int32_t level, const char *message, size_t length);

// The CoreFoundation os_log handle (hidden, was a private external).
OF_EXTERN os_log_t _CFOSLog(void);

// A CFNetwork symbol, or NULL (hidden, was a private external).
OF_EXTERN void *__CFLookupCFNetworkFunction(const char *name);

// The active processor count from the commpage (hidden, was a private external).
OF_EXTERN int32_t __CFActiveProcessorCount(void);

OF_EXPORT void _OCFSuddenTerminationDisable(void);
OF_EXPORT void _OCFSuddenTerminationEnable(void);

// Native _CFAppVersionCheckLessThan, 0x1c0259dbc (external; the CFUtilities cluster, after _logToStderr).
// systemVersion is a CFSystemVersion (W1, sign-extended), or 0xFFFFFFFF (-1) to skip the linked-on check.
// Native _CFAppVersionCheck, 0x1c0259ce8 (external): whether the main bundle has the identifier and, unless
// version is NULL, the short version string.
OF_EXPORT bool _OCFAppVersionCheck(OCFStringRef bundleID, OCFStringRef version);

OF_EXPORT bool _OCFAppVersionCheckLessThan(OCFStringRef bundleID, uint32_t systemVersion, double version);

// Native __CFGenerateReport, 0x1c02f8f14. Hidden (was a private external). The caller frees the report.
OF_EXTERN char *__CFGenerateReport(void);

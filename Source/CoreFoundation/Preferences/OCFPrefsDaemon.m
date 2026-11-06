//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Native _OUTLINED_FUNCTION_3_17, 0x1c02e9fe0. Non-external; folded register/frame/logging setup in its
// callers. Native _OUTLINED_FUNCTION_5_15, 0x1c02e9ffc. Non-external; folded register/frame/logging setup in
// its callers. Native _OUTLINED_FUNCTION_6_13, 0x1c02ea004. Non-external; folded register/frame/logging setup
// in its callers. Native _OUTLINED_FUNCTION_9_7, 0x1c02ea050. Non-external; folded register/frame/logging
// setup in its callers. Native _OUTLINED_FUNCTION_10_6, 0x1c02ea058. Non-external; folded
// register/frame/logging setup in its callers. Native _OUTLINED_FUNCTION_11_6, 0x1c02ea064. Non-external;
// folded register/frame/logging setup in its callers. Native _OUTLINED_FUNCTION_12_6, 0x1c02ea070.
// Non-external; folded register/frame/logging setup in its callers.
#import "CoreFoundation/Preferences/OCFPrefsDaemon.h"
#import "CoreFoundation/Preferences/OCFPDSource.h"
#import "CoreFoundation/Preferences/OCFPreferences.h"
#import "CoreFoundation/Preferences/OCFPreferencesDescription.h"
#import "CoreFoundation/Preferences/OCFPrefsPlistSource.h"
#import "CoreFoundation/Preferences/OCFXPCBridge.h"
#import "CoreFoundation/Preferences/OCFXPreferences.h"
#import "CoreFoundation/Base/OCFGroupContainers.h"
#import "CoreFoundation/URL/OCFURL.h"
#import "CoreFoundation/String/OCFStringFormat.h"

#include <alloca.h>
#include <assert.h>
#include <errno.h>
#include <mach/mach_time.h>
#include <os/signpost.h>
#include <pwd.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <dispatch/dispatch.h>
#include <fcntl.h>
#include <malloc/malloc.h>
#include <os/lock.h>
#include <os/log.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <unistd.h>

// Inferred organization: the native unit is cfprefsd.m (the file name of its assertions). Its restored
// functions run from populateErrorReply (0x1c02e58c4) through
// ____CFPrefsCopyEntitlementForMessage_block_invoke (0x1c02e9ea4) in the iOS Simulator __text section, with
// the unit's direct methods and .cold fragments in the cold region (0x1c0314ab4-0x1c03159e0) and
// -[CFPrefsDaemon shmem] (0x1c02e93ac); its C functions precede the
// @implementation here, and the methods follow in native address order.
// ____CFPrefsProcessPathRequiresDirectMode_block_invoke (0x1c02e983c) lies after -enterSandbox; its function,
// _CFPrefsProcessPathRequiresDirectMode (no symbol, inlined natively into _CFPrefsDirectMode by link-time
// optimization), is restored here with it. The machine-outlined fragments (OUTLINED_FUNCTION_0_58, _1_41,
// _2_35, _3_17, _4_15, _5_15, _6_13, _7_11, _8_10, _9_7, _10_6, _11_6, _12_6 and _13_6,
// 0x1c02e9f9c-0x1c02ea084) are kept in their parents. -handleSourceMessage:originalMessage:replyHandler:
// with its blocks and .cold fragments, _CFPrefsPIDForMessage, _CFPrefsCopyFixedUpContainerForMessage with the
// inlined _CFPrefsGetSuiteContainer and _CFPrefsIfClientIsInSecurityApplicationGroup and their blocks,
// _CFPrefsSandboxCheck (the cfprefsd.m copy), _CFPrefsBooleanEntitlementValueForMessage,
// _CFPrefsCopyEntitlementForMessage and the inlined _CFPrefsMessageSenderIsSandboxed with its block come from
// the delta11 capture; the global blocks of -initWithRole:testMode:,
// -handleMultiMessage:originalMessage:replyHandler:, -handleFlushSourceForDomainMessage:replyHandler:,
// -handleSourceMessage:originalMessage:replyHandler: and _CFPrefsIsAppleInternal, -updateShmemIndex:, -role
// and -getShmemName:bufLen: from the delta12 capture
// (Local/Evidence/cf-core/iphone-simulator-corpus-delta12.json). The
// unit is Objective-C compiled with manual reference counting. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full;
// the delta11 additions read from Local/Evidence/cf-core/iphone-simulator-corpus-delta11.json). iOS Device
// 24A446: unread. Reading: Local/Scratch/cf-core-prefs-daemon/REPORT.md (main checkout; integration reading
// pending).
//
// XPC type and constant objects are read through GOT slots outside the extracted image; their bindings
// (0x22241b900 XPC_BOOL_TRUE, 0x22241b910 XPC_ERROR_CONNECTION_INVALID, 0x22241b968 XPC_TYPE_ARRAY,
// 0x22241b970 XPC_TYPE_BOOL, 0x22241b980 XPC_TYPE_CONNECTION, 0x22241b998 XPC_TYPE_DICTIONARY, 0x22241b9b0
// XPC_TYPE_ERROR, 0x22241b9e8 XPC_TYPE_NULL) are inferred from the alphabetical, 8-byte-stride order of the
// image's libxpc data imports and from their use (the assertion "type == XPC_TYPE_DICTIONARY" settles
// 0x22241b998): confidence medium. 0x22241b9c8 (XPC_TYPE_INT64, its object read with xpc_int64_get_value) and
// 0x22241ba28 (XPC_TYPE_STRING, compared with xpc_equal against string entitlements and the result of
// xpc_string_create) are bound from their use alone: confidence medium.

// kCFStringEncodingUTF8 as the native call sites pass it.
#define kUTF8Encoding ((OCFStringEncoding)0x08000100)

// libsystem_sandbox's sandbox_check and its no-report flag (no SDK declaration); supplied. The flag is read
// through GOT slot 0x22241b290 (SANDBOX_CHECK_NO_REPORT, the binding inferred as in
// Preferences/OCFPrefsSource.m: confidence medium); the filter type 17 is ORed in, and the shmem name passed.
extern const uint32_t SANDBOX_CHECK_NO_REPORT;
extern int sandbox_check(pid_t pid, const char *operation, int type, ...);

// The other libsystem_sandbox interfaces the unit calls (no SDK declarations); supplied. GOT slots
// 0x22241b278 and 0x22241b288 are bound to SANDBOX_CHECK_ALLOW_APPROVAL and SANDBOX_CHECK_NOFOLLOW by the
// alphabetical, 8-byte-stride order of the image's four SANDBOX_CHECK_* imports (ALLOW_APPROVAL, CANONICAL,
// NOFOLLOW, NO_REPORT at 0x22241b278-0x22241b290): confidence medium. SANDBOX_FILTER_PATH (1) and
// SANDBOX_FILTER_PREFERENCE_DOMAIN (6) are ORed in as the native call sites do.
extern const uint32_t SANDBOX_CHECK_ALLOW_APPROVAL;
extern const uint32_t SANDBOX_CHECK_NOFOLLOW;
extern int sandbox_check_by_audit_token(audit_token_t token, const char *operation, int type, ...);
extern int sandbox_check_and_query_approval_category(
    audit_token_t token, char **approvalCategory, const char *operation, int type, ...
);
extern int sandbox_container_path_for_audit_token(audit_token_t token, char *buffer, size_t size);

// The shmem page size the native code maps and allocates.
#define kShmemPageSize 0x4000

// libxpc and libdispatch interfaces the SDK marks unavailable on iOS or does not install; supplied, never
// reimplemented. xpc_connection_create_mach_service and xpc_connection_get_pid are bound under local names
// (as in Preferences/OCFXPreferences.m).
extern xpc_connection_t createMachServiceConnection(
    const char *name, dispatch_queue_t targetQueue, uint64_t flags
) __asm("_xpc_connection_create_mach_service");
extern pid_t connectionProcessIdentifier(xpc_connection_t connection) __asm("_xpc_connection_get_pid");
extern void xpc_connection_get_audit_token(xpc_connection_t connection, audit_token_t *token);
extern xpc_object_t xpc_copy_entitlement_for_token(const char *entitlement, audit_token_t *token);
extern void
xpc_dictionary_handoff_reply(xpc_object_t message, dispatch_queue_t queue, dispatch_block_t block);
extern void *os_transaction_create(const char *description);
extern bool _os_feature_enabled_impl(const char *domain, const char *feature);
extern int proc_name(int pid, void *buffer, uint32_t buffersize);
extern int proc_pidpath(int pid, void *buffer, uint32_t buffersize);

// CFPDCloudSource, a class of this image the unit references by class symbol and that is not restored on this
// branch; declared, not defined. The CFPDSource classes are Preferences/OCFPDSource.h's and
// _CFPrefsClientContext is defined below.
extern char cloudSourceClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(OCFPDCloudSource));

// Selectors this unit sends to the daemon-side sources and tombstones, declared for the message sends only;
// no protocol object is emitted.
@protocol DaemonSourceMessages

- (BOOL)managed;
- (int)shmemIndex;
- (void)observingConnectionWasInvalidated:(xpc_connection_t)connection;
- (void)removeObservingConnection:(xpc_connection_t)connection daemon:(id)daemon;
- (void)respondToFileWrittenToBehindOurBack;
- (void)notifyObservers;
- (void)acceptMessage:(xpc_object_t)message populatingReply:(xpc_object_t)reply;
- (void)cleanUpAfterAcceptingMessage:(xpc_object_t)message;
- (void)synchronizeWithCloud;
- (id)initWithDomain:(OCFStringRef)domain
            userName:(OCFStringRef)user
           container:(OCFStringRef)container
              byHost:(BOOL)byHost
             managed:(BOOL)managed
          shmemIndex:(int16_t)shmemIndex
              daemon:(id)daemon;
- (id)initWithDomain:(OCFStringRef)domain
            userName:(OCFStringRef)user
              byHost:(BOOL)byHost
             managed:(BOOL)managed
        mirroredKeys:(id)mirroredKeys
          shmemIndex:(int16_t)shmemIndex
              daemon:(id)daemon;
- (id)initWithDomain:(OCFStringRef)domain
            userName:(OCFStringRef)user
              byHost:(BOOL)byHost
             managed:(BOOL)managed
          shmemIndex:(int16_t)shmemIndex
              daemon:(id)daemon;

@end

// The layout of a _CFPrefsClientContext (class_ro_t 0x2227c7ca0: instance size 55; the ivars are reached
// with fixed offsets natively). The type encodings lie outside the extracted image; the ivar names come from
// the image's _OBJC_IVAR_$__CFPrefsClientContext.* symbols (nm, matched by ivar offset address): _cfprefsd
// 8, _suiteCache 16, _observedSources 24, _entitlements 32, _appGroups 40, _lock 48, _sandboxed 52,
// _isPlatformBinary 53 and the bit fields at 54 (_impersonatingAnotherProcess 1, _hasInFlightKVONotifications
// 2, _didDeferKVONotifications 4, _valid 8, _canImpersonate 0x10, _hasPopulatedEntitlements 0x20).
// ClientContextLayout and its field spellings are local; the types are the ones the restored code
// establishes: the daemon (stored with release and loaded with acquire ordering), the suite container cache
// (a CFMutableDictionary of container URLs, or kCFNull, by domain), the observed sources (a CFSet handed over
// when the connection is invalidated), the entitlements (an XPC dictionary), the application groups (a CF
// array), the lock, the sandboxed and platform-binary states (0xff unknown) and the flags.
typedef struct {
    Class isa;
    _Atomic(OCFPrefsDaemon *) daemon;
    OCFMutableDictionaryRef suiteCache;
    OCFSetRef observedSources;
    xpc_object_t entitlements;
    OCFTypeRef applicationGroups;
    os_unfair_lock lock;
    uint8_t sandboxed;
    uint8_t isPlatformBinary;
    uint8_t flags;
} ClientContextLayout;

_Static_assert(__builtin_offsetof(ClientContextLayout, flags) == 54, "The client context flags are at 54.");

// Native _globalContextLock (__DATA,__bss 0x22e6d3cc0). Non-external. Orders context lookups against the
// connection teardown.
static os_unfair_lock _globalContextLock = OS_UNFAIR_LOCK_INIT;

// Native terminating (__DATA,__bss 0x22e6d3cd8). Non-external; its writer is outside the evidence, so it
// stays zero in this restoration.
static bool terminating;

// Native initWithRole:testMode:.onceToken (__DATA,__bss 0x22e6d3cd0). Non-external: the once predicate of
// -initWithRole:testMode: for the role-0 daemon.
static dispatch_once_t initWithRoleOnceToken;

// Native ___39-[CFPrefsDaemon initWithRole:testMode:]_block_invoke, 0x1c02e8e40. Native empty block (RET).
static void (^const initWithRoleOnceBlock)(void) = ^{
};

// Native ___65-[CFPrefsDaemon handleMultiMessage:originalMessage:replyHandler:]_block_invoke, 0x1c02e8a40.
static bool (^const multiMessageValidatorBlock)(size_t, xpc_object_t) =
    ^bool(size_t index, xpc_object_t value) {
      (void)index;
      xpc_type_t type = xpc_get_type(value);
      return type == XPC_TYPE_DICTIONARY || type == XPC_TYPE_NULL;
    };

// Native ___64-[CFPrefsDaemon handleFlushSourceForDomainMessage:replyHandler:]_block_invoke, 0x1c02e8a0c.
static void (^const flushSourceSnapshotBlock)(id *, OCFIndex) = ^(id *sources, OCFIndex count) {
  for (OCFIndex remaining = count; remaining != 0; --remaining) {
      [(id<DaemonSourceMessages>)*sources++ respondToFileWrittenToBehindOurBack];
  }
};

// Native entitlementsWeCareAboutInEduMode (__DATA_CONST,__const 0x2227aaea0, five C string pointers, read
// with Local/Scratch/cf-core/macho.py). Non-external.
static const char *const entitlementsWeCareAboutInEduMode[5] = {
    "com.apple.developer.ubiquity-kvstore-identifier",
    "com.apple.private.trust-defaults-kvstore-identifier",
    "com.apple.private.ubiquity-kvstore-access",
    "com.apple.private.ubiquity-additional-kvstore-identifiers",
    "com.apple.private.defaults-shared-ipad-loginui",
};

// The processes whose requests stay on a serial queue rather than an incoming workloop (__DATA_CONST,__const
// 0x2227aaf88, four C string pointers, unnamed natively; read with Local/Scratch/cf-core/macho.py).
static const char *const serialQueueProcessPaths[4] = {
    "/usr/bin/security",
    "/usr/libexec/lsd",
    "/usr/libexec/sandboxd",
    "/System/Library/PrivateFrameworks/TCC.framework/Support/tccd",
};

// Native _CFPrefsConnectionForMessage: no symbol; inlined from CFXPreferences_Internal.h into every caller
// (its assertion is OUTLINED_FUNCTION_0_58 and the callers' .cold fragments). The message's remote
// connection, else its "connection" value, which must be an XPC connection ("conn && xpc_get_type(conn) ==
// XPC_TYPE_CONNECTION", line 427).
static inline xpc_connection_t _CFPrefsConnectionForMessage(xpc_object_t message) {
    xpc_connection_t connection = xpc_dictionary_get_remote_connection(message);
    if (connection == NULL) {
        connection = xpc_dictionary_get_value(message, "connection");
        if (connection == NULL || xpc_get_type(connection) != XPC_TYPE_CONNECTION) {
            __assert_rtn(
                "_CFPrefsConnectionForMessage",
                "CFXPreferences_Internal.h",
                427,
                "conn && xpc_get_type(conn) == XPC_TYPE_CONNECTION"
            );
        }
    }
    return connection;
}

// The audit token of a message as the callers inline it: zeroed, then the "CFPreferencesAuditToken" data when
// it is exactly 32 bytes long, else the token of the message's connection. auditTokenForMessage is a local
// descriptive name for the inlined code.
static inline audit_token_t auditTokenForMessage(xpc_object_t message) {
    size_t length = 0;
    audit_token_t token;
    memset(&token, 0, sizeof(token));
    const void *data = xpc_dictionary_get_data(message, "CFPreferencesAuditToken", &length);
    if (data != NULL && length == 32) {
        memcpy(&token, data, sizeof(token));
    } else {
        xpc_connection_get_audit_token(_CFPrefsConnectionForMessage(message), &token);
    }
    return token;
}

// Native populateErrorReply, 0x1c02e58c4, with .cold.1 (0x1c0315420, the assertion "reply", cfprefsd.m line
// 174). Hidden (was a private external). The property list is removed, the description set when given and
// the error type stored.
void populateErrorReply(const char *description, xpc_object_t reply, int64_t errorType) {
    if (reply == NULL) {
        __assert_rtn("populateErrorReply", "cfprefsd.m", 174, "reply");
    }
    xpc_dictionary_set_value(reply, "CFPreferencesPropertyList", NULL);
    if (description != NULL) {
        xpc_dictionary_set_string(reply, "CFPreferencesErrorDescription", description);
    }
    xpc_dictionary_set_int64(reply, "CFPreferencesErrorType", errorType);
}

// Native withClientContext, 0x1c02e5934, with .cold.1 (0x1c0315448, the assertion "ctxt", cfprefsd.m line
// 228). Hidden (was a private external). Under the global context lock the connection's context (required)
// is locked and its valid flag read; the global lock is released first. A valid context is passed to the
// block under its own lock. Answers whether the context was valid.
bool withClientContext(xpc_connection_t connection, void (^block)(id context)) {
    os_unfair_lock_lock(&_globalContextLock);
    ClientContextLayout *context = xpc_connection_get_context(connection);
    if (context == NULL) {
        __assert_rtn("withClientContext", "cfprefsd.m", 228, "ctxt");
    }
    os_unfair_lock_lock(&context->lock);
    uint8_t flags = context->flags;
    os_unfair_lock_unlock(&_globalContextLock);
    if ((flags & 8) != 0) {
        block((id)context);
    }
    os_unfair_lock_unlock(&context->lock);
    return (flags & 8) != 0;
}

static xpc_object_t _CFPrefsGetEntitlementForMessageWithLockedContext(
    xpc_object_t message, const char *key, ClientContextLayout *context
);

// Native _CFPrefsFixUpIncomingMessageForPIDImpersonationIfNeeded, 0x1c02e59b8, with .cold.1 (0x1c0315470) and
// ____CFPrefsFixUpIncomingMessageForPIDImpersonationIfNeeded_block_invoke (0x1c02e5ad0, descriptor
// 0x2227aab30, with its .cold.1 and .cold.2, 0x1c031547c and 0x1c0315488: the _CFPrefsConnectionForMessage
// assertions). Hidden (was a private external).
// Any client-supplied "CFPreferencesAuditToken" is removed. When the message carries a 32-byte
// "CFPreferencesAuditTokenToImpersonate", the connection's context decides under its lock: once impersonation
// is checked, or when the client's entitlements (fetched now) allow it, the token becomes the message's
// "CFPreferencesAuditToken"; on the first allowed use the cached entitlements and application groups are
// released and cleared and the flags become checked (the may-impersonate and fetched bits cleared). Otherwise
// the client's process name (proc_name into a zeroed 256-byte buffer) is looked up and a fault logged with
// its pid (the token is read again inside the log's enabled check).
void _CFPrefsFixUpIncomingMessageForPIDImpersonationIfNeeded(xpc_object_t message) {
    xpc_dictionary_set_value(message, "CFPreferencesAuditToken", NULL);
    size_t length = 0;
    const void *token = xpc_dictionary_get_data(message, "CFPreferencesAuditTokenToImpersonate", &length);
    if (token == NULL || length != 32) {
        return;
    }
    withClientContext(_CFPrefsConnectionForMessage(message), ^(id contextObject) {
      ClientContextLayout *context = (ClientContextLayout *)contextObject;
      if ((context->flags & 1) != 0 ||
          (_CFPrefsGetEntitlementForMessageWithLockedContext(message, NULL, context),
           (context->flags & 0x10) != 0)) {
          xpc_dictionary_set_data(message, "CFPreferencesAuditToken", token, length);
          if ((context->flags & 1) == 0) {
              xpc_object_t entitlements = context->entitlements;
              if (entitlements != NULL) {
                  context->entitlements = NULL;
                  xpc_release(entitlements);
              }
              OCFTypeRef groups = context->applicationGroups;
              if (groups != NULL) {
                  context->applicationGroups = NULL;
                  OCFRelease(groups);
              }
              context->flags = (uint8_t)((context->flags & 0xCE) | 1);
          }
      } else {
          char name[256];
          memset(name, 0, sizeof(name));
          proc_name((int)auditTokenForMessage(message).val[5], name, 256);
          os_log_fault(
              _CFPrefsDaemonLog(),
              "Not allowing process impersonation by process %{public}s (%{public}d) due to missing "
              "com.apple.private.defaults-impersonate entitlement.",
              name,
              (int)auditTokenForMessage(message).val[5]
          );
      }
    });
}

// Native _CFPrefsGetEntitlementForMessageWithLockedContext, 0x1c02e5d54, with .cold.1 (0x1c0315494, the
// _CFPrefsConnectionForMessage assertion), .cold.2 (0x1c03154a0, "matched", cfprefsd.m line 285) and .cold.3
// (0x1c03154c8, "message", line 249). Non-external.
// The context's cached entitlements answer when present; when none are cached and they were already fetched,
// NULL. Otherwise the client's entitlements are copied (xpc_copy_entitlement_for_token with no key and the
// message's audit token): a nonempty "com.apple.security.application-groups" array is converted to CF into
// the context; a boolean "com.apple.private.defaults-impersonate" sets or clears the may-impersonate flag.
// Each of the five education-mode entitlements is compared with the key (any key matches when none is given),
// and in education mode copied (when present) into a new dictionary, which is cached (NULL outside education
// mode); a key that matches none of them fails the assertion. The copy is released and the fetched flag set.
// The answer is the cached value for the key (NULL without a key or a cache).
static xpc_object_t _CFPrefsGetEntitlementForMessageWithLockedContext(
    xpc_object_t message, const char *key, ClientContextLayout *context
) {
    if (message == NULL) {
        __assert_rtn("_CFPrefsGetEntitlementForMessageWithLockedContext", "cfprefsd.m", 249, "message");
    }
    xpc_object_t cached = context->entitlements;
    if (cached == NULL) {
        if ((context->flags & 0x20) != 0) {
            return NULL;
        }
        audit_token_t token = auditTokenForMessage(message);
        xpc_object_t entitlements = xpc_copy_entitlement_for_token(NULL, &token);
        if (entitlements != NULL) {
            xpc_object_t groups =
                xpc_dictionary_get_array(entitlements, "com.apple.security.application-groups");
            if (groups != NULL && xpc_array_get_count(groups) != 0) {
                context->applicationGroups = _OCFXPCCreateCFObjectFromXPCObject(groups);
            }
            xpc_object_t impersonate =
                xpc_dictionary_get_value(entitlements, "com.apple.private.defaults-impersonate");
            if (impersonate != NULL && xpc_get_type(impersonate) == XPC_TYPE_BOOL) {
                uint8_t allowed = xpc_bool_get_value(impersonate) ? 0x10 : 0;
                context->flags = (uint8_t)((context->flags & 0xEF) | allowed);
            }
            bool matched = false;
            bool lastMatched = false;
            for (size_t index = 0; index != 5; ++index) {
                if (key != NULL) {
                    matched |= strcmp(entitlementsWeCareAboutInEduMode[index], key) == 0;
                    lastMatched = matched;
                } else {
                    lastMatched = true;
                }
                if (eduModeEnabled()) {
                    const char *name = entitlementsWeCareAboutInEduMode[index];
                    xpc_object_t value = xpc_dictionary_get_value(entitlements, name);
                    if (value != NULL) {
                        if (cached == NULL) {
                            cached = xpc_dictionary_create(NULL, NULL, 0);
                        }
                        xpc_dictionary_set_value(cached, name, value);
                    }
                }
            }
            if (!lastMatched) {
                __assert_rtn(
                    "_CFPrefsGetEntitlementForMessageWithLockedContext", "cfprefsd.m", 285, "matched"
                );
            }
            xpc_release(entitlements);
        } else {
            cached = NULL;
        }
        context->entitlements = cached;
        context->flags |= 0x20;
    }
    if (key != NULL && cached != NULL) {
        return xpc_dictionary_get_value(cached, key);
    }
    return NULL;
}

// Native _CFPrefsPIDForMessage, 0x1c02e5fcc, with .cold.1 (0x1c03154f0, the _CFPrefsConnectionForMessage
// assertion through OUTLINED_FUNCTION_0_58). Non-external. The process identifier (audit token word 5) of
// the message's audit token.
static pid_t _CFPrefsPIDForMessage(xpc_object_t message) {
    return (pid_t)auditTokenForMessage(message).val[5];
}

// Native _CFPrefsIsAppleInternal: no symbol; inlined into _CFPrefsGetFixedUpDomainForMessage. Its statics
// .internal and .onceToken (__DATA,__bss 0x22e6d3d11 and 0x22e6d3d18) and its once block (.cold.1,
// 0x1c03154fc) with the global block ___block_literal_global.178 (0x2227aaec8), whose invoke
// ____CFPrefsIsAppleInternal_block_invoke (0x1c02e98ec) sets the byte to 1 when access("/AppleInternal",
// F_OK) succeeds. The answer is whether the byte is 1.
static bool isAppleInternal;
static dispatch_once_t isAppleInternalOnceToken;

// Native ____CFPrefsIsAppleInternal_block_invoke, 0x1c02e98ec. Non-external.
static void (^const isAppleInternalOnceBlock)(void) = ^{
  if (access("/AppleInternal", F_OK) == 0) {
      isAppleInternal = true;
  }
};

static inline bool _CFPrefsIsAppleInternal(void) {
    dispatch_once(&isAppleInternalOnceToken, isAppleInternalOnceBlock);
    return isAppleInternal == 1;
}

// Native _CFPrefsGetFixedUpDomainForMessage, 0x1c02e60a4, with .cold.1 (0x1c03154fc, the once of
// _CFPrefsIsAppleInternal) and .cold.2 (0x1c0315510, the error log). Hidden (was a private external).
// No "CFPreferencesDomain" string answers NULL (the out-parameter untouched). The string is made into a CF
// string (UTF-8, system default allocator); when that fails, the C string is stored and NULL answered.
// kCFPreferencesAnyApplication, "Apple Global Domain" and ".GlobalPreferences" are normalized: the copy is
// released, the message's domain rewritten to the constant's name (OF_NAME_STRING,
// "kCFPreferencesAnyApplication" natively) and the constant used. A domain containing ".." or starting with
// '/' is logged (error level, "invalid argument (path instead of domain/application identifier) %{private}s")
// on Apple-internal builds unless the write is synchronous. kCFPreferencesCurrentApplication, and
// kCFPreferencesAnyUser (compared first) when not by host, answer NULL with a NULL C string; any other domain
// is interned. The domain string is released and the C string stored.
OCFStringRef _CFPrefsGetFixedUpDomainForMessage(
    xpc_object_t message, OCFStringRef user, bool byHost, const char **outDomain
) {
    const char *domain = xpc_dictionary_get_string(message, "CFPreferencesDomain");
    if (domain == NULL) {
        return NULL;
    }
    OCFStringRef string = OCFStringCreateWithCString(kOCFAllocatorSystemDefault, domain, kUTF8Encoding);
    OCFStringRef result = NULL;
    if (string != NULL) {
        if (OCFEqual(string, kOCFPreferencesAnyApplication) ||
            OCFEqual(OF_STR("Apple Global Domain"), string) ||
            OCFEqual(OF_STR(".GlobalPreferences"), string)) {
            OCFRelease(string);
            domain = OF_NAME_STRING(kOCFPreferencesAnyApplication);
            xpc_dictionary_set_string(message, "CFPreferencesDomain", domain);
            string = kOCFPreferencesAnyApplication;
        }
        if (OCFStringFind(string, OF_STR(".."), 0).location != kOCFNotFound ||
            OCFStringGetCharacterAtIndex(string, 0) == '/') {
            if (!xpc_dictionary_get_bool(message, "CFPreferencesShouldWriteSynchronously") &&
                _CFPrefsIsAppleInternal()) {
                os_log_error(
                    _CFPrefsDaemonLog(),
                    "invalid argument (path instead of domain/application identifier) %{private}s",
                    domain
                );
            }
        }
        bool anyUser = false;
        bool rejected = OCFEqual(string, kOCFPreferencesCurrentApplication);
        if (!rejected && user != NULL) {
            anyUser = OCFEqual(user, kOCFPreferencesAnyUser);
            rejected = !byHost && anyUser;
        }
        if (rejected) {
            domain = NULL;
        } else {
            result = _CFPrefsDInternString(string);
        }
        OCFRelease(string);
    }
    if (outDomain != NULL) {
        *outDomain = domain;
    }
    return result;
}

static int _CFPrefsSandboxCheck(
    audit_token_t token, const char *operation, int type, const char *path, bool *outNeedsApproval
);

// Native _CFPrefsIfClientIsInSecurityApplicationGroup: no symbol; inlined into
// ____CFPrefsGetSuiteContainer_block_invoke. Its assertion "domain" (cfprefsd.m line 331) is
// ____CFPrefsGetSuiteContainer_block_invoke.cold.1 (0x1c0315a68); its block is
// ____CFPrefsIfClientIsInSecurityApplicationGroup_block_invoke (0x1c02e9c78, descriptor 0x2227aa018). For the
// current application nothing is looked up. Otherwise the groups (the whole range) are searched for the
// domain with CFEqual; on a match the domain is stored in a __block reference and the search stopped, and the
// answer is the block's. The parameter names are local descriptive spellings.
static inline bool _CFPrefsIfClientIsInSecurityApplicationGroup(
    OCFArrayRef groups, OCFStringRef domain, bool isCurrentApplication, bool (^block)(void)
) {
    if (domain == NULL) {
        __assert_rtn("_CFPrefsIfClientIsInSecurityApplicationGroup", "cfprefsd.m", 331, "domain");
    }
    if (isCurrentApplication) {
        return false;
    }
    __block OCFStringRef found = NULL;
    CFArrayApply(groups, (OCFRange){0, OCFArrayGetCount(groups)}, ^(const void *value, bool *stop) {
      if (OCFEqual(value, domain)) {
          found = domain;
          *stop = true;
      }
    });
    if (found != NULL) {
        return block();
    }
    return false;
}

// Native _CFPrefsGetSuiteContainer: no symbol; inlined into _CFPrefsCopyFixedUpContainerForMessage. Its
// assertion "domain" (cfprefsd.m line 356) is _CFPrefsCopyFixedUpContainerForMessage.cold.4 (0x1c03155dc);
// its blocks are ____CFPrefsGetSuiteContainer_block_invoke (0x1c02e991c, descriptor 0x2227aaf18, with
// .cold.1) and ____CFPrefsGetSuiteContainer_block_invoke_2 (0x1c02e9b84, descriptor 0x2227aaef0, with
// .cold.1, the error log through OUTLINED_FUNCTION_13_6, _4_15, _3_17 and _5_15, and .cold.2, the
// _CFPrefsConnectionForMessage assertion). The parameter names are local descriptive spellings.
// For the current application, false. Outside cfprefsd with a non-launch persona (_CFPNLPO not -1), the plist
// path of "<domain>-fakePersona(<persona>)" (CFStringCreateWithFormat, default allocator) for the current
// user, not by host and without a container, is written to the buffer. Otherwise, under the client's context
// lock: a cached URL for the domain answers its file system representation (resolved against its base), a
// cached kCFNull answers false. Uncached, the entitlements are fetched (no key) and the application groups
// retained; without groups the answer is false. The cache is created on first use (CFType keys and values,
// system default allocator). When the domain is one of the groups, the container URL of that group for the
// host bundle identifier and the connection's effective user is made (a root client is refused with an error
// log): its representation is written and on success the URL cached. A domain without a container is cached
// as kCFNull; the groups are released.
static inline bool _CFPrefsGetSuiteContainer(
    xpc_object_t message,
    OCFStringRef domain,
    char *buffer,
    OCFIndex size,
    OCFStringRef hostIdentifier,
    bool isCurrentApplication
) {
    if (domain == NULL) {
        __assert_rtn("_CFPrefsGetSuiteContainer", "cfprefsd.m", 356, "domain");
    }
    if (isCurrentApplication) {
        return false;
    }
    if ((isCFPrefsD & 1) == 0 && _CFPNLPO != -1) {
        OCFStringRef personaDomain =
            OCFStringCreateWithFormat(NULL, NULL, OF_STR("%@-fakePersona(%d)"), domain, _CFPNLPO);
        bool result =
            _CFPrefsGetPathForTriplet(personaDomain, kOCFPreferencesCurrentUser, false, NULL, buffer);
        OCFRelease(personaDomain);
        return result;
    }
    __block bool found = false;
    withClientContext(_CFPrefsConnectionForMessage(message), ^(id contextObject) {
      ClientContextLayout *context = (ClientContextLayout *)contextObject;
      OCFMutableDictionaryRef cache = context->suiteCache;
      if (cache != NULL) {
          OCFURLRef url = OCFDictionaryGetValue(cache, domain);
          if (url != NULL) {
              if ((OCFTypeRef)url == (OCFTypeRef)kOCFNull) {
                  found = false;
                  return;
              }
              found = OCFURLGetFileSystemRepresentation(url, true, (UInt8 *)buffer, size) != 0;
              return;
          }
      }
      _CFPrefsGetEntitlementForMessageWithLockedContext(message, NULL, context);
      OCFTypeRef groups = context->applicationGroups;
      if (groups == NULL || (groups = OCFRetain(groups)) == NULL) {
          found = false;
          return;
      }
      if (cache == NULL) {
          cache = OCFDictionaryCreateMutable(
              kOCFAllocatorSystemDefault,
              0,
              &kOCFTypeDictionaryKeyCallBacks,
              &kOCFTypeDictionaryValueCallBacks
          );
          context->suiteCache = cache;
      }
      found = _CFPrefsIfClientIsInSecurityApplicationGroup(
          (OCFArrayRef)groups, domain, isCurrentApplication, ^bool(void) {
            xpc_connection_t connection = _CFPrefsConnectionForMessage(message);
            uid_t euid = xpc_connection_get_euid(connection);
            if (euid == 0) {
                OCFStringRef client = CFPreferencesClientDescriptionForMessage(message);
                os_log_error(
                    _CFPrefsDaemonLog(),
                    "Process %{public}@ running as root is attempting to look up an app group container. "
                    "That's "
                    "not supported",
                    client
                );
                return false;
            }
            OCFURLRef containerURL =
                _OCFCreateContainerURLForSecurityApplicationIdentifierGroupIdentifierAndUser(
                    hostIdentifier, domain, euid
                );
            if (containerURL == NULL) {
                return false;
            }
            bool represented =
                OCFURLGetFileSystemRepresentation(containerURL, true, (UInt8 *)buffer, size) != 0;
            if (represented) {
                OCFDictionarySetValue(cache, domain, containerURL);
            }
            OCFRelease(containerURL);
            return represented;
          }
      );
      if (!found) {
          OCFDictionarySetValue(cache, domain, kOCFNull);
      }
      OCFRelease(groups);
    });
    return found;
}

// Folded native ____CFPrefsGetSuiteContainer_block_invoke_2.cold.1, 0x1c0315a90.
// Folded native ____CFPrefsGetSuiteContainer_block_invoke_2.cold.2, 0x1c0315ac0.
// Native _CFPrefsCopyFixedUpContainerForMessage, 0x1c02e6290, with .cold.1 and .cold.2 (0x1c0315540 and
// 0x1c03155a0, the error logs), .cold.3 (0x1c03155d0, the _CFPrefsConnectionForMessage assertion) and .cold.4
// (0x1c03155dc, the _CFPrefsGetSuiteContainer assertion). Hidden (was a private external).
// The client's "CFPreferencesContainer" path is replaced by the suite container (a zeroed 1,026-byte buffer;
// the "CFPreferencesHostBundleIdentifier" value converted from XPC, released after the lookup) when there is
// one. A nonempty path other than "kCFPreferencesNoContainer" (compared over 25 bytes; that one answers NULL)
// is checked for "file-read-data" by this process (errno cleared first; flags NO_REPORT, NOFOLLOW and the
// path filter): when refused, the client's path is answered as given (an errno is logged for -1). When
// allowed, it is opened with O_SEARCH and its canonical path (F_GETPATH) answered (NULL when that fails; the
// descriptor is closed); a path that cannot be opened is logged and answered as given. Without a path: only
// for a sandboxed client that specified a user is the client's sandbox container (for its audit token)
// answered, and only when both the "user-preference-read" and "user-preference-write" checks of the domain
// (NO_REPORT and ALLOW_APPROVAL with the preference-domain filter) refuse; otherwise NULL. Paths are made
// with the system default allocator.
OCFStringRef _CFPrefsCopyFixedUpContainerForMessage(
    xpc_object_t message,
    OCFStringRef domain,
    const char *domainString,
    bool userWasSpecified,
    bool sandboxed,
    bool *outNeedsApproval
) {
    const char *container = xpc_dictionary_get_string(message, "CFPreferencesContainer");
    char buffer[1026];
    bzero(buffer, sizeof(buffer));
    OCFTypeRef hostIdentifier = _OCFXPCCreateCFObjectFromXPCObject(
        xpc_dictionary_get_value(message, "CFPreferencesHostBundleIdentifier")
    );
    bool isCurrentApplication = xpc_dictionary_get_bool(message, "CFPreferencesCurrentApplicationDomain");
    if (_CFPrefsGetSuiteContainer(
            message, domain, buffer, 1026, (OCFStringRef)hostIdentifier, isCurrentApplication
        )) {
        container = buffer;
    }
    if (hostIdentifier != NULL) {
        OCFRelease(hostIdentifier);
    }
    if (container != NULL && *container != 0) {
        if (strncmp(container, "kCFPreferencesNoContainer", 25) == 0) {
            return NULL;
        }
        errno = 0;
        int result = sandbox_check(
            getpid(), "file-read-data", (int)(SANDBOX_CHECK_NO_REPORT | SANDBOX_CHECK_NOFOLLOW | 1), container
        );
        if (result != 0) {
            int error = errno;
            OCFStringRef path =
                OCFStringCreateWithFileSystemRepresentation(kOCFAllocatorSystemDefault, container);
            if (result == -1) {
                os_log_error(
                    _CFPrefsDaemonLog(),
                    "sandbox check for %{public}s on %{private}s failed with error: %{darwin.errno}d",
                    "file-read-data",
                    container,
                    error
                );
            }
            return path;
        }
        int fd = open(container, O_SEARCH);
        if (fd < 0) {
            os_log_error(
                _CFPrefsDaemonLog(),
                "attempt to write to a container %{private}s that does not yet exist. Using client-provided "
                "container path without resolving symlinks.",
                container
            );
            return OCFStringCreateWithFileSystemRepresentation(kOCFAllocatorSystemDefault, container);
        }
        OCFStringRef path = NULL;
        if (fcntl(fd, F_GETPATH, buffer) != -1) {
            path = OCFStringCreateWithFileSystemRepresentation(kOCFAllocatorSystemDefault, buffer);
        }
        close(fd);
        return path;
    }
    if (!userWasSpecified || !sandboxed) {
        return NULL;
    }
    audit_token_t token = auditTokenForMessage(message);
    if (sandbox_container_path_for_audit_token(token, buffer, 1026) != 0 || buffer[0] == 0) {
        return NULL;
    }
    int flags = (int)(SANDBOX_CHECK_NO_REPORT | SANDBOX_CHECK_ALLOW_APPROVAL);
    if (_CFPrefsSandboxCheck(token, "user-preference-read", flags | 6, domainString, outNeedsApproval) == 0) {
        return NULL;
    }
    if (_CFPrefsSandboxCheck(token, "user-preference-write", flags | 6, domainString, outNeedsApproval) ==
        0) {
        return NULL;
    }
    return OCFStringCreateWithFileSystemRepresentation(kOCFAllocatorSystemDefault, buffer);
}

// Native _CFPrefsSandboxCheck, 0x1c02e67a4. Non-external (the cfprefsd.m copy of three same-named statics).
// On the primary queue the approval flag, when requested, is dropped. errno is cleared before the query and
// read after it. When the check refuses with an approval category while on the primary queue, the
// out-parameter (when given) is set. A -1 result logs the operation, the path and errno. The approval
// category, if any, is freed.
static int _CFPrefsSandboxCheck(
    audit_token_t token, const char *operation, int type, const char *path, bool *outNeedsApproval
) {
    uint32_t allowApproval = SANDBOX_CHECK_ALLOW_APPROVAL;
    bool onPrimaryQueue = false;
    if ((allowApproval & (uint32_t)type) != 0) {
        onPrimaryQueue = _CFPrefsRunningOnPrimaryQueue();
        type ^= (int)(onPrimaryQueue ? allowApproval : 0);
    }
    char *approvalCategory = NULL;
    errno = 0;
    int result = sandbox_check_and_query_approval_category(token, &approvalCategory, operation, type, path);
    int error = errno;
    if (outNeedsApproval != NULL && onPrimaryQueue && result != 0 && approvalCategory != NULL) {
        *outNeedsApproval = true;
    }
    if (result == -1) {
        os_log_error(
            _CFPrefsDaemonLog(),
            "sandbox check for %{public}s on %{private}s failed with error: %{darwin.errno}d",
            operation,
            path,
            error
        );
    }
    if (approvalCategory != NULL) {
        free(approvalCategory);
    }
    return result;
}

static bool _isRootOrRoleAccountUserName(OCFStringRef userName);

// Native _CFPrefsCopyFixedUpUserForMessage, 0x1c02e692c, with .cold.1 (0x1c0315604, the
// _CFPrefsConnectionForMessage assertion). Hidden (was a private external).
// The connection is required. A "CFPreferencesUser" string becomes the user: "mobile" maps through
// _CFPrefsCopyUsernameForMobileUser (falling back to the string when that answers NULL), any other name is
// made into a CF string (UTF-8). Without one the user is kCFPreferencesAnyUser. kCFPreferencesCurrentUser is
// released and replaced by the connection's effective user's name (_CFPrefsCopyUserNameForUID), a root or
// role account when that UID is below 501; any other user is checked by name. The flag is stored, the user
// copied (system default allocator) and released, and whether a user string was given stored.
OCFStringRef
_CFPrefsCopyFixedUpUserForMessage(xpc_object_t message, bool *outUserWasSpecified, bool *outIsRootOrRole) {
    xpc_connection_t connection = _CFPrefsConnectionForMessage(message);
    const char *name = xpc_dictionary_get_string(message, "CFPreferencesUser");
    OCFStringRef user = NULL;
    if (name != NULL) {
        if (strcmp(name, "mobile") == 0) {
            user = _CFPrefsCopyUsernameForMobileUser();
        }
        if (user == NULL) {
            user = OCFStringCreateWithCString(kOCFAllocatorSystemDefault, name, kUTF8Encoding);
        }
    }
    if (user == NULL) {
        user = kOCFPreferencesAnyUser;
    }
    bool isRootOrRole;
    if (OCFEqual(user, kOCFPreferencesCurrentUser)) {
        OCFRelease(user);
        uid_t euid = xpc_connection_get_euid(connection);
        user = _CFPrefsCopyUserNameForUID(euid);
        isRootOrRole = euid < 501;
    } else {
        isRootOrRole = _isRootOrRoleAccountUserName(user);
    }
    *outIsRootOrRole = isRootOrRole;
    OCFStringRef result = NULL;
    if (user != NULL) {
        result = OCFStringCreateCopy(kOCFAllocatorSystemDefault, user);
        OCFRelease(user);
    }
    *outUserWasSpecified = name != NULL;
    return result;
}

// Native _isRootOrRoleAccountUserName, 0x1c02e6a84. Non-external.
// "root" is; a name starting with '_' is when its passwd entry exists with a UID below 501 (the result kept
// in a __block flag); any other name is not. The name is converted with its UTF-8 pointer when available,
// else into a buffer of the maximum UTF-8 size plus one: zeroed on the stack (no buffer for a maximum of -1)
// below 257 bytes, else malloc'd; the buffer is zeroed again, converted, and freed when it was malloc'd.
static bool _isRootOrRoleAccountUserName(OCFStringRef userName) {
    if (OCFEqual(userName, OF_STR("root"))) {
        return true;
    }
    if (OCFStringGetLength(userName) == 0 || OCFStringGetCharacterAtIndex(userName, 0) != '_') {
        return false;
    }
    __block bool result = false;
    const char *cString = OCFStringGetCStringPtr(userName, kUTF8Encoding);
    if (cString != NULL) {
        struct passwd *entry = getpwnam(cString);
        result = entry != NULL && entry->pw_uid < 501;
    } else {
        OCFIndex maximum = OCFStringGetMaximumSizeForEncoding(OCFStringGetLength(userName), kUTF8Encoding);
        size_t size = (size_t)(maximum + 1);
        char *buffer;
        if (size < 0x101) {
            if (maximum == -1) {
                buffer = NULL;
            } else {
                buffer = alloca(size);
                bzero(buffer, size);
            }
        } else {
            buffer = malloc_type_malloc(size, 0x100004077774924);
        }
        bzero(buffer, size);
        Boolean converted = OCFStringGetCString(userName, buffer, (OCFIndex)size, kUTF8Encoding);
        if (buffer != NULL && converted) {
            struct passwd *entry = getpwnam(buffer);
            result = entry != NULL && entry->pw_uid < 501;
        }
        if (size >= 0x101) {
            free(buffer);
        }
    }
    return result;
}

// Native indexSearchCallback, 0x1c02e6e04. Non-external. A CFSet applier: a CFPDSource whose shmem index
// equals the context's index is stored through the context's pointer.
typedef struct {
    int index;
    id *found;
} IndexSearchContext;

static void indexSearchCallback(const void *value, void *context) {
    IndexSearchContext *search = context;
    if ([(id)value isKindOfClass:[OCFPDSource class]]) {
        if (search->index == [(id<DaemonSourceMessages>)value shmemIndex]) {
            *search->found = (id)value;
        }
    }
}

// Native replyWithError, 0x1c02e7f2c. Non-external. A reply to the original message (a new dictionary when
// none can be made) is populated with the description and error type 1, handed to the reply handler and
// released.
static void
replyWithError(const char *description, xpc_object_t original, void (^replyHandler)(xpc_object_t reply)) {
    xpc_object_t reply = xpc_dictionary_create_reply(original);
    if (reply == NULL) {
        reply = xpc_dictionary_create(NULL, NULL, 0);
    }
    populateErrorReply(description, reply, 1);
    replyHandler(reply);
    xpc_release(reply);
}

// Native _CFPrefsBooleanEntitlementValueForMessage, 0x1c02e7f98, with
// ____CFPrefsBooleanEntitlementValueForMessage_block_invoke (0x1c02e9cc0, descriptor 0x2227aaf40).
// Non-external. The connection is required. Under the client's context lock, the entitlement's value when it
// is an XPC boolean; false otherwise.
static bool _CFPrefsBooleanEntitlementValueForMessage(xpc_object_t message, const char *entitlement) {
    __block bool value = false;
    withClientContext(_CFPrefsConnectionForMessage(message), ^(id contextObject) {
      xpc_object_t object = _CFPrefsGetEntitlementForMessageWithLockedContext(
          message, entitlement, (ClientContextLayout *)contextObject
      );
      if (object != NULL && xpc_get_type(object) == XPC_TYPE_BOOL) {
          value = xpc_bool_get_value(object);
      }
    });
    return value;
}

// Native _CFPrefsCopyEntitlementForMessage, 0x1c02e8164, with
// ____CFPrefsCopyEntitlementForMessage_block_invoke (0x1c02e9ea4, descriptor 0x2227aaf68) and the __block
// helpers ___Block_byref_object_copy__7 and
// ___Block_byref_object_dispose__7 (0x1c02e8148 and 0x1c02e8158: an object __block variable, assigned with
// BLOCK_FIELD_IS_OBJECT | BLOCK_BYREF_CALLER). Non-external. The connection is required. Under the client's
// context lock, a copy (xpc_copy, +1) of the entitlement's value, or NULL.
static xpc_object_t _CFPrefsCopyEntitlementForMessage(xpc_object_t message, const char *entitlement) {
    __block xpc_object_t copy = NULL;
    withClientContext(_CFPrefsConnectionForMessage(message), ^(id contextObject) {
      copy = _CFPrefsGetEntitlementForMessageWithLockedContext(
          message, entitlement, (ClientContextLayout *)contextObject
      );
      if (copy != NULL) {
          copy = xpc_copy(copy);
      }
    });
    return copy;
}

// Native _CFPrefsMessageSenderIsSandboxed: no symbol; inlined (from CFXPreferences_Internal.h) into
// -handleSourceMessage:originalMessage:replyHandler:. Its block is
// ____CFPrefsMessageSenderIsSandboxed_block_invoke_0 (0x1c02e9d1c, descriptor 0x2227aaa88), with .cold.1_0
// (0x1c0315acc, the _CFPrefsConnectionForMessage assertion) and .cold.2_0 (0x1c0315ad8, the error log through
// OUTLINED_FUNCTION_4_15, _11_6 and _5_15). The connection is required; the answer starts true (an invalid
// context leaves it so). Under the client's context lock an unknown state (0xff) is computed: errno is
// cleared, the client's audit token is checked with no operation (NO_REPORT, a NULL argument), a negative
// result is logged with the pid and errno, and the state becomes whether the result is nonzero; a known state
// answers whether it is 1.
static inline bool _CFPrefsMessageSenderIsSandboxed(xpc_object_t message) {
    xpc_connection_t connection = _CFPrefsConnectionForMessage(message);
    __block bool sandboxed = true;
    withClientContext(connection, ^(id contextObject) {
      ClientContextLayout *context = (ClientContextLayout *)contextObject;
      uint8_t state = context->sandboxed;
      bool result;
      if (state == 0xFF) {
          audit_token_t token = auditTokenForMessage(message);
          errno = 0;
          int check = sandbox_check_by_audit_token(token, NULL, (int)SANDBOX_CHECK_NO_REPORT, NULL);
          if (check < 0) {
              int error = errno;
              os_log_error(
                  _CFPrefsDaemonLog(),
                  "sandbox check for (NULL, NULL) on PID %i failed with error: %{darwin.errno}d. Treating "
                  "process "
                  "as sandboxed",
                  (int)token.val[5],
                  error
              );
          }
          result = check != 0;
          context->sandboxed = result;
      } else {
          result = state == 1;
      }
      sandboxed = result;
    });
    return sandboxed;
}

// Native log_client_activity, 0x1c02e8800, with .cold.1 (0x1c03157b4). Non-external. When the daemon log
// takes debug messages, the client's description and the activity (and the domain, when given) are logged at
// debug level.
static void log_client_activity(xpc_object_t message, const char *activity, const char *domain) {
    if (!os_log_type_enabled(_CFPrefsDaemonLog(), OS_LOG_TYPE_DEBUG)) {
        return;
    }
    OCFStringRef client = CFPreferencesClientDescriptionForMessage(message);
    if (domain != NULL) {
        os_log_debug(
            _CFPrefsDaemonLog(),
            "Process %{public}@ %{public}s for domain %{public}s",
            client,
            activity,
            domain
        );
    } else {
        os_log_debug(_CFPrefsDaemonLog(), "Process %{public}@ %{public}s", client, activity);
    }
}

// Native client_context_finalizer, 0x1c02e933c, with .cold.1 (0x1c03159e0, the assertion
// "!context->_valid", cfprefsd.m line 205). Non-external. The XPC connection finalizer: under the context's
// lock a still-valid context fails the assertion; the suite container cache, the entitlements and the
// application groups are released when present; the context itself is released after the lock.
static void client_context_finalizer(void *contextObject) {
    ClientContextLayout *context = contextObject;
    os_unfair_lock_lock(&context->lock);
    if ((context->flags & 8) != 0) {
        __assert_rtn("client_context_finalizer", "cfprefsd.m", 205, "!context->_valid");
    }
    if (context->suiteCache != NULL) {
        OCFRelease(context->suiteCache);
    }
    if (context->entitlements != NULL) {
        xpc_release(context->entitlements);
    }
    if (context->applicationGroups != NULL) {
        OCFRelease(context->applicationGroups);
    }
    os_unfair_lock_unlock(&context->lock);
    [(id)contextObject release];
}

// Native mirrored-keys constant array 0x2227ce268 (__DATA_CONST,__objc_arrayobj: an NSConstantArray of the
// constant strings "AppleLocale" 0x2227b40a0 and "AppleLanguages" 0x2227b15c0; read with
// Local/Scratch/cf-core/macho.py). The record layout (isa, count, objects) is the one
// String/OCFStringLocalizedFormatting.m spells; the names are local descriptive spellings.
extern char constantArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSConstantArray));
static const OCFConstantString mirroredKeyLocale = ConstantASCIIString("AppleLocale");
static const OCFConstantString mirroredKeyLanguages = ConstantASCIIString("AppleLanguages");
static const void *const mirroredKeyObjects[2] = {&mirroredKeyLocale, &mirroredKeyLanguages};

static const struct {
    uintptr_t isa;
    uint64_t count;
    const void *const *objects;
} mirroredKeys = {(uintptr_t)constantArrayClass, 2, mirroredKeyObjects};

// Native class _CFPrefsClientContext (see Preferences/OCFPrefsDaemon.h). No methods: the class object and its
// ivars only; contexts are made with alloc and init in -initWithRole:testMode:'s connection handler.
@implementation _OCFPrefsClientContext

@end

@implementation OCFPrefsDaemon

// Native -[CFPrefsDaemon handleMessage:originalMessage:fromPeer:replyHandler:], 0x1c02e6e5c, with .cold.1
// (0x1c03156b4, the assertion "type == XPC_TYPE_DICTIONARY", cfprefsd.m line 769). Direct method.
// An XPC error answers no reply; anything but a dictionary fails the assertion. With the injected-failure
// testing switch (_CFPrefsTestingFlags byte 4) set, a message not marked "PreviousMessageInjectedFailure"
// cancels the peer. The peer is stored as the message's "connection", and "CFPreferencesOperation" selects:
// 1, 7, 8 and 9 the source handler; 2 the agent check-in (no reply); 3 the managed flush; 4 the per-domain
// flush; 5 the multi-message; 6 is only logged ("reported a user was deleted") and never answered; 999
// synchronizes the dirty sources (no reply); anything else answers "Unsupported CFPreferences Daemon
// Operation".
- (void)handleMessage:(xpc_object_t)message
      originalMessage:(xpc_object_t)originalMessage
             fromPeer:(xpc_connection_t)peer
         replyHandler:(void (^)(xpc_object_t reply))replyHandler {
    xpc_type_t type = xpc_get_type(message);
    if (type == XPC_TYPE_ERROR) {
        replyHandler(NULL);
        return;
    }
    if (type != XPC_TYPE_DICTIONARY) {
        __assert_rtn(
            "-[CFPrefsDaemon handleMessage:originalMessage:fromPeer:replyHandler:]",
            "cfprefsd.m",
            769,
            "type == XPC_TYPE_DICTIONARY"
        );
    }
    if (_OCFPrefsTestingFlags[4] != 0 &&
        !xpc_dictionary_get_bool(message, "PreviousMessageInjectedFailure")) {
        xpc_connection_cancel(peer);
        return;
    }
    xpc_dictionary_set_value(message, "connection", peer);
    int64_t operation = xpc_dictionary_get_int64(message, "CFPreferencesOperation");
    switch (operation) {
        case 1:
        case 7:
        case 8:
        case 9:
            [self handleSourceMessage:message originalMessage:originalMessage replyHandler:replyHandler];
            return;
        case 2:
            [self handleAgentCheckInMessage:message];
            replyHandler(NULL);
            return;
        case 3:
            [self handleFlushManagedMessage:message replyHandler:replyHandler];
            return;
        case 4:
            [self handleFlushSourceForDomainMessage:message replyHandler:replyHandler];
            return;
        case 5:
            [self handleMultiMessage:message originalMessage:originalMessage replyHandler:replyHandler];
            return;
        case 6:
            log_client_activity(message, "reported a user was deleted", NULL);
            return;
        case 999:
            [[_OCFPrefsSynchronizer sharedInstance] synchronize];
            replyHandler(NULL);
            return;
        default:
            replyWithError("Unsupported CFPreferences Daemon Operation", message, replyHandler);
            return;
    }
}

// Native -[CFPrefsDaemon handleSourceMessage:originalMessage:replyHandler:], 0x1c02e7088, with .cold.1
// (0x1c03156dc, the once with ___block_literal_global.63, 0x2227aaba0, whose invoke ___66-[CFPrefsDaemon
// handleSourceMessage:originalMessage:replyHandler:]_block_invoke 0x1c02e80e0 sets
// enableContainerizedManagedPrefs when access("/containerized_managed_preferences", F_OK) succeeds), .cold.2
// (0x1c0315700, the invalid store identifier log), .cold.3 and .cold.4 (0x1c0315740 and 0x1c031574c, the
// _CFPrefsConnectionForMessage assertions), .cold.5 (0x1c0315758, the "Domain not found" path) and the blocks
// _block_invoke.67 (0x1c02e8110, descriptor 0x2227a7b58), _block_invoke.73 and _block_invoke_2 (0x1c02e82c0
// and 0x1c02e8320, descriptor 0x2227aabc8), _block_invoke_3 (0x1c02e8380, descriptor 0x2227aac40),
// _block_invoke_4 (0x1c02e84b4, descriptor 0x2227aabf0, with .cold.1 0x1c03157a8), _block_invoke_5
// (0x1c02e8720, descriptor 0x2227aac18) and _block_invoke_6 (0x1c02e8794, descriptor 0x2227aac68); the object
// __block variables use ___Block_byref_object_copy__7 and _dispose__7. Its statics .onceToken and
// .enableContainerizedManagedPrefs lie at __DATA,__bss 0x22e6d3cc8 and 0x22e6d3cc4. Direct method.
// The impersonation fix-up runs and the message is retained (released on every path but the approval
// hand-off, where the hand-off block releases it). The user is fixed up, then the domain (by host from
// "CFPreferencesIsByHost"); without a domain the user is released and "Domain not found" answered.
// "AllowWritingSpecialKeysToGlobalPreferences" is cleared, and set to true for kCFPreferencesAnyApplication
// (pointer comparison) in education mode when the connection's effective user is 502 and the client holds
// com.apple.private.defaults-shared-ipad-loginui. A managed request runs the once and, with containerized
// managed preferences, looks up the connection (the result is unused; managedUsesContainer stays false). The
// container is fixed up for an unmanaged request ("CFPreferencesHasFixedUpContainer" is cleared, then set
// when there is one). The plist is readable by an unsandboxed client, or by a sandboxed one for a message
// without "Key" that passes the domain's "user-preference-read" check. When a check needs approval, the
// request is handed off (with the original message retained) to a secondary queue for the client to run
// again, and only the user is released. Cloud requests: a "CFPreferencesCloudStoreIdentifier" that is not a
// string is logged and ignored; with "CFPreferencesCloudConfig" the store identifier (the given one retained,
// else the domain as an XPC string) is the cloud store entitlement when the plist is readable or the client
// holds com.apple.private.trust-defaults-kvstore-identifier, else (for the current application) when it
// equals the client's com.apple.developer.ubiquity-kvstore-identifier, else when it is listed in
// com.apple.private.ubiquity-kvstore-access or com.apple.private.ubiquity-additional-kvstore-identifiers; the
// entitlement is retained and the identifier released. A container of a request without a user must lie under
// "/private/var/containers/Shared/SystemGroup/", and a cloud configuration needs the entitlement; otherwise
// the error is answered. Else a reply (or new dictionary) is made and the source block run: a cloud request
// on the primary queue is moved to a secondary queue; otherwise the source is looked up and, under its lock,
// a full cloud sync is run (cloud sources) or refused, a missing source refused, a shmem index mismatch
// logged and refused, or the message accepted (error type 6 clears the error, asks for the secondary queue
// and answers no reply); after the lock the reply is answered and released and an accepting source cleans up.
// When the secondary queue is asked for, the block is handed off there with the messages retained; the
// hand-off block releases the entitlement, container, configuration path, user and messages, and they are
// cleared here. The message, the entitlement, the container, the configuration path and the user are
// released. The domain (interned and retained by _CFPrefsGetFixedUpDomainForMessage) is never released here,
// as natively.
- (void)handleSourceMessage:(xpc_object_t)message
            originalMessage:(xpc_object_t)originalMessage
               replyHandler:(void (^)(xpc_object_t reply))replyHandler {
    static dispatch_once_t onceToken;
    static bool enableContainerizedManagedPrefs;
    _CFPrefsFixUpIncomingMessageForPIDImpersonationIfNeeded(message);
    xpc_retain(message);
    bool isRootOrRole = false;
    bool userWasSpecified = false;
    OCFStringRef user = _CFPrefsCopyFixedUpUserForMessage(message, &userWasSpecified, &isRootOrRole);
    bool byHost = xpc_dictionary_get_bool(message, "CFPreferencesIsByHost");
    const char *domainString = NULL;
    OCFStringRef domain = _CFPrefsGetFixedUpDomainForMessage(message, user, byHost, &domainString);
    if (domain == NULL) {
        if (user != NULL) {
            OCFRelease(user);
        }
        replyWithError("Domain not found", message, replyHandler);
        xpc_release(message);
        return;
    }
    xpc_dictionary_set_value(message, "AllowWritingSpecialKeysToGlobalPreferences", NULL);
    if (domain == kOCFPreferencesAnyApplication && eduModeEnabled()) {
        if (xpc_connection_get_euid(_CFPrefsConnectionForMessage(message)) == 502 &&
            _CFPrefsBooleanEntitlementValueForMessage(
                message, "com.apple.private.defaults-shared-ipad-loginui"
            )) {
            xpc_dictionary_set_value(message, "AllowWritingSpecialKeysToGlobalPreferences", XPC_BOOL_TRUE);
        }
    }
    bool managed = xpc_dictionary_get_bool(message, "CFPreferencesIsManaged");
    bool sandboxed = _CFPrefsMessageSenderIsSandboxed(message);
    bool isCurrentApplication = xpc_dictionary_get_bool(message, "CFPreferencesCurrentApplicationDomain");
    __block bool managedUsesContainer = false;
    if (managed) {
        dispatch_once(&onceToken, ^{
          if (access("/containerized_managed_preferences", F_OK) == 0) {
              enableContainerizedManagedPrefs = true;
          }
        });
        if (enableContainerizedManagedPrefs == 1) {
            (void)_CFPrefsConnectionForMessage(message);
        }
    }
    bool needsApproval = false;
    xpc_dictionary_set_value(message, "CFPreferencesHasFixedUpContainer", NULL);
    OCFStringRef container = NULL;
    bool noContainer = true;
    if (!managed) {
        container = _CFPrefsCopyFixedUpContainerForMessage(
            message, domain, domainString, userWasSpecified, sandboxed, &needsApproval
        );
        if (container != NULL) {
            xpc_dictionary_set_bool(message, "CFPreferencesHasFixedUpContainer", true);
            noContainer = false;
        }
    }
    audit_token_t token = auditTokenForMessage(message);
    bool readable;
    if (sandboxed) {
        if (xpc_dictionary_get_string(message, "Key") != NULL) {
            readable = false;
        } else {
            int flags = (int)(SANDBOX_CHECK_NO_REPORT | SANDBOX_CHECK_ALLOW_APPROVAL);
            readable = _CFPrefsSandboxCheck(
                           token, "user-preference-read", flags | 6, domainString, &needsApproval
                       ) == 0;
        }
    } else {
        readable = true;
    }
    if (needsApproval == 1) {
        int pid = (int)auditTokenForMessage(message).val[5];
        dispatch_queue_t queue = [self createSecondaryQueueForClientWithPID:pid];
        os_log(
            _CFPrefsDaemonLog(),
            "Need to call TCC to ascertain if client %d has sandbox access. Moving to a different queue",
            pid
        );
        xpc_retain(originalMessage);
        xpc_dictionary_handoff_reply(originalMessage, queue, ^{
          [self handleSourceMessage:message originalMessage:originalMessage replyHandler:replyHandler];
          xpc_release(originalMessage);
          xpc_release(message);
        });
        dispatch_release(queue);
        if (user != NULL) {
            OCFRelease(user);
        }
        // CoreFoundation defect: -handleSourceMessage:originalMessage:replyHandler: (0x1c02e7088)
        // exits this handoff path without releasing the +1 container from
        // _CFPrefsCopyFixedUpContainerForMessage (0x1c02e6290). Confidence: verified from both captures.
        // The recursive handoff recomputes its container; release this attempt's untransferred copy.
        if (container != NULL) {
            OCFRelease(container);
        }
        return;
    }
    __block xpc_object_t cloudStoreEntitlement = NULL;
    const char *cloudConfig = xpc_dictionary_get_string(message, "CFPreferencesCloudConfig");
    xpc_object_t storeIdentifier = xpc_dictionary_get_value(message, "CFPreferencesCloudStoreIdentifier");
    OCFStringRef cloudConfigurationPath = NULL;
    bool hasStoreIdentifier = false;
    if (storeIdentifier != NULL) {
        if (xpc_get_type(storeIdentifier) != XPC_TYPE_STRING) {
            os_log_error(
                _CFPrefsDaemonLog(),
                "Client (%d) passed an invalid cloud store identifier value.",
                _CFPrefsPIDForMessage(message)
            );
        } else if (cloudConfig != NULL) {
            xpc_retain(storeIdentifier);
            hasStoreIdentifier = true;
        }
    } else if (cloudConfig != NULL) {
        storeIdentifier = xpc_string_create(domainString);
        hasStoreIdentifier = true;
    }
    if (hasStoreIdentifier) {
        cloudConfigurationPath =
            OCFStringCreateWithCString(kOCFAllocatorSystemDefault, cloudConfig, kUTF8Encoding);
        if (readable || _CFPrefsBooleanEntitlementValueForMessage(
                            message, "com.apple.private.trust-defaults-kvstore-identifier"
                        )) {
            cloudStoreEntitlement = storeIdentifier;
        } else if (isCurrentApplication) {
            xpc_object_t identifier =
                _CFPrefsCopyEntitlementForMessage(message, "com.apple.developer.ubiquity-kvstore-identifier");
            if (identifier != NULL) {
                if (xpc_get_type(identifier) == XPC_TYPE_STRING && xpc_equal(storeIdentifier, identifier)) {
                    cloudStoreEntitlement = storeIdentifier;
                }
                xpc_release(identifier);
            }
        }
        if (cloudStoreEntitlement == NULL) {
            xpc_object_t access =
                _CFPrefsCopyEntitlementForMessage(message, "com.apple.private.ubiquity-kvstore-access");
            if (access != NULL) {
                if (xpc_get_type(access) == XPC_TYPE_ARRAY) {
                    xpc_array_apply(access, ^bool(size_t index, xpc_object_t value) {
                      (void)index;
                      if (xpc_get_type(value) == XPC_TYPE_STRING && xpc_equal(storeIdentifier, value)) {
                          cloudStoreEntitlement = storeIdentifier;
                          return false;
                      }
                      return true;
                    });
                }
                xpc_release(access);
            }
        }
        if (cloudStoreEntitlement == NULL) {
            xpc_object_t additional = _CFPrefsCopyEntitlementForMessage(
                message, "com.apple.private.ubiquity-additional-kvstore-identifiers"
            );
            if (additional != NULL) {
                if (xpc_get_type(additional) == XPC_TYPE_ARRAY) {
                    xpc_array_apply(additional, ^bool(size_t index, xpc_object_t value) {
                      (void)index;
                      if (xpc_get_type(value) == XPC_TYPE_STRING && xpc_equal(storeIdentifier, value)) {
                          cloudStoreEntitlement = storeIdentifier;
                          return false;
                      }
                      return true;
                    });
                }
                xpc_release(additional);
            }
        }
        if (cloudStoreEntitlement != NULL) {
            xpc_retain(cloudStoreEntitlement);
        }
        if (storeIdentifier != NULL) {
            xpc_release(storeIdentifier);
        }
    }
    if (!noContainer && !userWasSpecified &&
        !OCFStringHasPrefix(container, OF_STR("/private/var/containers/Shared/SystemGroup/"))) {
        replyWithError(
            "Using kCFPreferencesAnyUser with a container is only allowed for System Containers",
            message,
            replyHandler
        );
        xpc_release(message);
    } else if (cloudConfig != NULL && cloudStoreEntitlement == NULL) {
        replyWithError(
            "'com.apple.developer.ubiquity-kvstore-identifier' entitlement needed to use cloud preferences",
            message,
            replyHandler
        );
        xpc_release(message);
    } else {
        __block bool accepted = false;
        __block bool needsSecondaryQueue = false;
        __block xpc_object_t reply = xpc_dictionary_create_reply(message);
        if (reply == NULL) {
            reply = xpc_dictionary_create(NULL, NULL, 0);
        }
        void (^withSource)(void) = ^{
          if (cloudConfigurationPath != NULL && _CFPrefsRunningOnPrimaryQueue()) {
              needsSecondaryQueue = true;
              return;
          }
          [self withSourceForDomain:domain
              inContainer:container
              user:user
              byHost:byHost
              managed:managed
              managedUsesContainer:managedUsesContainer
              cloudStoreEntitlement:cloudStoreEntitlement
              cloudConfigurationPath:cloudConfigurationPath
              constructReplyWithSourceLock:^xpc_object_t(id source) {
                if (xpc_dictionary_get_bool(message, "FullCloudSync")) {
                    if ([source isKindOfClass:[(Class)(void *)cloudSourceClass class]]) {
                        [(id<DaemonSourceMessages>)source synchronizeWithCloud];
                        return reply;
                    }
                    populateErrorReply("Domain not cloud-backed", reply, 1);
                    return reply;
                }
                if (source == nil) {
                    populateErrorReply("Domain or user not found", reply, 1);
                    return reply;
                }
                xpc_object_t index = xpc_dictionary_get_value(message, "CFPreferencesShmemIndex");
                if (index != NULL && xpc_get_type(index) == XPC_TYPE_INT64) {
                    int value = (int)xpc_int64_get_value(index);
                    if (value != -1 && value != [(id<DaemonSourceMessages>)source shmemIndex]) {
                        [self logDomainInconsistencyForProcess:(int)auditTokenForMessage(message).val[5]
                                                       message:message
                                                        source:source];
                        populateErrorReply("Lookup inconsistency", reply, 1);
                        return reply;
                    }
                }
                [(id<DaemonSourceMessages>)source acceptMessage:message populatingReply:reply];
                if (xpc_dictionary_get_int64(reply, "CFPreferencesErrorType") == 6) {
                    xpc_dictionary_set_value(reply, "CFPreferencesErrorType", NULL);
                    needsSecondaryQueue = true;
                    return NULL;
                }
                accepted = true;
                return reply;
              }
              afterReleasingSourceLock:^(id source, xpc_object_t sourceReply) {
                if (sourceReply != NULL) {
                    replyHandler(sourceReply);
                    xpc_release(sourceReply);
                }
                if (accepted == 1) {
                    [(id<DaemonSourceMessages>)source cleanUpAfterAcceptingMessage:message];
                }
              }];
        };
        withSource();
        if (needsSecondaryQueue == 1) {
            int pid = (int)auditTokenForMessage(message).val[5];
            dispatch_queue_t queue = [self createSecondaryQueueForClientWithPID:pid];
            xpc_retain(originalMessage);
            xpc_retain(message);
            xpc_dictionary_handoff_reply(originalMessage, queue, ^{
              withSource();
              if (cloudStoreEntitlement != NULL) {
                  xpc_release(cloudStoreEntitlement);
              }
              if (container != NULL) {
                  OCFRelease(container);
              }
              if (cloudConfigurationPath != NULL) {
                  OCFRelease(cloudConfigurationPath);
              }
              if (user != NULL) {
                  OCFRelease(user);
              }
              xpc_release(originalMessage);
              xpc_release(message);
            });
            cloudStoreEntitlement = NULL;
            dispatch_release(queue);
            cloudConfigurationPath = NULL;
            container = NULL;
            user = NULL;
        }
        xpc_release(message);
    }
    if (cloudStoreEntitlement != NULL) {
        xpc_release(cloudStoreEntitlement);
    }
    if (container != NULL) {
        OCFRelease(container);
    }
    if (cloudConfigurationPath != NULL) {
        OCFRelease(cloudConfigurationPath);
    }
    if (user != NULL) {
        OCFRelease(user);
    }
}

// Native -[CFPrefsDaemon handleMultiMessage:originalMessage:replyHandler:], 0x1c02e7cc0, with .cold.1
// (0x1c031579c, the _CFPrefsConnectionForMessage assertion) and
// ___65-[CFPrefsDaemon handleMultiMessage:originalMessage:replyHandler:]_block_invoke_2 (0x1c02e8a74,
// descriptor 0x2227aad40). Direct method.
// The connection is required. Without a "CFPreferencesMessages" array, or when an element is neither a
// dictionary nor null (the global validator block ___block_literal_global.94, 0x2227aad18, invoke
// ___65-[CFPrefsDaemon handleMultiMessage:originalMessage:replyHandler:]_block_invoke 0x1c02e8a40, through
// xpc_array_apply), the error is answered. An empty array answers a reply (or new dictionary) holding
// an empty array. Otherwise a zeroed reply slot per message (calloc, malloc type 0x80040b8603338) is filled
// by the sub-message handler from index 0; when the last one is done the reply (or new dictionary) gets the
// replies as an array, the non-null replies are released, the slots freed, and the reply answered and
// released.
- (void)handleMultiMessage:(xpc_object_t)message
           originalMessage:(xpc_object_t)originalMessage
              replyHandler:(void (^)(xpc_object_t reply))replyHandler {
    xpc_connection_t peer = _CFPrefsConnectionForMessage(message);
    xpc_object_t messages = xpc_dictionary_get_value(message, "CFPreferencesMessages");
    if (messages == NULL || xpc_get_type(messages) != XPC_TYPE_ARRAY) {
        replyWithError(
            "There must be an array of contained messages present in multimessages", message, replyHandler
        );
        return;
    }
    if (!xpc_array_apply(messages, multiMessageValidatorBlock)) {
        replyWithError(
            "Contained messages must be dictionaries or nulls in multimessages", message, replyHandler
        );
        return;
    }
    size_t count = xpc_array_get_count(messages);
    if (count == 0) {
        xpc_object_t reply = xpc_dictionary_create_reply(message);
        if (reply == NULL) {
            reply = xpc_dictionary_create(NULL, NULL, 0);
        }
        xpc_object_t empty = xpc_array_create_empty();
        xpc_dictionary_set_value(reply, "CFPreferencesMessages", empty);
        xpc_release(empty);
        replyHandler(reply);
        xpc_release(reply);
        return;
    }
    xpc_object_t *replies = malloc_type_calloc(count, 8, 0x80040B8603338);
    [self _handleSubMessagesFromArray:messages
                            withCount:count
                             fromPeer:peer
                           startingAt:0
                      originalMessage:originalMessage
                              replies:replies
                         replyHandler:^{
                           xpc_object_t reply = xpc_dictionary_create_reply(message);
                           if (reply == NULL) {
                               reply = xpc_dictionary_create(NULL, NULL, 0);
                           }
                           xpc_object_t array = xpc_array_create(replies, count);
                           xpc_dictionary_set_value(reply, "CFPreferencesMessages", array);
                           xpc_release(array);
                           for (size_t index = 0; (int64_t)count > (int64_t)index; ++index) {
                               if (xpc_get_type(replies[index]) != XPC_TYPE_NULL) {
                                   xpc_release(replies[index]);
                               }
                           }
                           free(replies);
                           replyHandler(reply);
                           xpc_release(reply);
                         }];
}

// Native -[CFPrefsDaemon initWithRole:testMode:], 0x1c02e8ba8, with .cold.1 (0x1c0315814, the assertion
// "isCFPrefsD", cfprefsd.m line 1495), .cold.2 (0x1c031583c, the once of initWithRoleOnceToken with the
// global block ___block_literal_global.102_0, whose invoke ___39-[CFPrefsDaemon
// initWithRole:testMode:]_block_invoke 0x1c02e8e40 is an empty function), .cold.3 (0x1c0315860, the workloop
// set-up) and the blocks
// ___39-[CFPrefsDaemon initWithRole:testMode:]_block_invoke.108 (0x1c02e8e44, descriptor 0x2227a70d0),
// _block_invoke_2 (0x1c02e8fa0, descriptor 0x2227a8548, with .cold.1 and .cold.2, 0x1c0315990 and
// 0x1c03159b8: "cfpd" and "ctxt", lines 1562 and 1560), _block_invoke_3 (0x1c02e91c8, descriptor
// 0x2227aadb0), _block_invoke.110 (0x1c02e9298, descriptor 0x2227aaa18) and _block_invoke_2.111 (0x1c02e92bc,
// descriptor 0x2227aa8e0). Direct method.
// After -[NSObject init] (selector reference 0x2387aa0ea, outside the extracted image; -init inferred,
// confidence medium), test mode is stored (and holds an os_transaction "cfprefsd testing", never released).
// A nonzero role is stored and names the service: role 2 the system daemon, any other the user daemon, each
// with a ".test" variant in test mode. Role 0 requires cfprefsd (bit 0 of isCFPrefsD), runs the once and
// becomes role 1 with the user daemon's name. Inside cfprefsd (isCFPrefsD == 1) the listener is a mach
// service listener (flags 1); elsewhere an anonymous connection. One incoming workloop is assumed; with the
// CoreFoundation/CFPreferencesWorkloops feature, $CFPrefsDWorkloops (1-10, strtonum) may change the count,
// the workloops ("com.apple.cfprefsd.incoming", marked as the primary queue) are made and the count logged,
// else only the listener is logged. A secondary workloop ("com.apple.cfprefsd.secondary") is made. Each new
// peer connection gets a queue (-createQueueForClientWithPID:uid:, released after use), an event handler, a
// new _CFPrefsClientContext (the daemon stored with release ordering, both state bytes 0xff, valid) as its
// context, the context finalizer, and is activated. A peer dictionary message is answered through the
// context's daemon (cancelling the peer instead while terminating); the reply is sent and a signpost event
// "handleRequest" (begin and end mach continuous times, by duration through _CFPrefsSignpostsLogForDuration)
// emitted. An invalidated peer invalidates its context and hands over its observed sources, which are told
// (sources) or asked to drop the connection (observer-only tombstones). Under the cache lock (initialized
// first) the source cache (CFType set) and usage counts (CFType bag) are made; the listener is stored and
// activated.
- (id)initWithRole:(int)role testMode:(BOOL)testMode {
    self = [super init];
    if (self == nil) {
        return nil;
    }
    _testMode = testMode;
    if (testMode) {
        os_transaction_create("cfprefsd testing");
    }
    const char *name;
    if (role != 0) {
        _role = role;
        const char *systemName =
            _testMode ? "com.apple.cfprefsd.daemon.system.test" : "com.apple.cfprefsd.daemon.system";
        const char *userName = _testMode ? "com.apple.cfprefsd.daemon.test" : "com.apple.cfprefsd.daemon";
        name = role == 2 ? systemName : userName;
    } else {
        if ((isCFPrefsD & 1) == 0) {
            __assert_rtn("-[CFPrefsDaemon initWithRole:testMode:]", "cfprefsd.m", 1495, "isCFPrefsD");
        }
        dispatch_once(&initWithRoleOnceToken, initWithRoleOnceBlock);
        _role = 1;
        name = _testMode ? "com.apple.cfprefsd.daemon.test" : "com.apple.cfprefsd.daemon";
    }
    _name = name;
    xpc_connection_t listener;
    if (isCFPrefsD == 1) {
        listener = createMachServiceConnection(name, NULL, 1);
    } else {
        listener = xpc_connection_create(NULL, NULL);
    }
    _listenerWorkloopsCount = 1;
    if (_os_feature_enabled_impl("CoreFoundation", "CFPreferencesWorkloops")) {
        const char *count = getenv("CFPrefsDWorkloops");
        if (count != NULL) {
            long long value = strtonum(count, 1, 10, NULL);
            if (value != 0) {
                _listenerWorkloopsCount = (int)value;
            }
        }
        _listenerWorkloops = malloc_type_calloc((size_t)_listenerWorkloopsCount, 8, 0x80040B8603338);
        for (int64_t index = 0; index < _listenerWorkloopsCount; ++index) {
            _listenerWorkloops[index] = dispatch_workloop_create("com.apple.cfprefsd.incoming");
            dispatch_queue_set_specific(
                _listenerWorkloops[index], primaryQueueSpecificKey, primaryQueueSpecificValue, NULL
            );
        }
        os_log(
            _CFPrefsDaemonLog(),
            "This is cfprefsd with %i workloops listening on %s",
            _listenerWorkloopsCount,
            _name
        );
    } else {
        os_log(_CFPrefsDaemonLog(), "This is cfprefsd listening on %s", _name);
    }
    _secondaryWorkloop = dispatch_workloop_create("com.apple.cfprefsd.secondary");
    xpc_connection_set_event_handler(listener, ^(xpc_object_t peer) {
      if (xpc_get_type(peer) != XPC_TYPE_CONNECTION) {
          return;
      }
      pid_t pid = connectionProcessIdentifier(peer);
      dispatch_queue_t queue = [self createQueueForClientWithPID:pid uid:xpc_connection_get_euid(peer)];
      xpc_connection_set_target_queue(peer, queue);
      [queue release];
      xpc_connection_set_event_handler(peer, ^(xpc_object_t event) {
        uint64_t begin = mach_continuous_time();
        if (xpc_get_type(event) == XPC_TYPE_DICTIONARY) {
            if (terminating) {
                xpc_connection_cancel(peer);
                return;
            }
            ClientContextLayout *context = xpc_connection_get_context(peer);
            if (context == NULL) {
                __assert_rtn(
                    "-[CFPrefsDaemon initWithRole:testMode:]_block_invoke_2", "cfprefsd.m", 1560, "ctxt"
                );
            }
            OCFPrefsDaemon *daemon = atomic_load(&context->daemon);
            if (daemon == nil) {
                __assert_rtn(
                    "-[CFPrefsDaemon initWithRole:testMode:]_block_invoke_2", "cfprefsd.m", 1562, "cfpd"
                );
            }
            [daemon handleMessage:event
                  originalMessage:event
                         fromPeer:peer
                     replyHandler:^(xpc_object_t reply) {
                       if (reply != NULL) {
                           xpc_connection_send_message(peer, reply);
                       }
                       uint64_t end = mach_continuous_time();
                       os_log_t log = _CFPrefsSignpostsLogForDuration(end - begin);
                       os_signpost_id_t identifier = os_signpost_id_make_with_pointer(log, self);
                       os_signpost_event_emit(
                           log,
                           identifier,
                           "handleRequest",
                           "client-%i %{public, signpost.description:begin_time}llu %{public, "
                           "signpost.description:end_time}llu",
                           pid,
                           begin,
                           end
                       );
                     }];
        } else if (event == XPC_ERROR_CONNECTION_INVALID) {
            __block OCFSetRef observed = NULL;
            withClientContext(peer, ^(id contextObject) {
              ClientContextLayout *context = (ClientContextLayout *)contextObject;
              context->flags &= (uint8_t)~8;
              observed = context->observedSources;
              context->observedSources = NULL;
            });
            if (observed != NULL) {
                if (OCFSetGetCount(observed) >= 1) {
                    CFSetApply(observed, ^(const void *value, Boolean *stop) {
                      (void)stop;
                      if ([(id)value isKindOfClass:[OCFPDSource class]]) {
                          [(id<DaemonSourceMessages>)value observingConnectionWasInvalidated:peer];
                      } else if ([(id)value isKindOfClass:[OCFPDObserverOnlyTombstone class]]) {
                          [(id<DaemonSourceMessages>)value removeObservingConnection:peer daemon:self];
                      }
                    });
                }
                OCFRelease(observed);
            }
        }
      });
      ClientContextLayout *context = (ClientContextLayout *)[[_OCFPrefsClientContext alloc] init];
      context->lock = OS_UNFAIR_LOCK_INIT;
      os_unfair_lock_lock(&context->lock);
      atomic_store_explicit(&context->daemon, self, memory_order_release);
      context->sandboxed = 0xFF;
      context->isPlatformBinary = 0xFF;
      context->flags |= 8;
      os_unfair_lock_unlock(&context->lock);
      xpc_connection_set_context(peer, context);
      xpc_connection_set_finalizer_f(peer, client_context_finalizer);
      xpc_connection_activate(peer);
    });
    _sourceCacheLock = OS_UNFAIR_LOCK_INIT;
    os_unfair_lock_lock(&_sourceCacheLock);
    _sourceCache = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeSetCallBacks);
    _sourceCacheUsageCounts = OCFBagCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeBagCallBacks);
    os_unfair_lock_unlock(&_sourceCacheLock);
    _listener = listener;
    xpc_connection_activate(listener);
    return self;
}

// Native -[CFPrefsDaemon shmem], 0x1c02e93ac, with its .cold.1 (0x1c0315a08, the assertion "_shmem",
// cfprefsd.m line 1758) and ___22-[CFPrefsDaemon shmem]_block_invoke (0x1c03153a8, descriptor 0x2227a3470).
// Direct method. In cfprefsd the page is initialized once (shmem.onceToken); elsewhere under
// shmem.directModeShmemLock when not yet set. A missing page asserts.
- (void *)shmem {
    static dispatch_once_t onceToken;
    static os_unfair_lock directModeShmemLock = OS_UNFAIR_LOCK_INIT;
    if (isCFPrefsD == 1) {
        dispatch_once(&onceToken, ^{
          [self _initializeShmemPage:&self->_shmem];
        });
    } else {
        os_unfair_lock_lock(&directModeShmemLock);
        if (_shmem == NULL) {
            [self _initializeShmemPage:&_shmem];
        }
        os_unfair_lock_unlock(&directModeShmemLock);
    }
    if (_shmem == NULL) {
        __assert_rtn("-[CFPrefsDaemon shmem]", "cfprefsd.m", 1758, "_shmem");
    }
    return _shmem;
}

// Native processes whose path starts with one of these prefixes (__DATA_CONST,__const 0x2227aae50, ten C
// string pointers, unnamed natively) talk to no daemon.
static const char *const directModeProcessPaths[10] = {
    "/System/Library/CoreServices/ReportCrash",
    "/usr/sbin/distnoted",
    "/System/Library/PrivateFrameworks/SyncedDefaults.framework/Support/syncdefaultsd",
    "/usr/libexec/fseventsd",
    "/usr/libexec/trustd",
    "/usr/libexec/xpcd",
    "/usr/bin/heap",
    "/usr/bin/leaks",
    "/usr/bin/vmmap",
    "/usr/local/bin/ddt",
};

// Native _CFPrefsProcessPathRequiresDirectMode: no symbol (see above). Its statics .onceToken and
// .onBlacklist
// (__DATA,__bss 0x22e6d3d08 and 0x22e6d3d10) and ____CFPrefsProcessPathRequiresDirectMode_block_invoke
// (0x1c02e983c, the global block ___block_literal_global.157, with its .cold.1 0x1c0315a30, the debug log,
// and OUTLINED_FUNCTION_4_15). Hidden. Once, a known process path is compared (strncmp over each prefix's
// length) with the list until one matches; a match is logged at debug level.
bool _CFPrefsProcessPathRequiresDirectMode(void) {
    static dispatch_once_t onceToken;
    static bool onBlacklist;
    dispatch_once(&onceToken, ^{
      const char *path = _OCFProcessPath();
      if (path != NULL) {
          for (size_t index = 0; index < 10 && !onBlacklist; ++index) {
              const char *prefix = directModeProcessPaths[index];
              onBlacklist = strncmp(prefix, path, strlen(prefix)) == 0;
          }
      }
      if (onBlacklist) {
          os_log_debug(
              _CFPrefsClientLog(), "NSUserDefaults/CFPreferences is doing direct file access for this process"
          );
      }
    });
    return onBlacklist;
}

// Native -[CFPrefsDaemon synchronousWithSourceCache:], 0x1c03124a8 (with OUTLINED_FUNCTION_36_0). Direct
// method; it lies with the CFPDSource.m cluster natively. Under the cache lock, the block receives the source
// cache and the usage counts.
- (void)synchronousWithSourceCache:(void (^)(OCFMutableSetRef cache, OCFMutableBagRef usageCounts))block {
    os_unfair_lock_lock(&_sourceCacheLock);
    block(_sourceCache, _sourceCacheUsageCounts);
    os_unfair_lock_unlock(&_sourceCacheLock);
}

// Native -[CFPrefsDaemon _setSource:isDead:], 0x1c031251c. Direct method (CFPDSource.m cluster). The cache
// lock must be owned. For a managed source, the bit of its shmem index (word (index >> 6) truncated to eight
// bits, bit index modulo 64) is set or cleared in the dead-managed-source bit set.
- (void)_setSource:(id)source isDead:(BOOL)isDead {
    os_unfair_lock_assert_owner(&_sourceCacheLock);
    if (![(id<DaemonSourceMessages>)source managed]) {
        return;
    }
    int index = [(id<DaemonSourceMessages>)source shmemIndex];
    uint8_t word = (uint8_t)((uint32_t)index >> 6);
    uint64_t bit = 1ULL << ((uint32_t)index & 63);
    uint64_t bits = _deadManagedSourceBitSet[word];
    _deadManagedSourceBitSet[word] = isDead ? (bits | bit) : (bits & ~bit);
}

// Native -[CFPrefsDaemon _createSourceWithDomain:user:container:byHost:managed:shmemIndex:], 0x1c03125a8
// (with OUTLINED_FUNCTION_53). Direct method (CFPDSource.m cluster).
// With a container, a CFPDContainerSource. Otherwise a CFPDMirroredSource (mirroring the constant array
// mirroredKeys, "AppleLocale" and "AppleLanguages") for kCFPreferencesAnyApplication when the user is not
// kCFPreferencesAnyUser (compared first) and neither byHost nor managed is set, or outside cfprefsd for a
// domain ending in "_TestMirror"; else a CFPDSource. Each is allocated (objc_alloc) and initialized with the
// daemon; the answer is +1.
- (id)_createSourceWithDomain:(OCFStringRef)domain
                         user:(OCFStringRef)user
                    container:(OCFStringRef)container
                       byHost:(BOOL)byHost
                      managed:(BOOL)managed
                   shmemIndex:(int16_t)shmemIndex {
    if (container != NULL) {
        return [(id<DaemonSourceMessages>)[OCFPDContainerSource alloc] initWithDomain:domain
                                                                             userName:user
                                                                            container:container
                                                                               byHost:byHost
                                                                              managed:managed
                                                                           shmemIndex:shmemIndex
                                                                               daemon:self];
    }
    bool mirrored = false;
    if (OCFEqual(domain, kOCFPreferencesAnyApplication)) {
        bool shared = OCFEqual(user, kOCFPreferencesAnyUser) ? true : byHost;
        mirrored = !(managed || shared);
    }
    bool testMirror =
        !_CFPrefsCurrentProcessIsCFPrefsD() && OCFStringHasSuffix(domain, OF_STR("_TestMirror"));
    if (mirrored || testMirror) {
        return [(id<DaemonSourceMessages>)[OCFPDMirroredSource alloc] initWithDomain:domain
                                                                            userName:user
                                                                              byHost:byHost
                                                                             managed:managed
                                                                        mirroredKeys:(id)&mirroredKeys
                                                                          shmemIndex:shmemIndex
                                                                              daemon:self];
    }
    return [(id<DaemonSourceMessages>)[OCFPDSource alloc] initWithDomain:domain
                                                                userName:user
                                                                  byHost:byHost
                                                                 managed:managed
                                                              shmemIndex:shmemIndex
                                                                  daemon:self];
}

// Native -[CFPrefsDaemon logDomainInconsistencyForProcess:message:source:], 0x1c0314ab4 (with
// OUTLINED_FUNCTION_1_41 and _2_35), with ___65-[CFPrefsDaemon
// logDomainInconsistencyForProcess:message:source:]_block_invoke (0x1c02e6c78, descriptor 0x2227aab80, with
// .cold.1 0x1c0315610), _block_invoke.50 (0x1c02e6d50, descriptor 0x2227aab58, with .cold.1 0x1c0315654).
// Direct method. The source and message are retained and the work is dispatched asynchronously to the global
// queue of qos_class_main() (flags 2): a fault "Lookup inconsistency for request from pid %d" is logged;
// under the cache lock the cached source with the message's "CFPreferencesShmemIndex" is looked up
// (indexSearchCallback) and the inconsistency between it and the given source faulted; the source and message
// are released.
// Folds the compiler-outlined pieces OUTLINED_FUNCTION_1_41 (0x1c02e9fbc), OUTLINED_FUNCTION_2_35
// (0x1c02e9fd0).
- (void)logDomainInconsistencyForProcess:(int)pid message:(xpc_object_t)message source:(id)source {
    [source retain];
    xpc_retain(message);
    dispatch_async(dispatch_get_global_queue(qos_class_main(), 2), ^{
      int shmemIndex = (int)xpc_dictionary_get_int64(message, "CFPreferencesShmemIndex");
      os_log_fault(_CFPrefsDaemonLog(), "Lookup inconsistency for request from pid %d", pid);
      [self synchronousWithSourceCache:^(OCFMutableSetRef cache, OCFMutableBagRef usageCounts) {
        (void)usageCounts;
        id found = nil;
        IndexSearchContext search = {shmemIndex, &found};
        OCFSetApplyFunction(cache, indexSearchCallback, &search);
        os_log_fault(
            _CFPrefsDaemonLog(),
            "CFPreferences detected an inconsistency. An attempt by process %{public}d to access preferences "
            "in \n"
            "%{private}@\n actually resolved to \n%{private}@\n Typically this indicates that the process's "
            "sandbox "
            "profile changed in a way that added or removed a shared-preference-* rule or changed its "
            "container "
            "path. To avoid overwriting data incorrectly, cfprefsd is disconnecting this client from this "
            "source, "
            "its preferences will not be saved to disk",
            pid,
            found,
            source
        );
        [source release];
        xpc_release(message);
      }];
    });
}

// Native -[CFPrefsDaemon handleFlushManagedMessage:replyHandler:], 0x1c0314b64 (with OUTLINED_FUNCTION_10_6,
// _1_41 and _2_35), with ___56-[CFPrefsDaemon handleFlushManagedMessage:replyHandler:]_block_invoke
// (0x1c02e88cc, descriptor 0x2227aacb8) and _block_invoke_2 (0x1c0314e00, descriptor 0x2227aac90, with
// OUTLINED_FUNCTION_12_6). Direct method.
// The flush is logged and the managed-state caches reset; with a reply (or new dictionary), every known
// managed source re-reads its file, the shmem slot of every dead managed source (by set bit of the 64-word
// bit set, as a 16-bit index) is incremented (skipping zero on wrap), every managed tombstone notifies its
// observers, and the reply is answered and released.
- (void)handleFlushManagedMessage:(xpc_object_t)message
                     replyHandler:(void (^)(xpc_object_t reply))replyHandler {
    log_client_activity(message, "requested flush of managed sources", NULL);
    _CFPrefsResetManagedPreferencesStateCache();
    xpc_object_t reply = xpc_dictionary_create_reply(message);
    if (reply == NULL) {
        reply = xpc_dictionary_create(NULL, NULL, 0);
    }
    [self
        withAllKnownManagedSources:^(
            id *sources, OCFIndex sourceCount, id *tombstones, OCFIndex tombstoneCount, uint64_t *deadSources
        ) {
          for (OCFIndex remaining = sourceCount; remaining != 0; --remaining) {
              [(id<DaemonSourceMessages>)*sources++ respondToFileWrittenToBehindOurBack];
          }
          void (^bump)(int16_t index) = ^(int16_t index) {
            if (self != nil) {
                _Atomic(uint32_t) *slots = (_Atomic(uint32_t) *)[self shmem];
                if (atomic_fetch_add(&slots[index], 1) == UINT32_MAX) {
                    atomic_store(&slots[index], 1);
                }
            }
          };
          for (int word = 0; word != 64; ++word) {
              for (uint64_t bits = deadSources[word]; bits != 0;) {
                  int bit = __builtin_ctzll(bits);
                  bump((int16_t)(((word << 6) | 1) + bit - 1));
                  bits &= ~(1ULL << bit);
              }
          }
          for (OCFIndex remaining = tombstoneCount; remaining != 0; --remaining) {
              [(id<DaemonSourceMessages>)*tombstones++ notifyObservers];
          }
          replyHandler(reply);
          xpc_release(reply);
        }];
}

// Native -[CFPrefsDaemon handleAgentCheckInMessage:], 0x1c0314c1c (with OUTLINED_FUNCTION_4_15). Direct
// method. Logs (error level) that agents do not exist on this platform.
// Folds the compiler-outlined pieces OUTLINED_FUNCTION_4_15 (0x1c02e9ff0).
- (void)handleAgentCheckInMessage:(xpc_object_t)message {
    os_log_error(_CFPrefsDaemonLog(), "cfprefsd agents don't exist on non-macOS platforms");
}

// Native -[CFPrefsDaemon handleFlushSourceForDomainMessage:replyHandler:], 0x1c0314c78 (with
// OUTLINED_FUNCTION_10_6). Direct method. With a "CFPreferencesDomain" string the flush is logged, and its CF
// string (UTF-8, no copy, kCFAllocatorNull deallocator) has its sources snapshotted with the global block
// ___block_literal_global.91_0 (invoke ___64-[CFPrefsDaemon
// handleFlushSourceForDomainMessage:replyHandler:]_block_invoke, 0x1c02e8a0c: every source re-reads its file)
// and its shmem slots bumped, then released. A reply
// (or new dictionary) is answered and released.
- (void)handleFlushSourceForDomainMessage:(xpc_object_t)message
                             replyHandler:(void (^)(xpc_object_t reply))replyHandler {
    const char *domain = xpc_dictionary_get_string(message, "CFPreferencesDomain");
    if (domain != NULL) {
        log_client_activity(message, "requested flush of sources", domain);
        OCFStringRef string = OCFStringCreateWithCStringNoCopy(
            kOCFAllocatorSystemDefault, domain, kUTF8Encoding, kOCFAllocatorNull
        );
        if (string != NULL) {
            [self withSnapshotOfSourcesForDomainIdentifier:string performBlock:flushSourceSnapshotBlock];
            [self updateShmemForDomain:string];
            OCFRelease(string);
        }
    }
    xpc_object_t reply = xpc_dictionary_create_reply(message);
    if (reply == NULL) {
        reply = xpc_dictionary_create(NULL, NULL, 0);
    }
    replyHandler(reply);
    xpc_release(reply);
}

// The queue label as both queue constructors write it: "client-%d" or "client-%d-secondary" into a zeroed
// 128-byte buffer; a result outside 1-127 overwrites the first six bytes with "client" (no terminator).
// formatClientQueueLabel is a local descriptive name for the inlined code.
static inline void formatClientQueueLabel(char *label, const char *format, int pid) {
    memset(label, 0, 128);
    if ((uint32_t)(snprintf(label, 128, format, pid) - 128) <= 0xFFFFFF80u) {
        memcpy(label, "client", 6);
    }
}

// Native -[CFPrefsDaemon createSecondaryQueueForClientWithPID:], 0x1c0314d60 (with OUTLINED_FUNCTION_1_41,
// _2_35 and _6_13). Direct method. A serial queue "client-<pid>-secondary" (autorelease frequency work item)
// targeting the secondary workloop; +1.
- (dispatch_queue_t)createSecondaryQueueForClientWithPID:(int)pid {
    char label[136];
    formatClientQueueLabel(label, "client-%d-secondary", pid);
    dispatch_queue_attr_t attributes =
        dispatch_queue_attr_make_with_autorelease_frequency(NULL, DISPATCH_AUTORELEASE_FREQUENCY_WORK_ITEM);
    return dispatch_queue_create_with_target(label, attributes, _secondaryWorkloop);
}

// Native -[CFPrefsDaemon updateShmemIndex:], 0x1c0314e3c (with OUTLINED_FUNCTION_12_6). Direct method. The
// shmem slot of the index (sign-extended) is incremented (sequentially consistent); a wrap to zero is stored
// as 1.
- (void)updateShmemIndex:(int)index {
    _Atomic(uint32_t) *slots = (_Atomic(uint32_t) *)[self shmem];
    if (atomic_fetch_add(&slots[index], 1) == UINT32_MAX) {
        atomic_store(&slots[index], 1);
    }
}

// Native -[CFPrefsDaemon updateShmemForDomain:], 0x1c0314e74. Direct method.
// The domain's first slot is 7 * (CFHash % 585) + (length % 7) (0 without a domain); a slot above 4088 or
// zero becomes (length % 7) + 1. Unless the 16-bit slot exceeds 0xfff8, seven consecutive slots are
// incremented (sequentially consistent), a wrap to zero being stored as 1.
- (void)updateShmemForDomain:(OCFStringRef)domain {
    _Atomic(uint32_t) *slots = (_Atomic(uint32_t) *)[self shmem];
    int64_t base = 0;
    int64_t offset = 0;
    if (domain != NULL) {
        base = (int64_t)(7 * (OCFHash(domain) % 0x249));
        offset = OCFStringGetLength(domain) % 7;
    }
    int64_t index = offset + base;
    uint16_t slot = (index > 4088 || index == 0) ? (uint16_t)(offset + 1) : (uint16_t)index;
    if (slot > 0xFFF8) {
        return;
    }
    for (uint16_t current = slot; current != (uint16_t)(slot + 7); ++current) {
        if (atomic_fetch_add(&slots[current], 1) == UINT32_MAX) {
            atomic_store(&slots[current], 1);
        }
    }
}

// Native -[CFPrefsDaemon
// _handleSubMessagesFromArray:withCount:fromPeer:startingAt:originalMessage:replies:replyHandler:],
// 0x1c0314f34 (with OUTLINED_FUNCTION_1_41 and _2_35), with its block (0x1c02e8b44, descriptor 0x2227aad68,
// with .cold.1 0x1c03157fc, the recursion). Direct method.
// The element at the index is handled as a message when it is a dictionary, else answered NULL. Its reply
// (retained, or the XPC null object) fills the slot; after the last element the handler runs, otherwise the
// next element is handled.
- (void)_handleSubMessagesFromArray:(xpc_object_t)messages
                          withCount:(size_t)count
                           fromPeer:(xpc_connection_t)peer
                         startingAt:(size_t)index
                    originalMessage:(xpc_object_t)originalMessage
                            replies:(xpc_object_t *)replies
                       replyHandler:(void (^)(void))replyHandler {
    void (^next)(xpc_object_t reply) = ^(xpc_object_t reply) {
      replies[index] = reply != NULL ? xpc_retain(reply) : xpc_null_create();
      if (index == count - 1) {
          replyHandler();
      } else {
          [self _handleSubMessagesFromArray:messages
                                  withCount:count
                                   fromPeer:peer
                                 startingAt:index + 1
                            originalMessage:originalMessage
                                    replies:replies
                               replyHandler:replyHandler];
      }
    };
    xpc_object_t message = xpc_array_get_value(messages, index);
    if (xpc_get_type(message) == XPC_TYPE_DICTIONARY) {
        [self handleMessage:message originalMessage:originalMessage fromPeer:peer replyHandler:next];
    } else {
        next(NULL);
    }
}

// Native -[CFPrefsDaemon createQueueForClientWithPID:uid:], 0x1c031500c (with OUTLINED_FUNCTION_1_41, _2_35,
// _4_15 and _6_13). Direct method; the UID is unused.
// The label is "client-<pid>". Without the CoreFoundation/CFPreferencesWorkloops feature, or for a process
// whose path (proc_pidpath into a zeroed 4,096-byte buffer) starts with one of serialQueueProcessPaths, a
// plain serial queue (autorelease frequency work item). Otherwise, after a failed path lookup is logged at
// debug level with errno, a serial queue targeting the incoming workloop pid % count (signed). +1.
- (dispatch_queue_t)createQueueForClientWithPID:(int)pid uid:(uid_t)uid {
    char label[136];
    formatClientQueueLabel(label, "client-%d", pid);
    if (_os_feature_enabled_impl("CoreFoundation", "CFPreferencesWorkloops")) {
        char path[4096];
        bzero(path, sizeof(path));
        if (proc_pidpath(pid, path, 4096) >= 1) {
            for (size_t index = 0; index != 4; ++index) {
                if (strncmp(serialQueueProcessPaths[index], path, strlen(serialQueueProcessPaths[index])) ==
                    0) {
                    goto serial;
                }
            }
        } else {
            int error = errno;
            os_log_debug(_CFPrefsDaemonLog(), "proc_pidpath(%i) error: %{darwin.errno}d", pid, error);
        }
        dispatch_workloop_t target = _listenerWorkloops[pid % _listenerWorkloopsCount];
        dispatch_queue_attr_t attributes = dispatch_queue_attr_make_with_autorelease_frequency(
            NULL, DISPATCH_AUTORELEASE_FREQUENCY_WORK_ITEM
        );
        return dispatch_queue_create_with_target(label, attributes, target);
    }
serial:;
    dispatch_queue_attr_t attributes =
        dispatch_queue_attr_make_with_autorelease_frequency(NULL, DISPATCH_AUTORELEASE_FREQUENCY_WORK_ITEM);
    return dispatch_queue_create(label, attributes);
}

// Native -[CFPrefsDaemon role], 0x1c03151c4. Direct method.
- (int)role {
    return _role;
}

// Native -[CFPrefsDaemon _initializeShmemPage:], 0x1c03151d0 (with OUTLINED_FUNCTION_2_35, _8_10, _7_11,
// _4_15 and _1_41). Direct method. The name ("<apple|test>.sim.cfprefs.<system|user>.daemonv1", at most 30
// bytes, in a zeroed 1,024-byte buffer) is opened as POSIX shared memory: created read-write (O_RDWR |
// O_CREAT, mode 0644) in cfprefsd, opened read-write elsewhere only when the sandbox allows writing it. A
// descriptor is sized to 16 KiB and mapped shared read-write; a failed map stores NULL. When no page was
// obtained, cfprefsd logs a fault, and a zeroed 16 KiB heap page is used instead.
// Folds the compiler-outlined pieces OUTLINED_FUNCTION_7_11 (0x1c02ea010), OUTLINED_FUNCTION_8_10
// (0x1c02ea034).
- (void)_initializeShmemPage:(void **)page {
    char name[1024];
    bzero(name, sizeof(name));
    const char *prefix = _testMode ? "test.sim." : "apple.sim.";
    const char *format = _role == 2 ? "%scfprefs.system.daemonv%d" : "%scfprefs.user.daemonv%d";
    snprintf(name, 31, format, prefix, 1);
    int fd;
    if ((isCFPrefsD & 1) != 0) {
        fd = shm_open(name, O_RDWR | O_CREAT, 0644);
    } else {
        if (sandbox_check(getpid(), "ipc-posix-shm-write-data", SANDBOX_CHECK_NO_REPORT | 17, name) != 0) {
            goto check;
        }
        fd = shm_open(name, O_RDWR, 0644);
    }
    if (fd >= 0) {
        ftruncate(fd, kShmemPageSize);
        *page = mmap(NULL, kShmemPageSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        if (*page == MAP_FAILED) {
            *page = NULL;
            goto fallback;
        }
    }
check:
    if (*page != NULL) {
        return;
    }
fallback:
    if (isCFPrefsD == 1) {
        os_log_fault(
            _CFPrefsDaemonLog(),
            "cfprefsd could not create a shmem named %s, cross-process preferences will not work correctly",
            name
        );
    }
    *page = malloc_type_calloc(1, kShmemPageSize, 0x100004052888210);
}

// Native -[CFPrefsDaemon getShmemName:bufLen:], 0x1c0315370 (with OUTLINED_FUNCTION_8_10 and _7_11). Direct
// method. The shmem name as -_initializeShmemPage: writes it
// ("<apple|test>.sim.cfprefs.<system|user>.daemonv1", snprintf with a size of 31); the given length is not
// used. The snprintf result is left in x0 natively; no caller reads it, and it is restored as a void method
// (confidence medium for the return type).
- (void)getShmemName:(char *)name bufLen:(size_t)length {
    (void)length;
    const char *prefix = _testMode ? "test.sim." : "apple.sim.";
    const char *format = _role == 2 ? "%scfprefs.system.daemonv%d" : "%scfprefs.user.daemonv%d";
    snprintf(name, 31, format, prefix, 1);
}

// Native -[CFPrefsDaemon listener], 0x1c03153b4. Direct method.
- (xpc_connection_t)listener {
    return _listener;
}

@end

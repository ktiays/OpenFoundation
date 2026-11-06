//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Pasteboard/OCFPasteboard.h"

#include <dlfcn.h>

// Inferred organization: initSecTaskCreateWithAuditToken (0x1c01c8204) through
// initUTTypeCreatePreferredIdentifierForTag (0x1c01c886c) are interleaved with the CFPasteboard functions and
// blocks (____onqueue_CFPasteboardCleanupPendingPromises_block_invoke_2 before them,
// ____onqueue_CFPasteboardFlushLocalEntriesIfNecessary_block_invoke after) in the iOS Simulator __text
// section, so they are kept in a translation unit for that native unit; only its soft links are restored
// here. The native source path and language are not in the evidence. The pattern is a soft link: each
// framework is opened with dlopen (mode 2, RTLD_NOW) and never linked. Unless a definition says otherwise:
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in
// full). iOS Device 24A446: unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json.

// Native SecurityLibrary: no symbol (inlined); its function static frameworkLibrary
// (SecurityLibrary.frameworkLibrary, __DATA,__bss 0x22e6d2d68) holds the dlopen handle, retried while NULL.
static inline void *SecurityLibrary(void) {
    static void *frameworkLibrary;
    if (frameworkLibrary == NULL) {
        frameworkLibrary = dlopen("/System/Library/Frameworks/Security.framework/Security", RTLD_NOW);
    }
    return frameworkLibrary;
}

// Native LaunchServicesLibrary: no symbol (inlined); its function static frameworkLibrary
// (LaunchServicesLibrary.frameworkLibrary, __DATA,__bss 0x22e6d2d90), as SecurityLibrary.
static inline void *LaunchServicesLibrary(void) {
    static void *frameworkLibrary;
    if (frameworkLibrary == NULL) {
        frameworkLibrary = dlopen(
            "/System/Library/Frameworks/CoreServices.framework/Versions/Current/Frameworks/"
            "LaunchServices.framework/LaunchServices",
            RTLD_NOW
        );
    }
    return frameworkLibrary;
}

// Native initSecTaskCreateWithAuditToken, 0x1c01c8204. Non-external.
// Resolves the symbol (dlsym on the handle, NULL when the framework could not be opened), stores it and calls
// it with the arguments (the audit token is copied to the stack and passed by reference, as the by-value ABI
// requires). No result is checked.
static OCFTypeRef initSecTaskCreateWithAuditToken(OCFAllocatorRef allocator, audit_token_t token) {
    softLinkSecTaskCreateWithAuditToken =
        (SecTaskCreateWithAuditTokenPointer)dlsym(SecurityLibrary(), "SecTaskCreateWithAuditToken");
    return softLinkSecTaskCreateWithAuditToken(allocator, token);
}

// Native initSecTaskCopyValueForEntitlement, 0x1c01c82ac. Non-external. As above; a tail call.
static OCFTypeRef
initSecTaskCopyValueForEntitlement(OCFTypeRef task, OCFStringRef entitlement, OCFErrorRef *error) {
    softLinkSecTaskCopyValueForEntitlement =
        (SecTaskCopyValueForEntitlementPointer)dlsym(SecurityLibrary(), "SecTaskCopyValueForEntitlement");
    return softLinkSecTaskCopyValueForEntitlement(task, entitlement, error);
}

// Native init_LSCopyFrontUIApplication, 0x1c01c8550. Non-external. As above; a tail call with one argument.
static uintptr_t init_LSCopyFrontUIApplication(uintptr_t argument) {
    softLink_LSCopyFrontUIApplication =
        (LSCopyFrontUIApplicationPointer)dlsym(LaunchServicesLibrary(), "_LSCopyFrontUIApplication");
    return softLink_LSCopyFrontUIApplication(argument);
}

// Native init_LSCopyLSASNForAuditToken, 0x1c01c85a8. Non-external. As initSecTaskCreateWithAuditToken.
static uintptr_t init_LSCopyLSASNForAuditToken(uintptr_t argument, audit_token_t token) {
    softLink_LSCopyLSASNForAuditToken =
        (LSCopyLSASNForAuditTokenPointer)dlsym(LaunchServicesLibrary(), "_LSCopyLSASNForAuditToken");
    return softLink_LSCopyLSASNForAuditToken(argument, token);
}

// Native init_LSCompareASNs, 0x1c01c8650. Non-external. As above; a tail call with three arguments.
static uintptr_t init_LSCompareASNs(uintptr_t first, uintptr_t second, uintptr_t third) {
    softLink_LSCompareASNs = (LSCompareASNsPointer)dlsym(LaunchServicesLibrary(), "_LSCompareASNs");
    return softLink_LSCompareASNs(first, second, third);
}

// Native constantkUTTypeData and constantkUTTagClassNSPboardType (__DATA,__bss 0x22e6d2da8 and 0x22e6d2db0).
// Non-external.
static OCFStringRef constantkUTTypeData;
static OCFStringRef constantkUTTagClassNSPboardType;

// Native initkUTTypeData, 0x1c01c878c. Non-external.
// Without the framework, NULL is answered and the getter is kept. Otherwise the symbol's value is loaded (the
// dlsym result is dereferenced without a NULL check), stored, the getter is repointed to kUTTypeDataFunction,
// and the value is answered.
static OCFStringRef kUTTypeDataFunction(void);

static OCFStringRef initkUTTypeData(void) {
    void *library = LaunchServicesLibrary();
    if (library == NULL) {
        return NULL;
    }
    constantkUTTypeData = *(OCFStringRef *)dlsym(library, "kUTTypeData");
    getkUTTypeData = kUTTypeDataFunction;
    return constantkUTTypeData;
}

// Native kUTTypeDataFunction, 0x1c01c87f0. Non-external. Answers the stored value.
static OCFStringRef kUTTypeDataFunction(void) {
    return constantkUTTypeData;
}

// Native initkUTTagClassNSPboardType, 0x1c01c87fc. Non-external. As initkUTTypeData.
static OCFStringRef kUTTagClassNSPboardTypeFunction(void);

static OCFStringRef initkUTTagClassNSPboardType(void) {
    void *library = LaunchServicesLibrary();
    if (library == NULL) {
        return NULL;
    }
    constantkUTTagClassNSPboardType = *(OCFStringRef *)dlsym(library, "kUTTagClassNSPboardType");
    getkUTTagClassNSPboardType = kUTTagClassNSPboardTypeFunction;
    return constantkUTTagClassNSPboardType;
}

// Native kUTTagClassNSPboardTypeFunction, 0x1c01c8860. Non-external. Answers the stored value.
static OCFStringRef kUTTagClassNSPboardTypeFunction(void) {
    return constantkUTTagClassNSPboardType;
}

// Native initUTTypeCreatePreferredIdentifierForTag, 0x1c01c886c. Non-external. As
// initSecTaskCopyValueForEntitlement.
static OCFStringRef initUTTypeCreatePreferredIdentifierForTag(
    OCFStringRef tagClass, OCFStringRef tag, OCFStringRef conformingToUTI
) {
    softLinkUTTypeCreatePreferredIdentifierForTag = (UTTypeCreatePreferredIdentifierForTagPointer)dlsym(
        LaunchServicesLibrary(), "UTTypeCreatePreferredIdentifierForTag"
    );
    return softLinkUTTypeCreatePreferredIdentifierForTag(tagClass, tag, conformingToUTI);
}

// Native softLinkSecTaskCreateWithAuditToken through softLinkUTTypeCreatePreferredIdentifierForTag
// (__DATA,__data 0x22e6cf710-0x22e6cf748), each initialized to its init function. Non-external natively;
// hidden here because their users (the pasteboard functions) are outside this restoration.
SecTaskCreateWithAuditTokenPointer softLinkSecTaskCreateWithAuditToken = initSecTaskCreateWithAuditToken;
SecTaskCopyValueForEntitlementPointer softLinkSecTaskCopyValueForEntitlement =
    initSecTaskCopyValueForEntitlement;
LSCopyFrontUIApplicationPointer softLink_LSCopyFrontUIApplication = init_LSCopyFrontUIApplication;
LSCopyLSASNForAuditTokenPointer softLink_LSCopyLSASNForAuditToken = init_LSCopyLSASNForAuditToken;
LSCompareASNsPointer softLink_LSCompareASNs = init_LSCompareASNs;
UTTypeConstantGetter getkUTTypeData = initkUTTypeData;
UTTypeConstantGetter getkUTTagClassNSPboardType = initkUTTagClassNSPboardType;
UTTypeCreatePreferredIdentifierForTagPointer softLinkUTTypeCreatePreferredIdentifierForTag =
    initUTTypeCreatePreferredIdentifierForTag;

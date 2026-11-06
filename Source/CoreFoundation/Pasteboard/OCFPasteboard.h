//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Error/OCFError.h"

#include <mach/message.h>
#include <stdint.h>

// The soft links of the CFPasteboard unit (Security, LaunchServices). Each pointer starts at its init
// function, which opens the framework with dlopen (never linked), resolves the symbol and repoints the
// pointer; the pasteboard functions that call through them are outside this restoration, so the pointers are
// hidden here (non-external natively). The interfaces of the private symbols are unread: their pointer types
// give the register shape the init functions forward (64-bit values, a by-value audit token), not the
// native parameter types. SecTaskCreateWithAuditToken, SecTaskCopyValueForEntitlement and
// UTTypeCreatePreferredIdentifierForTag use their SDK signatures (Security SecTask.h, MobileCoreServices
// UTType.h), spelled with OCF types. The type names are local descriptive spellings.
typedef OCFTypeRef (*SecTaskCreateWithAuditTokenPointer)(OCFAllocatorRef allocator, audit_token_t token);
typedef OCFTypeRef (*SecTaskCopyValueForEntitlementPointer)(
    OCFTypeRef task, OCFStringRef entitlement, OCFErrorRef *error
);
typedef uintptr_t (*LSCopyFrontUIApplicationPointer)(uintptr_t argument);
typedef uintptr_t (*LSCopyLSASNForAuditTokenPointer)(uintptr_t argument, audit_token_t token);
typedef uintptr_t (*LSCompareASNsPointer)(uintptr_t first, uintptr_t second, uintptr_t third);
typedef OCFStringRef (*UTTypeConstantGetter)(void);
typedef OCFStringRef (*UTTypeCreatePreferredIdentifierForTagPointer)(
    OCFStringRef tagClass, OCFStringRef tag, OCFStringRef conformingToUTI
);

// Native softLinkSecTaskCreateWithAuditToken through softLinkUTTypeCreatePreferredIdentifierForTag
// (__DATA,__data 0x22e6cf710-0x22e6cf748, eight pointers in this order). Non-external natively.
OF_EXTERN SecTaskCreateWithAuditTokenPointer softLinkSecTaskCreateWithAuditToken;
OF_EXTERN SecTaskCopyValueForEntitlementPointer softLinkSecTaskCopyValueForEntitlement;
OF_EXTERN LSCopyFrontUIApplicationPointer softLink_LSCopyFrontUIApplication;
OF_EXTERN LSCopyLSASNForAuditTokenPointer softLink_LSCopyLSASNForAuditToken;
OF_EXTERN LSCompareASNsPointer softLink_LSCompareASNs;
OF_EXTERN UTTypeConstantGetter getkUTTypeData;
OF_EXTERN UTTypeConstantGetter getkUTTagClassNSPboardType;
OF_EXTERN UTTypeCreatePreferredIdentifierForTagPointer softLinkUTTypeCreatePreferredIdentifierForTag;

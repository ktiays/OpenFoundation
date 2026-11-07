//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Error/ONSError.h"

// Swift-only views of Foundation's error domain constants (Error/ONSErrorConstants.m), bound to the same
// symbols through asm labels. The constants are declared as id, which Swift imports as Any; the native Swift
// code loads them as object pointers and bridges them (String._unconditionallyBridgeFromObjectiveC), so these
// views give Swift the raw pointer. They define nothing. The names are local descriptive names.
extern const void *const cocoaErrorDomainObject __asm(OF_SYMBOL("") OF_NAME_STRING(ONSCocoaErrorDomain));
extern const void *const posixErrorDomainObject __asm(OF_SYMBOL("") OF_NAME_STRING(ONSPOSIXErrorDomain));
extern const void *const
    debugDescriptionErrorKeyObject __asm(OF_SYMBOL("") OF_NAME_STRING(ONSDebugDescriptionErrorKey));

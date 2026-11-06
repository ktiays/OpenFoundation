//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Error/ONSErrorConstants.h"

// iOS Simulator 27.0 (24A434), arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured assembly, except explicitly marked bindings.
// iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Scratch/cf-core-fd-misc/Reading.md; native source paths are unread, organization inferred.

// Native NSPOSIXErrorDomain, 0x2227d1258. External; CFString object 0x222824090.
static const OCFConstantString posixDomain = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(ONSPOSIXErrorDomain),
    sizeof(OF_NAME_STRING(ONSPOSIXErrorDomain)) - 1
};
id const ONSPOSIXErrorDomain = (id)&posixDomain;

// Native NSCocoaErrorDomain, 0x2227d1270. External; CFString object 0x2228240f0.
static const OCFConstantString cocoaDomain = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(ONSCocoaErrorDomain),
    sizeof(OF_NAME_STRING(ONSCocoaErrorDomain)) - 1
};
id const ONSCocoaErrorDomain = (id)&cocoaDomain;

// Native NSLocalizedDescriptionKey, 0x2227d11c0. External; CFString object 0x222823e50.
static const OCFConstantString localizedDescriptionKey = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSLocalizedDescription", 22
};
id const ONSLocalizedDescriptionKey = (id)&localizedDescriptionKey;

// Native NSDebugDescriptionErrorKey, 0x2227d1218. External; CFString object 0x222823f90.
static const OCFConstantString debugDescriptionKey = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSDebugDescription", 18
};
id const ONSDebugDescriptionErrorKey = (id)&debugDescriptionKey;

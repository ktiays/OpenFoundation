//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/URL/OCFURLPromises.h"

// Inferred organization: _CFURLPromiseCopyPhysicalURL (0x1c0239724) lies before the CFURL unit of
// URL/OCFURL.c in the iOS Simulator CoreFoundation __text section, among the URL resource functions that are
// outside the scope; it has its own unit here. Unless a definition says otherwise: iOS Simulator 24A434,
// CoreFoundation UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS
// Device 24A446: unread.

// Native _kCFURLPromisePhysicalURLKey, 0x2227a9b08 (__DATA_CONST,__const, external): the constant string
// "_NSURLPromisePhysicalURLKey".
static const OCFConstantString urlPromisePhysicalURLKeyString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "_NSURLPromisePhysicalURLKey", 27
};
const OCFStringRef _kOCFURLPromisePhysicalURLKey = (OCFStringRef)&urlPromisePhysicalURLKeyString;

// Native _CFURLPromiseCopyPhysicalURL, 0x1c0239724. External. For a file URL, its physical URL resource
// property, or (when the property is kCFNull) the URL itself retained; NULL for NULL and non-file URLs. The
// property is answered as copied (the URL itself is not retained when the property is not kCFNull).
OCFURLRef _OCFURLPromiseCopyPhysicalURL(OCFURLRef url) {
    OCFTypeRef physical = NULL;
    if (url == NULL) {
        return NULL;
    }
    if (!_OCFURLIsFileURL(url)) {
        return NULL;
    }
    OCFURLCopyResourcePropertyForKey(url, _kOCFURLPromisePhysicalURLKey, &physical, NULL);
    if (physical == kOCFNull) {
        return (OCFURLRef)OCFRetain(url);
    }
    return (OCFURLRef)physical;
}

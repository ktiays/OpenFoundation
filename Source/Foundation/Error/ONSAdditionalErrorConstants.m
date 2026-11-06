//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Error/ONSErrorConstants.h"

// Foundation, iOS Simulator 27.0 (24A434), arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured IDA assembly. iOS Device divergence: iOS Device 24A446 is unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json.

// Native NSCharacterConversionException, 0x2227d05d0. External; constant-string object 0x222820d30.
static const OCFConstantString kCharacterConversionException = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NS"
    "CharacterConversionException",
    sizeof("NS"
           "CharacterConversionException") -
        1
};
id const ONSCharacterConversionException = (id)&kCharacterConversionException;

// Native NSUnderlyingErrorKey, 0x2227d11a8. External; constant-string object 0x222823df0.
static const OCFConstantString kUnderlyingErrorKey = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSUnderlyingError", 17
};
id const ONSUnderlyingErrorKey = (id)&kUnderlyingErrorKey;

// Native NSStringEncodingErrorKey, 0x2227d11f0. External; constant-string object 0x222823f10.
static const OCFConstantString kStringEncodingErrorKey = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSStringEncoding", 16
};
id const ONSStringEncodingErrorKey = (id)&kStringEncodingErrorKey;

// Native NSURLErrorKey, 0x2227d11f8. External; constant-string object 0x22281e4f0.
static const OCFConstantString kURLErrorKey = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "NS"
    "URL",
    5
};
id const ONSURLErrorKey = (id)&kURLErrorKey;

// Native NSFilePathErrorKey, 0x2227d1200. External; constant-string object 0x222823f30.
static const OCFConstantString kFilePathErrorKey = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSFilePath", 10
};
id const ONSFilePathErrorKey = (id)&kFilePathErrorKey;

// Native NSInconsistentArchiveException, 0x2227d1360. External; constant-string object 0x222824a90.
static const OCFConstantString kInconsistentArchiveException = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSArchiverArchiveInconsistency", 30
};
id const ONSInconsistentArchiveException = (id)&kInconsistentArchiveException;

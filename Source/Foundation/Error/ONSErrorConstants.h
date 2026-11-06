//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/String/OCFString.h"

#include <objc/objc.h>

// NSString * const in the SDK (NSError.h); the object is the constant string record defined beside it.
OF_EXPORT id const ONSPOSIXErrorDomain;
OF_EXPORT id const ONSCocoaErrorDomain;
OF_EXPORT id const ONSLocalizedDescriptionKey;
OF_EXPORT id const ONSDebugDescriptionErrorKey;
OF_EXPORT id const ONSInvalidValueErrorKey;

OF_EXPORT id const ONSCharacterConversionException;
OF_EXPORT id const ONSUnderlyingErrorKey;
OF_EXPORT id const ONSStringEncodingErrorKey;
OF_EXPORT id const ONSURLErrorKey;
OF_EXPORT id const ONSFilePathErrorKey;
OF_EXPORT id const ONSInconsistentArchiveException;

OF_EXPORT id const ONSMinimumValidValueErrorKey;
OF_EXPORT id const ONSMaximumValidValueErrorKey;

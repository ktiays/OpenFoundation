//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/URL/OCFURL.h"

#include <mach/message.h>
#include <objc/objc.h>
#include <stddef.h>
#include <stdint.h>

// The soft links of the pasteboard server and detection cluster (BaseBoard, BiomeLibrary, BackBoardServices,
// SafariShared, DataDetectorsCore, UniformTypeIdentifiers, ImageIO). Each pointer starts at its init
// function, which opens the framework with dlopen (never linked), resolves the symbol, class or constant and
// repoints the pointer; the functions that call through them are outside this restoration, so the pointers
// are hidden here (non-external natively). The interfaces of the private symbols are unread: their pointer
// types give the register shape the init functions forward or answer (64-bit values, a by-value audit token,
// a double), not the native types. The CGImageSource functions and the kCGImageProperty constants use their
// SDK signatures (ImageIO CGImageSource.h, CGImageProperties.h), spelled with OCF types (a CGImageSourceRef
// is an OCFTypeRef here). The type names are local descriptive spellings.
typedef uintptr_t (*BSVersionedPIDForAuditTokenPointer)(audit_token_t token);
typedef id (*BiomeLibraryPointer)(void);
typedef Class (*DetectionClassGetter)(void);
typedef uintptr_t (*WBSUnifiedFieldInputTypeForStringPointer)(uintptr_t string);
typedef double (*DDScannerSmallTimeoutGetter)(void);
typedef uintptr_t (*DDResultGetCategoryPointer)(uintptr_t result);
typedef uintptr_t (*DDResultHasTypePointer)(uintptr_t result, uintptr_t type);
typedef const void *(*DetectionConstantGetter)(void);
typedef OCFTypeRef (*CGImageSourceCreateWithURLPointer)(OCFURLRef url, OCFDictionaryRef options);
typedef size_t (*CGImageSourceGetCountPointer)(OCFTypeRef isrc);
typedef OCFDictionaryRef (*CGImageSourceCopyPropertiesAtIndexPointer)(
    OCFTypeRef isrc, size_t index, OCFDictionaryRef options
);
typedef OCFStringRef (*ImagePropertyGetter)(void);
typedef OCFTypeRef (*CGImageSourceCreateWithDataPointer)(OCFDataRef data, OCFDictionaryRef options);

// Native getBMPasteboardChangeClass (__DATA,__data 0x22e6cfc38) and softLinkBSVersionedPIDForAuditToken
// through softLinkCGImageSourceCreateWithData (__DATA,__data 0x22e6cfd60-0x22e6cfe20, in this order).
// Non-external natively.
OF_EXTERN DetectionClassGetter getBMPasteboardChangeClass;
OF_EXTERN BSVersionedPIDForAuditTokenPointer softLinkBSVersionedPIDForAuditToken;
OF_EXTERN BiomeLibraryPointer softLinkBiomeLibrary;
OF_EXTERN DetectionClassGetter getBKSHIDEventAuthenticationOriginatorClass;
OF_EXTERN DetectionClassGetter getBKSHIDEventAuthenticationMessageClass;
OF_EXTERN WBSUnifiedFieldInputTypeForStringPointer softLinkWBSUnifiedFieldInputTypeForString;
OF_EXTERN DetectionClassGetter getDDScannerServiceConfigurationClass;
OF_EXTERN DDScannerSmallTimeoutGetter getDDScannerSmallTimeout;
OF_EXTERN DetectionClassGetter getDDScannerServiceClass;
OF_EXTERN DetectionConstantGetter getDDBinderEmailKey;
OF_EXTERN DetectionConstantGetter getDDBinderFlightInformationKey;
OF_EXTERN DetectionConstantGetter getDDBinderTrackingNumberKey;
OF_EXTERN DDResultGetCategoryPointer softLinkDDResultGetCategory;
OF_EXTERN DDResultHasTypePointer softLinkDDResultHasType;
OF_EXTERN DetectionClassGetter getUTTypeClass;
OF_EXTERN CGImageSourceCreateWithURLPointer softLinkCGImageSourceCreateWithURL;
OF_EXTERN ImagePropertyGetter getkCGImagePropertyPixelWidth;
OF_EXTERN ImagePropertyGetter getkCGImagePropertyPixelHeight;
OF_EXTERN ImagePropertyGetter getkCGImagePropertyDPIWidth;
OF_EXTERN ImagePropertyGetter getkCGImagePropertyDPIHeight;
OF_EXTERN ImagePropertyGetter getkCGImagePropertyOrientation;
OF_EXTERN ImagePropertyGetter getkCGImagePropertyDepth;
OF_EXTERN ImagePropertyGetter getkCGImagePropertyHasAlpha;
OF_EXTERN CGImageSourceGetCountPointer softLinkCGImageSourceGetCount;
OF_EXTERN CGImageSourceCopyPropertiesAtIndexPointer softLinkCGImageSourceCopyPropertiesAtIndex;
OF_EXTERN CGImageSourceCreateWithDataPointer softLinkCGImageSourceCreateWithData;

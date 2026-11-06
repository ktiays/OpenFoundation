//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Pasteboard/OCFPasteboardDetection.h"

#include <dlfcn.h>
#include <objc/runtime.h>

// Inferred organization: initBSVersionedPIDForAuditToken (0x1c026f6b0) through
// initCGImageSourceCreateWithData (0x1c02707c8) are interleaved with the pasteboard server handlers
// (__sendUnknownPasteboardReply,
// ___handle_barrier_block_invoke, ____haveVersionedPIDs_block_invoke,
// ____getRemotePasteboardClass_block_invoke) and the CFPasteboardDetection functions
// (___CFPasteboardDetectionDetectProbableWebData,
// ___CFPasteboardDetectionsDetectMetadataForImageSource) in the iOS Simulator __text section, and are
// followed by that cluster's OUTLINED_FUNCTION_0-12 (0x1c0270830); they are kept in one translation unit for
// that cluster. Whether the server handlers and the detection functions are one native unit or two is not
// established (confidence: medium for the unit boundary); only the soft links are restored here. The native
// source paths and languages are not in the evidence. The pattern is a soft link: each framework is opened
// with dlopen (mode 2, RTLD_NOW) and never linked. Unless a definition says otherwise: iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json.

// Native BaseBoardLibrary: no symbol (inlined); its function static frameworkLibrary
// (BaseBoardLibrary.frameworkLibrary, __DATA,__bss 0x22e6d35b0) holds the dlopen handle, retried while NULL.
static inline void *BaseBoardLibrary(void) {
    static void *frameworkLibrary;
    if (frameworkLibrary == NULL) {
        frameworkLibrary =
            dlopen("/System/Library/PrivateFrameworks/BaseBoard.framework/BaseBoard", RTLD_NOW);
    }
    return frameworkLibrary;
}

// Native BiomeLibraryLibrary: no symbol (inlined); its function static frameworkLibrary
// (BiomeLibraryLibrary.frameworkLibrary, __DATA,__bss 0x22e6d35b8) holds the dlopen handle, retried while
// NULL.
static inline void *BiomeLibraryLibrary(void) {
    static void *frameworkLibrary;
    if (frameworkLibrary == NULL) {
        frameworkLibrary =
            dlopen("/System/Library/PrivateFrameworks/BiomeLibrary.framework/BiomeLibrary", RTLD_NOW);
    }
    return frameworkLibrary;
}

// Native BackBoardServicesLibrary: no symbol (inlined); its function static frameworkLibrary
// (BackBoardServicesLibrary.frameworkLibrary, __DATA,__bss 0x22e6d35e0) holds the dlopen handle, retried
// while NULL.
static inline void *BackBoardServicesLibrary(void) {
    static void *frameworkLibrary;
    if (frameworkLibrary == NULL) {
        frameworkLibrary = dlopen(
            "/System/Library/PrivateFrameworks/BackBoardServices.framework/BackBoardServices", RTLD_NOW
        );
    }
    return frameworkLibrary;
}

// Native SafariSharedLibrary: no symbol (inlined); its function static frameworkLibrary
// (SafariSharedLibrary.frameworkLibrary, __DATA,__bss 0x22e6d3628) holds the dlopen handle, retried while
// NULL.
static inline void *SafariSharedLibrary(void) {
    static void *frameworkLibrary;
    if (frameworkLibrary == NULL) {
        frameworkLibrary =
            dlopen("/System/Library/PrivateFrameworks/SafariShared.framework/SafariShared", RTLD_NOW);
    }
    return frameworkLibrary;
}

// Native DataDetectorsCoreLibrary: no symbol (inlined); its function static frameworkLibrary
// (DataDetectorsCoreLibrary.frameworkLibrary, __DATA,__bss 0x22e6d3640) holds the dlopen handle, retried
// while NULL.
static inline void *DataDetectorsCoreLibrary(void) {
    static void *frameworkLibrary;
    if (frameworkLibrary == NULL) {
        frameworkLibrary = dlopen(
            "/System/Library/PrivateFrameworks/DataDetectorsCore.framework/DataDetectorsCore", RTLD_NOW
        );
    }
    return frameworkLibrary;
}

// Native UniformTypeIdentifiersLibrary: no symbol (inlined); its function static frameworkLibrary
// (UniformTypeIdentifiersLibrary.frameworkLibrary, __DATA,__bss 0x22e6d3690) holds the dlopen handle, retried
// while NULL.
static inline void *UniformTypeIdentifiersLibrary(void) {
    static void *frameworkLibrary;
    if (frameworkLibrary == NULL) {
        frameworkLibrary = dlopen(
            "/System/Library/Frameworks/UniformTypeIdentifiers.framework/UniformTypeIdentifiers", RTLD_NOW
        );
    }
    return frameworkLibrary;
}

// Native ImageIOLibrary: no symbol (inlined); its function static frameworkLibrary
// (ImageIOLibrary.frameworkLibrary, __DATA,__bss 0x22e6d36a8) holds the dlopen handle, retried while NULL.
static inline void *ImageIOLibrary(void) {
    static void *frameworkLibrary;
    if (frameworkLibrary == NULL) {
        frameworkLibrary = dlopen("/System/Library/Frameworks/ImageIO.framework/ImageIO", RTLD_NOW);
    }
    return frameworkLibrary;
}

// Native classBMPasteboardChange (0x22e6d35c0), classBKSHIDEventAuthenticationOriginator (0x22e6d35d8),
// classBKSHIDEventAuthenticationMessage (0x22e6d35f8), classDDScannerServiceConfiguration (0x22e6d3648),
// constantDDScannerSmallTimeout (0x22e6d3650), classDDScannerService (0x22e6d3658), constantDDBinderEmailKey
// (0x22e6d3660), constantDDBinderFlightInformationKey (0x22e6d3668), constantDDBinderTrackingNumberKey
// (0x22e6d3670), classUTType (0x22e6d3688) and constantkCGImagePropertyPixelWidth through
// constantkCGImagePropertyHasAlpha (0x22e6d36b0-0x22e6d36e0), all __DATA,__bss. Non-external.
static Class classBMPasteboardChange;
static Class classBKSHIDEventAuthenticationOriginator;
static Class classBKSHIDEventAuthenticationMessage;
static Class classDDScannerServiceConfiguration;
static double constantDDScannerSmallTimeout;
static Class classDDScannerService;
static const void *constantDDBinderEmailKey;
static const void *constantDDBinderFlightInformationKey;
static const void *constantDDBinderTrackingNumberKey;
static Class classUTType;
static OCFStringRef constantkCGImagePropertyPixelWidth;
static OCFStringRef constantkCGImagePropertyPixelHeight;
static OCFStringRef constantkCGImagePropertyDPIWidth;
static OCFStringRef constantkCGImagePropertyDPIHeight;
static OCFStringRef constantkCGImagePropertyOrientation;
static OCFStringRef constantkCGImagePropertyDepth;
static OCFStringRef constantkCGImagePropertyHasAlpha;

// Native initBSVersionedPIDForAuditToken, 0x1c026f6b0. Non-external.
// Resolves the symbol (dlsym on the handle, NULL when the framework could not be opened), stores it and calls
// it with the audit token (copied to the stack and passed by reference, as the by-value ABI requires). No
// result is checked.
static uintptr_t initBSVersionedPIDForAuditToken(audit_token_t token) {
    softLinkBSVersionedPIDForAuditToken =
        (BSVersionedPIDForAuditTokenPointer)dlsym(BaseBoardLibrary(), "BSVersionedPIDForAuditToken");
    return softLinkBSVersionedPIDForAuditToken(token);
}

// Native initBiomeLibrary, 0x1c026f748. Non-external. As initBSVersionedPIDForAuditToken; a tail call
// without arguments.
static id initBiomeLibrary(void) {
    softLinkBiomeLibrary = (BiomeLibraryPointer)dlsym(BiomeLibraryLibrary(), "BiomeLibrary");
    return softLinkBiomeLibrary();
}

static Class BMPasteboardChangeFunction(void);

// Native initBMPasteboardChange, 0x1c026f794. Non-external. Without the framework (a handle that is
// still NULL after dlopen), Nil is answered and the getter is kept. Otherwise the class is looked up
// (objc_getClass), stored, the getter is repointed to BMPasteboardChangeFunction, and the class is answered.
static Class initBMPasteboardChange(void) {
    if (BiomeLibraryLibrary() == NULL) {
        return Nil;
    }
    classBMPasteboardChange = (Class)objc_getClass("BMPasteboardChange");
    getBMPasteboardChangeClass = BMPasteboardChangeFunction;
    return classBMPasteboardChange;
}

// Native BMPasteboardChangeFunction, 0x1c026f7f4. Non-external. Answers the stored class.
static Class BMPasteboardChangeFunction(void) {
    return classBMPasteboardChange;
}

static Class BKSHIDEventAuthenticationOriginatorFunction(void);

// Native initBKSHIDEventAuthenticationOriginator, 0x1c026f824. Non-external. As initBMPasteboardChange.
static Class initBKSHIDEventAuthenticationOriginator(void) {
    if (BackBoardServicesLibrary() == NULL) {
        return Nil;
    }
    classBKSHIDEventAuthenticationOriginator = (Class)objc_getClass("BKSHIDEventAuthenticationOriginator");
    getBKSHIDEventAuthenticationOriginatorClass = BKSHIDEventAuthenticationOriginatorFunction;
    return classBKSHIDEventAuthenticationOriginator;
}

// Native BKSHIDEventAuthenticationOriginatorFunction, 0x1c026f884. Non-external. Answers the stored class.
static Class BKSHIDEventAuthenticationOriginatorFunction(void) {
    return classBKSHIDEventAuthenticationOriginator;
}

static Class BKSHIDEventAuthenticationMessageFunction(void);

// Native initBKSHIDEventAuthenticationMessage, 0x1c026f8b4. Non-external. As initBMPasteboardChange.
static Class initBKSHIDEventAuthenticationMessage(void) {
    if (BackBoardServicesLibrary() == NULL) {
        return Nil;
    }
    classBKSHIDEventAuthenticationMessage = (Class)objc_getClass("BKSHIDEventAuthenticationMessage");
    getBKSHIDEventAuthenticationMessageClass = BKSHIDEventAuthenticationMessageFunction;
    return classBKSHIDEventAuthenticationMessage;
}

// Native BKSHIDEventAuthenticationMessageFunction, 0x1c026f914. Non-external. Answers the stored class.
static Class BKSHIDEventAuthenticationMessageFunction(void) {
    return classBKSHIDEventAuthenticationMessage;
}

// Native initWBSUnifiedFieldInputTypeForString, 0x1c026fbf8. Non-external. As
// initBSVersionedPIDForAuditToken; a tail call with one argument.
static uintptr_t initWBSUnifiedFieldInputTypeForString(uintptr_t string) {
    softLinkWBSUnifiedFieldInputTypeForString = (WBSUnifiedFieldInputTypeForStringPointer)dlsym(
        SafariSharedLibrary(), "WBSUnifiedFieldInputTypeForString"
    );
    return softLinkWBSUnifiedFieldInputTypeForString(string);
}

static Class DDScannerServiceConfigurationFunction(void);

// Native initDDScannerServiceConfiguration, 0x1c026fcb0. Non-external. As initBMPasteboardChange.
static Class initDDScannerServiceConfiguration(void) {
    if (DataDetectorsCoreLibrary() == NULL) {
        return Nil;
    }
    classDDScannerServiceConfiguration = (Class)objc_getClass("DDScannerServiceConfiguration");
    getDDScannerServiceConfigurationClass = DDScannerServiceConfigurationFunction;
    return classDDScannerServiceConfiguration;
}

// Native DDScannerServiceConfigurationFunction, 0x1c026fd10. Non-external. Answers the stored class.
static Class DDScannerServiceConfigurationFunction(void) {
    return classDDScannerServiceConfiguration;
}

static double DDScannerSmallTimeoutFunction(void);

// Native initDDScannerSmallTimeout, 0x1c026fd1c. Non-external.
// Without the framework, 0.0 is answered and the getter is kept. Otherwise the symbol is resolved, the getter
// is repointed to DDScannerSmallTimeoutFunction first, and then the double is loaded (the dlsym result is
// dereferenced without a NULL check), stored and answered.
static double initDDScannerSmallTimeout(void) {
    void *library = DataDetectorsCoreLibrary();
    if (library == NULL) {
        return 0.0;
    }
    const double *symbol = dlsym(library, "DDScannerSmallTimeout");
    getDDScannerSmallTimeout = DDScannerSmallTimeoutFunction;
    constantDDScannerSmallTimeout = *symbol;
    return constantDDScannerSmallTimeout;
}

// Native DDScannerSmallTimeoutFunction, 0x1c026fd88. Non-external. Answers the stored value.
static double DDScannerSmallTimeoutFunction(void) {
    return constantDDScannerSmallTimeout;
}

static Class DDScannerServiceFunction(void);

// Native initDDScannerService, 0x1c026fd94. Non-external. As initBMPasteboardChange.
static Class initDDScannerService(void) {
    if (DataDetectorsCoreLibrary() == NULL) {
        return Nil;
    }
    classDDScannerService = (Class)objc_getClass("DDScannerService");
    getDDScannerServiceClass = DDScannerServiceFunction;
    return classDDScannerService;
}

// Native DDScannerServiceFunction, 0x1c026fdf4. Non-external. Answers the stored class.
static Class DDScannerServiceFunction(void) {
    return classDDScannerService;
}

// Native initDDResultGetCategory, 0x1c026fe00. Non-external. As
// initWBSUnifiedFieldInputTypeForString.
static uintptr_t initDDResultGetCategory(uintptr_t result) {
    softLinkDDResultGetCategory =
        (DDResultGetCategoryPointer)dlsym(DataDetectorsCoreLibrary(), "DDResultGetCategory");
    return softLinkDDResultGetCategory(result);
}

// Native initDDResultHasType, 0x1c026fe58. Non-external. As
// initBSVersionedPIDForAuditToken; a tail call with two arguments.
static uintptr_t initDDResultHasType(uintptr_t result, uintptr_t type) {
    softLinkDDResultHasType = (DDResultHasTypePointer)dlsym(DataDetectorsCoreLibrary(), "DDResultHasType");
    return softLinkDDResultHasType(result, type);
}

static const void *DDBinderEmailKeyFunction(void);

// Native initDDBinderEmailKey, 0x1c026fec0. Non-external. Without the framework, NULL is answered and the
// getter is kept. Otherwise the symbol's value is loaded (the dlsym result is dereferenced without a NULL
// check), stored, the getter is repointed to DDBinderEmailKeyFunction, and the value is answered.
static const void *initDDBinderEmailKey(void) {
    void *library = DataDetectorsCoreLibrary();
    if (library == NULL) {
        return NULL;
    }
    constantDDBinderEmailKey = *(const void **)dlsym(library, "DDBinderEmailKey");
    getDDBinderEmailKey = DDBinderEmailKeyFunction;
    return constantDDBinderEmailKey;
}

// Native DDBinderEmailKeyFunction, 0x1c026ff24. Non-external. Answers the stored value.
static const void *DDBinderEmailKeyFunction(void) {
    return constantDDBinderEmailKey;
}

static const void *DDBinderFlightInformationKeyFunction(void);

// Native initDDBinderFlightInformationKey, 0x1c026ff30. Non-external. As
// initDDBinderEmailKey.
static const void *initDDBinderFlightInformationKey(void) {
    void *library = DataDetectorsCoreLibrary();
    if (library == NULL) {
        return NULL;
    }
    constantDDBinderFlightInformationKey = *(const void **)dlsym(library, "DDBinderFlightInformationKey");
    getDDBinderFlightInformationKey = DDBinderFlightInformationKeyFunction;
    return constantDDBinderFlightInformationKey;
}

// Native DDBinderFlightInformationKeyFunction, 0x1c026ff94. Non-external. Answers the stored value.
static const void *DDBinderFlightInformationKeyFunction(void) {
    return constantDDBinderFlightInformationKey;
}

static const void *DDBinderTrackingNumberKeyFunction(void);

// Native initDDBinderTrackingNumberKey, 0x1c026ffa0. Non-external. As
// initDDBinderEmailKey.
static const void *initDDBinderTrackingNumberKey(void) {
    void *library = DataDetectorsCoreLibrary();
    if (library == NULL) {
        return NULL;
    }
    constantDDBinderTrackingNumberKey = *(const void **)dlsym(library, "DDBinderTrackingNumberKey");
    getDDBinderTrackingNumberKey = DDBinderTrackingNumberKeyFunction;
    return constantDDBinderTrackingNumberKey;
}

// Native DDBinderTrackingNumberKeyFunction, 0x1c0270004. Non-external. Answers the stored value.
static const void *DDBinderTrackingNumberKeyFunction(void) {
    return constantDDBinderTrackingNumberKey;
}

static Class UTTypeFunction(void);

// Native initUTType, 0x1c02701b0. Non-external. As initBMPasteboardChange.
static Class initUTType(void) {
    if (UniformTypeIdentifiersLibrary() == NULL) {
        return Nil;
    }
    classUTType = (Class)objc_getClass("UTType");
    getUTTypeClass = UTTypeFunction;
    return classUTType;
}

// Native UTTypeFunction, 0x1c0270210. Non-external. Answers the stored class.
static Class UTTypeFunction(void) {
    return classUTType;
}

// Native initCGImageSourceCreateWithURL, 0x1c027027c. Non-external. As
// initDDResultHasType.
static OCFTypeRef initCGImageSourceCreateWithURL(OCFURLRef url, OCFDictionaryRef options) {
    softLinkCGImageSourceCreateWithURL =
        (CGImageSourceCreateWithURLPointer)dlsym(ImageIOLibrary(), "CGImageSourceCreateWithURL");
    return softLinkCGImageSourceCreateWithURL(url, options);
}

// Native initCGImageSourceGetCount, 0x1c02703f0. Non-external. As
// initWBSUnifiedFieldInputTypeForString.
static size_t initCGImageSourceGetCount(OCFTypeRef isrc) {
    softLinkCGImageSourceGetCount =
        (CGImageSourceGetCountPointer)dlsym(ImageIOLibrary(), "CGImageSourceGetCount");
    return softLinkCGImageSourceGetCount(isrc);
}

// Native initCGImageSourceCopyPropertiesAtIndex, 0x1c0270448. Non-external. As
// initBSVersionedPIDForAuditToken; a tail call with three arguments.
static OCFDictionaryRef
initCGImageSourceCopyPropertiesAtIndex(OCFTypeRef isrc, size_t index, OCFDictionaryRef options) {
    softLinkCGImageSourceCopyPropertiesAtIndex = (CGImageSourceCopyPropertiesAtIndexPointer)dlsym(
        ImageIOLibrary(), "CGImageSourceCopyPropertiesAtIndex"
    );
    return softLinkCGImageSourceCopyPropertiesAtIndex(isrc, index, options);
}

static OCFStringRef kCGImagePropertyPixelWidthFunction(void);

// Native initkCGImagePropertyPixelWidth, 0x1c02704b8. Non-external. As initDDBinderEmailKey.
static OCFStringRef initkCGImagePropertyPixelWidth(void) {
    void *library = ImageIOLibrary();
    if (library == NULL) {
        return NULL;
    }
    constantkCGImagePropertyPixelWidth = *(OCFStringRef *)dlsym(library, "kCGImagePropertyPixelWidth");
    getkCGImagePropertyPixelWidth = kCGImagePropertyPixelWidthFunction;
    return constantkCGImagePropertyPixelWidth;
}

// Native kCGImagePropertyPixelWidthFunction, 0x1c027051c. Non-external. Answers the stored value.
static OCFStringRef kCGImagePropertyPixelWidthFunction(void) {
    return constantkCGImagePropertyPixelWidth;
}

static OCFStringRef kCGImagePropertyPixelHeightFunction(void);

// Native initkCGImagePropertyPixelHeight, 0x1c0270528. Non-external. As initDDBinderEmailKey.
static OCFStringRef initkCGImagePropertyPixelHeight(void) {
    void *library = ImageIOLibrary();
    if (library == NULL) {
        return NULL;
    }
    constantkCGImagePropertyPixelHeight = *(OCFStringRef *)dlsym(library, "kCGImagePropertyPixelHeight");
    getkCGImagePropertyPixelHeight = kCGImagePropertyPixelHeightFunction;
    return constantkCGImagePropertyPixelHeight;
}

// Native kCGImagePropertyPixelHeightFunction, 0x1c027058c. Non-external. Answers the stored value.
static OCFStringRef kCGImagePropertyPixelHeightFunction(void) {
    return constantkCGImagePropertyPixelHeight;
}

static OCFStringRef kCGImagePropertyDPIWidthFunction(void);

// Native initkCGImagePropertyDPIWidth, 0x1c0270598. Non-external. As initDDBinderEmailKey.
static OCFStringRef initkCGImagePropertyDPIWidth(void) {
    void *library = ImageIOLibrary();
    if (library == NULL) {
        return NULL;
    }
    constantkCGImagePropertyDPIWidth = *(OCFStringRef *)dlsym(library, "kCGImagePropertyDPIWidth");
    getkCGImagePropertyDPIWidth = kCGImagePropertyDPIWidthFunction;
    return constantkCGImagePropertyDPIWidth;
}

// Native kCGImagePropertyDPIWidthFunction, 0x1c02705fc. Non-external. Answers the stored value.
static OCFStringRef kCGImagePropertyDPIWidthFunction(void) {
    return constantkCGImagePropertyDPIWidth;
}

static OCFStringRef kCGImagePropertyDPIHeightFunction(void);

// Native initkCGImagePropertyDPIHeight, 0x1c0270608. Non-external. As initDDBinderEmailKey.
static OCFStringRef initkCGImagePropertyDPIHeight(void) {
    void *library = ImageIOLibrary();
    if (library == NULL) {
        return NULL;
    }
    constantkCGImagePropertyDPIHeight = *(OCFStringRef *)dlsym(library, "kCGImagePropertyDPIHeight");
    getkCGImagePropertyDPIHeight = kCGImagePropertyDPIHeightFunction;
    return constantkCGImagePropertyDPIHeight;
}

// Native kCGImagePropertyDPIHeightFunction, 0x1c027066c. Non-external. Answers the stored value.
static OCFStringRef kCGImagePropertyDPIHeightFunction(void) {
    return constantkCGImagePropertyDPIHeight;
}

static OCFStringRef kCGImagePropertyOrientationFunction(void);

// Native initkCGImagePropertyOrientation, 0x1c0270678. Non-external. As initDDBinderEmailKey.
static OCFStringRef initkCGImagePropertyOrientation(void) {
    void *library = ImageIOLibrary();
    if (library == NULL) {
        return NULL;
    }
    constantkCGImagePropertyOrientation = *(OCFStringRef *)dlsym(library, "kCGImagePropertyOrientation");
    getkCGImagePropertyOrientation = kCGImagePropertyOrientationFunction;
    return constantkCGImagePropertyOrientation;
}

// Native kCGImagePropertyOrientationFunction, 0x1c02706dc. Non-external. Answers the stored value.
static OCFStringRef kCGImagePropertyOrientationFunction(void) {
    return constantkCGImagePropertyOrientation;
}

static OCFStringRef kCGImagePropertyDepthFunction(void);

// Native initkCGImagePropertyDepth, 0x1c02706e8. Non-external. As initDDBinderEmailKey.
static OCFStringRef initkCGImagePropertyDepth(void) {
    void *library = ImageIOLibrary();
    if (library == NULL) {
        return NULL;
    }
    constantkCGImagePropertyDepth = *(OCFStringRef *)dlsym(library, "kCGImagePropertyDepth");
    getkCGImagePropertyDepth = kCGImagePropertyDepthFunction;
    return constantkCGImagePropertyDepth;
}

// Native kCGImagePropertyDepthFunction, 0x1c027074c. Non-external. Answers the stored value.
static OCFStringRef kCGImagePropertyDepthFunction(void) {
    return constantkCGImagePropertyDepth;
}

static OCFStringRef kCGImagePropertyHasAlphaFunction(void);

// Native initkCGImagePropertyHasAlpha, 0x1c0270758. Non-external. As initDDBinderEmailKey.
static OCFStringRef initkCGImagePropertyHasAlpha(void) {
    void *library = ImageIOLibrary();
    if (library == NULL) {
        return NULL;
    }
    constantkCGImagePropertyHasAlpha = *(OCFStringRef *)dlsym(library, "kCGImagePropertyHasAlpha");
    getkCGImagePropertyHasAlpha = kCGImagePropertyHasAlphaFunction;
    return constantkCGImagePropertyHasAlpha;
}

// Native kCGImagePropertyHasAlphaFunction, 0x1c02707bc. Non-external. Answers the stored value.
static OCFStringRef kCGImagePropertyHasAlphaFunction(void) {
    return constantkCGImagePropertyHasAlpha;
}

// Native initCGImageSourceCreateWithData, 0x1c02707c8. Non-external. As
// initDDResultHasType.
static OCFTypeRef initCGImageSourceCreateWithData(OCFDataRef data, OCFDictionaryRef options) {
    softLinkCGImageSourceCreateWithData =
        (CGImageSourceCreateWithDataPointer)dlsym(ImageIOLibrary(), "CGImageSourceCreateWithData");
    return softLinkCGImageSourceCreateWithData(data, options);
}

// Native getBMPasteboardChangeClass (__DATA,__data 0x22e6cfc38) and softLinkBSVersionedPIDForAuditToken
// through softLinkCGImageSourceCreateWithData (__DATA,__data 0x22e6cfd60-0x22e6cfe20), each initialized to
// its init function. Non-external natively; hidden here because their users are outside this restoration.
DetectionClassGetter getBMPasteboardChangeClass = initBMPasteboardChange;
BSVersionedPIDForAuditTokenPointer softLinkBSVersionedPIDForAuditToken = initBSVersionedPIDForAuditToken;
BiomeLibraryPointer softLinkBiomeLibrary = initBiomeLibrary;
DetectionClassGetter getBKSHIDEventAuthenticationOriginatorClass = initBKSHIDEventAuthenticationOriginator;
DetectionClassGetter getBKSHIDEventAuthenticationMessageClass = initBKSHIDEventAuthenticationMessage;
WBSUnifiedFieldInputTypeForStringPointer softLinkWBSUnifiedFieldInputTypeForString =
    initWBSUnifiedFieldInputTypeForString;
DetectionClassGetter getDDScannerServiceConfigurationClass = initDDScannerServiceConfiguration;
DDScannerSmallTimeoutGetter getDDScannerSmallTimeout = initDDScannerSmallTimeout;
DetectionClassGetter getDDScannerServiceClass = initDDScannerService;
DetectionConstantGetter getDDBinderEmailKey = initDDBinderEmailKey;
DetectionConstantGetter getDDBinderFlightInformationKey = initDDBinderFlightInformationKey;
DetectionConstantGetter getDDBinderTrackingNumberKey = initDDBinderTrackingNumberKey;
DDResultGetCategoryPointer softLinkDDResultGetCategory = initDDResultGetCategory;
DDResultHasTypePointer softLinkDDResultHasType = initDDResultHasType;
DetectionClassGetter getUTTypeClass = initUTType;
CGImageSourceCreateWithURLPointer softLinkCGImageSourceCreateWithURL = initCGImageSourceCreateWithURL;
ImagePropertyGetter getkCGImagePropertyPixelWidth = initkCGImagePropertyPixelWidth;
ImagePropertyGetter getkCGImagePropertyPixelHeight = initkCGImagePropertyPixelHeight;
ImagePropertyGetter getkCGImagePropertyDPIWidth = initkCGImagePropertyDPIWidth;
ImagePropertyGetter getkCGImagePropertyDPIHeight = initkCGImagePropertyDPIHeight;
ImagePropertyGetter getkCGImagePropertyOrientation = initkCGImagePropertyOrientation;
ImagePropertyGetter getkCGImagePropertyDepth = initkCGImagePropertyDepth;
ImagePropertyGetter getkCGImagePropertyHasAlpha = initkCGImagePropertyHasAlpha;
CGImageSourceGetCountPointer softLinkCGImageSourceGetCount = initCGImageSourceGetCount;
CGImageSourceCopyPropertiesAtIndexPointer softLinkCGImageSourceCopyPropertiesAtIndex =
    initCGImageSourceCopyPropertiesAtIndex;
CGImageSourceCreateWithDataPointer softLinkCGImageSourceCreateWithData = initCGImageSourceCreateWithData;

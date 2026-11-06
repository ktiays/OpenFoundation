//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PlugIn/OCFBundleMain.h"
#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/PlugIn/OCFBundleExecutable.h"
#include "CoreFoundation/PlugIn/OCFBundleGrok.h"
#include "CoreFoundation/PlugIn/OCFPlugIn.h"
#include "CoreFoundation/URL/OCFURL.h"

#include <objc/message.h>
#include <objc/runtime.h>
#include <string.h>

// Inferred organization: CFBundleGetMainBundle (0x1c0169a0c) precedes the CFBundle.c loggers in the iOS
// Simulator __text section and logs the source path PlugIn.subproj/CFBundle_Main.c, which names this unit;
// the other functions of that unit are outside the scope. Its .cold.1 fragment (0x1c03063ec) is folded in.
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationBundle.md (pending; the coordinator writes
// the reading).

// libobjc's objc_opt_class, as the native code imports it.
extern Class objc_opt_class(id object);

// The source path the native _CFBundleLogSourceLocation call of this unit passes (__cstring).
static const char kSourcePath[] =
    "/Library/Caches/com.apple.xbs/C9620487-17E9-4D33-8BF5-CCA0DA6AFB18/TemporaryDirectory.R3jFjg/Sources/"
    "CoreFoundation_Sim/CoreFoundation/PlugIn.subproj/CFBundle_Main.c";

// Native _mainBundle (0x22e6d26f0), _mainBundleLock (0x22e6d26f8) and _initedMainBundle (0x22e6d26fc),
// __DATA,__bss. Non-external.
static OCFBundleRef _mainBundle = NULL;
static os_unfair_lock _mainBundleLock = OS_UNFAIR_LOCK_INIT;
static uint8_t _initedMainBundle = 0;

// Native __CFBundleMainID (0x22e6d3e90) and __CFBundleShortVersionString (0x22e6d3e98), __DATA,__common.
// Non-external (were private externals). strdup copies written once by CFBundleGetMainBundle.
const char *__CFBundleMainID = NULL;
const char *__CFBundleShortVersionString = NULL;

// Native CFBundleGetMainBundle, 0x1c0169a0c. External.
// The slot-18 prelude (line 243). The Swift path answers +[_NSSwiftBundle mainBundle] (sent to the class of
// the looked-up class; not retained). Otherwise, under the main-bundle lock and once only: the process path
// becomes a string and a file URL (system default allocator), its bundle URL is found and a main bundle
// created. The bundle is marked loaded (+52); its binary type is grokked from the executable URL when still
// 0; a NULL dlopen handle becomes RTLD_MAIN_ONLY (-5), logging "main bundle %{public}@ getting handle %p" at
// the debug level of the loading logger (.cold.1). With a nonempty info dictionary, a binary of type 2 whose
// executable name is not a suffix of the process path takes the info dictionary embedded in the executable
// when that one is nonempty. With an empty one, version 0 becomes 4 when the executable name is not a suffix
// of the path (or either is NULL), version 3 becomes 4, and binary types 2 and 6 replace the dictionary with
// the embedded one. A missing dictionary becomes an empty mutable one; the process path string is retained
// into +96 when that is empty; the identifier and short version strings (key globals) are copied as UTF-8
// (1,024 bytes into a zeroed 1,026-byte buffer; a failed conversion gives "") into __CFBundleMainID and
// __CFBundleShortVersionString. The plug-in is then initialized and registered outside the lock. The process
// path string, executable URL and bundle URL are released; a creation failure keeps NULL.
OCFBundleRef OCFBundleGetMainBundle(void) {
    char buffer[1026];
    void *nested = _OCFGetTSD(0x12);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(0x12, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(outermost, kSourcePath, 243, OF_NAME_STRING(OCFBundleGetMainBundle));
    OCFBundleRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        Class swiftBundleClass = objc_opt_class((id)objc_lookUpClass(OF_NAME_STRING(_ONSSwiftBundle)));
        result = ((OCFBundleRef (*)(id, SEL))objc_msgSend)((id)swiftBundleClass, sel_getUid("mainBundle"));
    } else {
        os_unfair_lock_lock(&_mainBundleLock);
        if (!(_initedMainBundle & 1)) {
            _initedMainBundle = 1;
            const char *processPath = _OCFProcessPath();
            if (processPath != NULL) {
                OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
                OCFStringRef path = OCFStringCreateWithFileSystemRepresentation(allocator, processPath);
                OCFURLRef executableURL =
                    OCFURLCreateWithFileSystemPath(allocator, path, kOCFURLPOSIXPathStyle, false);
                if (executableURL == NULL) {
                    if (path != NULL) {
                        OCFRelease(path);
                    }
                } else {
                    OCFURLRef bundleURL = _OCFBundleCopyBundleURLForExecutableURL(executableURL);
                    if (bundleURL != NULL) {
                        _mainBundle = _CFBundleCreateMain(allocator, bundleURL);
                        if (_mainBundle != NULL) {
                            _mainBundle->_isLoaded = 1;
                            if (_mainBundle->_binaryType == 0) {
                                _mainBundle->_binaryType = _OCFBundleGrokBinaryType(executableURL);
                            }
                            if (_mainBundle->_handleCookie == NULL) {
                                _mainBundle->_handleCookie = (void *)(intptr_t)-5;
                                os_log_t log = _CFBundleLoadingLogger();
                                if (os_log_type_enabled(log, OS_LOG_TYPE_DEBUG)) {
                                    os_log_debug(
                                        log,
                                        "main bundle %{public}@ getting handle %p",
                                        _mainBundle,
                                        _mainBundle->_handleCookie
                                    );
                                }
                            }
                            OCFBundleGetInfoDictionary(_mainBundle);
                            if (_mainBundle->_infoDict != NULL &&
                                OCFDictionaryGetCount(_mainBundle->_infoDict) != 0) {
                                if (_mainBundle->_binaryType == 2) {
                                    OCFStringRef executableName =
                                        _CFBundleCopyExecutableName(_mainBundle, NULL, NULL);
                                    if (path == NULL || executableName == NULL ||
                                        !OCFStringHasSuffix(path, executableName)) {
                                        OCFMutableDictionaryRef embedded =
                                            _CFBundleCreateInfoDictFromMainExecutable();
                                        if (embedded != NULL) {
                                            if (OCFDictionaryGetCount(embedded) < 1) {
                                                OCFRelease(embedded);
                                            } else {
                                                if (_mainBundle->_infoDict != NULL) {
                                                    OCFRelease(_mainBundle->_infoDict);
                                                }
                                                _mainBundle->_infoDict = embedded;
                                            }
                                        }
                                    }
                                    if (executableName != NULL) {
                                        OCFRelease(executableName);
                                    }
                                }
                            } else {
                                uint8_t version = _mainBundle->_version;
                                if (version != 0) {
                                    if (version == 3) {
                                        _mainBundle->_version = 4;
                                    }
                                } else {
                                    OCFStringRef executableName =
                                        _CFBundleCopyExecutableName(_mainBundle, NULL, NULL);
                                    if (path != NULL && executableName != NULL) {
                                        if (!OCFStringHasSuffix(path, executableName)) {
                                            _mainBundle->_version = 4;
                                        }
                                    } else {
                                        _mainBundle->_version = 4;
                                    }
                                    if (executableName != NULL) {
                                        OCFRelease(executableName);
                                    }
                                }
                                if ((_mainBundle->_binaryType | 4) == 6) {
                                    if (_mainBundle->_infoDict != NULL) {
                                        OCFRelease(_mainBundle->_infoDict);
                                    }
                                    _mainBundle->_infoDict = _CFBundleCreateInfoDictFromMainExecutable();
                                }
                            }
                            if (_mainBundle->_infoDict == NULL) {
                                _mainBundle->_infoDict = OCFDictionaryCreateMutable(
                                    allocator,
                                    0,
                                    &kOCFTypeDictionaryKeyCallBacks,
                                    &kOCFTypeDictionaryValueCallBacks
                                );
                            }
                            if (path != NULL && _mainBundle->_executablePath == NULL) {
                                _mainBundle->_executablePath = OCFRetain(path);
                            }
                            OCFStringRef identifier =
                                OCFDictionaryGetValue(_mainBundle->_infoDict, kOCFBundleIdentifierKey);
                            if (identifier != NULL) {
                                bzero(buffer, 1026);
                                if (!OCFStringGetCString(identifier, buffer, 1024, kOCFStringEncodingUTF8)) {
                                    buffer[0] = 0;
                                }
                                __CFBundleMainID = strdup(buffer);
                            }
                            OCFStringRef shortVersion = OCFDictionaryGetValue(
                                _mainBundle->_infoDict, _kOCFBundleShortVersionStringKey
                            );
                            if (shortVersion != NULL) {
                                bzero(buffer, 1026);
                                if (!OCFStringGetCString(
                                        shortVersion, buffer, 1024, kOCFStringEncodingUTF8
                                    )) {
                                    buffer[0] = 0;
                                }
                                __CFBundleShortVersionString = strdup(buffer);
                            }
                            os_unfair_lock_unlock(&_mainBundleLock);
                            OCFDictionaryRef infoDict = OCFBundleGetInfoDictionary(_mainBundle);
                            _CFBundleInitPlugIn(_mainBundle, infoDict, NULL);
                            _CFPlugInHandleDynamicRegistration(_mainBundle);
                            os_unfair_lock_lock(&_mainBundleLock);
                        }
                        OCFRelease(bundleURL);
                    }
                    if (path != NULL) {
                        OCFRelease(path);
                    }
                    OCFRelease(executableURL);
                }
            }
        }
        result = _mainBundle;
        os_unfair_lock_unlock(&_mainBundleLock);
    }
    if (nested != (void *)1) {
        _OCFSetTSD(0x12, NULL, NULL);
    }
    return result;
}

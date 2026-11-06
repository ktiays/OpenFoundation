//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PlugIn/OCFBundleInfoPlist.h"

#include "CoreFoundation/Base/OCFFileUtilities.h"
#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Collections/OCFSet.h"
#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/PlugIn/OCFBundleResources.h"
#include "CoreFoundation/PlugIn/OCFBundleStrings.h"
#include "CoreFoundation/PropertyList/OCFPropertyList.h"
#include "CoreFoundation/String/OCFStringFormat.h"
#include "CoreFoundation/String/OCFString.h"

#include <assert.h>
#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <os/lock.h>
#include <os/log.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: _CFBundleInfoKeyIsValidSpecialCase (0x1c027fea4) and its block (0x1c027ff0c) sit
// apart from the other bundle clusters of the iOS Simulator __text section, and _CFBundleSplitFileName
// (0x1c02f43f0) follows the Mac Thai converters; their callers are the Info.plist functions and the
// bundle-resource unit. The Info.plist functions from _CFGetProductName (0x1c02fb348) through
// _CFBundleLogLoadingInfoPlistError (0x1c02fe968) and their blocks form one cluster whose functions log the
// source path PlugIn.subproj/CFBundle_InfoPlist.c and assert with "CFBundle_InfoPlist.c". They are kept in
// one translation unit in native address order, each block literal's invoke written inside its owner; the
// .cold fragments (0x1c03173d8-0x1c03174f8) are folded into their callers. The unit's other functions
// (_CFSetProductName, _CFGetPlatformName, the package-info and filtered-plist functions) are not in scope on
// this branch; the local-info and value-for-key functions joined in round 3. Unless a definition says
// otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS
// Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBundle.md (pending; the
// coordinator writes the reading). Folds the compiler-outlined piece _OUTLINED_FUNCTION_0_62 (0x1c02fef10).
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_1_44 (0x1c02fef24), _OUTLINED_FUNCTION_3_20
// (0x1c02fef48).

// libsystem_platform's os_unfair_lock_lock_with_options (private SPI); every native call here passes 0x10000
// (OS_UNFAIR_LOCK_DATA_SYNCHRONIZATION).
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);

// libobjc's objc_opt_class (objc-internal.h), as the native code imports it.
extern Class objc_opt_class(id object);

// The source path the native _CFBundleLogSourceLocation calls of this unit pass (__cstring 0x1c049d005).
static const char kSourcePath[] =
    "/Library/Caches/com.apple.xbs/C9620487-17E9-4D33-8BF5-CCA0DA6AFB18/TemporaryDirectory.R3jFjg/Sources/"
    "CoreFoundation_Sim/CoreFoundation/PlugIn.subproj/CFBundle_InfoPlist.c";

// The per-thread slot 0x12 marks a CFBundle entry point in progress: only the outermost entry logs its source
// location, and it clears the slot on return.
#define kBundleEntryTSDSlot 0x12

static OCFMutableDictionaryRef _CFBundleCopyInfoDictionaryInDirectoryWithVersion(
    OCFAllocatorRef allocator,
    OCFURLRef url,
    OCFURLRef *infoPlistURL,
    OCFSetRef keyPaths,
    uint8_t unusedOption,
    uint8_t version
);
static void _CFBundleLogLoadingInfoPlistError(OCFURLRef *url, OCFErrorRef error);

// Native __CFBundleInfoKeyIsValidSpecialCase.onceToken and .useSpecialCase (__DATA,__bss 0x22e6d3720 and
// 0x22e6d3718). Non-external.
static dispatch_once_t specialCaseOnce;
static uint8_t useSpecialCase;

// Native _CFBundleInfoKeyIsValidSpecialCase, 0x1c027fea4; its .cold.1 (0x1c030f090) is the dispatch_once of
// ___block_literal_global_31. Hidden (was private external). A key equal (CFEqual) to "CH" answers the cached
// flag; otherwise whether the key equals "OV". The block, ____CFBundleInfoKeyIsValidSpecialCase_block_invoke
// (0x1c027ff0c), opens /usr/lib/libMobileGestalt.dylib (mode 5, RTLD_LAZY | RTLD_LOCAL) and looks up
// MGCopyAnswer; without either the flag stays 0. The answer for "h63QSdBCiT/z0WU6rdQv6Q" (options NULL) sets
// the flag to whether it equals "CH" and is released; a NULL answer clears the flag. The image handle is not
// closed.
Boolean _CFBundleInfoKeyIsValidSpecialCase(OCFStringRef key) {
    if (!OCFEqual(key, OF_STR("CH"))) {
        return OCFEqual(key, OF_STR("OV")) != 0;
    }
    dispatch_once(&specialCaseOnce, ^{
      void *handle = dlopen("/usr/lib/libMobileGestalt.dylib", RTLD_LAZY | RTLD_LOCAL);
      if (handle == NULL) {
          return;
      }
      OCFTypeRef (*copyAnswer)(OCFStringRef, OCFDictionaryRef) =
          (OCFTypeRef (*)(OCFStringRef, OCFDictionaryRef))dlsym(handle, "MGCopyAnswer");
      if (copyAnswer == NULL) {
          return;
      }
      OCFTypeRef answer = copyAnswer(OF_STR("h63QSdBCiT/z0WU6rdQv6Q"), NULL);
      if (answer == NULL) {
          useSpecialCase = 0;
          return;
      }
      useSpecialCase = OCFEqual(answer, OF_STR("CH")) != 0;
      OCFRelease(answer);
    });
    return useSpecialCase;
}

// Native _CFBundleSplitFileName, 0x1c02f43f0. Hidden (was private external). X6 is never read. When endType
// or startType is wanted, the last "." (scanned from the end; its index kept as a 16-bit value, 0 when there
// is none) gives endType, the text after it, unless it is the last unit; the first "." gives startType, the
// text after it, unless it is the last "." (index difference 1). Then, for a name of two or more units, the
// units from the last down to index 1 are scanned once: each "." ends the current component; the first "~"
// starts the product component, which matches when expectedProduct compares equal (anchored, option 8) over
// the units up to the next "." or the end; a "-" stops the scan and starts the platform component (up to the
// "~" when a product matched, else up to the next "." or the end), compared anchored with expectedPlatform.
// *outcome is 1 (nothing), 2 (product), 3 (platform) or 4 (both); a name of fewer than two units yields 1.
// For 2, 3 and 4 *noProductOrPlatform receives a mutable copy (system default allocator) with the matched
// component, or both, deleted.
void _CFBundleSplitFileName(
    OCFStringRef fileName,
    OCFStringRef *noProductOrPlatform,
    OCFStringRef *endType,
    OCFStringRef *startType,
    OCFStringRef expectedProduct,
    OCFStringRef expectedPlatform,
    uint8_t searchForProductAndPlatform,
    int32_t *outcome
) {
    (void)searchForProductAndPlatform;
    OCFIndex length = OCFStringGetLength(fileName);
    if (((uintptr_t)endType | (uintptr_t)startType) != 0) {
        OCFIndex lastDot = 0;
        if (length - 1 >= 0) {
            for (OCFIndex index = length + 1;;) {
                if (OCFStringGetCharacterAtIndex(fileName, index - 2) == '.') {
                    lastDot = (uint16_t)(index - 2);
                    if (endType != NULL && length - 1 != lastDot) {
                        *endType = OCFStringCreateWithSubstring(
                            kOCFAllocatorSystemDefault,
                            fileName,
                            (OCFRange){lastDot + 1, OCFStringGetLength(fileName) + ~lastDot}
                        );
                    }
                    break;
                }
                index--;
                if (index < 2) {
                    lastDot = 0;
                    break;
                }
            }
        }
        if (startType != NULL && length >= 1) {
            for (OCFIndex position = 1; position != length + 1; position++) {
                if (OCFStringGetCharacterAtIndex(fileName, position - 1) == '.') {
                    if (position - lastDot != 1) {
                        *startType = OCFStringCreateWithSubstring(
                            kOCFAllocatorSystemDefault,
                            fileName,
                            (OCFRange){position, OCFStringGetLength(fileName) - position}
                        );
                    }
                    break;
                }
            }
        }
    }
    OCFIndex nameLength = OCFStringGetLength(fileName);
    OCFIndex index = nameLength - 1;
    if (index <= 0) {
        *outcome = 1;
        return;
    }
    OCFIndex componentEnd = nameLength;
    OCFIndex productIndex = 0;
    OCFIndex productLength = 0;
    Boolean foundProduct = false;
    OCFIndex tildeIndex = 0;
    OCFIndex platformIndex = 0;
    OCFIndex platformLength = 0;
    Boolean foundPlatform = false;
    for (;;) {
        UniChar character = OCFStringGetCharacterAtIndex(fileName, index);
        OCFIndex nextEnd = character == '.' ? index : componentEnd;
        if (character == '~' && !foundProduct) {
            OCFIndex candidateLength = componentEnd - index;
            OCFComparisonResult result = OCFStringCompareWithOptions(
                fileName, expectedProduct, (OCFRange){index, candidateLength}, kOCFCompareAnchored
            );
            if (result == kOCFCompareEqualTo) {
                productIndex = index;
                productLength = candidateLength;
                foundProduct = true;
            } else {
                foundProduct = false;
            }
            tildeIndex = index;
        } else if (character == '-') {
            OCFIndex end = foundProduct ? tildeIndex : componentEnd;
            platformLength = end - index;
            if (OCFStringCompareWithOptions(
                    fileName, expectedPlatform, (OCFRange){index, platformLength}, kOCFCompareAnchored
                ) == kOCFCompareEqualTo) {
                platformIndex = index;
                foundPlatform = true;
            }
            break;
        }
        OCFIndex previous = index;
        index--;
        componentEnd = nextEnd;
        if (previous + 1 <= 2) {
            break;
        }
    }
    if (!foundPlatform) {
        platformIndex = 0;
        platformLength = 0;
    }
    int32_t result = (foundPlatform ? 3 : 1) + (foundProduct ? 1 : 0);
    *outcome = result;
    if ((uint32_t)(result - 2) > 2) {
        return;
    }
    OCFIndex location;
    OCFIndex deleteLength;
    if ((uint32_t)(result - 3) < 2) {
        location = platformIndex;
        deleteLength = result == 4 ? platformLength + productLength : platformLength;
    } else {
        location = productIndex;
        deleteLength = productLength;
    }
    OCFMutableStringRef stripped = OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, 0, fileName);
    OCFStringDelete(stripped, (OCFRange){location, deleteLength});
    *noProductOrPlatform = stripped;
}

// Native _CFGetProductName.onceToken and ._cfBundlePlatform (0x22e6cff70, __DATA,__data, initially the
// constant empty string record 0x2227ac180). Non-external.
static const OCFConstantString kEmptyString = {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "", 0};
static dispatch_once_t productNameOnce;
static OCFStringRef cfBundlePlatform = (OCFStringRef)&kEmptyString;

// Native _CFGetProductName, 0x1c02fb348, with its .cold.1 (0x1c03173d8, the dispatch_once through
// _OUTLINED_FUNCTION_0_62) and ____CFGetProductName_block_invoke (0x1c02fb378, with its .cold.1 through
// .cold.3 debug logs). External. The block takes CLASSIC_SUFFIX ("iphone" or "ipad" prefixes, logged as
// classic), then SIMULATOR_LEGACY_ASSET_SUFFIX (exactly "iphone" or "ipad"), then SIMULATOR_MODEL_IDENTIFIER
// (strnlen 32; prefixes "iPhone", "iPod", "iPad", "Watch", "AppleTV", "RealityDevice" give "iphone", "ipod",
// "ipad", "applewatch", "appletv", "applevision"), and logs "Using ~%@ resources" with the platform at the
// debug level of the resource logger in every case.
// Folds the compiler-outlined pieces ____CFGetProductName_block_invoke.cold.1 (0x1c03173dc),
// ____CFGetProductName_block_invoke.cold.2 (0x1c0317408), ____CFGetProductName_block_invoke.cold.3
// (0x1c0317434).
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_2_38 (0x1c02fef38), _OUTLINED_FUNCTION_4_18
// (0x1c02fef54).
OCFStringRef _OCFGetProductName(void) {
    dispatch_once(&productNameOnce, ^{
      OCFStringRef platform = NULL;
      const char *classic = getenv("CLASSIC_SUFFIX");
      if (classic != NULL) {
          if (strncmp(classic, "iphone", 6) == 0) {
              os_log_debug(_CFBundleResourceLogger(), "Using ~iphone resources (classic)");
              platform = OF_STR("iphone");
          } else if (strncmp(classic, "ipad", 4) == 0) {
              os_log_debug(_CFBundleResourceLogger(), "Using ~ipad resources (classic)");
              platform = OF_STR("ipad");
          }
      }
      if (platform == NULL) {
          const char *legacy = getenv("SIMULATOR_LEGACY_ASSET_SUFFIX");
          if (legacy != NULL) {
              if (strcmp(legacy, "iphone") == 0) {
                  platform = OF_STR("iphone");
              } else if (strcmp(legacy, "ipad") == 0) {
                  platform = OF_STR("ipad");
              }
          }
      }
      if (platform == NULL) {
          const char *model = getenv("SIMULATOR_MODEL_IDENTIFIER");
          if (model != NULL) {
              size_t length = strnlen(model, 32);
              if (length >= 6 && memcmp(model, "iPhone", 6) == 0) {
                  platform = OF_STR("iphone");
              } else if (length >= 4) {
                  if (memcmp(model, "iPod", 4) == 0) {
                      platform = OF_STR("ipod");
                  } else if (memcmp(model, "iPad", 4) == 0) {
                      platform = OF_STR("ipad");
                  } else if (length >= 5 && memcmp(model, "Watch", 5) == 0) {
                      platform = OF_STR("applewatch");
                  } else if (length >= 7 && memcmp(model, "AppleTV", 7) == 0) {
                      platform = OF_STR("appletv");
                  } else if (length >= 13 && memcmp(model, "RealityDevice", 13) == 0) {
                      platform = OF_STR("applevision");
                  }
              }
          }
      }
      if (platform != NULL) {
          cfBundlePlatform = platform;
      }
      os_log_debug(_CFBundleResourceLogger(), "Using ~%@ resources", cfBundlePlatform);
    });
    return cfBundlePlatform;
}

// Native _CFBundleGetProductNameSuffix.onceToken and ._cfBundlePlatformSuffix (0x22e6d3dd8, __DATA,__bss).
// Non-external.
static dispatch_once_t productNameSuffixOnce;
static OCFStringRef cfBundlePlatformSuffix;

// Native _CFBundleGetProductNameSuffix, 0x1c02fb5b8, with its .cold.1 (0x1c0317480, dispatch_once of
// ___block_literal_global.2_1) and ____CFBundleGetProductNameSuffix_block_invoke (0x1c02fb5e8; its .cold.1,
// 0x1c0317494, is the inlined _CFGetProductName's dispatch_once). Hidden (was private external). "~" plus the
// product name (system default allocator), with "ipod" replaced by "iphone".
OCFStringRef _CFBundleGetProductNameSuffix(void) {
    dispatch_once(&productNameSuffixOnce, ^{
      OCFStringRef platform = _OCFGetProductName();
      OCFStringRef name = OCFEqual(platform, OF_STR("ipod")) ? OF_STR("iphone") : platform;
      cfBundlePlatformSuffix =
          OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR("~%@"), name);
    });
    return cfBundlePlatformSuffix;
}

// Native _CFBundleGetPlatformNameSuffix, 0x1c02fb670. Hidden (was private external). The constant
// "-iphoneos".
OCFStringRef _CFBundleGetPlatformNameSuffix(void) {
    return OF_STR("-iphoneos");
}

// Native _CFBundleSupportedPlatformName, 0x1c02fb694. Hidden (was private external). A tail call of
// CFStringFindWithOptions for "iphoneos" in the range, anchored (option 8), without a result range.
Boolean _CFBundleSupportedPlatformName(OCFStringRef fileName, OCFRange searchRange) {
    return OCFStringFindWithOptions(fileName, OF_STR("iphoneos"), searchRange, kOCFCompareAnchored, NULL);
}

// Native _CFBundleSupportedProductName.platforms, 0x2227abb70 (__DATA_CONST,__const, three pointers to the
// __cfstring records 0x2227bcb20 "iphone", 0x2227bcb00 "ipad" and 0x2227bcae0 "ipod"). Non-external.
static const OCFConstantString kIPhoneString = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7c8, "iphone", 6
};
static const OCFConstantString kIPadString = {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "ipad", 4};
static const OCFConstantString kIPodString = {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "ipod", 4};
static const OCFStringRef supportedProductNames[3] = {
    (OCFStringRef)&kIPhoneString, (OCFStringRef)&kIPadString, (OCFStringRef)&kIPodString
};

// Native _CFBundleSupportedProductName, 0x1c02fb6b0. Hidden (was private external). Whether any of the three
// product names is found anchored (option 8) in the range, in table order.
Boolean _CFBundleSupportedProductName(OCFStringRef fileName, OCFRange searchRange) {
    Boolean found;
    for (int index = 0;; index++) {
        found = OCFStringFindWithOptions(
            fileName, supportedProductNames[index], searchRange, kOCFCompareAnchored, NULL
        );
        if (found || index == 2) {
            break;
        }
    }
    return found;
}

// Native _CFBundleInfoPlistProcessInfoDictionary, 0x1c02fb724, with
// ____CFBundleInfoPlistProcessInfoDictionary_ block_invoke (0x1c02fb898; its .cold.1-.cold.4,
// 0x1c0317498-0x1c03174a4, are the inlined _CFGetProductName's dispatch_once),
// ____CFBundleInfoPlistProcessInfoDictionary_block_invoke_2 (0x1c02fbe98) and
// ____CopySortedOverridesForBaseKey_block_invoke (0x1c02fe80c, of the inlined
// _CopySortedOverridesForBaseKey). Hidden (was private external). Nothing happens for a NULL or empty
// dictionary. The first CFDictionaryApply block collects keys to remove (an array) and values to add (a
// dictionary), both system default with kCFType callbacks; the keys are then removed in array order and the
// additions applied (CFDictionarySetValue). Per key: a key whose type ID is not 7 is removed. A string key
// with a "#" (backwards search) after its first unit and a suffix accepted by
// _CFBundleInfoKeyIsValidSpecialCase maps its value to the part before the
// "#" and is removed. Otherwise the last "-" and last "~" (backwards searches) split it into base, platform
// and product, the "~" after the "-" when both are present; each part must be nonempty, the product
// supported (_CFBundleSupportedProductName) and the platform contain "iphoneos" anchored. A key whose
// components do not equal this device (the product name and "iphoneos") is removed; one that does gathers the
// present keys of "base-iphoneos~product", "base~product" and "base-iphoneos" in that order, maps base to the
// value of the first and removes the others.
// Folds the compiler-outlined pieces ____CFBundleInfoPlistProcessInfoDictionary_block_invoke.cold.2
// (0x1c031749c), ____CFBundleInfoPlistProcessInfoDictionary_block_invoke.cold.3 (0x1c03174a0).
void _CFBundleInfoPlistProcessInfoDictionary(OCFMutableDictionaryRef dict) {
    if (dict == NULL || OCFDictionaryGetCount(dict) == 0) {
        return;
    }
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFMutableArrayRef keysToRemove = OCFArrayCreateMutable(allocator, 0, &kOCFTypeArrayCallBacks);
    OCFMutableDictionaryRef valuesToAdd = OCFDictionaryCreateMutable(
        allocator, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    CFDictionaryApply(dict, ^(const void *key, const void *value, Boolean *stop) {
      (void)value;
      (void)stop;
      OCFStringRef keyString = (OCFStringRef)key;
      if (OCFGetTypeID(key) != 7) {
          OCFArrayAppendValue(keysToRemove, key);
          return;
      }
      if (key == NULL) {
          return;
      }
      OCFAllocatorRef substringAllocator = kOCFAllocatorSystemDefault;
      OCFRange hash = OCFStringFind(keyString, OF_STR("#"), kOCFCompareBackwards);
      if (hash.location != kOCFNotFound && hash.location >= 1) {
          OCFIndex suffixStart = hash.location + hash.length;
          OCFStringRef suffix = OCFStringCreateWithSubstring(
              substringAllocator,
              keyString,
              (OCFRange){suffixStart, OCFStringGetLength(keyString) - suffixStart}
          );
          if (_CFBundleInfoKeyIsValidSpecialCase(suffix)) {
              OCFStringRef base =
                  OCFStringCreateWithSubstring(substringAllocator, keyString, (OCFRange){0, hash.location});
              OCFDictionarySetValue(valuesToAdd, base, OCFDictionaryGetValue(dict, key));
              OCFArrayAppendValue(keysToRemove, key);
              OCFRelease(base);
              if (suffix != NULL) {
                  OCFRelease(suffix);
              }
              return;
          }
          if (suffix != NULL) {
              OCFRelease(suffix);
          }
      }
      OCFRange dash = OCFStringFind(keyString, OF_STR("-"), kOCFCompareBackwards);
      OCFRange tilde = OCFStringFind(keyString, OF_STR("~"), kOCFCompareBackwards);
      if (dash.location == kOCFNotFound && tilde.location == kOCFNotFound) {
          return;
      }
      if (dash.location != kOCFNotFound && tilde.location != kOCFNotFound &&
          tilde.location <= dash.location) {
          return;
      }
      OCFIndex length = OCFStringGetLength(keyString);
      OCFIndex platformEnd = tilde.location == kOCFNotFound ? length : tilde.location;
      OCFIndex productStart = tilde.location == kOCFNotFound ? kOCFNotFound : tilde.location + tilde.length;
      OCFIndex productLength = tilde.location == kOCFNotFound ? 0 : length - (tilde.location + tilde.length);
      OCFIndex baseLength = dash.location == kOCFNotFound ? tilde.location : dash.location;
      OCFIndex platformStart = dash.location == kOCFNotFound ? kOCFNotFound : dash.location + dash.length;
      OCFIndex platformLength =
          dash.location == kOCFNotFound ? 0 : platformEnd - (dash.location + dash.length);
      if (baseLength < 1) {
          return;
      }
      if (platformStart != kOCFNotFound && platformLength < 1) {
          return;
      }
      if (productStart != kOCFNotFound && productLength < 1) {
          return;
      }
      Boolean supported;
      if (platformStart == kOCFNotFound && productStart != kOCFNotFound) {
          supported = _CFBundleSupportedProductName(keyString, (OCFRange){productStart, productLength});
      } else if (platformStart != kOCFNotFound && productStart == kOCFNotFound) {
          supported = OCFStringFindWithOptions(
              keyString,
              OF_STR("iphoneos"),
              (OCFRange){platformStart, platformLength},
              kOCFCompareAnchored,
              NULL
          );
      } else {
          supported = _CFBundleSupportedProductName(keyString, (OCFRange){productStart, productLength}) &&
                      OCFStringFindWithOptions(
                          keyString,
                          OF_STR("iphoneos"),
                          (OCFRange){platformStart, platformLength},
                          kOCFCompareAnchored,
                          NULL
                      );
      }
      if (!supported) {
          return;
      }
      OCFStringRef base =
          OCFStringCreateWithSubstring(substringAllocator, keyString, (OCFRange){0, baseLength});
      OCFStringRef platform = NULL;
      if (platformStart != kOCFNotFound) {
          platform = OCFStringCreateWithSubstring(
              substringAllocator, keyString, (OCFRange){platformStart, platformLength}
          );
      }
      OCFStringRef product = NULL;
      if (productStart != kOCFNotFound) {
          product = OCFStringCreateWithSubstring(
              substringAllocator, keyString, (OCFRange){productStart, productLength}
          );
      }
      Boolean matches;
      if (platform == NULL && product == NULL) {
          matches = true;
      } else if (platform == NULL) {
          matches = OCFEqual(_OCFGetProductName(), product);
      } else if (product == NULL) {
          matches = OCFEqual(OF_STR("iphoneos"), platform);
      } else {
          matches = OCFEqual(_OCFGetProductName(), product) && OCFEqual(OF_STR("iphoneos"), platform);
      }
      if (!matches) {
          OCFArrayAppendValue(keysToRemove, key);
      } else {
          OCFDictionaryRef infoDict = dict;
          OCFMutableArrayRef candidates =
              OCFArrayCreateMutable(substringAllocator, 0, &kOCFTypeArrayCallBacks);
          OCFStringRef both = OCFStringCreateWithFormat(
              substringAllocator, NULL, OF_STR("%@-%@~%@"), base, OF_STR("iphoneos"), _OCFGetProductName()
          );
          OCFStringRef productOnly = OCFStringCreateWithFormat(
              substringAllocator, NULL, OF_STR("%@~%@"), base, _OCFGetProductName()
          );
          OCFStringRef platformOnly =
              OCFStringCreateWithFormat(substringAllocator, NULL, OF_STR("%@-%@"), base, OF_STR("iphoneos"));
          OCFArrayAppendValue(candidates, both);
          OCFArrayAppendValue(candidates, productOnly);
          OCFArrayAppendValue(candidates, platformOnly);
          OCFRelease(both);
          OCFRelease(productOnly);
          OCFRelease(platformOnly);
          OCFMutableArrayRef overrides =
              OCFArrayCreateMutable(substringAllocator, 0, &kOCFTypeArrayCallBacks);
          CFArrayApply(
              candidates,
              (OCFRange){0, OCFArrayGetCount(candidates)},
              ^(const void *candidate, bool *stopApply) {
                (void)stopApply;
                if (OCFDictionaryGetValueIfPresent(infoDict, candidate, NULL)) {
                    OCFArrayAppendValue(overrides, candidate);
                }
              }
          );
          OCFRelease(candidates);
          OCFDictionarySetValue(
              valuesToAdd, base, OCFDictionaryGetValue(dict, OCFArrayGetValueAtIndex(overrides, 0))
          );
          OCFIndex count = OCFArrayGetCount(overrides);
          if (count - 1 >= 0) {
              OCFArrayAppendArray(keysToRemove, overrides, (OCFRange){1, count - 1});
          }
          OCFRelease(overrides);
      }
      if (platform != NULL) {
          OCFRelease(platform);
      }
      if (product != NULL) {
          OCFRelease(product);
      }
      OCFRelease(base);
    });
    for (OCFIndex index = 0; index < OCFArrayGetCount(keysToRemove); index++) {
        OCFDictionaryRemoveValue(dict, OCFArrayGetValueAtIndex(keysToRemove, index));
    }
    CFDictionaryApply(valuesToAdd, ^(const void *key, const void *value, Boolean *stop) {
      (void)stop;
      OCFDictionarySetValue(dict, key, value);
    });
    OCFRelease(keysToRemove);
    OCFRelease(valuesToAdd);
}

// Native __CFVersionNumberFromString.versionStageTable, the unnamed table dword_1C046FD08 (__TEXT,__const,
// six 32-bit stage values indexed by the letter minus 'a': 'a' 0x4000, 'b' 0x6000, 'd' 0x2000, 'f' 0x8000;
// 'c' and 'e' are rejected before the load). kVersionStageValues is a local name.
static const UInt32 kVersionStageValues[6] = {0x4000, 0x6000, 0x0, 0x2000, 0x0, 0x8000};

static Boolean isVersionDigit(UniChar character) {
    return character >= '0' && character <= '9';
}

// Native _CFVersionNumberFromString, 0x1c02fbea0. External. NULL and lengths outside 1..10 answer 0. The
// UTF-16 units are read into a 10-unit buffer. Up to two major digits, then an optional "." with one minor
// digit, then an optional "." with one bug-fix digit; the rest must be empty or one stage letter ('a', 'b',
// 'd' or 'f' through the stage table; without a letter the stage is 0x8000) followed by at most three decimal
// digits making a build number of at most 255. The answer is major tens << 28 + major units << 24 + minor <<
// 20 + bug fix << 16 + stage + build; any other text answers 0.
UInt32 _OCFVersionNumberFromString(OCFStringRef versStr) {
    if (versStr == NULL) {
        return 0;
    }
    OCFIndex length = OCFStringGetLength(versStr);
    if ((uint64_t)(length - 11) < (uint64_t)-10) {
        return 0;
    }
    UniChar buffer[10];
    OCFStringGetCharacters(versStr, (OCFRange){0, length}, buffer);
    const UniChar *cursor = buffer;
    OCFIndex remaining = length;
    UInt32 majorTens = 0;
    UInt32 majorUnits = 0;
    UInt32 minor = 0;
    UInt32 bugFix = 0;
    UInt32 stage = 0x8000;
    UInt32 build = 0;
    if (isVersionDigit(*cursor)) {
        majorUnits = *cursor - '0';
        cursor++;
        remaining--;
        if (remaining != 0 && isVersionDigit(*cursor)) {
            majorTens = majorUnits;
            majorUnits = *cursor - '0';
            cursor++;
            remaining--;
        }
    }
    if (remaining != 0 && *cursor == '.') {
        cursor++;
        remaining--;
        if (remaining != 0 && isVersionDigit(*cursor)) {
            minor = *cursor - '0';
            cursor++;
            remaining--;
            if (remaining != 0 && *cursor == '.') {
                cursor++;
                remaining--;
                if (remaining != 0 && isVersionDigit(*cursor)) {
                    bugFix = *cursor - '0';
                    cursor++;
                    remaining--;
                }
            }
        }
    }
    if (remaining != 0) {
        uint32_t letter = (uint32_t)*cursor - 'a';
        if (letter > 5 || ((0x2B >> letter) & 1) == 0) {
            return 0;
        }
        stage = kVersionStageValues[letter];
        cursor++;
        remaining--;
        if (remaining > 3) {
            return 0;
        }
        for (OCFIndex index = 0; index < remaining; index++) {
            if (!isVersionDigit(cursor[index])) {
                return 0;
            }
            build = build * 10 + (cursor[index] - '0');
        }
        if (build > 0xFF) {
            return 0;
        }
    }
    return stage + (minor << 20) + (majorUnits << 24) + (majorTens << 28) + (bugFix << 16) + build;
}

// Native _CFBundleLoadLimitedInfoDictionary, 0x1c02fc194, with its .cold.1 (0x1c03174a8) and .cold.2
// (0x1c03174d0) assertions (__assert_rtn, CFBundle_InfoPlist.c lines 1107 and 1112). Hidden (was private
// external). Asserts that +24 is NULL, loads the dictionary for the bundle URL with the creation option (+55)
// and version (+53) limited to the key paths, stores it at +24, asserts that +208 is NULL and stores the
// Info.plist URL there; X0 still holds the dictionary at the return.
OCFDictionaryRef _CFBundleLoadLimitedInfoDictionary(OCFBundleRef bundle, OCFSetRef keyPaths) {
    if (bundle->_infoDict != NULL) {
        __assert_rtn(
            "_CFBundleLoadLimitedInfoDictionary", "CFBundle_InfoPlist.c", 1107, "bundle->_infoDict == NULL"
        );
    }
    OCFURLRef infoPlistURL = NULL;
    OCFDictionaryRef result = _CFBundleCopyInfoDictionaryInDirectoryWithVersion(
        kOCFAllocatorSystemDefault,
        bundle->_url,
        &infoPlistURL,
        keyPaths,
        bundle->_createOption1,
        bundle->_version
    );
    bundle->_infoDict = result;
    if (bundle->_infoPlistURL != NULL) {
        __assert_rtn(
            "_CFBundleLoadLimitedInfoDictionary", "CFBundle_InfoPlist.c", 1112, "!bundle->_infoPlistUrl"
        );
    }
    bundle->_infoPlistURL = infoPlistURL;
    return result;
}

// Native _CFBundleCopyInfoDictionaryInDirectoryWithVersion, 0x1c02fc230, with
// ____CFBundleCopyInfoDictionaryInDirectoryWithVersion_block_invoke (0x1c02fe850) and the blocks of the
// inlined _CFBundleCopyLimitedInfoPlistKeys (0x1c02fea14, 0x1c02fea20) and
// _CFCopyLogicalKeysMappedToProcessedValues (0x1c02feaf0). Non-external. The fifth argument (W4) is never
// read. NULL for a NULL URL. The version selects the directory and names: 0 "Resources/", 1
// "Support%20Files/", 2 "Contents/", 12 "WrappedBundle/Contents/", 13 "WrappedBundle/" (URLs relative to the
// base, system default allocator), 3 the base itself unless its path ends in "Support Files", "Contents" or
// "Resources" (then no directory); the names are
// "<dir>Info-iphoneos.plist" and "<dir>Info.plist". Any other version or a missing directory answers an empty
// mutable dictionary in the allocator. The directory is iterated (no slash appended, no prefixes) until both
// names are found, matched case-insensitively and anchored (option 9) with equal lengths. With key paths, the
// platform file then the plain one is loaded mapped (allowMapping 1, flag 0), logging a load error; its
// top-level keys become a dictionary mapping each key to itself, processed like an Info.plist, through which
// the key paths' first components are mapped to the physical keys to filter; the filtered values are entered
// under the logical keys (a dictionary in the allocator). Without key paths, or when no file loads, the
// platform file then the plain one is read (_CFDataCreateFromURL) and parsed with mutable containers (option
// 1); a dictionary is processed, anything else or a parse failure (logged at the error level of the resource
// logger) answers an empty mutable dictionary. A filtering failure answers NULL after logging its error, and
// the empty dictionary is then the answer. The URL used is stored retained in *infoPlistURL when both are
// set. The platform URL is released before the plain one.
static OCFMutableDictionaryRef _CFBundleCopyInfoDictionaryInDirectoryWithVersion(
    OCFAllocatorRef allocator,
    OCFURLRef url,
    OCFURLRef *infoPlistURL,
    OCFSetRef keyPaths,
    uint8_t unusedOption,
    uint8_t version
) {
    (void)unusedOption;
    if (url == NULL) {
        return NULL;
    }
    OCFURLRef directoryURL = NULL;
    OCFStringRef platformInfoName = NULL;
    OCFStringRef infoName = NULL;
    OCFAllocatorRef urlAllocator = kOCFAllocatorSystemDefault;
    switch (version) {
        case 0:
            directoryURL = OCFURLCreateWithString(urlAllocator, OF_STR("Resources/"), url);
            platformInfoName = OF_STR("Resources/Info-iphoneos.plist");
            infoName = OF_STR("Resources/Info.plist");
            break;
        case 1:
            directoryURL = OCFURLCreateWithString(urlAllocator, OF_STR("Support%20Files/"), url);
            platformInfoName = OF_STR("Support%20Files/Info-iphoneos.plist");
            infoName = OF_STR("Support%20Files/Info.plist");
            break;
        case 2:
            directoryURL = OCFURLCreateWithString(urlAllocator, OF_STR("Contents/"), url);
            platformInfoName = OF_STR("Contents/Info-iphoneos.plist");
            infoName = OF_STR("Contents/Info.plist");
            break;
        case 3: {
            OCFStringRef path = OCFURLCopyFileSystemPath(url, kOCFURLPOSIXPathStyle);
            if (path == NULL) {
                break;
            }
            if (OCFStringHasSuffix(path, OF_STR("Support Files")) ||
                OCFStringHasSuffix(path, OF_STR("Contents")) ||
                OCFStringHasSuffix(path, OF_STR("Resources"))) {
                directoryURL = NULL;
                platformInfoName = OF_STR("Resources/Info-iphoneos.plist");
                infoName = OF_STR("Resources/Info.plist");
            } else {
                directoryURL = OCFRetain(url);
                platformInfoName = OF_STR("Info-iphoneos.plist");
                infoName = OF_STR("Info.plist");
            }
            OCFRelease(path);
            break;
        }
        case 12:
            directoryURL = OCFURLCreateWithString(urlAllocator, OF_STR("WrappedBundle/Contents/"), url);
            platformInfoName = OF_STR("WrappedBundle/Contents/Info-iphoneos.plist");
            infoName = OF_STR("WrappedBundle/Contents/Info.plist");
            break;
        case 13:
            directoryURL = OCFURLCreateWithString(urlAllocator, OF_STR("WrappedBundle/"), url);
            platformInfoName = OF_STR("WrappedBundle/Info-iphoneos.plist");
            infoName = OF_STR("WrappedBundle/Info.plist");
            break;
        default:
            break;
    }
    if (directoryURL == NULL) {
        return OCFDictionaryCreateMutable(
            allocator, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
    }
    OCFURLRef absoluteURL = OCFURLCopyAbsoluteURL(directoryURL);
    OCFStringRef directoryPath = OCFURLCopyFileSystemPath(absoluteURL, kOCFURLPOSIXPathStyle);
    OCFRelease(absoluteURL);
    __block OCFURLRef infoURL = NULL;
    __block OCFURLRef platformInfoURL = NULL;
    if (directoryPath != NULL) {
        OCFIndex infoLength = OCFStringGetLength(OF_STR("Info.plist"));
        OCFIndex platformInfoLength = OCFStringGetLength(OF_STR("Info-iphoneos.plist"));
        _CFIterateDirectory(
            directoryPath,
            false,
            NULL,
            ^Boolean(OCFStringRef fileName, OCFStringRef fileNameWithPrefix, uint8_t fileType) {
              (void)fileNameWithPrefix;
              (void)fileType;
              if (platformInfoURL == NULL) {
                  OCFIndex length = OCFStringGetLength(fileName);
                  if (length == platformInfoLength && OCFStringCompareWithOptions(
                                                          fileName,
                                                          OF_STR("Info-iphoneos.plist"),
                                                          (OCFRange){0, length},
                                                          kOCFCompareCaseInsensitive | kOCFCompareAnchored
                                                      ) == kOCFCompareEqualTo) {
                      platformInfoURL =
                          OCFURLCreateWithString(kOCFAllocatorSystemDefault, platformInfoName, url);
                  }
              }
              if (infoURL == NULL) {
                  OCFIndex length = OCFStringGetLength(fileName);
                  if (length == infoLength && OCFStringCompareWithOptions(
                                                  fileName,
                                                  OF_STR("Info.plist"),
                                                  (OCFRange){0, length},
                                                  kOCFCompareCaseInsensitive | kOCFCompareAnchored
                                              ) == kOCFCompareEqualTo) {
                      infoURL = OCFURLCreateWithString(kOCFAllocatorSystemDefault, infoName, url);
                  }
              }
              return infoURL == NULL || platformInfoURL == NULL;
            }
        );
        OCFRelease(directoryPath);
    }
    OCFRelease(directoryURL);
    OCFURLRef usedURL = NULL;
    OCFMutableDictionaryRef result = NULL;
    Boolean loadFull;
    if (keyPaths != NULL) {
        OCFURLRef candidateURLs[2] = {platformInfoURL, infoURL};
        OCFDataRef data = NULL;
        if (candidateURLs[0] != NULL) {
            Boolean mapped = false;
            OCFErrorRef error = NULL;
            data = _CFBundleCreateMappedOrLoadedPlistData(candidateURLs[0], true, false, &mapped, &error);
            if (data != NULL) {
                usedURL = candidateURLs[0];
            } else if (error != NULL) {
                _CFBundleLogLoadingInfoPlistError(&candidateURLs[0], error);
                OCFRelease(error);
            }
        }
        if (candidateURLs[1] != NULL && data == NULL) {
            Boolean mapped = false;
            OCFErrorRef error = NULL;
            data = _CFBundleCreateMappedOrLoadedPlistData(candidateURLs[1], true, false, &mapped, &error);
            if (data != NULL) {
                usedURL = candidateURLs[1];
            } else if (error != NULL) {
                _CFBundleLogLoadingInfoPlistError(&candidateURLs[1], error);
                OCFRelease(error);
            }
        }
        loadFull = data == NULL;
        if (data != NULL) {
            OCFErrorRef error = NULL;
            OCFAllocatorRef keysAllocator = kOCFAllocatorSystemDefault;
            OCFSetRef topLevelKeys = _OCFPropertyListCopyTopLevelKeys(keysAllocator, data, 0, &error);
            if (topLevelKeys != NULL) {
                OCFMutableDictionaryRef keyMap =
                    OCFDictionaryCreateMutable(keysAllocator, OCFSetGetCount(topLevelKeys), NULL, NULL);
                CFSetApply(topLevelKeys, ^(const void *value, Boolean *stop) {
                  (void)stop;
                  OCFDictionaryAddValue(keyMap, value, value);
                });
                _CFBundleInfoPlistProcessInfoDictionary(keyMap);
                OCFSetRef topLevelKeyPaths = __CFPropertyListCreateTopLevelKeyPaths(keysAllocator, keyPaths);
                OCFMutableSetRef physicalKeyPaths =
                    OCFSetCreateMutable(keysAllocator, OCFSetGetCount(keyPaths), &kOCFTypeSetCallBacks);
                OCFMutableSetRef logicalKeys =
                    OCFSetCreateMutable(keysAllocator, OCFSetGetCount(keyPaths), &kOCFTypeSetCallBacks);
                CFSetApply(topLevelKeyPaths, ^(const void *value, Boolean *stop) {
                  (void)stop;
                  OCFArrayRef keyPath = (OCFArrayRef)value;
                  if (OCFArrayGetCount(keyPath) < 1) {
                      return;
                  }
                  OCFTypeRef logicalKey = OCFArrayGetValueAtIndex(keyPath, 0);
                  OCFTypeRef physicalKey = OCFDictionaryGetValue(keyMap, logicalKey);
                  if (physicalKey != NULL && !OCFEqual(logicalKey, physicalKey)) {
                      OCFMutableArrayRef mappedPath =
                          OCFArrayCreateMutableCopy(kOCFAllocatorSystemDefault, 0, keyPath);
                      OCFArraySetValueAtIndex(mappedPath, 0, physicalKey);
                      OCFSetAddValue(physicalKeyPaths, mappedPath);
                      OCFRelease(mappedPath);
                  } else {
                      OCFSetAddValue(physicalKeyPaths, keyPath);
                  }
                  OCFSetAddValue(logicalKeys, logicalKey);
                });
                OCFRelease(topLevelKeyPaths);
                OCFPropertyListRef filtered = NULL;
                if (_OCFPropertyListCreateFiltered(
                        keysAllocator, data, 0, physicalKeyPaths, &filtered, &error
                    )) {
                    OCFDictionaryRef processedValues = filtered;
                    OCFMutableDictionaryRef logicalValues = OCFDictionaryCreateMutable(
                        allocator,
                        OCFSetGetCount(logicalKeys),
                        &kOCFTypeDictionaryKeyCallBacks,
                        &kOCFTypeDictionaryValueCallBacks
                    );
                    CFSetApply(logicalKeys, ^(const void *value, Boolean *stop) {
                      (void)stop;
                      OCFTypeRef physicalKey = OCFDictionaryGetValue(keyMap, value);
                      if (physicalKey == NULL) {
                          return;
                      }
                      OCFTypeRef processed = OCFDictionaryGetValue(processedValues, physicalKey);
                      if (processed != NULL) {
                          OCFDictionarySetValue(logicalValues, value, processed);
                      }
                    });
                    result = logicalValues;
                    OCFRelease(filtered);
                }
                OCFRelease(physicalKeyPaths);
                OCFRelease(topLevelKeys);
                OCFRelease(keyMap);
                OCFRelease(logicalKeys);
            }
            if (result == NULL && error != NULL) {
                _CFBundleLogLoadingInfoPlistError(&usedURL, error);
                OCFRelease(error);
            }
            OCFRelease(data);
        }
    } else {
        loadFull = true;
    }
    if (loadFull && result == NULL) {
        OCFDataRef data = NULL;
        OCFURLRef sourceURL = NULL;
        if (platformInfoURL != NULL) {
            data = _CFDataCreateFromURL(platformInfoURL, NULL);
            if (data != NULL) {
                sourceURL = platformInfoURL;
            }
        }
        if (data == NULL && infoURL != NULL) {
            data = _CFDataCreateFromURL(infoURL, NULL);
            if (data != NULL) {
                sourceURL = infoURL;
            }
        }
        if (data != NULL) {
            usedURL = sourceURL;
            OCFErrorRef error = NULL;
            result = (OCFMutableDictionaryRef)OCFPropertyListCreateWithData(
                allocator, data, kOCFPropertyListMutableContainers, NULL, &error
            );
            if (result != NULL && OCFDictionaryGetTypeID() == OCFGetTypeID(result)) {
                _CFBundleInfoPlistProcessInfoDictionary(result);
            } else {
                if (result != NULL) {
                    OCFRelease(result);
                } else if (error != NULL) {
                    OCFErrorDomain domain = OCFErrorGetDomain(error);
                    OCFIndex code = OCFErrorGetCode(error);
                    os_log_t logger = _CFBundleResourceLogger();
                    os_log_error(
                        logger,
                        "There was an error parsing the Info.plist for the bundle at URL <%{public}p>: "
                        "%{public}@ - "
                        "%{public}ld",
                        usedURL,
                        domain,
                        (long)code
                    );
                    OCFRelease(error);
                }
                result = OCFDictionaryCreateMutable(
                    allocator, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
                );
            }
            OCFRelease(data);
        }
    }
    if (infoPlistURL != NULL && usedURL != NULL) {
        OCFRetain(usedURL);
        *infoPlistURL = usedURL;
    }
    if (platformInfoURL != NULL) {
        OCFRelease(platformInfoURL);
    }
    if (infoURL != NULL) {
        OCFRelease(infoURL);
    }
    if (result == NULL) {
        return OCFDictionaryCreateMutable(
            allocator, 0, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
        );
    }
    return result;
}

// Native _CFBundleCopyInfoDictionaryInDirectory, 0x1c02fca78. Hidden (was private external). The URL's
// file-system representation (resolved, 1,026 bytes) becomes a directory URL (system default allocator; the
// URL itself, retained, when that fails); its bundle version selects the dictionary (no Info.plist URL, no
// key paths). A representation failure answers NULL with version 0; the version is stored when the pointer is
// set.
OCFDictionaryRef
_CFBundleCopyInfoDictionaryInDirectory(OCFAllocatorRef allocator, OCFURLRef url, uint8_t *version) {
    UInt8 buffer[1026];
    OCFDictionaryRef result = NULL;
    uint8_t localVersion = 0;
    if (OCFURLGetFileSystemRepresentation(url, true, buffer, 1026)) {
        OCFURLRef directoryURL = OCFURLCreateFromFileSystemRepresentation(
            kOCFAllocatorSystemDefault, buffer, (OCFIndex)strlen((const char *)buffer), true
        );
        if (directoryURL == NULL) {
            directoryURL = OCFRetain(url);
        }
        localVersion = _CFBundleGetBundleVersionForURL(directoryURL);
        result = _CFBundleCopyInfoDictionaryInDirectoryWithVersion(
            allocator, directoryURL, NULL, NULL, 0, localVersion
        );
        OCFRelease(directoryURL);
    }
    if (version != NULL) {
        *version = localVersion;
    }
    return result;
}

// Native _CFBundleCreateInfoPlistWithData, 0x1c02fcd18. Hidden (was private external). Sends
// +[_NSSwiftBundle infoDictionaryFromData:] (the class through objc_lookUpClass and objc_opt_class) and
// retains a non-NULL answer.
OCFMutableDictionaryRef _CFBundleCreateInfoPlistWithData(OCFDataRef data) {
    Class swiftBundleClass = objc_opt_class((id)objc_lookUpClass(OF_NAME_STRING(_ONSSwiftBundle)));
    OCFMutableDictionaryRef result = ((OCFMutableDictionaryRef (*)(id, SEL, OCFDataRef))objc_msgSend)(
        (id)swiftBundleClass, sel_getUid("infoDictionaryFromData:"), data
    );
    if (result != NULL) {
        OCFRetain(result);
    }
    return result;
}

// Native CFBundleGetInfoDictionary, 0x1c02fd4a8. External. The entry-point pattern (line 1340). With
// CFSwiftBundle the answer of -infoDictionary (not retained); otherwise the bundle must have type 31
// (_CFAssertMismatchedTypeID otherwise), the dictionary is refreshed under the lock at +104
// (os_unfair_lock_lock) and +24 is answered after the unlock.
OCFDictionaryRef OCFBundleGetInfoDictionary(OCFBundleRef bundle) {
    void *nested = _OCFGetTSD(kBundleEntryTSDSlot);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(kBundleEntryTSDSlot, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(outermost, kSourcePath, 1340, OF_NAME_STRING(OCFBundleGetInfoDictionary));
    OCFDictionaryRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = ((OCFDictionaryRef (*)(id, SEL))objc_msgSend)((id)bundle, sel_getUid("infoDictionary"));
    } else {
        OCFTypeID typeID = _CFGetNonObjCTypeID(bundle);
        if (typeID != 31) {
            _CFAssertMismatchedTypeID(31, typeID);
        }
        os_unfair_lock_lock(&bundle->_lock104);
        _CFBundleRefreshInfoDictionaryAlreadyLocked(bundle);
        os_unfair_lock_unlock(&bundle->_lock104);
        result = bundle->_infoDict;
    }
    if (nested != (void *)1) {
        _OCFSetTSD(kBundleEntryTSDSlot, NULL, NULL);
    }
    return result;
}

// Native _CFBundleRefreshInfoDictionaryAlreadyLocked, 0x1c02fd6f4. Hidden (was private external). Nothing
// happens when +24 is set. Otherwise the dictionary for the bundle URL (+16) with its creation option (+55)
// and version (+53) is stored at +24, the previous Info.plist URL (+208) is released and replaced. The
// CFBundleNumericVersion value, or else CFBundleVersion, when a string becomes the 32-bit number of
// _CFVersionNumberFromString (kCFNumberSInt32Type, the bundle's allocator) under CFBundleNumericVersion; a
// value that is neither a string nor a number removes CFBundleNumericVersion.
void _CFBundleRefreshInfoDictionaryAlreadyLocked(OCFBundleRef bundle) {
    if (bundle->_infoDict != NULL) {
        return;
    }
    OCFURLRef infoPlistURL = NULL;
    bundle->_infoDict = _CFBundleCopyInfoDictionaryInDirectoryWithVersion(
        kOCFAllocatorSystemDefault,
        bundle->_url,
        &infoPlistURL,
        NULL,
        bundle->_createOption1,
        bundle->_version
    );
    if (bundle->_infoPlistURL != NULL) {
        OCFRelease(bundle->_infoPlistURL);
    }
    bundle->_infoPlistURL = infoPlistURL;
    OCFMutableDictionaryRef infoDict = (OCFMutableDictionaryRef)bundle->_infoDict;
    if (infoDict == NULL) {
        return;
    }
    OCFStringRef numericKey = _kOCFBundleNumericVersionKey;
    OCFTypeRef value = OCFDictionaryGetValue(infoDict, numericKey);
    if (value == NULL) {
        value = OCFDictionaryGetValue(infoDict, kOCFBundleVersionKey);
        if (value == NULL) {
            return;
        }
    }
    if (OCFGetTypeID(value) == OCFStringGetTypeID()) {
        UInt32 versionNumber = _OCFVersionNumberFromString((OCFStringRef)value);
        OCFNumberRef number = OCFNumberCreate(OCFGetAllocator(bundle), kOCFNumberSInt32Type, &versionNumber);
        OCFDictionarySetValue(infoDict, numericKey, number);
        OCFRelease(number);
    } else if (OCFGetTypeID(value) != OCFNumberGetTypeID()) {
        OCFDictionaryRemoveValue(infoDict, numericKey);
    }
}

static OCFMutableDictionaryRef _parseBundleStrings(OCFBundleRef bundle, OCFDictionaryRef dictionary);
static void _CFBundleReleaseStringsSources_0(StringsSources sources);

// Native CFBundleGetLocalInfoDictionary, 0x1c02fd860. External. The entry-point pattern (line 1382). With
// CFSwiftBundle the answer of -localizedInfoDictionary; otherwise (type 31 asserted) the cached dictionary
// at +32 is answered under the lock at +104. Without one the lock is dropped and the InfoPlist strings
// sources are found (mapping allowed, no localizations): strings data is parsed
// (CFPropertyListCreateWithData, mutable containers, system default) and kept only as a dictionary; otherwise
// loctable data with a strings language is filtered to that language (_CFPropertyListCreateFiltered, a
// one-language key-path set) and the language's value kept, retained, only as a dictionary. A dictionary is
// processed
// (_CFBundleInfoPlistProcessInfoDictionary) and its strings parsed by _parseBundleStrings, whose result
// replaces it. The sources are released, and under the lock again a dictionary cached meanwhile wins (the new
// one is released); otherwise the new one is cached. The answer is read before the unlock and not retained.
OCFDictionaryRef OCFBundleGetLocalInfoDictionary(OCFBundleRef bundle) {
    void *nested = _OCFGetTSD(kBundleEntryTSDSlot);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(kBundleEntryTSDSlot, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(outermost, kSourcePath, 1382, OF_NAME_STRING(OCFBundleGetLocalInfoDictionary));
    OCFDictionaryRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result =
            ((OCFDictionaryRef (*)(id, SEL))objc_msgSend)((id)bundle, sel_getUid("localizedInfoDictionary"));
    } else {
        OCFTypeID typeID = _CFGetNonObjCTypeID(bundle);
        if (typeID != 31) {
            _CFAssertMismatchedTypeID(31, typeID);
        }
        os_unfair_lock_lock_with_options(&bundle->_lock104, 0x10000);
        result = bundle->_localInfoDict;
        if (result == NULL) {
            os_unfair_lock_unlock(&bundle->_lock104);
            StringsSources sources = _CFBundleGetStringsSources(bundle, true, OF_STR("InfoPlist"), NULL);
            OCFMutableDictionaryRef dictionary = NULL;
            Boolean found = false;
            if (sources.stringsData != NULL) {
                OCFPropertyListRef plist = OCFPropertyListCreateWithData(
                    kOCFAllocatorSystemDefault,
                    sources.stringsData,
                    kOCFPropertyListMutableContainers,
                    NULL,
                    NULL
                );
                if (plist != NULL) {
                    if (OCFDictionaryGetTypeID() == OCFGetTypeID(plist)) {
                        dictionary = (OCFMutableDictionaryRef)plist;
                        found = true;
                    } else {
                        OCFRelease(plist);
                    }
                }
            } else if (sources.locTableData != NULL && sources.stringsLanguage != NULL) {
                OCFSetRef keyPaths = OCFSetCreate(
                    kOCFAllocatorSystemDefault,
                    (const void **)&sources.stringsLanguage,
                    1,
                    &kOCFTypeSetCallBacks
                );
                OCFPropertyListRef filtered = NULL;
                OCFPropertyListRef value = NULL;
                if (_OCFPropertyListCreateFiltered(
                        kOCFAllocatorSystemDefault, sources.locTableData, 1, keyPaths, &filtered, NULL
                    )) {
                    value = _OCFPropertyListGetValueWithKeyPath(filtered, sources.stringsLanguage);
                    if (value != NULL) {
                        if (OCFDictionaryGetTypeID() == OCFGetTypeID(value)) {
                            OCFRetain(value);
                        } else {
                            value = NULL;
                        }
                    }
                }
                if (filtered != NULL) {
                    OCFRelease(filtered);
                }
                OCFRelease(keyPaths);
                if (value != NULL) {
                    dictionary = (OCFMutableDictionaryRef)value;
                    found = true;
                }
            }
            OCFDictionaryRef local = NULL;
            if (found) {
                _CFBundleInfoPlistProcessInfoDictionary(dictionary);
                local = _parseBundleStrings(bundle, dictionary);
                if (local != NULL) {
                    OCFRelease(dictionary);
                } else {
                    local = dictionary;
                }
            }
            _CFBundleReleaseStringsSources_0(sources);
            os_unfair_lock_lock_with_options(&bundle->_lock104, 0x10000);
            if (bundle->_localInfoDict != NULL) {
                if (local != NULL) {
                    OCFRelease(local);
                }
                local = bundle->_localInfoDict;
            } else {
                bundle->_localInfoDict = local;
            }
            result = local;
        }
        os_unfair_lock_unlock(&bundle->_lock104);
    }
    if (nested != (void *)1) {
        _OCFSetTSD(kBundleEntryTSDSlot, NULL, NULL);
    }
    return result;
}

// Native _parseBundleStrings, 0x1c02fdb9c, with ____parseBundleStrings_block_invoke (0x1c02feb44) and its
// .cold.1 (0x1c031753c, the error log). Non-external. A mutable copy (system default, CFType callbacks, the
// dictionary's count as capacity) receives every pair through CFDictionaryApply: a string value is replaced
// by _CFStringCreateByParsingMarkdownAndInflectingIfNeeded (no localization), or kept and an error logged
// on the resource logger when that answers NULL; other values are kept.
static OCFMutableDictionaryRef _parseBundleStrings(OCFBundleRef bundle, OCFDictionaryRef dictionary) {
    OCFMutableDictionaryRef result = OCFDictionaryCreateMutable(
        kOCFAllocatorSystemDefault,
        OCFDictionaryGetCount(dictionary),
        &kOCFTypeDictionaryKeyCallBacks,
        &kOCFTypeDictionaryValueCallBacks
    );
    CFDictionaryApply(dictionary, ^(const void *key, const void *value, Boolean *stop) {
      (void)stop;
      if (OCFGetTypeID(value) == OCFStringGetTypeID()) {
          OCFStringRef parsed = _OCFStringCreateByParsingMarkdownAndInflectingIfNeeded(value, bundle, NULL);
          if (parsed != NULL) {
              OCFDictionaryAddValue(result, key, parsed);
              OCFRelease(parsed);
          } else {
              OCFDictionaryAddValue(result, key, value);
              os_log_t log = _CFBundleResourceLogger();
              if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
                  os_log_error(
                      log,
                      "Could not parse markdown while retrieving localized string for key %{public}@ in "
                      "bundle "
                      "%{public}@",
                      key,
                      bundle
                  );
              }
          }
      } else {
          OCFDictionaryAddValue(result, key, value);
      }
    });
    return result;
}

// Native _CFBundleReleaseStringsSources, 0x1c02fdc6c (the copy of this unit; the strings unit has its own at
// 0x1c02c2a6c). Non-external. Releases the eight object fields in order, skipping NULL ones.
static void _CFBundleReleaseStringsSources_0(StringsSources sources) {
    if (sources.stringsURL != NULL) {
        OCFRelease(sources.stringsURL);
    }
    if (sources.stringsLanguage != NULL) {
        OCFRelease(sources.stringsLanguage);
    }
    if (sources.stringsData != NULL) {
        OCFRelease(sources.stringsData);
    }
    if (sources.stringsDictURL != NULL) {
        OCFRelease(sources.stringsDictURL);
    }
    if (sources.stringsDictLanguage != NULL) {
        OCFRelease(sources.stringsDictLanguage);
    }
    if (sources.stringsDictData != NULL) {
        OCFRelease(sources.stringsDictData);
    }
    if (sources.locTableURL != NULL) {
        OCFRelease(sources.locTableURL);
    }
    if (sources.locTableData != NULL) {
        OCFRelease(sources.locTableData);
    }
}

// Native CFBundleGetValueForInfoDictionaryKey, 0x1c02fdcf4. External. The entry-point pattern (line 1451).
// With CFSwiftBundle the answer of -objectForInfoDictionaryKey:; otherwise, with a bundle and a key, the
// local info dictionary's value, else the info dictionary's; not retained.
OCFTypeRef OCFBundleGetValueForInfoDictionaryKey(OCFBundleRef bundle, OCFStringRef key) {
    void *nested = _OCFGetTSD(kBundleEntryTSDSlot);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(kBundleEntryTSDSlot, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(
        outermost, kSourcePath, 1451, OF_NAME_STRING(OCFBundleGetValueForInfoDictionaryKey)
    );
    OCFTypeRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = ((OCFTypeRef (*)(id, SEL, OCFStringRef))objc_msgSend)(
            (id)bundle, sel_getUid("objectForInfoDictionaryKey:"), key
        );
    } else {
        result = NULL;
        if (bundle != NULL && key != NULL) {
            OCFDictionaryRef local = OCFBundleGetLocalInfoDictionary(bundle);
            if (local == NULL || (result = OCFDictionaryGetValue(local, key)) == NULL) {
                OCFDictionaryRef info = OCFBundleGetInfoDictionary(bundle);
                result = info != NULL ? OCFDictionaryGetValue(info, key) : NULL;
            }
        }
    }
    if (nested != (void *)1) {
        _OCFSetTSD(kBundleEntryTSDSlot, NULL, NULL);
    }
    return result;
}

// Native CFBundleGetIdentifier, 0x1c02fde20. External. The entry-point pattern (line 1470). With
// CFSwiftBundle the answer of -bundleIdentifier (not retained); otherwise the Info.plist CFBundleIdentifier
// value when its type ID is 7 (a string), else NULL.
OCFStringRef OCFBundleGetIdentifier(OCFBundleRef bundle) {
    void *nested = _OCFGetTSD(kBundleEntryTSDSlot);
    Boolean outermost;
    if (nested == (void *)1) {
        outermost = false;
    } else {
        outermost = true;
        _OCFSetTSD(kBundleEntryTSDSlot, (void *)1, NULL);
    }
    _CFBundleLogSourceLocation(outermost, kSourcePath, 1470, OF_NAME_STRING(OCFBundleGetIdentifier));
    OCFStringRef result;
    if (_os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle")) {
        result = ((OCFStringRef (*)(id, SEL))objc_msgSend)((id)bundle, sel_getUid("bundleIdentifier"));
    } else {
        OCFDictionaryRef infoDict = OCFBundleGetInfoDictionary(bundle);
        result = NULL;
        if (infoDict != NULL) {
            result = OCFDictionaryGetValue(infoDict, kOCFBundleIdentifierKey);
            if (result != NULL && OCFGetTypeID(result) != 7) {
                result = NULL;
            }
        }
    }
    if (nested != (void *)1) {
        _OCFSetTSD(kBundleEntryTSDSlot, NULL, NULL);
    }
    return result;
}

// Native _CFBundleLogLoadingInfoPlistError, 0x1c02fe968. Non-external. The error's domain and code are read
// before the resource logger; the error-level message shows the first argument, the address of the caller's
// URL variable, as a pointer.
static void _CFBundleLogLoadingInfoPlistError(OCFURLRef *url, OCFErrorRef error) {
    OCFErrorDomain domain = OCFErrorGetDomain(error);
    OCFIndex code = OCFErrorGetCode(error);
    os_log_t logger = _CFBundleResourceLogger();
    os_log_error(
        logger,
        "There was an error parsing the Info.plist for the bundle at URL <%{public}p>: %{public}@ - "
        "%{public}ld",
        url,
        domain,
        (long)code
    );
}

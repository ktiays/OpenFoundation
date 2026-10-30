//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include <OpenFoundation/OpenFoundation.h>

#include "CoreFoundation/Base/OCFBaseStandaloneTests.h"
#include "CoreFoundation/Date/ScalarStandaloneTests.h"
#include "Foundation/Range/RangeStandaloneTests.h"
#include "Foundation/Decimal/DecimalStandaloneTests.h"
#include "CoreFoundation/StringEncoding/EncodingNameStandaloneTests.h"
#include "CoreFoundation/StringEncoding/KernelStandaloneTests.h"
#include "CoreFoundation/Number/BigNumStandaloneTests.h"
#include "CoreFoundation/Locale/LocaleIdentifierStandaloneTests.h"
#include "CoreFoundation/Unicode/UnicodeStandaloneTests.h"
#include "CoreFoundation/PlugIn/BundleGrokStandaloneTests.h"
#include "CoreFoundation/String/StringHelperStandaloneTests.h"
#include "CoreFoundation/PropertyList/PropertyListHelperStandaloneTests.h"
#include "CoreFoundation/Base/FileUtilitiesStandaloneTests.h"
#include "CoreFoundation/PlugIn/BundleHelperStandaloneTests.h"
#include "CoreFoundation/URL/URLHelperStandaloneTests.h"
#include "CoreFoundation/CharacterSet/CharacterSetHelperStandaloneTests.h"

#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    for (uint32_t index = 0; index < _dyld_image_count(); ++index) {
        const char *path = _dyld_get_image_name(index);
        if (strstr(path, "/System/Library/Frameworks/Foundation.framework/") ||
            strstr(path, "/System/Library/Frameworks/CoreFoundation.framework/")) {
            fprintf(stderr, "Standalone isolation failure: loaded %s\n", path);
            return 1;
        }
    }
    puts("Standalone framework load passed without system Foundation or CoreFoundation.");
    int status = runCoreFoundationBaseStandaloneTests();
    if (status != 0) {
        return status;
    }
    status = runCoreFoundationScalarStandaloneTests();
    if (status != 0) {
        return status;
    }
    status = runFoundationRangeStandaloneTests();
    if (status != 0) {
        return status;
    }
    status = runStringEncodingKernelStandaloneTests();
    if (status != 0) {
        return status;
    }
    status = runCoreFoundationUnicodeStandaloneTests();
    if (status != 0) {
        return status;
    }
    status = runCoreFoundationUnicodePlaneStandaloneTests();
    if (status != 0) {
        return status;
    }
    status = runCoreFoundationBigNumStandaloneTests();
    if (status != 0) {
        return status;
    }
    status = runStringEncodingNameStandaloneTests();
    if (status != 0) {
        return status;
    }
    status = runCoreFoundationUnicodeCaseStandaloneTests();
    if (status != 0) {
        return status;
    }
    status = runCoreFoundationLocaleIdentifierStandaloneTests();
    if (status != 0) {
        return status;
    }
    status = runCoreFoundationBundleGrokStandaloneTests();
    if (status != 0) {
        return status;
    }
    status = runCoreFoundationStringHelperStandaloneTests();
    if (status != 0) {
        return status;
    }
    status = runCoreFoundationPropertyListHelperStandaloneTests();
    if (status != 0) {
        return status;
    }
    status = runCoreFoundationFileUtilitiesStandaloneTests();
    if (status != 0) {
        return status;
    }
    status = runCoreFoundationBundleHelperStandaloneTests();
    if (status != 0) {
        return status;
    }
    status = runCoreFoundationURLHelperStandaloneTests();
    if (status != 0) {
        return status;
    }
    status = runCoreFoundationCharacterSetHelperStandaloneTests();
    if (status != 0) {
        return status;
    }
    status = runFoundationDecimalStandaloneTests();
    if (status != 0) {
        return status;
    }
    puts("All registered standalone suites passed; coverage is limited to their finite cases.");
    return 0;
}

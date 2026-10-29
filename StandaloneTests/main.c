//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include <OpenFoundation/OpenFoundation.h>

#include "CoreFoundation/Base/OCFBaseStandaloneTests.h"
#include "CoreFoundation/Date/ScalarStandaloneTests.h"
#include "Foundation/Range/RangeStandaloneTests.h"
#include "Foundation/Decimal/DecimalStandaloneTests.h"
#include "CoreFoundation/StringEncoding/KernelStandaloneTests.h"
#include "CoreFoundation/Unicode/UnicodeStandaloneTests.h"

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
    status = runFoundationDecimalStandaloneTests();
    if (status != 0) {
        return status;
    }
    puts("All registered standalone suites passed; coverage is limited to their finite cases.");
    return 0;
}

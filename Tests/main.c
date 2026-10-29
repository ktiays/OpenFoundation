//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include <OpenFoundation/OpenFoundation.h>

#include "CoreFoundation/Base/OCFBaseTests.h"
#include "CoreFoundation/Date/ScalarTests.h"
#include "Foundation/Range/RangeDifferentialTests.h"
#include "CoreFoundation/StringEncoding/EncodingNameTests.h"
#include "CoreFoundation/StringEncoding/KernelTests.h"
#include "CoreFoundation/Number/BigNumTests.h"
#include "CoreFoundation/Locale/LocaleIdentifierTests.h"
#include "CoreFoundation/Unicode/UnicodeTests.h"
#include "CoreFoundation/PlugIn/BundleGrokTests.h"
#include "Foundation/Decimal/DecimalDifferentialTests.h"
#include "ReferencePins.h"
#include "Support/NativeReference.h"

#include <stdio.h>
#include <string.h>

int main(int argc, const char *argv[]) {
    int capture = argc == 2 && strcmp(argv[1], "--capture-reference") == 0;
    int status = checkNativeReference("CoreFoundation", OF_REFERENCE_CF_UUID, capture);
    if (status != 0) {
        return status;
    }
    status = checkNativeReference("Foundation", OF_REFERENCE_NS_UUID, capture);
    if (status != 0) {
        return status;
    }
    if (!capture) {
        status = runCoreFoundationBaseDifferentialTests();
        if (status != 0) {
            return status;
        }
        status = runCoreFoundationScalarDifferentialTests();
        if (status != 0) {
            return status;
        }
        status = runFoundationRangeDifferentialTests();
        if (status != 0) {
            return status;
        }
        status = runStringEncodingKernelDifferentialTests();
        if (status != 0) {
            return status;
        }
        status = runStringEncodingNameDifferentialTests();
        if (status != 0) {
            return status;
        }
        status = runCoreFoundationUnicodeDifferentialTests();
        if (status != 0) {
            return status;
        }
        status = runCoreFoundationUnicodePlaneDifferentialTests();
        if (status != 0) {
            return status;
        }
        status = runCoreFoundationBigNumDifferentialTests();
        if (status != 0) {
            return status;
        }
        status = runCoreFoundationUnicodeCaseDifferentialTests();
        if (status != 0) {
            return status;
        }
        status = runCoreFoundationLocaleIdentifierDifferentialTests();
        if (status != 0) {
            return status;
        }
        status = runCoreFoundationBundleGrokDifferentialTests();
        if (status != 0) {
            return status;
        }
        status = runFoundationDecimalDifferentialTests();
        if (status != 0) {
            return status;
        }
        puts("All registered differential suites passed; coverage is limited to their finite cases.");
    }
    return 0;
}

//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include <OpenFoundation/OpenFoundation.h>

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
        puts("Reference image checks passed. Restored API comparisons: 0 (bootstrap only).");
    }
    return 0;
}

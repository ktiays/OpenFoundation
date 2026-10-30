//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Runtime/RuntimeTraces.h"
#include "Support/Trace.h"

#include <stdio.h>

int main(void) {
    int status = checkTraceProcess();
    if (status != 0) {
        return status;
    }
    runRuntimeTraces();
    fflush(stdout);
    if (traceSetupFailed()) {
        fputs("Setup failure: a trace identity table overflowed.\n", stderr);
        return 77;
    }
    return 0;
}

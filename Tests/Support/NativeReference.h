//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <stdint.h>

int checkNativeReference(const char *framework, const char *expectedUUID, int capture);

int resolveReferenceFunction(
    const char *framework, const char *expectedUUID, const char *symbol, void **function
);

int resolveReplacementFunction(const char *framework, const char *symbol, void **function);

int resolveReferenceAddress(
    const char *framework,
    const char *expectedUUID,
    uintptr_t nativeAddress,
    uintptr_t nativeImageBase,
    void **function
);

int checkInternalImplementation(const void *function);

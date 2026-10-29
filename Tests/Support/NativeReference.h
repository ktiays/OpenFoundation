//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

int checkNativeReference(const char *framework, const char *expectedUUID, int capture);

int resolveReferenceFunction(
    const char *framework, const char *expectedUUID, const char *symbol, void **function
);

int resolveReplacementFunction(const char *framework, const char *symbol, void **function);

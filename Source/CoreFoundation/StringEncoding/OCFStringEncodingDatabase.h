//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdbool.h>
#include <stdint.h>

// Original hidden names are preserved; no process-shared symbol is exported.
OF_EXTERN uint32_t __CFStringEncodingGetWindowsCodePage(uint32_t encoding);

OF_EXTERN bool __CFStringEncodingGetCanonicalName(uint32_t encoding, char *buffer, OCFIndex bufferSize);

OF_EXTERN const char *__CFStringEncodingGetICUName(uint32_t encoding);

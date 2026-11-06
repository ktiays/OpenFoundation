//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/URL/OCFURL.h"

// The URL promise support (URL/OCFURLPromises.c), iOS Simulator 24A434, arm64, CoreFoundation UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. iOS Device 24A446: unread.

// Native _kCFURLPromisePhysicalURLKey, 0x2227a9b08, external (no SDK declaration).
OF_EXPORT const OCFStringRef _kOCFURLPromisePhysicalURLKey;

// Native _CFURLPromiseCopyPhysicalURL, 0x1c0239724, external (no SDK declaration).
OF_EXPORT OCFURLRef _OCFURLPromiseCopyPhysicalURL(OCFURLRef url);

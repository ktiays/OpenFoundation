//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/String/OCFString.h"

// The pseudo-language string transforms (native externals; no SDK declaration). Signatures from the native
// register use: each takes the string in X0 (and _CFAffixedStringCreate two more strings in X1 and X2) and
// returns a new string the caller owns. The parameter names of _CFAffixedStringCreate are descriptive (the
// format places X1 first and X2 last; confidence: medium for the names, verified for the order).
OF_EXPORT OCFMutableStringRef _OCFDoubledStringCreate(OCFStringRef string);

OF_EXPORT OCFStringRef _OCFAffixedStringCreate(OCFStringRef string, OCFStringRef prefix, OCFStringRef suffix);

OF_EXPORT OCFStringRef _OCFRLORightToLeftStringCreate(OCFStringRef string);

OF_EXPORT OCFStringRef _OCFAccentuatedStringCreate(OCFStringRef string);

OF_EXPORT OCFStringRef _OCFTallStringCreate(OCFStringRef string);

//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/String/OCFString.h"

OF_EXPORT void _OCFGetPathExtensionRangesFromPathComponentUniChars(
    const UniChar *characters, OCFIndex length, OCFRange *primary, OCFRange *secondary
);

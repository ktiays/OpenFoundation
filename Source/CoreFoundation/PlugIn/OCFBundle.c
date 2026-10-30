//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PlugIn/OCFBundle.h"

#include <string.h>

// Native _CFBundleNormalizedPath, iOS Simulator 24A434, 0x1c016a39c,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external (was a private
// external). The two literals are the __cstring bytes at 0x1c047d8ee (31 bytes with NUL) and
// 0x1c047d90d (22 bytes with NUL), which precede the "PlugIn.subproj/CFBundle.c" source path.
// iOS Device 24A446: 0x18063e8d4, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// the same literals and lengths, strncmp through a stub, authenticated frame; the results agree.
// NULL is returned unchanged. A path that begins with the 30-byte "/private/preboot/Cryptexes/OS/" is
// advanced by 29 bytes, one that begins with the 21-byte "/System/Cryptexes/OS/" by 20, keeping the
// slash before the remainder; any other path is returned unchanged.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
const char *_CFBundleNormalizedPath(const char *path) {
    if (path == NULL) {
        return path;
    }
    if (strncmp(path, "/private/preboot/Cryptexes/OS/", 30) == 0) {
        return path + 29;
    }
    return path + (strncmp(path, "/System/Cryptexes/OS/", 21) == 0 ? 20 : 0);
}

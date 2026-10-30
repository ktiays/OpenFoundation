//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Base/OCFUtilities.h"

// Native _CFBuildVersionForCFSystemVersion, iOS Simulator 24A434, 0x1c0256b10,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External; its callers are
// _CFPlatformExecutableLinkedOnOrAfter (0x1c0256c7c) and _CFExecutableLinkedOnOrAfter (0x1c0256ca8, with
// dyld_get_active_platform()).
// iOS Device 24A446: 0x18060c1c8, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; the
// branches are laid out in another order and its one caller is _CFPlatformExecutableLinkedOnOrAfter;
// the results agree for every input.
// Platform 1 is macOS, 2 iOS and 6 Mac Catalyst (dyld platform numbers). Versions 0..4 map to version 0
// (Mac Catalyst becomes iOS). Versions 5..20 index macos_map for macOS and Mac Catalyst, and otherwise
// give 0 below 10 and 0xffff0000 from 10. Versions 1005..1015 index the second table for macOS only
// (its native bound of eleven entries makes the 0xffff0000 fallback after it unreachable); 1000..1004,
// or other platforms in 1005..1015, map to version 0 (Mac Catalyst becomes iOS). Any
// other version passes its low 32 bits through, except that Mac Catalyst maps its macOS version to an
// iOS version: below 10.15.1 to 13.0, 10.15.1 to 13.2, 10.16 and 11.0 to 14.0 and anything else to
// 0xffff0000. Mac Catalyst entries of macos_map below index 11 (10.5 through 10.15) all map to 13.0.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
OCFBuildVersion _OCFBuildVersionForCFSystemVersion(uint64_t systemVersion, uint32_t platform) {
    // Native _CFBuildVersionForCFSystemVersion.macos_map, iOS Simulator 24A434, 0x1c0456ae0; confidence:
    // verified; sixteen 32-bit entries. iOS Device 24A446: 0x1809651d8; the sixty-four bytes agree.
    static const uint32_t macos_map[16] = {
        0x0a0500,
        0x0a0600,
        0x0a0700,
        0x0a0800,
        0x0a0900,
        0x0a0a00,
        0x0a0b00,
        0x0a0c00,
        0x0a0d00,
        0x0a0e00,
        0x0a0f00,
        0x0a1000,
        0x0c0000,
        0x0d0000,
        0x0e0000,
        0x0f0000,
    };
    // Native unnamed table, iOS Simulator 24A434, 0x1c0456b28 (inside the span nm attributes to
    // _CFLog_os_trace_type_map, which starts eight bytes earlier); confidence: verified for the eleven
    // 32-bit entries the bound admits. iOS Device 24A446: 0x180965220; the forty-four bytes agree.
    // The name is a local descriptive spelling.
    static const uint32_t versionMap1005[11] = {
        0x0a0600,
        0x0a0600,
        0x0a0600,
        0x0a0600,
        0xffff0000,
        0x0a0700,
        0xffff0000,
        0x0a0700,
        0xffff0000,
        0xffff0000,
        0x0a0800,
    };
    uint32_t version = (uint32_t)systemVersion;
    if (systemVersion > 0x14) {
        if (systemVersion - 1000 > 0xf) {
            if (platform != 6) {
                return (OCFBuildVersion){platform, version};
            }
            if (version < 0xa0f01) {
                return (OCFBuildVersion){2, 0xd0000};
            }
        } else {
            if (systemVersion <= 0x3ec || platform != 1) {
                return (OCFBuildVersion){platform == 6 ? 2 : platform, 0};
            }
            uint64_t index = systemVersion - 0x3ed;
            if (index >= 0xb) {
                return (OCFBuildVersion){1, 0xffff0000};
            }
            return (OCFBuildVersion){1, versionMap1005[index]};
        }
    } else {
        if (systemVersion <= 4) {
            return (OCFBuildVersion){platform == 6 ? 2 : platform, 0};
        }
        uint32_t unmapped = systemVersion < 0xa ? 0 : 0xffff0000;
        if (platform != 6 && platform != 1) {
            return (OCFBuildVersion){platform, unmapped};
        }
        uint64_t index = systemVersion - 5;
        version = macos_map[index];
        if (platform != 6) {
            return (OCFBuildVersion){platform, version};
        }
        if (index < 0xb) {
            return (OCFBuildVersion){2, 0xd0000};
        }
    }
    if (version == 0xa0f01) {
        return (OCFBuildVersion){2, 0xd0200};
    }
    if (version == 0xb0000 || version == 0xa1000) {
        return (OCFBuildVersion){2, 0xe0000};
    }
    return (OCFBuildVersion){2, 0xffff0000};
}

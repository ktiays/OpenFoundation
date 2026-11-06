//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Base/ONSSwiftFeatureFlags.h"
#import "Foundation/String/ONSCharacterSet.h"
#include "CoreFoundation/URL/OCFURL.h"

#include <dispatch/dispatch.h>
#include <stdint.h>
#include <stdlib.h>

// Inferred organization: the checks lie together at 0x1c051a370-0x1c051a70f of the iOS Simulator Foundation
// __text section, each followed by its block invoke; the characterSet block invoke is the cold copy defined
// by String/ONSCharacterSet.m. Natively Objective-C (global blocks). Unless a definition says otherwise: iOS
// Simulator 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read
// in full). iOS Device 24A446: unread.

extern bool _os_feature_enabled_impl(const char *domain, const char *feature);

// Native __MergedGlobals_82 (0x22e76c3e0, the NSSwiftURL flag), byte_22E76C3E1 (NSSwiftURL2), byte_22E76C3E3
// (CFSwiftBundle) and byte_22E76C3E5 (ICUResourceTimeZone), with their once tokens qword_22E76C3E8,
// qword_22E76C3F0, qword_22E76C400 and qword_22E76C410 (__DATA,__bss).
static uint8_t urlFeatureEnabled;
static uint8_t urlV2Enabled;
static uint8_t cfbundleEnabled;
static uint8_t icuResourceTimeZoneEnabled;
static dispatch_once_t urlFeatureOnce;
static dispatch_once_t urlV2Once;
static dispatch_once_t cfbundleOnce;
static dispatch_once_t icuResourceTimeZoneOnce;

// Native __foundation_swift_characterSet_enabled, 0x1c051a370. Hidden (was a private external). The flag byte
// as stored (not compared with 1).
bool _foundation_swift_characterSet_enabled(void) {
    dispatch_once(
        &foundationSwiftCharacterSetOnceToken,
        (__bridge dispatch_block_t)(void *)foundationSwiftCharacterSetEnabledBlock
    );
    return foundationSwiftCharacterSetFeatureEnabled;
}

// Native __foundation_swift_url_feature_enabled, 0x1c051a3b0. Hidden (was a private external).
bool _foundation_swift_url_feature_enabled(void) {
    dispatch_once(&urlFeatureOnce, ^{
      // Native ____foundation_swift_url_feature_enabled_block_invoke, 0x1c051a3f0. Non-external.
      urlFeatureEnabled =
          _os_feature_enabled_impl("Foundation", "NSSwiftURL") || getenv("NSSwiftURL") != NULL;
    });
    return urlFeatureEnabled;
}

// Native __foundation_swift_url_v2_enabled, 0x1c051a43c. Hidden (was a private external).
bool _foundation_swift_url_v2_enabled(void) {
    dispatch_once(&urlV2Once, ^{
      // Native ____foundation_swift_url_v2_enabled_block_invoke, 0x1c051a47c. Non-external. The flag is
      // stored first, then masked with CoreFoundation's NSSwiftURL check.
      urlV2Enabled = _os_feature_enabled_impl("Foundation", "NSSwiftURL2") || getenv("NSSwiftURL2") != NULL;
      urlV2Enabled &= _foundation_swift_nsurl_feature_enabled();
    });
    return urlV2Enabled;
}

// Native __foundation_swift_cfbundle_enabled, 0x1c051a56c. Hidden (was a private external).
bool _foundation_swift_cfbundle_enabled(void) {
    dispatch_once(&cfbundleOnce, ^{
      // Native ____foundation_swift_cfbundle_enabled_block_invoke, 0x1c051a5ac. Non-external.
      cfbundleEnabled =
          _os_feature_enabled_impl("CoreFoundation", "CFSwiftBundle") || getenv("CFSwiftBundle") != NULL;
    });
    return cfbundleEnabled;
}

// Native __foundation_swift_ICUResourceTimeZone_feature_enabled, 0x1c051a684. Hidden (was a private
// external).
bool _foundation_swift_ICUResourceTimeZone_feature_enabled(void) {
    dispatch_once(&icuResourceTimeZoneOnce, ^{
      // Native ____foundation_swift_ICUResourceTimeZone_feature_enabled_block_invoke, 0x1c051a6c4.
      // Non-external.
      icuResourceTimeZoneEnabled = _os_feature_enabled_impl("Foundation", "ICUResourceTimeZone") ||
                                   getenv("ICUResourceTimeZone") != NULL;
    });
    return icuResourceTimeZoneEnabled;
}

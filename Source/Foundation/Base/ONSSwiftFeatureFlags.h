//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>
#include <stdbool.h>

// The Swift feature checks of the Foundation image (Base/ONSSwiftFeatureFlags.m), iOS Simulator 24A434,
// arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified. iOS Device 24A446:
// unread. Each answers a flag computed once from an os_feature and an environment variable of the same name.
// The names are outside the CF/NS prefix rule and stay hidden under their native names until the owner
// records a naming decision (as CoreFoundation's _foundation_swift_nsurl_feature_enabled).

// Native __foundation_swift_characterSet_enabled, 0x1c051a370: the out-of-line copy of the check that
// String/ONSCharacterSet.h inlines (the same once token, block and flag).
OF_EXTERN bool _foundation_swift_characterSet_enabled(void);

// Native __foundation_swift_url_feature_enabled, 0x1c051a3b0: Foundation/NSSwiftURL.
OF_EXTERN bool _foundation_swift_url_feature_enabled(void);

// Native __foundation_swift_url_v2_enabled, 0x1c051a43c: Foundation/NSSwiftURL2, and the NSSwiftURL feature
// of CoreFoundation's _foundation_swift_nsurl_feature_enabled.
OF_EXTERN bool _foundation_swift_url_v2_enabled(void);

// Native __foundation_swift_cfbundle_enabled, 0x1c051a56c: CoreFoundation/CFSwiftBundle.
OF_EXTERN bool _foundation_swift_cfbundle_enabled(void);

// Native __foundation_swift_ICUResourceTimeZone_feature_enabled, 0x1c051a684: Foundation/ICUResourceTimeZone.
OF_EXTERN bool _foundation_swift_ICUResourceTimeZone_feature_enabled(void);

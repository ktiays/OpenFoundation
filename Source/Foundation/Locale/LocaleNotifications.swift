//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

import Synchronization

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Foundation/Number/Reading.md, round 2. Organization follows the native LocaleNotifications type.

struct LocaleNotifications: ~Copyable {
    // Native LocaleNotifications.cache initializer, 0x1c09c4788. Non-external.
    // The native reset path at 0x1c09c489c uses a relaxed compare-and-swap on this word.
    static let cache = LocaleNotifications(generation: Atomic(1))

    let generation: Atomic<Int>
}

// Native _localeNotificationCount, 0x1c09c490c. Hidden (was a private external).
@_cdecl("_localeNotificationCount")
func _localeNotificationCount() -> Int {
    return LocaleNotifications.cache.generation.load(ordering: .relaxed)
}

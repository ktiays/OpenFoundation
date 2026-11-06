//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FX-swift-and-functions.

struct WeekendRange {
    // The first field is the Optional<Double> yielded by the native accessor.
    private var onsetTimeStorage: Double?
    var ceaseTime: Double?
    var start: Int
    var end: Int

    // Native WeekendRange.onsetTime.modify, 0x1c082d8b8. Private external.
    // X1 yields the address of the first field. The compiler emits the no-op resume at 0x1c04cf210.
    var onsetTime: Double? {
        get {
            onsetTimeStorage
        }
        set {
            onsetTimeStorage = newValue
        }
        _modify {
            yield &onsetTimeStorage
        }
    }
}

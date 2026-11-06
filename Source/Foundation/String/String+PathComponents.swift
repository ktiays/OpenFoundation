//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FX-swift-and-functions.

extension String {
    // Native String._droppingTrailingSlashes.getter, 0x1c0b62318. Private external.
    // The UTF8View lastIndex(where:) specialization is at 0x1c0be447c.
    var _droppingTrailingSlashes: String {
        if isEmpty {
            return self
        }
        guard let last = utf8.lastIndex(where: { $0 != 0x2f }) else {
            return "/"
        }
        return String(self[..<index(after: last)])
    }

    // Native String._lastPathComponent.getter, 0x1c0b61c20. Private external. A string of fewer than two UTF-8
    // units, or without a slash, is itself. A trailing slash run is skipped ("/" when only slashes remain); the
    // component is the text after the previous slash (or from the start) up to the last non-slash unit. The
    // UTF8View lastIndex(of:) specialization and the Substring-to-String conversions are compiler-generated.
    var _lastPathComponent: String {
        let bytes = utf8
        guard bytes.count >= 2, let lastSlash = bytes.lastIndex(of: 0x2f) else {
            return self
        }
        if lastSlash != bytes.index(before: bytes.endIndex) {
            return String(self[bytes.index(after: lastSlash)...])
        }
        var end = lastSlash
        repeat {
            if end == bytes.startIndex {
                return "/"
            }
            end = bytes.index(before: end)
        } while bytes[end] == 0x2f
        var start = end
        repeat {
            if start == bytes.startIndex {
                return String(self[bytes.startIndex..<bytes.index(after: end)])
            }
            start = bytes.index(before: start)
        } while bytes[start] != 0x2f
        return String(self[bytes.index(after: start)..<bytes.index(after: end)])
    }

    // Native String._compressingSlashes() is inlined in String.standardizingPath at 0x1c0b63b58.
    // Its UTF-8 initialization closure is at 0x1c0b63558, with thunks 0x1c08ead6c and 0x1c08ead88.
    func _compressingSlashes() -> String {
        let count = utf8.count
        if count <= 1 {
            return self
        }
        return String(unsafeUninitializedCapacity: count) { destination in
            var written = 0
            var previousWasSlash = false
            for byte in utf8 {
                let isSlash = byte == 0x2f
                if !isSlash || !previousWasSlash {
                    destination[written] = byte
                    written += 1
                }
                previousWasSlash = isSlash
            }
            return written
        }
    }
}

extension StringProtocol {
    // Native StringProtocol._hasDotDotComponent(), Substring specialization 0x1c0b644d4. Private external.
    func _hasDotDotComponent() -> Bool {
        if utf8.count < 2 {
            return false
        }
        var state = 0
        for byte in utf8 {
            if byte == 0x2f {
                if state == 2 {
                    return true
                }
                state = 0
            } else if byte == 0x2e && state < 2 {
                state += 1
            } else {
                state = 3
            }
        }
        return state == 2
    }
}

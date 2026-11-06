//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FX-swift-and-functions.

extension String {
    // Native String._expandingTildeInPath.getter, 0x1c0b648a8. Private external.
    // UTF8View.first, firstIndex(of:) and String's suffix subscript specialize at
    // 0x1c0766840, 0x1c086ac1c and 0x1c086ab48. Substring sequence witness at 0x1c08da5f8.
    var _expandingTildeInPath: String {
        guard utf8.first == 0x7e else {
            return self
        }
        let start = utf8.index(after: utf8.startIndex)
        let slash = utf8.firstIndex(of: 0x2f) ?? utf8.endIndex
        var directory: String
        if start == slash {
            directory = Self.homeDirectoryPath()
        } else {
            guard let home = Self.homeDirectoryPath(forUser: String(self[start..<slash])) else {
                return self
            }
            directory = home
        }
        directory.append(contentsOf: self[slash...])
        return directory
    }
}

// Native static URL.isAbsolute(standardizing:pathStyle:), 0x1c0b84570. Private external.
// The thunk branches to the specialization at 0x1c0b95cdc. This function needs no URL instance layout.
// Compiler specializations: starts(with:) at 0x1c0b50670, UTF-8 collection copy at 0x1c0bf348c,
// UInt8 array storage at 0x1c0bf35c0, 0x1c0797fe0, 0x1c0799aa0 and 0x1c080dc0c,
// and single-element subrange replacement at 0x1c0cde9b0.
func urlIsAbsolute(standardizing path: inout String, pathStyle: UInt8) -> Bool {
    if path.utf8.first == 0x2f {
        return true
    }
    if pathStyle == 1 {
        let bytes = path.utf8
        guard bytes.count >= 3 else {
            return false
        }
        let second = bytes.index(after: bytes.startIndex)
        let third = bytes.index(after: second)
        let drive = bytes.first! & 0xdf
        let separator = bytes[second]
        guard (0x41...0x5a).contains(drive), separator == 0x3a || separator == 0x7c, bytes[third] == 0x2f
        else {
            return false
        }
        if separator == 0x7c {
            var rewritten = Array(bytes)
            rewritten[1] = 0x3a
            rewritten.replaceSubrange(0..<0, with: CollectionOfOne(0x2f))
            path = String(decoding: rewritten, as: UTF8.self)
        } else {
            var absolute = "/"
            absolute.append(path)
            path = absolute
        }
        return true
    }
    if path.utf8.starts(with: [UInt8(0x7e), 0x2f]) {
        path = path._expandingTildeInPath
    }
    return path.utf8.first == 0x2f
}

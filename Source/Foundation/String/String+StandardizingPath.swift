//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

import Darwin

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FX-swift-and-functions.

@_silgen_name("swiftStandardizingPathFileExists")
private func standardizingPathFileExists(_ bytes: UnsafePointer<UInt8>?, _ count: Int) -> Bool

extension String {
    // Native String.homeDirectoryPath(), 0x1c0b5ef8c. Private external.
    // The initial environment reads precede issetugid. The later reads are kept.
    static func homeDirectoryPath() -> String {
        if let environment = getenv("CFFIXED_USER_HOME") ?? getenv("HOME") {
            return String(cString: environment).standardizingPath
        }
        if issetugid() == 0, let fixed = getenv("CFFIXED_USER_HOME") {
            return String(cString: fixed).standardizingPath
        }
        let ids = Platform.getUGIDs(allowEffectiveRootUID: false)
        if let home = Platform.homeDirectory(forUID: ids.uid) {
            return home.standardizingPath
        }
        if let environment = getenv("HOME") {
            return String(cString: environment).standardizingPath
        }
        return "/var/empty"
    }

    // Native String.homeDirectoryPath(forUser:), 0x1c0b6062c. Private external.
    static func homeDirectoryPath(forUser name: String) -> String? {
        if issetugid() == 0, let fixed = getenv("CFFIXED_USER_HOME") {
            return String(cString: fixed).standardizingPath
        }
        return Platform.homeDirectory(forUserName: name)?.standardizingPath
    }

    // Native String.standardizingPath.getter, 0x1c0b63918. Private external.
    // The network prefix at 0x2227f77f0 is two backslashes. Test the candidate after the first
    // forward slash for a parent component, but resolve the full path if the test succeeds.
    var standardizingPath: String {
        var path = _expandingTildeInPath._compressingSlashes()._droppingTrailingSlashes
        let start =
            path.utf8.starts(with: Self.networkPrefix)
            ? path.utf8.firstIndex(of: 0x2f) ?? path.utf8.endIndex
            : path.utf8.startIndex
        if path[start...]._hasDotDotComponent(), let resolved = path._resolvingSymlinksInPath() {
            path = resolved
        }
        if !path.isEmpty {
            path = path._removingDotSegments(useRFC1808: false)
        }

        guard let prefix = Self._automountPrefixes.first(where: { path.utf8.starts(with: $0) }) else {
            return path
        }
        let stripped = path.utf8.index(path.utf8.startIndex, offsetBy: prefix.count - 1)
        let candidate = path[stripped...]
        for exclusion in Self._automountExclusionList where candidate.utf8.starts(with: exclusion) {
            let end = candidate.utf8.index(candidate.utf8.startIndex, offsetBy: exclusion.count)
            if end == candidate.utf8.endIndex || candidate.utf8[end] == 0x2f {
                return path
            }
        }
        var candidatePath = String(candidate)
        if candidatePath.withUTF8({ standardizingPathFileExists($0.baseAddress, $0.count) }) {
            return String(candidate)
        }
        return path
    }

    private static let networkPrefix: [UInt8] = [0x5c, 0x5c]

    // Native String._automountPrefixes one-time initializer, 0x1c0b64280. Private external.
    // UTF8View collection copy specialization 0x1c0bf348c, shared with the exclusion initializer.
    private static let _automountPrefixes: [[UInt8]] = [
        Array("/private/var/automount/".utf8), Array("/var/automount/".utf8), Array("/private/".utf8),
    ]

    // Native String._automountExclusionList one-time initializer, 0x1c0b64338. Private external.
    // The initialization closure is 0x1c0b64354.
    private static let _automountExclusionList: [[UInt8]] = {
        [
            Array("/Applications".utf8), Array("/Library".utf8), Array("/System".utf8), Array("/Users".utf8),
            Array("/Volumes".utf8), Array("/bin".utf8), Array("/cores".utf8), Array("/dev".utf8),
            Array("/etc".utf8), Array("/private".utf8), Array("/sbin".utf8), Array("/usr".utf8),
        ]
    }()
}

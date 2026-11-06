//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FX-swift-and-functions.

// Native sub_1C0BD9C34, 0x1c0bd9c34. Non-external. The 0x1c0bdd438 thunk calls this boundary.
@inline(never)
private func decomposeUTF8ThroughThunk(
    _ normalization: UInt8,
    _ output: UnsafeMutableBufferPointer<UInt8>,
    _ nullTerminated: Bool,
    _ input: UnsafeBufferPointer<UInt8>
) throws -> Int {
    try decomposeUTF8(normalization, output, nullTerminated, input)
}

extension String {
    @_silgen_name("$sSS18_uncheckedFromUTF8ySSSRys5UInt8VGFZ")
    private static func uncheckedStringFromUTF8(_ bytes: UnsafeBufferPointer<UInt8>) -> String

    // Native String._decomposed(_:into:nullTerminated:), 0x1c0b13494. Private external.
    func _decomposed(
        _ normalization: UInt8,
        into output: UnsafeMutableBufferPointer<UInt8>,
        nullTerminated: Bool
    ) -> Int? {
        var copy = self
        if !copy.isContiguousUTF8 {
            copy = Self._copying(copy)
        }
        return copy.withUTF8 { input in
            try? decomposeUTF8ThroughThunk(normalization, output, nullTerminated, input)
        }
    }

    // Native String._copying(_:), 0x1c0c211c0. Private external.
    static func _copying(_ string: String) -> String {
        _copying(string[string.startIndex..<string.endIndex])
    }

    // Native String._copying(_:), 0x1c0c2123c. Private external.
    // The foreign UTF-8 storage path specializes the sequence copy at 0x1c0c2a738.
    static func _copying(_ substring: Substring) -> String {
        if let copied = substring.utf8.withContiguousStorageIfAvailable(uncheckedStringFromUTF8) {
            return copied
        }
        return Array(substring.utf8).withUnsafeBufferPointer(uncheckedStringFromUTF8)
    }
}

//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FX-swift-and-functions.

struct BuiltInUnicodeScalarSet {
    let rawValue: UInt8

    // Native BuiltInUnicodeScalarSet._bitmapTableIndex.getter, 0x1c0b095d0. Private external.
    // The tag bytes at 0x1c10a3e88 mark absent optional values in the table at 0x1c10a3db0.
    private var _bitmapTableIndex: Int? {
        Self.bitmapTableIndices[Int(rawValue)]
    }

    private static let bitmapTableIndices: [Int?] = [
        19, nil, nil, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, nil, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21, 19, 5,
    ]

    // Native BuiltInUnicodeScalarSet.contains(_:), 0x1c0b0a280. Private external.
    func contains(_ scalar: Unicode.Scalar) -> Bool {
        let value = scalar.value
        let newline = (0x0a...0x0d).contains(value) || value == 0x85 || (0x2028...0x2029).contains(value)
        switch rawValue {
        case 1, 2:
            let space =
                value == 0x09 || value == 0x20 || value == 0xa0 || value == 0x1680
                || (0x2000...0x200b).contains(value) || value == 0x202f || value == 0x205f || value == 0x3000
            return space || (rawValue == 2 && newline)
        case 14:
            return newline
        default:
            break
        }

        let plane = Int((value >> 16) & 0xff)
        let lowByte = value & 0xff
        if rawValue == 11 {
            if plane == 15 || plane == 16 {
                return false
            }
            if plane == 14 {
                return lowByte != 1 && lowByte &- 0x80 < 0xffff_ffa0
            }
        } else if rawValue == 25 && plane == 14 {
            return lowByte == 1 || (0x20...0x7f).contains(lowByte)
        }

        guard let (span, shouldInvert) = _bitmapPtrForPlane(plane) else {
            return false
        }
        if (rawValue == 0 || rawValue == 25) && plane >= 15 {
            return false
        }
        let byteIndex = Int((value & 0xffff) >> 3)
        let contains = ((span[byteIndex] >> (value & 7)) & 1) != 0
        return contains != shouldInvert
    }

    // Native BuiltInUnicodeScalarSet._bitmapPtrForPlane(_:), 0x1c0b0a5d4. Private external.
    // The process-lifetime buffer uses the SDK-supported pointer/count view.
    private func _bitmapPtrForPlane(_ plane: Int) -> (span: UnsafeBufferPointer<UInt8>, shouldInvert: Bool)? {
        guard rawValue != 1, rawValue != 2, rawValue != 14, let index = _bitmapTableIndex, index <= 21 else {
            return nil
        }
        let bitmap = BuiltInUnicodeBitmapData.table[index]
        guard plane < bitmap.count, bitmap[plane] >= 0 else {
            return nil
        }
        return (BuiltInUnicodeBitmapData.span(forPlane: bitmap[plane]), rawValue == 11)
    }
}

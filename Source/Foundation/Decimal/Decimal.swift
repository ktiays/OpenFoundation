//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

extension ONSDecimal {
    // Native: NSDecimal._CalculationError, Foundation metadata 0x2227e5a90.
    // iOS Simulator UUID: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified payload tags 0/1/2.
    // iOS Device field descriptor 0x181b9e290, UUID C041F1D0-5EF1-30A3-BF96-830976173B56,
    // independently resolves the same three case names in tag order. Confidence: verified.
    // iOS Device divergence: seven encoded value-witness bindings remain unread; generated runtime
    // support is attached to this type in the restoration ledger, without invented source bodies.
    enum _CalculationError: Error, Hashable {
        case overflow
        case underflow
        case divideByZero
    }
    // Native specialized NSDecimal.init(UInt64), iOS Simulator 24A434, 0x1c08c87d4.
    // Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified.
    // iOS Device 24A446, 0x181198110, UUID C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified same zero, trailing-zero removal and limb count paths.
    // iOS Device divergence: NSDecimal.init(UInt64) at 0x181198110 uses authenticated calls/returns.
    // Reading: Documentation/Reference/Research/FoundationDecimal.md.
    init(_ value: UInt64) {
        self = ONSDecimal()
        if value != 0 {
            var mantissa = value
            var exponent: Int32 = 0
            while mantissa % 10 == 0 {
                mantissa /= 10
                exponent += 1
            }
            _isCompact = 1
            _exponent = exponent
            _length = UInt32((79 - mantissa.leadingZeroBitCount) >> 4)
            _mantissa = (
                UInt16(truncatingIfNeeded: mantissa), UInt16(truncatingIfNeeded: mantissa >> 16),
                UInt16(truncatingIfNeeded: mantissa >> 32), UInt16(truncatingIfNeeded: mantissa >> 48),
                0, 0, 0, 0
            )
        }
    }
}

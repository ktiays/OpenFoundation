//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

extension ONSDecimal {
    // Native storage used by all integer helpers: Int length at offset 0, 17 UInt16 words at offset 8.
    // iOS Simulator Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03, asVariableLengthInteger
    // 0x1c08cdf20.
    // iOS Device Foundation UUID C041F1D0-5EF1-30A3-BF96-830976173B56, 0x180c9f6d4. Confidence: verified.
    struct VariableLengthInteger {
        var length: Int
        var elements: InlineArray<17, UInt16>
    }

    // Native: NSDecimal.asVariableLengthInteger(), 0x1c08cdf20 / iOS Device 0x180c9f6d4.
    // Foundation UUIDs: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
    // iOS Device divergence: NSDecimal.asVariableLengthInteger at 0x180c9f6d4 uses PACIBSP/RETAB.
    // Confidence: verified; authenticated execution remains unvalidated in this simulator-only stage.
    func asVariableLengthInteger() -> VariableLengthInteger {
        var result = VariableLengthInteger(length: 0, elements: InlineArray(repeating: 0))
        result.elements[0] = _mantissa.0
        result.elements[1] = _mantissa.1
        result.elements[2] = _mantissa.2
        result.elements[3] = _mantissa.3
        result.elements[4] = _mantissa.4
        result.elements[5] = _mantissa.5
        result.elements[6] = _mantissa.6
        result.elements[7] = _mantissa.7
        var index = 8
        while index > 0 && result.elements[index - 1] == 0 {
            index -= 1
        }
        result.length = index
        return result
    }

    // Native: NSDecimal.copyVariableLengthInteger(_:), 0x1c08ce05c / iOS Device 0x180c9f82c.
    // Foundation UUIDs: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
    // iOS Device divergence: NSDecimal.copyVariableLengthInteger at 0x180c9f82c uses PACIBSP/RETAB.
    // Confidence: verified; authenticated execution remains unvalidated in this simulator-only stage.
    mutating func copyVariableLengthInteger(_ integer: VariableLengthInteger) throws {
        if integer.length > 8 {
            throw _CalculationError.overflow
        }
        _length = UInt32(integer.length)
        _mantissa = (
            integer.length > 0 ? integer.elements[0] : 0,
            integer.length > 1 ? integer.elements[1] : 0,
            integer.length > 2 ? integer.elements[2] : 0,
            integer.length > 3 ? integer.elements[3] : 0,
            integer.length > 4 ? integer.elements[4] : 0,
            integer.length > 5 ? integer.elements[5] : 0,
            integer.length > 6 ? integer.elements[6] : 0,
            integer.length > 7 ? integer.elements[7] : 0
        )
    }

    // Native: static NSDecimal._integerCompare(lhs:rhs:), 0x1c08cf5cc / iOS Device 0x18119c1d0.
    // Foundation UUIDs: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
    // iOS Device divergence: NSDecimal._integerCompare at 0x18119c1d0 uses PACIBSP/RETAB.
    // Confidence: verified; authenticated execution remains unvalidated in this simulator-only stage.
    static func _integerCompare(lhs: VariableLengthInteger, rhs: VariableLengthInteger) -> ONSComparisonResult
    {
        if rhs.length < lhs.length {
            return ONSComparisonResult(rawValue: 1)!
        }
        if lhs.length < rhs.length {
            return ONSComparisonResult(rawValue: -1)!
        }
        let upperBound = lhs.length + 1
        precondition(upperBound >= 1)
        var index = lhs.length
        while index > 0 {
            index -= 1
            if rhs.elements[index] < lhs.elements[index] {
                return ONSComparisonResult(rawValue: 1)!
            }
            if lhs.elements[index] < rhs.elements[index] {
                return ONSComparisonResult(rawValue: -1)!
            }
        }
        return ONSComparisonResult(rawValue: 0)!
    }

    // Native: static NSDecimal._integerAdd(lhs:rhs:maxResultLength:), 0x1c08cf2d4 / iOS Device 0x180cc42b8.
    // Foundation UUIDs: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
    // iOS Device divergence: NSDecimal._integerAdd at 0x180cc42b8 uses PACIBSP/RETAB.
    // Confidence: verified; authenticated execution remains unvalidated in this simulator-only stage.
    static func _integerAdd(
        lhs: VariableLengthInteger, rhs: VariableLengthInteger, maxResultLength: Int
    ) throws -> VariableLengthInteger {
        var result = VariableLengthInteger(length: maxResultLength, elements: InlineArray(repeating: 0))
        let commonLength = min(lhs.length, rhs.length)
        var index = 0
        var carry: UInt32 = 0
        if commonLength > 0 {
            while index < commonLength {
                let sum = carry + UInt32(lhs.elements[index]) + UInt32(rhs.elements[index])
                carry = sum >> 16
                result.elements[index] = UInt16(truncatingIfNeeded: sum)
                index += 1
            }
        }
        while index < lhs.length && carry != 0 {
            let sum = UInt32(lhs.elements[index]) + 1
            carry = sum >> 16
            result.elements[index] = UInt16(truncatingIfNeeded: sum)
            index += 1
        }
        while index < lhs.length {
            result.elements[index] = lhs.elements[index]
            index += 1
        }
        while index < rhs.length && carry != 0 {
            let sum = UInt32(rhs.elements[index]) + 1
            carry = sum >> 16
            result.elements[index] = UInt16(truncatingIfNeeded: sum)
            index += 1
        }
        while index < rhs.length {
            result.elements[index] = rhs.elements[index]
            index += 1
        }
        if carry != 0 {
            if maxResultLength < index {
                throw _CalculationError.overflow
            }
            result.elements[index] = 1
            index += 1
        }
        let remainingLength = maxResultLength - index
        precondition(maxResultLength >= remainingLength)
        result.length = maxResultLength - remainingLength
        return result
    }

    // Native: static NSDecimal._integerSubtract(term:subtrahend:maxResultLength:), 0x1c08cfd84 /
    // Device 0x18119c454.
    // Foundation UUIDs: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
    // iOS Device divergence: NSDecimal._integerSubtract at 0x18119c454 uses PACIBSP/RETAB.
    // Confidence: verified; authenticated execution remains unvalidated in this simulator-only stage.
    static func _integerSubtract(
        term: VariableLengthInteger, subtrahend: VariableLengthInteger, maxResultLength: Int
    ) throws -> VariableLengthInteger {
        var result = VariableLengthInteger(length: maxResultLength, elements: InlineArray(repeating: 0))
        let commonLength = min(term.length, subtrahend.length)
        var index = 0
        var noBorrow: UInt32 = 1
        if commonLength > 0 {
            while index < commonLength {
                let difference =
                    noBorrow + UInt32(term.elements[index])
                    + UInt32(subtrahend.elements[index] ^ 0xFFFF)
                noBorrow = difference >> 16
                result.elements[index] = UInt16(truncatingIfNeeded: difference)
                index += 1
            }
        }
        while index < term.length && noBorrow == 0 {
            let difference = UInt32(term.elements[index]) + 65535
            noBorrow = difference >> 16
            result.elements[index] = UInt16(truncatingIfNeeded: difference)
            index += 1
        }
        while index < term.length {
            result.elements[index] = term.elements[index]
            index += 1
        }
        while index < subtrahend.length {
            let difference = noBorrow + UInt32(subtrahend.elements[index] ^ 0xFFFF)
            noBorrow = difference >> 16
            result.elements[index] = UInt16(truncatingIfNeeded: difference)
            index += 1
        }
        if noBorrow == 0 {
            throw _CalculationError.overflow
        }
        while result.length > 0 && result.elements[result.length - 1] == 0 {
            result.length -= 1
        }
        return result
    }

    // Native: static NSDecimal._integerMultiply(lhs:rhs:maxResultLength:), 0x1c08ce208 / iOS Device
    // 0x180e59da8.
    // Foundation UUIDs: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
    // iOS Device divergence: NSDecimal._integerMultiply at 0x180e59da8 uses PACIBSP/RETAB.
    // Confidence: verified; authenticated execution remains unvalidated in this simulator-only stage.
    static func _integerMultiply(
        lhs: VariableLengthInteger, rhs: VariableLengthInteger, maxResultLength: Int
    ) throws -> VariableLengthInteger {
        if lhs.length == 0 || rhs.length == 0 {
            return VariableLengthInteger(length: 0, elements: InlineArray(repeating: 0))
        }
        let length = min(lhs.length + rhs.length, maxResultLength)
        var result = VariableLengthInteger(length: length, elements: InlineArray(repeating: 0))
        precondition(lhs.length >= 0 && rhs.length >= 0)
        for rhsIndex in 0..<rhs.length {
            var carry: UInt32 = 0
            for lhsIndex in 0..<lhs.length {
                let index = lhsIndex + rhsIndex
                if index >= length {
                    if carry != 0 || (rhs.elements[rhsIndex] != 0 && lhs.elements[lhsIndex] != 0) {
                        throw _CalculationError.overflow
                    }
                    carry = 0
                } else {
                    let product =
                        carry + UInt32(result.elements[index])
                        + UInt32(lhs.elements[lhsIndex]) * UInt32(rhs.elements[rhsIndex])
                    carry = product >> 16
                    result.elements[index] = UInt16(truncatingIfNeeded: product)
                }
            }
            if carry != 0 {
                let index = lhs.length + rhsIndex
                if index >= length {
                    throw _CalculationError.overflow
                }
                result.elements[index] = UInt16(carry)
            }
        }
        while result.length > 0 && result.elements[result.length - 1] == 0 {
            result.length -= 1
        }
        return result
    }

    // Native: static NSDecimal._integerMultiplyByShort(lhs:mulplicand:maxResultLength:), 0x1c08ce4d8 /
    // Device 0x18119c010.
    // Foundation UUIDs: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
    // iOS Device divergence: NSDecimal._integerMultiplyByShort at 0x18119c010 uses PACIBSP/RETAB.
    // Confidence: verified; authenticated execution remains unvalidated in this simulator-only stage.
    static func _integerMultiplyByShort(
        lhs: VariableLengthInteger, mulplicand: UInt32, maxResultLength: Int
    ) throws -> VariableLengthInteger {
        if mulplicand == 0 {
            return VariableLengthInteger(length: 0, elements: InlineArray(repeating: 0))
        }
        if maxResultLength < lhs.length {
            throw _CalculationError.overflow
        }
        var result = VariableLengthInteger(length: lhs.length, elements: InlineArray(repeating: 0))
        precondition(lhs.length >= 0)
        var carry: UInt32 = 0
        for index in 0..<lhs.length {
            let product = UInt32(lhs.elements[index]) * mulplicand
            let sum = product + carry
            carry = sum >> 16
            result.elements[index] = UInt16(truncatingIfNeeded: sum)
        }
        if carry != 0 {
            if maxResultLength == lhs.length {
                throw _CalculationError.overflow
            }
            result.elements[lhs.length] = UInt16(carry)
            result.length += 1
        }
        return result
    }

    // Native: static NSDecimal._integerDivideByShort(_:_:), 0x1c08cf6c8 / iOS Device 0x180ed3c6c.
    // Foundation UUIDs: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
    // iOS Device divergence: NSDecimal._integerDivideByShort at 0x180ed3c6c uses PACIBSP/RETAB.
    // Confidence: verified; authenticated execution remains unvalidated in this simulator-only stage.
    static func _integerDivideByShort(
        _ lhs: VariableLengthInteger, _ divisor: UInt32
    ) throws -> (quotient: VariableLengthInteger, remainder: UInt32) {
        if divisor == 0 {
            throw _CalculationError.divideByZero
        }
        var quotient = VariableLengthInteger(length: lhs.length, elements: InlineArray(repeating: 0))
        precondition(lhs.length >= 0)
        var remainder: UInt32 = 0
        var index = lhs.length
        while index > 0 {
            index -= 1
            let partial = UInt32(UInt16(remainder)) << 16 | UInt32(lhs.elements[index])
            quotient.elements[index] = UInt16(partial / divisor)
            remainder = partial % divisor
        }
        while quotient.length > 0 && quotient.elements[quotient.length - 1] == 0 {
            quotient.length -= 1
        }
        return (quotient, remainder)
    }

    // Native: static NSDecimal._integerAddShort(_:rhs:maxResultLength:), 0x1c08cf87c / iOS Device
    // 0x18119c2d4.
    // Foundation UUIDs: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
    // iOS Device divergence: NSDecimal._integerAddShort at 0x18119c2d4 uses PACIBSP/RETAB.
    // Confidence: verified; authenticated execution remains unvalidated in this simulator-only stage.
    static func _integerAddShort(
        _ lhs: VariableLengthInteger, rhs: UInt32, maxResultLength: Int?
    ) throws -> VariableLengthInteger {
        var result = VariableLengthInteger(length: lhs.length, elements: InlineArray(repeating: 0))
        precondition(lhs.length >= 0)
        var carry = rhs
        for index in 0..<lhs.length {
            let sum = UInt32(lhs.elements[index]) + carry
            result.elements[index] = UInt16(truncatingIfNeeded: sum)
            carry = sum >> 16
        }
        if carry != 0 {
            if let maxResultLength, lhs.length == maxResultLength {
                throw _CalculationError.overflow
            }
            result.elements[lhs.length] = UInt16(carry)
            result.length += 1
        }
        return result
    }

    // Native: static NSDecimal._integerMultiplyByPowerOfTen(lhs:power:maxResultLength:), 0x1c08ced24 /
    // Device 0x180e5a080.
    // Foundation UUIDs: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
    // iOS Device divergence: NSDecimal._integerMultiplyByPowerOfTen at 0x180e5a080 uses PACIBSP/RETAB.
    // Confidence: verified; authenticated execution remains unvalidated in this simulator-only stage.
    static func _integerMultiplyByPowerOfTen(
        lhs: VariableLengthInteger, power: Int, maxResultLength: Int
    ) throws -> VariableLengthInteger {
        if power == 0 {
            return lhs
        }
        var remainder = power < 0 ? -power : power
        let count = powerOfTen.count
        var value = lhs
        if count <= remainder {
            let factor = powerOfTen[count - 1]
            remainder = remainder - count + 1
            while true {
                if power < 0 {
                    value = try _integerDivide(
                        dividend: value, divisor: factor, maxResultLength: maxResultLength)
                } else {
                    value = try _integerMultiply(lhs: value, rhs: factor, maxResultLength: maxResultLength)
                }
                if count > remainder {
                    break
                }
                remainder += 1 - count
            }
        }
        let factor = powerOfTen[remainder]
        if power < 0 {
            return try _integerDivide(dividend: value, divisor: factor, maxResultLength: maxResultLength)
        }
        return try _integerMultiply(lhs: value, rhs: factor, maxResultLength: maxResultLength)
    }

    // Native: static NSDecimal._fitMantissa(_:roundingMode:), 0x1c08cf9f0 / iOS Device 0x180ebfdd8.
    // Foundation UUIDs: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
    // iOS Device divergence: NSDecimal._fitMantissa at 0x180ebfdd8 uses PACIBSP/RETAB.
    // Confidence: verified; authenticated execution remains unvalidated in this simulator-only stage.
    static func _fitMantissa(
        _ integer: VariableLengthInteger, roundingMode: ONSRoundingMode
    ) throws -> (result: VariableLengthInteger, exponent: Int, lossOfPrecision: Bool) {
        if integer.length <= 8 {
            return (integer, 0, false)
        }
        var value = integer
        var exponent = 0
        var remainder: UInt32 = 0
        var discardedNonzero = false
        while value.length > 9 {
            if remainder != 0 {
                discardedNonzero = true
            }
            let division = try _integerDivideByShort(value, 10000)
            value = division.quotient
            remainder = division.remainder
            exponent += 4
        }
        while value.length == 9 {
            if remainder != 0 {
                discardedNonzero = true
            }
            let division = try _integerDivideByShort(value, 10)
            value = division.quotient
            remainder = division.remainder
            exponent += 1
        }
        if discardedNonzero {
            if remainder == 0 || remainder == 5 {
                remainder += 1
            }
        } else if remainder == 0 {
            return (value, exponent, false)
        }
        let mode = roundingMode.rawValue
        var increment = false
        if mode == 0 {
            increment = remainder >= 5
        } else if mode == 2 {
            increment = true
        } else if mode == 3 {
            if remainder == 5 {
                increment = value.elements[0] & 1 == 0
            } else {
                increment = remainder >= 5
            }
        }
        if increment {
            let rounded = try _integerAddShort(value, rhs: 1, maxResultLength: value.length + 1)
            if rounded.length < value.length {
                _ = try _integerDivideByShort(rounded, 10)
                exponent += 1
            }
        }
        return (value, exponent, true)
    }
    // Native: static NSDecimal._integerDivide(dividend:divisor:maxResultLength:), 0x1c08ce694 / Device
    // 0x180ebf090.
    // Foundation UUIDs: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified instructions and device decompilation; simulator Hex-Rays INTERR 50920.
    // Reading: Documentation/Reference/Research/FoundationDecimal.md.
    // iOS Device divergence: NSDecimal._integerDivide at 0x180ebf090 uses PACIBSP/RETAB.
    // Confidence: verified; authenticated execution remains unvalidated in this simulator-only stage.
    static func _integerDivide(
        dividend: VariableLengthInteger,
        divisor: VariableLengthInteger,
        maxResultLength: Int
    ) throws -> VariableLengthInteger {
        if divisor.length == 0 {
            throw _CalculationError.divideByZero
        }
        if dividend.length < divisor.length {
            return VariableLengthInteger(length: 0, elements: InlineArray(repeating: 0))
        }
        if dividend.length == divisor.length {
            precondition(dividend.length >= 0)
            var index = dividend.length
            while index > 0 {
                index -= 1
                if dividend.elements[index] > divisor.elements[index] {
                    break
                }
                if dividend.elements[index] < divisor.elements[index] {
                    return VariableLengthInteger(length: 0, elements: InlineArray(repeating: 0))
                }
            }
        }
        if divisor.length == 1 {
            let shortDivisor = UInt32(divisor.elements[0])
            if shortDivisor == 0 {
                throw _CalculationError.divideByZero
            }
            var quotient = VariableLengthInteger(
                length: dividend.length, elements: InlineArray(repeating: 0)
            )
            precondition(dividend.length >= 0)
            var remainder: UInt32 = 0
            var index = dividend.length
            while index > 0 {
                index -= 1
                let numerator = UInt32(dividend.elements[index]) | (remainder << 16)
                quotient.elements[index] = UInt16(numerator / shortDivisor)
                remainder = numerator % shortDivisor
            }
            while quotient.length > 0 && quotient.elements[quotient.length - 1] == 0 {
                quotient.length -= 1
            }
            return quotient
        }
        let originalDividendLength = dividend.length
        let factor = UInt32(65536) / (UInt32(divisor.elements[divisor.length - 1]) + 1)
        var u = try _integerMultiplyByShort(
            lhs: dividend, mulplicand: factor, maxResultLength: originalDividendLength + 1
        )
        var v = try _integerMultiplyByShort(
            lhs: divisor, mulplicand: factor, maxResultLength: divisor.length + 1
        )
        if u.length == originalDividendLength {
            u.elements[originalDividendLength] = 0
            u.length = originalDividendLength + 1
        }
        let n = v.length
        v.elements[n] = 0
        v.length = n + 1
        let uLength = u.length
        let digitCount = uLength - n
        precondition(n != 0)
        let secondTop = n == 1 ? UInt32(0) : UInt32(v.elements[n - 2])
        let top = UInt32(v.elements[n - 1])
        var quotient = VariableLengthInteger(
            length: maxResultLength, elements: InlineArray(repeating: 0)
        )
        precondition(digitCount >= 0)
        if digitCount > 0 {
            precondition(top != 0)
            for j in 0..<digitCount {
                let upper = uLength - j
                let numerator =
                    (UInt32(u.elements[upper - 1]) << 16)
                    | UInt32(u.elements[upper - 2])
                var estimate = numerator / top
                var remainder = numerator % top
                if estimate == 65536
                    || estimate * secondTop > ((remainder << 16) | UInt32(u.elements[upper - 3]))
                {
                    estimate -= 1
                    remainder += top
                    if remainder < 65536 {
                        if estimate == 65536
                            || estimate * secondTop > ((remainder << 16) | UInt32(u.elements[upper - 3]))
                        {
                            estimate -= 1
                        }
                    }
                }
                let quotientIndex = digitCount - j - 1
                var carry: UInt32 = 0
                var noBorrow: UInt32 = 1
                for i in 0...n {
                    let product = UInt32(v.elements[i]) * estimate + carry
                    carry = product >> 16
                    let index = quotientIndex + i
                    let wide =
                        UInt32(u.elements[index]) &+ 65535
                        &+ (noBorrow &- (product & 65535))
                    noBorrow = wide >> 16
                    u.elements[index] = UInt16(truncatingIfNeeded: wide)
                }
                if noBorrow == 0 {
                    estimate -= 1
                    var carry: UInt32 = 0
                    for i in 0..<n {
                        let index = quotientIndex + i
                        let sum = UInt32(v.elements[i]) + carry + UInt32(u.elements[index])
                        carry = sum >> 16
                        u.elements[index] = UInt16(truncatingIfNeeded: sum)
                    }
                }
                quotient.elements[quotientIndex] = UInt16(estimate)
            }
        }
        while quotient.length > 0 && quotient.elements[quotient.length - 1] == 0 {
            quotient.length -= 1
        }
        return quotient
    }

}

// Native: one-time initialization function for powerOfTen, 0x1c08cd960 / iOS Device 0x180ebea80.
// Foundation UUIDs: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified; all 39 entries translated from recorded stores and constant vectors.
// Reading: Documentation/Reference/Research/FoundationDecimal.md.
// iOS Device divergence: powerOfTen initializer 0x180ebea80 uses PACIBSP/RETAB.
// Confidence: verified; authenticated execution remains unvalidated in this simulator-only stage.
let powerOfTen: [ONSDecimal.VariableLengthInteger] = [
    ONSDecimal.VariableLengthInteger(
        length: 1, elements: [1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 1, elements: [10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 1, elements: [100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 1, elements: [1000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 1, elements: [10000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 2, elements: [34464, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 2, elements: [16960, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 2, elements: [38528, 152, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 2, elements: [57600, 1525, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 2, elements: [51712, 15258, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 3, elements: [58368, 21515, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 3, elements: [59392, 18550, 23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 3, elements: [4096, 54437, 232, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 3, elements: [40960, 20082, 2328, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 3, elements: [16384, 4218, 23283, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 4, elements: [32768, 42182, 36222, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 4, elements: [0, 28609, 34546, 35, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 4, elements: [0, 23946, 17784, 355, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 4, elements: [0, 42852, 46771, 3552, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 4, elements: [0, 35304, 8964, 35527, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 5, elements: [0, 25360, 24109, 27591, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 5, elements: [0, 56992, 44485, 13769, 54, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 5, elements: [0, 45632, 51642, 6624, 542, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 5, elements: [0, 63104, 57674, 711, 5421, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 5, elements: [0, 41216, 52461, 7118, 54210, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 6, elements: [0, 18944, 328, 5652, 17813, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 6, elements: [0, 58368, 3282, 56520, 47058, 82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 6, elements: [0, 59392, 32828, 40912, 11836, 827, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 6, elements: [0, 4096, 609, 15909, 52830, 8271, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 7, elements: [0, 40960, 6090, 28018, 4014, 17182, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 7, elements: [0, 16384, 60906, 18036, 40144, 40748, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 7, elements: [0, 32768, 19238, 49297, 8226, 14270, 126, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 7, elements: [0, 0, 61313, 34220, 16731, 11629, 1262, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 7, elements: [0, 0, 23306, 14529, 36243, 50756, 12621, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 8, elements: [0, 0, 36452, 14221, 34752, 48813, 60681, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 8, elements: [0, 0, 36840, 11143, 19842, 29383, 16993, 19, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 8, elements: [0, 0, 40720, 45899, 1813, 31689, 38862, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 8, elements: [0, 0, 13984, 244, 18137, 54746, 60944, 1925, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    ONSDecimal.VariableLengthInteger(
        length: 8, elements: [0, 0, 8768, 2442, 50298, 23174, 19624, 19259, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
]

//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

extension ONSDecimal {
    // Native: NSDecimal._divide(by: UInt16), IDA 0x1c08cba50; iOS Simulator 27.0 (24A434), arm64.
    // Foundation UUID: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified.
    // iOS Device 27.0.1 (24A446), arm64e, IDA 0x180c9f494.
    // Foundation UUID: C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified arithmetic branches/constants.
    // iOS Device divergence: the function at the device address above uses authenticated calls/returns.
    // Reading: Documentation/Reference/Research/FoundationDecimal.md.
    func _divide(by divisor: UInt16) throws -> (result: ONSDecimal, remainder: UInt16) {
        let integer = asVariableLengthInteger()
        if divisor == 0 {
            throw _CalculationError.divideByZero
        }
        var quotient = VariableLengthInteger(length: integer.length, elements: InlineArray(repeating: 0))
        var remainder: UInt32 = 0
        var index = integer.length
        while index != 0 {
            index -= 1
            let value = UInt32(integer.elements[index]) | (remainder << 16)
            quotient.elements[index] = UInt16(value / UInt32(divisor))
            remainder = value % UInt32(divisor)
        }
        while quotient.length > 0 && quotient.elements[quotient.length - 1] == 0 {
            quotient.length -= 1
        }
        var result = self
        try result.copyVariableLengthInteger(quotient)
        result._length = UInt32(quotient.length)
        return (result, UInt16(remainder))
    }

    // Native: NSDecimal._multiply(byShort:), IDA 0x1c08cbc94; iOS Simulator 27.0 (24A434), arm64.
    // Foundation UUID: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified.
    // iOS Device 27.0.1 (24A446), arm64e, IDA 0x180c9ec68.
    // Foundation UUID: C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified arithmetic branches/constants.
    // iOS Device divergence: the function at the device address above uses authenticated calls/returns.
    // Reading: Documentation/Reference/Research/FoundationDecimal.md.
    func _multiply(byShort multiplier: UInt16) throws -> ONSDecimal {
        var result = self
        if multiplier == 0 {
            result._length = 0
            return result
        }
        if _length == 0 {
            result._length = 0
            return result
        }
        var carry: UInt32 = 0
        var index: UInt32 = 0
        repeat {
            let product = carry + UInt32(result[index]) * UInt32(multiplier)
            result[index] = UInt16(truncatingIfNeeded: product)
            carry = product >> 16
            index += 1
        } while index < result._length
        if carry != 0 {
            if result._length > 7 {
                throw _CalculationError.overflow
            }
            result[index] = UInt16(carry)
            index += 1
        }
        result._length = index
        return result
    }

    // Native: NSDecimal._add(UInt16), IDA 0x1c08cbff4; iOS Simulator 27.0 (24A434), arm64.
    // Foundation UUID: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified.
    // iOS Device 27.0.1 (24A446), arm64e, IDA 0x180c9efec.
    // Foundation UUID: C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified arithmetic branches/constants.
    // iOS Device divergence: the function at the device address above uses authenticated calls/returns.
    // Reading: Documentation/Reference/Research/FoundationDecimal.md.
    func _add(_ value: UInt16) throws -> ONSDecimal {
        var result = self
        var index: UInt32 = 0
        var carry = UInt32(value)
        if _length != 0 {
            repeat {
                let sum = carry + UInt32(result[index])
                result[index] = UInt16(truncatingIfNeeded: sum)
                carry = sum >> 16
                index += 1
            } while index < result._length
        }
        if carry != 0 {
            if result._length > 7 {
                throw _CalculationError.overflow
            }
            result[index] = UInt16(carry)
            index += 1
        }
        result._length = index
        return result
    }

    // Native: NSDecimal._round(scale:roundingMode:), IDA 0x1c08cd000; iOS Simulator 27.0 (24A434), arm64.
    // Foundation UUID: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified.
    // iOS Device 27.0.1 (24A446), arm64e, IDA 0x18119b668.
    // Foundation UUID: C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified arithmetic branches/constants.
    // iOS Device divergence: the function at the device address above uses authenticated calls/returns.
    // Reading: Documentation/Reference/Research/FoundationDecimal.md.
    func _round(scale: Int, roundingMode: ONSRoundingMode) throws -> ONSDecimal {
        let difference = scale + Int(_exponent)
        if scale == 32767 || difference >= 0 {
            return self
        }
        var digits = -difference
        var targetExponent = Int(_exponent) + digits
        var result = self
        var remainder: UInt16 = 0
        var sticky = false
        while digits >= 5 {
            let divided = try result._divide(by: UInt16(10000))
            if remainder != 0 {
                sticky = true
            }
            result = divided.result
            remainder = divided.remainder
            digits -= 4
        }
        repeat {
            if remainder != 0 {
                sticky = true
            }
            let divided = try result._divide(by: UInt16(10))
            result = divided.result
            remainder = divided.remainder
            digits -= 1
        } while digits != 0
        if sticky && (remainder == 5 || remainder == 0) {
            remainder += 1
        }
        if sticky || remainder != 0 {
            let isEven = result._mantissa.0 & 1 == 0
            if _isNegative != 0 {
                if (roundingMode.rawValue == 0 && remainder >= 5) || roundingMode.rawValue == 1
                    || (roundingMode.rawValue == 3 && remainder >= 5 && !(isEven && remainder == 5))
                {
                    result = try result._add(1)
                }
                if result._length == 0 {
                    result._isNegative = 0
                }
            } else if (roundingMode.rawValue == 0 && remainder >= 5) || roundingMode.rawValue == 2
                || (roundingMode.rawValue == 3 && remainder >= 5 && !(isEven && remainder == 5))
            {
                result = try result._add(1)
            }
        }
        result._isCompact = 0
        if targetExponent >= 128 {
            repeat {
                result = try result._multiply(byShort: 10)
                targetExponent -= 1
            } while targetExponent >= 128
            targetExponent = 127
        }
        result._exponent = Int32(targetExponent)
        var exponent = result._exponent
        if result._isCompact == 0 && (result._length != 0 || result._isNegative == 0) && result._length != 0 {
            var remainder: UInt16 = 0
            repeat {
                #sourceLocation(file:"Foundation/Decimal+Math.swift",line:542)
                let divided = try! result._divide(by: UInt16(10))
                #sourceLocation()
                result = divided.result
                remainder = divided.remainder
                exponent += 1
                if remainder != 0 {
                    break
                }
            } while result._length != 0
            if remainder == 0 && result._length == 0 {
                result = ONSDecimal()
            } else {
                #sourceLocation(file:"Foundation/Decimal+Math.swift",line:553)
                result = try! result._multiply(byShort: 10)
                #sourceLocation(file:"Foundation/Decimal+Math.swift",line:554)
                result = try! result._add(remainder)
                #sourceLocation()
                exponent -= 1
                if exponent >= 128 {
                    repeat {
                        #sourceLocation(file:"Foundation/Decimal+Math.swift",line:559)
                        result = try! result._multiply(byShort: 10)
                        #sourceLocation()
                        exponent -= 1
                    } while exponent >= 128
                    exponent = 127
                }
                result._exponent = exponent
                result._isCompact = 1
            }
        }
        return result
    }

    // Native: NSDecimal._normalize(a:b:roundingMode:), IDA 0x1c08cef54; iOS Simulator 27.0 (24A434), arm64.
    // Foundation UUID: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified.
    // iOS Device 27.0.1 (24A446), arm64e, IDA 0x180cc3f40.
    // Foundation UUID: C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified arithmetic branches/constants.
    // iOS Device divergence: the function at the device address above uses authenticated calls/returns.
    // Reading: Documentation/Reference/Research/FoundationDecimal.md.
    static func _normalize(
        a: inout ONSDecimal, b: inout ONSDecimal, roundingMode _: ONSRoundingMode
    ) throws -> Bool {
        let difference = a._exponent - b._exponent
        if difference == 0 {
            return false
        }
        let firstIsHigher = difference >= 0
        var higher = firstIsHigher ? a : b
        var lower = firstIsHigher ? b : a
        let power = Int(difference < 0 ? -difference : difference)
        let higherInteger = higher.asVariableLengthInteger()
        let multiplied: VariableLengthInteger
        do {
            multiplied = try _integerMultiplyByPowerOfTen(
                lhs: higherInteger, power: power, maxResultLength: 8)
        } catch {
            let currentHigherInteger = higher.asVariableLengthInteger()
            let estimate =
                (Double(8 - currentHigherInteger.length) * Double(bitPattern: 0x4013_4413_5094_c2f0)).rounded(
                    .down)
            let availableDigits = Int(estimate)
            let lowerInteger = lower.asVariableLengthInteger()
            let lowerPower = availableDigits - power
            let lowered = try _integerMultiplyByPowerOfTen(
                lhs: lowerInteger, power: lowerPower, maxResultLength: 8)
            try lower.copyVariableLengthInteger(lowered)
            lower._length = UInt32(Int32(lowered.length))
            lower._exponent = lower._exponent - Int32(lowerPower)
            lower._isCompact = 0
            if firstIsHigher {
                b = lower
            } else {
                a = lower
            }
            if lower._length == 0 {
                lower._exponent = higher._exponent
                if firstIsHigher {
                    b = lower
                } else {
                    a = lower
                }
                return true
            }
            let currentInteger = higher.asVariableLengthInteger()
            let raised = try _integerMultiplyByPowerOfTen(
                lhs: currentInteger, power: availableDigits, maxResultLength: 8)
            try higher.copyVariableLengthInteger(raised)
            higher._length = UInt32(raised.length)
            higher._exponent = higher._exponent - Int32(availableDigits)
            higher._isCompact = 0
            if firstIsHigher {
                a = higher
            } else {
                b = higher
            }
            return true
        }
        try higher.copyVariableLengthInteger(multiplied)
        higher._length = UInt32(multiplied.length)
        higher._exponent = lower._exponent
        higher._isCompact = 0
        if firstIsHigher {
            a = higher
        } else {
            b = higher
        }
        return false
    }

    // Native: NSDecimal._multiply(by:roundingMode:), IDA 0x1c08cc898; iOS Simulator 27.0 (24A434), arm64.
    // Foundation UUID: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified.
    // iOS Device 27.0.1 (24A446), arm64e, IDA 0x18119aef8.
    // Foundation UUID: C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified arithmetic branches/constants.
    // iOS Device divergence: the function at the device address above uses authenticated calls/returns.
    // Reading: Documentation/Reference/Research/FoundationDecimal.md.
    func _multiply(by other: ONSDecimal, roundingMode: ONSRoundingMode) throws -> ONSDecimal {
        if (_length == 0 && _isNegative != 0) || (other._length == 0 && other._isNegative != 0) {
            throw _CalculationError.overflow
        }
        if _length == 0 || other._length == 0 {
            return ONSDecimal()
        }
        let left = asVariableLengthInteger()
        let right = other.asVariableLengthInteger()
        var product = try Self._integerMultiply(lhs: left, rhs: right, maxResultLength: 16)
        var result = ONSDecimal()
        result._isNegative = _isNegative != other._isNegative ? 1 : 0
        var targetExponent = _exponent + other._exponent
        if product.length >= 9 {
            let fitted = try Self._fitMantissa(product, roundingMode: roundingMode)
            targetExponent += Int32(fitted.exponent)
            product = fitted.result
        }
        try result.copyVariableLengthInteger(product)
        result._length = UInt32(product.length)
        result._isCompact = 0
        if targetExponent >= 128 {
            throw _CalculationError.overflow
        }
        // Native only checks the upper bound; the signed 8-bit setter preserves lower-bound wrapping.
        result._exponent = targetExponent
        var exponent = result._exponent
        if result._isCompact == 0 && (result._length != 0 || result._isNegative == 0) && result._length != 0 {
            var remainder: UInt16 = 0
            repeat {
                #sourceLocation(file:"Foundation/Decimal+Math.swift",line:542)
                let divided = try! result._divide(by: UInt16(10))
                #sourceLocation()
                result = divided.result
                remainder = divided.remainder
                exponent += 1
                if remainder != 0 {
                    break
                }
            } while result._length != 0
            if remainder == 0 && result._length == 0 {
                result = ONSDecimal()
            } else {
                #sourceLocation(file:"Foundation/Decimal+Math.swift",line:553)
                result = try! result._multiply(byShort: 10)
                #sourceLocation(file:"Foundation/Decimal+Math.swift",line:554)
                result = try! result._add(remainder)
                #sourceLocation()
                exponent -= 1
                if exponent >= 128 {
                    repeat {
                        #sourceLocation(file:"Foundation/Decimal+Math.swift",line:559)
                        result = try! result._multiply(byShort: 10)
                        #sourceLocation()
                        exponent -= 1
                    } while exponent >= 128
                    exponent = 127
                }
                result._exponent = exponent
                result._isCompact = 1
            }
        }
        return result
    }

    // Native: NSDecimal._divide(by:roundingMode:), IDA 0x1c08d0038; iOS Simulator 27.0 (24A434), arm64.
    // Foundation UUID: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified.
    // iOS Device 27.0.1 (24A446), arm64e, IDA 0x180ebdf74.
    // Foundation UUID: C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified arithmetic branches/constants.
    // iOS Device divergence: the function at the device address above uses authenticated calls/returns.
    // Reading: Documentation/Reference/Research/FoundationDecimal.md.
    func _divide(by other: ONSDecimal, roundingMode: ONSRoundingMode) throws -> ONSDecimal {
        if (_length == 0 && _isNegative != 0) || (other._length == 0 && other._isNegative != 0) {
            throw _CalculationError.overflow
        }
        if other._length == 0 {
            throw _CalculationError.divideByZero
        }
        if _length == 0 {
            return ONSDecimal()
        }
        var numerator = self
        var denominator = other
        if numerator._exponent - denominator._exponent >= 19 {
            _ = try Self._normalize(a: &numerator, b: &denominator, roundingMode: roundingMode)
            if numerator._length == 0 || denominator._length == 0 {
                numerator = self
                denominator = other
            }
        }
        var scaled = numerator.asVariableLengthInteger()
        let tableCount = powerOfTen.count
        var power = 38
        if tableCount <= power {
            let last = powerOfTen[tableCount - 1]
            repeat {
                scaled = try Self._integerMultiply(lhs: scaled, rhs: last, maxResultLength: 16)
                power -= tableCount - 1
            } while tableCount <= power
        }
        let scaledProduct = try Self._integerMultiply(
            lhs: scaled, rhs: powerOfTen[power], maxResultLength: 16)
        let divisor = denominator.asVariableLengthInteger()
        let quotient = try Self._integerDivide(
            dividend: scaledProduct, divisor: divisor, maxResultLength: scaledProduct.length)
        // Native specialization ignores the caller's mode and fits with NSRoundDown.
        let fitted = try Self._fitMantissa(quotient, roundingMode: ONSRoundingMode(rawValue: 1)!)
        var result = ONSDecimal()
        try result.copyVariableLengthInteger(fitted.result)
        result._length = UInt32(fitted.result.length)
        result._isNegative = numerator._isNegative != denominator._isNegative ? 1 : 0
        let targetExponent = Int(numerator._exponent - denominator._exponent) - 38 + fitted.exponent
        if targetExponent < -128 {
            throw _CalculationError.underflow
        }
        if targetExponent >= 128 {
            throw _CalculationError.overflow
        }
        result._exponent = Int32(targetExponent)
        result._isCompact = 0
        var exponent = result._exponent
        if result._isCompact == 0 && (result._length != 0 || result._isNegative == 0) && result._length != 0 {
            var remainder: UInt16 = 0
            repeat {
                #sourceLocation(file:"Foundation/Decimal+Math.swift",line:542)
                let divided = try! result._divide(by: UInt16(10))
                #sourceLocation()
                result = divided.result
                remainder = divided.remainder
                exponent += 1
                if remainder != 0 {
                    break
                }
            } while result._length != 0
            if remainder == 0 && result._length == 0 {
                result = ONSDecimal()
            } else {
                #sourceLocation(file:"Foundation/Decimal+Math.swift",line:553)
                result = try! result._multiply(byShort: 10)
                #sourceLocation(file:"Foundation/Decimal+Math.swift",line:554)
                result = try! result._add(remainder)
                #sourceLocation()
                exponent -= 1
                if exponent >= 128 {
                    repeat {
                        #sourceLocation(file:"Foundation/Decimal+Math.swift",line:559)
                        result = try! result._multiply(byShort: 10)
                        #sourceLocation()
                        exponent -= 1
                    } while exponent >= 128
                    exponent = 127
                }
                result._exponent = exponent
                result._isCompact = 1
            }
        }
        return result
    }

    // Native: NSDecimal._multiplyByPowerOfTen(power:roundingMode:), IDA 0x1c08d0778; iOS Simulator 27.0 (24A434), arm64.
    // Foundation UUID: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified.
    // iOS Device 27.0.1 (24A446), arm64e, IDA 0x180e5981c.
    // Foundation UUID: C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Confidence: verified arithmetic branches/constants.
    // iOS Device divergence: the function at the device address above uses authenticated calls/returns.
    // Reading: Documentation/Reference/Research/FoundationDecimal.md.
    func _multiplyByPowerOfTen(power: Int, roundingMode _: ONSRoundingMode) throws -> ONSDecimal {
        if _length == 0 && _isNegative != 0 {
            throw _CalculationError.overflow
        }
        if _length == 0 {
            return ONSDecimal()
        }
        var result = self
        let targetExponent = _exponent + Int32(power)
        if targetExponent < -128 {
            throw _CalculationError.underflow
        }
        if targetExponent > 127 {
            throw _CalculationError.overflow
        }
        result._exponent = targetExponent
        return result
    }
}

// Native: _NSDecimalCompact C bridge 0x1c08be900 and shared Swift body 0x1c08c1388.
// iOS Simulator 27.0 (24A434), arm64.
// Foundation UUID: 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified.
// iOS Device 27.0.1 (24A446), arm64e, C bridge 0x180c9eac4 and shared body 0x180c9e7f0.
// Foundation UUID: C041F1D0-5EF1-30A3-BF96-830976173B56. Confidence: verified arithmetic.
// iOS Device divergence: _NSDecimalCompact at 0x180c9e7f0 authenticates calls/returns.
// Reading: Documentation/Reference/Research/FoundationDecimal.md.
@_cdecl("_ONSDecimalCompact")
func _ONSDecimalCompact(_ number: UnsafeMutablePointer<ONSDecimal>) {
    var result = number.pointee
    var exponent = result._exponent
    if result._isCompact == 0 && (result._length != 0 || result._isNegative == 0) && result._length != 0 {
        var remainder: UInt16 = 0
        repeat {
            #sourceLocation(file:"Foundation/Decimal+Math.swift",line:542)
            let divided = try! result._divide(by: UInt16(10))
            #sourceLocation()
            result = divided.result
            remainder = divided.remainder
            exponent += 1
            if remainder != 0 {
                break
            }
        } while result._length != 0
        if remainder == 0 && result._length == 0 {
            result = ONSDecimal()
        } else {
            #sourceLocation(file:"Foundation/Decimal+Math.swift",line:553)
            result = try! result._multiply(byShort: 10)
            #sourceLocation(file:"Foundation/Decimal+Math.swift",line:554)
            result = try! result._add(remainder)
            #sourceLocation()
            exponent -= 1
            if exponent >= 128 {
                repeat {
                    #sourceLocation(file:"Foundation/Decimal+Math.swift",line:559)
                    result = try! result._multiply(byShort: 10)
                    #sourceLocation()
                    exponent -= 1
                } while exponent >= 128
                exponent = 127
            }
            result._exponent = exponent
            result._isCompact = 1
        }
    }
    number.pointee = result
}

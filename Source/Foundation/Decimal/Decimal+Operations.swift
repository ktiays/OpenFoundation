//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// This is the exact native libswiftCore generic guard, not a replacement implementation.
// Foundation Decimal compare/add/subtract call it after their exhaustive comparison-result dispatch.
// Its mangled linkage remains verbatim in both naming modes; it is an external Swift runtime dependency.
@_silgen_name("$ss32_diagnoseUnexpectedEnumCaseValue4type03rawE0s5NeverOxm_q_tr0_lF")
private func _diagnoseUnexpectedEnumCaseValue<T, Raw>(type: T.Type, rawValue: Raw) -> Never

// Native _NSDecimalCompare(_:_:), 0x1c08c11c4 / iOS Device 0x180ec05bc.
// Foundation UUIDs 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
// iOS Device divergence: the function at the device address above authenticates calls/returns;
// the independently read arithmetic/error branches agree.
@_cdecl("_ONSDecimalCompare")
func _ONSDecimalCompare(
    _ leftOperand: UnsafePointer<ONSDecimal>, _ rightOperand: UnsafePointer<ONSDecimal>
) -> ONSComparisonResult {
    let left = leftOperand.pointee
    let right = rightOperand.pointee
    if left._length == 0 && left._isNegative != 0 {
        if right._length == 0 && right._isNegative != 0 {
            return ONSComparisonResult(rawValue: 0)!
        }
        return ONSComparisonResult(rawValue: -1)!
    }
    if right._length == 0 && right._isNegative != 0 {
        return ONSComparisonResult(rawValue: 1)!
    }
    if right._isNegative < left._isNegative {
        return ONSComparisonResult(rawValue: -1)!
    }
    if left._isNegative < right._isNegative {
        return ONSComparisonResult(rawValue: 1)!
    }
    if left._length == 0 {
        return ONSComparisonResult(rawValue: right._length != 0 ? -1 : 0)!
    }
    if right._length == 0 {
        return ONSComparisonResult(rawValue: 1)!
    }
    var first = left
    var second = right
    _ = try? ONSDecimal._normalize(a: &first, b: &second, roundingMode: ONSRoundingMode(rawValue: 0)!)
    let comparison = ONSDecimal._integerCompare(
        lhs: first.asVariableLengthInteger(), rhs: second.asVariableLengthInteger()
    )
    if first._isNegative == 0 {
        return comparison
    }
    switch comparison.rawValue {
    case -1:
        return ONSComparisonResult(rawValue: 1)!
    case 0:
        return ONSComparisonResult(rawValue: 0)!
    case 1:
        return ONSComparisonResult(rawValue: -1)!
    default:
        _diagnoseUnexpectedEnumCaseValue(type: ONSComparisonResult.self, rawValue: comparison.rawValue)
    }
}

// Native _NSDecimalRound(_:_:_:_:), 0x1c08be858 / iOS Device 0x18118ed48.
// Foundation UUIDs 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
// iOS Device divergence: the function at the device address above authenticates calls/returns;
// the independently read arithmetic/error branches agree.
@_cdecl("_ONSDecimalRound")
func _ONSDecimalRound(
    _ result: UnsafeMutablePointer<ONSDecimal>, _ number: UnsafePointer<ONSDecimal>,
    _ scale: Int, _ roundingMode: ONSRoundingMode
) {
    do {
        result.pointee = try number.pointee._round(scale: scale, roundingMode: roundingMode)
    } catch {
        // The native C bridge releases the Swift error and leaves the caller's result untouched.
    }
}

// Native __NSDecimalNormalize(_:_:_:), 0x1c08c1920 / iOS Device 0x1811910a8.
// Foundation UUIDs 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
// iOS Device divergence: the function at the device address above authenticates calls/returns;
// the independently read arithmetic/error branches agree.
@_cdecl("_ONSDecimalNormalize")
func _ONSDecimalNormalize(
    _ number1: UnsafeMutablePointer<ONSDecimal>, _ number2: UnsafeMutablePointer<ONSDecimal>,
    _ roundingMode: ONSRoundingMode
) -> ONSCalculationError {
    var first = number1.pointee
    var second = number2.pointee
    do {
        let lossOfPrecision = try ONSDecimal._normalize(a: &first, b: &second, roundingMode: roundingMode)
        number1.pointee = first
        number2.pointee = second
        return ONSCalculationError(rawValue: lossOfPrecision ? 1 : 0)!
    } catch {
        if let known = error as? ONSDecimal._CalculationError {
            switch known {
            case .overflow:
                return ONSCalculationError(rawValue: 3)!
            case .underflow:
                return ONSCalculationError(rawValue: 2)!
            case .divideByZero:
                return ONSCalculationError(rawValue: 4)!
            }
        }
        return ONSCalculationError(rawValue: 0)!
    }
}

// Native NSDecimalAdd(_:_:_:_:), 0x1c08bf4cc / iOS Device 0x180c988dc.
// Foundation UUIDs 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
// iOS Device divergence: the function at the device address above authenticates calls/returns;
// the independently read arithmetic/error branches agree.
// swift-format-ignore: AlwaysUseLowerCamelCase
@_cdecl("_ONSDecimalAdd")
func ONSDecimalAdd(
    _ result: UnsafeMutablePointer<ONSDecimal>, _ leftOperand: UnsafePointer<ONSDecimal>,
    _ rightOperand: UnsafePointer<ONSDecimal>, _ roundingMode: ONSRoundingMode
) -> ONSCalculationError {
    let left = leftOperand.pointee
    let right = rightOperand.pointee
    do {
        if (left._length == 0 && left._isNegative != 0) || (right._length == 0 && right._isNegative != 0) {
            throw ONSDecimal._CalculationError.overflow
        }
        if left._length == 0 {
            result.pointee = right
            return ONSCalculationError(rawValue: 0)!
        }
        if right._length == 0 {
            result.pointee = left
            return ONSCalculationError(rawValue: 0)!
        }
        var first = left
        var second = right
        let lossOfPrecision = try ONSDecimal._normalize(a: &first, b: &second, roundingMode: roundingMode)
        if first._length == 0 {
            result.pointee = second
            return ONSCalculationError(rawValue: lossOfPrecision ? 1 : 0)!
        }
        if second._length == 0 {
            result.pointee = first
            return ONSCalculationError(rawValue: lossOfPrecision ? 1 : 0)!
        }
        var output = first
        var integer: ONSDecimal.VariableLengthInteger
        if first._isNegative == second._isNegative {
            output._isNegative = first._isNegative
            integer = try ONSDecimal._integerAdd(
                lhs: first.asVariableLengthInteger(), rhs: second.asVariableLengthInteger(),
                maxResultLength: 9
            )
            if integer.length > 8 {
                let fit = try ONSDecimal._fitMantissa(integer, roundingMode: roundingMode)
                let exponent = output._exponent + Int32(fit.exponent)
                if exponent > 127 {
                    throw ONSDecimal._CalculationError.overflow
                }
                output._exponent = exponent
                integer = fit.result
            }
            output._length = UInt32(integer.length)
        } else {
            let comparison = ONSDecimal._integerCompare(
                lhs: first.asVariableLengthInteger(), rhs: second.asVariableLengthInteger()
            )
            switch comparison.rawValue {
            case -1:
                integer = try ONSDecimal._integerSubtract(
                    term: second.asVariableLengthInteger(), subtrahend: first.asVariableLengthInteger(),
                    maxResultLength: 8
                )
                output._length = UInt32(integer.length)
                output._isNegative = second._isNegative
            case 0:
                result.pointee = ONSDecimal()
                return ONSCalculationError(rawValue: lossOfPrecision ? 1 : 0)!
            case 1:
                integer = try ONSDecimal._integerSubtract(
                    term: first.asVariableLengthInteger(), subtrahend: second.asVariableLengthInteger(),
                    maxResultLength: 8
                )
                output._length = UInt32(integer.length)
                output._isNegative = first._isNegative
            default:
                _diagnoseUnexpectedEnumCaseValue(
                    type: ONSComparisonResult.self, rawValue: comparison.rawValue)
            }
        }
        try output.copyVariableLengthInteger(integer)
        output._isCompact = 0
        var exponent = output._exponent
        if output._isCompact == 0 && !(output._length == 0 && output._isNegative != 0) && output._length != 0
        {
            var remainder: UInt16 = 0
            repeat {
                #sourceLocation(file:"Foundation/Decimal+Math.swift",line:542)
                let division = try! output._divide(by: UInt16(10))
                #sourceLocation()
                output = division.result
                exponent += 1
                remainder = division.remainder
            } while remainder == 0 && output._length != 0
            if remainder == 0 && output._length == 0 {
                output = ONSDecimal()
            } else {
                #sourceLocation(file:"Foundation/Decimal+Math.swift",line:553)
                output = try! output._multiply(byShort: 10)
                #sourceLocation(file:"Foundation/Decimal+Math.swift",line:554)
                output = try! output._add(remainder)
                #sourceLocation()
                exponent -= 1
                while exponent >= 128 {
                    #sourceLocation(file:"Foundation/Decimal+Math.swift",line:559)
                    output = try! output._multiply(byShort: 10)
                    #sourceLocation()
                    exponent -= 1
                }
                output._exponent = exponent
                output._isCompact = 1
            }
        }
        result.pointee = output
        return ONSCalculationError(rawValue: lossOfPrecision ? 1 : 0)!
    } catch {
        let calculationError: ONSCalculationError
        if let known = error as? ONSDecimal._CalculationError {
            switch known {
            case .overflow:
                calculationError = ONSCalculationError(rawValue: 3)!
            case .underflow:
                calculationError = ONSCalculationError(rawValue: 2)!
            case .divideByZero:
                calculationError = ONSCalculationError(rawValue: 4)!
            }
        } else {
            calculationError = ONSCalculationError(rawValue: 0)!
        }
        var invalid = ONSDecimal()
        invalid._exponent = 0
        invalid._length = 0
        invalid._isNegative = 1
        invalid._isCompact = 0
        invalid._reserved = 0
        result.pointee = invalid
        return calculationError
    }
}

// Native NSDecimalSubtract(_:_:_:_:), 0x1c08bfd24 / iOS Device 0x18118faa4.
// Foundation UUIDs 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
// iOS Device divergence: the function at the device address above authenticates calls/returns;
// the independently read arithmetic/error branches agree.
// swift-format-ignore: AlwaysUseLowerCamelCase
@_cdecl("_ONSDecimalSubtract")
func ONSDecimalSubtract(
    _ result: UnsafeMutablePointer<ONSDecimal>, _ leftOperand: UnsafePointer<ONSDecimal>,
    _ rightOperand: UnsafePointer<ONSDecimal>, _ roundingMode: ONSRoundingMode
) -> ONSCalculationError {
    var right = rightOperand.pointee
    if right._length != 0 {
        right._isNegative ^= 1
    }
    let left = leftOperand.pointee
    do {
        if (left._length == 0 && left._isNegative != 0) || (right._length == 0 && right._isNegative != 0) {
            throw ONSDecimal._CalculationError.overflow
        }
        if left._length == 0 {
            result.pointee = right
            return ONSCalculationError(rawValue: 0)!
        }
        if right._length == 0 {
            result.pointee = left
            return ONSCalculationError(rawValue: 0)!
        }
        var first = left
        var second = right
        let lossOfPrecision = try ONSDecimal._normalize(a: &first, b: &second, roundingMode: roundingMode)
        _ = lossOfPrecision
        if first._length == 0 {
            result.pointee = second
            return ONSCalculationError(rawValue: 0)!
        }
        if second._length == 0 {
            result.pointee = first
            return ONSCalculationError(rawValue: 0)!
        }
        var output = first
        var integer: ONSDecimal.VariableLengthInteger
        if first._isNegative == second._isNegative {
            output._isNegative = first._isNegative
            integer = try ONSDecimal._integerAdd(
                lhs: first.asVariableLengthInteger(), rhs: second.asVariableLengthInteger(),
                maxResultLength: 9
            )
            if integer.length > 8 {
                let fit = try ONSDecimal._fitMantissa(integer, roundingMode: roundingMode)
                let exponent = output._exponent + Int32(fit.exponent)
                if exponent > 127 {
                    throw ONSDecimal._CalculationError.overflow
                }
                output._exponent = exponent
                integer = fit.result
            }
            output._length = UInt32(integer.length)
        } else {
            let comparison = ONSDecimal._integerCompare(
                lhs: first.asVariableLengthInteger(), rhs: second.asVariableLengthInteger()
            )
            switch comparison.rawValue {
            case -1:
                integer = try ONSDecimal._integerSubtract(
                    term: second.asVariableLengthInteger(), subtrahend: first.asVariableLengthInteger(),
                    maxResultLength: 8
                )
                output._length = UInt32(integer.length)
                output._isNegative = second._isNegative
            case 0:
                result.pointee = ONSDecimal()
                return ONSCalculationError(rawValue: 0)!
            case 1:
                integer = try ONSDecimal._integerSubtract(
                    term: first.asVariableLengthInteger(), subtrahend: second.asVariableLengthInteger(),
                    maxResultLength: 8
                )
                output._length = UInt32(integer.length)
                output._isNegative = first._isNegative
            default:
                _diagnoseUnexpectedEnumCaseValue(
                    type: ONSComparisonResult.self, rawValue: comparison.rawValue)
            }
        }
        try output.copyVariableLengthInteger(integer)
        output._isCompact = 0
        var exponent = output._exponent
        if output._isCompact == 0 && !(output._length == 0 && output._isNegative != 0) && output._length != 0
        {
            var remainder: UInt16 = 0
            repeat {
                #sourceLocation(file:"Foundation/Decimal+Math.swift",line:542)
                let division = try! output._divide(by: UInt16(10))
                #sourceLocation()
                output = division.result
                exponent += 1
                remainder = division.remainder
            } while remainder == 0 && output._length != 0
            if remainder == 0 && output._length == 0 {
                output = ONSDecimal()
            } else {
                #sourceLocation(file:"Foundation/Decimal+Math.swift",line:553)
                output = try! output._multiply(byShort: 10)
                #sourceLocation(file:"Foundation/Decimal+Math.swift",line:554)
                output = try! output._add(remainder)
                #sourceLocation()
                exponent -= 1
                while exponent >= 128 {
                    #sourceLocation(file:"Foundation/Decimal+Math.swift",line:559)
                    output = try! output._multiply(byShort: 10)
                    #sourceLocation()
                    exponent -= 1
                }
                output._exponent = exponent
                output._isCompact = 1
            }
        }
        result.pointee = output
        return ONSCalculationError(rawValue: 0)!
    } catch {
        let calculationError: ONSCalculationError
        if let known = error as? ONSDecimal._CalculationError {
            switch known {
            case .overflow:
                calculationError = ONSCalculationError(rawValue: 3)!
            case .underflow:
                calculationError = ONSCalculationError(rawValue: 2)!
            case .divideByZero:
                calculationError = ONSCalculationError(rawValue: 4)!
            }
        } else {
            calculationError = ONSCalculationError(rawValue: 0)!
        }
        var invalid = ONSDecimal()
        invalid._exponent = 0
        invalid._length = 0
        invalid._isNegative = 1
        invalid._isCompact = 0
        invalid._reserved = 0
        result.pointee = invalid
        return calculationError
    }
}

// Native NSDecimalMultiply(_:_:_:_:), 0x1c08c059c / iOS Device 0x18119031c.
// Foundation UUIDs 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
// iOS Device divergence: the function at the device address above authenticates calls/returns;
// the independently read arithmetic/error branches agree.
// swift-format-ignore: AlwaysUseLowerCamelCase
@_cdecl("_ONSDecimalMultiply")
func ONSDecimalMultiply(
    _ result: UnsafeMutablePointer<ONSDecimal>, _ leftOperand: UnsafePointer<ONSDecimal>,
    _ rightOperand: UnsafePointer<ONSDecimal>, _ roundingMode: ONSRoundingMode
) -> ONSCalculationError {
    do {
        result.pointee = try leftOperand.pointee._multiply(
            by: rightOperand.pointee, roundingMode: roundingMode)
        return ONSCalculationError(rawValue: 0)!
    } catch {
        let calculationError: ONSCalculationError
        if let known = error as? ONSDecimal._CalculationError {
            switch known {
            case .overflow:
                calculationError = ONSCalculationError(rawValue: 3)!
            case .underflow:
                calculationError = ONSCalculationError(rawValue: 2)!
            case .divideByZero:
                calculationError = ONSCalculationError(rawValue: 4)!
            }
        } else {
            calculationError = ONSCalculationError(rawValue: 0)!
        }
        var invalid = ONSDecimal()
        invalid._exponent = 0
        invalid._length = 0
        invalid._isNegative = 1
        invalid._isCompact = 0
        invalid._reserved = 0
        result.pointee = invalid
        return calculationError
    }
}

// Native _NSDecimalDivide(_:_:_:_:), 0x1c08c16a0 / iOS Device 0x180ec038c.
// Foundation UUIDs 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
// iOS Device divergence: the function at the device address above authenticates calls/returns;
// the independently read arithmetic/error branches agree.
@_cdecl("_ONSDecimalDivide")
func _ONSDecimalDivide(
    _ result: UnsafeMutablePointer<ONSDecimal>, _ leftOperand: UnsafePointer<ONSDecimal>,
    _ rightOperand: UnsafePointer<ONSDecimal>, _ roundingMode: ONSRoundingMode
) -> ONSCalculationError {
    do {
        result.pointee = try leftOperand.pointee._divide(by: rightOperand.pointee, roundingMode: roundingMode)
        return ONSCalculationError(rawValue: 0)!
    } catch {
        let calculationError: ONSCalculationError
        if let known = error as? ONSDecimal._CalculationError {
            switch known {
            case .overflow:
                calculationError = ONSCalculationError(rawValue: 3)!
            case .underflow:
                calculationError = ONSCalculationError(rawValue: 2)!
            case .divideByZero:
                calculationError = ONSCalculationError(rawValue: 4)!
            }
        } else {
            calculationError = ONSCalculationError(rawValue: 0)!
        }
        var invalid = ONSDecimal()
        invalid._exponent = 0
        invalid._length = 0
        invalid._isNegative = 1
        invalid._isCompact = 0
        invalid._reserved = 0
        result.pointee = invalid
        return calculationError
    }
}

// Native _NSDecimalMultiplyByPowerOf10(_:_:_:_:), 0x1c08c17e0 / iOS Device 0x180e59950.
// Foundation UUIDs 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
// iOS Device divergence: the function at the device address above authenticates calls/returns;
// the independently read arithmetic/error branches agree.
@_cdecl("_ONSDecimalMultiplyByPowerOf10")
func _ONSDecimalMultiplyByPowerOf10(
    _ result: UnsafeMutablePointer<ONSDecimal>, _ number: UnsafePointer<ONSDecimal>,
    _ power: Int16, _ roundingMode: ONSRoundingMode
) -> ONSCalculationError {
    do {
        result.pointee = try number.pointee._multiplyByPowerOfTen(
            power: Int(power), roundingMode: roundingMode)
        return ONSCalculationError(rawValue: 0)!
    } catch {
        let calculationError: ONSCalculationError
        if let known = error as? ONSDecimal._CalculationError {
            switch known {
            case .overflow:
                calculationError = ONSCalculationError(rawValue: 3)!
            case .underflow:
                calculationError = ONSCalculationError(rawValue: 2)!
            case .divideByZero:
                calculationError = ONSCalculationError(rawValue: 4)!
            }
        } else {
            calculationError = ONSCalculationError(rawValue: 0)!
        }
        var invalid = ONSDecimal()
        invalid._exponent = 0
        invalid._length = 0
        invalid._isNegative = 1
        invalid._isCompact = 0
        invalid._reserved = 0
        result.pointee = invalid
        return calculationError
    }
}

// Native NSDecimalPower(_:_:_:_:), 0x1c08c081c / iOS Device 0x1811905a4.
// Foundation UUIDs 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 / C041F1D0-5EF1-30A3-BF96-830976173B56.
// Confidence: verified. Reading: Documentation/Reference/Research/FoundationDecimal.md.
// iOS Device divergence: the function at the device address above authenticates calls/returns;
// the independently read arithmetic/error branches agree.
// The native C NSUInteger argument enters this Swift Int boundary without conversion.
// Foundation defect: native negation of Int.min at 0x1c08c0bb4 traps at 0x1c08c102c; device
// negation at 0x1811909c0 traps at 0x181190e90. Confidence: verified in both captured streams.
// After native NaN/zero/zero-power paths, return overflow plus NaN for that single exponent instead.
// The destructive native Int.min exponent case is excluded from differential tests.
// swift-format-ignore: AlwaysUseLowerCamelCase
@_cdecl("_ONSDecimalPower")
func ONSDecimalPower(
    _ result: UnsafeMutablePointer<ONSDecimal>, _ number: UnsafePointer<ONSDecimal>,
    _ power: Int, _ roundingMode: ONSRoundingMode
) -> ONSCalculationError {
    var base = number.pointee
    do {
        if base._length == 0 && base._isNegative != 0 {
            throw ONSDecimal._CalculationError.overflow
        }
        if power == 0 {
            result.pointee = ONSDecimal(UInt64(1))
            return ONSCalculationError(rawValue: 0)!
        }
        // The native emitted equality check first compares all fields to canonical zero,
        // then inlines comparison against that same zero. Its zero result is precisely this
        // finite zero-length case; no mantissa or reserved-field normalization is performed.
        if base._length == 0 {
            var zero = ONSDecimal()
            if power <= 0 {
                zero._isNegative = 1
            }
            result.pointee = zero
            return ONSCalculationError(rawValue: 0)!
        }
        if power == Int.min {
            throw ONSDecimal._CalculationError.overflow
        }
        var remaining = power < 0 ? -power : power
        var accumulated = ONSDecimal(UInt64(1))
        while remaining >= 2 {
            if remaining & 1 != 0 {
                accumulated = try accumulated._multiply(by: base, roundingMode: roundingMode)
                remaining -= 1
            }
            base = try base._multiply(by: base, roundingMode: roundingMode)
            remaining = Int(UInt(remaining) >> 1)
        }
        var value = try accumulated._multiply(by: base, roundingMode: roundingMode)
        if power < 0 {
            value = try ONSDecimal(UInt64(1))._divide(by: value, roundingMode: roundingMode)
        }
        result.pointee = value
        return ONSCalculationError(rawValue: 0)!
    } catch {
        let calculationError: ONSCalculationError
        if let known = error as? ONSDecimal._CalculationError {
            switch known {
            case .overflow:
                calculationError = ONSCalculationError(rawValue: 3)!
            case .underflow:
                calculationError = ONSCalculationError(rawValue: 2)!
            case .divideByZero:
                calculationError = ONSCalculationError(rawValue: 4)!
            }
        } else {
            calculationError = ONSCalculationError(rawValue: 0)!
        }
        var invalid = ONSDecimal()
        invalid._exponent = 0
        invalid._length = 0
        invalid._isNegative = 1
        invalid._isCompact = 0
        invalid._reserved = 0
        result.pointee = invalid
        return calculationError
    }
}

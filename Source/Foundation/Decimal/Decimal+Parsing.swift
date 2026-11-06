//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// iOS Simulator 27.0 (24A434), arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from captured assembly. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Scratch/cf-core-fd-misc/Reading.md. Native Swift source organization inferred.

extension ONSDecimal {
    // The captured return has a 32-byte success payload (decimal and consumed length), a byte tag at 32,
    // and failure payload 0 (no prefix) or 1 (out of range). Case/error spelling is inferred, not read metadata.
    enum DecimalParseResult {
        case success(ONSDecimal, Int)
        case failure(Int)
    }

    // Native static NSDecimal._decimal(from:decimalSeparator:matchEntireString:), 0x1c08bc604.
    // Non-external specialization; the generic source induces the compiler's UTF8View specialization.
    // Imported C-struct accessors are compiler generated: exponent getter 0x1c0820cd0, length getter
    // 0x1c0820cd8, isNegative getter 0x1c0820ce4, isCompact getter 0x1c0820cf0; exponent setter
    // 0x1c08bddd8, length setter 0x1c08bdde0, isNegative setter 0x1c08bddf0, isCompact setter
    // 0x1c08bde04 and reserved setter 0x1c08bde18. Never handwritten.
    // _CalculationError witness accessor 0x1c08c1660 is induced by catching the primitive errors.
    static func _decimal<C: Collection>(
        from bytes: C, decimalSeparator: String.UTF8View, matchEntireString: Bool
    ) -> DecimalParseResult where C.Element == UInt8 {
        // Native nested skipWhiteSpaces(from:), UTF8View specialization 0x1c08bd51c.
        // Each byte becomes a Unicode scalar before Character.isWhitespace (kept), not a UTF-8 decode.
        func skipWhiteSpaces(from start: C.Index) -> C.Index {
            var index = start
            while index != bytes.endIndex {
                if !Character(Unicode.Scalar(bytes[index])).isWhitespace {
                    return index
                }
                bytes.formIndex(after: &index)
            }
            return index
        }

        // Native nested stringViewContainsDecimalSeparator(at:), specialization 0x1c08bd8c4.
        // Foundation defect: 0x1c08bda20-0x1c08bda24 subscripts at/past end when a multi-byte separator
        // matches a truncated input prefix. Return false for that prefix instead of trapping.
        func stringViewContainsDecimalSeparator(at index: C.Index) -> Bool {
            for offset in 0..<decimalSeparator.count {
                guard let source = bytes.index(index, offsetBy: offset, limitedBy: bytes.endIndex),
                    source != bytes.endIndex
                else {
                    return false
                }
                let separator = decimalSeparator.index(decimalSeparator.startIndex, offsetBy: offset)
                if bytes[source] != decimalSeparator[separator] {
                    return false
                }
            }
            return true
        }

        var value = ONSDecimal()
        var index = skipWhiteSpaces(from: bytes.startIndex)
        if index != bytes.endIndex && (bytes[index] == 43 || bytes[index] == 45) {
            value._isNegative = bytes[index] == 45 ? 1 : 0
            bytes.formIndex(after: &index)
        }
        var fullMantissa = false
        while index != bytes.endIndex {
            let digit = bytes[index] &- 48
            if digit >= 10 {
                break
            }
            if !fullMantissa {
                do {
                    let multiplied = try value._multiply(byShort: 10)
                    value = try multiplied._add(UInt16(digit))
                    bytes.formIndex(after: &index)
                    continue
                } catch {
                    fullMantissa = true
                }
            }
            if value._exponent == 127 {
                value = ONSDecimal()
                value._isNegative = 1
            } else {
                value._exponent += 1
            }
            // Native defer specialization 0x1c08bdb9c advances even on this failing iteration.
            defer { bytes.formIndex(after: &index) }
            if value._length == 0 && value._isNegative != 0 {
                return .failure(1)
            }
        }
        if index != bytes.endIndex && stringViewContainsDecimalSeparator(at: index) {
            // Compiler copies Collection.formIndex(_:offsetBy:) 0x1c08bbb64 and
            // _StringGuts._slowEnsureMatchingEncoding 0x1c07e225c arise from these native index operations.
            bytes.formIndex(&index, offsetBy: decimalSeparator.count)
            while index != bytes.endIndex {
                let digit = bytes[index] &- 48
                if digit >= 10 {
                    break
                }
                defer { bytes.formIndex(after: &index) }
                if fullMantissa {
                    continue
                }
                do {
                    let multiplied = try value._multiply(byShort: 10)
                    value = try multiplied._add(UInt16(digit))
                } catch {
                    fullMantissa = true
                    continue
                }
                if value._exponent == -128 {
                    return .failure(1)
                }
                value._exponent -= 1
            }
        }
        if index != bytes.endIndex && (bytes[index] == 69 || bytes[index] == 101) {
            bytes.formIndex(after: &index)
            // An exponent marker at end returns immediately without compacting (kept).
            if index == bytes.endIndex {
                return .success(value, bytes.distance(from: bytes.startIndex, to: index))
            }
            var negative = false
            if bytes[index] == 45 || bytes[index] == 43 {
                negative = bytes[index] == 45
                bytes.formIndex(after: &index)
            }
            var exponent = 0
            while index != bytes.endIndex {
                let digit = bytes[index] &- 48
                if digit >= 10 {
                    break
                }
                exponent = exponent * 10 + Int(digit)
                if exponent > 254 {
                    return .failure(1)
                }
                bytes.formIndex(after: &index)
            }
            if negative {
                exponent = -exponent
            }
            exponent += Int(value._exponent)
            if Int(Int8(truncatingIfNeeded: exponent)) != exponent {
                return .failure(1)
            }
            value._exponent = Int32(exponent)
        }
        if matchEntireString {
            index = skipWhiteSpaces(from: index)
            if index != bytes.endIndex {
                return .failure(0)
            }
        }
        if index == bytes.startIndex {
            return .failure(0)
        }
        value.compact()
        let length = bytes.distance(from: bytes.startIndex, to: index)
        if value._length == 0 && value._isNegative != 0 {
            return .success(ONSDecimal(UInt64(0)), length)
        }
        return .success(value, length)
    }

    // Native NSDecimal.compact(), 0x1c08cc490. Non-external Swift method.
    // The InlineArray(repeating:) copy at 0x1c08c1b14 is compiler support for the existing
    // VariableLengthInteger initialization used by these arithmetic operations; no handwritten symbol.
    mutating func compact() {
        var exponent = _exponent
        if _isCompact != 0 || (_length == 0 && _isNegative != 0) {
            return
        }
        var temporary = self
        if temporary._length == 0 {
            return
        }
        var remainder: UInt16 = 0
        repeat {
            #sourceLocation(file:"Foundation/Decimal+Math.swift",line:542)
            let divided = try! self._divide(by: 10)
            #sourceLocation()
            self = divided.result
            remainder = divided.remainder
            exponent += 1
            if remainder != 0 {
                break
            }
            temporary = self
        } while temporary._length != 0
        if remainder == 0 && _length == 0 {
            self = ONSDecimal()
        } else {
            #sourceLocation(file:"Foundation/Decimal+Math.swift",line:553)
            self = try! self._multiply(byShort: 10)
            #sourceLocation(file:"Foundation/Decimal+Math.swift",line:554)
            self = try! self._add(remainder)
            #sourceLocation()
            exponent -= 1
            if exponent >= 128 {
                repeat {
                    #sourceLocation(file:"Foundation/Decimal+Math.swift",line:559)
                    self = try! self._multiply(byShort: 10)
                    #sourceLocation()
                    exponent -= 1
                } while exponent >= 128
                exponent = 127
            }
            _exponent = exponent
            _isCompact = 1
        }
    }
}

// Native _NSStringToDecimal, 0x1c08bf420. Was a private external, C entry to a Swift body.
// Calls the local restored String bridge, parses '.', accepts a prefix, writes length=0 on failure,
// and leaves the caller's decimal untouched on failure (assembly differs from a zero-result assumption).
@_cdecl("_NSStringToDecimal")
func _NSStringToDecimal(
    _ object: UnsafeRawPointer?, _ length: UnsafeMutablePointer<UInt>,
    _ result: UnsafeMutablePointer<ONSDecimal>
) {
    let string = String._unconditionallyBridgeFromObjectiveC(object)
    switch ONSDecimal._decimal(from: string.utf8, decimalSeparator: ".".utf8, matchEntireString: false) {
    case .failure:
        length.pointee = 0
    case .success(let value, let count):
        length.pointee = UInt(count)
        result.pointee = value
    }
}

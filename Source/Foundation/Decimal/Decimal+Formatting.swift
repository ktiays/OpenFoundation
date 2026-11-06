//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation, iOS Simulator 27.0 (24A434), arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified except the locale-key GOT binding (medium).
// Reading: Foundation/Number/Reading.md, round 2.

// Swift source adapter for the native Locale cast and witness call at 0x1c08be974–0x1c08be9b8.
// Opening the selected module's metatype leaves the cast and lifetime operations to Swift.
private func decimalLocaleSeparator<T>(_ locale: Any, as type: T.Type) -> String? {
    guard let value = locale as? T else {
        return nil
    }
    return withUnsafeBytes(of: value) { bytes in
        withUnsafeTemporaryAllocation(of: Optional<String>.self, capacity: 1) { result in
            decimalLocaleReadSeparator(bytes.baseAddress!, result.baseAddress!)
            return result.baseAddress!.move()
        }
    }
}

// Native __NSDecimalString(_:_:), 0x1c08be914. Hidden (was a private external).
// iOS Device divergence: 0x18118ee04 (24A446) authenticates the locale witness call at 0x18118eea8.
// Confidence: verified for this call boundary. Full iOS Device behavior comparison remains open.
// The Optional<Any> cast operands account for the outlined copy and destroy at 0x1c08c1a70 and 0x1c08c1a28.
// The two Dictionary<AnyHashable, Any> subscripts account for the find specializations at
// 0x1c06cdbd8 and 0x1c06d01ac, the latter's AnyHashable copy at 0x1c06eaeb4,
// and the key destroys at 0x1c06eaef0. The lookup values use the Any copy thunk at 0x1c06eb9b8
// and its outlined body at 0x1c06ea91c. These helpers are compiler-generated.
// String.init on the reversed collection induces the witness accessor at 0x1c08c1ac0
// and its abstract mangled-type metadata cache at 0x1c06fc930.
// swift-format-ignore: AlwaysUseLowerCamelCase
private func __ONSDecimalString(_ number: UnsafePointer<ONSDecimal>, _ locale: Any?) -> String {
    var separator = "."
    if let locale {
        let type = unsafeBitCast(decimalLocaleTypeMetadata()!, to: Any.Type.self)
        separator = decimalLocaleSeparator(locale, as: type) ?? "."
    }
    if let dictionary = locale as? [AnyHashable: Any] {
        if let value = dictionary[AnyHashable("NSDecimalSeparator")] as? String {
            separator = value
        }
        // The NSLocaleDecimalSeparator GOT load at 0x1c08beae0 uses the CoreFoundation key alias.
        let keyObject = kOCFLocaleDecimalSeparatorKey
        let keyPointer = withUnsafeBytes(of: keyObject) { $0.load(as: UnsafeRawPointer?.self) }
        let key = String._unconditionallyBridgeFromObjectiveC(keyPointer)
        if let value = dictionary[AnyHashable(key)] as? String {
            separator = value
        }
    }

    if number.pointee._length == 0 {
        return number.pointee._isNegative != 0 ? "NaN" : "0"
    }

    var decimal = number.pointee
    var reversed = ""
    if number.pointee._exponent > 0 {
        repeat {
            reversed.append("0")
            decimal._exponent -= 1
        } while decimal._exponent > 0
    }
    if decimal._exponent == 0 {
        decimal._exponent = 1
    }

    while decimal._length != 0 {
        if decimal._exponent == 0 {
            reversed.append(separator)
        }
        decimal._exponent += 1
        // The native loop inlines the existing short-division routine.
        #sourceLocation(file:"Foundation/Decimal.swift",line:236)
        let divided = try! decimal._divide(by: UInt16(10))
        #sourceLocation()
        decimal = divided.result
        // The native unchecked UTF-8 initializer receives exactly one ASCII digit.
        reversed.append(String(UnicodeScalar(UInt8(divided.remainder) | 0x30)))
    }

    if decimal._exponent <= 0 {
        while decimal._exponent < 0 {
            reversed.append("0")
            decimal._exponent += 1
        }
        reversed.append(separator)
        reversed.append("0")
    }
    if decimal._isNegative != 0 {
        reversed.append("-")
    }
    return String(reversed.reversed())
}

// Native _NSDecimalString, 0x1c08bf398. Hidden (was a private external).
// iOS Device divergence: 0x18118fa08 (24A446) authenticates returns. Confidence: verified.
// The C entry point bridges the nonnil locale through Swift, then returns the bridged string at +0.
@_cdecl("_ONSDecimalString")
func decimalStringBridge(_ number: UnsafePointer<ONSDecimal>, _ locale: UnsafeRawPointer?) -> AnyObject {
    let text: String
    do {
        let value: Any?
        if let locale {
            let retained = Unmanaged<AnyObject>.fromOpaque(locale).retain()
            value = _bridgeAnyObjectToAny(retained.takeUnretainedValue())
            retained.release()
        } else {
            value = nil
        }
        text = __ONSDecimalString(number, value)
    }
    return text._bridgeToObjectiveCImpl()
}

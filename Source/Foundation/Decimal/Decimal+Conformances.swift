//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

extension ONSDecimal {
    // Native subscript getter is inlined into _multiply(byShort:) at 0x1c08cbda4 and _add(UInt16).
    // Its complete eight selections and failure path are read in both native bodies; no emitted getter exists.
    // iOS Simulator Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Confidence: verified.
    // iOS Device containing primitive: 0x180c9ec68, UUID C041F1D0-5EF1-30A3-BF96-830976173B56.
    // Setter: 0x1c08c59c4 / 0x180dab138, same UUIDs. Confidence: verified.
    // Reading: Documentation/Reference/Research/FoundationDecimal.md.
    subscript(index: UInt32) -> UInt16 {
        get {
            switch index {
            case 0:
                return _mantissa.0
            case 1:
                return _mantissa.1
            case 2:
                return _mantissa.2
            case 3:
                return _mantissa.3
            case 4:
                return _mantissa.4
            case 5:
                return _mantissa.5
            case 6:
                return _mantissa.6
            case 7:
                return _mantissa.7
            default:
                #sourceLocation(file:"Foundation/Decimal+Conformances.swift",line:503)
                fatalError("Invalid index \(index) for _mantissa")
                #sourceLocation()
            }
        }
        set {
            switch index {
            case 0:
                _mantissa.0 = newValue
            case 1:
                _mantissa.1 = newValue
            case 2:
                _mantissa.2 = newValue
            case 3:
                _mantissa.3 = newValue
            case 4:
                _mantissa.4 = newValue
            case 5:
                _mantissa.5 = newValue
            case 6:
                _mantissa.6 = newValue
            case 7:
                _mantissa.7 = newValue
            default:
                #sourceLocation(file:"Foundation/Decimal+Conformances.swift",line:516)
                fatalError("Invalid index \(index) for _mantissa")
                #sourceLocation()
            }
        }
    }
}

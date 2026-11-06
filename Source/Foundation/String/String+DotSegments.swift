//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FX-swift-and-functions.

extension String {
    // Native String._removingDotSegments(useRFC1808:) UTF-8 initialization closure, 0x1c0b623d0.
    // Private external. Its partial applies are 0x1c0b653ec and 0x1c0b6540c.
    func _removingDotSegments(useRFC1808: Bool) -> String {
        String(unsafeUninitializedCapacity: utf8.count) { buffer in
            for (index, byte) in utf8.enumerated() {
                buffer.initializeElement(at: index, to: byte)
            }
            let count = resolveDotSegmentsInPlace(buffer: buffer, useRFC1808: useRFC1808)
            if !useRFC1808 && count == 1 && buffer[0] == 0x2e {
                return 0
            }
            return count
        }
    }
}

private enum DotSegmentState {
    case componentStart
    case dot
    case dotDot
    case slash
    case slashDot
    case slashDotDot
    case component
    case removedFirstComponent
}

// Native resolveDotSegmentsInPlace(buffer:useRFC1808:) specialization, 0x1c0b624ec. Private external.
// The native algorithm delays writing separators and dots until the following byte is read.
private func resolveDotSegmentsInPlace<CodeUnit: FixedWidthInteger & UnsignedInteger>(
    buffer: UnsafeMutableBufferPointer<CodeUnit>,
    useRFC1808: Bool
) -> Int {
    if buffer.baseAddress == nil || buffer.isEmpty {
        return 0
    }

    let dot: CodeUnit = 0x2e
    let slash: CodeUnit = 0x2f
    var state = DotSegmentState.componentStart
    var written = 0

    func append(_ byte: CodeUnit) {
        buffer[written] = byte
        written += 1
    }

    func endsWithParentSegment() -> Bool {
        written >= 2 && buffer[written - 1] == dot && buffer[written - 2] == dot
            && (written == 2 || buffer[written - 3] == slash)
    }

    func lastSeparator() -> Int? {
        buffer[..<written].lastIndex(of: slash)
    }

    for index in buffer.indices {
        let byte = buffer[index]
        switch state {
        case .componentStart, .removedFirstComponent:
            if byte == dot {
                state = .dot
            } else if byte == slash {
                state = state == .componentStart ? .slash : .removedFirstComponent
            } else {
                append(byte)
                state = .component
            }
        case .dot:
            if byte == dot {
                state = .dotDot
            } else if byte == slash {
                state = .componentStart
            } else {
                append(dot)
                append(byte)
                state = .component
            }
        case .dotDot:
            if useRFC1808 || byte != slash {
                append(dot)
                append(dot)
                append(byte)
            }
            state = byte == slash ? .componentStart : .component
        case .slash:
            if byte == dot {
                state = .slashDot
            } else {
                append(slash)
                if byte != slash {
                    append(byte)
                    state = .component
                }
            }
        case .slashDot:
            if byte == dot {
                state = .slashDotDot
            } else if byte == slash {
                state = .slash
            } else {
                append(slash)
                append(dot)
                append(byte)
                state = .component
            }
        case .slashDotDot:
            if byte != slash {
                append(slash)
                append(dot)
                append(dot)
                append(byte)
                state = .component
            } else if useRFC1808 && (written == 0 || endsWithParentSegment()) {
                append(slash)
                append(dot)
                append(dot)
                state = .slash
            } else if written == 0 {
                state = .slash
            } else if let separator = lastSeparator() {
                written = separator
                state = .slash
            } else {
                written = 0
                state = .removedFirstComponent
            }
        case .component:
            if byte == slash {
                state = .slash
            } else {
                append(byte)
            }
        }
    }

    switch state {
    case .slash, .slashDot:
        append(slash)
    case .dotDot where useRFC1808:
        append(dot)
        append(dot)
    case .slashDotDot:
        if useRFC1808 && (written == 0 || endsWithParentSegment()) {
            append(slash)
            append(dot)
            append(dot)
        } else if written == 0 {
            append(slash)
        } else if let separator = lastSeparator() {
            written = separator + 1
        } else {
            written = 0
        }
    default:
        break
    }

    if written == 0 {
        append(dot)
    }
    return written
}

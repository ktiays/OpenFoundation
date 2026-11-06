//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified for the buffer, parser and normalization operations.
// iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FX-swift-and-functions.

// The native getter takes the String in X0/X1 and initializes the opaque _NFD result in X8
// (0x1c0bd9eb0–0x1c0bd9ebc). A generic result lets the caller allocate the result from
// its native metadata. The extra generic metadata argument does not change those registers.
@_silgen_name("$ss7UnicodeO4_NFDVMa")
private func nfdMetadata(_ request: UInt) -> (type: Any.Type, state: UInt)

@_silgen_name("$sSS4_nfds7UnicodeO4_NFDVvg")
private func nativeNFD<Elements>(_ string: String) -> Elements

// Native Array<Unicode.Scalar>.append(contentsOf:) for Unicode._NFD, 0x1c07d0a20.
private func appendNFD<Elements: Sequence>(
    _ string: String, as type: Elements.Type, to scalars: inout [Unicode.Scalar]
) where Elements.Element == Unicode.Scalar {
    let decomposition: Elements = nativeNFD(string)
    scalars.append(contentsOf: decomposition)
}

// Native DecompositionError witness table accessor, 0x1c0b16684.
private enum DecompositionError: Error {
    case outputExhausted
    case dataAfterNull
    case invalidUTF8
}

private struct DecompositionInput: IteratorProtocol {
    let bytes: UnsafeBufferPointer<UInt8>
    private(set) var offset = 0

    mutating func next() -> UInt8? {
        if offset == bytes.count {
            return nil
        }
        let byte = bytes[offset]
        offset += 1
        return byte
    }
}

// Native specialized UnsafeBufferPointer._decomposed, 0x1c0bd9c48. Non-external.
func decomposeUTF8(
    _ normalization: UInt8,
    _ output: UnsafeMutableBufferPointer<UInt8>,
    _ nullTerminated: Bool,
    _ bytes: UnsafeBufferPointer<UInt8>
) throws -> Int {
    let decompositionType = nfdMetadata(0).type
    if output.isEmpty {
        guard bytes.isEmpty && !nullTerminated else {
            throw DecompositionError.outputExhausted
        }
        return 0
    }

    var written = 0
    // Native UnsafeBufferPointer._decomposed defer #1, 0x1c0b14408. Non-external.
    // The assembly uses the live write count on both the throwing and successful exits.
    defer {
        if nullTerminated {
            if written >= output.count {
                output[output.count - 1] = 0
            } else if written == 0 || output[written - 1] != 0 {
                output[written] = 0
            }
        }
    }

    func appendByte(_ byte: UInt8) throws {
        guard written < output.count else {
            throw DecompositionError.outputExhausted
        }
        output[written] = byte
        written += 1
    }

    let bitmap = BuiltInUnicodeScalarSet(rawValue: normalization == 1 ? 16 : 8)
    var scalars: [Unicode.Scalar] = []

    // The native stable sort of the combining sequence uses 0x1c0b15634 and 0x1c0b15c9c.
    // Its run merge and run-array specializations are 0x1c0b15e7c, 0x1c0b160e4, 0x1c0bd5e54,
    // 0x1c0bd5edc and 0x1c0bd5fdc. Its scalar storage and uniqueness paths use 0x1c0b95408
    // and 0x1c080dbbc, with the allocator-size thunk 0x1c089df6c.
    func fillFromSortBuffer() throws {
        if scalars.isEmpty {
            return
        }
        scalars.withUnsafeMutableBufferPointer { buffer in
            buffer.sort {
                $0.properties.canonicalCombiningClass.rawValue
                    < $1.properties.canonicalCombiningClass.rawValue
            }
        }
        for scalar in scalars {
            let encoded = Unicode.UTF8.encode(scalar)!
            guard encoded.count <= output.count - written else {
                throw DecompositionError.outputExhausted
            }
            for byte in encoded {
                output[written] = byte
                written += 1
            }
        }

        // Native scalar-array replacement by an empty collection: 0x1c073f42c, 0x1c073f58c,
        // and the buffer specialization 0x1c0cdecac.
        scalars.replaceSubrange(scalars.indices, with: EmptyCollection<Unicode.Scalar>())
    }

    var input = DecompositionInput(bytes: bytes)
    // Native Unicode.UTF8.ForwardParser witness accessor, 0x1c0bd6ec4.
    var parser = Unicode.UTF8.ForwardParser()
    var nullOffset: Int?
    parse: while written < output.count {
        switch parser.parseScalar(from: &input) {
        case .emptyInput:
            break parse
        case .error:
            throw DecompositionError.invalidUTF8
        case .valid(let encoded):
            let scalar = Unicode.UTF8.decode(encoded)
            if scalar.value == 0 {
                nullOffset = written
            } else if nullOffset != nil {
                throw DecompositionError.dataAfterNull
            }

            if scalar.value < 0x80 {
                try fillFromSortBuffer()
                try appendByte(UInt8(scalar.value))
            } else {
                if scalar.properties.canonicalCombiningClass == .notReordered {
                    try fillFromSortBuffer()
                }
                if bitmap.contains(scalar) {
                    let type = decompositionType as! any Sequence<Unicode.Scalar>.Type
                    appendNFD(String(scalar), as: type, to: &scalars)
                } else {
                    // Native scalar-array growth at 0x1c0796e78 and 0x1c079ae8c.
                    // The associated malloc_size thunk is 0x1c079d5c0.
                    scalars.append(scalar)
                }
            }
        }
    }
    try fillFromSortBuffer()

    // The native final check compares the underlying iterator position, including parser read-ahead.
    guard input.offset == bytes.count else {
        throw DecompositionError.outputExhausted
    }
    if let nullOffset {
        return nullOffset + 1
    }
    if nullTerminated {
        try appendByte(0)
    }
    return written
}

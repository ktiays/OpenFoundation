//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified (assembly read in full; layouts from the reflection metadata, nominal type descriptors of
// Data, Data._Representation 0x1c145be10, Data.InlineData 0x1c145bd80, Data.InlineSlice, Data.LargeSlice,
// Data.RangeReference 0x1c145bdb8, Data.Deallocator and Data.Iterator; the element types of InlineData.bytes
// (UInt8) and InlineSlice.slice (Int32) are imported stdlib descriptors, confirmed by the code's byte and 32-bit
// accesses). iOS Device divergence: iOS Device 24A446 remains unread.
//
// Native source path Foundation/Data.swift (its fatal error). Data's members that the native image inlines at
// every caller (the representation's count and indices, its byte buffers, its bounds subscript, and the payload
// conversions) are written here once and cited at their first restored caller. Compiler-generated pieces are cited
// and never written: the outlined copies and consumes of Data, Data._Representation and Data.Deallocator
// (0x1c06ea62c, 0x1c06eaac0, 0x1c086d654, 0x1c08906e4, 0x1c089dfa4), the outlined destroy of Data.Iterator
// 0x1c089df70, its enum tag witnesses (0x1c08a6e40, 0x1c08a6e94), and the RangeReference metadata accessor
// 0x1c0888068 and allocating initializer 0x1c08873fc.
//
// Members written with their first restored caller's group: Data.init<S>(_:) for String.UTF8View 0x1c0b678bc with
// its closure #2 0x1c089142c and Data.init(capacity:initializingWith:) 0x1c0c42d28 (String.data(using:)),
// Data.withBufferView(_:) 0x1c0945ea0 (the property list and JSON coders) and Data.init(buffer:) 0x1c0a98994
// (BPlistMap).
//
// ABI divergence: native Foundation is built with library evolution, so Data is resilient there; this module is
// not. Behavior is unchanged.

import Darwin

// Native __DataInvokeDeallocatorVirtualMemory(_:_:), 0x1c08904d8: vm_deallocate, a fatal error if it fails. The
// native name is kept.
// swift-format-ignore: AlwaysUseLowerCamelCase
func __DataInvokeDeallocatorVirtualMemory(_ mem: UnsafeMutableRawPointer, _ length: Int) {
    let result = vm_deallocate(
        _platform_mach_task_self(), vm_address_t(UInt(bitPattern: mem)), vm_size_t(length))
    if result != KERN_SUCCESS {
        fatalError(
            "*** __DataInvokeDeallocatorVirtualMemory(\(mem), \(length)) failed",
            file: "Foundation/Data.swift",
            line: 68
        )
    }
}

// Native Data: a value-semantic byte buffer.
struct Data {
    // Native Data.Deallocator: how Data(bytesNoCopy:count:deallocator:) releases adopted bytes.
    enum Deallocator {
        case virtualMemory
        case unmap
        case free
        case none
        case custom((UnsafeMutableRawPointer, Int) -> Void)

        // Native Data.Deallocator._deallocator.getter, inlined at its callers (first restored caller
        // Data.init(bytesNoCopy:count:deallocator:) 0x1c089c708): munmap (closure #1 0x1c08906d8), free (closure #2
        // 0x1c08906dc), nothing (closure #3, identical-code folded natively with _NSBundleDeallocatingImmortalBundle)
        // and vm_deallocate (closure #4 0x1c08906e0), or the custom block.
        var _deallocator: ((UnsafeMutableRawPointer, Int) -> Void) {
            switch self {
            case .unmap:
                return { munmap($0, $1) }
            case .free:
                return { pointer, _ in Darwin.free(pointer) }
            case .none:
                return { _, _ in }
            case .virtualMemory:
                return { __DataInvokeDeallocatorVirtualMemory($0, $1) }
            case .custom(let deallocator):
                return deallocator
            }
        }
    }

    // Native Data.RangeReference (Swift class, instance size 0x20): the shared range of a LargeSlice.
    final class RangeReference {
        var range: Range<Int>

        init(_ range: Range<Int>) {
            self.range = range
        }
    }

    // Native Data.InlineData: up to fourteen bytes stored in place, with their count.
    struct InlineData {
        typealias Buffer = (
            UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8
        )

        var bytes: Buffer
        var length: UInt8

        static func canStore(count: Int) -> Bool {
            return count <= MemoryLayout<Buffer>.size
        }

        // Inlined at its callers (first restored caller _Representation.init(_:) 0x1c088b8ec): zeroed, then the
        // buffer's bytes.
        init(_ buffer: UnsafeRawBufferPointer) {
            let count = buffer.count
            bytes = (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
            length = UInt8(count)
            if count > 0 {
                Swift.withUnsafeMutableBytes(of: &bytes) { destination in
                    _ = memcpy(destination.baseAddress!, buffer.baseAddress!, count)
                }
            }
        }

        // Inlined at its callers (first restored caller _Representation.init(count:) 0x1c088b850).
        init(count: Int = 0) {
            bytes = (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
            length = UInt8(count)
        }

        // Inlined at its callers (first restored caller _Representation.replaceSubrange(_:with:count:)
        // 0x1c088a5d8): the first count bytes of the slice.
        init(_ slice: InlineSlice, count: Int) {
            bytes = (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
            length = UInt8(count)
            slice.withUnsafeBytes { source in
                Swift.withUnsafeMutableBytes(of: &bytes) { destination in
                    _ = memcpy(destination.baseAddress!, source.baseAddress!, count)
                }
            }
        }

        // Inlined at its callers (first restored caller Data.subdata(in:) 0x1c08931f0).
        init(_ slice: LargeSlice, count: Int) {
            bytes = (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
            length = UInt8(count)
            slice.withUnsafeBytes { source in
                Swift.withUnsafeMutableBytes(of: &bytes) { destination in
                    _ = memcpy(destination.baseAddress!, source.baseAddress!, count)
                }
            }
        }

        var count: Int {
            get {
                return Int(length)
            }
            set {
                length = UInt8(newValue)
            }
        }

        // Inlined at its callers: the bytes, copied to a local.
        func withUnsafeBytes<Result>(_ apply: (UnsafeRawBufferPointer) throws -> Result) rethrows -> Result {
            var bytes = bytes
            let count = Int(length)
            return try Swift.withUnsafeBytes(of: &bytes) { rawBuffer in
                try apply(UnsafeRawBufferPointer(start: rawBuffer.baseAddress, count: count))
            }
        }

        // Inlined at its callers (first restored caller _Representation.append(contentsOf:) 0x1c0889acc).
        mutating func append(contentsOf buffer: UnsafeRawBufferPointer) {
            guard buffer.count > 0 else {
                return
            }
            let count = Int(length)
            Swift.withUnsafeMutableBytes(of: &bytes) { destination in
                _ = memcpy(destination.baseAddress! + count, buffer.baseAddress!, buffer.count)
            }
            length += UInt8(buffer.count)
        }

        // Native Data.InlineData.replaceSubrange(_:with:count:), 0x1c0885fdc: the tail moves to its new place,
        // then the replacement is copied.
        mutating func replaceSubrange(
            _ subrange: Range<Int>,
            with replacementBytes: UnsafeRawPointer?,
            count replacementLength: Int
        ) {
            let currentLength = count
            precondition(subrange.lowerBound <= currentLength && subrange.upperBound <= currentLength)
            let subrangeCount = subrange.upperBound - subrange.lowerBound
            let resultingLength = currentLength - subrangeCount + replacementLength
            let shift = resultingLength - currentLength
            Swift.withUnsafeMutableBytes(of: &bytes) { mutableBytes in
                let base = mutableBytes.baseAddress!
                if shift != 0 {
                    let destination = base + (subrange.lowerBound + replacementLength)
                    let source = base + (subrange.lowerBound + subrangeCount)
                    _ = memmove(destination, source, currentLength - subrange.lowerBound - subrangeCount)
                }
                if replacementLength != 0 {
                    _ = memmove(base + subrange.lowerBound, replacementBytes!, replacementLength)
                }
            }
            count = resultingLength
        }
    }

    // Native Data.InlineSlice: a range below Int32.max of a shared storage.
    struct InlineSlice {
        typealias HalfInt = Int32

        var slice: Range<HalfInt>
        var storage: __DataStorage

        static func canStore(count: Int) -> Bool {
            return count < HalfInt.max
        }

        // Inlined at its callers (first restored caller _Representation.init(_:) 0x1c088b8ec).
        init(_ buffer: UnsafeRawBufferPointer) {
            storage = __DataStorage(bytes: buffer.baseAddress, length: buffer.count)
            slice = 0..<HalfInt(buffer.count)
        }

        // Inlined at its callers (first restored caller _Representation.append(contentsOf:) 0x1c0889acc).
        init(_ inline: InlineData) {
            storage = inline.withUnsafeBytes { buffer in
                __DataStorage(bytes: buffer.baseAddress, length: buffer.count)
            }
            slice = 0..<HalfInt(inline.count)
        }

        // Inlined at its callers (first restored caller _Representation.replaceSubrange(_:with:count:)
        // 0x1c088a5d8).
        init(_ large: LargeSlice) {
            storage = large.storage
            slice = HalfInt(large.range.lowerBound)..<HalfInt(large.range.upperBound)
        }

        // Inlined at its callers (first restored caller _Representation.init(_:count:) 0x1c088ba18).
        init(_ storage: __DataStorage, count: Int) {
            self.storage = storage
            slice = 0..<HalfInt(count)
        }

        // Inlined at its callers (first restored caller Data._bridgeFromObjectiveCAdoptingNativeStorageOf(_:)
        // 0x1c08a5838).
        init(_ storage: __DataStorage, range: Range<Int>) {
            self.storage = storage
            slice = HalfInt(range.lowerBound)..<HalfInt(range.upperBound)
        }

        var range: Range<Int> {
            return Int(slice.lowerBound)..<Int(slice.upperBound)
        }

        var startIndex: Int {
            return Int(slice.lowerBound)
        }

        var endIndex: Int {
            return Int(slice.upperBound)
        }

        var count: Int {
            return Int(slice.upperBound - slice.lowerBound)
        }

        // Native Data.InlineSlice.ensureUniqueReference(), 0x1c088673c: a shared storage is replaced by a copy of
        // the slice.
        mutating func ensureUniqueReference() {
            if !isKnownUniquelyReferenced(&storage) {
                storage = storage.mutableCopy(range)
            }
        }

        // Inlined at its callers.
        func withUnsafeBytes<Result>(_ apply: (UnsafeRawBufferPointer) throws -> Result) rethrows -> Result {
            return try storage.withUnsafeBytes(in: range, apply: apply)
        }

        // Inlined at its callers (first restored caller Data.init(repeating:count:) 0x1c089c37c).
        mutating func withUnsafeMutableBytes<Result>(
            _ apply: (UnsafeMutableRawBufferPointer) throws -> Result
        ) rethrows -> Result {
            ensureUniqueReference()
            return try storage.withUnsafeMutableBytes(in: range, apply: apply)
        }

        // Native Data.InlineSlice.append(contentsOf:), 0x1c0886d08.
        mutating func append(contentsOf buffer: UnsafeRawBufferPointer) {
            ensureUniqueReference()
            let lowerBound = slice.lowerBound
            let upperBound = Int(slice.upperBound)
            storage.replaceBytes(
                in: (location: upperBound, length: storage.length - (upperBound - storage._offset)),
                with: buffer.baseAddress,
                length: buffer.count
            )
            slice = lowerBound..<HalfInt(upperBound + buffer.count)
        }

        // Native Data.InlineSlice.replaceSubrange(_:with:count:), 0x1c0886ffc.
        mutating func replaceSubrange(
            _ subrange: Range<Int>,
            with bytes: UnsafeRawPointer?,
            count cnt: Int
        ) {
            precondition(startIndex <= subrange.lowerBound)
            precondition(subrange.upperBound <= endIndex && startIndex <= subrange.upperBound)
            precondition(subrange.lowerBound <= endIndex)
            ensureUniqueReference()
            let lowerBound = slice.lowerBound
            let upperBound = Int(slice.upperBound)
            let subrangeCount = subrange.upperBound - subrange.lowerBound
            storage.replaceBytes(
                in: (location: subrange.lowerBound, length: subrangeCount),
                with: bytes,
                length: cnt
            )
            slice = lowerBound..<HalfInt(upperBound - subrangeCount + cnt)
        }
    }

    // Native Data.LargeSlice: any range of a shared storage, the range itself shared by reference.
    struct LargeSlice {
        var slice: RangeReference
        var storage: __DataStorage

        // Inlined at its callers (first restored caller _Representation.init(_:) 0x1c088b8ec).
        init(_ buffer: UnsafeRawBufferPointer) {
            storage = __DataStorage(bytes: buffer.baseAddress, length: buffer.count)
            slice = RangeReference(0..<buffer.count)
        }

        // Inlined at its callers (first restored caller _Representation.append(contentsOf:) 0x1c0889acc).
        init(_ inline: InlineData) {
            storage = inline.withUnsafeBytes { buffer in
                __DataStorage(bytes: buffer.baseAddress, length: buffer.count)
            }
            slice = RangeReference(0..<inline.count)
        }

        // Inlined at its callers (first restored caller _Representation.append(contentsOf:) 0x1c0889acc).
        init(_ inline: InlineSlice) {
            storage = inline.storage
            slice = RangeReference(inline.range)
        }

        // Inlined at its callers (first restored caller _Representation.init(_:count:) 0x1c088ba18).
        init(_ storage: __DataStorage, count: Int) {
            self.storage = storage
            slice = RangeReference(0..<count)
        }

        // Inlined at its callers (first restored caller Data._bridgeFromObjectiveCAdoptingNativeStorageOf(_:)
        // 0x1c08a5838).
        init(_ storage: __DataStorage, range: Range<Int>) {
            self.storage = storage
            slice = RangeReference(range)
        }

        var range: Range<Int> {
            return slice.range
        }

        var startIndex: Int {
            return slice.range.lowerBound
        }

        var endIndex: Int {
            return slice.range.upperBound
        }

        var count: Int {
            return slice.range.upperBound - slice.range.lowerBound
        }

        // Native Data.LargeSlice.ensureUniqueReference(), 0x1c0887734: a shared storage is replaced by a copy of
        // the range, then a shared range reference by a new one.
        mutating func ensureUniqueReference() {
            if !isKnownUniquelyReferenced(&storage) {
                storage = storage.mutableCopy(range)
            }
            if !isKnownUniquelyReferenced(&slice) {
                slice = RangeReference(range)
            }
        }

        // Inlined at its callers.
        func withUnsafeBytes<Result>(_ apply: (UnsafeRawBufferPointer) throws -> Result) rethrows -> Result {
            return try storage.withUnsafeBytes(in: range, apply: apply)
        }

        // Inlined at its callers (first restored caller Data.init(repeating:count:) 0x1c089c37c).
        mutating func withUnsafeMutableBytes<Result>(
            _ apply: (UnsafeMutableRawBufferPointer) throws -> Result
        ) rethrows -> Result {
            ensureUniqueReference()
            return try storage.withUnsafeMutableBytes(in: range, apply: apply)
        }

        // Native Data.LargeSlice.append(contentsOf:), 0x1c0887be4.
        mutating func append(contentsOf buffer: UnsafeRawBufferPointer) {
            ensureUniqueReference()
            let reference = slice
            let upperBound = reference.range.upperBound
            storage.replaceBytes(
                in: (location: upperBound, length: storage.length - (upperBound - storage._offset)),
                with: buffer.baseAddress,
                length: buffer.count
            )
            reference.range = reference.range.lowerBound..<(reference.range.upperBound + buffer.count)
        }

        // Native Data.LargeSlice.replaceSubrange(_:with:count:), 0x1c0887e90.
        mutating func replaceSubrange(
            _ subrange: Range<Int>,
            with bytes: UnsafeRawPointer?,
            count cnt: Int
        ) {
            precondition(startIndex <= subrange.lowerBound)
            precondition(subrange.upperBound <= endIndex && startIndex <= subrange.upperBound)
            precondition(subrange.lowerBound <= endIndex)
            ensureUniqueReference()
            let subrangeCount = subrange.upperBound - subrange.lowerBound
            let reference = slice
            let upperBound = reference.range.upperBound
            storage.replaceBytes(
                in: (location: subrange.lowerBound, length: subrangeCount),
                with: bytes,
                length: cnt
            )
            reference.range = reference.range.lowerBound..<(upperBound - subrangeCount + cnt)
        }
    }

    // Native Data._Representation: no bytes, in place, a small slice or a large slice.
    enum _Representation {
        case inline(InlineData)
        case slice(InlineSlice)
        case large(LargeSlice)
        case empty

        // Native specialized Data._Representation.init(_:), 0x1c088b8ec.
        init(_ buffer: UnsafeRawBufferPointer) {
            if buffer.count == 0 {
                self = .empty
            } else if InlineData.canStore(count: buffer.count) {
                self = .inline(InlineData(buffer))
            } else if InlineSlice.canStore(count: buffer.count) {
                self = .slice(InlineSlice(buffer))
            } else {
                self = .large(LargeSlice(buffer))
            }
        }

        // Native specialized Data._Representation.init(_:count:), 0x1c088ba18.
        init(_ storage: __DataStorage, count: Int) {
            if count == 0 {
                self = .empty
            } else if InlineData.canStore(count: count) {
                self = .inline(storage.withUnsafeBytes(in: 0..<count) { InlineData($0) })
            } else if InlineSlice.canStore(count: count) {
                self = .slice(InlineSlice(storage, count: count))
            } else {
                self = .large(LargeSlice(storage, count: count))
            }
        }

        // Native specialized Data._Representation.init(count:), 0x1c088b850.
        init(count: Int) {
            if count == 0 {
                self = .empty
            } else if InlineData.canStore(count: count) {
                self = .inline(InlineData(count: count))
            } else if InlineSlice.canStore(count: count) {
                self = .slice(InlineSlice(__DataStorage(length: count), count: count))
            } else {
                self = .large(LargeSlice(__DataStorage(length: count), count: count))
            }
        }

        var count: Int {
            switch self {
            case .empty:
                return 0
            case .inline(let inline):
                return inline.count
            case .slice(let slice):
                return slice.count
            case .large(let slice):
                return slice.count
            }
        }

        var startIndex: Int {
            switch self {
            case .empty, .inline:
                return 0
            case .slice(let slice):
                return slice.startIndex
            case .large(let slice):
                return slice.startIndex
            }
        }

        var endIndex: Int {
            switch self {
            case .empty:
                return 0
            case .inline(let inline):
                return inline.count
            case .slice(let slice):
                return slice.endIndex
            case .large(let slice):
                return slice.endIndex
            }
        }

        var isEmpty: Bool {
            return startIndex == endIndex
        }

        // Inlined at its callers (first restored caller Data.append(_:) 0x1c0891b34): without bytes, an empty
        // buffer at a zeroed local.
        func withUnsafeBytes<Result>(_ apply: (UnsafeRawBufferPointer) throws -> Result) rethrows -> Result {
            switch self {
            case .empty:
                var zero = 0
                return try Swift.withUnsafeMutableBytes(of: &zero) { local in
                    try apply(UnsafeRawBufferPointer(start: local.baseAddress, count: 0))
                }
            case .inline(let inline):
                return try inline.withUnsafeBytes(apply)
            case .slice(let slice):
                return try slice.withUnsafeBytes(apply)
            case .large(let slice):
                return try slice.withUnsafeBytes(apply)
            }
        }

        // Inlined at its callers (first restored caller Data.init(repeating:count:) 0x1c089c37c). The payload is
        // moved out while it is mutated.
        mutating func withUnsafeMutableBytes<Result>(
            _ apply: (UnsafeMutableRawBufferPointer) throws -> Result
        ) rethrows -> Result {
            switch self {
            case .empty:
                var zero = 0
                return try Swift.withUnsafeMutableBytes(of: &zero) { local in
                    try apply(UnsafeMutableRawBufferPointer(start: local.baseAddress, count: 0))
                }
            case .inline(var inline):
                defer {
                    self = .inline(inline)
                }
                let count = inline.count
                return try Swift.withUnsafeMutableBytes(of: &inline.bytes) { rawBuffer in
                    try apply(UnsafeMutableRawBufferPointer(start: rawBuffer.baseAddress, count: count))
                }
            case .slice(var slice):
                self = .empty
                defer {
                    self = .slice(slice)
                }
                return try slice.withUnsafeMutableBytes(apply)
            case .large(var slice):
                self = .empty
                defer {
                    self = .large(slice)
                }
                return try slice.withUnsafeMutableBytes(apply)
            }
        }

        // Native Data._Representation.bytes.getter, 0x1c0889068: the bytes in place, or the slice of the storage
        // (based without overflow checks), or none.
        var bytes: RawSpan {
            @_lifetime(borrow self)
            borrowing get {
                let buffer: UnsafeRawBufferPointer
                switch self {
                case .inline:
                    buffer = Swift.withUnsafeBytes(of: self) { selfBytes in
                        UnsafeRawBufferPointer(start: selfBytes.baseAddress, count: count)
                    }
                case .slice(let slice):
                    if let storageBytes = slice.storage._bytes {
                        let base = UnsafeRawPointer(storageBytes) + (0 &- slice.storage._offset)
                        let start = base + slice.startIndex
                        buffer = UnsafeRawBufferPointer(start: start, count: (base + slice.endIndex) - start)
                    } else {
                        buffer = UnsafeRawBufferPointer(start: nil, count: 0)
                    }
                case .large(let slice):
                    if let storageBytes = slice.storage._bytes {
                        let base = UnsafeRawPointer(storageBytes) + (0 &- slice.storage._offset)
                        let start = base + slice.startIndex
                        buffer = UnsafeRawBufferPointer(start: start, count: (base + slice.endIndex) - start)
                    } else {
                        buffer = UnsafeRawBufferPointer(start: nil, count: 0)
                    }
                case .empty:
                    buffer = UnsafeRawBufferPointer(start: nil, count: 0)
                }
                let span = RawSpan(_unsafeBytes: buffer)
                return _overrideLifetime(span, borrowing: self)
            }
        }

        // Native Data._Representation.mutableBytes.getter, 0x1c08890f4: a slice's storage is made unique first
        // (the payload moved out meanwhile); the bytes in place, or the slice of the storage, or none.
        var mutableBytes: MutableRawSpan {
            @_lifetime(&self)
            mutating get {
                let buffer: UnsafeMutableRawBufferPointer
                switch self {
                case .inline(let inline):
                    let count = inline.count
                    buffer = Swift.withUnsafeMutableBytes(of: &self) { selfBytes in
                        UnsafeMutableRawBufferPointer(start: selfBytes.baseAddress, count: count)
                    }
                case .slice(var slice):
                    self = .empty
                    slice.ensureUniqueReference()
                    self = .slice(slice)
                    if let storageBytes = slice.storage._bytes {
                        let base = storageBytes + (0 &- slice.storage._offset)
                        let start = base + slice.startIndex
                        buffer = UnsafeMutableRawBufferPointer(
                            start: start, count: (base + slice.endIndex) - start)
                    } else {
                        buffer = UnsafeMutableRawBufferPointer(start: nil, count: 0)
                    }
                case .large(var slice):
                    self = .empty
                    slice.ensureUniqueReference()
                    self = .large(slice)
                    if let storageBytes = slice.storage._bytes {
                        let base = storageBytes + (0 &- slice.storage._offset)
                        let start = base + slice.startIndex
                        buffer = UnsafeMutableRawBufferPointer(
                            start: start, count: (base + slice.endIndex) - start)
                    } else {
                        buffer = UnsafeMutableRawBufferPointer(start: nil, count: 0)
                    }
                case .empty:
                    buffer = UnsafeMutableRawBufferPointer(start: nil, count: 0)
                }
                let span = MutableRawSpan(_unsafeBytes: buffer)
                return _overrideLifetime(span, mutating: &self)
            }
        }

        // Native Data._Representation.subscript.getter, 0x1c088ae30.
        subscript(index: Int) -> UInt8 {
            switch self {
            case .empty:
                preconditionFailure()
            case .inline(let inline):
                precondition(index < inline.count)
                return inline.withUnsafeBytes { $0[index] }
            case .slice(let slice):
                precondition(index < slice.endIndex)
                precondition(index >= slice.startIndex)
                return slice.storage.get(index)
            case .large(let slice):
                precondition(index >= slice.startIndex)
                precondition(UInt(bitPattern: index) < UInt(bitPattern: slice.endIndex))
                return slice.storage.get(index)
            }
        }

        // Native Data._Representation.subscript(bounds:) getter, inlined at its callers (first restored caller
        // Data.subdata(in:) 0x1c08931f0): the narrowest representation of the range.
        subscript(bounds range: Range<Int>) -> _Representation {
            switch self {
            case .empty:
                precondition(range.lowerBound == 0 && range.upperBound == 0)
                return .empty
            case .inline(let inline):
                precondition(range.upperBound <= inline.count)
                if range.lowerBound != 0 {
                    var newSlice = InlineSlice(inline)
                    newSlice.slice =
                        InlineSlice.HalfInt(range.lowerBound)..<InlineSlice.HalfInt(range.upperBound)
                    return .slice(newSlice)
                }
                var newInline = inline
                newInline.count = range.upperBound
                return .inline(newInline)
            case .slice(let slice):
                precondition(range.upperBound <= slice.endIndex && slice.startIndex <= range.upperBound)
                precondition(range.lowerBound <= slice.endIndex && slice.startIndex <= range.lowerBound)
                if range.lowerBound == 0 && range.upperBound == 0 {
                    return .empty
                } else if range.lowerBound == 0 && InlineData.canStore(count: range.count) {
                    return .inline(InlineData(slice, count: range.count))
                }
                var newSlice = slice
                newSlice.slice =
                    InlineSlice.HalfInt(
                        truncatingIfNeeded: range.lowerBound)..<InlineSlice.HalfInt(
                        truncatingIfNeeded: range.upperBound)
                return .slice(newSlice)
            case .large(let slice):
                precondition(slice.startIndex <= range.lowerBound)
                precondition(range.upperBound <= slice.endIndex && slice.startIndex <= range.upperBound)
                precondition(range.lowerBound <= slice.endIndex)
                if range.lowerBound == 0 && range.upperBound == 0 {
                    return .empty
                } else if range.lowerBound == 0 && InlineData.canStore(count: range.upperBound) {
                    return .inline(InlineData(slice, count: range.upperBound))
                } else if InlineSlice.canStore(count: range.lowerBound)
                    && InlineSlice.canStore(count: range.upperBound)
                {
                    return .slice(InlineSlice(slice.storage, range: range))
                }
                var newSlice = slice
                newSlice.slice = RangeReference(range)
                return .large(newSlice)
            }
        }

        // Native Data._Representation.append(contentsOf:), 0x1c0889acc. The payload is moved out while it is
        // mutated; an inline payload that outgrows its place becomes a slice, a small slice a large one.
        mutating func append(contentsOf buffer: UnsafeRawBufferPointer) {
            switch self {
            case .empty:
                self = _Representation(buffer)
            case .inline(var inline):
                if InlineData.canStore(count: inline.count + buffer.count) {
                    inline.append(contentsOf: buffer)
                    self = .inline(inline)
                } else if InlineSlice.canStore(count: inline.count + buffer.count) {
                    var newSlice = InlineSlice(inline)
                    newSlice.append(contentsOf: buffer)
                    self = .slice(newSlice)
                } else {
                    var newSlice = LargeSlice(inline)
                    newSlice.append(contentsOf: buffer)
                    self = .large(newSlice)
                }
            case .slice(var slice):
                if InlineSlice.canStore(count: slice.range.upperBound + buffer.count) {
                    self = .empty
                    defer {
                        self = .slice(slice)
                    }
                    slice.append(contentsOf: buffer)
                } else {
                    self = .empty
                    var newSlice = LargeSlice(slice)
                    newSlice.append(contentsOf: buffer)
                    self = .large(newSlice)
                }
            case .large(var slice):
                self = .empty
                defer {
                    self = .large(slice)
                }
                slice.append(contentsOf: buffer)
            }
        }

        // Native Data._Representation.replaceSubrange(_:with:count:), 0x1c088a5d8. The result takes the narrowest
        // representation its new end allows: none, in place (a slice starting at zero), a small or a large slice.
        mutating func replaceSubrange(
            _ subrange: Range<Int>,
            with bytes: UnsafeRawPointer?,
            count cnt: Int
        ) {
            switch self {
            case .empty:
                precondition(subrange.lowerBound == 0 && subrange.upperBound == 0)
                if cnt == 0 {
                    return
                }
                if InlineData.canStore(count: cnt) {
                    self = .inline(InlineData(UnsafeRawBufferPointer(start: bytes, count: cnt)))
                } else if InlineSlice.canStore(count: cnt) {
                    self = .slice(InlineSlice(UnsafeRawBufferPointer(start: bytes, count: cnt)))
                } else {
                    self = .large(LargeSlice(UnsafeRawBufferPointer(start: bytes, count: cnt)))
                }
            case .inline(var inline):
                let resultingCount = inline.count + cnt - (subrange.upperBound - subrange.lowerBound)
                if resultingCount == 0 {
                    self = .empty
                } else if !InlineData.canStore(count: resultingCount) {
                    if InlineSlice.canStore(count: resultingCount) {
                        var newSlice = InlineSlice(inline)
                        newSlice.replaceSubrange(subrange, with: bytes, count: cnt)
                        self = .slice(newSlice)
                    } else {
                        var newSlice = LargeSlice(inline)
                        newSlice.replaceSubrange(subrange, with: bytes, count: cnt)
                        self = .large(newSlice)
                    }
                } else {
                    inline.replaceSubrange(subrange, with: bytes, count: cnt)
                    self = .inline(inline)
                }
            case .slice(var slice):
                let resultingUpper = slice.endIndex + cnt - (subrange.upperBound - subrange.lowerBound)
                if slice.startIndex == 0 && resultingUpper == 0 {
                    self = .empty
                } else if slice.startIndex == 0 && InlineData.canStore(count: resultingUpper) {
                    self = .empty
                    slice.replaceSubrange(subrange, with: bytes, count: cnt)
                    self = .inline(InlineData(slice, count: slice.count))
                } else if InlineSlice.canStore(count: resultingUpper) {
                    self = .empty
                    slice.replaceSubrange(subrange, with: bytes, count: cnt)
                    self = .slice(slice)
                } else {
                    self = .empty
                    var newSlice = LargeSlice(slice)
                    newSlice.replaceSubrange(subrange, with: bytes, count: cnt)
                    self = .large(newSlice)
                }
            case .large(var slice):
                let resultingUpper = slice.endIndex + cnt - (subrange.upperBound - subrange.lowerBound)
                if slice.startIndex == 0 && resultingUpper == 0 {
                    self = .empty
                } else if slice.startIndex == 0 && InlineData.canStore(count: resultingUpper) {
                    var inline = InlineData(count: resultingUpper)
                    Swift.withUnsafeMutableBytes(of: &inline.bytes) { inlineBuffer in
                        let inlineBase = inlineBuffer.baseAddress!
                        if cnt > 0 {
                            _ = memcpy(inlineBase + subrange.lowerBound, bytes!, cnt)
                        }
                        slice.withUnsafeBytes { buffer in
                            let base = buffer.baseAddress!
                            if subrange.lowerBound > 0 {
                                _ = memcpy(inlineBase, base, subrange.lowerBound)
                            }
                            if subrange.upperBound < slice.endIndex {
                                let suffixCount = slice.endIndex - subrange.upperBound
                                precondition(suffixCount >= 0)
                                _ = memcpy(
                                    inlineBase + (subrange.lowerBound + cnt),
                                    base + subrange.upperBound,
                                    suffixCount
                                )
                            }
                        }
                    }
                    self = .inline(inline)
                } else if InlineSlice.canStore(count: slice.startIndex)
                    && InlineSlice.canStore(count: resultingUpper)
                {
                    self = .empty
                    var newSlice = InlineSlice(slice)
                    newSlice.replaceSubrange(subrange, with: bytes, count: cnt)
                    self = .slice(newSlice)
                } else {
                    self = .empty
                    slice.replaceSubrange(subrange, with: bytes, count: cnt)
                    self = .large(slice)
                }
            }
        }
    }

    var _representation: _Representation

    init(representation: _Representation) {
        _representation = representation
    }

    // Native Data.init(), inlined at its callers.
    init() {
        _representation = .empty
    }

    // Native Data.init(bytes:count:) over a buffer, inlined at its callers (first restored caller
    // Data.subdata(in:) 0x1c08931f0).
    init(_ buffer: UnsafeRawBufferPointer) {
        _representation = _Representation(buffer)
    }

    // Native Data.init(count:), inlined at its callers.
    init(count: Int) {
        _representation = _Representation(count: count)
    }

    // Native specialized Data.init(repeating:count:), 0x1c089c37c, with its closure #1 0x1c0890738 (memset over the
    // buffer).
    init(repeating repeatedValue: UInt8, count: Int) {
        self.init(count: count)
        _representation.withUnsafeMutableBytes { buffer in
            let start = buffer.baseAddress!
            _ = memset(start, Int32(repeatedValue), buffer.count)
        }
    }

    // Native specialized Data.init(bytesNoCopy:count:deallocator:), 0x1c089c708. A zero count hands the bytes to
    // the deallocator at once; otherwise the storage adopts them, unowned for .none.
    init(bytesNoCopy bytes: UnsafeMutableRawPointer, count: Int, deallocator: Deallocator) {
        let whichDeallocator = deallocator._deallocator
        if count == 0 {
            deallocator._deallocator(bytes, count)
            _representation = .empty
        } else {
            let storage = __DataStorage(
                bytes: bytes,
                length: count,
                copy: false,
                deallocator: whichDeallocator,
                offset: 0
            )
            if case .none = deallocator {
                storage._capacity |= Int.min
            }
            _representation = _Representation(storage, count: count)
        }
    }

    var count: Int {
        return _representation.count
    }

    var startIndex: Int {
        return _representation.startIndex
    }

    var endIndex: Int {
        return _representation.endIndex
    }

    var isEmpty: Bool {
        return _representation.isEmpty
    }

    func withUnsafeBytes<Result>(_ body: (UnsafeRawBufferPointer) throws -> Result) rethrows -> Result {
        return try _representation.withUnsafeBytes(body)
    }

    // Native Data.mutableSpan.getter, 0x1c08916c4: the representation's mutable bytes as UInt8 elements (no
    // elements without a base).
    var mutableSpan: MutableSpan<UInt8> {
        @_lifetime(&self)
        mutating get {
            var bytes = _representation.mutableBytes
            let (base, count) = bytes.withUnsafeMutableBytes { buffer in
                (buffer.baseAddress, buffer.count)
            }
            let elements = UnsafeMutableBufferPointer<UInt8>(
                start: base?.assumingMemoryBound(to: UInt8.self),
                count: base == nil ? 0 : count
            )
            let span = MutableSpan<UInt8>(_unsafeElements: elements)
            return _overrideLifetime(span, mutating: &self)
        }
    }

    // The stdlib default RandomAccessCollection.distance(from:to:) as specialized for Data (native 0x1c08b6784,
    // reached through the thunks 0x1c08bb7cc and 0x1c08b6780; compiler-generated): both indices within the bounds.
    fileprivate func _distance(from start: Int, to end: Int) -> Int {
        precondition(start >= startIndex && start <= endIndex)
        precondition(end >= startIndex && end <= endIndex)
        return end - start
    }

    // Native specialized DataProtocol<>.copyBytes<A, B>(to:from:) for Data, UInt8 and Range<Int>, 0x1c08a6a70.
    func copyBytes(to pointer: UnsafeMutableBufferPointer<UInt8>, from range: Range<Int>) {
        let destination = pointer.baseAddress!
        precondition(range.lowerBound >= startIndex)
        precondition(range.upperBound <= endIndex)
        withUnsafeBytes { fullBuffer in
            let offset = _distance(from: startIndex, to: range.lowerBound)
            let count = _distance(from: range.lowerBound, to: range.upperBound)
            _ = memcpy(destination, fullBuffer.baseAddress! + offset, Swift.min(pointer.count, count))
        }
    }

    // Native Data.append(_:), 0x1c0891b34: the other bytes appended, nothing for empty data.
    mutating func append(_ other: Data) {
        guard !other.isEmpty else {
            return
        }
        other.withUnsafeBytes { buffer in
            _representation.append(contentsOf: buffer)
        }
    }

    // Native Data.subdata(in:), 0x1c08931f0: a copy of the range (empty data for an empty receiver or range).
    func subdata(in range: Range<Int>) -> Data {
        if isEmpty || range.upperBound - range.lowerBound == 0 {
            return Data()
        }
        let slice = Data(representation: _representation[bounds: range])
        return slice.withUnsafeBytes { buffer in
            Data(buffer)
        }
    }

    // Native Data.hash(into:), 0x1c0896730: the count, then the bytes (none for empty data).
    func hash(into hasher: inout Hasher) {
        switch _representation {
        case .empty:
            hasher.combine(0)
        case .inline(let inline):
            hasher.combine(inline.count)
            inline.withUnsafeBytes { hasher.combine(bytes: $0) }
        case .slice(let slice):
            hasher.combine(slice.count)
            slice.withUnsafeBytes { hasher.combine(bytes: $0) }
        case .large(let slice):
            hasher.combine(slice.count)
            slice.withUnsafeBytes { hasher.combine(bytes: $0) }
        }
    }

    // Native specialized static Data.==(_:_:), 0x1c089bd98, with its closure 0x1c0896f48: two empty
    // representations are equal; otherwise equal counts and equal bytes (identical buffers compare equal).
    static func == (d1: Data, d2: Data) -> Bool {
        if case .empty = d1._representation, case .empty = d2._representation {
            return true
        }
        let length1 = d1.count
        if length1 != d2.count {
            return false
        }
        if length1 > 0 {
            return d1.withUnsafeBytes { b1 in
                d2.withUnsafeBytes { b2 in
                    let base1 = b1.baseAddress!
                    let base2 = b2.baseAddress!
                    if base1 == base2 {
                        return true
                    }
                    return memcmp(base1, base2, length1) == 0
                }
            }
        }
        return true
    }

    // Native Data.Iterator: the data, a 32-byte window of it, and the current and end indices.
    struct Iterator {
        let _data: Data
        var _buffer:
            (
                UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8,
                UInt8, UInt8,
                UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8, UInt8,
                UInt8, UInt8,
                UInt8, UInt8
            )
        var _idx: Data.Index
        let _endIdx: Data.Index

        // Native Data.Iterator.init(_:at:), 0x1c08a6888: the window aligned to 32 bytes from the start index,
        // filled with copyBytes(to:from:).
        init(_ data: Data, at loc: Data.Index) {
            _data = data
            _buffer = (
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            )
            _idx = loc
            _endIdx = data.endIndex
            let bufferSize = MemoryLayout.size(ofValue: _buffer)
            Swift.withUnsafeMutableBytes(of: &_buffer) { buffer in
                let start = loc - (loc - data.startIndex) % bufferSize
                let end = _endIdx - start > bufferSize ? start + bufferSize : _endIdx
                data.copyBytes(to: buffer.assumingMemoryBound(to: UInt8.self), from: start..<end)
            }
        }
    }

    typealias Index = Int
}

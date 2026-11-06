//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified (assembly read in full; fields from the reflection metadata, nominal type descriptor
// 0x1c145be2c). iOS Device divergence: iOS Device 24A446 remains unread.
//
// Native source path Foundation/DataStorage.swift (its fatal error). __DataStorage's members that the native image
// inlines at every caller (mutableCopy(_:), replaceBytes(in:with:length:), withUnsafeBytes(in:apply:),
// withUnsafeMutableBytes(in:apply:) and the deallocation of the old buffer) are written here once and cited at
// their first restored caller. Compiler-generated pieces are cited and never written: the type metadata accessor
// 0x1c088da14, the allocating initializers (0x1c088bcd4, 0x1c088bd20, 0x1c088bd5c; swift_allocObject of 0x41
// bytes, then the initializer), the C bit-field setter 0x1c08871fc of malloc_type_layout_semantics_v0_t and the
// outlined copies and consumes of the deallocator closure.

// Native _pageSize (fileprivate, discriminator _94D41B673AAD2ECFC4609546347998E1), one-time initializer 0x1c0c4339c:
// the VM page size as Int (a trap if it does not fit).
private let _pageSize: Int = Int(_platform_shims_vm_size())

// The malloc type identifier of every Data allocation, built inline at each native call site: a type summary
// whose layout semantics contain generic data, in the upper word (the hash is zero).
@inline(__always)
private func _dataMallocTypeID() -> malloc_type_id_t {
    var layoutSemantics = malloc_type_layout_semantics_v0_t()
    layoutSemantics.contains_generic_data = true
    var summary = malloc_type_summary_v0_t()
    summary.layout_semantics = layoutSemantics
    var descriptor = malloc_type_descriptor_v0_t()
    descriptor.summary = summary
    return descriptor.type_id
}

// Native __DataStorage (Swift class, metadata accessor 0x1c088da14; instance size 0x41): a byte buffer with its
// length, capacity, the offset of its first byte in the Data index space, an optional deallocator and whether
// bytes beyond the length must be zeroed before use. A capacity with the sign bit set marks a buffer that is
// neither owned nor resized in place (Data(bytesNoCopy:count:deallocator: .none)).
final class __DataStorage {
    // Native static __DataStorage.vmOpsThreshold, one-time initializer 0x1c088cba8: four pages.
    static let vmOpsThreshold: Int = _pageSize * 4

    var _bytes: UnsafeMutableRawPointer? = nil
    var _length: Int
    var _capacity: Int
    var _offset: Int
    var _deallocator: ((UnsafeMutableRawPointer, Int) -> Void)? = nil
    var _needToZero: Bool

    // Native specialized static __DataStorage.move(_:_:_:), 0x1c088d820: page-aligned copies of at least
    // vmOpsThreshold bytes copy whole pages with vm_copy (memmove if it fails), then memmove the rest.
    static func move(_ dest: UnsafeMutableRawPointer, _ source: UnsafeRawPointer?, _ num: Int) {
        var dest = dest
        var source = source
        var num = num
        if __DataStorage.vmOpsThreshold <= num {
            let pageMask = _pageSize - 1
            if (UInt(bitPattern: source) | UInt(bitPattern: dest)) & UInt(bitPattern: pageMask) == 0 {
                let unwrappedSource = source!
                let pages = num & ~pageMask
                if vm_copy(
                    _platform_mach_task_self(),
                    vm_address_t(UInt(bitPattern: unwrappedSource)),
                    vm_size_t(pages),
                    vm_address_t(UInt(bitPattern: dest))
                ) != KERN_SUCCESS {
                    memmove(dest, unwrappedSource, pages)
                }
                num -= pages
                source = unwrappedSource + pages
                dest = dest + pages
            }
        }
        if num > 0 {
            memmove(dest, source!, num)
        }
    }

    // The deallocation of the current buffer, inlined at each native caller: the deallocator if any, otherwise
    // free; then the deallocator is dropped.
    @inline(__always)
    private func _freeBytes() {
        if let bytes = _bytes {
            if let deallocator = _deallocator {
                deallocator(bytes, _length)
            } else {
                free(bytes)
            }
        }
        _deallocator = nil
    }

    // Native __DataStorage.length.setter, 0x1c088cf88 (the getter is inlined at its callers). Growth within the
    // owned capacity zeroes the new bytes when required; shrinking marks the tail as needing zeroing; other growth
    // reallocates, cleared.
    var length: Int {
        get {
            return _length
        }
        set {
            let origLength = _length
            if (_capacity & Int.max) >= newValue, let bytes = _bytes {
                if origLength < newValue && _needToZero {
                    memset(bytes + origLength, 0, newValue - origLength)
                } else if newValue < origLength {
                    _needToZero = true
                }
            } else {
                ensureUniqueBufferReference(growingTo: newValue, clear: true)
            }
            _length = newValue
        }
    }

    // Native __DataStorage.setLength(_:), 0x1c088db38: the length setter.
    func setLength(_ length: Int) {
        self.length = length
    }

    // Native __DataStorage.ensureUniqueBufferReference(growingTo:clear:), 0x1c088bf0c. An owned buffer of
    // sufficient nonzero capacity is kept. Otherwise a buffer is allocated (growing by a half, or a quarter from
    // vmOpsThreshold, and to the malloc good size), cleared with calloc above 128 KiB when asked, by realloc when
    // the buffer has no deallocator, with fallbacks to the exact length; the old bytes are moved and released.
    func ensureUniqueBufferReference(growingTo newLength: Int = 0, clear: Bool = false) {
        var allocateCapacity = _capacity
        if allocateCapacity != 0 && allocateCapacity >= newLength {
            return
        }
        if newLength == 0 {
            if allocateCapacity == 0 {
                allocateCapacity = malloc_good_size(_length)
                let newBytes = malloc_type_malloc(allocateCapacity, _dataMallocTypeID())
                if _bytes != nil {
                    __DataStorage.move(newBytes!, _bytes, _length)
                }
                _freeBytes()
                _bytes = newBytes
                _capacity = allocateCapacity
                _needToZero = false
            }
            return
        }
        if allocateCapacity == 0 {
            allocateCapacity = malloc_good_size(newLength)
            let newBytes: UnsafeMutableRawPointer?
            if clear {
                newBytes = malloc_type_calloc(1, allocateCapacity, _dataMallocTypeID())
            } else {
                newBytes = malloc_type_malloc(allocateCapacity, _dataMallocTypeID())
            }
            if _bytes != nil {
                __DataStorage.move(newBytes!, _bytes, _length)
            }
            _freeBytes()
            _bytes = newBytes
            _capacity = allocateCapacity
            _needToZero = true
            return
        }

        let shift = newLength >= __DataStorage.vmOpsThreshold ? 2 : 1
        var growth = newLength >> shift
        if Int.max - growth < newLength {
            growth = 0
        }
        let wanted = newLength + growth
        allocateCapacity = malloc_good_size(wanted <= allocateCapacity ? allocateCapacity : wanted)
        var shouldClear = clear && allocateCapacity > 0x20000
        let origLength = _length
        var newBytes: UnsafeMutableRawPointer?
        var zeroed: Bool

        if _bytes == nil {
            if shouldClear {
                newBytes = malloc_type_calloc(1, allocateCapacity, _dataMallocTypeID())
            } else {
                newBytes = malloc_type_malloc(allocateCapacity, _dataMallocTypeID())
            }
            if newBytes == nil {
                if clear && newLength > 0x20000 {
                    shouldClear = true
                    newBytes = malloc_type_calloc(1, newLength, _dataMallocTypeID())
                } else {
                    newBytes = malloc_type_malloc(newLength, _dataMallocTypeID())
                    shouldClear = false
                }
                if newBytes == nil {
                    fatalError(
                        "unable to allocate memory for length (\(newLength))",
                        file: "Foundation/DataStorage.swift",
                        line: 379
                    )
                }
            }
            zeroed = (origLength < newLength && clear) || shouldClear
            if origLength < newLength && clear && !shouldClear {
                memset(newBytes! + origLength, 0, newLength - origLength)
                zeroed = false
            }
        } else {
            var tryCalloc: Bool
            var callocFirst: Bool
            if origLength != 0 {
                let ratioAboveThree = newLength / origLength > 3
                callocFirst = ratioAboveThree && clear && allocateCapacity > 0x20000
                tryCalloc = ratioAboveThree && !shouldClear
            } else {
                callocFirst = clear && allocateCapacity > 0x20000
                tryCalloc = true
            }
            var done = false
            zeroed = false
            if callocFirst {
                tryCalloc = true
                newBytes = malloc_type_calloc(1, allocateCapacity, _dataMallocTypeID())
                if newBytes != nil {
                    __DataStorage.move(newBytes!, _bytes!, _length)
                    _freeBytes()
                    zeroed = (origLength < newLength && clear) || shouldClear
                    if origLength < newLength && clear && !shouldClear {
                        memset(newBytes! + origLength, 0, newLength - origLength)
                        zeroed = false
                    }
                    done = true
                }
            }
            if !done {
                if _deallocator != nil {
                    newBytes = malloc_type_calloc(1, allocateCapacity, _dataMallocTypeID())
                    if newBytes != nil {
                        __DataStorage.move(newBytes!, _bytes!, _length)
                        _freeBytes()
                    }
                } else {
                    newBytes = malloc_type_realloc(_bytes!, allocateCapacity, _dataMallocTypeID())
                }
                if newBytes != nil {
                    if origLength < newLength && clear {
                        memset(newBytes! + origLength, 0, newLength - origLength)
                    }
                    zeroed = false
                    done = true
                }
            }
            if !done {
                allocateCapacity = malloc_good_size(newLength)
                if clear && tryCalloc && allocateCapacity > 0x20000 {
                    newBytes = malloc_type_calloc(1, allocateCapacity, _dataMallocTypeID())
                    if newBytes != nil {
                        __DataStorage.move(newBytes!, _bytes!, _length)
                        _freeBytes()
                        zeroed = true
                        done = true
                    }
                }
            }
            if !done {
                newBytes = malloc_type_realloc(_bytes!, allocateCapacity, _dataMallocTypeID())
                if newBytes == nil {
                    fatalError(
                        "unable to allocate memory for length (\(newLength))",
                        file: "Foundation/DataStorage.swift",
                        line: 379
                    )
                }
                shouldClear = false
                zeroed = origLength < newLength && clear
                if origLength < newLength && clear {
                    memset(newBytes! + origLength, 0, newLength - origLength)
                    zeroed = false
                }
            }
        }
        _bytes = newBytes
        _capacity = allocateCapacity
        _needToZero = !zeroed
    }

    // Native __DataStorage.init(length:), 0x1c088d144: a quarter of headroom below 1 GiB, whole pages from
    // vmOpsThreshold, calloc above 128 KiB; then the length setter.
    init(length: Int) {
        precondition(length < .max / 2)
        var capacity = length
        if length < 1024 * 1024 * 1024 {
            capacity += (length >> 2)
        }
        if __DataStorage.vmOpsThreshold <= capacity {
            capacity = (_pageSize + capacity - 1) & ~(_pageSize - 1)
        }
        let needToZero = length <= 0x20000
        let bytes: UnsafeMutableRawPointer?
        if length <= 0x20000 {
            bytes = malloc_type_malloc(capacity, _dataMallocTypeID())
        } else {
            bytes = malloc_type_calloc(1, capacity, _dataMallocTypeID())
        }
        _needToZero = needToZero
        _bytes = bytes!
        _length = 0
        _capacity = capacity
        _offset = 0
        setLength(length)
    }

    // Native __DataStorage.init(capacity:), 0x1c088d2a0: whole pages from vmOpsThreshold; empty and needing zeroing.
    init(capacity capacity_: Int) {
        precondition(capacity_ < .max / 2)
        var capacity = capacity_
        if __DataStorage.vmOpsThreshold <= capacity {
            capacity = (_pageSize + capacity - 1) & ~(_pageSize - 1)
        }
        _length = 0
        _bytes = malloc_type_malloc(capacity, _dataMallocTypeID())!
        _needToZero = true
        _capacity = capacity
        _offset = 0
    }

    // Native __DataStorage.init(bytes:length:), 0x1c088d3ac: a copy of the bytes (none for a zero length).
    init(bytes: UnsafeRawPointer?, length: Int) {
        precondition(length < .max / 2)
        _offset = 0
        if length == 0 {
            _needToZero = false
            _length = 0
            _capacity = 0
            _bytes = nil
        } else if __DataStorage.vmOpsThreshold <= length {
            _length = length
            _capacity = length
            _needToZero = true
            _bytes = malloc_type_malloc(length, _dataMallocTypeID())!
            __DataStorage.move(_bytes!, bytes, length)
        } else {
            _length = length
            _bytes = malloc_type_malloc(length, _dataMallocTypeID())!
            _capacity = length
            _needToZero = true
            __DataStorage.move(_bytes!, bytes, length)
        }
    }

    // Native __DataStorage.init(bytes:length:copy:deallocator:offset:) (allocating entry 0x1c088c8d8). A copy hands
    // the source to the deallocator at once; otherwise the storage adopts the bytes and the deallocator. A zero
    // length hands any bytes to the deallocator.
    init(
        bytes: UnsafeMutableRawPointer?,
        length: Int,
        copy: Bool,
        deallocator: ((UnsafeMutableRawPointer, Int) -> Void)?,
        offset: Int
    ) {
        precondition(length < .max / 2)
        _offset = offset
        if length == 0 {
            _needToZero = false
            _length = 0
            _capacity = 0
            _bytes = nil
            if let deallocator, let bytes {
                deallocator(bytes, 0)
            }
        } else if !copy {
            _length = length
            _capacity = length
            _needToZero = false
            _bytes = bytes
            _deallocator = deallocator
        } else if __DataStorage.vmOpsThreshold <= length {
            _length = length
            _capacity = length
            _needToZero = true
            _bytes = malloc_type_malloc(length, _dataMallocTypeID())!
            __DataStorage.move(_bytes!, bytes, length)
            if let deallocator {
                deallocator(bytes!, length)
            }
        } else {
            _length = length
            _bytes = malloc_type_malloc(length, _dataMallocTypeID())!
            _capacity = length
            _needToZero = true
            __DataStorage.move(_bytes!, bytes, length)
            if let deallocator {
                deallocator(bytes!, length)
            }
        }
    }

    // Native __DataStorage.init(offset:bytes:capacity:needToZero:length:deallocator:) (allocating entry
    // 0x1c088d68c).
    init(
        offset: Int,
        bytes: UnsafeMutableRawPointer,
        capacity: Int,
        needToZero: Bool,
        length: Int,
        deallocator: ((UnsafeMutableRawPointer, Int) -> Void)?
    ) {
        _offset = offset
        _bytes = bytes
        _capacity = capacity
        _needToZero = needToZero
        _length = length
        _deallocator = deallocator
    }

    // Native __DataStorage.get(_:), inlined at its callers (first restored caller Data._Representation.subscript
    // 0x1c088ae30): the byte at a Data index, read without a nil or overflow check.
    @inline(__always)
    func get(_ index: Int) -> UInt8 {
        return UnsafeRawPointer(_bytes.unsafelyUnwrapped).load(
            fromByteOffset: index &- _offset, as: UInt8.self)
    }

    // Native __DataStorage.mutableCopy(_:), inlined at its callers (first restored caller
    // Data.InlineSlice.ensureUniqueReference() 0x1c088673c): a copy of the range, at the range's offset.
    @inline(__always)
    func mutableCopy(_ range: Range<Int>) -> __DataStorage {
        return __DataStorage(
            bytes: _bytes?.advanced(by: range.lowerBound - _offset),
            length: range.count,
            copy: true,
            deallocator: nil,
            offset: range.lowerBound
        )
    }

    // Native __DataStorage.withUnsafeBytes(in:apply:), inlined at its callers (first restored caller Data.append(_:)
    // 0x1c0891b34): the range clamped to the length; without bytes, an empty buffer at a zeroed local.
    @inline(__always)
    func withUnsafeBytes<Result>(
        in range: Range<Int>,
        apply: (UnsafeRawBufferPointer) throws -> Result
    ) rethrows -> Result {
        if let bytes = _bytes {
            return try apply(
                UnsafeRawBufferPointer(
                    start: bytes.advanced(by: range.lowerBound - _offset),
                    count: Swift.min(range.upperBound - range.lowerBound, _length)
                )
            )
        }
        var zero = 0
        return try Swift.withUnsafeMutableBytes(of: &zero) { local in
            try apply(UnsafeRawBufferPointer(start: local.baseAddress, count: 0))
        }
    }

    // Native __DataStorage.withUnsafeMutableBytes(in:apply:), inlined at its callers (first restored caller
    // Data.init(repeating:count:) 0x1c089c37c), as withUnsafeBytes(in:apply:).
    @inline(__always)
    func withUnsafeMutableBytes<Result>(
        in range: Range<Int>,
        apply: (UnsafeMutableRawBufferPointer) throws -> Result
    ) rethrows -> Result {
        if let bytes = _bytes {
            return try apply(
                UnsafeMutableRawBufferPointer(
                    start: bytes.advanced(by: range.lowerBound - _offset),
                    count: Swift.min(range.upperBound - range.lowerBound, _length)
                )
            )
        }
        var zero = 0
        return try Swift.withUnsafeMutableBytes(of: &zero) { local in
            try apply(UnsafeMutableRawBufferPointer(start: local.baseAddress, count: 0))
        }
    }

    // Native __DataStorage.replaceBytes(in:with:length:), inlined at its callers (first restored caller
    // Data.InlineSlice.replaceSubrange(_:with:count:) 0x1c0886ffc). The range is in the Data index space; the tail
    // moves when the length changes, the replacement is copied (or zeroed without bytes), and a shorter result
    // sets the length.
    @inline(__always)
    func replaceBytes(
        in subrange: (location: Int, length: Int), with replacementBytes: UnsafeRawPointer?,
        length replacementLength: Int
    ) {
        let range = (location: subrange.location - _offset, length: subrange.length)
        let currentLength = _length
        let resultingLength = currentLength - range.length + replacementLength
        let shift = resultingLength - currentLength
        if resultingLength > currentLength {
            ensureUniqueBufferReference(growingTo: resultingLength)
            _length = resultingLength
        } else {
            ensureUniqueBufferReference()
        }
        let mutableBytes = _bytes!
        let start = range.location
        if shift != 0 {
            let destination = mutableBytes + start + replacementLength
            let source = mutableBytes + start + range.length
            memmove(destination, source, currentLength - start - range.length)
        }
        if replacementLength != 0 {
            if let replacementBytes {
                memmove(mutableBytes + start, replacementBytes, replacementLength)
            } else {
                memset(mutableBytes + start, 0, replacementLength)
            }
        }
        if resultingLength < currentLength {
            setLength(resultingLength)
        }
    }

    // Native __DataStorage.deinit, 0x1c088d710 (deallocating deinit 0x1c088d794; reached through swift_release, an
    // edge the scope closure does not follow): the buffer goes to the deallocator, or to free without one.
    deinit {
        if let bytes = _bytes {
            if let deallocator = _deallocator {
                deallocator(bytes, _length)
            } else {
                free(bytes)
            }
        }
    }
}

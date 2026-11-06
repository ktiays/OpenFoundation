//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified (assembly read in full; __NSSwiftData's fields from the reflection metadata). iOS Device
// divergence: iOS Device 24A446 remains unread.
//
// Native source path Foundation/Data+Bridging.swift (its fatal error). NSData's own methods, which __NSSwiftData
// overrides and messages through super, are Foundation's NSData(NSData) category, outside the restored scope: those
// sends are indirect edges that stay open (Data/ONSDataSwiftSupport.h). Compiler-generated pieces are cited and
// never written: the __NSSwiftData metadata accessor 0x1c08a4624, its ivar destroyer 0x1c08a57d8 and deallocating
// deinit, and the @objc thunks (each cited at its member). __NSSwiftData.init(contentsOf:) 0x1c08a5584 (thunk
// 0x1c08a5644) is written with the URL group, whose _BridgedURL it needs.
//
// Data's _ObjectiveCBridgeable conformance is not declared: only its _bridgeToObjectiveC and
// _unconditionallyBridgeFromObjectiveC witnesses are in the scope; they are written as plain members.

import ObjectiveC

// Native __NSSwiftData (Swift class, runtime name _TtC10Foundation13__NSSwiftData): the NSData that a sliced Data
// bridges to, viewing a range of the Data's storage. Ivars _backing (__DataStorage?) and _range (Range<Int>?).
final class __NSSwiftData: ONSData {
    var _backing: __DataStorage! = nil
    var _range: Range<Int>! = nil

    // Native @objc __NSSwiftData.init(), 0x1c08a48fc. Dynamic, so Swift callers send -init, as the native ones do.
    @objc override dynamic init() {
        super.init()
    }

    // Native @objc __NSSwiftData.init(coder:), 0x1c08a495c.
    @objc override init!(coder: ONSCoder!) {
        super.init(coder: coder)
    }

    // Native @objc __NSSwiftData.init(bytes:length:), 0x1c08a4df0 (merged body 0x1c08a4e08).
    @objc override init!(bytes: UnsafeRawPointer!, length: UInt) {
        super.init(bytes: bytes, length: length)
    }

    // Native @objc __NSSwiftData.init(bytesNoCopy:length:freeWhenDone:), 0x1c08a4e80.
    @objc override init!(bytesNoCopy bytes: UnsafeMutableRawPointer!, length: UInt, freeWhenDone: Bool) {
        super.init(bytesNoCopy: bytes, length: length, freeWhenDone: freeWhenDone)
    }

    // Native @objc __NSSwiftData.init(contentsOfFile:), 0x1c08a5548, and init(contentsOfMappedFile:), 0x1c08a4bd8
    // (merged thunk 0x1c08a5554 and merged body 0x1c08a5494, parameterized by the super selector): the path bridged
    // to String and back for the super message.
    @objc override convenience init!(contentsOfFile path: ONSString!) {
        self.init(_contentsOf: String._unconditionallyBridgeFromObjectiveC(path), mapped: false)
    }

    @objc override convenience init!(contentsOfMappedFile path: ONSString!) {
        self.init(_contentsOf: String._unconditionallyBridgeFromObjectiveC(path), mapped: true)
    }

    private init?(_contentsOf path: String, mapped: Bool) {
        let object = unsafeDowncast(path._bridgeToObjectiveCImpl(), to: ONSString.self)
        if mapped {
            super.init(contentsOfMappedFile: object)
        } else {
            super.init(contentsOfFile: object)
        }
    }

    // Native @objc __NSSwiftData.length.getter, 0x1c08a4644: the range's count.
    @objc override func length() -> UInt {
        return UInt(bitPattern: _range.upperBound - _range.lowerBound)
    }

    // Native __NSSwiftData.bytes.getter, 0x1c08a4694 (@objc thunk 0x1c08a4674): the storage's bytes at the range
    // (computed without overflow checks), or the placeholder 0xBAD0 when the storage has none.
    @objc override func bytes() -> UnsafeRawPointer! {
        let backing = _backing!
        guard let bytes = backing._bytes else {
            return UnsafeRawPointer(bitPattern: 0xBAD0)!
        }
        return UnsafeRawPointer(bytes) + (0 &- backing._offset) + _range.lowerBound
    }

    // Native __NSSwiftData.copy(with:), 0x1c08a46e8 (@objc thunk 0x1c08a47dc bridges the result with
    // _bridgeAnythingToObjectiveC): an unowned storage is copied into a new NSData; otherwise the receiver.
    @objc override func copy(with zone: NSZone!) -> Any! {
        let backing = _backing!
        if backing._capacity < 0 {
            return ONSData(bytes: bytes(), length: length())
        }
        return self
    }

    // Native @objc __NSSwiftData.mutableCopy(with:), 0x1c08a4838: a new NSMutableData with the bytes.
    @objc override func mutableCopy(with zone: NSZone!) -> Any! {
        return ONSMutableData(bytes: bytes(), length: length())
    }

    // Native @objc __NSSwiftData._bridgingCopy(_:length:), 0x1c08a48a4: never sent to this class.
    @objc override func _bridgingCopy(
        _ bytes: UnsafeMutablePointer<UnsafeRawPointer?>!,
        length: UnsafeMutablePointer<UInt>!
    ) -> ONSData! {
        fatalError(
            "Unexpected call to __NSSwiftData._bridgingCopy(_:length:)",
            file: "Foundation/Data+Bridging.swift",
            line: 80
        )
    }
}

extension __DataStorage {
    // Native __DataStorage.bridgedReference(_:), 0x1c08a458c: an empty NSData for an empty range, otherwise a
    // __NSSwiftData viewing the range.
    func bridgedReference(_ range: Range<Int>) -> AnyObject {
        if range.lowerBound == range.upperBound {
            return ONSData()
        }
        let data = __NSSwiftData()
        data._backing = self
        data._range = range
        return data
    }
}

extension Data {
    // Native Data._bridgeToObjectiveC(), 0x1c0dbd4ec: an empty NSData, a copy of in-place bytes, or the storage's
    // bridged reference.
    func _bridgeToObjectiveC() -> ONSData {
        switch _representation {
        case .empty:
            return ONSData()
        case .inline(let inline):
            return inline.withUnsafeBytes { buffer in
                ONSData(bytes: buffer.baseAddress, length: UInt(buffer.count))
            }
        case .slice(let slice):
            return unsafeDowncast(slice.storage.bridgedReference(slice.range), to: ONSData.self)
        case .large(let slice):
            return unsafeDowncast(slice.storage.bridgedReference(slice.range), to: ONSData.self)
        }
    }

    // Native specialized static Data._bridgeFromObjectiveCAdoptingNativeStorageOf(_:), 0x1c08a5838: a
    // __NSSwiftData's storage is shared when its range starts at zero, otherwise its range is copied.
    static func _bridgeFromObjectiveCAdoptingNativeStorageOf(_ source: AnyObject) -> Data? {
        guard let cls = object_getClass(source), ObjectIdentifier(cls) == ObjectIdentifier(__NSSwiftData.self)
        else {
            return nil
        }
        let nativeData = unsafeDowncast(source, to: __NSSwiftData.self)
        let range = nativeData._range!
        let backing = nativeData._backing!
        if range.lowerBound == 0 {
            if InlineSlice.canStore(count: range.upperBound) {
                return Data(representation: .slice(InlineSlice(backing, range: range)))
            }
            return Data(representation: .large(LargeSlice(backing, range: range)))
        }
        let bytes = backing._bytes.map { UnsafeRawPointer($0) + range.lowerBound + (0 &- backing._offset) }
        let storage = __DataStorage(bytes: bytes, length: range.upperBound - range.lowerBound)
        let count = storage._length
        if InlineSlice.canStore(count: count) {
            return Data(representation: .slice(InlineSlice(storage, count: count)))
        }
        return Data(representation: .large(LargeSlice(storage, count: count)))
    }

    // Native specialized static Data._unconditionallyBridgeFromObjectiveC(_:), 0x1c0dbd9c8, with Data.init(referencing:)
    // inlined: empty data for nil; an adopted __NSSwiftData; otherwise the bytes of -_bridgingCopy:length:, kept
    // alive by the returned object (closure #1, whose partial apply 0x1c0ba11c8 is identical-code folded natively with
    // _NSBundleDeallocatingImmortalBundle) or freed (closure #2 0x1c0dbd640).
    static func _unconditionallyBridgeFromObjectiveC(_ source: ONSData?) -> Data {
        guard let source else {
            return Data()
        }
        if let data = Data._bridgeFromObjectiveCAdoptingNativeStorageOf(source) {
            return data
        }
        var length: UInt = 0
        var bytes: UnsafeRawPointer? = nil
        let bridged: ONSData? = source._bridgingCopy(&bytes, length: &length)
        guard let bytes else {
            return Data()
        }
        let deallocator: Data.Deallocator
        if let bridged {
            deallocator = .custom({ _, _ in
                withExtendedLifetime(bridged) {}
            })
        } else {
            deallocator = .custom({ pointer, _ in
                free(pointer)
            })
        }
        return Data(
            bytesNoCopy: UnsafeMutableRawPointer(mutating: bytes),
            count: Int(bitPattern: length),
            deallocator: deallocator
        )
    }
}

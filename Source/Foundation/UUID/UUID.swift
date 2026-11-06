//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified (assembly read in full; layouts from the reflection metadata). iOS Device divergence:
// iOS Device 24A446 remains unread.
//
// Inferred organization: the UUID value type (0x1c0c45ef8-0x1c0c46018) and its Objective-C class
// __NSConcreteUUID (0x1c0c46d80-0x1c0c49690). Compiler-generated pieces are cited at their source cause and
// never written: the type metadata accessors (__NSConcreteUUID 0x1c0c46aec, NSUUID 0x1c0c49854), the lazy
// _Pointer witness table accessor of UnsafePointer<__NSConcreteUUID> 0x1c0c499d4, and the @objc thunks (each
// cited at its member).
//
// ABI divergence: native Foundation is built with library evolution, so UUID is resilient there
// (UUID.init(uuidString:) returns its Optional indirectly); this module is not. Behavior is unchanged.

// Native UUID: the 16 bytes of a universally unique identifier. Only the members the scope reaches are restored.
struct UUID {
    var uuid: uuid_t

    init(uuid: uuid_t) {
        self.uuid = uuid
    }

    // Native UUID.init(uuidString:), 0x1c0c45f78: the string's UTF-8 C string parsed with
    // _foundation_uuid_parse; nil when it fails.
    init?(uuidString string: String) {
        var uuid: uuid_t = (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        let result = string.utf8CString.withUnsafeBufferPointer { buffer in
            withUnsafeMutableBytes(of: &uuid) { bytes in
                _foundation_uuid_parse(
                    buffer.baseAddress, bytes.baseAddress!.assumingMemoryBound(to: UInt8.self))
            }
        }
        if result != 0 {
            return nil
        }
        self.init(uuid: uuid)
    }

    // Native UUID.uuidString.getter, 0x1c0c45ef8: the 37-byte zeroed buffer filled by
    // _foundation_uuid_unparse_upper, read as a C string.
    var uuidString: String {
        var bytes: uuid_string_t = (
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0,
            0, 0
        )
        var uuid = self.uuid
        return withUnsafeMutableBytes(of: &bytes) { out in
            withUnsafeBytes(of: &uuid) { uu in
                _foundation_uuid_unparse_upper(
                    uu.baseAddress!.assumingMemoryBound(to: UInt8.self),
                    out.baseAddress!.assumingMemoryBound(to: CChar.self)
                )
            }
            return String(cString: out.baseAddress!.assumingMemoryBound(to: CChar.self))
        }
    }
}

// Native __NSConcreteUUID (Objective-C class 0x22e6e3098, class_ro_t flags 0x80, instance size 24): the
// concrete NSUUID that +[NSUUID allocWithZone:] answers, holding its bytes in the ivar _storage (offset 8).
@objc(__ONSConcreteUUID)
final class __ONSConcreteUUID: _ONSUUIDBridge {
    var _storage: UUID

    // Native static __NSConcreteUUID.supportsSecureCoding.getter, @objc thunk 0x1c0c46e18.
    @objc override class func supportsSecureCoding() -> Bool {
        return true
    }

    // Native __NSConcreteUUID.init(), 0x1c0c46d80 (@objc thunk 0x1c0c46df8): random bytes, then super.init
    // (-[_NSUUIDBridge init]).
    override init() {
        var uuid: uuid_t = (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
        withUnsafeMutableBytes(of: &uuid) { bytes in
            _foundation_uuid_generate_random(bytes.baseAddress!.assumingMemoryBound(to: UInt8.self))
        }
        _storage = UUID(uuid: uuid)
        super.init()
    }

    // Native __NSConcreteUUID.init(coder:), 0x1c0c46e20 (@objc thunk 0x1c0c48cd0). A non-keyed coder fails with
    // NSCoderReadCorruptError (4864); missing bytes with NSCoderValueNotFoundError (4865) and bytes of another
    // length with 4864, each answering the zero UUID instead under +[NSUUID _compatibilityBehavior]. A failure
    // is bridged to NSError (_bridgeErrorToNSError) and sent with -failWithError:, and the initializer answers
    // nil.
    @objc override init?(coder: ONSCoder) {
        guard coder.allowsKeyedCoding() else {
            let error = CocoaError(
                CocoaError.Code(rawValue: 4864),
                userInfo: [
                    String._unconditionallyBridgeFromObjectiveC(debugDescriptionErrorKeyObject):
                        "Cannot be decoded without keyed coding"
                ]
            )
            coder.failWithError(_bridgeErrorToNSError(error))
            return nil
        }
        var length = 0
        guard
            let bytes = coder.decodeBytes(
                forKey: "NS.uuidbytes"._bridgeToObjectiveCImpl(), returnedLength: &length)
        else {
            if ONSUUID._compatibilityBehavior() {
                _storage = UUID(uuid: (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
                super.init()
                return
            }
            let error = CocoaError(
                CocoaError.Code(rawValue: 4865),
                userInfo: [
                    String._unconditionallyBridgeFromObjectiveC(debugDescriptionErrorKeyObject):
                        "UUID bytes not found in archive"
                ]
            )
            coder.failWithError(_bridgeErrorToNSError(error))
            return nil
        }
        guard length == 16 else {
            if ONSUUID._compatibilityBehavior() {
                _storage = UUID(uuid: (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
                super.init()
                return
            }
            let error = CocoaError(
                CocoaError.Code(rawValue: 4864),
                userInfo: [
                    String._unconditionallyBridgeFromObjectiveC(debugDescriptionErrorKeyObject):
                        "UUID bytes were not the expected length"
                ]
            )
            coder.failWithError(_bridgeErrorToNSError(error))
            return nil
        }
        _storage = UUID(uuid: UnsafeRawPointer(bytes).load(as: uuid_t.self))
        super.init()
    }

    // Native __NSConcreteUUID.encode(with:), @objc thunk 0x1c0c48cf4: the 16 bytes under "NS.uuidbytes".
    @objc override func encode(with coder: ONSCoder) {
        var uuid = _storage.uuid
        let key = "NS.uuidbytes"._bridgeToObjectiveCImpl()
        withUnsafeBytes(of: &uuid) { bytes in
            coder.encodeBytes(bytes.baseAddress!.assumingMemoryBound(to: UInt8.self), length: 16, forKey: key)
        }
    }

    // Native __NSConcreteUUID.init(uuidString:), 0x1c0c48d94 (@objc thunk 0x1c0c48ed0 bridges the string with
    // String._unconditionallyBridgeFromObjectiveC). An unparsable string answers the zero UUID under
    // +[NSUUID _compatibilityBehavior], otherwise nil.
    @objc override convenience init?(uuidString string: ONSString?) {
        self.init(uuidString: String._unconditionallyBridgeFromObjectiveC(string))
    }

    init?(uuidString string: String) {
        guard let uuid = UUID(uuidString: string) else {
            if ONSUUID._compatibilityBehavior() {
                _storage = UUID(uuid: (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
                super.init()
                return
            }
            return nil
        }
        _storage = uuid
        super.init()
    }

    // Native __NSConcreteUUID.init(uuidBytes:), 0x1c0c48ef8 (@objc thunk 0x1c0c48f44): the 16 bytes, or zero
    // for a null pointer.
    @objc override init(uuidBytes bytes: UnsafePointer<UInt8>?) {
        if let bytes {
            _storage = UUID(uuid: UnsafeRawPointer(bytes).load(as: uuid_t.self))
        } else {
            _storage = UUID(uuid: (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
        }
        super.init()
    }

    // Native __NSConcreteUUID.getBytes(_:), @objc thunk 0x1c0c48f68 (selector getUUIDBytes:).
    @objc override func getBytes(_ bytes: UnsafeMutablePointer<UInt8>) {
        UnsafeMutableRawPointer(bytes).storeBytes(of: _storage.uuid, as: uuid_t.self)
    }

    // Native __NSConcreteUUID.uuidString.getter, @objc thunk 0x1c0c48fbc (selector UUIDString).
    @objc override func uuidString() -> ONSString {
        return _storage.uuidString._bridgeToObjectiveC()
    }

    // Native __NSConcreteUUID.description.getter, @objc thunk 0x1c0c4900c: -UUIDString sent as a message (the
    // upcast keeps the Objective-C dispatch), bridged to String and back.
    override var description: ONSString {
        let string = String._unconditionallyBridgeFromObjectiveC((self as ONSUUID).uuidString())
        return string._bridgeToObjectiveC()
    }

    // Native __NSConcreteUUID.debugDescription.getter, @objc thunk 0x1c0c49074, with its closure 0x1c0c49118 (run
    // in withUnsafePointer(to: self), so the pointer printed is that of the temporary holding self).
    override var debugDescription: ONSString {
        let objectType = type(of: self)
        let string = withUnsafePointer(to: self) { pointer in
            var string = "<"
            string.append(_typeName(objectType, qualified: false))
            string.append(" ")
            string.append(pointer.debugDescription)
            string.append("> ")
            string.append(String._unconditionallyBridgeFromObjectiveC((self as ONSUUID).uuidString()))
            return string
        }
        return string._bridgeToObjectiveC()
    }

    // Native __NSConcreteUUID.classForCoder.getter, @objc thunk 0x1c0c49690: the NSUUID class.
    @objc override func classForCoder() -> AnyClass {
        return ONSUUID.self
    }
}

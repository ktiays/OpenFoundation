//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified (assembly read in full; layouts from the reflection metadata, nominal type descriptors
// 0x1c145c158, 0x1c145c1c8, 0x1c145c274 and 0x1c14649c0). iOS Device divergence: iOS Device 24A446 remains
// unread.
//
// The bridged error types. Only the members the captured code reaches are restored; the members the overlay
// inlines at their callers (the _BridgedStoredNSError initializers and constants) are written with their first
// restored caller.
//
// The Error conformances are reached through their conformance descriptors (CocoaError 0x1c1020684, POSIXError
// 0x1c10208ec, _GenericObjCError 0x1c127541c; swift_getWitnessTable), an indirect edge the scope closure does
// not follow; their resilient witnesses were resolved by reading. The witness thunks are compiler-generated and
// identical-code folded natively: _domain is the type's errorDomain witness (CocoaError 0x1c08d17e0,
// POSIXError 0x1c08d5024), _code (0x1c08d1890) reads -code, _getEmbeddedNSError (0x1c08d18a0) answers the
// stored error retained, and _userInfo (0x1c083e11c) is the standard library default; _GenericObjCError uses
// the standard library defaults for all four (0x1c083e114-0x1c083e120). The lazy Error witness table accessors
// (CocoaError 0x1c08d3a10, POSIXError 0x1c08d5540) are compiler-generated.
//
// ABI divergence: native Foundation is built with library evolution, so its public value types are resilient
// (CocoaError.Code.init(fileErrno:reading:) returns its result indirectly); this module is not, so the same
// functions use the direct convention. Behavior is unchanged.

// Native CocoaError: the stored NSError of NSCocoaErrorDomain.
struct CocoaError: Error {
    // Native CocoaError.Code: the NSCocoaErrorDomain code.
    struct Code: RawRepresentable {
        let rawValue: Int

        init(rawValue: Int) {
            self.rawValue = rawValue
        }

        // Native CocoaError.Code.init(fileErrno:reading:), 0x1c08d3b9c. Reading: EPERM and EACCES are
        // fileReadNoPermission (257), ENOENT fileReadNoSuchFile (260), EFBIG fileReadTooLarge (263),
        // ENAMETOOLONG fileReadInvalidFileName (258), otherwise fileReadUnknown (256). Writing: EPERM and EACCES
        // are fileWriteNoPermission (513), ENOENT fileNoSuchFile (4), EEXIST fileWriteFileExists (516),
        // ENAMETOOLONG fileWriteInvalidFileName (514), ENOSPC and EDQUOT fileWriteOutOfSpace (640), EROFS
        // fileWriteVolumeReadOnly (642), otherwise fileWriteUnknown (512).
        init(fileErrno: Int32, reading: Bool) {
            if reading {
                switch fileErrno {
                case EPERM, EACCES:
                    self.init(rawValue: 257)
                case ENOENT:
                    self.init(rawValue: 260)
                case EFBIG:
                    self.init(rawValue: 263)
                case ENAMETOOLONG:
                    self.init(rawValue: 258)
                default:
                    self.init(rawValue: 256)
                }
            } else {
                switch fileErrno {
                case EPERM, EACCES:
                    self.init(rawValue: 513)
                case ENOENT:
                    self.init(rawValue: 4)
                case EEXIST:
                    self.init(rawValue: 516)
                case ENAMETOOLONG:
                    self.init(rawValue: 514)
                case ENOSPC, EDQUOT:
                    self.init(rawValue: 640)
                case EROFS:
                    self.init(rawValue: 642)
                default:
                    self.init(rawValue: 512)
                }
            }
        }
    }

    let _nsError: ONSError

    // Native CocoaError.init(_nsError:), inlined at its callers (first restored caller __NSConcreteUUID.init(coder:)
    // 0x1c0c46e20): the error's domain must be NSCocoaErrorDomain (both bridged and compared; a trap otherwise).
    init(_nsError error: ONSError) {
        precondition(
            String._unconditionallyBridgeFromObjectiveC(error.domain())
                == String._unconditionallyBridgeFromObjectiveC(cocoaErrorDomainObject)
        )
        _nsError = error
    }

    // Native _BridgedStoredNSError.init(_:userInfo:) for CocoaError, the overlay's protocol extension inlined at
    // its callers (first restored caller __NSConcreteUUID.init(coder:) 0x1c0c46e20): NSCocoaErrorDomain bridged,
    // then an NSError allocated and initialized with the domain, the code and the user info (bridged with
    // _NativeDictionary.bridged).
    init(_ code: Code, userInfo: [String: Any]) {
        let domain = String._unconditionallyBridgeFromObjectiveC(cocoaErrorDomainObject)
        self.init(
            _nsError: ONSError(
                domain: domain._bridgeToObjectiveCImpl(),
                code: code.rawValue,
                userInfo: userInfo._bridgeToObjectiveCImpl()
            )
        )
    }

    // Native CocoaError errorDomain witness, 0x1c08d17e0: NSCocoaErrorDomain bridged.
    var _domain: String {
        return String._unconditionallyBridgeFromObjectiveC(cocoaErrorDomainObject)
    }

    // Native CocoaError _code witness, 0x1c08d1890 (shared with POSIXError).
    var _code: Int {
        return _nsError.code()
    }

    // Native CocoaError _getEmbeddedNSError witness, 0x1c08d18a0 (shared with POSIXError).
    func _getEmbeddedNSError() -> AnyObject? {
        return _nsError
    }
}

// Native POSIXError: the stored NSError of NSPOSIXErrorDomain.
struct POSIXError: Error {
    let _nsError: ONSError

    init(_nsError error: ONSError) {
        _nsError = error
    }

    // Native POSIXError errorDomain witness, 0x1c08d5024: NSPOSIXErrorDomain bridged.
    var _domain: String {
        return String._unconditionallyBridgeFromObjectiveC(posixErrorDomainObject)
    }

    // Native POSIXError _code witness, folded with CocoaError's (0x1c08d1890).
    var _code: Int {
        return _nsError.code()
    }

    // Native POSIXError _getEmbeddedNSError witness, folded with CocoaError's (0x1c08d18a0).
    func _getEmbeddedNSError() -> AnyObject? {
        return _nsError
    }
}

// Native _GenericObjCError: the error thrown for an Objective-C failure that answered no NSError.
enum _GenericObjCError: Error {
    case nilError
}

// Native _nilObjCError, one-time initializer 0x1c0dc3af4 (swift_allocError of _GenericObjCError.nilError; the
// Error witness table accessor 0x1c0dccd8c is compiler-generated).
let _nilObjCError: Error = _GenericObjCError.nilError

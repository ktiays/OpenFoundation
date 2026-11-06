//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// iOS Simulator 27.0 (24A434), arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured assembly; iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Scratch/cf-core-fd-misc/Reading.md. Native source organization inferred.

// Supplied libswiftCore entry points, not Foundation implementations. Their private initializer boundaries
// are explicit in native 0x1c0b0b6c0; __owned preserves the retains immediately before each native call.
@_silgen_name("$sSS12_cocoaStringSSyXl_tcfC")
private func cocoaString(_ object: __owned AnyObject) -> String

@_silgen_name("$sSS14_nativeStorageSSSgyXl_tcfC")
private func nativeStringStorage(_ object: __owned AnyObject) -> String?

@_silgen_name("$sSS20_immortalCocoaString5count8encodingSSyXl_Sixmtcs16_UnicodeEncodingRzlufC")
private func immortalCocoaString<Encoding: _UnicodeEncoding>(
    _ object: __owned AnyObject, count: Int, encoding: Encoding.Type
) -> String

@_silgen_name("$ss40_SwiftCreateImmortalString_ForFoundation6buffer7isASCIISSSgSRys5UInt8VG_SbtF")
private func immortalStringForFoundation(buffer: UnsafeBufferPointer<UInt8>, isASCII: Bool) -> String?

extension String {
    // Native String._unconditionallyBridgeFromObjectiveC, 0x1c0b0b6c0. Non-external local copy.
    // libobjc inline _objc_isTaggedPointer 0x1c06ea66c and _objc_getTaggedPointerTag 0x1c06ea730 are
    // folded here: high-bit test, obfuscator exception, seven-entry inverse permutation, extended tag.
    // The unsafeUninitializedCapacity closures induce the compiler-generated pieces:
    // 0x1c06e8964 __swift_instantiateConcreteTypeFromMangledNameV2;
    // 0x1c06ea674, 0x1c06ea698, 0x1c06eb8a8, 0x1c06eb940, 0x1c0796e1c (partial applications/thunks);
    // 0x1c06ea6b0 Unicode.UTF16 witness accessor; 0x1c06ea6f0 Unicode.ASCII witness accessor;
    // 0x1c0b0d040, 0x1c0b0d054, 0x1c0b0d068, 0x1c0b0d07c (partial applications);
    // 0x1c0b0df4c, 0x1c0b0dfb0, 0x1c0b0e000 (merged partial applications).
    // These are cited at their source cause and are never handwritten as mangled function definitions.
    static func _unconditionallyBridgeFromObjectiveC(_ object: UnsafeRawPointer?) -> String {
        guard let object else {
            return ""
        }
        let source = Unmanaged<ONSString>.fromOpaque(object).takeUnretainedValue()
        // Both selected C declarations carry this same native object pointer. Swift imports the native
        // CFStringRef spelling as an object reference, and the prefixed spelling as an opaque C pointer.
        let cf = withUnsafeBytes(of: object) { $0.load(as: DecimalStringPointer.self) }
        let pointer = UInt(bitPattern: object)
        if pointer >> 63 != 0 {
            let tag = Int(decimalStringTaggedPointerTag(object))
            switch tag {
            case 0:
                var count: UInt16 = 0
                let contents = _OCFIndirectTaggedPointerStringGetContents(cf, &count)
                let bytes = UnsafeBufferPointer<UInt8>(start: contents, count: Int(count))
                if let string = immortalStringForFoundation(buffer: bytes, isASCII: true) {
                    return string
                }
                // Native mutableCopy -> Any -> unconditional String dynamic cast, including release.
                let copy: Any = source.mutableCopy() as Any
                return copy as! String
            case 2:
                return String(unsafeUninitializedCapacity: 15) { buffer in
                    // Native closure #1 0x1c0b0c8b4, with partial applications 0x1c06ea7f0, 0x1c06eb814 and
                    // 0x1c0b0d07c. It calls _NSTaggedPointerStringGetBytes.
                    precondition(buffer.baseAddress != nil)
                    return _ONSTaggedPointerStringGetBytes(cf, buffer.baseAddress)
                }
            case 22:
                var count: UInt = 0
                _ = objc_retainAutorelease(UnsafeMutableRawPointer(mutating: object))
                if let contents = source._fastUTF8StringContents(false, utf8Length: &count) {
                    return String(unsafeUninitializedCapacity: Int(count)) { buffer in
                        // Native memcpy closure #2 0x1c0b0c8e8, with thunks 0x1c06ea7d0 and 0x1c06eb854,
                        // and merged partial application 0x1c0b0e000.
                        if count != 0 {
                            precondition(buffer.count >= Int(count))
                            buffer.baseAddress!.update(
                                from: UnsafeRawPointer(contents).assumingMemoryBound(to: UInt8.self),
                                count: Int(count)
                            )
                        }
                        return Int(count)
                    }
                }
                let length = Int(source.length())
                if length == 0 {
                    return ""
                }
                let maximum = length * 3
                if maximum <= 1024 {
                    precondition(maximum >= 0)
                    return withUnsafeTemporaryAllocation(of: UInt8.self, capacity: maximum) { temporary in
                        var used: UInt = 0
                        var remaining = ONSRange(location: 0, length: 0)
                        let converted = source.getBytes(
                            temporary.baseAddress, maxLength: UInt(maximum),
                            usedLength: &used, encoding: 4, options: 0,
                            range: ONSRange(location: 0, length: numericCast(length)), remaining: &remaining)
                        let written = converted ? Int(used) : 0
                        return String(unsafeUninitializedCapacity: maximum) { buffer in
                            // Native memcpy closure #3 0x1c0b0c930, with thunks 0x1c06ea7b0 and 0x1c06eb840,
                            // and merged partial application 0x1c0b0dfb0.
                            if written != 0 {
                                precondition(buffer.count >= written)
                                buffer.baseAddress!.update(from: temporary.baseAddress!, count: written)
                            }
                            return written
                        }
                    }
                }
                let capacity = Int(source.lengthOfBytes(usingEncoding: 4))
                return String(unsafeUninitializedCapacity: capacity) { buffer in
                    // Native closure #4 0x1c0b0c978, with thunks 0x1c06ea790 and 0x1c06eb82c,
                    // and merged partial application 0x1c0b0df4c.
                    var used: UInt = 0
                    var remaining = ONSRange(location: 0, length: 0)
                    let converted = source.getBytes(
                        buffer.baseAddress, maxLength: UInt(capacity),
                        usedLength: &used, encoding: 4, options: 0,
                        range: ONSRange(location: 0, length: numericCast(length)), remaining: &remaining)
                    return converted ? Int(used) : 0
                }
            default:
                break
            }
        }
        var mutable: DarwinBoolean = false
        var length: OCFIndex = 0
        var eightBit: DarwinBoolean = false
        var constant = false
        if !__OCFStringIsCF(cf, &mutable, &length, &eightBit, &constant) {
            if let native = nativeStringStorage(source) {
                return native
            }
            length = Int(source.length())
            if length == 0 {
                return ""
            }
            return cocoaString(source)
        }
        if length == 0 {
            return ""
        }
        if constant {
            if eightBit.boolValue {
                return immortalCocoaString(source, count: length, encoding: Unicode.ASCII.self)
            }
            return immortalCocoaString(source, count: length, encoding: Unicode.UTF16.self)
        }
        if !mutable.boolValue {
            return cocoaString(source)
        }
        let capacity = eightBit.boolValue ? length : Int(source.lengthOfBytes(usingEncoding: 4))
        let encoding: OCFStringEncoding = eightBit.boolValue ? 0x600 : 0x0800_0100
        let string = String(unsafeUninitializedCapacity: capacity) { buffer in
            // Native closure #1 in closure #5, 0x1c0b0ca10. Non-external Swift body.
            // Assembly settles the pseudocode's incorrect unconditional zero: usedLength is returned only
            // when _CFNonObjCStringGetBytes converts exactly the original UTF-16 length.
            var used: OCFIndex = 0
            // Native 0x1c0b0ca58/0x1c0b0ca8c brackets the C conversion with retain/release.
            let retained = Unmanaged<ONSString>.fromOpaque(object).retain()
            let converted = _OCFNonObjCStringGetBytes(
                cf, OCFRange(location: 0, length: length),
                encoding, 0, false, buffer.baseAddress, capacity, &used)
            retained.release()
            return converted == length ? used : 0
        }
        if string.isEmpty {
            return cocoaString(source)
        }
        return string
    }
}

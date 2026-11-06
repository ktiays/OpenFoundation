//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified (assembly read in full). iOS Device divergence: iOS Device 24A446 remains unread.
//
// The Objective-C-to-Swift dictionary bridge of the Foundation overlay, as far as the restored callers reach
// it. Native Foundation declares Dictionary's _ObjectiveCBridgeable conformance; only its [String: String]
// specialization is reached, inlined at its callers except for one closure, so only that specialization is
// written here, without the conformance.

extension Dictionary where Key == String, Value == String {
    // Native static Dictionary._unconditionallyBridgeFromObjectiveC(_:) for [String: String], with the
    // _forceBridgeFromObjectiveC(_:result:) it calls, inlined at its callers (first restored caller
    // NSMorphologyPronoun.morphunConstraints() 0x1c0f7b6d0). A native dictionary storage is adopted; otherwise an
    // empty dictionary answers the empty singleton, and any other is copied (String keys may collide once bridged,
    // so duplicates are allowed and dropped).
    static func _unconditionallyBridgeFromObjectiveC(_ object: AnyObject) -> [String: String] {
        if let native = _bridgeFromObjectiveCAdoptingNativeStorageOf(object) {
            return native
        }
        let dictionary = unsafeBitCast(object, to: (any DictionaryBridgingMessages).self)
        // NSUInteger imports as UInt here; the native code reads the count as Int.
        let count = Int(bitPattern: dictionary.count())
        if count == 0 {
            return [:]
        }
        return Dictionary(_unsafeUninitializedCapacity: count, allowingDuplicates: true) { keys, values in
            _forceBridgeElements(keys: keys, values: values, from: dictionary, count: count)
        }
    }

    // Native specialized closure #2 in static Dictionary._forceBridgeFromObjectiveC(_:result:) for
    // [String: String], 0x1c0dec17c. The objects and keys are copied into the front halves of the uninitialized
    // buffers with -_getObjects:andKeys:count:, then each is cast to String (an unconditional dynamic cast), from
    // the last to the first so that no unread pointer is overwritten.
    private static func _forceBridgeElements(
        keys: UnsafeMutableBufferPointer<String>,
        values: UnsafeMutableBufferPointer<String>,
        from dictionary: any DictionaryBridgingMessages,
        count: Int
    ) -> Int {
        let keyBase = keys.baseAddress!
        let valueBase = values.baseAddress!
        let keyObjects = UnsafeMutableRawPointer(keyBase)
        let valueObjects = UnsafeMutableRawPointer(valueBase)
        dictionary._getObjects(
            AutoreleasingUnsafeMutablePointer(valueObjects.assumingMemoryBound(to: AnyObject?.self)),
            andKeys: AutoreleasingUnsafeMutablePointer(keyObjects.assumingMemoryBound(to: AnyObject?.self)),
            count: UInt(bitPattern: count)
        )
        for index in (0..<count).reversed() {
            let object = keyObjects.load(
                fromByteOffset: index * MemoryLayout<AnyObject>.stride, as: AnyObject.self)
            (keyBase + index).initialize(to: object as! String)
        }
        for index in (0..<count).reversed() {
            let object = valueObjects.load(
                fromByteOffset: index * MemoryLayout<AnyObject>.stride,
                as: AnyObject.self
            )
            (valueBase + index).initialize(to: object as! String)
        }
        return count
    }
}

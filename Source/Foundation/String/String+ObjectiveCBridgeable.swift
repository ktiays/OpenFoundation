//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified (assembly read in full). iOS Device divergence: iOS Device 24A446 remains unread.
//
// String's _ObjectiveCBridgeable conformance, which makes String representable in Objective-C as NSString
// (the @objc thunks bridge with _bridgeToObjectiveCImpl) and lets the runtime cast NSString to String
// (swift_dynamicCast calls the force and conditional witnesses). The two from-Objective-C witnesses are reached
// only through the conformance (an indirect edge the scope closure does not follow); they were resolved by
// reading. Each inlines String._unconditionallyBridgeFromObjectiveC (Decimal/Decimal+StringBridge.swift,
// 0x1c0b0b6c0), whose compiler-generated closure pieces they share.

// The conformance is retroactive only in the prefixed build (module OpenFoundation); native Foundation owns it.
// swift-format-ignore: AvoidRetroactiveConformances
extension String: @retroactive _ObjectiveCBridgeable {
    public typealias _ObjectiveCType = ONSString

    // Native String._bridgeToObjectiveC(), 0x1c0b0b6bc: a tail call to _bridgeToObjectiveCImpl.
    public func _bridgeToObjectiveC() -> ONSString {
        return unsafeDowncast(_bridgeToObjectiveCImpl(), to: ONSString.self)
    }

    // Native static String._forceBridgeFromObjectiveC(_:result:), 0x1c0b0bfc8.
    public static func _forceBridgeFromObjectiveC(_ source: ONSString, result: inout String?) {
        result = _unconditionallyBridgeFromObjectiveC(source)
    }

    // Native static String._conditionallyBridgeFromObjectiveC(_:result:), 0x1c0b0c43c. Always succeeds.
    public static func _conditionallyBridgeFromObjectiveC(_ source: ONSString, result: inout String?) -> Bool
    {
        result = _unconditionallyBridgeFromObjectiveC(source)
        return result != nil
    }

    // Native static String._unconditionallyBridgeFromObjectiveC(_:), 0x1c0b0b6c0 (restored on the raw object
    // pointer in Decimal/Decimal+StringBridge.swift; this witness passes the same pointer).
    public static func _unconditionallyBridgeFromObjectiveC(_ source: ONSString?) -> String {
        return _unconditionallyBridgeFromObjectiveC(
            source.map { UnsafeRawPointer(Unmanaged.passUnretained($0).toOpaque()) })
    }
}

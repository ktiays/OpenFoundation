//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

import Synchronization
import notify

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified (assembly read in full; field names and types from the reflection metadata, field
// descriptors 0x1c14778a4, 0x1c14778cc, 0x1c14778f4, 0x1c147b198 and 0x1c147b208). iOS Device divergence: iOS
// Device 24A446 remains unread.
//
// Inferred organization: _ProcessInfo (0x1c0a3a890-0x1c0a3b0f0), _NSSwiftProcessInfo (0x1c0a3b81c-0x1c0a3bb54)
// and its hardware-state statics (0x1c0ce6a48-0x1c0ce7104). Compiler-generated pieces are cited at their source
// cause and never written: the type metadata accessors (_ProcessInfo 0x1c0a3aaf0, _NSSwiftProcessInfo
// 0x1c0a3ba00), the ivar destroyer 0x1c0a3b964 with the outlined destroy of State 0x1c0a3b9cc, the State enum
// tag witnesses 0x1c0ce6d4c and 0x1c0ce6d94, and the @objc thunks (each cited at its method).

// Native _ProcessInfo (Swift class, metadata accessor 0x1c0a3aaf0; instance size 0x48): the process state
// shared by the Swift process info, its lazily computed name and arguments behind one mutex, and the host
// name behind another.
final class _ProcessInfo: Sendable {
    // Native _ProcessInfo.State: processName and arguments, nil until first computed.
    struct State {
        var processName: String?
        var arguments: [String]?
    }

    // Native static _ProcessInfo.processInfo, one-time initializer 0x1c0a3a890 (swift_allocObject of 0x48
    // bytes, both mutexes and their values zeroed).
    static let processInfo = _ProcessInfo()

    let state = Mutex(State(processName: nil, arguments: nil))
    let _hostName = Mutex<String?>(nil)

    // Native closure #1 in _ProcessInfo.arguments.getter, 0x1c0a3a8cc (run under the state mutex; the
    // _ArrayBuffer consume-and-create specialization is compiler-generated). CommandLine.arguments, with the
    // first element replaced by the standardized executable path (or the path alone for no arguments).
    var arguments: [String] {
        state.withLock { state in
            if let arguments = state.arguments {
                return arguments
            }
            var arguments = CommandLine.arguments
            if let path = _OCFProcessPath() {
                let standardized = String(cString: path).standardizingPath
                if arguments.isEmpty {
                    arguments = [standardized]
                } else {
                    arguments[0] = standardized
                }
            }
            state.arguments = arguments
            return arguments
        }
    }

    // Native _ProcessInfo.processName.getter, 0x1c0a3a9e8 (the mutex closure inlined). The last path component
    // of the standardized executable path, or the empty string without one.
    var processName: String {
        state.withLock { state in
            if let processName = state.processName {
                return processName
            }
            let processName: String
            if let path = _OCFProcessPath() {
                processName = String(cString: path).standardizingPath._lastPathComponent
            } else {
                processName = ""
            }
            state.processName = processName
            return processName
        }
    }

    // Native specialized _ProcessInfo.environment.getter, 0x1c0a3b0f0 (self is unused). The environ entries are
    // duplicated under the environ lock, then parsed: an entry without "=" is skipped, a later name replaces an
    // earlier one; the copies are freed. The Array and Dictionary specializations it calls are
    // compiler-generated.
    var environment: [String: String] {
        _platform_shims_lock_environ()
        guard let environ = _platform_shims_get_environ() else {
            _platform_shims_unlock_environ()
            return [:]
        }
        var copies: [UnsafeMutablePointer<CChar>] = []
        var cursor = environ
        while let entry = cursor.pointee {
            copies.append(strdup(entry)!)
            cursor += 1
        }
        _platform_shims_unlock_environ()
        var result: [String: String] = [:]
        for copy in copies {
            let entry = String(cString: copy)
            guard let separator = entry.firstIndex(of: "=") else {
                continue
            }
            let name = String(entry[..<separator])
            let value = String(entry[entry.index(after: separator)...])
            result[name] = value
        }
        for copy in copies {
            free(copy)
        }
        return result
    }
}

// Native _NSSwiftProcessInfo (__DATA,__objc_data 0x22e6e2b70, was a private external): the Swift subclass of
// NSProcessInfo that +[NSProcessInfo allocWithZone:] answers when the NSSwiftProcessInfo feature is enabled.
// Ivars _state (Mutex<State>) and _processInfo (_ProcessInfo). Only the members the captured groups reach are
// restored.
@objc(_ONSSwiftProcessInfo)
final class _ONSSwiftProcessInfo: ONSProcessInfo, @unchecked Sendable {
    // Native _NSSwiftProcessInfo.State: the process-killing and termination bookkeeping (none of its readers is
    // in the scope), the thermal state and the power state.
    struct State {
        var processKillingDisablingCount: Int
        var processKillingWasTurnedOn: Bool
        var automaticTerminationOptOutCounter: Int
        var automaticTerminationEnabled: Bool
        var relaunchableChangedHandler: (() -> Void)?
        var thermalState: ONSProcessInfoThermalState
        // The element is a class-bound protocol existential (the protocol is an indirect symbol, unresolved
        // here); AnyObject keeps its one-word layout.
        var suddenTerminationTransactions: [AnyObject]
        var powerState: PowerState
    }

    // Native _NSSwiftProcessInfo.PowerState.
    enum PowerState {
        case normal
        case restricted
    }

    // Native _NSSwiftProcessInfo.GlobalState (no field descriptor; the member names are local descriptive
    // names): the cached thermal state and the low power mode state, each with its notify token and whether the
    // native notification was cancelled.
    struct GlobalState {
        var thermalState: ONSProcessInfoThermalState?
        var thermalToken: Int32
        var thermalNotificationCancelled: Bool
        var powerState: Bool?
        var powerToken: Int32
        var powerNotificationCancelled: Bool
    }

    // Native static _NSSwiftProcessInfo._globalState, 0x22e6ed840 (__DATA,__data, statically initialized: no
    // thermal or power state, both notifications cancelled).
    static let _globalState = Mutex(
        GlobalState(
            thermalState: nil,
            thermalToken: 0,
            thermalNotificationCancelled: true,
            powerState: nil,
            powerToken: 0,
            powerNotificationCancelled: true
        )
    )

    // Native static _NSSwiftProcessInfo._shared, one-time initializer 0x1c0a3b81c (+allocWithZone: and -init).
    static let _shared: _ONSSwiftProcessInfo = _ONSSwiftProcessInfo()

    let _state: Mutex<State>
    let _processInfo: _ProcessInfo

    // Native static _NSSwiftProcessInfo.processInfo.getter, @objc thunk 0x1c0a3b840.
    @objc override static func processInfo() -> Any {
        return _shared
    }

    // Native _NSSwiftProcessInfo.init(), 0x1c0a3b880 (@objc thunk 0x1c0a3b944): the state starts with a
    // process-killing count of 1, nominal thermal state and normal power state.
    override init() {
        _state = Mutex(
            State(
                processKillingDisablingCount: 1,
                processKillingWasTurnedOn: false,
                automaticTerminationOptOutCounter: 0,
                automaticTerminationEnabled: false,
                relaunchableChangedHandler: nil,
                thermalState: .nominal,
                suddenTerminationTransactions: [],
                powerState: .normal
            )
        )
        _processInfo = _ProcessInfo.processInfo
        super.init()
    }

    // Native _NSSwiftProcessInfo.arguments.getter, @objc thunk 0x1c0a3ba20 (the array bridged with
    // _asCocoaArray).
    @objc override func arguments() -> Any {
        return _processInfo.arguments._bridgeToObjectiveCImpl()
    }

    // Native _NSSwiftProcessInfo.environment.getter, @objc thunk 0x1c0a3ba90 (the dictionary bridged with
    // _NativeDictionary.bridged).
    @objc func environment() -> Any {
        return _processInfo.environment._bridgeToObjectiveCImpl()
    }

    // Native _NSSwiftProcessInfo.processName.getter, @objc thunk 0x1c0a3baf4.
    @objc override func processName() -> Any {
        return _processInfo.processName._bridgeToObjectiveCImpl()
    }

    // Native static _NSSwiftProcessInfo.cancelThermalStateNotifyNotification(), @objc thunk 0x1c0ce6a48, merged
    // body 0x1c0ce6b5c (shared with the power variant): cancels the native thermal notification once.
    @objc static func cancelThermalStateNotifyNotification() {
        _globalState.withLock { state in
            if !state.thermalNotificationCancelled {
                notify_cancel(state.thermalToken)
                state.thermalToken = 0
                state.thermalNotificationCancelled = true
            }
        }
    }

    // Native specialized static _NSSwiftProcessInfo.updateThermalState(_:), 0x1c0ce6f6c (@objc thunk
    // 0x1c0ce6a58). The notify state (which must fit 32 bits) mapped to the thermal state: 10 fair, 20 serious,
    // 30, 40 or 50 critical, otherwise nominal.
    @objc static func updateThermalState(_ state: UInt64) {
        _globalState.withLock { globalState in
            let thermalState: ONSProcessInfoThermalState
            switch UInt32(state) {
            case 10:
                thermalState = .fair
            case 20:
                thermalState = .serious
            case 30, 40, 50:
                thermalState = .critical
            default:
                thermalState = .nominal
            }
            globalState.thermalState = thermalState
        }
    }

    // Native _NSSwiftProcessInfo.isLowPowerModeEnabled.getter, @objc thunk 0x1c0ce6a60, with its mutex closure
    // 0x1c0ce6ad0. The cached power state; without one, the notification is registered and the state read (a
    // failure answers false and caches nothing).
    @objc override func isLowPowerModeEnabled() -> Bool {
        // Native specialized setupPowerStateNotificationAndFetchState #1, 0x1c0ce6ff8 (the block thunk
        // 0x1c0ce7104 and its descriptor are compiler-generated). The registration's own token is stored only by
        // the handler (native).
        @Sendable func setupPowerStateNotificationAndFetchState() -> UInt64? {
            var token: Int32 = 0
            let queue = __NSDispatchQueueGetGenericMatchingMain()
            let status = notify_register_dispatch("com.apple.system.lowpowermode", &token, queue) { changed in
                // Native specialized closure #1 in setupPowerStateNotificationAndFetchState, 0x1c0ce6df4.
                var value: UInt64 = 0
                guard notify_get_state(changed, &value) == 0 else {
                    return
                }
                let enabled = value != 0
                _ONSSwiftProcessInfo._globalState.withLock { state in
                    state.powerToken = changed
                    state.powerNotificationCancelled = false
                    state.powerState = enabled
                }
            }
            guard status == 0 else {
                return nil
            }
            var value: UInt64 = 0
            guard notify_get_state(token, &value) == 0 else {
                return nil
            }
            return value
        }

        return Self._globalState.withLock { state in
            if let powerState = state.powerState {
                return powerState
            }
            guard let value = setupPowerStateNotificationAndFetchState() else {
                return false
            }
            let enabled = value != 0
            state.powerState = enabled
            return enabled
        }
    }

    // Native static _NSSwiftProcessInfo.cancelPowerStateNotifyNotification(), @objc thunk 0x1c0ce6b4c (the merged
    // body 0x1c0ce6b5c).
    @objc static func cancelPowerStateNotifyNotification() {
        _globalState.withLock { state in
            if !state.powerNotificationCancelled {
                notify_cancel(state.powerToken)
                state.powerToken = 0
                state.powerNotificationCancelled = true
            }
        }
    }

    // Native static _NSSwiftProcessInfo.updatePowerState(_:), @objc thunk 0x1c0ce6bb0.
    @objc static func updatePowerState(_ state: UInt64) {
        _globalState.withLock { globalState in
            globalState.powerState = state != 0
        }
    }
}

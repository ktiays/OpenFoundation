//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

import Darwin

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FX-swift-and-functions.

// Native _getSVUID(), 0x1c0c433c0. Private external.
// The native code supplies an initial oldlen of zero even with a nonnull buffer (kept).
private func _getSVUID() -> uid_t? {
    var process = kinfo_proc()
    var size = 0
    var mib = [CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()]
    guard sysctl(&mib, 4, &process, &size, nil, 0) == 0 else {
        return nil
    }
    return process.kp_eproc.e_pcred.p_svuid
}

// Native one-time initializer for _canChangeUIDs, 0x1c0c434b4. Private external.
private let _canChangeUIDs: Bool = {
    let effective = geteuid()
    let real = getuid()
    let saved = _getSVUID()
    return saved != effective || real != effective || real == 0
}()

// Native one-time initializer for _cachedUGIDs, 0x1c0c43514. Private external.
private let _cachedUGIDs: (uid: uid_t, gid: gid_t) = {
    var uid: uid_t = 0
    var gid: gid_t = 0
    if pthread_getugid_np(&uid, &gid) != 0 {
        uid = geteuid()
        gid = getegid()
    }
    return (uid, gid)
}()

enum Platform {
    // Native withUserGroupBuffer specializations for getpwnam_r and getpwuid_r, 0x1c0c43590 and
    // 0x1c0c43744. Private externals. Only the captured passwd call paths are used below.
    private static func withUserGroupBuffer<Key, Record, Value>(
        _ key: Key,
        _ record: Record,
        sizeProperty: Int32,
        operation: (
            Key, UnsafeMutablePointer<Record>, UnsafeMutablePointer<CChar>?, Int,
            UnsafeMutablePointer<UnsafeMutablePointer<Record>?>
        ) -> Int32,
        block: (Record) -> Value?
    ) -> Value? {
        let size = sysconf(sizeProperty)
        precondition(size >= -1)
        let capacity = size == -1 ? 4096 : size

        func read(_ buffer: UnsafeMutablePointer<CChar>?) -> Value? {
            var record = record
            var result: UnsafeMutablePointer<Record>?
            guard operation(key, &record, buffer, capacity, &result) == 0, result != nil else {
                return nil
            }
            return block(record)
        }

        if capacity <= 1024 {
            return withUnsafeTemporaryAllocation(of: CChar.self, capacity: max(capacity, 1)) {
                read($0.baseAddress)
            }
        }
        let buffer = UnsafeMutablePointer<CChar>.allocate(capacity: capacity)
        defer { buffer.deallocate() }
        return read(buffer)
    }

    // Native Platform.homeDirectory(forUserName:), inlined at 0x1c0b60650.
    static func homeDirectory(forUserName name: String) -> String? {
        let record = passwd()
        return name.utf8CString.withUnsafeBufferPointer { nameBytes in
            withUserGroupBuffer(
                nameBytes.baseAddress!, record, sizeProperty: _SC_GETPW_R_SIZE_MAX, operation: getpwnam_r
            ) { record in
                record.pw_dir.map { String(cString: $0) }
            }
        }
    }

    // Native Platform.homeDirectory(forUID:), inlined in String.homeDirectoryPath() at 0x1c0b5ef8c.
    static func homeDirectory(forUID uid: uid_t) -> String? {
        withUserGroupBuffer(uid, passwd(), sizeProperty: _SC_GETPW_R_SIZE_MAX, operation: getpwuid_r) {
            $0.pw_dir.map { String(cString: $0) }
        }
    }

    // Native Platform.getUGIDs(allowEffectiveRootUID:), 0x1c0c43f58. Private external specialization.
    static func getUGIDs(allowEffectiveRootUID: Bool) -> (uid: uid_t, gid: gid_t) {
        var ids: (uid: uid_t, gid: gid_t)
        if _canChangeUIDs {
            ids = (0, 0)
            if pthread_getugid_np(&ids.uid, &ids.gid) != 0 {
                ids.uid = geteuid()
                ids.gid = getegid()
            }
        } else {
            ids = _cachedUGIDs
        }
        if !allowEffectiveRootUID && ids.uid == 0 {
            ids.uid = getuid()
        }
        return ids
    }
}

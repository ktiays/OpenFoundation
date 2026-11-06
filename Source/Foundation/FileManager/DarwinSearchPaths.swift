//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

import Darwin

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device 24A446: comparison pending.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FB-bundle-paths-plist.

@_silgen_name("objc_release")
private func releaseSearchPathFileSystemString(_ string: UnsafeRawPointer?)

@_silgen_name("objc_autoreleaseReturnValue")
private func autoreleaseSearchPathArray(_ array: UnsafeRawPointer) -> UnsafeRawPointer

// The iterator operations are inlined into the native _copySequenceToContiguousArray specialization.
private struct DarwinSearchPathSequence: Sequence, IteratorProtocol {
    var state: UInt32?

    mutating func next() -> String? {
        guard let current = state else {
            return nil
        }
        let path = UnsafeMutablePointer<CChar>.allocate(capacity: 0x402)
        defer {
            path.deallocate()
        }
        let next = sysdir_get_next_search_path_enumeration(current, path)
        if next == 0 {
            state = nil
            return nil
        }
        state = next
        let object = copySearchPathFileSystemString(path)
        defer {
            releaseSearchPathFileSystemString(object)
        }
        return String._unconditionallyBridgeFromObjectiveC(object)
    }
}

// Native _NSSearchPathsForDirectoryInDomain, 0x1c08d5f38. Was a private external.
@_silgen_name("_NSSearchPathsForDirectoryInDomain")
func searchPathsForDirectoryInDomain(_ directory: UInt, _ domains: UInt, _ expandTilde: Bool)
    -> UnsafeRawPointer
{
    let sequence = DarwinSearchPathSequence(
        state: sysdir_start_search_path_enumeration_private(
            UInt32(truncatingIfNeeded: directory), UInt32(domains & 0xc0f))
    )
    let paths = Array(
        sequence.lazy.map { path in
            // Native closure #1 in _DarwinSearchPaths(for:in:expandTilde:), 0x1c08d5fbc. Non-external.
            // Compiler-generated partial applications 0x1c08d61cc and 0x1c08da688 and the
            // _copySequenceToContiguousArray specialization 0x1c08d9e1c originate at this lazy map.
            guard expandTilde, path.utf8.first == 0x7e else {
                return path
            }

            let slash = path.utf8.firstIndex(of: 0x2f) ?? path.endIndex
            let userStart = path.utf8.index(after: path.utf8.startIndex)
            var home: String
            if userStart == slash {
                home = String.homeDirectoryPath()
            } else {
                guard let userHome = String.homeDirectoryPath(forUser: String(path[userStart..<slash])) else {
                    return path
                }
                home = userHome
            }
            home.append(contentsOf: path[slash...])
            return home
        })
    return autoreleaseSearchPathArray(Unmanaged.passRetained(paths._bridgeToObjectiveCImpl()).toOpaque())
}

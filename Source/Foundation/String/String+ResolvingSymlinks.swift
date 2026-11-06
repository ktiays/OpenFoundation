//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

import Darwin

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FX-swift-and-functions.

extension String {
    // Native String._resolvingSymlinksInPath(), 0x1c0b64710. Private external.
    func _resolvingSymlinksInPath() -> String? {
        guard !isEmpty else {
            return nil
        }
        let capacity = utf8.count * 3 + 1

        // Native withFileSystemRepresentation closure specialization, 0x1c0b139b8.
        func withFileSystemPath(_ storage: UnsafeMutableBufferPointer<UInt8>) -> String? {
            guard _decomposed(1, into: storage, nullTerminated: true) != nil else {
                return nil
            }
            return storage.withMemoryRebound(to: CChar.self) { buffer in
                resolve(buffer.baseAddress)
            }
        }

        // Native String._resolvingSymlinksInPath closure #1, 0x1c0b64c58. Private external.
        func resolve(_ source: UnsafePointer<CChar>?) -> String? {
            guard let source else {
                return nil
            }
            var path = self
            if urlIsAbsolute(standardizing: &path, pathStyle: 0),
                let fullPath = Self._resolvingSymlinksInPathUsingFullPathAttribute(source)
            {
                return fullPath
            }

            let pathBuffer = UnsafeMutablePointer<CChar>.allocate(capacity: 1026)
            defer { pathBuffer.deallocate() }
            pathBuffer.initialize(repeating: 0, count: 1026)
            let length = strlcpy(pathBuffer, source, 1026) + 1
            let linkBuffer = UnsafeMutablePointer<CChar>.allocate(capacity: 1026)
            defer { linkBuffer.deallocate() }
            return resolveComponents(pathBuffer, length: length, linkBuffer: linkBuffer)
        }

        // Native inner readlink closure, 0x1c0b64d8c. Private external.
        func resolveComponents(
            _ path: UnsafeMutablePointer<CChar>,
            length initialLength: Int,
            linkBuffer: UnsafeMutablePointer<CChar>
        ) -> String? {
            linkBuffer.initialize(repeating: 0, count: 1026)
            var length = initialLength
            var cursor = 0
            var links = 0
            while true {
                var end = cursor
                while path[end] == 0x2f {
                    end += 1
                }
                while path[end] != 0 && path[end] != 0x2f {
                    end += 1
                }
                let separator = path[end]
                path[end] = 0
                var info = stat()
                guard lstat(path, &info) >= 0 else {
                    return nil
                }
                if info.st_mode & S_IFMT == S_IFLNK {
                    guard links < 33 else {
                        return nil
                    }
                    let count = readlink(path, linkBuffer, 1025)
                    guard count >= 0 else {
                        return nil
                    }
                    linkBuffer[count] = 0
                    path[end] = separator
                    if count != 0 {
                        let absolute = linkBuffer[0] == 0x2f
                        let target = absolute ? 0 : cursor + 1
                        let tail = length - end
                        let destination = target + count
                        guard destination + tail < 1027 else {
                            return nil
                        }
                        precondition(tail >= 0 && destination <= 1026 && tail <= 1026 - destination)
                        if destination != end || destination >= length {
                            // Foundation defect: 0x1c0b65000 can read beyond the original 1,026-byte
                            // buffer after strlcpy truncates and a shorter symlink target fits.
                            // Confidence: verified. Return the native resolution failure for that read.
                            guard tail <= 1026 - end else {
                                return nil
                            }
                            memmove(path + destination, path + end, tail)
                        }
                        memmove(path + target, linkBuffer, count)
                        length = destination + tail
                        links += 1
                        if absolute {
                            cursor = 0
                        }
                        continue
                    }
                } else {
                    path[end] = separator
                }
                if separator == 0 {
                    return String(cString: path)
                }
                cursor = end
            }
        }

        if capacity <= 1024 {
            return withUnsafeTemporaryAllocation(of: UInt8.self, capacity: capacity, withFileSystemPath)
        }
        let storage = UnsafeMutableBufferPointer<UInt8>.allocate(capacity: capacity)
        defer { storage.deallocate() }
        return withFileSystemPath(storage)
    }

    // Native String._resolvingSymlinksInPathUsingFullPathAttribute(_:), 0x1c0b64a74. Private external.
    // The native response has a 12-byte header followed by 1,024 bytes of path storage.
    private static func _resolvingSymlinksInPathUsingFullPathAttribute(_ path: UnsafePointer<CChar>)
        -> String?
    {
        var attributes = attrlist()
        attributes.bitmapcount = UInt16(ATTR_BIT_MAP_COUNT)
        attributes.commonattr = UInt32(ATTR_CMN_FULLPATH)
        return withUnsafeTemporaryAllocation(byteCount: 1036, alignment: 4) { bytes in
            let address = bytes.baseAddress!
            bzero(address, 1036)
            guard getattrlist(path, &attributes, address, 1036, 0) == 0,
                address.load(fromByteOffset: 8, as: UInt32.self) != 0
            else {
                return nil
            }
            return String(cString: (address + 12).assumingMemoryBound(to: CChar.self))
        }
    }
}

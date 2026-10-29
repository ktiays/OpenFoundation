# Reference images

`Configuration/ReferenceImages.json` records the authoritative identities.
The bootstrap uses iOS Simulator 27.0 (24A434), arm64, and iOS Device 27.0.1
(24A446), arm64e, aligned with the consumers' reference OS versions.

Simulator pins come from LC_UUID on the system Foundation/CoreFoundation
loaded inside a fresh simulator. Normal tests verify them and never update
them. This is runtime identity evidence, not a binary implementation reading.
Both simulator images are now extracted from that same runtime cache and recorded
with SHA-256 and byte length. Closed canonical IDA databases exist for both image sets.

The iOS Device copies come from Xcode's local iPhone18,1 27.0.1 (24A446)
DeviceSupport extraction. The manifest records the source path, copied file's
SHA-256, byte length, architecture, platform and UUID. Copies live in
`Local/Databases/iphone-device`, outside version control. Neither the copy
nor a recorded UUID claims an on-iPhone behavioral test.

Use `Tools/reference-images.py record` on verified cache extractions to add
canonical binaries and hashes. The tool refuses a different UUID for an
already pinned image and refuses a canonical copy whose hash differs.
Never open a canonical IDA database directly: clone it under
`Local/Scratch/<investigation>`, then use IDA MCP and close the clone afterward.

The [native inventory](NativeInventory.md) records the four databases and interface
boundaries. The scalar readings record the implemented functions and independent
iOS Device comparisons; the remaining framework bodies and object ABI are incomplete.

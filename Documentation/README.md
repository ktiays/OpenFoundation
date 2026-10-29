# Documentation

| Document | Purpose |
| --- | --- |
| [Method](Method.md) | Binary readings, source conventions, builds and comparison policy |
| [Roadmap](Roadmap.md) | Dependency-first restoration sequence and consumer acceptance gates |
| [Tracks](Tracks.md) | Work scopes, owned paths, dependencies and state |
| [Foundation reference](Reference/FoundationReference.md) | Current evidence and unread implementation boundaries |
| [Reference images](Reference/ReferenceImages.md) | iOS Simulator/iOS Device identity and provenance |
| [Native symbol inventory](Reference/NativeInventory.md) | Four-image IDA inventory and remaining ownership boundaries |
| [CoreFoundation base](Reference/Research/CoreFoundationBase.md) | Range construction, byte hashing and exact length/arithmetic behavior |
| [Encoding kernels](Reference/Research/CoreFoundationStringEncoding.md) | Complete UTF-8 encoder and six single-byte codec pairs, hidden-source tests |
| [Date scalar validation](Reference/Research/CoreFoundationScalar.md) | Gregorian flags, NaNs and floating state |
| [Foundation ranges](Reference/Research/FoundationRange.md) | Unsigned union/intersection and wraparound |
| [Runtime prerequisites](Reference/Research/RuntimePrerequisites.md) | Captured entry points and the unread object-runtime dependency group |
| [Consumer requirements](Reference/ConsumerRequirements.md) | Current OpenCoreGraphics/OpenCoreAnimation dependency surface |
| [Consumer snapshot](Reference/Consumers.json) | Reproducible lexical source inventory at the recorded revisions |
| [Reading template](Reference/Research/ReadingTemplate.md) | Required identity, confidence, dependency and validation fields |

The identity manifest is `Configuration/ReferenceImages.json`; the machine-readable
definition ledger is `Configuration/Restoration.json`. The ledger accounts for exact production definitions and their validated scope.
Raw binaries, IDA databases, disassembly and probe output belong in unversioned
`Local/`, not in this documentation tree.

# Documentation

| Document | Purpose |
| --- | --- |
| [Method](Method.md) | Binary readings, source conventions, builds and comparison policy |
| [Roadmap](Roadmap.md) | Dependency-first restoration sequence and consumer acceptance gates |
| [Tracks](Tracks.md) | Work scopes, owned paths, dependencies and state |
| [Restoration scope](Reference/Scope.md) | Consumer-derived roots, native closure, dependency groups and cuts |
| [Foundation reference](Reference/FoundationReference.md) | Current evidence and unread implementation boundaries |
| [Reference images](Reference/ReferenceImages.md) | iOS Simulator/iOS Device identity and provenance |
| [Native symbol inventory](Reference/NativeInventory.md) | Four-image IDA inventory and remaining ownership boundaries |
| [CoreFoundation base](Reference/Research/CoreFoundationBase.md) | Range construction, byte hashing and exact length/arithmetic behavior |
| [Raw Decimal arithmetic](Reference/Research/FoundationDecimal.md) | Native C/Swift arithmetic boundaries, exact rounding/errors and established defect fixes |
| [Unicode primitives](Reference/Research/CoreFoundationUnicode.md) | Bitmap membership, raw decomposition and destination-buffer behavior |
| [BigNum initialization and text](Reference/Research/CoreFoundationBigNum.md) | Signed BigNum initializers, C-string formatting, `emit128` and its power tables |
| [Locale-identifier helpers](Reference/Research/CoreFoundationLocaleIdentifier.md) | Locale-identifier canonicalization helpers, comparators and mapping tables |
| [Encoding kernels](Reference/Research/CoreFoundationStringEncoding.md) | Complete UTF-8 encoder and six single-byte codec pairs, hidden-source tests |
| [Date scalar validation](Reference/Research/CoreFoundationScalar.md) | Gregorian flags, NaNs and floating state |
| [Foundation ranges](Reference/Research/FoundationRange.md) | Unsigned union/intersection and wraparound |
| [Runtime prerequisites](Reference/Research/RuntimePrerequisites.md) | Captured entry points and the unread object-runtime dependency group |
| [Consumer requirements](Reference/ConsumerRequirements.md) | Lexical OpenCoreGraphics/OpenCoreAnimation inventory and acceptance rules |
| [Consumer snapshot](Reference/Consumers.json) | Reproducible lexical source inventory at the recorded revisions |
| [Reading template](Reference/Research/ReadingTemplate.md) | Required identity, confidence, dependency and validation fields |

The identity manifest is `Configuration/ReferenceImages.json`; the machine-readable
definition ledger is `Configuration/Restoration.json`; the restoration scope is
`Configuration/Scope.json`. The ledger accounts for exact production definitions and their validated scope.
Raw binaries, IDA databases, disassembly and probe output belong in unversioned
`Local/`, not in this documentation tree.

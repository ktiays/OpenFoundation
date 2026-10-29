# Documentation

| Document | Purpose |
| --- | --- |
| [Method](Method.md) | Binary readings, source conventions, builds and comparison policy |
| [Roadmap](Roadmap.md) | Dependency-first restoration sequence and consumer acceptance gates |
| [Tracks](Tracks.md) | Work scopes, owned paths, dependencies and state |
| [Foundation reference](Reference/FoundationReference.md) | Current evidence and unread implementation boundaries |
| [Reference images](Reference/ReferenceImages.md) | iOS Simulator/iOS Device identity and provenance |
| [Consumer requirements](Reference/ConsumerRequirements.md) | Current OpenCoreGraphics/OpenCoreAnimation dependency surface |
| [Consumer snapshot](Reference/Consumers.json) | Reproducible lexical source inventory at the recorded revisions |
| [Reading template](Reference/Research/ReadingTemplate.md) | Required identity, confidence, dependency and validation fields |

The identity manifest is `Configuration/ReferenceImages.json`; the machine-readable
definition ledger is `Configuration/Restoration.json`. The ledger starts empty.
Raw binaries, IDA databases, disassembly and probe output belong in unversioned
`Local/`, not in this documentation tree.

# OpenFoundation

A project to restore, one to one from their pinned binaries, the part of the iOS
Foundation and CoreFoundation implementations that OpenCoreAnimation and
OpenCoreGraphics require, and to run it on other platforms toward a
cross-platform OpenCoreAnimation. The scope is derived from the consumers'
linked binaries and closed over the native call graphs
([scope reading](Documentation/Reference/Scope.md)); the rest of the framework
surface is not a goal. Inside the scope, native implementation boundaries and
observable behavior define the restoration.

**Status:** the first scalar groups restore **one hundred seven function definitions, one hundred twelve data definitions and two Swift support types**:
CoreFoundation range construction, byte hashing and Gregorian field validation,
and Foundation range union/intersection, plus fourteen hidden UTF-8 and single-byte
conversion kernels, the converter wrappers, encoding-name and code-page lookups and the
NSStringEncoding mapping. Twenty-nine C functions are dynamically exported; the hidden kernels
are verified through the same implementation sources in the internal test tools.
Unicode bitmap membership, raw character decomposition, destination formatting,
plane counts, bitmap copies, property-plane lookup and combining-priority sorting
are also restored, as are the signed BigNum initializers, BigNum C-string
formatting and the 128-bit decimal writer of CFNumber descriptions. The complete eleven-entry raw NSDecimal arithmetic group preserves
its native C-to-Swift boundary, with ten compiler-generated C bridges counted
separately in the ledger. Each has independent iOS Simulator and iOS Device readings;
twenty-three iOS Device descriptor/vector pointer bindings and seven Swift
value-witness bindings remain explicitly unread. Object runtime, strings, collections and consumer replacement
remain incomplete; these frameworks cannot replace either consumer's system dependency.

The first iOS Simulator scope holds 3,593 CoreFoundation and 3,375 Foundation
functions, most of them in over-approximating message tiers. Sixty-three restored
function definitions fall inside it.

Stage 1 matches iOS 27.0 Simulator (24A434), arm64. Stage 2 compares and restores
iOS Device 27.0.1 (24A446), using arm64e reference images. Port TARGETs follow
the porting rules in AGENTS.md; none is admitted yet. Other TARGET/MATCH pairs
are refused until their dependencies and behavior are established.

| Path | Purpose |
| --- | --- |
| [AGENTS.md](AGENTS.md) | English-only project content, evidence rules, naming, formatting and verification |
| [OpenFoundation.xcodeproj](OpenFoundation.xcodeproj) | Native synchronized-folder project and shared `OpenFoundation` scheme |
| `Source/CoreFoundation/` | `OpenCoreFoundation.framework` sources and headers |
| `Source/Foundation/` | `OpenFoundation.framework` sources and headers |
| `Tests/`, `StandaloneTests/` | Native-reference checks and tests without system Foundation/CoreFoundation |
| [Documentation](Documentation/README.md) | Method, reference identities, consumer inventory and restoration sequence |
| `Configuration/` | Shared build defaults, admitted platforms, image pins, restoration ledger and scope |
| `Local/` (unversioned) | Reference images, IDA databases, evidence and research scratch |

## Building

Requirements: Apple Silicon Mac, Xcode with the iOS SDK, Python 3.9 or later,
clang-format, and the iOS 27.0 Simulator runtime (24A434) for execution. Both
build systems use C11/C++20, arm64, hidden visibility, Debug without LTO and
Release with full LTO. Foundation/CoreFoundation behavior is not delegated to
the system libraries.

```sh
# Direct compiler build of both frameworks.
make
make CONFIGURATION=Release

# Native Xcode build of the same sources, both frameworks and test tools.
make xcode
make CONFIGURATION=Release xcode

# Isolated/differential tests, packaging/dependency checks and infrastructure regressions.
make test validate coexistence

# Native names in an independent output directory; isolated execution only.
make OF_SYSTEM_NAMES=YES test validate

# Formatting and checked-in project consistency.
make format
make check

# Complete Make/Xcode, Debug/Release and naming-mode verification matrix.
make verify
```

The two products are deliberately separate. `OF_SYSTEM_NAMES=NO` (default)
keeps CF/NS names prefixed as OCF/ONS for coexistence with the system;
`OF_SYSTEM_NAMES=YES` generates concrete system spellings from source
declarations. Both products use the same switch. Installed headers record
the selected mode and reject conflicting consumers. No implementation naming
aliases are installed.

Make products live in
`.build/Make/<TARGET>-<MATCH>-<CONFIGURATION>-<OF_SYSTEM_NAMES>/`.
Xcode products live under `.build/Xcode/<CONFIGURATION>-<OF_SYSTEM_NAMES>/`.
Each test session creates and deletes its own simulator, checking the runtime
build and the loaded reference image UUIDs. To run Xcode-built test tools:

```sh
python3 Tools/build.py --products .build/Xcode/Debug-NO/Build/Products/Debug-iphonesimulator test
```

The reference and validation workflow is in [Documentation/Method.md](Documentation/Method.md).
The restoration order is in [Documentation/Roadmap.md](Documentation/Roadmap.md).
The consumer projects remain unchanged. Active subsystem scopes are recorded in the work ledger.

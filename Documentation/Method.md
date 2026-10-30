# Method

## Establishing a reference

The simulator reference is iOS 27.0 (24A434), arm64. The iOS Device reference is
iOS 27.0.1 (24A446), arm64e. These are the same OS baselines used by the consumer
projects. Their Foundation/CoreFoundation UUIDs are recorded independently in
`Configuration/ReferenceImages.json`; a QuartzCore or CoreGraphics UUID says
nothing about either of these images.

The bootstrap pins simulator UUIDs by opening the system images in a dedicated
simulator and reading their LC_UUID commands. This establishes loaded identity
only. The iOS Device images are APFS copies of Xcode's extracted symbols and
carry byte lengths, SHA-256 and Mach-O platform checks. Binary analysis remains
unread until an IDA reading is recorded.

```sh
# Recapture identities explicitly; normal tests never silently update a pin.
python3 Tools/build.py test-build
python3 Tools/reference-images.py capture-simulator \
    --products .build/Make/iphonesimulator-iphonesimulator-Debug-NO

# Record extracted images. The root contains Foundation.framework and
# CoreFoundation.framework. The tool verifies platform, architecture and UUID,
# clones the files into Local/Databases, and records their hashes.
python3 Tools/reference-images.py record --platform iphone-simulator \
    --framework-root /absolute/path/to/extracted/System/Library/Frameworks
```

Only record an extraction after verifying the source cache/runtime's build.
Mach-O platform numbers and UUIDs alone do not establish the OS build. Never
substitute the macOS framework or a newer runtime when an image is unavailable.
Missing simulator extraction/IDA databases block implementation readings, even
when runtime identity checks pass.

## Reading and implementing

1. Identify the operation's owning image and native dependency group. Read the
   simulator implementation through IDA on a scratch clone. Preserve function
   boundaries and recovered names; establish C, C++, Objective-C or Swift
   language and ARC/MRC policy from evidence.
2. Read registration, object layout, callbacks, failure paths and dependencies
   before their consumers. Compare the iOS Device image. Record addresses,
   confidence, unread boundaries and reproducible measurements using the
   [reading template](Reference/Research/ReadingTemplate.md).
3. Implement only the complete read group. Every native-equivalent definition
   gets a ledger row with `path`, `symbol`, `native_symbol`, `address`,
   `reference_uuid`, `confidence`, `reading`, `tests` and `ios_device`.
   Exported definitions also record `exported: true` and `product`.
   No implementation may import system Foundation/CoreFoundation.
4. Add standalone assertions and public differential cases. Hidden/internal
   probes are needed only where the public API cannot reach a path. Pin image
   ownership and calling convention before invoking a recovered address.
5. Validate both build systems, configurations and naming modes, then land the
   complete track with its reading and tests. Update the track ledger and remove
   obsolete local clones/builds. Keep evidence cited by a reading.

The progress check uses Clang ASTs to match each source function and data
definition, including hidden functions, header definitions and Objective-C
classes/methods, against the ledger. It also checks translation-unit accounting
and the emptiness of the two exempt Bootstrap.c files. Packaging matches every
dynamic export to the ledger. The infrastructure regressions reject missing and
extra rows; file-level accounting alone does not establish function coverage. Likewise, extend declaration parsing for any new
grammar before introducing its public names. The initialization does not claim
a complete Foundation parser or a native symbol inventory.

## Naming and packaging

The source uses OCF/ONS names. `Tools/prefix-mapping.py` discovers declarations
and generates `Generated/OFPrefixMapping.h` outside the source tree. The compiler
force-includes it; `OF_SYSTEM_NAMES=YES` turns `OCFRetain` into `CFRetain`, for
example. `OF_NAME_STRING(ONSObject)` follows the compiled spelling for runtime
lookups. Public headers are copied with token mapping, leaving comments and
literal data unchanged. The implementation mapping is never installed.
`Configuration/PublicHeaders.json` controls both installation and Xcode public
header visibility, so adding a private source header cannot expose it by accident.

OCF/ONS prefixing is a coexistence mechanism, not an ABI bridge. The runtime's
global class table, constant-string objects, tagged pointers, CF type
registration, and toll-free bridging must be restored before object exchange
can be accepted. Native-owned objects are never accidentally treated as restored
objects. Differential cases allocate equivalent fixtures independently.

Foundation/CoreFoundation separation does not assert class ownership. Classes
with NS names may belong to a different image or the Objective-C runtime;
the binary inventory must establish ownership before any class is defined.
Do not duplicate libobjc-owned classes or compiler support.

## Build verification

The native Xcode project uses filesystem-synchronized source folders and
separate target membership. The direct Make driver invokes clang/clang++ rather
than wrapping xcodebuild. Both use `Configuration/Build.xcconfig` for shared
defaults, the same naming/header generator, platform gate and restoration check.
The Xcode project is deterministically generated by `make project`; `make
project-check` rejects drift. A normal new implementation file needs no project
edit. Change target/header metadata in the generator when needed.

```sh
make check
make test validate coexistence
make CONFIGURATION=Release test validate coexistence
make OF_SYSTEM_NAMES=YES test validate
make CONFIGURATION=Release OF_SYSTEM_NAMES=YES test validate

make xcode
make CONFIGURATION=Release xcode
make OF_SYSTEM_NAMES=YES xcode
make CONFIGURATION=Release OF_SYSTEM_NAMES=YES xcode
```

`make test` links and executes the isolated test tool. In prefixed mode it also
runs the UUID-guarded native-reference tool. Replacement names are not executed
alongside native Foundation/CoreFoundation. Each session creates a fresh
simulator; cleanup touches only the UDID the session created.

`make validate` checks exported symbols, packaged headers/module maps, code
signatures and production load commands, and runs naming infrastructure
regressions. Internal behavioral cases start at zero. `make coexistence`
checks both header orders and both linker orders in C, C++, Objective-C and
Objective-C++. C/C++ clients use CoreFoundation headers; Objective-C/Objective-C++
clients additionally exercise Foundation headers, which require Objective-C syntax.
The current scalar suites independently resolve native and replacement symbols,
verify owning images and native UUIDs, and reject identical function addresses.
Internal kernels retain their native hidden names. `Configuration/InternalTestSources.json`
selects the same production source files for isolated/internal test-tool compilation
in both build systems. Native counterparts are resolved from pinned image bases
and IDA offsets, checked against executable segments and image ownership; the
replacement must belong to the test executable. No test-only production export
is added. Object interoperability remains unbuilt.

Use `python3 Tools/build.py --configuration Release --system-names YES
--products /absolute/path/to/Xcode/products test` to execute existing native
Xcode products without rebuilding them. The configuration/mode must match the
selected products. Xcode's main scheme builds command-line test tools, matching
OpenCoreGraphics's arrangement; `xcodebuild test` is not the runner for these
tools. Execute them with the simulator runner above.

## Comparison policy

`make verify` runs the complete matrix above, including execution of the native
Xcode test tools and header/link coexistence in prefixed mode. Its unversioned
logs are under `Local/Scratch/verification`. Its logs report actual per-suite cases and exported definition counts; finite
scalar tests do not establish framework equivalence.

Native-reference setup failures return 77, incompatibilities return 1 and
success returns 0. Missing pins, unexpected owners, wrong OS builds, absent
fixtures and unresolved functions are setup failures. A passing empty corpus
never increases a compatibility percentage.

Use exact bytes and floating-point bit patterns where the native operation is
deterministic. For lifetime and callback behavior, compare event order and
ownership on independently created fixtures. Exceptions, errors, mutation
during enumeration, allocator failures and multithreaded behavior are part of
each API's eventual acceptance. State any justified tolerance in the reading;
never change one merely to pass. Ownership changes also require static
analysis, memory guards/leak checks and ASan. Performance claims require
Release and an iOS Device.

## Trace comparison

Object-runtime groups are compared across processes (AGENTS.md, "Trace
comparison"), because per-thread keys, tagged-pointer classes and the
Objective-C class table are process global. `TraceTests` holds scenarios written
with native names; `TraceTests/Support/TraceInterface.h` selects the system
CoreFoundation for the reference build and the restored framework for the
replacement build.

```sh
make trace-record     # reference side, three fresh processes, must be identical
make trace            # reference side against TraceTests/Recorded/<match>/Traces.trace
make trace-compare    # replacement side (OF_SYSTEM_NAMES=YES products) against it
```

The reference side verifies the pinned CoreFoundation UUID and that no restored
framework is loaded; the replacement side verifies that OpenCoreFoundation is
loaded and no native Foundation/CoreFoundation image is. Lines starting with `#`
carry that identity and are not compared. Objects appear as first-seen indices
(`@n`), registered type IDs as `dynamic#n`; deallocation and allocation sizes
are observed through a logging allocator, callback order through logging
callbacks. A scenario never prints addresses, timing or another nondeterministic
value; one that needs it is recorded as unsupported. `trace-compare` is a setup
failure (77) while the restored frameworks do not export every CoreFoundation
symbol the reference side imports, and it never counts as a result until then.

The recorded trace is the expected trace of every port TARGET with the same MATCH.
The trace tool is built by `Tools/trace.py` only; it has no Xcode target yet.

## Native Swift units

The raw Decimal reading establishes the actual C wrappers, Swift arithmetic and
throwing helpers, value layouts, generated C bridges and ordinary libswiftCore /
weak libswift_Builtin_float dependencies. Swift is compiled only for units with
an entry in `Configuration/SwiftSources.json`; source declarations use concrete
ONS names and the same single naming switch selects both products.

`Tools/swift-build.py` is shared by Make and the native Xcode target's Swift
phase. It derives concrete selected-name Swift copies outside Source, maps only
exact `_cdecl` / `_silgen_name` linkage annotations, and preserves ordinary
literal data. Generated physical filenames carry an OF_ prefix so native virtual
`#sourceLocation` filenames remain unambiguous. Private textual bridge headers
have no module map and never import the system Foundation overlay. Swift helper
names live in OpenFoundation for coexistence and isolated Foundation in system
mode; no Swift public module is installed.

The source audit queries every primary file through the Swift compiler's JSON
AST. Explicit functions, accessors, constructors, support types and global
storage are matched by actual source USR. Generated C entry rows must point to an
authored function with the matching observed C binding; generated metadata and
witness support attach to verified type rows. An inlined native accessor records
its real containing instruction ranges rather than an invented entry address.
These records do not turn compiler support into additional handwritten bodies.

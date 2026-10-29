# Foundation range arithmetic

## Identity and scope

This reading restores only the two exported arithmetic functions below. The
owning image is Foundation, not CoreFoundation or the Objective-C runtime.
The `Range` source directory is inferred organization; no original source path
was recovered. No range string conversion, object category, inline SDK helper,
collection or object interoperability is implemented by this group.

| Reference | Identity | SHA-256 |
| --- | --- | --- |
| iOS Simulator | iOS 27.0 (24A434), arm64, UUID `49FCDA5F-A93E-3259-857C-FC75AC2C7B03` | `bc7e0a5f693ffc75ae7c3311b817716e86d149fda2f8c45669a976727322cb18` |
| iOS Device | iOS 27.0.1 (24A446), arm64e, UUID `C041F1D0-5EF1-30A3-BF96-830976173B56` | `c54c72935c506e8b3348e20aa758a8a97e95051506ffc2afb9b186726a369348` |

| Native function | iOS Simulator IDA address | iOS Device IDA address | Complete body | Confidence |
| --- | --- | --- | --- | --- |
| `NSUnionRange` | `0x1c05bd644` | `0x180f67520` | 32 bytes, 8 instructions | Verified |
| `NSIntersectionRange` | `0x1c05bd664` | `0x180e02318` | 72 bytes, 18 instructions | Verified |

The evidence consists of complete IDA disassembly, decompilation, function
boundaries, outgoing code references and body bytes, saved under
`Local/Evidence/ns-range/{iphone-simulator,iphone-device}-Foundation-range.json`.
`Local/Evidence/ns-range/ReadRange.py` is the identity-guarded IDAPython capture.
It checks each database input SHA-256 against the canonical binary and reads
LC_UUID before collecting evidence. The full interface inventories are at
`Local/Evidence/cf-runtime-reference/*-Foundation-inventory.json`.

Each raw image was APFS-cloned from `Local/Databases/<platform>/Foundation`
into `Local/Scratch/ns-range/<platform>/Foundation`, opened through IDA MCP,
and analyzed. After the database was closed, its `.i64` was APFS-copied to the
corresponding canonical database directory. Canonical databases were never
opened. The available MCP server returns a client `session_id`, not a
`database_id`; reads used the returned session identifier, an exclusive
coordinator-granted reading slot and an exact input-path/hash guard.

## Native ABI and compilation boundary

The native entry points accept two two-field aggregates in `x0`/`x1` and
`x2`/`x3` and return the aggregate in `x0`/`x1`. Location precedes length. The
arithmetic uses full 64-bit registers and unsigned `HI`, `CC` and `CS`
conditions. Both complete bodies are memory-free scalar leaves with no calls,
external branches, global reads, allocation, callbacks, thread state,
exceptions or retain/release operations. Confidence: verified at the function
addresses and image UUIDs above.

The SDK declaration supplies the interface names, an unsigned-long field type
and field order. It is not the source of the implementation rules. The emitted
register operations independently establish the 64-bit unsigned field behavior
and aggregate call boundary. The implementation asserts 8-byte field width,
8-byte alignment, a 16-byte aggregate and offsets 0/8. Original compilation
language was not recovered; the externally callable scalar boundary is
established, and C11 is this project's tooling choice for these leaves. This
reading infers neither framework-wide C nor an ARC/MRC ownership policy.

## Native implementation

All additions and subtractions wrap modulo 2^64. Keep that defined unsigned
behavior, including results for overflowing ranges and zero lengths. There are
no validity checks, saturation or exception paths.

`NSUnionRange` first adds each range's location and length, selects the greater
unsigned end, selects the smaller unsigned location, then subtracts that
location from the selected end. Its complete eight instructions establish the
operation and comparison order. Equal ends select the second end; equal
locations select the second location, with identical scalar values.

`NSIntersectionRange` first computes both ends and selects the smaller unsigned
end. It then follows two ordered cases:

1. If the second location is no greater than the first location and the first
   location is below the second end, return the first location and the selected
   end minus that location.
2. Otherwise, subtract the second location from the selected end. Return the
   second location and that difference only if the second location is below the
   first end and the first location is no greater than the second location.
   Return `{0, 0}` when those conditions fail.

These ordered tests matter for empty and overflowing ranges. For example,
intersecting `{5, 0}` with `{2, 10}` returns `{5, 0}`, while intersecting the two
adjacent ranges `{2, 3}` and `{5, 4}` returns `{0, 0}`. With `M = 2^64 - 1`,
intersecting `{M - 1, 4}` and `{M - 1, 1}` returns `{M - 1, 4}`. The latter
comes from the read modulo arithmetic and branch order, not a fitted result.
It is preserved; neither function exhibits a memory-safety defect because its
complete body performs only register arithmetic and comparisons.

## Dependencies, naming and iOS Device comparison

This dependency group is closed: neither native body calls any function or
loads any object/data. No native Foundation/CoreFoundation dependency is used
by the replacement. `ONSRange`, `ONSUInteger`, `ONSUnionRange` and
`ONSIntersectionRange` are the concrete coexistence declarations; `_ONSRange`
is the corresponding declaration tag. The shared declaration-derived switch
maps them to the SDK/native spellings in an isolated system-name build. The
aggregate crosses a proven raw-value ABI boundary and contains no CF/NS object.

The full native body bytes of each function are identical between the pinned
iOS Simulator and iOS Device images, including their unsigned conditions and
return instructions. This is an independent binary comparison, with verified
confidence at the device addresses above. It establishes no on-iPhone test,
iOS Device replacement build or equivalence of other Foundation functions.
There is no observed behavior divergence in this group.

## Verification and limits

`StandaloneTests/Foundation/Range/RangeTests.c` contains 30 literal fixtures
and 60 exact aggregate comparisons across two APIs. They cover overlap,
containment, disjoint/adjacent ranges, empty ranges at interior/start/end
positions, high-bit values and wrapping arithmetic. Its runner is
`runFoundationRangeStandaloneTests`; system-name execution is isolated from
native Foundation/CoreFoundation.

`Tests/Foundation/Range/RangeTests.c` compares the independently resolved
native and replacement functions on 11^4 edge-value tuples and 16,384 seeded
full-width tuples: 31,025 fixtures and 62,050 exact aggregate comparisons across
two APIs per invocation. Native and replacement aggregates are constructed
separately. The shared resolver verifies each native function's owning image
and UUID, each replacement function's owning image, and missing symbols; the
suite additionally rejects identical native/replacement addresses before any
call. Setup failures return 77, mismatches 1 and success 0. There is no tolerance.
The runner is `runFoundationRangeDifferentialTests`.

The focused local harness and logs are preserved under
`Local/Evidence/ns-range/FocusedVerification.log` and
`Local/Scratch/ns-range/FocusedVerification.py`. It checks Debug/Release and
both naming modes on a fresh pinned simulator, runs native differential cases
only in prefixed mode, scans standalone loaded images and invokes Clang static
analysis. The integrated acceptance commands are `make check` and `make verify`;
shared packaging, definition accounting, project generation and complete
Make/Xcode execution belong to the coordinating integration track. These two
scalar leaves have no object lifetime changes, and no performance claim or
framework replacement claim is made.

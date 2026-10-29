<!--
  Created by ktiays on 2026/10/29.
  Copyright (c) 2026 ktiays. All rights reserved.
-->

# CoreFoundation base scalar and byte utilities

## Identity and scope

This dependency group contains exactly two exported native functions:
`__CFRangeMake` and `CFHashBytes`. Both bodies are leaves: the IDA instruction
readings contain no calls, global data reads, registration, allocation,
Objective-C dispatch, callbacks, thread state or ownership operations. They can
be restored before the object runtime without a placeholder dependency.
`Source/CoreFoundation/Base` is inferred organization; no native source path was
recovered for these functions.

| Reference | Image UUID | SHA-256 | Functions |
| --- | --- | --- | --- |
| iOS Simulator 27.0 (24A434), arm64 | `D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC` | `e3598ca3b1ed3649c1ff8aadce058e98aa2d7be5385036a5ebb6ab986a5cc829` | `__CFRangeMake` at `0x1c0150dec`; `CFHashBytes` at `0x1c02561ec` |
| iOS Device 27.0.1 (24A446), arm64e | `9426FC9A-B36C-3096-B836-D17D6744A75B` | `ba22deb3759f56d32e5847fec0a5e675837e189d9a000b87ffdefe88a7942166` | `__CFRangeMake` at `0x18072d198`; `CFHashBytes` at `0x180665d1c` |

Canonical binaries and closed analyzed databases are under
`Local/Databases/iphone-simulator` and `Local/Databases/iphone-device`.
The simulator reading used an APFS binary clone under
`Local/Scratch/cf-base/iphone-simulator` and the IDA MCP session
`e9607603-2997-4674-a78d-45df8bafdd5d`. The coordinating session supplied the
independent device reading. Exact assembly, Hex-Rays output and image identity
are retained in `Local/Evidence/cf-runtime-reference/` as
`iphone-simulator-CoreFoundation-reading.json` and
`iphone-device-CoreFoundation-reading.json`. Confidence in the complete emitted
behavior of both leaves is **verified**.

The current IDA server returns `session_id`, not `database_id`. Access was
serialized by the coordinator; calls carried the returned session identifier,
and scripts asserted the scratch input path and hash before reading. The
simulator database was closed before another track received the reading slot.

The source compilation language was not recovered. The C-callable scalar
boundaries and lack of language-runtime operations are established by the
instructions; they do not prove that CoreFoundation was compiled as C. C11 is
the implementation tooling for these leaves. They introduce no ARC/MRC or
C++ ownership boundary, and establish no policy for unread object code.

## Native implementation

### `__CFRangeMake`

Both images contain only `RET`. The two incoming 64-bit values remain in
`X0` and `X1`, which are also the two-register aggregate return. The declared
`CFRange` ABI has `location` at offset 0 and `length` at offset 8, size 16 and
alignment 8. The binary preserves every bit of both arguments, including
negative lengths and signed extrema; it performs no range validation or
normalization.

The installed base declarations use `OCFIndex` (`signed long`),
`OCFHashCode` and `OCFOptionFlags` (`unsigned long`), and the anonymous
`OCFRange` aggregate. SDK declarations and the IDA loaded types establish their
interface widths, not implementation behavior. The untouched register return
establishes the implementation of the range constructor. The public header
exposes `__OCFRangeMake`; a separate `OCFRangeMake` inline or macro has not been
introduced by this track.

### `CFHashBytes`

`X0` is the byte-address input and the complete 64-bit `X1` participates in
address calculations. The initial branch compares signed `W1` with 4, so the
low 32 bits govern entry to the block loop. This truncation is preserved.

When signed low-32 length is at least 4, the remainder starts at
`length & 0x7fffffff`, and the first byte address is
`bytes + (length & 0xffffffff80000000)`. Each iteration reads four unsigned
bytes in increasing address order. Each byte is added to the previous 32-bit
hash shifted left by 4, with modulo-32 arithmetic. After each of the first
three bytes, the top nibble shifted right by 28 and then left by 4 is XORed
into the hash. After the fourth byte, the top-nibble mask `0xf0000000` is
XOR-folded right by 24 and cleared. The cursor advances by four and the
remainder decreases by four. The native old-remainder `> 7` continuation test
is equivalent to the restored new-remainder `> 3` test because the loop starts
with a remainder of at least four and exits with 0 through 3.

For a signed low-32 length below 4, the loop is skipped with a zero hash.
The tail switch uses only the low 32 remainder bits. Cases 3, 2 and 1 consume
respectively `bytes + length - 3`, `- 2` and `- 1`, in that order, applying the
same nibble fold and clear after every byte. Other values consume no bytes.
The final `MOV W0` returns a zero-extended hash in `X0`; equality tests compare
the entire 64-bit result.

For ordinary lengths 0 through `INT32_MAX`, this consumes exactly the requested
byte sequence. The unusual low-32 gate, high-address offset and no-read results
for selected negative/large lengths are retained. No generalized negative
length check or null check was added. The null-pointer cases in the tests are
limited to lengths whose instruction paths contain no load. Large lengths
whose low bits select a load need a valid correspondingly addressed buffer;
those fixtures are not executed.

## Dependencies and naming

The group has no native function or object dependency. `OCFBase.c` preserves
one native function boundary, and `OCFHashBytes.c` preserves the other, with
no added production helper function or global data definition. The existing
compiler/build policy remains responsible for the Mach-O platform and standard
low-level linkage.

`OCFBase.h` is installed through the shared public-header registry.
`OCFHashBytes.h` is a private source header because the symbol is exported by
the pinned image but is absent from the inspected public SDK headers. The
restored function remains exported, with its exposure recorded in the
restoration ledger. Its name follows the declaration-derived OCF-to-CF switch.
The same switch maps `__OCFRangeMake` to `__CFRangeMake` and all declared base
types. No native object or global Objective-C class is exchanged or registered.

## iOS Device comparison

Both range functions are the same register-preserving `RET`.
The device hash function computes a block address from original length minus
remainder on each iteration. The simulator advances a cursor. This instruction
scheduling/addressing difference is recorded at the definition as an
`iOS Device divergence:` comment. The independently read byte addresses,
read order, low-32 gate, masks, integer widths and returned bit patterns agree.
No behavioral divergence was found in these two emitted scalar bodies. This
is a binary comparison; no iOS Device build or on-iPhone execution is claimed.

## Verification and limits

`StandaloneTests/CoreFoundation/Base/OCFBaseStandaloneTests.c` checks the
aggregate size/offsets, 121 range cases, and 58 hash cases. The hash cases
include fixed native-confirmed outputs, high bytes, zero and selected no-read
lengths, and identical sequence fixtures placed against both inaccessible
page boundaries. Input byte immutability is also checked. The fixed outputs
were confirmed after the IDA reading by the UUID-guarded native probe retained
as `Local/Evidence/cf-base/HashProbe.c` and `NativeHashValues.txt`.

`Tests/CoreFoundation/Base/OCFBaseTests.c` resolves native and restored symbols
independently through the shared owning-image helpers, requires different
function addresses, and then compares exact bits. It covers 121 range cases
and 17,031 byte-hash cases: four byte patterns, sixteen alignments, every
length 0 through 257, eight larger lengths through 4096, and seven confirmed
no-read lengths. Native and restored input storage are independent. Setup
failures return 77; result differences return 1.

The coordinating session runs these suites through `make test` and the full
Make/Xcode, Debug/Release, prefixed/system-name matrix. Targeted compiler builds and execution passed for Debug and Release in both
naming modes; prefixed builds also passed both differential suites. Each
standalone process checked that no system Foundation/CoreFoundation image was
loaded. The produced base library had exactly the two selected exports and no
undefined symbols. Clang static analysis reported no diagnostics for either
definition. Evidence is retained as `Local/Evidence/cf-base/TargetedValidation.json`
and `StaticAnalysis.json`. These targeted builds do not substitute for the
coordinated packaging, ledger, coexistence and full build-system checks.

Object allocation, runtime type registration, retain/release, bridging and all
other CoreFoundation behavior remain outside this group's restored scope.
The finite scalar corpus does not establish framework replacement equivalence.

# CoreFoundation raw Unicode primitives

## Identity and scope

This reading restores the closed table-backed group needed by the raw UTF decoder:
`CFUniCharIsMemberOf`, `CFUniCharGetBitmapPtrForPlane`,
`CFUniCharDecomposeCharacter`, `CFUniCharFillDestinationBuffer`, and the native
recursive character-decomposition helper. It is not a restoration of complete
Unicode stream normalization, case conversion, bidirectional layout or CFString
objects.

| Reference | Identity |
| --- | --- |
| iOS Simulator | iOS 27.0 (24A434), arm64; CoreFoundation UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; SHA-256 e3598ca3b1ed3649c1ff8aadce058e98aa2d7be5385036a5ebb6ab986a5cc829 |
| iOS Device | iOS 27.0.1 (24A446), arm64e; CoreFoundation UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; SHA-256 ba22deb3759f56d32e5847fec0a5e675837e189d9a000b87ffdefe88a7942166 |

All code, constants and table payloads were read through IDA MCP on private APFS
clones. The full code/data evidence is retained in
`Local/Evidence/cf-unicode/{iphone-simulator,iphone-device}-unicode-reading.json`
and `*-unicode-data-spans.json`. The endpoint returned `session_id` rather than
`database_id`; scripts checked their exact scratch IDB path and input SHA-256.
Both databases were closed after reading.

| Native function | iOS Simulator address | iOS Device address | Confidence |
| --- | --- | --- | --- |
| CFUniCharIsMemberOf | 0x1c02512c4 | 0x1806d523c | Code verified; Device descriptor bindings unread |
| CFUniCharGetBitmapPtrForPlane | 0x1c0251524 | 0x1806400ec | Code verified; Device descriptor bindings unread |
| CFUniCharDecomposeCharacter | 0x1c0252910 | 0x1806fc7b4 | Code and direct table payloads verified |
| __CFUniCharRecursivelyDecomposeCharacter | 0x1c02529c8 | 0x1806fc864 | Code and direct table payloads verified |
| CFUniCharFillDestinationBuffer | 0x1c0252754 | 0x18077483c | Code and seven-byte prefix table verified |

The physical boundary consists of integer scalars, raw buffers, counts and const
bitmap pointers. There are no CF/NS objects, allocator requests, callbacks or
owned language-runtime values in these five definitions. Their C-callable ABI
is established by instructions and callers; it does not establish the original
source language of the framework. C11 is local implementation tooling, not an
ownership or language claim about other units.

The `Unicode` directory is an inferred subsystem organization. No original source
path was recovered for this group.

## Membership and bitmap records

`CFUniCharIsMemberOf` remaps character-set IDs 16 through 99 by adding 85, and ID 1
to 108. Whitespace, whitespace/newline and newline sets use their native scalar
branches. Other supported sets select one of 22 records after subtracting the
native offsets. Each record has a 32-bit plane count at offset 0, four bytes of
padding, and a pointer to a plane-pointer vector at offset 8; its stride is 16.
`UniCharBitmapData` and its field names are local descriptive spellings for this
read private record, not recovered source identifiers or a public ABI addition.

The table-backed path selects the plane from bits 16..23 and a bit from the low 16
bits. The high 8 bits of the raw 32-bit input are ignored on that path. Special
plane 14 control/format and inverted-legal-set branches retain their low-byte
checks. IDs outside the read record range return zero.

`CFUniCharGetBitmapPtrForPlane` follows its own ID remapping and 0x900f exclusion
mask, checks the full32-bit plane argument against the descriptor count, and
returns either the independently stored bitmap pointer or null. Pointer identity
is local to each implementation; it does not create CF/NS interoperability.

The simulator descriptor at 0x2227a65d8 names 22 vectors and 56 distinct bitmap
pools. Each plane is 8192 bytes: its exact storage is established both by the
native low 16-bit indexing and the following named storage boundary. The
production data unit defines those real pools and vectors, the descriptor, the
16,632-byte decomposition-table storage span and the 8,352-byte multiple-table
storage span. The word storage spans retain read padding through the next named
boundary; original declaration spelling/sizeof is not claimed. Every byte was
captured through IDA. These are required production constants, not copied code
from another Foundation implementation.

All 56 bitmap payloads and both decomposition payload spans independently agree
with their matching Device symbols. Their per-definition addresses and UUIDs
are recorded at definitions and in the ledger. Native private data identifiers
are preserved with hidden visibility. The inferred private record type remains
uninstalled. No global alias to a system identifier is packaged.

## Character decomposition

The Hangul branch uses an inclusive subtraction test through offset 0x2ba4,
including the unusual U+D7A4 upper boundary. It emits two or three 32-bit values
only when the caller's capacity suffices. The Device function outlines this body
into 0x1807c7d04; its branch constants, divisions and writes agree.

The recursive helper searches the read key/descriptor pairs, extracts the low 24
bits and the five-bit count, selects single or multiple storage, and follows the
native recursive-first-element flag. It subtracts the whole current count before
passing capacity into the recursive call. This can reject a buffer whose final
expanded result would fit; the implementation preserves that behavior. It
retains partial writes when a later native capacity check fails. An unmapped
character returns zero rather than copying itself.

## Destination buffer

Format 0 writes little-endian16-bit units; format 2 writes the native UTF byte
sequence; every other format writes 32-bit units. Capacity zero is count-only:
no destination bytes are written and the destination pointer remains unchanged.
The count starts from the caller's existing filled count.

The function publishes the destination pointer and filled count only on success.
A failure can leave earlier destination bytes changed while both inout values
remain unchanged. Empty/negative input counts retain the native successful
publication of their original state. Count arithmetic uses the native64-bit
wrap semantics.

The format 2 thresholds are 0x80,0x800,0x10000 and 0x200000. Scalars at or above
0x200000 substitute 65533 while selecting width 2, producing the native unusual
byte result; this behavior is preserved. The seven-byte firstByteMark payload is
`00 00 c0 e0 f0 f8 fc`. The implementation does not add Unicode-validity checks
that the native function lacks. Fixed byte stores also preserve unaligned raw
buffer behavior on the admitted little-endian arm64 target.

## iOS Device binding boundary

The Device descriptor/vector storage retains chained/auth pointer encodings in
this extracted image. A narrow IDA follow-up inspected descriptor cell
0x1e01748f0 and vector0x1e0539880. IDA returned no fixup and no data reference
for either cell; its image base was0x1805fb000. The exact result is retained in
`iphone-device-pointer-bindings.json`.

No mask or shared-cache base was fitted to these values. The 22 vector bindings
and descriptor target bindings remain unread. Their Device ledger state and
source comments explicitly say so. Descriptor widths/counts and individual
bitmap payloads were read independently, but these facts do not prove the
Device pointer bindings or Device behavioral equivalence. No Device build or
execution is claimed.

## Plane, property and priority group

Added 2026-10-29 as the first group below the scope's core component
([scope reading](../Scope.md)). It depends only on the primitives above and
on read tables; it has no CF/NS objects, allocator or callbacks. Evidence:
`Local/Evidence/cf-unicode-planes/{iphone-simulator,iphone-device}-reading.json`
(disassembly, pseudocode and byte spans captured through IDA on scratch clones)
and the generator `GenerateUnicodePlaneData.py` beside them.

| Native function | iOS Simulator | iOS Device | Linkage | Confidence |
| --- | --- | --- | --- | --- |
| CFUniCharGetBitmapForPlane | 0x1c02515a0 | 0x180715aa0 | private external | Verified; Device inlines the bitmap-pointer lookup |
| CFUniCharGetNumberOfPlanes | 0x1c0251870 | 0x180715de4 | private external | Verified; identical instructions |
| CFUniCharGetUnicodePropertyDataForPlane | 0x1c025268c | 0x18065673c | exported | Verified; identical instructions |
| __CFUniCharPrioritySort | 0x1c0253324 | 0x1806fcf40 | non-external | Verified; register allocation differs |
| CFUniCharPrioritySort | 0x1c0253320 | 0x1806fcc28 | exported | Verified; tail call |

**Plane counts.** Sets 1 and 108 answer 15, sets below 4 answer 1 and set 12
answers 17. Other sets take the membership remapping and read the 32-bit count
of their `__CFUniCharBitmapDataArray` record. The record index is not checked:
identifiers 26..99 and above 110 read past the 22 records in the native code
too. The restoration keeps the unchecked load, and the tests use only the
identifiers that map to read records.

**Bitmap copy.** The function first asks `CFUniCharGetBitmapPtrForPlane` and
copies or inverts its 8192 bytes. Without a stored bitmap:
- The legal set (12) copies the legal bitmaps with the opposite inversion.
- Plane 14 of the legal set writes 2 followed by three bytes that differ from
  the remaining 8188.
- Planes 15 and 16 of the legal set are filled except byte 8187 (0x3f/0xc0).
- Plane 0 of sets below 4 and set 15 is built from the whitespace and newline
  scalars. The seven newline values (LF, VT, FF, CR, NEL, LS, PS) are an
  unnamed 16-byte local table at 0x1c03a46a8 (Device 0x1808a0718), restored as
  `kNewlineCharacters`.
- Otherwise the result is 1 or 0xff by inversion and set kind, and nothing is
  written.

The inversion flag is tested by bit 0 and as a whole word. Every native caller
passes 0 or 1, so the restoration declares it `bool`.

**Property data.** `__CFUniCharUnicodePropertyTable` holds two records with
the bitmap-record layout: 2 combining-priority planes and 15 bidi planes, of
which planes 2..13 are null. The property type is not checked.

**Priority sort.** It skips leading characters whose combining priority is zero
(characters at or above U+20000 have priority zero), stops when fewer than five
bytes remain, and bubble-sorts the rest. After a swap the previous priority
becomes the current element's, not the larger moved one; the restoration keeps
that order. No native caller reads the returned register (callers in
`__CFStringFoldCharacterClusterAtIndex`, `CFStringNormalize` and
`CFUniCharDecomposeWithErrorLocation` were checked), so it returns `void`.

**Tables.** The combining-priority and bidi planes are captured byte for byte
and agree with the Device image. The combining-priority planes exist three
times as non-external symbols, which suggests a table header included in
several native translation units. The property table uses the copy at
0x2227a6738; the sort uses the copy at 0x2227a67e0, which sits beside the
decomposition tables. Both are restored; the sort's copy is named
`__CFUniCharDecompositionCombiningPriorityTable*`, a recorded collision mapping
because the restored data unit holds both. Bidi plane 14 is restored as 768
bytes. That is the Device storage up to its next symbol; the simulator span is
788, and its last 20 bytes are unnamed data this group does not reach. Device
pointer vectors are chained fixups whose targets decode by offset to the matching
Device symbols; their authentication/diversity bits remain unread.

**Tests.** The standalone suite adds 20 fixed cases. The differential suite
resolves the hidden native functions by IDA offset from the pinned image base,
and calls the hidden replacements from production sources compiled into the
test tools (`Configuration/InternalTestSources.json`). It adds 206,237 cases:
- plane counts for every read identifier;
- property planes 0..299 for both types;
- 140 sets × 20 planes × both inversions of bitmap copies, with canaries;
- 200,000 deterministic sort arrays of lengths -1..14.

## Verification and remaining work

The standalone corpus has 20 fixed raw primitive cases and checks execution
without system Foundation/CoreFoundation. It covers whitespace distinctions,
letter/digit membership, recursive capacity behavior, the Hangul upper edge,
UTF byte output, count-only conversion and failure publication after a partial
write.

The differential corpus independently resolves each owning native and replacement
image, pins the native UUID, rejects equal function addresses, and constructs
separate output buffers and counters. It compares exact result bytes, complete
output/canary buffers, pointer offsets and filled counts. Coverage includes all
0..0x10ffff inputs for 12 character sets and decomposition, 4096 deterministic
raw 32-bit inputs, all BMP values with capacities0..5, 150 set IDs over 256 planes,
and destination format/capacity/preexisting-count/alignment boundaries.
There are 14,980,800 finite differential cases per prefixed configuration.

Targeted Debug/Release and both naming modes run on fresh pinned simulators.
The coordinating session owns shared suite registration, definition accounting
and the complete Make/Xcode matrix. Per-mode logs and the focused harness are
under `Local/Scratch/cf-unicode`; products use
`.build/Unicode/iphonesimulator-iphonesimulator-<configuration>-<mode>`.

The full stream functions `CFUniCharDecompose`/`WithErrorLocation` call
CFAllocatorAllocateTyped and CFAllocatorDeallocate when a combining run grows
past its 32-scalar stack buffer. They remain unbuilt until the real allocator
closure lands. Case mapping and compatibility decomposition (`CFUniCharMapCaseTo`,
`CFUniCharGetConditionalCaseMappingFlags`, `CFUniCharCompatibilityDecompose`)
are not restored yet; they need the case-mapping tables. No placeholder allocator, partial
stream flag branch, lifetime claim or performance claim is introduced.

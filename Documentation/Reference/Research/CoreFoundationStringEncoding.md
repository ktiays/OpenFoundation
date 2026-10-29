# CoreFoundation stateless string-encoding kernels

## Identity and restored boundary

This group restores fourteen original hidden functions and nine private tables:
UTF16-to-UTF8 encoding and encoded-length calculation, and six pairs of scalar
single-byte conversion functions. The pairs are ASCII, ISO Latin1, Windows
Latin1, NextStep Latin, MacRoman Latin1 and DOS Latin1. MacRoman Latin1 is the
native `__CFToMacRomanLatin1` family; it is not relabeled as the separate
MacRoman encoding-zero converter. No public encoding dispatcher, registry,
CFString object operation, or UTF8 decoding/normalization path is restored by
this group. The family has zero new dynamic exports.

| Reference | UUID | SHA-256 | IDA image base |
| --- | --- | --- | --- |
| iOS Simulator 27.0 (24A434), arm64 CoreFoundation | `D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC` | `e3598ca3b1ed3649c1ff8aadce058e98aa2d7be5385036a5ebb6ab986a5cc829` | `0x1c014a000` |
| iOS Device 27.0.1 (24A446), arm64e CoreFoundation | `9426FC9A-B36C-3096-B836-D17D6744A75B` | `ba22deb3759f56d32e5847fec0a5e675837e189d9a000b87ffdefe88a7942166` | `0x1805fb000` |

All implemented branch, arithmetic, table, write-order and ABI conclusions have
**verified** confidence from the independent pinned IDA readings. Native
addresses occur at every source definition and in the restoration ledger. The
native original source paths/compilation language are unestablished. This
project's `StringEncoding` directory and C11 implementation setting are tooling
choices, not claims that the native framework was compiled entirely as C. These
functions have C-callable raw integer/pointer boundaries and no language-runtime
or object ownership operation.

Evidence was captured through IDA MCP on private APFS clones under
`Local/Scratch/cf-encoding/{iphone-simulator,iphone-device}`. Both databases were
closed before handing the server to the Unicode owner. The server returned
`session_id`, not `database_id`; the owner serialized access and every capture
asserted the exact IDB path and input SHA-256. The simulator `__CFToUTF8` body at
`0x1c0168fe4` was additionally read using explicit MCP decompile and disasm calls.
Evidence files are `Local/Evidence/cf-encoding/iphone-simulator-encoding-reading.json`
and `iphone-device-encoding-reading.json`. Additional exact scalar-map spans
were captured through the Unicode owner's guarded IDA MCP session in
`Local/Evidence/cf-unicode/{iphone-simulator,iphone-device}-unicode-data-spans.json`,
under `encoding_primitives`; no table was inferred by inversion or fitted to probes.

## UTF8 encoder and length kernel

`__CFToUTF8` accepts flags, a UTF16 input pointer and signed-long unit count, a
byte output pointer/capacity, and an optional signed-long used-byte pointer. It
returns consumed UTF16 units. It returns zero and publishes zero used bytes for
an input count below one. Capacity zero is a count-only operation and performs
no destination writes. Pointer/count arithmetic follows the emitted 64-bit
address operations; integer addresses avoid C null-pointer arithmetic in
count-only mode.

ASCII units write one byte. A high surrogate combines with a following low
surrogate; otherwise bit 7 determines whether an unpaired surrogate is encoded
as a three-byte value or stops conversion. A standalone low surrogate follows
the same flag decision. Other flag bits are ignored by this native kernel.
Insufficient output capacity leaves the current character or surrogate pair
unconsumed, with no bytes written for that character. Multibyte output is stored
from its final continuation byte backwards, ending with the lead byte, exactly
as read. The `firstByteMark[7]` contents come directly from each image.

The output-end comparison is 64-bit, but the remaining-capacity subtraction for
multibyte values is intentionally 32-bit. The restoration keeps this unusual
width and does not substitute a size_t capacity check. The finite corpus uses
ordinary small buffers; the greater-than-four-GiB capacity boundary is read but
not dynamically exercised.

`__CFToUTF8Len` is a separate native function. It ignores flags, counts an
unpaired surrogate as three bytes, combines valid pairs, and accumulates in a
wrapping unsigned 32-bit register before returning the zero-extended value.
It is not rewritten as a call to the encoder. Negative counts and invalid input
pointers are not native fixtures; the native length loop has no negative-count
validation. Large accumulator-wrap cases remain statically read but untested.

## Single-byte functions and tables

Each encoder retains its own native function and original range gates and
binary-search order. ASCII and ISO Latin1 use direct range checks. Windows
Latin1 handles ASCII and U+00A0 through U+00FF directly, with its remaining
mapping in a 27-entry pair table. NextStep Latin, MacRoman Latin1 and DOS Latin1
use their independently read range gates and 127-, 130- and 128-entry pair
tables. No common replacement lookup function is introduced where the native
functions contain their own loops.

All byte-to-unit functions keep their native output timing. An undefined
Windows/NextStep byte writes U+FFFD before returning false. ASCII rejects bytes
with bit 7 set without touching the output. ISO Latin1 always writes the
zero-extended byte. MacRoman Latin1 and DOS Latin1 use their native high-byte
lookup tables and the native U+FFFD comparison. Flags are ignored by all twelve
scalar functions.

Source table storage preserves the captured bytes and native names. The DOS
reverse table's IDA item contains 130 uint16 values; the native index uses only
the first 128. The extra captured bytes are preserved, without claiming their
original C declaration or making them reachable through a fabricated API.

## Dependencies, naming and iOS Device comparison

These fourteen functions have no reachable allocator, CF/NS object, callback,
Unicode-normalization, ICU, registry, or thread-state dependency. The nine
constant tables close their dependency graph. Original private function/data
names are preserved as hidden or static definitions; the headers are private
and not installed. The naming switch still controls public OCF/ONS declarations;
these original internal names do not become process-shared symbols.

The iOS Device counterparts were read independently. Function branch conditions,
raw scalar ABI, output-before-failure behavior, flags, table bytes and integer
widths agree. UTF8 encoder and length-code block ordering/register allocation
varies, with no observed raw-value behavior difference. This does not establish
iOS Device runtime equivalence; there is no iOS Device build or execution here.
No native defect is intentionally changed in this group.

The already captured exported dispatchers remain unbuilt. Their complete
closure includes converter caches/registry, ICU/platform paths and Unicode
stream decomposition. `__CFFromUTF8`/`__CFFromUTF8Len` require real
`CFUniCharIsMemberOf`, `CFUniCharDecomposeCharacter` and
`CFUniCharFillDestinationBuffer` implementations for their normalization flags.
Those are a separate dependency group; no flag is silently ignored or forwarded
to native Foundation/CoreFoundation to make a partial dispatcher appear complete.

## Verification and limits

Targeted internal tools compile the same two production implementation files
into their own executable image. No test-only production export, helper or stub
is introduced. The shared native-address resolver verifies the owning native
image UUID and that each IDA-relative address lies in executable __TEXT; tests
also verify every replacement pointer belongs to the current test image and is
different from the native pointer. Native/replacement inputs and output buffers
are independently constructed.

Debug/Release with both naming modes passed 41 standalone assertions per run.
Debug/Release prefixed differential runs each passed **7,911,208 exact
comparisons**: every uint16 value for each scalar encoder, all 256 bytes for each
scalar decoder, flag variations, output guards, every UTF16 unit at six output
capacities, mixed surrogate/encoding boundaries, deterministic multicharacter
vectors, consumed/used counts and the separate length function. Standalone tools
verified that system Foundation/CoreFoundation were absent. Test logs are under
`.build/Kernel/iphonesimulator-iphonesimulator-<configuration>-<mode>`; retained
results are recorded in `Local/Evidence/cf-encoding/validation.json`.

The coordinated Make/Xcode Debug/Release, both-name matrix and coexistence checks
must pass after shared integration before the track lands. Finite raw-kernel
coverage is distinct from public API, CFString, registry, Unicode-normalization
or framework replacement coverage. There are no object lifetime changes or
performance claims in this group.

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

## Encoding names, NSStringEncoding mapping and converter wrappers

A second group restores seventeen functions and five tables: the nine converter
wrappers `__CF{ToBytes,ToUnicode,ToCanonicalUnicode}{CheapEightBit,StandardEightBit,CheapMultiByte}Wrapper`,
`__CFDefaultToUnicodeFallbackProc`, the three platform converter entry points,
`CFStringConvertEncodingToNSStringEncoding` (exported as
`OCFStringConvertEncodingToNSStringEncoding`), and
`__CFStringEncodingGetWindowsCodePage`, `__CFStringEncodingGetCanonicalName` and
`__CFStringEncodingGetICUName` with `__CFKnownEncodingList`, `__CFWindowsCPList`,
`__CFCanonicalNameList` and two unnamed tables. Evidence is the `encoding-names`
group of `Local/Evidence/below-core-capture/{iphone-simulator,iphone-device}.json`
(disassembly, pseudocode, caller contexts, data bytes, decoded pointer slots).
`Local/Evidence/cf-encoding-names/generate-tables.py` decodes every table from that
capture, checks that both images agree entry for entry (565 entries, including the 257 of the open
byte-conversion group below) and prints
the initializers used in the source. Every definition carries **verified**
iOS Simulator confidence; native addresses are at each definition and in the ledger.

Placement is inferred from address adjacency, not from recovered paths:
`OCFStringEncodingConverter.c` holds the contiguous wrappers and fallback; `OCFPlatformConverter.c` the platform entry
points next to `__CFStringEncodingGetExternalConverter`;
`OCFStringEncodingDatabase.c` the name/code-page functions and their sorted
tables; `OCFStringEncodings.c` the NSStringEncoding mapping next to
`CFStringConvertEncodingToIANACharSetName`. The existing kernel files are unchanged.

**Converter record.** Each wrapper receives a pointer to the converter definition
record in X0 and reads exactly two slots: offset 0, the to-bytes callback, and
offset 8, the to-unicode callback. `StringEncodingConverterRecord` describes only
those 16 bytes; the rest of the native record is not read here. Callback shapes
follow the argument registers: cheap eight-bit `(flags, unit, uint8_t *)` and
`(flags, byte, uint16_t *)`; standard eight-bit `(flags, units, count, uint8_t *)`
and `(flags, byte, uint16_t buffer[])`; cheap multibyte `(flags, unit, uint8_t buffer[])`
and `(flags, bytes, count, uint16_t *)`. Every callback result is tested or
zero-extended as a whole 32-bit W0. No concrete converter is restored.

**Wrapper behavior.** Cheap eight-bit wrappers process `min(maxLen, count)` (or the
count when maxLen is zero), stop at the first failing callback and publish the
processed count; the scalar scratch is zeroed once, so a callback that succeeds
without writing repeats the previous value. Standard eight-bit to-bytes and cheap
multibyte to-unicode return early only for a zero count (CBZ) and then advance by
each callback's count until the remaining count reaches zero; the other wrappers
reject counts below one. Standard eight-bit to-unicode copies from a 40-byte stack
buffer with `memcpy` after a capacity check; cheap multibyte to-bytes copies from a
20-byte uninitialized buffer, reloading the used count per byte. The canonical
wrappers decompose units at or above U+0080 that are members of character set
0x65 with `CFUniCharDecomposeCharacter(unit, buffer, 10)`, store the used count
before the decomposition, and return at once without a final store when a
decomposed value does not fit; with a zero capacity this ends the conversion at
the first decomposable unit. Values at or above 0x10000 become surrogate pairs.
`__CFToCanonicalUnicodeStandardEightBitWrapper` writes the callback's source unit
(not the decomposed values) once per decomposed value, building its surrogate pair
from that unit, and stores undecomposed units without a capacity check, so a
callback returning several units can write past `maxCharLen`. Both are kept as
native behavior: the image establishes them, and whether a native converter
reaches the overrun is outside this evidence (no defect fix is applied).

**Fallback and platform.** `__CFDefaultToUnicodeFallbackProc` writes `?`
only for a nonzero capacity, publishes one used unit and returns one. The three
platform functions ignore their arguments and return 3, 3 and 0; their parameter
lists follow their only callers' argument registers.

**Open group: byte-to-unit conversion and the character tables.**
`__CFStrConvertBytesToUnicode` (0x1c022c2a4, external; iOS Device 0x180627fa0)
loads `___CFCharToUniCharTable` once and maps each byte through it for a count of
at least one; no caller reads X0. It is read but **not restored**, because its
observable behavior depends on a run-time writer that is not in the evidence, so
its dependency group is not whole. The tables (linkage from `nm -m`):
`___CFCharToUniCharTable`, `__DATA,__data`, private external, 0x22e6cfbc8, an
8-byte pointer whose static value is `___CFMutableCharToUniCharTable` (iOS Device
0x1e6f09d18, chained fixup to 0x1e3f239f0); `___CFMutableCharToUniCharTable`,
`__DATA,__data`, non-external, 0x22e6cf9c8, 256 uint16 entries whose static
contents are the identity mapping in both images; and
`___CFIdempotentCharToUniCharTable`, `__TEXT,__const`, external, 0x1c03a30d4, not
referenced by any function of this group. In a test process with the native
framework loaded, the native table differs from the static identity contents in
123 of 256 entries (byte 0x80 maps to U+00C4, a MacRoman mapping), so it is
rewritten at run time. The writer, and whether it also repoints
`___CFCharToUniCharTable` (for example to the idempotent table), is **unread**; this
group is reopened when that code is read.

**Names and code pages.** `CFStringConvertEncodingToNSStringEncoding` maps the two
exact values 0x8000100 and 0x600, then the low twelve bits (a zero field maps to
30 for any upper bits, 0x100 only for the exact value), and otherwise returns
`encoding | 0x80000000` zero-extended. `__CFStringEncodingGetWindowsCodePage`
returns `28590 + (encoding & 0xff)` for group 0x200, a seven-entry table indexed by
`(encoding - 0x100)` rotated right 26 bits for group 0x100, and otherwise the
parallel `__CFWindowsCPList` entry found by a binary search of the 98 sorted
`__CFKnownEncodingList` entries. `__CFStringEncodingGetCanonicalName` formats
`cp%d`/`windows-%d` for groups 0x400/0x500, `iso-8859-%u` for 0x200, one of eight
UTF names for 0x100, and otherwise `%s%s` with the `x-mac-` prefix for a nonzero
group-0x000 encoding or 0xa04 and the parallel `__CFCanonicalNameList` name; it
returns `bufferSize > snprintf(...)` compared as signed 64-bit values.
`__CFStringEncodingGetICUName` returns the constant `IMAP-mailbox-name` for 0xa10,
tries `ucnv_getAlias` on `windows-<code page>` outside group 0x100, then on the
canonical name, in a 60-byte buffer; one error code zeroed on entry is shared by
both ICU calls. ICU is an external cut: the native image carries an ordinary
`LC_LOAD_DYLIB` load command (not weak, re-export or upward) for
`/usr/lib/libicucore.A.dylib` (`otool -L`: compatibility 1.0.0, current 78.1.0),
and `nm -m` lists `_ucnv_getAlias` as undefined from libicucore; the SDK's `libicucore.tbd` exports `ucnv_getAlias`,
and `OF_CORE_FOUNDATION_LDFLAGS = -licucore` links it into OpenCoreFoundation and
the test tools (Make and Xcode). The SDK has no `ucnv.h`; the one prototype is
declared in the database source.

**iOS Device comparison.** Tables agree entry for entry; Device pointer slots are
chained fixups whose targets decode to the same strings (authentication bits
unread). Every wrapper keeps its control flow and stack layout but calls the
record's callbacks with `BLRAAZ`; this is recorded as an `iOS Device divergence:`
at each wrapper. `__CFStringEncodingGetCanonicalName` (0x1806eb200) tests the ISO
group first and outlines its binary-search steps, read from
`Local/Evidence/below-core-capture/iphone-device-encoding-outlined.json`:
`_OUTLINED_FUNCTION_1_48` (0x180613ec0) loads the list base, the last element
(base + 0xc2) and the key `W0 & 0xfff`; `_OUTLINED_FUNCTION_0_65` (0x18065cb54)
forms the midpoint `low + ((high - low) >> 2) * 2` with an arithmetic shift, loads
its halfword and compares it with the key; `_OUTLINED_FUNCTION_2_30` (0x180613938)
forms `mid - 2` and `mid + 2`, compares again and selects the low bound with
`CSEL ... CC`, and the caller selects the high bound with the same flags (`RET`
preserves them) before the unsigned bound test. This is the iOS Simulator search
instruction for instruction, split across calls; its Device confidence is verified. `CFStringConvertEncodingToNSStringEncoding` orders its comparisons
differently with the same mapping; the remaining functions match instruction for
instruction or by indexing only.

**Verification.** The differential suite resolves the exported native by name
and the sixteen hidden ones by IDA offset, compiles the replacements into the test
tool, and compares 3,491,806 cases per prefixed run: every encoding below
0x100000 for the NSStringEncoding mapping; every value below 0x20000, every
twelve-bit field under thirteen upper-bit patterns and all 64 Unicode rotations for
code pages, canonical names (nine buffer sizes, canary buffers) and ICU names;
fallback and platform functions; and 24,000 random cases per wrapper driven by a test converter
record whose deterministic callbacks are shared by native and replacement (failure
inputs, non-writing successes, nonzero results with zero low bits, multi-unit
results, decompositions to one to four values and to supplementary values, ten
capacities, negative counts where the native code rejects them). The standalone
suite adds 46 hand-checked assertions in both naming modes.

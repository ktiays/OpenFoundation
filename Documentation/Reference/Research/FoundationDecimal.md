<!--
  Created by ktiays on 2026/10/29.
  Copyright (c) 2026 ktiays. All rights reserved.
-->

# Foundation raw decimal arithmetic

## Identity, scope and evidence

This dependency group restores the eleven raw decimal C APIs and their complete
integer and Swift arithmetic dependencies. It does not restore NSDecimalString,
NSDecimalNumber, NSNumber, locale formatting, object bridging, or a public Swift
Foundation module. The Foundation framework remains incomplete.

| Reference | Image identity | SHA-256 |
| --- | --- | --- |
| iOS Simulator | iOS 27.0 (24A434), arm64, Foundation UUID `49FCDA5F-A93E-3259-857C-FC75AC2C7B03` | `bc7e0a5f693ffc75ae7c3311b817716e86d149fda2f8c45669a976727322cb18` |
| iOS Device | iOS 27.0.1 (24A446), arm64e, Foundation UUID `C041F1D0-5EF1-30A3-BF96-830976173B56` | `c54c72935c506e8b3348e20aa758a8a97e95051506ffc2afb9b186726a369348` |

All implementation facts below come from IDA MCP disassembly, decompilation,
function boundaries, code/data references and referenced bytes in these two
images. SDK declarations establish only the public interfaces. Differential
execution confirms the reading; it does not supply unread behavior.

The identity-guarded captures are
`Local/Evidence/ns-decimal/{iphone-simulator,iphone-device}-Foundation-decimal.json`,
with one text stream per function address and separate `*-Supplement.json`
records for the error case descriptors and inlined getter boundary. Canonical
images and closed databases are under `Local/Databases/<platform>/Foundation*`.
Only private APFS clones in `Local/Scratch/ns-decimal/<platform>` were opened.
The server exposes a client session identifier rather than a database identifier;
every read used the returned session, coordinator-granted exclusive ownership,
and exact input path/hash guards. Both databases were closed before releasing
that ownership.

The simulator capture contains 78 functions and the device capture 71. These
counts include compiler support, shared bodies, specializations and outlined
code. They are not API counts. The authored group contains eleven C definitions,
33 explicit Swift function/accessor/initializer bodies, two Swift support types
and one storage definition. Ten hidden C bridges are compiler generated from
those Swift definitions. The exact compiler USRs, native addresses and emitted
bridge symbols are recorded in the restoration ledger; the unversioned handoff
is `Local/Evidence/ns-decimal/RestorationRows.json`.

## ABI and native language boundaries

`NSDecimal` occupies 20 bytes, has alignment 4, and stores the mantissa at offset
4. The complete C and Swift field access streams establish the following layout.

| Storage | Native field | Width and position |
| --- | --- | --- |
| Header word | `_exponent` | Signed 8-bit integer, bits 0–7 |
| Header word | `_length` | Unsigned 4-bit integer, bits 8–11 |
| Header word | `_isNegative` | Bit 12 |
| Header word | `_isCompact` | Bit 13 |
| Header word | `_reserved` | Bits 14–31 |
| Bytes 4–19 | `_mantissa` | Eight little-endian UInt16 words, least significant first |

A zero declared length with the negative flag set is NaN. A finite zero has zero
length and a clear negative flag; other stored fields can remain noncanonical.
Algorithms preserve or canonicalize those fields exactly where the native body
does so. Result comparisons include all 20 bytes, including inactive words and
reserved bits.

The public rounding enum has UInt-sized storage: Plain 0, Down 1, Up 2, Bankers
3. The public calculation error enum is NoError 0, LossOfPrecision 1, Underflow
2, Overflow 3, DivideByZero 4. The private Swift `_CalculationError` is a
three-case payload-free enum: overflow tag 0, underflow tag 1, divideByZero tag
2. Both field descriptors independently resolve the same ordered case strings
at simulator `0x1c14752c4` and device `0x181b9e290`.

The public Copy routine has a raw C ABI; its original authored source language
is not recoverable from its symbol alone. The replacement compiles that routine
and the public tail thunks as C. The other ten public entries are single tail
branches into hidden C bridges owned by Swift. Mangled functions, Swift error
register handling, metadata and recovered fatal filenames establish the Swift
boundary. Each bridge is represented by an internal Swift function with
`@_cdecl`, rather than by a replacement C arithmetic implementation. The Round
bridge itself contains its Swift body in the native image; a distinct mangled
body is not present. Compiler emission may split that entry and its C thunk.

`Decimal+Math.swift` and `Decimal+Conformances.swift` are recovered native source
filenames. The subsystem directory and `Decimal+IntegerArithmetic.swift` and
`Decimal+Operations.swift` organization are inferred. Repeated native compaction
code stays repeated inside its native arithmetic bodies. No convenience helper
was invented to combine those functions.

Source declarations use concrete ONS names. The shared selected-source compiler
uses module OpenFoundation in coexistence mode and Foundation in isolated system
name mode, with generated copies outside Source and a private textual C bridge.
Only exact `@_cdecl`/`@_silgen_name` linkage literals participate in declaration
mapping. Native source filenames, diagnostic text and runtime strings retain
their literal values. The external Swift enum-case diagnostic mangled name is
preserved verbatim. Private Swift identifiers and metadata remain private;
installed headers expose only the eleven declared raw C entries and their types.

The native image loads libswiftCore with ordinary load strength and
libswift_Builtin_float weakly. Those Swift compiler/runtime dependencies and
libSystem are the resulting production dependencies. There is no native
Foundation/CoreFoundation or FoundationEssentials arithmetic fallback. Native
Swift allocation, errors, dynamic casts, once initialization and fatal-error
calls remain Swift compiler/runtime boundaries.

## Public entries

Addresses in this reading are IDA virtual addresses. Every row below was read
independently in both pinned images, with verified confidence. Device comparison
is a static image comparison, not a device build or behavioral test.

| Native entry | iOS Simulator | iOS Device |
| --- | --- | --- |
| `NSDecimalCopy` | `0x1c04f90d0` | `0x180c9dab8` |
| `NSDecimalCompact` | `0x1c04f9214` | `0x180eff4a0` |
| `NSDecimalCompare` | `0x1c04f9208` | `0x180eff494` |
| `NSDecimalRound` | `0x1c04f920c` | `0x180eff498` |
| `NSDecimalNormalize` | `0x1c04f9210` | `0x180eff49c` |
| `NSDecimalAdd` | `0x1c04f91f0` | `0x180eff47c` |
| `NSDecimalSubtract` | `0x1c04f91f4` | `0x180eff480` |
| `NSDecimalMultiply` | `0x1c04f91f8` | `0x180eff484` |
| `NSDecimalDivide` | `0x1c04f91fc` | `0x180eff488` |
| `NSDecimalPower` | `0x1c04f9200` | `0x180eff48c` |
| `NSDecimalMultiplyByPowerOf10` | `0x1c04f9204` | `0x180eff490` |

Copy first clears destination reserved bits, writes the source sign, samples the
source length, then writes length, exponent and compact. It transfers only the
active mantissa words and preserves inactive destination words. This order also
applies to self-copy. The device uses indexed transfers while the simulator uses
post-indexed pointers; the observable writes agree for valid lengths.

Compare orders NaN before all finite values and equates NaNs. It compares signs
and declared zero lengths, normalizes local copies while discarding the thrown
error, then compares integer words and reverses the ordering for a negative
operand. Its unexpected internal comparison value calls the read external Swift
standard-library diagnostic.

Round computes the throwing helper before storing its output. Its C bridge
catches and releases a Swift error and leaves the caller's result bytes unchanged.
Normalize also works on two local copies. It stores both only on success,
including when the two pointers alias, and maps the private error tags to public
codes without changing the original values on failure.

Add and Subtract retain separate native arithmetic bodies. Subtract first copies
the right operand and flips its sign only when its declared length is nonzero.
Equal signs call integer addition; opposite signs compare and subtract. Equality
returns canonical zero. Add returns Normalize's precision-loss flag but ignores
the fitMantissa precision-loss flag. Subtract's successful result returns NoError,
even when normalization loses precision. Both compact in the same native bodies.

Multiply, Divide and MultiplyByPowerOf10 compute the throwing result before
storing it. Their catch paths dynamically cast the error first, derive public
codes 3/2/4 (or 0 for an unknown error), write a canonical NaN, then release the
caught error when leaving the catch. No retained error escapes the raw C API.

Power's public NSUInteger bits enter a signed Swift Int without conversion. For
example ULONG_MAX means exponent -1. NaN is checked before the zero exponent,
so NaN to power zero returns overflow and NaN. A finite zero with a negative
signed exponent returns NaN with NoError. Other exponents use native repeated
squaring, a final multiply and, for a negative exponent, division of one by the
computed value. Checked arithmetic and the native operation order are retained.

## Integer and decimal kernels

`VariableLengthInteger` has an Int length at offset 0 and seventeen UInt16 words
at offset 8: size 42, alignment 8, stride 48. `asVariableLengthInteger` copies all
eight stored NSDecimal words independently of declared length. The reverse copy
throws overflow before changing the decimal when length exceeds eight, updates
the length, preserves other header fields and clears inactive result words.

The integer helpers preserve the native carry/borrow widths, result-length
trimming and capacity tests. Addition permits a carry append at an index equal
to maxResultLength and throws only when the limit is smaller than that index.
Long division preserves the two estimate-correction tests, wrapping borrow
arithmetic and limited add-back. Single-word division does not enforce the
result capacity argument. Simulator long-division decompilation failed with
Hex-Rays INTERR 50920; its complete ARM64 instructions and the independent device
decompilation establish the implementation. Instruction anchors are retained in
`Local/Evidence/ns-decimal-integer/DivisionReading.md`.

The global powerOfTen storage has 39 literal entries. All active and inactive
words were transcribed from the captured initializer stores and referenced
vectors, then reconstructed independently from device stores. The two tables
agree. Its native once initializer is simulator `0x1c08cd960`, device
`0x180ebea80`. Optimized emitted IR retains swift_once and a single 1904-byte
Swift allocation with alignment mask 7, matching the native storage boundary.

fitMantissa removes decimal digits until the integer fits eight words and
records precision loss. It computes a rounded-add value but normally returns the
pre-add words because of the native branch. In its banker's exact-half branch it
requests an increment for an even retained low word. These unusual native paths
are preserved rather than replaced with a different decimal rounding algorithm.

Short multiplication by zero clears only length and preserves flags and stored
words. Short addition iterates the entire declared length even after carry is
zero. Short division forms the integer before checking the divisor and clears
inactive result words through the reverse-copy helper.

Round checks signed `scale + exponent` before the 32767 no-scale sentinel. It
removes four decimal digits per step only while at least five remain, then one
at a time. Earlier nonzero remainders make the final zero/five remainder sticky
by incrementing it. The native negative-zero cleanup is skipped when all removed
digits were zero. Unknown rounding values take the no-increment path.

Normalize ignores the supplied rounding mode. It tries full scaling first. On
failure, available decimal digits are computed with the exact Double bit pattern
`0x401344135094c2f0`, multiplication and floor. Device constant storage is
`0x18174bc68`; the simulator constructs the same bits in registers. The retry can
clear the lower operand's length while retaining its sign. This path returns
precision loss. Its local mutations and throw guards follow the native order.

Full multiplication checks NaNs, then finite zeros, multiplies with sixteen-word
capacity, and fits only products of at least nine words. It discards fitMantissa's
precision flag. It checks exponent >= 128 but has no lower exponent bound;
negative exponents wrap through the signed 8-bit setter. This is retained.

Full division checks NaNs, then zero divisor, then zero numerator. It attempts
normalization only for an exponent difference of at least 19, restoring both
original operands if normalization yields zero. It scales by the native table,
divides, and fits with literal NSRoundDown 1, ignoring caller rounding mode.
It then checks both exponent bounds. A device Hex-Rays rendering omits an error
guard after integerDivide; the native `CBNZ X21` at `0x180ebe2c4` proves the guard,
matching simulator `0x1c08d037c`. No behavior difference is inferred from that
decompiler omission.

MultiplyByPowerOfTen throws overflow for NaN, returns canonical all-zero storage
for a finite zero, checks conversion to Int32 and checked Int32 exponent addition,
and throws underflow below -128 or overflow above 127.

Compact skips already compact values, NaNs and declared zeros without changing
bytes. Otherwise it divides until a remainder or zero, undoes the extra division
with multiply-by-ten/add, multiplies back to cap exponent at 127, and marks the
result compact. Native try-force locations 542/553/554/559 in
Foundation/Decimal+Math.swift are retained with source-location directives.

The explicit Swift definition addresses are:

| Restored body | iOS Simulator evidence | iOS Device evidence |
| --- | --- | --- |
| `ONSDecimal.asVariableLengthInteger()` | `0x1c08cdf20` | `0x180c9f6d4` |
| `ONSDecimal.copyVariableLengthInteger(_:)` | `0x1c08ce05c` | `0x180c9f82c` |
| `ONSDecimal._integerCompare(lhs:rhs:)` | `0x1c08cf5cc` | `0x18119c1d0` |
| `ONSDecimal._integerAdd(lhs:rhs:maxResultLength:)` | `0x1c08cf2d4` | `0x180cc42b8` |
| `ONSDecimal._integerSubtract(term:subtrahend:maxResultLength:)` | `0x1c08cfd84` | `0x18119c454` |
| `ONSDecimal._integerMultiply(lhs:rhs:maxResultLength:)` | `0x1c08ce208` | `0x180e59da8` |
| `ONSDecimal._integerMultiplyByShort(lhs:mulplicand:maxResultLength:)` | `0x1c08ce4d8` | `0x18119c010` |
| `ONSDecimal._integerDivideByShort(_:_:)` | `0x1c08cf6c8` | `0x180ed3c6c` |
| `ONSDecimal._integerAddShort(_:rhs:maxResultLength:)` | `0x1c08cf87c` | `0x18119c2d4` |
| `ONSDecimal._integerMultiplyByPowerOfTen(lhs:power:maxResultLength:)` | `0x1c08ced24` | `0x180e5a080` |
| `ONSDecimal._fitMantissa(_:roundingMode:)` | `0x1c08cf9f0` | `0x180ebfdd8` |
| `ONSDecimal._integerDivide(dividend:divisor:maxResultLength:)` | `0x1c08ce694` | `0x180ebf090` |
| `ONSDecimal._divide(by: UInt16)` | `0x1c08cba50` | `0x180c9f494` |
| `ONSDecimal._multiply(byShort:)` | `0x1c08cbc94` | `0x180c9ec68` |
| `ONSDecimal._add(UInt16)` | `0x1c08cbff4` | `0x180c9efec` |
| `ONSDecimal._round(scale:roundingMode:)` | `0x1c08cd000` | `0x18119b668` |
| `ONSDecimal._normalize(a:b:roundingMode:)` | `0x1c08cef54` | `0x180cc3f40` |
| `ONSDecimal._multiply(by:roundingMode:)` | `0x1c08cc898` | `0x18119aef8` |
| `ONSDecimal._divide(by: ONSDecimal, roundingMode:)` | `0x1c08d0038` | `0x180ebdf74` |
| `ONSDecimal._multiplyByPowerOfTen(power:roundingMode:)` | `0x1c08d0778` | `0x180e5981c` |
| `_ONSDecimalCompact(_:)` | `0x1c08c1388` | `0x180c9e7f0` |
| `_ONSDecimalCompare (Swift body)` | `0x1c08c11c4` | `0x180ec05bc` |
| `_ONSDecimalRound (Swift body)` | `0x1c08be858` | `0x18118ed48` |
| `_ONSDecimalNormalize (Swift body)` | `0x1c08c1920` | `0x1811910a8` |
| `ONSDecimalAdd (Swift body)` | `0x1c08bf4cc` | `0x180c988dc` |
| `ONSDecimalSubtract (Swift body)` | `0x1c08bfd24` | `0x18118faa4` |
| `ONSDecimalMultiply (Swift body)` | `0x1c08c059c` | `0x18119031c` |
| `_ONSDecimalDivide (Swift body)` | `0x1c08c16a0` | `0x180ec038c` |
| `_ONSDecimalMultiplyByPowerOf10 (Swift body)` | `0x1c08c17e0` | `0x180e59950` |
| `ONSDecimalPower (Swift body)` | `0x1c08c081c` | `0x1811905a4` |
| `ONSDecimal.subscript(UInt32).getter` (inlined; containing primitive) | `0x1c08cbda4` | `0x180c9ec68` |
| `ONSDecimal.subscript(UInt32).setter` | `0x1c08c59c4` | `0x180dab138` |
| `ONSDecimal.init(UInt64)` | `0x1c08c87d4` | `0x181198110` |

The mantissa getter has no standalone emitted native function in either image.
All eight selections and its invalid-index fatal path are read in the complete
short-multiply and short-add bodies. Its simulator selection anchor is
`0x1c08cbda4`, inside `0x1c08cbc94`; the corresponding device containing primitive
is `0x180c9ec68`. The setter is emitted at the tabled address. A fabricated getter
symbol or an ordinary helper body is not substituted for that evidence.

## Compiler support and device limits

The private error enum metadata is at simulator `0x2227e5a90` and device
`0x1e8ea1870`. Its Error-conformance accessor, metadata accessor, Hashable to
Equatable bridge and Equatable accessor are respectively simulator
`0x1c08c1660`/`0x1c08cb9fc`/`0x1c08cba0c`/`0x1c08cba10`, independently read at
device `0x181191054`/`0x18119ae90`/`0x18119aea0`/`0x18119aea4`.

The simulator witness table at `0x2227e5a10` refers to seven pooled function
bodies: `0x1c06e93b0`, `0x1c07d520c`, `0x1c0838910`, `0x1c08389a0`,
`0x1c07d5358`, `0x1c04cf210`, `0x1c07d5360`. They implement no-op destruction,
byte copy, single-payload tag access/storage and enum tag access/storage.
The native linker names some shared bodies after Calendar, AttributedString or
NSBundle; that is not a dependency on those object subsystems. The compiler
supplies the callbacks from the actual three-case enum declaration. They are
attached to that declaration's ledger row rather than counted as fabricated
source functions.

The device table at `0x1e8ea17f0` has independently read sizes and encoded pointer
words, but its seven callback target bindings have not been decoded. Those
addresses remain explicitly unread. The source compiles only the admitted
simulator target/match pair; device execution and device equivalence are not
claimed. Arithmetic bodies otherwise independently agree in conditions, values,
errors and widths. The arm64e image adds authenticated call/return handling and
uses different import encodings; definition-local device comments record that
compilation difference.

## Established native defects

NSDecimalCopy at simulator `0x1c04f90d0` and device `0x180c9dab8` trusts the
four-bit length and transfers 9–15 words past the eight-word mantissa for malformed
input. The replacement preserves the header but bounds the copy to eight words.
A replacement-only guarded test checks that a length-15 input cannot overwrite
the following destination storage. The destructive native case is never executed.

NSDecimalPower's signed Int.min negation is `NEGS` plus an overflow branch:
simulator `0x1c08c0bb4` traps at `0x1c08c102c`; device `0x1811909c0` traps at
`0x181190e90`. After the native NaN, zero-exponent and finite-zero paths, the
replacement returns overflow and canonical NaN for this single exponent. The
corresponding replacement-only test does not call the destructive oracle case.
Other checked native arithmetic traps and well-defined unusual behavior are
preserved. No general error-success stub or fitted result is used.

## Verification and integration

The versioned differential suite is `Tests/Foundation/Decimal/DecimalTests.c`;
the isolated suite is `StandaloneTests/Foundation/Decimal/DecimalTests.c`.
Native and replacement symbols are resolved independently, with owning-image
checks, native UUID checks and a rejection of identical function addresses.
Setup failures return 77, behavioral differences 1 and success 0.

The public differential corpus uses 81 structured decimal fixtures across every
length 0–8 and exponents -128, -127, -32, -1, 0, 1, 32, 126, 127. It exercises
sign/compact/reserved fields, inactive words, self-copy, all pairwise comparisons,
four rounding modes, ten scales, two Round output alias modes, two Normalize
alias modes, three binary output alias modes, six signed-bit-pattern powers and
nine powers of ten. Each run reports exactly 32,724 byte/error comparisons across
all eleven APIs. Malformed lengths and destructive Int.min power/scale probes
are excluded from the oracle corpus.

A focused matrix built the complete C/Swift group in Debug and Release with
OF_SYSTEM_NAMES NO and YES. Both NO runs passed all 32,724 comparisons. All four
isolated runs passed 45 assertions and 12,288 repeated Swift error paths with
exactly zero growth in live allocation count or bytes after warmup. The isolated
executables check before and after the suite that native Foundation and
CoreFoundation are not loaded. All runs used a fresh private pinned simulator.
Export checks found exactly eleven public entries; load-command checks found
only libSystem and the read Swift dependencies. Clang static analysis was clean.

The instrumented Debug NO build additionally passed the same 32,724 differential
comparisons and 44 isolated assertions, including all 12,288 error paths, under
AddressSanitizer. ASan quarantine retains freed blocks in malloc-zone statistics;
only the allocator-stat assertion is disabled under ASan, with the quarantine
kept enabled. The ordinary matrix provides the exact allocation-growth check.
No ASan access error was reported.

Auxiliary private-helper probes under `Local/Evidence/ns-decimal-integer`
validated 44,000 unoptimized and 44,029 optimized exact native helper results or
throw outcomes, plus eleven reverse-copy/header/overflow fixtures. Their private
Swift resolver additionally pins the owning image and recorded leading opcode
bytes before calling the read raw ABI. A Swift 6.4 compiler crash in optimizing
the probe's unsafeBitCast thin calls was avoided by linking the optimized
production kernel with an unoptimized probe driver. The arithmetic kernel itself
compiled optimized. These auxiliary probes are separate from the versioned
public suite and do not inflate its reported count.

The focused driver and logs are unversioned at
`Local/Scratch/ns-decimal/VerifyFocused.py` and
`Local/Evidence/ns-decimal/{FocusedMatrix,ASan}.log`. The coordinator owns shared
Make/Xcode Swift compilation, declaration registry, umbrella/header packaging,
test-main integration and the complete framework matrix. Focused validation does
not substitute for those integration checks. No device performance or behavior
claim is made.

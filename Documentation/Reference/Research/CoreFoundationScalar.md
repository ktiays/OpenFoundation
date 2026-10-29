# CoreFoundation Gregorian scalar validation

## Identity and scope

This reading restores only `CFGregorianDateIsValid` and its private
`daysInMonth` table. It does not restore `CFDate` objects, time zones, calendar
conversion, clock state, allocator/type registration, or Foundation date classes.
`Source/CoreFoundation/Date` is an inferred subsystem directory; an original
source path has not been recovered.

| Reference | Image identity | Function | Private data |
| --- | --- | --- | --- |
| iOS Simulator 27.0 (24A434), arm64 | CoreFoundation `D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC`; SHA-256 `e3598ca3b1ed3649c1ff8aadce058e98aa2d7be5385036a5ebb6ab986a5cc829` | `CFGregorianDateIsValid` `0x1c018dca8` | `daysInMonth` `0x1c0373524` |
| iOS Device 27.0.1 (24A446), arm64e | CoreFoundation `9426FC9A-B36C-3096-B836-D17D6744A75B`; SHA-256 `ba22deb3759f56d32e5847fec0a5e675837e189d9a000b87ffdefe88a7942166` | `CFGregorianDateIsValid` `0x1807427bc` | `daysInMonth` `0x18082f754` |

All implementation conclusions below have **verified** confidence from IDA MCP
instruction and data readings. The coordinator read the iOS Device image on an
APFS scratch clone under `Local/Scratch/cf-runtime-reference/iphone-device`;
the base track read the iOS Simulator image on its clone under
`Local/Scratch/cf-base/iphone-simulator`. Captured MCP evidence is
`Local/Evidence/cf-runtime-reference/iphone-device-CoreFoundation-reading.json`
and `Local/Evidence/cf-runtime-reference/iphone-simulator-CoreFoundation-reading.json`.
Canonical images/databases stay under `Local/Databases`. The endpoint returned
`session_id` rather than `database_id`; database access was serialized by its
owner, and each script checked its expected input path before reading.

The SDK supplies declaration spelling and the unsigned-long flags interface.
The mixed aggregate register ABI and accesses independently establish the date
layout: signed 32-bit year at offset 0; signed bytes month/day/hour/minute at
4/5/6/7; double second at 8; total size 16 and alignment 8. The two aggregate
words arrive in X0/X1, flags in X2, and the result is 0 or 1 in W0. The
replacement's return is `uint8_t`, preserving the native Boolean boundary
without publishing the unclassified `Boolean` name.

The native source compilation language is unestablished. C linkage is not used
as evidence that the framework was compiled as C. This reachable unit has only
raw scalar operations and no Objective-C/C++/Swift runtime or ownership
boundary; C11 is this project's implementation setting for that unit. No ARC
or MRC policy is inferred for any other unit.

## Native implementation

Checks occur in year, month, day, hour, minute, second order. Bit 0 rejects
years below 1; bit 1 accepts month bytes 1 through 12; bit 2 accepts day bytes
1 through 31; bit 3 accepts hour bytes 0 through 23; bit 4 accepts minute bytes
0 through 59. The unsigned byte checks reject negative signed-byte inputs.
Only the six low flag bits are consulted; higher bits have no effect.

With bit 5, native instructions compare second with 0.0 and branch `B.MI`,
then compare with exact double 60.0 (`0x404e000000000000`) and branch `B.GE`.
Both signed zeros pass; negative finite values and infinities fail; positive
finite values below 60 pass. **Unordered NaNs pass both branches.** The
Hex-Rays text incorrectly renders this as a conjunction that rejects NaNs;
the implementation follows the instructions. A signaling NaN sets the invalid
floating-point exception flag; a quiet NaN does not. Selected integer failures
return before either floating comparison.

Actual month lengths are checked only when `(unitFlags & 7) == 7`, after the
selected time fields. A day-only or month/day-only check permits a nominal day
31 in April. Otherwise the native code subtracts 2001 in 32 bits, sign-extends,
adds 1 in 64 bits, takes signed remainder modulo 400, and takes the remainder's
absolute value. Divisibility by 4 adds a February day except for remainders
100, 200, and 300. All three flags already require a positive year, so the
32-bit subtraction cannot overflow for a reachable input.

The simulator IDA data item is `_BYTE[16]`, with the next named object
`daysBeforeMonth` at +16. Both images contain exactly
`00 1f 1c 1f 1e 1f 1e 1f 1f 1e 1f 1e 1f 00 00 00`.
The native load indexes the low month nibble and adds the leap byte only for
month 2, then compares with the signed day byte. The replacement preserves the
private native data name and sixteen-byte contents.

## Dependencies and unreachable code

The table is the sole reachable dependency. There are no reachable external
calls, callbacks, allocations, object ownership operations, errors, exceptions,
or thread-local storage accesses. Floating-point comparisons preserve existing
exception flags and rounding mode while possibly adding the signaling-NaN
invalid flag. There is no CF/NS object boundary or bridging claim.

The month logger is reached only after the three low flags are all present and
only if the month range check fails. The same input has already passed the
identical enabled month check, so this edge is impossible for every public
input. The later `month >= 16` bounds trap is likewise unreachable because the
month is already 1 through 12. The simulator logger site at `0x1c018dd6c` and
cold helper `0x1c0307e4c`, and device cold helper `0x1807bc0ac`, were read to
establish that boundary. Their nested logging helpers and logging state remain
unread and unbuilt. They are not replaced by stubs or added to the restored
ledger. Omitting a proven unreachable edge does not create a partial reachable
dependency implementation. No native defect or intentional behavioral divergence
was identified in this group.

`OCFGregorianDate` and `OCFGregorianDateIsValid` map to their native CF names
through the declaration-derived naming switch. The header imports the verified
base `OCFOptionFlags` declaration. The restored scalar subset declares no CFDate
objects, time zone objects, or extra flag enums.

## iOS Device comparison

The independently read iOS Device function has the same reachable checks,
constants, date layout, table contents, NaN branches and leap-year arithmetic.
It adds arm64e pointer authentication to the ordinary function prologue/return.
Its unreachable logging enable check is outlined into its cold helper rather
than emitted at the simulator call site. The definition records this code
variation as an `iOS Device divergence:` comment. No reachable raw-value
behavior difference was found. No iOS Device build or execution is claimed.

## Verification and limits

The standalone suite has 68 fixed cases, plus compile-time field-offset, size,
alignment and flag-width checks. It covers ordinary field bounds, omitted
fields, calendar-flag interaction, leap centuries, extreme years, exact seconds
bit patterns, both NaN signs, early exits before signaling-NaN comparisons, and
floating exception state. Standalone execution checks that system Foundation
and CoreFoundation are absent.

The differential suite independently resolves the native and replacement
functions, verifies image ownership and native UUID through the shared helpers,
rejects identical addresses, and independently constructs raw-value fixtures.
It compares exact result bytes, floating exception masks and unchanged rounding
mode. The corpus includes the 68 fixtures, all 256 representations of each byte
field across all 64 low-flag combinations with and without high bits, a complete
400-year Gregorian cycle at month-end boundaries, thirteen boundary years,
4,096 deterministic raw aggregates, and four rounding modes with preexisting
floating exceptions: 155,156 differential cases in total. These checks cover
default denormal handling. Alternate flush-to-zero control and enabled hardware
floating traps remain untested; complete floating-environment equivalence is
not claimed.

Targeted scalar builds and execution passed Debug/Release and both naming modes
for the 68 standalone cases, and Debug/Release prefixed mode for all 155,156
differential cases, on a fresh pinned simulator. The shared full Make/Xcode
matrix is coordinated separately. The standalone entry point is
`runCoreFoundationScalarStandaloneTests`; the differential entry point is
`runCoreFoundationScalarDifferentialTests`. The temporary scalar harness lives under
`Local/Scratch/cf-scalar`, with products under
`.build/Scalar/iphonesimulator-iphonesimulator-<configuration>-<naming-mode>`;
its validation record is `Local/Evidence/cf-scalar/validation.json`. The shared
`make test validate`, prefixed `make coexistence`, and Debug/Release Make/Xcode
matrix must pass after integration before acceptance.
These cases establish finite coverage of one scalar API, never Date-family or
framework replacement equivalence. There are no lifetime changes or performance
claims in this group.

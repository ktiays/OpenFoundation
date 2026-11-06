<!--
  Created by ktiays on 2026/11/03.
  Copyright (c) 2026 ktiays. All rights reserved.
-->

# Foundation number and value reading

This reading covers the 134 captured `FN-number-value` entries only. It does not establish that NSNumber,
NSValue, their class clusters, or the consumers' scope are complete. The coordinator owns the scope and
restoration ledgers; this branch supplies the Number/Value translation and integration declarations.

## Reference and evidence

- Foundation, iOS Simulator 27.0 (24A434), arm64, UUID
  `49FCDA5F-A93E-3259-857C-FC75AC2C7B03`, SHA-256
  `bc7e0a5f693ffc75ae7c3311b817716e86d149fda2f8c45669a976727322cb18`.
- Original IDA capture: `Local/Evidence/cf-core/iphone-simulator-foundation-link.json` in the main checkout.
  The assembly governs the translated branches, widths, constants, ordering, and ownership.
- Supplemental IDA reading on 2026-11-03, using an APFS clone at
  `Local/Scratch/cf-core-fd-number/audit-simulator/Foundation.i64` in the main checkout. The database's
  input SHA-256 was checked. The canonical database was not opened or edited. The owned clone was closed.
- `Local/Evidence/cf-core-fd-number/simulator-exception-regions.json` records the 11 native LSDA records
  and seven handler ranges missing from the original capture. It was read through IDA, including
  `__unwind_info` at `0x1c14aaebc` and `__gcc_except_tab` at `0x1c149fb1c`.
- `Local/Evidence/cf-core-fd-number/simulator-class-data.json` records the class/ivar bytes, nine cached
  type records, and the two static placeholder records, read through IDA.
- Native linkage was checked with `xcrun nm -m` on the pinned image; the retained output is
  `Local/Scratch/cf-core-fd-number/foundation-nm.txt` in the main checkout.
- iOS Device 27.0.1 (24A446), arm64e, Foundation UUID `C041F1D0-5EF1-30A3-BF96-830976173B56`:
  **unread for this family**. No iOS Device matching or behavioral equivalence is claimed.

The source organization is inferred from native address neighborhoods. `ONSValue.m` holds the contiguous
value/number unit, including its helpers and placeholder classes. The three separate category files follow
their native category neighborhoods. This does not assert an unrecovered Apple source filename.

## Class data and ownership

NSValue (`0x22e6de478`, class_ro `0x222862a50`) and NSNumber (`0x22e6de400`, class_ro `0x222862ae0`)
have instance start/size 8 and no ivars. Their native superclass boundaries are NSObject and NSValue.
NSObject is the supplied libobjc class; its pointer is outside the extracted Foundation image.

NSPlaceholderValue (`0x22e6de568`, class_ro `0x222862b98`) derives from NSNumber. It has instance start 8,
size 16, and exactly one eight-byte ivar: `zoneForInstance`, `^{_NSZone=}`, offset 8, offset variable
`0x22e6d4e00`. NSPlaceholderNumber (`0x22e6de518`, class_ro `0x222862c28`) derives from it, has size 16,
and adds no ivars. The static placeholders at `0x2227d7ad8` and `0x2227d7ac8` contain their class and a
null zone. There are **no unread ivars in the four defined classes**. Their zero/nil/no-op lifetime and
initialization methods below are native bodies, not substitution stubs.

The protocol-list references in NSValue/NSNumber's class_ro records (`0x23d88fcf9` and `0x23d890469`)
lie outside this extracted image and remain unread. No guessed protocol-conformance objects are emitted.
Both placeholders' base-protocol pointers are null. Protocol metadata remains an integration boundary.

NSConcreteValue is not defined by this family capture. Its metadata was read to establish the native offset
variables: `_specialFlags`, `Q`, offset 8 (`0x22e6d4dfc`), and `typeInfo`, `^v`, offset 16
(`0x22e6d4df8`), instance size 24. The implementation loads those variables rather than inventing a
concrete subclass. The foreign interface has no ivars, and both offset symbols remain undefined until the
coordinator supplies the concrete class. NSWeakObjectValue and NSDecimalNumber methods also remain outside
this capture. The send-only declarations do not restore those classes.

## Exception behavior recovered beyond the original capture

The original capture stopped at stack-check failure calls in five methods and omitted the handlers below.
The LSDA establishes that only the sizing call is caught; a throwing `objCType` or getter is not swallowed.

| Parent | Sizing handler | Result |
| --- | --- | --- |
| `-[NSValue copyWithZone:]`, `0x1c05f9194` | `0x1c05f9278` | nil |
| `-[NSValue isEqualToValue:]`, `0x1c05f9290` | `0x1c05f93e0` | NO |
| `-[NSValue hash]`, `0x1c05f9464` | `0x1c05f9580` | 0 |
| `-[NSValue description]`, `0x1c05f9598` | `0x1c05f963c` | nil |
| `-[NSNumber copyWithZone:]`, `0x1c05fd250` | `0x1c05fd388` | nil |

All sizing handlers except `matchTypeEncoding` have a null catch-all type entry and use `@catch (...)`,
including foreign exceptions. `matchTypeEncoding`, `0x1c05f7eb8`, catches its typed exception only: its
landing pads at `0x1c05f7fb8`/`0x1c05f7fbc` compare the type selector with 1. Its type entry resolves
indirectly through GOT `0x22227fba0`, bound to NSException with medium confidence; NSException is the image's
only imported class exception-type symbol. The other imported exception type is libobjc's `id` type.

The encoder cleanup/termination tails at `0x1c05f9e90-0x1c05f9eaf` and
`0x1c05fd230-0x1c05fd24f` are compiler unwinding code for those catch regions, not additional restored
functions. Exception-helper declarations are non-returning because both native request helpers end in
`objc_exception_throw`, at `0x1c0fe44b8` and `0x1c0fe4528`.

## Numeric and value details

- Tagged number encoders preserve the native tag constants, the two-stage XOR/permutation operation, the
  signed integer interval, and the negative-zero exclusion. The fallback calls libobjc's `objc_allocWithZone`
  directly before the matching initializer and autorelease. Boolean constructors use the restored CF constants.
- Floating-to-integer operations use always-inlined spellings of the captured ARM64 FCVTZS/FCVTZU
  instructions. This preserves conversion width, saturation, NaNs, and FPSR side effects without undefined
  out-of-range C casts. These are expressions within the native functions, not new native function boundaries.
- Accessors keep the type-driven sends, including delegation back to the same abstract accessor when the
  matching concrete getter is not overridden (kept). Integer narrowing and sign extension follow the capture.
- `compare:` keeps signed/unsigned 64-bit dispatch and the native NaN ordering by the other value's sign
  (kept). `hash` keeps the integer paths and the ordered floor/truncation/fused-subtraction calculation.
- Formatting keeps the native format strings, precision, multiply order, and conversion width. In particular,
  an unsigned-char value is scaled through signed FCVTZS W, and printf applies the short length modifiers
  to the 32-bit conversion results (kept). The explicit short casts satisfy the compiler's format checking.
- Non-keyed number archives widen one-character `l`/`L` encodings to `q`/`Q`; decoding maps `l`/`L` to
  `i`/`I` before sizing (kept). Keyed field order and the different exact-class tests are preserved.
- The size word reused as the first stack-buffer word is retained. Raw byte loads/stores use `memcpy` so
  they preserve the native unaligned access without imposing C effective-type/alignment assumptions.
- The nine type-information records preserve their size, encoding, and special flags. Geometry/range
  conversions, cache lock order, and indexed-ivar copying follow the capture.

Hex-Rays disagreements followed at the source: `compare:` loses the greater-than and NaN handling in its
pseudocode, but the assembly contains both; the floating constructor pseudocode misrepresents the S-register
float argument as a reinterpreted double; exception-handler tails and catch types were not represented
completely by the original pseudocode/capture. None of those pseudocode artifacts supplies behavior.

## Recorded Foundation defects and divergences

Confidence is verified against the Simulator assembly/metadata for each item. These exceptions to literal
restoration are documented at their source definitions. They have not received post-link lifetime testing.

| Native definition | Established defect | Restoration divergence |
| --- | --- | --- |
| `__NSNewValue`, `0x1c05f77f4`; `-[NSValue isEqual:]`, `0x1c05f93f4` | Unsynchronized cache load/store at `0x1c05f7cc4`/`0x1c05f7cd8` and read at `0x1c05f9420` | Relaxed atomics on pointer-width storage; symbol size/alignment stay eight bytes. Future readers must also be atomic. |
| `-[NSNumber initWithCoder:]`, `0x1c05fcaf4` | A second receiver release at `0x1c05fcbc8`, after `0x1c05fcb20` | Release the receiver once. Never probe the native over-release. |
| `newDecodedValue`, `0x1c05f9ef4` | Decoded type string leaks on empty string (`0x1c05fa014`), failed allocation (`0x1c05fa054`), and sizing exception (`0x1c05fa3f4`) | Free the encoding on these exits. |
| `newDecodedNumber`, `0x1c05fccd4` | Type string leaks through sizing handler `0x1c05fce44` | Free the encoding on that exit. |
| `-[NSNumber copyWithZone:]`, `0x1c05fd250` | Unsupported type branches to `0x1c05fd334` without freeing the heap buffer | Free the heap buffer before constructing the exception. |
| `-[NSNumber _getCString:length:multiplier:]`, `0x1c05fd510` | Signed capacity is widened to an enormous size_t for negative length, e.g. `0x1c05fd57c`, permitting an out-of-bounds write | Pass zero snprintf capacity for negative lengths while keeping getter calls and the signed result comparison. Never probe the native write. |

## Confidence and open boundaries

Unless specified otherwise, the Simulator instruction reading is verified. The following bindings are medium
confidence because shared-cache GOT/selector storage lies outside the extracted image:

- `0x222280b50`, `0x222280b70`, `0x222280b80`, `0x222280b88`: allocator default/system default and
  Boolean false/true respectively, from import order and consistent native uses.
- `0x22227fae0`, `0x22227fb20`: NSException and NSMutableData classes.
- `0x22227f500`, `0x22227f4f8`: NSInvalidArgumentException and NSInternalInconsistencyException.
- `0x222409408`, `0x222409418`, `0x222409420`: libobjc tag permutation, mask, and obfuscator.
- `0x2387c85eb`, `0x2387aef89`: `getValue:size:` and `getValue:`, from the two reciprocal dispatch paths.
- `0x22227fba0`: the typed NSException catch described above. NSObject's external superclass boundary is
  identified through the native class import and SDK hierarchy; its data is not supplied by this image.

`ONSUnrestoredInterfaces.h` is the only integration-declaration header. It declares missing owner interfaces,
not implementations. Calls to restored CF operations use their OCF names; no operation forwards to system
Foundation/CoreFoundation. Libobjc, libSystem allocation/locks, and libm remain supplied external boundaries.
No new high-level framework dependency or port TARGET is admitted.

Before linking, the coordinator must supply the missing CF declarations/definitions and other families,
the three uncaptured concrete class boundaries, scanner decimal parsing and the NSError corrupt-error factory,
resolve protocol metadata, and replace the integration declarations with their owning headers. The compiler
currently emits weak private NSException type information for the typed catch; the CoreFoundation exception
owner should declare its interface with `__attribute__((objc_exception))` and supply the native strong
exception-type symbol to preserve that import boundary. This branch cannot edit that shared owner header.

The coordinator must also replace the ONSValue/ONSNumber placeholders in
`Source/CoreFoundation/Objective-C/OCFBridgedClasses.h` with these owner headers, register the source units
and this reading in the shared ledgers, and wire public header installation. No shared registry is edited here.

## Verification boundary

The handoff explicitly selects compile-and-symbol acceptance before the five families' first link.
The acceptance command and its full output are retained in the main checkout under
`Local/Scratch/cf-core-fd-number/`. It compiles both products in both naming modes and checks citations,
formatting, prefixes, duplicate declarations/definitions, allowed undefined symbols, ownership and commits.
The two explicit undefined ivar offsets are justified in `allow-undefined.txt`; other unresolved boundaries
are declared in `ONSUnrestoredInterfaces.h`. No CoreFoundation runtime-name conversions were required.

Clang static analysis is run on the four owned implementations. Framework linking, Make/Xcode framework
build matrices, Simulator traces, ASan runtime checks, allocation/leak checks, and iOS Device comparisons
remain for integration; compile success is not behavioral coverage. No destructive native defect is probed.

## Captured entries by implementation file

All addresses below refer to the Simulator Foundation image and UUID pinned above. Each definition also
carries its native address in source. Supplemental handlers are folded into the named parent methods.

### ONSNumberCFAdditions.m (4 entries)

- `0x1c04d1d5c` `-[NSNumber(NSCFAdditions) _cfTypeID]`
- `0x1c04d1d60` `-[NSNumber(NSCFAdditions) _cfNumberType]`
- `0x1c04d1e80` `-[NSNumber(NSCFAdditions) _getValue:forType:]`
- `0x1c04d1f40` `-[NSNumber(NSCFAdditions) _reverseCompare:]`

### ONSNumberDecimalNumberExtensions.m (1 entries)

- `0x1c04fb228` `-[NSNumber(NSDecimalNumberExtensions) decimalValue]`

### ONSValue.m (128 entries)

- `0x1c05f77f4` `__NSNewValue`
- `0x1c05f7eb8` `_matchTypeEncoding`
- `0x1c05f8d40` `+[NSValue allocWithZone:]`
- `0x1c05f8d64` `-[NSValue init]`
- `0x1c05f8dfc` `+[NSValue value:withObjCType:]`
- `0x1c05f8e00` `+[NSValue valueWithBytes:objCType:]`
- `0x1c05f8e20` `-[NSValue getValue:]`
- `0x1c05f8f5c` `-[NSValue getValue:size:]`
- `0x1c05f90e8` `-[NSValue _matchType:size:]`
- `0x1c05f90f0` `-[NSValue _matchType:size:strict:]`
- `0x1c05f9194` `-[NSValue copyWithZone:]`
- `0x1c05f9290` `-[NSValue isEqualToValue:]`
- `0x1c05f93f4` `-[NSValue isEqual:]`
- `0x1c05f9464` `-[NSValue hash]`
- `0x1c05f9598` `-[NSValue description]`
- `0x1c05f96b4` `+[NSValue valueWithWeakObject:]`
- `0x1c05f96e4` `-[NSValue nonretainedObjectValue]`
- `0x1c05f984c` `+[NSValue valueWithPoint:]`
- `0x1c05f98ac` `+[NSValue valueWithSize:]`
- `0x1c05f990c` `+[NSValue valueWithRect:]`
- `0x1c05f9970` `+[NSValue valueWithRange:]`
- `0x1c05f9a34` `-[NSValue pointValue]`
- `0x1c05f9ab8` `-[NSValue sizeValue]`
- `0x1c05f9b3c` `-[NSValue rectValue]`
- `0x1c05f9bd0` `-[NSValue rangeValue]`
- `0x1c05f9cec` `-[NSValue classForCoder]`
- `0x1c05f9cf8` `-[NSValue encodeWithCoder:]`
- `0x1c05f9eb0` `+[NSValue supportsSecureCoding]`
- `0x1c05f9eb8` `-[NSValue initWithCoder:]`
- `0x1c05f9ef4` `_newDecodedValue`
- `0x1c05fa40c` `+[NSNumber initialize]`
- `0x1c05fa458` `+[NSNumber allocWithZone:]`
- `0x1c05fa480` `+[NSNumber numberWithChar:]`
- `0x1c05fa510` `+[NSNumber numberWithUnsignedChar:]`
- `0x1c05fa5b0` `+[NSNumber numberWithShort:]`
- `0x1c05fa650` `+[NSNumber numberWithUnsignedShort:]`
- `0x1c05fa6f0` `+[NSNumber numberWithInt:]`
- `0x1c05fa790` `+[NSNumber numberWithUnsignedInt:]`
- `0x1c05fa830` `+[NSNumber numberWithInteger:]`
- `0x1c05fa8d0` `+[NSNumber numberWithUnsignedInteger:]`
- `0x1c05fa970` `+[NSNumber numberWithLong:]`
- `0x1c05faa10` `+[NSNumber numberWithUnsignedLong:]`
- `0x1c05faab0` `+[NSNumber numberWithLongLong:]`
- `0x1c05fab50` `+[NSNumber numberWithUnsignedLongLong:]`
- `0x1c05fabe8` `+[NSNumber numberWithFloat:]`
- `0x1c05faca8` `+[NSNumber numberWithDouble:]`
- `0x1c05fad6c` `+[NSNumber numberWithBool:]`
- `0x1c05fad8c` `-[NSNumber boolValue]`
- `0x1c05fae94` `-[NSNumber integerValue]`
- `0x1c05fae98` `-[NSNumber unsignedIntegerValue]`
- `0x1c05fae9c` `-[NSNumber charValue]`
- `0x1c05fb058` `-[NSNumber unsignedCharValue]`
- `0x1c05fb214` `-[NSNumber shortValue]`
- `0x1c05fb3d0` `-[NSNumber unsignedShortValue]`
- `0x1c05fb58c` `-[NSNumber intValue]`
- `0x1c05fb750` `-[NSNumber unsignedIntValue]`
- `0x1c05fb914` `-[NSNumber longValue]`
- `0x1c05fbaf8` `-[NSNumber unsignedLongValue]`
- `0x1c05fbcdc` `-[NSNumber doubleValue]`
- `0x1c05fbea0` `-[NSNumber floatValue]`
- `0x1c05fc064` `-[NSNumber longLongValue]`
- `0x1c05fc248` `-[NSNumber unsignedLongLongValue]`
- `0x1c05fc42c` `-[NSNumber compare:]`
- `0x1c05fc5e0` `-[NSNumber isEqualToNumber:]`
- `0x1c05fc60c` `-[NSNumber isEqual:]`
- `0x1c05fc658` `-[NSNumber hash]`
- `0x1c05fc7c4` `-[NSNumber descriptionWithLocale:]`
- `0x1c05fcac0` `-[NSNumber description]`
- `0x1c05fcac8` `-[NSNumber redactedDescription]`
- `0x1c05fcad0` `-[NSNumber stringValue]`
- `0x1c05fcad8` `-[NSNumber classForCoder]`
- `0x1c05fcae4` `-[NSNumber _allowsDirectEncoding]`
- `0x1c05fcaec` `+[NSNumber supportsSecureCoding]`
- `0x1c05fcaf4` `-[NSNumber initWithCoder:]`
- `0x1c05fccd4` `_newDecodedNumber`
- `0x1c05fce8c` `-[NSNumber encodeWithCoder:]`
- `0x1c05fd250` `-[NSNumber copyWithZone:]`
- `0x1c05fd3a0` `_newNumber`
- `0x1c05fd510` `-[NSNumber _getCString:length:multiplier:]`
- `0x1c05fd814` `-[NSPlaceholderValue retain]`
- `0x1c05fd818` `-[NSPlaceholderValue retainCount]`
- `0x1c05fd820` `-[NSPlaceholderValue release]`
- `0x1c05fd834` `-[NSPlaceholderValue dealloc]`
- `0x1c05fd838` `-[NSPlaceholderValue init]`
- `0x1c05fd840` `-[NSPlaceholderValue initWithBytes:objCType:]`
- `0x1c05fd854` `+[NSPlaceholderValue supportsSecureCoding]`
- `0x1c05fd85c` `-[NSPlaceholderValue initWithCoder:]`
- `0x1c05fd870` `-[NSPlaceholderNumber init]`
- `0x1c05fd878` `-[NSPlaceholderNumber initWithChar:]`
- `0x1c05fd8d8` `-[NSPlaceholderNumber initWithUnsignedChar:]`
- `0x1c05fd938` `-[NSPlaceholderNumber initWithShort:]`
- `0x1c05fd998` `-[NSPlaceholderNumber initWithUnsignedShort:]`
- `0x1c05fd9f8` `-[NSPlaceholderNumber initWithInt:]`
- `0x1c05fda58` `-[NSPlaceholderNumber initWithUnsignedInt:]`
- `0x1c05fdabc` `-[NSPlaceholderNumber initWithInteger:]`
- `0x1c05fdb1c` `-[NSPlaceholderNumber initWithUnsignedInteger:]`
- `0x1c05fdb9c` `-[NSPlaceholderNumber initWithLong:]`
- `0x1c05fdbfc` `-[NSPlaceholderNumber initWithUnsignedLong:]`
- `0x1c05fdc7c` `-[NSPlaceholderNumber initWithLongLong:]`
- `0x1c05fdcdc` `-[NSPlaceholderNumber initWithUnsignedLongLong:]`
- `0x1c05fdd5c` `-[NSPlaceholderNumber initWithFloat:]`
- `0x1c05fddbc` `-[NSPlaceholderNumber initWithDouble:]`
- `0x1c05fde1c` `-[NSPlaceholderNumber initWithBool:]`
- `0x1c05fde3c` `+[NSPlaceholderNumber supportsSecureCoding]`
- `0x1c05fde44` `-[NSPlaceholderNumber initWithCoder:]`
- `0x1c05fdfec` `_cachedObjCTypeInfoHash`
- `0x1c05fe018` `_cachedObjCTypeInfoIsEqual`
- `0x1c05fe03c` `_mungeUnionsAndStructures`
- `0x1c0fe7098` `-[NSValue objCType]`
- `0x1c0fe70b8` `-[NSValue initWithBytes:objCType:]`
- `0x1c0fe70e4` `-[NSNumber initWithChar:]`
- `0x1c0fe7108` `-[NSNumber initWithUnsignedChar:]`
- `0x1c0fe712c` `-[NSNumber initWithShort:]`
- `0x1c0fe7150` `-[NSNumber initWithUnsignedShort:]`
- `0x1c0fe7174` `-[NSNumber initWithInt:]`
- `0x1c0fe7198` `-[NSNumber initWithUnsignedInt:]`
- `0x1c0fe71bc` `-[NSNumber initWithInteger:]`
- `0x1c0fe71e0` `-[NSNumber initWithUnsignedInteger:]`
- `0x1c0fe7204` `-[NSNumber initWithLong:]`
- `0x1c0fe7228` `-[NSNumber initWithUnsignedLong:]`
- `0x1c0fe724c` `-[NSNumber initWithLongLong:]`
- `0x1c0fe7270` `-[NSNumber initWithUnsignedLongLong:]`
- `0x1c0fe7294` `-[NSNumber initWithFloat:]`
- `0x1c0fe72b8` `-[NSNumber initWithDouble:]`
- `0x1c0fe72dc` `-[NSNumber initWithBool:]`
- `0x1c0fe7308` `-[NSPlaceholderValue objCType]`
- `0x1c0fe7380` `-[NSPlaceholderValue getValue:]`
- `0x1c0fe73f0` `-[NSPlaceholderValue getValue:size:]`

### ONSValuePortCoding.m (1 entries)

- `0x1c064e890` `-[NSValue(NSValuePortCoding) replacementObjectForPortCoder:]`

## Round 2. Decimal, number formatting, and concrete values

The round-2 reading uses the 24A434 arm64 Foundation capture
`Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json`, family
`FD-decimal-number-formatting`, pinned to the Foundation image above. Function addresses and
linkage are recorded above the definitions. Class layouts and linkage were checked against the same
image's Mach-O data and symbol table. The iOS Device
24A446 Foundation capture is in `Local/Scratch/cf-core-r2-fd/device-foundation-by-name.json`.
The instruction inventory matched 208 of the 212 assigned names. Of these, 88 have the same mnemonic
sequence after ignoring return authentication and no-ops, and 120 differ. This inventory leaves the
operand, control flow, and behavior comparison open.

- `ONSValue.m` adds the 12 NSConcreteValue methods and `_stringFromEdgeInsets` (0x1c05f8608).
  The class remains in the value unit to keep its calls to the local `_NSNewValue` and
  `matchTypeEncoding` definitions. The header declares both exported ivars. The geometry strings
  use 17 significant digits. The range encoder takes the two-word return and writes length before
  location. Hex-Rays loses the second return register in both range cases.
- `ONSValueGeometry.m` holds the three geometry string functions (0x1c0519ecc, 0x1c0519f64,
  0x1c0519ffc) and NSShouldRetainWithZone (0x1c0593f64), grouped by their use in NSValue.
- `ONSConstantIntegerNumber.m` adds the 29 captured methods. The class_ro_t at 0x222864e38 has
  a const char pointer at 8 and a signed 64-bit value at 16. Numeric casts use the loaded width and
  signedness. The bare RET in retain, autorelease, copy, and copyWithZone preserves X0. The
  incompatible-size exception at 0x1c0615bd0 always names type "d" (kept). The invalid encoding paths
  store the native diagnostic and execute BRK #1. The crash annotation storage uses a separate
  identifier because CoreFoundation already defines gCRAnnotations.
- `ONSWeakObjectValue.m` adds the nine captured methods. The class_ro_t at 0x222862970 identifies
  the raw identity pointer, managed weak location, and fallback flag. The raw identity is retained
  as a pointer value in hash and nonretainedObjectValue, even when the weak location clears (kept).
  The weak getters have an unread CoreFoundation dependency, _NSObjectLoadWeak at 0x1c029ee9c.
- `ONSNumberKindOfAdditions.m` adds NSNumber's isNSNumber__ at 0x1c05a19d0. The category is on the
  restored NSNumber class.
- `ONSDecimalNumberHandler.m` adds the seven captured methods. The scale is signed 16-bit and
  is followed by the mode and flags at byte 10. The encoder loads "NS.roundingmode" at 0x1c04faf84,
  which Hex-Rays omits. Foundation defect: at 0x1c04fadb0, concurrent initialization of the
  singleton can race and leak a handler. Atomic publication and release of the losing allocation
  preserve the initialized singleton.
- `ONSJSONRoundTrippingNumber.m` adds the ten captured methods. The real superclass binds to
  NSObject, while the superclass selector reports NSNumber (kept). Equality sends isNSNumber__,
  whose full selector the pseudocode obscures. The NSXPCCoder class remains an undefined
  dependency. The forwarding and method-signature methods remain unread, as does the NSObject
  category that supplies the default isNSNumber__ for nonnumeric receivers.
- The four captured _NSDecimal C bridge addresses identify existing Swift definitions.
  `Decimal+Operations.swift` adds the missing compare and multiply citations. The round and
  compact bridge addresses were already recorded. The coordinator's acceptance symbol check now
  compiles and inspects the Swift object as well as the C-family objects.

The exception class and name references 0x22227fae0, 0x22227f500, and 0x22227f4f8 use the earlier
number-unit GOT bindings (medium confidence). The JSON decoder's dictionary class at 0x22227fad0
is bound by use (medium confidence). The superclass slot 0x22e6b6088 for NSDecimalNumberHandler
and _NSJSONRoundTrippingNumber matches NSValue's NSObject slot (medium confidence). The capture
occasionally prints the function after objc_exception_throw as a continuation. The corresponding
assembly ends at the throw. The class_ro_t ivar lists for the new class definitions were readable.
The source does not add protocol-conformance metadata from the shared cache.

Supplemental reads used APFS clones of the pinned Simulator and Device IDA databases in
`Local/Scratch/cf-core-r2-fd/ida`. The captures, including the NSDecimalNumber lifetime methods
are retained next to those clones.

- `ONSDecimalNumber.m` adds the captured decimal-number class, placeholder, static subclass,
  compatibility methods, and _checkErrorAndRound. Supplemental reads establish the no-op
  retain/release on the static instances and the bytes at 0x2227d0ec0, 0x2227d0ee0, and 0x2227d0f30.
  The mantissa is a zero-sized ivar at byte 12. Clang spells the zero-length array encoding [0S]
  while the native metadata spells it ^S. Foundation defect: the native NaN singleton at
  0x1c04f9b0c races and publishes before setting the sign bit. Atomic publication follows
  initialization and releases any losing allocation. The fused accumulation at 0x1c04f9cf4,
  which Hex-Rays separates, uses an explicit fused multiply-add. The native integer conversions
  use the arm64 saturating instructions with their original destination widths. The signed and
  unsigned wide integer conversions return 9 for the NaN representation (kept).
- `ONSNumberFormatter.m` adds the 36 captured methods and the two captured C routines, together
  with the native default setter, initializer, and Keynote selector read in the supplemental
  capture. The default behavior and first-call IMP caches use atomics to fix the races at
  0x1c057671c and 0x1c0577c28. The CF property and class GOT bindings use slot order and call sites
  (medium confidence). The native pattern fallback filters tokens before retrying and restores
  the original format. The parse and format entry points in CoreFoundation remain declared.
- `ONSFormatter.m` restores the three scoped superclass coding and copy methods. Its class
  binds to NSObject via 0x22e6b6088 (medium confidence).
- `ONSRecursiveLock.m` restores the eight scoped methods and the attribute initialization block
  at 0x1c0567244. The native private record is 96 bytes at instance offset 8. The NSObject
  superclass uses the same external binding (medium confidence).
- FA owns the 15 _NSStringProxyForContext methods. This branch declares only the class symbol and
  formatter messages used by NSNumberFormatter.
- `LocaleNotifications.swift` restores the Swift generation read and one-time initializer.
  The native generation starts at 1 and reset writers use a relaxed CAS at 0x1c09c489c. The
  locale, calendar, and time zone reset group remains open.
- `ONSFormatterErrorConstants.m` defines the two bounds error keys. IDA reads their string
  objects at 0x222824010 and 0x222824030.
- The Swift importer needs an ARC-compatible cast for the existing character-set block.
  The existing Swift string bridge copies the same bytes through raw pointers when the SDK
  imports its source as UnsafePointer<CChar>.

The private Swift decimal-to-string body at 0x1c08be914 reads the native Locale separator first, then
applies the legacy dictionary keys in order, then builds the reversed decimal digits through short
division. The wrapper at 0x1c08bf398 calls the native libswiftCore object bridges. The locale adapter
references the selected Foundation module's Locale metadata, whose owner remains responsible for the
definition. The iOS Simulator value witness table at 0x2227ea070 reports size and stride 16 and
alignment eight. The native call at 0x1c08be9a8 uses witness offset 0xe0 with the class-bound object
in the Swift context register. The second dictionary key is bound to the NSLocaleDecimalSeparator
alias of kCFLocaleDecimalSeparatorKey (medium confidence for GOT 0x22227f550). The compiler-generated
helpers are cited at their Swift source causes. Framework linking and object traces remain open.

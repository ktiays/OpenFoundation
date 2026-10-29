# CoreFoundation BigNum initialization and decimal text

## Identity and scope

This reading restores the signed `_CFBigNum` initializers, `_CFBigNumToCString`
and the 128-bit decimal writer `emit128` with its two power tables. They are
below the core group of the scope: CFNumber description and the number
formatter reach them, and they call nothing but compiler-rt and libsystem. It is
not a restoration of CFNumber, the number formatter, the unsigned initializers,
BigNum arithmetic, comparison or parsing.

| Reference | Identity |
| --- | --- |
| iOS Simulator | iOS 27.0 (24A434), arm64; CoreFoundation UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; SHA-256 e3598ca3b1ed3649c1ff8aadce058e98aa2d7be5385036a5ebb6ab986a5cc829 |
| iOS Device | iOS 27.0.1 (24A446), arm64e; CoreFoundation UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; SHA-256 ba22deb3759f56d32e5847fec0a5e675837e189d9a000b87ffdefe88a7942166 |

The evidence is the IDA capture `Local/Evidence/below-core-capture/iphone-simulator.json`
and `iphone-device.json`, group `bignum`: function boundaries, complete
disassembly, Hex-Rays pseudocode, every caller with its call-site instructions,
and the bytes of each referenced datum up to the next named symbol. The
disassembly is the authority; the pseudocode located questions only. Linkage is
from `nm -m` on the canonical images. No IDA session was opened for this track.

| Native function | Linkage | iOS Simulator | iOS Device | Confidence |
| --- | --- | --- | --- | --- |
| `_CFBigNumInitWithInt8` | external | 0x1c02e4428 | 0x1807ac9a4 | verified |
| `_CFBigNumInitWithInt16` | external | 0x1c02e4450 | 0x1807ac9cc | verified |
| `_CFBigNumInitWithInt32` | external | 0x1c02e4478 | 0x180723988 | verified |
| `_CFBigNumInitWithInt64` | external | 0x1c02e44bc | 0x1806f4ca4 | verified |
| `_CFBigNumInitWithInt128` | external | 0x1c02e4534 | 0x1806f2440 | verified |
| `_CFBigNumToCString` | external | 0x1c02e51f4 | 0x1806f2c64 | verified |
| `emit128` | non-external | 0x1c01ac500 | 0x18069c78c | verified |

| Native datum | Linkage | iOS Simulator | iOS Device | Bytes | Confidence |
| --- | --- | --- | --- | --- | --- |
| `powersOf10` | non-external | 0x1c03752c0 | 0x1808314e0 | 624 | verified |
| `neg_powersOf10` | non-external | 0x1c0375530 | 0x180831750 | 624 | verified |
| `"%09u%09u%09u%09u%09u"` literal | `__cstring` | 0x1c049bc9f | 0x180acdf83 | 21 | verified |

No original source path was recovered. `emit128` lies among the CFNumber
functions (0x1c01ab..0x1c01ad), and the BigNum functions lie together near
0x1c02e4; the restoration follows that split as
`Source/CoreFoundation/Number/OCFNumber.c` and `OCFBigNumber.c`. The directory
and file names are inferred.

## Native implementation

### BigNum value layout

The initializers clear 24 bytes (`STP XZR, XZR, [X0]` and `STR XZR, [X0,#0x10]`);
every native caller clears the same 24 bytes of its stack slot before the call.

| Offset | Size | Content |
| --- | --- | --- |
| 0, 4, 8, 12, 16 | 4 each | Unsigned base-10^9 digits, least significant first |
| 20 | 1 | Sign: 0xff when negative, otherwise 0 |
| 21..23 | 3 | Zero after initialization |

A negative input sets the sign with a 32-bit read-modify-write of the word at
offset 20 (`LDR W8, [X0,#0x14]`, `ORR W8, W8, #0xFF`, `STR`). `_CFBigNumToCString`
reads only the byte (`LDRB`, `TBNZ #7`): any sign byte with bit 7 set is
negative. The restoration declares the word as an 8-bit signed bit-field
followed by a 24-bit field; a lone 8-bit field compiles to a byte store. The
offsets and stores are verified. `BigNum`, its field names and the bit-field
representation are inferred local spellings; the binary carries no type name.

### Initializers

All five clear the value, test the sign bit of the argument at its width and, if
negative, set the sign and negate the argument in two's complement.

- `Int8`/`Int16` take a sign-extended argument (the only caller emits `SXTB`/`SXTH`)
  and store the low 8 or 16 bits of the negation in digit 0. The minimum stores
  128 and 32,768.
- `Int32` negates in 32 bits, stores `n % 10^9` in digit 0 and `n / 10^9` in digit 1.
  INT32_MIN becomes 2^31.
- `Int64` negates in 64 bits (unsigned), then stores `n % 10^18 % 10^9`,
  `n % 10^18 / 10^9` and `n / 10^18` in digits 0..2.
- `Int128` receives the value in X1 (low) and X2 (high). This is the Darwin
  arm64 `__int128` assignment; it does not round to an even register pair.
  `_CFBigNumInitWithBytes` loads a high/low pair from memory and passes it this
  way. The 128-bit negation is `NEGS`/`NGC`. Four `__udivti3` calls divide by
  10^36, 10^27, 10^18 and 10^9. The 10^36 remainder subtracts the whole 128-bit
  quotient times 10^36; the 10^27 and 10^18 remainders use only the low 32 bits
  of their quotients; digit 0 is the 32-bit difference
  `remainder + quotient * (uint32_t)-10^9`. All five digits are stored after the
  last division (digits 0/1, 2/3, then 4). For valid inputs every quotient
  fits, so the truncations are exact; they are reproduced as written.

No native caller reads the result register; the restoration returns `void`.

### `_CFBigNumToCString(value, leadingZeros, forcePlus, buffer, size)`

1. If the sign byte has bit 7 set, write `-`; otherwise, if `forcePlus`, write
   `+`. Either prefix advances the buffer and subtracts one from `size`,
   unchecked: a size of 0 wraps.
2. `snprintf` into a 46-byte stack buffer with `"%09u%09u%09u%09u%09u"`, digits
   4 down to 0. Digits wider than nine characters (not produced by the
   initializers) truncate the result at 45 characters.
3. With `leadingZeros`, `memset(buffer, '0', size)`, then `memmove` the last
   `min(size, 46)` bytes of the stack buffer, its NUL included, to the end of
   `buffer`. With `size` 0 after a prefix, the fill is unbounded and the process
   faults; this is the native behavior and the tests never call it.
4. Otherwise skip leading `'0'` characters; if that reaches the NUL, step back
   one character so that zero prints as `0`. Then `strlcpy(buffer, start, size)`.

Both flags are tested as 32-bit words (`CBZ`). The only caller passes 0 and 1
and ignores the result register; the restoration takes `uint8_t` flags and
returns `void`. The function has a stack protector; it is the compiler's.

### `emit128(buffer, value, forcePlus)`

The value is a pointer to a high/low pair: the signed high word at offset 0 and
the unsigned low word at offset 8 (`LDP X8, X9, [X1]`, sign bit of X8). A
negative value is negated in 128 bits (`low == 0 ? -high : ~high`, `-low`)
and prefixed with `-`; otherwise `forcePlus` prefixes `+`. For each of 39
positions (10^38 down to 10^0) the digit counts how many times the negated power
can be added while the power is not greater than the remainder. The high words
compare signed and the low words unsigned. A zero digit is skipped while the
leading flag is set. If every position was skipped, a single `0` is written,
then a NUL. The 128-bit minimum stays negative after negation, so no power
compares less than or equal to it, and it is written as `-0`. This is
well-defined native behavior and is kept. The three callers
(`__CFNumberCreateFormattingDescription`, `__CFNumberCopyFormattingDescription`,
`__CFNumberCopyDescription`) pass 0 or 1 and ignore the result register.

`powersOf10` and `neg_powersOf10` each hold 39 sixteen-byte high/low pairs,
10^38..10^0 and their negations. Their length comes from the loop bound
(`CMP X10, #0x27`) and the 16-byte stride; `neg_powersOf10` begins exactly 624
bytes after `powersOf10`. The capture's byte spans are upper bounds.
Each entry was decoded and checked against the exact power.

## Dependencies and naming

| Dependency | Owner | Classification |
| --- | --- | --- |
| `__udivti3` | compiler-rt in libSystem | external |
| `snprintf`, `strlcpy`, `memmove`, `memset` | libsystem | external |
| `___stack_chk_guard`, `___stack_chk_fail` | libsystem (compiler protector) | external |

Only raw integers, a 24-byte value and character buffers cross these boundaries.
There are no CF/NS objects, allocations or callbacks.

The six external functions are declared `_OCFBigNum…` and map to their native
spellings in system-name mode. `emit128` is non-external natively. It keeps its
native spelling as a hidden symbol: it is never exported, appears in no installed
header, and is reached from tests through the internal test sources. This is the
recorded naming decision for this name outside CF/NS. The tables are file-local
`static` definitions with their native names. `BigNum` and `Int128Parts` are
local descriptive type names in private headers.

Release builds use full LTO, which lowers the `memmove` in
`_CFBigNumToCString` to `memcpy`: the stack buffer cannot overlap the caller's
buffer. The bytes written are the same. Debug builds call `memmove` as the
native code does.

## iOS Device comparison

The iOS Device functions were compared instruction by instruction with the iOS
Simulator functions; the table bytes and the format literal were compared byte
for byte.

- `_CFBigNumInitWithInt8`, `Int16`, `Int32`: identical instructions.
- `_CFBigNumInitWithInt128`: identical apart from `PACIBSP`/`RETAB`.
- `_CFBigNumInitWithInt64`: the negative-sign block is placed after the return
  and branches back; stores and arithmetic agree.
- `_CFBigNumToCString`: the `-` and leading-zero blocks are placed after the
  return. The leading-zero address arithmetic is written differently
  (`NEG`/`ADD` instead of `SUB`), with the same result. The calls, their
  arguments and their order agree.
- `emit128`: the negation block is placed after the return and other scratch
  registers are used; comparisons, table reads and stores agree.
- `powersOf10`, `neg_powersOf10`: the 624 bytes of each agree. The format
  literal's 21 bytes agree.

Each difference is recorded as an `iOS Device divergence:` comment at its
definition. No observable difference was found.

## Verification and limits

The standalone suite has 14 fixed cases with independently derived
expectations: the minimum of each width, INT64_MAX, 2^127, signed, plus and
padded text, truncation by `size`, a 48-character padded result, and `emit128`
on -2^64, 0 with plus, the 128-bit minimum (`-0`) and maximum.

The differential suite resolves the six external native functions by name and
`emit128` by IDA offset (image base 0x1c014a000), each in the UUID-pinned
CoreFoundation image. The replacements come from OpenCoreFoundation, and
`emit128` from the internal test sources (`checkInternalImplementation`).
Identical addresses are a setup failure (77). Each comparison covers the
24-byte value with 32-byte canaries on both sides, or the 160-byte text area
with 16 canary bytes before the buffer. It covers:

- every Int8 value, with all text variants, and every Int16 value;
- the type limits and their negations, and 10^k−1, 10^k and 10^k+1 with both
  signs for k = 0..38, through Int128, Int64 (k ≤ 18) and Int32 (k ≤ 9);
- 2,000,000 deterministic random 128-bit values with random bit lengths, and as
  many 64-bit and 32-bit values;
- 500,000 arbitrary values that the initializers never produce, with digits
  above 10^9−1 and arbitrary sign bytes;
- buffer sizes 0, 1, 2, 3, 5, 7, 9, 10, 11, 17, 19, 31, 44..48, 63 and 96, both
  flags, exhaustive for the fixed values and the first 2,000 arbitrary values,
  and one random variant otherwise.

The prefixed Debug run reports 14,764,165 comparisons. The unbounded
leading-zero fill at wrapped size zero is excluded: it is destructive in both
implementations. A plain `strlcpy` at wrapped size zero is tested; the area is
large enough for it. Commands: `make test`, `make OF_SYSTEM_NAMES=YES test`
(standalone only), `make verify`.

Not built: the unsigned initializers, `_CFBigNumInitWithBytes`,
`_CFBigNumInitWithCFNumber`, the getters, `_CFBigNumCompare`/`Neg`/`Add`/`Sub`,
`_CFBigNumFromCString`, `_CFBigNumCopyDescription` and
`_CFNumberCreateWithBigNum`. They are outside this group's evidence; the
CFNumber and number-formatter callers wait on the object runtime.

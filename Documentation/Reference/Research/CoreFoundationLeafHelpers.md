# CoreFoundation leaf helpers

## Identity and scope

This reading restores sixteen small CoreFoundation functions from five
subsystems (string formatting, property lists, file-path utilities, bundles and
URLs) plus one character-set comparator, and a second batch of three: the
Gregorian day count `__CFAbsoluteFromYMD`, the binary-plist header and trailer
validation `__CFBinaryPlistGetTopLevelInfo` and the system-version mapping
`_CFBuildVersionForCFSystemVersion`, with their three tables. Each is a leaf of the scope: it calls
nothing but `memcmp`, `strncmp` (libsystem, external) or a block its caller
passes, and it reads or writes no CF/NS object. It is not a restoration of
string formatting, the binary, XML or old-style property-list parsers, CFBundle,
CFURL, CFCharacterSet, CFDate or the linked-on-or-after checks.

| Reference | Identity |
| --- | --- |
| iOS Simulator | iOS 27.0 (24A434), arm64; CoreFoundation UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; SHA-256 e3598ca3b1ed3649c1ff8aadce058e98aa2d7be5385036a5ebb6ab986a5cc829 |
| iOS Device | iOS 27.0.1 (24A446), arm64e; CoreFoundation UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; SHA-256 ba22deb3759f56d32e5847fec0a5e675837e189d9a000b87ffdefe88a7942166 |

The evidence is the IDA capture `Local/Evidence/below-core-capture/batch2-iphone-simulator.json`
and `batch2-iphone-device.json`, group `leaf-helpers`: function boundaries,
complete disassembly, Hex-Rays pseudocode, every caller with its call-site
instructions, and the bytes of each referenced datum. The disassembly is the
authority; the pseudocode located questions only. Linkage is from `nm -m` on the
canonical images. No IDA session was opened for the first batch. The second
batch is `batch3-iphone-simulator.json` and `batch3-iphone-device.json`, group
`scalar-leaves`, captured with the same script through IDA on APFS clones under
`Local/Scratch/cf-scalar-leaves`, plus `iphone-device-toplevel-cold.json` for the
Device cold entries of `__CFBinaryPlistGetTopLevelInfo`.

| Native function | Linkage | iOS Simulator | iOS Device | Source | Confidence |
| --- | --- | --- | --- | --- | --- |
| `__CFStringValidateFormatSpecBuffers` | non-external | 0x1c02242fc | 0x1806ba570 | `String/OCFString.c` | verified |
| `__CFStringFindTypeAndSizeForArgumentIndex` | non-external | 0x1c022452c | 0x1806d64a8 | `String/OCFString.c` | verified |
| `__rangeEqual` | non-external | 0x1c0218d64 | 0x180764320 | `String/OCFString.c` | verified |
| `__CFBinaryPlistGetOffsetForValueFromArray2` | external | 0x1c0163e48 | 0x180733140 | `PropertyList/OCFBinaryPList.c` | verified |
| `__CFBinaryPlistParseASCIIString` | external | 0x1c01667dc | 0x18061f684 | `PropertyList/OCFBinaryPList.c` | verified |
| `__CFBinaryPlistParseUnicode16String` | external | 0x1c01668f4 | 0x18068035c | `PropertyList/OCFBinaryPList.c` | verified |
| `__CFPropertyListKeyBufferMatchesFilterBuffer` | non-external (was a private external) | 0x1c01d446c | 0x18068f8c4 | `PropertyList/OCFPropertyList.c` | verified |
| `lineNumber` | non-external | 0x1c01d84dc | 0x1806f6734 | `PropertyList/OCFPropertyList.c` | verified |
| `advanceToNonSpace` | non-external | 0x1c02d4e94 | 0x180702bac | `PropertyList/OCFOldStylePList.c` | verified |
| `lineNumberStrings` | non-external | 0x1c02d54e0 | 0x18070d188 | `PropertyList/OCFOldStylePList.c` | verified |
| `_CFGetSlash` | non-external (was a private external) | 0x1c019cefc | 0x18066f0b8 | `Base/OCFFileUtilities.c` | verified |
| `_CFStartOfLastPathComponent` | non-external (was a private external) | 0x1c019d374 | 0x180698360 | `Base/OCFFileUtilities.c` | verified |
| `_CFLengthAfterDeletingLastPathComponent` | non-external (was a private external) | 0x1c019d458 | 0x18069ab0c | `Base/OCFFileUtilities.c` | verified |
| `_CFBundleNormalizedPath` | non-external (was a private external) | 0x1c016a39c | 0x18063e8d4 | `PlugIn/OCFBundle.c` | verified |
| `_netLocationRange` | non-external | 0x1c0244938 | 0x180712ef4 | `URL/OCFURL.c` | verified |
| `chcompar` | non-external | 0x1c0174a34 | 0x180718e44 | `CharacterSet/OCFCharacterSet.c` | verified |
| `__CFAbsoluteFromYMD` | non-external | 0x1c018dee0 | 0x1806d56e4 | `Date/OCFDate.c` | verified |
| `daysBeforeMonth` (data, 16 entries) | non-external | 0x1c0373534 | 0x18082f764 | `Date/OCFDate.c` | verified |
| `__CFBinaryPlistGetTopLevelInfo` | external | 0x1c0163b94 | 0x18061ce78 | `PropertyList/OCFBinaryPList.c` | verified |
| `_CFBuildVersionForCFSystemVersion` | external | 0x1c0256b10 | 0x18060c1c8 | `Base/OCFUtilities.c` | verified |
| `_CFBuildVersionForCFSystemVersion.macos_map` (data) | non-external | 0x1c0456ae0 | 0x1809651d8 | `Base/OCFUtilities.c` | verified |
| unnamed version table (data, 11 entries) | no symbol | 0x1c0456b28 | 0x180965220 | `Base/OCFUtilities.c` | verified |

The five external functions are exported as `__OCFBinaryPlist...` and
`_OCFBuildVersionForCFSystemVersion` (prefixed) or under their native names
(system-names mode); the others keep their native spelling and stay hidden.
`OCFDate.c` now holds an exported and a hidden function, so it joins
`Configuration/InternalTestSources.json` as `OCFUniChar.c` did.

Source paths. Only the bundle path is recovered: the `__cstring` bytes that
follow the two literals of `_CFBundleNormalizedPath` name
`PlugIn.subproj/CFBundle.c`. Every other file is inferred from the native address
neighborhood (`nm -m`): the two format functions lie between
`__CFStringSetUpFormatAndSpecBuffers` and `__CFStringValidateFormatSpec`, and
`__rangeEqual` beside `__rangeRetain`, `__rangeRelease`, `__rangeCopyDescription`
and `CFStringFind`, all CFString code (`String/OCFString.c`); the array and string
functions among the binary-plist functions (`OCFBinaryPList.c`); the filter test
among the `__CFPropertyList...` functions and `lineNumber` among the XML parser
(`skipXMLComment`, `parseXMLElement`) (`OCFPropertyList.c`); `advanceToNonSpace`
and `lineNumberStrings` among the old-style parser
(`__CFCreateOldStylePropertyListOrStringsFile`, `parsePlistObject`)
(`OCFOldStylePList.c`); the path functions among `_CFWriteBytesToFile`,
`_CFAppendPathComponent2` and the other file utilities (`Base/OCFFileUtilities.c`);
`_netLocationRange` beside `CFURLCopyNetLocation` (`URL/OCFURL.c`); `chcompar`
between `CFCharacterSetCreateWithCharactersInString` and
`CFCharacterSetCreateMutable` (`CharacterSet/OCFCharacterSet.c`);
`__CFAbsoluteFromYMD` between `CFGregorianDateGetAbsoluteTime` and
`CFAbsoluteTimeGetGregorianDate`, beside the restored `CFGregorianDateIsValid`
(`Date/OCFDate.c`); `__CFBinaryPlistGetTopLevelInfo` among the binary-plist
functions; `_CFBuildVersionForCFSystemVersion` between
`_CFOperatingSystemVersionGetCurrent` and `_CFExecutableLinkedOnOrAfter`
(`Base/OCFUtilities.c`, a new file). The directory and file names are inferred.

Unread and unbuilt: the producers of the format-spec buffers and both parser
contexts; the callers of `__CFBinaryPlistGetTopLevelInfo` and of
`_CFBuildVersionForCFSystemVersion`; the callers' string blocks; four iOS Device
linker-outlined bodies reached from the `.cold` entries listed below.

## Native implementation

### Format-specification record

Both format functions walk records with a 32-byte stride (`ADD X8, X8, #0x20`).

| Offset | Size | Content |
| --- | --- | --- |
| 0 | 2 | Size: copied as 16 bits, compared by its low byte |
| 2 | 2 | Type: copied as 16 bits, compared by its low byte |
| 4..23 | 20 | Unread by these functions |
| 24 | 1 | Main argument number (signed); -1 takes the next sequential number |
| 25 | 1 | Precision argument number (signed); -2 takes the next number, other negative values mean none |
| 26 | 1 | Width argument number (signed); as offset 25 |
| 27..31 | 5 | Unread by these functions |

Numbers are assigned per record in the order width, precision, main from a
64-bit counter; each assigned number is the counter's low byte, sign-extended
(`SXTB`). The width/precision/main roles are inferred from that order; the
offsets, widths and stride are verified. `FormatSpec` and its field names are
local spellings.

`__CFStringFindTypeAndSizeForArgumentIndex(specs, count, argumentIndex, type, size)`
takes `count` as 32 bits (`CMP W1, #1`, `SXTW`) and `argumentIndex` as 64 bits
(`CMP X2, Wn, SXTB`; the callers sign-extend an 8-bit number). The counter starts
at zero. A record whose width or precision number equals the index stores type
0x21 and size 3 (both `STRH`); one whose main number equals it stores the
record's type and then its size. The first match returns 1; no match returns 0
with the out parameters untouched.

`__CFStringValidateFormatSpecBuffers(specs, count, expectedSpecs, expectedCount, nextArgumentIndex)`
numbers each record of `specs` the same way, starting from the 64-bit fifth
argument. Per record it clears one 32-bit stack word holding both results, then:

1. A width number other than -1 must be found in `expectedSpecs` with type low
   byte 0x21 and size low byte at most 4; likewise a precision number.
2. The main number must be found.
3. The record's type low byte must equal the found one, or form an accepted
   pair: record 0x23 with found 0x21 or 0x25..0x28; record 0x26 with 0x23;
   records 0x27 and 0x28 with 0x23, 0x27 or 0x28 (mask 0x18800000000 for found
   types up to 0x28); record 0x29 with 0x21.
4. Both size low bytes, with values up to 4 raised to 4, must be equal.

Any failure returns 0; zero records or all records passing return 1.

`__rangeEqual(first, second)` returns whether both 64-bit words of two 16-byte
ranges are equal; it is an address-taken callback with no direct caller.

### Binary property-list trailer and array lookup

`__CFBinaryPlistGetTopLevelInfo` fills the 32-byte trailer record and
`__CFBinaryPlistGetOffsetForValueFromArray2` reads four of its fields:

| Offset | Size | Content |
| --- | --- | --- |
| 0 | 6 | The file trailer's first six bytes, copied (4-byte store at 0, 2-byte at 4) |
| 6 | 1 | Offset-table entry width |
| 7 | 1 | Object-reference width |
| 8 | 8 | Object count |
| 16 | 8 | Top object index |
| 24 | 8 | Offset-table offset |

The fields are host-endian. `BinaryPlistTrailer` is a local spelling.

`__CFBinaryPlistGetTopLevelInfo(data, length, marker, offset, trailer)` checks,
in order, each failing with 0 and no store: null data or `length < 41`; the
header `bplist0` (32-bit compares at 0 and 3, so the version digit is free); the
big-endian trailer in the last 32 bytes: a zero count, a count or table offset
with bit 63 set, `count <= top`, `tableOffset < 9`, `length - 32 <= tableOffset`,
a zero entry or reference width; `count * entryWidth` overflowing (`UMULH`) or
zero; `tableOffset + tableSize > 2^64 - 33`; `tableOffset > ~tableSize`;
`length != tableOffset + tableSize + 32`; a reference width at most 7 too narrow
for the count; data above `2^64 - 10`; an entry width at most 7 too narrow for the
table offset; `data + tableOffset` or the table's last byte wrapping. Every entry
must then be at most `tableOffset - 1`, and the top object's entry must lie in
`[8, tableOffset)`. On success it stores the trailer, then `*offset`, then
`*marker` (the byte at that offset), each only when nonnull, and returns 1. The
1/2/4/8-byte `REV` loads and the byte loop of other widths all yield the
big-endian value the restoration's byte loop reads.

The function never reads its second argument (the data length) or X6, which one
of its two callers sets. Checks, in order, each failing with 0 and no store:
`startOffset < 8`; `offsetTableOffset - 1 < startOffset` (the object area ends
one byte before the table; the subtraction wraps for 0); the marker address is
the last address; the marker's high nibble is not 0xA. A low nibble of 0xF reads
the count from the following integer object: its marker must lie inside the
object area and have high nibble 1; its 2^n bytes must not overflow and must end
inside the object area; n > 7 fails; the big-endian value fails when bit 63 is
set (possible only for 8 bytes and more). Then `count <= index` (unsigned) fails,
the product `count * objectRefSize` must not overflow (`UMULH`), the reference
table must not overflow and must end inside the object area, the reference
offset `index * objectRefSize + start` must be at least 8 (signed), and the
reference must start no later than `offsetTableOffset - objectRefSize`. The
reference value (width 0 yields 0) must be below the object count. Only then,
and only for a nonnull out pointer, the offset-table entry is read without a
bound and stored; the function returns 1. The native code switches on the low
byte of each width (byte, `LDRH`/`LDR` with `REV`, or a byte loop); every case
yields the big-endian value of the byte loop the restoration uses. The unbounded
table read relies on the trailer validation of its callers' producer (unread);
the tests keep every table read inside the buffer.

### Binary property-list strings

`__CFBinaryPlistParseASCIIString(base, limit, marker, block)` and
`__CFBinaryPlistParseUnicode16String` share the count read: the marker's low
nibble, or for 0xF the following integer object, with `base + limit` as the last
readable byte. The integer marker byte must lie at or before it and have high
nibble 1; its 2^n bytes must not overflow and must end at or before it; n > 7
yields a count of zero (unlike the array lookup, no failure); bit 63 set fails.
The marker's own high nibble is never tested. A marker at the last address
fails before any read.

The ASCII function then requires that `characters + count` does not overflow
and that the last character byte lies at or before `base + limit`. The UTF-16
function fails when `(characters + count - 1) + count` overflows, then when
`characters + count` overflows, then when the `2 * count` bytes end after
`base + limit`. Both tail-call the block (`LDR X4, [X3, #0x10]`, `MOV X0, X3`,
`MOV X1, characters`, X2 = count, `BR X4`): the Block ABI invocation of a block
parameter, `bool (^)(const uint8_t *, uint64_t)`, whose result they return. The
callers (`__CFBinaryPlistCreateObjectFiltered`,
`__CFPropertyListCreateFilteredDictionary`) pass stack blocks; their bodies
belong to those callers and are not part of this group.

### XML and old-style parser helpers

`__CFPropertyListKeyBufferMatchesFilterBuffer(key, keyLength, filter, filterLength)`:
a one-byte filter `*` matches anything and any other one-byte filter matches
only the equal one-byte key; a longer filter ending in `*` matches a key of at
least `filterLength - 1` bytes (signed) by `memcmp` of that prefix; otherwise
the lengths must be equal and `memcmp` decides. Equal negative lengths pass a
huge size to `memcmp`; no native caller does so, and the tests never call it.

`lineNumber` reads three pointers of the XML parser context and
`lineNumberStrings` the same three of the old-style context (UTF-16 units):

| Offset | Size | Content |
| --- | --- | --- |
| 0 | 8 | First byte or unit |
| 8 | 8 | Current position (written only by `advanceToNonSpace`) |
| 16 | 8 | End |

Later fields are unread and undeclared. Both count from 1 in 32 bits while the
position is below both the end and the current position: LF and CR each count,
and a CR followed by LF inside both bounds counts once.

`advanceToNonSpace` skips U+0009..U+000D, U+2028, U+2029 and the space, `//`
comments up to (not past) LF, CR, U+2028 or U+2029, and `/*` comments past the
closing `*/`. It returns 1 with the position at the first other unit (a final
lone `/` included) and 0 with the position at the stop point when the end is
reached; an unterminated `/*` stops at the end. When the position is already at
or past the end it returns 0 without a store.

### Paths, bundles, URLs and character sets

`_CFGetSlash` returns `/`. `_CFStartOfLastPathComponent(unichars, length)` and
`_CFLengthAfterDeletingLastPathComponent` return 0 for lengths below 2 and scan
from index `length - 2` down. The first returns one past the last slash; the
second returns its index, except 1 for a slash at index 0 and 3 for a slash at
index 2 after an `X:` drive prefix (X an ASCII letter). Without a slash, a length
above 2 with a drive prefix gives 2, otherwise 0. The no-slash test masks bit 5
and compares with `A`..`Z`; the slash-at-2 test compares both letter ranges;
they select the same units.

`_CFBundleNormalizedPath(path)` returns NULL unchanged, advances a path that
begins with the 30-byte `/private/preboot/Cryptexes/OS/` by 29 and one that
begins with the 21-byte `/System/Cryptexes/OS/` by 20, and otherwise returns it
unchanged. The literals are 31 and 22 bytes with their NULs at 0x1c047d8ee and
0x1c047d90d.

`_netLocationRange(flags, ranges)` takes 32-bit component flags and a packed
array holding one 16-byte range per set bit, bit 0 first. Without any of bits
1..4 the result is {-1, 0}. Otherwise the ranges of bits 1..4 are collected in
order, each at the index equal to the number of lower set bits, {-1, 0} for a
clear bit. The first collected location other than -1 starts the result; each
later one sets the length to its location minus the start plus its length, with
64-bit wraparound. Callers pass a URL's range array (`X19 + 0x38` in
`CFURLCopyNetLocation`) or a local array; the function reads only that array.

`chcompar(first, second)` returns the first UTF-16 unit minus the second as a
32-bit int; it is an address-taken `qsort` comparator.

### Gregorian day count

`__CFAbsoluteFromYMD(year, month, day)` takes the year relative to 2001 and 8-bit
month and day (its callers sign-extend 8-bit fields with `SBFX #8`). It starts at
`fma(year / 400, 146097.0, +0.0)` (a fused `FMADD` with a zero addend; the
quotient truncates). With `r = year % 400`, it adds 365.0 or 366.0 for each of
years 1..r when r > 0, or subtracts them for years `-r - 1` down to 0 when r < 0,
in wrapping unsigned arithmetic modulo 400; a year y is leap when `y % 400` is a
multiple of 4 other than 100, 200 and 300. If the month, as unsigned 32 bits, is
at most 15, `daysBeforeMonth[month]` plus the leap flag of `|(r + 1) % 400|` (only
after February) is added unless its low 16 bits are 0xffff. The table is
0xffff, 0, 31, ..., 334, 365, 0xffff, 0xffff, so month 0 adds nothing, month 13
a whole year and month 14 or 15 adds 0.0 in a leap year (0xffff + 1 wraps).
Last, `day - 1` is added. The restoration keeps each addition in this order.

### System-version mapping

`_CFBuildVersionForCFSystemVersion(version, platform)` returns, in X0, a platform
(low 32 bits) and a packed version (high 32 bits), the layout of dyld's
build-version pair (`OCFBuildVersion` is a local spelling). The version is
compared as 64 bits, the platform as 32 (1 macOS, 2 iOS, 6 Mac Catalyst): 0..4
give version 0 with Mac Catalyst rewritten to iOS; 5..20 index `macos_map`
(10.5 ... 10.16, 12 ... 15) for macOS and Mac Catalyst, and otherwise give 0 below
10 and 0xffff0000 from 10; 1005..1015 index an unnamed eleven-entry table for
macOS only, and 1000..1004 or any other platform in that range give version 0;
every other version passes its low 32 bits through. Mac Catalyst then maps the
macOS version to iOS: `macos_map` entries below index 11 and versions below
10.15.1 to 13.0, 10.15.1 to 13.2, 10.16 and 11.0 to 14.0, anything else to
0xffff0000. The unnamed table lies eight bytes into the span `nm` assigns to
`_CFLog_os_trace_type_map`; the function's bound reads only its 44 bytes. A
native fallback after that bound (macOS, 0xffff0000) is unreachable because the
range check admits at most index 10; it is kept.

## Dependencies and naming

The second batch calls nothing. `memcmp` and `strncmp` are libsystem functions, external cuts called with raw
bytes. The string parsers invoke the caller's block through the Block ABI; no
runtime call is made. No function exchanges a CF/NS object, reads run-time
global state or makes an unresolved indirect call. The record and context types
are local spellings with only the read fields declared. No defect was
established; the unbounded offset-table read and the negative-length `memcmp`
are native contract behavior kept as is.

## iOS Device comparison

Every function was compared with its iOS Device body (table above, UUID
9426FC9A-B36C-3096-B836-D17D6744A75B). `_CFGetSlash`, `__rangeEqual`, `lineNumber`
and `chcompar` are identical; the others differ in block order, counter
indexing, stack guards and pointer authentication with the same checks.

iOS Device divergences: six code paths are moved out of line. Their entries were
captured through IDA in `Local/Evidence/below-core-capture/iphone-device-leaf-cold.json`.

| Cold entry | Address | Size | Called from | Moved code |
| --- | --- | --- | --- | --- |
| `__CFBinaryPlistGetOffsetForValueFromArray2.cold.1` | 0x1807b9414 | 4 | 0x18073347c | 8-byte object-reference read |
| `__CFBinaryPlistGetOffsetForValueFromArray2.cold.2` | 0x1807b9418 | 4 | 0x180733460 | 4-byte object-reference read |
| `__CFBinaryPlistGetOffsetForValueFromArray2.cold.3` | 0x1807b941c | 4 | 0x1807333f0 | 4-byte offset-table read |
| `__CFBinaryPlistParseUnicode16String.cold.1` | 0x1807b955c | 4 | 0x1806804d8 | 8-byte count read |
| `_CFStartOfLastPathComponent.cold.1` | 0x1807bd49c | 44 | 0x1806983b4 | No-slash drive test |
| `_CFLengthAfterDeletingLastPathComponent.cold.1` | 0x1807bd4c8 | 44 | 0x18069abbc | No-slash drive test |

- The four 4-byte entries are a single unconditional `B`, not a trap, into a
  linker-outlined function: `_OUTLINED_FUNCTION_2_0` (array `.cold.1`),
  `_OUTLINED_FUNCTION_1_2` (array `.cold.2` and `.cold.3`) and
  `_OUTLINED_FUNCTION_0_5` (Unicode16 `.cold.1`). The callers pass the address in
  X0 (Unicode16: the integer bytes in X0 and the marker in X1) and a stack slot
  in X1 (Unicode16: X2), then reload the value from the slot; Unicode16 then tests
  bit 63 as the simulator does. The outlined bodies were read later through IDA
  (`Local/Evidence/below-core-capture/iphone-device-leaf-outlined.json`): `_OUTLINED_FUNCTION_2_0` (0x18061363c) is an 8-byte `LDR` + `REV`
  stored through the slot, `_OUTLINED_FUNCTION_1_2` (0x180613ac4) a 4-byte `LDR`
  + `REV` zero-extended into the 8-byte slot, and `_OUTLINED_FUNCTION_0_5`
  (0x18065c54c) assembles the eight bytes at marker + 2 big-endian with
  `LDRB`/`ORR`. They are the simulator's reads. Confidence: verified.
- The two 44-byte entries are read completely up to their final branch. W8 is
  1 when `unichars[1]` is not `:`. Otherwise it is 0 when
  `(unichars[0] & ~0x20) - 'A' <= 25` and all ones (`CSETM`) for any other unit.
  Both then branch to the shared `_OUTLINED_FUNCTION_0_19` (0x18065c694,
  `AND W0, W8, #1; RET`, read through IDA). The callers return 0 when bit 0 of W0 is set
  and 2 otherwise, selecting the same units as the simulator. Confidence:
  verified.

Second batch: `__CFAbsoluteFromYMD` computes the month leap flag with the tests
in another order and the same floating-point operations; `daysBeforeMonth` and
both version tables agree byte for byte. `_CFBuildVersionForCFSystemVersion`
lays its branches out in another order, with the same result for every input; its
only Device caller is `_CFPlatformExecutableLinkedOnOrAfter`.
`__CFBinaryPlistGetTopLevelInfo` adds a stack guard and moves the 8-byte and
4-byte reads of the top object's offset to `.cold.1` (0x1807b9410, `B
_OUTLINED_FUNCTION_2_0`) and `.cold.2` (0x18070d4d4, `B _OUTLINED_FUNCTION_1_2`),
the same `LDR` + `REV` bodies that serve the array lookup. Confidence: verified.

No Device trap replaces a simulator read.

## Verification and limits

The differential suites resolve the external functions by name and the hidden
ones by IDA offset from 0x1c014a000 in the UUID-guarded native image; the hidden
replacements are compiled into the test tools through
`Configuration/InternalTestSources.json`. Results, every out parameter between
canaries, every block call (count, pointer, argument) and the parser context are
compared exactly.

- Format records: every byte triple of the three argument numbers in one record
  against several indices; random arrays of up to 299 records so the 8-bit
  numbering wraps; 1,500,000 random and mutated validation pairs; every pair of
  type low bytes with all size pairs 0..8.
- Binary plist: 2,000,000 arrays with inline and integer counts of every width,
  reference and offset widths in {0, 1, 2, 3, 4, 5, 8, 16, 255}, wrapped and
  out-of-range offsets and indices; every marker and integer-marker byte pair at
  nine ends and 1,000,000 random string cases.
- Filters over every string of length 0..4 in `a`, `b`, `*`; line numbers over
  every LF/CR/`a` buffer up to 10 bytes (UTF-16: five units up to 8) at every
  current position and three ends; `advanceToNonSpace` over every 8-symbol
  buffer up to 7 units at every start.
- Paths over every 5-symbol buffer up to 9 units and every first unit with a
  drive colon; bundle paths over every prefix cut and last byte of both
  literals; `_netLocationRange` over all low 16 flag bits with random high bits
  and 64 range arrays; `chcompar` over 33 million unit pairs.

- Second batch: `__CFAbsoluteFromYMD` over years -4000..4000 with every 8-bit
  month, years -60..60 with every month and day, the 64-bit year extremes and
  1,000,000 random years up to 2^40, compared as bit patterns (10,979,392 cases);
  `__CFBinaryPlistGetTopLevelInfo` over 400,000 generated lists (entry widths
  1..9, 12, 16, arbitrary reference widths) and their perturbations, with every
  null/nonnull output combination, prefilled outputs and all 32 trailer bytes
  compared (2,000,008 cases, 1,065,774 accepted); `_CFBuildVersionForCFSystemVersion`
  over versions 0..4095 and the macOS-version edges with clear, single and random
  high bits, over 64 small and random platforms, and 1,000,000 random pairs
  (1,887,976 cases). Standalone: 13, 10 and 19 known answers.

Standalone cases check the layouts and known answers in both naming modes.
Coverage is finite. Pointer-wrap paths of the binary-plist checks are reachable
only near the top of the address space and are untested.

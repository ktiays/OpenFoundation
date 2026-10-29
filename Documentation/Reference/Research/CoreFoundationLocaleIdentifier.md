# CoreFoundation locale-identifier canonicalization helpers

## Identity and scope

This reading restores the seven hidden helpers that the CoreFoundation
locale-identifier canonicalization entry points call, and the two tables they
read: the prefix search `_UpdateFullLocaleString` with its case/separator scan
and region special cases, the tag test `_CheckForTag`, the three `bsearch`
comparators, `_RemoveSubstringsIfPresent` and `_AppendKeyValueString`. They are
below the core group of the scope and call only libicucore and libsystem. It is
not a restoration of `CFLocaleCreateCanonicalLanguageIdentifierFromString`,
`CFLocaleCreateCanonicalLocaleIdentifierFromString`, CFLocale or the other
mapping tables of those entry points.

| Reference | Identity |
| --- | --- |
| iOS Simulator | iOS 27.0 (24A434), arm64; CoreFoundation UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; SHA-256 e3598ca3b1ed3649c1ff8aadce058e98aa2d7be5385036a5ebb6ab986a5cc829 |
| iOS Device | iOS 27.0.1 (24A446), arm64e; CoreFoundation UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; SHA-256 ba22deb3759f56d32e5847fec0a5e675837e189d9a000b87ffdefe88a7942166 |

The evidence is the IDA capture `Local/Evidence/below-core-capture/batch2-iphone-simulator.json`
and `batch2-iphone-device.json`, group `locale-identifier`, plus
`iphone-device-locale-outlined.json` for the three iOS Device outlined bodies, with
`pointed_data["locale-identifier"]` for the strings the tables point at:
function boundaries, complete disassembly, Hex-Rays pseudocode, every caller
with its call-site instructions, and the bytes of each referenced datum up to
the next named symbol. The disassembly is the authority; the pseudocode located
questions only. Linkage is from `nm -m` on the canonical images. No IDA session
was opened for this track. `Local/Evidence/cf-locale-identifier/generate-tables.py`
decodes both tables from both captures, resolves iOS Device chained-fixup
targets (0x180000000 + low 32 bits), checks that the images agree record for
record, that the hoisted `"-UK"` literal is the first special-case region tag,
and that the prefix table is sorted, and prints the C initializers.

| Native function | Linkage | iOS Simulator | iOS Device | Confidence |
| --- | --- | --- | --- | --- |
| `_CompareTestEntryToTableEntryKey` | non-external | 0x1c01a0324 | 0x1806a63f0 | verified |
| `_UpdateFullLocaleString` | non-external | 0x1c01a0330 | 0x1806a6a84 | verified |
| `_CompareTestEntryPrefixToTableEntryKey` | non-external | 0x1c01a0888 | 0x1806a7388 | verified |
| `_RemoveSubstringsIfPresent` | non-external | 0x1c01a08e4 | 0x1806ca710 | verified |
| `_AppendKeyValueString` | non-external | 0x1c01a0a14 | 0x1806a751c | verified |
| `_CompareLowerTestEntryPrefixToTableEntryKey` | non-external | 0x1c01a1810 | 0x1806a6fbc | verified |
| `_CheckForTag` | non-external | 0x1c01a18fc | 0x1806a7118 | verified |

| Native datum | Linkage | iOS Simulator | iOS Device | Size | Confidence |
| --- | --- | --- | --- | --- | --- |
| `_localeStringPrefixToCanonical` | non-external, `__DATA_CONST,__const` | 0x2227a0b98 | 0x1e04ead30 | 304 x 16 bytes | verified |
| `_specialCases` | non-external, `__DATA_CONST,__const` | 0x2227a1e98 | 0x1e04ec030 | 7 x 40 bytes + 40-byte terminator | verified |

The captured spans (5,704/5,416 and 840/552 bytes) are upper bounds. The prefix
table length is the `bsearch` count 0x130 and width 0x10; the special-case
length is the loop's 0x28 stride and its stop on a NULL region tag, reached at
+0x118 in both images. The strings are `__cstring` literals; all 304 prefix
records and all seven special-case records decode to the same strings in both
images.

No original source path was recovered. The functions lie together in
0x1c01a0324..0x1c01a1960 with their callers
`_CFLocaleCreateCanonicalLanguageIdentifierFromString` and
`_CFLocaleCreateCanonicalLocaleIdentifierFromString`; the restoration places
them in `Source/CoreFoundation/Locale/OCFLocaleIdentifier.c` (inferred directory
and file name).

## Native implementation

### Record layouts

`_localeStringPrefixToCanonical` and the `bsearch` keys share one 16-byte record
(`LocaleStringMapping`, a local name):

| Offset | Size | Content |
| --- | --- | --- |
| 0 | 8 | Key string; the only field the comparators read (`LDR X, [X0]`, `LDR X, [X1]`) |
| 8 | 8 | Result string; read by `_UpdateFullLocaleString` after a match (`LDR X23, [X23,#8]`) and by the callers before `_RemoveSubstringsIfPresent` |

`_UpdateFullLocaleString` builds its key on the stack as `{inLocaleString, NULL}`
(`STP X0, XZR`). The table is sorted by key in byte order.

`_specialCases` (`LocaleSpecialCase`, a local name) has five string pointers:

| Offset | Field | Use |
| --- | --- | --- |
| 0 | languageTag | NULL, or a two-letter language required by `_CheckForTag(inLocaleString, tag)` |
| 8 | regionTag | `"-XX"`; searched with `strstr`, and `regionTag + 1` tested against the `_` region |
| 16 | replacement | Upper-case first byte: a region replacement; otherwise a language replacement |
| 24 | alternateRegionTag | Read only on the language-replacement path |
| 32 | alternateReplacement | Language replacement for the alternate tag |

| Language | Region | Replacement | Alternate | Alternate replacement |
| --- | --- | --- | --- | --- |
| NULL | `-UK` | `GB` | NULL | NULL |
| NULL | `-TP` | `TL` | NULL | NULL |
| `cs` | `-CS` | `CZ` | NULL | NULL |
| `sk` | `-CS` | `SK` | NULL | NULL |
| NULL | `-CS` | `RS` | NULL | NULL |
| NULL | `-YU` | `RS` | NULL | NULL |
| `sh` | `-HR` | `hr` | `-RS` | `sr` |

Only the `sh` record has a lower-case replacement, so only it reaches the two
alternate fields; the terminator record is all NULL.

### Character classes

Every class test is the `<ctype.h>` inline against `_DefaultRuneLocale`: a byte
is sign-extended, a non-negative value indexes `__runetype` (offset 0x3c),
otherwise `__maskrune` is called. Masks: 0x100 alpha, 0x500 alnum, 0x800 graph,
0x4000 space, 0x8000 upper. Case conversion calls `__tolower` and `__toupper`.
The restoration uses the same inlines and calls.

### Comparators

- `_CompareTestEntryToTableEntryKey` tail-calls `strcmp` on the two keys; its
  result is returned unchanged.
- `_CompareTestEntryPrefixToTableEntryKey` walks both keys bytewise while the
  bytes are equal and the table byte is nonzero. It returns 0 when the table key
  ended (a prefix match), otherwise -1 or 1 from a signed comparison of the last
  bytes read.
- `_CompareLowerTestEntryPrefixToTableEntryKey` lowers each test byte with
  `__tolower` and compares it, as a signed char, with the table byte; the walk
  stops when they differ, the table key ends or the lowered byte is `_`. If the
  table key has not ended, a lowered `_` returns -1, otherwise the signed
  comparison decides. If it has ended, the result is 1 when the lowered test
  byte is alphanumeric and 0 otherwise. A key followed by `_`, `-`, `@` or the
  terminator therefore matches; a key followed by a letter or digit does not.

### `_CheckForTag`

True (W0 = 1) when `strncmp(localeString, tag, 2)` is 0 and the third byte is
not alphanumeric; otherwise false.

### `_UpdateFullLocaleString`

Arguments: the 257-byte locale buffer, two output pointers and a 257-byte
key/value buffer. Both callers pass stack buffers of that capacity
(`uloc_canonicalize` capacity 0x101).

1. **Prefix replacement.** `bsearch` over the 304 records with
   `_CompareLowerTestEntryPrefixToTableEntryKey`. On a match the 32-bit
   difference `strlen(value) - strlen(key)` decides: when negative the tail from
   the key end, through its terminator, moves left; when positive and
   `delta + strlen(buffer) > 256` (signed 32-bit) nothing is replaced; otherwise
   the tail from `buffer + strlen(buffer)` down to `buffer + strlen(key)` moves
   right by `delta` (skipped when `strlen(buffer) < strlen(key)`). The value is
   then copied to the start without its terminator. Two records grow the
   string: `hbs` to `sr_Latn` and `tl` to `fil`.
2. **Scan.** From the start, letters are lowered, or raised once a `_` region
   has been seen; digits set a digit flag. Any other byte closes the current
   segment. Before the first separator, a digit run ends the scan. A segment
   that starts at a separator and is closed at index `i` is classified by the
   32-bit comparisons `offset + 3 == i` (two letters), `offset + 4 == i` with
   digits (three digits), `offset + 5 == i` without digits (four letters) and
   `offset + 1 == i` (an empty segment). Two letters after `_` are raised and
   recorded as the `_` region; after `-` they are raised and recorded as the `-`
   region if none was recorded. Three digits are recorded the same way without
   case changes. Four letters get an upper-case first letter. An empty `_`
   segment, like a `_` region, switches to upper case; every other classified
   or unclassified segment turns its separator into `-` and leaves upper case.
   Once upper case is on, the next closing records that segment's separator as
   the variant separator. The scan continues only while the closing byte is `_`
   or `-`, so the terminator, `@`, `.` and the like end it.
3. **Key/value string.** The key/value buffer is cleared. With a variant
   separator, `uloc_canonicalize(buffer, keyValue, 0x101, &status)` runs; on
   `status <= 0` the result is terminated at `min(length, 256)`, and if it
   contains `@`, the variant separator and the `_` bytes before it are cleared
   in the locale buffer and the key/value buffer is replaced by the text from
   the `@`; otherwise it is cleared again.
4. **Special cases**, in table order, each guarded by its language tag. With an
   upper-case replacement: the first `regionTag` occurrence whose fourth byte is
   not alphanumeric gets the replacement written after its `-`, and a `_`
   region equal to the tag gets it written over the region. With a lower-case
   replacement, the first test that holds — `_` region equals the tag, the tag
   occurs, `_` region equals the alternate tag, the alternate tag occurs —
   writes the replacement (or the alternate replacement) over the start of the
   buffer. Replacements are copied without terminators.
5. The `-` region pointer is stored through the second argument, then the `_`
   region pointer through the third. The function returns nothing the callers
   read.

**CoreFoundation defect** (0x1c01a0644, confidence verified): the backward walk
of step 3 clears and steps over `_` bytes without a lower bound. When every byte
before the variant separator is `_` (for example `"__X@a=b"`), it reads the
byte before the buffer and clears it if it is `_`, continuing backwards. The
differential probe establishes it natively on bytes the test owns: with `_`
before the buffer the native function clears it. The restoration stops at the
first byte of the buffer, so every result whose native walk stays inside the
buffer is unchanged. The iOS Device image has the same walk (0x1806a6e8c).

### `_RemoveSubstringsIfPresent`

For each run of graph bytes in the substring list: the run is copied to a
257-byte stack buffer and terminated, the following space bytes are skipped,
and the first occurrence of the run in the locale string is removed by shifting
the tail, terminator included. The callers ignore the result. Native
preconditions kept as read: a run longer than 256 bytes overflows the stack
buffer, and a list byte that is neither graph, space nor NUL is never consumed,
so the loop does not terminate. Every native caller passes a result string of
its own tables; neither case is exercised.

### `_AppendKeyValueString`

With a nonempty key/value string, `uloc_openKeywords(keyValue, &status)`; on a
non-NULL enumeration and `status <= 0`, each `uenum_next` keyword is read with
`uloc_getKeywordValue(keyValue, keyword, value, 96, &status)` and written with
`uloc_setKeywordValue(keyword, value, locale, 0x101, &status)` while
`status < 1`; the enumeration is then closed. One 32-bit slot receives the
`uenum_next` length and both results. A value of exactly 96 bytes would be
passed on unterminated if libicucore reports only a warning; that libicucore
behavior is not read, the native code is kept, and the case is not exercised.

## Dependencies and naming

| Dependency | Owner | Boundary |
| --- | --- | --- |
| `uloc_canonicalize`, `uloc_openKeywords`, `uenum_next`, `uloc_getKeywordValue`, `uloc_setKeywordValue`, `uenum_close` | libicucore (external; ordinary load command, already linked through `OF_CORE_FOUNDATION_LDFLAGS`) | C strings, 32-bit `UErrorCode`, opaque `UEnumeration`; prototypes declared locally because the SDK installs no ICU headers |
| `bsearch`, `strlen`, `strncmp`, `strcmp`, `strstr`, `__maskrune`, `__tolower`, `__toupper`, `_DefaultRuneLocale` | libsystem (external) | Raw values |

No CF or NS object crosses these functions. All seven functions and both tables
are non-external natively; the restoration keeps their native names, gives the
functions external linkage with hidden visibility so the internal test tools can
call them, keeps the tables `static`, and exports nothing. `LocaleStringMapping`,
`LocaleSpecialCase` and their field names are local descriptive spellings.

## iOS Device comparison

Independently read from the iOS Device capture:

- `_CompareTestEntryToTableEntryKey`: identical instructions.
- `_CompareTestEntryPrefixToTableEntryKey`, `_CompareLowerTestEntryPrefixToTableEntryKey`:
  rotated loops with pre-indexed loads; same comparisons and results.
- `_RemoveSubstringsIfPresent`: one counter serves as index and removal length
  and the space skip starts at the byte after the run; same copies, searches and
  removals, the same 257-byte buffer and the same unbounded run copy.
- `_AppendKeyValueString`: early return placed first, rotated loop; same calls,
  arguments, capacities and status tests.
- `_CheckForTag`: same `strncmp`, sign test and `__maskrune` call; the
  non-negative lookup is the shared outlined `_OUTLINED_FUNCTION_0_21`
  (0x18065c6b0, called at 0x1806a7150), `__runetype[c] & 0x500` through the
  same rune-locale GOT slot (0x1e016af20) as the inline lookups.
- `_UpdateFullLocaleString`: same `bsearch` arguments, replacement arithmetic,
  scan classification, canonicalization step (including the unbounded walk) and
  upper-case special-case path; the language-tag `_CheckForTag` is inlined
  (0x1806a6c48). The lower-case special-case path is outlined:
  `__UpdateFullLocaleString.cold.1` (0x1807bd6b0, called at 0x1806a6f90)
  returns whether the fourth byte of the first region-tag occurrence is not
  alphanumeric, and the caller then copies the replacement over the buffer;
  `__UpdateFullLocaleString.cold.2` (0x1807bd6f0, called at 0x1806a6f38 with the
  record, a NULL-region flag, the buffer and region + 1) tests the alternate
  tag against the `_` region, then by `strstr` and a non-alphanumeric fourth
  byte, and copies the alternate replacement over the buffer. The tests run in
  the iOS Simulator order with the same results.
- Both tables decode to the same records; chained-fixup metadata bits are not read.

## Verification and limits

- `Tests/CoreFoundation/Locale/LocaleIdentifierTests.c` resolves every native
  function by IDA offset from the pinned image and reads the native tables
  through the ADRL instructions at 0x1c01a0370 and 0x1c01a0678, so its inputs do
  not come from the restoration's tables. It compares, byte for byte over
  guarded 257-byte buffers: the three comparators for 8,192 derived and random
  test keys (with high-bit bytes) against every native key and 600 more;
  `_CheckForTag` over all 16^5 combinations of 16 bytes;
  `_UpdateFullLocaleString` over every key and value with 19 suffixes and
  upper-case variants, every special case with language/region forms, lengths
  at the 256-byte growth limit, leading-underscore identifiers and 400,000
  random identifiers, comparing both buffers and both output pointers; the
  underscore-walk defect probe; `_RemoveSubstringsIfPresent` over 300,000 random
  string/list pairs; `_AppendKeyValueString` over fixed and 60,000 random
  locale/keyword pairs with values of at most 40 bytes plus an over-long value.
  24,044,515 cases per prefixed configuration.
- `StandaloneTests/CoreFoundation/Locale/LocaleIdentifierStandaloneTests.c`
  checks 45 hand-derived cases, including the bounded walk.
- Not exercised: list runs over 256 bytes, list bytes outside graph/space,
  keyword values of exactly 96 bytes, and the native walk below a buffer outside
  the probe. libicucore behavior is supplied and not compared.
- Commands: `make test`, `make OF_SYSTEM_NAMES=YES test`, `make verify`.

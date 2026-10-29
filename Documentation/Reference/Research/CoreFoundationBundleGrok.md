# CoreFoundation bundle file-type sniffing

## Identity and scope

This reading restores the four helpers that `_CFBundleGrokFileType` and
`_CFBundleGrokMachTypeForFatFile` call to classify a file:
`_CFBundleGrokFileTypeForZipFile`, `_CFBundleGrokFileTypeForOLEFile`,
`_CFBundleGrokX11FromFile` and `_CFBundleGrokObjcImageInfoFromFile`, with the
three tables they read. They are level-0 functions of the scope, reached by the
consumers through the CFBundle group, and call only libsystem. It is not a
restoration of `_CFBundleGrokFileType`, `_CFBundleGrokBinaryType`,
`_CFBundleGrokMachTypeForFatFile`, `_CFBundleGrokObjCImageInfoFromMainExecutable`
or any other CFBundle function.

| Reference | Identity |
| --- | --- |
| iOS Simulator | iOS 27.0 (24A434), arm64; CoreFoundation UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; SHA-256 e3598ca3b1ed3649c1ff8aadce058e98aa2d7be5385036a5ebb6ab986a5cc829 |
| iOS Device | iOS 27.0.1 (24A446), arm64e; CoreFoundation UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; SHA-256 ba22deb3759f56d32e5847fec0a5e675837e189d9a000b87ffdefe88a7942166 |

The evidence is the IDA capture
`Local/Evidence/below-core-capture/batch2-iphone-simulator.json` and
`batch2-iphone-device.json`, group `bundle-grok`: function boundaries, complete
disassembly, Hex-Rays pseudocode, every caller with its call-site instructions,
and the bytes of each referenced datum up to the next named symbol. The
disassembly is the authority; the pseudocode located questions only. Linkage is
from `nm -m` on the canonical images. No IDA session was opened for this track.

| Native function | Linkage | iOS Simulator | Bytes | iOS Device | Bytes | Confidence |
| --- | --- | --- | --- | --- | --- | --- |
| `_CFBundleGrokFileTypeForZipFile` | non-external | 0x1c025d7c4 | 3,512 | 0x18077acd4 | 3,640 | verified |
| `_CFBundleGrokFileTypeForOLEFile` | non-external | 0x1c025e57c | 592 | 0x18077bb0c | 652 | verified |
| `_CFBundleGrokX11FromFile` | non-external | 0x1c025e7cc | 680 | 0x18077bd98 | 712 | verified |
| `_CFBundleGrokObjcImageInfoFromFile` | non-external | 0x1c025eedc | 1,392 | 0x18077c060 | 1,560 | verified |

| Native datum | Linkage | iOS Simulator | iOS Device | Bytes | Restored as | Confidence |
| --- | --- | --- | --- | --- | --- | --- |
| `_CFBundleGrokX11FromFile.libX11name` | non-external | 0x1c0456dd1 | 0x1809654d1 | 20 | function-local `libX11name` | verified |
| unnamed extension table | none | 0x1c0456de5 | 0x1809654e5 | 30 | `kSunExtensions[6][5]` | verified |
| unnamed extension table | none | 0x1c0456e04 | 0x180965504 | 45 | `kOpenDocumentExtensions[9][5]` | verified |

The two unnamed tables follow `libX11name` in `__const`: "sxc", "sxd", "sxg",
"sxi", "sxm", "sxw" and "odc", "odf", "odg", "oth", "odi", "odm", "odp", "ods",
"odt", each NUL-padded to five bytes; one padding byte at 0x1c0456e03 aligns the
second. Their lengths are the largest returned offsets (25 and 40) plus five;
both images agree byte for byte. The names, MIME strings and results are
`__cstring` literals whose bytes agree in both images. `libX11name` is 20 bytes
with its NUL: the name check reserves `sizeof` (20) bytes and compares 19.

The image's assertion strings name sources under
`CoreFoundation/PlugIn.subproj/` (`CFBundle.c`, `CFBundle_Binary.c`,
`CFBundle_Executable.c` and others). No string names the file of these
functions. The restoration places them in
`Source/CoreFoundation/PlugIn/OCFBundleGrok.c`: the directory follows the
recovered subproject; the file name is inferred.

## Native implementation

### Parameters

Parameter widths and signedness are those of the native register use and
comparisons; the names are descriptive. Every function takes a descriptor
(`int`; negative means none) and a byte buffer with its 64-bit length. The
buffer is read only when the descriptor is negative, when `lseek` does not
return the requested position, or, for the image-information words, when the
8-byte read fails. Flags are tested as 32-bit registers (`CBZ W`) and every
native caller passes 0 or 1; the restoration declares them `uint8_t`, which
Darwin arm64 widens to 32 bits.

| Function | Further arguments | Result | Native callers |
| --- | --- | --- | --- |
| Zip | `off_t fileLength` (signed) | extension string | `_CFBundleGrokFileType` at 0x1c025c96c |
| OLE | `off_t offset` (checked unsigned) | extension string | `_CFBundleGrokFileType` at 0x1c025c7b8: `((uint32_t)word at header + 0x30 << 9) + 512`, computed in 32 bits |
| X11 | `uint32_t offset`, `swapped`, `sixtyFour` | 0 or 1 (`CSET`; the callers store it with `STRB`) | `_CFBundleGrokFileType` (offset 0, four flag pairs), `_CFBundleGrokMachTypeForFatFile` (32-bit slice offset, four flag pairs) |
| Image information | `uint32_t offset`, `swapped`, `sixtyFour`, `uint8_t *`, `uint32_t *`, `uint32_t *` (ninth argument on the stack) | none | `_CFBundleGrokFileType` (offset 0; flags 1, 1 in the iOS Simulator, 0, 0 in the iOS Device), `_CFBundleGrokMachTypeForFatFile` (not swapped, 64-bit) |

The zip function's fourth argument is loaded before the captured call-site
context; the code uses it only as the end of the trailing 1,024-byte window.

Each allocation is `malloc_type_malloc` with the native size and type
identifier: 1,024 (zip) and 512 (OLE) bytes with 0x100004077774924, 4,096 bytes
with 0xe7329b55 (X11) and 0x298ae5f0 (image information). The restoration passes
the same literals. Every allocation is freed before return.

### Zip

1. With `length >= 31` (unsigned), offsets 0 to `length - 31` of `bytes` are
   scanned. "PK\3\4" is a local entry (30-byte header, name length at +26),
   "PK\1\2" a central entry (46 bytes, +28). An entry with
   `offset + header + name length <= length` (signed) is classified, and the scan
   resumes after its name.
2. A local entry named "mimetype" (eight bytes, case-insensitive) reads its data
   at `entry + 30 + name length (+26) + extra length (+28)`:
   - with 56 data bytes inside the buffer, method (+8) 0 and the prefix
     "application/vnd." or "application/x-vnd.", the kind after it (at 16 when
     byte 15 is '.', else 18) is tested for "sun.xml." followed by calc, draw,
     writer.global, impress, math or writer ("sxc".."sxw"), then for
     "oasis.opendocument." followed by chart, formula, graphics, text-web,
     image, text-master, presentation, spreadsheet or text ("odc".."odt"), in
     that order. A match returns the table entry; otherwise the scan continues.
   - Otherwise, with 41 bytes, method 8 and the data starting
     4b 2c 28 c8 c9 4c 4e 2c, the result becomes "odt" and the scan continues.
   - Otherwise "application/oebps-package+xml" (29 bytes inside) or
     "application/epub+zip" (20 bytes inside) returns "epub".
3. Other names set flags: length 9 "META-INF/", 11 "content.xml" or
   "_rels/.rels", 19 "[Content_Types].xml", 20 "META-INF/MANIFEST.MF", 21
   "META-INF/manifest.xml". A name of at least five bytes that matched none of
   these is tested by suffix: ".opf", ".sml", then (at least 6) ".smil", then
   (at least 8) "xl/" with ".xml", (at least 9) "ppt/", (at least 10) "word/",
   (at least 11) "excel/", (at least 16) "powerpoint/", each with ".xml". All
   comparisons are `strncasecmp_l` with a NULL locale.
4. If the scan did not return and `fileLength >= 1024`, the trailing window is
   the 1,024 bytes read at `fileLength - 1024`, or, when the descriptor cannot
   be positioned and `length >= 1024`, the last 1,024 bytes of `bytes`. A failed
   allocation or a short read skips it. Offsets 0..993 of the window are scanned
   as in step 1 (bound 1,024), with the flags of step 3 only.
5. The result is "jar" for MANIFEST.MF; otherwise, given rels or content types,
   "docx" (word), "xlsx" (xl/excel) or "pptx" (ppt/powerpoint, its flag tested
   on the low byte); then "odt" for content.xml or manifest.xml, "jar" for
   META-INF/, "dtb" for opf and smil, "oeb" for opf; else the current result
   ("zip", or "odt" from step 2).

### OLE

The 512 bytes at `offset` are read from the descriptor, or taken from `bytes`
when `length >= offset + 512` (unsigned). A failed allocation, short read or
check returns "ole". In each of the four 128-byte entries the byte at +64 is
halved as a signed char toward zero (the native `(b + (b >> 7)) >> 1` on the
sign-extended byte; the dispatch tests `b & 0xfe`): 5 "Book" and 9 "Workbook"
select "xls", 13 "WordDocument" "doc", 20 "PowerPoint Document" "ppt". That
count minus one characters are compared with the low bytes of the entry's 16-bit
units. The first matching entry decides; otherwise "ole". The native index
`(2 * i) & 0xfffffffe` equals `2 * i` for the reachable counts.

### X11

A 4,096-byte window at `offset` comes from the descriptor (a failed allocation
or short read returns 0) or from `bytes` when
`length >= (uint32_t)(offset + 4096)`. `ncmds` (+16) and `sizeofcmds` (+20) are
byte-swapped when `swapped`. Commands start at 32 (`sixtyFour`) or 28. The end
is the window end when `sizeofcmds` exceeds 4,064 (or 4,068), otherwise the
start plus `sizeofcmds`. For each command cmd is read; LC_LOAD_DYLIB (12; the
swapped path compares the raw word with 0x0c000000) reads the name offset (+8)
and, when the name plus 20 bytes lies within the end, compares 19 bytes with
`strncmp`. cmdsize (+4) is read for every command. The scan stops on a match,
after `ncmds` commands, or when the next start reaches the end.

### Objective-C image information

The window is obtained as for X11; a failed allocation, short read or check
leaves every output zero. LC_SEGMENT_64 (0x19) has nsects at +64 and 80-byte
sections from +72; LC_SEGMENT (1) has +48 and 68-byte sections from +56. A
segment with sections whose first section starts before the end compares each
section's segment name (+16) with "__DATA" or "__OBJC", then its name (+0) with
"__objc_imageinfo" or "__image_info", 16 bytes each with `strncmp`, segment
name first. A matching segment name sets the found byte. A section matching both
stops the scan with its size (the low 32 bits of the 64-bit field at +40, or the
32-bit field at +36) and 32-bit offset (+48, or +40). Sections advance while the
count and the end allow, commands while `ncmds`, an offset of at least the
header size and the end allow. With a size of at least 8 the two 32-bit words at
`offset + section offset` (a 32-bit sum) are read from the descriptor, or else
from `bytes` when `length >= (uint32_t)(offset + section offset + 8)`, swapped
when `swapped`; otherwise both are 0. The window is freed first; then each
non-NULL output receives the found byte, the first word and the second word. The
code does not interpret the words.

### CoreFoundation defects

`CoreFoundation defect:` confidence verified in both images.

- `_CFBundleGrokX11FromFile` (0x1c025e7cc; iOS Device 0x18077bd98) and
  `_CFBundleGrokObjcImageInfoFromFile` (0x1c025eedc; iOS Device 0x18077c060)
  check only that a load command or section starts before the end. A command
  then reads up to 12 bytes (X11) or 68 bytes (nsects of LC_SEGMENT_64) from its
  start, a section its two 16-byte names and its size and offset fields, up to
  52 bytes. With `sizeofcmds` above 4,064 the end is the window end, so a large
  load-command area reads past the 4,096-byte heap window or the caller's
  bytes.
- Both check the bytes-path window with `(uint32_t)(offset + 4096) <= length`
  and then read at the unwrapped `bytes + offset`; the image-information words
  are checked with `(uint32_t)(offset + section offset + 8)` and read at the
  unwrapped address. A wrapping sum passes the check and the read leaves the
  buffer.

The restoration never reads outside the window or `bytes`. A field that does
not fit ends the scan (X11 result 0; image information as if no section
matched). A name is compared over its readable bytes: when they contain a
difference or the literal's NUL the result is the native one, otherwise the
comparison counts as unequal. A window or image-information words that do not
fit fail their check. Every result decided by in-range bytes is unchanged; the
native results in these cases depend on bytes outside the buffer.

`_CFBundleGrokFileTypeForOLEFile` reads before `bytes` for a negative offset on
the bytes path. Its only caller passes a zero-extended 32-bit value, so no
native caller reaches it; the native behavior is kept and never tested. Every
read of the zip function is bounded by its checks.

## Dependencies and naming

`read`, `lseek`, `malloc_type_malloc`, `free`, `strncmp` and `strncasecmp_l`
are external libsystem cuts exchanging raw values; no CF or NS object crosses
the boundary. The functions keep their native non-external names, declared with
`OF_EXTERN` in `Source/CoreFoundation/PlugIn/OCFBundleGrok.h`. The product
builds with hidden visibility, so no process-shared symbol is added in either
naming mode. The tables are `static`; `libX11name` keeps its native
function-local name.

## iOS Device comparison

The iOS Device bodies were read and compared with the iOS Simulator bodies;
constants, literals, comparisons and table bytes agree.

- Zip (0x18077acd4): the MANIFEST.MF flag lives in a stack slot, signature bytes
  are indexed from the buffer base and blocks are placed differently.
- OLE (0x18077bb0c): the entry address is formed with `BFI` and the compared
  character count starts at one.
- X11 (0x18077bd98) and image information (0x18077c060): load-command and
  section offsets are compared instead of addresses, the next offset is formed
  before the result test and more state is kept on the stack; the defects above
  are the same.
- The callers differ: the iOS Device `_CFBundleGrokFileType` passes 0, 0 as the
  image-information flags where the iOS Simulator passes 1, 1. The callers are
  not restored.

## Verification and limits

The standalone suite (48 cases) runs the restoration on independently built
fixtures: mimetype kinds of each family, case variants, a compressed entry, the
deflated-text prefix, the docx/xlsx/pptx/jar/odt/dtb/oeb decisions, a trailing
window read from a file and a short file, OLE names with odd and signed length
bytes and nonzero high bytes, X11 names at the 20-byte bound and swapped 32-bit
images, image information relative to a slice offset, a 64-bit size above 2^32
and NULL outputs. The defect cases place a 4,096-byte window against a
`PROT_NONE` page: commands two and eight bytes before its end, sections 23, 20
and 48 bytes before its end (decided, undecided and unreadable fields) and both
wrapping checks. They never run against the native code.

The differential suite resolves the four native functions by IDA offset (image
base 0x1c014a000) in the UUID-pinned CoreFoundation image and the replacements
from the internal test sources (`checkInternalImplementation`); identical
addresses are a setup failure (77). Fixtures are written to a per-run `mkdtemp`
directory and passed with no descriptor, the file, an unseekable pipe and a
directory descriptor (reads fail). Extension strings are compared by content;
the X11 result and the image-information outputs, with canaries and every
NULL-output combination, byte for byte. It covers:

- 12,000 zip heads (0 to 4,000 bytes) of signature-dense random bytes with up to
  six entries, including entries clipped by the end, random name lengths,
  methods and extra fields and every mimetype kind, in files whose separate tail
  holds central entries, with four `fileLength` values;
- 6,000 OLE buffers (0 to 4,096 bytes) of random and named entries at ten
  offsets, including `length - 512`, `length - 511` and beyond the file;
- 12,000 Mach-O windows (swapped or not, 32- or 64-bit, slice offsets 0, 1,234,
  4,096 and 8,192, short files) with segment, dylib and other commands. Bounded
  windows (`sizeofcmds` at most 3,800, end bounds one byte before, at and after
  command starts and dylib-name ends, arbitrary command sizes) are also read
  with the other header width; open windows (end at the window end) keep exact
  command sizes below 3,100 bytes. Every generated byte that can be read as a
  section offset is below 0x80. No input reaches a native over-read or wrapping
  check.

The prefixed Debug run reports 702,927 comparisons (54,847 classified archives,
5,551 X11 matches, 16,598 found segments and 5,127 read image-information
words). Loosening the dylib-name bound by one byte makes it fail. Commands:
`make test`, `make OF_SYSTEM_NAMES=YES test` (standalone only), `make verify`.

Not tested: native behavior in the defect cases (outside the buffer by
definition), negative OLE offsets, `ncmds` values that loop billions of times
over a zero command size, and descriptors repositioned between the two
implementations' calls (each call positions absolutely). Not built: the callers
listed above and the rest of CFBundle.

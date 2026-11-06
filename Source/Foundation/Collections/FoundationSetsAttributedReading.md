<!--
  Created by ktiays on 2026/11/03.
  Copyright (c) 2026 ktiays. All rights reserved.
-->

# Foundation sets and attributed-string reading

This reading covers the 205 `FC-sets-attributed` entries supplied for cf-core batch 8. It specifies the
captured portions of NSIndexSet, NSMutableIndexSet, NSAttributedString, NSMutableAttributedString,
NSCharacterSet and NSMutableCharacterSet, their reached categories and plain functions, and two direct
NSMorphology methods. It does not specify complete classes or a complete Foundation scope.

## Reference and evidence

- MATCH: iOS Simulator, iOS 27.0 (24A434), arm64.
- Foundation UUID: `49FCDA5F-A93E-3259-857C-FC75AC2C7B03`.
- Foundation SHA-256: `bc7e0a5f693ffc75ae7c3311b817716e86d149fda2f8c45669a976727322cb18`.
- Canonical image: `Local/Databases/iphone-simulator/Foundation`. Its UUID and SHA-256 were checked.
- Implementation evidence: the assembly in
  `Local/Evidence/cf-core/iphone-simulator-foundation-link.json`, family `FC-sets-attributed`.
  These are the supplied IDA captures. Hex-Rays pseudocode is a reading aid, with instruction evidence
  taking precedence. No uncaptured implementation is inferred from another Foundation implementation.
- Static-data and linkage evidence: this image's Mach-O sections and `nm -m` metadata, read with
  `Local/Scratch/cf-core-fc/fmacho.py` and `classes.py`. Identical local symbol spellings are not unique:
  global blocks below were identified by virtual address and invoke-pointer value.
- CoreFoundation declarations below refer to iOS Simulator 24A434, UUID
  `D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC`.
- Confidence: verified for the captured instructions and in-image static records; medium bindings and
  unread boundaries are enumerated below. iOS Device divergence: every function in this reading remains
  unread on iOS Device 27.0.1 (24A446). Simulator results do not establish iOS Device behavior.

The source layout is inferred from native unit boundaries and assertion file names. C functions preserve
native boundaries and address order within each unit; blocks are block literals at their parent call sites.
The compiler emits the two byref helpers, rather than a second handwritten block runtime.

## Class layout

| Class | Class object | class_ro_t | Superclass | Instance start / size | Ivars |
| --- | --- | --- | --- | --- | --- |
| NSIndexSet | `0x22e6d97c0` | `0x222852ce0` | NSObject | 8 / 32 | `_indexSetFlags` at 8, size/alignment 8; `_internal` at 16, size 16 |
| NSMutableIndexSet | `0x22e6d9888` | `0x222852d70` | NSIndexSet | 32 / 32 | None of its own |
| NSAttributedString | `0x22e6d6138` | `0x222847ff8` | NSObject | 8 / 8 | None |
| NSMutableAttributedString | `0x22e6d61b0` | `0x222848160` | NSAttributedString | 8 / 8 | None of its own |
| NSCharacterSet | `0x22e6d6958` | `0x222849a98` | NSObject | 8 / 8 | None |
| NSMutableCharacterSet | `0x22e6d6980` | `0x222849b68` | NSCharacterSet | 8 / 8 | None of its own |
| NSMorphology | `0x22e6d6408` | `0x222848860` | NSObject | 8 / 96 | `_customPronouns` at 8; ten NSInteger fields at 16 through 88 |

NSIndexSet's low flag bits are `_hasSingleRange` and `_hasBitfield`, followed by 30 reserved bits.
The eight-byte declaration produces 32-bit flag loads/stores in the inspected compiler IR, matching the
capture. `_internal` is a union of an inline NSRange, a data/reserved-pointer pair, and a 64-bit bitfield.
The heap range-storage header is 64 bytes: capacity, range count, index count, first live range, cached
index, cached range, cached offset and cache-owner thread, followed by NSRange slots. All fields are
64-bit; the cache-owner loads and exchanges preserve the captured atomic operations.

NSMorphology's features, in order, are `_adjectival`, `_clusivity`, `_grammaticalGender`, `_partOfSpeech`,
`_number`, `_grammaticalCase`, `_determination`, `_grammaticalPerson`, `_pronounType`, `_definiteness`.
Its class object and other methods belong to the coordinator's other groups. Only mergeMorphology:override:
and isIdentity are defined here; compiler IR confirms their nil-receiver return and NO behavior.

There are no unread ivars in these seven declared layouts. Uncaptured dependency classes have selector-only
or forward declarations, never fabricated ivars. Protocol conformances and property lists outside the
captured scope are not asserted. The static 16-byte placeholder-index-set instance at `0x2227d48b8` has its
class pointer and a zero second word; the second word's semantic type remains unread (medium confidence).
The 48-byte Markdown UTF-8 offset cache is an opaque caller buffer, not a class layout; only its six initial
words (0, 0, 0, 0, 1, 1) are known.

## Recorded native defects and precise divergences

All six divergences below are established from the supplied instruction capture, confidence verified.
Known destructive native paths must not be executed in differential or reference traces.

| Native function | Evidence | Divergence |
| --- | --- | --- |
| `-[NSIndexSet initWithCoder:]`, `0x1c0528da0` | Missing NSRangeData releases the temporary at `0x1c0528fb0`, then releases it again at `0x1c0529034` after the common nil-init path | After failing the coder, return nil without the second release or redundant nil message |
| `-[NSIndexSet rangeAtIndex:]`, `0x1c052ab4c` | `B.LS` at `0x1c052abb4` and `B.HI` at `0x1c052ac64` accept index == rangeCount | Reject equality for heap storage and nonempty inline storage, preventing an unused range read or a read beyond the object; keep empty inline index 0, whose slot is valid |
| `-[NSMutableIndexSet _removeRangeInArrayAtIndex:]`, `0x1c052cf60` | `B.HI` at `0x1c052d000` accepts equality; the heap path then reads an unused slot and can underflow memmove length | Reject equality only for the unsafe heap path; keep the inline path that clears its valid length slot even for the accepted equality index |
| `-[NSMutableIndexSet _replaceRangeInArrayAtIndex:withRange:]`, `0x1c052d0ec` | `B.HI` at `0x1c052d174` accepts equality, writing an unused heap slot or beyond a nonempty inline object | Reject those unsafe equality writes; keep empty inline index 0, which writes its valid slot |
| `_NSAttributedStringFromMarkdownWithCreator`, `0x1c0601d2c` | Null-document path at `0x1c0601f14` omits cmark_parser_free | Free the successfully allocated parser before the native error construction/return |
| `_NSReadMutableAttributedStringWithCoder`, `0x1c04c4280` | `0x1c04c4490-0x1c04c4498` overwrites the previous heap pointer on reallocation failure; NSReallocateObjectArray retains that allocation on failure | Free the old buffer on failure before returning the native failure result |

## Well-defined unusual behavior kept

- `-[NSIndexSet rangeAtIndex:]` keeps empty inline index 0, and the mutable remove/replace methods keep
  the safe inline equality cases described above. The fixes are not blanket stricter bounds checks.
- `__NSIndexSetChunkIterate`, `0x1c052edb8`, pushes a pool and pops it only on a stopped or successful-test
  early exit. Exhaustion and an empty range chunk return without popping; pending objects remain in the
  thread's enclosing pool. No evidence of a permanent leak is assumed from the unusual pool nesting.
- `_indexClosestToIndex:equalAllowed:following:`, `0x1c0529fd4`, wraps ADD before signed tests. The source
  performs unsigned addition before interpreting the signed result, avoiding C signed-overflow assumptions.
- `_NSGetUnsignedInt`, `0x1c0fdcbd0`, keeps the native recursive varint reader and its original checks.
- `-[NSAttributedString attributedSubstringFromRange:]`, `0x1c04c31cc`, uses a full-range copy fast path;
  the shorter path produces an autoreleased mutable concrete string. Dictionary identity does not bypass
  dictionary equality in `isEqualToAttributedString:`, `0x1c04c355c`.
- The attributed-string keyed decoder keeps unsigned MUL/LSL wrapping and checks addition carry, without
  adding a fitted maximum varint length or changing its malformed-input policy.
- `addAttribute:value:range:`, `0x1c04c4fa4`, and `addAttributes:range:`, `0x1c04c5134`, return for an empty
  range before validating a nil value/dictionary. Editing and intent-marker calls keep native order; no
  exception cleanup or self-replacement snapshot is added. Abstract primitives, beginEditing and endEditing
  keep their captured reporting or empty bodies; these are native implementations, not dependency stubs.
- `NSCharacterSet` bitmap generation, `0x1c04d8fe0`, visits U+0000 through U+FFFE, omitting U+FFFF.
  `longCharacterIsMember:`, `0x1c04d8ea8`, truncates to 16 bits before the native character-member message.
  Mutable abstract methods log and send doesNotRecognizeSelector:; no set implementation is invented.
- `_NSInflect`'s add-word-attributes block, `0x1c05327e0`, merges the noun twice on its noun/adjective path,
  first with override YES and then NO. Both inflectable/formattable enumerators (`0x1c0533edc`,
  `0x1c0534154`) retain the final span's zero location, including a nonzero requested start.
- `_NSLexiconInflectedStringUsingMorphun`, `0x1c057f55c`, ignores its length argument, preserves the first
  nonwhitespace offset across prefix replacements, and retains the native nil downgrade when the loaded
  provider function is absent. At `0x1c057fcc4`, a nonnil print result is still autoreleased/returned with
  an error; error release follows all three captured engine cleanup calls.
- `NSReallocateObjectArray`, `0x1c0593bd8`, returns NULL without freeing the old allocation on failure. It
  copies min(malloc_size(old), requested bytes), zeros the remaining good-size allocation and frees the old
  buffer only on success. Count zero and high-bit rejection preserve the captured allocator behavior.

## Assembly and pseudocode disagreements

- `NSReallocateObjectArray`, `0x1c0593bd8`: Hex-Rays prints a void return; assembly at `0x1c0593c70` returns
  the new pointer in X0.
- `_NSReadMutableAttributedStringWithCoder`, `0x1c04c4280`: Hex-Rays omits the reallocation count argument;
  X1 carries the new capacity. The pointer result is significant.
- `-[NSAttributedString(NSAttributedStringPortCoding) replacementObjectForPortCoder:]`, `0x1c064e3a0`:
  native RET leaves self in X0, although the pseudocode prints a void function.
- `-[NSAttributedString(NSAttributedStringFormattingSPI)
  __initWithFormat:attributeOptions:formattingOptions:locale:context:arguments:]`, `0x1c0548b8c`:
  the first attribute enumeration's range, options and block arguments are incomplete in pseudocode;
  assembly supplies {sourceCursor, span}, 0x100000, and the block.
- Zone arguments omitted from some copy/mutable-copy pseudocode calls remain live in X2 at the message
  boundary. The source passes the captured zone instead of replacing it with NULL; explicit NULL calls
  remain NULL.
- The rangeAtIndex branch at `0x1c052abb4` is B.LS, not an imagined B.HI bounds rejection. The safe empty
  inline case and unsafe nonempty case must be distinguished as above.

## Medium-confidence shared-cache bindings

These addresses are GOT slots or selector values outside the extracted image. Their names follow call use
and matching imported symbol metadata; inaccessible bytes are not treated as verified symbol bindings.

| Binding | Slot / address | Units |
| --- | --- | --- |
| NSException | `0x22227fae0` | IndexSet, CharacterSet, attributed base/mutable, object allocation |
| NSGenericException | `0x22227f4e0` | CharacterSet, object allocation |
| NSInvalidArgumentException | `0x22227f500` | IndexSet, mutable attributed string |
| NSRangeException | `0x22227f5e0` | IndexSet, attributed base |
| NSArray | `0x22227fa90` | Inflection |
| NSData | `0x22227fab8` | IndexSet, CharacterSet, Markdown, attributed base |
| NSDictionary | `0x22227fad0` | All collection/attribute helper units |
| NSLocale | `0x22227fb00` | Inflection, formatting |
| NSMutableArray | `0x22227fb18` | Inflection, Markdown, attributed base |
| NSMutableData | `0x22227fb20` | IndexSet, CharacterSet, attributed base |
| NSMutableDictionary | `0x22227fb28` | Inflection |
| NSNull | `0x22227fb40` | Inflection |
| NSSet | `0x22227fb68` | Attributed base |
| NSURL | `0x22227fb88` | Markdown |
| __NSArray0__struct | `0x2222807b0` | Inflection |
| __NSDictionary0__struct | `0x2222807b8` | Markdown, attributed base; imported symbol also confirmed by nm |
| __kCFBooleanFalse / __kCFBooleanTrue | `0x2222807e0` / `0x2222807e8` | Inflection, intent dirtying |
| kCFAllocatorSystemDefault | `0x222280b70` | Formatting |
| _kCFStringFormatMetadataAddedIsolatesKey | `0x222280890` | Formatting |
| _kCFStringFormatMetadataArgumentObjectKey | `0x2222808a0` | Formatting |
| _kCFStringFormatMetadataReplacementIndexKey | `0x2222808a8` | Formatting |
| _kCFStringFormatMetadataReplacementRangeLengthKey / LocationKey | `0x2222808b0` / `0x2222808b8` | Formatting |
| _kCFStringFormatMetadataSpecifierRangeLengthInFormatStringKey / LocationInFormatStringKey | `0x2222808c0` / `0x2222808c8` | Formatting |
| objc_debug_tag60_permutations / objc_debug_taggedpointer_obfuscator | `0x222409408` / `0x222409420` | IndexSet; supplied by libobjc |
| Active processor count commpage byte | `0xfffffc034` | IndexSet concurrent enumeration; descriptive name _COMM_PAGE_ACTIVE_CPUS |
| bitmapRepresentation selector | `0x2387ad760` | CharacterSet respondsToSelector: test |
| finalizeWithLanguageIdentifier: selector | `0x2387b599c` | Markdown respondsToSelector: test |

CharacterSet's super sends at selectors `0x2387af76f` and `0x2387d34a3` are inferred as initWithCoder: and
replacementObjectForPortCoder: from their own method boundary (high confidence, still outside-image bytes).
The placeholder-index-set zero word's type remains medium, as described under layout.

## Unread dependencies and integration cuts

The provided 205-entry capture does not include the dependencies below. They are declared and left undefined;
no fitted success, empty object, permanently disabled flag or system-Foundation fallback implements them.
The groups remain open. A build requiring these groups is unsupported until the coordinator supplies or
classifies the missing boundary. The mechanical acceptance allow list does not close these cuts.

| Boundary | Native evidence | Required next evidence |
| --- | --- | --- |
| Attributed decode class-set initializer | Invoke `0x1c04c7b64`; global block `0x22281c378`, descriptor `0x2227d03f8`; strict/ordinary sets/predicate at `0x22e76c0e0`, `0x22e76c0e8`, `0x22e76c0f0` | Capture initializer, its callees and exact class sets; supply the three declared state fields |
| Morphun soft-link loader | Invoke `0x1c0582224`; global block `0x22281cdd8`; predicate `0x22e76c840`; eleven pointers `0x22e76c710-0x22e76c760` | Capture loader, library identity/load strength, symbol resolution and ownership; supply typed table and predicate |
| Inflection log initializer | Invoke `0x1c0fdcb88`; global block `0x22281c978`; log/predicate `0x22e776208`, `0x22e776210` | Capture initializer and actual log subsystem/category; current inline call order is preserved |
| Swift character-set feature initializer | Invoke `0x1c0fdc9cc`; global block `0x22281c8d8`, descriptor `0x22281c8b8`; common state `0x22e7761f8`, `0x22e776200` | Capture Foundation's initializer; do not assume CoreFoundation's similar check is identical |
| Markdown initializer | Invoke `0x1c0602df4`; global block `0x22281d3c0` | Capture extension registration and all reached calls |
| Tagged-index-set flag writer | `_NSTaggedIndexSetAllowed` byte `0x22e6fcb5a` | Capture writer and tagged-pointer registration before running index-set traces |
| Inflection observer setter | `_NSInflectionObserver` at `0x22e6fcb60` | Capture setter/lifetime/thread boundary; no nil-only replacement |
| Concrete, placeholder, proxy, Swift character-set and other dependency classes | Forward/selector-only interfaces in ONSUnrestoredInterfaces.h; NSMorphology full layout separately in Inflection.h | Owner headers and complete reached dependency groups; no class implementation emitted here |
| CoreFoundation functions and formatting keys absent on this branch | Temporary declarations in ONSCharacterSet.h, Inflection.h, Formatting.h | Coordinator restores definitions and moves declarations to the existing subsystem owners |
| Foundation attribute-name data | Native addresses listed in ONSUnrestoredInterfaces.h | Coordinator supplies exact native constant objects and owner declarations |

Each global block's in-image record is verified: flags 0x50000000, size 32, signature v8@?0, no layout,
with descriptor reserved word 0x50000000. Those records reference the unread invoke without implementing it.
Inflection's `_NSInflectionLog` is an inlined once-test/load expansion sharing the native common state, not
an invented external function. The two other native private data symbols above use explicit asm aliases so
Release cannot constant-fold their paths using a fitted local zero.

The native Morphun table's eleven caller-side signatures are recovered from arguments/results and its
error strings. The captured caller performs the native log/error/cleanup order. Library setup and ownership
outside those calls remain unread. The error log packs its 22-byte native argument record explicitly. The inflection debug log at
`0x1c0530fbc-0x1c0531000` likewise passes its exact 22-byte record (header 0x08400202; object at 4,
descriptor 0x0840 at 12, object at 14) directly to _os_log_debug_impl, preserving the native enabled
check and avoiding an additional compiler-generated helper shared with another translation unit.

libcmark-gfm is an external supplied dependency, not restored Foundation behavior. The pinned image loads
`/usr/lib/libcmark-gfm.dylib` with LC_LOAD_DYLIB and delay-init (current version 1.0.0). Its actual functions
and exported node-type data are declared in ONSAttributedStringMarkdown.h; no implementation is imported.
libobjc is reexported by the native image. ICU uses the native C interface and its existing project boundary;
no new Foundation/CoreFoundation dynamic dependency is introduced.

Compiler-generated stack-block copy/dispose metadata needs integration validation. The capture's 0xc0000000
blocks have no copy/dispose helper flag; Clang emits additional helpers for some source literals under the
current MRC build. Synchronous invocation follows the captured bodies, but copied-block ownership/metadata
has not been validated and is not claimed equivalent. The compiler emits the native byref assign/dispose
operations with flags 131/131; direct-method nil guards were separately inspected in IR. Runtime traces,
ASan and leak/allocation checks are still required after the first combined link. No destructive native
defect should be triggered by those tests.

## Captured native entries by implementation file

All 205 entries are accounted for below: 147 methods, 23 plain functions, 33 block invokes folded into
parents, and two compiler-emitted byref helpers. Each address is cited in exactly one implementation file.
The table identifies source coverage of the supplied family; it is not a behavioral-test coverage count.

### `Source/Foundation/Collections/ONSIndexSet.m` (59 entries)

| Address | Native entry | Source form |
| --- | --- | --- |
| `0x1c0527ddc` | `+[NSIndexSet _alloc]` | Method |
| `0x1c0527e40` | `+[NSIndexSet allocWithZone:]` | Method |
| `0x1c0527ec8` | `+[NSIndexSet indexSet]` | Method |
| `0x1c0528264` | `-[NSIndexSet _init]` | Method |
| `0x1c05282d4` | `-[NSIndexSet init]` | Method |
| `0x1c05282d8` | `-[NSIndexSet initWithIndex:]` | Method |
| `0x1c0528324` | `-[NSIndexSet initWithIndexesInRange:]` | Method |
| `0x1c052842c` | `-[NSIndexSet _setContentToContentFromIndexSet:]` | Method |
| `0x1c0528740` | `__isEmpty` | Function |
| `0x1c05287ac` | `-[NSIndexSet initWithIndexSet:]` | Method |
| `0x1c0528a44` | `-[NSIndexSet initWithBitfield:]` | Method |
| `0x1c0528af8` | `-[NSIndexSet dealloc]` | Method |
| `0x1c0528b74` | `+[NSIndexSet supportsSecureCoding]` | Method |
| `0x1c0528b7c` | `-[NSIndexSet encodeWithCoder:]` | Method |
| `0x1c0528da0` | `-[NSIndexSet initWithCoder:]` | Method |
| `0x1c052912c` | `-[NSIndexSet classForCoder]` | Method |
| `0x1c0529138` | `+[NSIndexSet _subclassesMustBeExplicitlyMentionedWhenDecoded]` | Method |
| `0x1c052934c` | `-[NSIndexSet _indexOfRangeBeforeOrContainingIndex:]` | Method |
| `0x1c0529544` | `___51-[NSIndexSet _indexOfRangeBeforeOrContainingIndex:]_block_invoke` | Block in parent |
| `0x1c05295b8` | `-[NSIndexSet _indexOfRangeAfterOrContainingIndex:]` | Method |
| `0x1c05297b8` | `___50-[NSIndexSet _indexOfRangeAfterOrContainingIndex:]_block_invoke` | Block in parent |
| `0x1c05297fc` | `-[NSIndexSet hash]` | Method |
| `0x1c0529848` | `-[NSIndexSet isEqualToIndexSet:]` | Method |
| `0x1c0529cf8` | `-[NSIndexSet isEqual:]` | Method |
| `0x1c0529d58` | `-[NSIndexSet copyWithZone:]` | Method |
| `0x1c0529dd0` | `-[NSIndexSet mutableCopyWithZone:]` | Method |
| `0x1c0529dfc` | `-[NSIndexSet count]` | Method |
| `0x1c0529e80` | `-[NSIndexSet firstIndex]` | Method |
| `0x1c0529f28` | `-[NSIndexSet lastIndex]` | Method |
| `0x1c0529fd4` | `-[NSIndexSet _indexClosestToIndex:equalAllowed:following:]` | Method |
| `0x1c052a3d8` | `-[NSIndexSet indexGreaterThanIndex:]` | Method |
| `0x1c052a3e8` | `-[NSIndexSet indexLessThanIndex:]` | Method |
| `0x1c052a3f8` | `-[NSIndexSet indexGreaterThanOrEqualToIndex:]` | Method |
| `0x1c052a418` | `-[NSIndexSet getIndexes:maxCount:inIndexRange:]` | Method |
| `0x1c052a760` | `-[NSIndexSet __getContainmentVector:inRange:]` | Method |
| `0x1c052aa48` | `-[NSIndexSet rangeCount]` | Method |
| `0x1c052ab00` | `__bitfieldSetRangesCount` | Function |
| `0x1c052ab4c` | `-[NSIndexSet rangeAtIndex:]` | Method |
| `0x1c052acb8` | `-[NSIndexSet getBitfield:]` | Method |
| `0x1c052b53c` | `-[NSIndexSet description]` | Method |
| `0x1c052b890` | `___NSIndexSetEnumerateBitfield` | Function |
| `0x1c052bbf4` | `___NSINDEXSET_IS_CALLING_OUT_TO_A_RANGE_BLOCK__` | Function |
| `0x1c052bc50` | `___NSIndexSetParameterCheckIterate` | Function |
| `0x1c052bd5c` | `___NSIndexSetEnumerate` | Function |
| `0x1c052c948` | `-[NSIndexSet enumerateRangesUsingBlock:]` | Method |
| `0x1c052ca5c` | `-[NSMutableIndexSet classForCoder]` | Method |
| `0x1c052ca68` | `-[NSMutableIndexSet _ensureRangeCapacity:]` | Method |
| `0x1c052ccb8` | `-[NSMutableIndexSet _addRangeToArray:]` | Method |
| `0x1c052ccf8` | `-[NSMutableIndexSet _insertRange:inArrayAtIndex:]` | Method |
| `0x1c052cf60` | `-[NSMutableIndexSet _removeRangeInArrayAtIndex:]` | Method |
| `0x1c052d0ec` | `-[NSMutableIndexSet _replaceRangeInArrayAtIndex:withRange:]` | Method |
| `0x1c052dc8c` | `-[NSMutableIndexSet _mergeOverlappingRangesStartingAtIndex:]` | Method |
| `0x1c052de64` | `-[NSMutableIndexSet _convertToUsingRanges]` | Method |
| `0x1c052dff4` | `-[NSMutableIndexSet addIndexesInRange:]` | Method |
| `0x1c052e2e4` | `-[NSMutableIndexSet addIndex:]` | Method |
| `0x1c052ed40` | `_____NSIndexSetEnumerate_block_invoke` | Block in parent |
| `0x1c052edb8` | `___NSIndexSetChunkIterate` | Function |
| `0x1c064e664` | `-[NSIndexSet(NSIndexSetPortCoding) replacementObjectForPortCoder:]` | Method |
| `0x1c0fdcbd0` | `__NSGetUnsignedInt` | Function |

### `Source/Foundation/Collections/ONSObjectArray.m` (2 entries)

| Address | Native entry | Source form |
| --- | --- | --- |
| `0x1c0593b60` | `_NSAllocateObjectArray` | Function |
| `0x1c0593bd8` | `_NSReallocateObjectArray` | Function |

### `Source/Foundation/String/ONSAttributedString.m` (22 entries)

| Address | Native entry | Source form |
| --- | --- | --- |
| `0x1c04c3150` | `+[NSAttributedString allocWithZone:]` | Method |
| `0x1c04c31a0` | `-[NSAttributedString attribute:atIndex:effectiveRange:]` | Method |
| `0x1c04c31cc` | `-[NSAttributedString attributedSubstringFromRange:]` | Method |
| `0x1c04c34e0` | `-[NSAttributedString length]` | Method |
| `0x1c04c34f4` | `-[NSAttributedString hash]` | Method |
| `0x1c04c3508` | `-[NSAttributedString isEqual:]` | Method |
| `0x1c04c355c` | `-[NSAttributedString isEqualToAttributedString:]` | Method |
| `0x1c04c3698` | `-[NSAttributedString mutableCopyWithZone:]` | Method |
| `0x1c04c36c4` | `-[NSAttributedString copyWithZone:]` | Method |
| `0x1c04c36f0` | `-[NSAttributedString description]` | Method |
| `0x1c04c37f8` | `-[NSAttributedString attributesAtIndex:longestEffectiveRange:inRange:]` | Method |
| `0x1c04c391c` | `-[NSAttributedString attribute:atIndex:longestEffectiveRange:inRange:]` | Method |
| `0x1c04c3a64` | `-[NSAttributedString enumerateAttributesInRange:options:usingBlock:]` | Method |
| `0x1c04c3c54` | `-[NSAttributedString enumerateAttribute:inRange:options:usingBlock:]` | Method |
| `0x1c04c3e50` | `+[NSAttributedString _subclassesMustBeExplicitlyMentionedWhenDecoded]` | Method |
| `0x1c04c3e58` | `-[NSAttributedString classForCoder]` | Method |
| `0x1c04c3e64` | `-[NSAttributedString encodeWithCoder:]` | Method |
| `0x1c04c4210` | `-[NSAttributedString initWithCoder:]` | Method |
| `0x1c04c4280` | `__NSReadMutableAttributedStringWithCoder` | Function |
| `0x1c04c4814` | `+[NSAttributedString supportsSecureCoding]` | Method |
| `0x1c0fe2098` | `-[NSAttributedString string]` | Method |
| `0x1c0fe20d8` | `-[NSAttributedString attributesAtIndex:effectiveRange:]` | Method |

### `Source/Foundation/String/ONSAttributedStringAdditions.m` (7 entries)

| Address | Native entry | Source form |
| --- | --- | --- |
| `0x1c04c481c` | `-[NSAttributedString(_NSPresentationIntentDirtying) _mayRequireIntentResolution]` | Method |
| `0x1c04c4840` | `-[NSAttributedString(_NSPresentationIntentDirtying) _markRequiringIntentResolution]` | Method |
| `0x1c04c486c` | `-[NSAttributedString(_NSPresentationIntentDirtying) _willRequireIntentResolutionWhenContainingAttributes:]` | Method |
| `0x1c04c48d4` | `-[NSAttributedString(_NSPresentationIntentDirtying) _willRequireIntentResolutionWhenContainingAttribute:value:]` | Method |
| `0x1c04d1d44` | `-[NSAttributedString(NSCFAdditions) _cfTypeID]` | Method |
| `0x1c04d1d48` | `-[NSAttributedString(NSCFAdditions) _createAttributedSubstringWithRange:]` | Method |
| `0x1c064e3a0` | `-[NSAttributedString(NSAttributedStringPortCoding) replacementObjectForPortCoder:]` | Method |

### `Source/Foundation/String/ONSAttributedStringFormatting.m` (7 entries)

| Address | Native entry | Source form |
| --- | --- | --- |
| `0x1c0548b6c` | `-[NSAttributedString(NSAttributedStringFormattingSPI) _initWithFormat:options:locale:arguments:]` | Method |
| `0x1c0548b7c` | `-[NSAttributedString(NSAttributedStringFormattingSPI) __initWithFormat:attributeOptions:formattingOptions:locale:arguments:]` | Method |
| `0x1c0548b8c` | `-[NSAttributedString(NSAttributedStringFormattingSPI) __initWithFormat:attributeOptions:formattingOptions:locale:context:arguments:]` | Method |
| `0x1c0549494` | `__NSAttributedStringDescriptionWithLocaleFunc` | Function |
| `0x1c05494e4` | `___132-[NSAttributedString(NSAttributedStringFormattingSPI) __initWithFormat:attributeOptions:formattingOptions:locale:context:arguments:]_block_invoke` | Block in parent |
| `0x1c05495cc` | `___132-[NSAttributedString(NSAttributedStringFormattingSPI) __initWithFormat:attributeOptions:formattingOptions:locale:context:arguments:]_block_invoke_2` | Block in parent |
| `0x1c05496b0` | `___132-[NSAttributedString(NSAttributedStringFormattingSPI) __initWithFormat:attributeOptions:formattingOptions:locale:context:arguments:]_block_invoke_3` | Block in parent |

### `Source/Foundation/String/ONSAttributedStringInflection.m` (41 entries)

| Address | Native entry | Source form |
| --- | --- | --- |
| `0x1c04bb12c` | `-[NSMorphology mergeMorphology:override:]` | Method |
| `0x1c04bb398` | `-[NSMorphology isIdentity]` | Method |
| `0x1c052fb64` | `-[NSAttributedString(NSInflection) _firstValueOfAttributeWithKey:inRange:]` | Method |
| `0x1c052fc50` | `___74-[NSAttributedString(NSInflection) _firstValueOfAttributeWithKey:inRange:]_block_invoke` | Block in parent |
| `0x1c052fc8c` | `-[NSAttributedString(NSInflection) _inflectedAttributedString]` | Method |
| `0x1c052fcd8` | `-[NSAttributedString(NSInflection) _identicalAttributesInRange:]` | Method |
| `0x1c052fdd4` | `___64-[NSAttributedString(NSInflection) _identicalAttributesInRange:]_block_invoke` | Block in parent |
| `0x1c0530060` | `__NSInflect` | Function |
| `0x1c053032c` | `____NSInflect_block_invoke` | Block in parent |
| `0x1c05303d4` | `____NSInflect_block_invoke_2` | Block in parent |
| `0x1c053047c` | `____NSInflect_block_invoke_3` | Block in parent |
| `0x1c05327e0` | `____NSInflect_block_invoke.24` | Block in parent |
| `0x1c0532a9c` | `____NSInflect_block_invoke.27` | Block in parent |
| `0x1c0532cec` | `__NSInflectionGrammarWithLensAndRange` | Function |
| `0x1c0532da0` | `____NSInflect_block_invoke.35` | Block in parent |
| `0x1c0532ee4` | `____NSInflect_block_invoke.39` | Block in parent |
| `0x1c0532f48` | `____NSInflect_block_invoke.41` | Block in parent |
| `0x1c0533100` | `____NSInflect_block_invoke.45` | Block in parent |
| `0x1c05332c4` | `-[NSMutableAttributedString(NSInflection) _addAttributesIfNotPresentMergingInlinePresentationIntents:toRange:]` | Method |
| `0x1c053339c` | `___110-[NSMutableAttributedString(NSInflection) _addAttributesIfNotPresentMergingInlinePresentationIntents:toRange:]_block_invoke` | Block in parent |
| `0x1c0533528` | `-[NSMutableAttributedString(NSInflection) _formatInflectionAlternative:withReplacements:]` | Method |
| `0x1c0533928` | `-[NSMutableAttributedString(NSInflection) _inflectWithLocale:replacements:concepts:preflight:]` | Method |
| `0x1c0533b64` | `___94-[NSMutableAttributedString(NSInflection) _inflectWithLocale:replacements:concepts:preflight:]_block_invoke` | Block in parent |
| `0x1c0533b6c` | `___94-[NSMutableAttributedString(NSInflection) _inflectWithLocale:replacements:concepts:preflight:]_block_invoke_2` | Block in parent |
| `0x1c0533b88` | `___94-[NSMutableAttributedString(NSInflection) _inflectWithLocale:replacements:concepts:preflight:]_block_invoke_3` | Block in parent |
| `0x1c0533b9c` | `___94-[NSMutableAttributedString(NSInflection) _inflectWithLocale:replacements:concepts:preflight:]_block_invoke_4` | Block in parent |
| `0x1c0533bb0` | `___94-[NSMutableAttributedString(NSInflection) _inflectWithLocale:replacements:concepts:preflight:]_block_invoke_5` | Block in parent |
| `0x1c0533bc4` | `___94-[NSMutableAttributedString(NSInflection) _inflectWithLocale:replacements:concepts:preflight:]_block_invoke_6` | Block in parent |
| `0x1c0533c84` | `___94-[NSMutableAttributedString(NSInflection) _inflectWithLocale:replacements:concepts:preflight:]_block_invoke_7` | Block in parent |
| `0x1c0533c98` | `___94-[NSMutableAttributedString(NSInflection) _inflectWithLocale:replacements:concepts:preflight:]_block_invoke_8` | Block in parent |
| `0x1c0533cc8` | `___94-[NSMutableAttributedString(NSInflection) _inflectWithLocale:replacements:concepts:preflight:]_block_invoke_9` | Block in parent |
| `0x1c0533d74` | `___94-[NSMutableAttributedString(NSInflection) _inflectWithLocale:replacements:concepts:preflight:]_block_invoke_10` | Block in parent |
| `0x1c0533d84` | `___94-[NSMutableAttributedString(NSInflection) _inflectWithLocale:replacements:concepts:preflight:]_block_invoke_11` | Block in parent |
| `0x1c0533eb4` | `___94-[NSMutableAttributedString(NSInflection) _inflectWithLocale:replacements:concepts:preflight:]_block_invoke_12` | Block in parent |
| `0x1c0533ecc` | `___94-[NSMutableAttributedString(NSInflection) _inflectWithLocale:replacements:concepts:preflight:]_block_invoke_13` | Block in parent |
| `0x1c0533edc` | `-[NSMutableAttributedString(NSInflection) enumerateInflectableBlocksInRange:usingBlock:]` | Method |
| `0x1c053402c` | `___88-[NSMutableAttributedString(NSInflection) enumerateInflectableBlocksInRange:usingBlock:]_block_invoke` | Block in parent |
| `0x1c0534154` | `-[NSMutableAttributedString(NSInflection) enumerateFormattableBlocksInRange:usingBlock:]` | Method |
| `0x1c05342a4` | `___88-[NSMutableAttributedString(NSInflection) enumerateFormattableBlocksInRange:usingBlock:]_block_invoke` | Block in parent |
| `0x1c0fdcc80` | `___Block_byref_object_copy__8` | Compiler byref helper |
| `0x1c0fdcc90` | `___Block_byref_object_dispose__8` | Compiler byref helper |

### `Source/Foundation/String/ONSAttributedStringMarkdown.m` (6 entries)

| Address | Native entry | Source form |
| --- | --- | --- |
| `0x1c0601568` | `-[NSAttributedString(_NSAttributedStringFromMarkdown) initWithMarkdownString:options:baseURL:error:]` | Method |
| `0x1c060177c` | `_newAttributedStringFromMarkdown` | Function |
| `0x1c0601d2c` | `__NSAttributedStringFromMarkdownWithCreator` | Function |
| `0x1c0602e84` | `_appendNodeText` | Function |
| `0x1c0603024` | `_appendStringWithSourcePosition` | Function |
| `0x1c06030e0` | `_attributesForIntents` | Function |

### `Source/Foundation/String/ONSAttributedStringMorphun.m` (3 entries)

| Address | Native entry | Source form |
| --- | --- | --- |
| `0x1c057ed5c` | `__NSRequiresMorphunInflectionForLanguageIdentifier` | Function |
| `0x1c057ee30` | `__NSLanguageIdentifierBeginsWithLanguageCode` | Function |
| `0x1c057f55c` | `__NSLexiconInflectedStringUsingMorphun` | Function |

### `Source/Foundation/String/ONSAttributedStringTermOfAddress.m` (1 entries)

| Address | Native entry | Source form |
| --- | --- | --- |
| `0x1c05c4ba4` | `_preferredTermOfAddressForLanguageFromList` | Function |

### `Source/Foundation/String/ONSCharacterSet.m` (45 entries)

| Address | Native entry | Source form |
| --- | --- | --- |
| `0x1c04d89ec` | `-[NSCharacterSet _cfTypeID]` | Method |
| `0x1c04d89f0` | `-[NSCharacterSet _expandedCFCharacterSet]` | Method |
| `0x1c04d8a00` | `+[NSCharacterSet whitespaceCharacterSet]` | Method |
| `0x1c04d8a08` | `+[NSCharacterSet whitespaceAndNewlineCharacterSet]` | Method |
| `0x1c04d8a10` | `+[NSCharacterSet decimalDigitCharacterSet]` | Method |
| `0x1c04d8a30` | `+[NSCharacterSet alphanumericCharacterSet]` | Method |
| `0x1c04d8b9c` | `+[NSCharacterSet characterSetWithCharactersInString:]` | Method |
| `0x1c04d8c94` | `-[NSCharacterSet init]` | Method |
| `0x1c04d8d38` | `-[NSCharacterSet hash]` | Method |
| `0x1c04d8d4c` | `-[NSCharacterSet copyWithZone:]` | Method |
| `0x1c04d8d50` | `-[NSCharacterSet invertedSet]` | Method |
| `0x1c04d8d84` | `-[NSCharacterSet isEmpty]` | Method |
| `0x1c04d8de4` | `-[NSCharacterSet count]` | Method |
| `0x1c04d8e3c` | `-[NSCharacterSet isMutable]` | Method |
| `0x1c04d8e44` | `-[NSCharacterSet makeImmutable]` | Method |
| `0x1c04d8e48` | `-[NSCharacterSet characterIsMember:]` | Method |
| `0x1c04d8ea8` | `-[NSCharacterSet longCharacterIsMember:]` | Method |
| `0x1c04d8eb0` | `-[NSCharacterSet isSupersetOfSet:]` | Method |
| `0x1c04d8f70` | `-[NSCharacterSet isEqual:]` | Method |
| `0x1c04d8fe0` | `-[NSCharacterSet bitmapRepresentation]` | Method |
| `0x1c04d9070` | `-[NSCharacterSet mutableCopyWithZone:]` | Method |
| `0x1c04d90d0` | `-[NSCharacterSet encodeWithCoder:]` | Method |
| `0x1c04d9150` | `-[NSCharacterSet initWithCoder:]` | Method |
| `0x1c04d94d4` | `+[NSCharacterSet supportsSecureCoding]` | Method |
| `0x1c04d94dc` | `-[NSCharacterSet _retainedBitmapRepresentation]` | Method |
| `0x1c04d9590` | `+[NSMutableCharacterSet characterSetWithCharactersInString:]` | Method |
| `0x1c04d961c` | `-[NSMutableCharacterSet init]` | Method |
| `0x1c04d96d4` | `-[NSMutableCharacterSet copyWithZone:]` | Method |
| `0x1c04d96fc` | `-[NSMutableCharacterSet mutableCopyWithZone:]` | Method |
| `0x1c04d975c` | `-[NSMutableCharacterSet addCharactersInRange:]` | Method |
| `0x1c04d97b4` | `-[NSMutableCharacterSet removeCharactersInRange:]` | Method |
| `0x1c04d980c` | `-[NSMutableCharacterSet addCharactersInString:]` | Method |
| `0x1c04d9864` | `-[NSMutableCharacterSet removeCharactersInString:]` | Method |
| `0x1c04d98bc` | `-[NSMutableCharacterSet formUnionWithCharacterSet:]` | Method |
| `0x1c04d9914` | `-[NSMutableCharacterSet formIntersectionWithCharacterSet:]` | Method |
| `0x1c04d996c` | `-[NSMutableCharacterSet invert]` | Method |
| `0x1c04d99c4` | `-[NSMutableCharacterSet isMutable]` | Method |
| `0x1c04d99f0` | `+[NSMutableCharacterSet whitespaceCharacterSet]` | Method |
| `0x1c04d9a14` | `+[NSMutableCharacterSet whitespaceAndNewlineCharacterSet]` | Method |
| `0x1c04d9a38` | `+[NSMutableCharacterSet decimalDigitCharacterSet]` | Method |
| `0x1c04d9aec` | `+[NSMutableCharacterSet alphanumericCharacterSet]` | Method |
| `0x1c04d9c30` | `-[NSMutableCharacterSet initWithCoder:]` | Method |
| `0x1c04d9d48` | `+[NSMutableCharacterSet supportsSecureCoding]` | Method |
| `0x1c05ebac8` | `+[NSCharacterSet(NSURLUtilities) URLPathAllowedCharacterSet]` | Method |
| `0x1c064e42c` | `-[NSCharacterSet(NSCharacterSetPortCoding) replacementObjectForPortCoder:]` | Method |

### `Source/Foundation/String/ONSMutableAttributedString.m` (12 entries)

| Address | Native entry | Source form |
| --- | --- | --- |
| `0x1c04c4f24` | `+[NSMutableAttributedString allocWithZone:]` | Method |
| `0x1c04c4f74` | `-[NSMutableAttributedString mutableString]` | Method |
| `0x1c04c4fa4` | `-[NSMutableAttributedString addAttribute:value:range:]` | Method |
| `0x1c04c5134` | `-[NSMutableAttributedString addAttributes:range:]` | Method |
| `0x1c04c53f0` | `-[NSMutableAttributedString removeAttribute:range:]` | Method |
| `0x1c04c54f8` | `-[NSMutableAttributedString replaceCharactersInRange:withAttributedString:]` | Method |
| `0x1c04c5638` | `-[NSMutableAttributedString deleteCharactersInRange:]` | Method |
| `0x1c04c56cc` | `-[NSMutableAttributedString beginEditing]` | Method |
| `0x1c04c56d0` | `-[NSMutableAttributedString endEditing]` | Method |
| `0x1c04c56d4` | `-[NSMutableAttributedString classForCoder]` | Method |
| `0x1c0fe2118` | `-[NSMutableAttributedString replaceCharactersInRange:withString:]` | Method |
| `0x1c0fe2150` | `-[NSMutableAttributedString setAttributes:range:]` | Method |

<!--
  Created by ktiays on 2026/10/31.
  Copyright (c) 2026 ktiays. All rights reserved.
-->

# CFString core

## Identity and scope

- Native framework and owning image: CoreFoundation, iOS Simulator 27.0 (24A434), arm64, UUID
  D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC. Evidence: the cf-core corpus
  (`Local/Evidence/cf-core/iphone-simulator-core-corpus.json`, assembly and pseudocode per function), static data
  read from the extracted image, linkage from `Local/Evidence/cf-core/iphone-simulator-nm.txt`. The assembly
  settles every branch, width, constant and call order; pseudocode was a guide only.
- Native functions: the 136 entries of the cf-core string-core work list (136 including cold fragments), restored
  as 115 C definitions; `.cold` and `OUTLINED_FUNCTION` fragments are inside their parents.
  `__CFStringCollectionCopy.cold.1` is the crash path of `__CFStringCollectionCopy`, already restored in
  `Source/CoreFoundation/Runtime/OCFRuntime.c`.
- Confidence: verified for every layout, constant and branch named here, except the items marked medium below.
  iOS Device 24A446: unread for every function.
- Not built: no isolated or differential test exists yet; nothing is linked; the work list's callees owned by
  other work lists are declared and left undefined.

## Units and source files

The native source paths are not in the evidence; the organization follows contiguous `__text` clusters.

| File | Native cluster | Functions |
| --- | --- | --- |
| `String/OCFString.c` | 0x1c02085f4-0x1c0225954, cold fragments 0x1c030c440-0x1c030ca4c | 90 CFString-unit functions, the class record and the unit's data |
| `String/OCFTaggedPointerString.c` | 0x1c015a344-0x1c015adec (NSTaggedPointerString unit) | inline tagged-string equality, bytes, length |
| `String/OCFIndirectTaggedPointerString.c` | 0x1c02058fc-0x1c0206b1c (NSIndirectTaggedPointerString unit) | creation, contents, equality |
| `String/OCFStringPropertyListSupport.c` | 0x1c01d8110 (property-list unit) | `__CFStringContainsNullCharacter` |
| `String/OCFStringByteStream.c` | 0x1c022c2a4-0x1c022de0c (string-encodings unit) | byte-stream decode and encode, file-system representation, user default encoding |
| `String/OCFStringScanner.c` | 0x1c022e34c-0x1c022ea4c | integer and double scanners |
| `String/OCFStringEncodingNames.c` | 0x1c0233f00 (beside CFStringConvertEncodingToNSStringEncoding) | IANA charset name to encoding |
| `String/OCFStringUtilities.c` | 0x1c02342d0-0x1c0235140 | locale comparison and the collator pool |
| `String/OCFStringBundleSupport.c` | 0x1c02c2704-0x1c02c38dc (bundle-strings unit) | append helpers and the dictionary applier |
| `String/OCFStringFormatConfiguration.c` | 0x1c02caf38-0x1c02cbea0 | format-configuration and markdown entry points |

Non-external functions whose native unit is owned by another work list (`__CFStringContainsNullCharacter`, the
bundle helpers, the formatter call-outs and `_CFStringHasStrongOrWeakRTL`) are hidden with native names until
their callers join the unit, then static.

## Native implementation

- Object: 16-byte header (isa, info word). Info bits 0-6: 0x1 mutable, 0x4 length byte, 0x8 null byte, 0x10
  Unicode (UTF-16 contents; clear: eight-bit contents in `__CFDefaultEightBitStringEncoding`), 0x60 contents kind
  (0 inline, 0x20 freed with the object's allocator, 0x40 not freed, 0x60 freed with the stored contents
  deallocator). Every access reads the info word with LDAR; flag updates are compare-and-swap loops.
- Immutable layout: inline contents start at 0x10, after an explicit length (+8) unless the string has a length
  byte ((info & 5) == 4); external contents are the pointer at 0x10 with the length at 0x18 and, for kind 0x60, the
  contents deallocator at 0x18 (length byte) or 0x20.
- Mutable layout: buffer 0x10, length 0x18, capacity 0x20, a bits word at 0x28 (bit 1 fixed capacity, bit 2
  external, bit 3 capacity provided externally, bits 4-63 the desired capacity) and the contents allocator 0x30
  (kind 0x60 only). `CFStringCreateMutable` asks for 32 extra bytes, so 0x30 is outside its object; it is written
  only for kind 0x60, which that function never sets.
- Class record `__CFStringClass` (0x2227a5300): version 1, type ID 7, "CFString"; copy
  `_CFNonObjCStringCreateCopy`, finalize `__CFStringDeallocate`, equal `__CFStringEqual`, hash `__CFStringHash`,
  formatting description `__CFStringCopyFormattingDescription`, debug description `__CFStringCopyDescription`.
- `__CFStringCreateImmutableFunnel3` takes an options byte: 0x1 possibly external representation, 0x2 try to reduce
  Unicode, 0x4 length byte, 0x8 null byte, 0x10 no copy, 0x20 no copy with ROM disallowed. Short ASCII contents
  become inline tagged strings when `__CFTaggedStringClass` is set; the decoder buffer is the 0x410-byte
  `VarWidthCharBuffer` (characters, isASCII at 8, shouldFreeChars at 9, allocator 0x10, count 0x18, 1,008-byte
  local buffer 0x20).
- Tagged strings: the inlined tag search decodes the pointer with `objc_debug_taggedpointer_obfuscator` (skipped
  for pointers with bits 63, 62 and 0-2 set) and looks the low three bits up in the first seven entries of
  `objc_debug_tag60_permutations`; index 2 is an inline tagged string, 0 an indirect one.
- Inline buffers: units outside CFString compile the SDK inline accessors (`CFStringGetCharactersPtr`, then
  `CFStringGetCStringPtr(str, kCFStringEncodingASCII)`); inside CFString the initialization calls
  `_CFStringGetCStringPtrInternal(str, kCFStringEncodingASCII, false, true)`. The fourth argument of
  `_CFStringGetCStringPtrInternal` and `_CFStringGetUTF8PtrWithOptions` is an Objective-C dispatch flag.
- The per-definition comments in the sources carry the remaining branch conditions, constants and call orders.

## Dependencies and naming

- Other families, declared in the "other families" block of `String/OCFString.h`: arrays, data, dictionaries,
  numbers, locales, bundles, character sets (including the inline character-set buffer and
  `_CFCharacterSetIsLongCharacterMemberForInline`), the encoding converters (`CFStringEncodingGetConverter`,
  `CFStringEncodingBytesToUnicode`, `CFStringEncodingUnicodeToBytes`, the length queries,
  `__CFStringEncodingGetFromCanonicalName`, `__CFStringEncodingGetFromICUName`, `__CFConverterUTF8`),
  `__CFSetCharToUniCharFunc`, `__CFCharToUniCharFunc`, `_CFOSLog`, `_CFRaiseMemoryException`, `__CFLookUpClass`,
  `_CFStringGetFormatSpecifierConfiguration`, and CFString-unit statics of other work lists (`copyBlocks`,
  `_CFNonObjCStringCreateCopy`, `__CFStringCopyFormattingDescription`, `__rangeRetain`, `__rangeRelease`,
  `__rangeCopyDescription`, `_objc_getTaggedPointerTag`, the two `isEqualToTaggedPointer` statics,
  `__CompareTextDefault`, `__extendLocationBackward`, `__extendLocationForward`, `__collatorFinalize`).
- Supplied libraries: libobjc (`objc_msgSend`, `objc_alloc`, `objc_opt_respondsToSelector`, the tagged-pointer
  debug variables), libicucore (`u_charDirection`, `u_charType`, `u_hasBinaryProperty`, `u_getIntPropertyValue`,
  `u_isdigit`, `uscript_getScript`, `ucol_*`, `usearch_*`), libdispatch, libsystem (`os_log`, unfair locks,
  `dyld_program_sdk_at_least`, `_dyld_is_memory_immutable`, `strtol_l`, `strtod_l`).
- `__CFSafelyReallocateWithAllocatorTyped`: every native call site of this family passes the malloc type ID in X3
  and 0 in X4, while `Base/OCFBase.c` names X3 `hint` and X4 `typeID`. The family passes the registers in native
  order; the Base reading needs a check.

## Defects and divergences

- `CoreFoundation defect:` `__CFStringAppendBytes` (0x1c021cb50) returns after the Objective-C send without
  freeing decoded storage; the restoration frees it.
- `CoreFoundation defect:` `_CFCompareStringsWithLocale` (0x1c0234804, 0x1c02348cc) records the cluster copy
  destination as the heap buffer even when it is the stack buffer; the restoration records only heap pointers.
- Kept (well defined): the swapped surrogate combination in the backward joiner walk of
  `CFStringGetRangeOfCharacterClusterAtIndex` (0x1c02144b4); the pattern continuation read at the subject index + 1
  in the eight-bit path of `CFStringFindWithOptionsAndLocale` (0x1c02157c8); the 32-bit truncations of
  `CFStringFindAndReplace`, `CFStringCreateArrayWithFindResults` and `CFStringFindCharacterFromSet`; the
  greater-than answer of the whole-range fast compare in `_CFCompareStringsWithLocale` when memcmp returns 0;
  `__CFStringHandleOutOfMemory` callers continue after the call.

## Medium confidence and unread

- `CFStringCompareWithOptionsAndLocale`: two jump targets inferred from register flow (0x1c0211f40 and the
  fall-through after a second successful fold).
- The signatures of `__rangeRelease` and `__rangeCopyDescription` (unread, referenced only as data); the
  function-pointer types of the formatter call-outs; the converter record beyond offsets 0, 8, 0x10 and 0x14; the
  inline character-set buffer flags (read from the string callers, not from a character-set reading).
- Unread: the writers of `__CFTaggedStringClass` and `__CFWantsToUseASCIICompatibleConversion` (none in the
  closure); the iOS Device image.

## Verification and limits

Compile-only: both naming modes build with zero diagnostics (`Local/Scratch/cf-core/devbuild.py <worktree> NO` and
`YES`). No isolated, differential or reference-image test exists; no definition is recorded in
`Configuration/Restoration.json`.

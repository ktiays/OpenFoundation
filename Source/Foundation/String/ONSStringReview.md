<!--
  Created by ktiays on 2026/11/03.
  Copyright (c) 2026 ktiays. All rights reserved.
-->

# Foundation string reading and integration handoff

This reading accompanies the FS-string source batch on `codex/cf-core-fd-string`, based on `b518e26`.
It lives in the owned source directory because this handoff permits no edits to shared documentation;
the coordinator can move it to `Documentation/Reference/Research/FoundationString.md` at integration.
It records translations of the 226 assigned IDA entries, not a completed or behaviorally validated string
restoration. The frameworks remain unlinked. No entry was added to `Configuration/Restoration.json`.

## Reference and review

- iOS Simulator 27.0 (24A434), arm64, Foundation UUID `49FCDA5F-A93E-3259-857C-FC75AC2C7B03`,
  SHA-256 `bc7e0a5f693ffc75ae7c3311b817716e86d149fda2f8c45669a976727322cb18`.
- Primary IDA capture: `Local/Evidence/cf-core/iphone-simulator-foundation-link.json`, exactly the
  226 `FS-string` entries. The source inventory below includes each native address and function name.
- Supplemental IDA evidence: `Local/Scratch/cf-core-fd-string-review/supplemental-ida.json` and
  `review-identity.json` in the coordinating checkout. The canonical `.i64` was APFS-cloned to that scratch
  directory. Its recorded input hash matched the pinned image. All 11,286 captured instruction lines were
  checked against the clone: 18 textual differences were symbol-label or added-comment differences,
  with no changed instruction. The clone's IDA session was closed after reading.
- Method bodies were reviewed against the capture, with the assembly settling register widths, missing
  arguments, initializations and decompiler artifacts. `xcrun nm -m` supplies linkage/interface metadata.
- Implementation confidence is verified against this reading except the medium-confidence bindings below.
- **iOS Device divergence:** all functions in this batch remain unread against iOS Device 24A446,
  Foundation UUID `C041F1D0-5EF1-30A3-BF96-830976173B56`. No Device equivalence is claimed.

The interrupted source had six declared-only entries. They now have bodies: `__NSOSLog`,
`__NSScanDoubleFromString`, `-[NSString(NSURLUtilities) stringByAddingPercentEncodingWithAllowedCharacters:]`
and all three `NSRegularExpressionSupport` methods. Supplemental block invokes were read at
`0x1c05938c8` (create the Foundation OS-log handle), `0x1c05c1de0` (free failed reallocation and set stop),
and `0x1c063fe88` (create/name/limit the shared regex cache). Their blocks are implemented at their use sites.
The shared Foundation Swift-character-set gate was also read (`0x1c051a370`, invoke `0x1c0fdc9cc`), but
its data/block definition remains an explicit FM/FC integration dependency; no second copy of its state
is introduced here.

## Class layouts and placeholder replacement

All four class records have `instanceStart == instanceSize == 8`, no ivar list and flags 0. No ivar is unread.

| Class | Class address | class_ro_t | Superclass | Owner header |
| --- | --- | --- | --- | --- |
| NSString | 0x22e6dd528 | 0x22285f860 | NSObject (external pointer 0x22e6b6088; name binding medium) | ONSString.h |
| NSMutableString | 0x22e6db6b0 | 0x222859240 | NSString | ONSMutableString.h |
| NSPlaceholderString | 0x22e6dd5a0 | 0x22285f8f0 | NSString | ONSPlaceholderString.h |
| NSPlaceholderMutableString | 0x22e6db750 | 0x2228593e8 | NSMutableString | ONSPlaceholderMutableString.h |

The coordinator must replace exactly the `ONSString` and `ONSMutableString` placeholder interfaces in
`CoreFoundation/Objective-C/OCFBridgedClasses.h` with imports of the owners above. The placeholder subclass
interfaces are new; that shared header contains neither of them. CF callers using `OCFRange` must explicitly
form `ONSRange` at the source boundary where the selected Objective-C declaration requires it; the native
ABI remains two 64-bit words. Existing declarations of out-of-batch selectors such as `-cStringLength` must
be preserved as declarations while their closure is assigned/read; this batch does not add their bodies.
No CF runtime type-registration record is defined by this Foundation class family.

The class-state words `__NSCFStringClass` (0x22e775b08), `__NSMutableStringClass` (0x22e775b18) and
`__NSStringClass` (0x22e775b20) are native zero-initialized common storage. Their writer is outside the
assigned capture and remains unread. Initialization of these words is required before runtime acceptance;
zero storage is not a substitute for that initialization.

## Corrections and native defects

Translation corrections preserve the native behavior rather than define a new policy:

- `__NSNewStringByAppendingStrings` (0x1c0571a50): the per-string byte-count output is zeroed before each
  conversion, matching `STP XZR, XZR` at 0x1c0571dac. The interrupted source left it uninitialized.
- `__NSSetCStringCharToUnichar` (0x1c05d9ea4): preserve the callback store even on the unchanged-table path.
- The five fixed placeholder misuse logs preserve their exact `NSLog(OF_STR(...))` calls. Clang recognizes
  the system spelling `NSLog` as a format function but cannot recognize the constant-string record produced
  by `OF_STR` as an Objective-C literal. A diagnostic push/pop suppresses only `-Wformat-security` around
  those five constant, substitution-free calls; no variable format or compiler policy is exempted.
- The retained return of `-[NSString(NSCFAdditions) _createSubstringWithRange:]` (0x1c04d1f68) is declared
  `ns_returns_retained`; this records the actual +1 contract and resolves the analyzer's naming-convention
  diagnostics without changing its retain or autorelease operations.
- `-[NSString(NSPathUtilities) stringByAppendingPathComponent:]` (0x1c05901c8): emit the native 42-byte
  log payload and call `_os_log_fault_impl` after the one `os_log_type_enabled` test. The previous nested
  `os_log_fault` macro would test the log type again.
- The conflicting binding of GOT 0x22227F4F8 in the variadic append unit is recorded consistently as
  `NSInternalInconsistencyException`, as in the other units using that slot. The binding remains medium
  confidence; it is not promoted to a verified pointer target.

The following precise divergences fix native memory-safety defects or unintended leaks. All are established
from the iOS Simulator assembly, confidence verified; their iOS Device sides remain unread. No destructive
native case was executed.

| Native function | Native evidence | Divergence |
| --- | --- | --- |
| `-[NSMutableString(NSCFAdditions) _cfPad:length:padIndex:]`, 0x1c04d2314 | 0x1c04d2424 compares against the previous chunk; 0x1c04d2434 then obtains the full pad length. A 3-unit pad, index 2, count 3 copies a 3-unit second chunk into a 2-unit remainder. | Clamp that unsafe next copy to the remainder, retaining the native calls/order for safe copies. |
| `__NSConvertToASCIIorUnicode2`, 0x1c05dad38 | 0x1c05dad9c reads a halfword after only checking length != 0. A 1-byte UTF-16 input reads outside its declared span and can underflow the BOM-adjusted count. | Inspect a BOM only with at least 2 bytes; the truncated input yields the same zero-unit result as a non-BOM partial unit. |
| `-[NSString encodeWithCoder:]`, 0x1c05d8074 | The throw at 0x1c05d828c bypasses the heap-buffer free at 0x1c05d821c. | After forming the reason, free a heap encoding buffer before creating/throwing the exception. |
| `__NSScanDoubleFromString`, 0x1c05c18d0 | Initial malloc at 0x1c05c1c2c is followed by unguarded stores at 0x1c05c1c40-0x1c05c1c44. The later realloc failure already returns NO. | A failed first malloc returns NO with output/location untouched. |
| `-[NSString(NSPathUtilities) stringByAppendingPathComponent:]`, 0x1c05901c8 | Nil paths at 0x1c0590384 and 0x1c0590448 bypass frees of long-path/component buffers. | Free those allocated buffers before returning nil. This correction was present in the interrupted source and was verified in review. |

Well-defined unusual behavior is kept: placeholder retain/autorelease answer the receiver, retainCount is all
ones, release/dealloc do nothing; mutable placeholder capacity is ignored; no-BOM UTF-16 conversion byte-swaps;
invalid encodings 0/0x8000100 retain their native compatibility mapping where the native code uses it;
empty input paths and empty search ranges retain their native results; scanning skips the supplied set after
a sign, clamps signed-integer overflow, preserves the first digit-membership result, and counts normalized
bytes in the double scanner's location even with a multi-character separator. String enumeration retains
the native option masks, reverse windows, mutation adjustments, callback/release order and 1,001-iteration
pool-drain cadence. No value, tolerance, successful stub or native Foundation/CoreFoundation forwarding was
introduced.

## Assembly and pseudocode disagreements

- Bare `RET` placeholder ownership methods and the string port-coding method return the incoming X0;
  Hex-Rays prints empty `void` bodies. The actual object/void signatures follow the selectors and assembly.
- `_NSExpandTildeInPath` (0x1c058f95c) is misidentified as `__swiftcall` with extra parameters by Hex-Rays;
  assembly reads the path in X0 and the length pointer in X1. Both captured callers pass exactly those.
- Tail-call thunks often omit live arguments in pseudocode: copy/mutable-copy zones, substring/range values,
  the path-store length and formatting argument lists are preserved from register/stack setup.
- `NSLog` (0x1c0593870) preserves its incoming X30 as the third `_NSLogv` argument; pseudocode shows only the
  format and variadic list. The restored function passes `__builtin_return_address(0)`.
- The scanning and line-boundary functions' repeated character-buffer steps are reconstructed from loads,
  refill branches and selector sends; decompiler-only phantom arguments and post-throw fallthrough calls are
  not implemented. The double scanner's ctypes test applies only to ASCII and uses the native whitespace
  classification; normalized bytes are then passed to `strtod_l(..., NULL)`.
- Regex methods return NSRange in X0/X1, including `{NSNotFound, 0}` for failed compilation; pseudocode
  renders only X0 for some paths. The capture's X1 assignments settle the second word.
- Typed allocation/reallocation arguments that disappear in pseudocode are kept from the assembly, including
  the native hint/type-ID positions in quoted-string growth and shrink operations.

## Medium-confidence bindings and unread boundaries

The extracted image does not contain the shared-cache GOT/selector targets listed below. These are recorded
as bindings inferred from use/import metadata, not as direct reads of the target storage. The comments at
each source/header retain the uncertainty.

- NSObject superclass name at 0x22e6b6088.
- Exception names: 0x22227F4E0 NSGenericException; 0x22227F4F8 NSInternalInconsistencyException;
  0x22227F500 NSInvalidArgumentException; 0x22227F5D8 NSMallocException; 0x22227F5E0 NSRangeException.
- CoreFoundation class imports: 0x22227FA90 NSArray, 0x22227FAA8 NSCache, 0x22227FAB8 NSData,
  0x22227FAD0 NSDictionary, 0x22227FAE0 NSException, 0x22227FB00 NSLocale, 0x22227FB18 NSMutableArray,
  0x22227FB20 NSMutableData, 0x22227FB38 NSMutableSet and 0x22227FB88 NSURL.
- CFNetwork class imports: 0x222278DA8 NSHTTPURLResponse, 0x222278DE0 NSURLConnection,
  0x222278E08 NSURLRequest. Their prefixed object boundary is an integration dependency, not a bridge to
  native CFNetwork objects.
- 0x22227F550 NSLocaleDecimalSeparator (the CoreFoundation locale-key reading records the alias to
  kCFLocaleDecimalSeparatorKey); 0x222280698 __CFCharToUniCharFunc; 0x2222806A8
  __CFDefaultEightBitStringEncoding; 0x2222806C0 and 0x2222806D0 the idempotent and MacRoman tables;
  0x2222806D8 __CFOASafe; 0x222280B50/B58/B68/B70 default/malloc/null/system-default allocators.
- Selector references 0x2387B3255 substringWithRange:, 0x2387AE4CD descriptionWithLocale: and
  0x2387ABBDF _dynamicContextEvaluation:patternString: are bound by the subsequent direct sends.
- `_CFStringCreateWithNullTerminatedBytesNoCopy` has a medium-confidence caller-derived signature;
  it remains declared/undefined here.

The stat-cache key callbacks `__isEqualCString` (0x1c0592180) and `__hashCString2` (0x1c05921c0) are outside
this assignment. Their bodies were captured in the supplemental evidence while reviewing the table, but
are left undefined for the coordinator, as the handoff requires for out-of-list dependencies. The first is
identity-or-non-null-strcmp equality; the second converts UTF-8 to UTF-16 and hashes its units, otherwise
hashes the original C bytes, freeing allocated conversion storage on the successful Unicode path. No other
assigned body is left undefined. Native class-state initialization and every iOS Device body remain unread.

## Shared declarations, naming conversion and allowed undefined symbols

All temporary cross-family functions, constants, class interfaces, class-symbol aliases and message-only
protocols now live in `ONSUnrestoredInterfaces.h`. It declares interfaces only and is reproduced verbatim
in the appendix. Replace it with the owners' headers at integration; do not copy its declarations into
multiple private headers. CoreFoundation string/tokenizer/encoding/URL declarations belong to their CF
owners, `_ONSIsNSURL` to `CoreFoundation/Objective-C/OCFNSTypeChecks.h`, NSError constants/classes to FM,
number/value classes to FN, coder/archiver classes to FK, and attributed-string/character-set classes to FC.
The out-of-batch file/path/localized-string/regex/cache families still owe their class objects.

For the shared feature gate the coordinator must expose the exact three native labels declared in that
header, with one shared token/byte/block. Renaming a C declaration must preserve its assembler label; the
block label designates the literal's storage, not a variable containing a block pointer.

The only edit outside this family is `Source/CoreFoundation/Error/OCFError.c`:

1. `domainCocoaString`: `"NSCocoaErrorDomain"` becomes `OF_NAME_STRING(ONSCocoaErrorDomain)` and its adjacent
   length 18 becomes `sizeof(OF_NAME_STRING(ONSCocoaErrorDomain)) - 1`.
2. `_CFErrorInitializeCallBackTable`: the same literal inside `OF_STR` becomes
   `OF_STR(OF_NAME_STRING(ONSCocoaErrorDomain))`.

The separate allow list is
`/Users/ktiays/Developer/OpenAppleOS/OpenFoundation/Local/Scratch/cf-core-fd-string/allow-undefined.txt`.
It contains exactly:

```text
#
#  Created by ktiays on 2026/11/03.
#  Copyright (c) 2026 ktiays. All rights reserved.
#
# Native Foundation shared character-set gate at 0x22e7761f8; its FM/FC owner must expose the same once token.
__foundation_swift_characterSet_enabled.onceToken
# Native Foundation shared character-set gate byte at 0x22e776200; do not create separate FS-string state.
__foundation_swift_characterSet_enabled.featureEnabled
# Native Foundation global block at 0x22281c8d8, invoke 0x1c0fdc9cc; the shared gate's owner supplies the literal.
___block_literal_global_10
# CFNetwork class import through 0x222278da8; prefixed integration remains unsupported until its object boundary is read.
_OBJC_CLASS_$_ONSHTTPURLResponse
# CFNetwork class import through 0x222278de0; no forwarding to native CFNetwork objects is supplied in this batch.
_OBJC_CLASS_$_ONSURLConnection
# CFNetwork class import through 0x222278e08; the coordinator must classify/supply this boundary before linking.
_OBJC_CLASS_$_ONSURLRequest
```

Every other new undefined function/data symbol is declared in the temporary header, and every other new
undefined class is a named family dependency there. The raw symbol audit is preserved as
`Local/Scratch/cf-core-fd-string-review/symbols-reviewed-NO.txt`; its pre-existing unresolved symbols and
compiler block-helper coalescing are not counted as new regressions.

## Verification boundaries and reproducible checks

The object audit compiled 244 C-family source files from both products with zero failures. An additional
source-to-object audit resolves every one of the 222 non-block assigned functions/methods independently
in the produced objects and finds all four enumeration block invokes: zero definition/citation mismatches.
This is stronger than the acceptance script's address-text check but remains compile-only evidence.

Release compilation also produced all 244 objects in each naming mode with zero diagnostics, using the
shared build driver's Release flags (including full LTO) and each mode's prepared headers. Scratch objects
and the two-mode summary are under `Local/Scratch/cf-core-fd-string-review/Release`. No link is implied.

Static analysis was run on CF additions, scanning, the primary string unit, path utilities and other
encodings. The CF-additions retained-return annotation resolves the two naming-contract warnings; CF
additions, scanning and other encodings then have no analyzer diagnostics. The remaining seven reports
are triaged: three enumeration reports do not connect type bits to the tokenizer setup or model writes
through the range-collecting block; two cache reports do not model process-lifetime C-string storage in the
CFDictionary; two path-buffer-free reports require mutually contradictory comparisons of unchanged `total`
(`total <= 1` and later `total >= 257`). Detailed paths remain under
`Local/Scratch/cf-core-fd-string-review/analyzer-*.txt`. These are not counted as runtime test results.

The handoff's acceptance command is:

```sh
python3 /Users/ktiays/Developer/OpenAppleOS/OpenFoundation/Local/Scratch/cf-core/accept-foundation.py \
    /Users/ktiays/.codex/worktrees/cf-core-fd-string/OpenFoundation FS-string \
    --allow-undefined /Users/ktiays/Developer/OpenAppleOS/OpenFoundation/Local/Scratch/cf-core-fd-string/allow-undefined.txt
```

The final full output is retained in `Local/Scratch/cf-core-fd-string-review/acceptance.txt` in the
coordinating checkout. The worktree must be committed before that command can print `ACCEPTED`.

No linked framework execution, cross-process trace comparison, ASan/leak execution, Xcode build matrix or
iOS Device execution is claimed. Those remain for the combined link and independently initialized runtime.
The native callbacks, class-state writer, foreign family objects and explicit gate/CFNetwork cuts must be
supplied/classified first. This batch does not ship a partial dependency or make an incomplete group pass
as restored.

## Files and assigned native entries

Each `.m` below has its corresponding owner `.h`; `ONSUnrestoredInterfaces.h` is the temporary shared
interface exception and this Markdown file is the scoped reading. No assigned entry is omitted.
Counts include block invokes folded into their parent method.

| Source | Assigned entries |
| --- | ---: |
| `ONSAllocateObject.m` | 1 |
| `ONSFileUtilities.m` | 3 |
| `ONSLog.m` | 4 |
| `ONSMutableString.m` | 15 |
| `ONSPathUtilities.m` | 24 |
| `ONSPlaceholderMutableString.m` | 17 |
| `ONSPlaceholderString.m` | 19 |
| `ONSRuntimeSupport.m` | 5 |
| `ONSString.m` | 91 |
| `ONSStringCFAdditions.m` | 11 |
| `ONSStringDecimalExtension.m` | 1 |
| `ONSStringDistantString.m` | 3 |
| `ONSStringFromRange.m` | 1 |
| `ONSStringOriginalAttributedString.m` | 1 |
| `ONSStringOtherEncodings.m` | 19 |
| `ONSStringPortCoding.m` | 1 |
| `ONSStringRegularExpressionSupport.m` | 3 |
| `ONSStringScanning.m` | 4 |
| `ONSStringURLUtilities.m` | 2 |
| `ONSZone.m` | 1 |
| **Total** | **226** |

### ONSAllocateObject.m

| Native address | Native function |
| --- | --- |
| 0x1c0582f54 | `_NSAllocateObject` |

### ONSFileUtilities.m

| Native address | Native function |
| --- | --- |
| 0x1c05051ec | `__NSErrorWithFilePath` |
| 0x1c0505288 | `__NSErrorWithFilePathAndEncoding` |
| 0x1c050de18 | `__NSFileExistsAtPath` |

### ONSLog.m

| Native address | Native function |
| --- | --- |
| 0x1c0592ae4 | `__NSOSLog` |
| 0x1c0593870 | `_NSLog` |
| 0x1c0fde558 | `__NSLogv` |
| 0x1c0fde5d0 | `___NSLogAdaptor` |

### ONSMutableString.m

| Native address | Native function |
| --- | --- |
| 0x1c0570720 | `+[NSMutableString allocWithZone:]` |
| 0x1c0570744 | `+[NSMutableString stringWithCapacity:]` |
| 0x1c0570760 | `-[NSMutableString insertString:atIndex:]` |
| 0x1c0570770 | `-[NSMutableString appendString:]` |
| 0x1c05707a4 | `-[NSMutableString deleteCharactersInRange:]` |
| 0x1c05707b0 | `-[NSMutableString replaceCharactersInRange:withCString:length:]` |
| 0x1c0570828 | `-[NSMutableString replaceCharactersInRange:withCharacters:length:]` |
| 0x1c05708a0 | `-[NSMutableString appendFormat:]` |
| 0x1c057093c | `-[NSMutableString setString:]` |
| 0x1c0570970 | `-[NSMutableString replaceOccurrencesOfString:withString:options:range:]` |
| 0x1c0570ba8 | `+[NSMutableString _subclassesMustBeExplicitlyMentionedWhenDecoded]` |
| 0x1c0570bb0 | `-[NSMutableString classForCoder]` |
| 0x1c0571a50 | `__NSNewStringByAppendingStrings` |
| 0x1c0fe39b0 | `-[NSMutableString replaceCharactersInRange:withString:]` |
| 0x1c0fe39bc | `-[NSMutableString initWithCapacity:]` |

### ONSPathUtilities.m

| Native address | Native function |
| --- | --- |
| 0x1c058cb70 | `__NSPathComponents` |
| 0x1c058ccd4 | `__convertToUnichars` |
| 0x1c058ce00 | `__NSResolveSymlinksInPathUsingCache` |
| 0x1c058d240 | `__addToStatCache` |
| 0x1c058d364 | `__NSStandardizePathUsingCache` |
| 0x1c058dd14 | `__NSTransmutePathSlashes` |
| 0x1c058ddc8 | `__pathHasDotDot` |
| 0x1c058df14 | `__NSStandardizePathRewriteAutomountIsPathExcluded` |
| 0x1c058e9e0 | `__NSAppendPathComponent` |
| 0x1c058efb0 | `__NSStartOfPathExtension` |
| 0x1c058f95c | `__NSExpandTildeInPath` |
| 0x1c058fcbc | `+[NSString(NSPathUtilities) pathWithComponents:]` |
| 0x1c058fcc8 | `-[NSString(NSPathUtilities) pathComponents]` |
| 0x1c058fe74 | `-[NSString(NSPathUtilities) lastPathComponent]` |
| 0x1c0590018 | `-[NSString(NSPathUtilities) stringByDeletingLastPathComponent]` |
| 0x1c05901c8 | `-[NSString(NSPathUtilities) stringByAppendingPathComponent:]` |
| 0x1c05904f8 | `-[NSString(NSPathUtilities) pathExtension]` |
| 0x1c059069c | `-[NSString(NSPathUtilities) stringByDeletingPathExtension]` |
| 0x1c0590dd0 | `-[NSString(NSPathUtilities) _stringByStandardizingPathUsingCache:]` |
| 0x1c0590e90 | `-[NSString(NSPathUtilities) stringByStandardizingPath]` |
| 0x1c0590e98 | `-[NSString(NSPathUtilities) _stringByResolvingSymlinksInPathUsingCache:]` |
| 0x1c05910a4 | `-[NSString(NSPathUtilities) fileSystemRepresentation]` |
| 0x1c05911dc | `-[NSString(NSPathUtilities) getFileSystemRepresentation:maxLength:]` |
| 0x1c0591f0c | `_NSHomeDirectoryForUser` |

### ONSPlaceholderMutableString.m

| Native address | Native function |
| --- | --- |
| 0x1c057104c | `-[NSPlaceholderMutableString autorelease]` |
| 0x1c0571050 | `-[NSPlaceholderMutableString retain]` |
| 0x1c0571054 | `-[NSPlaceholderMutableString retainCount]` |
| 0x1c057105c | `-[NSPlaceholderMutableString release]` |
| 0x1c0571070 | `-[NSPlaceholderMutableString dealloc]` |
| 0x1c0571074 | `-[NSPlaceholderMutableString init]` |
| 0x1c0571088 | `-[NSPlaceholderMutableString initWithCapacity:]` |
| 0x1c057109c | `-[NSPlaceholderMutableString initWithString:]` |
| 0x1c0571178 | `-[NSPlaceholderMutableString initWithBytes:length:encoding:]` |
| 0x1c0571240 | `-[NSPlaceholderMutableString initWithUTF8String:]` |
| 0x1c057132c | `-[NSPlaceholderMutableString initWithBytesNoCopy:length:encoding:freeWhenDone:]` |
| 0x1c0571490 | `-[NSPlaceholderMutableString initWithCharactersNoCopy:length:freeWhenDone:]` |
| 0x1c057155c | `-[NSPlaceholderMutableString initWithCString:encoding:]` |
| 0x1c05716d8 | `-[NSPlaceholderMutableString initWithFormat:locale:arguments:]` |
| 0x1c0fe39e0 | `-[NSPlaceholderMutableString length]` |
| 0x1c0fe3a24 | `-[NSPlaceholderMutableString characterAtIndex:]` |
| 0x1c0fe3a68 | `-[NSPlaceholderMutableString replaceCharactersInRange:withString:]` |

### ONSPlaceholderString.m

| Native address | Native function |
| --- | --- |
| 0x1c05d8430 | `-[NSPlaceholderString autorelease]` |
| 0x1c05d8434 | `-[NSPlaceholderString retain]` |
| 0x1c05d8438 | `-[NSPlaceholderString retainCount]` |
| 0x1c05d8440 | `-[NSPlaceholderString release]` |
| 0x1c05d8454 | `-[NSPlaceholderString dealloc]` |
| 0x1c05d8458 | `-[NSPlaceholderString init]` |
| 0x1c05d8464 | `-[NSPlaceholderString initWithCharactersNoCopy:length:freeWhenDone:]` |
| 0x1c05d85d0 | `-[NSPlaceholderString initWithCharacters:length:]` |
| 0x1c05d85e8 | `-[NSPlaceholderString initWithCString:encoding:]` |
| 0x1c05d86e8 | `-[NSPlaceholderString initWithBytesNoCopy:length:encoding:freeWhenDone:]` |
| 0x1c05d8948 | `-[NSPlaceholderString initWithCString:length:]` |
| 0x1c05d89cc | `-[NSPlaceholderString initWithString:]` |
| 0x1c05d8df8 | `__NSDescriptionWithLocaleFunc` |
| 0x1c05d8e60 | `__NSDescriptionWithStringProxyFunc` |
| 0x1c05d8ef0 | `-[NSPlaceholderString initWithFormat:locale:arguments:]` |
| 0x1c05d9194 | `-[NSPlaceholderString initWithData:encoding:]` |
| 0x1c05d9238 | `-[NSPlaceholderString initWithBytes:length:encoding:]` |
| 0x1c0fe5564 | `-[NSPlaceholderString length]` |
| 0x1c0fe55a8 | `-[NSPlaceholderString characterAtIndex:]` |

### ONSRuntimeSupport.m

| Native address | Native function |
| --- | --- |
| 0x1c05a1038 | `__NSNameOfClass` |
| 0x1c05a1124 | `__NSMethodExceptionProem` |
| 0x1c05a19e0 | `-[NSString(NSKindOfAdditions) isNSString__]` |
| 0x1c0fe4408 | `_NSRequestConcreteImplementation` |
| 0x1c0fe44bc | `__NSRequestConcreteObject` |

### ONSString.m

| Native address | Native function |
| --- | --- |
| 0x1c05d1e14 | `+[NSString initialize]` |
| 0x1c05d1ebc | `+[NSString allocWithZone:]` |
| 0x1c05d1ee0 | `-[NSString init]` |
| 0x1c05d1ee4 | `+[NSString string]` |
| 0x1c05d1efc | `+[NSString stringWithString:]` |
| 0x1c05d1f24 | `+[NSString stringWithCharacters:length:]` |
| 0x1c05d1ffc | `+[NSString stringWithCString:encoding:]` |
| 0x1c05d20a0 | `+[NSString stringWithUTF8String:]` |
| 0x1c05d2378 | `+[NSString stringWithFormat:]` |
| 0x1c05d2640 | `-[NSString _fastCharacterContents]` |
| 0x1c05d2648 | `-[NSString _fastCStringContents:]` |
| 0x1c05d2650 | `-[NSString _fastUTF8StringContents:utf8Length:]` |
| 0x1c05d269c | `-[NSString _isCString]` |
| 0x1c05d26bc | `-[NSString _stringRepresentation]` |
| 0x1c05d287c | `-[NSString quotedStringRepresentation]` |
| 0x1c05d2c1c | `-[NSString description]` |
| 0x1c05d2c20 | `-[NSString copyWithZone:]` |
| 0x1c05d2c4c | `-[NSString mutableCopyWithZone:]` |
| 0x1c05d2c78 | `-[NSString isEqual:]` |
| 0x1c05d2cc4 | `-[NSString hash]` |
| 0x1c05d2cc8 | `-[NSString isEqualToString:]` |
| 0x1c05d2d1c | `-[NSString compare:options:range:]` |
| 0x1c05d2d34 | `-[NSString compare:options:range:locale:]` |
| 0x1c05d2e40 | `-[NSString compare:]` |
| 0x1c05d2e88 | `-[NSString caseInsensitiveCompare:]` |
| 0x1c05d30c0 | `-[NSString rangeOfString:options:range:locale:]` |
| 0x1c05d32f8 | `-[NSString rangeOfString:options:range:]` |
| 0x1c05d3348 | `-[NSString rangeOfString:]` |
| 0x1c05d3384 | `-[NSString rangeOfCharacterFromSet:options:range:]` |
| 0x1c05d352c | `-[NSString rangeOfCharacterFromSet:options:]` |
| 0x1c05d3570 | `-[NSString rangeOfCharacterFromSet:]` |
| 0x1c05d35a8 | `-[NSString componentsSeparatedByString:]` |
| 0x1c05d3a2c | `-[NSString hasPrefix:]` |
| 0x1c05d3a74 | `-[NSString hasSuffix:]` |
| 0x1c05d3b84 | `-[NSString containsString:]` |
| 0x1c05d3cec | `-[NSString stringByTrimmingCharactersInSet:]` |
| 0x1c05d4208 | `-[NSString stringByReplacingOccurrencesOfString:withString:options:range:]` |
| 0x1c05d43f4 | `-[NSString getCharacters:range:]` |
| 0x1c05d4478 | `-[NSString _newSubstringWithRange:zone:]` |
| 0x1c05d45b0 | `-[NSString substringWithRange:]` |
| 0x1c05d4674 | `-[NSString substringFromIndex:]` |
| 0x1c05d4718 | `-[NSString substringToIndex:]` |
| 0x1c05d47c0 | `-[NSString stringByAppendingString:]` |
| 0x1c05d4908 | `-[NSString doubleValue]` |
| 0x1c05d4990 | `-[NSString floatValue]` |
| 0x1c05d4a1c | `-[NSString intValue]` |
| 0x1c05d4aa0 | `-[NSString longLongValue]` |
| 0x1c05d4b20 | `-[NSString integerValue]` |
| 0x1c05d4b24 | `-[NSString boolValue]` |
| 0x1c05d4e44 | `-[NSString uppercaseString]` |
| 0x1c05d4e4c | `-[NSString lowercaseString]` |
| 0x1c05d4e54 | `-[NSString capitalizedString]` |
| 0x1c05d4eec | `-[NSString uppercaseStringWithLocale:]` |
| 0x1c05d4f44 | `-[NSString lowercaseStringWithLocale:]` |
| 0x1c05d4f9c | `-[NSString capitalizedStringWithLocale:]` |
| 0x1c05d5204 | `-[NSString _getBlockStart:end:contentsEnd:forRange:stopAtLineSeparators:]` |
| 0x1c05d5a48 | `-[NSString enumerateSubstringsInRange:options:usingBlock:]` |
| 0x1c05d6198 | `___58-[NSString enumerateSubstringsInRange:options:usingBlock:]_block_invoke` |
| 0x1c05d61e0 | `___58-[NSString enumerateSubstringsInRange:options:usingBlock:]_block_invoke_2` |
| 0x1c05d6220 | `___58-[NSString enumerateSubstringsInRange:options:usingBlock:]_block_invoke_3` |
| 0x1c05d6410 | `___58-[NSString enumerateSubstringsInRange:options:usingBlock:]_block_invoke_4` |
| 0x1c05d69d0 | `__NSEncodingToNewDataForExtendedAttribute` |
| 0x1c05d6c4c | `-[NSString getExternalRepresentation:extendedAttributes:forWritingToURLOrPath:usingEncoding:error:]` |
| 0x1c05d6df8 | `_writeStringToURLOrPath` |
| 0x1c05d6f50 | `-[NSString writeToFile:atomically:]` |
| 0x1c05d6f88 | `-[NSString writeToURL:atomically:]` |
| 0x1c05d7068 | `-[NSString initWithCString:length:]` |
| 0x1c05d718c | `-[NSString initWithCString:encoding:]` |
| 0x1c05d7224 | `-[NSString initWithUTF8String:]` |
| 0x1c05d72f4 | `-[NSString initWithCharacters:length:]` |
| 0x1c05d7380 | `-[NSString initWithString:]` |
| 0x1c05d7438 | `-[NSString initWithData:encoding:]` |
| 0x1c05d7544 | `-[NSString _initWithDataOfUnknownEncoding:]` |
| 0x1c05d7640 | `-[NSString initWithContentsOfFile:]` |
| 0x1c05d7690 | `-[NSString initWithContentsOfURL:]` |
| 0x1c05d76e0 | `-[NSString initWithContentsOfURL:encoding:error:]` |
| 0x1c05d78b4 | `-[NSString initWithContentsOfFile:encoding:error:]` |
| 0x1c05d7d54 | `-[NSString initWithFormat:]` |
| 0x1c05d7dac | `-[NSString initWithFormat:locale:]` |
| 0x1c05d7e00 | `-[NSString initWithFormat:arguments:]` |
| 0x1c05d8058 | `-[NSString classForCoder]` |
| 0x1c05d8064 | `-[NSString _allowsDirectEncoding]` |
| 0x1c05d806c | `+[NSString supportsSecureCoding]` |
| 0x1c05d8074 | `-[NSString encodeWithCoder:]` |
| 0x1c05d8290 | `-[NSString initWithCoder:]` |
| 0x1c05d8428 | `+[NSString _subclassesMustBeExplicitlyMentionedWhenDecoded]` |
| 0x1c0fe53e0 | `-[NSString length]` |
| 0x1c0fe5400 | `-[NSString characterAtIndex:]` |
| 0x1c0fe5420 | `-[NSString initWithCharactersNoCopy:length:freeWhenDone:]` |
| 0x1c0fe548c | `-[NSString initWithBytesNoCopy:length:encoding:freeWhenDone:]` |
| 0x1c0fe54d4 | `-[NSString initWithFormat:locale:arguments:]` |

### ONSStringCFAdditions.m

| Native address | Native function |
| --- | --- |
| 0x1c04d1f64 | `-[NSString(NSCFAdditions) _cfTypeID]` |
| 0x1c04d1f68 | `-[NSString(NSCFAdditions) _createSubstringWithRange:]` |
| 0x1c04d20a8 | `-[NSString(NSCFAdditions) _encodingCantBeStoredInEightBitCFString]` |
| 0x1c04d2100 | `-[NSString(NSCFAdditions) _fastestEncodingInCFStringEncoding]` |
| 0x1c04d2114 | `-[NSString(NSCFAdditions) _getCString:maxLength:encoding:]` |
| 0x1c04d21d8 | `-[NSMutableString(NSCFAdditions) appendCharacters:length:]` |
| 0x1c04d221c | `-[NSMutableString(NSCFAdditions) _cfAppendCString:length:]` |
| 0x1c04d2260 | `-[NSMutableString(NSCFAdditions) _cfLowercase:]` |
| 0x1c04d229c | `-[NSMutableString(NSCFAdditions) _cfUppercase:]` |
| 0x1c04d22d8 | `-[NSMutableString(NSCFAdditions) _cfCapitalize:]` |
| 0x1c04d2314 | `-[NSMutableString(NSCFAdditions) _cfPad:length:padIndex:]` |

### ONSStringDecimalExtension.m

| Native address | Native function |
| --- | --- |
| 0x1c04fb7d8 | `-[NSString(NSDecimalExtension) decimalValue]` |

### ONSStringDistantString.m

| Native address | Native function |
| --- | --- |
| 0x1c0500628 | `-[NSMutableString(NSDistantString) replacementObjectForPortCoder:]` |
| 0x1c050066c | `-[NSString(NSDistantString) _getCharactersAsStringInRange:]` |
| 0x1c0500670 | `-[NSString(NSDistantString) _getBytesAsData:maxLength:usedLength:encoding:options:range:remainingRange:]` |

### ONSStringFromRange.m

| Native address | Native function |
| --- | --- |
| 0x1c05bd710 | `_NSStringFromRange` |

### ONSStringOriginalAttributedString.m

| Native address | Native function |
| --- | --- |
| 0x1c04d10f8 | `-[NSString(_NSOriginalAttributedString) __baseAttributedString]` |

### ONSStringOtherEncodings.m

| Native address | Native function |
| --- | --- |
| 0x1c05d9e2c | `-[NSString(__NSLocalizedStringAddition) _copyFormatStringWithConfiguration:]` |
| 0x1c05d9e8c | `-[NSString(__NSLocalizedStringAddition) formatConfiguration]` |
| 0x1c05d9ea4 | `___NSSetCStringCharToUnichar` |
| 0x1c05d9fdc | `__NSDefaultCStringEncoding` |
| 0x1c05da068 | `+[NSString(NSStringOtherEncodings) defaultCStringEncoding]` |
| 0x1c05da284 | `-[NSString(NSStringOtherEncodings) UTF8String]` |
| 0x1c05da298 | `_bytesInEncoding` |
| 0x1c05da490 | `-[NSString(NSStringOtherEncodings) cString]` |
| 0x1c05da5e0 | `-[NSString(NSStringOtherEncodings) fastestEncoding]` |
| 0x1c05da730 | `-[NSString(NSStringOtherEncodings) dataUsingEncoding:allowLossyConversion:]` |
| 0x1c05da92c | `-[NSString(NSStringOtherEncodings) dataUsingEncoding:]` |
| 0x1c05da934 | `-[NSString(NSStringOtherEncodings) cStringUsingEncoding:]` |
| 0x1c05da948 | `-[NSString(NSStringOtherEncodings) getCString:maxLength:encoding:]` |
| 0x1c05daaac | `-[NSString(NSStringOtherEncodings) lengthOfBytesUsingEncoding:]` |
| 0x1c05dab98 | `-[NSString(NSStringOtherEncodings) getBytes:maxLength:usedLength:encoding:options:range:remainingRange:]` |
| 0x1c05dad24 | `-[NSString(NSStringOtherEncodings) getBytes:maxLength:filledLength:encoding:allowLossyConversion:range:remainingRange:]` |
| 0x1c05dad38 | `__NSConvertToASCIIorUnicode2` |
| 0x1c0fdedf0 | `___NSASCIICharToUnichar` |
| 0x1c0fdedf4 | `___NSCharToUnicharCFWrapper` |

### ONSStringPortCoding.m

| Native address | Native function |
| --- | --- |
| 0x1c064e804 | `-[NSString(NSStringPortCoding) replacementObjectForPortCoder:]` |

### ONSStringRegularExpressionSupport.m

| Native address | Native function |
| --- | --- |
| 0x1c063fb14 | `-[NSString(NSRegularExpressionSupport) _rangeOfRegularExpressionPattern:options:range:locale:]` |
| 0x1c063fc3c | `-[NSString(NSRegularExpressionSupport) _stringByReplacingOccurrencesOfRegularExpressionPattern:withTemplate:options:range:]` |
| 0x1c063fd6c | `-[NSMutableString(NSRegularExpressionSupport) _replaceOccurrencesOfRegularExpressionPattern:withTemplate:options:range:]` |

### ONSStringScanning.m

| Native address | Native function |
| --- | --- |
| 0x1c05c0c74 | `__NSScanLongLongFromString` |
| 0x1c05c11f8 | `__NSScanIntFromString` |
| 0x1c05c177c | `__NSCheckDecimalSeparator` |
| 0x1c05c18d0 | `__NSScanDoubleFromString` |

### ONSStringURLUtilities.m

| Native address | Native function |
| --- | --- |
| 0x1c05ebbc4 | `-[NSString(NSURLUtilities) stringByAddingPercentEncodingWithAllowedCharacters:]` |
| 0x1c05ebd44 | `-[NSString(NSURLUtilities) stringByReplacingPercentEscapesUsingEncoding:]` |

### ONSZone.m

| Native address | Native function |
| --- | --- |
| 0x1c0593e6c | `_NSZoneMalloc` |

## Temporary interfaces: exact contents

```objc
//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/Locale/OCFLocale.h"
#include "CoreFoundation/String/OCFString.h"
#include <OpenFoundation/Base/ONSObjCRuntime.h>

#import "Foundation/String/ONSString.h"
#import "Foundation/String/ONSMutableString.h"
#include <OpenFoundation/Decimal/ONSDecimal.h>
#include <objc/objc.h>
#include <dispatch/dispatch.h>
#include <OpenFoundation/Range/ONSRange.h>
#include "CoreFoundation/CharacterSet/OCFCharacterSet.h"

// Temporary interfaces needed by FS-string from other restoration families. The coordinator replaces these
// declarations with their owners' headers at integration. These are declarations, never replacement bodies.
// Reference: iOS Simulator 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03 unless stated.
// CoreFoundation UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC. iOS Device 24A446: unread.

// Native CFStringHashNSString (CoreFoundation, external), the tail call of -[NSString hash]; SDK-style
// signature from the call site (the receiver in X0, the hash in X0).
// Native CFStringHashNSString, 0x1c0209620; CoreFoundation string/URL/encoding family (interface metadata).
OF_EXPORT OCFHashCode OCFStringHashNSString(OCFStringRef string);

// CoreFoundation's string tokenizer (CFStringTokenizer.h; external natively). SDK signatures.
typedef struct __OCFStringTokenizer *OCFStringTokenizerRef;
typedef OCFOptionFlags OCFStringTokenizerTokenType;
// Native CFStringTokenizerCreate, 0x1c022ee70; CoreFoundation string/URL/encoding family (interface
// metadata).
OF_EXPORT OCFStringTokenizerRef OCFStringTokenizerCreate(
    OCFAllocatorRef alloc, OCFStringRef string, OCFRange range, OCFOptionFlags options, OCFLocaleRef locale
);
// Native CFStringTokenizerSetString, 0x1c022ef34; CoreFoundation string/URL/encoding family (interface
// metadata).
OF_EXPORT void
OCFStringTokenizerSetString(OCFStringTokenizerRef tokenizer, OCFStringRef string, OCFRange range);
// Native CFStringTokenizerGoToTokenAtIndex, 0x1c022ef44; CoreFoundation string/URL/encoding family (interface
// metadata).
OF_EXPORT OCFStringTokenizerTokenType
OCFStringTokenizerGoToTokenAtIndex(OCFStringTokenizerRef tokenizer, OCFIndex index);
// Native CFStringTokenizerAdvanceToNextToken, 0x1c022ef54; CoreFoundation string/URL/encoding family
// (interface metadata).
OF_EXPORT OCFStringTokenizerTokenType OCFStringTokenizerAdvanceToNextToken(OCFStringTokenizerRef tokenizer);
// Native CFStringTokenizerGetCurrentTokenRange, 0x1c022ef64; CoreFoundation string/URL/encoding family
// (interface metadata).
OF_EXPORT OCFRange OCFStringTokenizerGetCurrentTokenRange(OCFStringTokenizerRef tokenizer);

// CFStringCapitalize and CFStringConvertEncodingToIANACharSetName (CFString.h; external natively).
// Native CFStringCapitalize, 0x1c021ed30; CoreFoundation string/URL/encoding family (interface metadata).
OF_EXPORT void OCFStringCapitalize(OCFMutableStringRef theString, OCFLocaleRef locale);
// Native CFStringConvertEncodingToIANACharSetName, 0x1c0233ffc; CoreFoundation string/URL/encoding family
// (interface metadata).
OF_EXPORT OCFStringRef OCFStringConvertEncodingToIANACharSetName(OCFStringEncoding encoding);

// Native _NSIsNSURL (external), in the CoreFoundation image's Objective-C type-check cluster beside
// _NSIsNSString (Objective-C/OCFNSTypeChecks.m); bit 0 of the result is tested (a 32-bit test).
OF_EXPORT BOOL _ONSIsNSURL(id object);

// Native _CFStringCreateWithNullTerminatedBytesNoCopy (CoreFoundation, external), the string over
// NUL-terminated bytes without copying them. Signature from the caller's register use (allocator, bytes,
// length without the terminator, CFStringEncoding, contents deallocator); confidence: medium.
// Native _CFStringCreateWithNullTerminatedBytesNoCopy, 0x1c020ac08; CoreFoundation string/URL/encoding family
// (interface metadata).
OF_EXPORT OCFStringRef _OCFStringCreateWithNullTerminatedBytesNoCopy(
    OCFAllocatorRef alloc,
    const uint8_t *bytes,
    OCFIndex numBytes,
    OCFStringEncoding encoding,
    OCFAllocatorRef contentsDeallocator
);

// Native NSCocoaErrorDomain (0x2227d1270), NSFilePathErrorKey (0x2227d1200), NSURLErrorKey (0x2227d11f8) and
// NSStringEncodingErrorKey (0x2227d11f0), external, __DATA_CONST,__const of the Foundation image: the error
// domain and user info key constants of NSError's unit.
OF_EXPORT id const ONSCocoaErrorDomain;
OF_EXPORT id const ONSFilePathErrorKey;
OF_EXPORT id const ONSURLErrorKey;
OF_EXPORT id const ONSStringEncodingErrorKey;

// Native NSCharacterConversionException (0x2227d05d0, __DATA_CONST,__const, external) of the Foundation
// image: the exception name -cString and -UTF8String raise with (SDK NSString.h); not in this family's
// entries.
OF_EXPORT id const ONSCharacterConversionException;

// CoreFoundation's CFStringIsEncodingAvailable and CFStringCreateFromExternalRepresentation (external, SDK
// CFString.h), not restored on this branch.
// Native CFStringIsEncodingAvailable, 0x1c0233de8; CoreFoundation string/URL/encoding family (interface
// metadata).
OF_EXPORT Boolean OCFStringIsEncodingAvailable(OCFStringEncoding encoding);
// Native CFStringCreateFromExternalRepresentation, 0x1c021b1d8; CoreFoundation string/URL/encoding family
// (interface metadata).
OF_EXPORT OCFStringRef
OCFStringCreateFromExternalRepresentation(OCFAllocatorRef alloc, OCFDataRef data, OCFStringEncoding encoding);

// Native _CFGetPathExtensionRangesFromPathComponentUniChars (external in the CoreFoundation image, not
// restored on this branch): the primary and secondary extension ranges of a path component; a range not
// found has location kCFNotFound. Signature from the caller's register use (characters, length, two range
// pointers; the result is not read).
// Native _CFGetPathExtensionRangesFromPathComponentUniChars, 0x1c025832c; CoreFoundation string/URL/encoding
// family (interface metadata).
OF_EXPORT void _OCFGetPathExtensionRangesFromPathComponentUniChars(
    const UniChar *characters,
    OCFIndex length,
    OCFRange *primaryExtensionRange,
    OCFRange *secondaryExtensionRange
);

// Native __isEqualCString (0x1c0592180) and __hashCString2 (0x1c05921c0), non-external functions of the
// Foundation image that the stat cache's key callbacks (0x22281cea8) reference; they are outside the capture
// and stay undefined in this batch. Supplemental IDA captures now record both bodies; they remain a
// coordinator dependency outside the 226 assigned entries. Signatures are CFDictionary's equal/hash
// callbacks.
OF_EXTERN Boolean _isEqualCString(const void *value1, const void *value2);
OF_EXTERN OCFHashCode _hashCString2(const void *value);

// Native _CFStringCreateByAddingPercentEncodingWithAllowedCharacters, CoreFoundation external; caller
// -[NSString(NSURLUtilities) stringByAddingPercentEncodingWithAllowedCharacters:], 0x1c05ebbc4. CF URL/string
// family; the allocator, string and allowed character set are passed in X0-X2, a +1 string returned in X0.
// Native _CFStringCreateByAddingPercentEncodingWithAllowedCharacters, 0x1c0270cc8; CoreFoundation
// string/URL/encoding family (interface metadata).
OF_EXPORT OCFStringRef _OCFStringCreateByAddingPercentEncodingWithAllowedCharacters(
    OCFAllocatorRef allocator, OCFStringRef string, OCFCharacterSetRef allowedCharacters
);

// Foundation's shared Swift-character-set gate, FM/FC integration dependency. Hidden native data from the
// pinned Foundation image: __foundation_swift_characterSet_enabled.onceToken (0x22e7761f8, __common),
// .featureEnabled (0x22e776200, __common) and ___block_literal_global_10 (0x22281c8d8, __const; its invoke is
// 0x1c0fdc9cc). Native labels containing dots are spelled with assembler labels. No separate gate or copied
// state is supplied by FS-string: the coordinator must expose these from the shared gate's owner.
OF_EXTERN dispatch_once_t
    foundationCharacterSetOnceToken __asm("__foundation_swift_characterSet_enabled.onceToken");
OF_EXTERN uint8_t
    foundationCharacterSetFeatureEnabled __asm("__foundation_swift_characterSet_enabled.featureEnabled");
OF_EXTERN const unsigned char foundationCharacterSetEnableBlock[] __asm("___block_literal_global_10");

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native NSCache, CoreFoundation class import through Foundation GOT 0x22227FAA8 in the initializer
// ____createRegexForPattern_block_invoke, 0x1c063fe88 (binding confidence: medium). Message-only interface;
// no ivars or class implementation. The CoreFoundation cache family supplies its class object.
@interface ONSCache : NSObject
- (void)setName:(id)name;
- (void)setCountLimit:(ONSUInteger)limit;
- (id)objectForKey:(id)key;
- (void)setObject:(id)object forKey:(id)key;
@end

// Native NSRegularExpression class, 0x22e6e0598. Foundation regex family, outside batch 8; these selectors
// are sent by the three captured NSRegularExpressionSupport methods, 0x1c063fb14-0x1c063fe88.
@interface ONSRegularExpression : NSObject
- (id)initWithPattern:(id)pattern options:(ONSUInteger)options error:(id *)error;
- (ONSRange)rangeOfFirstMatchInString:(id)string options:(ONSUInteger)options range:(ONSRange)range;
- (id)stringByReplacingMatchesInString:(id)string
                               options:(ONSUInteger)options
                                 range:(ONSRange)range
                          withTemplate:(id)replacementTemplate;
- (ONSUInteger)replaceMatchesInString:(id)string
                              options:(ONSUInteger)options
                                range:(ONSRange)range
                         withTemplate:(id)replacementTemplate;
@end

// Messages into the shared locale and character-set families from 0x1c05c18d0 and 0x1c05ebbc4. These are
// only method declarations; no protocol object is emitted.
@protocol StringSharedDependencyMessages
- (id)decimalSeparator;
- (id)__copySwiftStringByAddingPercentEncodingWithAllowedCharacters:(id)allowedCharacters;
@end
#endif

#ifdef __OBJC__
// Foundation classes outside this family that the restored code references by name (class symbols and runtime
// names), so that the prefix tool maps them. Those not restored on this branch keep undefined class symbols.
@class ONSArray;
@class ONSAttributedString;
@class ONSCharacterSet;
@class ONSCheapMutableString;
@class ONSData;
@class ONSDecimalNumber;
@class ONSDictionary;
@class ONSError;
@class ONSException;
@class ONSFileManager;
@class ONSKeyedArchiver;
@class ONSMutableSet;
@class ONSLocale;
@class ONSMutableArray;
@class ONSMutableData;
@class ONSNumber;
@class ONSPathStore2;
@class ONSRegularExpression;
@class ONSScanner;
@class ONSURL;
@class __ONSLocalizedString;
// CoreFoundation's tagged string class, looked up by name in +[NSString initialize].
@class ONSTaggedPointerString;
// CFNetwork's classes (an external image) that -initWithContentsOfURL:encoding:error: references.
@class ONSHTTPURLResponse;
@class ONSURLConnection;
@class ONSURLRequest;
#endif

#ifdef __OBJC__
extern char fileErrorClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSError));
extern char fileNumberClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSNumber));
extern char fileDictionaryClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDictionary));
extern char fileURLClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSURL));
extern char fileCheapMutableStringClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSCheapMutableString));
extern char mutableStringExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char pathStoreClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSPathStore2));
extern char pathCheapMutableStringClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSCheapMutableString));
extern char pathFileManagerClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSFileManager));
extern char pathMutableArrayClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableArray));
extern char pathMutableSetClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableSet));
extern char pathDataClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSData));
extern char pathExceptionClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char placeholderMutableStringExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char placeholderExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char placeholderAttributedStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSAttributedString));
extern char exceptionClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char arrayClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSArray));
extern char mutableArrayClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableArray));
extern char localeClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSLocale));
extern char placeholderStringClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSPlaceholderString));
extern char characterSetClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSCharacterSet));
extern char dataClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSData));
extern char mutableDataClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableData));
extern char dictionaryClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDictionary));
extern char urlClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSURL));
extern char keyedArchiverClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSKeyedArchiver));
extern char errorClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSError));
extern char httpURLResponseClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSHTTPURLResponse));
extern char urlConnectionClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSURLConnection));
extern char urlRequestClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSURLRequest));
extern char stringCFAdditionsExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char scannerClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSScanner));
extern char decimalNumberClassSymbol[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDecimalNumber));
extern char encodingsLocalizedStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSLocalizedString));
extern char encodingsExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char encodingsDictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDictionary));
extern char encodingsMutableDataClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableData));

@protocol StringForeignMessages

+ (id)defaultManager;
+ (void)_setAsTaggedStringClass;
- (BOOL)fileExistsAtPath:(id)path;
- (const void *)bytes;
- (void *)mutableBytes;
- (void)setLength:(ONSUInteger)length;
+ (id)dataWithLength:(ONSUInteger)length;
+ (id)dataWithBytesNoCopy:(void *)bytes length:(ONSUInteger)length;
- (id)initWithLength:(ONSUInteger)length;
- (id)initWithBytes:(const void *)bytes length:(ONSUInteger)length;
- (BOOL)writeToURL:(id)url options:(ONSUInteger)options error:(id *)error;
- (ONSInteger)code;
+ (id)errorWithDomain:(id)domain code:(ONSInteger)code userInfo:(id)userInfo;
+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
- (id)objectForKey:(id)key;
- (void)setObject:(id)object forKey:(id)key;
+ (id)dictionaryWithObjectsAndKeys:(id)firstObject, ...;
- (id)initWithObjectsAndKeys:(id)firstObject, ...;
+ (id)array;
+ (id)arrayWithObject:(id)object;
- (void)addObject:(id)object;
- (BOOL)containsObject:(id)object;
- (BOOL)isFileURL;
- (id)path;
+ (id)fileURLWithPath:(id)path;
- (BOOL)allowsKeyedCoding;
- (BOOL)containsValueForKey:(id)key;
- (const uint8_t *)decodeBytesForKey:(id)key returnedLength:(ONSUInteger *)length;
- (void *)decodeBytesWithReturnedLength:(ONSUInteger *)length;
- (void)encodeBytes:(const void *)bytes length:(ONSUInteger)length;
- (void)encodeBytes:(const uint8_t *)bytes length:(ONSUInteger)length forKey:(id)key;
- (ONSInteger)versionForClassName:(id)className;
- (void)failWithError:(id)error;
- (id)_decodePropertyListForKey:(id)key;
- (void)_encodePropertyList:(id)propertyList forKey:(id)key;
- (BOOL)isByref;
+ (id)whitespaceCharacterSet;
+ (id)decimalDigitCharacterSet;
- (BOOL)characterIsMember:(unsigned short)character;
+ (id)numberWithUnsignedInteger:(ONSUInteger)value;
- (id)descriptionWithLocale:(id)locale;
- (id)copyWithZone:(struct _NSZone *)zone;

@end

@protocol FileUtilitiesMessages

- (BOOL)getFileSystemRepresentation:(char *)buffer maxLength:(ONSUInteger)maxLength withPath:(id)path;
- (void)setContentsNoCopy:(void *)contents
                   length:(ONSUInteger)length
             freeWhenDone:(BOOL)freeWhenDone
                isUnicode:(BOOL)isUnicode;
+ (id)pathStoreWithCharacters:(const unsigned short *)characters length:(ONSUInteger)length;
- (BOOL)__swiftFillFileSystemRepresentationWithPointer:(char *)buffer maxLength:(ONSInteger)maxLength;

@end

@protocol PlaceholderStringMessages

- (id)_initWithFormat:(id)format options:(ONSUInteger)options locale:(id)locale arguments:(va_list)arguments;
- (id)string;
- (id)_dynamicContextEvaluation:(const void *)context patternString:(const void *)patternString;

@end

@protocol StringOtherEncodingsMessages

- (id)baseString;
- (id)initWithString:(id)string withFormatConfiguration:(id)configuration;

@end

@protocol StringDecimalExtensionMessages

+ (id)scannerWithString:(id)string;
- (BOOL)scanDecimal:(ONSDecimal *)decimal;
+ (id)notANumber;

@end

@protocol StringDistantStringMessages

- (id)_proxyForLocalObject:(id)object;

@end

@protocol StringPartBMessages

- (id)initWithContentsOfMappedFile:(id)path;
+ (BOOL)_writeDataToPath:(id)path
                           data:(id)data
                        options:(ONSUInteger)options
    stringEncodingAttributeData:(id)attributeData
                 reportProgress:(BOOL)reportProgress
                          error:(id *)error;
+ (BOOL)_writeDataToPath:(id)path
                    data:(id)data
                 options:(ONSUInteger)options
          reportProgress:(BOOL)reportProgress
                   error:(id *)error;
+ (BOOL)_readBytesFromPath:(id)path
                 maxLength:(ONSUInteger)maxLength
                     bytes:(void **)bytes
                    length:(ONSUInteger *)length
                    didMap:(BOOL *)didMap
                   options:(ONSUInteger)options
            reportProgress:(BOOL)reportProgress
                     error:(id *)error;
- (id)_securePath;
- (id)initWithURL:(id)url cachePolicy:(ONSUInteger)cachePolicy timeoutInterval:(double)timeoutInterval;
+ (id)sendSynchronousRequest:(id)request returningResponse:(id *)response error:(id *)error;
- (ONSInteger)statusCode;
+ (BOOL)isErrorStatusCode:(ONSInteger)statusCode;
+ (id)_readCorruptErrorWithFormat:(id)format, ...;

@end
#endif

#ifdef __OBJC__
// Foreign Foundation class records read through IDA on 2026/11/03. NSObject's shared-cache superclass
// address is 0x22e6b6088 (name binding confidence: medium); all other superclass links below are in-image.
// No ivars are declared: these are message-only interfaces, not restored class layouts.

// Native NSAttributedString, 0x22e6d6138; owner FC-sets-attributed.
@interface ONSAttributedString : NSObject
- (id)_initWithFormat:(id)format options:(ONSUInteger)options locale:(id)locale arguments:(va_list)arguments;
- (id)string;
@end

// Native NSCharacterSet, 0x22e6d6958; owner FC-sets-attributed.
@interface ONSCharacterSet : NSObject
+ (id)whitespaceCharacterSet;
+ (id)decimalDigitCharacterSet;
- (BOOL)characterIsMember:(unsigned short)character;
@end

// Native NSError, 0x22e6d78d0; owner FM-misc.
@interface ONSError : NSObject
+ (id)errorWithDomain:(id)domain code:(ONSInteger)code userInfo:(id)userInfo;
+ (id)_readCorruptErrorWithFormat:(id)format, ...;
- (ONSInteger)code;
@end

// Native NSValue (0x22e6de478), NSNumber (0x22e6de400) and NSDecimalNumber (0x22e6d7358);
// owner FN-number-value (decimal-number implementation remains outside this batch).
@interface ONSValue : NSObject
@end
@interface ONSNumber : ONSValue
+ (id)numberWithUnsignedInteger:(ONSUInteger)value;
@end
@interface ONSDecimalNumber : ONSNumber
+ (id)notANumber;
- (ONSDecimal)decimalValue;
@end

// Native NSScanner, 0x22e6dcee8; owner FM-misc.
@interface ONSScanner : NSObject
+ (id)scannerWithString:(id)string;
- (BOOL)scanDecimal:(ONSDecimal *)decimal;
@end

// Native NSCoder (0x22e6d6ae8) and NSKeyedArchiver (0x22e6dadf0); owner FK-keyed-archiving.
@interface ONSCoder : NSObject
- (BOOL)allowsKeyedCoding;
- (BOOL)containsValueForKey:(id)key;
- (const uint8_t *)decodeBytesForKey:(id)key returnedLength:(ONSUInteger *)length;
- (void *)decodeBytesWithReturnedLength:(ONSUInteger *)length;
- (void)encodeBytes:(const void *)bytes length:(ONSUInteger)length;
- (void)encodeBytes:(const uint8_t *)bytes length:(ONSUInteger)length forKey:(id)key;
- (ONSInteger)versionForClassName:(id)className;
- (void)failWithError:(id)error;
- (id)_decodePropertyListForKey:(id)key;
- (void)_encodePropertyList:(id)propertyList forKey:(id)key;
@end
@interface ONSKeyedArchiver : ONSCoder
@end

// Native NSFileManager (0x22e6d7cb8), NSCheapMutableString (0x22e6db6d8), NSPathStore2 (0x22e6dc268)
// and __NSLocalizedString (0x22e6dd5c8); their Foundation families are outside this batch.
@interface ONSFileManager : NSObject
+ (id)defaultManager;
- (BOOL)fileExistsAtPath:(id)path;
- (BOOL)getFileSystemRepresentation:(char *)buffer maxLength:(ONSUInteger)maxLength withPath:(id)path;
@end
@interface ONSCheapMutableString : ONSMutableString
- (void)setContentsNoCopy:(void *)contents
                   length:(ONSUInteger)length
             freeWhenDone:(BOOL)freeWhenDone
                isUnicode:(BOOL)isUnicode;
@end
@interface ONSPathStore2 : ONSString
+ (id)pathWithComponents:(id)components;
+ (id)pathStoreWithCharacters:(const unsigned short *)characters length:(ONSUInteger)length;
@end
@interface __ONSLocalizedString : ONSMutableString
- (id)baseString;
- (id)initWithString:(id)string withFormatConfiguration:(id)configuration;
@end
#endif
```

<!--
  Created by ktiays on 2026/11/03.
  Copyright (c) 2026 ktiays. All rights reserved.
-->

# Captured Foundation keyed archiving scope

## Identity and evidence

This reading covers the 147 `FK-keyed-archiving` entries in
`Local/Evidence/cf-core/iphone-simulator-foundation-link.json` on the coordinating checkout. It describes
source restoration of that capture, with unresolved dependencies below. It does not establish a complete
class, a closed dependency group, behavioral coverage, or consumer replacement.

The reference is Foundation, iOS 27.0 Simulator (24A434), arm64, UUID
`49FCDA5F-A93E-3259-857C-FC75AC2C7B03`, SHA-256
`bc7e0a5f693ffc75ae7c3311b817716e86d149fda2f8c45669a976727322cb18`.
`Configuration/ReferenceImages.json` pins its identity. The iOS Device reference is Foundation, iOS 27.0.1
(24A446), arm64e, UUID `C041F1D0-5EF1-30A3-BF96-830976173B56`; its bodies remain unread. No iOS Device match
is asserted.

The supplied IDA pseudocode and assembly are the implementation evidence. Follow-up IDA reads use an APFS
clone under the coordinating checkout's `Local/Scratch/cf-core-fd-archiving`, never the canonical database.
The current IDA service is session-bound: it rejects `database_id` unless started with `--workspace`.
The session opened only that clone. Additional evidence in that scratch directory consists of
`class-records.json`, `static-data.json`, `generic-keys.json`, `selectors.json`, `bindings.json`,
`exception-gaps.json`, `exception-tables.json`, and `missing-methods.json`.

For shared-cache slots outside the extracted image, IDA read the pinned Simulator cache's static mappings
and data. All 147 captured function ranges matched the database bytes before those bindings were used.
`nm -m` establishes symbol identity and linkage only. No runtime introspection, foreign Foundation source,
or system forwarding supplies implementation behavior. Confidence in the recorded Simulator instructions,
class layouts, and resolved data bindings is **verified**. Executable behavior remains untested at this stage.
There are no remaining medium-confidence data bindings in these units.

`Archiving/` and the five implementation units are an inferred subsystem organization; a native source path
was not recovered. The plain helper functions retain their boundaries within Objective-C MRC units. Their
C calling convention is not evidence of the native source language.

## Definition inventory and layouts

| Unit | Captured entries | Boundaries |
|---|---:|---|
| `ONSCoder.m` | 19 | Thirteen direct methods, trust function, three folded block invokes, two compiler byref helpers |
| `ONSKeyedArchiver.m` | 50 | Archiver methods and encode helpers, including the UID cache helper |
| `ONSKeyedUnarchiver.m` | 72 | Unarchiver methods, binary/XML decoders, primitives and class-allocation checks |
| `ONSObjCRuntimeNames.m` | 5 | Selector/class name conversion and Copy functions |
| `ONSRuntimeIssuesLog.m` | 1 | Runtime issue log accessor; its initializer remains undefined |

Each entry's native name and address are cited at its source definition or folded block. The two MRC
byref helpers at `0x1c0fdc1f8` and `0x1c0fdc208` are compiler-emitted from `__block id`, using object
copy/dispose flags `0x83`; they are not separately callable restored APIs.

| Native class | Class / class_ro_t | Superclass | Instance start / size |
|---|---|---|---|
| NSCoder | `0x22e6d6ae8` / `0x222849ea0` | NSObject `0x22e6b6088` | 8 / 8, no ivars |
| NSKeyedArchiver | `0x22e6dadf0` / `0x222857290` | NSCoder | 8 / 128, fifteen ivars |
| NSKeyedUnarchiver | `0x22e6dadc8` / `0x2228576d0` | NSCoder | 8 / 136, sixteen ivars |
| _NSKeyedUnarchiverHelper | `0x22e6dae68` / `0x222857400` | NSObject | 8 / 42, six ivars; class undefined here |

All ivars of these four records were read, including their offsets, widths and type encodings; no unread
ivar is substituted. The owned headers record each field. Archiver flags are at 16, its generic key at 88,
and its UID cache and capacity at 96 and 104. Unarchiver flags are at 16, its four-byte generic key at 64,
its offset record at 72, bytes/length at 104/112, and helper at 120. The 48-byte offset record contains a
32-byte binary plist trailer, then container and objects offsets at 32 and 40. The helper overlay records
allowed array at 8, last reference at 16, allowed-class stack at 24, error at 32, and collection flags at
40/41; it does not allocate or implement that foreign class. Full method lists, category surfaces and
protocol conformance metadata are outside this captured class restoration.

The thirteen NSCoder methods and the two explicitly annotated unarchiver methods use `objc_direct`, with
no `_cmd` argument; Clang supplies their native nil-receiver handling. Other methods use ordinary Objective-C
dispatch. No categories are implemented on system-owned classes. The private NSObject category provides
message declarations only. Direct class-reference aliases preserve native loads without introducing a
`+class` message. Atomic flags preserve acquire loads, release stores and the observed acquire-release
fetch-or. Object ownership is MRC, with the observed retain/release/autorelease calls and cleanup order.

## Reading decisions and retained behavior

- `_NSCoderGetTrustRestrictions` (`0x1c04dc3a4`), collection-class state and explicit-subclass state use native
  once blocks. `_shouldExemptProcessBasedOnArchivingExceptionRules:` (`0x1c04dc9fc`, invoke `0x1c04dcde0`)
  retains the process-wide first-path result (kept). Inlined once checks are kept at their native callers.
- `_NSCoderCheckPotentiallyMangledClassNameAndWarn` (`0x1c0556fbc`) recognizes a name longer than two bytes
  beginning `_T`, caches only a positive selector-response result, and sends the 32-bit `archiving XOR 1`
  operation (kept).
- `_temporaryMapReplaceObject:withObject:` (`0x1c055cde8`) replaces only the first matching value (kept).
  Instructions `0x1c055cea0-0x1c055ced4` allocate two distinct stack buffers; the decompiler aliases them.
  The heap branch starts above 512 bytes and retains the two observed malloc type identifiers.
- XML existence queries `containsValueForKey:` (`0x1c055d220`) and `_containsNextUnkeyedObject`
  (`0x1c0561f44`) consume an array's first value (kept). The latter does not increment the generic key.
  A key loses exactly one leading `$`. Generic keys `$0` through `$39` use the static records read at
  `0x2227d5530`; later keys use the native format and integer width.
- `_readInt` (`0x1c0560720`) advances by `1 << nibble`; nibble 4-7 values accumulate modulo 64 bits,
  and nibble 8-15 payloads advance but return zero (kept). `_getUIDFromData` (`0x1c056082c`) accepts
  markers `0x80-0x8f`, retaining the low 64 bits of long UIDs (kept). Both retain the native extra-byte
  bound `marker + payload width + 1 <= last-byte limit`.
- Compatibility encode/decode (`0x1c055a228`, `0x1c05627e4`) sign-extend `c/C` and `s/S` together; `i/I/l/L`
  use 32 bits on this arm64 image (kept). Byte decode stores a narrowed raw byte, not normalized `_Bool`.
  Range errors retain the native subsequent stores. The vararg type walkers (`0x1c0559e8c`, `0x1c05623f0`)
  advance one encoding byte per address, including composite encodings (kept).
- `_decodeFloat` (`0x1c05613e0`) accepts NaNs and infinities and uses exactly `0x47efffffe0000000`
  (`FLT_MAX` as double) for the finite limit. It converts to float even after reporting a range failure.
  `_decodeDouble` (`0x1c05615f8`) keeps its pointer bounds and successive corruption/type errors.
- Binary/XML decode (`0x1c055e1a8`, `0x1c055eeb0`) keep separate control flow: UID zero fails whereas
  `$null` can succeed with a nil result; the binary scalar path can succeed with nil after a failed parser.
  Binary class hints validate every element as a string; XML does not (kept). Repeated XML type-ID calls,
  32-bit reference indices and post-call byte-field reloads are retained. Delegate class fallback bypasses
  the helper's class-name check as read. Class selector validation (`0x1c056428c`) retains its gates and
  superclass fallback; these paths are not combined into a new helper.
- `decodingCyclesIsAllowedForCoder` (`0x1c055fd24`) keeps three flag loads and the trust call. Failure policy
  (`0x1c055fe18`, `0x1c055fe38`) uses the native complemented bit and refuses changes once decoding begins.
- Name conversions (`0x1c057e81c`, `0x1c057e880`, `0x1c057ead8`, `0x1c057eb28`, `0x1c057eb8c`) preserve
  nil handling, UTF-8/UTF-16-length fast-path checks and embedded-NUL scanning. Copy functions return +1
  strings over runtime-owned bytes without freeing them; NSStringFromClass copies and autoreleases.
  NSClassFromString uses `objc_lookUpClass` without a missing-class hook.

Hex-Rays additionally truncates pointer arguments in encode wrappers (`0x1c055811c`) and primitive wrappers,
misidentifies the receiver registers in unarchiver `replaceObject:withObject:` (`0x1c055cfa8`), and prints
unreliable uninitialized arguments after nonreturning throws. Source uses the assembly's X/W/S/D register
widths and omits unreachable fallthrough. It does not reproduce those pseudocode artifacts.

## Exception fragments omitted by the capture

IDA's split functions excluded adjacent landing pads from the supplied ranges. The follow-up reads include
their instructions and compact-unwind LSDA tables; `exception-tables.json` records protected instruction
ranges. The Objective-C `id` type entry resolves through slot `0x222409088` to `0x22241c480`, whose static
type name is `id`. A zero type entry is the separate catch-all.

| Parent | Folded landing pads | LSDA |
|---|---|---|
| NSCoder validation `0x1c04db734` | `0x1c04db98c-0x1c04db9ac` | `0x1c149fdb0` |
| Archiver convenience `0x1c05571a4` | `0x1c0557254-0x1c0557388` | `0x1c14a1de0` |
| Archiver old-style array `0x1c055a7e4` | `0x1c055a9b0-0x1c055a9d8` | `0x1c14a1e2c` |
| Unarchiver initializer `0x1c055ae40` | `0x1c055b884-0x1c055b92c` | `0x1c14a1e60` |
| Unarchiver single-class convenience `0x1c055b938` | `0x1c055b9e8-0x1c055ba28` | `0x1c14a1ef0` |
| Unarchiver class-set convenience `0x1c055bc3c` | `0x1c055bccc-0x1c055bcf8` | `0x1c14a1f9c` |
| Collection decode `0x1c055de40` | `0x1c055e034-0x1c055e07c` | `0x1c14a2118` |
| Binary decode `0x1c055e1a8` | `0x1c055eda0-0x1c055eeac` | `0x1c14a2164` |
| XML decode `0x1c055eeb0` | `0x1c055f874-0x1c055f97c` | `0x1c14a21dc` |

The convenience archiver catches Objective-C objects from encode/encodedData, optionally categorizes an
NSException, and writes Cocoa error 4866 with an underlying error or formatted debug description; normal
success leaves the error pointer untouched. Its finally calls finishEncoding, then releases the coder.
The unarchiver initializer's outer self cleanup encloses helper allocation, nil-data errors and meter
cleanup. Failure still releases self if meter invalidation throws. Its binary and XML decoders protect
`initWithCoder:` with an `id` catch: conditionally release the allocated object, remove its temporary map
entry, then categorize NSException only when the failure policy permits. A successful categorization sets
the decode error and returns false; otherwise the exception is rethrown. `awakeAfterUsingCoder:` has a
separate catch-all that removes the map entry and rethrows. Neither catch restores the container/generic-key
state on that path (kept). Captured pseudocode alone omits these exception behaviors.

## Established native defect and precise divergence

**Foundation defect:** `__NSKeyedArchiverUIDCreateCached`, `0x1c0563d14`, confidence: verified. Instructions
`0x1c0563d58-0x1c0563d70` choose `max(uid, doubled 32-bit capacity)` (initial capacity 256). The access at
`0x1c0563dcc` then reads `cache[uid]`: a sparse UID at or beyond that capacity goes out of bounds, and
32-bit growth can wrap. Source reserves at least `(uint64_t)uid + 1` entries and grows in 64 bits. Sequential
UID growth below the wrapping boundary is unchanged. The paired dealloc loop at `0x1c0557800-0x1c055780c`
uses a 64-bit index so a repaired capacity of 2^32 does not wrap during release. These are the only intentional
behavioral divergences. No destructive native probe was run. Static analysis is possible before linking;
ASan, allocation/leak checks and behavioral traces require the integrated group and remain pending.

## Open boundaries and integration

No entry from the 147-entry capture is omitted. Six initializer invokes are absent from that capture:
`0x1c04dc3e4`, `0x1c04de0cc`, `0x1c04de1bc`, `0x1c0564228`, `0x1c05642e8`, `0x1c05939a0`.
They stay undefined, with native global block literals (flags `0x50000000`, size 32, signature `v8@?0`,
descriptor `0x2227d03f8`). No fallback state or fabricated log handle is supplied. The private header declares
the state their eventual bodies must initialize.

`ONSUnrestoredInterfaces.h` is the temporary declaration owner for foreign classes/functions/constants,
missing CoreFoundation entry points, and uncaptured NSCoder messages. Its comments list their native names,
addresses and ownership. Missing indirect methods include base allowedClasses, collection enforcement,
allowed-class validation, secure-coding validation, top-level decode, exception descriptions, failure and
categorization. The optional Swift warning selector has no implementation captured here. NSBundle,
_NSPerformanceMeter, _NSKeyedCoderOldStyleArray and _NSKeyedUnarchiverHelper also need their reached methods.
These are open indirect edges, not implementations supplied by the declarations.

At the base revision, `OCFBridgedClasses.h` has no @interface placeholder for the three owned coder classes;
its `@class ONSKeyedArchiver;` can become the owner-header import. Foreign NSString/NSMutableString,
NSValue/NSNumber and NSError placeholders must be replaced by their respective families, not these temporary
interfaces. Move the five temporary CF entry declarations to their owning Collections/PropertyList headers,
import class owners, remove the temporary foreign declarations, and record the combined scope/reading/ledger
before the coordinator's first link. The two exact `NSCocoaErrorDomain` literal conversions in `OCFError.c`
are solely the handoff-authorized naming changes.

The handoff acceptance checks compile both products and naming modes, formatting, declaration duplication,
class symbols, new undefined/duplicate symbols, ownership and a committed tree. It does not link or establish
behavior. No behavior tests, Simulator session, full Debug/Release Make/Xcode matrix, or consumer replacement
are claimed by this subtrack. Those checks, isolated process trace comparisons and iOS Device reading remain
integration work. A definition must not be marked restored in the shared ledger before its dependency group
is independently implemented and tested.

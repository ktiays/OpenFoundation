<!--
  Created by ktiays on 2026/10/29.
  Copyright (c) 2026 ktiays. All rights reserved.
-->

# Consumer-derived restoration scope

The restoration is limited to what the consumers require (AGENTS.md, "Scope";
owner decision, 2026-10-29). This reading describes how the first scope was
derived, what it contains and where it is still an approximation. The
machine-readable result is [Configuration/Scope.json](../../Configuration/Scope.json).

## Identity

| Input | Revision / build | Identity |
| --- | --- | --- |
| OpenCoreAnimation | `97b8a0dafa9b7b61c790d597a455b8afebf5c006`, `TARGET=iphonesimulator MATCH=iphonesimulator`, Release | UUID `7C01F68D-E3E6-3217-8547-77D52900866D` |
| OpenCoreAnimation | same commit, `TARGET=iphoneos MATCH=iphoneos`, Release | UUID `42E9DC17-4D8D-39A1-A4B6-75F21CB176E6` |
| OpenCoreGraphics | `88639e6d633dfb4ac20e3ab7e277c3a1bfe2cd04`, `PLATFORM=iphonesimulator`, `OCG_USE_PREFIX=1` | UUID `E131D0BF-7674-34EC-8B27-F4FC93644B58` |
| CoreFoundation | iOS Simulator 27.0 (24A434), arm64 | `D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC` |
| Foundation | iOS Simulator 27.0 (24A434), arm64 | `49FCDA5F-A93E-3259-857C-FC75AC2C7B03` |

The consumers were built from `git archive` exports under
`Local/Scratch/scope-closure`, leaving their checkouts and build directories
untouched. Image SHA-256 values are checked against
`Configuration/ReferenceImages.json` when the scope is assembled. The iOS Device
images have not been closed yet.

## Method

1. **Roots.** `Tools/scope.py roots` reads each linked consumer binary: the
   symbols bound to CoreFoundation and Foundation (`dyld_info -imports`), and
   every selector its `__objc_selrefs` references. OpenCoreAnimation imports
   185/189 CoreFoundation and 35/36 Foundation symbols (iOS Simulator/iOS Device
   builds) and references about 2,800 selectors; OpenCoreGraphics imports 146
   and 6, including five Swift overlay entry points, and references 11.
2. **Closure.** `Tools/scope-ida.py` runs inside IDA on a scratch clone. Starting
   from the roots it follows direct and tail calls, function addresses taken in
   code and targets reached through `__got`. Exported aliases that IDA names once
   (`_NSRunLoopCommonModes` shares `_kCFRunLoopCommonModes`'s address) resolve
   through the image's symbol table. Data roots are recorded, not expanded.
3. **Messages.** Objective-C messages are not resolved by the call graph.
   A consumer selector becomes a `message-candidate` root on methods of a root
   class, its superclasses, its categories and its subclasses in the image.
   Selectors sent inside the closure then reach methods of the classes the
   closure already relates to (`message-reachable`), repeated to a fixpoint.
4. **Both images.** Each image's closure contributes its imports, classes and
   sent selectors to the other's roots. Three alternating passes reached a
   fixpoint: the last CoreFoundation pass added no Foundation symbol or selector.
5. **Assembly.** `Tools/scope.py assemble` checks image identity, derives a
   name-based family label, computes strongly connected components and
   bottom-up levels over call edges, marks already restored definitions by
   native address and lists cuts per external library.

Reproduce, with the IDA database of each image open in turn:

```sh
python3 Tools/scope.py roots --consumer NAME COMMIT BUILD BINARY ...
python3 Tools/scope.py frontier --image CoreFoundation   # then run Tools/scope-ida.py in IDA
python3 Tools/scope.py frontier --image Foundation       # repeat until no new roots appear
python3 Tools/scope.py assemble
python3 Tools/scope.py assemble --check
```

The check needs the evidence under `Local/Evidence/scope`, which is unversioned;
it is not part of `make check`.

## Result

| Image | Functions | Code bytes | `symbol` | `message-candidate` | `message-reachable` | Already restored |
| --- | --- | --- | --- | --- | --- | --- |
| CoreFoundation | 3,593 | 933,000 | 2,015 | 806 | 772 | 73 |
| Foundation | 3,375 | 575,568 | 26 | 1,298 | 2,051 | 26 |

The scope is about 49% of CoreFoundation's `__text` and about 5% of Foundation's.
The `symbol` tier, about 646 KB in total, is required by evidence. The two
message tiers over-approximate and must be narrowed by readings.

One hundred of the 142 restored function definitions (99 native addresses) fall
inside the scope: the
UTF-8 encoder, Unicode membership/decomposition, the Unicode plane/property/priority
group (the first group below the core), the case-mapping and compatibility-decomposition group, the BigNum initializer/text group with
`emit128`, the encoding-name, NSStringEncoding-mapping and
converter-wrapper group with both default fallbacks, the locale-identifier canonicalization helpers, the four CFBundle file-type
sniffers, the sixteen leaf helpers (format specs, binary-plist arrays and strings,
parser line numbers, paths, bundle paths, URL net location, `chcompar`), the Gregorian
day count, binary-plist top-level validation, system-version mapping, the linked-on-or-after checks, `CFHashBytes` and the raw Decimal group. The range, Gregorian validation and single-byte codec units are
outside it. They are kept and reported separately ("Progress and Verification").

### Dependency structure

- CoreFoundation has 1,214 leaf functions (level 0) and depth 38. Foundation
  has 2,062 leaves and depth 34.
- **One strongly connected group dominates CoreFoundation**: 396 functions,
  273,224 bytes at level 13. It ties CFString (78), CFBundle (50), CFURL (30),
  CFCharacterSet, the number formatter, the collections, property lists and
  runtime description together. Without CFBundle it shrinks to 175 functions.
  The joining edges come from string formatting and locale selection:
  `__CFCopyResolvedFormatStringWithConfiguration` calls `CFBundleGetMainBundle`
  and the bundle localization queries; `__CFStringGetRelevantLocaleInfoFromLocaleSummary`
  calls `CFBundleCopyLocalizationsForPreferences`. Whether those edges are cuts
  (downgraded through the native code's own path, for example no main bundle)
  was the first owner decision the scope needed; see "Decisions".
- A second group of 14 joins CFError and CFBundle. Foundation's call graph has no
  group larger than 3. Its coupling is through messages, which the components do
  not model.

### Largest families

CoreFoundation, by code bytes: CFString (149 functions, 103 KB), CFBundle (148,
46 KB), CFURL (87, 30 KB), CFCharacterSet (70, 29 KB), the CFPrefs sources
(about 110 functions, 35 KB), NSArray/NSSet/NSDictionary/NSOrderedSet and their
mutable classes, CFBasicHash, CFNumberFormatter, CFBinaryPlist, CFRunLoop
(55 + 9), CFPropertyList, CFStorage, CFDateFormatter, CFXNotification, CFArray
and CFRuntime (44). 602 functions have no CF/NS-derived label.

Foundation: Swift support (82), NSString (125), NSKeyedUnarchiver/NSKeyedArchiver,
NSNumber, NSCoder, NSIndexSet, NSNumberFormatter, NSAttributedString,
NSXPCEncoder/NSXPCDecoder, the map and hash tables, NSBundle, NSValue,
NSDecimalNumber and NSDateFormatter.

The full per-family table is in `Scope.json`. Families are name-derived planning
labels, not source-file claims.

### Known over-approximation

- Subclass expansion of root classes brings in Distributed Objects:
  `NSConnection`, `NSDistantObject`, `NSKeyedPortCoder`, `NSUnkeyedPortCoder`
  (NSPortCoder is an NSCoder subclass). The consumers have no evidence of
  using them.
- NSAttributedString and NSString subclasses bring in inflection and morphology
  (`NSMorphology`, `_NSAttributedStringGrammarInflection`), `NSPresentationIntent`,
  data detectors and regular expressions.
- NSPredicate, which OpenCoreAnimation imports, brings in the expression and
  predicate utility classes.

Each entry needs a reading of the consumer's actual receivers before it leaves
the candidate tiers.

### Not followed yet (unresolved indirect edges)

- CF callbacks stored in data: runtime class tables, collection callback
  structures, run-loop source contexts and allocator contexts.
- Blocks invoked through pointers, and C++ virtual calls.
- Swift protocol witnesses and generic metadata accessors.
- Messages to receivers outside the related class families, including
  `CF_OBJC_FUNCDISPATCH`-style bridging dispatch into Foundation classes.
- The iOS Device images.
- OpenCoreAnimation links the system CoreGraphics. Its 531 CoreGraphics imports
  reach CoreFoundation through CoreGraphics, and none of that is in these roots.
  As OpenCoreGraphics covers more of CoreGraphics, its own CF imports grow.
  Regenerate the scope when either consumer moves.

## Cuts

| Image | Library | Symbols | Port-relevant examples |
| --- | --- | --- | --- |
| CoreFoundation | libSystem | 312 | `xpc_*` (63, preferences), `dispatch_*` and the `_4CF` run-loop SPI (`_dispatch_main_queue_callback_4CF`, `_dispatch_runloop_root_queue_*_4CF`, `_dispatch_get_main_queue_port_4CF`), `mach_msg`/`mach_port_*`, `notify_*`, `sandbox_*`, `container_*`, `voucher_*`, `os_*`, `pthread_*` |
| CoreFoundation | libicucore | 107 | date/number formatting, calendars, converters, collation, locales |
| CoreFoundation | libobjc | 65 | runtime, class and selector APIs |
| CoreFoundation | CoreServicesInternal | 2 | file-system URL helpers; **unclassified** |
| Foundation | libSystem | 101 | `pthread_*`, `xpc_*` (12), `os_*`, `dispatch_*`, `mach_*` |
| Foundation | libobjc | 100 | runtime and property/protocol APIs |
| Foundation | libswiftCore | 58 | Swift strings, arrays, hashing |
| Foundation | libicucore | 22 | regular expressions, plural rules |

The libSystem, ICU, libobjc and Swift runtime cuts are **external**: libraries
the native images link, which a port TARGET supplies (AGENTS.md, "Porting").
On a port the critical ones are the run loop's Mach-port and `_4CF` libdispatch
SPI, and the XPC-based preferences daemon. swift-corelibs-libdispatch's
counterparts to the `_4CF` interfaces have not been checked against these
call sites.

## Decisions

### CFBundle is retained (2026-10-29)

Owner criterion: CFBundle stays in the scope if it can be ported without
difficulty; otherwise it is removed. Evaluation against the iOS Simulator
image (`D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC`, IDA, high confidence for
the functions named here):

- The consumers import CFBundle directly. OpenCoreAnimation imports
  `CFBundleGetMainBundle`, `CFBundleGetIdentifier`, `CFBundleGetInfoDictionary`,
  `CFBundleGetValueForInfoDictionaryKey`, `_CFBundleGetMainBundleIfLooksLikeBundle`
  and `NSBundle`; OpenCoreGraphics imports `CFBundleGetIdentifier`. Removing
  CFBundle would cut consumer roots, not only the string-formatting edges.
- The main bundle is found from the executable path and the directory layout:
  `CFBundleGetMainBundle` (0x1c0169a0c) calls `_CFProcessPath`, builds a URL,
  and asks `_CFBundleCopyBundleURLForExecutableURL`. When no bundle URL is
  found, the main bundle is NULL. `__CFCopyResolvedFormatStringWithConfiguration`
  (0x1c02cb238) then gets no localizations and creates the `en` locale itself.
  This is POSIX file-system logic over CFURL and property lists, and it ports.
- The Mach-O-specific readers already have native no-data paths.
  `_CFBundleCreateInfoDictFromMainExecutable` (0x1c025b0a8) passes NULL data
  when `getsegbyname("__TEXT")` fails, and
  `_CFBundleGrokObjCImageInfoFromMainExecutable` (0x1c025b34c) reports zeros
  when `getsegbyname("__DATA")` fails. A port supplies the interface and
  answers that there is no Mach-O segment; it does not edit the code.
- The loader interfaces CFBundle uses are not specific to it. `CFEqual`,
  `CFHash`, `CFGetTypeID`, `CFCopyDescription` and `__CFStringAppendFormatCore`
  call `dyld_program_sdk_at_least`, and `__CFLookUpClass` uses `dlopen`. A port
  supplies those answers either way; removing CFBundle would not remove that work.

Result: the CFBundle edges are **restored**, and the core group stays at 396
functions. On a port TARGET, `_NSGetExecutablePath`, `_NSGetMachExecuteHeader`,
`getsegbyname`, `getsectbynamefromheader_64`, the `_dyld_*` image queries,
`NXGetLocalArchInfo`/`NXFindBestFatArch` and the `dlfcn` family are
**substituted** interfaces in the platform dependency table. Unread: how
`_CFBundleGrokBinaryType` and the fat-file reader treat an ELF executable;
how `_CFBundleDYLDCopyLoadedImagePathsIfChanged` behaves with an empty image
list.

## Next steps

1. Owner decision on the CoreServicesInternal cut.
2. Narrow the message tiers by reading consumer receivers, starting with the
   over-approximation families above.
3. Resolve data-stored callbacks for the runtime class table and collection
   callbacks; they are needed for the runtime group anyway.
4. Close the iOS Device images with the same tooling.
5. Restore bottom up from the leaves, with the RUNTIME track first, then the
   strings group once its cuts are decided.

# Consumer requirements

The [source snapshot](Consumers.json) records CF/NS tokens and direct system
header imports from OpenCoreGraphics (`src`, `include`) and OpenCoreAnimation
(`Source`) at their listed revisions. Reproduce it with:

```sh
python3 Tools/consumer-inventory.py \
    --coregraphics /absolute/path/to/OpenCoreGraphics \
    --coreanimation /absolute/path/to/OpenCoreAnimation
```

The initial snapshot contains 319 distinct tokens in 336 OpenCoreGraphics files
and 326 in 465 OpenCoreAnimation files. Tokens include types, constants, macros,
Objective-C class references and private APIs. They are neither a native export
inventory nor an API compatibility percentage. Lexical scanning cannot recover
Objective-C selectors, dynamic lookups, Swift overlays, transitive dependencies
or the platform-dependent active subset; those require subsequent readings and
actual build/link audits.

OpenCoreGraphics directly includes CF base/geometry scalar headers, allocator,
collection, data, string, number, date, URL, property-list, locale/time-zone and
stream families. Its source also uses private CF runtime/type registration.
Replacing only public string and array APIs would leave its object model and
callback/ownership boundaries unresolved.

OpenCoreAnimation includes CoreFoundation through its X helpers, public C types,
layer/transaction and CAML units, and Foundation across its Objective-C model,
coding, value, parser and renderer-related units. Its local
`Source/ObjC/FoundationDeclarations.h` additionally declares geometry-related
NSValue methods and NSAffineTransform absent from the iOS public declarations.
These are consumer declarations; native method/image ownership is still unread.

Consumer acceptance requires all of the following for an explicitly selected
replacement build:

- Headers, compiler constants and public/private declarations have verified ABI.
- Every referenced operation's whole native dependency group is restored.
- CF/NS object registration, lifetime, constant objects and bridging work without
  system Foundation/CoreFoundation doing the operation.
- Undefined-symbol, runtime image and load-command checks exclude accidental
  native Foundation/CoreFoundation fallback, including transitive dependencies.
- Both consumers' existing tests pass against replacement objects on the pinned
  platform. Finite passing cases are reported with their actual scope.

No consumer source or build configuration is changed by initialization. Its
snapshot is deliberately a recorded revision, not a claim about a future checkout.

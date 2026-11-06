# Runtime prerequisite reading boundary

## Identity and scope

The first reference track captured these entry-point instruction streams through
IDA MCP. This is a dependency-discovery boundary, not a completed restoration of
the allocator, type-registration or ownership group.

- iOS Simulator 27.0 (24A434), arm64, CoreFoundation UUID
  D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC, SHA-256
  e3598ca3b1ed3649c1ff8aadce058e98aa2d7be5385036a5ebb6ab986a5cc829.
- iOS Device 27.0.1 (24A446), arm64e, CoreFoundation UUID
  9426FC9A-B36C-3096-B836-D17D6744A75B, SHA-256
  ba22deb3759f56d32e5847fec0a5e675837e189d9a000b87ffdefe88a7942166.
- Raw evidence: `Local/Evidence/cf-runtime-reference/iphone-simulator-CoreFoundation-reading.json`
  and `iphone-device-CoreFoundation-reading.json`.

| Native function | iOS Simulator address | iOS Device address | Confidence and limit |
| --- | --- | --- | --- |
| CFAllocatorGetTypeID | 0x1c0150450 | 0x18072d124 | Entry body verified; allocator group unbuilt |
| _CFRuntimeRegisterClass | 0x1c01e54dc | 0x18072373c | Entry body captured; registration state and complete failure dependencies unread |
| CFGetTypeID | 0x1c01e5fb0 | 0x18062493c | Entry body captured; complete object-domain dispatch and state unread |
| CFRetain | 0x1c01e5abc | 0x180607a70 | Entry body captured; complete lifetime, callback and thread-state group unread |
| CFRelease | 0x1c01e6984 | 0x180600c70 | Entry body captured; release/deallocation closure unread |

`CFAllocatorGetTypeID` is `MOV W0,#2; RET` in both independently read images.
Confidence: verified for that body only. It is deliberately not shipped as an
isolated constant-returning substitute for the missing allocator group.

The iOS Device `CFRetain` body calls `___CFCheckCFInfoPACSignature` at
0x180607c28, targeting 0x180607980. Confidence: verified call boundary; the target
and its complete dependencies remain unread. Other captured entry streams include
indirect operations and cold paths. The saved direct-code-reference lists are
not a complete call graph and cannot establish dependency closure.

## Next complete group

The runtime track must read allocator records and thread defaults, registration
state, object headers and type flags, Objective-C dispatch/ownership boundaries,
constant objects, tagged pointers, retain/release/autorelease transitions,
callback order and every reachable failure path. The independently pinned
libobjc boundary and iOS Device authentication differences are prerequisites.
No table size, object layout, lock policy, retain count or bridge has been
inferred from another implementation or from symbol names.

The first restored scalar groups have no CF/NS objects, allocator requests,
registration, callbacks or runtime dependency. Their individual readings prove
that narrow closure. They neither establish nor bypass this runtime work for
strings, collections or consumer integration.

## Core-track reading, 2026-10-30

The core track (`codex/cf-core`, [Scope decisions](../Scope.md#decisions)) reads
the runtime together with the 396-function core component. This section records
the first pass from the iOS Simulator clone under `Local/Scratch/cf-core`; raw
disassembly and pseudocode of every function named here are in
`Local/Evidence/cf-core/iphone-simulator-below-cycle.json`. Nothing here is built.

### Group structure

Over call edges, `CFRetain`, `CFRelease`, `CFGetTypeID`, `CFGetRetainCount`,
`CFAutorelease`, `CFHash`, `CFAllocatorGetDefault` and `CFGetAllocator` close to
53 functions (9,676 bytes, 19 of them `.cold` entries) without entering the core
component: the retain/release bodies, the allocator default and deallocation
entry points, the per-thread data table (`_CFGetTSD`, `_CFGetTSDCreateIfNeeded`,
`__CFTSDGetTable`, `__CFTSDFinalize`), `__CF_IS_OBJC`, the tagged-number hash and
value readers (`__CFNumberHash`, `__CFNumberGetValue`), the simple log path
(`_CFLogSimple`, `__CFLogCStringLegacy`, `_populateBanner`, `_logToStderr`,
`also_do_stderr`, `_CFGetProgname`, `_CFProcessPath`, `__CFGetUGIDs`,
`__CFProcessIsRestricted`) and `CFAbsoluteTimeGetCurrent`. Confidence: verified
for the edge set; the bodies are captured, not yet read line by line.
`_CFRuntimeCreateInstance`, `_CFRuntimeSetInstanceTypeID`, `CFEqual`,
`CFCopyDescription` and `CFLog` are core-component members, and
`_CFRuntimeRegisterClass` (component 1729, level 14) sits above it through
`CFLog`.

### Class lookup (verified, iOS Simulator)

`CFHash` (0x1c01e7444) and `_CFRelease` (0x1c01e6444) read the type ID from bits
8-17 of the info word at offset 8 (acquire load). IDs up to 71 index
`__CFRuntimeBuiltinClassTable` (0x2227a40c8); larger IDs index
`__CFRuntimeClassTables[(id - 72) >> 6]` at `(id - 72) & 63`, bounded by the
dynamic count at 0x22e6d3fd8 in `_CFRelease` but not in `CFHash`. Neither checks
the record for NULL before loading a callback slot. `__CF_IS_OBJC` (0x1c01e5410)
compares the object's isa with `__CFRuntimeBuiltinObjCClassTable[id]` (or the
dynamic Objective-C table at `__CFRuntimeClassTables[16 + ...]`), treats the
constant-string class for ID 7 as CF, and otherwise compares `isa &
objc_debug_isa_class_mask` for non-pointer isas.

### Empty builtin slots (owner condition, settled 2026-10-30)

Twenty-four functions read `__CFRuntimeBuiltinClassTable`,
`__CFRuntimeBuiltinObjCClassTable` or `__CFRuntimeClassTables` (complete list from
IDA data references). Those that take the type ID from an object (`CFHash`,
`CFEqual`, `CFCopyDescription`, `_CFRelease`, `_CFRetain`, `CFGetRetainCount`,
`__CF_IS_OBJC` and the others) dereference the record without a check, so they
reach an empty slot only through an object of that type. The functions that take
a caller-supplied type ID (verified, iOS Simulator):

| Function | Address | Empty slot |
| --- | --- | --- |
| `_CFRuntimeGetClassWithTypeID` | 0x1c01e56c0 | returns NULL |
| `_CFGetTypeIDDescription` | 0x1c01e61e4 | returns NULL; type 1 always returns NULL |
| `_CFRuntimeCreateInstance` | 0x1c01e578c | returns NULL before allocating |
| `_CFRuntimeSetInstanceTypeID` | 0x1c01e5cc0 | traps (`BRK #1`) |
| `CFCopyTypeIDDescription` | 0x1c01e6254 | dereferences the empty record (offset 8), as for the native holes 5, 6, 8-15 |

An emptied slot therefore behaves as a native hole, which settles the owner's
condition ([Scope decisions](../Scope.md#decisions)).
`_CFRuntimeSetInstanceTypeID` stores only the low eight bits of the new type ID:
`UBFIZ X9, X1, #8, #8` inserts eight bits at bit 8 while the mask
`0xFFFFFFFFFFFC00FF` clears the whole ten-bit field, inside a `CASAL` loop
(verified, disassembly). An ID of 256 or more loses its high bits; this is native
behavior and is kept. A class with custom refcount on either side logs "Cannot
change the CFTypeID ... due to custom ref counting" through `CFLog` instead. The
lookup of the current class for an ID above 71 is not bounded by the dynamic
count (the count is touched with `LDR WZR` only).

### Release (verified control flow, iOS Simulator)

`_CFRelease` panics through `.cold.1` when the deallocating bit (0x200000) is
already set. A custom-refcount class (info bit 0x800000) requires record flag bit
3 and the slot at offset 80, which is called as `(-1, object)`. Otherwise the
retain count in bits 44-63 is decremented by compare-and-swap; on the last
reference the reclaim callback (offset 72, record flag bit 2) runs, the
deallocating-in-progress bit 0x400000 is set, finalize (offset 32) runs, a
resurrected object has the bit cleared and the loop repeats, and otherwise the
count is dropped with bit 0x200000 set. Allocator objects (ID 2) go to
`__CFAllocatorDeallocate`; others resolve their allocator (info bit 63 selects
the system default, else the pointer stored 16 bytes before the object) and
call `objc_destructInstance` when the isa's class is live, or, with
`__CFZombieEnabled`, retype the object to `_NSZombie_<class>` through
`objc_lookUpClass`/`objc_duplicateClass` under `__CFBigRuntimeFunnel`. Memory is
freed with `CFAllocatorDeallocate` at the object or 16 bytes before it, and a
non-default allocator is released last. `__CFOASafe` adds the allocation-event
cold calls.

Consequence: every CF object is an Objective-C object with a live isa, and the
release path calls libobjc on it. Restored CF types need registered Objective-C
classes (`__NSCFType` and the bridged classes) in the restored domain; this is
part of the group, not a later bridge.

### Tagged pointers (verified use, medium-confidence names)

`CFGetTypeID` (0x1c01e5fb0) and `CFHash` decode tagged pointers inline, without
calling libobjc functions, through three libobjc data imports reached at
0x222409420 (XORed with the pointer: `objc_debug_taggedpointer_obfuscator`),
0x222409408 (a seven-byte tag table: `objc_debug_tag60_permutations`) and
0x222409400 (ANDed with the isa: `objc_debug_isa_class_mask`). The names follow
the image's libobjc imports and each slot's use; the slot-to-import binding is
not yet read from the fixups. Tag 0 answers 7 (string) or 1 depending on
`dyld_program_sdk_at_least(0x7E30901FFFFFFFF)`, tag 2 and extended tag 22 answer
7, tag 3 answers 22 (number), tag 6 answers 42 (date), tags 4, 5 and 20 answer
1, and other tags send `_cfTypeID`. Tagged-pointer classes are registered
through `_objc_registerTaggedPointerClass`. These are external libobjc cuts; how
the restored domain coexists with the system's tagged classes in one process is
an open decision.

### Instance creation and registration (verified, iOS Simulator)

`_CFRuntimeCreateInstance(allocator, typeID, extraBytes, category)` (0x1c01e578c):

1. Looks the class up: IDs above 71 under `__CFBigRuntimeFunnel` (an ID at or
   beyond the dynamic count traps); an empty record returns NULL.
2. A class with flag bit 3 (custom refcount) and no slot at offset 80 logs
   "found inconsistent class" through `CFLog` and returns NULL.
3. Flag bit 4 (required alignment) forces the system default allocator; a NULL
   allocator becomes the per-thread default (TSD slot 1) or the system default;
   `kCFAllocatorNull` returns NULL.
4. The size is `(extraBytes + 31 + (16 when the allocator is not the default)) &
   ~15`, with malloc type ID `(403 * (typeID ^ 0xC9DC5)) & 0xFFFFF | 0xCF000000`.
   Aligned classes use `malloc_type_zone_memalign` on the default zone with the
   class's alignment (offset 88); otherwise `CFAllocatorAllocateTyped`: an
   allocator that honors the zero hint (system default or malloc zone,
   `__CFAllocatorRespectsHintZeroWhenAllocating`) is asked with hint 1 for zeroed
   memory (`MOV W3, #1` at 0x1c01e5970), any other with hint 0 followed by `bzero`.
   Aligned allocations are not cleared.
5. The allocation-event name is recorded when `__CFOASafe` is set (the category
   argument, else the class name).
6. A non-default allocator is retained and stored in the 16 bytes before the
   object. The info word is `typeID << 8` (24 bits taken), bit 7 when the
   allocator is the default, and either a retain count of one (`1 << 44`) or, for
   custom refcount, `0xFFFFFFFFFF800000`; it is stored atomically, the isa is
   zeroed and then set with `object_setClass` to the bridged Objective-C class,
   and the class's `init` (offset 16) runs last.

`_CFRuntimeRegisterClass` (0x1c01e54dc) rejects an inconsistent custom-refcount
class through `CFLog`, then under the funnel fails with a `CFLog` message when
1,024 classes are registered, allocates each 64-entry page of class records and
Objective-C classes with `malloc_type_calloc` on first use, stores the record and
the default bridged class (0x22e6d3fe8), increments the count and returns
`count + 72`. `_CFRuntimeBridgeClasses` (0x1c01e5638) stores
`objc_getFutureClass(name)` under the funnel.

### Static instances and type-ID changes (verified, iOS Simulator)

`_CFRuntimeInitStaticInstance(object, typeID)` (0x1c01e5b98) traps for an empty or
out-of-range slot, refuses a custom-refcount class through `CFLog`, stores the
info word `typeID << 8 | 0x80` (allocator bit set, retain field zero), zeroes and
sets the isa to the bridged class and calls the class's `init`. A zero retain
field is what makes static instances immortal: `_CFRelease` returns at once for
it, and `CFGetRetainCount` reports it as 2^60-1, the value the runtime traces
record for constant strings and `kCFBooleanTrue`.
`_CFRuntimeSetInstanceTypeIDAndIsa` (0x1c01e5dd0) sets the type ID and then
replaces the isa only when it differs from the bridged class of the new ID.

### Allocators and per-thread data (verified, iOS Simulator)

`CFAllocatorCreate` (0x1c01504fc) accepts only NULL, `kCFAllocatorUseContext`
(the literal 939) or a CFAllocator (isa equal to the allocator class at
0x22e6d3ff0). It retains the context info first, then allocates a 224-byte
object: from its own `allocate` callback for `kCFAllocatorUseContext`, otherwise
from the given or default allocator, which it retains. The first 128 bytes are a
working `malloc_zone_t` (size, malloc, calloc, valloc, free, realloc and destroy
functions, the name "Custom CFAllocator", introspection, version 6); bytes
128-223 hold the owning allocator, the context version, info and the eight
context callbacks. `_CFAllocatorAllocateImpl` (0x1c0150924) treats any argument
whose isa is not the allocator class as a plain malloc zone, prefers the typed
allocate slot, else calls `allocate(size, hint, info)`, and returns NULL for a
zero size. `CFAllocatorSetDefault` ignores a non-allocator or the current
default, and otherwise releases the old default and retains the new one twice
before storing it in TSD slot 1.

Per-thread data lives under the reserved pthread key 55: a 1,128-byte calloc table
created with `pthread_key_init_np(55, __CFTSDFinalize)` and
`_pthread_setspecific_static` (`__CFTSDGetTable`, 0x1c01ce9f4); the value 4096
marks torn-down data, and slot indices must be below 70. Both are libsystem
interfaces, and `pthread_key_init_np` and `_pthread_setspecific_static` are
private; on a port they are substituted interfaces.

### Load-time initialization (verified call order, iOS Simulator)

`__CFInitialize` (0x1c01e7bd4), the image's only initializer, runs once
(`__CFInitialized`/`__CFInitializing`) with `__CFProphylacticAutofsAccess` set,
in this order:

1. Fills `__CFEnv` (11 name/value slots at 0x22e6cf770) with `getenv` for `USER`,
   `TZFILE`, `TZ`, `NEXT_ROOT`, `DYLD_IMAGE_SUFFIX`,
   `CFBundleDisableStringsSharing`, `CF_CHARSET_PATH`, `__CF_USER_TEXT_ENCODING`,
   `APPLE_FRAMEWORKS_ROOT` and `IPHONE_SIMULATOR_ROOT`; the last slot is empty.
2. `__exceptionInit`: `objc_setExceptionPreprocessor`, `objc_setExceptionMatcher`
   and `objc_setUncaughtExceptionHandler` with CF's handlers.
3. `objc_setForwardHandler(_CF_forwarding_prep_0, NULL)` and
   `objc_setEnumerationMutationHandler(__NSFastEnumerationMutationHandler)`.
4. `_CFStringGetUserDefaultEncoding`: parses `__CF_USER_TEXT_ENCODING` as
   `<euid>:<encoding>:<region>`; when it is absent or names another user and the
   effective user ID is positive, it sets the variable to `0x<euid>:0:0`. Encodings
   4 and 5, and Unicode-group encodings whose rotated index is 0 or 3-7, become 0.
5. `pthread_atfork(__cf_atfork_prepare, NULL, __cf_atfork_child)`, and
   `instrumentObjcMessageSends(1)` when `NSObjCMessageLoggingEnabled` starts with
   `y` or `Y`.
6. Bridges `__NSCFType` into slot 0 and copies it into all 72 Objective-C slots,
   then bridges, in order, 7 `__NSCFString`, 16 `NSNull`, 17 `__NSCFSet`,
   18 `__NSCFDictionary`, 19 `__NSCFArray`, 20 `__NSCFData`, 21 `__NSCFBoolean`
   and 22 `__NSCFNumber`.
7. `CFNumberGetTypeID` (a `dispatch_once` before returning 22) and
   `__CFCharacterSetInitialize` (a `dispatch_once`), then bridges by type ID
   `NSMachPort`, `NSURL`, `__NSCFTimer`, `__NSCFAttributedString`, `__NSCFLocale`,
   `__NSCFError`, `__NSCFCharacterSet`, `__NSCFInputStream` and
   `__NSCFOutputStream` (each `GetTypeID` returns its builtin constant).
8. `_CFProcessPath`, then stores `_CFAutoreleasePoolPush` and
   `_CFAutoreleasePoolPop` into two external slots (0x2224170d0, 0x222417128;
   their owner is **unread**).
9. `__CFOAInitialize`: only with `OAKeepAllocationStatistics` set, resolves
   `_OAInitialize`, `_OARecordAllocationEvent` and `_OASetLastAllocationEventName`
   with `dlsym(RTLD_DEFAULT)` and sets `__CFOASafe`.
10. `__CFStringInitializeSystemEncoding`: sets
    `__CFIsReadyToInitializeSystemEncoding` and calls `CFStringGetSystemEncoding`.
11. `__CFMakeNSBlockClasses`: when `__NSStackBlock__` exists, re-parents the four
    block classes under `NSBlock`; otherwise turns libsystem_blocks' concrete
    block storage into Objective-C classes with `objc_initializeClassPair`
    (`__NSStackBlock__`, `__NSMallocBlock__`, `__NSAutoBlock__`,
    `__NSFinalizingBlock__`, `__NSGlobalBlock__`, `__NSBlockVariable__`) and
    registers them; then `_Block_use_RR2` with CF's retain/release callbacks.
12. Zombies: with `NSZombieEnabled` starting with `y`/`Y`, sets
    `__CFZombieEnabled` and `__CFZombifyNSObject` exchanges two NSObject instance
    methods (their selectors lie outside the extracted image and are **unread**);
    `NSDeallocateZombies` sets `__CFDeallocateZombies`.
13. Clears `__CFProphylacticAutofsAccess`, installs `os_log_callback` with
    `_os_log_set_nscf_formatter`, and, when `dyld_has_inserted_or_interposing_libraries`
    finds `libclang_rt.tsan`, resolves the four `__tsan_external_*` functions and
    registers five mutable-collection tags, clearing all of them if any is missing.
14. Sets `__CFInitialized`.

CoreFoundation's `+[NSObject(NSObject) load]` (0x1c029d5d8) runs a one-time step
guarded by an external libobjc value (0x222409418, **unread**) and installs
`_objc_setBadAllocHandler(couldNotInstantiate)`.

Every hook in steps 2, 3, 11 and 13 and the bad-alloc handler is process-global
in libobjc, libsystem_blocks or libsystem_trace. A process can have only one
owner of them, which is the other reason object groups are compared across
processes. The replacement build installs them itself; a port supplies the same
interfaces.

### iOS Device comparison (unread in detail)

The same 35 functions were captured from the iOS Device image (`9426FC9A-...`)
into `Local/Evidence/cf-core/iphone-device-runtime-entries.json` and compared by
constants and call targets only. The layout constants agree (key 55, the 0x468
table, 70 slots, the malloc type IDs). The Device build differs beyond
instruction selection: almost every function carries constants consistent with
pointer-authentication discriminators (0x6AE1, libobjc's isa discriminator, and
0xC471-0xC473 and 0xCB84-0xCB87), `CFRetain` and `_CFRetain` call
`___CFCheckCFInfoPACSignature`, so the Device info word carries a signature, and
`__CFTSDGetTable` is inlined into its callers. This is an ABI difference of the
object header on the Device. Every Device body of this group is **unread** beyond
that comparison and needs its own reading before any Device claim.

### Next reads

1. The callees of the load-time steps: `_CF_forwarding_prep_0`,
   `____forwarding___` and NSInvocation, the exception handlers, `_CFProcessPath`,
   `CFStringGetSystemEncoding`, the block retain/release callbacks and
   `os_log_callback`.
2. The bridged Objective-C classes (`__NSCFType`, `__NSCFString`, `__NSCFArray`
   and the others). Their names begin with `NS` after the leading underscores, so
   the existing rule spells them `__ONSCFType`, `__ONSCFArray` and so on in
   prefixed mode, and runtime lookups use `OF_NAME_STRING`; the owner confirmed
   this spelling on 2026-10-30.
3. The core component, family by family.
4. The iOS Device bodies of this group, including the info-word signature.

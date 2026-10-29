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

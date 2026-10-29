# Restoration sequence

This is the initial dependency order, inferred from the consumers' source
requirements. It is not an assertion about unread native call graphs. Native
readings may split, reorder or combine groups, especially CF/NS bridging cycles.
Each stage needs a complete read dependency group, standalone verification,
differential tests, both naming modes and both build systems before acceptance.

| Stage | Scope | Acceptance |
| --- | --- | --- |
| Bootstrap | Product separation, English engineering rules, shared formatting, build/mapping checks, identity pins, consumer inventory | Empty frameworks build and load; zero APIs claimed; production never loads system Foundation/CoreFoundation |
| Reference inventory | Simulator and iOS Device images, exports/reexports, undefined imports, Objective-C/Swift ownership, native class clusters and ABI boundaries | Hashes/UUIDs and owning images recorded; IDA databases ready; dependency graph read |
| Independent scalar prerequisites | Native-read base utilities, ranges, date field validation and complete raw numerical/encoding groups with no object dependency | Exact raw ABI, branches, tables and errors; standalone/differential coverage; object runtime remains a separate prerequisite |
| Runtime foundations | Allocator/type registration, retain/release, equality/hash/description dispatch, compiler/runtime boundaries, constant objects and CF/NS bridging | Correct native ABI and lifetime behavior; no duplicate runtime-owned classes; independent object domains in coexistence mode |
| Scalar and storage families | String/encoding, data, number/boolean/date, array/dictionary/set, callback tables, class-cluster primitives | Native layouts and callbacks restored; consumer-critical public/private entry points verified |
| Runtime services | Autorelease/thread state, run loops/sources/timers, notifications, character sets/locales/time zones, URLs, errors | Timing, ordering, failure and ownership semantics established against the pinned system |
| Foundation consumers | NS value/data/string/collection behavior, locks/threads, exceptions, coding and keyed archives, XML/parser requirements | CF-backed and Objective-C behavior agree with the reference; geometry/category ownership established independently |
| Consumer integration | OpenCoreGraphics first, OpenCoreAnimation by complete dependency group | Consumers explicitly select replacement headers/libraries and pass their own tests; dependency audits exclude system Foundation/CoreFoundation |
| iOS Device stage | Simulator/iOS Device code and ABI differences, tagged pointers/authentication, device-only services | iOS Device builds and on-iPhone behavior verified; simulator results never relabeled as iOS Device results |
| Remaining framework surface | Additional public/private APIs, language-runtime surfaces and dependencies | Exact scope tracked; declaration coverage and finite test coverage remain distinct from full equivalence |

The first implementation work is reference inventory and runtime ownership.
Do not start with convenient collection wrappers backed by native CF/NS objects.
OpenCoreGraphics uses CF runtime internals and callback-defined objects;
OpenCoreAnimation also needs Objective-C collections, values, coding and
Foundation geometry declarations. Those boundaries must work independently.

The iOS Device output and foreign TARGET/MATCH pairs remain gated. No macOS
behavior is used as an unrecorded fallback. Swift overlays and ABI surfaces are
inventoried separately from Objective-C APIs, not assumed implemented by headers.

# Work tracks

Implementation tracks use isolated `codex/<track>` branches and build directories.
The coordinating session owns shared build policy, public-header registration,
restoration records, test entry points and squash integration into linear main.

| Track | Scope and owned paths | Depends on | State |
| --- | --- | --- | --- |
| INIT | Root engineering files, Configuration, Tools, Xcode project, Resources, bootstrap Source/Tests | None | Complete; empty bootstrap is not behavior coverage |
| REF | Four canonical images/databases, identity-guarded IDA symbol inventories, `Documentation/Reference` | INIT | Image inventory complete; full class-cluster/ABI/reexport graph still in progress |
| CF-BASE | `codex/cf-base`; `Source/CoreFoundation/Base`, mirror tests, base reading | REF scalar closure | Complete: targeted and full combined Make/Xcode Debug/Release naming-mode matrix passed; included in the initial scalar batch |
| NS-RANGE | `codex/ns-range`; `Source/Foundation/Range`, mirror tests, range reading | REF scalar closure | Complete: targeted and full combined Make/Xcode Debug/Release naming-mode matrix passed; included in the initial scalar batch |
| CF-SCALAR | `codex/cf-scalar`; `Source/CoreFoundation/Date/OCFDate`, mirror tests, scalar reading | CF-BASE types | Complete: targeted and full combined Make/Xcode Debug/Release naming-mode matrix passed; included in the initial scalar batch |
| RUNTIME | `codex/cf-runtime`; allocator/type/ownership/initialization group under `Source/CoreFoundation/Runtime` and `Allocator`, mirror tests/readings | REF and verified CFString/number/date callbacks | Evidence preserved in Local/Evidence/cf-runtime/Handoff.md; implementation deferred while raw groups land; no placeholder slots admitted |
| NS-DECIMAL | `codex/ns-decimal`; complete raw decimal arithmetic under `Source/Foundation/Decimal`, mirrors/reading | REF; any native raw helper closure | In progress; eleven-entry interface inventory and complete arithmetic closure reading |
| CF-ENCODING | `codex/cf-encoding`; complete raw converter/registry/backend groups under `Source/CoreFoundation/StringEncoding`, mirrors/reading | REF, Unicode and verified allocator/string boundaries | Complete kernel batch: 14 functions and 9 tables, 7,911,208 differential cases per prefixed run; full Make/Xcode matrix passed. Public dispatch/cache and additional backends remain unbuilt |
| CF-UNICODE | `codex/cf-unicode`; complete normalization/property/table group under `Source/CoreFoundation/Unicode`, mirrors/reading | REF; exact native table/resource boundary | In progress; owns CFUniChar closure required by encoding and string objects |
| CF-STRING | `codex/cf-string`; complete string objects/core/formatting under `Source/CoreFoundation/String`, mirrors/reading | Runtime ABI, encoding and Unicode | Evidence preserved; implementation deferred with its exact class/bridge dependencies |
| CF-NUMBER-DATE | Independent worktree; complete number/boolean/date object callback group in coordinated new source paths, mirrors/readings | Runtime ABI and exact native dependencies | Evidence preserved; implementation deferred; separate from completed date-field scalar unit |
| CG-DEPS | Complete dependency groups required by OpenCoreGraphics | Runtime/storage groups | Not started |
| CA-DEPS | Complete Foundation/CoreFoundation groups required by OpenCoreAnimation | Runtime/storage groups | Not started |
| INTEGRATION | Explicit replacement builds and dependency audits in the consumers | CG-DEPS, CA-DEPS | Not started |
| IOS-DEVICE | Device execution and admitted replacement build | Validated complete simulator groups and device ABI | Deferred; scalar image comparison is not device equivalence |

`Configuration/Restoration.json` records 29 definitions: nineteen functions and ten
private tables. Active subsystem readings and implementation work never increase
that count until their complete dependency group has independently passed tests.

# Work tracks

The initial bootstrap is the sole exception to implementation-track worktrees:
it initializes the unborn main branch. Subsequent tracks use isolated
`codex/<track>` branches and build directories. Shared build/header changes are
coordinated here before independent implementation work begins.

| Track | Scope and owned paths | Depends on | State |
| --- | --- | --- | --- |
| INIT | Root engineering files, Configuration, Tools, Xcode project, Resources, bootstrap Source/Tests, documentation | None | Complete: Make/Xcode, Debug/Release, both naming modes, isolated execution, identity guards, packaging and coexistence checks passed |
| REF | Extract simulator images; inventory both images' exports, imports, class ownership and language boundaries; establish IDA databases under Local | INIT | Not started |
| RUNTIME | Read the allocator/type/ownership and CF/NS bridging dependency group; declare its ABI and verification corpus | REF | Not started |
| CG-DEPS | Restore the complete dependency groups required by OpenCoreGraphics; exact source scope follows RUNTIME readings | RUNTIME | Not started |
| CA-DEPS | Restore the complete Foundation/CoreFoundation groups required by OpenCoreAnimation; exact source scope follows RUNTIME readings | RUNTIME | Not started |
| INTEGRATION | Explicit replacement builds and dependency audits in the consumers | CG-DEPS, CA-DEPS | Not started |
| IOS-DEVICE | Restore and validate iOS Device divergences | Validated simulator dependency groups | Deferred |

`Configuration/Restoration.json` is the definition ledger, not a second work
status document. It currently contains zero restored definitions. New tracks
state their exact files and dependency interfaces before implementation.

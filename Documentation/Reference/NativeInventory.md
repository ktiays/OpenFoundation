# Pinned native symbol inventory

The [inventory](NativeInventory.json) summarizes IDA MCP exports, imports and
Objective-C symbol entries for four independently identified images. All four
canonical Mach-O files and closed, analyzed databases now exist under
`Local/Databases/{iphone-simulator,iphone-device}`. SHA-256, architecture, OS
build and image UUID are checked against
[ReferenceImages.json](../../Configuration/ReferenceImages.json).

| Image | iOS Simulator 27.0 / 24A434 / arm64 | iOS Device 27.0.1 / 24A446 / arm64e |
| --- | --- | --- |
| CoreFoundation | UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; 2,554 exports, 917 imports | UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; 2,481 exports, 885 imports |
| Foundation | UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; 12,986 exports, 4,432 imports | UUID C041F1D0-5EF1-30A3-BF96-830976173B56; 12,972 exports, 4,419 imports |

These are IDA entry counts, not public API counts, restored behavior or a
compatibility percentage. The raw JSON inventories and function readings are
retained under `Local/Evidence/cf-runtime-reference` and `Local/Evidence/ns-range`.
Regenerate the concise inventory with:

```sh
python3 Tools/native-inventory.py
python3 Tools/native-inventory.py --check
```

The selected class-symbol boundaries in the JSON are interface evidence only.
In both image sets CoreFoundation exports the `NSDate` and `NSDateComponents`
class symbols, Foundation exports `NSValue`, and both images import `NSObject`
from `/usr/lib/libobjc.A.dylib`. An NS spelling does not assign an implementation
to Foundation. No method body, object layout, class-cluster relationship,
constant-object representation or object interoperability follows from these
symbol records. The owning libobjc implementation still needs its own pinned
reading before the object-runtime track can close.

The complete transitive reexport graph, non-exported class ownership, Swift
surfaces and class clusters remain unread. Raw Objective-C symbol entries may
include references as well as definitions. This inventory is not a completed
runtime ABI specification.

## Database ownership

Canonical databases are never opened directly. Each investigation opens an
APFS clone under its own `Local/Scratch` directory, checks the recorded input
path and SHA-256, and closes its session before saving a canonical database or
passing the reading slot to another investigation. The current MCP endpoint
returns `session_id`, not `database_id`, and exposes one open database at a
time. Requests carry that returned session identifier; scripts additionally
guard the exact scratch input. Database reading is serialized while implementation
and test work proceed in separate worktrees. No session is forced or unlocked.

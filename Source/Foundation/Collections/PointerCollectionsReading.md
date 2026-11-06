<!--
  Created by ktiays on 2026/11/04.
  Copyright (c) 2026 ktiays. All rights reserved.
-->

# Pointer Collections Reading

iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. iOS Device 24A446:
unread. Confidence: verified (assembly read in full) unless stated.

## Units

| Unit | Native range | Restored in |
| --- | --- | --- |
| NSHashTable, NSConcreteHashTable, `_hashProbe` | 0x1c051ded0-0x1c05201c7; cold 0x1c0fdca18-0x1c0fdcb87 | `Collections/ONSHashTable.m` |
| NSMapTable, NSConcreteMapTable, `_probeGC`, `_NSConcreteMapTable_reclaim`, `_dealloc` | 0x1c0567a5c-0x1c056a957; cold 0x1c0fddc84-0x1c0fdddf3 | `Collections/ONSMapTable.m` |
| NSPointerArray, NSConcretePointerArray, NSConcretePointerFunctions, slice and barrier functions | 0x1c0595674-0x1c0598463; cold 0x1c0fde5f0-0x1c0fde6eb | `Collections/ONSPointerFunctions.m` |

Only the methods reached by the captured groups are restored; the other methods of these classes are outside
the scope and unread.

## Slice

Every collection stores its items through an `NSSlice` (names from the ivar type encoding): the item store
and three property tables. The acquisition table (24 bytes) carries the memory flags and the
acquire/relinquish functions, the personality table (40 bytes) the size, hash, isEqual and description
functions, and the internal table (64 bytes) the store's allocate, free, read, clear and store functions. The
tables are constants selected by `+[NSConcretePointerFunctions initializeSlice:withOptions:]` from the
options' low byte (memory) and second byte (personality); the internal table by
`+initializeBackingStore:sentinel:weakAutoreleasing:dynamic:` (ARC weak storage, optionally with the
never-used sentinel 1 and autoreleasing reads, or the plain store). Member names of the three tables are
local; their offsets and uses are verified.

## Behavior notes

- Hash and map tables probe linearly over a power-of-two capacity from the 64-bit Jenkins mix of the item's
  hash and double when the count reaches half the capacity. Removal clears the slot and reinserts its chain.
- Weak entries that read nil are purged during mutating probes; the map table also reclaims dangling pairs
  when it grows, inside one autorelease pool per slot.
- Archiving accepts only strong or weak memory with the object or object-pointer personality
  (`__NSPointerFunctionCoding_isValidOptions`).
- `_NSConcreteMapTable_reclaim` and `-[NSConcreteMapTable grow]` relinquish a dangling value with the keys'
  size function (native).
- The class_ro_t flags 0x10 of the concrete classes (weak ivars without ARC) are not reproduced.

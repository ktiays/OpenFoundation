# Foundation and CoreFoundation reference

The compatibility target is the pinned iOS implementation, not an API-compatible
redesign. Five scalar functions and one private table have complete per-group readings and
tests. Initial object-runtime entry readings identify further dependencies; no
complete object runtime, collection algorithm, constant-string representation,
tagged-pointer encoding or bridge is restored.

| Area | Evidence | Status |
| --- | --- | --- |
| Four-image native symbol inventory | [IDA inventories](NativeInventory.md) | Symbol boundaries established; class clusters/layouts remain unread |
| OS/image identity | [Identity manifest](../../Configuration/ReferenceImages.json), LC_UUID and extracted-image hashes | Metadata only |
| Consumer source requirements | [Consumer snapshot](Consumers.json) and [interpretation](ConsumerRequirements.md) | Lexical inventory; no native ownership claim |
| Scalar base/date/range groups | [Base](Research/CoreFoundationBase.md), [date validation](Research/CoreFoundationScalar.md), [Foundation ranges](Research/FoundationRange.md) | Five restored functions and one private table; finite test coverage |
| Allocator and CF runtime | [Initial dependency boundary](Research/RuntimePrerequisites.md) | Full runtime group in progress; unbuilt |
| NSObject/NS classes, runtime ownership and CF bridge | No implementation reading | Unread |
| Strings/data, numbers and collections | No implementation reading | Unread |
| Threads, autorelease, run loops and notifications | No implementation reading | Unread |
| URLs, property lists, keyed coding and parser requirements | No implementation reading | Unread |
| Swift overlays/runtime surfaces | No implementation reading | Unread |
| iOS Simulator/iOS Device differences | Independent body/table comparison in each scalar reading | Five functions compared; no iOS Device build or execution claimed |

The two Bootstrap.c files are infrastructure with no native equivalent and no
dynamic exports. They are not Foundation implementations. Keep this reference
and the per-subsystem readings current as definitions are restored. Use the
[reading template](Research/ReadingTemplate.md); no unchecked assertion in a
consumer or an earlier project becomes a specification here.

# Foundation and CoreFoundation reference

The compatibility target is the pinned iOS implementation, not an API-compatible
redesign. The implementation itself is unread at initialization. No class layout,
type ID, allocator record, collection algorithm, constant-string representation,
tagged-pointer encoding or bridging rule has been established by this project.

| Area | Evidence | Status |
| --- | --- | --- |
| OS/image identity | [Identity manifest](../../Configuration/ReferenceImages.json), LC_UUID and extracted-image hashes | Metadata only |
| Consumer source requirements | [Consumer snapshot](Consumers.json) and [interpretation](ConsumerRequirements.md) | Lexical inventory; no native ownership claim |
| Allocator and CF runtime | No implementation reading | Unread |
| NSObject/NS classes, runtime ownership and CF bridge | No implementation reading | Unread |
| Strings/data, numbers and collections | No implementation reading | Unread |
| Threads, autorelease, run loops and notifications | No implementation reading | Unread |
| URLs, property lists, keyed coding and parser requirements | No implementation reading | Unread |
| Swift overlays/runtime surfaces | No implementation reading | Unread |
| iOS Simulator/iOS Device differences | Independent image identities only | Implementation comparison unread |

The two Bootstrap.c files are infrastructure with no native equivalent and no
dynamic exports. They are not Foundation implementations. Keep this reference
and the per-subsystem readings current as definitions are restored. Use the
[reading template](Research/ReadingTemplate.md); no unchecked assertion in a
consumer or an earlier project becomes a specification here.

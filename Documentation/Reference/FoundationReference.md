# Foundation and CoreFoundation reference

The compatibility target is the pinned iOS implementation, not an API-compatible
redesign, limited to the consumer-derived [scope](Scope.md). One hundred eleven function definitions, one hundred twenty-six data definitions and two Swift support types have complete per-group readings and
tests. Initial object-runtime entry readings identify further dependencies; no
complete object runtime, collection algorithm, constant-string representation,
tagged-pointer encoding or bridge is restored.

| Area | Evidence | Status |
| --- | --- | --- |
| Four-image native symbol inventory | [IDA inventories](NativeInventory.md) | Symbol boundaries established; class clusters/layouts remain unread |
| OS/image identity | [Identity manifest](../../Configuration/ReferenceImages.json), LC_UUID and extracted-image hashes | Metadata only |
| Restoration scope | [Scope reading](Scope.md), `Configuration/Scope.json` | iOS Simulator closure assembled; message tiers and indirect edges open; iOS Device unclosed |
| Consumer source requirements | [Consumer snapshot](Consumers.json) and [interpretation](ConsumerRequirements.md) | Lexical inventory; no native ownership claim |
| Scalar base/date/range groups | [Base](Research/CoreFoundationBase.md), [date validation](Research/CoreFoundationScalar.md), [Foundation ranges](Research/FoundationRange.md) | Nineteen scalar/encoding functions and ten private tables; finite test coverage |
| Raw NSDecimal arithmetic | [Decimal reading](Research/FoundationDecimal.md) | Eleven C APIs, native Swift helpers/error paths and ten C bridges; object Decimal/String APIs unbuilt |
| Raw Unicode primitives | [Unicode reading](Research/CoreFoundationUnicode.md) | Fourteen functions and 108 data definitions, including the plane/property/priority and case-mapping/compatibility groups; stream decomposition unbuilt; 23 Device pointer bindings unread |
| BigNum initialization and text | [BigNum reading](Research/CoreFoundationBigNum.md) | Seven functions (signed initializers, C-string formatting, `emit128`) and two power tables; unsigned initializers, arithmetic and parsing unbuilt |
| String-encoding kernels | [Encoding reading](Research/CoreFoundationStringEncoding.md) | Fourteen hidden kernels and nine tables; seventeen wrapper/name/code-page functions and five tables; dispatcher/cache, concrete converters and the byte-conversion group with its unread character-table writer remain unbuilt |
| Allocator and CF runtime | [Initial dependency boundary](Research/RuntimePrerequisites.md) | Full runtime group in progress; unbuilt |
| NSObject/NS classes, runtime ownership and CF bridge | No implementation reading | Unread |
| Strings/data, numbers and collections | No implementation reading | Unread |
| Threads, autorelease, run loops and notifications | No implementation reading | Unread |
| URLs, property lists, keyed coding and parser requirements | No implementation reading | Unread |
| Swift overlays/runtime surfaces | Raw Decimal C/Swift boundaries and compiler support are read | Seven Device error-witness bindings and remaining Swift surface unread |
| iOS Simulator/iOS Device differences | Independent body/table comparison in each scalar reading | All shipped functions compared at their recorded scope; no iOS Device build or execution claimed |

The two Bootstrap.c files are infrastructure with no native equivalent and no
dynamic exports. They are not Foundation implementations. Keep this reference
and the per-subsystem readings current as definitions are restored. Use the
[reading template](Research/ReadingTemplate.md); no unchecked assertion in a
consumer or an earlier project becomes a specification here.

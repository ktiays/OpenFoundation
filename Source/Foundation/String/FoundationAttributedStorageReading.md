<!--
  Created by ktiays on 2026/11/03.
  Copyright (c) 2026 ktiays. All rights reserved.
-->

# Foundation attributed storage

The implementation evidence is the IDA capture `Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json`,
family `FA-attributed-storage`, from iOS Simulator 27.0 (24A434), arm64. The Foundation image UUID is
`49FCDA5F-A93E-3259-857C-FC75AC2C7B03` and its SHA-256 is
`bc7e0a5f693ffc75ae7c3311b817716e86d149fda2f8c45669a976727322cb18`.
Mach-O class metadata and symbol attributes establish layout and linkage. Confidence is verified for the
captured instructions and class ivars. iOS Device 24A446 remains unread. Behavior and ownership traces
remain pending the first integrated link. The units are grouped by adjacent native addresses and class cluster.

## Data and dependency boundaries

- The two presentation-intent attributes use the native immutable key bytes in either naming mode.
  The class-set fields and dispatch predicate at `0x22e76c0e0`, `0x22e76c0e8`, and `0x22e76c0f0` are zero-initialized.
  Their initializer belongs to FX-swift-and-functions at `0x1c04c7b64`.
- `_NSTaggedIndexSetAllowed` is the native zero-initialized byte at `0x22e6fcb5a`. Its writer and registration
  remain unread. The placeholder's `retain` and `release` at `0x1c052799c` and `0x1c05279a0`, and the
  constant class's `retain`, `release`, and `autorelease` at `0x1c0527dd0` through `0x1c0527dd8`,
  are outside the capture. Index-set traces need those ownership overrides before exercising these objects.
- The longest-effective-range method invokes `-[NSRLEArray objectAtRunIndex:length:]` at `0x1c04c6a50`
  through an IMP. Its body is outside the capture. Affected behavior remains open.
- The descriptor table's 11 constant instances reference eight unread descriptor subclasses. The table and
  methods that use it retain those class dependencies. The class names and their static instance names
  come from the captured table at `0x2227d5bb8`. Base `+initialize` and `-initWithName:` are outside the capture.
- The public CoreFoundation wrappers `_CFCharacterSetIsMutable` at `0x1c017ad5c` and `_CFBytesInASCII`
  at `0x1c0208960` remain unread and undefined. `_CFCharacterSetIsMutable` and the private CF helper
  at `0x1c01732b8` are distinct symbols.
- FO-notifications-locks-json-coders owns the JSON serialization class, FG-grammar-inflection-rules owns
  its parser method, C2-strings-charsets-plist owns the tokenizer functions, and FD-decimal-number-formatting
  owns the constant integer class. The Markdown parser continues to use the existing libcmark-gfm cut.
- The metadata supplies every declared ivar's size and offset. The captured simple dictionary methods
  do not access its `refCount` field. The concrete mutable string methods only read or write the
  `mayNeedIntentResolution` byte in the `fields` ivar. `_NSStringProxyForContext` stores the formatter and
  item as object pointers with the native retain and copy entry points. Their class/protocol encodings
  are recorded in class metadata but the additional protocol methods are outside this capture.

## Bindings with medium confidence

The shared-cache slots for NSObject (`0x22e6b6088`), NSDictionary (`0x22e6cde50` and `0x22227fad0`),
NSEnumerator (`0x22e6cdec8`), NSArray (`0x22227fa90`), NSMutableArray (`0x22227fb18`),
NSMutableDictionary (`0x22227fb28`), NSSet (`0x22227fb68`), NSException (`0x22227fae0`),
NSInvalidArgumentException (`0x22227f500`), NSRangeException (`0x22227f5e0`), and kCFAllocatorDefault
(`0x222280b50`) are identified by their use and neighboring metadata. The indirect selectors
`objectAtRunIndex:length:`, `_runArrayHoldingAttributes`, `isEqual:`, and `setFormattingContext:`
at `0x2387cf54a`, `0x2387bdfe7`, `0x2387aa169`, and `0x2387d4f8a` follow adjacent direct sends,
method symbols, and the captured argument registers.

## Assembly details and divergences

- The proxy forwarding paths pass the original buffer, range and replacement arguments in registers.
  Hex-Rays drops them on several tail calls. The index-set single-index initializer likewise passes
  length 1 in X3 on its large-index path.
- Markdown source-position initialization and cache invalidation use integer bit pattern
  `0x7fffffffffffffff` from MOVI and FNEG. Hex-Rays labels the SIMD lanes as NaN. The cache pointer is X4
  in both offset-update calls and the method returns void. The position hash includes offset fields
  while equality checks only line and column getters (kept).
- `-[NSSimpleAttributeDictionary newWithKey:object:]` retains the supplied object in its replacement
  path. Hex-Rays leaves the corresponding alias uninitialized.
- `-[NSConcreteMutableAttributedString edited:range:changeInLength:]` has a single RET instruction.
  Other routines displayed with a pointer result merely leave a register live at return, including
  Objective-C destruction, release, and the mutable string proxy's forwarding methods.
- The string context test at `0x1c054e0ec` branches on the location word. A match at zero takes
  the immediate formatting-context-4 path and NSNotFound enters tokenization (kept).
- `-[NSMutableRLEArray replaceObjectsInRange:withObject:length:]` checks bounds before ignoring a
  zero-length removed range (kept). `-[_NSPlaceholderCharacterSet isEmpty]` always returns NO for the
  non-inverted placeholder (kept).
- Foundation defect at `0x1c04da204` (verified). The ordinary read of the lazily published inverted
  set races with CASAL. Atomic pointer loads and compare/exchange preserve publication and ownership.
- Foundation defects at `0x1c04c4b50`, `0x1c04c4948`, `0x1c04c5918`, and `0x1c04c5a70` (verified).
  The native cached class and warning-byte accesses are unsynchronized. Relaxed atomic accesses
  preserve the read, log or lookup, and store order.

## Captured definitions

### `ONSConcreteAttributedString.m`

- `0x1c04c4948` `-[NSConcreteAttributedString initWithString:]`
- `0x1c04c4aa8` `-[NSConcreteAttributedString initWithString:attributes:]`
- `0x1c04c4b98` `-[NSConcreteAttributedString initWithAttributedString:]`
- `0x1c04c4d2c` `-[NSConcreteAttributedString init]`
- `0x1c04c4d38` `-[NSConcreteAttributedString dealloc]`
- `0x1c04c4db4` `-[NSConcreteAttributedString copyWithZone:]`
- `0x1c04c4e64` `-[NSConcreteAttributedString attributesAtIndex:effectiveRange:]`
- `0x1c04c4eb0` `-[NSConcreteAttributedString attribute:atIndex:effectiveRange:]`
- `0x1c04c4f0c` `-[NSConcreteAttributedString length]`
- `0x1c04c4f14` `-[NSConcreteAttributedString string]`
- `0x1c04c4f1c` `-[NSConcreteAttributedString _runArrayHoldingAttributes]`
- `0x1c04c56ec` `+[NSConcreteMutableAttributedString _mutableStringClass]`
- `0x1c04c56f8` `-[NSConcreteMutableAttributedString initWithAttributedString:]`
- `0x1c04c5918` `-[NSConcreteMutableAttributedString initWithString:attributes:]`
- `0x1c04c5a70` `-[NSConcreteMutableAttributedString initWithString:]`
- `0x1c04c5b78` `-[NSConcreteMutableAttributedString init]`
- `0x1c04c5b84` `-[NSConcreteMutableAttributedString dealloc]`
- `0x1c04c5c00` `-[NSConcreteMutableAttributedString attributesAtIndex:effectiveRange:]`
- `0x1c04c5c08` `-[NSConcreteMutableAttributedString attribute:atIndex:effectiveRange:]`
- `0x1c04c5c38` `-[NSConcreteMutableAttributedString length]`
- `0x1c04c5c40` `-[NSConcreteMutableAttributedString string]`
- `0x1c04c5c48` `-[NSConcreteMutableAttributedString _runArrayHoldingAttributes]`
- `0x1c04c5c50` `-[NSConcreteMutableAttributedString replaceCharactersInRange:withAttributedString:]`
- `0x1c04c5d88` `-[NSConcreteMutableAttributedString addAttribute:value:range:]`
- `0x1c04c5f40` `-[NSConcreteMutableAttributedString attribute:atIndex:longestEffectiveRange:inRange:]`
- `0x1c04c616c` `-[NSConcreteMutableAttributedString setAttributes:range:]`
- `0x1c04c61fc` `-[NSConcreteMutableAttributedString replaceCharactersInRange:withString:]`
- `0x1c04c6310` `-[NSConcreteMutableAttributedString edited:range:changeInLength:]`
- `0x1c04c6314` `-[NSConcreteMutableAttributedString initWithCoder:]`
- `0x1c04c6354` `+[NSConcreteMutableAttributedString supportsSecureCoding]`
- `0x1c04c635c` `-[NSConcreteMutableAttributedString _mayRequireIntentResolution]`
- `0x1c04c636c` `-[NSConcreteMutableAttributedString _markRequiringIntentResolution]`

### `ONSSimpleAttributeDictionary.m`

- `0x1c04c4b50` `__NSAttributeDictionaryClass`
- `0x1c04c72c0` `+[NSSimpleAttributeDictionary emptyAttributeDictionary]`
- `0x1c04c7300` `___55+[NSSimpleAttributeDictionary emptyAttributeDictionary]_block_invoke`
- `0x1c04c733c` `+[NSSimpleAttributeDictionary newWithDictionary:]`
- `0x1c04c74c8` `-[NSSimpleAttributeDictionary newWithKey:object:]`
- `0x1c04c770c` `-[NSSimpleAttributeDictionary count]`
- `0x1c04c771c` `-[NSSimpleAttributeDictionary slotForKey:]`
- `0x1c04c77f0` `-[NSSimpleAttributeDictionary objectForKey:]`
- `0x1c04c783c` `-[NSSimpleAttributeDictionary keyEnumerator]`
- `0x1c04c786c` `-[NSSimpleAttributeDictionary dealloc]`
- `0x1c04c7924` `-[NSSimpleAttributeDictionary mutableCopyWithZone:]`
- `0x1c04c7a00` `-[NSSimpleAttributeDictionary copyWithZone:]`
- `0x1c04c7a04` `-[NSSimpleAttributeDictionaryEnumerator initWithAttributeDictionary:]`
- `0x1c04c7a98` `-[NSSimpleAttributeDictionaryEnumerator nextObject]`
- `0x1c04c7ae8` `-[NSSimpleAttributeDictionaryEnumerator dealloc]`

### `ONSMutableAttributedString.m`

- `0x1c04c5600` `-[NSMutableAttributedString appendAttributedString:]`

### `ONSMutableStringProxyForMutableAttributedString.m`

- `0x1c04c6388` `-[NSMutableStringProxyForMutableAttributedString initWithMutableAttributedString:]`
- `0x1c04c6410` `-[NSMutableStringProxyForMutableAttributedString dealloc]`
- `0x1c04c648c` `-[NSMutableStringProxyForMutableAttributedString length]`
- `0x1c04c64ac` `-[NSMutableStringProxyForMutableAttributedString characterAtIndex:]`
- `0x1c04c64dc` `-[NSMutableStringProxyForMutableAttributedString getCharacters:range:]`
- `0x1c04c6524` `-[NSMutableStringProxyForMutableAttributedString replaceCharactersInRange:withString:]`

### `ONSRLEArray.m`

- `0x1c04c6534` `-[NSRLEArray init]`
- `0x1c04c653c` `-[NSRLEArray initWithRefCountedRunArray:]`
- `0x1c04c65e4` `-[NSRLEArray copyWithZone:]`
- `0x1c04c65e8` `-[NSRLEArray mutableCopyWithZone:]`
- `0x1c04c6614` `-[NSRLEArray dealloc]`
- `0x1c04c66e8` `-[NSRLEArray _makeNewListFrom:]`
- `0x1c04c67f4` `-[NSRLEArray objectAtIndex:effectiveRange:]`
- `0x1c04c689c` `_blockForLocation`
- `0x1c04c6994` `-[NSRLEArray objectAtIndex:effectiveRange:runIndex:]`
- `0x1c04c6a80` `-[NSRLEArray count]`
- `0x1c04c6a8c` `-[NSRLEArray description]`
- `0x1c04c6b50` `-[NSMutableRLEArray copyWithZone:]`
- `0x1c04c6b7c` `-[NSMutableRLEArray _setBlockCapacity:]`
- `0x1c04c6bf8` `-[NSMutableRLEArray insertObject:range:]`
- `0x1c04c6f08` `-[NSMutableRLEArray replaceObjectsInRange:withObject:length:]`
- `0x1c04c72b4` `-[NSMutableRLEArray deleteObjectsInRange:]`

### `ONSCharacterSet.m`

- `0x1c04d8a20` `+[NSCharacterSet lowercaseLetterCharacterSet]`
- `0x1c04d9a80` `+[NSMutableCharacterSet lowercaseLetterCharacterSet]`
- `0x1c04d9d50` `-[NSCFCharacterSet isEqual:]`
- `0x1c04d9d8c` `-[NSCFCharacterSet hash]`
- `0x1c04d9ddc` `-[NSCFCharacterSet copyWithZone:]`
- `0x1c04d9de8` `-[NSCFCharacterSet mutableCopyWithZone:]`
- `0x1c04d9df4` `-[NSCFCharacterSet characterIsMember:]`
- `0x1c04d9e14` `-[NSCFCharacterSet longCharacterIsMember:]`
- `0x1c04d9e34` `-[NSCFCharacterSet isSupersetOfSet:]`
- `0x1c04d9e74` `-[NSCFCharacterSet bitmapRepresentation]`
- `0x1c04d9e90` `-[NSCFCharacterSet invertedSet]`
- `0x1c04d9ee4` `-[NSCFCharacterSet invert]`
- `0x1c04d9ef0` `-[NSCFCharacterSet classForArchiver]`
- `0x1c04d9f20` `-[NSCFCharacterSet classForKeyedArchiver]`
- `0x1c04d9f24` `-[NSCFCharacterSet encodeWithCoder:]`
- `0x1c04da0e8` `-[_NSPlaceholderCharacterSet initWithSet:options:]`
- `0x1c04da1ec` `-[_NSPlaceholderCharacterSet _expandedCFCharacterSet]`
- `0x1c04da204` `-[_NSPlaceholderCharacterSet _expandInverted]`
- `0x1c04da274` `-[_NSPlaceholderCharacterSet mutableCopyWithZone:]`
- `0x1c04da2cc` `-[_NSPlaceholderCharacterSet characterIsMember:]`
- `0x1c04da2d0` `-[_NSPlaceholderCharacterSet longCharacterIsMember:]`
- `0x1c04da30c` `-[_NSPlaceholderCharacterSet isSupersetOfSet:]`
- `0x1c04da3d0` `-[_NSPlaceholderCharacterSet isEqual:]`
- `0x1c04da454` `-[_NSPlaceholderCharacterSet bitmapRepresentation]`
- `0x1c04da498` `-[_NSPlaceholderCharacterSet isEmpty]`
- `0x1c04da4e4` `-[_NSPlaceholderCharacterSet invertedSet]`
- `0x1c04da4fc` `-[_NSPlaceholderCharacterSet autorelease]`
- `0x1c04da564` `-[_NSPlaceholderCharacterSet retain]`
- `0x1c04da5cc` `-[_NSPlaceholderCharacterSet retainCount]`
- `0x1c04da63c` `-[_NSPlaceholderCharacterSet release]`
- `0x1c04da784` `-[_NSPlaceholderCharacterSet dealloc]`
- `0x1c04da808` `-[_NSPlaceholderCharacterSet replacementObjectForKeyedArchiver:]`
- `0x1c04da844` `-[_NSPlaceholderCharacterSet replacementObjectForCoder:]`

### `ONSIndexSet.m`

- `0x1c05279a4` `-[_NSPlaceholderIndexSet retainCount]`
- `0x1c05279ac` `-[_NSPlaceholderIndexSet init]`
- `0x1c0527a08` `-[_NSPlaceholderIndexSet initWithIndex:]`
- `0x1c0527a24` `-[_NSPlaceholderIndexSet initWithIndexesInRange:]`
- `0x1c0527b4c` `-[_NSPlaceholderIndexSet initWithBitfield:]`
- `0x1c0527be8` `-[_NSPlaceholderIndexSet initWithIndexSet:]`
- `0x1c0527d9c` `-[_NSPlaceholderIndexSet initWithCoder:]`
- `0x1c0527dc8` `-[_NSConstantIndexSet retainCount]`

### `ONSStringProxyForContext.m`

- `0x1c054df84` `-[_NSStringProxyForContext dealloc]`
- `0x1c054e02c` `-[_NSStringProxyForContext length]`
- `0x1c054e03c` `-[_NSStringProxyForContext characterAtIndex:]`
- `0x1c054e04c` `-[_NSStringProxyForContext getCharacters:range:]`
- `0x1c054e05c` `-[_NSStringProxyForContext _retainFormatter:]`
- `0x1c054e0a4` `-[_NSStringProxyForContext _dynamicContextEvaluation:patternString:]`
- `0x1c054e258` `-[_NSStringProxyForContext copyWithZone:]`
- `0x1c054e2cc` `-[_NSStringProxyForContext initWithCoder:]`
- `0x1c054e3c8` `-[_NSStringProxyForContext encodeWithCoder:]`
- `0x1c054e49c` `-[_NSStringProxyForContext string]`
- `0x1c054e4ac` `-[_NSStringProxyForContext setString:]`
- `0x1c054e4b8` `-[_NSStringProxyForContext formatter]`
- `0x1c054e4c8` `-[_NSStringProxyForContext setFormatter:]`
- `0x1c054e4d4` `-[_NSStringProxyForContext item]`
- `0x1c054e4e4` `-[_NSStringProxyForContext setItem:]`

### `ONSAttributeDescriptor.m`

- `0x1c0574514` `-[_NSAttributeDescriptor dealloc]`
- `0x1c0574588` `-[_NSAttributeDescriptor name]`
- `0x1c05748ec` `+[_NSAttributeDescriptor(NSAttributedDescriptorForAttributeKey) descriptorForAttributeKey:]`
- `0x1c0574a24` `___91+[_NSAttributeDescriptor(NSAttributedDescriptorForAttributeKey) descriptorForAttributeKey:]_block_invoke`
- `0x1c0fe3b7c` `-[_NSAttributeDescriptor attributeValueFromValue:]`

### `ONSObjCRuntimeNames.m`

- `0x1c057e724` `_NSStringFromSelector`
- `0x1c057e800` `___NSStringFromSelector_block_invoke`

### `ONSString.m`

- `0x1c05d4444` `-[NSString getCharacters:]`

### `ONSAttributedStringMarkdownSourcePosition.m`

- `0x1c060061c` `-[NSAttributedStringMarkdownSourcePosition startLine]`
- `0x1c0600624` `-[NSAttributedStringMarkdownSourcePosition startColumn]`
- `0x1c060062c` `-[NSAttributedStringMarkdownSourcePosition endLine]`
- `0x1c0600634` `-[NSAttributedStringMarkdownSourcePosition endColumn]`
- `0x1c060063c` `-[NSAttributedStringMarkdownSourcePosition initWithStartLine:startColumn:endLine:endColumn:]`
- `0x1c06007ac` `-[NSAttributedStringMarkdownSourcePosition setOffsetsFromUTF8:size:usingCache:]`
- `0x1c0600828` `_updateOffsetsForPosition`
- `0x1c0600ac4` `-[NSAttributedStringMarkdownSourcePosition copyWithZone:]`
- `0x1c0600ac8` `+[NSAttributedStringMarkdownSourcePosition supportsSecureCoding]`
- `0x1c0600ad0` `-[NSAttributedStringMarkdownSourcePosition description]`
- `0x1c0600b68` `-[NSAttributedStringMarkdownSourcePosition debugDescription]`
- `0x1c0600bd8` `-[NSAttributedStringMarkdownSourcePosition encodeWithCoder:]`
- `0x1c0600d60` `-[NSAttributedStringMarkdownSourcePosition initWithCoder:]`
- `0x1c06010b8` `-[NSAttributedStringMarkdownSourcePosition hash]`
- `0x1c0601140` `-[NSAttributedStringMarkdownSourcePosition isEqual:]`

### `ONSAttributedStringMarkdownParsingOptions.m`

- `0x1c0601228` `-[NSAttributedStringMarkdownParsingOptions init]`
- `0x1c0601298` `-[NSAttributedStringMarkdownParsingOptions dealloc]`
- `0x1c060130c` `-[NSAttributedStringMarkdownParsingOptions copyWithZone:]`
- `0x1c060139c` `-[NSAttributedStringMarkdownParsingOptions hash]`
- `0x1c0601404` `-[NSAttributedStringMarkdownParsingOptions isEqual:]`
- `0x1c060150c` `-[NSAttributedStringMarkdownParsingOptions allowsExtendedAttributes]`
- `0x1c0601518` `-[NSAttributedStringMarkdownParsingOptions setAllowsExtendedAttributes:]`
- `0x1c0601520` `-[NSAttributedStringMarkdownParsingOptions interpretedSyntax]`
- `0x1c0601528` `-[NSAttributedStringMarkdownParsingOptions setInterpretedSyntax:]`
- `0x1c0601530` `-[NSAttributedStringMarkdownParsingOptions failurePolicy]`
- `0x1c0601538` `-[NSAttributedStringMarkdownParsingOptions setFailurePolicy:]`
- `0x1c0601540` `-[NSAttributedStringMarkdownParsingOptions languageCode]`
- `0x1c060154c` `-[NSAttributedStringMarkdownParsingOptions setLanguageCode:]`
- `0x1c0601554` `-[NSAttributedStringMarkdownParsingOptions appliesSourcePositionAttributes]`
- `0x1c0601560` `-[NSAttributedStringMarkdownParsingOptions setAppliesSourcePositionAttributes:]`

### `ONSAttributedStringFromMarkdownCreatorConcrete.m`

- `0x1c06018dc` `-[_NSAttributedStringFromMarkdownCreatorConcrete init]`
- `0x1c0601960` `-[_NSAttributedStringFromMarkdownCreatorConcrete dealloc]`
- `0x1c06019d4` `-[_NSAttributedStringFromMarkdownCreatorConcrete result]`
- `0x1c06019ec` `-[_NSAttributedStringFromMarkdownCreatorConcrete appendString:with:]`
- `0x1c0601a9c` `-[_NSAttributedStringFromMarkdownCreatorConcrete appendString:]`
- `0x1c0601ac4` `-[_NSAttributedStringFromMarkdownCreatorConcrete parseExtendedAttributesFromData:error:]`
- `0x1c0601c4c` `-[_NSAttributedStringFromMarkdownCreatorConcrete finalizeWithLanguageIdentifier:]`
- `0x1c0601d08` `___81-[_NSAttributedStringFromMarkdownCreatorConcrete finalizeWithLanguageIdentifier:]_block_invoke`
- `0x1c0602df8` `___attributesFromKnownAttributeKeys_block_invoke`

### `ONSPresentationIntent.m`

- `0x1c068dd2c` `-[NSPresentationIntent _init]`
- `0x1c068dd8c` `-[NSPresentationIntent dealloc]`
- `0x1c068de10` `-[NSPresentationIntent hash]`
- `0x1c068de74` `-[NSPresentationIntent isEqual:]`
- `0x1c068df00` `-[NSPresentationIntent isEquivalentToPresentationIntent:]`
- `0x1c068e0ec` `-[NSPresentationIntent description]`
- `0x1c068e444` `-[NSPresentationIntent copyWithZone:]`
- `0x1c068e448` `+[NSPresentationIntent supportsSecureCoding]`
- `0x1c068e450` `-[NSPresentationIntent initWithCoder:]`
- `0x1c068e644` `-[NSPresentationIntent encodeWithCoder:]`
- `0x1c068e790` `+[NSPresentationIntent paragraphIntentWithIdentity:nestedInsideIntent:]`
- `0x1c068e7e4` `+[NSPresentationIntent headerIntentWithIdentity:level:nestedInsideIntent:]`
- `0x1c068e85c` `+[NSPresentationIntent codeBlockIntentWithIdentity:languageHint:nestedInsideIntent:]`
- `0x1c068e8dc` `+[NSPresentationIntent thematicBreakIntentWithIdentity:nestedInsideIntent:]`
- `0x1c068e934` `+[NSPresentationIntent orderedListIntentWithIdentity:nestedInsideIntent:]`
- `0x1c068e9a4` `+[NSPresentationIntent unorderedListIntentWithIdentity:nestedInsideIntent:]`
- `0x1c068ea14` `+[NSPresentationIntent listItemIntentWithIdentity:ordinal:nestedInsideIntent:]`
- `0x1c068ea8c` `+[NSPresentationIntent blockQuoteIntentWithIdentity:nestedInsideIntent:]`
- `0x1c068eb00` `+[NSPresentationIntent tableIntentWithIdentity:columnCount:alignments:nestedInsideIntent:]`
- `0x1c068ebe4` `+[NSPresentationIntent tableRowIntentWithIdentity:row:nestedInsideIntent:]`
- `0x1c068ec44` `+[NSPresentationIntent tableHeaderRowIntentWithIdentity:nestedInsideIntent:]`
- `0x1c068ec9c` `+[NSPresentationIntent tableCellIntentWithIdentity:column:nestedInsideIntent:]`
- `0x1c068ecfc` `-[NSPresentationIntent intentKind]`
- `0x1c068ed04` `-[NSPresentationIntent parentIntent]`
- `0x1c068ed10` `-[NSPresentationIntent identity]`
- `0x1c068ed18` `-[NSPresentationIntent ordinal]`
- `0x1c068ed20` `-[NSPresentationIntent columnAlignments]`
- `0x1c068ed28` `-[NSPresentationIntent columnCount]`
- `0x1c068ed30` `-[NSPresentationIntent headerLevel]`
- `0x1c068ed38` `-[NSPresentationIntent languageHint]`
- `0x1c068ed44` `-[NSPresentationIntent column]`
- `0x1c068ed4c` `-[NSPresentationIntent row]`
- `0x1c068ed54` `-[NSPresentationIntent indentationLevel]`

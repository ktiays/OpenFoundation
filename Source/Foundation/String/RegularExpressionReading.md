<!--
  Created by ktiays on 2026/11/05.
  Copyright (c) 2026 ktiays. All rights reserved.
-->

# Regular Expression Reading

iOS Simulator 24A434, arm64: Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03, CoreFoundation UUID
D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC. iOS Device 24A446: unread. Confidence: verified (assembly read in
full) unless stated.

## Units

| Unit | Native range | Restored in |
| --- | --- | --- |
| NSRegularExpression, NSMatching, NSReplacement, ICU callbacks | Foundation 0x1c0650f0c-0x1c0653e7b | `Foundation/String/ONSRegularExpression.m` |
| Simple, extended and complex regular-expression checking results | Foundation 0x1c061e508-0x1c061f23b | `Foundation/String/ONSTextCheckingResult.m` |
| `__NSFullMethodName` | Foundation 0x1c05a104c | `Foundation/String/ONSRuntimeSupport.m` |
| CFString UText provider (`CFStringOpenUText`, `__CFStringUTextInit`, callbacks) | CoreFoundation 0x1c0232fd8-0x1c0233de7 | `CoreFoundation/String/OCFStringUText.c` |
| `CFAttributedStringBeginEditing`, `CFAttributedStringEndEditing` | CoreFoundation 0x1c014f5b8, 0x1c014f5f8 | `CoreFoundation/String/OCFAttributedString.c` |

The scope closure reached `CFStringOpenUText` and `__CFStringUTextInit` only. Their UText callbacks are stored
in the function tables `__CFStringFuncs` and `__CFAttributedStringFuncs`; these indirect edges were resolved by
reading, and the attributed-string callbacks need the two editing functions. ICU (`uregex_*`, `utext_*`) is
supplied by libicucore; the UText and UTextFuncs layouts are declared from the native code's field accesses.

## Behavior notes

- Matching runs over a CFString UText. The compiled `URegularExpression` is used in place while the
  `_checkout` flag is free, and cloned otherwise.
- Results with 1-3 ranges are simple, 4-7 extended, more complex (an array of NSValue ranges). Unused fixed
  ranges hold NSNotFound in both words.
- The anchored option is enforced by the find-progress callback: a find past the previous match end stops.
- When the UText setup fails after the checkout was taken, the native code returns without releasing it or
  closing a clone; this is kept.
- `replacementStringForResult:` expands `$n` (as many digits as the range count has) and `\c`.
- The UText chunk is the string's own buffer when available, otherwise a q buffer refilled with 32 to 512
  characters, growing while accesses stay near the previous chunk. Extracting an empty range reports
  `U_BUFFER_OVERFLOW_ERROR` (native).

<!--
  Created by ktiays on 2026/10/31.
  Copyright (c) 2026 ktiays. All rights reserved.
-->

# CoreFoundation string formatting and logging

## Identity and scope

This reading covers the cf-core string-format family: the formatter
`__CFStringAppendFormatCore` and its helpers, the public
`CFStringCreateWithFormat(AndArguments)` and `CFStringAppendFormat(AndArguments)`
entry points and SPI, format validation, localized numbers, replacement
metadata, stringsdict-style configuration resolution, the vocative and
conditional-formatting helpers, the inflection hand-off, `__plistAppendFormat`
and `CFLog` with `_CFLogvEx3`. It is not a restoration of the CFString object
core, CFNumberFormatter, CFLocale, the collections or the call-out trampolines,
which other families own.

| Reference | Identity |
| --- | --- |
| iOS Simulator | iOS 27.0 (24A434), arm64; CoreFoundation UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC |
| iOS Device | iOS 27.0.1 (24A446), arm64e; unread for this family |

The evidence is the cf-core corpus
`Local/Evidence/cf-core/iphone-simulator-core-corpus.json` (assembly and
pseudocode per function), static data read from the canonical image and the
`nm -m` linkage. The assembly settles every branch, constant and call order;
the pseudocode located questions only. No IDA session was opened.

## Units

| Source | Native span | Contents |
| --- | --- | --- |
| `Source/CoreFoundation/String/OCFStringLocalizedFormatting.m` | 0x1c02027f4-0x1c02049a4, data 0x2227a4fe0-0x2227a50a8 | formatting-family character sets, locale summaries, conditional formatting, vocative case (cs, el, uk, lt) and the constant dictionary `__CFStringLocalizedFormattingInfo` |
| `Source/CoreFoundation/String/OCFStringFormat.c` | CFString.c, 0x1c0209e60-0x1c02258c4 (formatting share) | parser, formatter, validation, localized numbers, metadata, isolates, public entry points |
| `Source/CoreFoundation/String/OCFStringFormatConfiguration.m` | 0x1c02caef8-0x1c02cbfa8, data 0x2227aa1a0-0x2227aa2a8 | configuration lookup, affix classes, configuration resolution, inflection hand-off, six of the unit's keys |
| `Source/CoreFoundation/Base/OCFUtilities.c` | 0x1c025738c-0x1c02599d0 | `__CFLogvEx2Predicate`, `_CFLogvEx3`, `_CFLogSimpleCString`, `CFLog`, `__CFLogCString` |
| `Source/CoreFoundation/PropertyList/OCFPropertyList.c` | 0x1c01d7f84 | `__plistAppendFormat` |

Both Objective-C units are `.m` because the native code throws from the
`new_id_array` pattern and `__CFStringGetRelevantLocaleInfoFromLocaleSummary`
has a cleanup landing pad that frees the temporary key buffer while an
exception unwinds.

## Native implementation

- The 32-byte specification record (`FormatSpecRecord`): size and type at 0
  and 2, location and length at 4 and 8, width and precision at 12 and 16,
  flags at 20, the main, precision, width and configuration indexes at 24..27
  and the numeric style at 28. The 16-byte value record (`PrintValue`): type,
  size and the eight-byte value at 8. Both are verified from every store and
  load the functions make.
- The formatter reads the Darwin arm64 `va_list` as eight-byte slots; unused
  argument numbers skip a slot when the program is linked on or after the SDK
  0x7e60901.
- Literal text and unknown conversions are not recorded by the validation
  parser `__CFStringFindFormatSpecifiersInString` (the record index is kept).
- Errors are `NSCocoaErrorDomain` 2048 with `NSDebugDescription`.
- `_CFLogvEx3` uses the os_log NSLog shim for its caller when enabled and
  repeats the message on stderr when `also_do_stderr(false)` allows it; the
  shim's log handle is the GOT slot 0x22241b4d8 outside the image, taken to be
  `OS_LOG_DEFAULT` (confidence: medium).

## Confidence

Verified from the assembly for every function except the body of
`__CFStringAppendFormatCore`, which is medium: its constants, calls, record
layouts, argument fetch, `snprintf_l` argument lists, error sites and cleanup
order were checked instruction by instruction; the remaining control flow
(notably the inline value-type parser) follows the pseudocode.

## Defects and divergences

- `__CFStringValidateFormat` returns -1 for an over-long expected string
  without releasing the format's buffers (a leak); kept.
- `__CFStringAppendFormatCore` loses the previous attribute dictionary when a
  second `%[...]@` attribute is resolved (only the last is released); kept.
- `__CFLogvEx2Predicate` runs `strlen` on the conversion buffer whether or not
  `CFStringGetCString` succeeded; kept, pending the string-core reading of its
  failure behavior.

## iOS Device comparison

Unread for every function of this family.

## Verification and limits

Compile-only in both naming modes. No behavioral, differential or leak test
exists yet; the family links only once the declared dependencies of other
families are restored.

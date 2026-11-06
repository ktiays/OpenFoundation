//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>
#include <OpenCoreFoundation/Date/OCFDate.h>

#include "CoreFoundation/Locale/OCFLocale.h"
#include "CoreFoundation/Locale/OCFLocaleKeys.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"

#include <stdint.h>

// CFDateFormatter.h (iOS Simulator SDK interface), spelled OCF. Locale/OCFDateFormatter.c restores the
// functions of the CFDateFormatter unit that the scope reaches; the rest of the unit
// (CFDateFormatterGetTypeID, the class record and its callbacks, the getters, parsing, templates and ISO
// 8601) is not restored on this branch.
typedef struct __OCFDateFormatter *OCFDateFormatterRef;
typedef OCFStringRef OCFDateFormatterKey;

// CFDateFormatterStyle (SDK CFDateFormatter.h values).
typedef enum : OCFIndex {
    kOCFDateFormatterNoStyle = 0,
    kOCFDateFormatterShortStyle = 1,
    kOCFDateFormatterMediumStyle = 2,
    kOCFDateFormatterLongStyle = 3,
    kOCFDateFormatterFullStyle = 4,
} OCFDateFormatterStyle;

OF_EXPORT OCFDateFormatterRef OCFDateFormatterCreate(
    OCFAllocatorRef allocator,
    OCFLocaleRef locale,
    OCFDateFormatterStyle dateStyle,
    OCFDateFormatterStyle timeStyle
);
OF_EXPORT void OCFDateFormatterSetFormat(OCFDateFormatterRef formatter, OCFStringRef formatString);
OF_EXPORT OCFStringRef OCFDateFormatterCreateStringWithDate(
    OCFAllocatorRef allocator, OCFDateFormatterRef formatter, OCFDateRef date
);
OF_EXPORT OCFStringRef OCFDateFormatterCreateStringWithAbsoluteTime(
    OCFAllocatorRef allocator, OCFDateFormatterRef formatter, OCFAbsoluteTime at
);
OF_EXPORT OCFTypeRef OCFDateFormatterCopyProperty(OCFDateFormatterRef formatter, OCFDateFormatterKey key);
OF_EXPORT void OCFDateFormatterSetProperty(OCFDateFormatterRef formatter, OCFStringRef key, OCFTypeRef value);

// The CFDateFormatter property keys are defined in the locale-keys table, Locale/OCFLocaleKeys.c, and
// declared by Locale/OCFLocaleKeys.h.

// The two global block literals of the unit, defined in Locale/OCFDateFormatter.c with their restored invoke
// functions: ___block_literal_global (0x22279d7a0), whose invoke is
// ___useTemplatePatternGenerator_block_invoke (0x1c0193c8c), the work block the memory-pressure handler of
// useTemplatePatternGenerator calls; and
// ___block_literal_global.31 (0x22279d7e8), whose invoke is
// _____CFDateFormatterCreateForcedString_block_invoke (0x1c0193d20), the dispatch_once block of
// __CFDateFormatterCreateForcedString that fills forcedStringHourCharacters. Both are non-external natively;
// the spellings are local descriptive names, and each is the block object itself (its address is the block).
OF_EXTERN uint8_t useTemplatePatternGeneratorResetBlock[];
OF_EXTERN uint8_t forcedStringHourCharactersOnceBlock[];

// The function-local statics of useTemplatePatternGenerator (natively _useTemplatePatternGenerator.ptg,
// .ptgLock, .ptgLocaleName and .pressureSrc, 0x22e6d27b8-0x22e6d27d0, __DATA,__bss) and of
// __CFDateFormatterCreateForcedString (.hourCharacters 0x22e6d27d8, .onceToken 0x22e6d27e0), defined in
// Locale/OCFDateFormatter.c. They are hidden rather than static, as the block invokes above use them; local
// descriptive names.
OF_EXTERN void **templatePatternGenerator;
OF_EXTERN char *templatePatternGeneratorLocaleName;
OF_EXTERN const struct __OCFCharacterSet *forcedStringHourCharacters;

// Native __CFDateFormatterClass, 0x22279d718 (__DATA_CONST,__const); was a private external. Type 56.
OF_EXTERN const OCFRuntimeClass __CFDateFormatterClass;

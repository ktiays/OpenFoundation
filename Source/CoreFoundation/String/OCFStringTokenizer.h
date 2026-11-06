//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

typedef struct __OCFStringTokenizer *OCFStringTokenizerRef;
typedef OCFOptionFlags OCFStringTokenizerTokenType;

OF_EXTERN const OCFRuntimeClass __CFStringTokenizerRuntimeClass;

OF_EXPORT OCFStringTokenizerRef OCFStringTokenizerCreate(
    OCFAllocatorRef alloc, OCFStringRef string, OCFRange range, OCFOptionFlags options, OCFLocaleRef locale
);
OF_EXPORT void
OCFStringTokenizerSetString(OCFStringTokenizerRef tokenizer, OCFStringRef string, OCFRange range);
OF_EXPORT OCFStringTokenizerTokenType
OCFStringTokenizerGoToTokenAtIndex(OCFStringTokenizerRef tokenizer, OCFIndex index);
OF_EXPORT OCFStringTokenizerTokenType OCFStringTokenizerAdvanceToNextToken(OCFStringTokenizerRef tokenizer);
OF_EXPORT OCFRange OCFStringTokenizerGetCurrentTokenRange(OCFStringTokenizerRef tokenizer);

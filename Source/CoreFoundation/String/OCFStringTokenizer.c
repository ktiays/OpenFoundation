//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/String/OCFStringTokenizer.h"

#include "CoreFoundation/String/OCFStringFormat.h"

#include <assert.h>
#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <malloc/malloc.h>
#include <stdlib.h>

// Native unit: CFStringTokenizer.c, named by the simulator-root assertion. iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. iOS Device divergence:
// this unit is unread on 24A446. Reading: Local/Evidence/cf-core/iphone-simulator-corpus-delta13.json
// and Local/Scratch/cf-core-r2-c2/tokenizer-runtime-ida.json (class record, table and finalizer).

struct __OCFStringTokenizer {
    OCFRuntimeBase base;
    OCFTypeRef tokenizer;
};

_Static_assert(offsetof(struct __OCFStringTokenizer, tokenizer) == 0x10, "The CoreNLP object is at 16.");

static dispatch_once_t coreNLPOnce;
static bool coreNLPAvailable;

// Native __NLStringTokenizer* and ___NLStringTokenizer* pointers, 0x22e6d3128 through 0x22e6d31d8.
// The lookup tests all 23 results. The signatures of unused entry points remain unread.
static void *__NLStringTokenizerCopyBestStringLanguage;
static OCFTypeRef (*__NLStringTokenizerCreate)(
    OCFAllocatorRef, OCFStringRef, OCFRange, OCFOptionFlags, OCFLocaleRef
);
static void (*__NLStringTokenizerSetString)(OCFTypeRef, OCFStringRef, OCFRange);
static OCFStringTokenizerTokenType (*__NLStringTokenizerGoToTokenAtIndex)(OCFTypeRef, OCFIndex);
static OCFStringTokenizerTokenType (*__NLStringTokenizerAdvanceToNextToken)(OCFTypeRef);
static OCFRange (*__NLStringTokenizerGetCurrentTokenRange)(OCFTypeRef);
static void *__NLStringTokenizerCopyCurrentTokenAttribute;
static void *__NLStringTokenizerGetCurrentSubTokens;
static void *__NLStringTokenizerCopyBestStringLanguageWithHints;
static void *__NLStringTokenizerCopyPossibleStringLanguages;
static void *__NLStringTokenizerTokenize;
static void *__NLStringTokenizerTokenizeWithTranscriptions;
static void *__NLStringTokenizerTokenizeCompoundWord;
static void *__NLStringTokenizerGetDerivedTokens;
static void *__NLStringTokenizerGetNextBestTokens;
static void *__NLStringTokenizerCreateNamePartsArrayWithString;
static void *__NLStringTokenizerCreateNamePartsArrayWithStringAsCompositeName;
static void *__NLStringTokenizerCreatePossibleNamePartsArrayWithStringAsCompositeName;
static void *__NLStringTokenizerGetCurrentTokenPartOfSpeech;
static void *__NLStringTokenizerSetLocale;
static void *__NLStringTokenizerSetTokenizerAttribute;
static void *__NLStringTokenizerGetTokenizerAttribute;
static void *__NLStringTokenizerSetStringWithOptions;

// Native __CFStringTokenizerDeallocate, 0x1c022eddc. Non-external.
static void __CFStringTokenizerDeallocate(OCFTypeRef object) {
    OCFStringTokenizerRef tokenizer = (OCFStringTokenizerRef)object;
    if (tokenizer != NULL && tokenizer->tokenizer != NULL) {
        OCFRelease(tokenizer->tokenizer);
    }
}

// Native __CFStringTokenizerRuntimeClass, 0x2227a55c0. Hidden (was private external).
// Slot 40, at 0x2227a4208, points at this record. Only the name and finalizer are nonzero.
const OCFRuntimeClass __CFStringTokenizerRuntimeClass = {
    .className = "CFStringTokenizer",
    .finalize = __CFStringTokenizerDeallocate,
};

// Native CFStringTokenizerCreate, 0x1c022ee70, with .cold.1 at 0x1c030cadc and
// OUTLINED_FUNCTION_0_28 at 0x1c022f5e4. External.
OCFStringTokenizerRef OCFStringTokenizerCreate(
    OCFAllocatorRef alloc, OCFStringRef string, OCFRange range, OCFOptionFlags options, OCFLocaleRef locale
) {
    // Native ____CFSTLoadCoreNLP_block_invoke, 0x1c022f15c, with .cold.1 at 0x1c030cae8.
    dispatch_once(&coreNLPOnce, ^{
      const char *simulatorRoot = getenv("IPHONE_SIMULATOR_ROOT");
      if (simulatorRoot == NULL) {
          __assert_rtn("__CFSTCreateSystemRootPath", "CFStringTokenizer.c", 96, "simulatorRoot");
      }
      OCFStringRef root =
          OCFStringCreateWithCString(kOCFAllocatorDefault, simulatorRoot, kOCFStringEncodingUTF8);
      if (root == NULL) {
          return;
      }
      OCFStringRef path = OCFStringCreateWithFormat(
          kOCFAllocatorDefault,
          NULL,
          OF_STR("%@/%s"),
          root,
          "System/Library/PrivateFrameworks/CoreNLP.framework/CoreNLP"
      );
      OCFRelease(root);
      if (path == NULL) {
          return;
      }
      OCFIndex capacity =
          OCFStringGetMaximumSizeForEncoding(OCFStringGetLength(path), kOCFStringEncodingUTF8) + 1;
      char *buffer = malloc_type_malloc((size_t)capacity, 0x100004077774924);
      if (buffer == NULL) {
          OCFRelease(path);
          return;
      }
      if (!OCFStringGetCString(path, buffer, capacity, kOCFStringEncodingUTF8)) {
          free(buffer);
          OCFRelease(path);
          return;
      }
      OCFRelease(path);
      void *handle = dlopen(buffer, RTLD_LAZY | RTLD_LOCAL);
      if (handle != NULL) {
          __NLStringTokenizerCopyBestStringLanguage =
              dlsym(handle, "NLStringTokenizerCopyBestStringLanguage");
          __NLStringTokenizerCreate = dlsym(handle, "NLStringTokenizerCreate");
          __NLStringTokenizerSetString = dlsym(handle, "NLStringTokenizerSetString");
          __NLStringTokenizerGoToTokenAtIndex = dlsym(handle, "NLStringTokenizerGoToTokenAtIndex");
          __NLStringTokenizerAdvanceToNextToken = dlsym(handle, "NLStringTokenizerAdvanceToNextToken");
          __NLStringTokenizerGetCurrentTokenRange = dlsym(handle, "NLStringTokenizerGetCurrentTokenRange");
          __NLStringTokenizerCopyCurrentTokenAttribute =
              dlsym(handle, "NLStringTokenizerCopyCurrentTokenAttribute");
          __NLStringTokenizerGetCurrentSubTokens = dlsym(handle, "NLStringTokenizerGetCurrentSubTokens");
          __NLStringTokenizerCopyBestStringLanguageWithHints =
              dlsym(handle, "NLStringTokenizerCopyBestStringLanguageWithHints");
          __NLStringTokenizerCopyPossibleStringLanguages =
              dlsym(handle, "NLStringTokenizerCopyPossibleStringLanguages");
          __NLStringTokenizerTokenize = dlsym(handle, "NLStringTokenizerTokenize");
          __NLStringTokenizerTokenizeWithTranscriptions =
              dlsym(handle, "NLStringTokenizerTokenizeWithTranscriptions");
          __NLStringTokenizerTokenizeCompoundWord = dlsym(handle, "NLStringTokenizerTokenizeCompoundWord");
          __NLStringTokenizerGetDerivedTokens = dlsym(handle, "NLStringTokenizerGetDerivedTokens");
          __NLStringTokenizerGetNextBestTokens = dlsym(handle, "NLStringTokenizerGetNextBestTokens");
          __NLStringTokenizerCreateNamePartsArrayWithString =
              dlsym(handle, "NLStringTokenizerCreateNamePartsArrayWithString");
          __NLStringTokenizerCreateNamePartsArrayWithStringAsCompositeName =
              dlsym(handle, "NLStringTokenizerCreateNamePartsArrayWithStringAsCompositeName");
          __NLStringTokenizerCreatePossibleNamePartsArrayWithStringAsCompositeName =
              dlsym(handle, "NLStringTokenizerCreatePossibleNamePartsArrayWithStringAsCompositeName");
          __NLStringTokenizerGetCurrentTokenPartOfSpeech =
              dlsym(handle, "NLStringTokenizerGetCurrentTokenPartOfSpeech");
          __NLStringTokenizerSetLocale = dlsym(handle, "NLStringTokenizerSetLocale");
          __NLStringTokenizerSetTokenizerAttribute = dlsym(handle, "NLStringTokenizerSetTokenizerAttribute");
          __NLStringTokenizerGetTokenizerAttribute = dlsym(handle, "NLStringTokenizerGetTokenizerAttribute");
          __NLStringTokenizerSetStringWithOptions = dlsym(handle, "NLStringTokenizerSetStringWithOptions");
          coreNLPAvailable =
              __NLStringTokenizerCopyBestStringLanguage != NULL && __NLStringTokenizerCreate != NULL &&
              __NLStringTokenizerSetString != NULL && __NLStringTokenizerGoToTokenAtIndex != NULL &&
              __NLStringTokenizerAdvanceToNextToken != NULL &&
              __NLStringTokenizerGetCurrentTokenRange != NULL &&
              __NLStringTokenizerCopyCurrentTokenAttribute != NULL &&
              __NLStringTokenizerGetCurrentSubTokens != NULL &&
              __NLStringTokenizerCopyBestStringLanguageWithHints != NULL &&
              __NLStringTokenizerCopyPossibleStringLanguages != NULL && __NLStringTokenizerTokenize != NULL &&
              __NLStringTokenizerTokenizeWithTranscriptions != NULL &&
              __NLStringTokenizerTokenizeCompoundWord != NULL &&
              __NLStringTokenizerGetDerivedTokens != NULL && __NLStringTokenizerGetNextBestTokens != NULL &&
              __NLStringTokenizerCreateNamePartsArrayWithString != NULL &&
              __NLStringTokenizerCreateNamePartsArrayWithStringAsCompositeName != NULL &&
              __NLStringTokenizerCreatePossibleNamePartsArrayWithStringAsCompositeName != NULL &&
              __NLStringTokenizerGetCurrentTokenPartOfSpeech != NULL &&
              __NLStringTokenizerSetLocale != NULL && __NLStringTokenizerSetTokenizerAttribute != NULL &&
              __NLStringTokenizerGetTokenizerAttribute != NULL &&
              __NLStringTokenizerSetStringWithOptions != NULL;
      }
      free(buffer);
    });
    if (!coreNLPAvailable) {
        return NULL;
    }
    OCFStringTokenizerRef tokenizer =
        (OCFStringTokenizerRef)_OCFRuntimeCreateInstance(alloc, 40, sizeof(OCFTypeRef), NULL);
    if (tokenizer != NULL) {
        tokenizer->tokenizer = __NLStringTokenizerCreate(alloc, string, range, options, locale);
        if (tokenizer->tokenizer == NULL) {
            OCFRelease(tokenizer);
            return NULL;
        }
    }
    return tokenizer;
}

// Native CFStringTokenizerSetString, 0x1c022ef34. External.
void OCFStringTokenizerSetString(OCFStringTokenizerRef tokenizer, OCFStringRef string, OCFRange range) {
    __NLStringTokenizerSetString(tokenizer->tokenizer, string, range);
}

// Native CFStringTokenizerGoToTokenAtIndex, 0x1c022ef44. External.
OCFStringTokenizerTokenType
OCFStringTokenizerGoToTokenAtIndex(OCFStringTokenizerRef tokenizer, OCFIndex index) {
    return __NLStringTokenizerGoToTokenAtIndex(tokenizer->tokenizer, index);
}

// Native CFStringTokenizerAdvanceToNextToken, 0x1c022ef54. External.
OCFStringTokenizerTokenType OCFStringTokenizerAdvanceToNextToken(OCFStringTokenizerRef tokenizer) {
    return __NLStringTokenizerAdvanceToNextToken(tokenizer->tokenizer);
}

// Native CFStringTokenizerGetCurrentTokenRange, 0x1c022ef64. External.
OCFRange OCFStringTokenizerGetCurrentTokenRange(OCFStringTokenizerRef tokenizer) {
    return __NLStringTokenizerGetCurrentTokenRange(tokenizer->tokenizer);
}

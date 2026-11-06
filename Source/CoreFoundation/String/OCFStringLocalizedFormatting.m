//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/String/OCFStringLocalizedFormatting.h"
#import "CoreFoundation/PlugIn/OCFBundleLocalization.h"
#import "CoreFoundation/Locale/OCFNumberFormatter.h"
#import "CoreFoundation/Number/OCFNumber.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"
#import "CoreFoundation/Runtime/OCFRuntime.h"
#import "CoreFoundation/String/OCFStringFormat.h"
#import "CoreFoundation/String/OCFStringFormatConfiguration.h"
#import "CoreFoundation/Unicode/OCFUniChar.h"

#include <dispatch/dispatch.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: the functions from __CFStringGetInputIdentifierFormatterMappingFromDescriptor
// (0x1c02027f4) through ___CFStringApproximateSyllableCount_el_GR (0x1c020459c, ending at 0x1c02049a4) are
// contiguous in the iOS Simulator __text section between the stream units, with their data
// (__CFStringLocalizedFormattingInfo and the Greek vocative tables, 0x2227a4fe0-0x2227a50a8) contiguous in
// __DATA_CONST,__const; they are kept in one translation unit. The native source path is not in the evidence.
// The unit is Objective-C: it throws with objc_exception_throw from the native new_id_array pattern and
// __CFStringGetRelevantLocaleInfoFromLocaleSummary has a cleanup landing pad (free, then _Unwind_Resume).
// Unless a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified (assembly read in full). iOS Device 24A446: unread.
// Reading: Documentation/Reference/Research/CoreFoundationStringFormat.md.

// Constant objects of the image: NSConstantDictionary records (isa, options, count, keys, objects; 40 bytes,
// __DATA_CONST,__objc_dictobj) and NSConstantArray records (isa, count, objects; 24 bytes,
// __DATA_CONST,__objc_arrayobj). The record type names are local descriptive spellings.
typedef struct {
    uintptr_t isa;
    uint64_t options;
    uint64_t count;
    const void *const *keys;
    const void *const *objects;
} ConstantDictionary;

typedef struct {
    uintptr_t isa;
    uint64_t count;
    const void *const *objects;
} ConstantArray;

_Static_assert(sizeof(ConstantDictionary) == 40, "A constant dictionary is 40 bytes.");
_Static_assert(sizeof(ConstantArray) == 24, "A constant array is 24 bytes.");

extern char constantDictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSConstantDictionary));
extern char constantArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSConstantArray));
extern char exceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));

// Native constant dictionary 0x2227cde78 (__objc_dictobj) and the objects it reaches, generated from the
// image's records in native key order; every string is the image's literal (UTF-16 where the native literal
// is UTF-16).
static const OCFConstantString formattingString0 = ConstantASCIIString("NSStringFormatFamilyInfoKey");
static const OCFConstantString formattingString1 = ConstantASCIIString("NSStringFormatRuleInfoKey");
static const OCFConstantString formattingString2 = ConstantASCIIString("CJK");
static const OCFConstantString formattingString3 = ConstantASCIIString("ar");
static const OCFConstantString formattingString4 = ConstantASCIIString("he");
static const OCFConstantString formattingString5 = ConstantASCIIString("latin");
static const OCFConstantString formattingString6 = ConstantASCIIString("th");
static const OCFConstantString formattingString7 = ConstantUTF16String(
    u"[[:Hani:][:Hiragana:][:Katakana:][:Katakana_Or_Hiragana:][:Bopomofo:][:Hangul:]["
    u"\u02c9\u02ca\u02c7\u02cb\u02d9]]"
);
static const OCFConstantString formattingString8 = ConstantASCIIString("[[:Arabic:]]");
static const OCFConstantString formattingString9 = ConstantASCIIString("[[:Hebrew:]]");
static const OCFConstantString formattingString10 = ConstantASCIIString("[[:Latin:]]");
static const OCFConstantString formattingString11 = ConstantASCIIString("[[:Thai:]]");
static const void *const formattingInfoDictionary0Keys[5] = {
    &formattingString2, &formattingString3, &formattingString4, &formattingString5, &formattingString6
};
static const void *const formattingInfoDictionary0Objects[5] = {
    &formattingString7, &formattingString8, &formattingString9, &formattingString10, &formattingString11
};
static const ConstantDictionary formattingInfoDictionary0 = {
    (uintptr_t)constantDictionaryClass, 1, 5, formattingInfoDictionary0Keys, formattingInfoDictionary0Objects
}; // 0x2227cd9f0
static const OCFConstantString formattingString12 = ConstantASCIIString("TechnicalTerm");
static const OCFConstantString formattingString13 = ConstantASCIIString("VocativeName");
static const OCFConstantString formattingString14 = ConstantASCIIString("NSStringFormatClassInfoKey");
static const OCFConstantString formattingString15 = ConstantASCIIString("NSStringFormatLocaleInfoKey");
static const OCFConstantString formattingString16 = ConstantASCIIString("NSStringFormatSpecTypeKey");
static const OCFConstantString formattingString17 = ConstantASCIIString("NSStringFormatValueTypeKey");
static const OCFConstantString formattingString18 = ConstantASCIIString("default");
static const OCFConstantString formattingString19 = ConstantASCIIString("%@");
static const void *const formattingInfoDictionary1Keys[2] = {&formattingString18, &formattingString5};
static const void *const formattingInfoDictionary1Objects[2] = {&formattingString19, &formattingString19};
static const ConstantDictionary formattingInfoDictionary1 = {
    (uintptr_t)constantDictionaryClass, 1, 2, formattingInfoDictionary1Keys, formattingInfoDictionary1Objects
}; // 0x2227cda18
static const void *const formattingInfoDictionary2Keys[1] = {&formattingString18};
static const void *const formattingInfoDictionary2Objects[1] = {&formattingInfoDictionary1};
static const ConstantDictionary formattingInfoDictionary2 = {
    (uintptr_t)constantDictionaryClass, 1, 1, formattingInfoDictionary2Keys, formattingInfoDictionary2Objects
}; // 0x2227cda40
static const OCFConstantString formattingString20 = ConstantASCIIString("en");
static const OCFConstantString formattingString21 = ConstantASCIIString("fi");
static const OCFConstantString formattingString22 = ConstantASCIIString("ja");
static const OCFConstantString formattingString23 = ConstantASCIIString("zh-Hans");
static const OCFConstantString formattingString24 = ConstantASCIIString("zh-Hant");
static const OCFConstantString formattingString25 = ConstantASCIIString("NSStringFormatClassTypeKey");
static const void *const formattingInfoDictionary3Keys[1] = {&formattingString25};
static const void *const formattingInfoDictionary3Objects[1] = {&formattingString18};
static const ConstantDictionary formattingInfoDictionary3 = {
    (uintptr_t)constantDictionaryClass, 1, 1, formattingInfoDictionary3Keys, formattingInfoDictionary3Objects
}; // 0x2227cda68
static const OCFConstantString formattingString26 = ConstantASCIIString("NSStringFormatRuleStartEndInfoKey");
static const void *const formattingInfoDictionary4Keys[1] = {&formattingString18};
static const void *const formattingInfoDictionary4Objects[1] = {&formattingString19};
static const ConstantDictionary formattingInfoDictionary4 = {
    (uintptr_t)constantDictionaryClass, 1, 1, formattingInfoDictionary4Keys, formattingInfoDictionary4Objects
}; // 0x2227cda90
static const void *const formattingInfoDictionary5Keys[1] = {&formattingString18};
static const void *const formattingInfoDictionary5Objects[1] = {&formattingString19};
static const ConstantDictionary formattingInfoDictionary5 = {
    (uintptr_t)constantDictionaryClass, 1, 1, formattingInfoDictionary5Keys, formattingInfoDictionary5Objects
}; // 0x2227cdab8
static const void *const formattingInfoDictionary6Keys[1] = {&formattingString18};
static const void *const formattingInfoDictionary6Objects[1] = {&formattingInfoDictionary5};
static const ConstantDictionary formattingInfoDictionary6 = {
    (uintptr_t)constantDictionaryClass, 1, 1, formattingInfoDictionary6Keys, formattingInfoDictionary6Objects
}; // 0x2227cdae0
static const void *const formattingInfoDictionary7Keys[2] = {&formattingString1, &formattingString26};
static const void *const formattingInfoDictionary7Objects[2] = {
    &formattingInfoDictionary4, &formattingInfoDictionary6
};
static const ConstantDictionary formattingInfoDictionary7 = {
    (uintptr_t)constantDictionaryClass, 1, 2, formattingInfoDictionary7Keys, formattingInfoDictionary7Objects
}; // 0x2227cdb08
static const OCFConstantString formattingString27 =
    ConstantASCIIString("NSStringConditionalFormatRuleInfoKey");
static const OCFConstantString formattingString28 = ConstantASCIIString("NSStringConditionalFormatKey");
static const OCFConstantString formattingString29 = ConstantASCIIString("%@ ");
static const void *const formattingInfoDictionary8Keys[2] = {&formattingString28, &formattingString18};
static const void *const formattingInfoDictionary8Objects[2] = {&formattingString29, &formattingString19};
static const ConstantDictionary formattingInfoDictionary8 = {
    (uintptr_t)constantDictionaryClass, 1, 2, formattingInfoDictionary8Keys, formattingInfoDictionary8Objects
}; // 0x2227cdb30
static const void *const formattingInfoDictionary9Keys[1] = {&formattingString18};
static const void *const formattingInfoDictionary9Objects[1] = {&formattingInfoDictionary8};
static const ConstantDictionary formattingInfoDictionary9 = {
    (uintptr_t)constantDictionaryClass, 1, 1, formattingInfoDictionary9Keys, formattingInfoDictionary9Objects
}; // 0x2227cdb58
static const void *const formattingInfoDictionary10Keys[1] = {&formattingString27};
static const void *const formattingInfoDictionary10Objects[1] = {&formattingInfoDictionary9};
static const ConstantDictionary formattingInfoDictionary10 = {
    (uintptr_t)constantDictionaryClass,
    1,
    1,
    formattingInfoDictionary10Keys,
    formattingInfoDictionary10Objects
}; // 0x2227cdb80
static const OCFConstantString formattingString30 = ConstantUTF16String(u"\u201c%@\u201d");
static const void *const formattingInfoDictionary11Keys[3] = {
    &formattingString2, &formattingString18, &formattingString5
};
static const void *const formattingInfoDictionary11Objects[3] = {
    &formattingString30, &formattingString30, &formattingString19
};
static const ConstantDictionary formattingInfoDictionary11 = {
    (uintptr_t)constantDictionaryClass,
    1,
    3,
    formattingInfoDictionary11Keys,
    formattingInfoDictionary11Objects
}; // 0x2227cdba8
static const void *const formattingInfoDictionary12Keys[1] = {&formattingString1};
static const void *const formattingInfoDictionary12Objects[1] = {&formattingInfoDictionary11};
static const ConstantDictionary formattingInfoDictionary12 = {
    (uintptr_t)constantDictionaryClass,
    1,
    1,
    formattingInfoDictionary12Keys,
    formattingInfoDictionary12Objects
}; // 0x2227cdbd0
static const OCFConstantString formattingString31 = ConstantASCIIString(" %@ ");
static const OCFConstantString formattingString32 = ConstantASCIIString(" %@");
static const void *const formattingInfoDictionary13Keys[2] = {&formattingString18, &formattingString6};
static const void *const formattingInfoDictionary13Objects[2] = {&formattingString31, &formattingString32};
static const ConstantDictionary formattingInfoDictionary13 = {
    (uintptr_t)constantDictionaryClass,
    1,
    2,
    formattingInfoDictionary13Keys,
    formattingInfoDictionary13Objects
}; // 0x2227cdbf8
static const void *const formattingInfoDictionary14Keys[2] = {&formattingString18, &formattingString6};
static const void *const formattingInfoDictionary14Objects[2] = {&formattingString29, &formattingString19};
static const ConstantDictionary formattingInfoDictionary14 = {
    (uintptr_t)constantDictionaryClass,
    1,
    2,
    formattingInfoDictionary14Keys,
    formattingInfoDictionary14Objects
}; // 0x2227cdc20
static const void *const formattingInfoDictionary15Keys[2] = {&formattingString18, &formattingString6};
static const void *const formattingInfoDictionary15Objects[2] = {
    &formattingInfoDictionary13, &formattingInfoDictionary14
};
static const ConstantDictionary formattingInfoDictionary15 = {
    (uintptr_t)constantDictionaryClass,
    1,
    2,
    formattingInfoDictionary15Keys,
    formattingInfoDictionary15Objects
}; // 0x2227cdc48
static const void *const formattingInfoDictionary16Keys[1] = {&formattingString26};
static const void *const formattingInfoDictionary16Objects[1] = {&formattingInfoDictionary15};
static const ConstantDictionary formattingInfoDictionary16 = {
    (uintptr_t)constantDictionaryClass,
    1,
    1,
    formattingInfoDictionary16Keys,
    formattingInfoDictionary16Objects
}; // 0x2227cdc70
static const void *const formattingInfoDictionary17Keys[3] = {
    &formattingString2, &formattingString18, &formattingString5
};
static const void *const formattingInfoDictionary17Objects[3] = {
    &formattingString30, &formattingString30, &formattingString19
};
static const ConstantDictionary formattingInfoDictionary17 = {
    (uintptr_t)constantDictionaryClass,
    1,
    3,
    formattingInfoDictionary17Keys,
    formattingInfoDictionary17Objects
}; // 0x2227cdc98
static const void *const formattingInfoDictionary18Keys[1] = {&formattingString1};
static const void *const formattingInfoDictionary18Objects[1] = {&formattingInfoDictionary17};
static const ConstantDictionary formattingInfoDictionary18 = {
    (uintptr_t)constantDictionaryClass,
    1,
    1,
    formattingInfoDictionary18Keys,
    formattingInfoDictionary18Objects
}; // 0x2227cdcc0
static const OCFConstantString formattingString33 = ConstantUTF16String(u"\u300c%@\u300d");
static const void *const formattingInfoDictionary19Keys[3] = {
    &formattingString2, &formattingString18, &formattingString5
};
static const void *const formattingInfoDictionary19Objects[3] = {
    &formattingString33, &formattingString33, &formattingString19
};
static const ConstantDictionary formattingInfoDictionary19 = {
    (uintptr_t)constantDictionaryClass,
    1,
    3,
    formattingInfoDictionary19Keys,
    formattingInfoDictionary19Objects
}; // 0x2227cdce8
static const void *const formattingInfoDictionary20Keys[1] = {&formattingString1};
static const void *const formattingInfoDictionary20Objects[1] = {&formattingInfoDictionary19};
static const ConstantDictionary formattingInfoDictionary20 = {
    (uintptr_t)constantDictionaryClass,
    1,
    1,
    formattingInfoDictionary20Keys,
    formattingInfoDictionary20Objects
}; // 0x2227cdd10
static const void *const formattingInfoDictionary21Keys[7] = {
    &formattingString18,
    &formattingString20,
    &formattingString21,
    &formattingString22,
    &formattingString6,
    &formattingString23,
    &formattingString24
};
static const void *const formattingInfoDictionary21Objects[7] = {
    &formattingInfoDictionary3,
    &formattingInfoDictionary7,
    &formattingInfoDictionary10,
    &formattingInfoDictionary12,
    &formattingInfoDictionary16,
    &formattingInfoDictionary18,
    &formattingInfoDictionary20
};
static const ConstantDictionary formattingInfoDictionary21 = {
    (uintptr_t)constantDictionaryClass,
    1,
    7,
    formattingInfoDictionary21Keys,
    formattingInfoDictionary21Objects
}; // 0x2227cdd38
static const OCFConstantString formattingString34 = ConstantASCIIString("NSStringEntityFormattingRuleType");
static const OCFConstantString formattingString35 = ConstantASCIIString("@");
static const void *const formattingInfoDictionary22Keys[4] = {
    &formattingString14, &formattingString15, &formattingString16, &formattingString17
};
static const void *const formattingInfoDictionary22Objects[4] = {
    &formattingInfoDictionary2, &formattingInfoDictionary21, &formattingString34, &formattingString35
};
static const ConstantDictionary formattingInfoDictionary22 = {
    (uintptr_t)constantDictionaryClass,
    1,
    4,
    formattingInfoDictionary22Keys,
    formattingInfoDictionary22Objects
}; // 0x2227cdd60
static const void *const formattingInfoDictionary23Keys[1] = {&formattingString18};
static const void *const formattingInfoDictionary23Objects[1] = {&formattingString19};
static const ConstantDictionary formattingInfoDictionary23 = {
    (uintptr_t)constantDictionaryClass,
    1,
    1,
    formattingInfoDictionary23Keys,
    formattingInfoDictionary23Objects
}; // 0x2227cdd88
static const void *const formattingInfoDictionary24Keys[1] = {&formattingString18};
static const void *const formattingInfoDictionary24Objects[1] = {&formattingInfoDictionary23};
static const ConstantDictionary formattingInfoDictionary24 = {
    (uintptr_t)constantDictionaryClass,
    1,
    1,
    formattingInfoDictionary24Keys,
    formattingInfoDictionary24Objects
}; // 0x2227cddb0
static const void *const formattingInfoDictionary25Keys[1] = {&formattingString25};
static const void *const formattingInfoDictionary25Objects[1] = {&formattingString18};
static const ConstantDictionary formattingInfoDictionary25 = {
    (uintptr_t)constantDictionaryClass,
    1,
    1,
    formattingInfoDictionary25Keys,
    formattingInfoDictionary25Objects
}; // 0x2227cddd8
static const void *const formattingInfoDictionary26Keys[1] = {&formattingString18};
static const void *const formattingInfoDictionary26Objects[1] = {&formattingInfoDictionary25};
static const ConstantDictionary formattingInfoDictionary26 = {
    (uintptr_t)constantDictionaryClass,
    1,
    1,
    formattingInfoDictionary26Keys,
    formattingInfoDictionary26Objects
}; // 0x2227cde00
static const OCFConstantString formattingString36 =
    ConstantASCIIString("NSStringVocativeNameFormattingRuleType");
static const void *const formattingInfoDictionary27Keys[4] = {
    &formattingString14, &formattingString15, &formattingString16, &formattingString17
};
static const void *const formattingInfoDictionary27Objects[4] = {
    &formattingInfoDictionary24, &formattingInfoDictionary26, &formattingString36, &formattingString35
};
static const ConstantDictionary formattingInfoDictionary27 = {
    (uintptr_t)constantDictionaryClass,
    1,
    4,
    formattingInfoDictionary27Keys,
    formattingInfoDictionary27Objects
}; // 0x2227cde28
static const void *const formattingInfoDictionary28Keys[2] = {&formattingString12, &formattingString13};
static const void *const formattingInfoDictionary28Objects[2] = {
    &formattingInfoDictionary22, &formattingInfoDictionary27
};
static const ConstantDictionary formattingInfoDictionary28 = {
    (uintptr_t)constantDictionaryClass,
    1,
    2,
    formattingInfoDictionary28Keys,
    formattingInfoDictionary28Objects
}; // 0x2227cde50
static const void *const formattingInfoDictionary29Keys[2] = {&formattingString0, &formattingString1};
static const void *const formattingInfoDictionary29Objects[2] = {
    &formattingInfoDictionary0, &formattingInfoDictionary28
};
static const ConstantDictionary formattingInfoDictionary29 = {
    (uintptr_t)constantDictionaryClass,
    1,
    2,
    formattingInfoDictionary29Keys,
    formattingInfoDictionary29Objects
}; // 0x2227cde78
// Native __CFStringLocalizedFormattingInfo, 0x2227a4fe0 (__DATA_CONST,__const), the pointer to the constant
// dictionary 0x2227cde78. Hidden (was a private external).
const OCFDictionaryRef _CFStringLocalizedFormattingInfo = (OCFDictionaryRef)&formattingInfoDictionary29;

// Native __CFStringGetInputIdentifierFormatterMappingFromDescriptor.localizedStringFormattingFamilyInfo and
// .loc_formatting_family_info_init (__DATA,__bss). Non-external.
static OCFMutableDictionaryRef localizedStringFormattingFamilyInfo;
static dispatch_once_t loc_formatting_family_info_init;

static bool __CFStringCopyCharSetWithPattern(OCFStringRef pattern, OCFMutableCharacterSetRef *characterSet);

// ICU (libicucore, linked by the native image); supplied, never reimplemented. The SDK installs no uset.h, so
// the two entry points are declared here.
extern void *uset_openPattern(const UniChar *pattern, int32_t patternLength, int32_t *status);
extern void uset_close(void *set);

// Native __CFStringGetInputIdentifierFormatterMappingFromDescriptor, 0x1c02027f4, with
// ____CFStringGetInputIdentifierFormatterMappingFromDescriptor_block_invoke (0x1c0202894, the stack block
// that captures descriptor). Hidden (was a private external).
// Once per process: a mutable dictionary (system default allocator, CFType callbacks, capacity the
// descriptor's count) maps each descriptor key to the character set its pattern value compiles to; patterns
// that fail are skipped. The keys and values are fetched into one temporary buffer of 2 * count objects (the
// native new_id_array pattern: an exception for 2^60 or more, a stack buffer up to 256, _CFCreateArrayStorage
// above). Later calls return the first dictionary whatever descriptor they pass.
OCFMutableDictionaryRef
_CFStringGetInputIdentifierFormatterMappingFromDescriptor(OCFDictionaryRef descriptor) {
    dispatch_once(&loc_formatting_family_info_init, ^{
      localizedStringFormattingFamilyInfo = OCFDictionaryCreateMutable(
          kOCFAllocatorSystemDefault,
          OCFDictionaryGetCount(descriptor),
          &kOCFTypeDictionaryKeyCallBacks,
          &kOCFTypeDictionaryValueCallBacks
      );
      OCFIndex count = OCFDictionaryGetCount(descriptor);
      size_t bufferCount = (size_t)count * 2;
      if (((size_t)count >> 59 & 0xf) != 0) {
          OCFStringRef reason = OCFStringCreateWithFormat(
              NULL,
              NULL,
              OF_STR(
                  "*** attempt to create a temporary id buffer which is too large or with a negative count "
                  "(%lu) -- possibly data is corrupt"
              ),
              bufferCount
          );
          id exception = ((id (*)(id, SEL, id, id, id))objc_msgSend)(
              (id)exceptionClass,
              sel_getUid("exceptionWithName:reason:userInfo:"),
              (id)ONSGenericException,
              (id)reason,
              nil
          );
          OCFRelease(reason);
          objc_exception_throw(exception);
      }
      size_t used = bufferCount <= 0x100 ? (count != 0 ? bufferCount : 1) : 1;
      const void *stackBuffer[used];
      size_t allocatedSize = 0;
      const void **keys = stackBuffer;
      void *allocated = NULL;
      if (bufferCount > 0x100) {
          keys = _OCFCreateArrayStorage(count != 0 ? bufferCount : 1, false, &allocatedSize);
          allocated = keys;
      }
      OCFDictionaryGetKeysAndValues(descriptor, keys, keys + count);
      for (OCFIndex index = 0; index < count; ++index) {
          const void *key = keys[index];
          OCFMutableCharacterSetRef characterSet = NULL;
          bool compiled = __CFStringCopyCharSetWithPattern((OCFStringRef)keys[index + count], &characterSet);
          if (characterSet != NULL && compiled) {
              OCFDictionarySetValue(localizedStringFormattingFamilyInfo, key, characterSet);
              OCFRelease(characterSet);
          }
      }
      free(allocated);
    });
    return localizedStringFormattingFamilyInfo;
}

// Native ___CFStringCopyCharSetWithPattern, 0x1c0202a80. Non-external.
// Compiles the pattern with uset_openPattern (the string's own UTF-16 buffer, or a stack copy) and converts
// it with _CFCreateCharacterSetFromUSet; the ICU set is closed only on success. An ICU error (status > 0)
// answers false and leaves *characterSet untouched; otherwise the answer is whether a set was created.
static bool __CFStringCopyCharSetWithPattern(OCFStringRef pattern, OCFMutableCharacterSetRef *characterSet) {
    int32_t status = 0;
    OCFIndex length = OCFStringGetLength(pattern);
    const UniChar *characters = OCFStringGetCharactersPtr(pattern);
    void *set;
    if (characters != NULL) {
        set = uset_openPattern(characters, (int32_t)length, &status);
    } else {
        UniChar buffer[length];
        OCFStringGetCharacters(pattern, (OCFRange){0, length}, buffer);
        set = uset_openPattern(buffer, (int32_t)length, &status);
    }
    if (status > 0) {
        return false;
    }
    *characterSet = _OCFCreateCharacterSetFromUSet(set);
    uset_close(set);
    return *characterSet != NULL;
}

static void freeTemporaryBuffer(void **buffer) {
    free(*buffer);
}

// Native __CFStringGetRelevantLocaleInfoFromLocaleSummary, 0x1c0202b8c. Hidden (was a private external).
// The summary's keys become a mutable array (capacity the count; NULL answers NULL at once) that
// CFBundleCopyLocalizationsForPreferences matches against the locale's identifier; the first preferred
// localization's entry is the answer, or the "default" entry when it has none. With no preferred
// localization the answer is NULL. The keys buffer follows the native new_id_array pattern (count >= 2^60
// throws) and is freed by a cleanup, also when an exception unwinds through the calls that use it.
const void *
_CFStringGetRelevantLocaleInfoFromLocaleSummary(OCFDictionaryRef localeSummary, OCFLocaleRef locale) {
    const void *value = NULL;
    OCFIndex count = OCFDictionaryGetCount(localeSummary);
    OCFMutableArrayRef keyArray =
        OCFArrayCreateMutable(kOCFAllocatorSystemDefault, count, &kOCFTypeArrayCallBacks);
    if (keyArray == NULL) {
        return value;
    }
    if ((size_t)count >> 60 != 0) {
        OCFStringRef reason = OCFStringCreateWithFormat(
            NULL,
            NULL,
            OF_STR(
                "*** attempt to create a temporary id buffer which is too large or with a negative count "
                "(%lu) -- possibly data is corrupt"
            ),
            (size_t)count
        );
        id exception = ((id (*)(id, SEL, id, id, id))objc_msgSend)(
            (id)exceptionClass,
            sel_getUid("exceptionWithName:reason:userInfo:"),
            (id)ONSGenericException,
            (id)reason,
            nil
        );
        OCFRelease(reason);
        objc_exception_throw(exception);
    }
    OCFArrayRef preferences;
    OCFArrayRef localizations;
    OCFArrayRef preferred;
    bool found = false;
    const void *bestLocalization = NULL;
    {
        size_t used = (size_t)count <= 0x100 ? ((size_t)count > 1 ? (size_t)count : 1) : 1;
        const void *stackBuffer[used];
        size_t allocatedSize = 0;
        const void **keys = stackBuffer;
        void *allocated __attribute__((cleanup(freeTemporaryBuffer))) = NULL;
        if ((size_t)count > 0x100) {
            keys = _OCFCreateArrayStorage((size_t)count > 1 ? (size_t)count : 1, false, &allocatedSize);
            allocated = keys;
        }
        OCFDictionaryGetKeysAndValues(localeSummary, keys, NULL);
        for (OCFIndex remaining = count; remaining != 0; --remaining) {
            OCFArrayAppendValue(keyArray, *keys++);
        }
        const void *identifier = OCFLocaleGetIdentifier(locale);
        preferences = OCFArrayCreate(kOCFAllocatorSystemDefault, &identifier, 1, &kOCFTypeArrayCallBacks);
        localizations = OCFBundleCopyLocalizationsForPreferences(keyArray, preferences);
        preferred = OCFBundleCopyPreferredLocalizationsFromArray(localizations);
        if (OCFArrayGetCount(preferred) > 0) {
            bestLocalization = OCFArrayGetValueAtIndex(preferred, 0);
            found = true;
        }
    }
    if (found && !OCFDictionaryGetValueIfPresent(localeSummary, bestLocalization, &value)) {
        value = OCFDictionaryGetValue(localeSummary, kOCFStringFormatDefaultValueKey);
    }
    if (preferred != NULL) {
        OCFRelease(preferred);
    }
    if (localizations != NULL) {
        OCFRelease(localizations);
    }
    OCFRelease(keyArray);
    if (preferences != NULL) {
        OCFRelease(preferences);
    }
    return value;
}

// Native __CFStringContentsInCharacterSet, 0x1c0202e00. Hidden (was a private external).
// True for a NULL string or set, or when the set cannot be copied; otherwise true when no character of the
// string lies outside the set with the space character added (the inverted copy minus " ").
bool _CFStringContentsInCharacterSet(OCFStringRef string, OCFCharacterSetRef characterSet) {
    bool contained = true;
    if (string != NULL && characterSet != NULL) {
        OCFMutableCharacterSetRef outside =
            OCFCharacterSetCreateMutableCopy(kOCFAllocatorDefault, characterSet);
        if (outside != NULL) {
            OCFCharacterSetInvert(outside);
            OCFCharacterSetRemoveCharactersInString(outside, OF_STR(" "));
            OCFRange result = {-1, 0};
            Boolean found = OCFStringFindCharacterFromSet(
                string, outside, (OCFRange){0, OCFStringGetLength(string)}, 0, &result
            );
            contained = !found && result.location == -1;
            OCFRelease(outside);
        }
    }
    return contained;
}

// Native __CFStringConditionalFormattingMeetsCondition, 0x1c0202edc. Hidden (was a private external).
// Only Finnish ("fi") has a condition: the string contains a whitespace character (predefined set 2).
bool _CFStringConditionalFormattingMeetsCondition(OCFStringRef string, OCFLocaleRef locale) {
    OCFTypeRef language = OCFLocaleGetValue(locale, kOCFLocaleLanguageCodeKey);
    if (language == NULL) {
        return false;
    }
    if (!OCFEqual(language, OF_STR("fi"))) {
        return false;
    }
    OCFCharacterSetRef whitespace = OCFCharacterSetGetPredefined(2);
    OCFRange result = {-1, 0};
    return OCFStringFindCharacterFromSet(
        string, whitespace, (OCFRange){0, OCFStringGetLength(string)}, 0, &result
    );
}

static OCFStringRef _CFStringCopyVocativeCaseOfGivenName_cs(OCFStringRef givenName, OCFLocaleRef locale);
static OCFStringRef _CFStringCopyVocativeCaseOfGivenName_el(OCFStringRef givenName, OCFLocaleRef locale);
static OCFStringRef _CFStringCopyVocativeCaseOfGivenName_uk(OCFStringRef givenName, OCFLocaleRef locale);
static OCFStringRef _CFStringCopyVocativeCaseOfGivenName_lt(OCFStringRef givenName, OCFLocaleRef locale);
static bool __CFStringHasOneOfSuffixes(OCFStringRef string, ...);
static OCFIndex __CFStringApproximateSyllableCount_el_GR(OCFStringRef string);

// Native __CFStringCopyVocativeCaseOfGivenName, 0x1c0202f9c. External.
// Dispatches on the locale's language code (cs, el, uk, lt, compared in that order); any other language, or
// none, answers the given name retained.
OCFStringRef _OCFStringCopyVocativeCaseOfGivenName(OCFStringRef givenName, OCFLocaleRef locale) {
    OCFTypeRef language = OCFLocaleGetValue(locale, kOCFLocaleLanguageCodeKey);
    if (language != NULL) {
        if (OCFEqual(language, OF_STR("cs"))) {
            return _CFStringCopyVocativeCaseOfGivenName_cs(givenName, locale);
        }
        if (OCFEqual(language, OF_STR("el"))) {
            return _CFStringCopyVocativeCaseOfGivenName_el(givenName, locale);
        }
        if (OCFEqual(language, OF_STR("uk"))) {
            return _CFStringCopyVocativeCaseOfGivenName_uk(givenName, locale);
        }
        if (OCFEqual(language, OF_STR("lt"))) {
            return _CFStringCopyVocativeCaseOfGivenName_lt(givenName, locale);
        }
    }
    return OCFRetain(givenName);
}

// The native crash pattern of these functions: the function name, ": " and the message are copied into a
// stack buffer of exactly their combined size (strlcpy, strlcat, strlcat), whose address is stored as the
// crash message before BRK #1.
#define haltWithFunctionMessage(function, message)                                                           \
    do {                                                                                                     \
        char haltBuffer[sizeof(function) + 2 + sizeof(message) - 1];                                         \
        strlcpy(haltBuffer, function, sizeof(haltBuffer));                                                   \
        strlcat(haltBuffer, ": ", sizeof(haltBuffer));                                                       \
        strlcat(haltBuffer, message, sizeof(haltBuffer));                                                    \
        crashWithMessage(haltBuffer);                                                                        \
    } while (0)

// The native __CFStringReplaceLastNCharactersWithSuffix, inlined at every use in the four vocative functions
// (no native symbol): a string shorter than count halts; otherwise the last count characters are replaced by
// suffix, or deleted when suffix is NULL (the Greek removal of a final sigma). The helper name is a local
// descriptive spelling.
static inline void
replaceLastCharactersWithSuffix(OCFMutableStringRef string, OCFIndex count, OCFStringRef suffix) {
    OCFIndex length = OCFStringGetLength(string);
    if (length < count) {
        haltWithFunctionMessage("__CFStringReplaceLastNCharactersWithSuffix", "Invalid character count");
    }
    if (suffix != NULL) {
        OCFStringReplace(string, (OCFRange){length - count, count}, suffix);
    } else {
        OCFStringDelete(string, (OCFRange){length - count, count});
    }
}

// Native constant array 0x2227ce238 (__objc_arrayobj): Czech given names that keep their nominative form,
// sorted for CFArrayBSearchValues.
static const OCFConstantString formattingString37 = ConstantASCIIString("alyson");
static const OCFConstantString formattingString38 = ConstantASCIIString("astrid");
static const OCFConstantString formattingString39 = ConstantASCIIString("carmen");
static const OCFConstantString formattingString40 = ConstantASCIIString("dagmar");
static const OCFConstantString formattingString41 = ConstantASCIIString("elen");
static const OCFConstantString formattingString42 = ConstantASCIIString("ellen");
static const OCFConstantString formattingString43 = ConstantASCIIString("ester");
static const OCFConstantString formattingString44 = ConstantASCIIString("etel");
static const OCFConstantString formattingString45 = ConstantASCIIString("ingrid");
static const OCFConstantString formattingString46 = ConstantASCIIString("iris");
static const OCFConstantString formattingString47 = ConstantASCIIString("jasmin");
static const OCFConstantString formattingString48 = ConstantASCIIString("karmen");
static const OCFConstantString formattingString49 = ConstantASCIIString("marlen");
static const OCFConstantString formattingString50 = ConstantASCIIString("miriam");
static const OCFConstantString formattingString51 = ConstantASCIIString("nikol");
static const OCFConstantString formattingString52 = ConstantASCIIString("noemi");
static const void *const czechNamesArray30Objects[16] = {
    &formattingString37,
    &formattingString38,
    &formattingString39,
    &formattingString40,
    &formattingString41,
    &formattingString42,
    &formattingString43,
    &formattingString44,
    &formattingString45,
    &formattingString46,
    &formattingString47,
    &formattingString48,
    &formattingString49,
    &formattingString50,
    &formattingString51,
    &formattingString52
};
static const ConstantArray czechNamesArray30 = {
    (uintptr_t)constantArrayClass, 16, czechNamesArray30Objects
}; // 0x2227ce238

// Native _CFStringCopyVocativeCaseOfGivenName_cs, 0x1c0203090. Non-external.
// The lowercased copy (CFStringLowercase with the locale) selects the rule; every rule first restores the
// original spelling (CFStringReplaceAll) and edits that. Names found in the sorted exception array answer the
// given name. A failed copy halts with "Allocation failure". The suffix lists are passed in native order.
static OCFStringRef _CFStringCopyVocativeCaseOfGivenName_cs(OCFStringRef givenName, OCFLocaleRef locale) {
    OCFMutableStringRef name = OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, 0, givenName);
    if (name == NULL) {
        haltWithFunctionMessage("_CFStringCopyVocativeCaseOfGivenName_cs", "Allocation failure");
    }
    OCFStringLowercase(name, locale);
    OCFArrayRef exceptions = (OCFArrayRef)&czechNamesArray30;
    OCFIndex count = OCFArrayGetCount(exceptions);
    OCFIndex index = OCFArrayBSearchValues(
        exceptions, (OCFRange){0, count}, name, (OCFComparatorFunction)(void *)OCFStringCompare, NULL
    );
    OCFStringRef result;
    if (index < count &&
        OCFStringCompare(name, OCFArrayGetValueAtIndex(exceptions, index), 0) == kOCFCompareEqualTo) {
        result = OCFRetain(givenName);
        OCFRelease(name);
        return result != NULL ? result : OCFRetain(givenName);
    }
    UniChar character;
    if (OCFStringCompare(name, OF_STR("seth"), 0) == kOCFCompareEqualTo) {
        OCFIndex lowercasedLength = OCFStringGetLength(name);
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, lowercasedLength - 1, OF_STR("ethe"));
    } else if (OCFStringHasSuffix(name, OF_STR("a"))) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 1, OF_STR("o"));
    } else if (
        __CFStringHasOneOfSuffixes(
            name, OF_STR("h"), OF_STR("g"), OF_STR("ik"), OF_USTR(u"ík"), OF_STR("ck"), OF_STR("uk"), NULL
        )
    ) {
        OCFStringReplaceAll(name, givenName);
        character = 'u';
        OCFStringAppendCharacters(name, &character, 1);
    } else if (OCFStringHasSuffix(name, OF_STR("nec"))) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 2, OF_USTR(u"če"));
    } else if (OCFStringHasSuffix(name, OF_STR("ius"))) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 2, OF_STR("e"));
    } else if (OCFStringHasSuffix(name, OF_STR("uce"))) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 1, OF_STR("i"));
    } else if (
        __CFStringHasOneOfSuffixes(name, OF_STR("vel"), OF_STR("rel"), OF_STR("bel"), OF_STR("xel"), NULL)
    ) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 2, OF_STR("le"));
    } else if (OCFStringHasSuffix(name, OF_STR("les"))) {
        // No length check: the native code deletes the last character directly.
        OCFStringReplaceAll(name, givenName);
        OCFStringDelete(name, (OCFRange){OCFStringGetLength(name) - 1, 1});
    } else if (
        __CFStringHasOneOfSuffixes(
            name,
            OF_STR("s"),
            OF_USTR(u"š"),
            OF_STR("z"),
            OF_USTR(u"ž"),
            OF_STR("el"),
            OF_STR("j"),
            OF_STR("ex"),
            OF_STR("c"),
            OF_STR("lix"),
            OF_USTR(u"oř"),
            NULL
        )
    ) {
        OCFStringReplaceAll(name, givenName);
        character = 'i';
        OCFStringAppendCharacters(name, &character, 1);
    } else if (OCFStringHasSuffix(name, OF_USTR(u"děk"))) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 3, OF_USTR(u"ďku"));
    } else if (OCFStringHasSuffix(name, OF_USTR(u"něk"))) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 3, OF_USTR(u"ňku"));
    } else if (OCFStringHasSuffix(name, OF_STR("ek"))) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 2, OF_STR("ku"));
    } else if (OCFStringHasSuffix(name, OF_STR("ter"))) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 2, OF_USTR(u"ře"));
    } else if (OCFStringHasSuffix(name, OF_STR("tr"))) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 1, OF_USTR(u"ře"));
    } else if (
        __CFStringHasOneOfSuffixes(
            name,
            OF_STR("h"),
            OF_STR("ch"),
            OF_STR("k"),
            OF_STR("r"),
            OF_STR("d"),
            OF_STR("t"),
            OF_STR("n"),
            OF_STR("v"),
            OF_STR("m"),
            OF_STR("l"),
            OF_STR("p"),
            OF_STR("f"),
            OF_STR("b"),
            NULL
        )
    ) {
        OCFStringReplaceAll(name, givenName);
        character = 'e';
        OCFStringAppendCharacters(name, &character, 1);
    } else {
        OCFRelease(name);
        return OCFRetain(givenName);
    }
    result = OCFRetain(name);
    OCFRelease(name);
    if (result == NULL) {
        return OCFRetain(givenName);
    }
    return result;
}

// Native __CFStringCopyVocativeCaseOfGivenName_el.keys and .values, 0x2227a4fe8 and 0x2227a5048
// (__DATA_CONST,__const, twelve string pointers each). Non-external.
static const OCFConstantString formattingString53 =
    ConstantUTF16String(u"\u03a3\u03c0\u03c5\u03c1\u03af\u03b4\u03c9\u03bd");
static const OCFConstantString formattingString54 =
    ConstantUTF16String(u"\u03a3\u03a0\u03a5\u03a1\u0399\u0394\u03a9\u039d");
static const OCFConstantString formattingString55 =
    ConstantUTF16String(u"\u03c3\u03c0\u03c5\u03c1\u03af\u03b4\u03c9\u03bd");
static const OCFConstantString formattingString56 = ConstantUTF16String(u"\u039a\u03bb\u03ad\u03c9\u03bd");
static const OCFConstantString formattingString57 = ConstantUTF16String(u"\u039a\u039b\u0395\u03a9\u039d");
static const OCFConstantString formattingString58 = ConstantUTF16String(u"\u03ba\u03bb\u03ad\u03c9\u03bd");
static const OCFConstantString formattingString59 = ConstantUTF16String(u"\u0399\u03ac\u03c3\u03c9\u03bd");
static const OCFConstantString formattingString60 = ConstantUTF16String(u"\u0399\u0391\u03a3\u03a9\u039d");
static const OCFConstantString formattingString61 = ConstantUTF16String(u"\u03b9\u03ac\u03c3\u03c9\u03bd");
static const OCFConstantString formattingString62 =
    ConstantUTF16String(u"\u03a4\u03c1\u03cd\u03c6\u03c9\u03bd");
static const OCFConstantString formattingString63 =
    ConstantUTF16String(u"\u03a4\u03a1\u03a5\u03a6\u03a9\u039d");
static const OCFConstantString formattingString64 =
    ConstantUTF16String(u"\u03c4\u03c1\u03cd\u03c6\u03c9\u03bd");
static const OCFStringRef __CFStringCopyVocativeCaseOfGivenName_el_keys[12] = {
    (OCFStringRef)&formattingString53,
    (OCFStringRef)&formattingString54,
    (OCFStringRef)&formattingString55,
    (OCFStringRef)&formattingString56,
    (OCFStringRef)&formattingString57,
    (OCFStringRef)&formattingString58,
    (OCFStringRef)&formattingString59,
    (OCFStringRef)&formattingString60,
    (OCFStringRef)&formattingString61,
    (OCFStringRef)&formattingString62,
    (OCFStringRef)&formattingString63,
    (OCFStringRef)&formattingString64
};
static const OCFConstantString formattingString65 =
    ConstantUTF16String(u"\u03a3\u03c0\u03c5\u03c1\u03af\u03b4\u03c9\u03bd\u03b1");
static const OCFConstantString formattingString66 =
    ConstantUTF16String(u"\u03a3\u03a0\u03a5\u03a1\u0399\u0394\u03a9\u039d\u0391");
static const OCFConstantString formattingString67 =
    ConstantUTF16String(u"\u03c3\u03c0\u03c5\u03c1\u03af\u03b4\u03c9\u03bd\u03b1");
static const OCFConstantString formattingString68 =
    ConstantUTF16String(u"\u039a\u03bb\u03ad\u03c9\u03bd\u03b1");
static const OCFConstantString formattingString69 =
    ConstantUTF16String(u"\u039a\u039b\u0395\u03a9\u039d\u0391");
static const OCFConstantString formattingString70 =
    ConstantUTF16String(u"\u03ba\u03bb\u03ad\u03c9\u03bd\u03b1");
static const OCFConstantString formattingString71 =
    ConstantUTF16String(u"\u0399\u03ac\u03c3\u03bf\u03bd\u03b1");
static const OCFConstantString formattingString72 =
    ConstantUTF16String(u"\u0399\u0391\u03a3\u039f\u039d\u0391");
static const OCFConstantString formattingString73 =
    ConstantUTF16String(u"\u03b9\u03ac\u03c3\u03bf\u03bd\u03b1");
static const OCFConstantString formattingString74 =
    ConstantUTF16String(u"\u03a4\u03c1\u03cd\u03c6\u03c9\u03bd\u03b1");
static const OCFConstantString formattingString75 =
    ConstantUTF16String(u"\u03a4\u03a1\u03a5\u03a6\u03a9\u039d\u0391");
static const OCFConstantString formattingString76 =
    ConstantUTF16String(u"\u03c4\u03c1\u03cd\u03c6\u03c9\u03bd\u03b1");
static const OCFStringRef __CFStringCopyVocativeCaseOfGivenName_el_values[12] = {
    (OCFStringRef)&formattingString65,
    (OCFStringRef)&formattingString66,
    (OCFStringRef)&formattingString67,
    (OCFStringRef)&formattingString68,
    (OCFStringRef)&formattingString69,
    (OCFStringRef)&formattingString70,
    (OCFStringRef)&formattingString71,
    (OCFStringRef)&formattingString72,
    (OCFStringRef)&formattingString73,
    (OCFStringRef)&formattingString74,
    (OCFStringRef)&formattingString75,
    (OCFStringRef)&formattingString76
};

// Native _CFStringCopyVocativeCaseOfGivenName_el, 0x1c02037ac. Non-external.
// The twelve irregular names (compared with options 0 before any copy) answer their vocative retained. The
// other rules use the lowercased copy, as in the Czech function, with the syllable estimate deciding between
// the -ος and -ων endings.
static OCFStringRef _CFStringCopyVocativeCaseOfGivenName_el(OCFStringRef givenName, OCFLocaleRef locale) {
    for (OCFIndex index = 0; index < 12; ++index) {
        if (OCFStringCompare(givenName, __CFStringCopyVocativeCaseOfGivenName_el_keys[index], 0) ==
            kOCFCompareEqualTo) {
            return OCFRetain(__CFStringCopyVocativeCaseOfGivenName_el_values[index]);
        }
    }
    OCFMutableStringRef name = OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, 0, givenName);
    if (name == NULL) {
        haltWithFunctionMessage("_CFStringCopyVocativeCaseOfGivenName_el", "Allocation failure");
    }
    OCFStringLowercase(name, locale);
    if (__CFStringHasOneOfSuffixes(
            name, OF_USTR(u"ης"), OF_USTR(u"ής"), OF_USTR(u"ας"), OF_USTR(u"άς"), NULL
        )) {
        goto removeFinalSigma;
    }
    if (OCFStringHasSuffix(name, OF_USTR(u"ιος"))) {
        goto replaceWithEpsilon;
    }
    if (OCFStringHasSuffix(name, OF_USTR(u"ός"))) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 2, OF_USTR(u"έ"));
        goto done;
    }
    if (OCFStringHasSuffix(name, OF_USTR(u"ος"))) {
        OCFIndex syllables = __CFStringApproximateSyllableCount_el_GR(name);
        if (syllables == 2) {
            goto removeFinalSigma;
        }
        if (syllables >= 3) {
            goto replaceWithEpsilon;
        }
        goto unchanged;
    }
    if (__CFStringHasOneOfSuffixes(name, OF_USTR(u"ων"), OF_USTR(u"ών"), NULL)) {
        OCFIndex syllables = __CFStringApproximateSyllableCount_el_GR(name);
        if (syllables == 4 || syllables == 2) {
            OCFStringReplaceAll(name, givenName);
            replaceLastCharactersWithSuffix(name, 2, OF_USTR(u"ονα"));
            goto done;
        }
        if (syllables == 3) {
            OCFStringReplaceAll(name, givenName);
            OCFStringAppend(name, OF_USTR(u"τα"));
            goto done;
        }
    }
unchanged:
    OCFRelease(name);
    return OCFRetain(givenName);
removeFinalSigma:
    OCFStringReplaceAll(name, givenName);
    replaceLastCharactersWithSuffix(name, 1, NULL);
    goto done;
replaceWithEpsilon:
    OCFStringReplaceAll(name, givenName);
    replaceLastCharactersWithSuffix(name, 2, OF_USTR(u"ε"));
done:;
    OCFStringRef result = OCFRetain(name);
    OCFRelease(name);
    if (result != NULL) {
        return result;
    }
    return OCFRetain(givenName);
}

// Native _CFStringCopyVocativeCaseOfGivenName_uk, 0x1c0203b40. Non-external.
// Four irregular names are rewritten from their second character (the lowercased length minus one characters
// replaced); the suffix rules follow in native order, with the consonant list's duplicate "в" kept.
static OCFStringRef _CFStringCopyVocativeCaseOfGivenName_uk(OCFStringRef givenName, OCFLocaleRef locale) {
    OCFMutableStringRef name = OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, 0, givenName);
    if (name == NULL) {
        haltWithFunctionMessage("_CFStringCopyVocativeCaseOfGivenName_uk", "Allocation failure");
    }
    OCFStringLowercase(name, locale);
    OCFStringRef irregular = NULL;
    if (OCFStringCompare(name, OF_USTR(u"фредрік"), 0) == kOCFCompareEqualTo) {
        irregular = OF_USTR(u"редеріку");
    } else if (OCFStringCompare(name, OF_USTR(u"зорян"), 0) == kOCFCompareEqualTo) {
        irregular = OF_USTR(u"оране");
    } else if (OCFStringCompare(name, OF_USTR(u"зиновій"), 0) == kOCFCompareEqualTo) {
        irregular = OF_USTR(u"іновію");
    } else if (OCFStringCompare(name, OF_USTR(u"младен"), 0) == kOCFCompareEqualTo) {
        irregular = OF_USTR(u"ладлене");
    }
    UniChar character;
    if (irregular != NULL) {
        OCFIndex lowercasedLength = OCFStringGetLength(name);
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, lowercasedLength - 1, irregular);
    } else if (__CFStringHasOneOfSuffixes(name, OF_USTR(u"ва"), OF_USTR(u"на"), OF_USTR(u"ма"), NULL)) {
        goto replaceWithO;
    } else if (__CFStringHasOneOfSuffixes(name, OF_USTR(u"ія"), OF_USTR(u"оя"), OF_USTR(u"лля"), NULL)) {
        goto replaceWithYe;
    } else if (
        __CFStringHasOneOfSuffixes(
            name, OF_USTR(u"ло"), OF_USTR(u"ися"), OF_USTR(u"ро"), OF_USTR(u"хіль"), OF_USTR(u"по"), NULL
        )
    ) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 1, OF_USTR(u"е"));
    } else if (
        __CFStringHasOneOfSuffixes(
            name,
            OF_USTR(u"й"),
            OF_USTR(u"ь"),
            OF_USTR(u"ля"),
            OF_USTR(u"ль"),
            OF_USTR(u"ня"),
            OF_USTR(u"ая"),
            OF_USTR(u"тя"),
            OF_USTR(u"ся"),
            NULL
        )
    ) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 1, OF_USTR(u"ю"));
    } else if (OCFStringHasSuffix(name, OF_USTR(u"я"))) {
        goto replaceWithYe;
    } else if (__CFStringHasOneOfSuffixes(name, OF_USTR(u"к"), OF_USTR(u"г"), NULL)) {
        OCFStringReplaceAll(name, givenName);
        character = 0x443;
        OCFStringAppendCharacters(name, &character, 1);
    } else if (OCFStringHasSuffix(name, OF_USTR(u"ко"))) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 1, OF_USTR(u"у"));
    } else if (OCFStringHasSuffix(name, OF_USTR(u"ір"))) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 2, OF_USTR(u"оре"));
    } else if (OCFStringHasSuffix(name, OF_USTR(u"ів"))) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 2, OF_USTR(u"ове"));
    } else if (OCFStringHasSuffix(name, OF_USTR(u"а"))) {
        goto replaceWithO;
    } else if (OCFStringHasSuffix(name, OF_USTR(u"шан"))) {
        OCFStringReplaceAll(name, givenName);
        character = 0x43e;
        OCFStringAppendCharacters(name, &character, 1);
    } else if (
        __CFStringHasOneOfSuffixes(
            name,
            OF_USTR(u"н"),
            OF_USTR(u"в"),
            OF_USTR(u"р"),
            OF_USTR(u"в"),
            OF_USTR(u"м"),
            OF_USTR(u"т"),
            OF_USTR(u"с"),
            OF_USTR(u"л"),
            OF_USTR(u"п"),
            OF_USTR(u"д"),
            OF_USTR(u"ф"),
            OF_USTR(u"б"),
            OF_USTR(u"ш"),
            OF_USTR(u"ц"),
            NULL
        )
    ) {
        OCFStringReplaceAll(name, givenName);
        character = 0x435;
        OCFStringAppendCharacters(name, &character, 1);
    } else {
        OCFRelease(name);
        return OCFRetain(givenName);
    }
    goto done;
replaceWithO:
    OCFStringReplaceAll(name, givenName);
    replaceLastCharactersWithSuffix(name, 1, OF_USTR(u"о"));
    goto done;
replaceWithYe:
    OCFStringReplaceAll(name, givenName);
    replaceLastCharactersWithSuffix(name, 1, OF_USTR(u"є"));
done:;
    OCFStringRef result = OCFRetain(name);
    OCFRelease(name);
    if (result == NULL) {
        return OCFRetain(givenName);
    }
    return result;
}

// Native _CFStringCopyVocativeCaseOfGivenName_lt, 0x1c0204220. Non-external.
// -jas, -as, -us and -ė take a replacement (the length checked); -is and -ys lose their last character
// without a length check.
static OCFStringRef _CFStringCopyVocativeCaseOfGivenName_lt(OCFStringRef givenName, OCFLocaleRef locale) {
    OCFMutableStringRef name = OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, 0, givenName);
    if (name == NULL) {
        haltWithFunctionMessage("_CFStringCopyVocativeCaseOfGivenName_lt", "Allocation failure");
    }
    OCFStringLowercase(name, locale);
    if (OCFStringHasSuffix(name, OF_STR("jas"))) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 1, OF_STR("u"));
    } else if (OCFStringHasSuffix(name, OF_STR("as"))) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 1, OF_STR("i"));
    } else if (OCFStringHasSuffix(name, OF_STR("us"))) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 2, OF_STR("au"));
    } else if (OCFStringHasSuffix(name, OF_STR("is")) || OCFStringHasSuffix(name, OF_STR("ys"))) {
        OCFStringReplaceAll(name, givenName);
        OCFStringDelete(name, (OCFRange){OCFStringGetLength(name) - 1, 1});
    } else if (OCFStringHasSuffix(name, OF_USTR(u"ė"))) {
        OCFStringReplaceAll(name, givenName);
        replaceLastCharactersWithSuffix(name, 1, OF_STR("e"));
    } else {
        OCFRelease(name);
        return OCFRetain(givenName);
    }
    OCFStringRef result = OCFRetain(name);
    OCFRelease(name);
    if (result == NULL) {
        return OCFRetain(givenName);
    }
    return result;
}

// Native ___CFStringHasOneOfSuffixes, 0x1c0204518. Non-external.
// The variadic arguments are a NULL-terminated list of suffixes, tested in order with CFStringHasSuffix.
static bool __CFStringHasOneOfSuffixes(OCFStringRef string, ...) {
    va_list suffixes;
    va_start(suffixes, string);
    OCFStringRef suffix;
    do {
        suffix = va_arg(suffixes, OCFStringRef);
    } while (suffix != NULL && !OCFStringHasSuffix(string, suffix));
    va_end(suffixes);
    return suffix != NULL;
}

// Native ___CFStringApproximateSyllableCount_el_GR, 0x1c020459c. Non-external.
// Counts Greek vowels through the SDK inline buffer (the buffer record zero-initialized natively): ά-ί, α, ε,
// η, ι, ο, υ, ω and ό-ώ each count once; the digraphs α/ε/υ + ι/ί, ι + ά/α/ω/ώ and ο + ί/ι/υ/ύ count once for
// both characters. A NUL character ends the count early.
static OCFIndex __CFStringApproximateSyllableCount_el_GR(OCFStringRef string) {
    OCFStringInlineBuffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    OCFIndex length = OCFStringGetLength(string);
    stringInitInlineBuffer(string, &buffer, (OCFRange){0, length});
    OCFIndex count = 0;
    for (OCFIndex index = 0; index < length; ++index) {
        UniChar character = stringGetCharacterFromInlineBuffer(&buffer, index);
        UniChar next;
        switch (character) {
            case 0:
                return count;
            case 0x3ac:
            case 0x3ad:
            case 0x3ae:
            case 0x3af:
            case 0x3b7:
            case 0x3c9:
            case 0x3cc:
            case 0x3cd:
            case 0x3ce:
                ++count;
                break;
            case 0x3b1:
            case 0x3b5:
            case 0x3c5:
                if (index + 1 < length) {
                    next = stringGetCharacterFromInlineBuffer(&buffer, index + 1);
                    if (next == 0x3b9 || next == 0x3af) {
                        ++index;
                    }
                }
                ++count;
                break;
            case 0x3b9:
                if (index + 1 < length) {
                    next = stringGetCharacterFromInlineBuffer(&buffer, index + 1);
                    if ((unsigned)(next - 0x3ac) <= 0x22 &&
                        ((UINT64_C(1) << (next - 0x3ac)) & 0x420000021) != 0) {
                        ++index;
                    }
                }
                ++count;
                break;
            case 0x3bf:
                if (index + 1 < length) {
                    next = stringGetCharacterFromInlineBuffer(&buffer, index + 1);
                    if ((unsigned)(next - 0x3af) <= 0x1e && ((1U << (next - 0x3af)) & 0x40400401) != 0) {
                        ++index;
                    }
                }
                ++count;
                break;
            default:
                break;
        }
    }
    return count;
}

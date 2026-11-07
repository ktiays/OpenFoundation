//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/OFPortHooks.h"
#include "CoreFoundation/String/OCFStringFormat.h"
#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Locale/OCFNumberFormatter.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFStringFormatConfiguration.h"
#include "CoreFoundation/String/OCFStringLocalizedFormatting.h"
#include "CoreFoundation/Unicode/OCFUniChar.h"

#include <assert.h>
#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <malloc/malloc.h>
#include <math.h>
#include <os/lock.h>
#include <os/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xlocale.h>

// Inferred organization: the formatting functions of the native CFString.c (the source path that
// _CFStringFormatAppendMetadata's assertion names) from ___CFStringCopyFormattingDescription (0x1c0209e60) to
// ___locked_CFStringFormatLocalizedNumberConfigureFormatter (0x1c02258c4), interleaved in __text with the
// string object functions of the string-core family; this unit holds the formatting family's share of that
// source file. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read; the pseudocode was a guide
// only). iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationStringFormat.md.

// libdyld, libsystem_trace and libsystem_c entry points the native code calls; supplied, never reimplemented.
// dyld_program_sdk_at_least receives dyld's packed build-version pair in X0, as in Base/OCFUtilities.c.
extern bool dyld_program_sdk_at_least(OCFBuildVersion version);

extern bool _os_feature_enabled_impl(const char *domain, const char *feature);

extern struct mach_header __dso_handle;

// Format metadata keys. The exported pointers and constant string records are read from
// __DATA_CONST,__const and __DATA_CONST,__cfstring in the pinned iOS Simulator image.
// The formatter at 0x1c0225068 refers directly to the same constant objects as these exported pointers.
// iOS Device divergence: the format metadata key bindings remain unread.
// Native _kCFStringFormatMetadataReplacementIndexKey, 0x2227a52b8. External.
static const OCFConstantString indexMetadataKeyString = ConstantASCIIString("Index");
const OCFStringRef _kOCFStringFormatMetadataReplacementIndexKey = (OCFStringRef)&indexMetadataKeyString;

// Native _kCFStringFormatMetadataSpecifierRangeLocationInFormatStringKey, 0x2227a52c0. External.
static const OCFConstantString specLocationMetadataKeyString = ConstantASCIIString("SpecLocation");
const OCFStringRef _kOCFStringFormatMetadataSpecifierRangeLocationInFormatStringKey =
    (OCFStringRef)&specLocationMetadataKeyString;

// Native _kCFStringFormatMetadataSpecifierRangeLengthInFormatStringKey, 0x2227a52c8. External.
static const OCFConstantString specLengthMetadataKeyString = ConstantASCIIString("SpecLength");
const OCFStringRef _kOCFStringFormatMetadataSpecifierRangeLengthInFormatStringKey =
    (OCFStringRef)&specLengthMetadataKeyString;

// Native _kCFStringFormatMetadataReplacementRangeLocationKey, 0x2227a52d0. External.
static const OCFConstantString replacementLocationMetadataKeyString =
    ConstantASCIIString("ReplacementLocation");
const OCFStringRef _kOCFStringFormatMetadataReplacementRangeLocationKey =
    (OCFStringRef)&replacementLocationMetadataKeyString;

// Native _kCFStringFormatMetadataReplacementRangeLengthKey, 0x2227a52d8. External.
static const OCFConstantString replacementLengthMetadataKeyString = ConstantASCIIString("ReplacementLength");
const OCFStringRef _kOCFStringFormatMetadataReplacementRangeLengthKey =
    (OCFStringRef)&replacementLengthMetadataKeyString;

// Native _kCFStringFormatMetadataArgumentObjectKey, 0x2227a52e0. External.
static const OCFConstantString objectMetadataKeyString = ConstantASCIIString("Object");
const OCFStringRef _kOCFStringFormatMetadataArgumentObjectKey = (OCFStringRef)&objectMetadataKeyString;

// Native _kCFStringFormatMetadataAddedIsolatesKey, 0x2227a52f0. External.
static const OCFConstantString addedIsolatesMetadataKeyString = ConstantASCIIString("AddedIsolates");
const OCFStringRef _kOCFStringFormatMetadataAddedIsolatesKey = (OCFStringRef)&addedIsolatesMetadataKeyString;

// Native kCFStringVariableWidthRuleType (0x2227aa268), kCFStringDeviceSpecificRuleType (0x2227aa270) and
// kCFStringLocalizedFormatKey (0x2227aa280): external pointers in __DATA_CONST,__const, among this unit's
// stringsdict keys (kCFStringFormatValueTypeKey at 0x2227aa278 and the verbatim and direct rule types after
// them are not reached by restored code yet), each to a constant string in __cfstring (0x2227bcd20,
// 0x2227bcd40, 0x2227b4020: class reference, info 0x7c8, the ASCII bytes, lengths 29, 30 and 26). OF_STR is
// an expression and cannot initialize file-scope data, so the records are spelled with ConstantASCIIString,
// the file-scope form of the same record. The values are stringsdict data keys, not runtime names: the prefix
// tool flags only literals equal to a declared name, and no NS name is declared for them, so they keep the
// native spelling. Evidence: Local/Scratch/cf-core/macho.py (static data); confidence: verified.
static const OCFConstantString variableWidthRuleTypeString =
    ConstantASCIIString("NSStringVariableWidthRuleType");
static const OCFConstantString deviceSpecificRuleTypeString =
    ConstantASCIIString("NSStringDeviceSpecificRuleType");
static const OCFConstantString localizedFormatKeyString = ConstantASCIIString("NSStringLocalizedFormatKey");
const OCFStringRef kOCFStringVariableWidthRuleType = (OCFStringRef)&variableWidthRuleTypeString;
const OCFStringRef kOCFStringDeviceSpecificRuleType = (OCFStringRef)&deviceSpecificRuleTypeString;
const OCFStringRef kOCFStringLocalizedFormatKey = (OCFStringRef)&localizedFormatKeyString;

// The CFString object header fields the formatter reads inline (the info word with LDAR, the contents) are
// read through the string family's helpers (stringInfo, stringContents; String/OCFString.h), which perform
// the same loads.

// Native ___CFStringCopyFormattingDescription, 0x1c0209e60. Hidden (non-external; the CFString class's
// formatting-description callback).
// A copy with the string's own allocator: the system default for tagged pointers and for strings whose info
// has bit 7 set, else the allocator stored before the object.
OCFStringRef __CFStringCopyFormattingDescription(OCFTypeRef cf, const void *formatOptions) {
    (void)formatOptions;
    OCFAllocatorRef allocator;
    if ((intptr_t)cf < 0) {
        allocator = kOCFAllocatorSystemDefault;
    } else if ((stringInfo(cf) & 0x80) != 0) {
        allocator = kOCFAllocatorSystemDefault;
    } else {
        allocator = *(const OCFAllocatorRef *)((const char *)cf - 16);
    }
    return OCFStringCreateCopy(allocator, cf);
}

static void __CFStringSetUpFormatAndSpecBuffers(
    OCFStringRef string,
    OCFIndex length,
    const uint8_t **cformat,
    const UniChar **uformat,
    UniChar **allocatedUniBuffer,
    UniChar *stackUniBuffer,
    FormatSpecRecord **specs,
    FormatSpecRecord *stackSpecs,
    OCFStringRef **configs,
    OCFStringRef *stackConfigs
);
static OCFIndex __CFStringValidateFormatSpec(
    OCFStringRef expected, const FormatSpecRecord *spec, OCFIndex initialArgPosition
);
static bool __CFStringFormatLocalizedNumber(
    OCFMutableStringRef output,
    OCFLocaleRef locale,
    const PrintValue *values,
    const FormatSpecRecord *spec,
    int32_t width,
    int32_t precision,
    bool hasPrecision
);
static void _CFStringFormatAppendMetadata(
    OCFMutableArrayRef *outMetadata,
    OCFIndex capacity,
    const PrintValue *values,
    OCFIndex valuesCount,
    const FormatSpecRecord *spec,
    OCFIndex replacementStart,
    OCFIndex replacementEnd,
    uint32_t addedIsolates
);
static void __CFAppendFormattedValueWithStatisticalWritingDirections(
    OCFStringRef value, OCFMutableStringRef output, bool isDirectionalMark, bool useIsolates
);
static void __CFAppendFormattedValue(OCFStringRef value, OCFMutableStringRef output, bool useIsolates);

// Native ___CFStringAppendFormatCore.warnOnce and .disableIsolates (__DATA,__data, initially 0xff).
// Non-external.
static dispatch_once_t warnOnce;
static uint8_t disableIsolates = 0xff;

// Native ___CFStringAppendFormatCore.cold.1, 0x1c030c4ac, with _____CFStringAppendFormatCore_block_invoke
// (0x1c0225390, the global block it passes to dispatch_once). Kept inside the formatter's %s path.
// For a %s argument that is a tag-0 tagged pointer of a program linked before the SDK check: when the object
// has an internal C string (MacRoman, NUL-terminated, bridging-checked) that dladdr places in an image, the
// argument becomes "" after a one-time level-2 log; otherwise the pointer itself is used as the C string.
static const char *taggedCStringArgument(const void *argument) {
    const char *result = argument;
    const char *cString = _CFStringGetCStringPtrInternal(argument, 0x600, true, true);
    Dl_info information;
    memset(&information, 0, sizeof(information));
    if (cString != NULL && dladdr(cString, &information) != 0) {
        dispatch_once(&warnOnce, ^{
          OCFLog(
              2,
              OF_STR(
                  "Passing an object as the argument to a %%s format string is invalid, and will crash in a "
                  "future release of the operating system. Call it with a char * as the argument instead. "
                  "This "
                  "warning is displayed only once."
              )
          );
        });
        result = "";
    }
    return result;
}

// An error with NSDebugDescription for *errorPtr (kCFErrorDomainCocoa, code 2048), as every failure path of
// the formatter builds it inline: the dictionary uses the copy-string key callbacks and is released after
// CFErrorCreate. The helper name is a local descriptive spelling.
static inline void
setFormatError(OCFAllocatorRef allocator, OCFErrorRef *errorPtr, OCFStringRef description) {
    OCFMutableDictionaryRef userInfo = OCFDictionaryCreateMutable(
        allocator, 0, &kOCFCopyStringDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    OCFDictionarySetValue(userInfo, kOCFErrorDebugDescriptionKey, description);
    *errorPtr = OCFErrorCreate(allocator, kOCFErrorDomainCocoa, 2048, userInfo);
    OCFRelease(userInfo);
}

static inline OCFAllocatorRef formattingAllocator(void) {
    OCFAllocatorRef allocator = _OCFGetTSD(1);
    return allocator != NULL ? allocator : kOCFAllocatorSystemDefault;
}

static inline OCFStringEncoding eightBitEncoding(void) {
    OCFStringEncoding encoding = __OCFDefaultEightBitStringEncoding;
    if (encoding == (OCFStringEncoding)-1) {
        encoding = __OCFStringComputeEightBitStringEncoding();
    }
    return encoding;
}

// The inlined system-encoding read: the encoding byte's bit 0 answers MacRoman (0) at once; otherwise
// CFStringGetSystemEncoding runs and the byte minus one is used.
static inline OCFStringEncoding systemEncoding(void) {
    if ((__CFDefaultSystemEncoding & 1) != 0) {
        return 0;
    }
    OCFStringGetSystemEncoding();
    return (OCFStringEncoding)(__CFDefaultSystemEncoding - 1);
}

static inline void appendSpaces(OCFMutableStringRef output, int32_t count) {
    do {
        __OCFStringAppendBytes(output, "          ", (uint32_t)count < 10 ? count : 10, 0x600);
        count -= 10;
    } while (count > 0);
}

// An argument-number assignment that reaches 127 overflows the signed byte (the native compares the sign-
// extended byte minus 127 unsigned against 0xfffffeff).
static inline bool argumentNumberOverflows(int32_t number) {
    return (uint32_t)((int32_t)(int8_t)number - 127) <= 0xfffffeffu;
}

// Native CFStringCreateStringWithValidatedFormat, 0x1c020ac9c. External.
// Formats through _CFStringCreateStringWithValidatedFormatAuxWithDesc without description functions or an
// implicit argument.
OCFStringRef OCFStringCreateStringWithValidatedFormat(
    OCFAllocatorRef alloc,
    OCFDictionaryRef formatOptions,
    OCFStringRef validFormatSpecifiers,
    OCFStringRef format,
    OCFErrorRef *errorPtr,
    ...
) {
    va_list arguments;
    va_start(arguments, errorPtr);
    OCFStringRef result = _OCFStringCreateStringWithValidatedFormatAuxWithDesc(
        alloc, NULL, NULL, formatOptions, NULL, validFormatSpecifiers, format, arguments, errorPtr
    );
    va_end(arguments);
    return result;
}

// Native _CFStringCreateStringWithValidatedFormatAuxWithDesc, 0x1c020ad8c. External.
// Formats into a mutable string of the system default allocator whose desired capacity is set to 120 (the
// word at offset 0x28 keeps its four low flag bits and takes 120 << 4), validating the format against the
// valid specifiers (no configuration, no metadata; the error goes to the caller's pointer), then copies it
// with alloc. A failure answers NULL without logging.
OCFStringRef _OCFStringCreateStringWithValidatedFormatAuxWithDesc(
    OCFAllocatorRef alloc,
    CopyDescriptionFunction copyDescFunc,
    ContextDescriptionFunction contextDescFunc,
    OCFDictionaryRef formatOptions,
    OCFTypeRef implicitArgument,
    OCFStringRef validFormatSpecifiers,
    OCFStringRef format,
    va_list arguments,
    OCFErrorRef *errorPtr
) {
    OCFMutableStringRef outputString = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
    uint64_t *capacityWord =
        (uint64_t *)((char *)outputString + offsetof(struct __OCFString, variants.notInlineMutable.bits));
    *capacityWord = (*capacityWord & 0xf) | 0x780;
    OCFStringRef result;
    if (__CFStringAppendFormatCore(
            outputString,
            copyDescFunc,
            contextDescFunc,
            formatOptions,
            NULL,
            implicitArgument,
            validFormatSpecifiers,
            format,
            0,
            0,
            NULL,
            0,
            arguments,
            NULL,
            errorPtr
        )) {
        result = OCFStringCreateCopy(alloc, outputString);
    } else {
        result = NULL;
    }
    OCFRelease(outputString);
    return result;
}

// Native ___CFStringAppendFormatCore, 0x1c020afb8. Hidden (non-external).
// Parses formatString into 32-byte specification records (61 on the stack; 2n + 1 allocated for n '%' from
// 31), resolves %#@key@ and %[attribute]@ configurations, assigns argument numbers (positional N$ or
// sequential from initialArgPosition), validates them against validFormatSpecifiers, collects the arguments
// from args (eight-byte slots of the Darwin arm64 va_list) or originalValues, and appends each piece to
// outputString: literal text, snprintf_l-formatted numbers (or CFNumberFormatter output for localized %d/%f
// in a locale), strings, characters, object descriptions with optional bidirectional isolates, and nested
// formats resolved from the configuration. With outReplacementMetadata, one dictionary per replacement is
// collected and copied out at the end. Overflow, nesting deeper than ten levels, a '%' count of 2^20 or more
// and validation failures answer false, with an NSCocoaErrorDomain 2048 error when errorPtr is given.
// Confidence: medium for this body as a whole. The constants, call sites and their arguments, the record
// layouts, the argument fetch, the snprintf_l argument lists, the Funnel3 options, the error sites and the
// cleanup order were checked against the assembly; the rest of the control flow (notably the value-type
// parser) follows the pseudocode and has not been re-read instruction by instruction.
bool __CFStringAppendFormatCore(
    OCFMutableStringRef outputString,
    CopyDescriptionFunction copyDescFunc,
    ContextDescriptionFunction contextDescFunc,
    OCFTypeRef formatOptions,
    OCFDictionaryRef formatConfiguration,
    OCFTypeRef implicitArgument,
    OCFStringRef validFormatSpecifiers,
    OCFStringRef formatString,
    OCFIndex initialArgPosition,
    OCFIndex nestingDepth,
    const PrintValue *originalValues,
    OCFIndex originalValuesSize,
    va_list args,
    OCFArrayRef *outReplacementMetadata,
    OCFErrorRef *errorPtr
) {
    PrintValue stackValues[61];
    FormatSpecRecord stackSpecs[61];
    OCFDictionaryRef stackConfigs[61];
    UniChar stackUniBuffer[400];
    char stackFormatBuffer[128];
    char stackOutputBuffer[528];
    memset(stackValues, 0, sizeof(stackValues));
    bool formatOptionsIsLocale = formatOptions != NULL && OCFGetTypeID(formatOptions) == OCFLocaleGetTypeID();
    memset(stackConfigs, 0, sizeof(stackConfigs));
    bool statisticalIsolates[2] = {false, false};
    OCFMutableArrayRef metadataArray = NULL;
    OCFMutableArrayRef *metadataSlot = outReplacementMetadata != NULL ? &metadataArray : NULL;
    OCFDictionaryRef configuration = _OCFStringGetFormatSpecifierConfiguration(formatString);
    if (configuration == NULL) {
        configuration = formatConfiguration;
    }
    int64_t dummyCount = 0;
    // The argument cursor: on Darwin arm64 a va_list is a pointer to eight-byte slots.
    char *argumentCursor = (char *)args;

    if (validFormatSpecifiers != NULL) {
        if (nestingDepth >= 11) {
            if (errorPtr != NULL) {
                setFormatError(
                    NULL,
                    errorPtr,
                    OF_STR("Exceeded maximum number of allowed nested stringsdict entry references")
                );
            }
            return false;
        }
        // Natively the buffers are set up for the valid specifiers and released again unused.
        const uint8_t *validCFormat = NULL;
        const UniChar *validUFormat = NULL;
        UniChar *validAllocated = NULL;
        FormatSpecRecord *validSpecs = NULL;
        OCFStringRef *validConfigs = NULL;
        OCFStringRef validStackConfigs[4];
        memset(validStackConfigs, 0, sizeof(validStackConfigs));
        __CFStringSetUpFormatAndSpecBuffers(
            validFormatSpecifiers,
            OCFStringGetLength(validFormatSpecifiers),
            &validCFormat,
            &validUFormat,
            &validAllocated,
            stackUniBuffer,
            &validSpecs,
            stackSpecs,
            &validConfigs,
            validStackConfigs
        );
        if (validAllocated != stackUniBuffer) {
            OCFAllocatorDeallocate(NULL, validAllocated);
        }
        if (validSpecs != stackSpecs) {
            OCFAllocatorDeallocate(NULL, validSpecs);
        }
        if (validConfigs != validStackConfigs) {
            OCFAllocatorDeallocate(NULL, validConfigs);
        }
        if (configuration == NULL &&
            __CFStringValidateFormat(validFormatSpecifiers, formatString, 0, errorPtr) < 0) {
            return false;
        }
    }

    bool result = false;
    OCFAllocatorRef allocator = NULL;
    UniChar *uniBuffer = NULL;
    FormatSpecRecord *specs = NULL;
    PrintValue *values = NULL;
    OCFDictionaryRef *configs = NULL;
    OCFMutableDictionaryRef bracketDictionary = NULL;
    const uint8_t *cformat = NULL;
    const UniChar *uformat = NULL;
    int32_t percentCount = 0;

    OCFIndex formatLength = OCFStringGetLength(formatString);
    if (formatLength >= 0x80000000) {
        if (errorPtr == NULL) {
            goto cleanupUnconditional;
        }
        goto overflow;
    }
    if ((intptr_t)formatString >= 0 && !__CF_IS_OBJC(7, formatString)) {
        uint64_t info = stringInfo(formatString);
        if ((info & 0x10) != 0) {
            uformat = stringContents(formatString);
        } else {
            const uint8_t *contents = stringContents(formatString);
            if (contents != NULL) {
                cformat = contents + ((stringInfo(formatString) & 4) != 0 ? 1 : 0);
            }
        }
    }
    if (cformat == NULL && uformat == NULL) {
        UniChar *buffer = stackUniBuffer;
        if (formatLength >= 401) {
            allocator = formattingAllocator();
            buffer = OCFAllocatorAllocateTyped(allocator, 2 * formatLength, 0x1000040bdfb0063, 0);
            if (buffer != stackUniBuffer && __OCFOASafe) {
                __OCFSetLastAllocationEventName(buffer, "CFString (temp)");
            }
        }
        OCFStringGetCharacters(formatString, (OCFRange){0, formatLength}, buffer);
        uformat = buffer;
        uniBuffer = buffer;
    }
    if (cformat != NULL) {
        for (OCFIndex index = 0; index < formatLength; ++index) {
            if (cformat[index] == '%' && __builtin_add_overflow(percentCount, 1, &percentCount)) {
                goto countOverflow;
            }
        }
    } else {
        for (OCFIndex index = 0; index < formatLength; ++index) {
            if (uformat[index] == '%' && __builtin_add_overflow(percentCount, 1, &percentCount)) {
                goto countOverflow;
            }
        }
    }
    if (percentCount >= 0x100000) {
    countOverflow:
        if (errorPtr == NULL) {
            goto cleanupUnconditional;
        }
        goto overflow;
    }

    allocator = formattingAllocator();
    specs = stackSpecs;
    configs = stackConfigs;
    if (percentCount >= 31) {
        specs = OCFAllocatorAllocateTyped(
            allocator, (OCFIndex)((uint64_t)(uint32_t)((percentCount << 1) | 1) << 5), 0x10000408e5dae1c, 0
        );
        if (specs != stackSpecs && __OCFOASafe) {
            __OCFSetLastAllocationEventName(specs, "CFString (temp)");
        }
        if ((uint32_t)percentCount >= 61) {
            configs = OCFAllocatorAllocateTyped(
                allocator, (OCFIndex)((uint64_t)(uint32_t)percentCount << 3), 0x6004044c4a2df, 0
            );
        }
    }

    // Parsing.
    int32_t specCount = 0;
    OCFIndex configCount = 0;
    if (formatLength >= 1) {
        int32_t position = 0;
        OCFIndex specIndex = 0;
        while (true) {
            FormatSpecRecord *spec = &specs[specIndex];
            spec->size = 0;
            spec->type = 0;
            spec->loc = position;
            spec->len = 0;
            spec->widthArg = -1;
            spec->precArg = -1;
            spec->flags = 0;
            *(int32_t *)&spec->mainArgNum = -1;
            int32_t start = position;
            OCFStringRef key = NULL;
            OCFIndex scan = start;
            if (cformat != NULL) {
                while (scan < formatLength && cformat[scan] != '%') {
                    ++scan;
                }
            } else {
                while (scan < formatLength && uformat[scan] != '%') {
                    ++scan;
                }
            }
            position = (int32_t)scan;
            if (position != start) {
                spec->type = 0x20;
                goto recordLength;
            }
            position = start + 1;
            if (position >= (int32_t)formatLength) {
                goto truncated;
            }
            {
                uint32_t flags = 0;
                OCFIndex bracketLength = 0;
                bool bracketClosed = false;
                bool inBracket = false;
                bool hash = false;
                bool sawPeriod = false;
                OCFIndex keyStart = -1;
                uint8_t widthArgNum = 0xff;
                uint8_t precArgNum = 0xff;
                uint32_t character;
                while (true) {
                    OCFIndex characterIndex = position;
                    character = cformat != NULL ? cformat[position] : uformat[position];
                    ++position;
                    if (keyStart >= 0) {
                        bool identifier = character >= '0' && character - ':' >= 7 && character <= 'z' &&
                                          (character == '_' || character - '[' >= 6);
                        if (identifier) {
                            goto nextCharacter;
                        }
                        if (character == ']') {
                            if (inBracket) {
                                bracketClosed = true;
                                bracketLength = characterIndex - keyStart;
                            }
                            goto nextCharacter;
                        }
                        if (character != '@') {
                            keyStart = -1;
                            goto nextCharacter;
                        }
                        OCFIndex keyLength;
                        if (bracketClosed) {
                            flags |= 0x40;
                            keyLength = bracketLength;
                        } else {
                            keyLength = characterIndex - keyStart;
                        }
                        spec->flags = flags | 0x10;
                        spec->size = 4;
                        spec->type = 0x25;
                        if (keyLength < 1) {
                            key = NULL;
                            goto configure;
                        }
                        if (cformat != NULL) {
                            key = __CFStringCreateImmutableFunnel3(
                                NULL,
                                cformat + keyStart,
                                keyLength,
                                eightBitEncoding(),
                                2,
                                (OCFAllocatorRef)-1,
                                0
                            );
                        } else {
                            key = __CFStringCreateImmutableFunnel3(
                                NULL, uformat + keyStart, 2 * keyLength, 0x100, 0x10, kOCFAllocatorNull, 0
                            );
                        }
                        if (spec->type != 0x20) {
                            goto configure;
                        }
                        flags = spec->flags;
                        goto unknownConversion;
                    }
                parseCharacter:
                    while (character - '1' >= 9) {
                        switch (character) {
                            case ' ':
                                if ((flags & 4) != 0) {
                                    goto nextCharacter;
                                }
                                flags |= 8;
                                spec->flags = flags;
                                goto nextCharacter;
                            case '#':
                                hash = true;
                                goto nextCharacter;
                            case '*':
                                widthArgNum = 0xfe;
                                spec->widthArgNum = -2;
                                goto nextCharacter;
                            case '+':
                                flags = (flags & 0xfffffff3) | 4;
                                spec->flags = flags;
                                goto nextCharacter;
                            case '-':
                                flags = (flags & 0xfffffffc) | 2;
                                spec->flags = flags;
                                goto nextCharacter;
                            case '.':
                                if (position >= (int32_t)formatLength) {
                                    key = NULL;
                                    spec->type = 0x2b;
                                    goto configure;
                                }
                                character = cformat != NULL ? cformat[position] : uformat[position];
                                ++position;
                                sawPeriod = true;
                                if (character == '*') {
                                    precArgNum = 0xfe;
                                    spec->precArgNum = -2;
                                    sawPeriod = true;
                                    goto nextCharacter;
                                }
                                continue;
                            case '0':
                                if (sawPeriod) {
                                    spec->precArg = 0;
                                    sawPeriod = true;
                                    goto nextCharacter;
                                }
                                sawPeriod = false;
                                if ((flags & 2) == 0) {
                                    flags |= 1;
                                    spec->flags = flags;
                                }
                                goto nextCharacter;
                            case '@':
                                if (hash) {
                                    hash = false;
                                    keyStart = position;
                                    goto nextCharacter;
                                }
                                key = NULL;
                                spec->size = 4;
                                spec->type = 0x25;
                                goto configure;
                            case 'A':
                            case 'a':
                                goto floatingType;
                            case 'C':
                                key = NULL;
                                spec->size = 2;
                                spec->type = 0x29;
                                goto configure;
                            case 'D':
                            case 'U':
                            case 'd':
                            case 'i':
                            case 'u':
                                if (_OCFExecutableLinkedOnOrAfter(8)) {
                                    spec->flags |= 0x20;
                                }
                                spec->numericStyle = (character & 0xffdf) == 'U' ? 4 : 1;
                                goto integerType;
                            case 'E':
                            case 'F':
                            case 'G':
                            case 'e':
                            case 'f':
                            case 'g': {
                                if (_OCFExecutableLinkedOnOrAfter(8)) {
                                    spec->flags |= 0x20;
                                }
                                uint32_t lowered = character - 'A' < 26 ? character + 32 : character;
                                uint8_t style = (lowered & 0xfffd) == 'e' ? 2 : 0;
                                if ((lowered & 0xfffe) == 'f') {
                                    ++style;
                                }
                                spec->numericStyle = style;
                                if (sawPeriod && spec->precArg == -1 && (uint8_t)spec->precArgNum == 0xff) {
                                    spec->precArg = 0;
                                }
                            }
                            floatingType:
                                spec->type = 0x22;
                                if (spec->size != 5) {
                                    key = NULL;
                                    spec->size = 4;
                                    goto configure;
                                }
                                key = NULL;
                                goto configure;
                            case 'L':
                                spec->size = 5;
                                goto nextCharacter;
                            case 'O':
                            case 'X':
                            case 'o':
                            case 'x':
                            integerType:
                                key = NULL;
                                spec->type = 0x21;
                                goto configure;
                            case 'P':
                                key = NULL;
                                spec->size = 4;
                                spec->type = 0x28;
                                goto configure;
                            case 'S':
                                key = NULL;
                                spec->size = 4;
                                spec->type = 0x26;
                                goto configure;
                            case '[':
                                if (!inBracket) {
                                    keyStart = position;
                                }
                                inBracket = true;
                                goto nextCharacter;
                            case 'c':
                                key = NULL;
                                spec->size = 1;
                                spec->type = 0x21;
                                goto configure;
                            case 'h':
                                if (position < (int32_t)formatLength &&
                                    (cformat != NULL ? cformat[position] : uformat[position]) == 'h') {
                                    ++position;
                                    spec->size = 1;
                                } else {
                                    spec->size = 2;
                                }
                                goto nextCharacter;
                            case 'j':
                            case 'q':
                            case 't':
                            case 'z':
                                spec->size = OF_PORT_ANSWER(formatIntegerSize, 4, character);
                                goto nextCharacter;
                            case 'l':
                                if (position < (int32_t)formatLength &&
                                    (cformat != NULL ? cformat[position] : uformat[position]) == 'l') {
                                    ++position;
                                }
                                spec->size = OF_PORT_ANSWER(
                                    formatLongSize, 4, (const uint8_t *)cformat, uformat, position
                                );
                                goto nextCharacter;
                            case 'n':
                                key = NULL;
                                spec->size = 4;
                                spec->type = 0x2a;
                                goto configure;
                            case 'p':
                                key = NULL;
                                spec->size = 4;
                                spec->type = 0x23;
                                goto configure;
                            case 's':
                                key = NULL;
                                spec->size = 4;
                                spec->type = 0x27;
                                goto configure;
                            default:
                            unknownConversion:
                                // Kept as literal text without the '%'.
                                spec->type = 0x20;
                                spec->loc = start + 1;
                                spec->len = position + ~start;
                                spec->flags = flags | 0x80;
                                goto nextSpec;
                        }
                    }
                    {
                        // Digits: a width, a precision or a positional argument number ending in '$'.
                        int64_t number = 0;
                        int32_t limit = position <= (int32_t)formatLength ? (int32_t)formatLength : position;
                        OCFIndex remaining = (OCFIndex)limit - position;
                        while (true) {
                            if (__builtin_mul_overflow(number, 10, &number) ||
                                __builtin_add_overflow(number, (int64_t)character - '0', &number)) {
                                goto overflow;
                            }
                            if (remaining == 0) {
                                key = NULL;
                                position = limit;
                                goto truncatedAtEnd;
                            }
                            character = cformat != NULL ? cformat[position] : uformat[position];
                            --remaining;
                            ++position;
                            if (character - '0' >= 10) {
                                break;
                            }
                        }
                        if (character == '$') {
                            if (number >= 128) {
                                if (errorPtr != NULL) {
                                    setFormatError(
                                        formattingAllocator(), errorPtr, OF_STR("Overflow occurred")
                                    );
                                }
                                result = false;
                                goto cleanupWithBuffers;
                            }
                            if (precArgNum == 0xfe) {
                                precArgNum = (uint8_t)(number - 1);
                                spec->precArgNum = (int8_t)(number - 1);
                            } else if (widthArgNum == 0xfe) {
                                spec->widthArgNum = (int8_t)(number - 1);
                                widthArgNum = (uint8_t)(number - 1);
                            } else {
                                spec->mainArgNum = (int8_t)(number - 1);
                            }
                            goto nextCharacter;
                        }
                        if (number >= 0x80000000) {
                            goto overflow;
                        }
                        if (sawPeriod) {
                            spec->precArg = (int32_t)number;
                        } else {
                            spec->widthArg = (int32_t)number;
                        }
                        goto parseCharacter;
                    }
                nextCharacter:
                    if (position < (int32_t)formatLength) {
                        continue;
                    }
                    key = NULL;
                    goto truncatedAtEnd;
                }
            }
        truncated:
            key = NULL;
        truncatedAtEnd:
            spec->type = 0x2b;
        configure:
            if ((spec->flags & 0x10) == 0) {
                goto recordLength;
            }
            {
                OCFDictionaryRef dictionary;
                OCFDictionaryRef *slot;
                if ((spec->flags & 0x40) != 0) {
                    if (key == NULL) {
                        configs[configCount] = bracketDictionary;
                        dictionary = bracketDictionary;
                        goto haveDictionary;
                    }
                    OCFStringRef ruleName;
                    if (OCFStringCompareWithOptionsAndLocale(
                            key, OF_STR("tt"), (OCFRange){0, OCFStringGetLength(key)}, 0, NULL
                        ) == kOCFCompareEqualTo) {
                        ruleName = OF_STR("TechnicalTerm");
                    } else if (
                        OCFStringFindWithOptionsAndLocale(
                            key, OF_STR("VocativeName"), (OCFRange){0, OCFStringGetLength(key)}, 8, NULL, NULL
                        )
                    ) {
                        ruleName = OF_STR("VocativeName");
                    } else {
                        bracketDictionary = NULL;
                        slot = &configs[configCount];
                        *slot = NULL;
                        goto releaseKey;
                    }
                    OCFMutableDictionaryRef mapping =
                        _CFStringGetInputIdentifierFormatterMappingFromDescriptor(OCFDictionaryGetValue(
                            _CFStringLocalizedFormattingInfo, kOCFStringFormatFamilyInfoKey
                        ));
                    OCFDictionaryRef rule = OCFDictionaryGetValue(
                        OCFDictionaryGetValue(_CFStringLocalizedFormattingInfo, kOCFStringFormatRuleInfoKey),
                        ruleName
                    );
                    OCFMutableDictionaryRef copy = OCFDictionaryCreateMutableCopy(
                        kOCFAllocatorSystemDefault, OCFDictionaryGetCount(rule), rule
                    );
                    OCFDictionaryAddValue(copy, kOCFStringFormatFamilyInfoKey, mapping);
                    bracketDictionary = copy;
                    slot = &configs[configCount];
                    *slot = copy;
                } else if (configuration != NULL && key != NULL) {
                    slot = &configs[configCount];
                    *slot = OCFDictionaryGetValue(configuration, key);
                } else {
                    slot = &configs[configCount];
                    *slot = NULL;
                    if (key == NULL) {
                        spec->type = 0x20;
                        goto recordLength;
                    }
                }
            releaseKey:
                OCFRelease(key);
                dictionary = *slot;
            haveDictionary:
                if (dictionary != NULL) {
                    spec->configDictIndex = (int8_t)configCount;
                    OCFTypeRef ruleType = OCFDictionaryGetValue(dictionary, kOCFStringFormatSpecTypeKey);
                    OCFTypeRef valueType = OCFDictionaryGetValue(dictionary, kOCFStringFormatValueTypeKey);
                    ++configCount;
                    if (ruleType != NULL && OCFEqual(ruleType, kOCFStringPluralRuleType) &&
                        valueType == NULL) {
                        spec->type = 0x2c;
                    }
                } else {
                    spec->type = 0x20;
                }
            }
        recordLength:
            spec->len = position - start;
        nextSpec:
            ++specIndex;
            if (formatLength <= position) {
                break;
            }
        }
        specCount = (int32_t)specIndex;
    }

    // Argument numbering and value types.
    int32_t valuesCount = 3 * (percentCount + (int32_t)initialArgPosition) + 1;
    values = stackValues;
    if (valuesCount >= 62) {
        values = OCFAllocatorAllocateTyped(
            allocator, (OCFIndex)((uint64_t)(uint32_t)valuesCount << 4), 0x1082040f23858eb, 0
        );
        if (values != stackValues && __OCFOASafe) {
            __OCFSetLastAllocationEventName(values, "CFString (temp)");
        }
    }
    memset(values, 0, (size_t)(int64_t)valuesCount << 4);
    char *nestedArguments = configCount >= 1 ? argumentCursor : NULL;
    OCFIndex nextArgument = initialArgPosition;
    if (specCount != 0) {
        bool validatesWithConfiguration = validFormatSpecifiers != NULL && configuration != NULL;
        int32_t next = (int32_t)initialArgPosition;
        for (OCFIndex index = 0; index != specCount; ++index) {
            FormatSpecRecord *spec = &specs[index];
            uint32_t type = (uint16_t)spec->type;
            if (type <= 0x2c && ((UINT64_C(1) << type) & 0x180100000001) != 0) {
                continue;
            }
            int32_t highest = valuesCount <= spec->mainArgNum ? spec->mainArgNum : valuesCount - 1;
            if (highest <= spec->precArgNum) {
                highest = spec->precArgNum;
            }
            if (highest <= spec->widthArgNum) {
                highest = spec->widthArgNum;
            }
            if (valuesCount <= highest) {
                int32_t grown = 3 * highest + 1;
                if (highest >= 21) {
                    PrintValue *larger = OCFAllocatorAllocateTyped(
                        allocator, (OCFIndex)((uint64_t)(uint32_t)grown << 4), 0x1082040f23858eb, 0
                    );
                    if (larger != stackValues) {
                        if (__OCFOASafe) {
                            __OCFSetLastAllocationEventName(larger, "CFString (temp)");
                        }
                        memcpy(larger, values, (size_t)(int64_t)valuesCount << 4);
                        memset(&larger[valuesCount], 0, (size_t)(grown - valuesCount) << 4);
                        if (values != stackValues) {
                            OCFAllocatorDeallocate(allocator, values);
                        }
                        values = larger;
                    }
                }
                valuesCount = grown;
            }
            if ((uint8_t)spec->widthArgNum == 0xfe) {
                spec->widthArgNum = (int8_t)next;
                if (argumentNumberOverflows(next)) {
                    goto argumentOverflow;
                }
                ++next;
            }
            if ((uint8_t)spec->precArgNum == 0xfe) {
                spec->precArgNum = (int8_t)next;
                if (argumentNumberOverflows(next)) {
                    goto argumentOverflow;
                }
                ++next;
            }
            int8_t main = spec->mainArgNum;
            if ((uint8_t)main == 0xff) {
                spec->mainArgNum = (int8_t)next;
                if (argumentNumberOverflows(next)) {
                argumentOverflow:
                    if (errorPtr != NULL) {
                        setFormatError(formattingAllocator(), errorPtr, OF_STR("Overflow occurred"));
                    }
                    result = false;
                    goto cleanupWithBuffers;
                }
                main = (int8_t)next;
                ++next;
            }
            PrintValue *value = &values[main];
            value->size = spec->size;
            value->type = spec->type;
            if (spec->configDictIndex == -1) {
                if (value->type != 0x20 && validatesWithConfiguration) {
                    OCFIndex location = spec->loc;
                    OCFStringRef specString;
                    if (cformat != NULL) {
                        specString = __CFStringCreateImmutableFunnel3(
                            allocator,
                            cformat + location,
                            spec->len,
                            eightBitEncoding(),
                            0x12,
                            kOCFAllocatorNull,
                            0
                        );
                    } else {
                        specString = __CFStringCreateImmutableFunnel3(
                            allocator,
                            uformat + location,
                            2 * (OCFIndex)spec->len,
                            0x100,
                            0x10,
                            kOCFAllocatorNull,
                            0
                        );
                    }
                    if (specString == NULL) {
                        setFormatError(
                            allocator,
                            errorPtr,
                            OF_STR("Unable to allocate CFString to validate format specifiers")
                        );
                        result = false;
                        goto cleanupWithBuffers;
                    }
                    nextArgument =
                        __CFStringValidateFormat(validFormatSpecifiers, specString, nextArgument, errorPtr);
                    OCFRelease(specString);
                    if (nextArgument < 0) {
                        result = false;
                        goto cleanupWithBuffers;
                    }
                }
            } else {
                OCFStringRef valueType =
                    OCFDictionaryGetValue(configs[spec->configDictIndex], kOCFStringFormatValueTypeKey);
                if (valueType != NULL) {
                    const UniChar *characters = OCFStringGetCharactersPtr(valueType);
                    OCFStringEncoding encoding = __OCFDefaultEightBitStringEncoding;
                    if (encoding == (OCFStringEncoding)-1) {
                        encoding = __OCFStringComputeEightBitStringEncoding();
                    }
                    const char *cValueType = _CFStringGetCStringPtrInternal(valueType, encoding, true, true);
                    OCFIndex valueTypeLength = OCFStringGetLength(valueType);
                    UniChar *allocatedCharacters = NULL;
                    const UniChar *uValueType = characters;
                    if (characters == NULL && cValueType == NULL) {
                        allocatedCharacters =
                            OCFAllocatorAllocateTyped(allocator, 2 * valueTypeLength, 0x1000040bdfb0063, 0);
                        OCFStringGetCharacters(
                            valueType, (OCFRange){0, valueTypeLength}, allocatedCharacters
                        );
                        uValueType = allocatedCharacters;
                    }
                    // The value type is parsed as one conversion: its size and type replace the
                    // specification's.
                    int16_t valueSize = 0;
                    uint16_t valueKind;
                    if ((int32_t)valueTypeLength < 1) {
                        valueSize = 0;
                        valueKind = 0x2b;
                        goto valueTypeParsed;
                    }
                    {
                        bool inBracket = false;
                        bool hash = false;
                        int32_t cursor = 0;
                        OCFIndex keyStart = -1;
                        uint8_t widthMark = 0xff;
                        uint8_t precisionMark = 0xff;
                        uint32_t c;
                        while (true) {
                            c = cValueType != NULL ? (uint8_t)cValueType[cursor] : uValueType[cursor];
                            ++cursor;
                            if (keyStart >= 0) {
                                bool identifier =
                                    c >= '0' && c - ':' >= 7 && c <= 'z' && (c == '_' || c - '[' >= 6);
                                if (!identifier && c != ']') {
                                    if (c == '@') {
                                        valueSize = 4;
                                        valueKind = 0x25;
                                        goto valueTypeParsed;
                                    }
                                    keyStart = -1;
                                }
                                goto nextValueCharacter;
                            }
                        valueSwitch:
                            valueKind = 0x20;
                            switch (c) {
                                case ' ':
                                case '+':
                                case '-':
                                case '0':
                                    goto nextValueCharacter;
                                case '#':
                                    hash = true;
                                    goto nextValueCharacter;
                                case '*':
                                    widthMark = 0xfe;
                                    goto nextValueCharacter;
                                case '.':
                                    if (cursor >= (int32_t)valueTypeLength) {
                                        valueKind = 0x2b;
                                        goto valueTypeParsed;
                                    }
                                    c = cValueType != NULL ? (uint8_t)cValueType[cursor] : uValueType[cursor];
                                    ++cursor;
                                    if (c != '*') {
                                        goto valueSwitch;
                                    }
                                    precisionMark = 0xfe;
                                    goto nextValueCharacter;
                                case '1':
                                case '2':
                                case '3':
                                case '4':
                                case '5':
                                case '6':
                                case '7':
                                case '8':
                                case '9': {
                                    int64_t number = 0;
                                    while (true) {
                                        if (__builtin_mul_overflow(number, 10, &number) ||
                                            __builtin_add_overflow(number, (int64_t)c - '0', &number)) {
                                            valueKind = 0;
                                            goto valueTypeParsed;
                                        }
                                        if (cursor >= (int32_t)((uint32_t)valueTypeLength & 0x7fffffff)) {
                                            valueKind = 0x2b;
                                            goto valueTypeParsed;
                                        }
                                        c = cValueType != NULL ? (uint8_t)cValueType[cursor]
                                                               : uValueType[cursor];
                                        ++cursor;
                                        if (c - '0' >= 10) {
                                            break;
                                        }
                                    }
                                    if (c != '$') {
                                        if (number <= 0x7fffffff) {
                                            goto valueSwitch;
                                        }
                                        valueKind = 0;
                                        goto valueTypeParsed;
                                    }
                                    if (number > 127) {
                                        valueKind = 0;
                                        goto valueTypeParsed;
                                    }
                                    if (precisionMark == 0xfe) {
                                        precisionMark = (uint8_t)(number - 1);
                                    } else if (widthMark == 0xfe) {
                                        widthMark = (uint8_t)(number - 1);
                                    }
                                    goto nextValueCharacter;
                                }
                                case '@':
                                    if (!hash) {
                                        valueSize = 4;
                                        valueKind = 0x25;
                                        goto valueTypeParsed;
                                    }
                                    hash = false;
                                    keyStart = cursor;
                                    goto nextValueCharacter;
                                case 'A':
                                case 'a':
                                    valueSize = valueSize == 5 ? 5 : 4;
                                    valueKind = 0x22;
                                    goto valueTypeParsed;
                                case 'C':
                                    valueSize = 2;
                                    valueKind = 0x29;
                                    goto valueTypeParsed;
                                case 'D':
                                case 'U':
                                case 'd':
                                case 'i':
                                case 'u':
                                    _OCFExecutableLinkedOnOrAfter(8);
                                    valueKind = 0x21;
                                    goto valueTypeParsed;
                                case 'E':
                                case 'F':
                                case 'G':
                                case 'e':
                                case 'f':
                                case 'g':
                                    _OCFExecutableLinkedOnOrAfter(8);
                                    valueSize = valueSize == 5 ? 5 : 4;
                                    valueKind = 0x22;
                                    goto valueTypeParsed;
                                case 'L':
                                    valueSize = 5;
                                    goto nextValueCharacter;
                                case 'O':
                                case 'X':
                                case 'o':
                                case 'x':
                                    valueKind = 0x21;
                                    goto valueTypeParsed;
                                case 'P':
                                    valueSize = 4;
                                    valueKind = 0x28;
                                    goto valueTypeParsed;
                                case 'S':
                                    valueSize = 4;
                                    valueKind = 0x26;
                                    goto valueTypeParsed;
                                case '[':
                                    if (!inBracket) {
                                        keyStart = cursor;
                                    }
                                    inBracket = true;
                                    goto nextValueCharacter;
                                case 'c':
                                    valueSize = 1;
                                    valueKind = 0x21;
                                    goto valueTypeParsed;
                                case 'h':
                                    if (cursor >= (int32_t)valueTypeLength) {
                                        valueSize = 2;
                                        goto nextValueCharacter;
                                    }
                                    if ((cValueType != NULL ? (uint8_t)cValueType[cursor]
                                                            : uValueType[cursor]) == 'h') {
                                        ++cursor;
                                        valueSize = 1;
                                    } else {
                                        valueSize = 2;
                                    }
                                    goto checkValueEnd;
                                case 'j':
                                case 'q':
                                case 't':
                                case 'z':
                                    valueSize = OF_PORT_ANSWER(formatIntegerSize, 4, c);
                                    goto nextValueCharacter;
                                case 'l':
                                    if (cursor >= (int32_t)valueTypeLength) {
                                        valueSize = OF_PORT_ANSWER(
                                            formatLongSize, 4, (const uint8_t *)cValueType, uValueType, cursor
                                        );
                                        goto nextValueCharacter;
                                    }
                                    if ((cValueType != NULL ? (uint8_t)cValueType[cursor]
                                                            : uValueType[cursor]) == 'l') {
                                        ++cursor;
                                    }
                                    valueSize = OF_PORT_ANSWER(
                                        formatLongSize, 4, (const uint8_t *)cValueType, uValueType, cursor
                                    );
                                    goto checkValueEnd;
                                case 'n':
                                    valueSize = 4;
                                    valueKind = 0x2a;
                                    goto valueTypeParsed;
                                case 'p':
                                    valueSize = 4;
                                    valueKind = 0x23;
                                    goto valueTypeParsed;
                                case 's':
                                    valueSize = 4;
                                    valueKind = 0x27;
                                    goto valueTypeParsed;
                                default:
                                    goto valueTypeParsed;
                            }
                        nextValueCharacter:
                        checkValueEnd:
                            if (cursor >= (int32_t)valueTypeLength) {
                                valueKind = 0x2b;
                                goto valueTypeParsed;
                            }
                        }
                    }
                valueTypeParsed:
                    if (allocatedCharacters != NULL) {
                        OCFAllocatorDeallocate(allocator, allocatedCharacters);
                    }
                    if (validFormatSpecifiers != NULL) {
                        OCFRange percent = OCFStringFind(valueType, OF_STR("%"), 0);
                        if (percent.location != -1 || percent.length != 0) {
                            setFormatError(
                                allocator, errorPtr, OF_STR("Invalid format for NSStringFormatValueTypeKey")
                            );
                            result = false;
                            goto cleanupWithBuffers;
                        }
                        if (((UINT64_C(0xc0100000000) >> valueKind) & 1) == 0) {
                            FormatSpecRecord valueSpec = *spec;
                            valueSpec.size = valueSize;
                            valueSpec.type = (int16_t)valueKind;
                            nextArgument =
                                __CFStringValidateFormatSpec(validFormatSpecifiers, &valueSpec, nextArgument);
                            if (nextArgument < 0) {
                                OCFIndex location = spec->loc;
                                OCFStringRef specString;
                                if (cformat != NULL) {
                                    specString = __CFStringCreateImmutableFunnel3(
                                        allocator,
                                        cformat + location,
                                        spec->len,
                                        eightBitEncoding(),
                                        0x12,
                                        kOCFAllocatorNull,
                                        0
                                    );
                                } else {
                                    specString = __CFStringCreateImmutableFunnel3(
                                        allocator,
                                        uformat + location,
                                        2 * (OCFIndex)spec->len,
                                        0x100,
                                        0x10,
                                        kOCFAllocatorNull,
                                        0
                                    );
                                }
                                OCFStringRef description = OCFStringCreateWithFormat(
                                    allocator,
                                    NULL,
                                    OF_STR(
                                        "NSStringFormatValueTypeKey '%@' (referenced by specifier '%@') does "
                                        "not "
                                        "match expected specifiers '%@'"
                                    ),
                                    valueType,
                                    specString,
                                    validFormatSpecifiers
                                );
                                OCFMutableDictionaryRef userInfo = OCFDictionaryCreateMutable(
                                    allocator,
                                    0,
                                    &kOCFCopyStringDictionaryKeyCallBacks,
                                    &kOCFTypeDictionaryValueCallBacks
                                );
                                OCFDictionarySetValue(userInfo, kOCFErrorDebugDescriptionKey, description);
                                *errorPtr = OCFErrorCreate(allocator, kOCFErrorDomainCocoa, 2048, userInfo);
                                OCFRelease(userInfo);
                                OCFRelease(description);
                                OCFRelease(specString);
                                result = false;
                                goto cleanupWithBuffers;
                            }
                        }
                    }
                    PrintValue *typed = &values[spec->mainArgNum];
                    typed->size = valueSize;
                    typed->type = (int16_t)valueKind;
                }
            }
            if (spec->widthArgNum != -1) {
                *(int32_t *)&values[spec->widthArgNum] = 0x21;
            }
            if (spec->precArgNum != -1) {
                *(int32_t *)&values[spec->precArgNum] = 0x21;
            }
        }
    }

    // Collecting the arguments.
    if (valuesCount >= 1) {
        uint32_t lastUsed = 0;
        for (int32_t index = 0; index != valuesCount; ++index) {
            if (originalValues != NULL && originalValuesSize > index && values[index].type == 0) {
                values[index] = originalValues[index];
            }
            uint32_t type = (uint16_t)values[index].type;
            if (type > 0x2c) {
                __builtin_trap();
            }
            if (((UINT64_C(1) << type) & 0x7ee00000000) != 0) {
                lastUsed = (uint32_t)index;
            } else if (((UINT64_C(1) << type) & 0x180100000001) == 0) {
                __builtin_trap();
            }
        }
        for (uint64_t index = 0; index != (uint32_t)valuesCount; ++index) {
            PrintValue *value = &values[index];
            uint32_t type = (uint16_t)value->type;
            switch (type) {
                case 0: {
                    bool skips = dyld_program_sdk_at_least((OCFBuildVersion){0xffffffff, 0x7e60901});
                    if (index < lastUsed && skips) {
                        argumentCursor += OF_PORT_ANSWER(formatArgumentSlotSize, 8, );
                    }
                    break;
                }
                case 0x21:
                case 0x29:
                    switch ((uint16_t)value->size) {
                        case 1:
                            value->value.int64Value =
                                OF_PORT_ANSWER(formatReadInt8, *(int8_t *)argumentCursor, &argumentCursor);
                            break;
                        case 2:
                            value->value.int64Value =
                                OF_PORT_ANSWER(formatReadInt16, *(int16_t *)argumentCursor, &argumentCursor);
                            break;
                        case 4:
                            value->value.int64Value =
                                OF_PORT_ANSWER(formatReadInt64, *(int64_t *)argumentCursor, &argumentCursor);
                            break;
                        default:
                            value->value.int64Value =
                                OF_PORT_ANSWER(formatReadInt32, *(int32_t *)argumentCursor, &argumentCursor);
                            break;
                    }
                    OF_PORT_NATIVE(argumentCursor += 8);
                    break;
                case 0x22:
                    value->value.int64Value =
                        OF_PORT_ANSWER(formatReadDoubleBits, *(int64_t *)argumentCursor, &argumentCursor);
                    OF_PORT_NATIVE(argumentCursor += 8);
                    break;
                case 0x23:
                case 0x25:
                case 0x26:
                case 0x27:
                case 0x28:
                    value->value.int64Value =
                        OF_PORT_ANSWER(formatReadPointer, *(int64_t *)argumentCursor, &argumentCursor);
                    OF_PORT_NATIVE(argumentCursor += 8);
                    break;
                case 0x2a:
                    argumentCursor += OF_PORT_ANSWER(formatArgumentSlotSize, 8, );
                    value->value.pointerValue = &dummyCount;
                    break;
                default:
                    break;
            }
        }
    }
    const PrintValue *nestedValues = originalValues != NULL ? originalValues : values;
    OCFIndex nestedValuesCount = originalValues != NULL ? originalValuesSize : valuesCount;
    if (specCount == 0) {
        free(NULL);
        result = true;
        goto cleanupWithBuffers;
    }

    // Output.
    {
        FormatSpecRecord *deferred = malloc_type_calloc((uint32_t)specCount, 32, 0x10000408e5dae1c);
        int32_t deferredCount = 0;
        bool previousUsedIsolates = false;
        OCFIndex lastAppendedLength = 0;
        for (OCFIndex index = 0; index != specCount; ++index) {
            FormatSpecRecord *spec = &specs[index];
            int32_t argumentWidth = 0;
            bool hasArgumentWidth = false;
            if (spec->widthArgNum != -1) {
                argumentWidth = *(const int32_t *)&values[spec->widthArgNum].value;
                hasArgumentWidth = true;
            }
            int32_t argumentPrecision = 0;
            bool hasArgumentPrecision = false;
            if (spec->precArgNum != -1) {
                argumentPrecision = *(const int32_t *)&values[spec->precArgNum].value;
                hasArgumentPrecision = true;
            }
            int32_t width = spec->widthArg == -1 ? argumentWidth : spec->widthArg;
            bool hasWidth = spec->widthArg == -1 ? hasArgumentWidth : true;
            int32_t precision = spec->precArg == -1 ? argumentPrecision : spec->precArg;
            bool hasPrecision = spec->precArg == -1 ? hasArgumentPrecision : true;
            uint32_t type = (uint16_t)spec->type;
            OCFIndex replacementStart = 0;
            OCFIndex replacementEnd = 0;
            uint32_t addedIsolates = 0;
            FormatSpecRecord specCopy;
#define mainValue (&values[spec->mainArgNum])
            switch (type) {
                case 0x20:
                    if (outReplacementMetadata != NULL) {
                        replacementStart = OCFStringGetLength(outputString);
                    }
                    if (cformat != NULL) {
                        __OCFStringAppendBytes(
                            outputString, (const char *)cformat + spec->loc, spec->len, eightBitEncoding()
                        );
                    } else {
                        OCFStringAppendCharacters(outputString, uformat + spec->loc, spec->len);
                    }
                    if (outReplacementMetadata != NULL) {
                        replacementEnd = OCFStringGetLength(outputString);
                    }
                    goto appendMetadata;
                case 0x21:
                case 0x22:
                    if (formatOptionsIsLocale && (spec->flags & 0x20) != 0) {
                        if (outReplacementMetadata != NULL) {
                            replacementStart = OCFStringGetLength(outputString);
                            if (!__CFStringFormatLocalizedNumber(
                                    outputString, formatOptions, values, spec, width, precision, hasPrecision
                                )) {
                                goto printfConversion;
                            }
                            replacementEnd = OCFStringGetLength(outputString);
                        } else if (!__CFStringFormatLocalizedNumber(
                                       outputString,
                                       formatOptions,
                                       values,
                                       spec,
                                       width,
                                       precision,
                                       hasPrecision
                                   )) {
                            goto printfConversion;
                        }
                        goto appendMetadata;
                    }
                    goto printfConversion;
                case 0x23:
                printfConversion:
                    {
                        int32_t specLength = spec->len;
                        char *formatBuffer = stackFormatBuffer;
                        char *allocatedFormat = NULL;
                        if ((uint32_t)(specLength + 1) >= 0x81) {
                            allocatedFormat = OCFAllocatorAllocateTyped(
                                kOCFAllocatorSystemDefault, specLength + 1, 0x100004077774924, 0
                            );
                            formatBuffer = allocatedFormat;
                        }
                        char *outputBuffer = stackOutputBuffer;
                        char *allocatedOutput = NULL;
                        size_t outputSize = 528;
                        if (precision + width >= 17) {
                            outputSize = (size_t)(int64_t)(precision + width + 512);
                            allocatedOutput = OCFAllocatorAllocateTyped(
                                kOCFAllocatorSystemDefault, (OCFIndex)outputSize, 0x100004077774924, 0
                            );
                            outputBuffer = allocatedOutput;
                        }
                        // The specification text with any "N$" argument numbers removed.
                        int32_t length = spec->len;
                        int32_t out = 0;
                        for (int32_t offset = 0; offset < length; ++offset) {
                            uint32_t c =
                                cformat != NULL ? cformat[spec->loc + offset] : uformat[spec->loc + offset];
                            if (c == '$') {
                                if (out >= 0) {
                                    uint32_t previous;
                                    do {
                                        previous = (uint8_t)formatBuffer[out - 1];
                                        --out;
                                    } while (previous - '0' < 10);
                                }
                            } else {
                                formatBuffer[out] = (char)c;
                                length = spec->len;
                            }
                            ++out;
                        }
                        formatBuffer[out] = 0;
                        outputBuffer[outputSize - 1] = 0;
                        if (outReplacementMetadata != NULL) {
                            replacementStart = OCFStringGetLength(outputString);
                        }
                        const PrintValue *argument = &values[spec->mainArgNum];
                        bool appended = false;
                        if (type == 0x21 || type == 0x23 || type == 0x2a) {
                            int64_t integer = argument->value.int64Value;
                            if ((uint8_t)spec->widthArgNum == 0xff) {
                                if ((uint8_t)spec->precArgNum == 0xff) {
                                    snprintf_l(outputBuffer, outputSize, NULL, formatBuffer, integer);
                                } else {
                                    snprintf_l(
                                        outputBuffer, outputSize, NULL, formatBuffer, precision, integer
                                    );
                                }
                            } else if ((uint8_t)spec->precArgNum == 0xff) {
                                snprintf_l(outputBuffer, outputSize, NULL, formatBuffer, width, integer);
                            } else {
                                snprintf_l(
                                    outputBuffer, outputSize, NULL, formatBuffer, width, precision, integer
                                );
                            }
                        } else if (type == 0x22) {
                            double number = argument->value.doubleValue;
                            if ((uint8_t)spec->widthArgNum == 0xff) {
                                if ((uint8_t)spec->precArgNum == 0xff) {
                                    snprintf_l(outputBuffer, outputSize, NULL, formatBuffer, number);
                                } else {
                                    snprintf_l(
                                        outputBuffer, outputSize, NULL, formatBuffer, precision, number
                                    );
                                }
                            } else if ((uint8_t)spec->precArgNum == 0xff) {
                                snprintf_l(outputBuffer, outputSize, NULL, formatBuffer, width, number);
                            } else {
                                snprintf_l(
                                    outputBuffer, outputSize, NULL, formatBuffer, width, precision, number
                                );
                            }
                            if (formatOptions != NULL) {
                                OCFStringRef separator;
                                if (OCFGetTypeID(formatOptions) == OCFLocaleGetTypeID()) {
                                    separator =
                                        OCFLocaleGetValue(formatOptions, kOCFLocaleDecimalSeparatorKey);
                                } else {
                                    separator =
                                        OCFDictionaryGetValue(formatOptions, OF_STR("NSDecimalSeparator"));
                                }
                                if (separator != NULL) {
                                    for (char *cursor = outputBuffer + 1; cursor[-1] != 0; ++cursor) {
                                        if (cursor[-1] == '.') {
                                            cursor[-1] = 0;
                                            OCFStringAppendCString(
                                                outputString, outputBuffer, eightBitEncoding()
                                            );
                                            OCFStringAppend(outputString, separator);
                                            OCFStringAppendCString(outputString, cursor, eightBitEncoding());
                                            appended = true;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        if (!appended) {
                            OCFStringAppendCString(outputString, outputBuffer, eightBitEncoding());
                        }
                        if (outReplacementMetadata != NULL) {
                            replacementEnd = OCFStringGetLength(outputString);
                        }
                        specCopy = *spec;
                        _CFStringFormatAppendMetadata(
                            metadataSlot,
                            specCount,
                            values,
                            valuesCount,
                            &specCopy,
                            replacementStart,
                            replacementEnd,
                            0
                        );
                        if (allocatedOutput != NULL) {
                            OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, allocatedOutput);
                        }
                        if (allocatedFormat != NULL) {
                            OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, allocatedFormat);
                        }
                        continue;
                    }
                case 0x26: {
                    if (outReplacementMetadata != NULL) {
                        replacementStart = OCFStringGetLength(outputString);
                    }
                    const UniChar *characters = mainValue->value.pointerValue;
                    if (characters != NULL) {
                        int32_t count;
                        if (hasPrecision) {
                            count = 0;
                            if (precision >= 1) {
                                while (characters[count] != 0) {
                                    if (precision == ++count) {
                                        count = precision;
                                        break;
                                    }
                                }
                            }
                        } else {
                            count = -1;
                            const UniChar *cursor = characters;
                            UniChar c;
                            do {
                                c = *cursor++;
                                ++count;
                            } while (c != 0);
                        }
                        if ((spec->flags & 2) != 0) {
                            OCFStringAppendCharacters(outputString, characters, (uint32_t)count);
                            if (hasWidth && width > count) {
                                appendSpaces(outputString, width - count);
                            }
                        } else {
                            if (hasWidth && width > count) {
                                appendSpaces(outputString, width - count);
                            }
                            OCFStringAppendCharacters(outputString, characters, (uint32_t)count);
                        }
                    } else {
                        OCFStringAppendCString(outputString, "(null)", 0x600);
                    }
                    if (outReplacementMetadata != NULL) {
                        replacementEnd = OCFStringGetLength(outputString);
                    }
                    goto appendMetadata;
                }
                case 0x27:
                case 0x28: {
                    if (outReplacementMetadata != NULL) {
                        replacementStart = OCFStringGetLength(outputString);
                    }
                    const char *bytes = mainValue->value.pointerValue;
                    if (bytes == NULL) {
                        OCFStringAppendCString(outputString, "(null)", 0x600);
                    } else {
                        int32_t count;
                        if (spec->type == 0x28) {
                            int32_t pascalLength = (uint8_t)*bytes++;
                            int32_t limited = precision >= pascalLength ? pascalLength : precision;
                            count = hasPrecision ? limited : pascalLength;
                        } else if (!hasPrecision) {
                            if ((intptr_t)bytes < 0 && _objc_getTaggedPointerTag(bytes) == 0 &&
                                !dyld_program_sdk_at_least((OCFBuildVersion){0xffffffff, 0x7e90000})) {
                                bytes = taggedCStringArgument(bytes);
                            }
                            count = (int32_t)strlen(bytes);
                        } else {
                            const char *terminator = memchr(bytes, 0, (size_t)(int64_t)precision);
                            count = terminator != NULL ? (int32_t)(terminator - bytes) : precision;
                        }
                        if ((spec->flags & 2) != 0) {
                            __OCFStringAppendBytes(outputString, bytes, count, systemEncoding());
                            if (hasWidth && width > count) {
                                appendSpaces(outputString, width - count);
                            }
                        } else {
                            if (hasWidth && width > count) {
                                appendSpaces(outputString, width - count);
                            }
                            __OCFStringAppendBytes(outputString, bytes, count, systemEncoding());
                        }
                    }
                    if (outReplacementMetadata != NULL) {
                        replacementEnd = OCFStringGetLength(outputString);
                    }
                    goto appendMetadata;
                }
                case 0x29: {
                    if (outReplacementMetadata != NULL) {
                        replacementStart = OCFStringGetLength(outputString);
                    }
                    UniChar character = (UniChar)mainValue->value.int64Value;
                    OCFStringAppendCharacters(outputString, &character, 1);
                    if (outReplacementMetadata != NULL) {
                        replacementEnd = OCFStringGetLength(outputString);
                    }
                    goto appendMetadata;
                }
                case 0x2b:
                    if (outReplacementMetadata != NULL) {
                        replacementStart = OCFStringGetLength(outputString);
                    }
                    replacementEnd = replacementStart;
                    goto appendMetadata;
                case 0x25:
                case 0x2c: {
                    if (outReplacementMetadata != NULL) {
                        replacementStart = OCFStringGetLength(outputString);
                    }
                    if ((uint8_t)spec->configDictIndex != 0xff) {
                        OCFTypeRef object = NULL;
                        if (spec->type == 0x2c) {
                            if (implicitArgument != NULL) {
                                object = OCFRetain(implicitArgument);
                            }
                        } else {
                            switch ((uint16_t)mainValue->type) {
                                case 0x21:
                                    object = OCFNumberCreate(allocator, 4, &mainValue->value);
                                    break;
                                case 0x22: {
                                    const void *number = &mainValue->value;
                                    double copied;
                                    if (mainValue->size == 5) {
                                        copied = mainValue->value.doubleValue;
                                        number = &copied;
                                    }
                                    object = OCFNumberCreate(allocator, 13, number);
                                    break;
                                }
                                case 0x23:
                                    object = OCFNumberCreate(allocator, 14, &mainValue->value);
                                    break;
                                case 0x25:
                                    if (mainValue->value.pointerValue != NULL) {
                                        object = OCFRetain(mainValue->value.pointerValue);
                                    }
                                    break;
                                case 0x26: {
                                    const UniChar *characters = mainValue->value.pointerValue;
                                    if (characters == NULL) {
                                        break;
                                    }
                                    OCFMutableStringRef string = OCFStringCreateMutable(allocator, 0);
                                    int32_t full = -1;
                                    const UniChar *cursor = characters;
                                    UniChar c;
                                    do {
                                        c = *cursor++;
                                        ++full;
                                    } while (c != 0);
                                    int32_t limited = precision >= full ? full : precision;
                                    int32_t count = hasPrecision ? limited : full;
                                    if ((spec->flags & 2) != 0) {
                                        OCFStringAppendCharacters(string, characters, count);
                                        if (hasWidth && width > count) {
                                            appendSpaces(string, width - count);
                                        }
                                    } else {
                                        if (hasWidth && width > count) {
                                            appendSpaces(string, width - count);
                                        }
                                        OCFStringAppendCharacters(string, characters, count);
                                    }
                                    object = string;
                                    break;
                                }
                                case 0x27:
                                case 0x28: {
                                    if (mainValue->value.pointerValue == NULL) {
                                        break;
                                    }
                                    OCFMutableStringRef string = OCFStringCreateMutable(allocator, 0);
                                    const char *bytes = mainValue->value.pointerValue;
                                    int32_t count;
                                    if (spec->type == 0x28) {
                                        int32_t pascalLength = (uint8_t)*bytes++;
                                        int32_t limited =
                                            precision >= pascalLength ? pascalLength : precision;
                                        count = hasPrecision ? limited : pascalLength;
                                    } else if (hasPrecision) {
                                        const char *terminator = memchr(bytes, 0, (size_t)(int64_t)precision);
                                        count =
                                            terminator != NULL ? (int32_t)(terminator - bytes) : precision;
                                    } else {
                                        count = (int32_t)strlen(bytes);
                                    }
                                    if ((spec->flags & 2) != 0) {
                                        __OCFStringAppendBytes(string, bytes, count, systemEncoding());
                                        if (hasWidth && width > count) {
                                            appendSpaces(string, width - count);
                                        }
                                    } else {
                                        if (hasWidth && width > count) {
                                            appendSpaces(string, width - count);
                                        }
                                        __OCFStringAppendBytes(string, bytes, count, systemEncoding());
                                    }
                                    object = string;
                                    break;
                                }
                                case 0x29: {
                                    UniChar character = (UniChar)mainValue->value.int64Value;
                                    object = __CFStringCreateImmutableFunnel3(
                                        allocator, &character, 2, 0x100, 0x10, kOCFAllocatorNull, 0
                                    );
                                    break;
                                }
                                default:
                                    break;
                            }
                        }
                        if (object == NULL) {
                            goto appendNull;
                        }
                        OCFTypeRef resolved = _CFCopyResolvedFormatStringWithConfiguration(
                            object, configs[spec->configDictIndex], formatOptions
                        );
                        OCFRelease(object);
                        if (resolved == NULL) {
                            goto appendNull;
                        }
                        OCFIndex nestedPosition;
                        if (spec->mainArgNum >= 0) {
                            nestedPosition = (uint8_t)spec->mainArgNum;
                        } else {
                            nestedPosition = 0;
                            for (OCFIndex previous = index; previous != 0; --previous) {
                                int8_t number = specs[previous - 1].mainArgNum;
                                if (number >= 0) {
                                    nestedPosition = (uint8_t)number + 1;
                                    break;
                                }
                            }
                        }
                        bool nested = __CFStringAppendFormatCore(
                            outputString,
                            copyDescFunc,
                            contextDescFunc,
                            formatOptions,
                            configuration,
                            NULL,
                            validFormatSpecifiers,
                            resolved,
                            nestedPosition,
                            nestingDepth + 1,
                            nestedValues,
                            nestedValuesCount,
                            (va_list)nestedArguments,
                            NULL,
                            errorPtr
                        );
                        OCFRelease(resolved);
                        if (!nested) {
                            free(deferred);
                            result = false;
                            goto cleanupWithBuffers;
                        }
                        addedIsolates = 0;
                        goto objectAppended;
                    }
                    const void *argument = mainValue->value.pointerValue;
                    if (argument == NULL) {
                        OCFStringAppendCString(outputString, "(null)", 0x600);
                        addedIsolates = 0;
                        goto objectAppended;
                    }
                    OCFStringRef description = NULL;
                    if (contextDescFunc != NULL) {
                        bool isDeferred = false;
                        description = __CFSTRING_IS_CALLING_OUT_TO_AN_OBJECT_FORMAT_ARGUMENT_WITH_CONTEXT__(
                            contextDescFunc, argument, formatString, false, &isDeferred
                        );
                        if (isDeferred) {
                            description = OCFRetain(OF_STR("%@NSCONTEXT"));
                            deferred[deferredCount] = *spec;
                            ++deferredCount;
                        }
                        if (description == NULL) {
                            argument = mainValue->value.pointerValue;
                        }
                    }
                    if (description == NULL) {
                        if (copyDescFunc != NULL) {
                            description =
                                __CFSTRING_IS_CALLING_OUT_TO_AN_OBJECT_FORMAT_ARGUMENT_WITH_LOCALE__(
                                    copyDescFunc, argument, formatOptions
                                );
                        } else {
                            description =
                                __CFSTRING_IS_CALLING_OUT_TO_AN_OBJECT_FORMAT_ARGUMENT_WITH_LOCALE__(
                                    (CopyDescriptionFunction)(void *)__OCFCopyFormattingDescription,
                                    argument,
                                    formatOptions
                                );
                            if (description == NULL) {
                                description = __CFSTRING_IS_CALLING_OUT_TO_AN_OBJECT_FORMAT_ARGUMENT__(
                                    mainValue->value.pointerValue
                                );
                            }
                        }
                        if (description == NULL) {
                            OCFStringAppendCString(outputString, "(null description)", 0x600);
                            addedIsolates = 0;
                            goto objectAppended;
                        }
                    }
                    if (disableIsolates == 0xff) {
                        disableIsolates =
                            !dyld_program_sdk_at_least((OCFBuildVersion){0xffffffff, 0x7e00901}) ||
                            __CFgetenv("CFStringDisableIsolates") != NULL;
                    }
                    if (disableIsolates == 0 && formatOptionsIsLocale) {
                        bool isDirectionalMark =
                            OCFStringGetLength(description) == 1 &&
                            (OCFStringGetCharacterAtIndex(description, 0) & 0xfffe) == 0x200e;
                        bool useIsolates;
                        if (previousUsedIsolates) {
                            useIsolates = true;
                        } else if (
                            _os_feature_enabled_impl(
                                "Foundation", "LanguageAwareStringStatisticalLocalizedStringWithFormat"
                            )
                        ) {
                            useIsolates =
                                __CFShouldUseIsolatesForAppendingValueWithStatisticalWritingDirections(
                                    description,
                                    outputString,
                                    &statisticalIsolates[0],
                                    &statisticalIsolates[1],
                                    isDirectionalMark,
                                    lastAppendedLength,
                                    specs,
                                    index,
                                    formatLength,
                                    formatString,
                                    formatOptions
                                );
                        } else {
                            useIsolates = __CFShouldUseIsolatesForAppendingValue(
                                description,
                                outputString,
                                lastAppendedLength,
                                specs,
                                index,
                                formatLength,
                                formatString
                            );
                        }
                        if (_os_feature_enabled_impl(
                                "Foundation", "LanguageAwareStringStatisticalLocalizedStringWithFormat"
                            )) {
                            __CFAppendFormattedValueWithStatisticalWritingDirections(
                                description, outputString, isDirectionalMark, useIsolates
                            );
                        } else {
                            __CFAppendFormattedValue(description, outputString, useIsolates);
                        }
                        if (useIsolates) {
                            previousUsedIsolates = true;
                            addedIsolates = 1;
                        } else {
                            lastAppendedLength = OCFStringGetLength(outputString);
                            previousUsedIsolates = false;
                            addedIsolates = 0;
                        }
                    } else {
                        OCFStringAppend(outputString, description);
                        addedIsolates = 0;
                    }
                    OCFRelease(description);
                    goto objectAppended;
                appendNull:
                    OCFStringAppendCString(outputString, "(null)", 0x600);
                    addedIsolates = 0;
                objectAppended:
                    if (outReplacementMetadata != NULL) {
                        replacementEnd = OCFStringGetLength(outputString);
                    }
                    goto appendMetadata;
                }
                default:
                    continue;
            }
#undef mainValue
        appendMetadata:
            specCopy = *spec;
            _CFStringFormatAppendMetadata(
                metadataSlot,
                specCount,
                values,
                valuesCount,
                &specCopy,
                replacementStart,
                replacementEnd,
                addedIsolates
            );
        }
        for (int32_t remaining = deferredCount, offset = 0; remaining > 0; --remaining, ++offset) {
            OCFRange range = OCFStringFind(outputString, OF_STR("%@NSCONTEXT"), 0);
            OCFStringRef description = __CFSTRING_IS_CALLING_OUT_TO_AN_OBJECT_FORMAT_ARGUMENT_WITH_CONTEXT__(
                contextDescFunc,
                values[deferred[offset].mainArgNum].value.pointerValue,
                outputString,
                true,
                NULL
            );
            if (description != NULL) {
                OCFStringReplace(outputString, range, description);
                OCFRelease(description);
            }
        }
        free(deferred);
        result = true;
        goto cleanupWithBuffers;
    }

overflow:
    if (errorPtr != NULL) {
        setFormatError(formattingAllocator(), errorPtr, OF_STR("Overflow occurred"));
    }
    result = false;
    if (specs == NULL) {
        goto cleanupUnconditional;
    }
cleanupWithBuffers:
    if (specs != stackSpecs) {
        OCFAllocatorDeallocate(allocator, specs);
    }
    goto cleanupValues;
cleanupUnconditional:
    // The early failures release the never-allocated buffers through CFAllocatorDeallocate with NULL, as
    // natively.
    OCFAllocatorDeallocate(allocator, specs);
cleanupValues:
    if (values != stackValues) {
        OCFAllocatorDeallocate(allocator, values);
    }
    if (uniBuffer != NULL && uniBuffer != stackUniBuffer) {
        OCFAllocatorDeallocate(allocator, uniBuffer);
    }
    if (configs != stackConfigs) {
        OCFAllocatorDeallocate(allocator, configs);
    }
    if (bracketDictionary != NULL) {
        OCFRelease(bracketDictionary);
    }
    if (outReplacementMetadata != NULL && metadataArray != NULL) {
        *outReplacementMetadata = OCFArrayCreateCopy(kOCFAllocatorSystemDefault, metadataArray);
        OCFRelease(metadataArray);
    }
    return result;
}

// Native CFStringCreateWithFormatAndArguments, 0x1c020e334. External.
// A format that is an Objective-C object (a tagged pointer or a bridged instance, __CF_IS_OBJC with type 7)
// of class __NSCFLocalizedAttributedString is handed to the inflection path; every other format is formatted
// with _CFStringCreateWithFormatAndArgumentsReturningMetadata without callbacks, configuration or metadata.
OCFStringRef OCFStringCreateWithFormatAndArguments(
    OCFAllocatorRef alloc, OCFDictionaryRef formatOptions, OCFStringRef format, va_list arguments
) {
    if (((intptr_t)format < 0 || __CF_IS_OBJC(7, format)) && _CFStringObjCFormatRequiresInflection(format)) {
        return _CFStringCreateFormatWithInflectionAndArguments(alloc, formatOptions, format, arguments);
    }
    return _OCFStringCreateWithFormatAndArgumentsReturningMetadata(
        alloc, NULL, NULL, formatOptions, NULL, NULL, format, NULL, arguments
    );
}

// Native _CFStringCreateWithFormatAndArgumentsAux2, 0x1c020e3d8. External.
// The fifth argument is the implicit argument of plural rules without a value type; no configuration and no
// metadata.
OCFStringRef _OCFStringCreateWithFormatAndArgumentsAux2(
    OCFAllocatorRef alloc,
    CopyDescriptionFunction copyDescFunc,
    ContextDescriptionFunction contextDescFunc,
    OCFDictionaryRef formatOptions,
    OCFTypeRef implicitArgument,
    OCFStringRef format,
    va_list arguments
) {
    return _OCFStringCreateWithFormatAndArgumentsReturningMetadata(
        alloc, copyDescFunc, contextDescFunc, formatOptions, NULL, implicitArgument, format, NULL, arguments
    );
}

// Native _CFStringCreateWithFormatAndArgumentsReturningMetadata, 0x1c020e40c. External.
// Formats into a mutable string of the system default allocator whose desired capacity is set to 120 (the
// word at offset 0x28 keeps its four low flag bits and takes 120 << 4), then copies it with alloc. A failure
// logs "ERROR: Failed to format string: %@" at level 3 with the error, releases the error and answers NULL.
OCFStringRef _OCFStringCreateWithFormatAndArgumentsReturningMetadata(
    OCFAllocatorRef alloc,
    CopyDescriptionFunction copyDescFunc,
    ContextDescriptionFunction contextDescFunc,
    OCFDictionaryRef formatOptions,
    OCFDictionaryRef formatConfiguration,
    OCFTypeRef implicitArgument,
    OCFStringRef format,
    OCFArrayRef *outReplacementMetadata,
    va_list arguments
) {
    OCFMutableStringRef outputString = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
    uint64_t *capacityWord =
        (uint64_t *)((char *)outputString + offsetof(struct __OCFString, variants.notInlineMutable.bits));
    *capacityWord = (*capacityWord & 0xf) | 0x780;
    OCFErrorRef error = NULL;
    OCFStringRef result;
    if (__CFStringAppendFormatCore(
            outputString,
            copyDescFunc,
            contextDescFunc,
            formatOptions,
            formatConfiguration,
            implicitArgument,
            NULL,
            format,
            0,
            0,
            NULL,
            0,
            arguments,
            outReplacementMetadata,
            &error
        )) {
        result = OCFStringCreateCopy(alloc, outputString);
    } else {
        OCFLog(3, OF_STR("ERROR: Failed to format string: %@"), error);
        if (error != NULL) {
            OCFRelease(error);
        }
        result = NULL;
    }
    OCFRelease(outputString);
    return result;
}

// Native CFStringCreateWithFormat, 0x1c020e580. External.
OCFStringRef
OCFStringCreateWithFormat(OCFAllocatorRef alloc, OCFDictionaryRef formatOptions, OCFStringRef format, ...) {
    va_list arguments;
    va_start(arguments, format);
    OCFStringRef result = OCFStringCreateWithFormatAndArguments(alloc, formatOptions, format, arguments);
    va_end(arguments);
    return result;
}

// The fault both append entry points log for an immutable string: CFString: %s(): Expect mutable string, with
// the function's signature as a public string argument (the native buffer header 02 01 22 08, then the
// pointer; twelve bytes), through _os_log_fault_impl with this image's handle. The format literal is placed
// in
// __TEXT,__oslogstring like the compiler-generated native one.
__attribute__((
    section("__TEXT,__oslogstring,cstring_literals")
)) static const char expectMutableStringFormat[] = "CFString: %s(): Expect mutable string";

static void logExpectMutableString(os_log_t log, const char *function) {
    uint8_t buffer[12] = {0x02, 0x01, 0x22, 0x08};
    memcpy(&buffer[4], &function, sizeof(function));
    _os_log_fault_impl(&__dso_handle, log, OS_LOG_TYPE_FAULT, expectMutableStringFormat, buffer, 12);
}

// Native CFStringAppendFormat, 0x1c020f24c, with CFStringAppendFormat.cold.1 (0x1c030c578). External.
// The mutable bit (info bit 0, read with acquire semantics even for bridged objects) is checked first; an
// immutable string logs a fault on the CoreFoundation log when the fault level is enabled.
// Folds the compiler-outlined pieces _OUTLINED_FUNCTION_0_27 (0x1c0225de4), _OUTLINED_FUNCTION_1_19
// (0x1c0225df4), _OUTLINED_FUNCTION_2_16 (0x1c0225e04).
void OCFStringAppendFormat(
    OCFMutableStringRef theString, OCFDictionaryRef formatOptions, OCFStringRef format, ...
) {
    va_list arguments;
    va_start(arguments, format);
    if ((stringInfo(theString) & 1) != 0) {
        OCFStringAppendFormatAndArguments(theString, formatOptions, format, arguments);
    } else {
        os_log_t log = _CFOSLog();
        if (os_log_type_enabled(log, OS_LOG_TYPE_FAULT)) {
            logExpectMutableString(
                log, "void CFStringAppendFormat(CFMutableStringRef, CFDictionaryRef, CFStringRef, ...)"
            );
        }
    }
    va_end(arguments);
}

// Native CFStringAppendFormatAndArguments, 0x1c021d014, with CFStringAppendFormatAndArguments.cold.1
// (0x1c030c7d0). External.
// A failure logs "ERROR: Failed to format string: %@" at level 3 and releases the error.
void OCFStringAppendFormatAndArguments(
    OCFMutableStringRef theString, OCFDictionaryRef formatOptions, OCFStringRef format, va_list arguments
) {
    if ((stringInfo(theString) & 1) == 0) {
        os_log_t log = _CFOSLog();
        if (os_log_type_enabled(log, OS_LOG_TYPE_FAULT)) {
            logExpectMutableString(
                log,
                "void CFStringAppendFormatAndArguments(CFMutableStringRef, CFDictionaryRef, CFStringRef, "
                "va_list)"
            );
        }
        return;
    }
    OCFErrorRef error = NULL;
    if (!__CFStringAppendFormatCore(
            theString,
            NULL,
            NULL,
            formatOptions,
            NULL,
            NULL,
            NULL,
            format,
            0,
            0,
            NULL,
            0,
            arguments,
            NULL,
            &error
        )) {
        OCFLog(3, OF_STR("ERROR: Failed to format string: %@"), error);
        if (error != NULL) {
            OCFRelease(error);
        }
    }
}

// Native _CFStringAppendFormatAndArgumentsAux2, 0x1c0220c78 (Local/Evidence/cf-core/
// iphone-simulator-corpus-delta5.json). External. Formats into the given string with the description
// functions, the options and the implicit argument (no configuration, no valid-specifier string, no
// metadata). A failure logs "ERROR: Failed to format string: %@" at level 3 with the error and releases the
// error.
void _OCFStringAppendFormatAndArgumentsAux2(
    OCFMutableStringRef outputString,
    CopyDescriptionFunction copyDescFunc,
    ContextDescriptionFunction contextDescFunc,
    OCFDictionaryRef formatOptions,
    OCFTypeRef implicitArgument,
    OCFStringRef formatString,
    va_list arguments
) {
    OCFErrorRef error = NULL;
    if (!__CFStringAppendFormatCore(
            outputString,
            copyDescFunc,
            contextDescFunc,
            formatOptions,
            NULL,
            implicitArgument,
            NULL,
            formatString,
            0,
            0,
            NULL,
            0,
            arguments,
            NULL,
            &error
        )) {
        OCFLog(3, OF_STR("ERROR: Failed to format string: %@"), error);
        if (error != NULL) {
            OCFRelease(error);
        }
    }
}

// Native __CFStringFindFormatSpecifiersInString, 0x1c0220d78. External.
// The validation parser: the same grammar as the formatter without arguments or configurations. Literal text
// and unknown conversions do not take a record (the index is kept when it is not negative). When configs and
// configsCount are both given, every record that ends a conversion appends one entry: the specification text
// without its "N$" argument numbers (at most 128 bytes, UTF-8, kCFAllocatorDefault, released by the caller)
// for %c, %p and the integer and floating conversions, NULL for the others and for truncated or overflowing
// specifications. Answers the number of records.
int32_t __OCFStringFindFormatSpecifiersInString(
    const uint8_t *cformat,
    const UniChar *uformat,
    OCFIndex formatLength,
    FormatSpecRecord *specs,
    OCFStringRef *configs,
    OCFIndex *configsCount
) {
    if (formatLength < 1) {
        return 0;
    }
    bool recordsConfigs = configs != NULL && configsCount != NULL;
    OCFAllocatorRef allocator = kOCFAllocatorDefault;
    char text[128];
    int32_t position = 0;
    int32_t specIndex = 0;
    do {
        FormatSpecRecord *spec = &specs[specIndex];
        spec->size = 0;
        spec->type = 0;
        spec->loc = position;
        spec->len = 0;
        spec->widthArg = -1;
        spec->precArg = -1;
        spec->flags = 0;
        *(int32_t *)&spec->mainArgNum = -1;
        int32_t start = position;
        OCFIndex scan = start;
        if (cformat != NULL) {
            while (scan < formatLength && cformat[scan] != '%') {
                ++scan;
            }
        } else {
            while (scan < formatLength && uformat[scan] != '%') {
                ++scan;
            }
        }
        position = (int32_t)scan;
        if (position != start) {
            goto notRecorded;
        }
        position = start + 1;
        if (position >= (int32_t)formatLength) {
            spec->type = 0x2b;
            goto recordWithoutText;
        }
        {
            uint32_t flags = 0;
            bool bracketClosed = false;
            bool inBracket = false;
            bool hash = false;
            bool sawPeriod = false;
            OCFIndex keyStart = -1;
            uint8_t widthArgNum = 0xff;
            uint8_t precArgNum = 0xff;
            uint32_t character;
            while (true) {
                character = cformat != NULL ? cformat[position] : uformat[position];
                ++position;
                if (keyStart >= 0) {
                    bool identifier = character >= '0' && character - ':' >= 7 && character <= 'z' &&
                                      (character == '_' || character - '[' >= 6);
                    if (!identifier) {
                        if (character == ']') {
                            if (inBracket) {
                                bracketClosed = true;
                            }
                        } else if (character == '@') {
                            if (bracketClosed) {
                                flags |= 0x40;
                            }
                            spec->flags = flags | 0x10;
                            spec->size = 4;
                            spec->type = 0x25;
                            goto recordWithoutText;
                        } else {
                            keyStart = -1;
                        }
                    }
                    goto nextCharacter;
                }
            parseCharacter:
                if (character - '1' < 9) {
                    int64_t number = 0;
                    int32_t limit = position <= (int32_t)formatLength ? (int32_t)formatLength : position;
                    OCFIndex remaining = (OCFIndex)limit - position;
                    while (true) {
                        if (__builtin_mul_overflow(number, 10, &number) ||
                            __builtin_add_overflow(number, (int64_t)character - '0', &number)) {
                            goto recordWithoutText;
                        }
                        if (remaining == 0) {
                            spec->type = 0x2b;
                            position = limit;
                            goto recordWithoutText;
                        }
                        character = cformat != NULL ? cformat[position] : uformat[position];
                        ++position;
                        --remaining;
                        if (character - '0' >= 10) {
                            break;
                        }
                    }
                    if (character == '$') {
                        if (number > 127) {
                            goto recordWithoutText;
                        }
                        if (precArgNum == 0xfe) {
                            precArgNum = (uint8_t)(number - 1);
                            spec->precArgNum = (int8_t)(number - 1);
                        } else if (widthArgNum == 0xfe) {
                            spec->widthArgNum = (int8_t)(number - 1);
                            widthArgNum = (uint8_t)(number - 1);
                        } else {
                            spec->mainArgNum = (int8_t)(number - 1);
                        }
                        goto nextCharacter;
                    }
                    if (number > 0x7fffffff) {
                        goto recordWithoutText;
                    }
                    if (sawPeriod) {
                        spec->precArg = (int32_t)number;
                    } else {
                        spec->widthArg = (int32_t)number;
                    }
                    goto parseCharacter;
                }
                switch (character) {
                    case ' ':
                        if ((flags & 4) != 0) {
                            goto nextCharacter;
                        }
                        flags |= 8;
                        spec->flags = flags;
                        goto nextCharacter;
                    case '#':
                        hash = true;
                        goto nextCharacter;
                    case '*':
                        widthArgNum = 0xfe;
                        spec->widthArgNum = -2;
                        goto nextCharacter;
                    case '+':
                        flags = (flags & 0xfffffff3) | 4;
                        spec->flags = flags;
                        goto nextCharacter;
                    case '-':
                        flags = (flags & 0xfffffffc) | 2;
                        spec->flags = flags;
                        goto nextCharacter;
                    case '.':
                        if (position >= (int32_t)formatLength) {
                            spec->type = 0x2b;
                            goto recordWithoutText;
                        }
                        character = cformat != NULL ? cformat[position] : uformat[position];
                        ++position;
                        sawPeriod = true;
                        if (character != '*') {
                            goto parseCharacter;
                        }
                        precArgNum = 0xfe;
                        spec->precArgNum = -2;
                        sawPeriod = true;
                        goto nextCharacter;
                    case '0':
                        if (sawPeriod) {
                            spec->precArg = 0;
                            sawPeriod = true;
                            goto nextCharacter;
                        }
                        sawPeriod = false;
                        if ((flags & 2) == 0) {
                            flags |= 1;
                            spec->flags = flags;
                        }
                        goto nextCharacter;
                    case '@':
                        if (!hash) {
                            spec->size = 4;
                            spec->type = 0x25;
                            goto recordWithoutText;
                        }
                        hash = false;
                        keyStart = position;
                        goto nextCharacter;
                    case 'A':
                    case 'a':
                        goto floatingType;
                    case 'C':
                        spec->size = 2;
                        spec->type = 0x29;
                        goto recordWithoutText;
                    case 'D':
                    case 'U':
                    case 'd':
                    case 'i':
                    case 'u':
                        if (_OCFExecutableLinkedOnOrAfter(8)) {
                            spec->flags |= 0x20;
                        }
                        spec->numericStyle = (character & 0xffdf) == 'U' ? 4 : 1;
                        goto integerType;
                    case 'E':
                    case 'F':
                    case 'G':
                    case 'e':
                    case 'f':
                    case 'g': {
                        if (_OCFExecutableLinkedOnOrAfter(8)) {
                            spec->flags |= 0x20;
                        }
                        uint32_t lowered = character - 'A' < 26 ? character + 32 : character;
                        uint8_t style = (lowered & 0xfffd) == 'e' ? 2 : 0;
                        if ((lowered & 0xfffe) == 'f') {
                            ++style;
                        }
                        spec->numericStyle = style;
                        if (sawPeriod && spec->precArg == -1 && (uint8_t)spec->precArgNum == 0xff) {
                            spec->precArg = 0;
                        }
                    }
                    floatingType:
                        spec->type = 0x22;
                        if (spec->size != 5) {
                            spec->size = 4;
                        }
                        goto recordWithText;
                    case 'L':
                        spec->size = 5;
                        goto nextCharacter;
                    case 'O':
                    case 'X':
                    case 'o':
                    case 'x':
                    integerType:
                        spec->type = 0x21;
                        goto recordWithText;
                    case 'P':
                        spec->size = 4;
                        spec->type = 0x28;
                        goto recordWithoutText;
                    case 'S':
                        spec->size = 4;
                        spec->type = 0x26;
                        goto recordWithoutText;
                    case '[':
                        if (!inBracket) {
                            keyStart = position;
                        }
                        inBracket = true;
                        goto nextCharacter;
                    case 'c':
                        spec->size = 1;
                        spec->type = 0x21;
                        goto recordWithText;
                    case 'h':
                        if (position < (int32_t)formatLength &&
                            (cformat != NULL ? cformat[position] : uformat[position]) == 'h') {
                            ++position;
                            spec->size = 1;
                        } else {
                            spec->size = 2;
                        }
                        goto nextCharacter;
                    case 'j':
                    case 'q':
                    case 't':
                    case 'z':
                        spec->size = OF_PORT_ANSWER(formatIntegerSize, 4, character);
                        goto nextCharacter;
                    case 'l':
                        if (position < (int32_t)formatLength &&
                            (cformat != NULL ? cformat[position] : uformat[position]) == 'l') {
                            ++position;
                        }
                        spec->size =
                            OF_PORT_ANSWER(formatLongSize, 4, (const uint8_t *)cformat, uformat, position);
                        goto nextCharacter;
                    case 'n':
                        spec->size = 4;
                        spec->type = 0x2a;
                        goto recordWithoutText;
                    case 'p':
                        spec->size = 4;
                        spec->type = 0x23;
                        goto recordWithText;
                    case 's':
                        spec->size = 4;
                        spec->type = 0x27;
                        goto recordWithoutText;
                    default:
                        spec->type = 0x20;
                        goto notRecorded;
                }
            nextCharacter:
                if (position >= (int32_t)formatLength) {
                    spec->type = 0x2b;
                    goto recordWithoutText;
                }
            }
        }
    recordWithText:
        {
            int32_t length = position - start;
            spec->len = length;
            if (!recordsConfigs) {
                goto nextSpec;
            }
            int32_t out = 0;
            for (int32_t offset = 0; offset < length;) {
                uint32_t c = cformat != NULL ? cformat[spec->loc + offset] : uformat[spec->loc + offset];
                int32_t last;
                if (c == '$') {
                    if (out < 0) {
                        last = out;
                    } else if (out == 0) {
                        last = -1;
                    } else {
                        // Remove the digits of the argument number before '$'.
                        int64_t scanIndex = (uint32_t)out;
                        while (true) {
                            if ((uint32_t)((uint8_t)text[scanIndex - 1] - '0') >= 10) {
                                last = (int32_t)scanIndex - 1;
                                break;
                            }
                            --scanIndex;
                            if (scanIndex + 1 < 2) {
                                last = -1;
                                break;
                            }
                        }
                    }
                } else {
                    text[out] = (char)c;
                    last = out;
                }
                out = last + 1;
                ++offset;
                if (offset >= length || last >= 127) {
                    break;
                }
            }
            text[out] = 0;
            OCFStringRef textString = __CFStringCreateImmutableFunnel3(
                allocator, text, (OCFIndex)strlen(text), 0x8000100, 8, (OCFAllocatorRef)-1, 0
            );
            configs[(*configsCount)++] = textString;
            goto nextSpec;
        }
    recordWithoutText:
        spec->len = position - start;
        if (recordsConfigs) {
            configs[(*configsCount)++] = NULL;
        }
        goto nextSpec;
    notRecorded:
        if (specIndex >= 0) {
            --specIndex;
        }
    nextSpec:
        ++specIndex;
    } while (formatLength > position);
    return specIndex;
}

// Native __CFStringValidateFormat, 0x1c022154c. Hidden (non-external).
// Parses both strings (each at most 0x7ffffffe characters; longer answers -1) and validates the format's
// records against the expected records from initialArgPosition. On a mismatch with errorPtr, an
// NSCocoaErrorDomain 2048 error describes "Format '%@'%@ does not match expected '%@'" (with " (implicitly
// starting at argument %ld)" for a nonzero start). Answers initialArgPosition plus the format's record count,
// or -1. CoreFoundation defect: an expected string that is too long returns -1 without releasing the format's
// buffers (the specification text strings and any allocated buffers leak); kept.
OCFIndex __CFStringValidateFormat(
    OCFStringRef expected, OCFStringRef format, OCFIndex initialArgPosition, OCFErrorRef *errorPtr
) {
    OCFIndex expectedConfigsCount = 0;
    OCFIndex formatConfigsCount = 0;
    OCFAllocatorRef allocator = formattingAllocator();
    const uint8_t *formatCString = NULL;
    const UniChar *formatUString = NULL;
    const uint8_t *expectedCString = NULL;
    const UniChar *expectedUString = NULL;
    UniChar *expectedAllocated = NULL;
    UniChar *formatAllocated = NULL;
    FormatSpecRecord *expectedSpecs = NULL;
    FormatSpecRecord *formatSpecs = NULL;
    OCFStringRef *expectedConfigs = NULL;
    OCFStringRef *formatConfigs = NULL;
    FormatSpecRecord formatStackSpecs[61];
    FormatSpecRecord expectedStackSpecs[61];
    OCFStringRef formatStackConfigs[61];
    OCFStringRef expectedStackConfigs[61];
    UniChar expectedStackBuffer[400];
    UniChar formatStackBuffer[400];
    OCFIndex formatLength = OCFStringGetLength(format);
    if (formatLength > 0x7ffffffe) {
        return -1;
    }
    memset(formatStackConfigs, 0, sizeof(formatStackConfigs));
    memset(expectedStackConfigs, 0, sizeof(expectedStackConfigs));
    __CFStringSetUpFormatAndSpecBuffers(
        format,
        formatLength,
        &formatCString,
        &formatUString,
        &formatAllocated,
        formatStackBuffer,
        &formatSpecs,
        formatStackSpecs,
        &formatConfigs,
        formatStackConfigs
    );
    OCFIndex expectedLength = OCFStringGetLength(expected);
    if (expectedLength > 0x7ffffffe) {
        return -1;
    }
    __CFStringSetUpFormatAndSpecBuffers(
        expected,
        expectedLength,
        &expectedCString,
        &expectedUString,
        &expectedAllocated,
        expectedStackBuffer,
        &expectedSpecs,
        expectedStackSpecs,
        &expectedConfigs,
        expectedStackConfigs
    );
    int32_t formatCount = __OCFStringFindFormatSpecifiersInString(
        formatCString, formatUString, formatLength, formatSpecs, formatConfigs, &formatConfigsCount
    );
    int32_t expectedCount = __OCFStringFindFormatSpecifiersInString(
        expectedCString,
        expectedUString,
        expectedLength,
        expectedSpecs,
        expectedConfigs,
        &expectedConfigsCount
    );
    bool valid = __CFStringValidateFormatSpecBuffers(
        (const FormatSpec *)formatSpecs,
        formatCount,
        (const FormatSpec *)expectedSpecs,
        expectedCount,
        initialArgPosition
    );
    if (errorPtr != NULL && !valid) {
        OCFStringRef description;
        if (initialArgPosition != 0) {
            OCFStringRef implicit = OCFStringCreateWithFormat(
                allocator, NULL, OF_STR(" (implicitly starting at argument %ld)"), initialArgPosition + 1
            );
            description = OCFStringCreateWithFormat(
                allocator,
                NULL,
                OF_STR("Format '%@'%@ does not match expected '%@'"),
                format,
                implicit,
                expected
            );
            OCFRelease(implicit);
        } else {
            description = OCFStringCreateWithFormat(
                allocator,
                NULL,
                OF_STR("Format '%@'%@ does not match expected '%@'"),
                format,
                OF_STR(""),
                expected
            );
        }
        setFormatError(allocator, errorPtr, description);
        OCFRelease(description);
    }
    if (formatAllocated != NULL && formatAllocated != formatStackBuffer) {
        OCFAllocatorDeallocate(allocator, formatAllocated);
    }
    if (expectedAllocated != NULL && expectedAllocated != expectedStackBuffer) {
        OCFAllocatorDeallocate(allocator, expectedAllocated);
    }
    if (formatSpecs != formatStackSpecs) {
        OCFAllocatorDeallocate(allocator, formatSpecs);
    }
    if (expectedSpecs != expectedStackSpecs) {
        OCFAllocatorDeallocate(allocator, expectedSpecs);
    }
    for (OCFIndex index = 0; index < formatConfigsCount; ++index) {
        if (formatConfigs[index] != NULL) {
            OCFRelease(formatConfigs[index]);
        }
    }
    for (OCFIndex index = 0; index < expectedConfigsCount; ++index) {
        if (expectedConfigs[index] != NULL) {
            OCFRelease(expectedConfigs[index]);
        }
    }
    if (formatConfigs != formatStackConfigs) {
        OCFAllocatorDeallocate(allocator, formatConfigs);
    }
    if (expectedConfigs != expectedStackConfigs) {
        OCFAllocatorDeallocate(allocator, expectedConfigs);
    }
    return valid ? initialArgPosition + formatCount : -1;
}

// Native ___CFStringSetUpFormatAndSpecBuffers, 0x1c0223ff8. Non-external.
// Points *cformat or *uformat at the string's own contents when it is a CF string (8-bit contents after any
// length byte, or UTF-16), else copies the characters into stackUniBuffer (up to 400) or a typed allocation
// of the formatting allocator ("CFString (temp)"). From 31 '%' characters (an overflowing count takes
// 0x1fffff, the count is capped at 0xfffff) the specification and configuration buffers are allocated for 2n
// + 1 entries (sizes computed in 32 bits); otherwise the stack buffers are used.
static void __CFStringSetUpFormatAndSpecBuffers(
    OCFStringRef string,
    OCFIndex length,
    const uint8_t **cformat,
    const UniChar **uformat,
    UniChar **allocatedUniBuffer,
    UniChar *stackUniBuffer,
    FormatSpecRecord **specs,
    FormatSpecRecord *stackSpecs,
    OCFStringRef **configs,
    OCFStringRef *stackConfigs
) {
    OCFAllocatorRef allocator = formattingAllocator();
    int32_t count = 0;
    int32_t entries;
    if ((intptr_t)string >= 0 && !__CF_IS_OBJC(7, string)) {
        if ((stringInfo(string) & 0x10) != 0) {
            *uformat = stringContents(string);
        } else {
            *cformat = stringContents(string);
            if (*cformat != NULL) {
                *cformat += (stringInfo(string) >> 2) & 1;
                if (*cformat != NULL) {
                    goto countBytes;
                }
            }
            goto copyCharacters;
        }
    }
    if (*cformat != NULL) {
        goto countBytes;
    }
copyCharacters:
    if (*uformat == NULL) {
        UniChar *buffer = stackUniBuffer;
        if (length <= 400) {
            *allocatedUniBuffer = stackUniBuffer;
        } else {
            *allocatedUniBuffer = OCFAllocatorAllocateTyped(allocator, 2 * length, 0x1000040bdfb0063, 0);
            buffer = *allocatedUniBuffer;
            if (buffer != stackUniBuffer && __OCFOASafe) {
                __OCFSetLastAllocationEventName(buffer, "CFString (temp)");
                buffer = *allocatedUniBuffer;
            }
        }
        OCFStringGetCharacters(string, (OCFRange){0, length}, buffer);
        *uformat = *allocatedUniBuffer;
        if (*cformat != NULL) {
            goto countBytes;
        }
    }
    if (length < 1) {
        goto stackBuffers;
    }
    {
        const UniChar *characters = *uformat;
        for (OCFIndex remaining = length; remaining != 0; --remaining) {
            if (*characters++ == '%' && __builtin_add_overflow(count, 1, &count)) {
                goto overflowed;
            }
        }
    }
    goto counted;
countBytes:
    if (length < 1) {
        goto stackBuffers;
    }
    {
        const uint8_t *bytes = *cformat;
        for (OCFIndex remaining = length; remaining != 0; --remaining) {
            if (*bytes++ == '%' && __builtin_add_overflow(count, 1, &count)) {
                goto overflowed;
            }
        }
    }
counted:
    if (count < 31) {
        goto stackBuffers;
    }
    entries = ((count < 0xfffff ? count : 0xfffff) << 1) | 1;
    goto allocate;
overflowed:
    entries = 0x1fffff;
allocate:
    *specs = OCFAllocatorAllocateTyped(allocator, (uint32_t)(entries << 5), 0x10000408e5dae1c, 0);
    if (*specs != stackSpecs && __OCFOASafe) {
        __OCFSetLastAllocationEventName(*specs, "CFString (temp)");
    }
    *configs = OCFAllocatorAllocateTyped(allocator, (uint32_t)(entries << 3), 0x6004044c4a2df, 0);
    if (*configs != stackConfigs && __OCFOASafe) {
        __OCFSetLastAllocationEventName(*configs, "CFString (temp)");
    }
    return;
stackBuffers:
    *specs = stackSpecs;
    *configs = stackConfigs;
}

// Native ___CFStringValidateFormatSpec, 0x1c02245e4. Non-external.
// Validates one record against the records parsed from expected (at most 0x7ffffffe characters) from
// initialArgPosition; answers initialArgPosition + 1 or -1.
static OCFIndex __CFStringValidateFormatSpec(
    OCFStringRef expected, const FormatSpecRecord *spec, OCFIndex initialArgPosition
) {
    OCFAllocatorRef allocator = formattingAllocator();
    const uint8_t *cformat = NULL;
    OCFIndex configsCount = 0;
    UniChar *allocated = NULL;
    const UniChar *uformat = NULL;
    OCFStringRef *configs = NULL;
    FormatSpecRecord *specs = NULL;
    OCFStringRef stackConfigs[61];
    FormatSpecRecord stackSpecs[61];
    UniChar stackBuffer[400];
    OCFIndex length = OCFStringGetLength(expected);
    if (length > 0x7ffffffe) {
        return -1;
    }
    memset(stackConfigs, 0, sizeof(stackConfigs));
    __CFStringSetUpFormatAndSpecBuffers(
        expected,
        length,
        &cformat,
        &uformat,
        &allocated,
        stackBuffer,
        &specs,
        stackSpecs,
        &configs,
        stackConfigs
    );
    int32_t count =
        __OCFStringFindFormatSpecifiersInString(cformat, uformat, length, specs, configs, &configsCount);
    bool valid = __CFStringValidateFormatSpecBuffers(
        (const FormatSpec *)spec, 1, (const FormatSpec *)specs, count, initialArgPosition
    );
    if (allocated != NULL && allocated != stackBuffer) {
        OCFAllocatorDeallocate(allocator, allocated);
    }
    if (specs != stackSpecs) {
        OCFAllocatorDeallocate(allocator, specs);
    }
    for (OCFIndex index = 0; index < configsCount; ++index) {
        if (configs[index] != NULL) {
            OCFRelease(configs[index]);
        }
    }
    if (configs != stackConfigs) {
        OCFAllocatorDeallocate(allocator, configs);
    }
    return valid ? initialArgPosition + 1 : -1;
}

// Native ___CFStringFormatLocalizedNumber.disableLocalizedFormatting (__DATA,__data, initially -1),
// .formatterLock, .decimalFormatter, .scientificFormatter, .gFormatter, .groupingSize,
// .secondaryGroupingSize, .origFormatDecimal, .origFormatScientific and .origFormatGeneric (__DATA,__bss).
// Non-external.
static int32_t disableLocalizedFormatting = -1;
static os_unfair_lock formatterLock = OS_UNFAIR_LOCK_INIT;
static OCFNumberFormatterRef decimalFormatter;
static OCFNumberFormatterRef scientificFormatter;
static OCFNumberFormatterRef gFormatter;
static int32_t groupingSize;
static int32_t secondaryGroupingSize;
static OCFStringRef origFormatDecimal;
static OCFStringRef origFormatScientific;
static OCFStringRef origFormatGeneric;

static void __locked_CFStringFormatLocalizedNumberConfigureFormatter(OCFNumberFormatterRef formatter);

// Native ___CFStringFormatLocalizedNumber, 0x1c02247d0, with ___CFStringFormatLocalizedNumber.cold.1
// (0x1c030ca60, "Unexpected formatter style"). Non-external.
// Disabled by CFStringDisableLocalizedNumberFormatting (read once). Under a private lock, one cached
// formatter per style is kept for the last locale (decimal style 1 for %d, %u and %f; scientific style 4 with
// "#E+00" and significant digits for %e; decimal with the locale's grouping sizes for %g), configured once,
// and each use works on a copy with the ICU format: precision (default 0 for integers, 6 otherwise), %g's
// "@###" pattern with E+00 beyond 10^precision or below 1e-4, the plus sign, width, zero padding and padding
// position. Integers with a precision are padded with spaces to the width afterwards. Answers whether a
// string was appended.
static bool __CFStringFormatLocalizedNumber(
    OCFMutableStringRef output,
    OCFLocaleRef locale,
    const PrintValue *values,
    const FormatSpecRecord *spec,
    int32_t width,
    int32_t precision,
    bool hasPrecision
) {
    if (disableLocalizedFormatting == -1) {
        disableLocalizedFormatting = getenv("CFStringDisableLocalizedNumberFormatting") != NULL;
        if (disableLocalizedFormatting != 0) {
            return false;
        }
    } else if (disableLocalizedFormatting != 0) {
        return false;
    }
    os_unfair_lock_lock(&formatterLock);
    OCFNumberFormatterRef *cached;
    switch (spec->numericStyle) {
        case 2:
            if (scientificFormatter != NULL) {
                if (OCFEqual(OCFNumberFormatterGetLocale(scientificFormatter), locale)) {
                    cached = &scientificFormatter;
                    goto haveFormatter;
                }
                if (scientificFormatter != NULL) {
                    OCFRelease(scientificFormatter);
                }
            }
            scientificFormatter = OCFNumberFormatterCreate(NULL, locale, 4);
            if (scientificFormatter == NULL) {
                goto failed;
            }
            cached = &scientificFormatter;
            OCFNumberFormatterSetFormat(scientificFormatter, OF_STR("#E+00"));
            _CFNumberFormatterSetPropertyNoRefresh(
                scientificFormatter, kOCFNumberFormatterUseSignificantDigitsKey, kOCFBooleanTrue
            );
            __locked_CFStringFormatLocalizedNumberConfigureFormatter(*cached);
            goto haveFormatter;
        case 1:
        case 4:
            if (decimalFormatter != NULL) {
                if (OCFEqual(OCFNumberFormatterGetLocale(decimalFormatter), locale)) {
                    cached = &decimalFormatter;
                    goto haveFormatter;
                }
                if (decimalFormatter != NULL) {
                    OCFRelease(decimalFormatter);
                }
            }
            cached = &decimalFormatter;
            decimalFormatter = OCFNumberFormatterCreate(NULL, locale, 1);
            if (decimalFormatter == NULL) {
                goto failed;
            }
            __locked_CFStringFormatLocalizedNumberConfigureFormatter(decimalFormatter);
            goto haveFormatter;
        case 3: {
            if (gFormatter != NULL) {
                if (OCFEqual(OCFNumberFormatterGetLocale(gFormatter), locale)) {
                    cached = &gFormatter;
                    goto haveFormatter;
                }
                if (gFormatter != NULL) {
                    OCFRelease(gFormatter);
                }
            }
            gFormatter = OCFNumberFormatterCreate(NULL, locale, 1);
            if (gFormatter == NULL) {
                goto failed;
            }
            cached = &gFormatter;
            OCFNumberRef size =
                OCFNumberFormatterCopyProperty(gFormatter, kOCFNumberFormatterGroupingSizeKey);
            OCFNumberGetValue(size, 3, &groupingSize);
            OCFRelease(size);
            size = OCFNumberFormatterCopyProperty(*cached, kOCFNumberFormatterSecondaryGroupingSizeKey);
            OCFNumberGetValue(size, 3, &secondaryGroupingSize);
            OCFRelease(size);
            __locked_CFStringFormatLocalizedNumberConfigureFormatter(*cached);
            goto haveFormatter;
        }
        default:
            crashWithMessage("Unexpected formatter style");
    }
haveFormatter:;
    OCFNumberFormatterRef formatter = *cached;
    if (formatter == NULL) {
        goto failed;
    }
    OCFStringRef currentFormat = NULL;
    OCFStringRef *original = NULL;
    switch (spec->numericStyle) {
        case 2:
            original = &origFormatScientific;
            break;
        case 1:
        case 4:
            original = &origFormatDecimal;
            break;
        case 3:
            original = &origFormatGeneric;
            break;
        default:
            break;
    }
    if (original != NULL) {
        currentFormat = *original;
        if (currentFormat == NULL) {
            currentFormat = OCFStringCreateCopy(NULL, OCFNumberFormatterGetFormat(formatter));
            *original = currentFormat;
        }
    }
    OCFNumberFormatterRef copy = CFNumberFormatterCreateCopyWithICUFormat(kOCFAllocatorDefault, formatter);
    if (!hasPrecision) {
        precision = spec->type == 0x21 ? 0 : 6;
    }
    bool changed;
    if (spec->numericStyle == 3) {
        precision = (uint32_t)precision > 1 ? precision : 1;
        OCFMutableStringRef pattern = OCFStringCreateMutable(NULL, 0);
        OCFStringAppendCString(pattern, "@", 0x600);
        OCFStringPad(pattern, OF_STR("#"), precision, 0);
        double number = values[spec->mainArgNum].value.doubleValue;
        double limit = __exp10((double)precision);
        bool exponent;
        if (number > 0.0) {
            exponent = number > limit || number < 0.0001;
        } else {
            exponent = number < 0.0 && (number < -limit || number > -0.0001);
        }
        if (exponent) {
            OCFStringAppendCString(pattern, "E+00", 0x600);
        } else if (precision - groupingSize > 0 && groupingSize != 0) {
            OCFStringInsert(pattern, (uint32_t)(precision - groupingSize), OF_STR(","));
            if (secondaryGroupingSize != 0 && precision - (groupingSize + secondaryGroupingSize) > 0) {
                OCFStringInsert(pattern, precision - (groupingSize + secondaryGroupingSize), OF_STR(","));
            }
        }
        OCFNumberFormatterSetFormat(copy, pattern);
        OCFRelease(pattern);
        _CFNumberFormatterSetInt32PropertyNoRefresh(copy, kOCFNumberFormatterMaxFractionDigitsKey, precision);
        changed = true;
    } else if (precision != 0) {
        _CFNumberFormatterSetInt32PropertyNoRefresh(copy, kOCFNumberFormatterMaxFractionDigitsKey, precision);
        changed = true;
    } else {
        changed = false;
    }
    if (spec->type == 0x22) {
        _CFNumberFormatterSetInt32PropertyNoRefresh(copy, kOCFNumberFormatterMinFractionDigitsKey, precision);
        changed = true;
    }
    const PrintValue *value = &values[spec->mainArgNum];
    bool suppressPlus;
    if (value->type == 0x22) {
        suppressPlus = value->value.doubleValue < 0.0;
    } else if (value->type == 0x21) {
        suppressPlus = (uint64_t)value->value.int64Value >> 63;
    } else {
        suppressPlus = false;
    }
    if (changed) {
        currentFormat = OCFNumberFormatterGetFormat(copy);
    }
    if ((spec->flags & 4) == 0) {
        suppressPlus = true;
    }
    UniChar first = OCFStringGetCharacterAtIndex(currentFormat, 0);
    if (suppressPlus) {
        if (first == '+') {
            OCFStringRef withoutPlus = OCFStringCreateWithSubstring(
                NULL, currentFormat, (OCFRange){1, OCFStringGetLength(currentFormat) - 1}
            );
            OCFNumberFormatterSetFormat(copy, withoutPlus);
            OCFRelease(withoutPlus);
            changed = true;
        } else {
            changed = false;
        }
    } else if (first == '+') {
        changed = false;
    } else {
        OCFMutableStringRef withPlus = OCFStringCreateMutableCopy(NULL, 0, OF_STR("+"));
        OCFStringAppend(withPlus, currentFormat);
        OCFNumberFormatterSetFormat(copy, withPlus);
        OCFRelease(withPlus);
        changed = true;
    }
    bool integerPrecision = hasPrecision && spec->type == 0x21;
    int32_t formatWidth = integerPrecision ? precision : width;
    uint8_t flagsByte = (uint8_t)spec->flags;
    if (formatWidth != 0) {
        _CFNumberFormatterSetInt32PropertyNoRefresh(copy, kOCFNumberFormatterFormatWidthKey, formatWidth);
        changed = true;
    }
    bool zeroPad = (flagsByte & 1) != 0;
    if (integerPrecision) {
        zeroPad = true;
    }
    uint32_t flags = spec->flags;
    if (formatWidth >= 1 && zeroPad && spec->type != 0x22) {
        _CFNumberFormatterSetInt32PropertyNoRefresh(
            copy, kOCFNumberFormatterMinIntegerDigitsKey, formatWidth
        );
        changed = true;
    }
    uint8_t style;
    if (integerPrecision || ((uint8_t)spec->flags & 2) != 0) {
        _CFNumberFormatterSetInt32PropertyNoRefresh(
            copy, kOCFNumberFormatterPaddingPositionKey, integerPrecision ? 1 : 3
        );
        if (!zeroPad) {
            changed = true;
            style = spec->numericStyle;
            goto significantDigits;
        }
    } else {
        if (!zeroPad) {
            style = spec->numericStyle;
            goto significantDigits;
        }
        _CFNumberFormatterSetInt32PropertyNoRefresh(copy, kOCFNumberFormatterPaddingPositionKey, 1);
    }
    style = spec->numericStyle;
    if (style == 3) {
        _CFNumberFormatterSetPropertyNoRefresh(copy, kOCFNumberFormatterPaddingCharacterKey, OF_STR("0"));
        changed = true;
        style = spec->numericStyle;
    } else {
        changed = true;
    }
significantDigits:
    if (style == 2) {
        _CFNumberFormatterSetInt32PropertyNoRefresh(
            copy, kOCFNumberFormatterMinSignificantDigitsKey, precision + 1
        );
        _CFNumberFormatterSetInt32PropertyNoRefresh(
            copy, kOCFNumberFormatterMaxSignificantDigitsKey, precision + 1
        );
        changed = true;
    }
    OCFStringRef formatted;
    if (spec->type == 0x22) {
        if (changed) {
            OCFNumberFormatterGetFormat(copy);
        }
        const PrintValue *argument = &values[spec->mainArgNum];
        const void *number = &argument->value;
        double copied;
        if (argument->size == 5) {
            copied = argument->value.doubleValue;
            number = &copied;
        }
        formatted = OCFNumberFormatterCreateStringWithValue(NULL, copy, 13, number);
    } else if (spec->type == 0x21) {
        if (changed) {
            OCFNumberFormatterGetFormat(copy);
        }
        if ((spec->numericStyle & 4) != 0) {
            // CFSInt128Struct {high, low}: the unsigned 64-bit value as a 128-bit integer.
            int64_t wide[2] = {0, values[spec->mainArgNum].value.int64Value};
            formatted = OCFNumberFormatterCreateStringWithValue(NULL, copy, 17, wide);
        } else {
            formatted =
                OCFNumberFormatterCreateStringWithValue(NULL, copy, 4, &values[spec->mainArgNum].value);
        }
    } else {
        formatted = NULL;
    }
    OCFRelease(copy);
    os_unfair_lock_unlock(&formatterLock);
    if (formatted == NULL) {
        return false;
    }
    if (hasPrecision && spec->type == 0x21 && OCFStringGetLength(formatted) < width) {
        OCFIndex total = width;
        OCFMutableStringRef padded;
        if ((flags & 2) != 0) {
            padded = OCFStringCreateMutableCopy(NULL, 0, formatted);
            OCFStringPad(padded, OF_STR(" "), total, 0);
        } else {
            padded = OCFStringCreateMutable(NULL, 0);
            OCFStringPad(padded, OF_STR(" "), total - OCFStringGetLength(formatted), 0);
            OCFStringAppend(padded, formatted);
        }
        OCFRelease(formatted);
        formatted = padded;
    }
    OCFStringAppend(output, formatted);
    OCFRelease(formatted);
    return true;
failed:
    os_unfair_lock_unlock(&formatterLock);
    return false;
}

// Native _CFStringFormatAppendMetadata, 0x1c0225068, with _CFStringFormatAppendMetadata.cold.1 (0x1c030ca74,
// the assertion at CFString.c:8257). Non-external. Records one replacement (system default allocator, CFType
// callbacks): SpecLocation and SpecLength (SInt32), the replacement range ReplacementLocation (start plus
// added isolates) and ReplacementLength (end minus start minus twice the added isolates, clamped at 0), Index
// (the 1-based argument), Number (SInt64 or double) or Object for the argument, and AddedIsolates. Literal
// text is recorded only for unknown conversions kept as text (flag 0x80); other records need a main argument,
// which must lie inside the values.
static void _CFStringFormatAppendMetadata(
    OCFMutableArrayRef *outMetadata,
    OCFIndex capacity,
    const PrintValue *values,
    OCFIndex valuesCount,
    const FormatSpecRecord *spec,
    OCFIndex replacementStart,
    OCFIndex replacementEnd,
    uint32_t addedIsolates
) {
    if (outMetadata == NULL) {
        return;
    }
    uint32_t type = (uint16_t)spec->type;
    if (type - 0x2b >= 2) {
        if (type == 0x20) {
            if (((uint8_t)spec->flags & 0x80) == 0) {
                return;
            }
        } else {
            OCFIndex main = spec->mainArgNum;
            if (main < 0) {
                return;
            }
            if (valuesCount <= main) {
                __assert_rtn(
                    "_CFStringFormatAppendMetadata", "CFString.c", 8257, "spec.mainArgNum < valuesCount"
                );
            }
        }
    }
    if (*outMetadata == NULL) {
        *outMetadata = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, capacity, &kOCFTypeArrayCallBacks);
    }
    OCFMutableDictionaryRef record = OCFDictionaryCreateMutable(
        kOCFAllocatorSystemDefault, 2, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    int32_t specLocation = spec->loc;
    int32_t specLength = spec->len;
    OCFIndex location = replacementStart + addedIsolates;
    OCFNumberRef number = OCFNumberCreate(kOCFAllocatorSystemDefault, 3, &specLocation);
    OCFDictionarySetValue(record, _kOCFStringFormatMetadataSpecifierRangeLocationInFormatStringKey, number);
    OCFRelease(number);
    number = OCFNumberCreate(kOCFAllocatorSystemDefault, 3, &specLength);
    OCFDictionarySetValue(record, _kOCFStringFormatMetadataSpecifierRangeLengthInFormatStringKey, number);
    OCFRelease(number);
    number = OCFNumberCreate(kOCFAllocatorSystemDefault, 14, &location);
    OCFDictionarySetValue(record, _kOCFStringFormatMetadataReplacementRangeLocationKey, number);
    OCFRelease(number);
    OCFIndex length = replacementEnd - (location + addedIsolates);
    if (length < 0) {
        length = 0;
    }
    number = OCFNumberCreate(kOCFAllocatorSystemDefault, 14, &length);
    OCFDictionarySetValue(record, _kOCFStringFormatMetadataReplacementRangeLengthKey, number);
    OCFRelease(number);
    OCFIndex main = spec->mainArgNum;
    if (main >= 0) {
        if (valuesCount > main && spec->type != 0x20) {
            OCFIndex argumentNumber = main + 1;
            number = OCFNumberCreate(kOCFAllocatorSystemDefault, 14, &argumentNumber);
            OCFDictionarySetValue(record, _kOCFStringFormatMetadataReplacementIndexKey, number);
            OCFRelease(number);
            main = spec->mainArgNum;
        }
        if (main >= 0 && valuesCount > (int32_t)main) {
            PrintValue argument = values[(int32_t)main];
            if (argument.type == 0x21) {
                number = OCFNumberCreate(kOCFAllocatorSystemDefault, 4, &argument.value);
                OCFDictionarySetValue(record, OF_STR("Number"), number);
                OCFRelease(number);
            } else if (argument.type == 0x22) {
                number = OCFNumberCreate(kOCFAllocatorSystemDefault, 13, &argument.value);
                OCFDictionarySetValue(record, OF_STR("Number"), number);
                OCFRelease(number);
            } else if (argument.type == 0x25 && argument.value.pointerValue != NULL) {
                OCFDictionarySetValue(
                    record, _kOCFStringFormatMetadataArgumentObjectKey, argument.value.pointerValue
                );
            }
        }
    }
    OCFIndex isolates = addedIsolates;
    number = OCFNumberCreate(kOCFAllocatorSystemDefault, 14, &isolates);
    OCFDictionarySetValue(record, _kOCFStringFormatMetadataAddedIsolatesKey, number);
    OCFRelease(number);
    OCFArrayAppendValue(*outMetadata, record);
    OCFRelease(record);
}

// Native __CFShouldUseIsolatesForAppendingValueWithStatisticalWritingDirections, 0x1c0225410. Non-external.
// The locale's language direction is computed once per format (when *localeDirectionComputed is clear):
// *localeIsRightToLeft = CFLocaleGetLanguageCharacterDirection(CFLocaleGetIdentifier(locale)) ==
// kCFLocaleLanguageDirectionRightToLeft, and the computed flag is set. Starting from that byte, unless
// isDirectionalMark is set or the locale is right-to-left the value's whole range is tested with
// _CFStringHasStrongOrWeakRTL. When nothing was appended yet (lastAppendedLength 0) and the result is still
// false, a nonempty output string is tested over its whole range and decides alone when it answers true;
// otherwise (and for an empty output) a true result answers true. Else the format string after the current
// specifier (location + length of specs[specIndex]) up to formatLength is tested (a tail call); nothing
// remaining answers false. Bit 0 of each result is tested.
bool __CFShouldUseIsolatesForAppendingValueWithStatisticalWritingDirections(
    OCFStringRef value,
    OCFMutableStringRef outputString,
    bool *localeDirectionComputed,
    bool *localeIsRightToLeft,
    bool isDirectionalMark,
    OCFIndex lastAppendedLength,
    const void *specs,
    OCFIndex specIndex,
    OCFIndex formatLength,
    OCFStringRef formatString,
    OCFTypeRef locale
) {
    if ((*(const uint8_t *)localeDirectionComputed & 1) == 0) {
        OCFLocaleIdentifier identifier = OCFLocaleGetIdentifier((OCFLocaleRef)locale);
        *localeIsRightToLeft = OCFLocaleGetLanguageCharacterDirection(identifier) == 2;
        *localeDirectionComputed = true;
    }
    uint64_t rightToLeft = *(const uint8_t *)localeIsRightToLeft;
    if (((uint8_t)isDirectionalMark & 1) == 0 && (rightToLeft & 1) == 0) {
        OCFIndex length = OCFStringGetLength(value);
        rightToLeft = _CFStringHasStrongOrWeakRTL(value, 0, length);
    }
    if (lastAppendedLength == 0 && (rightToLeft & 1) == 0) {
        OCFIndex outputLength = OCFStringGetLength(outputString);
        if (outputLength != 0) {
            if ((_CFStringHasStrongOrWeakRTL(outputString, 0, outputLength) & 1) != 0) {
                return true;
            }
            goto formatTail;
        }
    }
    if ((rightToLeft & 1) != 0) {
        return true;
    }
formatTail:
    {
        const uint8_t *spec = (const uint8_t *)specs + ((OCFIndex)(int32_t)specIndex << 5);
        OCFIndex location =
            (OCFIndex) * (const int32_t *)(spec + 8) + (OCFIndex) * (const int32_t *)(spec + 4);
        if (formatLength - location <= 0) {
            return false;
        }
        return _CFStringHasStrongOrWeakRTL(formatString, location, formatLength - location);
    }
}

// Native __CFShouldUseIsolatesForAppendingValue, 0x1c022552c. Non-external. A value with strong or weak RTL
// characters (bit 0) answers true. Otherwise a nonempty output string is tested from lastAppendedLength to
// its end, and an empty one makes the format string after the current specifier (location + length of
// specs[specIndex]) up to formatLength the tested range; an empty remaining range answers false (both tests
// are tail calls).
bool __CFShouldUseIsolatesForAppendingValue(
    OCFStringRef value,
    OCFMutableStringRef outputString,
    OCFIndex lastAppendedLength,
    const void *specs,
    OCFIndex specIndex,
    OCFIndex formatLength,
    OCFStringRef formatString
) {
    OCFIndex length = OCFStringGetLength(value);
    if ((_CFStringHasStrongOrWeakRTL(value, 0, length) & 1) != 0) {
        return true;
    }
    OCFIndex outputLength = OCFStringGetLength(outputString);
    if (outputLength != 0) {
        if (outputLength - lastAppendedLength <= 0) {
            return false;
        }
        return _CFStringHasStrongOrWeakRTL(
            outputString, lastAppendedLength, outputLength - lastAppendedLength
        );
    }
    const uint8_t *spec = (const uint8_t *)specs + ((OCFIndex)(int32_t)specIndex << 5);
    OCFIndex location = (OCFIndex) * (const int32_t *)(spec + 8) + (OCFIndex) * (const int32_t *)(spec + 4);
    if (formatLength - location <= 0) {
        return false;
    }
    return _CFStringHasStrongOrWeakRTL(formatString, location, formatLength - location);
}

// Native ___CFAppendFormattedValueWithStatisticalWritingDirections, 0x1c02255f0. Non-external.
// Without isolates, or for a lone directional mark, a plain append. Otherwise, when UIFoundation's
// LanguageAwareString feature is enabled and the value has statistical writing directions, the first entry
// (of several, those with Location and Length) whose WritingDirection is not -1 picks the opening isolate
// (-1 when none does); otherwise U+2068. The value and U+2069 follow.
static void __CFAppendFormattedValueWithStatisticalWritingDirections(
    OCFStringRef value, OCFMutableStringRef output, bool isDirectionalMark, bool useIsolates
) {
    if (!useIsolates || isDirectionalMark) {
        OCFStringAppend(output, value);
        return;
    }
    UniChar character;
    OCFArrayRef directions = NULL;
    if (_os_feature_enabled_impl("UIFoundation", "LanguageAwareString")) {
        directions = CFStringCopyStatisticalWritingDirections(value);
        OCFIndex count = OCFArrayGetCount(directions);
        if (count != 0) {
            int8_t direction = -1;
            for (OCFIndex index = 0; index < count && count >= 1; ++index) {
                if (index >= OCFArrayGetCount(directions)) {
                    continue;
                }
                OCFDictionaryRef entry = OCFArrayGetValueAtIndex(directions, index);
                OCFIndex range[2] = {0, 0};
                if (count >= 2) {
                    OCFNumberRef location = OCFDictionaryGetValue(entry, OF_STR("Location"));
                    if (location == NULL) {
                        continue;
                    }
                    OCFNumberGetValue(location, 14, &range[0]);
                    OCFNumberRef length = OCFDictionaryGetValue(entry, OF_STR("Length"));
                    if (length == NULL) {
                        continue;
                    }
                    OCFNumberGetValue(length, 14, &range[1]);
                }
                uint8_t writingDirection = 0;
                OCFNumberRef number = OCFDictionaryGetValue(entry, OF_STR("WritingDirection"));
                if (number == NULL) {
                    continue;
                }
                OCFNumberGetValue(number, 1, &writingDirection);
                if (writingDirection != 0xff) {
                    direction = (int8_t)writingDirection;
                    break;
                }
            }
            __CFAppendDirectionalIsolate(direction, output);
            goto appendValue;
        }
    }
    character = 0x2068;
    OCFStringAppendCharacters(output, &character, 1);
appendValue:
    OCFStringAppend(output, value);
    character = 0x2069;
    OCFStringAppendCharacters(output, &character, 1);
    if (directions != NULL) {
        OCFRelease(directions);
    }
}

// Native ___CFAppendFormattedValue, 0x1c0225800. Non-external.
// With isolates the value is wrapped in U+2068 FIRST STRONG ISOLATE and U+2069 POP DIRECTIONAL ISOLATE.
static void __CFAppendFormattedValue(OCFStringRef value, OCFMutableStringRef output, bool useIsolates) {
    if (useIsolates) {
        UniChar character = 0x2068;
        OCFStringAppendCharacters(output, &character, 1);
        OCFStringAppend(output, value);
        UniChar closing = 0x2069;
        OCFStringAppendCharacters(output, &closing, 1);
    } else {
        OCFStringAppend(output, value);
    }
}

// Native ___locked_CFStringFormatLocalizedNumberConfigureFormatter, 0x1c02258c4. Non-external.
// Resets a cached formatter: width 0, maximum and minimum fraction digits 0, padding position 0 and padding "
// ".
static void __locked_CFStringFormatLocalizedNumberConfigureFormatter(OCFNumberFormatterRef formatter) {
    _CFNumberFormatterSetInt32PropertyNoRefresh(formatter, kOCFNumberFormatterFormatWidthKey, 0);
    _CFNumberFormatterSetInt32PropertyNoRefresh(formatter, kOCFNumberFormatterMaxFractionDigitsKey, 0);
    _CFNumberFormatterSetInt32PropertyNoRefresh(formatter, kOCFNumberFormatterMinFractionDigitsKey, 0);
    _CFNumberFormatterSetInt32PropertyNoRefresh(formatter, kOCFNumberFormatterPaddingPositionKey, 0);
    _CFNumberFormatterSetPropertyNoRefresh(formatter, kOCFNumberFormatterPaddingCharacterKey, OF_STR(" "));
}

// Native __CFAppendDirectionalIsolate, 0x1c0225d04. Non-external. Appends one isolate initiator: U+2066
// (left-to-right) for direction 0, U+2067 (right-to-left) for 1 and U+2068 (first strong) for any other value
// (a 32-bit comparison).
void __CFAppendDirectionalIsolate(int32_t direction, OCFMutableStringRef outputString) {
    UniChar character;
    if (direction == 1) {
        character = 0x2067;
    } else if (direction != 0) {
        character = 0x2068;
    } else {
        character = 0x2066;
    }
    OCFStringAppendCharacters(outputString, &character, 1);
}

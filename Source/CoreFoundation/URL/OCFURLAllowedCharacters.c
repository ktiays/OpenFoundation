//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/URL/OCFURLAllowedCharacters.h"

#include "CoreFoundation/Base/OCFUtilities.h"

#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <stdlib.h>
#include <strings.h>

// Inferred organization: the URL allowed-character unit, iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified except as noted.
// iOS Device divergence: this unit remains unread on 24A446.
// Reading: Local/Evidence/cf-core/iphone-simulator-corpus-delta13.json, C2-strings-charsets-plist.

extern bool dyld_program_sdk_at_least(OCFBuildVersion version);
extern bool _os_feature_enabled_impl(const char *domain, const char *feature);

// The class reference 0x22229c238 is outside the extracted image. The selector and neighboring
// class reference order bind it to NSCharacterSet (confidence: medium), as in OCFCharacterSet.c.
extern char swiftCharacterSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSCharacterSet));

// Native sURLAllowedCharacters, 0x1c0456e32. Non-external, 128 halfwords.
static const uint16_t sURLAllowedCharacters[128] = {
    0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
    0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
    0x000, 0x000, 0x000, 0x4ee, 0x000, 0x000, 0x4ee, 0x000, 0x0ee, 0x4ee, 0x4ee, 0x4ee, 0x4ee, 0x4ef, 0x4ee,
    0x4ef, 0x4ef, 0x4e0, 0x5ff, 0x5ff, 0x5ff, 0x5ff, 0x5ff, 0x5ff, 0x5ff, 0x5ff, 0x5ff, 0x5ff, 0x4e0, 0x4ee,
    0x000, 0x0ee, 0x000, 0x4c0, 0x4e0, 0x7ef, 0x7ef, 0x7ef, 0x7ef, 0x7ef, 0x7ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef,
    0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef,
    0x6ef, 0x000, 0x000, 0x000, 0x000, 0x4ee, 0x000, 0x7ef, 0x7ef, 0x7ef, 0x7ef, 0x7ef, 0x7ef, 0x6ef, 0x6ef,
    0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef, 0x6ef,
    0x6ef, 0x6ef, 0x6ef, 0x000, 0x000, 0x000, 0x4ee, 0x000,
};

// Native _CFStringCreateByAddingPercentEncodingWithAllowedCharacters.hexchars, 0x1c0490948.
static const char hexchars[] = "0123456789ABCDEF";

static dispatch_once_t swiftCharacterSetOnce;
static bool swiftCharacterSetEnabled;
static dispatch_once_t urlAllowedCharacterSetsOnce;
static OCFCharacterSetRef *sURLAllowedCharacterSets;
static dispatch_once_t semicolonAllowedInPathOnce;
static bool semicolonAllowedInPath;

// The two entry points inline the same once checks. Keep the original separate predicates.
static inline bool checkSwiftCharacterSet(void) {
    // Native ___foundation_swift_characterSet_enabled_block_invoke_0, 0x1c02715a4.
    // OUTLINED_FUNCTION_0_36, 0x1c027171c, calls dispatch_once on this block.
    dispatch_once(&swiftCharacterSetOnce, ^{
      swiftCharacterSetEnabled = _os_feature_enabled_impl("Foundation", "SwiftNSCharacterSet") ||
                                 getenv("SwiftNSCharacterSet") != NULL;
    });
    return swiftCharacterSetEnabled;
}

static inline bool checkSemicolonAllowedInPath(void) {
    // Native ___CFURIParserSemicolonAllowedInPath_block_invoke, 0x1c030ef54.
    // OUTLINED_FUNCTION_2_20, 0x1c0271744, calls dispatch_once on this block.
    dispatch_once(&semicolonAllowedInPathOnce, ^{
      semicolonAllowedInPath = dyld_program_sdk_at_least((OCFBuildVersion){0xffffffff, 0x07e70901}) &&
                               _os_feature_enabled_impl("Foundation", "NSURLUseNewParsing");
    });
    return semicolonAllowedInPath;
}

static inline void initializeURLAllowedCharacterSets(void) {
    // Native __InitializeURLAllowedCharacterSets_block_invoke, 0x1c02715f0, with .cold.1 at 0x1c030f01c.
    // OUTLINED_FUNCTION_1_24, 0x1c0271730, calls dispatch_once on this block.
    dispatch_once(&urlAllowedCharacterSetsOnce, ^{
      OCFAllocatorRef allocator = kOCFAllocatorDefault;
      sURLAllowedCharacterSets = OCFAllocatorAllocateTyped(allocator, 0x38, 0x6004044c4a2df, 0);
      OCFStringRef user =
          OF_STR("!$&'()*+,-.0123456789;=ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~");
      sURLAllowedCharacterSets[0] = OCFCharacterSetCreateWithCharactersInString(allocator, user);
      sURLAllowedCharacterSets[1] = OCFCharacterSetCreateWithCharactersInString(allocator, user);
      sURLAllowedCharacterSets[2] = OCFCharacterSetCreateWithCharactersInString(
          allocator,
          OF_STR("!$&'()*+,-.0123456789:;=ABCDEFGHIJKLMNOPQRSTUVWXYZ[]_abcdefghijklmnopqrstuvwxyz~")
      );
      sURLAllowedCharacterSets[6] =
          OCFCharacterSetCreateWithCharactersInString(allocator, OF_STR("0123456789"));
      OCFStringRef path =
          checkSemicolonAllowedInPath()
              ? OF_STR("!$&'()*+,-./0123456789:;=@ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~")
              : OF_STR("!$&'()*+,-./0123456789:=@ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~");
      sURLAllowedCharacterSets[3] = OCFCharacterSetCreateWithCharactersInString(allocator, path);
      OCFStringRef query =
          OF_STR("!$&'()*+,-./0123456789:;=?@ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~");
      sURLAllowedCharacterSets[4] = OCFCharacterSetCreateWithCharactersInString(allocator, query);
      sURLAllowedCharacterSets[5] = OCFCharacterSetCreateWithCharactersInString(allocator, query);
    });
}

// Native _CFURLComponentsGetURLPathAllowedCharacterSet, 0x1c0270b84, with .cold.1 at 0x1c030efb0 and
// .cold.2 at 0x1c030efb4. External.
OCFCharacterSetRef _OCFURLComponentsGetURLPathAllowedCharacterSet(void) {
    if (checkSwiftCharacterSet()) {
        return ((OCFCharacterSetRef (*)(id, SEL))objc_msgSend)(
            (id)swiftCharacterSetClass, sel_getUid("URLPathAllowedCharacterSet")
        );
    }
    initializeURLAllowedCharacterSets();
    return sURLAllowedCharacterSets[3];
}

// Native _CFStringCreateByAddingPercentEncodingWithAllowedCharacters, 0x1c0270cc8. External.
// .cold.1 (0x1c030efc8) checks the Swift feature, .cold.2 (0x1c030efcc) initializes the sets, and
// .cold.3/.cold.4 (0x1c030eff0, 0x1c030f004) check the semicolon policy.
// CoreFoundation defect: the verified heap path at 0x1c0270de4 uses an unchecked allocation result.
// A failed allocation returns NULL before CFStringGetBytes and the subsequent buffer accesses.
OCFStringRef _OCFStringCreateByAddingPercentEncodingWithAllowedCharacters(
    OCFAllocatorRef allocator, OCFStringRef string, OCFCharacterSetRef allowedCharacters
) {
    if (checkSwiftCharacterSet()) {
        return ((OCFStringRef (*)(id, SEL, OCFCharacterSetRef))objc_msgSend)(
            (id)string,
            sel_getUid("__copySwiftStringByAddingPercentEncodingWithAllowedCharacters:"),
            allowedCharacters
        );
    }
    OCFIndex length = OCFStringGetLength(string);
    if (length == 0) {
        return OCFStringCreateCopy(allocator, string);
    }
    OCFIndex maximum = OCFStringGetMaximumSizeForEncoding(length, kOCFStringEncodingUTF8);
    if (maximum <= 0 || (uint64_t)maximum >= (UINT64_C(1) << 62)) {
        return NULL;
    }
    size_t capacity = 4 * (size_t)maximum;
    uint8_t stackBuffer[1024];
    uint8_t *buffer = stackBuffer;
    if (capacity <= sizeof(stackBuffer)) {
        bzero(buffer, capacity);
    } else {
        buffer = malloc_type_malloc(capacity, 0x100004077774924);
        if (buffer == NULL) {
            return NULL;
        }
    }
    OCFStringRef result = NULL;
    OCFIndex used = 0;
    if (OCFStringGetBytes(
            string, (OCFRange){0, length}, kOCFStringEncodingUTF8, 0, false, buffer, maximum, &used
        ) == length) {
        initializeURLAllowedCharacterSets();
        unsigned component = 7;
        if (allowedCharacters != NULL) {
            for (unsigned index = 0; index < 7; ++index) {
                if (allowedCharacters == sURLAllowedCharacterSets[index]) {
                    component = index;
                    break;
                }
            }
        }

        const uint16_t masks[] = {2, 4, 8, 32, 64, 128, 0};
        uint8_t *encoded = buffer + maximum;
        uint8_t *output = encoded;
        OCFIndex start = 0;
        bool bracketed = component == 2 && used > 1 && buffer[0] == '[' && buffer[used - 1] == ']';
        if (bracketed) {
            *output++ = '[';
            start = 1;
            --used;
        }
        bool firstSegment = true;
        for (OCFIndex index = start; index < used; ++index) {
            uint8_t byte = buffer[index];
            bool allowed = false;
            if (byte < 0x80) {
                if (component == 7) {
                    allowed = OCFCharacterSetIsCharacterMember(allowedCharacters, byte);
                } else {
                    allowed = (sURLAllowedCharacters[byte] & masks[component]) != 0;
                }
            }
            if (component == 2 && bracketed && byte == ':') {
                allowed = true;
            } else if (component == 3) {
                if (firstSegment && byte == ':') {
                    allowed = false;
                }
                if (!checkSemicolonAllowedInPath() && byte == ';') {
                    allowed = false;
                }
                firstSegment = firstSegment && byte != '/';
            }
            if (allowed) {
                *output++ = byte;
            } else {
                *output++ = '%';
                *output++ = (uint8_t)hexchars[byte >> 4];
                *output++ = (uint8_t)hexchars[byte & 0xf];
            }
        }
        if (bracketed) {
            *output++ = ']';
        }
        result = OCFStringCreateWithBytes(
            kOCFAllocatorDefault, encoded, output - encoded, kOCFStringEncodingUTF8, false
        );
    }
    if (buffer != stackBuffer) {
        free(buffer);
    }
    return result;
}

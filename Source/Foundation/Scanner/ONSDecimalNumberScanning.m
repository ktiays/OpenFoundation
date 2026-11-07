//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Scanner/ONSDecimalNumberScanning.h"
#import "Foundation/Decimal/ONSDecimalParsing.h"

#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/Locale/OCFLocaleKeys.h"
#include "CoreFoundation/String/OCFString.h"
#include <malloc/malloc.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// iOS Simulator 27.0 (24A434), arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified; iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Local/Scratch/cf-core-fd-misc/Reading.md; native source path unread, organization inferred.

// Layout read from scanDecimal:'s stack accesses, also passed to the unread _NSCheckDecimalSeparator.
struct ScannerStringBuffer {
    NSUInteger count;
    NSUInteger index;
    id string;
    NSUInteger length;
    NSUInteger start;
    unsigned short characters[32];
    unsigned short current;
};

OF_ABI_ASSERT(offsetof(struct ScannerStringBuffer, current) == 104, "Native scanner buffer current offset");

// Native _NSCheckDecimalSeparator 0x1c05c177c, private external, missing capture.
extern int _NSCheckDecimalSeparator(
    struct ScannerStringBuffer *buffer, unsigned short *current, unsigned int first, id separator
);
// Native uniqued GOT 0x22227f550 (medium): NSLocaleDecimalSeparator; CoreFoundation's nm binds that
// alias to the same data address as kCFLocaleDecimalSeparatorKey (0x2227a8910). Use its restored owner.

@protocol DecimalScanningMessages
- (id)objectForKey:(id)key;
- (NSUInteger)length;
- (unsigned short)characterAtIndex:(NSUInteger)index;
- (BOOL)characterIsMember:(unsigned short)character;
@end

// Native __NSNumberSet, 0x1c100e0b0. Private external; sixteen bytes read from the image.
// Hidden native name and linkage retained for other scanner entries when their group is captured.
const unsigned char __NSNumberSet[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0xff, 0x03, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00
};

@implementation ONSScanner (NSDecimalNumberScanning)

// Native -[NSScanner(NSDecimalNumberScanning) scanDecimal:], 0x1c04fb2f8. Non-external method.
// Foundation defect: 0x1c04fb640-0x1c04fb650 writes through an unchecked malloc result. Return NO on failure.
// Native 0x1c04fb734-0x1c04fb780 performs address arithmetic after free. Capture the consumed byte count
// before ending the C allocation lifetime, preserving native free-before-setScanLocation: ordering.
- (BOOL)scanDecimal:(ONSDecimal *)result {
    NSUInteger location = [self scanLocation];
    id string = [self string];
    id skip = [self charactersToBeSkipped];
    id locale = [self locale];
    struct ScannerStringBuffer buffer = {0};
    id separator = [locale objectForKey:(id)kOCFLocaleDecimalSeparatorKey];
    if (separator == nil) {
        separator = [locale objectForKey:(id)OF_STR("NSDecimalSeparator")];
    }
    if (separator == nil) {
        separator = (id)OF_STR(".");
    }
    unsigned int first = [separator length] != 0 ? [separator characterAtIndex:0] : '.';
    buffer.length = [string length];
    buffer.start = location;
    buffer.string = string;
    if (buffer.length > location) {
        buffer.count = buffer.length - location < 32 ? buffer.length - location : 32;
        ((void (*)(id, SEL, unsigned short *, NSUInteger, NSUInteger))objc_msgSend)(
            string, sel_getUid("getCharacters:range:"), buffer.characters, location, buffer.count
        );
        buffer.current = buffer.characters[0];
    } else {
        buffer.current = 0xffff;
    }
    buffer.index = 1;
    NSUInteger skipped = 0;
    if (skip != nil) {
        if ([skip characterIsMember:buffer.current] != 0) {
            do {
                if (buffer.index - 1 >= buffer.count) {
                    break;
                }
                if (buffer.index < buffer.count) {
                    buffer.current = buffer.characters[buffer.index++];
                } else {
                    NSUInteger next = buffer.start + buffer.count;
                    if (buffer.length > next) {
                        buffer.start = next;
                        buffer.count = buffer.length - next < 32 ? buffer.length - next : 32;
                        ((void (*)(id, SEL, unsigned short *, NSUInteger, NSUInteger))objc_msgSend)(
                            buffer.string,
                            sel_getUid("getCharacters:range:"),
                            buffer.characters,
                            next,
                            buffer.count
                        );
                        buffer.current = buffer.characters[0];
                        buffer.index = 1;
                    } else {
                        buffer.index = buffer.count + 1;
                        buffer.current = 0xffff;
                    }
                }
            } while (([skip characterIsMember:buffer.current] & 1) != 0);
        }
        skipped = buffer.index - 1;
    }
    NSUInteger start = buffer.start;
    unsigned short current = buffer.current;
    if (current <= 0x7f && ((unsigned int)(current - 9) < 5 || current == 32)) {
        return NO;
    }
    char stack[64];
    char *base = stack;
    char *write = stack;
    char *limit = stack + 63;
    char *allocation = NULL;
    size_t capacity = 0;
    for (;;) {
        if (current <= 0x7f) {
            if (((__NSNumberSet[current >> 3] >> (current & 7)) & 1) == 0) {
                if ((_NSCheckDecimalSeparator(&buffer, &current, first, separator) & 1) == 0) {
                    break;
                }
                current = '.';
            }
        } else {
            if (_NSCheckDecimalSeparator(&buffer, &current, first, separator) == 0) {
                break;
            }
            current = '.';
        }
        if (write >= limit) {
            capacity += 128;
            size_t used = write - base;
            char *larger;
            if (allocation != NULL) {
                larger = __OCFSafelyReallocateTyped(allocation, capacity, 0x7cadf66b, 0);
                if (larger == NULL) {
                    return NO;
                }
            } else {
                larger = malloc_type_malloc(capacity, UINT64_C(0x100004077774924));
                if (larger == NULL) {
                    return NO;
                }
                memcpy(larger, stack, sizeof(stack));
            }
            base = larger;
            write = larger + used;
            limit = larger + capacity - 1;
            allocation = larger;
        }
        *write++ = (char)current;
        if (buffer.index < buffer.count) {
            current = buffer.characters[buffer.index++];
        } else {
            NSUInteger next = buffer.start + buffer.count;
            if (buffer.length > next) {
                buffer.start = next;
                buffer.count = buffer.length - next < 32 ? buffer.length - next : 32;
                ((void (*)(id, SEL, unsigned short *, NSUInteger, NSUInteger))objc_msgSend)(
                    buffer.string, sel_getUid("getCharacters:range:"), buffer.characters, next, buffer.count
                );
                current = buffer.characters[0];
                buffer.index = 1;
            } else {
                buffer.index = buffer.count + 1;
                current = 0xffff;
            }
        }
        buffer.current = current;
    }
    *write = 0;
    char *end;
    ONSDecimal parsed = _NSCStringToDecimal(base, &end);
    size_t consumed = end - base;
    if (allocation != NULL) {
        free(allocation);
    }
    if (consumed == 0) {
        return NO;
    }
    if (result != NULL) {
        *result = parsed;
    }
    [self setScanLocation:start + skipped + consumed];
    return YES;
}

@end

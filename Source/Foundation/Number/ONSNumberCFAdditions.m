//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Number/ONSNumberCFAdditions.h"

#include "CoreFoundation/Number/OCFNumber.h"

#include <string.h>

// Inferred organization: the NSCFAdditions methods (0x1c04d1d44-0x1c04d2200: NSCFAttributedString's methods
// and the NSAttributedString, NSNumber, NSString and NSMutableString categories) lie together in the iOS
// Simulator
// __text section and form one native unit, shared with other families; this file holds the NSNumber category
// (0x1c04d1d5c-0x1c04d1f63), named for the native cluster. Objective-C compiled with manual reference
// counting. Unless a definition says otherwise: Foundation, iOS Simulator 24A434, UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Source/Foundation/Number/Reading.md.

@implementation ONSNumber (NSCFAdditions)

// Native -[NSNumber(NSCFAdditions) _cfTypeID], 0x1c04d1d5c. Non-external; a tail call of CFNumberGetTypeID.
- (OCFTypeID)_cfTypeID {
    return OCFNumberGetTypeID();
}

// Native -[NSNumber(NSCFAdditions) _cfNumberType], 0x1c04d1d60. Non-external. The CFNumber type for the first
// -objCType character: 'B' and 'c' kCFNumberCharType (7), 'C' and 's' kCFNumberShortType (8), 'S' and 'i'
// kCFNumberIntType (9), 'l' kCFNumberLongType (10), 'q' kCFNumberLongLongType (11), 'f' kCFNumberFloatType
// (12), 'd' kCFNumberDoubleType (13), 'Q' the private 128-bit type 17; 'I' is kCFNumberIntType when
// -unsignedIntValue is non-negative as an int and kCFNumberLongLongType otherwise, 'L' kCFNumberLongType when
// -unsignedLongValue is non-negative as a long and kCFNumberLongLongType otherwise. Any other character traps
// (BRK #1).
- (OCFNumberType)_cfNumberType {
    switch (*[self objCType]) {
        case 'B':
        case 'c':
            return 7;
        case 'C':
        case 's':
            return 8;
        case 'I':
            return (int)[self unsignedIntValue] > -1 ? 9 : 11;
        case 'L':
            return (long)[self unsignedLongValue] <= -1 ? 11 : 10;
        case 'Q':
            return 17;
        case 'S':
        case 'i':
            return 9;
        case 'd':
            return 13;
        case 'f':
            return 12;
        case 'l':
            return 10;
        case 'q':
            return 11;
        default:
            __builtin_trap();
    }
}

// Native -[NSNumber(NSCFAdditions) _getValue:forType:], 0x1c04d1e80. Non-external. Stores the value for the
// CFNumber type: 1 -charValue (a byte), 2 -shortValue, 3 -intValue, 4 -longLongValue, 5 -floatValue, 6
// -doubleValue, 17 -longLongValue as a 128-bit value (the sign-extension word, then the value); answers YES.
// Any other type traps (BRK #1).
- (BOOL)_getValue:(void *)value forType:(OCFNumberType)type {
    switch ((int64_t)type) {
        case 1: {
            char number = [self charValue];
            memcpy(value, &number, sizeof(number));
            break;
        }
        case 2: {
            short number = [self shortValue];
            memcpy(value, &number, sizeof(number));
            break;
        }
        case 3: {
            int number = [self intValue];
            memcpy(value, &number, sizeof(number));
            break;
        }
        case 4: {
            long long number = [self longLongValue];
            memcpy(value, &number, sizeof(number));
            break;
        }
        case 5: {
            float number = [self floatValue];
            memcpy(value, &number, sizeof(number));
            break;
        }
        case 6: {
            double number = [self doubleValue];
            memcpy(value, &number, sizeof(number));
            break;
        }
        case 17: {
            long long integer = [self longLongValue];
            int64_t words[2] = {integer >> 63, integer};
            memcpy(value, words, sizeof(words));
            break;
        }
        default:
            __builtin_trap();
    }
    return YES;
}

// Native -[NSNumber(NSCFAdditions) _reverseCompare:], 0x1c04d1f40. Non-external. -compare: with the order
// reversed: NSOrderedAscending answers NSOrderedDescending, NSOrderedDescending NSOrderedAscending, anything
// else NSOrderedSame.
- (NSInteger)_reverseCompare:(id)otherNumber {
    NSInteger result = [self compare:otherNumber];
    NSInteger reversed = result == 1 ? -1 : 0;
    return result == -1 ? 1 : reversed;
}

@end

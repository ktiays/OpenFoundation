//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSString.h"

// The string scanning functions (String/ONSStringScanning.m), iOS Simulator 24A434, arm64, UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified unless a declaration says otherwise. iOS Device
// 24A446: unread. The functions are hidden (were private externals).

// The character buffer the scanning functions keep on the stack and pass to __NSCheckDecimalSeparator (0x70
// bytes): the characters fetched, the next index into them, the string, its length, the location of the
// fetched run, 32 fetched characters and the current character (0xFFFF past the end). StringScanBuffer and
// its field names are local descriptive names (layout from the offsets the functions use).
typedef struct {
    ONSUInteger count;
    ONSUInteger index;
    id string;
    ONSUInteger length;
    ONSUInteger location;
    unsigned short characters[32];
    unsigned short current;
} StringScanBuffer;

_Static_assert(sizeof(StringScanBuffer) == 0x70, "The scanning character buffer is 0x70 bytes.");

// Native __NSScanLongLongFromString (0x1c05c0c74) and __NSScanIntFromString (0x1c05c11f8): skip the skip
// set's characters, an optional sign and the skip set again, then scan decimal digits from *location. The
// result is the digit set's answer for the first character (NO: nothing is stored); on YES the value (clamped
// to the type's range on overflow) is stored through value when it is not NULL and *location moves past the
// digits.
OF_EXTERN BOOL _NSScanLongLongFromString(id string, id skipSet, long long *value, ONSUInteger *location);
OF_EXTERN BOOL _NSScanIntFromString(id string, id skipSet, int *value, ONSUInteger *location);

// Native __NSCheckDecimalSeparator, 0x1c05c177c: whether the current character starts the decimal separator
// (separatorCharacter is its first character; a nil separator counts as one character); a longer separator
// consumes its remaining characters from the buffer, storing each into *character.
OF_EXTERN BOOL _NSCheckDecimalSeparator(
    StringScanBuffer *buffer, unsigned short *character, unsigned short separatorCharacter, id separator
);

// Native __NSScanDoubleFromString, 0x1c05c18d0. Hidden (was a private external): scans normalized ASCII
// bytes with strtod_l and a NULL locale. The reallocation-failure block (0x1c05c1de0) was read in IDA on
// 2026/11/03: free the original pointer and set *stop to true.
OF_EXTERN BOOL
_NSScanDoubleFromString(id string, id skipSet, double *value, ONSUInteger *location, id locale);

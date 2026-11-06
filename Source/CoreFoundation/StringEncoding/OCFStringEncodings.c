//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/StringEncoding/OCFStringEncodings.h"

// Inferred organization: CFStringConvertEncodingToNSStringEncoding (0x1c02340fc) follows
// CFStringConvertEncodingToIANACharSetName in the iOS Simulator __text section, and
// CFStringConvertNSStringEncodingToEncoding (0x1c0234254) follows it. The native source path is not in the
// evidence.

// Native CFStringConvertEncodingToNSStringEncoding, iOS Simulator 24A434, 0x1c02340fc,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External.
// iOS Device 24A446: 0x18062dab0, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified; the
// comparison tree is ordered differently (0x100 is tested first after the two exact values) and maps the same
// values. Every result is written to W0, so the 64-bit result is zero-extended; an unmapped encoding returns
// encoding | 0x80000000. A low twelve-bit field of zero maps to 30 for any upper bits.
// Reading: Documentation/Reference/Research/CoreFoundationStringEncoding.md.
unsigned long OCFStringConvertEncodingToNSStringEncoding(uint32_t encoding) {
    if (encoding == 0x8000100) {
        return 4;
    }
    if (encoding == 0x600) {
        return 1;
    }
    switch (encoding & 0xfff) {
        case 0:
            return 30;
        case 0x21:
            return 6;
        case 0x100:
            if (encoding == 0x100) {
                return 10;
            }
            break;
        case 0x201:
            return 5;
        case 0x202:
            return 9;
        case 0x420:
            return 8;
        case 0x500:
            return 12;
        case 0x501:
            return 15;
        case 0x502:
            return 11;
        case 0x503:
            return 13;
        case 0x504:
            return 14;
        case 0x820:
            return 21;
        case 0x920:
            return 3;
        case 0xb01:
            return 2;
        case 0xbff:
            return 7;
        default:
            break;
    }
    return encoding | UINT32_C(0x80000000);
}

// The NSStringEncoding values 1-15 as CFStringEncodings, 15 halfwords at 0x1c03a424c (__TEXT,__const; no
// symbol). Entry 4 (NSUTF8StringEncoding) is 0 and never read.
static const uint16_t kNSStringEncodingToEncoding[15] = {
    0x600,
    0xb01,
    0x920,
    0x0,
    0x201,
    0x21,
    0xbff,
    0x420,
    0x202,
    0x100,
    0x502,
    0x500,
    0x503,
    0x504,
    0x501,
};

// Native CFStringConvertNSStringEncodingToEncoding, iOS Simulator 24A434, 0x1c0234254,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (delta capture 4). External. 4 maps to
// 0x8000100 and 1-15 through the table. Otherwise the low 32 bits with their sign bit set give the low 31
// bits, and any other value 0xFFFFFFFF; 21 then maps to 0x820 and 30 to 0 (both compared as 64-bit values).
// iOS Device 24A446: unread.
uint32_t OCFStringConvertNSStringEncodingToEncoding(unsigned long encoding) {
    if (encoding == 4) {
        return 0x8000100;
    }
    if (encoding - 1 <= 14) {
        return kNSStringEncodingToEncoding[encoding - 1];
    }
    uint32_t result = (int32_t)encoding <= -1 ? (uint32_t)encoding & 0x7FFFFFFF : 0xFFFFFFFF;
    if (encoding == 21) {
        result = 0x820;
    }
    if (encoding == 30) {
        return 0;
    }
    return result;
}

//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Number/OCFNumber.h"

#include <stdbool.h>
#include <stddef.h>

// Native powersOf10, iOS Simulator 24A434, 0x1c03752c0 (624 bytes: 39 high/low pairs, 10^38 down to
// 10^0), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x1808314e0, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// the 624 bytes agree. The length is the 39-iteration, 16-byte-stride loop of emit128.
// Reading: Documentation/Reference/Research/CoreFoundationBigNum.md.
static const Int128Parts powersOf10[39] = {
    {(int64_t)UINT64_C(0x4b3b4ca85a86c47a), UINT64_C(0x098a224000000000)},
    {(int64_t)UINT64_C(0x0785ee10d5da46d9), UINT64_C(0x00f436a000000000)},
    {(int64_t)UINT64_C(0x00c097ce7bc90715), UINT64_C(0xb34b9f1000000000)},
    {(int64_t)UINT64_C(0x0013426172c74d82), UINT64_C(0x2b878fe800000000)},
    {(int64_t)UINT64_C(0x0001ed09bead87c0), UINT64_C(0x378d8e6400000000)},
    {(int64_t)UINT64_C(0x0000314dc6448d93), UINT64_C(0x38c15b0a00000000)},
    {(int64_t)UINT64_C(0x000004ee2d6d415b), UINT64_C(0x85acef8100000000)},
    {(int64_t)UINT64_C(0x0000007e37be2022), UINT64_C(0xc0914b2680000000)},
    {(int64_t)UINT64_C(0x0000000c9f2c9cd0), UINT64_C(0x4674edea40000000)},
    {(int64_t)UINT64_C(0x00000001431e0fae), UINT64_C(0x6d7217caa0000000)},
    {(int64_t)UINT64_C(0x00000000204fce5e), UINT64_C(0x3e25026110000000)},
    {(int64_t)UINT64_C(0x00000000033b2e3c), UINT64_C(0x9fd0803ce8000000)},
    {(int64_t)UINT64_C(0x000000000052b7d2), UINT64_C(0xdcc80cd2e4000000)},
    {(int64_t)UINT64_C(0x0000000000084595), UINT64_C(0x161401484a000000)},
    {(int64_t)UINT64_C(0x000000000000d3c2), UINT64_C(0x1bcecceda1000000)},
    {(int64_t)UINT64_C(0x000000000000152d), UINT64_C(0x02c7e14af6800000)},
    {(int64_t)UINT64_C(0x000000000000021e), UINT64_C(0x19e0c9bab2400000)},
    {(int64_t)UINT64_C(0x0000000000000036), UINT64_C(0x35c9adc5dea00000)},
    {(int64_t)UINT64_C(0x0000000000000005), UINT64_C(0x6bc75e2d63100000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x8ac7230489e80000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x0de0b6b3a7640000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x016345785d8a0000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x002386f26fc10000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x00038d7ea4c68000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x00005af3107a4000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x000009184e72a000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x000000e8d4a51000)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x000000174876e800)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x00000002540be400)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x000000003b9aca00)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x0000000005f5e100)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x0000000000989680)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x00000000000f4240)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x00000000000186a0)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x0000000000002710)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x00000000000003e8)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000064)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x000000000000000a)},
    {(int64_t)UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000001)},
};

// Native neg_powersOf10, iOS Simulator 24A434, 0x1c0375530 (624 bytes: the 39 negated powers of ten,
// -10^38 up to -10^0), UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x180831750, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified;
// the 624 bytes agree.
// Reading: Documentation/Reference/Research/CoreFoundationBigNum.md.
static const Int128Parts neg_powersOf10[39] = {
    {(int64_t)UINT64_C(0xb4c4b357a5793b85), UINT64_C(0xf675ddc000000000)},
    {(int64_t)UINT64_C(0xf87a11ef2a25b926), UINT64_C(0xff0bc96000000000)},
    {(int64_t)UINT64_C(0xff3f68318436f8ea), UINT64_C(0x4cb460f000000000)},
    {(int64_t)UINT64_C(0xffecbd9e8d38b27d), UINT64_C(0xd478701800000000)},
    {(int64_t)UINT64_C(0xfffe12f64152783f), UINT64_C(0xc872719c00000000)},
    {(int64_t)UINT64_C(0xffffceb239bb726c), UINT64_C(0xc73ea4f600000000)},
    {(int64_t)UINT64_C(0xfffffb11d292bea4), UINT64_C(0x7a53107f00000000)},
    {(int64_t)UINT64_C(0xffffff81c841dfdd), UINT64_C(0x3f6eb4d980000000)},
    {(int64_t)UINT64_C(0xfffffff360d3632f), UINT64_C(0xb98b1215c0000000)},
    {(int64_t)UINT64_C(0xfffffffebce1f051), UINT64_C(0x928de83560000000)},
    {(int64_t)UINT64_C(0xffffffffdfb031a1), UINT64_C(0xc1dafd9ef0000000)},
    {(int64_t)UINT64_C(0xfffffffffcc4d1c3), UINT64_C(0x602f7fc318000000)},
    {(int64_t)UINT64_C(0xffffffffffad482d), UINT64_C(0x2337f32d1c000000)},
    {(int64_t)UINT64_C(0xfffffffffff7ba6a), UINT64_C(0xe9ebfeb7b6000000)},
    {(int64_t)UINT64_C(0xffffffffffff2c3d), UINT64_C(0xe43133125f000000)},
    {(int64_t)UINT64_C(0xffffffffffffead2), UINT64_C(0xfd381eb509800000)},
    {(int64_t)UINT64_C(0xfffffffffffffde1), UINT64_C(0xe61f36454dc00000)},
    {(int64_t)UINT64_C(0xffffffffffffffc9), UINT64_C(0xca36523a21600000)},
    {(int64_t)UINT64_C(0xfffffffffffffffa), UINT64_C(0x9438a1d29cf00000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0x7538dcfb76180000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xf21f494c589c0000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfe9cba87a2760000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffdc790d903f0000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfffc72815b398000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffffa50cef85c000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfffff6e7b18d6000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffffff172b5af000)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffe8b7891800)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfffffffdabf41c00)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffc4653600)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfffffffffa0a1f00)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffff676980)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfffffffffff0bdc0)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfffffffffffe7960)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffd8f0)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfffffffffffffc18)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffff9c)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xfffffffffffffff6)},
    {(int64_t)UINT64_C(0xffffffffffffffff), UINT64_C(0xffffffffffffffff)},
};

// Native emit128, iOS Simulator 24A434, 0x1c01ac500,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Non-external.
// iOS Device 24A446: 0x18069c78c, UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// iOS Device divergence: emit128 at 0x18069c78c moves the negation block after the return and
// allocates other scratch registers; comparisons, table reads, stores and order agree.
// The value is a high/low pair read from memory. A negative value is negated in 128 bits and
// prefixed with '-'; forcePlus prefixes '+' otherwise. Each of the 39 decimal positions repeatedly
// adds the negated power while the power is not greater than the remainder (signed high, unsigned low
// comparison). Leading zero positions are skipped, a lone '0' is written when every position was
// skipped, and a NUL ends the string. The 128-bit minimum stays negative after negation and is written
// as "-0", as in the native code. The three native callers ignore the result register; the
// restoration returns void.
// Reading: Documentation/Reference/Research/CoreFoundationBigNum.md.
void emit128(char *buffer, const Int128Parts *value, uint8_t forcePlus) {
    uint64_t high = (uint64_t)value->high;
    uint64_t low = value->low;
    if ((int64_t)high < 0) {
        high = low == 0 ? -high : ~high;
        low = -low;
        *buffer++ = '-';
    } else if (forcePlus) {
        *buffer++ = '+';
    }
    bool leading = true;
    for (size_t index = 0; index != 39; ++index) {
        uint8_t digit = 0;
        const Int128Parts *power = &powersOf10[index];
        const Int128Parts *negated = &neg_powersOf10[index];
        int64_t powerHigh = power->high;
        while (powerHigh < (int64_t)high || (powerHigh <= (int64_t)high && power->low <= low)) {
            uint64_t addend = negated->low;
            high += (uint64_t)negated->high + (low + addend < low);
            low += addend;
            ++digit;
        }
        if (digit != 0 || !leading) {
            leading = false;
            *buffer++ = (char)(digit + '0');
        }
    }
    if (leading) {
        *buffer++ = '0';
    }
    *buffer = 0;
}

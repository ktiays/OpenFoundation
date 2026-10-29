//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "Foundation/Range/RangeStandaloneTests.h"

#include <OpenFoundation/Range/ONSRange.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>

// Native NSUnionRange: iOS Simulator 24A434, 0x1c05bd644, Foundation
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; iOS Device 24A446, 0x180f67520,
// C041F1D0-5EF1-30A3-BF96-830976173B56. Confidence: verified.
// Native NSIntersectionRange: same images, 0x1c05bd664 / 0x180e02318.
// Reading: Documentation/Reference/Research/FoundationRange.md.
// Literal fixtures preserve the read unsigned arithmetic, ordered tests and empty-range results.
typedef struct RangeCase {
    ONSRange first;
    ONSRange second;
    ONSRange unionResult;
    ONSRange intersectionResult;
} RangeCase;

static const RangeCase kRangeCases[] = {
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{2, 3}, {2, 3}, {2, 3}, {2, 3}},
    {{2, 3}, {4, 4}, {2, 6}, {4, 1}},
    {{4, 4}, {2, 3}, {2, 6}, {4, 1}},
    {{2, 10}, {4, 2}, {2, 10}, {4, 2}},
    {{4, 2}, {2, 10}, {2, 10}, {4, 2}},
    {{1, 2}, {5, 2}, {1, 6}, {0, 0}},
    {{5, 2}, {1, 2}, {1, 6}, {0, 0}},
    {{2, 3}, {5, 4}, {2, 7}, {0, 0}},
    {{5, 4}, {2, 3}, {2, 7}, {0, 0}},
    {{5, 0}, {5, 0}, {5, 0}, {0, 0}},
    {{5, 0}, {2, 10}, {2, 10}, {5, 0}},
    {{2, 10}, {5, 0}, {2, 10}, {5, 0}},
    {{2, 0}, {2, 10}, {2, 10}, {2, 0}},
    {{2, 10}, {2, 0}, {2, 10}, {2, 0}},
    {{12, 0}, {2, 10}, {2, 10}, {0, 0}},
    {{2, 10}, {12, 0}, {2, 10}, {0, 0}},
    {{ULONG_MAX - 3, 2}, {ULONG_MAX - 2, 1}, {ULONG_MAX - 3, 2}, {ULONG_MAX - 2, 1}},
    {{ULONG_MAX - 1, 4}, {0, 3}, {0, 3}, {0, 0}},
    {{0, 3}, {ULONG_MAX - 1, 4}, {0, 3}, {0, 0}},
    {{ULONG_MAX - 1, 4}, {ULONG_MAX - 1, 1}, {ULONG_MAX - 1, 1}, {ULONG_MAX - 1, 4}},
    {{ULONG_MAX - 1, 1}, {ULONG_MAX - 1, 4}, {ULONG_MAX - 1, 1}, {ULONG_MAX - 1, 4}},
    {{ULONG_MAX, 2}, {ULONG_MAX, 4}, {ULONG_MAX, 4}, {0, 0}},
    {{0, ULONG_MAX}, {ULONG_MAX, 0}, {0, ULONG_MAX}, {0, 0}},
    {{0, ULONG_MAX}, {ULONG_MAX - 1, 1}, {0, ULONG_MAX}, {ULONG_MAX - 1, 1}},
    {{0, 0}, {ULONG_MAX, ULONG_MAX}, {0, ULONG_MAX - 1}, {0, 0}},
    {{0, ULONG_MAX}, {0, ULONG_MAX}, {0, ULONG_MAX}, {0, ULONG_MAX}},
    {{ULONG_MAX, 0}, {ULONG_MAX, 0}, {ULONG_MAX, 0}, {0, 0}},
    {{ULONG_MAX - 3, 8}, {ULONG_MAX - 2, 8}, {ULONG_MAX - 3, 9}, {0, 0}},
    {{ULONG_MAX - 3, 8}, {ULONG_MAX - 3, 5}, {ULONG_MAX - 3, 8}, {0, 0}},
};

int runFoundationRangeStandaloneTests(void) {
    size_t count = sizeof(kRangeCases) / sizeof(kRangeCases[0]);
    for (size_t index = 0; index < count; ++index) {
        const RangeCase *test = &kRangeCases[index];
        ONSRange actualUnion = ONSUnionRange(test->first, test->second);
        ONSRange actualIntersection = ONSIntersectionRange(test->first, test->second);
        if (memcmp(&actualUnion, &test->unionResult, sizeof(actualUnion)) != 0) {
            fprintf(
                stderr,
                "Foundation range union mismatch at fixture %zu: {%lu, %lu}.\n",
                index,
                actualUnion.location,
                actualUnion.length
            );
            return 1;
        }
        if (memcmp(&actualIntersection, &test->intersectionResult, sizeof(actualIntersection)) != 0) {
            fprintf(
                stderr,
                "Foundation range intersection mismatch at fixture %zu: {%lu, %lu}.\n",
                index,
                actualIntersection.location,
                actualIntersection.length
            );
            return 1;
        }
    }
    printf(
        "Foundation range standalone passed: %zu fixtures, %zu exact comparisons across 2 APIs.\n",
        count,
        count * 2
    );
    return 0;
}

//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/PropertyList/TopLevelInfoStandaloneTests.h"
#include "CoreFoundation/PropertyList/OCFBinaryPList.h"

#include <stdio.h>
#include <string.h>

// "bplist00", one false object (0x08) at offset 8, a one-entry offset table at 9 and the trailer:
// six unused bytes, widths 1 and 1, one object, top object 0, table offset 9.
static const uint8_t kMinimalList[42] = {
    'b', 'p', 'l', 'i', 's', 't', '0', '0', 0x08, 0x08, 1, 2, 3, 4, 5, 6, 1, 1, 0, 0, 0,
    0,   0,   0,   0,   1,   0,   0,   0,   0,    0,    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9,
};

static int expectFailure(const uint8_t *data, uint64_t length, int line) {
    uint8_t marker = 0x5a;
    uint64_t offset = 77;
    if (__OCFBinaryPlistGetTopLevelInfo(data, length, &marker, &offset, NULL) || marker != 0x5a ||
        offset != 77) {
        fprintf(stderr, "Top-level-info standalone failure case accepted at source line %d.\n", line);
        return 1;
    }
    return 0;
}

// Native __CFBinaryPlistGetTopLevelInfo 0x1c0163b94, iOS Simulator 24A434
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. Device comparison: 0x18061ce78,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
int runCoreFoundationTopLevelInfoStandaloneTests(void) {
    size_t cases = 0;
    uint8_t marker = 0;
    uint64_t offset = 0;
    BinaryPlistTrailer trailer;
    memset(&trailer, 0xa5, sizeof(trailer));
    if (!__OCFBinaryPlistGetTopLevelInfo(kMinimalList, sizeof(kMinimalList), &marker, &offset, &trailer) ||
        marker != 0x08 || offset != 8 || trailer.offsetIntSize != 1 || trailer.objectRefSize != 1 ||
        trailer.numObjects != 1 || trailer.topObject != 0 || trailer.offsetTableOffset != 9 ||
        memcmp(trailer.unused, kMinimalList + 10, 6) != 0) {
        fputs("Top-level-info standalone minimal list rejected or misread.\n", stderr);
        return 1;
    }
    ++cases;
    if (!__OCFBinaryPlistGetTopLevelInfo(kMinimalList, sizeof(kMinimalList), NULL, NULL, NULL)) {
        fputs("Top-level-info standalone minimal list rejected without outputs.\n", stderr);
        return 1;
    }
    ++cases;
    uint8_t list[sizeof(kMinimalList)];
    if (expectFailure(NULL, sizeof(kMinimalList), __LINE__) != 0 ||
        expectFailure(kMinimalList, sizeof(kMinimalList) - 1, __LINE__) != 0) {
        return 1;
    }
    cases += 2;
    // The version digit after "bplist0" is not tested.
    memcpy(list, kMinimalList, sizeof(list));
    list[7] = '9';
    if (!__OCFBinaryPlistGetTopLevelInfo(list, sizeof(list), NULL, NULL, NULL)) {
        fputs("Top-level-info standalone version digit was tested.\n", stderr);
        return 1;
    }
    ++cases;

    // A wrong header byte, a top object outside the count, an offset entry at the table and an offset
    // below 8 each fail.
    const struct {
        size_t index;
        uint8_t value;
        int line;
    } failures[] = {
        {6, '1', __LINE__}, {33, 1, __LINE__}, {9, 9, __LINE__}, {9, 7, __LINE__}, {16, 0, __LINE__}
    };

    for (size_t index = 0; index < sizeof(failures) / sizeof(failures[0]); ++index) {
        memcpy(list, kMinimalList, sizeof(list));
        list[failures[index].index] = failures[index].value;
        if (expectFailure(list, sizeof(list), failures[index].line) != 0) {
            return 1;
        }
        ++cases;
    }
    printf("Binary-plist top-level-info standalone cases passed: %zu.\n", cases);
    return 0;
}

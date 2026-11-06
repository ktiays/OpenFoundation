//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

// Native CFSortIndexes, 0x1c01fa4b4. External. Fills indexBuffer with 0..count-1 and sorts it stably with the
// comparator block; bit 0 of opts requests the concurrent merge sort.
OF_EXPORT void OCFSortIndexes(
    OCFIndex *indexBuffer, OCFIndex count, OCFOptionFlags opts, OCFComparisonResult (^cmp)(OCFIndex, OCFIndex)
);

// Native CFQSortArray, 0x1c01fac1c. External; the qsort of element arrays through CFSortIndexes.
OF_EXPORT void OCFQSortArray(
    void *list, OCFIndex count, OCFIndex elementSize, OCFComparatorFunction comparator, void *context
);

//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Collections/OCFSortFunctions.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: CFSortIndexes (0x1c01fa4b4) through __CFSortIndexesN_block_invoke_3 (0x1c01fb3c8)
// are one contiguous cluster of the iOS Simulator __text section, interleaved with __CFSimpleMergeSort and
// CFQSortArray (restored here in native order) and CFMergeSortArray (not restored here), and are kept in one
// translation unit; the native source path is not in the evidence. Unless a definition says otherwise:
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (disassembly); iOS
// Device 24A446: unread. Reading: Local/Scratch/cf-core/tasks/array-storage.txt (sub-track reading; no
// versioned reading yet).

static void __CFSortIndexesN(
    OCFIndex listp[], OCFIndex count, int32_t ncores, OCFComparisonResult (^cmp)(OCFIndex, OCFIndex)
);
static void __CFSimpleMergeSort(
    OCFIndex listp[], OCFIndex cnt, OCFIndex tmp[], OCFComparisonResult (^cmp)(OCFIndex, OCFIndex)
);
static void __CFSortIndexesNMerge(
    OCFIndex listp1[],
    OCFIndex cnt1,
    OCFIndex listp2[],
    OCFIndex cnt2,
    OCFIndex tmp[],
    size_t right,
    OCFComparisonResult (^cmp)(OCFIndex, OCFIndex)
);

// Native CFSortIndexes, 0x1c01fa4b4. External. A count above 2^60 - 1 is CFSortIndexes.cold.1 (0x1c030c3ac,
// crash annotation, BRK #1). The concurrent option (bit 0) asks __CFActiveProcessorCount and is dropped below
// 160 values or 2 processors; otherwise the count limits the cores to 2 (below 640), 4 (below 3,200) or 8
// (below 16,000), and at most 16 are used. Up to 65,536 values the indices are filled in a loop; above, a
// concurrent queue "com.apple.NSSortIndexes" is created (attribute DISPATCH_QUEUE_CONCURRENT, the image's
// only imported queue attribute; confidence: medium for that binding) and released unused, and dispatch_apply
// fills 8 chunks of ((count + 15) / 16) * 2 indices. The concurrent sort is __CFSortIndexesN; the serial one
// is
// __CFSimpleMergeSort with a stack buffer of count values (up to 4,096, else one value) or a malloc buffer
// (malloc type 0x100004000313F17) that is freed afterwards. The filling block is __CFSortIndexes_block_invoke
// (0x1c01fa70c, non-external).
void OCFSortIndexes(
    OCFIndex *indexBuffer, OCFIndex count, OCFOptionFlags opts, OCFComparisonResult (^cmp)(OCFIndex, OCFIndex)
) {
    if (count < 1) {
        return;
    }
    if (((uint64_t)count >> 60) != 0) {
        crashWithMessage("Size of array to be sorted is too big");
    }
    int32_t ncores = 0;
    if ((opts & 1) != 0) {
        ncores = __CFActiveProcessorCount();
        if ((uint64_t)count < 160 || ncores < 2) {
            opts = 0;
        } else if ((uint64_t)count <= 639 && (uint32_t)ncores > 2) {
            ncores = 2;
        } else if ((uint64_t)count <= 3199 && (uint32_t)ncores > 4) {
            ncores = 4;
        } else if (((uint64_t)count >> 7) < 125) {
            ncores = (uint32_t)ncores < 8 ? ncores : 8;
        }
        ncores = ncores < 16 ? ncores : 16;
    }
    if ((uint64_t)count <= 65536) {
        for (OCFIndex idx = 0; idx != count; idx++) {
            indexBuffer[idx] = idx;
        }
    } else {
        dispatch_queue_t queue = dispatch_queue_create("com.apple.NSSortIndexes", DISPATCH_QUEUE_CONCURRENT);
        OCFIndex sz = (OCFIndex)((((uint64_t)count + 15) >> 3) & 0x3FFFFFFFFFFFFFFE);
        dispatch_apply(8, NULL, ^(size_t n) {
          OCFIndex idx = sz * (OCFIndex)n;
          OCFIndex lim = idx + sz < count ? idx + sz : count;
          for (; idx < lim; idx++) {
              indexBuffer[idx] = idx;
          }
        });
        dispatch_release(queue);
    }
    if ((opts & 1) != 0) {
        __CFSortIndexesN(indexBuffer, count, ncores, cmp);
        return;
    }
    OCFIndex local[(uint64_t)count <= 4096 ? count : 1];
    OCFIndex *tmp = local;
    if ((uint64_t)count > 4096) {
        tmp = malloc_type_malloc((size_t)count * sizeof(OCFIndex), 0x100004000313F17);
    }
    __CFSimpleMergeSort(indexBuffer, count, tmp, cmp);
    if (local != tmp) {
        free(tmp);
    }
}

// Native __CFSortIndexesN, 0x1c01fa740. Non-external. Sections of sz values (the per-core share rounded up to
// a multiple of 16, signed divisions) are sorted concurrently into place with one malloc buffer each (malloc
// type 0x100004000313F17); then (num_sect + 1) / 2 rounds each run an even phase (merging into the buffers)
// and an odd phase (merging back into the list), with the unpaired section copied around them. The native
// code zeroes the first min(8 * num_sect, 512) bytes of the stack table of buffers before filling it. Its
// blocks are
// __CFSortIndexesN_block_invoke (0x1c01fb17c, the section sort; the last section has last_sect_len values),
// __CFSortIndexesN_block_invoke_2 (0x1c01fb1b4, the even phase; the pair's second section is the last one
// when sect + 2 - right equals num_sect) and __CFSortIndexesN_block_invoke_3 (0x1c01fb3c8, the odd phase; the
// last one when (sect & ~1) + 3 equals num_sect), all non-external.
static void __CFSortIndexesN(
    OCFIndex listp[], OCFIndex count, int32_t ncores, OCFComparisonResult (^cmp)(OCFIndex, OCFIndex)
) {
    OCFIndex sz = (((count + ncores - 1) / ncores + 15) / 16) * 16;
    OCFIndex num_sect = (count + sz - 1) / sz;
    OCFIndex last_sect_len = count + sz - sz * num_sect;
    OCFIndex *stackStorage[num_sect];
    OCFIndex **stack_tmps = stackStorage;
    size_t tableSize = (size_t)num_sect * sizeof(OCFIndex *);
    bzero(stack_tmps, tableSize < 512 ? tableSize : 512);
    for (OCFIndex idx = 0; idx < num_sect; idx++) {
        stack_tmps[idx] = malloc_type_malloc((size_t)sz * sizeof(OCFIndex), 0x100004000313F17);
    }
    dispatch_apply((size_t)num_sect, NULL, ^(size_t sect) {
      OCFIndex sect_len = sect < (size_t)(num_sect - 1) ? sz : last_sect_len;
      __CFSimpleMergeSort(listp + (OCFIndex)sect * sz, sect_len, stack_tmps[sect], cmp);
    });
    if (num_sect < 1) {
        return;
    }
    OCFIndex even_phase_cnt = (num_sect / 2) * 2;
    OCFIndex odd_phase_cnt = ((num_sect - 1) / 2) * 2;
    for (uint64_t idx = 0; idx < ((uint64_t)num_sect + 1) >> 1; idx++) {
        dispatch_apply((size_t)even_phase_cnt, NULL, ^(size_t sect) {
          size_t right = sect & 1;
          OCFIndex *left_base = listp + (OCFIndex)sect * sz - (right != 0 ? sz : 0);
          OCFIndex *right_base = listp + (OCFIndex)sect * sz + (right != 0 ? 0 : sz);
          OCFIndex sect2_len = sect + 2 - right == (size_t)num_sect ? last_sect_len : sz;
          __CFSortIndexesNMerge(left_base, sz, right_base, sect2_len, stack_tmps[sect], right, cmp);
        });
        if ((num_sect & 1) != 0) {
            memmove(
                stack_tmps[num_sect - 1],
                listp + (num_sect - 1) * sz,
                (size_t)last_sect_len * sizeof(OCFIndex)
            );
        }
        dispatch_apply((size_t)odd_phase_cnt, NULL, ^(size_t sect) {
          size_t right = sect & 1;
          OCFIndex *left_base = stack_tmps[sect + (right != 0 ? 0 : 1)];
          OCFIndex *right_base = stack_tmps[sect + 2 - right];
          OCFIndex sect2_len = (sect & ~(size_t)1) + 3 == (size_t)num_sect ? last_sect_len : sz;
          __CFSortIndexesNMerge(
              left_base, sz, right_base, sect2_len, listp + (OCFIndex)sect * sz + sz, right, cmp
          );
        });
        memmove(listp, stack_tmps[0], (size_t)sz * sizeof(OCFIndex));
        if ((num_sect & 1) == 0) {
            memmove(
                listp + (num_sect - 1) * sz,
                stack_tmps[num_sect - 1],
                (size_t)last_sect_len * sizeof(OCFIndex)
            );
        }
    }
    for (OCFIndex idx = 0; idx < num_sect; idx++) {
        free(stack_tmps[idx]);
    }
}

// Native __CFSimpleMergeSort, 0x1c01faa0c. Non-external; CFSortIndexes and the concurrent sort's section
// block call it. Two values swap when the comparator answers above zero; three are ordered with at most three
// comparisons (the larger of the first two against the third, then the smaller against the third). Longer
// lists sort both halves (count / 2 and the rest, unsigned) recursively; above 16 values halves already in
// order (comparator at most zero across the boundary) are left as they are. The merge takes the right value
// only when the comparator answers above zero, and stops when the left half is used up (the merged prefix is
// copied back from tmp) or the right half is (the left remainder moves to the end first).
static void __CFSimpleMergeSort(
    OCFIndex listp[], OCFIndex cnt, OCFIndex tmp[], OCFComparisonResult (^cmp)(OCFIndex, OCFIndex)
) {
    if (cnt < 2) {
        return;
    }
    if (cnt == 3) {
        OCFIndex first = listp[0];
        OCFIndex second = listp[1];
        OCFIndex third = listp[2];
        OCFComparisonResult result = cmp(first, second);
        OCFIndex larger = result > 0 ? first : second;
        OCFIndex smaller = result > 0 ? second : first;
        if (cmp(larger, third) < 1) {
            listp[0] = smaller;
            listp[1] = larger;
            listp[2] = third;
            return;
        }
        result = cmp(smaller, third);
        OCFIndex middle = result > 0 ? smaller : third;
        smaller = result > 0 ? third : smaller;
        listp[0] = smaller;
        listp[1] = middle;
        listp[2] = larger;
        return;
    }
    if (cnt == 2) {
        OCFIndex first = listp[0];
        OCFIndex second = listp[1];
        if (cmp(first, second) >= 1) {
            listp[0] = second;
            listp[1] = first;
        }
        return;
    }
    OCFIndex half = (OCFIndex)((uint64_t)cnt >> 1);
    __CFSimpleMergeSort(listp, half, tmp, cmp);
    OCFIndex rest = cnt - half;
    __CFSimpleMergeSort(listp + half, rest, tmp, cmp);
    if (rest < 1) {
        return;
    }
    if ((uint64_t)cnt >= 17 && cmp(listp[half - 1], listp[half]) < 1) {
        return;
    }
    OCFIndex count = 0;
    uint64_t left = 0;
    OCFIndex right = half;
    while (cnt > right) {
        OCFIndex leftValue = listp[left];
        OCFIndex rightValue = listp[right];
        OCFComparisonResult result = cmp(leftValue, rightValue);
        OCFIndex value = result > 0 ? rightValue : leftValue;
        if (result > 0) {
            right++;
        }
        if (result < 1) {
            left++;
        }
        tmp[count] = value;
        count++;
        if ((uint64_t)half <= left) {
            do {
                listp[count - 1] = tmp[count - 1];
                count--;
            } while (count != 0);
            return;
        }
    }
    if (count < cnt) {
        OCFIndex source = half;
        do {
            listp[cnt - 1] = listp[source - 1];
            cnt--;
            source--;
        } while (count < cnt);
    }
    for (; count != 0; count--) {
        listp[count - 1] = tmp[count - 1];
    }
}

// Native CFQSortArray, 0x1c01fac1c. External. Nothing happens below 2 values or for an element size below 1.
// A count * elementSize product that overflows (signed) and a list + size that wraps (unsigned) are logged at
// level 3 and crash with their annotations. The index buffer holds count values on the stack up to 4,096,
// else malloc (type 0x100004000313F17); a failed malloc is CFQSortArray.cold.1 (0x1c030c3c0: "unable to qsort
// array - malloc failed" at level 3, then the crash annotation "qsort - malloc failed"). The indexes are
// sorted with CFSortIndexes (no options) and the comparator block ___CFQSortArray_block_invoke (0x1c01faeb4,
// non-external; it compares the elements at list + elementSize * index, passing the context). The values are
// gathered into a store of count * elementSize bytes, on the stack when count is at most 0x4000 / elementSize
// (unsigned), else malloc (type 0x8B25C98D); an 8-byte element is copied by a load and store, any other size
// by memmove. The store is moved back over the list and the malloc buffers are freed. CoreFoundation defect:
// kept. The malloc result of the store is not checked; a NULL store passes the wrap check (NULL + size does
// not wrap) and the copy writes through it. Not reached unless malloc fails.
void OCFQSortArray(
    void *list, OCFIndex count, OCFIndex elementSize, OCFComparatorFunction comparator, void *context
) {
    if (count < 2 || elementSize < 1) {
        return;
    }
    OCFIndex size;
    if (__builtin_mul_overflow(count, elementSize, &size)) {
        OCFLog(
            3,
            OF_STR("Unable to qsort array - count: %ld elementSize: %ld product overflows"),
            count,
            elementSize
        );
        crashWithMessage("qsort - count/elementSize overflow");
    }
    uintptr_t end;
    if (__builtin_add_overflow((uintptr_t)list, (uintptr_t)size, &end)) {
        OCFLog(
            3,
            OF_STR("Unable to qsort array - list: %lu count: %ld elementSize: %ld - array access overflows"),
            list,
            count,
            elementSize
        );
        crashWithMessage("qsort - array access overflow");
    }
    OCFIndex localIndexes[(uint64_t)count <= 4096 ? count : 1];
    OCFIndex *indexes = localIndexes;
    if ((uint64_t)count > 4096) {
        indexes = malloc_type_malloc((size_t)count * sizeof(OCFIndex), 0x100004000313F17);
        if (indexes == NULL) {
            OCFLog(3, OF_STR("unable to qsort array - malloc failed"));
            crashWithMessage("qsort - malloc failed");
        }
    }
    OCFSortIndexes(indexes, count, 0, ^OCFComparisonResult(OCFIndex a, OCFIndex b) {
      return comparator((char *)list + elementSize * a, (char *)list + elementSize * b, context);
    });
    uint64_t limit = 0x4000 / (uint64_t)elementSize;
    uint8_t localStore[(uint64_t)count <= limit ? size : 1];
    uint8_t *store = localStore;
    if ((uint64_t)count > limit) {
        store = malloc_type_malloc((size_t)size, 0x8B25C98D);
    }
    if (__builtin_add_overflow((uintptr_t)store, (uintptr_t)size, &end)) {
        OCFLog(
            3,
            OF_STR("Unable to qsort array - list: %lu count: %ld elementSize: %ld array - store overflows"),
            list,
            count,
            elementSize
        );
        crashWithMessage("qsort - array storage overflow");
    }
    uint8_t *cursor = store;
    for (OCFIndex idx = 0; idx != count; idx++) {
        if (elementSize == 8) {
            ((uint64_t *)store)[idx] = ((uint64_t *)list)[indexes[idx]];
        } else {
            memmove(cursor, (uint8_t *)list + indexes[idx] * elementSize, (size_t)elementSize);
        }
        cursor += elementSize;
    }
    memmove(list, store, (size_t)size);
    if (localStore != store) {
        free(store);
    }
    if (localIndexes != indexes) {
        free(indexes);
    }
}

// Native __CFSortIndexesNMerge, 0x1c01fb20c. Non-external. Merges two sorted runs into tmp: from the front
// for the left half (cnt1 values), from the back for the right half (cnt2 values); ties keep the left value
// first. Above 16 values in all, runs already in order are copied whole. Native behavior kept: when one run
// is exhausted while merging from the back, the native code reads the value below the other run's cursor (LDR
// X8, [X27,#-8]! and LDR X8, [X26,#-8]!); with cnt1 >= cnt2, as every caller passes, neither branch is
// reached.
static void __CFSortIndexesNMerge(
    OCFIndex listp1[],
    OCFIndex cnt1,
    OCFIndex listp2[],
    OCFIndex cnt2,
    OCFIndex tmp[],
    size_t right,
    OCFComparisonResult (^cmp)(OCFIndex, OCFIndex)
) {
    if (cnt1 + cnt2 >= 17) {
        if (cmp(listp1[cnt1 - 1], listp2[0]) <= 0) {
            memmove(tmp, right != 0 ? listp2 : listp1, (size_t)(right != 0 ? cnt2 : cnt1) * sizeof(OCFIndex));
            return;
        }
    }
    if (right != 0) {
        if (cnt2 < 1) {
            return;
        }
        OCFIndex *dest = tmp + cnt2;
        OCFIndex *right_cur = listp2 + cnt2 - 1;
        OCFIndex *left_cur = listp1 + cnt1 - 1;
        do {
            dest--;
            if ((uintptr_t)right_cur < (uintptr_t)listp2) {
                *dest = *--left_cur;
            } else if ((uintptr_t)left_cur < (uintptr_t)listp1) {
                *dest = *--right_cur;
            } else {
                OCFIndex left_value = *left_cur;
                OCFIndex right_value = *right_cur;
                if (cmp(left_value, right_value) <= 0) {
                    *dest = right_value;
                    right_cur--;
                } else {
                    *dest = left_value;
                    left_cur--;
                }
            }
        } while ((uintptr_t)tmp < (uintptr_t)dest);
    } else {
        if (cnt1 < 1) {
            return;
        }
        OCFIndex *left_lim = listp1 + cnt1;
        OCFIndex *right_lim = listp2 + cnt2;
        OCFIndex *dest_lim = tmp + cnt1;
        do {
            if ((uintptr_t)right_lim <= (uintptr_t)listp2) {
                *tmp = *listp1++;
            } else if ((uintptr_t)left_lim <= (uintptr_t)listp1) {
                *tmp = *listp2++;
            } else {
                OCFIndex left_value = *listp1;
                OCFIndex right_value = *listp2;
                if (cmp(left_value, right_value) <= 0) {
                    *tmp = left_value;
                    listp1++;
                } else {
                    *tmp = right_value;
                    listp2++;
                }
            }
            tmp++;
        } while ((uintptr_t)tmp < (uintptr_t)dest_lim);
    }
}

//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSSetEnumerate.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFBasicHash.h"
#include "CoreFoundation/Objective-C/OCFNSSet.h"

#import <objc/NSObject.h>
#include <dispatch/dispatch.h>
#include <pthread.h>

// Inferred organization: __NSSET_IS_CALLING_OUT_TO_A_BLOCK__ (0x1c01960d4), __NSSetEnumerate (0x1c01960ec)
// and its blocks ____NSSetEnumerate_block_invoke (0x1c01963d8) and _2 (0x1c0196514) lie together in the iOS
// Simulator __text section, after CFDictionaryRemoveAllValues and before __cow_create; their native unit is
// not in the evidence, and they belong to no unit restored on this branch, so they have their own
// translation unit, named for the cluster. Objective-C (message sends, blocks, fast enumeration), compiled
// with manual reference counting. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full,
// iphone-simulator-corpus-delta8.json). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The __NSCFSet class symbol, referenced natively by address (ADRP/ADD of _OBJC_CLASS_$___NSCFSet).
extern char enumerateCFSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSCFSet));

// Native __NSSET_IS_CALLING_OUT_TO_A_BLOCK__, 0x1c01960d4. Hidden (was a private external). The block's
// invoke function receives the object and the stop flag unchanged; the call is not a tail call (the frame
// stays in backtraces) and the function is not inlined into its callers.
__attribute__((noinline, disable_tail_calls)) void
__NSSET_IS_CALLING_OUT_TO_A_BLOCK__(void (^block)(id object, BOOL *stop), id object, BOOL *stop) {
    block(object, stop);
}

// Native __NSSetEnumerate, 0x1c01960ec, with ____NSSetEnumerate_block_invoke (0x1c01963d8) and
// ____NSSetEnumerate_block_invoke_2 (0x1c0196514). Hidden (was a private external). An empty set does
// nothing. With bit 0 of the options (NSEnumerationConcurrent) and at least two active processors, the
// objects are taken from -objectEnumerator in strides under an error-checking mutex (the stride is 64, 48,
// 32, 16, 4 or 1 for counts of at least 64, 48, 32, 16 and 4 times the processor count) and each stride is
// called out on a dispatch_apply iteration (default queue) inside its own autorelease pool; the shared stop
// flag is a __block BOOL read and written with sequentially consistent atomics, and a stride stops when its
// local flag reads 1. Otherwise, inside an autorelease pool, an __NSCFSet (isKindOfClass:) is walked with
// CFBasicHashApply (the bucket's key at offset 8; a set stop flag ends the walk) and any other set with fast
// enumeration until the stop flag is set. The objects are not retained. The mutex is never destroyed.
void __NSSetEnumerate(id set, NSUInteger options, void (^block)(id object, BOOL *stop)) {
    if ([set count] == 0) {
        return;
    }
    NSUInteger processorCount;
    if ((options & 1) != 0) {
        processorCount = (NSUInteger)(NSInteger)__CFActiveProcessorCount();
        if ((NSInteger)processorCount < 2) {
            options = 0;
        }
    } else {
        processorCount = 1;
    }
    if ((options & 1) != 0) {
        NSUInteger count = [set count];
        NSUInteger stride;
        if (count >= processorCount << 6) {
            stride = 64;
        } else if (processorCount * 48 <= count) {
            stride = 48;
        } else if (count >= processorCount << 5) {
            stride = 32;
        } else if (count >= processorCount << 4) {
            stride = 16;
        } else if (count >= processorCount << 2) {
            stride = 4;
        } else {
            stride = 1;
        }
        size_t iterations = (count + stride - 1) / stride;
        void *pool = _OCFAutoreleasePoolPush();
        id enumerator = [set objectEnumerator];
        pthread_mutex_t lock = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER;
        pthread_mutex_t *lockPointer = &lock;
        __block BOOL stop = NO;
        dispatch_apply(iterations, NULL, ^(__attribute__((unused)) size_t iteration) {
          if (__atomic_load_n(&stop, __ATOMIC_SEQ_CST)) {
              return;
          }
          id objects[stride];
          void *iterationPool = _OCFAutoreleasePoolPush();
          pthread_mutex_lock(lockPointer);
          for (NSUInteger index = 0; index < stride; index++) {
              objects[index] = [enumerator nextObject];
              if (objects[index] == nil) {
                  break;
              }
          }
          pthread_mutex_unlock(lockPointer);
          BOOL localStop = NO;
          for (NSUInteger index = 0; index < stride && objects[index] != nil; index++) {
              __NSSET_IS_CALLING_OUT_TO_A_BLOCK__(block, objects[index], &localStop);
              // The native test compares the flag's byte with 1 (CMP W8, #1).
              if (*(unsigned char *)&localStop == 1) {
                  __atomic_store_n(&stop, YES, __ATOMIC_SEQ_CST);
                  break;
              }
          }
          _OCFAutoreleasePoolPop(iterationPool);
        });
        _OCFAutoreleasePoolPop(pool);
    } else {
        void *pool = _OCFAutoreleasePoolPush();
        if ([set isKindOfClass:[(Class)enumerateCFSetClass class]]) {
            __block BOOL stop = NO;
            CFBasicHashApply((BasicHashRef)set, ^Boolean(BasicHashBucket bucket) {
              __NSSET_IS_CALLING_OUT_TO_A_BLOCK__(block, (id)bucket.key, &stop);
              return !stop;
            });
        } else {
            BOOL stop = NO;
            for (id object in set) {
                __NSSET_IS_CALLING_OUT_TO_A_BLOCK__(block, object, &stop);
                if (stop) {
                    break;
                }
            }
        }
        _OCFAutoreleasePoolPop(pool);
    }
}

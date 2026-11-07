//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSDictionaryEnumerate.h"
#include "CoreFoundation/Collections/OCFBasicHash.h"

#include <dispatch/dispatch.h>
#include <pthread.h>
#include <stdlib.h>

// Inferred organization: __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__ (0x1c01cbed4),
// __NSDictionaryParameterCheckIterate (0x1c01cbeec), __NSDictionaryEnumerate (0x1c01cbf5c) and its blocks
// ____NSDictionaryEnumerate_block_invoke (0x1c01cc2b0) and ____NSDictionaryEnumerate_block_invoke_2
// (0x1c01cc408) lie together in the iOS Simulator __text section, after the preferences soft-link helpers'
// OUTLINED_FUNCTION pieces and before -[__NSDictionaryObjectEnumerator nextObject]; they are kept in their
// own translation unit (the enumerator class is not restored on this branch). Objective-C (message sends,
// Objective-C exceptions), manual reference counting. Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full).
// iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not
// written yet).

// Native __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__, 0x1c01cbed4. Hidden (was a private external). The
// block's invoke function (offset 16) with the key, the object and the stop flag; a frame of its own so that
// the call out is visible in backtraces.
void __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__(
    void (^block)(id key, id object, BOOL *stop), id key, id object, BOOL *stop
) {
    block(key, object, stop);
}

// Native __NSDictionaryParameterCheckIterate, 0x1c01cbeec. Hidden (was a private external). A nil block
// raises NSInvalidArgumentException "%@: block is nil" (the __CFExceptionProem of the object and selector,
// kCFAllocatorSystemDefault, no os_log pack). The options, which every caller passes in X3, are not read.
void __NSDictionaryParameterCheckIterate(
    id object, SEL selector, id block, __attribute__((unused)) NSUInteger options
) {
    if (block != nil) {
        return;
    }
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFStringRef reason = OCFStringCreateWithFormat(
        allocator, NULL, OF_STR("%@: block is nil"), __CFExceptionProem(object, selector)
    );
    id exception = [(id)dictionaryExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                            reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                                                          userInfo:nil];
    objc_exception_throw(exception);
}

// Native __NSDictionaryEnumerate, 0x1c01cbf5c, with its blocks ____NSDictionaryEnumerate_block_invoke
// (0x1c01cc2b0) and ____NSDictionaryEnumerate_block_invoke_2 (0x1c01cc408), non-external. Hidden (was a
// private external). An empty dictionary (-count) returns. Option bit 0 (concurrent) is dropped with fewer
// than two active processors. Concurrently: with n processors the chunk is 64 when count >= 64n, else 48 when
// 48n <= count, else 32 (count >= 32n), 16 (count >= 16n), 4 (count >= 4n) or 1; inside an autorelease pool,
// dispatch_apply(ceil(count / chunk), NULL, ...) runs a block (the -keyEnumerator, the dictionary, the block,
// a __block stop flag, the chunk and a PTHREAD_MUTEX_INITIALIZER mutex on the stack) that, unless the stop
// flag is set (acquire load), takes up to chunk keys from the enumerator under the mutex into a stack array
// (stopping at nil) and calls the block for each with -objectForKey:, inside its own autorelease pool,
// setting the shared flag (release store) when the block sets its stop flag; the mutex is not destroyed.
// Serially, inside an autorelease pool: an instance of __NSCFDictionary (objc_lookUpClass, -isKindOfClass:)
// is enumerated with CFBasicHashApply (bucket key and value; the apply block answers NO once the stop flag is
// set); any other dictionary's keys and objects are fetched with -getObjects:andKeys:count: (keys first) into
// a stack buffer of 2 * count words (at least one) up to 0x100 words, otherwise _CFCreateArrayStorage(2 *
// count, false, ...), freed afterwards and on unwinding (an @finally), and the block is called in order until
// it sets the stop flag. A count with any of bits 59-62 set raises NSGenericException "*** attempt to create
// a temporary id buffer which is too large or with a negative count (%lu) -- possibly data is corrupt" (2 *
// count; formatted with the default allocator, released after the exception is made) before the buffer.
void __NSDictionaryEnumerate(
    id dictionary, NSUInteger options, void (^block)(id key, id object, BOOL *stop)
) {
    NSUInteger count = [dictionary count];
    if (count == 0) {
        return;
    }
    if ((options & 1) != 0) {
        int32_t processors = __CFActiveProcessorCount();
        if (processors < 2) {
            options = 0;
        }
        if ((options & 1) != 0) {
            NSUInteger processorCount = (NSUInteger)(intptr_t)processors;
            NSUInteger chunk = count >= processorCount * 4 ? 4 : 1;
            if (count >= processorCount * 16) {
                chunk = 16;
            }
            if (count >= processorCount * 32) {
                chunk = 32;
            }
            if (processorCount * 48 <= count) {
                chunk = 48;
            }
            if (count >= processorCount * 64) {
                chunk = 64;
            }
            size_t iterations = (count + chunk - 1) / chunk;
            void *pool = _OCFAutoreleasePoolPush();
            id enumerator = [dictionary keyEnumerator];
            pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
            pthread_mutex_t *lockPointer = &lock;
            __block BOOL stop = NO;
            dispatch_apply(iterations, DISPATCH_APPLY_AUTO, ^(size_t iteration) {
              (void)iteration;
              if (__atomic_load_n(&stop, __ATOMIC_ACQUIRE)) {
                  return;
              }
              id keys[chunk];
              void *chunkPool = _OCFAutoreleasePoolPush();
              pthread_mutex_lock(lockPointer);
              for (NSUInteger index = 0; index < chunk; index += 1) {
                  keys[index] = [enumerator nextObject];
                  if (keys[index] == nil) {
                      break;
                  }
              }
              pthread_mutex_unlock(lockPointer);
              for (NSUInteger index = 0; index < chunk; index += 1) {
                  id key = keys[index];
                  if (key == nil) {
                      break;
                  }
                  BOOL innerStop = NO;
                  __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__(
                      block, key, [dictionary objectForKey:key], &innerStop
                  );
                  if (innerStop == YES) {
                      __atomic_store_n(&stop, YES, __ATOMIC_RELEASE);
                      break;
                  }
              }
              _OCFAutoreleasePoolPop(chunkPool);
            });
            _OCFAutoreleasePoolPop(pool);
            return;
        }
    }
    void *pool = _OCFAutoreleasePoolPush();
    if ([dictionary isKindOfClass:objc_lookUpClass(OF_NAME_STRING(__ONSCFDictionary))]) {
        CFBasicHashApply((BasicHashRef)dictionary, ^Boolean(BasicHashBucket bucket) {
          BOOL stop = NO;
          __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__(block, (id)bucket.key, (id)bucket.value, &stop);
          return !stop;
        });
    } else {
        NSUInteger words = count * 2;
        if (((OF_WORD_SHIFT(count, 59)) & 0xF) != 0) {
            OCFStringRef reason = OCFStringCreateWithFormat(
                NULL,
                NULL,
                OF_STR(
                    "*** attempt to create a temporary id buffer which is too large or with a negative count "
                    "(%lu) -- possibly data is corrupt"
                ),
                words
            );
            id exception = [(id)dictionaryExceptionClass exceptionWithName:(id)ONSGenericException
                                                                    reason:(id)reason
                                                                  userInfo:nil];
            OCFRelease(reason);
            objc_exception_throw(exception);
        }
        NSUInteger bufferWords = words > 1 ? words : 1;
        id stackBuffer[words >= 0x101 ? 1 : bufferWords];
        id *buffer = stackBuffer;
        size_t storageCount = 0;
        void *storage = NULL;
        if (words >= 0x101) {
            buffer = _OCFCreateArrayStorage(bufferWords, false, &storageCount);
            storage = buffer;
        }
        @try {
            [dictionary getObjects:buffer + count andKeys:buffer count:count];
            BOOL stop = NO;
            NSUInteger remaining = count - 1;
            for (;;) {
                __NSDICTIONARY_IS_CALLING_OUT_TO_A_BLOCK__(block, buffer[0], buffer[count], &stop);
                if (stop) {
                    break;
                }
                buffer += 1;
                if (remaining == 0) {
                    break;
                }
                remaining -= 1;
            }
        } @finally {
            free(storage);
        }
    }
    _OCFAutoreleasePoolPop(pool);
}

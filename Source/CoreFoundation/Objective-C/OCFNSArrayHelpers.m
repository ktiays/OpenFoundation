//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSArrayHelpers.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Objective-C/OCFNSArray.h"
#include "CoreFoundation/String/OCFString.h"

#import <objc/NSObject.h>

#include <alloca.h>
#include <dispatch/dispatch.h>
#include <mach-o/loader.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stddef.h>
#include <stdint.h>

// Inferred organization: __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__ (0x1c01e8134), __NSArrayParameterCheckIterate
// (0x1c01e814c), PROEM_SEL (0x1c01e83d4), the compiler's os_log helpers
// __os_log_helper_1_2_4_8_32_8_32_8_0_8_0 (0x1c01e83e8) and __os_log_helper_1_2_3_8_32_8_32_8_0
// (0x1c01e841c), __NSArrayChunkIterate (0x1c01e8448),
// __NSARRAY_IS_CALLING_OUT_TO_A_BOOL_BLOCK__ (0x1c01e8704), __NSArrayEnumerate (0x1c01e871c) with its block
// (0x1c01e8ae0), __NSArrayGetIndexPassingTest (0x1c01e8b64) with its block (0x1c01e8f2c) and
// __NSArrayGetIndexesPassingTest (0x1c01e8fb0) with its block (0x1c01e93c8) are contiguous in the iOS
// Simulator
// __text section, after the runtime's ThreadSanitizer tag functions and before the methods of another class;
// they form one unit (PROEM_SEL is a static copy of its own), restored here and named for the cluster.
// Objective-C (blocks, fast enumeration, objc_enumerationMutation), manual reference counting. The os_log
// helpers are the argument packing __builtin_os_log_format generates; they are restored as that builtin at
// their call sites. Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json. Unless a definition
// says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified
// (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).
//
// The block literals keep the native captures; their descriptors (unk_2227A4330, unk_2227A4358) lie outside
// the extracted image and are unread, so the capture layout clang computes stands for them (the native
// layouts read from the stores: the captured objects first, then the __block variables, then the scalars).

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// The NSException class symbol (_OBJC_CLASS_$_NSException), sent to directly.
extern char exceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));

// The messages this unit sends to the index sets it is given and to objects through id, declared for the
// sends only (no protocol object is emitted). -__getContainmentVector:inRange: is tested as a 64-bit word
// (CBZ X0), so its result is spelled as one (confidence: medium for the native result type).
@protocol ArrayHelperMessages

- (NSUInteger)firstIndex;
- (NSUInteger)lastIndex;
- (BOOL)containsIndex:(NSUInteger)index;
- (void)addIndex:(NSUInteger)index;
- (uintptr_t)__getContainmentVector:(uint8_t *)vector inRange:(OCFRange)range;

@end

// The enumeration and test block types.
typedef void (^EnumerationBlock)(id object, NSUInteger index, BOOL *stop);
typedef BOOL (^TestBlock)(id object, NSUInteger index, BOOL *stop);

// Native PROEM_SEL, 0x1c01e83d4. Non-external (this unit's static copy; others at 0x1c01cd94c and
// 0x1c01f73f8). The selector's name (sel_getName as a tail call), or "(null selector)" for a NULL selector.
static const char *PROEM_SEL(SEL selector) {
    if (selector == NULL) {
        return "(null selector)";
    }
    return sel_getName(selector);
}

// Native __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__, 0x1c01e8134. Hidden (was a private external). A framed call
// of the block's invoke pointer (offset 16) with the arguments unchanged.
__attribute__((noinline, disable_tail_calls)) void __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__(
    void (^block)(id object, NSUInteger index, BOOL *stop), id object, NSUInteger index, BOOL *stop
) {
    block(object, index, stop);
}

// Native __NSArrayParameterCheckIterate, 0x1c01e814c. Hidden (was a private external). Only with bit 60 of
// options: a nil index set raises NSInvalidArgumentException "%@: index set is nil" (the __CFExceptionProem
// of the array and the selector; reason with kCFAllocatorSystemDefault, autoreleased through
// _CFAutoreleasePoolAddObject, +exceptionWithName:reason:userInfo:). Otherwise the index set's -lastIndex is
// read, then the array's -count; a last index with bit 63 clear that is NSNotFound or below the count passes.
// Any other raises NSRangeException through an os_log pack: "*** -[%s %s]: index %lu beyond bounds for empty
// array" (buffer 0x20) for an empty array, else "*** -[%s %s]: index %lu beyond bounds [0 .. %lu]" (buffer
// 0x2A) with the count minus one, the class name being NSArray and the selector name PROEM_SEL's, read once
// for the pack and again for the reason.
void __NSArrayParameterCheckIterate(id array, SEL selector, id block, NSUInteger options, id indexes) {
    (void)block;
    if (!(options & 0x1000000000000000ULL)) {
        return;
    }
    if (indexes == nil) {
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        OCFStringRef reason = OCFStringCreateWithFormat(
            allocator, NULL, OF_STR("%@: index set is nil"), __CFExceptionProem(array, selector)
        );
        id exception = ((id (*)(id, SEL, id, id, id))objc_msgSend)(
            (id)exceptionClass,
            sel_getUid("exceptionWithName:reason:userInfo:"),
            (id)ONSInvalidArgumentException,
            _OCFAutoreleasePoolAddObject(NULL, reason),
            nil
        );
        objc_exception_throw(exception);
    }
    NSUInteger lastIndex = [indexes lastIndex];
    NSUInteger count = [array count];
    if ((intptr_t)lastIndex >= 0 && (lastIndex == kNotFound || count > lastIndex)) {
        return;
    }
    size_t size;
    void *pack;
    OCFStringRef reason;
    if (count == 0) {
        size = _os_log_pack_size(0x20);
        pack = alloca((size + 15) & ~(size_t)15);
        uint8_t *buffer = _os_log_pack_fill(
            pack, size, 0, &__dso_handle, "*** -[%s %s]: index %lu beyond bounds for empty array"
        );
        const char *selectorName = PROEM_SEL(selector);
        __builtin_os_log_format(
            buffer,
            "*** -[%s %s]: index %lu beyond bounds for empty array",
            OF_NAME_STRING(ONSArray),
            selectorName,
            lastIndex
        );
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        const char *reasonSelectorName = PROEM_SEL(selector);
        reason = OCFStringCreateWithFormat(
            allocator,
            NULL,
            OF_STR("*** -[%s %s]: index %lu beyond bounds for empty array"),
            OF_NAME_STRING(ONSArray),
            reasonSelectorName,
            lastIndex
        );
    } else {
        size = _os_log_pack_size(0x2A);
        pack = alloca((size + 15) & ~(size_t)15);
        uint8_t *buffer = _os_log_pack_fill(
            pack, size, 0, &__dso_handle, "*** -[%s %s]: index %lu beyond bounds [0 .. %lu]"
        );
        const char *selectorName = PROEM_SEL(selector);
        NSUInteger lastValid = count - 1;
        __builtin_os_log_format(
            buffer,
            "*** -[%s %s]: index %lu beyond bounds [0 .. %lu]",
            OF_NAME_STRING(ONSArray),
            selectorName,
            lastIndex,
            lastValid
        );
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        const char *reasonSelectorName = PROEM_SEL(selector);
        reason = OCFStringCreateWithFormat(
            allocator,
            NULL,
            OF_STR("*** -[%s %s]: index %lu beyond bounds [0 .. %lu]"),
            OF_NAME_STRING(ONSArray),
            reasonSelectorName,
            lastIndex,
            lastValid
        );
    }
    id exception = ((id (*)(id, SEL, id, id, id, void *, size_t))objc_msgSend)(
        (id)exceptionClass,
        sel_getUid("exceptionWithName:reason:userInfo:osLogPack:size:"),
        (id)ONSRangeException,
        _OCFAutoreleasePoolAddObject(NULL, reason),
        nil,
        pack,
        size
    );
    objc_exception_throw(exception);
}

// Native __NSArrayChunkIterate, 0x1c01e8448. Hidden (was a private external). The result word is result, or a
// local NSNotFound when result is NULL. With bit 61, a result already found (read with acquire ordering) that
// lies before start (forward) or after end (reverse) answers NO at once. Inside an autorelease pool
// (_CFAutoreleasePoolPush/Pop), end - start + 1 objects are copied to a stack buffer with -getObjects:range:,
// after the index set's -__getContainmentVector:inRange: has filled a stack byte vector (bit 60; a zero
// result ends the walk). The walk visits buffer slot i (forward) or end - start - i (reverse); with bit 60 a
// slot whose vector byte is not 1 is skipped. The index passed is start + i (forward) or end - i (reverse).
// Without bits 61-62 the block is called through __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__; with them, through
// __NSARRAY_IS_CALLING_OUT_TO_A_BOOL_BLOCK__, and a passing index either (bit 61) replaces the result word by
// compare-and-swap (relaxed) while it improves on the current value (reread with acquire ordering each time;
// NSNotFound always improves), ending the walk, or (bit 62) is added to resultIndexes, under the mutex when
// one is given. A set stop flag ends the walk; with bit 61, so does a result word that moved before start
// (forward) or after end (reverse). Answers the stop flag.
BOOL __NSArrayChunkIterate(
    id array,
    NSUInteger options,
    NSUInteger start,
    NSUInteger end,
    id indexes,
    id block,
    NSUInteger *result,
    id resultIndexes,
    pthread_mutex_t *mutex
) {
    BOOL stop = NO;
    NSUInteger localResult = kNotFound;
    NSUInteger *resultWord = result == NULL ? &localResult : result;
    if (options & 0x2000000000000000ULL) {
        NSUInteger found = __atomic_load_n(resultWord, __ATOMIC_ACQUIRE);
        if (found != kNotFound) {
            BOOL inside = (options & 2) == 0 ? found >= start : end >= found;
            if (inside != YES) {
                return NO;
            }
        }
    }
    void *pool = _OCFAutoreleasePoolPush();
    NSUInteger last = end - start;
    NSUInteger count = last + 1;
    id *objects = (id *)alloca(count * sizeof(id));
    uint8_t *vector = (uint8_t *)alloca(count);
    if (options & 0x1000000000000000ULL) {
        if ([indexes __getContainmentVector:vector
                                    inRange:(OCFRange){(OCFIndex)start, (OCFIndex)count}] == 0) {
            _OCFAutoreleasePoolPop(pool);
            return stop & 1;
        }
    }
    [array getObjects:objects range:(OCFRange){(OCFIndex)start, (OCFIndex)count}];
    if (count != 0) {
        NSUInteger position = 0;
        NSUInteger reversePosition = last;
        NSUInteger reverseIndex = end;
        while (1) {
            NSUInteger slot = (options & 2) == 0 ? position : reversePosition;
            if (!(options & 0x1000000000000000ULL) || vector[slot] == 1) {
                id object = objects[slot];
                NSUInteger index = (options & 2) == 0 ? start + position : reverseIndex;
                if (options & 0x6000000000000000ULL) {
                    if (__NSARRAY_IS_CALLING_OUT_TO_A_BOOL_BLOCK__((TestBlock)block, object, index, &stop) &
                        1) {
                        if (options & 0x2000000000000000ULL) {
                            while (1) {
                                NSUInteger found = __atomic_load_n(resultWord, __ATOMIC_ACQUIRE);
                                if (found != kNotFound) {
                                    BOOL better = (options & 2) == 0 ? index < found : found < index;
                                    if (better != YES) {
                                        break;
                                    }
                                }
                                NSUInteger expected = found;
                                if (__atomic_compare_exchange_n(
                                        resultWord,
                                        &expected,
                                        index,
                                        false,
                                        __ATOMIC_RELAXED,
                                        __ATOMIC_RELAXED
                                    )) {
                                    break;
                                }
                            }
                            break;
                        }
                        if (options & 0x4000000000000000ULL) {
                            if (mutex != NULL) {
                                pthread_mutex_lock(mutex);
                                [resultIndexes addIndex:index];
                                pthread_mutex_unlock(mutex);
                            } else {
                                [resultIndexes addIndex:index];
                            }
                        }
                    }
                } else {
                    __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__((EnumerationBlock)block, object, index, &stop);
                }
                if (stop & 1) {
                    break;
                }
                if (options & 0x2000000000000000ULL) {
                    NSUInteger found = __atomic_load_n(resultWord, __ATOMIC_ACQUIRE);
                    if (found != kNotFound) {
                        BOOL inside = (options & 2) == 0 ? found >= start : end >= found;
                        if (inside != YES) {
                            break;
                        }
                    }
                }
            }
            if (position + 1 >= count) {
                break;
            }
            position++;
            reversePosition--;
            reverseIndex--;
        }
    }
    _OCFAutoreleasePoolPop(pool);
    return stop & 1;
}

// Native __NSARRAY_IS_CALLING_OUT_TO_A_BOOL_BLOCK__, 0x1c01e8704. Hidden (was a private external). A framed
// call of the block's invoke pointer with the arguments unchanged; answers its result.
__attribute__((noinline, disable_tail_calls)) BOOL __NSARRAY_IS_CALLING_OUT_TO_A_BOOL_BLOCK__(
    BOOL (^block)(id object, NSUInteger index, BOOL *stop), id object, NSUInteger index, BOOL *stop
) {
    return block(object, index, stop);
}

// The chunk size of the concurrent and reverse walks (inlined natively in the three walkers): the span's
// length divided twice by the processor count (unsigned), then 16 below 16, 0x1000 above 0x1000, and
// otherwise the low 32 bits plus 15 masked with 0x3FF0. chunkSize is a local descriptive name.
static inline NSUInteger chunkSize(NSUInteger span, NSUInteger processors) {
    NSUInteger perProcessor = (span + 1) / processors / processors;
    if (perProcessor < 0x10) {
        return 0x10;
    }
    if (perProcessor > 0x1000) {
        return 0x1000;
    }
    return ((uint32_t)perProcessor + 15) & 0x3FF0;
}

// Native __NSArrayEnumerate, 0x1c01e871c, with its block (___NSArrayEnumerate_block_invoke, 0x1c01e8ae0).
// Hidden (was a private external). An empty array (-count) does nothing. The walk covers the index set's
// -firstIndex..-lastIndex (bit 60) or 0..count-1 (-count again); an NSNotFound bound does nothing.
// Concurrency (bit 0) asks __CFActiveProcessorCount and is dropped below two processors. Neither concurrent
// nor reverse: fast enumeration (objc_enumerationMutation on a change) inside an autorelease pool, calling
// the block for every object whose running index passes the index set (first <= index <= last and
// -containsIndex:, bit 60); the stop flag ends the walk, and the pool is drained and pushed again after every
// 0x400 calls. Otherwise the span is cut in chunks and a __block result word (NSNotFound) is shared:
// concurrently, dispatch_apply (NULL queue) runs the chunks in a stack block that skips its chunk when the
// __block stop flag is set (acquire) and stores __NSArrayChunkIterate's answer into it (release); serially,
// while chunk + span does not wrap, the chunks run in order (from the end when reverse) until the last one or
// a stop.
void __NSArrayEnumerate(id array, id block, NSUInteger options, id indexes) {
    if ([array count] == 0) {
        return;
    }
    NSUInteger first;
    NSUInteger last;
    if (options & 0x1000000000000000ULL) {
        first = [indexes firstIndex];
        last = [indexes lastIndex];
    } else {
        first = 0;
        last = [array count] - 1;
    }
    if (first == kNotFound || last == kNotFound) {
        return;
    }
    NSUInteger processors;
    if (options & 1) {
        OCFIndex active = __CFActiveProcessorCount();
        if (active < 2) {
            options &= ~(NSUInteger)1;
        }
        processors = (NSUInteger)active;
    } else {
        processors = 1;
    }
    if ((options & 3) == 0) {
        void *pool = _OCFAutoreleasePoolPush();
        BOOL stop = NO;
        NSUInteger calls = 0;
        NSUInteger index = 0;
        for (id object in array) {
            if (!(options & 0x1000000000000000ULL) ||
                (first <= index && index <= last && [indexes containsIndex:index])) {
                __NSARRAY_IS_CALLING_OUT_TO_A_BLOCK__((EnumerationBlock)block, object, index, &stop);
                if (stop & 1) {
                    break;
                }
                if (calls < 0x3FF) {
                    calls++;
                } else {
                    _OCFAutoreleasePoolPop(pool);
                    pool = _OCFAutoreleasePoolPush();
                    calls = 0;
                }
            }
            index++;
        }
        _OCFAutoreleasePoolPop(pool);
        return;
    }
    NSUInteger span = last - first;
    NSUInteger chunk = chunkSize(span, processors);
    NSUInteger total = chunk + span;
    NSUInteger iterations = total / chunk;
    __block NSUInteger result = kNotFound;
    if (options & 1) {
        __block BOOL stop = NO;
        dispatch_apply(iterations, NULL, ^(size_t iteration) {
          if (__atomic_load_n(&stop, __ATOMIC_ACQUIRE) & 1) {
              return;
          }
          NSUInteger chunkStart = first + chunk * iteration;
          NSUInteger chunkEnd = chunk + chunkStart - 1;
          if (chunkEnd >= last) {
              chunkEnd = last;
          }
          BOOL stopped =
              __NSArrayChunkIterate(array, options, chunkStart, chunkEnd, indexes, block, &result, nil, NULL);
          __atomic_store_n(&stop, stopped, __ATOMIC_RELEASE);
        });
    } else if (chunk <= total) {
        NSUInteger iteration = 0;
        NSUInteger number = 1;
        NSUInteger forwardStart = first;
        NSUInteger reverseEnd = last;
        while (1) {
            NSUInteger chunkStart;
            NSUInteger chunkEnd;
            if (!(options & 2)) {
                chunkEnd = chunk - 1 < reverseEnd - first ? chunk - 1 + (iteration * chunk + first) : last;
                chunkStart = forwardStart;
            } else {
                chunkStart = chunk - 1 < reverseEnd - first ? (1 - chunk) + reverseEnd : first;
                chunkEnd = reverseEnd;
            }
            BOOL stopped = __NSArrayChunkIterate(
                array, options, chunkStart, chunkEnd, indexes, block, &result, nil, NULL
            );
            iteration++;
            BOOL done = number >= iterations;
            reverseEnd -= chunk;
            forwardStart += chunk;
            number++;
            if ((done | stopped) & 1) {
                break;
            }
        }
    }
}

// Native __NSArrayGetIndexPassingTest, 0x1c01e8b64, with its block
// (___NSArrayGetIndexPassingTest_block_invoke, 0x1c01e8f2c). Hidden (was a private external). The walk of
// __NSArrayEnumerate with the test block: an empty array or an NSNotFound bound answers NSNotFound. Serially:
// the first index whose test passes is answered (the pool popped); a set stop flag answers NSNotFound.
// Reverse: a local result word (NSNotFound) is passed to the chunks, which run until the last one or a stop,
// and is answered (read with acquire ordering). Concurrent: __block result (NSNotFound) and stop words are
// shared with the dispatch_apply block as in
// __NSArrayEnumerate; the result is read with acquire ordering after the apply.
NSUInteger __NSArrayGetIndexPassingTest(id array, id block, NSUInteger options, id indexes) {
    if ([array count] == 0) {
        return kNotFound;
    }
    NSUInteger first;
    NSUInteger last;
    if (options & 0x1000000000000000ULL) {
        first = [indexes firstIndex];
        last = [indexes lastIndex];
    } else {
        first = 0;
        last = [array count] - 1;
    }
    if (first == kNotFound || last == kNotFound) {
        return kNotFound;
    }
    NSUInteger processors;
    if (options & 1) {
        OCFIndex active = __CFActiveProcessorCount();
        if (active < 2) {
            options &= ~(NSUInteger)1;
        }
        processors = (NSUInteger)active;
    } else {
        processors = 1;
    }
    if ((options & 3) == 0) {
        void *pool = _OCFAutoreleasePoolPush();
        BOOL stop = NO;
        NSUInteger calls = 0;
        NSUInteger index = 0;
        NSUInteger found = kNotFound;
        for (id object in array) {
            if (!(options & 0x1000000000000000ULL) ||
                (first <= index && index <= last && [indexes containsIndex:index])) {
                if (__NSARRAY_IS_CALLING_OUT_TO_A_BOOL_BLOCK__((TestBlock)block, object, index, &stop) & 1) {
                    found = index;
                    break;
                }
                if (stop & 1) {
                    break;
                }
                if (calls < 0x3FF) {
                    calls++;
                } else {
                    _OCFAutoreleasePoolPop(pool);
                    pool = _OCFAutoreleasePoolPush();
                    calls = 0;
                }
            }
            index++;
        }
        _OCFAutoreleasePoolPop(pool);
        return found;
    }
    NSUInteger span = last - first;
    NSUInteger chunk = chunkSize(span, processors);
    NSUInteger total = chunk + span;
    NSUInteger iterations = total / chunk;
    if (options & 1) {
        __block NSUInteger result = kNotFound;
        __block BOOL stop = NO;
        dispatch_apply(iterations, NULL, ^(size_t iteration) {
          if (__atomic_load_n(&stop, __ATOMIC_ACQUIRE) & 1) {
              return;
          }
          NSUInteger chunkStart = first + chunk * iteration;
          NSUInteger chunkEnd = chunk + chunkStart - 1;
          if (chunkEnd >= last) {
              chunkEnd = last;
          }
          BOOL stopped =
              __NSArrayChunkIterate(array, options, chunkStart, chunkEnd, indexes, block, &result, nil, NULL);
          __atomic_store_n(&stop, stopped, __ATOMIC_RELEASE);
        });
        return __atomic_load_n(&result, __ATOMIC_ACQUIRE);
    }
    NSUInteger result = kNotFound;
    if (chunk <= total) {
        NSUInteger iteration = 0;
        NSUInteger number = 1;
        NSUInteger forwardStart = first;
        NSUInteger reverseEnd = last;
        while (1) {
            NSUInteger chunkStart;
            NSUInteger chunkEnd;
            if (!(options & 2)) {
                chunkEnd = chunk - 1 < reverseEnd - first ? chunk - 1 + (iteration * chunk + first) : last;
                chunkStart = forwardStart;
            } else {
                chunkStart = chunk - 1 < reverseEnd - first ? (1 - chunk) + reverseEnd : first;
                chunkEnd = reverseEnd;
            }
            BOOL stopped = __NSArrayChunkIterate(
                array, options, chunkStart, chunkEnd, indexes, block, &result, nil, NULL
            );
            if (number >= iterations) {
                break;
            }
            iteration++;
            reverseEnd -= chunk;
            forwardStart += chunk;
            number++;
            if (stopped & 1) {
                break;
            }
        }
    }
    return __atomic_load_n(&result, __ATOMIC_ACQUIRE);
}

// Native __NSArrayGetIndexesPassingTest, 0x1c01e8fb0, with its block
// (___NSArrayGetIndexesPassingTest_block_invoke, 0x1c01e93c8). Hidden (was a private external). An empty
// array or an NSNotFound bound answers an autoreleased new NSIndexSet (__CFLookUpClass, objc_opt_new).
// Otherwise a new NSMutableIndexSet collects the passing indexes: serially through fast enumeration as in
// __NSArrayEnumerate (-addIndex: for each pass, then the stop check); reverse through the chunks with no
// result word and the set; concurrently in a dispatch_apply block that also passes a stack mutex
// (PTHREAD_ERRORCHECK_MUTEX_INITIALIZER, signature 0x32AAABA1; never destroyed) around -addIndex:. The answer
// is the set's autoreleased -copy; the set is released.
id __NSArrayGetIndexesPassingTest(id array, id block, NSUInteger options, id indexes) {
    if ([array count] == 0) {
        return [[(Class)__CFLookUpClass(OF_NAME_STRING(ONSIndexSet)) new] autorelease];
    }
    NSUInteger first;
    NSUInteger last;
    if (options & 0x1000000000000000ULL) {
        first = [indexes firstIndex];
        last = [indexes lastIndex];
    } else {
        first = 0;
        last = [array count] - 1;
    }
    if (first == kNotFound || last == kNotFound) {
        return [[(Class)__CFLookUpClass(OF_NAME_STRING(ONSIndexSet)) new] autorelease];
    }
    NSUInteger processors;
    if (options & 1) {
        OCFIndex active = __CFActiveProcessorCount();
        processors = (NSUInteger)active;
        if (active < 2) {
            options &= ~(NSUInteger)1;
        }
    } else {
        processors = 1;
    }
    id resultIndexes = [(Class)__CFLookUpClass(OF_NAME_STRING(ONSMutableIndexSet)) new];
    if ((options & 3) == 0) {
        void *pool = _OCFAutoreleasePoolPush();
        BOOL stop = NO;
        NSUInteger calls = 0;
        NSUInteger index = 0;
        for (id object in array) {
            if (!(options & 0x1000000000000000ULL) ||
                (first <= index && index <= last && [indexes containsIndex:index])) {
                if (__NSARRAY_IS_CALLING_OUT_TO_A_BOOL_BLOCK__((TestBlock)block, object, index, &stop)) {
                    [resultIndexes addIndex:index];
                }
                if (stop & 1) {
                    break;
                }
                if (calls < 0x3FF) {
                    calls++;
                } else {
                    _OCFAutoreleasePoolPop(pool);
                    pool = _OCFAutoreleasePoolPush();
                    calls = 0;
                }
            }
            index++;
        }
        _OCFAutoreleasePoolPop(pool);
    } else {
        NSUInteger span = last - first;
        NSUInteger chunk = chunkSize(span, processors);
        NSUInteger total = chunk + span;
        NSUInteger iterations = total / chunk;
        if (options & 1) {
            pthread_mutex_t mutex = PTHREAD_ERRORCHECK_MUTEX_INITIALIZER;
            pthread_mutex_t *mutexPointer = &mutex;
            __block BOOL stop = NO;
            dispatch_apply(iterations, NULL, ^(size_t iteration) {
              if (__atomic_load_n(&stop, __ATOMIC_ACQUIRE) & 1) {
                  return;
              }
              NSUInteger chunkStart = first + chunk * iteration;
              NSUInteger chunkEnd = chunk + chunkStart - 1;
              if (chunkEnd >= last) {
                  chunkEnd = last;
              }
              BOOL stopped = __NSArrayChunkIterate(
                  array, options, chunkStart, chunkEnd, indexes, block, NULL, resultIndexes, mutexPointer
              );
              __atomic_store_n(&stop, stopped, __ATOMIC_RELEASE);
            });
        } else if (chunk <= total) {
            NSUInteger iteration = 0;
            NSUInteger number = 1;
            NSUInteger forwardStart = first;
            NSUInteger reverseEnd = last;
            while (1) {
                NSUInteger chunkStart;
                NSUInteger chunkEnd;
                if (!(options & 2)) {
                    chunkEnd =
                        chunk - 1 < reverseEnd - first ? chunk - 1 + (iteration * chunk + first) : last;
                    chunkStart = forwardStart;
                } else {
                    chunkStart = chunk - 1 < reverseEnd - first ? (1 - chunk) + reverseEnd : first;
                    chunkEnd = reverseEnd;
                }
                BOOL stopped = __NSArrayChunkIterate(
                    array, options, chunkStart, chunkEnd, indexes, block, NULL, resultIndexes, NULL
                );
                if (number >= iterations) {
                    break;
                }
                iteration++;
                reverseEnd -= chunk;
                forwardStart += chunk;
                number++;
                if (stopped & 1) {
                    break;
                }
            }
        }
    }
    id answer = [[resultIndexes copy] autorelease];
    [resultIndexes release];
    return answer;
}

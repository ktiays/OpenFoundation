//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Collections/OCFNSCFDictionary.h"
#include "CoreFoundation/Objective-C/OCFNSDictionaryCopyOnWrite.h"
#include "CoreFoundation/Objective-C/OCFNSMutableDictionary.h"

#ifdef __OBJC__
// The storage ivar of __NSDictionaryM (16 bytes at 8), as the unit's methods read it: the buffer pointer
// (keys at [0, size), objects at [size, 2 size), size = __NSDictionarySizes[szidx], zeroed by
// malloc_type_calloc), then one 64-bit word: bits 0-30 a mutation counter (incremented by every mutation,
// reset to 0 once bits 9-30 reach 125), bit 31 whether keys are copied (option bit 1 of __NSDictionaryM_new;
// read by -setObject:forKeyedSubscript: and -__setObject:forKey:), bits 32-56 the entry count, bit 57 whether
// observation info is set (-setObservationInfo:, sending the KVO notifications), bits 58-63 the size index
// (confidence: verified for the positions and widths; the native field names and type encoding lie outside
// the extracted image, the names are descriptive). DictionaryMStorage is a local descriptive name.
typedef struct {
    id *buffer;

    union {
        uint64_t word;

        struct {
            uint64_t mutations : 31;
            uint64_t copyKeys : 1;
            uint64_t used : 25;
            uint64_t kvo : 1;
            uint64_t szidx : 6;
        };
    } bits;
} DictionaryMStorage;

_Static_assert(sizeof(DictionaryMStorage) == 16, "The __NSDictionaryM storage ivar is 16 bytes.");

// Native class __NSDictionaryM (__DATA,__objc_data 0x22e6cf2f0, was a private external), the mutable hashed
// dictionary, an NSMutableDictionary subclass; class_ro_t 0x2227c8708: instance start 8, size 32; ivars
// (instance variable list 0x2227c86c0, offsets from their _OBJC_IVAR_$ symbols) storage at 8 (16 bytes) and
// cow at 24 (8 bytes, the copy-on-write control block, read with load-acquire) (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for offsets and sizes; the type encodings lie
// outside the extracted image). The ivars are public so that the unit's C helpers (mdict_*, cow_cleanup)
// reach them, as the native functions do.
@interface __ONSDictionaryM : ONSMutableDictionary {
    @public
    DictionaryMStorage storage;
    CopyOnWriteBlock *cow;
}
@end

// Native mdict_keysOfEntriesWithOptionsPassingTest, 0x1c02f46b8. External natively; a name outside the CF/NS
// prefix rule, hidden until the owner records a naming decision. The storage of a __NSDictionaryM (or of
// __NSFrozenDictionaryM, whose methods are not restored), the NSEnumerationOptions and the predicate; answers
// the autoreleased set of the keys that pass. Defined by Objective-C/OCFNSDictionaryM.m.
OF_EXTERN id mdict_keysOfEntriesWithOptionsPassingTest(
    DictionaryMStorage *storage, NSUInteger options, BOOL (^predicate)(id key, id object, BOOL *stop)
);
#endif

//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSBlock.h"
#import "CoreFoundation/String/OCFStringFormat.h"

#include <Block.h>
#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <objc/runtime.h>
#include <pthread/qos.h>

// Inferred organization: the NSBlock family methods (0x1c02b5028-0x1c02b5368) lie together in the iOS
// Simulator __text section, followed by __CFMakeNSBlockClasses (0x1c02b536c, restored with the Objective-C
// runtime units, not here); following the owner's decision, the family has its own translation unit. The unit
// is Objective-C compiled with manual reference counting (objc_autorelease, -retain and -release overrides).
// Unless a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The blocks ABI layout the methods read: the flags word at 8 (bit 25, BLOCK_HAS_COPY_DISPOSE), the invoke
// function at 16 and the descriptor at 24, whose copy and dispose helpers sit at 16 and 24 when bit 25 is
// set. BlockLayout and BlockDescriptor are local descriptive spellings.
typedef struct {
    uintptr_t reserved;
    uintptr_t size;
    void (*copy)(void *destination, const void *source);
    void (*dispose)(const void *block);
} BlockDescriptor;

typedef struct {
    void *isa;
    int32_t flags;
    int32_t reserved;
    void *invoke;
    const BlockDescriptor *descriptor;
} BlockLayout;

enum {
    kBlockHasCopyDispose = 1 << 25,
};

@implementation ONSBlock

// Native +[NSBlock allocWithZone:], 0x1c02b5028. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}

// Native +[NSBlock alloc], 0x1c02b5044. Non-external.
+ (id)alloc {
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}

// Native -[NSBlock copy], 0x1c02b5060. Non-external.
- (id)copy {
    return _Block_copy(self);
}

// Native -[NSBlock copyWithZone:], 0x1c02b5064. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return _Block_copy(self);
}

// Native -[NSBlock invoke], 0x1c02b5068. Non-external. A tail jump to the block's invoke function with the
// receiver in X0 and the invoke address in X1.
// Divergence: the native jump leaves every other argument register as the caller set it; this C call passes
// only those two (confidence: medium that no caller relies on more, since -invoke takes no arguments).
- (void)invoke {
    void *invoke = ((BlockLayout *)self)->invoke;
    ((void (*)(void *, void *))invoke)(self, invoke);
}

// Native -[NSBlock performAfterDelay:], 0x1c02b5070. Non-external. The delay in seconds becomes nanoseconds
// through three multiplications by 1000.0 (qword 0x1c0373570) and a truncating conversion, at least 2; the
// receiver is dispatched after that delay on the global queue of the caller's QoS class.
- (void)performAfterDelay:(double)delay {
    uint64_t nanoseconds = (uint64_t)(delay * 1000.0 * 1000.0 * 1000.0);
    dispatch_time_t when = dispatch_time(DISPATCH_TIME_NOW, nanoseconds > 2 ? (int64_t)nanoseconds : 2);
    dispatch_queue_t queue = dispatch_get_global_queue(qos_class_self(), 0);
    dispatch_after(when, queue, (dispatch_block_t)self);
}

// Native -[NSBlock debugDescription], 0x1c02b50d8. Non-external. Formats the class name, the block and its
// signature, then the invoke, copy and dispose helpers (the last two only when the block has copy/dispose
// helpers), each with the library and symbol dladdr reports; a helper that is NULL or that dladdr cannot
// place is omitted. The Dl_info record is zeroed once and reused.
- (id)debugDescription {
    BlockLayout *block = (BlockLayout *)self;
    const char *signature;
    void (*copyHelper)(void *, const void *);
    void (*disposeHelper)(const void *);
    if ((block->flags & kBlockHasCopyDispose) != 0) {
        const BlockDescriptor *descriptor = block->descriptor;
        signature = _Block_signature(self);
        copyHelper = descriptor->copy;
        disposeHelper = descriptor->dispose;
    } else {
        signature = _Block_signature(self);
        copyHelper = NULL;
        disposeHelper = NULL;
    }
    void *invoke = block->invoke;
    OCFMutableStringRef description = OCFStringCreateMutable(kOCFAllocatorSystemDefault, 0);
    const char *className = class_getName([self class]);
    OCFStringAppendFormat(
        description,
        NULL,
        OF_STR("<%s: %p>\n signature: \"%s\""),
        className,
        self,
        signature == NULL ? "<unknown signature>" : signature
    );
    Dl_info info = {0};
    if (invoke != NULL && dladdr(invoke, &info) != 0) {
        OCFStringAppendFormat(
            description,
            NULL,
            OF_STR("\n invoke   : %p (%s`%s)"),
            invoke,
            info.dli_fname == NULL ? "<unknown invoke library>" : info.dli_fname,
            info.dli_sname == NULL ? "<unknown invoke symbol>" : info.dli_sname
        );
    }
    if (copyHelper != NULL && dladdr((const void *)copyHelper, &info) != 0) {
        OCFStringAppendFormat(
            description,
            NULL,
            OF_STR("\n copy     : %p (%s`%s)"),
            copyHelper,
            info.dli_fname == NULL ? "<unknown copy library>" : info.dli_fname,
            info.dli_sname == NULL ? "<unknown copy symbol>" : info.dli_sname
        );
    }
    if (disposeHelper != NULL && dladdr((const void *)disposeHelper, &info) != 0) {
        OCFStringAppendFormat(
            description,
            NULL,
            OF_STR("\n dispose  : %p (%s`%s)"),
            disposeHelper,
            info.dli_fname == NULL ? "<unknown dispose library>" : info.dli_fname,
            info.dli_sname == NULL ? "<unknown dispose symbol>" : info.dli_sname
        );
    }
    return [(id)description autorelease];
}

@end

@implementation __ONSStackBlock

// Native -[__NSStackBlock retain], 0x1c02b52d8. Non-external. Answers the receiver.
- (id)retain {
    return self;
}

// Native -[__NSStackBlock release], 0x1c02b52dc. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[__NSStackBlock retainCount], 0x1c02b52e0. Non-external.
- (NSUInteger)retainCount {
    return 1;
}

// Native -[__NSStackBlock autorelease], 0x1c02b52e8. Non-external. Answers the receiver.
- (id)autorelease {
    return self;
}

@end

@implementation __ONSMallocBlock

// Native -[__NSMallocBlock retain], 0x1c02b52ec. Non-external. _Block_copy's result is discarded; the
// receiver is answered.
- (id)retain {
    _Block_copy(self);
    return self;
}

// Native -[__NSMallocBlock release], 0x1c02b5310. Non-external.
- (oneway void)release {
    _Block_release(self);
}

// Native -[__NSMallocBlock retainCount], 0x1c02b5314. Non-external.
- (NSUInteger)retainCount {
    return 1;
}

// Native -[__NSMallocBlock _tryRetain], 0x1c02b531c. Non-external.
- (BOOL)_tryRetain {
    return _Block_tryRetain(self);
}

// Native -[__NSMallocBlock _isDeallocating], 0x1c02b5320. Non-external.
- (BOOL)_isDeallocating {
    return _Block_isDeallocating(self);
}

@end

@implementation __ONSAutoBlock

// Native -[__NSAutoBlock copy], 0x1c02b5324. Non-external. Answers the receiver, unretained.
- (id)copy {
    return self;
}

// Native -[__NSAutoBlock copyWithZone:], 0x1c02b5328. Non-external. Answers the receiver, unretained.
- (id)copyWithZone:(struct _NSZone *)zone {
    return self;
}

@end

@implementation __ONSFinalizingBlock

// Native -[__NSFinalizingBlock finalize], 0x1c02b532c. Non-external. A block with copy/dispose helpers runs
// its dispose helper (a tail jump with the receiver in X0); the descriptor is loaded before the flags test.
- (void)finalize {
    const BlockDescriptor *descriptor = ((BlockLayout *)self)->descriptor;
    if ((((BlockLayout *)self)->flags & kBlockHasCopyDispose) != 0) {
        descriptor->dispose(self);
    }
}

@end

@implementation __ONSGlobalBlock

// Native -[__NSGlobalBlock copy], 0x1c02b5344. Non-external. Answers the receiver, unretained.
- (id)copy {
    return self;
}

// Native -[__NSGlobalBlock copyWithZone:], 0x1c02b5348. Non-external. Answers the receiver, unretained.
- (id)copyWithZone:(struct _NSZone *)zone {
    return self;
}

// Native -[__NSGlobalBlock release], 0x1c02b534c. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[__NSGlobalBlock retain], 0x1c02b5350. Non-external. Answers the receiver.
- (id)retain {
    return self;
}

// Native -[__NSGlobalBlock retainCount], 0x1c02b5354. Non-external.
- (NSUInteger)retainCount {
    return 1;
}

// Native -[__NSGlobalBlock _tryRetain], 0x1c02b535c. Non-external.
- (BOOL)_tryRetain {
    return YES;
}

// Native -[__NSGlobalBlock _isDeallocating], 0x1c02b5364. Non-external.
- (BOOL)_isDeallocating {
    return NO;
}

@end

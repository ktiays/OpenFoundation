//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Archiving/ONSKeyedCoderOldStyleArray.h"

#import "Foundation/Archiving/ONSArchivingPrivate.h"

#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "Foundation/Number/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSRuntimeSupport.h"

#include <objc/objc-exception.h>
#include <stdlib.h>
#include <string.h>

// Foundation, iOS Simulator 27.0 (24A434), arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured IDA assembly. iOS Device divergence: iOS Device 24A446 is unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json.

@implementation _ONSKeyedCoderOldStyleArray

// Native +[_NSKeyedCoderOldStyleArray supportsSecureCoding], 0x1c0563498. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[_NSKeyedCoderOldStyleArray initWithObjCType:count:at:], 0x1c05634a0. Non-external.
// Foundation defect: 0x1c0563548 releases self before 0x1c0563560 inspects its class for the
// diagnostic. Compute the proem first so the error path never messages a deallocated receiver.
- (id)initWithObjCType:(char)type count:(NSUInteger)count at:(const void *)address {
    NSUInteger size = 0;
    NSUInteger alignment = 0;
    char encoding[2] = {type, '\0'};
    ONSGetSizeAndAlignment(encoding, &size, &alignment);
    if (size == 0 || alignment == 0) {
        // Shared-cache selector 0x2387b58ec is bound by its encoder call site (confidence: medium).
        id proem = _ONSMethodExceptionProem(self, sel_getUid("encodeArrayOfObjCType:count:at:"));
        objc_release(self);
        id reason = [(id)archivingStringClass
            stringWithFormat:(id)OF_STR("%@: size (%ld) or alignment (%ld) of type ('%c') is zero"),
                             proem,
                             (NSInteger)size,
                             (NSInteger)alignment,
                             (int)type];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInternalInconsistencyException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    _type = type;
    _addr = (void *)address;
    _count = count;
    _size = size > alignment ? size : alignment;
    _decoded = NO;
    return self;
}

// Native -[_NSKeyedCoderOldStyleArray dealloc], 0x1c05635ac. Non-external.
- (void)dealloc {
    if (_decoded && _addr != NULL) {
        free(_addr);
    }
    [super dealloc];
}

// Native -[_NSKeyedCoderOldStyleArray fillObjCType:count:at:], 0x1c0563630. Non-external.
- (void)fillObjCType:(char)type count:(NSUInteger)count at:(void *)address {
    if (_count == 0 || _size == 0) {
        id proem = _ONSMethodExceptionProem(self, _cmd);
        id reason = [(id)archivingStringClass
            stringWithFormat:(id)OF_STR("%@: stored element size (%ld) or count (%ld) is zero"),
                             proem,
                             (NSInteger)_size,
                             (NSInteger)_count];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInternalInconsistencyException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (count != _count || type != _type) {
        // Shared-cache selector 0x2387b3efe is bound by its decoder call site (confidence: medium).
        id proem = _ONSMethodExceptionProem(self, sel_getUid("decodeArrayOfObjCType:count:at:"));
        id reason = [(id)archivingStringClass
            stringWithFormat:
                (id)OF_STR("%@: type ('%c') or count (%ld) does not match stored type and count ('%c', %ld)"),
                proem,
                (int)type,
                (NSInteger)count,
                (int)_type,
                (NSInteger)_count];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    memmove(address, _addr, _size * count);
}

@end

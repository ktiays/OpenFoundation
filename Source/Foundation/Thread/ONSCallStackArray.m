//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Thread/ONSCallStackArray.h"

#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "Foundation/Number/ONSNumber.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSString.h"

#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include <execinfo.h>
#include <objc/objc-exception.h>
#include <stdlib.h>

// Foundation, iOS Simulator 27.0 (24A434), arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured IDA assembly. iOS Device divergence: iOS Device 24A446 is unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json.

extern id objc_opt_new(id cls);
extern id objc_retain(id object);
extern id objc_autorelease(id object);
extern char callStackArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(_ONSCallStackArray));

@implementation _ONSCallStackArray

// Native +[_NSCallStackArray arrayWithFrames:count:symbols:], 0x1c05e2454. Non-external.
+ (id)arrayWithFrames:(void **)frames count:(NSUInteger)count symbols:(BOOL)symbols {
    _ONSCallStackArray *array = objc_opt_new((id)callStackArrayClass);
    array->_ignore = 2;
    array->_frames = frames;
    array->_cnt = count >= 2 ? count - 2 : 0;
    array->_pcstrs = NULL;
    array->_wantSyms = symbols;
    return objc_autorelease(array);
}

// Native -[_NSCallStackArray dealloc], 0x1c05e24c8. Non-external.
- (void)dealloc {
    free(_frames);
    if (_pcstrs != NULL) {
        free(_pcstrs);
    }
    [super dealloc];
}

// Native -[_NSCallStackArray count], 0x1c05e2558. Non-external.
- (NSUInteger)count {
    return _cnt;
}

// Native -[_NSCallStackArray objectAtIndex:], 0x1c05e2568. Non-external.
// Foundation defect: the unsynchronized LDR at 0x1c05e25b8 and STR at 0x1c05e25dc race when readers
// first resolve the same immutable array. A release/acquire compare-and-exchange publishes one buffer
// and releases the losing allocation. A failed allocation still answers an address and permits a retry.
- (id)objectAtIndex:(NSUInteger)index {
    if (index >= _cnt) {
        id proem = _ONSMethodExceptionProem(self, _cmd);
        id reason = [ONSString stringWithFormat:(id)OF_STR("%@: index (%ld) beyond bounds (%ld)"),
                                                proem,
                                                (NSInteger)index,
                                                (NSInteger)_cnt];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSRangeException reason:reason
                                                    userInfo:nil]);
    }
    if (_wantSyms) {
        char **symbols = __atomic_load_n(&_pcstrs, __ATOMIC_ACQUIRE);
        if (symbols == NULL) {
            char **resolved = backtrace_symbols(_frames + _ignore, (int)_cnt);
            if (resolved != NULL) {
                if (__atomic_compare_exchange_n(
                        &_pcstrs, &symbols, resolved, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
                    )) {
                    symbols = resolved;
                } else {
                    free(resolved);
                }
            }
        }
        if (symbols != NULL) {
            return [ONSString stringWithUTF8String:symbols[index]];
        }
    }
    return [ONSNumber numberWithUnsignedInteger:(uintptr_t)_frames[index + _ignore]];
}

// Native -[_NSCallStackArray copyWithZone:], 0x1c05e2a04. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return objc_retain(self);
}

@end

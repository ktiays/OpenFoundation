//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSMachPort.h"

// Inferred organization: the private NSMachPort accessors (0x1c01b17f8-0x1c01b1904) lie together in the iOS
// Simulator __text section, just before the __NSCFConstantString and __NSCFString methods; following the
// owner's decision, the class has its own translation unit. The native code addresses the ivars at fixed
// offsets (24, 32, 40, 48); the restored code names them. Objective-C, manual reference counting (an explicit
// -dealloc message). Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

@implementation ONSMachPort

// Native -[NSMachPort _setDelegate:], 0x1c01b17f8. Non-external. Stores the delegate unretained.
- (void)_setDelegate:(id)delegate {
    _portDelegate = delegate;
}

// Native -[NSMachPort _delegate], 0x1c01b1800. Non-external.
- (id)_delegate {
    return _portDelegate;
}

// Native -[NSMachPort _setFlags:], 0x1c01b1808. Non-external.
- (void)_setFlags:(uint64_t)flags {
    _portFlags = flags;
}

// Native -[NSMachPort _flags], 0x1c01b1810. Non-external.
- (uint64_t)_flags {
    return _portFlags;
}

// Native -[NSMachPort _increaseRetainCountWithLock:], 0x1c01b1818. Non-external. Under the caller's lock, the
// count grows unless it is saturated at all ones.
- (void)_increaseRetainCountWithLock:(os_unfair_lock_t)lock {
    os_unfair_lock_lock(lock);
    if (_portRetainCount != UINT64_MAX) {
        _portRetainCount = _portRetainCount + 1;
    }
    os_unfair_lock_unlock(lock);
}

// Native -[NSMachPort _decreaseRetainCountAndMaybeDeallocWithLock:andPerformWhenZero:], 0x1c01b1858.
// Non-external. Under the lock: a count of zero first runs the block outside the lock and reads the count
// again. A saturated count is left alone; a nonzero count is decremented; a count still zero unlocks and
// sends -dealloc to the receiver.
- (void)_decreaseRetainCountAndMaybeDeallocWithLock:(os_unfair_lock_t)lock
                                 andPerformWhenZero:(void (^)(void))block {
    os_unfair_lock_lock(lock);
    uint64_t count = _portRetainCount;
    if (count == 0) {
        os_unfair_lock_unlock(lock);
        block();
        os_unfair_lock_lock(lock);
        count = _portRetainCount;
    }
    if (count != UINT64_MAX) {
        if (count == 0) {
            os_unfair_lock_unlock(lock);
            [self dealloc];
            return;
        }
        _portRetainCount = count - 1;
    }
    os_unfair_lock_unlock(lock);
}

// Native -[NSMachPort _retainCount], 0x1c01b18e8. Non-external.
- (uint64_t)_retainCount {
    return _portRetainCount;
}

// Native -[NSMachPort _setMachPort:], 0x1c01b18f0. Non-external.
- (void)_setMachPort:(uint32_t)port {
    _portName = port;
}

// Native -[NSMachPort _machPort], 0x1c01b18f8. Non-external.
- (uint32_t)_machPort {
    return _portName;
}

// Native -[NSMachPort machPort], 0x1c01b1900. Non-external.
- (uint32_t)machPort {
    return _portName;
}

@end

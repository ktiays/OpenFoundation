//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <objc/NSObject.h>

// Native _NSKeyedUnarchiverHelper, 0x22e6dae68, non-external. class_ro_t 0x222857400:
// instance start/size 8/42; offsets 8, 16, 24, 32, 40 and 41.
@class ONSArray;
@class ONSMutableArray;
@class ONSError;

@interface _ONSKeyedUnarchiverHelper : NSObject {
    @private
    ONSArray *_allowed;
    unsigned int _lastRef;
    ONSMutableArray *_allowedClasses;
    ONSError *_decodeError;
    BOOL expectsUnnestedCollection;
    BOOL decodedCollection;
}
- (BOOL)classNameAllowed:(Class)cls;
- (void)setDecodeError:(id)error;
@end

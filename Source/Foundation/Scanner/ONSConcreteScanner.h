//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Scanner/ONSScanner.h"

@class ONSString;
@class ONSCharacterSet;

// Native NSConcreteScanner, 0x22e6dcf10, non-external. class_ro_t 0x22285dfd0:
// instance start/size 8/52, ivars at offsets 8, 16, 24, 32, 40 and 48.
@interface ONSConcreteScanner : ONSScanner {
    @private
    ONSString *scanString;
    ONSCharacterSet *skipSet;
    ONSCharacterSet *invertedSkipSet;
    id locale;
    NSUInteger scanLocation;

    struct {
        unsigned int caseSensitive : 1;
        unsigned int : 31;
    } flags;
}
@end

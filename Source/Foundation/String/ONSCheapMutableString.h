//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSMutableString.h"

#ifdef __OBJC__
// Native NSCheapMutableString, 0x22e6db6d8. External. class_ro_t 0x222859358:
// NSMutableString superclass, instance start 8 and size 40.
__attribute__((visibility("default")))
@interface ONSCheapMutableString : ONSMutableString {
    @private
    union {
        unsigned short *fat;
        char *thin;
    } contents;

    struct {
        unsigned int isFat : 1;
        unsigned int freeWhenDone : 1;
        unsigned int refs : 30;
    } flags;

    ONSUInteger numCharacters;
    void *_reserved;
}

- (void)setContentsNoCopy:(void *)buffer
                   length:(ONSUInteger)length
             freeWhenDone:(BOOL)freeWhenDone
                isUnicode:(BOOL)isUnicode;

@end
#endif

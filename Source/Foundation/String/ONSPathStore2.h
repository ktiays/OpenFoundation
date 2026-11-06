//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSString.h"

#ifdef __OBJC__
// Native NSPathStore2, 0x22e6dc268. Was a private external. class_ro_t 0x22285b470:
// NSString superclass, instance start 8 and size 12, with tail storage starting at 12.
@interface ONSPathStore2 : ONSString {
    @private
    uint32_t _lengthAndRefCount;
    unsigned short _characters[0];
}

+ (id)pathStoreWithCharacters:(const unsigned short *)characters length:(ONSUInteger)length;
+ (id)pathWithComponents:(id)components;

@end
#endif

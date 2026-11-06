//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "CoreFoundation/Objective-C/OCFNSArray.h"

// Native _NSCallStackArray, 0x22e6dd938, non-external; class_ro_t 0x2228604d0.
// Instance start/size 8/41; ivars at offsets 8, 16, 24, 32 and 40. NSArray is bound to
// 0x22e6cdc70 in CoreFoundation.
@interface _ONSCallStackArray : ONSArray {
    @private
    void **_frames;
    NSUInteger _cnt;
    NSUInteger _ignore;
    char **_pcstrs;
    BOOL _wantSyms;
}
+ (id)arrayWithFrames:(void **)frames count:(NSUInteger)count symbols:(BOOL)symbols;
@end

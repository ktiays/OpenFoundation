//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <Foundation/NSObject.h>

#if defined(__OBJC__)
@interface NSNull : NSObject <NSCopying, NSSecureCoding>

+ (NSNull *)null;

@end
#endif

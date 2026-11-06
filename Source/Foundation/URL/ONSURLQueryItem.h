//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <objc/NSObject.h>

@class ONSString;

// Native NSURLQueryItem class 0x22e6ddd98, class_ro_t 0x222861510.
// iOS Simulator Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified ivar data.
// Superclass 0x22e6b6088 is external: NSObject (SDK hierarchy; confidence: medium).
// Instance start/size 8/24; _name at 8 and _value at 16, both @"NSString", alignment/size 8/8.
__attribute__((visibility("default")))
// Native ivar-offset symbols are non-external in nm. @package preserves hidden linkage while allowing
// same-image helpers to use the read layout; the original source access-specifier spelling is unread.
@interface ONSURLQueryItem : NSObject {
    @package
    ONSString *_name;
    ONSString *_value;
}
@end

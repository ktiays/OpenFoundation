//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSMutableString.h"

// The NSPlaceholderMutableString unit (String/ONSPlaceholderMutableString.m), Foundation image iOS Simulator
// 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. Its overrides are inherited declarations
// (String/ONSString.h); the placeholder object it compares with is String/ONSMutableString.h's.

#ifdef __OBJC__
// Native class NSPlaceholderMutableString (__DATA,__objc_data 0x22e6db750, external), an NSMutableString
// subclass; class_ro_t 0x2228593e8: flags 0, instance start and size 8, no ivars.
__attribute__((visibility("default")))
@interface ONSPlaceholderMutableString : ONSMutableString
@end
#endif

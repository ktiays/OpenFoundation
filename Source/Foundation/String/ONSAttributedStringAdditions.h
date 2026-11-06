//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSAttributedString.h"

@interface ONSAttributedString (_NSPresentationIntentDirtying)
- (BOOL)_mayRequireIntentResolution;
- (void)_markRequiringIntentResolution;
- (BOOL)_willRequireIntentResolutionWhenContainingAttributes:(id)attributes;
- (BOOL)_willRequireIntentResolutionWhenContainingAttribute:(id)attribute value:(id)value;
@end

@interface ONSAttributedString (NSCFAdditions)
- (unsigned long)_cfTypeID;
- (id)_createAttributedSubstringWithRange:(ONSRange)range;
@end

@interface ONSAttributedString (NSAttributedStringPortCoding)
- (id)replacementObjectForPortCoder:(id)coder;
@end

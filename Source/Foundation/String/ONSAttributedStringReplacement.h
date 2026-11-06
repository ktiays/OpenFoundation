//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <OpenFoundation/Range/ONSRange.h>
#import <objc/NSObject.h>

// Native _NSAttributedStringReplacement, 0x22e6da918, private external. class_ro_t 0x222855d28,
// instance start 8, size 64.
@interface _ONSAttributedStringReplacement : NSObject {
    NSInteger _index;
    id _argument;
    NSInteger _replacementKind;
    ONSRange _specifierRangeInFormatString;
    ONSRange _replacementRangeInResult;
}
+ (id)_replacementsFromMetadataArray:(id)metadata;
- (ONSRange)replacementRangeInResult;
- (NSInteger)replacementKind;
- (NSInteger)index;
- (id)argument;
@end

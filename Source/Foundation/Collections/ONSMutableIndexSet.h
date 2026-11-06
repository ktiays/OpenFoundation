//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Collections/ONSIndexSet.h"

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md.

#ifdef __OBJC__
// Native class NSMutableIndexSet (__DATA,__objc_data 0x22e6d9888, external), an NSIndexSet subclass;
// class_ro_t 0x222852d70: flags 0, instance start and size 32, no ivars of its own (confidence: verified).
__attribute__((visibility("default")))
@interface ONSMutableIndexSet : ONSIndexSet
- (Class)classForCoder;
- (void)_ensureRangeCapacity:(NSUInteger)capacity;
- (void)_addRangeToArray:(ONSRange)range;
- (void)_insertRange:(ONSRange)range inArrayAtIndex:(NSUInteger)rangeIndex;
- (void)_removeRangeInArrayAtIndex:(NSUInteger)rangeIndex;
- (void)_replaceRangeInArrayAtIndex:(NSUInteger)rangeIndex withRange:(ONSRange)range;
- (void)_mergeOverlappingRangesStartingAtIndex:(NSUInteger)rangeIndex;
- (void)_convertToUsingRanges;
- (void)addIndexesInRange:(ONSRange)range;
- (void)addIndex:(NSUInteger)value;
@end
#endif

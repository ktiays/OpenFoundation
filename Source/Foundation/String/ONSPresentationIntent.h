//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <objc/NSObject.h>
#include <OpenCoreFoundation/OCFBuild.h>

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified unless noted. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.

@class ONSArray;
@class ONSString;

// Native NSPresentationIntent, 0x22e6e14e8, external. class_ro_t 0x22286e390 gives instance start
// 8, size 96 and the ivars below. Superclass 0x22e6b6088 is NSObject (confidence: medium).
__attribute__((visibility("default")))
@interface ONSPresentationIntent : NSObject {
    @private
    NSInteger _intentKind;
    ONSPresentationIntent *_parentIntent;
    NSInteger _identity;
    NSInteger _ordinal;
    ONSArray *_columnAlignments;
    NSInteger _columnCount;
    NSInteger _headerLevel;
    ONSString *_languageHint;
    NSInteger _column;
    NSInteger _row;
    NSInteger _indentationLevel;
}
@property(nonatomic, readonly) NSInteger intentKind;
@property(atomic, readonly) ONSPresentationIntent *parentIntent;
@property(nonatomic, readonly) NSInteger identity;
@property(nonatomic, readonly) NSInteger ordinal;
@property(nonatomic, readonly) ONSArray *columnAlignments;
@property(nonatomic, readonly) NSInteger columnCount;
@property(nonatomic, readonly) NSInteger headerLevel;
@property(atomic, readonly) ONSString *languageHint;
@property(nonatomic, readonly) NSInteger column;
@property(nonatomic, readonly) NSInteger row;
@property(nonatomic, readonly) NSInteger indentationLevel;
+ (id)paragraphIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent;
+ (id)headerIntentWithIdentity:(NSInteger)identity level:(NSInteger)level nestedInsideIntent:(id)parent;
+ (id)codeBlockIntentWithIdentity:(NSInteger)identity languageHint:(id)language nestedInsideIntent:(id)parent;
+ (id)thematicBreakIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent;
+ (id)orderedListIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent;
+ (id)unorderedListIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent;
+ (id)listItemIntentWithIdentity:(NSInteger)identity ordinal:(NSInteger)ordinal nestedInsideIntent:(id)parent;
+ (id)blockQuoteIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent;
+ (id)tableIntentWithIdentity:(NSInteger)identity
                  columnCount:(NSInteger)count
                   alignments:(id)alignments
           nestedInsideIntent:(id)parent;
+ (id)tableRowIntentWithIdentity:(NSInteger)identity row:(NSInteger)row nestedInsideIntent:(id)parent;
+ (id)tableHeaderRowIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent;
+ (id)tableCellIntentWithIdentity:(NSInteger)identity column:(NSInteger)column nestedInsideIntent:(id)parent;
- (BOOL)isEquivalentToPresentationIntent:(ONSPresentationIntent *)other;
@end

// External Foundation attribute names. Their values are independent of class naming mode.
OF_EXPORT id const ONSInlinePresentationIntentAttributeName;
OF_EXPORT id const ONSPresentationIntentAttributeName;

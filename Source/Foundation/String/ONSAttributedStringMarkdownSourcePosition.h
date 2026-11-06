//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <objc/NSObject.h>

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified unless noted. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.

// The signed, 48-byte record shared by both source-position ivars and the Markdown caller's cache.
typedef union {
    NSInteger words[6];

    struct {
        NSInteger utf8Offset;
        NSInteger utf16Offset;
        NSInteger utf8NextCodePoint;
        NSInteger utf16CurrentCodePointLength;
        NSInteger line;
        NSInteger column;
    };
} MarkdownOffsetCache;

// Native NSAttributedStringMarkdownSourcePosition, 0x22e6de928, external. class_ro_t 0x222863540
// gives instance start 8 and size 104. Superclass 0x22e6b6088 is NSObject (confidence: medium).
__attribute__((visibility("default")))
@interface ONSAttributedStringMarkdownSourcePosition : NSObject {
    @private
    MarkdownOffsetCache _startOffsets;
    MarkdownOffsetCache _endOffsets;
}
- (id)initWithStartLine:(NSInteger)startLine
            startColumn:(NSInteger)startColumn
                endLine:(NSInteger)endLine
              endColumn:(NSInteger)endColumn;
- (void)setOffsetsFromUTF8:(const char *)bytes size:(size_t)size usingCache:(MarkdownOffsetCache *)cache;
- (NSInteger)startLine;
- (NSInteger)startColumn;
- (NSInteger)endLine;
- (NSInteger)endColumn;
@end

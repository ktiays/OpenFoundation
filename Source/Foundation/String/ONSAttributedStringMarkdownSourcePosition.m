//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSAttributedStringMarkdownSourcePosition.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#import "Foundation/Error/ONSErrorKeyedArchivalErrors.h"
#include "CoreFoundation/Objective-C/OCFNSConstantArray.h"
#include "CoreFoundation/Objective-C/OCFNSException.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/String/OCFString.h"

#include <objc/objc-exception.h>

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified unless noted. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.

extern char
    sourcePositionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSAttributedStringMarkdownSourcePosition));
extern char sourcePositionArchiveKeysClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSConstantArray));
extern char sourcePositionStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSString));
extern char sourcePositionErrorClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSError));
extern char sourcePositionExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));

@protocol MarkdownSourcePositionCoder
- (BOOL)allowsKeyedCoding;
- (void)encodeInteger:(NSInteger)value forKey:(id)key;
- (BOOL)containsValueForKey:(id)key;
- (NSInteger)decodeIntegerForKey:(id)key;
- (id)error;
- (void)failWithError:(id)error;
@end

// FNEG of the all-ones 64-bit word in the native initializer and cache helper clears the sign bit.
// The result is an integer sentinel in this record. Hex-Rays displays the SIMD lanes as floating-point NaNs.
static const NSInteger kUnreadOffset = NSIntegerMax;
static void updateOffsetsForPosition(
    const unsigned char *bytes, size_t size, NSInteger line, NSInteger column, MarkdownOffsetCache *cache
);

// Native _trailingBytesForUTF8, 0x1c100e0d8. Non-external. The helper reads these bytes with LDRSB.
static const int8_t trailingBytesForUTF8[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5,
};

// The twelve constant strings referenced by the NSConstantArray at 0x22289bb28.
static const OCFConstantString kArchiveKeyStrings[] = {
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSStartLine", sizeof("NSStartLine") - 1},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSStartColumn", sizeof("NSStartColumn") - 1},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSEndLine", sizeof("NSEndLine") - 1},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSEndColumn", sizeof("NSEndColumn") - 1},
    {(uintptr_t)__OCFConstantStringClassReference,
     0x7c8,
     "NSStartUTF8Offset",
     sizeof("NSStartUTF8Offset") - 1},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSEndUTF8Offset", sizeof("NSEndUTF8Offset") - 1},
    {(uintptr_t)__OCFConstantStringClassReference,
     0x7c8,
     "NSStartUTF16Offset",
     sizeof("NSStartUTF16Offset") - 1},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "NSEndUTF16Offset", sizeof("NSEndUTF16Offset") - 1},
    {(uintptr_t)__OCFConstantStringClassReference,
     0x7c8,
     "NSStartUTF8NextCodePoint",
     sizeof("NSStartUTF8NextCodePoint") - 1},
    {(uintptr_t)__OCFConstantStringClassReference,
     0x7c8,
     "NSEndUTF8NextCodePoint",
     sizeof("NSEndUTF8NextCodePoint") - 1},
    {(uintptr_t)__OCFConstantStringClassReference,
     0x7c8,
     "NSStartUTF16CurrentCodePointLength",
     sizeof("NSStartUTF16CurrentCodePointLength") - 1},
    {(uintptr_t)__OCFConstantStringClassReference,
     0x7c8,
     "NSEndUTF16CurrentCodePointLength",
     sizeof("NSEndUTF16CurrentCodePointLength") - 1},
};

static const id kArchiveKeyObjects[] = {
    (id)&kArchiveKeyStrings[0],
    (id)&kArchiveKeyStrings[1],
    (id)&kArchiveKeyStrings[2],
    (id)&kArchiveKeyStrings[3],
    (id)&kArchiveKeyStrings[4],
    (id)&kArchiveKeyStrings[5],
    (id)&kArchiveKeyStrings[6],
    (id)&kArchiveKeyStrings[7],
    (id)&kArchiveKeyStrings[8],
    (id)&kArchiveKeyStrings[9],
    (id)&kArchiveKeyStrings[10],
    (id)&kArchiveKeyStrings[11],
};

// Native __objc_arrayobj at 0x22289bb28, backing object pointers at 0x22288f880. Non-external.
static const struct {
    const void *isa;
    NSUInteger count;
    const id *objects;
} kArchiveKeys = {
    sourcePositionArchiveKeysClass,
    sizeof(kArchiveKeyObjects) / sizeof(*kArchiveKeyObjects),
    kArchiveKeyObjects
};

@implementation ONSAttributedStringMarkdownSourcePosition

// Native -[NSAttributedStringMarkdownSourcePosition startLine], 0x1c060061c. Non-external.
- (NSInteger)startLine {
    return _startOffsets.line;
}

// Native -[NSAttributedStringMarkdownSourcePosition startColumn], 0x1c0600624. Non-external.
- (NSInteger)startColumn {
    return _startOffsets.column;
}

// Native -[NSAttributedStringMarkdownSourcePosition endLine], 0x1c060062c. Non-external.
- (NSInteger)endLine {
    return _endOffsets.line;
}

// Native -[NSAttributedStringMarkdownSourcePosition endColumn], 0x1c0600634. Non-external.
- (NSInteger)endColumn {
    return _endOffsets.column;
}

// Native -[NSAttributedStringMarkdownSourcePosition initWithStartLine:startColumn:endLine:endColumn:],
// 0x1c060063c. Non-external.
- (id)initWithStartLine:(NSInteger)startLine
            startColumn:(NSInteger)startColumn
                endLine:(NSInteger)endLine
              endColumn:(NSInteger)endColumn {
    self = [super init];
    if (self != nil) {
        _startOffsets = (MarkdownOffsetCache){
            {kUnreadOffset, kUnreadOffset, kUnreadOffset, kUnreadOffset, startLine, startColumn}
        };
        _endOffsets = (MarkdownOffsetCache){
            {kUnreadOffset, kUnreadOffset, kUnreadOffset, kUnreadOffset, endLine, endColumn}
        };
    }
    return self;
}

// Native -[NSAttributedStringMarkdownSourcePosition setOffsetsFromUTF8:size:usingCache:],
// 0x1c06007ac. Non-external. Hex-Rays omits the first call's X4 argument and shows the final load
// as a vector return. The assembly passes the same cache to each call and stores both snapshots.
- (void)setOffsetsFromUTF8:(const char *)bytes size:(size_t)size usingCache:(MarkdownOffsetCache *)cache {
    updateOffsetsForPosition(
        (const unsigned char *)bytes, size, _startOffsets.line, _startOffsets.column, cache
    );
    _startOffsets = *cache;
    updateOffsetsForPosition((const unsigned char *)bytes, size, _endOffsets.line, _endOffsets.column, cache);
    _endOffsets = *cache;
}

// Native _updateOffsetsForPosition, 0x1c0600828. Non-external. The scan advances from the supplied
// cache, including when the requested line precedes its current line. CRLF is treated as one line break
// only while seeking a different line. The assembly compares the offset sentinel as integer words.
static void updateOffsetsForPosition(
    const unsigned char *bytes, size_t size, NSInteger line, NSInteger column, MarkdownOffsetCache *cache
) {
    if (line > 0 && column > 0) {
        MarkdownOffsetCache position = *cache;
        if (position.utf8Offset == kUnreadOffset || position.utf16Offset == kUnreadOffset ||
            position.utf8NextCodePoint == kUnreadOffset ||
            position.utf16CurrentCodePointLength == kUnreadOffset || position.line < 1 ||
            position.column <= 0) {
            position = (MarkdownOffsetCache){{0, 0, 0, 0, 1, 1}};
            *cache = position;
        }
        while (position.utf8Offset < (NSInteger)size) {
            NSInteger offset = position.utf8Offset;
            unsigned char byte = bytes[offset];
            if (offset == position.utf8NextCodePoint) {
                position.utf16Offset += position.utf16CurrentCodePointLength;
                NSInteger codePointLength = trailingBytesForUTF8[byte] + 1;
                position.utf8NextCodePoint += codePointLength;
                position.utf16CurrentCodePointLength = codePointLength == 0 ? 0 : (byte < 0xF0 ? 1 : 2);
            }
            if (position.line == line) {
                if (position.column == column) {
                    *cache = position;
                    return;
                }
                ++position.column;
                ++position.utf8Offset;
            } else if (byte == '\n' || byte == '\r') {
                NSInteger newlineLength = 1;
                if (byte == '\r' && size - offset != 1 && bytes[offset + 1] == '\n') {
                    newlineLength = 2;
                }
                ++position.line;
                position.column = 1;
                position.utf8Offset += newlineLength;
            } else {
                ++position.column;
                ++position.utf8Offset;
            }
        }
    }
    *cache =
        (MarkdownOffsetCache){{kUnreadOffset, kUnreadOffset, kUnreadOffset, kUnreadOffset, line, column}};
}

// Native -[NSAttributedStringMarkdownSourcePosition copyWithZone:], 0x1c0600ac4. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return objc_retain(self);
}

// Native +[NSAttributedStringMarkdownSourcePosition supportsSecureCoding], 0x1c0600ac8. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSAttributedStringMarkdownSourcePosition description], 0x1c0600ad0. Non-external.
- (id)description {
    id base = [super description];
    return [(id)sourcePositionStringClass
        stringWithFormat:(id)OF_STR("%@{startLine=%lu, startColumn=%lu, endLine=%lu, endColumn=%lu}"),
                         base,
                         _startOffsets.line,
                         _startOffsets.column,
                         _endOffsets.line,
                         _endOffsets.column];
}

// Native -[NSAttributedStringMarkdownSourcePosition debugDescription], 0x1c0600b68. Non-external.
- (id)debugDescription {
    return [(id)sourcePositionStringClass
        stringWithFormat:(id)OF_STR(
                             "<%@ %p>{ startPosition = (%lu, %lu), endPosition = (%lu, %lu), "
                             "utf8Offsets = (%lu, %lu), utf16Offsets = (%lu, %lu), nextCodePoints = "
                             "(%lu, %lu), currentCodePointUTF16Lengths = (%lu, %lu) }"
                         ),
                         objc_opt_class(self),
                         self,
                         _startOffsets.line,
                         _startOffsets.column,
                         _endOffsets.line,
                         _endOffsets.column,
                         _startOffsets.utf8Offset,
                         _endOffsets.utf8Offset,
                         _startOffsets.utf16Offset,
                         _endOffsets.utf16Offset,
                         _startOffsets.utf8NextCodePoint,
                         _endOffsets.utf8NextCodePoint,
                         _startOffsets.utf16CurrentCodePointLength,
                         _endOffsets.utf16CurrentCodePointLength];
}

// Native -[NSAttributedStringMarkdownSourcePosition encodeWithCoder:], 0x1c0600bd8. Non-external.
- (void)encodeWithCoder:(id<MarkdownSourcePositionCoder>)coder {
    if (![coder allowsKeyedCoding]) {
        id reason = [(id)sourcePositionStringClass
            stringWithFormat:(id)OF_STR("%@ cannot be encoded by non-keyed archivers"), objc_opt_class(self)];
        objc_exception_throw([(id)sourcePositionExceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:reason
                     userInfo:nil]);
    }
    [coder encodeInteger:[self startLine] forKey:kArchiveKeyObjects[0]];
    [coder encodeInteger:[self startColumn] forKey:kArchiveKeyObjects[1]];
    [coder encodeInteger:[self endLine] forKey:kArchiveKeyObjects[2]];
    [coder encodeInteger:[self endColumn] forKey:kArchiveKeyObjects[3]];
    [coder encodeInteger:_startOffsets.utf8Offset forKey:kArchiveKeyObjects[4]];
    [coder encodeInteger:_endOffsets.utf8Offset forKey:kArchiveKeyObjects[5]];
    [coder encodeInteger:_startOffsets.utf16Offset forKey:kArchiveKeyObjects[6]];
    [coder encodeInteger:_endOffsets.utf16Offset forKey:kArchiveKeyObjects[7]];
    [coder encodeInteger:_startOffsets.utf8NextCodePoint forKey:kArchiveKeyObjects[8]];
    [coder encodeInteger:_endOffsets.utf8NextCodePoint forKey:kArchiveKeyObjects[9]];
    [coder encodeInteger:_startOffsets.utf16CurrentCodePointLength forKey:kArchiveKeyObjects[10]];
    [coder encodeInteger:_endOffsets.utf16CurrentCodePointLength forKey:kArchiveKeyObjects[11]];
}

// Native -[NSAttributedStringMarkdownSourcePosition initWithCoder:], 0x1c0600d60. Non-external.
// The native decoder allocates a new instance of the concrete class after decoding every key.
- (id)initWithCoder:(id<MarkdownSourcePositionCoder>)coder {
    if (![coder allowsKeyedCoding]) {
        id reason = [(id)sourcePositionStringClass
            stringWithFormat:(id)OF_STR("%@ cannot be decoded by non-keyed archivers"), objc_opt_class(self)];
        objc_exception_throw([(id)sourcePositionExceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:reason
                     userInfo:nil]);
    }
    for (id key in (ONSConstantArray *)&kArchiveKeys) {
        if (![coder containsValueForKey:key]) {
            id proem = _ONSMethodExceptionProem(self, _cmd);
            id error = [(id)sourcePositionErrorClass
                _readCorruptErrorWithFormat:(id)OF_STR("%@: missing key %@"), proem, key];
            [coder failWithError:error];
            return nil;
        }
    }
    NSInteger values[sizeof(kArchiveKeyObjects) / sizeof(*kArchiveKeyObjects)];
    for (NSUInteger index = 0; index < sizeof(kArchiveKeyObjects) / sizeof(*kArchiveKeyObjects); ++index) {
        values[index] = [coder decodeIntegerForKey:kArchiveKeyObjects[index]];
        if ([coder error] != nil) {
            return nil;
        }
    }
    ONSAttributedStringMarkdownSourcePosition *result =
        [objc_alloc((Class)sourcePositionClass) initWithStartLine:values[0]
                                                      startColumn:values[1]
                                                          endLine:values[2]
                                                        endColumn:values[3]];
    result->_startOffsets.utf8Offset = values[4];
    result->_startOffsets.utf16Offset = values[6];
    result->_endOffsets.utf8Offset = values[5];
    result->_endOffsets.utf16Offset = values[7];
    result->_startOffsets.utf8NextCodePoint = values[8];
    result->_startOffsets.utf16CurrentCodePointLength = values[10];
    result->_endOffsets.utf8NextCodePoint = values[9];
    result->_endOffsets.utf16CurrentCodePointLength = values[11];
    return result;
}

// Native -[NSAttributedStringMarkdownSourcePosition hash], 0x1c06010b8. Non-external.
- (NSUInteger)hash {
    NSUInteger startLine = [self startLine];
    NSUInteger startColumn = [self startColumn];
    NSUInteger endLine = [self endLine];
    NSUInteger endColumn = [self endColumn];
    return startLine ^ startColumn ^ endLine ^ endColumn ^ _startOffsets.utf8Offset ^ _endOffsets.utf8Offset ^
           _startOffsets.utf16Offset ^ _endOffsets.utf16Offset ^ _startOffsets.utf8NextCodePoint ^
           _endOffsets.utf8NextCodePoint ^ _startOffsets.utf16CurrentCodePointLength ^
           _endOffsets.utf16CurrentCodePointLength;
}

// Native -[NSAttributedStringMarkdownSourcePosition isEqual:], 0x1c0601140. Non-external.
// The comparison intentionally observes only line and column getters (kept).
- (BOOL)isEqual:(id)object {
    if (!objc_opt_isKindOfClass(object, (id)objc_opt_class((id)sourcePositionClass))) {
        return NO;
    }
    ONSAttributedStringMarkdownSourcePosition *other = object;
    NSInteger startLine = [other startLine];
    if (startLine != [self startLine]) {
        return NO;
    }
    NSInteger endLine = [other endLine];
    if (endLine != [self endLine]) {
        return NO;
    }
    NSInteger startColumn = [other startColumn];
    if (startColumn != [self startColumn]) {
        return NO;
    }
    NSInteger endColumn = [other endColumn];
    return endColumn == [self endColumn];
}

@end

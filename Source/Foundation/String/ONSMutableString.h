//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSString.h"

// Shared declarations of the NSMutableString unit (String/ONSMutableString.m), Foundation image iOS Simulator
// 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified unless a declaration says
// otherwise. iOS Device 24A446: unread. The class declaration follows the native storage declarations.

// The 16-byte record of a static placeholder object: the isa and a zero word. StringPlaceholderObject is a
// local descriptive type name.
typedef struct {
    void *isa;
    uintptr_t reserved;
} StringPlaceholderObject;

// Native ___mutablePlaceholder (0x2227d5ad0, __DATA_CONST,__const, non-external): {
// NSPlaceholderMutableString, 0 }, the object +[NSMutableString allocWithZone:] answers for NSMutableString
// itself. Hidden native name: String/ONSPlaceholderMutableString.m compares receivers with it.
OF_EXTERN const StringPlaceholderObject __mutablePlaceholder;

// Native __NSNewStringByAppendingStrings, 0x1c0571a50, external: a new (+1) string of the nil-terminated
// variadic strings (all of them on the stack after the zone; at most 40), allocated in the zone (NULL: the
// default malloc zone).
OF_EXPORT id _ONSNewStringByAppendingStrings(struct _NSZone *zone, ...);

#ifdef __OBJC__
// Native class NSMutableString (__DATA,__objc_data 0x22e6db6b0, external), an NSString subclass; class_ro_t
// 0x222859240: flags 0, instance start and size 8, no ivars.
__attribute__((visibility("default")))
@interface ONSMutableString : ONSString

+ (id)allocWithZone:(struct _NSZone *)zone;
+ (id)stringWithCapacity:(ONSUInteger)capacity;
+ (BOOL)_subclassesMustBeExplicitlyMentionedWhenDecoded;

- (void)replaceCharactersInRange:(ONSRange)range withString:(id)string;
- (id)initWithCapacity:(ONSUInteger)capacity;
- (void)insertString:(id)string atIndex:(ONSUInteger)location;
- (void)appendString:(id)string;
- (void)deleteCharactersInRange:(ONSRange)range;
- (void)replaceCharactersInRange:(ONSRange)range withCString:(const char *)cString length:(ONSUInteger)length;
- (void)replaceCharactersInRange:(ONSRange)range
                  withCharacters:(const unsigned short *)characters
                          length:(ONSUInteger)length;
- (void)appendFormat:(id)format, ...;
- (void)setString:(id)string;
- (ONSUInteger)replaceOccurrencesOfString:(id)target
                               withString:(id)replacement
                                  options:(ONSUInteger)options
                                    range:(ONSRange)searchRange;
- (Class)classForCoder;

@end
#endif

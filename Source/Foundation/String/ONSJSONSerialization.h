//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <OpenFoundation/Range/ONSRange.h>
#import <objc/NSObject.h>
#include "CoreFoundation/OCFBuild.h"
#include <stdbool.h>
#include <stddef.h>

// Native NSJSONSerialization, 0x22e6e0020, external; class_ro_t 0x2228685e0.
// Instance start and size 8, no ivars.
__attribute__((visibility("default")))
@interface ONSJSONSerialization : NSObject
+ (id)JSONObjectWithData:(id)data options:(NSUInteger)options error:(id *)error;
+ (id)dataWithJSONObject:(id)object options:(NSUInteger)options error:(id *)error;
@end

@class ONSOutputStream;
@class ONSError;

// Native _NSJSONWriter, 0x22e6dff58, non-external; class_ro_t 0x2228684c0.
// Instance start and size 8/88. Ivar offsets 8, 16, 24, 32, 40, 48, 56, 64, 72 and 80.

@interface _ONSJSONWriter : NSObject {
    @package
    ONSOutputStream *outputStream;
    int kind;
    char *dataBuffer;
    NSUInteger dataBufferLen;
    NSUInteger dataLen;
    bool freeDataBuffer;
    char *tempBuffer;
    NSUInteger tempBufferLen;
    NSInteger totalDataWritten;
    ONSError *_failure;
}
- (id)dataWithRootObject:(id)object options:(NSUInteger)options;
// Native -[_NSJSONWriter resizeTemporaryBuffer:], 0x1c0630bd4, is direct: -appendString:range: (0x1c0630c80)
// and _convertJSONString (0x1c06345a8) reach it with a BL to its body.
- (void)resizeTemporaryBuffer:(NSUInteger)size __attribute__((objc_direct));
- (ONSError *)failure;
- (BOOL)appendString:(id)string range:(ONSRange)range;
- (void)setFailure:(ONSError *)error;
@end

// Native _appendBytes, 0x1c0630d48. Hidden across the FG and FO units.
OF_EXTERN BOOL _appendBytes(_ONSJSONWriter *writer, const char *bytes, size_t length);

// The shared JSON crash-reason slot in the native image, 0x22e6fc8d0.
OF_EXTERN const char *volatile _NSJSONCrashMessage;

//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFError.h>
#include <CoreFoundation/CFRunLoop.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFURL.h>

CF_EXTERN_C_BEGIN

typedef CFStringRef CFStreamPropertyKey CF_EXTENSIBLE_STRING_ENUM;

typedef CF_ENUM(CFIndex, CFStreamStatus) {
    kCFStreamStatusNotOpen = 0,
    kCFStreamStatusOpening,
    kCFStreamStatusOpen,
    kCFStreamStatusReading,
    kCFStreamStatusWriting,
    kCFStreamStatusAtEnd,
    kCFStreamStatusClosed,
    kCFStreamStatusError
};

typedef CF_OPTIONS(CFOptionFlags, CFStreamEventType) {
    kCFStreamEventNone = 0,
    kCFStreamEventOpenCompleted = 1,
    kCFStreamEventHasBytesAvailable = 2,
    kCFStreamEventCanAcceptBytes = 4,
    kCFStreamEventErrorOccurred = 8,
    kCFStreamEventEndEncountered = 16
};

typedef struct {
    CFIndex version;
    void *info;
    void *(*retain)(void *info);
    void (*release)(void *info);
    CFStringRef (*copyDescription)(void *info);
} CFStreamClientContext;

typedef struct CF_BRIDGED_MUTABLE_TYPE(NSInputStream) __CFReadStream *CFReadStreamRef;
typedef struct CF_BRIDGED_MUTABLE_TYPE(NSOutputStream) __CFWriteStream *CFWriteStreamRef;

typedef void (*CFReadStreamClientCallBack)(CFReadStreamRef stream, CFStreamEventType type, void *clientCallBackInfo);
typedef void (*CFWriteStreamClientCallBack)(
    CFWriteStreamRef stream,
    CFStreamEventType type,
    void *clientCallBackInfo
);

CF_EXPORT CFTypeID CFReadStreamGetTypeID(void);
CF_EXPORT CFTypeID CFWriteStreamGetTypeID(void);

CF_EXPORT const CFStreamPropertyKey kCFStreamPropertyDataWritten;
CF_EXPORT const CFStreamPropertyKey kCFStreamPropertyAppendToFile;

CF_EXPORT CFReadStreamRef CFReadStreamCreateWithData(CFAllocatorRef alloc, CFDataRef data);
CF_EXPORT CFWriteStreamRef CFWriteStreamCreateWithBuffer(CFAllocatorRef alloc, UInt8 *buffer, CFIndex bufferCapacity);
CF_EXPORT CFWriteStreamRef CFWriteStreamCreateWithAllocatedBuffers(
    CFAllocatorRef alloc,
    CFAllocatorRef bufferAllocator
);
CF_EXPORT CFReadStreamRef CFReadStreamCreateWithFile(CFAllocatorRef alloc, CFURLRef fileURL);
CF_EXPORT CFWriteStreamRef CFWriteStreamCreateWithFile(CFAllocatorRef alloc, CFURLRef fileURL);

CF_EXPORT CFStreamStatus CFReadStreamGetStatus(CFReadStreamRef stream);
CF_EXPORT CFStreamStatus CFWriteStreamGetStatus(CFWriteStreamRef stream);
CF_EXPORT CFErrorRef CFReadStreamCopyError(CFReadStreamRef stream);
CF_EXPORT CFErrorRef CFWriteStreamCopyError(CFWriteStreamRef stream);
CF_EXPORT Boolean CFReadStreamOpen(CFReadStreamRef stream);
CF_EXPORT Boolean CFWriteStreamOpen(CFWriteStreamRef stream);
CF_EXPORT void CFReadStreamClose(CFReadStreamRef stream);
CF_EXPORT void CFWriteStreamClose(CFWriteStreamRef stream);
CF_EXPORT Boolean CFReadStreamHasBytesAvailable(CFReadStreamRef stream);
CF_EXPORT CFIndex CFReadStreamRead(CFReadStreamRef stream, UInt8 *buffer, CFIndex bufferLength);
CF_EXPORT const UInt8 *CFReadStreamGetBuffer(CFReadStreamRef stream, CFIndex maxBytesToRead, CFIndex *numBytesRead);
CF_EXPORT Boolean CFWriteStreamCanAcceptBytes(CFWriteStreamRef stream);
CF_EXPORT CFIndex CFWriteStreamWrite(CFWriteStreamRef stream, const UInt8 *buffer, CFIndex bufferLength);
CF_EXPORT CFTypeRef CFReadStreamCopyProperty(CFReadStreamRef stream, CFStreamPropertyKey propertyName);
CF_EXPORT CFTypeRef CFWriteStreamCopyProperty(CFWriteStreamRef stream, CFStreamPropertyKey propertyName);
CF_EXPORT Boolean CFReadStreamSetProperty(
    CFReadStreamRef stream,
    CFStreamPropertyKey propertyName,
    CFTypeRef propertyValue
);
CF_EXPORT Boolean CFWriteStreamSetProperty(
    CFWriteStreamRef stream,
    CFStreamPropertyKey propertyName,
    CFTypeRef propertyValue
);
CF_EXPORT Boolean CFReadStreamSetClient(
    CFReadStreamRef stream,
    CFOptionFlags streamEvents,
    CFReadStreamClientCallBack clientCB,
    CFStreamClientContext *clientContext
);
CF_EXPORT Boolean CFWriteStreamSetClient(
    CFWriteStreamRef stream,
    CFOptionFlags streamEvents,
    CFWriteStreamClientCallBack clientCB,
    CFStreamClientContext *clientContext
);
CF_EXPORT void CFReadStreamScheduleWithRunLoop(CFReadStreamRef stream, CFRunLoopRef runLoop, CFRunLoopMode runLoopMode);
CF_EXPORT void CFWriteStreamScheduleWithRunLoop(
    CFWriteStreamRef stream,
    CFRunLoopRef runLoop,
    CFRunLoopMode runLoopMode
);
CF_EXPORT void CFReadStreamUnscheduleFromRunLoop(
    CFReadStreamRef stream,
    CFRunLoopRef runLoop,
    CFRunLoopMode runLoopMode
);
CF_EXPORT void CFWriteStreamUnscheduleFromRunLoop(
    CFWriteStreamRef stream,
    CFRunLoopRef runLoop,
    CFRunLoopMode runLoopMode
);

CF_EXTERN_C_END

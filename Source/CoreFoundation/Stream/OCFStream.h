//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/RunLoop/OCFRunLoop.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/Base/OCFUtilities.h"

#include <dispatch/dispatch.h>
#include <pthread.h>
#include <stdint.h>

// Public CFStream interface, iOS Simulator SDK CFStream.h, spelled with the OCF prefix (the subset this unit
// defines or reads).
typedef struct __OCFReadStream *OCFReadStreamRef;
typedef struct __OCFWriteStream *OCFWriteStreamRef;
typedef OCFStringRef OCFStreamPropertyKey;

typedef enum : OCFIndex {
    kOCFStreamStatusNotOpen = 0,
    kOCFStreamStatusOpening = 1,
    kOCFStreamStatusOpen = 2,
    kOCFStreamStatusReading = 3,
    kOCFStreamStatusWriting = 4,
    kOCFStreamStatusAtEnd = 5,
    kOCFStreamStatusClosed = 6,
    kOCFStreamStatusError = 7,
} OCFStreamStatus;

typedef enum : OCFOptionFlags {
    kOCFStreamEventNone = 0,
    kOCFStreamEventOpenCompleted = 1,
    kOCFStreamEventHasBytesAvailable = 2,
    kOCFStreamEventCanAcceptBytes = 4,
    kOCFStreamEventErrorOccurred = 8,
    kOCFStreamEventEndEncountered = 16,
} OCFStreamEventType;

typedef struct {
    OCFIndex domain;
    SInt32 error;
} OCFStreamError;

// The stream objects, iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified
// for every offset named here (the accesses of Stream/OCFStream.c), unread for the bytes marked unread. The
// type and field names are local descriptive spellings.
//
// Stream callbacks (version 1 here; versions above 1 take a CFErrorRef slot instead of a CFStreamError).
typedef struct {
    OCFIndex version;
    void *(*create)(void *stream, void *info);
    void (*finalize)(void *stream, void *info);
    OCFStringRef (*copyDescription)(void *stream, void *info);
    Boolean (*open)(void *stream, void *error, Boolean *openComplete, void *info);
    Boolean (*openCompleted)(void *stream, void *error, void *info);
    void *read;
    void *getBuffer;
    void *canRead;
    OCFIndex (*write)(void *stream, const UInt8 *buffer, OCFIndex bufferLength, void *error, void *info);
    void *canWrite;
    void (*close)(void *stream, void *info);
    OCFTypeRef (*copyProperty)(void *stream, OCFStreamPropertyKey propertyName, void *info);
    void *setProperty;
    void *requestEvents;
    void (*schedule)(void *stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode, void *info);
    void (*unschedule)(void *stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode, void *info);
} StreamCallBacks;

_Static_assert(sizeof(StreamCallBacks) == 136, "The native stream callbacks are 136 bytes.");

// The client context (SDK CFStreamClientContext, spelled OCF) and the client callbacks (SDK CFStream.h).
typedef struct {
    OCFIndex version;
    void *info;
    void *(*retain)(void *info);
    void (*release)(void *info);
    OCFStringRef (*copyDescription)(void *info);
} OCFStreamClientContext;

typedef void (*OCFReadStreamClientCallBack)(OCFReadStreamRef stream, OCFStreamEventType type, void *info);
typedef void (*OCFWriteStreamClientCallBack)(OCFWriteStreamRef stream, OCFStreamEventType type, void *info);

// The client record (80 bytes, typed allocation 0x10A00402EE2ECAD): the client context (version, info,
// retain, release and copy-description callbacks, copied by _CFStreamSetClient), the client callback, the
// events the client wants, the run loop source, the scheduled run loops and modes (pairs) and the events to
// signal.
typedef struct {
    OCFIndex version;
    void *info;
    void *(*retain)(void *info);
    void (*release)(void *info);
    OCFStringRef (*copyDescription)(void *info);
    void (*callback)(void *stream, OCFStreamEventType type, void *info);
    OCFOptionFlags when;
    OCFRunLoopSourceRef rlSource;
    OCFMutableArrayRef runLoopsAndModes;
    OCFOptionFlags whatToSignal;
} StreamClient;

_Static_assert(sizeof(StreamClient) == 80, "The native stream client is 80 bytes.");

// CFReadStream (type 38) and CFWriteStream (type 39), 144 bytes. Flag bits 0-4 are the status, bit 5 constant
// callbacks, bit 6 a callback in progress, bit 7 closing, bit 8 a shared run loop source. At 0x78 an object
// the finalizer releases (CFRelease; its producer is unread), at 0x80 a dispatch queue that event delivery
// uses when set, at 0x88 the byte that marks a shared source's delivery in progress.
typedef struct {
    OCFRuntimeBase base;
    uint64_t flags;
    void *error;
    StreamClient *client;
    void *info;
    const StreamCallBacks *callBacks;
    pthread_mutex_t lock;
    OCFTypeRef unread0;
    dispatch_queue_t queue;
    uint8_t signalling;
    uint8_t unread1[7];
} StreamObject;

_Static_assert(sizeof(StreamObject) == 144, "The native stream object is 144 bytes.");

OF_EXPORT OCFTypeID OCFReadStreamGetTypeID(void);
OF_EXPORT OCFTypeID OCFWriteStreamGetTypeID(void);
OF_EXPORT OCFStreamStatus OCFReadStreamGetStatus(OCFReadStreamRef stream);
OF_EXPORT Boolean OCFReadStreamOpen(OCFReadStreamRef stream);
OF_EXPORT void OCFReadStreamClose(OCFReadStreamRef stream);
OF_EXPORT OCFIndex OCFReadStreamRead(OCFReadStreamRef stream, UInt8 *buffer, OCFIndex bufferLength);
OF_EXPORT OCFTypeRef OCFReadStreamCopyProperty(OCFReadStreamRef stream, OCFStreamPropertyKey propertyName);
OF_EXPORT Boolean OCFReadStreamSetProperty(
    OCFReadStreamRef stream, OCFStreamPropertyKey propertyName, OCFTypeRef propertyValue
);
OF_EXPORT OCFStreamStatus OCFWriteStreamGetStatus(OCFWriteStreamRef stream);
OF_EXPORT OCFErrorRef OCFWriteStreamCopyError(OCFWriteStreamRef stream);
OF_EXPORT OCFErrorRef OCFReadStreamCopyError(OCFReadStreamRef stream);
OF_EXPORT Boolean OCFReadStreamHasBytesAvailable(OCFReadStreamRef stream);
OF_EXPORT const UInt8 *
OCFReadStreamGetBuffer(OCFReadStreamRef stream, OCFIndex maxBytesToRead, OCFIndex *numBytesRead);
OF_EXPORT Boolean OCFWriteStreamCanAcceptBytes(OCFWriteStreamRef stream);
OF_EXPORT Boolean OCFWriteStreamSetProperty(
    OCFWriteStreamRef stream, OCFStreamPropertyKey propertyName, OCFTypeRef propertyValue
);
OF_EXPORT Boolean OCFReadStreamSetClient(
    OCFReadStreamRef stream,
    OCFOptionFlags streamEvents,
    OCFReadStreamClientCallBack clientCB,
    OCFStreamClientContext *clientContext
);
OF_EXPORT Boolean OCFWriteStreamSetClient(
    OCFWriteStreamRef stream,
    OCFOptionFlags streamEvents,
    OCFWriteStreamClientCallBack clientCB,
    OCFStreamClientContext *clientContext
);
OF_EXPORT void
OCFReadStreamScheduleWithRunLoop(OCFReadStreamRef stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode);
OF_EXPORT void OCFWriteStreamScheduleWithRunLoop(
    OCFWriteStreamRef stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode
);
OF_EXPORT void OCFReadStreamUnscheduleFromRunLoop(
    OCFReadStreamRef stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode
);
OF_EXPORT void OCFWriteStreamUnscheduleFromRunLoop(
    OCFWriteStreamRef stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode
);
OF_EXPORT Boolean OCFWriteStreamOpen(OCFWriteStreamRef stream);
OF_EXPORT void OCFWriteStreamClose(OCFWriteStreamRef stream);
OF_EXPORT OCFIndex OCFWriteStreamWrite(OCFWriteStreamRef stream, const UInt8 *buffer, OCFIndex bufferLength);
// kCFStreamPropertyDataWritten (0x2227a4f60, external); the property-list writer also loads it under its
// Foundation name, NSStreamDataWrittenToMemoryStreamKey.
OF_EXPORT const OCFStreamPropertyKey kOCFStreamPropertyDataWritten;
OF_EXPORT OCFTypeRef OCFWriteStreamCopyProperty(OCFWriteStreamRef stream, OCFStreamPropertyKey propertyName);
OF_EXPORT void OCFWriteStreamSignalEvent(
    OCFWriteStreamRef stream, OCFStreamEventType streamEvent, const OCFStreamError *error
);
// kCFStreamPropertyAppendToFile (0x2227a4f68, external), beside kCFStreamPropertyDataWritten.
OF_EXPORT const OCFStreamPropertyKey kOCFStreamPropertyAppendToFile;
// The stream SPI of CFStreamAbstract.h (external natively; signatures from the native register use).
OF_EXPORT void OCFReadStreamSignalEvent(
    OCFReadStreamRef stream, OCFStreamEventType streamEvent, const OCFStreamError *error
);
OF_EXPORT void *OCFReadStreamGetInfoPointer(OCFReadStreamRef stream);
OF_EXPORT void *OCFWriteStreamGetInfoPointer(OCFWriteStreamRef stream);
// The client accessors (external natively; no SDK declaration): the client context's info.
OF_EXPORT void *_OCFReadStreamGetClient(OCFReadStreamRef stream);
OF_EXPORT void *_OCFWriteStreamGetClient(OCFWriteStreamRef stream);

// Hidden (were private externals).
OF_EXTERN const OCFRuntimeClass __CFReadStreamClass;
OF_EXTERN const OCFRuntimeClass __CFWriteStreamClass;
OF_EXTERN OCFErrorRef
_CFStreamCreateErrorFromStreamError(OCFAllocatorRef alloc, const OCFStreamError *streamError);
OF_EXTERN void _CFStreamClose(StreamObject *stream);
OF_EXTERN OCFStreamStatus _CFStreamGetStatus(StreamObject *stream);
OF_EXTERN StreamObject *_CFStreamCreateWithConstantCallbacks(
    OCFAllocatorRef alloc, void *info, const StreamCallBacks *cb, Boolean isReading
);
OF_EXTERN void _CFStreamSignalEvent(
    StreamObject *stream, OCFStreamEventType event, const void *error, Boolean synchronousAllowed
);
OF_EXTERN Boolean _CFStreamOpen(StreamObject *stream);
OF_EXTERN void
_CFStreamScheduleWithRunLoop(StreamObject *stream, OCFRunLoopRef runLoop, OCFStringRef runLoopMode);
OF_EXTERN void
_CFStreamUnscheduleFromRunLoop(StreamObject *stream, OCFRunLoopRef runLoop, OCFStringRef runLoopMode);
OF_EXTERN Boolean _CFStreamSetClient(
    StreamObject *stream,
    OCFOptionFlags streamEvents,
    void (*clientCB)(void *stream, OCFStreamEventType type, void *info),
    OCFStreamClientContext *clientContext
);

// The delegate class of this image that CFReadStreamSetClient and CFWriteStreamSetClient create for bridged
// streams (native _CFStreamDelegate, not restored on this branch).
#ifdef __OBJC__
@class _OCFStreamDelegate;
#endif

// The CFNetwork support table of the stream unit (native CFNetworkSupport, 0x22e6cf898, __DATA,__data,
// non-external, 96 bytes): a statically initialized mutex, a 32-bit flags word at 64 (bit 0 lookup started,
// bit 1 lookup finished) and the three CFNetwork functions initializeCFNetworkSupport looks up: the socket
// stream pair creator (72), the stream-error conversion (80) and its inverse (88). Only the conversion at 80
// is called by the restored code; the other two slots keep their looked-up addresses untyped (their callers
// are unread).
typedef struct {
    pthread_mutex_t lock;
    uint32_t flags;
    uint32_t unread0;
    void *createSocketStreamPair;
    OCFErrorRef (*createErrorFromStreamError)(OCFAllocatorRef alloc, const OCFStreamError *streamError);
    void *streamErrorFromCFError;
} StreamNetworkSupport;

_Static_assert(sizeof(StreamNetworkSupport) == 96, "The native CFNetwork support table is 96 bytes.");

OF_EXTERN StreamNetworkSupport CFNetworkSupport;

// Interfaces of families not yet restored on this branch
// Native __CFLookupCFNetworkFunction, 0x1c0256cbc (was a private external): the named CFNetwork function, or
// NULL. Signature from initializeCFNetworkSupport's register use (the name in X0, the address in X0).

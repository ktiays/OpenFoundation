//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Stream/OCFStream.h"

// The URL parameter is spelled with its structure tag (OCFURLRef, URL/OCFURL.h), as Collections/OCFData.h
// does, to keep the URL header's include chain out of the stream headers.
struct __OCFURL;

OF_EXPORT OCFReadStreamRef OCFReadStreamCreateWithFile(OCFAllocatorRef alloc, const struct __OCFURL *fileURL);
OF_EXPORT OCFWriteStreamRef
OCFWriteStreamCreateWithAllocatedBuffers(OCFAllocatorRef alloc, OCFAllocatorRef bufferAllocator);
OF_EXPORT OCFWriteStreamRef
OCFWriteStreamCreateWithFile(OCFAllocatorRef alloc, const struct __OCFURL *fileURL);
// The data parameter is spelled with its structure tag (OCFDataRef, Collections/OCFData.h), as fileURL is.
struct __OCFData;
OF_EXPORT OCFReadStreamRef OCFReadStreamCreateWithData(OCFAllocatorRef alloc, const struct __OCFData *data);
OF_EXPORT OCFWriteStreamRef
OCFWriteStreamCreateWithBuffer(OCFAllocatorRef alloc, UInt8 *buffer, OCFIndex bufferCapacity);

// The descriptor helpers of this unit (hidden; were private externals natively). Signatures from the native
// register use.
OF_EXTERN OCFIndex
fdRead(int fd, void *buffer, OCFIndex bufferLength, OCFStreamError *errorCode, Boolean *atEOF);
OF_EXTERN Boolean fdCanRead(int fd);
OF_EXTERN OCFIndex fdWrite(int fd, const UInt8 *buffer, OCFIndex bufferLength, OCFStreamError *errorCode);
OF_EXTERN Boolean fdCanWrite(int fd);

// Interfaces of families not yet restored on this branch
// The read data stream callbacks of this unit (non-external natively: readDataCreate 0x1c017cf0c,
// readDataFinalize 0x1c017cf6c, readDataCopyDescription 0x1c017cfa0, readDataOpen 0x1c017cfa8, dataRead
// 0x1c017d010, dataGetBuffer 0x1c017d0d8, dataCanRead 0x1c017d18c, readDataSchedule 0x1c017d1cc), which
// readDataCallBacks references. They are not in the evidence; hidden and left undefined until an IDA capture.
// Signatures from the StreamCallBacks slots (read, get-buffer and can-read from CFReadStreamRead,
// CFReadStreamGetBuffer and CFReadStreamHasBytesAvailable).
OF_EXTERN void *readDataCreate(void *stream, void *info);
OF_EXTERN void readDataFinalize(void *stream, void *info);
OF_EXTERN OCFStringRef readDataCopyDescription(void *stream, void *info);
OF_EXTERN Boolean readDataOpen(void *stream, void *error, Boolean *openComplete, void *info);
OF_EXTERN OCFIndex
dataRead(void *stream, UInt8 *buffer, OCFIndex bufferLength, void *error, Boolean *atEOF, void *info);
OF_EXTERN const UInt8 *dataGetBuffer(
    void *stream, OCFIndex maxBytesToRead, OCFIndex *numBytesRead, void *error, Boolean *atEOF, void *info
);
OF_EXTERN Boolean dataCanRead(void *stream, void *info);
OF_EXTERN void readDataSchedule(void *stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode, void *info);

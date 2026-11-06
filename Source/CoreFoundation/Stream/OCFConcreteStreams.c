//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Stream/OCFConcreteStreams.h"

#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/RunLoop/OCFFileDescriptor.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/URL/OCFURL.h"

#include <errno.h>
#include <fcntl.h>
#include <malloc/malloc.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>

// Inferred organization: the descriptor helpers fdRead (0x1c017bd38), fdCanRead (0x1c017bd8c), fdWrite
// (0x1c017bea8) and fdCanWrite (0x1c017beec), CFReadStreamCreateWithFile (0x1c017c008),
// _CFStreamCreateWithFile (0x1c017c010), CFWriteStreamCreateWithFile (0x1c017c0e0),
// CFReadStreamCreateWithData (0x1c017c23c), CFWriteStreamCreateWithBuffer (0x1c017c2c4),
// CFWriteStreamCreateWithAllocatedBuffers (0x1c017c33c), the file stream callbacks fileCreate (0x1c017c3a0)
// through fileUnschedule (0x1c017cb2c) with constructCFFD (0x1c017ccc0), _fs_retain (0x1c017ce24),
// _fs_release (0x1c017ce50) and fileCallBack (0x1c017ce5c), and the data stream callbacks (writeDataCreate
// 0x1c017d254 through writeDataSchedule 0x1c017d70c) lie in the concrete streams cluster of the iOS Simulator
// __text section, with fileCallBacks (0x22279d410) and readDataCallBacks (0x22279d498) and writeDataCallBacks
// (0x22279d520,
// __DATA_CONST,__const); they are kept in this unit in native order (the tables first). Not restored (not in
// the evidence): _CFReadStreamCreateFromFileDescriptor (0x1c017c0e8) and
// _CFWriteStreamCreateFromFileDescriptor (0x1c017c150). The native source path is not in the evidence. iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (disassembly and the
// tables' 17 words each); iOS Device 24A446: unread. Reading: Local/Scratch/cf-core/tasks/runloop-streams.txt
// (sub-track reading; no versioned reading yet); the file stream entry points:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta4.json; the descriptor helpers and file callbacks:
// iphone-simulator-corpus-delta5.json; the write file, read data and write buffer creators and
// readDataCallBacks: iphone-simulator-corpus-delta7.json.

// The native unit calls select and pselect through their unlimited-descriptor variants
// (select$DARWIN_EXTSN, pselect$DARWIN_EXTSN), which the SDK selects only under _DARWIN_C_SOURCE or
// _DARWIN_UNLIMITED_SELECT; they are bound here by symbol name.
extern int selectUnlimited(
    int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds, struct timeval *timeout
) __asm("_select$DARWIN_EXTSN");

// The file stream create callback's context (40 bytes on the caller's stack, zeroed): the URL, the file
// descriptor (-1), and three words that are zero here and unread. Local descriptive spellings.
typedef struct {
    OCFURLRef url;
    int32_t fd;
    uint32_t unread0;
    void *unread1[3];
} FileStreamContext;

_Static_assert(sizeof(FileStreamContext) == 40, "The file stream context is 40 bytes.");

// The file stream's info (40 bytes, typed allocation 0x10600401BE82CDF): the URL (retained), the descriptor,
// the run loops and modes scheduled before the open (pairs in a CFArray) or, once opened, the
// CFFileDescriptor, the flags and the offset (-1 for none). Flag bit 0 marks a write callback to re-enable,
// bit 1 a read callback to re-enable, bit 3 appending, bit 5 a failed open. Local descriptive spellings.
typedef struct {
    OCFURLRef url;
    int32_t fd;
    uint32_t unread0;

    union {
        OCFMutableArrayRef runLoopsAndModes;
        OCFFileDescriptorRef cffd;
    } scheduling;

    uint64_t flags;
    off_t offset;
} FileStreamInfo;

_Static_assert(sizeof(FileStreamInfo) == 40, "The file stream info is 40 bytes.");

// The read data stream's create context (24 bytes on the caller's stack, zeroed): the data copy, then two
// words that are zero here and read only by readDataCreate (not restored; unread). Local descriptive
// spellings.
typedef struct {
    OCFDataRef data;
    void *unread0;
    void *unread1;
} ReadDataContext;

// The create callback's context (32 bytes on the caller's stack): word 0 points at a fixed buffer and its
// capacity (read only with kCFAllocatorNull as the buffer allocator), word 2 is the buffer allocator; the
// other words are unread. The type and field names are local descriptive spellings.
typedef struct {
    UInt8 *bytes;
    OCFIndex capacity;
} WriteDataFixedBuffer;

typedef struct {
    const WriteDataFixedBuffer *fixedBuffer;
    void *unread0;
    OCFAllocatorRef bufferAllocator;
    void *unread1;
} WriteDataContext;

// The stream's info (32 bytes, followed by its first buffer block): the first and current blocks, the buffer
// allocator (retained unless kCFAllocatorNull) and the scheduled byte. A block (32 bytes) holds its bytes,
// its capacity, the bytes written and the next block. Local descriptive spellings.
typedef struct WriteDataBlock {
    UInt8 *buffer;
    OCFIndex capacity;
    OCFIndex length;
    struct WriteDataBlock *next;
} WriteDataBlock;

typedef struct {
    WriteDataBlock *firstBuf;
    WriteDataBlock *currentBuf;
    OCFAllocatorRef bufferAllocator;
    Boolean scheduled;
} WriteDataStreamContext;

static StreamObject *_CFStreamCreateWithFile(OCFAllocatorRef alloc, OCFURLRef fileURL, Boolean forReading);
static void *fileCreate(void *stream, void *info);
static void fileFinalize(void *stream, void *info);
static OCFStringRef fileCopyDescription(void *stream, void *info);
static Boolean fileOpen(void *stream, void *error, Boolean *openComplete, void *info);
static OCFIndex
fileRead(void *stream, UInt8 *buffer, OCFIndex bufferLength, void *error, Boolean *atEOF, void *info);
static Boolean fileCanRead(void *stream, void *info);
static OCFIndex fileWrite(void *stream, const UInt8 *buffer, OCFIndex bufferLength, void *error, void *info);
static Boolean fileCanWrite(void *stream, void *info);
static void fileClose(void *stream, void *info);
static OCFTypeRef fileCopyProperty(void *stream, OCFStreamPropertyKey propertyName, void *info);
static Boolean
fileSetProperty(void *stream, OCFStreamPropertyKey propertyName, OCFTypeRef propertyValue, void *info);
static void fileSchedule(void *stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode, void *info);
static void fileUnschedule(void *stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode, void *info);
static Boolean constructCFFD(FileStreamInfo *fileStream, Boolean forRead, void *stream);
static void *_fs_retain(void *info);
static void _fs_release(void *info);
static void fileCallBack(OCFFileDescriptorRef f, OCFOptionFlags callBackTypes, void *info);
static void *writeDataCreate(void *stream, void *info);
static void writeDataFinalize(void *stream, void *info);
static OCFStringRef writeDataCopyDescription(void *stream, void *info);
static Boolean writeDataOpen(void *stream, void *error, Boolean *openComplete, void *info);
static OCFIndex dataWrite(void *stream, const UInt8 *buffer, OCFIndex bufferLength, void *error, void *info);
static Boolean dataCanWrite(void *stream, void *info);
static OCFTypeRef dataCopyProperty(void *stream, OCFStreamPropertyKey propertyName, void *info);
static void writeDataSchedule(void *stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode, void *info);

// Native fileCallBacks, 0x22279d410 (__DATA_CONST,__const, 136 bytes). Non-external; version 1, create,
// finalize, description, open, read, can-read, write, can-write, close, copy and set property, schedule and
// unschedule; openCompleted, getBuffer and requestEvents NULL.
static const StreamCallBacks fileCallBacks = {
    1,
    fileCreate,
    fileFinalize,
    fileCopyDescription,
    fileOpen,
    NULL,
    (void *)fileRead,
    NULL,
    (void *)fileCanRead,
    fileWrite,
    (void *)fileCanWrite,
    fileClose,
    fileCopyProperty,
    (void *)fileSetProperty,
    NULL,
    fileSchedule,
    fileUnschedule,
};

// Native readDataCallBacks, 0x22279d498 (__DATA_CONST,__const, 136 bytes; corpus delta capture 7 "data", the
// words read with Local/Scratch/cf-core/macho.py). Non-external; version 1, create, finalize, description,
// open, read, get-buffer, can-read and schedule. Its eight callbacks (readDataCreate 0x1c017cf0c through
// readDataSchedule 0x1c017d1cc) are defined below in native order (corpus delta capture 8) and declared in
// Stream/OCFConcreteStreams.h.
static const StreamCallBacks readDataCallBacks = {
    1,
    readDataCreate,
    readDataFinalize,
    readDataCopyDescription,
    readDataOpen,
    NULL,
    (void *)dataRead,
    (void *)dataGetBuffer,
    (void *)dataCanRead,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    readDataSchedule,
    NULL,
};

// Native writeDataCallBacks, 0x22279d520 (__DATA_CONST,__const, 136 bytes). Non-external; version 1, create,
// finalize, description, open, write, can-write, copy property and schedule.
static const StreamCallBacks writeDataCallBacks = {
    1,
    writeDataCreate,
    writeDataFinalize,
    writeDataCopyDescription,
    writeDataOpen,
    NULL,
    NULL,
    NULL,
    NULL,
    dataWrite,
    (void *)dataCanWrite,
    NULL,
    dataCopyProperty,
    NULL,
    NULL,
    writeDataSchedule,
    NULL,
};

// Native fdRead, 0x1c017bd38. Hidden (was a private external). read(2); a negative result stores the POSIX
// domain (1) and errno and gives -1, otherwise the error code is cleared and end-of-file is a zero count.
OCFIndex fdRead(int fd, void *buffer, OCFIndex bufferLength, OCFStreamError *errorCode, Boolean *atEOF) {
    ssize_t bytesRead = read(fd, buffer, (size_t)bufferLength);
    if (bytesRead < 0) {
        errorCode->domain = 1;
        errorCode->error = errno;
        return -1;
    }
    *atEOF = bytesRead == 0;
    errorCode->error = 0;
    return bytesRead;
}

// Native fdCanRead, 0x1c017bd8c. Hidden (was a private external). A zero-timeout select(2) (the
// unlimited-descriptor variant) on the descriptor alone answers whether it returns 1. Descriptors below 1024
// use a stack fd_set; larger ones a zeroed heap set of ((fd + 32) >> 3) & 0x1FFFFFFC bytes (32-bit
// arithmetic, type 0x100004052888210), freed afterwards. The stack set is zeroed in either case; the
// descriptor's bit is set (byte offset (fd >> 3) & ~3 of the sign-extended descriptor, bit fd mod 32) only
// when
// __darwin_check_fd_set_overflow allows it.
Boolean fdCanRead(int fd) {
    struct timeval timeout = {0, 0};
    fd_set stackSet;
    fd_set *readSet;
    if (fd < 1024) {
        readSet = &stackSet;
    } else {
        size_t size = ((uint32_t)fd + 32) >> 3 & 0x1FFFFFFC;
        readSet = malloc_type_malloc(size, 0x100004052888210);
        bzero(readSet, size);
    }
    memset(&stackSet, 0, sizeof(stackSet));
    if (__darwin_check_fd_set_overflow(fd, readSet, 1)) {
        uint32_t *word =
            (uint32_t *)((uint8_t *)readSet + (((uint64_t)(int64_t)fd >> 3) & 0x1FFFFFFFFFFFFFFC));
        *word |= 1u << (fd & 31);
    }
    int result = selectUnlimited(fd + 1, readSet, NULL, NULL, &timeout);
    if (readSet != &stackSet) {
        free(readSet);
    }
    return result == 1;
}

// Native fdWrite, 0x1c017bea8. Hidden (was a private external). write(2); a negative result stores the POSIX
// domain (1) and errno and gives -1, otherwise the error code is cleared.
OCFIndex fdWrite(int fd, const UInt8 *buffer, OCFIndex bufferLength, OCFStreamError *errorCode) {
    ssize_t bytesWritten = write(fd, buffer, (size_t)bufferLength);
    if (bytesWritten < 0) {
        errorCode->domain = 1;
        errorCode->error = errno;
        return -1;
    }
    errorCode->error = 0;
    return bytesWritten;
}

// Native fdCanWrite, 0x1c017beec. Hidden (was a private external). As fdCanRead with the descriptor in the
// write set.
Boolean fdCanWrite(int fd) {
    struct timeval timeout = {0, 0};
    fd_set stackSet;
    fd_set *writeSet;
    if (fd < 1024) {
        writeSet = &stackSet;
    } else {
        size_t size = ((uint32_t)fd + 32) >> 3 & 0x1FFFFFFC;
        writeSet = malloc_type_malloc(size, 0x100004052888210);
        bzero(writeSet, size);
    }
    memset(&stackSet, 0, sizeof(stackSet));
    if (__darwin_check_fd_set_overflow(fd, writeSet, 1)) {
        uint32_t *word =
            (uint32_t *)((uint8_t *)writeSet + (((uint64_t)(int64_t)fd >> 3) & 0x1FFFFFFFFFFFFFFC));
        *word |= 1u << (fd & 31);
    }
    int result = selectUnlimited(fd + 1, NULL, writeSet, NULL, &timeout);
    if (writeSet != &stackSet) {
        free(writeSet);
    }
    return result == 1;
}

// Native CFReadStreamCreateWithFile, 0x1c017c008. External; a reading file stream (a tail call).
OCFReadStreamRef OCFReadStreamCreateWithFile(OCFAllocatorRef alloc, OCFURLRef fileURL) {
    return (OCFReadStreamRef)_CFStreamCreateWithFile(alloc, fileURL, true);
}

// Native _CFStreamCreateWithFile, 0x1c017c010. Non-external. Without a URL, or without a scheme, or
// with a scheme other than "file", NULL (the scheme is released). Otherwise the scheme is released and a
// stream is created with the file callbacks over a zeroed context holding the URL and descriptor -1.
static StreamObject *_CFStreamCreateWithFile(OCFAllocatorRef alloc, OCFURLRef fileURL, Boolean forReading) {
    if (fileURL == NULL) {
        return NULL;
    }
    OCFStringRef scheme = OCFURLCopyScheme(fileURL);
    if (scheme == NULL) {
        return NULL;
    }
    if (!OCFEqual(scheme, OF_STR("file"))) {
        OCFRelease(scheme);
        return NULL;
    }
    FileStreamContext fileContext;
    memset(&fileContext, 0, sizeof(fileContext));
    OCFRelease(scheme);
    fileContext.url = fileURL;
    fileContext.fd = -1;
    return _CFStreamCreateWithConstantCallbacks(alloc, &fileContext, &fileCallBacks, forReading);
}

// Native CFWriteStreamCreateWithFile, 0x1c017c0e0 (corpus delta capture 7). External; a writing file stream
// (a tail call).
OCFWriteStreamRef OCFWriteStreamCreateWithFile(OCFAllocatorRef alloc, OCFURLRef fileURL) {
    return (OCFWriteStreamRef)_CFStreamCreateWithFile(alloc, fileURL, false);
}

// Native CFReadStreamCreateWithData, 0x1c017c23c (corpus delta capture 7). External. A read stream (isReading
// true) with the data callbacks over a zeroed 24-byte context holding a copy of the data (CFDataCreateCopy
// with the allocator), which is released after the creation.
OCFReadStreamRef OCFReadStreamCreateWithData(OCFAllocatorRef alloc, OCFDataRef data) {
    ReadDataContext ctxt = {NULL, NULL, NULL};
    OCFDataRef copy = OCFDataCreateCopy(alloc, data);
    ctxt.data = copy;
    StreamObject *stream = _CFStreamCreateWithConstantCallbacks(alloc, &ctxt, &readDataCallBacks, true);
    OCFRelease(copy);
    return (OCFReadStreamRef)stream;
}

// Native CFWriteStreamCreateWithBuffer, 0x1c017c2c4 (corpus delta capture 7). External. A write stream
// (isReading false) with the data callbacks over a context whose first two words point at a stack block
// holding the buffer, its capacity and two zero words, with kCFAllocatorNull as the buffer allocator.
OCFWriteStreamRef
OCFWriteStreamCreateWithBuffer(OCFAllocatorRef alloc, UInt8 *buffer, OCFIndex bufferCapacity) {
    WriteDataBlock block = {buffer, bufferCapacity, 0, NULL};
    WriteDataContext ctxt = {(const WriteDataFixedBuffer *)&block, &block, kOCFAllocatorNull, NULL};
    return (OCFWriteStreamRef)_CFStreamCreateWithConstantCallbacks(alloc, &ctxt, &writeDataCallBacks, false);
}

// Native CFWriteStreamCreateWithAllocatedBuffers, 0x1c017c33c. External; a write stream (isReading false)
// with the data callbacks.
OCFWriteStreamRef
OCFWriteStreamCreateWithAllocatedBuffers(OCFAllocatorRef alloc, OCFAllocatorRef bufferAllocator) {
    WriteDataContext ctxt = {NULL, NULL, bufferAllocator, NULL};
    return (OCFWriteStreamRef)_CFStreamCreateWithConstantCallbacks(alloc, &ctxt, &writeDataCallBacks, false);
}

// Native fileCreate, 0x1c017c3a0. Non-external. The info (40 bytes from the stream's allocator, type
// 0x10600401BE82CDF) takes the context's URL (retained when not NULL) and descriptor, no scheduling, no flags
// and offset -1; a failed allocation gives NULL.
static void *fileCreate(void *stream, void *info) {
    FileStreamContext *ctxt = info;
    FileStreamInfo *fileStream =
        OCFAllocatorAllocateTyped(OCFGetAllocator(stream), 0x28, 0x10600401BE82CDF, 0);
    if (fileStream == NULL) {
        return NULL;
    }
    fileStream->url = ctxt->url;
    if (fileStream->url != NULL) {
        OCFRetain(fileStream->url);
    }
    fileStream->fd = ctxt->fd;
    fileStream->scheduling.runLoopsAndModes = NULL;
    fileStream->flags = 0;
    fileStream->offset = -1;
    return fileStream;
}

// Native fileFinalize, 0x1c017c40c. Non-external. With a descriptor above 0, the CFFileDescriptor (when set)
// is invalidated and released and the descriptor closed; otherwise the scheduling object (when set) is
// released. The URL (when set) is released and the info freed with the stream's allocator (a tail call).
static void fileFinalize(void *stream, void *info) {
    FileStreamInfo *fileStream = info;
    if (fileStream->fd >= 1) {
        if (fileStream->scheduling.cffd != NULL) {
            OCFFileDescriptorInvalidate(fileStream->scheduling.cffd);
            OCFRelease(fileStream->scheduling.cffd);
            fileStream->scheduling.cffd = NULL;
        }
        close(fileStream->fd);
    } else if (fileStream->scheduling.runLoopsAndModes != NULL) {
        OCFRelease(fileStream->scheduling.runLoopsAndModes);
    }
    if (fileStream->url != NULL) {
        OCFRelease(fileStream->url);
    }
    OCFAllocatorDeallocate(OCFGetAllocator(stream), fileStream);
}

// Native fileCopyDescription, 0x1c017c484. Non-external. The URL's description (a tail call), or "fd = %d"
// with the descriptor in the stream's allocator.
static OCFStringRef fileCopyDescription(void *stream, void *info) {
    FileStreamInfo *fileStream = info;
    if (fileStream->url != NULL) {
        return OCFCopyDescription(fileStream->url);
    }
    return OCFStringCreateWithFormat(OCFGetAllocator(stream), NULL, OF_STR("fd = %d"), fileStream->fd);
}

// Native fileOpen, 0x1c017c4d4. Non-external. The open completes at once. Without a URL, a stream scheduled
// before the open gets its CFFileDescriptor (the result is ignored) and the open succeeds. With a URL, its
// file system representation (resolved against the base, 1026 bytes) is opened with O_RDONLY for a read
// stream, otherwise O_WRONLY | O_CREAT | O_TRUNC, or O_WRONLY | O_CREAT | O_APPEND when appending (flag bit
// 3), mode 0666; a set offset is sought (SEEK_SET); a scheduled stream gets its CFFileDescriptor. A failed
// open, seek or CFFileDescriptor marks the open failed (flag bit 5) and stores the POSIX domain and errno; an
// unrepresentable URL stores POSIX error 2.
static Boolean fileOpen(void *stream, void *error, Boolean *openComplete, void *info) {
    FileStreamInfo *fileStream = info;
    OCFStreamError *errorCode = error;
    OCFTypeID typeID = OCFGetTypeID(stream);
    OCFTypeID readTypeID = OCFReadStreamGetTypeID();
    Boolean forRead = typeID == readTypeID;
    *openComplete = true;
    int32_t errorNumber;
    if (fileStream->url == NULL) {
        if (fileStream->scheduling.runLoopsAndModes != NULL) {
            constructCFFD(fileStream, forRead, stream);
        }
        return true;
    }
    char path[1028];
    if (!OCFURLGetFileSystemRepresentation(fileStream->url, true, (UInt8 *)path, 0x402)) {
        errorNumber = 2;
    } else {
        int flags = typeID == readTypeID ? 0 : 0x601;
        if ((fileStream->flags & 0x8) != 0) {
            flags = (flags & 0x3FF) | 0x8;
        }
        fileStream->fd = open(path, flags, 0666);
        if (fileStream->fd >= 0) {
            if (fileStream->offset == -1 || lseek(fileStream->fd, fileStream->offset, SEEK_SET) != -1) {
                if (fileStream->scheduling.runLoopsAndModes == NULL) {
                    return true;
                }
                if (constructCFFD(fileStream, forRead, stream)) {
                    return true;
                }
            }
        }
        fileStream->flags |= 0x20;
        errorNumber = errno;
    }
    errorCode->error = errorNumber;
    errorCode->domain = 1;
    return false;
}

// Native fileRead, 0x1c017c640. Non-external. fdRead on the descriptor. After a read callback (flag bit 1,
// then cleared): when not at the end, a regular file (the native test is the signed 16-bit mode below
// -0x7000, which is S_ISREG) whose current offset equals its size gets a fake read callback on its
// CFFileDescriptor; read callbacks are then enabled again on the CFFileDescriptor (when set).
static OCFIndex
fileRead(void *stream, UInt8 *buffer, OCFIndex bufferLength, void *error, Boolean *atEOF, void *info) {
    (void)stream;
    FileStreamInfo *fileStream = info;
    OCFIndex result = fdRead(fileStream->fd, buffer, bufferLength, error, atEOF);
    if ((fileStream->flags & 0x2) == 0) {
        return result;
    }
    fileStream->flags &= ~(uint64_t)0x2;
    if (!*atEOF && fileStream->scheduling.cffd != NULL) {
        struct stat statBuffer;
        memset(&statBuffer, 0, sizeof(statBuffer));
        if (fstat(fileStream->fd, &statBuffer) >= 0 && (int16_t)statBuffer.st_mode < -0x7000) {
            off_t offset = lseek(fileStream->fd, 0, SEEK_CUR);
            if (statBuffer.st_size == offset) {
                _CFFileDescriptorInduceFakeReadCallBack(fileStream->scheduling.cffd);
            }
        }
    }
    if (fileStream->scheduling.cffd != NULL) {
        OCFFileDescriptorEnableCallBacks(fileStream->scheduling.cffd, kOCFFileDescriptorReadCallBack);
    }
    return result;
}

// Native fileCanRead, 0x1c017c738. Non-external. fdCanRead on the descriptor (a tail call).
static Boolean fileCanRead(void *stream, void *info) {
    (void)stream;
    return fdCanRead(((FileStreamInfo *)info)->fd);
}

// Native fileWrite, 0x1c017c740. Non-external. fdWrite on the descriptor; after a write callback (flag bit 0,
// then cleared) write callbacks are enabled again on the CFFileDescriptor (when set).
static OCFIndex fileWrite(void *stream, const UInt8 *buffer, OCFIndex bufferLength, void *error, void *info) {
    (void)stream;
    FileStreamInfo *fileStream = info;
    OCFIndex result = fdWrite(fileStream->fd, buffer, bufferLength, error);
    if ((fileStream->flags & 0x1) != 0) {
        fileStream->flags &= ~(uint64_t)0x1;
        if (fileStream->scheduling.cffd != NULL) {
            OCFFileDescriptorEnableCallBacks(fileStream->scheduling.cffd, kOCFFileDescriptorWriteCallBack);
        }
    }
    return result;
}

// Native fileCanWrite, 0x1c017c78c. Non-external. fdCanWrite on the descriptor (a tail call).
static Boolean fileCanWrite(void *stream, void *info) {
    (void)stream;
    return fdCanWrite(((FileStreamInfo *)info)->fd);
}

// Native fileClose, 0x1c017c794. Non-external. An open descriptor is closed and forgotten (-1) and the
// CFFileDescriptor (when set) invalidated; the scheduling object (when set) is then released and cleared.
static void fileClose(void *stream, void *info) {
    (void)stream;
    FileStreamInfo *fileStream = info;
    if (fileStream->fd >= 0) {
        close(fileStream->fd);
        fileStream->fd = -1;
        if (fileStream->scheduling.cffd == NULL) {
            return;
        }
        OCFFileDescriptorInvalidate(fileStream->scheduling.cffd);
    } else if (fileStream->scheduling.cffd == NULL) {
        return;
    }
    OCFRelease(fileStream->scheduling.cffd);
    fileStream->scheduling.cffd = NULL;
}

// Native fileCopyProperty, 0x1c017c7e8. Non-external. The names are the __cfstring literals
// "kCFStreamPropertyFileCurrentOffset" (0x2227ae120) and "_kCFStreamPropertyFileNativeHandle" (0x2227ae140,
// cfstr_Kcfstreamprope_0 in the capture; its binding by the section's order is medium confidence), compared
// with CFEqual. The offset: unless appending (flag bit 3), an open descriptor's current offset is stored
// first; a set offset is returned as a CFNumber (kCFNumberSInt64Type) in the stream's allocator (a tail
// call). The native handle: an open descriptor as 4 bytes of CFData in the stream's allocator. Otherwise
// NULL.
static OCFTypeRef fileCopyProperty(void *stream, OCFStreamPropertyKey propertyName, void *info) {
    FileStreamInfo *fileStream = info;
    if (OCFEqual(propertyName, OF_STR("kCFStreamPropertyFileCurrentOffset"))) {
        if ((fileStream->flags & 0x8) == 0 && fileStream->fd != -1) {
            fileStream->offset = lseek(fileStream->fd, 0, SEEK_CUR);
        }
        if (fileStream->offset == -1) {
            return NULL;
        }
        return OCFNumberCreate(OCFGetAllocator(stream), kOCFNumberSInt64Type, &fileStream->offset);
    }
    if (OCFEqual(propertyName, OF_STR("_kCFStreamPropertyFileNativeHandle"))) {
        int32_t fd = fileStream->fd;
        if (fd == -1) {
            return NULL;
        }
        return OCFDataCreate(OCFGetAllocator(stream), (const UInt8 *)&fd, 4);
    }
    return NULL;
}

// Native fileSetProperty, 0x1c017c908. Non-external. kCFStreamPropertyAppendToFile on an unopened write
// stream sets appending (flag bit 3, the offset forgotten) for kCFBooleanTrue (pointer equality) and clears
// it for anything else; true. The literal "kCFStreamPropertyFileCurrentOffset" (CFEqual) stores a CFNumber
// offset (kCFNumberSInt64Type; the result is the conversion's) unless appending (false); an open descriptor
// then seeks to the stored offset (SEEK_SET), a failed seek giving false. Anything else: false.
static Boolean
fileSetProperty(void *stream, OCFStreamPropertyKey propertyName, OCFTypeRef propertyValue, void *info) {
    FileStreamInfo *fileStream = info;
    if (OCFEqual(propertyName, kOCFStreamPropertyAppendToFile) &&
        OCFGetTypeID(stream) == OCFWriteStreamGetTypeID() &&
        OCFWriteStreamGetStatus(stream) == kOCFStreamStatusNotOpen) {
        if (propertyValue == kOCFBooleanTrue) {
            fileStream->flags |= 0x8;
            fileStream->offset = -1;
        } else {
            fileStream->flags &= ~(uint64_t)0x8;
        }
        return true;
    }
    if (!OCFEqual(propertyName, OF_STR("kCFStreamPropertyFileCurrentOffset"))) {
        return false;
    }
    Boolean result = false;
    if ((fileStream->flags & 0x8) == 0) {
        result = OCFNumberGetValue(propertyValue, kOCFNumberSInt64Type, &fileStream->offset);
    }
    if (fileStream->fd != -1) {
        if (lseek(fileStream->fd, fileStream->offset, SEEK_SET) == -1) {
            result = false;
        }
    }
    return result;
}

// Native fileSchedule, 0x1c017ca18. Non-external. The status comes from CFReadStreamGetStatus or
// CFWriteStreamGetStatus by the stream's type. A stream without a descriptor that is past opening does
// nothing. An unopened stream records the run loop and mode in its array (created on first use in the
// stream's allocator with kCFTypeArrayCallBacks). An open stream gets its CFFileDescriptor first if it has
// none (the result ignored), then a run loop source (order 0, the stream's allocator) added to the run loop
// in the mode and released.
static void fileSchedule(void *stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode, void *info) {
    FileStreamInfo *fileStream = info;
    Boolean forRead = OCFGetTypeID(stream) == OCFReadStreamGetTypeID();
    OCFStreamStatus status = forRead ? OCFReadStreamGetStatus(stream) : OCFWriteStreamGetStatus(stream);
    if (fileStream->fd < 0 && status != kOCFStreamStatusNotOpen) {
        return;
    }
    if (status != kOCFStreamStatusNotOpen) {
        if (fileStream->scheduling.cffd == NULL) {
            constructCFFD(fileStream, forRead, stream);
        }
        OCFRunLoopSourceRef rls =
            OCFFileDescriptorCreateRunLoopSource(OCFGetAllocator(stream), fileStream->scheduling.cffd, 0);
        OCFRunLoopAddSource(runLoop, rls, runLoopMode);
        OCFRelease(rls);
        return;
    }
    OCFMutableArrayRef runLoopsAndModes = fileStream->scheduling.runLoopsAndModes;
    if (runLoopsAndModes == NULL) {
        runLoopsAndModes = OCFArrayCreateMutable(OCFGetAllocator(stream), 0, &kOCFTypeArrayCallBacks);
        fileStream->scheduling.runLoopsAndModes = runLoopsAndModes;
    }
    OCFArrayAppendValue(runLoopsAndModes, runLoop);
    OCFArrayAppendValue(fileStream->scheduling.runLoopsAndModes, runLoopMode);
}

// Native fileUnschedule, 0x1c017cb2c. Non-external. The status comes as in fileSchedule. An opened stream
// with a CFFileDescriptor (and no failed open, flag bit 5) removes a run loop source made for it (order 0,
// the stream's allocator; released afterwards) from the run loop in the mode. An unopened stream, or one
// whose open failed, removes the first recorded pair whose run loop and mode are equal (CFEqual) to the given
// ones; the pairs are walked while a whole pair remains.
static void fileUnschedule(void *stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode, void *info) {
    FileStreamInfo *fileStream = info;
    OCFStreamStatus status = OCFGetTypeID(stream) == OCFReadStreamGetTypeID()
                                 ? OCFReadStreamGetStatus(stream)
                                 : OCFWriteStreamGetStatus(stream);
    OCFMutableArrayRef runLoopsAndModes = fileStream->scheduling.runLoopsAndModes;
    if (runLoopsAndModes == NULL) {
        return;
    }
    if (status != kOCFStreamStatusNotOpen && (fileStream->flags & 0x20) == 0) {
        OCFRunLoopSourceRef rls =
            OCFFileDescriptorCreateRunLoopSource(OCFGetAllocator(stream), fileStream->scheduling.cffd, 0);
        OCFRunLoopRemoveSource(runLoop, rls, runLoopMode);
        OCFRelease(rls);
        return;
    }
    OCFIndex count = OCFArrayGetCount(runLoopsAndModes);
    for (OCFIndex idx = 0; idx + 1 < count; idx += 2) {
        if (OCFEqual(OCFArrayGetValueAtIndex(runLoopsAndModes, idx), runLoop) &&
            OCFEqual(OCFArrayGetValueAtIndex(runLoopsAndModes, idx + 1), runLoopMode)) {
            OCFArrayRemoveValueAtIndex(runLoopsAndModes, idx);
            OCFArrayRemoveValueAtIndex(runLoopsAndModes, idx);
            return;
        }
    }
}

// Native constructCFFD, 0x1c017ccc0. Non-external. A CFFileDescriptor for the descriptor (the stream's
// allocator, not closed on invalidation, fileCallBack, a context of version 0 whose info is the stream with
// _fs_retain, _fs_release and CFCopyDescription) gets read callbacks for a read stream, write callbacks
// otherwise. Recorded pairs (when any) each get one run loop source (order 0, the stream's allocator) added
// in their mode, the array is released and the source too. The CFFileDescriptor then replaces the array;
// true. A failed creation releases and clears the array (when set); false.
static Boolean constructCFFD(FileStreamInfo *fileStream, Boolean forRead, void *stream) {
    OCFFileDescriptorContext context = {
        0,
        stream,
        _fs_retain,
        _fs_release,
        (OCFStringRef (*)(void *))OCFCopyDescription,
    };
    OCFFileDescriptorRef cffd =
        OCFFileDescriptorCreate(OCFGetAllocator(stream), fileStream->fd, false, fileCallBack, &context);
    if (cffd == NULL) {
        if (fileStream->scheduling.runLoopsAndModes != NULL) {
            OCFRelease(fileStream->scheduling.runLoopsAndModes);
            fileStream->scheduling.runLoopsAndModes = NULL;
        }
        return false;
    }
    OCFFileDescriptorEnableCallBacks(
        cffd, forRead ? kOCFFileDescriptorReadCallBack : kOCFFileDescriptorWriteCallBack
    );
    if (fileStream->scheduling.runLoopsAndModes != NULL) {
        OCFIndex count = OCFArrayGetCount(fileStream->scheduling.runLoopsAndModes);
        OCFRunLoopSourceRef rls = OCFFileDescriptorCreateRunLoopSource(OCFGetAllocator(stream), cffd, 0);
        for (OCFIndex idx = 0; idx + 1 < count; idx += 2) {
            OCFRunLoopRef runLoop =
                (OCFRunLoopRef)OCFArrayGetValueAtIndex(fileStream->scheduling.runLoopsAndModes, idx);
            OCFRunLoopMode runLoopMode =
                (OCFRunLoopMode)OCFArrayGetValueAtIndex(fileStream->scheduling.runLoopsAndModes, idx + 1);
            OCFRunLoopAddSource(runLoop, rls, runLoopMode);
        }
        OCFRelease(fileStream->scheduling.runLoopsAndModes);
        OCFRelease(rls);
    }
    fileStream->scheduling.cffd = cffd;
    return true;
}

// Native _fs_retain, 0x1c017ce24. Non-external. Retains the stream when not NULL; returns it.
static void *_fs_retain(void *info) {
    if (info != NULL) {
        OCFRetain(info);
    }
    return info;
}

// Native _fs_release, 0x1c017ce50. Non-external. Releases the stream when not NULL (a tail call).
static void _fs_release(void *info) {
    if (info != NULL) {
        OCFRelease(info);
    }
}

// Native fileCallBack, 0x1c017ce5c. Non-external; the CFFileDescriptor callout. A NULL stream, or one without
// info (CFReadStreamGetInfoPointer or CFWriteStreamGetInfoPointer by its type), does nothing. A write
// callback (callback types exactly 2) sets flag bit 0 and signals can-accept-bytes with
// CFWriteStreamSignalEvent; anything else sets flag bit 1 and signals has-bytes-available with
// CFReadStreamSignalEvent (tail calls, whatever the stream's type).
static void fileCallBack(OCFFileDescriptorRef f, OCFOptionFlags callBackTypes, void *info) {
    (void)f;
    if (info == NULL) {
        return;
    }
    FileStreamInfo *fileStream;
    if (OCFGetTypeID(info) == OCFReadStreamGetTypeID()) {
        fileStream = OCFReadStreamGetInfoPointer(info);
    } else {
        fileStream = OCFWriteStreamGetInfoPointer(info);
    }
    if (fileStream == NULL) {
        return;
    }
    if (callBackTypes == kOCFFileDescriptorWriteCallBack) {
        fileStream->flags |= 0x1;
        OCFWriteStreamSignalEvent(info, kOCFStreamEventCanAcceptBytes, NULL);
        return;
    }
    fileStream->flags |= 0x2;
    OCFReadStreamSignalEvent(info, kOCFStreamEventHasBytesAvailable, NULL);
}

// The can-accept-bytes test the callbacks inline: always for growable buffers, otherwise while the current
// block has room (capacity above length, signed).
static inline OCFStreamEventType writeDataReadyEvent(const WriteDataStreamContext *dataStream) {
    if (dataStream->bufferAllocator != kOCFAllocatorNull) {
        return kOCFStreamEventCanAcceptBytes;
    }
    return dataStream->currentBuf->capacity > dataStream->currentBuf->length ? kOCFStreamEventCanAcceptBytes
                                                                             : kOCFStreamEventEndEncountered;
}

// The read data stream's info (24 bytes, typed allocation 0x107004054E2031F): the data (retained), the read
// position in its bytes and the scheduled byte. Local descriptive spellings.
typedef struct {
    OCFDataRef data;
    const UInt8 *loc;
    Boolean scheduled;
} ReadDataStreamContext;

_Static_assert(sizeof(ReadDataStreamContext) == 24, "The read data stream info is 24 bytes.");

// Native readDataCreate, 0x1c017cf0c (Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json, as are the
// seven callbacks after it). Non-external natively; hidden here because Stream/OCFConcreteStreams.h declares
// it. The info is allocated from the stream's allocator; when the allocation succeeds the context's data is
// retained (CFRetain's result is stored) and the position starts at its bytes, not scheduled. Only the
// context's first word is read.
void *readDataCreate(void *stream, void *info) {
    ReadDataContext *ctxt = info;
    ReadDataStreamContext *result = OCFAllocatorAllocateTyped(
        OCFGetAllocator(stream), sizeof(ReadDataStreamContext), 0x107004054E2031F, 0
    );
    if (result != NULL) {
        OCFDataRef data = OCFRetain(ctxt->data);
        result->data = data;
        result->loc = OCFDataGetBytePtr(data);
        result->scheduled = false;
    }
    return result;
}

// Native readDataFinalize, 0x1c017cf6c. Non-external natively; hidden here (see readDataCreate). The data is
// released before the info is returned to the stream's allocator (a tail call).
void readDataFinalize(void *stream, void *info) {
    ReadDataStreamContext *dataStream = info;
    OCFRelease(dataStream->data);
    OCFAllocatorDeallocate(OCFGetAllocator(stream), dataStream);
}

// Native readDataCopyDescription, 0x1c017cfa0. Non-external natively; hidden here (see readDataCreate). A
// tail call of CFCopyDescription of the data.
OCFStringRef readDataCopyDescription(void *stream, void *info) {
    (void)stream;
    return OCFCopyDescription(((ReadDataStreamContext *)info)->data);
}

// Native readDataOpen, 0x1c017cfa8. Non-external natively; hidden here (see readDataCreate). A scheduled
// stream signals end-encountered (0x10) for empty data, else has-bytes-available (2), with no error; the
// error code is cleared (the 32-bit field at offset 8) and the open completes at once.
Boolean readDataOpen(void *stream, void *error, Boolean *openComplete, void *info) {
    ReadDataStreamContext *dataStream = info;
    if (dataStream->scheduled) {
        OCFIndex length = OCFDataGetLength(dataStream->data);
        OCFReadStreamSignalEvent(
            stream, length == 0 ? kOCFStreamEventEndEncountered : kOCFStreamEventHasBytesAvailable, NULL
        );
    }
    ((OCFStreamError *)error)->error = 0;
    *openComplete = true;
    return true;
}

// Native dataRead, 0x1c017d010. Non-external natively; hidden here (see readDataCreate). The count is the
// smaller of the bytes left and the buffer length, clamped at zero; a positive count is copied (memmove) and
// advances the position. The error code is cleared, at-end is whether the position reached the end (an
// unsigned comparison), and a scheduled stream with bytes left signals has-bytes-available.
OCFIndex
dataRead(void *stream, UInt8 *buffer, OCFIndex bufferLength, void *error, Boolean *atEOF, void *info) {
    ReadDataStreamContext *dataStream = info;
    const UInt8 *bytes = OCFDataGetBytePtr(dataStream->data);
    const UInt8 *end = bytes + OCFDataGetLength(dataStream->data);
    const UInt8 *loc = dataStream->loc;
    OCFIndex bytesToCopy = end - loc < bufferLength ? end - loc : bufferLength;
    OCFIndex count = bytesToCopy < 0 ? 0 : bytesToCopy;
    if (bytesToCopy >= 1) {
        memmove(buffer, loc, count);
        loc = dataStream->loc + count;
        dataStream->loc = loc;
    }
    ((OCFStreamError *)error)->error = 0;
    *atEOF = loc >= end;
    if (dataStream->scheduled && loc < end) {
        OCFReadStreamSignalEvent(stream, kOCFStreamEventHasBytesAvailable, NULL);
    }
    return count;
}

// Native dataGetBuffer, 0x1c017d0d8. Non-external natively; hidden here (see readDataCreate). The count read
// is the smaller of the bytes left and the maximum (not clamped), and at-end is whether the bytes left do not
// exceed the maximum. The error code is cleared, the old position is returned, and the position advances by
// the stored count (read back); a scheduled stream not at its end signals has-bytes-available.
const UInt8 *dataGetBuffer(
    void *stream, OCFIndex maxBytesToRead, OCFIndex *numBytesRead, void *error, Boolean *atEOF, void *info
) {
    ReadDataStreamContext *dataStream = info;
    const UInt8 *bytes = OCFDataGetBytePtr(dataStream->data);
    OCFIndex length = OCFDataGetLength(dataStream->data);
    OCFIndex bytesLeft = (bytes + length) - dataStream->loc;
    *numBytesRead = bytesLeft < maxBytesToRead ? bytesLeft : maxBytesToRead;
    *atEOF = bytesLeft <= maxBytesToRead;
    ((OCFStreamError *)error)->error = 0;
    const UInt8 *result = dataStream->loc;
    dataStream->loc = result + *numBytesRead;
    if (dataStream->scheduled && !*atEOF) {
        OCFReadStreamSignalEvent(stream, kOCFStreamEventHasBytesAvailable, NULL);
    }
    return result;
}

// Native dataCanRead, 0x1c017d18c. Non-external natively; hidden here (see readDataCreate). Whether the end
// of the bytes lies above the position (an unsigned comparison).
Boolean dataCanRead(void *stream, void *info) {
    (void)stream;
    ReadDataStreamContext *dataStream = info;
    const UInt8 *bytes = OCFDataGetBytePtr(dataStream->data);
    OCFIndex length = OCFDataGetLength(dataStream->data);
    return bytes + length > dataStream->loc;
}

// Native readDataSchedule, 0x1c017d1cc. Non-external natively; hidden here (see readDataCreate). The first
// schedule sets the scheduled byte; when the stream is then open (status 2) it signals has-bytes-available
// with bytes left, else end-encountered (a tail call, no error). Later schedules do nothing; the run loop and
// mode are not read.
void readDataSchedule(void *stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode, void *info) {
    (void)runLoop;
    (void)runLoopMode;
    ReadDataStreamContext *dataStream = info;
    if (dataStream->scheduled) {
        return;
    }
    dataStream->scheduled = true;
    if (OCFReadStreamGetStatus(stream) != kOCFStreamStatusOpen) {
        return;
    }
    const UInt8 *bytes = OCFDataGetBytePtr(dataStream->data);
    OCFIndex length = OCFDataGetLength(dataStream->data);
    OCFReadStreamSignalEvent(
        stream,
        bytes + length > dataStream->loc ? kOCFStreamEventHasBytesAvailable : kOCFStreamEventEndEncountered,
        NULL
    );
}

// Native writeDataCreate, 0x1c017d254. Non-external. With a buffer allocator other than kCFAllocatorNull
// (NULL is replaced in the caller's context by CFAllocatorGetDefault()) the allocator is retained and the
// info is allocated with its first 1024-byte block (0x440 bytes from the stream's allocator, type
// 0x10600405C4CBD68); with kCFAllocatorNull the info (0x40 bytes) wraps the caller's fixed buffer. The
// allocation is not checked.
static void *writeDataCreate(void *stream, void *info) {
    WriteDataContext *ctxt = info;
    OCFAllocatorRef bufferAllocator = ctxt->bufferAllocator;
    WriteDataStreamContext *result;
    if (bufferAllocator != kOCFAllocatorNull) {
        if (bufferAllocator == NULL) {
            bufferAllocator = OCFAllocatorGetDefault();
            ctxt->bufferAllocator = bufferAllocator;
        }
        OCFRetain(bufferAllocator);
        result = OCFAllocatorAllocateTyped(OCFGetAllocator(stream), 0x440, 0x10600405C4CBD68, 0);
        WriteDataBlock *block = (WriteDataBlock *)((uint8_t *)result + 0x20);
        block->buffer = (UInt8 *)result + 0x40;
        block->length = 0;
        block->next = NULL;
        block->capacity = 0x400;
        result->firstBuf = block;
        result->currentBuf = block;
        bufferAllocator = ctxt->bufferAllocator;
    } else {
        result = OCFAllocatorAllocateTyped(OCFGetAllocator(stream), 0x40, 0x10600405C4CBD68, 0);
        WriteDataBlock *block = (WriteDataBlock *)((uint8_t *)result + 0x20);
        result->firstBuf = block;
        block->buffer = ctxt->fixedBuffer->bytes;
        block->capacity = ctxt->fixedBuffer->capacity;
        block->length = 0;
        block->next = NULL;
        result->currentBuf = block;
    }
    result->bufferAllocator = bufferAllocator;
    result->scheduled = false;
    return result;
}

// Native writeDataFinalize, 0x1c017d32c. Non-external. Growable buffers free every block after the first
// with the buffer allocator and release it; the info is freed with the stream's allocator (a tail call).
static void writeDataFinalize(void *stream, void *info) {
    WriteDataStreamContext *dataStream = info;
    if (dataStream->bufferAllocator != kOCFAllocatorNull) {
        WriteDataBlock *block = dataStream->firstBuf->next;
        while (block != NULL) {
            WriteDataBlock *next = block->next;
            OCFAllocatorDeallocate(dataStream->bufferAllocator, block);
            block = next;
        }
        OCFRelease(dataStream->bufferAllocator);
    }
    OCFAllocatorDeallocate(OCFGetAllocator(stream), dataStream);
}

// Native writeDataCopyDescription, 0x1c017d3a0. Non-external. "<CFWriteDataContext %p>" with the info, in
// kCFAllocatorSystemDefault.
static OCFStringRef writeDataCopyDescription(void *stream, void *info) {
    (void)stream;
    return OCFStringCreateWithFormat(
        kOCFAllocatorSystemDefault, NULL, OF_STR("<CFWriteDataContext %p>"), info
    );
}

// Native writeDataOpen, 0x1c017d3d8. Non-external. A scheduled stream signals can-accept-bytes (or end
// encountered for a full fixed buffer); the error code is cleared and the open completes at once.
static Boolean writeDataOpen(void *stream, void *error, Boolean *openComplete, void *info) {
    WriteDataStreamContext *dataStream = info;
    if (dataStream->scheduled) {
        OCFWriteStreamSignalEvent(stream, writeDataReadyEvent(dataStream), NULL);
    }
    ((OCFStreamError *)error)->error = 0;
    *openComplete = true;
    return true;
}

// Native dataWrite, 0x1c017d450. Non-external. A fixed buffer without room for the whole write fails at once
// with POSIX error 12 (domain 1) and -1. Otherwise the bytes fill the current block's room (the room computed
// once, before the first copy) and then new blocks of max(remaining, 1024) bytes (32-byte header, buffer
// allocator, type 0x1030040A9292222) appended to the chain; a failed allocation fails the same way. On
// success the error code is cleared and a scheduled stream signals can-accept-bytes while room remains
// (always for growable buffers); the length written is the full length.
static OCFIndex dataWrite(void *stream, const UInt8 *buffer, OCFIndex bufferLength, void *error, void *info) {
    WriteDataStreamContext *dataStream = info;
    OCFStreamError *streamError = error;
    WriteDataBlock *current = dataStream->currentBuf;
    OCFIndex room = current->capacity - current->length;
    if (dataStream->bufferAllocator == kOCFAllocatorNull && bufferLength > room) {
        goto failure;
    }
    OCFIndex remaining = bufferLength;
    const UInt8 *source = buffer;
    while (remaining >= 1) {
        if (room >= 1) {
            OCFIndex count = (uint64_t)remaining < (uint64_t)room ? remaining : room;
            memmove(current->buffer + current->length, source, (size_t)count);
            source += count;
            remaining -= count;
            current = dataStream->currentBuf;
            current->length += count;
            if (remaining < 1) {
                continue;
            }
        }
        OCFIndex capacity = (uint64_t)remaining > 0x400 ? remaining : 0x400;
        WriteDataBlock *block =
            OCFAllocatorAllocateTyped(dataStream->bufferAllocator, capacity + 0x20, 0x1030040A9292222, 0);
        if (block == NULL) {
            goto failure;
        }
        block->buffer = (UInt8 *)block + 0x20;
        block->capacity = capacity;
        block->length = 0;
        block->next = NULL;
        dataStream->currentBuf->next = block;
        dataStream->currentBuf = block;
        current = block;
        room = capacity;
    }
    streamError->error = 0;
    if (dataStream->scheduled) {
        if (dataStream->bufferAllocator != kOCFAllocatorNull || current->capacity > current->length) {
            OCFWriteStreamSignalEvent(stream, kOCFStreamEventCanAcceptBytes, NULL);
        }
    }
    return bufferLength;
failure:
    streamError->error = 12;
    streamError->domain = 1;
    return -1;
}

// Native dataCanWrite, 0x1c017d5d8. Non-external. Always for growable buffers, otherwise while the current
// block has room.
static Boolean dataCanWrite(void *stream, void *info) {
    (void)stream;
    WriteDataStreamContext *dataStream = info;
    if (dataStream->bufferAllocator != kOCFAllocatorNull) {
        return true;
    }
    return dataStream->currentBuf->capacity > dataStream->currentBuf->length;
}

// Native dataCopyProperty, 0x1c017d60c. Non-external. Only kCFStreamPropertyDataWritten (CFEqual) of growable
// buffers: the blocks' bytes are gathered into one allocation of the total length from the buffer allocator
// (type 0x100004077774924; NULL on failure) and returned as CFData owned by that allocator; an empty stream
// gives CFData with no bytes.
static OCFTypeRef dataCopyProperty(void *stream, OCFStreamPropertyKey propertyName, void *info) {
    (void)stream;
    WriteDataStreamContext *dataStream = info;
    if (!OCFEqual(propertyName, kOCFStreamPropertyDataWritten)) {
        return NULL;
    }
    OCFAllocatorRef bufferAllocator = dataStream->bufferAllocator;
    if (bufferAllocator == kOCFAllocatorNull) {
        return NULL;
    }
    OCFIndex size = 0;
    UInt8 *bytes = NULL;
    for (WriteDataBlock *block = dataStream->firstBuf; block != NULL; block = block->next) {
        size += block->length;
    }
    if (size >= 1) {
        bytes = OCFAllocatorAllocateTyped(bufferAllocator, size, 0x100004077774924, 0);
        if (bytes == NULL) {
            return NULL;
        }
        UInt8 *destination = bytes;
        for (WriteDataBlock *block = dataStream->firstBuf; block != NULL; block = block->next) {
            memmove(destination, block->buffer, (size_t)block->length);
            destination += block->length;
        }
    }
    return OCFDataCreateWithBytesNoCopy(bufferAllocator, bytes, size, bufferAllocator);
}

// Native writeDataSchedule, 0x1c017d70c. Non-external. The first schedule marks the stream scheduled; an open
// stream (status 2) then signals can-accept-bytes (or end encountered for a full fixed buffer; a tail call).
static void writeDataSchedule(void *stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode, void *info) {
    (void)runLoop;
    (void)runLoopMode;
    WriteDataStreamContext *dataStream = info;
    if (dataStream->scheduled) {
        return;
    }
    dataStream->scheduled = true;
    if (OCFWriteStreamGetStatus(stream) != kOCFStreamStatusOpen) {
        return;
    }
    OCFWriteStreamSignalEvent(stream, writeDataReadyEvent(dataStream), NULL);
}

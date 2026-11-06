//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Stream/OCFStream.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/String/OCFString.h"

#include <objc/message.h>
#include <objc/runtime.h>
#include <os/lock.h>

// Inferred organization: the native CFStream functions of this unit lie in the CFStream cluster of the iOS
// Simulator __text section, initializeCFNetworkSupport (0x1c01fa23c), _CFStreamCreateErrorFromStreamError
// (0x1c01fa2f8) and _CFStreamClose
// (0x1c01fed28) through ____signalEventQueue_block_invoke (0x1c02025ec), with the class callbacks and the
// stream class records, the read stream entry points of the delta capture 4
// (Local/Evidence/cf-core/iphone-simulator-corpus-delta4.json), the stream SPI of the delta capture 5
// (iphone-simulator-corpus-delta5.json) and the client, scheduling, buffer and error entry points of the
// delta capture 7 (iphone-simulator-corpus-delta7.json). They are kept in one translation unit in native
// address order; the native source path is not in the evidence. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (disassembly); iOS Device
// 24A446: unread. Reading: Local/Scratch/cf-core/tasks/runloop-streams.txt (sub-track reading; no versioned
// reading yet).

// libobjc's objc_retain, which the native image imports; the SDK installs no declaration for it.
extern id objc_retain(id object);
// libobjc's allocation, class, release and respondsToSelector entries (objc-internal.h), as the native image
// imports them.
extern id objc_alloc(Class cls);
extern Class objc_opt_class(id object);
extern void objc_release(id object);
extern BOOL objc_opt_respondsToSelector(id object, SEL selector);

// The delegate class of this image that bridged streams receive from CFReadStreamSetClient and
// CFWriteStreamSetClient (native _CFStreamDelegate, class 0x22e6cd8b0, was a private external; its methods
// 0x1c01febc4-0x1c01fed00 are not restored on this branch).
extern char streamDelegateClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(_OCFStreamDelegate));

// The unit's static functions, defined below in native order.
static void initializeCFNetworkSupport(void);
static void _cfstream_solo_signalEventSync(void *info);
static void _cfstream_shared_signalEventSync(void *info);
static void waitForOpen(StreamObject *stream);
static void _signalEventSync(StreamObject *stream);

// Flag bits of the stream object.
enum {
    kStreamStatusMask = 0x1F,
    kStreamConstantCallBacks = 0x20,
    kStreamCallingClient = 0x40,
    kStreamClosing = 0x80,
    kStreamSharedSource = 0x100,
};

static OCFStreamError *streamErrorStorage(StreamObject *stream);
static StreamObject *_CFStreamCreate(OCFAllocatorRef allocator, Boolean isReadStream);
static void _wakeUpRunLoop(StreamObject *stream);
static OCFErrorRef _CFStreamCopyError(StreamObject *stream);
static OCFArrayRef _CFStreamCopyRunLoopsAndModes(StreamObject *stream);

// Native sSharedSources and sSourceLock, 0x22e6d3008 and 0x22e6d3000 (__DATA,__bss). Non-external; the shared
// run loop sources' table (a {run loop, mode} array to its streams, and each stream to its key) and its lock.
static OCFMutableDictionaryRef sSharedSources;
static os_unfair_lock sSourceLock;

// The helpers below reproduce code the native functions inline at each use (no native symbols): the
// bridged-object test for the write stream type (39), the error storage of version 0 and 1 callbacks (a
// 16-byte typed allocation 0x1000040D9A13B51 on the stream's allocator, created on first use), the client
// signal (the source is retained under the stream's lock, the event joins whatToSignal, the source is
// signalled and released and the run loops woken) and the detachment of the client's source.
static inline bool isObjCWriteStream(OCFTypeRef cf) {
    return ((uintptr_t)cf >> 63) != 0 || __CF_IS_OBJC(39, cf) != 0;
}

// The same test for the read stream type (38).
static inline bool isObjCReadStream(OCFTypeRef cf) {
    return ((uintptr_t)cf >> 63) != 0 || __CF_IS_OBJC(38, cf) != 0;
}

static OCFStreamError *streamErrorStorage(StreamObject *stream) {
    if (stream->error == NULL) {
        stream->error = OCFAllocatorAllocateTyped(OCFGetAllocator(stream), 16, 0x1000040D9A13B51, 0);
    }
    return stream->error;
}

static inline void setStatus(StreamObject *stream, uint64_t status) {
    stream->flags = (stream->flags & 0xFFFFFFE0) | status;
}

static void signalClientSource(StreamObject *stream, OCFOptionFlags event) {
    pthread_mutex_lock(&stream->lock);
    StreamClient *client = stream->client;
    OCFRunLoopSourceRef source = client != NULL ? client->rlSource : NULL;
    if (source == NULL) {
        pthread_mutex_unlock(&stream->lock);
        return;
    }
    OCFRetain(source);
    pthread_mutex_unlock(&stream->lock);
    stream->client->whatToSignal |= event;
    OCFRunLoopSourceSignal(source);
    OCFRelease(source);
    _wakeUpRunLoop(stream);
}

// Detaches the client's source under the stream's lock; when invalidate is set the source is invalidated.
// Both releases follow the unlock, the invalidation between them.
static void detachClientSource(StreamObject *stream, bool invalidate) {
    pthread_mutex_lock(&stream->lock);
    StreamClient *client = stream->client;
    if (client == NULL) {
        pthread_mutex_unlock(&stream->lock);
        return;
    }
    OCFRunLoopSourceRef source = client->rlSource;
    if (source == NULL) {
        client->rlSource = NULL;
        pthread_mutex_unlock(&stream->lock);
        return;
    }
    OCFRetain(source);
    stream->client->rlSource = NULL;
    pthread_mutex_unlock(&stream->lock);
    OCFRelease(source);
    if (invalidate) {
        OCFRunLoopSourceInvalidate(source);
    }
    OCFRelease(source);
}

// Replaces the client's source with a retained new one under the stream's lock; the old source is released
// twice after the unlock.
static void setClientSource(StreamObject *stream, OCFRunLoopSourceRef source) {
    pthread_mutex_lock(&stream->lock);
    StreamClient *client = stream->client;
    if (client == NULL) {
        pthread_mutex_unlock(&stream->lock);
        return;
    }
    OCFRunLoopSourceRef old = client->rlSource;
    if (old != NULL) {
        OCFRetain(old);
    }
    stream->client->rlSource = source;
    if (source != NULL) {
        OCFRetain(source);
    }
    pthread_mutex_unlock(&stream->lock);
    if (old != NULL) {
        OCFRelease(old);
        OCFRelease(old);
    }
}

// Removes the stream from its shared source's stream list under sSourceLock; the last stream removes the
// source from the run loop and mode (the key's, or the given ones) and the key. The stream's entry is
// removed, its source detached (invalidated when it was the last) and the shared flag cleared.
static void removeFromSharedSource(StreamObject *stream, OCFRunLoopRef runLoop, OCFStringRef runLoopMode) {
    OCFArrayRef key = OCFDictionaryGetValue(sSharedSources, stream);
    OCFMutableArrayRef list = (OCFMutableArrayRef)OCFDictionaryGetValue(sSharedSources, key);
    OCFIndex count = OCFArrayGetCount(list);
    OCFIndex idx = OCFArrayGetFirstIndexOfValue(list, (OCFRange){0, count}, stream);
    if (idx != -1) {
        OCFArrayRemoveValueAtIndex(list, idx);
        count--;
    }
    if (count == 0) {
        pthread_mutex_lock(&stream->lock);
        StreamClient *client = stream->client;
        OCFRunLoopSourceRef source = client != NULL ? client->rlSource : NULL;
        if (source != NULL) {
            OCFRetain(source);
            pthread_mutex_unlock(&stream->lock);
            if (runLoop == NULL) {
                OCFRunLoopRemoveSource(
                    (OCFRunLoopRef)OCFArrayGetValueAtIndex(key, 0), source, OCFArrayGetValueAtIndex(key, 1)
                );
            } else {
                OCFRunLoopRemoveSource(runLoop, source, runLoopMode);
            }
            OCFRelease(source);
        } else {
            pthread_mutex_unlock(&stream->lock);
        }
        OCFDictionaryRemoveValue(sSharedSources, key);
    }
    OCFDictionaryRemoveValue(sSharedSources, stream);
    detachClientSource(stream, count == 0);
    stream->flags &= ~(uint64_t)kStreamSharedSource;
}

// Native CFNetworkSupport, 0x22e6cf898 (__DATA,__data, 96 bytes). Non-external; hidden here because the
// stream units share it. The mutex is statically initialized (signature 0x32AAABA1); everything else is zero.
StreamNetworkSupport CFNetworkSupport = {PTHREAD_MUTEX_INITIALIZER, 0, 0, NULL, NULL, NULL};

// Native initializeCFNetworkSupport, 0x1c01fa23c. Non-external; called with CFNetworkSupport's lock held.
// Bit 0 of the flags is set, the three functions are looked up and stored in order, each slot is read back
// and a NULL one logs "CoreFoundation: failed to dynamically link symbol <name>" (level 3), and bit 1 is set.
static void initializeCFNetworkSupport(void) {
    CFNetworkSupport.flags |= 1;
    CFNetworkSupport.createSocketStreamPair = __CFLookupCFNetworkFunction("_CFSocketStreamCreatePair");
    CFNetworkSupport.createErrorFromStreamError = (OCFErrorRef (*)(OCFAllocatorRef, const OCFStreamError *))
        __CFLookupCFNetworkFunction("_CFErrorCreateWithStreamError");
    CFNetworkSupport.streamErrorFromCFError = __CFLookupCFNetworkFunction("_CFStreamErrorFromCFError");
    if (CFNetworkSupport.createSocketStreamPair == NULL) {
        OCFLog(3, OF_STR("CoreFoundation: failed to dynamically link symbol _CFSocketStreamCreatePair"));
    }
    if (CFNetworkSupport.createErrorFromStreamError == NULL) {
        OCFLog(3, OF_STR("CoreFoundation: failed to dynamically link symbol _CFErrorCreateWithStreamError"));
    }
    if (CFNetworkSupport.streamErrorFromCFError == NULL) {
        OCFLog(3, OF_STR("CoreFoundation: failed to dynamically link symbol _CFStreamErrorFromCFError"));
    }
    CFNetworkSupport.flags |= 2;
}

// Native _CFStreamCreateErrorFromStreamError, 0x1c01fa2f8. Hidden (was private external). CFNetwork's
// converter (looked up once under the support lock, called through the table after the unlock) takes
// precedence; otherwise the OSStatus (2) and POSIX (1) domains map to their error domains and any other
// domain to "BogusCFStreamErrorCompatibilityDomain" with the original domain under "CFStreamErrorDomainKey"
// (a CFIndex number).
OCFErrorRef _CFStreamCreateErrorFromStreamError(OCFAllocatorRef alloc, const OCFStreamError *streamError) {
    pthread_mutex_lock(&CFNetworkSupport.lock);
    if ((CFNetworkSupport.flags & 1) == 0) {
        initializeCFNetworkSupport();
    }
    void *converter = (void *)CFNetworkSupport.createErrorFromStreamError;
    pthread_mutex_unlock(&CFNetworkSupport.lock);
    if (converter != NULL) {
        return CFNetworkSupport.createErrorFromStreamError(alloc, streamError);
    }
    if (streamError->domain == 2) {
        return OCFErrorCreate(alloc, kOCFErrorDomainOSStatus, streamError->error, NULL);
    }
    if (streamError->domain == 1) {
        return OCFErrorCreate(alloc, kOCFErrorDomainPOSIX, streamError->error, NULL);
    }
    const void *keys[1] = {OF_STR("CFStreamErrorDomainKey")};
    const void *values[1] = {OCFNumberCreate(alloc, 14, &streamError->domain)};
    OCFDictionaryRef dict = OCFDictionaryCreate(
        alloc, keys, values, 1, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    OCFErrorRef result =
        OCFErrorCreate(alloc, OF_STR("BogusCFStreamErrorCompatibilityDomain"), streamError->error, dict);
    OCFRelease(values[0]);
    OCFRelease(dict);
    return result;
}

// Native _CFStreamClose, 0x1c01fed28. Hidden (was private external). An open stream not already closing is
// marked closing and calling, its close callback runs and its client's source is given up: a shared source
// through the shared table (the last stream removes and invalidates it), an own source invalidated. The
// status becomes closed unless closed or in error (clearing the calling bit and the high word; the closing
// bit stays).
void _CFStreamClose(StreamObject *stream) {
    OCFStreamStatus status = _CFStreamGetStatus(stream);
    if (status == kOCFStreamStatusNotOpen || status == kOCFStreamStatusClosed) {
        return;
    }
    const StreamCallBacks *cb = stream->callBacks;
    if ((stream->flags & kStreamClosing) != 0) {
        return;
    }
    stream->flags |= kStreamClosing | kStreamCallingClient;
    if (cb->close != NULL) {
        cb->close(stream, stream->info);
    }
    if (stream->client != NULL && stream->client->rlSource != NULL) {
        if ((stream->flags & kStreamSharedSource) != 0) {
            os_unfair_lock_lock(&sSourceLock);
            removeFromSharedSource(stream, NULL, NULL);
            os_unfair_lock_unlock(&sSourceLock);
        } else {
            detachClientSource(stream, true);
        }
    }
    uint64_t flags = stream->flags & ~(uint64_t)kStreamCallingClient;
    if ((stream->flags & 0x1E) != 6) {
        flags = (stream->flags & 0xFFFFFFA0) | kOCFStreamStatusClosed;
    }
    stream->flags = flags;
}

// Native _CFStreamGetStatus, 0x1c01fef90. Hidden (was private external). Marks the call in progress; an
// opening stream asks its openCompleted callback (version 0 and 1 errors are copied into the error storage
// when nonzero). A completed open becomes open, or in error with an error, unless closed or in error (an
// error still moves a closed stream to the error status); the client is signalled for the error or the open
// completion.
OCFStreamStatus _CFStreamGetStatus(StreamObject *stream) {
    uint64_t flags = stream->flags;
    OCFStreamStatus result = flags & kStreamStatusMask;
    stream->flags = flags | kStreamCallingClient;
    if (result == kOCFStreamStatusOpening) {
        const StreamCallBacks *cb = stream->callBacks;
        result = kOCFStreamStatusOpening;
        if (cb->openCompleted != NULL) {
            Boolean isComplete;
            if (cb->version <= 1) {
                OCFStreamError err = {0, 0};
                isComplete = cb->openCompleted(stream, &err, stream->info);
                if (err.error != 0) {
                    *streamErrorStorage(stream) = err;
                }
            } else {
                isComplete = cb->openCompleted(stream, &stream->error, stream->info);
            }
            if (isComplete) {
                void *error = stream->error;
                uint64_t status = stream->flags & kStreamStatusMask;
                bool closedWithError = error != NULL && status == kOCFStreamStatusClosed;
                if (status - 8 < (uint64_t)-2 || closedWithError) {
                    setStatus(stream, error != NULL ? kOCFStreamStatusError : kOCFStreamStatusOpen);
                }
                StreamClient *client = stream->client;
                if (error != NULL) {
                    if (client != NULL && (client->when & kOCFStreamEventErrorOccurred) != 0) {
                        signalClientSource(stream, kOCFStreamEventErrorOccurred);
                    }
                    result = kOCFStreamStatusError;
                } else {
                    if (client != NULL && (client->when & kOCFStreamEventOpenCompleted) != 0) {
                        signalClientSource(stream, kOCFStreamEventOpenCompleted);
                    }
                    result = kOCFStreamStatusOpen;
                }
            }
        }
    }
    stream->flags &= ~(uint64_t)kStreamCallingClient;
    return result;
}

// Native __CFStreamDeallocate, 0x1c01ff1c8. Non-external; the finalize callback of both stream classes. The
// stream is closed first. A client releases its context info (release callback, when both are set), its run
// loops and modes, and is freed with the stream's allocator. The callbacks' finalize runs with the info alone
// for version 0 callbacks and with the stream and info otherwise; an error is released for callbacks above
// version 1 and freed otherwise (released when there are no callbacks). Callbacks the stream owns (constant
// bit clear) are freed. The object at 0x78 is released and the queue at 0x80 released (dispatch_release),
// both cleared.
static void __CFStreamDeallocate(OCFTypeRef cf) {
    StreamObject *stream = (StreamObject *)cf;
    const StreamCallBacks *cb = stream->callBacks;
    OCFAllocatorRef alloc = OCFGetAllocator(stream);
    _CFStreamClose(stream);
    StreamClient *client = stream->client;
    if (client != NULL) {
        if (client->info != NULL && client->release != NULL) {
            client->release(client->info);
            client = stream->client;
        }
        if (client->runLoopsAndModes != NULL) {
            OCFRelease(client->runLoopsAndModes);
            client = stream->client;
        }
        OCFAllocatorDeallocate(alloc, client);
        stream->client = NULL;
    }
    if (cb == NULL) {
        if (stream->error != NULL) {
            OCFRelease(stream->error);
        }
    } else {
        if (cb->finalize != NULL) {
            if (cb->version == 0) {
                ((void (*)(void *))(void *)cb->finalize)(stream->info);
            } else {
                cb->finalize(stream, stream->info);
            }
        }
        if (stream->error != NULL) {
            if (cb->version > 1) {
                OCFRelease(stream->error);
            } else {
                OCFAllocatorDeallocate(alloc, stream->error);
            }
        }
    }
    if ((stream->flags & kStreamConstantCallBacks) == 0) {
        OCFAllocatorDeallocate(alloc, (void *)stream->callBacks);
    }
    if (stream->unread0 != NULL) {
        OCFRelease(stream->unread0);
        stream->unread0 = NULL;
    }
    if (stream->queue != NULL) {
        dispatch_release(stream->queue);
        stream->queue = NULL;
    }
}

// Native __CFStreamCopyDescription, 0x1c01ff2d0. Non-external; the debug description callback of both stream
// classes. The callbacks' description (with the info alone for version 0, the stream and info otherwise), or
// "info = %p" in the stream's allocator, is wrapped in "<CFReadStream %p>{%@}" for type 38 and
// "<CFWriteStream %p>{%@}" otherwise, and released.
static OCFStringRef __CFStreamCopyDescription(OCFTypeRef cf) {
    StreamObject *stream = (StreamObject *)cf;
    const StreamCallBacks *cb = stream->callBacks;
    OCFStringRef contextDescription;
    if (cb->copyDescription == NULL) {
        contextDescription =
            OCFStringCreateWithFormat(OCFGetAllocator(stream), NULL, OF_STR("info = %p"), stream->info);
    } else if (cb->version == 0) {
        contextDescription = ((OCFStringRef (*)(void *))(void *)cb->copyDescription)(stream->info);
    } else {
        contextDescription = cb->copyDescription(stream, stream->info);
    }
    OCFTypeID typeID = OCFGetTypeID(stream);
    OCFStringRef format = typeID == 38 ? OF_STR("<CFReadStream %p>{%@}") : OF_STR("<CFWriteStream %p>{%@}");
    OCFStringRef result =
        OCFStringCreateWithFormat(OCFGetAllocator(stream), NULL, format, stream, contextDescription);
    OCFRelease(contextDescription);
    return result;
}

// Native __CFReadStreamClass, 0x2227a4e88, and __CFWriteStreamClass, 0x2227a4ee8 (__DATA_CONST,__const).
// Hidden (were private externals). Version 0, types 38 and 39 (the IDs _CFStreamCreate instantiates);
// finalize and debug description only.
const OCFRuntimeClass __CFReadStreamClass = {
    0,
    "CFReadStream",
    NULL,
    NULL,
    __CFStreamDeallocate,
    NULL,
    NULL,
    NULL,
    __CFStreamCopyDescription,
    NULL,
    NULL,
    0,
};

const OCFRuntimeClass __CFWriteStreamClass = {
    0,
    "CFWriteStream",
    NULL,
    NULL,
    __CFStreamDeallocate,
    NULL,
    NULL,
    NULL,
    __CFStreamCopyDescription,
    NULL,
    NULL,
    0,
};

// Native kCFStreamPropertyDataWritten (0x2227a4f60, natively the same address as
// NSStreamDataWrittenToMemoryStreamKey, an alias this branch does not define) and
// kCFStreamPropertyAppendToFile (0x2227a4f68), __DATA_CONST,__const, after the stream class records.
// External. Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta6.json ("data"); the constant
// strings (0x2227b1dc0 and 0x2227b1de0, flags 0x7c8) read with Local/Scratch/cf-core/macho.py. As the locale
// keys (Locale/OCFLocaleKeys.c), the strings are spelled as split literals: their contents are the native
// names in both naming modes.
static const OCFConstantString streamPropertyDataWrittenString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFStreamProperty"
    "DataWritten",
    28
};
const OCFStreamPropertyKey kOCFStreamPropertyDataWritten =
    (OCFStreamPropertyKey)&streamPropertyDataWrittenString;

static const OCFConstantString streamPropertyAppendToFileString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    "kCFStreamProperty"
    "AppendToFile",
    29
};
const OCFStreamPropertyKey kOCFStreamPropertyAppendToFile =
    (OCFStreamPropertyKey)&streamPropertyAppendToFileString;

// Native CFReadStreamGetTypeID, 0x1c01ff398. External.
OCFTypeID OCFReadStreamGetTypeID(void) {
    return 38;
}

// Native CFWriteStreamGetTypeID, 0x1c01ff3a0. External.
OCFTypeID OCFWriteStreamGetTypeID(void) {
    return 39;
}

// Native _CFStreamCreateWithConstantCallbacks, 0x1c01ff3a8. Hidden (was private external). Only version 1
// callbacks are accepted; the stream keeps the callbacks (constant-callbacks bit) and the create callback's
// info.
StreamObject *_CFStreamCreateWithConstantCallbacks(
    OCFAllocatorRef alloc, void *info, const StreamCallBacks *cb, Boolean isReading
) {
    if (cb->version != 1) {
        return NULL;
    }
    StreamObject *newStream = _CFStreamCreate(alloc, isReading);
    if (newStream != NULL) {
        newStream->flags |= kStreamConstantCallBacks;
        newStream->callBacks = cb;
        if (cb->create != NULL) {
            info = cb->create(newStream, info);
        }
        newStream->info = info;
    }
    return newStream;
}

// Native _CFStreamCreate, 0x1c01ff424. Non-external. A read (38) or write (39) stream with 128 extra bytes;
// the status is cleared unless closed or in error, the lock statically initialized (signature 0x32AAABA1) and
// the next bytes zeroed.
static StreamObject *_CFStreamCreate(OCFAllocatorRef allocator, Boolean isReadStream) {
    StreamObject *newStream = (StreamObject *)_OCFRuntimeCreateInstance(
        allocator, isReadStream ? 38 : 39, sizeof(StreamObject) - 16, NULL
    );
    if (newStream == NULL) {
        return NULL;
    }
    if ((newStream->flags & 0x1E) != 6) {
        newStream->flags &= 0xFFFFFFE0;
    }
    newStream->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    return newStream;
}

// Native _CFStreamSignalEvent, 0x1c01ff74c. Hidden (was private external). An unopened stream logs "Stream %p
// is sending an event before being opened" and signals nothing, as does a closed or failed one. Opening
// streams open on the open-completed event; other states drop that event (at end only the error event
// remains); the end event moves a stream up to writing to at end. An error event stores the error (a retained
// CFError for callbacks above version 1, else a copy) and sets the error status. A wanted, not yet pending
// event is signalled: synchronously through _cfstream_solo_signalEventSync when allowed, no callback is in
// progress and the current run loop's current mode holds the source; else the source is signalled and the run
// loops woken.
void _CFStreamSignalEvent(
    StreamObject *stream, OCFStreamEventType event, const void *error, Boolean synchronousAllowed
) {
    uint64_t flags = stream->flags;
    uint64_t status = flags & kStreamStatusMask;
    OCFOptionFlags signal;
    if (status == kOCFStreamStatusNotOpen) {
        OCFLog(3, OF_STR("Stream %p is sending an event before being opened"), stream);
        signal = 0;
    } else if ((flags & 0x1E) == 6) {
        signal = 0;
    } else {
        signal = event;
        if (status == kOCFStreamStatusOpening) {
            if ((event & kOCFStreamEventOpenCompleted) != 0) {
                flags = (flags & 0xFFFFFFE0) | kOCFStreamStatusOpen;
                stream->flags = flags;
            }
        } else if (status != kOCFStreamStatusAtEnd) {
            signal = event & ~(OCFOptionFlags)kOCFStreamEventOpenCompleted;
        } else {
            signal = event & kOCFStreamEventErrorOccurred;
        }
        if (status != kOCFStreamStatusAtEnd) {
            if ((signal & kOCFStreamEventEndEncountered) != 0 && status <= 4 && (flags & 0x1E) != 6) {
                stream->flags = (flags & 0xFFFFFFE0) | kOCFStreamStatusAtEnd;
            }
        }
        if ((signal & kOCFStreamEventErrorOccurred) != 0) {
            if (stream->callBacks->version > 1) {
                OCFRetain(error);
                if (stream->error != NULL) {
                    OCFRelease(stream->error);
                }
                stream->error = (void *)error;
            } else {
                *streamErrorStorage(stream) = *(const OCFStreamError *)error;
            }
            if ((stream->flags & kStreamStatusMask) != kOCFStreamStatusError) {
                setStatus(stream, kOCFStreamStatusError);
            }
        }
    }
    StreamClient *client = stream->client;
    if (client == NULL || (client->when & signal) == 0 || (client->whatToSignal & signal) != 0) {
        return;
    }
    pthread_mutex_lock(&stream->lock);
    client = stream->client;
    OCFRunLoopSourceRef source = client != NULL ? client->rlSource : NULL;
    if (source == NULL) {
        pthread_mutex_unlock(&stream->lock);
        return;
    }
    OCFRetain(source);
    pthread_mutex_unlock(&stream->lock);
    stream->client->whatToSignal |= signal;
    bool signalNow = false;
    if (synchronousAllowed && (stream->flags & kStreamCallingClient) == 0) {
        OCFRunLoopRef rl = OCFRunLoopGetCurrent();
        OCFStringRef mode = OCFRunLoopCopyCurrentMode(rl);
        if (mode != NULL) {
            signalNow = OCFRunLoopContainsSource(rl, source, mode);
            OCFRelease(mode);
        }
    }
    if (signalNow) {
        _cfstream_solo_signalEventSync(stream);
    } else {
        OCFRunLoopSourceSignal(source);
        _wakeUpRunLoop(stream);
    }
    OCFRelease(source);
}

// The delivery of the inlined _signalEventQueue (no native symbol): the stream is retained for the block,
// which runs _signalEventSync on the queue and releases it (____signalEventQueue_block_invoke, 0x1c02025ec,
// below in native order as the block's invoke).
static inline void signalEventQueue(StreamObject *stream, dispatch_queue_t queue) {
    OCFRetain(stream);
    dispatch_async(queue, ^{
      _signalEventSync(stream);
      OCFRelease(stream);
    });
}

// Native _cfstream_solo_signalEventSync, 0x1c01ff978. Non-external; the perform callback of a stream's own
// run loop source. Anything but a read or write stream logs "Expected a read or write stream for %p" (level
// 3). Under the stream's lock a shared delivery in progress makes it return; otherwise the stream is retained
// (and its queue, if any) before the unlock, and the events are delivered on the queue (asynchronously, the
// queue then released) or at once with _signalEventSync; the stream is released.
static void _cfstream_solo_signalEventSync(void *info) {
    StreamObject *stream = info;
    if ((OCFGetTypeID(stream) & ~(OCFTypeID)1) != 38) {
        OCFLog(3, OF_STR("Expected a read or write stream for %p"), stream);
        return;
    }
    pthread_mutex_lock(&stream->lock);
    if (stream->signalling) {
        pthread_mutex_unlock(&stream->lock);
        return;
    }
    dispatch_queue_t queue = stream->queue;
    if (queue != NULL) {
        dispatch_retain(queue);
        OCFRetain(stream);
        pthread_mutex_unlock(&stream->lock);
        signalEventQueue(stream, queue);
        dispatch_release(queue);
    } else {
        OCFRetain(stream);
        pthread_mutex_unlock(&stream->lock);
        _signalEventSync(stream);
    }
    OCFRelease(stream);
}

// Native _wakeUpRunLoop, 0x1c01ffabc. Non-external. Two entries wake the first run loop; when every run loop
// in three or more entries is the first one it is woken; otherwise the first run loop waiting in its
// scheduled mode is woken, else the first one.
static void _wakeUpRunLoop(StreamObject *stream) {
    OCFArrayRef rlArray = _CFStreamCopyRunLoopsAndModes(stream);
    if (rlArray == NULL) {
        return;
    }
    OCFIndex cnt = OCFArrayGetCount(rlArray);
    OCFRunLoopRef rl = NULL;
    if (cnt == 2) {
        rl = (OCFRunLoopRef)OCFArrayGetValueAtIndex(rlArray, 0);
    } else if (cnt >= 3) {
        rl = (OCFRunLoopRef)OCFArrayGetValueAtIndex(rlArray, 0);
        bool allSame = false;
        if (rl != NULL) {
            OCFIndex idx = 4;
            OCFRunLoopRef value;
            bool more;
            do {
                value = (OCFRunLoopRef)OCFArrayGetValueAtIndex(rlArray, idx - 2);
                more = value == rl && idx < cnt;
                idx += 2;
            } while (more);
            allSame = value == rl;
        }
        if (!allSame) {
            bool found = false;
            for (OCFIndex idx = 0; idx < cnt; idx += 2) {
                OCFRunLoopRef value = (OCFRunLoopRef)OCFArrayGetValueAtIndex(rlArray, idx);
                OCFStringRef currentMode = OCFRunLoopCopyCurrentMode(value);
                if (currentMode != NULL) {
                    if (OCFEqual(currentMode, OCFArrayGetValueAtIndex(rlArray, idx + 1)) &&
                        OCFRunLoopIsWaiting(value)) {
                        OCFRelease(currentMode);
                        rl = value;
                        found = rl != NULL;
                        break;
                    }
                    OCFRelease(currentMode);
                }
            }
            if (!found) {
                rl = (OCFRunLoopRef)OCFArrayGetValueAtIndex(rlArray, 0);
            }
        }
    }
    if (rl != NULL) {
        OCFRunLoopWakeUp(rl);
    }
    OCFRelease(rlArray);
}

// Native CFReadStreamGetStatus, 0x1c01ffbe4. External; bridged streams answer -streamStatus.
OCFStreamStatus OCFReadStreamGetStatus(OCFReadStreamRef stream) {
    if (isObjCReadStream(stream)) {
        return ((OCFStreamStatus (*)(id, SEL))objc_msgSend)((id)stream, sel_getUid("streamStatus"));
    }
    return _CFStreamGetStatus((StreamObject *)stream);
}

// Native CFWriteStreamGetStatus, 0x1c01ffc28. External; bridged streams answer -streamStatus.
OCFStreamStatus OCFWriteStreamGetStatus(OCFWriteStreamRef stream) {
    if (isObjCWriteStream(stream)) {
        return ((OCFStreamStatus (*)(id, SEL))objc_msgSend)((id)stream, sel_getUid("streamStatus"));
    }
    return _CFStreamGetStatus((StreamObject *)stream);
}

// Native CFReadStreamCopyError, 0x1c01ffd38 (corpus delta capture 7). External; bridged streams answer
// -streamError, retained (objc_retain, a tail call); otherwise _CFStreamCopyError (a tail call).
OCFErrorRef OCFReadStreamCopyError(OCFReadStreamRef stream) {
    if (isObjCReadStream(stream)) {
        return (OCFErrorRef)objc_retain(
            ((id (*)(id, SEL))objc_msgSend)((id)stream, sel_getUid("streamError"))
        );
    }
    return _CFStreamCopyError((StreamObject *)stream);
}

// Native _CFStreamCopyError, 0x1c01ffd80. Non-external. Callbacks above version 1 hold a CFError, which is
// retained; otherwise the stored stream error is converted.
static OCFErrorRef _CFStreamCopyError(StreamObject *stream) {
    if (stream->error == NULL) {
        return NULL;
    }
    if (stream->callBacks->version > 1) {
        OCFRetain(stream->error);
        return stream->error;
    }
    return _CFStreamCreateErrorFromStreamError(OCFGetAllocator(stream), stream->error);
}

// Native CFWriteStreamCopyError, 0x1c01ffdd4. External; bridged streams answer -streamError, retained.
OCFErrorRef OCFWriteStreamCopyError(OCFWriteStreamRef stream) {
    if (isObjCWriteStream(stream)) {
        return (OCFErrorRef)objc_retain(
            ((id (*)(id, SEL))objc_msgSend)((id)stream, sel_getUid("streamError"))
        );
    }
    return _CFStreamCopyError((StreamObject *)stream);
}

// Native _CFStreamOpen, 0x1c01ffe1c. Hidden (was private external). Only an unopened stream opens: the status
// becomes opening (unless closed or in error) with the call in progress; the open callback reports success
// and completion (version 0 and 1 errors copied when nonzero). A completed failure sets the error status and
// signals the error; a completed success (or no callback) opens an opening stream and signals the open
// completion.
Boolean _CFStreamOpen(StreamObject *stream) {
    const StreamCallBacks *cb = stream->callBacks;
    if (_CFStreamGetStatus(stream) != kOCFStreamStatusNotOpen) {
        return false;
    }
    Boolean success;
    Boolean openComplete = false;
    uint64_t flags = stream->flags;
    if ((flags & 0x1E) == 6) {
        flags = flags | kStreamCallingClient;
    } else {
        flags = ((flags & 0xFFFFFFA0) | kStreamCallingClient) + kOCFStreamStatusOpening;
    }
    stream->flags = flags;
    if (cb->open != NULL) {
        if (cb->version > 1) {
            success = cb->open(stream, &stream->error, &openComplete, stream->info);
        } else {
            OCFStreamError err = {0, 0};
            success = cb->open(stream, &err, &openComplete, stream->info);
            if (err.error != 0) {
                *streamErrorStorage(stream) = err;
            }
        }
        if (!openComplete) {
            stream->flags &= ~(uint64_t)kStreamCallingClient;
            return success;
        }
        flags = stream->flags;
        if (!success) {
            if ((stream->flags & kStreamStatusMask) != kOCFStreamStatusError) {
                setStatus(stream, kOCFStreamStatusError);
            }
            if (stream->client != NULL && (stream->client->when & kOCFStreamEventErrorOccurred) != 0) {
                signalClientSource(stream, kOCFStreamEventErrorOccurred);
            }
            stream->flags &= ~(uint64_t)kStreamCallingClient;
            return false;
        }
    } else {
        success = true;
        openComplete = true;
    }
    if ((flags & kStreamStatusMask) == kOCFStreamStatusOpening) {
        stream->flags = (flags & 0xFFFFFFE0) | kOCFStreamStatusOpen;
    }
    if (stream->client != NULL && (stream->client->when & kOCFStreamEventOpenCompleted) != 0) {
        signalClientSource(stream, kOCFStreamEventOpenCompleted);
    }
    stream->flags &= ~(uint64_t)kStreamCallingClient;
    return success;
}

// Native CFReadStreamOpen, 0x1c0200070. External; bridged streams are sent -open and report success.
Boolean OCFReadStreamOpen(OCFReadStreamRef stream) {
    if (!isObjCReadStream(stream)) {
        return _CFStreamOpen((StreamObject *)stream);
    }
    ((void (*)(id, SEL))objc_msgSend)((id)stream, sel_getUid("open"));
    return true;
}

// Native CFWriteStreamOpen, 0x1c02000bc. External; bridged streams are sent -open and report success.
Boolean OCFWriteStreamOpen(OCFWriteStreamRef stream) {
    if (!isObjCWriteStream(stream)) {
        return _CFStreamOpen((StreamObject *)stream);
    }
    ((void (*)(id, SEL))objc_msgSend)((id)stream, sel_getUid("open"));
    return true;
}

// Native CFReadStreamClose, 0x1c0200108. External; bridged streams are sent -close.
void OCFReadStreamClose(OCFReadStreamRef stream) {
    if (isObjCReadStream(stream)) {
        ((void (*)(id, SEL))objc_msgSend)((id)stream, sel_getUid("close"));
        return;
    }
    _CFStreamClose((StreamObject *)stream);
}

// Native CFWriteStreamClose, 0x1c020014c. External; bridged streams are sent -close.
void OCFWriteStreamClose(OCFWriteStreamRef stream) {
    if (isObjCWriteStream(stream)) {
        ((void (*)(id, SEL))objc_msgSend)((id)stream, sel_getUid("close"));
        return;
    }
    _CFStreamClose((StreamObject *)stream);
}

// Native CFReadStreamHasBytesAvailable, 0x1c0200190 (corpus delta capture 7). External; bridged streams
// answer -hasBytesAvailable. Streams neither open nor reading answer false; without a can-read callback,
// true. Otherwise the callback runs with the call in progress: versions 0 and 1 take only the stream and the
// info (no error); later versions take the stream's error slot, and an error there sets the error status
// (unless already set) and signals it to a client that asked. The call-in-progress bit is then cleared.
Boolean OCFReadStreamHasBytesAvailable(OCFReadStreamRef readStream) {
    if (isObjCReadStream(readStream)) {
        return ((Boolean (*)(id, SEL))objc_msgSend)((id)readStream, sel_getUid("hasBytesAvailable"));
    }
    StreamObject *stream = (StreamObject *)readStream;
    OCFStreamStatus status = _CFStreamGetStatus(stream);
    if ((uint64_t)(status - kOCFStreamStatusWriting) < (uint64_t)-2) {
        return false;
    }
    const StreamCallBacks *cb = stream->callBacks;
    if (cb->canRead == NULL) {
        return true;
    }
    stream->flags |= kStreamCallingClient;
    Boolean result;
    if (cb->version > 1) {
        result = ((Boolean (*)(void *, void *, void *))cb->canRead)(stream, &stream->error, stream->info);
        if (stream->error != NULL) {
            if ((stream->flags & kStreamStatusMask) != kOCFStreamStatusError) {
                setStatus(stream, kOCFStreamStatusError);
            }
            if (stream->client != NULL && (stream->client->when & kOCFStreamEventErrorOccurred) != 0) {
                signalClientSource(stream, kOCFStreamEventErrorOccurred);
            }
        }
    } else {
        result = ((Boolean (*)(void *, void *))cb->canRead)(stream, stream->info);
    }
    stream->flags &= ~(uint64_t)kStreamCallingClient;
    return result;
}

// Native CFReadStreamRead, 0x1c02002e8. External; bridged streams are sent -read:maxLength:. An opening
// stream waits for the open (call in progress meanwhile). Streams neither open nor reading return 0 at end,
// else -1, without touching the flags. Otherwise the call-in-progress bit is set, the has-bytes-available
// signal cleared and, unless closed or at end ((flags & 0x1E) == 6, from the flags read before), the status
// becomes reading; the read callback runs with an end-of-file flag (version 0 and 1 errors copied when
// nonzero). An error sets the error status (unless already set), signals it to a client that asked, and
// returns -1; an end of file sets the at-end status and signals the end; otherwise the status returns to open
// (neither when closed or at end). The call-in-progress bit is then cleared.
OCFIndex OCFReadStreamRead(OCFReadStreamRef readStream, UInt8 *buffer, OCFIndex bufferLength) {
    if (isObjCReadStream(readStream)) {
        return ((OCFIndex (*)(id, SEL, UInt8 *, OCFIndex))objc_msgSend)(
            (id)readStream, sel_getUid("read:maxLength:"), buffer, bufferLength
        );
    }
    StreamObject *stream = (StreamObject *)readStream;
    OCFStreamStatus status = _CFStreamGetStatus(stream);
    const StreamCallBacks *cb = stream->callBacks;
    if (status == kOCFStreamStatusOpening) {
        stream->flags |= kStreamCallingClient;
        waitForOpen(stream);
        stream->flags &= ~(uint64_t)kStreamCallingClient;
        status = _CFStreamGetStatus(stream);
    }
    if ((uint64_t)(status - kOCFStreamStatusOpen) >= 2) {
        return status != kOCFStreamStatusAtEnd ? -1 : 0;
    }
    Boolean atEOF = false;
    uint64_t flags = stream->flags;
    stream->flags = flags | kStreamCallingClient;
    if (stream->client != NULL) {
        stream->client->whatToSignal &= ~(OCFOptionFlags)kOCFStreamEventHasBytesAvailable;
    }
    if ((flags & 0x1E) != 6) {
        stream->flags = ((flags | kStreamCallingClient) & 0xFFFFFFE0) | kOCFStreamStatusReading;
    }
    OCFIndex (*read)(void *, UInt8 *, OCFIndex, void *, Boolean *, void *) =
        (OCFIndex (*)(void *, UInt8 *, OCFIndex, void *, Boolean *, void *))cb->read;
    OCFIndex result;
    if (cb->version > 1) {
        result = read(stream, buffer, bufferLength, &stream->error, &atEOF, stream->info);
    } else {
        OCFStreamError err = {0, 0};
        result = read(stream, buffer, bufferLength, &err, &atEOF, stream->info);
        if (err.error != 0) {
            *streamErrorStorage(stream) = err;
        }
    }
    if (stream->error != NULL) {
        if ((stream->flags & kStreamStatusMask) != kOCFStreamStatusError) {
            setStatus(stream, kOCFStreamStatusError);
        }
        if (stream->client != NULL && (stream->client->when & kOCFStreamEventErrorOccurred) != 0) {
            signalClientSource(stream, kOCFStreamEventErrorOccurred);
        }
        result = -1;
    } else if (atEOF) {
        if ((stream->flags & 0x1E) != 6) {
            setStatus(stream, kOCFStreamStatusAtEnd);
        }
        if (stream->client != NULL && (stream->client->when & kOCFStreamEventEndEncountered) != 0) {
            signalClientSource(stream, kOCFStreamEventEndEncountered);
        }
    } else {
        if ((stream->flags & 0x1E) != 6) {
            setStatus(stream, kOCFStreamStatusOpen);
        }
    }
    stream->flags &= ~(uint64_t)kStreamCallingClient;
    return result;
}

// Native waitForOpen, 0x1c0200614. Non-external. The stream is scheduled on the current run loop in
// "_kCFStreamBlockingOpenMode", which runs (1e20 seconds, returning after a handled source) while the status
// is opening; the stream is then unscheduled (a tail call).
static void waitForOpen(StreamObject *stream) {
    OCFRunLoopRef runLoop = OCFRunLoopGetCurrent();
    _CFStreamScheduleWithRunLoop(stream, runLoop, OF_STR("_kCFStreamBlockingOpenMode"));
    while ((stream->flags & kStreamStatusMask) == kOCFStreamStatusOpening) {
        OCFRunLoopRunInMode(OF_STR("_kCFStreamBlockingOpenMode"), 1e20, true);
    }
    _CFStreamUnscheduleFromRunLoop(stream, runLoop, OF_STR("_kCFStreamBlockingOpenMode"));
}

// Native CFReadStreamGetBuffer, 0x1c02006ac (corpus delta capture 7). External. Bridged streams answer the
// buffer of -getBuffer:length: (NULL when it answers NO; numBytesRead is passed through). The callbacks are
// read before an opening stream waits for the open (call in progress meanwhile). A stream neither open nor
// reading stores 0 at end and -1 otherwise, and answers NULL; without a get-buffer callback 0 is stored and
// NULL answered. Otherwise the call-in-progress bit is set, a pending has-bytes-available signal is taken
// back (remembered), and unless closed or at end ((flags & 0x1E) == 6) the status becomes reading; the
// callback runs with an end-of-file flag (version 0 and 1 errors copied when nonzero). An error stores -1,
// sets the error status (unless set), signals it to a client that asked and answers NULL; an end of file
// sets the at-end status (unless closed or at end) and signals the end; otherwise, when the signal was taken
// back and no buffer came, it is put back, and the status returns to open (unless closed or at end). The
// call-in-progress bit is then cleared.
const UInt8 *
OCFReadStreamGetBuffer(OCFReadStreamRef readStream, OCFIndex maxBytesToRead, OCFIndex *numBytesRead) {
    if (isObjCReadStream(readStream)) {
        const UInt8 *buffer = NULL;
        BOOL success = ((BOOL (*)(id, SEL, const UInt8 **, OCFIndex *))objc_msgSend)(
            (id)readStream, sel_getUid("getBuffer:length:"), &buffer, numBytesRead
        );
        return success ? buffer : NULL;
    }
    StreamObject *stream = (StreamObject *)readStream;
    OCFStreamStatus status = _CFStreamGetStatus(stream);
    const StreamCallBacks *cb = stream->callBacks;
    if (status == kOCFStreamStatusOpening) {
        stream->flags |= kStreamCallingClient;
        waitForOpen(stream);
        stream->flags &= ~(uint64_t)kStreamCallingClient;
        status = _CFStreamGetStatus(stream);
    }
    if ((uint64_t)(status - kOCFStreamStatusOpen) >= 2) {
        *numBytesRead = status == kOCFStreamStatusAtEnd ? 0 : -1;
        return NULL;
    }
    const UInt8 *(*getBuffer)(void *, OCFIndex, OCFIndex *, void *, Boolean *, void *) =
        (const UInt8 *(*)(void *, OCFIndex, OCFIndex *, void *, Boolean *, void *))cb->getBuffer;
    if (getBuffer == NULL) {
        *numBytesRead = 0;
        return NULL;
    }
    Boolean atEOF = false;
    bool hadBytesSignal = false;
    StreamClient *client = stream->client;
    stream->flags |= kStreamCallingClient;
    if (client != NULL && (client->whatToSignal & kOCFStreamEventHasBytesAvailable) != 0) {
        client->whatToSignal &= ~(OCFOptionFlags)kOCFStreamEventHasBytesAvailable;
        hadBytesSignal = true;
    }
    if ((stream->flags & 0x1E) != 6) {
        setStatus(stream, kOCFStreamStatusReading);
    }
    const UInt8 *result;
    if (cb->version > 1) {
        result = getBuffer(stream, maxBytesToRead, numBytesRead, &stream->error, &atEOF, stream->info);
    } else {
        OCFStreamError err = {0, 0};
        result = getBuffer(stream, maxBytesToRead, numBytesRead, &err, &atEOF, stream->info);
        if (err.error != 0) {
            *streamErrorStorage(stream) = err;
        }
    }
    if (stream->error != NULL) {
        *numBytesRead = -1;
        if ((stream->flags & kStreamStatusMask) != kOCFStreamStatusError) {
            setStatus(stream, kOCFStreamStatusError);
        }
        if (stream->client != NULL && (stream->client->when & kOCFStreamEventErrorOccurred) != 0) {
            signalClientSource(stream, kOCFStreamEventErrorOccurred);
        }
        result = NULL;
    } else if (atEOF) {
        if ((stream->flags & 0x1E) != 6) {
            setStatus(stream, kOCFStreamStatusAtEnd);
        }
        if (stream->client != NULL && (stream->client->when & kOCFStreamEventEndEncountered) != 0) {
            signalClientSource(stream, kOCFStreamEventEndEncountered);
        }
    } else {
        if (hadBytesSignal && result == NULL) {
            stream->client->whatToSignal |= kOCFStreamEventHasBytesAvailable;
        }
        if ((stream->flags & 0x1E) != 6) {
            setStatus(stream, kOCFStreamStatusOpen);
        }
    }
    stream->flags &= ~(uint64_t)kStreamCallingClient;
    return result;
}

// Native CFWriteStreamCanAcceptBytes, 0x1c0200a40 (corpus delta capture 7). External; bridged streams answer
// -hasSpaceAvailable. Streams neither writing nor open answer false; without a can-write callback, true.
// Otherwise as CFReadStreamHasBytesAvailable with the can-write callback.
Boolean OCFWriteStreamCanAcceptBytes(OCFWriteStreamRef writeStream) {
    if (isObjCWriteStream(writeStream)) {
        return ((Boolean (*)(id, SEL))objc_msgSend)((id)writeStream, sel_getUid("hasSpaceAvailable"));
    }
    StreamObject *stream = (StreamObject *)writeStream;
    OCFStreamStatus status = _CFStreamGetStatus(stream);
    if (status != kOCFStreamStatusWriting && status != kOCFStreamStatusOpen) {
        return false;
    }
    const StreamCallBacks *cb = stream->callBacks;
    if (cb->canWrite == NULL) {
        return true;
    }
    stream->flags |= kStreamCallingClient;
    Boolean result;
    if (cb->version > 1) {
        result = ((Boolean (*)(void *, void *, void *))cb->canWrite)(stream, &stream->error, stream->info);
        if (stream->error != NULL) {
            if ((stream->flags & kStreamStatusMask) != kOCFStreamStatusError) {
                setStatus(stream, kOCFStreamStatusError);
            }
            if (stream->client != NULL && (stream->client->when & kOCFStreamEventErrorOccurred) != 0) {
                signalClientSource(stream, kOCFStreamEventErrorOccurred);
            }
        }
    } else {
        result = ((Boolean (*)(void *, void *))cb->canWrite)(stream, stream->info);
    }
    stream->flags &= ~(uint64_t)kStreamCallingClient;
    return result;
}

// Native CFWriteStreamWrite, 0x1c0200b9c. External; bridged streams are sent -write:maxLength:. An opening
// stream waits for the open (call in progress meanwhile). Open or writing streams write (status writing and
// call in progress, the can-accept-bytes signal cleared; version 0 and 1 errors copied when nonzero): an
// error sets the error status and signals it; a nonzero count reopens the status; zero bytes mean at end and
// signal the end. Other states return -1.
OCFIndex OCFWriteStreamWrite(OCFWriteStreamRef writeStream, const UInt8 *buffer, OCFIndex bufferLength) {
    if (isObjCWriteStream(writeStream)) {
        return ((OCFIndex (*)(id, SEL, const UInt8 *, OCFIndex))objc_msgSend)(
            (id)writeStream, sel_getUid("write:maxLength:"), buffer, bufferLength
        );
    }
    StreamObject *stream = (StreamObject *)writeStream;
    OCFStreamStatus status = _CFStreamGetStatus(stream);
    const StreamCallBacks *cb = stream->callBacks;
    if (status == kOCFStreamStatusOpening) {
        stream->flags |= kStreamCallingClient;
        waitForOpen(stream);
        stream->flags &= ~(uint64_t)kStreamCallingClient;
        status = _CFStreamGetStatus(stream);
    }
    if (status != kOCFStreamStatusWriting && status != kOCFStreamStatusOpen) {
        return -1;
    }
    if ((stream->flags & 0x1E) == 6) {
        stream->flags = stream->flags | kStreamCallingClient;
    } else {
        stream->flags = (stream->flags & 0xFFFFFFA0) | kStreamCallingClient | kOCFStreamStatusWriting;
    }
    if (stream->client != NULL) {
        stream->client->whatToSignal &= ~(OCFOptionFlags)kOCFStreamEventCanAcceptBytes;
    }
    OCFIndex result;
    if (cb->version > 1) {
        result = cb->write(stream, buffer, bufferLength, &stream->error, stream->info);
    } else {
        OCFStreamError err = {0, 0};
        result = cb->write(stream, buffer, bufferLength, &err, stream->info);
        if (err.error != 0) {
            *streamErrorStorage(stream) = err;
        }
    }
    if (stream->error != NULL) {
        if ((stream->flags & kStreamStatusMask) != kOCFStreamStatusError) {
            setStatus(stream, kOCFStreamStatusError);
        }
        if (stream->client != NULL && (stream->client->when & kOCFStreamEventErrorOccurred) != 0) {
            signalClientSource(stream, kOCFStreamEventErrorOccurred);
        }
    } else if (result != 0) {
        if ((stream->flags & 0x1E) != 6) {
            setStatus(stream, kOCFStreamStatusOpen);
        }
    } else {
        if ((stream->flags & 0x1E) != 6) {
            setStatus(stream, kOCFStreamStatusAtEnd);
        }
        if (stream->client != NULL && (stream->client->when & kOCFStreamEventEndEncountered) != 0) {
            signalClientSource(stream, kOCFStreamEventEndEncountered);
        }
    }
    stream->flags &= ~(uint64_t)kStreamCallingClient;
    return result;
}

// Native CFReadStreamCopyProperty, 0x1c0200e84. External; bridged streams answer -propertyForKey:, retained.
// The copy property callback runs with the call in progress.
OCFTypeRef OCFReadStreamCopyProperty(OCFReadStreamRef readStream, OCFStreamPropertyKey propertyName) {
    if (isObjCReadStream(readStream)) {
        return objc_retain(((id (*)(id, SEL, OCFStreamPropertyKey))objc_msgSend)(
            (id)readStream, sel_getUid("propertyForKey:"), propertyName
        ));
    }
    StreamObject *stream = (StreamObject *)readStream;
    OCFTypeRef (*copyProperty)(void *, OCFStreamPropertyKey, void *) = stream->callBacks->copyProperty;
    if (copyProperty == NULL) {
        return NULL;
    }
    stream->flags |= kStreamCallingClient;
    OCFTypeRef result = copyProperty(stream, propertyName, stream->info);
    stream->flags &= ~(uint64_t)kStreamCallingClient;
    return result;
}

// Native CFWriteStreamCopyProperty, 0x1c0200f0c. External; bridged streams answer -propertyForKey:, retained.
// The copy property callback runs with the call in progress.
OCFTypeRef OCFWriteStreamCopyProperty(OCFWriteStreamRef writeStream, OCFStreamPropertyKey propertyName) {
    if (isObjCWriteStream(writeStream)) {
        return objc_retain(((id (*)(id, SEL, OCFStreamPropertyKey))objc_msgSend)(
            (id)writeStream, sel_getUid("propertyForKey:"), propertyName
        ));
    }
    StreamObject *stream = (StreamObject *)writeStream;
    OCFTypeRef (*copyProperty)(void *, OCFStreamPropertyKey, void *) = stream->callBacks->copyProperty;
    if (copyProperty == NULL) {
        return NULL;
    }
    stream->flags |= kStreamCallingClient;
    OCFTypeRef result = copyProperty(stream, propertyName, stream->info);
    stream->flags &= ~(uint64_t)kStreamCallingClient;
    return result;
}

// Native CFReadStreamSetProperty, 0x1c0200f94. External; bridged streams are sent -setProperty:forKey:. The
// set property callback (false without one) runs with the call in progress.
Boolean OCFReadStreamSetProperty(
    OCFReadStreamRef readStream, OCFStreamPropertyKey propertyName, OCFTypeRef propertyValue
) {
    if (isObjCReadStream(readStream)) {
        return ((Boolean (*)(id, SEL, OCFTypeRef, OCFStreamPropertyKey))objc_msgSend)(
            (id)readStream, sel_getUid("setProperty:forKey:"), propertyValue, propertyName
        );
    }
    StreamObject *stream = (StreamObject *)readStream;
    Boolean (*setProperty)(void *, OCFStreamPropertyKey, OCFTypeRef, void *) =
        (Boolean (*)(void *, OCFStreamPropertyKey, OCFTypeRef, void *))stream->callBacks->setProperty;
    if (setProperty == NULL) {
        return false;
    }
    stream->flags |= kStreamCallingClient;
    Boolean result = setProperty(stream, propertyName, propertyValue, stream->info);
    stream->flags &= ~(uint64_t)kStreamCallingClient;
    return result;
}

// Native CFWriteStreamSetProperty, 0x1c0201028 (corpus delta capture 7). External; bridged streams are sent
// -setProperty:forKey:. The set property callback (false without one) runs with the call in progress.
Boolean OCFWriteStreamSetProperty(
    OCFWriteStreamRef writeStream, OCFStreamPropertyKey propertyName, OCFTypeRef propertyValue
) {
    if (isObjCWriteStream(writeStream)) {
        return ((Boolean (*)(id, SEL, OCFTypeRef, OCFStreamPropertyKey))objc_msgSend)(
            (id)writeStream, sel_getUid("setProperty:forKey:"), propertyValue, propertyName
        );
    }
    StreamObject *stream = (StreamObject *)writeStream;
    Boolean (*setProperty)(void *, OCFStreamPropertyKey, OCFTypeRef, void *) =
        (Boolean (*)(void *, OCFStreamPropertyKey, OCFTypeRef, void *))stream->callBacks->setProperty;
    if (setProperty == NULL) {
        return false;
    }
    stream->flags |= kStreamCallingClient;
    Boolean result = setProperty(stream, propertyName, propertyValue, stream->info);
    stream->flags &= ~(uint64_t)kStreamCallingClient;
    return result;
}

// Native _CFStreamSetClient, 0x1c02010bc (corpus delta capture 7). Hidden (was private external). No events,
// no callback or no context clears the client (callback and events NULL and 0). Without a client record a
// clear answers true at once; otherwise the record is created (80 bytes, typed 0x10A00402EE2ECAD, zeroed;
// NULL answers false). An existing callback's context info is released (when the release callback is set).
// The callback is stored; a clear zeroes the info, retain, release and description words, otherwise the
// context's version, retain, release and description are copied and its info retained (when both the retain
// callback and the info are set). When the events change they are stored and the request-events callback
// (when set) receives them. True is answered.
Boolean _CFStreamSetClient(
    StreamObject *stream,
    OCFOptionFlags streamEvents,
    void (*clientCB)(void *stream, OCFStreamEventType type, void *info),
    OCFStreamClientContext *clientContext
) {
    bool clear = streamEvents == 0 || clientCB == NULL || clientContext == NULL;
    void (*callback)(void *, OCFStreamEventType, void *) = clear ? NULL : clientCB;
    OCFOptionFlags events = clear ? 0 : streamEvents;
    StreamClient *client = stream->client;
    if (client == NULL) {
        if (clear) {
            return true;
        }
        client = OCFAllocatorAllocateTyped(OCFGetAllocator(stream), 0x50, 0x10A00402EE2ECAD, 0);
        stream->client = client;
        memset(client, 0, sizeof(StreamClient));
        client = stream->client;
        if (client == NULL) {
            return false;
        }
    }
    if (client->callback != NULL && client->release != NULL) {
        client->release(client->info);
        client = stream->client;
    }
    client->callback = callback;
    if (clear) {
        client->release = NULL;
        client->copyDescription = NULL;
        client->info = NULL;
        client->retain = NULL;
    } else {
        void *(*retain)(void *) = clientContext->retain;
        client->retain = clientContext->retain;
        client->release = clientContext->release;
        void *info = clientContext->info;
        client->version = clientContext->version;
        client->copyDescription = clientContext->copyDescription;
        if (retain != NULL && info != NULL) {
            info = retain(info);
            client = stream->client;
        }
        client->info = info;
    }
    if (client->when != events) {
        const StreamCallBacks *cb = stream->callBacks;
        client->when = events;
        void (*requestEvents)(void *, OCFOptionFlags, void *) =
            (void (*)(void *, OCFOptionFlags, void *))cb->requestEvents;
        if (requestEvents != NULL) {
            requestEvents(stream, events, stream->info);
        }
    }
    return true;
}

// Native CFReadStreamSetClient, 0x1c020123c (corpus delta capture 7). External. A CF stream sets its client
// without kCFStreamEventCanAcceptBytes (a tail call of _CFStreamSetClient). A bridged stream answering
// +respondsToSelector: for the selector at 0x2387ac9ff (outside the image; read as
// _setCFClientFlags:callback:context:, the message that follows; confidence: medium) receives that message
// and answers its result; otherwise a callback makes a new _CFStreamDelegate
// (-initWithStreamEvents:callback:context:), sets it as the stream's delegate and associates it with the
// stream under the class as key (OBJC_ASSOCIATION_RETAIN, 0x301) before releasing it, and no callback sets a
// nil delegate; true is answered.
Boolean OCFReadStreamSetClient(
    OCFReadStreamRef stream,
    OCFOptionFlags streamEvents,
    OCFReadStreamClientCallBack clientCB,
    OCFStreamClientContext *clientContext
) {
    if (!isObjCReadStream(stream)) {
        return _CFStreamSetClient(
            (StreamObject *)stream,
            streamEvents & ~(OCFOptionFlags)kOCFStreamEventCanAcceptBytes,
            (void (*)(void *, OCFStreamEventType, void *))clientCB,
            clientContext
        );
    }
    if (objc_opt_respondsToSelector((id)stream, sel_getUid("_setCFClientFlags:callback:context:"))) {
        return ((Boolean (*)(id, SEL, OCFOptionFlags, OCFReadStreamClientCallBack, OCFStreamClientContext *))
                    objc_msgSend)(
            (id)stream,
            sel_getUid("_setCFClientFlags:callback:context:"),
            streamEvents,
            clientCB,
            clientContext
        );
    }
    if (clientCB == NULL) {
        ((void (*)(id, SEL, id))objc_msgSend)((id)stream, sel_getUid("setDelegate:"), nil);
        return true;
    }
    id delegate = ((id (*)(id, SEL, OCFOptionFlags, OCFReadStreamClientCallBack, OCFStreamClientContext *))
                       objc_msgSend)(
        objc_alloc((Class)streamDelegateClass),
        sel_getUid("initWithStreamEvents:callback:context:"),
        streamEvents,
        clientCB,
        clientContext
    );
    ((void (*)(id, SEL, id))objc_msgSend)((id)stream, sel_getUid("setDelegate:"), delegate);
    objc_setAssociatedObject(
        (id)stream, (const void *)objc_opt_class((id)streamDelegateClass), delegate, OBJC_ASSOCIATION_RETAIN
    );
    objc_release(delegate);
    return true;
}

// Native CFWriteStreamSetClient, 0x1c020133c (corpus delta capture 7). External. As CFReadStreamSetClient for
// the write stream type (39), a CF stream's events without kCFStreamEventHasBytesAvailable.
Boolean OCFWriteStreamSetClient(
    OCFWriteStreamRef stream,
    OCFOptionFlags streamEvents,
    OCFWriteStreamClientCallBack clientCB,
    OCFStreamClientContext *clientContext
) {
    if (!isObjCWriteStream(stream)) {
        return _CFStreamSetClient(
            (StreamObject *)stream,
            streamEvents & ~(OCFOptionFlags)kOCFStreamEventHasBytesAvailable,
            (void (*)(void *, OCFStreamEventType, void *))clientCB,
            clientContext
        );
    }
    if (objc_opt_respondsToSelector((id)stream, sel_getUid("_setCFClientFlags:callback:context:"))) {
        return ((Boolean (*)(id, SEL, OCFOptionFlags, OCFWriteStreamClientCallBack, OCFStreamClientContext *))
                    objc_msgSend)(
            (id)stream,
            sel_getUid("_setCFClientFlags:callback:context:"),
            streamEvents,
            clientCB,
            clientContext
        );
    }
    if (clientCB == NULL) {
        ((void (*)(id, SEL, id))objc_msgSend)((id)stream, sel_getUid("setDelegate:"), nil);
        return true;
    }
    id delegate = ((id (*)(id, SEL, OCFOptionFlags, OCFWriteStreamClientCallBack, OCFStreamClientContext *))
                       objc_msgSend)(
        objc_alloc((Class)streamDelegateClass),
        sel_getUid("initWithStreamEvents:callback:context:"),
        streamEvents,
        clientCB,
        clientContext
    );
    ((void (*)(id, SEL, id))objc_msgSend)((id)stream, sel_getUid("setDelegate:"), delegate);
    objc_setAssociatedObject(
        (id)stream, (const void *)objc_opt_class((id)streamDelegateClass), delegate, OBJC_ASSOCIATION_RETAIN
    );
    objc_release(delegate);
    return true;
}

// Native _CFReadStreamGetClient, 0x1c020143c (corpus delta capture 7). External. The client's info, or NULL
// without a client record.
void *_OCFReadStreamGetClient(OCFReadStreamRef readStream) {
    StreamClient *client = ((StreamObject *)readStream)->client;
    return client != NULL ? client->info : NULL;
}

// Native _CFWriteStreamGetClient, 0x1c0201454 (corpus delta capture 7). External. As _CFReadStreamGetClient.
void *_OCFWriteStreamGetClient(OCFWriteStreamRef writeStream) {
    StreamClient *client = ((StreamObject *)writeStream)->client;
    return client != NULL ? client->info : NULL;
}

// Native _CFStreamScheduleWithRunLoop, 0x1c020146c. Hidden (was private external). The client record is
// created on first use (80 zeroed bytes). Without a source the stream joins the shared source of its {run
// loop, mode} (created with the CFType callbacks, the stream list as info and
// _cfstream_shared_signalEventSync as perform); with a shared source a second schedule moves the stream to a
// source of its own (CFCopyDescription, _cfstream_solo_signalEventSync) added to the old and the new run loop
// and mode; an own source is added to the run loop. The pair is recorded, the schedule callback runs with the
// call in progress and pending events are signalled.
void _CFStreamScheduleWithRunLoop(StreamObject *stream, OCFRunLoopRef runLoop, OCFStringRef runLoopMode) {
    const StreamCallBacks *cb = stream->callBacks;
    if (stream->client == NULL) {
        StreamClient *client = OCFAllocatorAllocateTyped(OCFGetAllocator(stream), 80, 0x10A00402EE2ECAD, 0);
        stream->client = client;
        memset(client, 0, sizeof(StreamClient));
        if (stream->client == NULL) {
            return;
        }
    }
    if (stream->client->rlSource == NULL) {
        const void *values[2] = {runLoop, runLoopMode};
        OCFArrayRef key = OCFArrayCreate(kOCFAllocatorSystemDefault, values, 2, &kOCFTypeArrayCallBacks);
        os_unfair_lock_lock(&sSourceLock);
        if (sSharedSources == NULL) {
            sSharedSources = OCFDictionaryCreateMutable(
                kOCFAllocatorSystemDefault,
                0,
                &kOCFTypeDictionaryKeyCallBacks,
                &kOCFTypeDictionaryValueCallBacks
            );
        }
        OCFMutableArrayRef list = (OCFMutableArrayRef)OCFDictionaryGetValue(sSharedSources, key);
        if (list == NULL) {
            OCFRunLoopSourceContext ctxt = {
                0,
                NULL,
                (const void *(*)(const void *))OCFRetain,
                (void (*)(const void *))OCFRelease,
                (OCFStringRef (*)(const void *))OCFCopyDescription,
                NULL,
                NULL,
                NULL,
                NULL,
                _cfstream_shared_signalEventSync,
            };
            list = OCFArrayCreateMutable(kOCFAllocatorSystemDefault, 0, &kOCFTypeArrayCallBacks);
            OCFDictionaryAddValue(sSharedSources, key, list);
            ctxt.info = list;
            OCFRunLoopSourceRef source = OCFRunLoopSourceCreate(kOCFAllocatorSystemDefault, 0, &ctxt);
            setClientSource(stream, source);
            OCFRunLoopAddSource(runLoop, source, runLoopMode);
            OCFRelease(source);
        } else {
            StreamObject *first = (StreamObject *)OCFArrayGetValueAtIndex(list, 0);
            if (first != NULL) {
                pthread_mutex_lock(&first->lock);
                OCFRunLoopSourceRef source = first->client != NULL ? first->client->rlSource : NULL;
                if (source != NULL) {
                    OCFRetain(source);
                    pthread_mutex_unlock(&first->lock);
                    setClientSource(stream, source);
                    OCFRelease(source);
                } else {
                    pthread_mutex_unlock(&first->lock);
                }
            }
            OCFRetain(list);
        }
        OCFArrayAppendValue(list, stream);
        OCFDictionaryAddValue(sSharedSources, stream, key);
        OCFRelease(key);
        OCFRelease(list);
        stream->flags |= kStreamSharedSource;
        os_unfair_lock_unlock(&sSourceLock);
    } else if ((stream->flags & kStreamSharedSource) != 0) {
        OCFAllocatorRef allocator = OCFGetAllocator(stream);
        OCFRunLoopSourceContext ctxt = {
            0,
            stream,
            NULL,
            NULL,
            (OCFStringRef (*)(const void *))OCFCopyDescription,
            NULL,
            NULL,
            NULL,
            NULL,
            _cfstream_solo_signalEventSync,
        };
        os_unfair_lock_lock(&sSourceLock);
        OCFArrayRef key = OCFRetain(OCFDictionaryGetValue(sSharedSources, stream));
        removeFromSharedSource(stream, NULL, NULL);
        os_unfair_lock_unlock(&sSourceLock);
        OCFRunLoopSourceRef source = OCFRunLoopSourceCreate(allocator, 0, &ctxt);
        setClientSource(stream, source);
        OCFRunLoopAddSource(
            (OCFRunLoopRef)OCFArrayGetValueAtIndex(key, 0), source, OCFArrayGetValueAtIndex(key, 1)
        );
        OCFRelease(key);
        OCFRunLoopAddSource(runLoop, source, runLoopMode);
        OCFRelease(source);
    } else {
        pthread_mutex_lock(&stream->lock);
        OCFRunLoopSourceRef source = stream->client != NULL ? stream->client->rlSource : NULL;
        if (source != NULL) {
            OCFRetain(source);
            pthread_mutex_unlock(&stream->lock);
            OCFRunLoopAddSource(runLoop, source, runLoopMode);
            OCFRelease(source);
        } else {
            pthread_mutex_unlock(&stream->lock);
        }
    }
    pthread_mutex_lock(&stream->lock);
    if (stream->client->runLoopsAndModes == NULL) {
        stream->client->runLoopsAndModes =
            OCFArrayCreateMutable(OCFGetAllocator(stream), 0, &kOCFTypeArrayCallBacks);
    }
    OCFArrayAppendValue(stream->client->runLoopsAndModes, runLoop);
    OCFArrayAppendValue(stream->client->runLoopsAndModes, runLoopMode);
    pthread_mutex_unlock(&stream->lock);
    if (cb != NULL && cb->schedule != NULL) {
        stream->flags |= kStreamCallingClient;
        cb->schedule(stream, runLoop, runLoopMode, stream->info);
        stream->flags &= ~(uint64_t)kStreamCallingClient;
    }
    if (stream->client != NULL && stream->client->whatToSignal != 0) {
        pthread_mutex_lock(&stream->lock);
        OCFRunLoopSourceRef source = stream->client != NULL ? stream->client->rlSource : NULL;
        if (source != NULL) {
            OCFRetain(source);
            pthread_mutex_unlock(&stream->lock);
            OCFRunLoopSourceSignal(source);
            OCFRelease(source);
            _wakeUpRunLoop(stream);
        } else {
            pthread_mutex_unlock(&stream->lock);
        }
    }
}

// Native _cfstream_shared_signalEventSync, 0x1c0201b10. Non-external; the perform callback of a shared run
// loop source, whose info is the array of its streams. Anything but an array logs "Expected an array for %p"
// (level 3). Under sSourceLock the first stream that is not being delivered and has events to signal (its
// client is read without a NULL test) is retained with its queue and marked; the next such stream after it
// has its client's source signalled (retained under that stream's lock), so the remaining streams are
// served by another perform. After the unlock the chosen stream's events are delivered on its queue
// (asynchronously) or at once, and it is released.
static void _cfstream_shared_signalEventSync(void *info) {
    OCFArrayRef list = info;
    if (OCFGetTypeID(list) != OCFArrayGetTypeID()) {
        OCFLog(3, OF_STR("Expected an array for %p"), list);
        return;
    }
    os_unfair_lock_lock(&sSourceLock);
    OCFIndex count = OCFArrayGetCount(list);
    OCFIndex index = 0;
    StreamObject *stream = NULL;
    dispatch_queue_t queue = NULL;
    if (count >= 1) {
        for (;;) {
            StreamObject *candidate = (StreamObject *)OCFArrayGetValueAtIndex(list, index);
            if (!candidate->signalling && candidate->client->whatToSignal != 0) {
                stream = candidate;
                break;
            }
            ++index;
            if (index == count) {
                os_unfair_lock_unlock(&sSourceLock);
                return;
            }
        }
        OCFRetain(stream);
        queue = stream->queue;
        if (queue != NULL) {
            dispatch_retain(queue);
        }
        stream->signalling = true;
    }
    for (; index < count; ++index) {
        StreamObject *other = (StreamObject *)OCFArrayGetValueAtIndex(list, index);
        if (other == stream || other->signalling || other->client->whatToSignal == 0) {
            continue;
        }
        pthread_mutex_lock(&other->lock);
        StreamClient *client = other->client;
        OCFRunLoopSourceRef source = client != NULL ? client->rlSource : NULL;
        if (source != NULL) {
            OCFRetain(source);
            pthread_mutex_unlock(&other->lock);
            OCFRunLoopSourceSignal(source);
            OCFRelease(source);
        } else {
            pthread_mutex_unlock(&other->lock);
        }
        break;
    }
    os_unfair_lock_unlock(&sSourceLock);
    if (stream == NULL) {
        return;
    }
    if (queue != NULL) {
        signalEventQueue(stream, queue);
        dispatch_release(queue);
    } else {
        _signalEventSync(stream);
    }
    OCFRelease(stream);
}

// Native CFReadStreamScheduleWithRunLoop, 0x1c0201d50 (corpus delta capture 7). External. A CF stream goes to
// _CFStreamScheduleWithRunLoop (a tail call). A bridged stream answering +respondsToSelector: for the
// selector at 0x2387ac97e (outside the image; read as _scheduleInCFRunLoop:forMode:, the message that
// follows; confidence: medium) is sent that message with the CF run loop; otherwise
// -scheduleInRunLoop:forMode: with the run loop's NSRunLoop
// (_CFRunLoopGet2). Both are tail calls.
void OCFReadStreamScheduleWithRunLoop(
    OCFReadStreamRef stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode
) {
    if (!isObjCReadStream(stream)) {
        _CFStreamScheduleWithRunLoop((StreamObject *)stream, runLoop, runLoopMode);
        return;
    }
    if (objc_opt_respondsToSelector((id)stream, sel_getUid("_scheduleInCFRunLoop:forMode:"))) {
        ((void (*)(id, SEL, OCFRunLoopRef, OCFRunLoopMode))objc_msgSend)(
            (id)stream, sel_getUid("_scheduleInCFRunLoop:forMode:"), runLoop, runLoopMode
        );
        return;
    }
    ((void (*)(id, SEL, OCFTypeRef, OCFRunLoopMode))objc_msgSend)(
        (id)stream, sel_getUid("scheduleInRunLoop:forMode:"), _OCFRunLoopGet2(runLoop), runLoopMode
    );
}

// Native CFWriteStreamScheduleWithRunLoop, 0x1c0201df0 (corpus delta capture 7). External. A CF stream goes
// to _CFStreamScheduleWithRunLoop (a tail call). A bridged stream answering +respondsToSelector: for the
// selector at 0x2387ac97e (outside the image; read as _scheduleInCFRunLoop:forMode:, the message that
// follows; confidence: medium) is sent that message with the CF run loop; otherwise
// -scheduleInRunLoop:forMode: with the run loop's NSRunLoop
// (_CFRunLoopGet2). Both are tail calls.
void OCFWriteStreamScheduleWithRunLoop(
    OCFWriteStreamRef stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode
) {
    if (!isObjCWriteStream(stream)) {
        _CFStreamScheduleWithRunLoop((StreamObject *)stream, runLoop, runLoopMode);
        return;
    }
    if (objc_opt_respondsToSelector((id)stream, sel_getUid("_scheduleInCFRunLoop:forMode:"))) {
        ((void (*)(id, SEL, OCFRunLoopRef, OCFRunLoopMode))objc_msgSend)(
            (id)stream, sel_getUid("_scheduleInCFRunLoop:forMode:"), runLoop, runLoopMode
        );
        return;
    }
    ((void (*)(id, SEL, OCFTypeRef, OCFRunLoopMode))objc_msgSend)(
        (id)stream, sel_getUid("scheduleInRunLoop:forMode:"), _OCFRunLoopGet2(runLoop), runLoopMode
    );
}

// Native _CFStreamUnscheduleFromRunLoop, 0x1c0201e90. Hidden (was private external). A scheduled source is
// removed from the run loop and mode (a shared one through the shared table, the last stream removing it from
// the given run loop and mode), the first matching pair (compared with CFEqual) is removed and the unschedule
// callback runs.
void _CFStreamUnscheduleFromRunLoop(StreamObject *stream, OCFRunLoopRef runLoop, OCFStringRef runLoopMode) {
    if (stream->client == NULL || stream->client->rlSource == NULL) {
        return;
    }
    const StreamCallBacks *cb = stream->callBacks;
    if ((stream->flags & kStreamSharedSource) == 0) {
        pthread_mutex_lock(&stream->lock);
        OCFRunLoopSourceRef source = stream->client != NULL ? stream->client->rlSource : NULL;
        if (source != NULL) {
            OCFRetain(source);
            pthread_mutex_unlock(&stream->lock);
            OCFRunLoopRemoveSource(runLoop, source, runLoopMode);
            OCFRelease(source);
        } else {
            pthread_mutex_unlock(&stream->lock);
        }
    } else {
        os_unfair_lock_lock(&sSourceLock);
        removeFromSharedSource(stream, runLoop, runLoopMode);
        os_unfair_lock_unlock(&sSourceLock);
    }
    pthread_mutex_lock(&stream->lock);
    OCFMutableArrayRef runLoopsAndModes = stream->client->runLoopsAndModes;
    if (runLoopsAndModes != NULL) {
        OCFIndex cnt = OCFArrayGetCount(runLoopsAndModes);
        if (cnt >= 2) {
            for (OCFIndex idx = 0; idx + 1 < cnt; idx += 2) {
                if (OCFEqual(OCFArrayGetValueAtIndex(runLoopsAndModes, idx), runLoop) &&
                    OCFEqual(OCFArrayGetValueAtIndex(runLoopsAndModes, idx + 1), runLoopMode)) {
                    OCFArrayRemoveValueAtIndex(runLoopsAndModes, idx);
                    OCFArrayRemoveValueAtIndex(runLoopsAndModes, idx);
                    break;
                }
            }
        }
    }
    pthread_mutex_unlock(&stream->lock);
    if (cb != NULL && cb->unschedule != NULL) {
        cb->unschedule(stream, runLoop, runLoopMode, stream->info);
    }
}

// Native CFReadStreamUnscheduleFromRunLoop, 0x1c020213c (corpus delta capture 7). External. A CF stream goes
// to _CFStreamUnscheduleFromRunLoop (a tail call). A bridged stream answering +respondsToSelector: for the
// selector at 0x2387acd81 (outside the image; read as _unscheduleFromCFRunLoop:forMode:, the message that
// follows; confidence: medium) is sent that message with the CF run loop; otherwise
// -removeFromRunLoop:forMode: with the run loop's NSRunLoop
// (_CFRunLoopGet2). Both are tail calls.
void OCFReadStreamUnscheduleFromRunLoop(
    OCFReadStreamRef stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode
) {
    if (!isObjCReadStream(stream)) {
        _CFStreamUnscheduleFromRunLoop((StreamObject *)stream, runLoop, runLoopMode);
        return;
    }
    if (objc_opt_respondsToSelector((id)stream, sel_getUid("_unscheduleFromCFRunLoop:forMode:"))) {
        ((void (*)(id, SEL, OCFRunLoopRef, OCFRunLoopMode))objc_msgSend)(
            (id)stream, sel_getUid("_unscheduleFromCFRunLoop:forMode:"), runLoop, runLoopMode
        );
        return;
    }
    ((void (*)(id, SEL, OCFTypeRef, OCFRunLoopMode))objc_msgSend)(
        (id)stream, sel_getUid("removeFromRunLoop:forMode:"), _OCFRunLoopGet2(runLoop), runLoopMode
    );
}

// Native CFWriteStreamUnscheduleFromRunLoop, 0x1c02021dc (corpus delta capture 7). External. A CF stream goes
// to _CFStreamUnscheduleFromRunLoop (a tail call). A bridged stream answering +respondsToSelector: for the
// selector at 0x2387acd81 (outside the image; read as _unscheduleFromCFRunLoop:forMode:, the message that
// follows; confidence: medium) is sent that message with the CF run loop; otherwise
// -removeFromRunLoop:forMode: with the run loop's NSRunLoop
// (_CFRunLoopGet2). Both are tail calls.
void OCFWriteStreamUnscheduleFromRunLoop(
    OCFWriteStreamRef stream, OCFRunLoopRef runLoop, OCFRunLoopMode runLoopMode
) {
    if (!isObjCWriteStream(stream)) {
        _CFStreamUnscheduleFromRunLoop((StreamObject *)stream, runLoop, runLoopMode);
        return;
    }
    if (objc_opt_respondsToSelector((id)stream, sel_getUid("_unscheduleFromCFRunLoop:forMode:"))) {
        ((void (*)(id, SEL, OCFRunLoopRef, OCFRunLoopMode))objc_msgSend)(
            (id)stream, sel_getUid("_unscheduleFromCFRunLoop:forMode:"), runLoop, runLoopMode
        );
        return;
    }
    ((void (*)(id, SEL, OCFTypeRef, OCFRunLoopMode))objc_msgSend)(
        (id)stream, sel_getUid("removeFromRunLoop:forMode:"), _OCFRunLoopGet2(runLoop), runLoopMode
    );
}

// Native _CFStreamCopyRunLoopsAndModes, 0x1c0202408. Non-external; a copy of the scheduled pairs, made under
// the stream's lock on the stream's allocator.
static OCFArrayRef _CFStreamCopyRunLoopsAndModes(StreamObject *stream) {
    if (stream == NULL || stream->client == NULL) {
        return NULL;
    }
    pthread_mutex_lock(&stream->lock);
    OCFArrayRef result = NULL;
    if (stream->client->runLoopsAndModes != NULL) {
        result = OCFArrayCreateCopy(OCFGetAllocator(stream), stream->client->runLoopsAndModes);
    }
    pthread_mutex_unlock(&stream->lock);
    return result;
}

// Native CFReadStreamSignalEvent, 0x1c020247c (delta capture 5). External; a tail call allowing
// synchronous delivery.
void OCFReadStreamSignalEvent(
    OCFReadStreamRef stream, OCFStreamEventType streamEvent, const OCFStreamError *error
) {
    _CFStreamSignalEvent((StreamObject *)stream, streamEvent, error, true);
}

// Native CFWriteStreamSignalEvent, 0x1c0202484. External; a tail call allowing synchronous delivery.
void OCFWriteStreamSignalEvent(
    OCFWriteStreamRef stream, OCFStreamEventType streamEvent, const OCFStreamError *error
) {
    _CFStreamSignalEvent((StreamObject *)stream, streamEvent, error, true);
}

// Native CFReadStreamGetInfoPointer, 0x1c02024b4 (delta capture 5). External. The stream's info, or NULL
// for a NULL stream.
void *OCFReadStreamGetInfoPointer(OCFReadStreamRef stream) {
    if (stream == NULL) {
        return NULL;
    }
    return ((StreamObject *)stream)->info;
}

// Native CFWriteStreamGetInfoPointer, 0x1c02024c0 (delta capture 5). External. As
// CFReadStreamGetInfoPointer.
void *OCFWriteStreamGetInfoPointer(OCFWriteStreamRef stream) {
    if (stream == NULL) {
        return NULL;
    }
    return ((StreamObject *)stream)->info;
}

// Native _signalEventSync, 0x1c02024d4. Non-external. The callback-in-progress bit is set (a plain
// read-modify-write) for the delivery. Under the stream's lock, a stream without a client or without events
// to signal only clears its delivery byte. Otherwise the client's callback and info are taken (the info
// passed through the retain callback when there is one, the release callback and events then read again),
// the events to signal and the delivery byte are cleared and the lock released. Each event bit from 1 up to
// the events taken is delivered, in increasing order, when it is among them and the client (re-read under
// the lock) still wants it; the callback runs outside the lock. A retained info is then released.
static void _signalEventSync(StreamObject *stream) {
    stream->flags |= kStreamCallingClient;
    pthread_mutex_lock(&stream->lock);
    StreamClient *client = stream->client;
    if (client == NULL || client->whatToSignal == 0) {
        stream->signalling = false;
        pthread_mutex_unlock(&stream->lock);
        stream->flags &= ~(uint64_t)kStreamCallingClient;
        return;
    }
    OCFOptionFlags whatToSignal = client->whatToSignal;
    void (*callback)(void *, OCFStreamEventType, void *) = client->callback;
    void *info = client->info;
    void (*release)(void *) = NULL;
    if (client->retain != NULL) {
        info = client->retain(info);
        client = stream->client;
        release = client->release;
        whatToSignal = client->whatToSignal;
    }
    client->whatToSignal = 0;
    stream->signalling = false;
    pthread_mutex_unlock(&stream->lock);
    if (whatToSignal != 0) {
        OCFOptionFlags event = 1;
        do {
            pthread_mutex_lock(&stream->lock);
            if ((event & whatToSignal) != 0 && stream->client != NULL) {
                OCFOptionFlags wanted = stream->client->when & event;
                pthread_mutex_unlock(&stream->lock);
                if (wanted != 0) {
                    callback(stream, event, info);
                }
            } else {
                pthread_mutex_unlock(&stream->lock);
            }
            event <<= 1;
        } while (event <= whatToSignal);
    }
    if (release != NULL) {
        release(info);
    }
    stream->flags &= ~(uint64_t)kStreamCallingClient;
}

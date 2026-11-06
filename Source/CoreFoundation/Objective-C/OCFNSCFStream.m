//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSCFStream.h"
#import "CoreFoundation/Objective-C/OCFNSURL.h"

// Inferred organization: the __NSCFInputStream (0x1c01b506c-0x1c01b5494) and __NSCFOutputStream
// (0x1c01b5498-0x1c01b58dc) methods lie together in the iOS Simulator __text section, after the
// __NSCFStreamWeakDelegateWrapper methods (0x1c01b4f58-0x1c01b5068, not on this track's list and absent from
// the evidence corpus) and with the two client callbacks between -setDelegate: and -propertyForKey: (restored
// from the IDA capture, iphone-simulator-corpus-delta2.json); following the owner's decision, the two stream
// classes share a translation unit. Objective-C compiled with manual reference counting (explicit -dealloc of
// the placeholder receivers, objc_release, objc_autorelease). Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full).
// iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not
// written yet).

// The class symbol of __NSCFStreamWeakDelegateWrapper (an image-local class, addressed directly natively).
extern char
    weakDelegateWrapperClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSCFStreamWeakDelegateWrapper));

// The client events -setDelegate: subscribes: open completed, bytes available, error and end (0x1B) for
// input; open completed, can accept bytes, error and end (0x1D) for output.
enum {
    kInputStreamDelegateEvents = 0x1B,
    kOutputStreamDelegateEvents = 0x1D,
};

// Native _inputStreamCallbackFunc and _outputStreamCallbackFunc; defined below, in native address order.
static void _inputStreamCallbackFunc(OCFReadStreamRef stream, OCFStreamEventType type, void *info);
static void _outputStreamCallbackFunc(OCFWriteStreamRef stream, OCFStreamEventType type, void *info);

@implementation __ONSCFInputStream

// Native -[__NSCFInputStream isEqual:], 0x1c01b506c. Non-external. nil answers NO and the receiver itself YES
// before _CFNonObjCEqual.
- (BOOL)isEqual:(id)object {
    if (object == nil) {
        return NO;
    }
    if (self == object) {
        return YES;
    }
    return _OCFNonObjCEqual(self, object) != 0;
}

// Native -[__NSCFInputStream hash], 0x1c01b50a8. Non-external.
- (NSUInteger)hash {
    return _OCFNonObjCHash(self);
}

// Native -[__NSCFInputStream retain], 0x1c01b50ac. Non-external.
- (id)retain {
    return (id)_OCFNonObjCRetain(self);
}

// Native -[__NSCFInputStream release], 0x1c01b50b0. Non-external.
- (oneway void)release {
    _OCFNonObjCRelease(self);
}

// Native -[__NSCFInputStream _tryRetain], 0x1c01b50b4. Non-external.
- (BOOL)_tryRetain {
    return _OCFTryRetain(self) != NULL;
}

// Native -[__NSCFInputStream _isDeallocating], 0x1c01b50d0. Non-external.
- (BOOL)_isDeallocating {
    return _OCFIsDeallocating(self) != 0;
}

// Native -[__NSCFInputStream retainCount], 0x1c01b50ec. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)OCFGetRetainCount(self);
}

// Native +[__NSCFInputStream automaticallyNotifiesObserversForKey:], 0x1c01b50f0. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native +[__NSCFInputStream allocWithZone:], 0x1c01b50f8. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}

// Native -[__NSCFInputStream initWithData:], 0x1c01b5114. Non-external. The receiver is sent -dealloc and a
// new CFReadStream in the system default allocator is answered.
- (id)initWithData:(id)data {
    [self dealloc];
    return (id)OCFReadStreamCreateWithData(kOCFAllocatorSystemDefault, (OCFDataRef)data);
}

// Native -[__NSCFInputStream initWithFileAtPath:], 0x1c01b5144. Non-external. Without a file URL for the path
// nil is answered and the receiver is left as is; otherwise -initWithURL: with the URL, which is released.
- (id)initWithFileAtPath:(id)path {
    id url = [[ONSURL alloc] initFileURLWithPath:path];
    if (url == nil) {
        return nil;
    }
    id stream = [self initWithURL:url];
    [url release];
    return stream;
}

// Native -[__NSCFInputStream initWithURL:], 0x1c01b5198. Non-external. The receiver is sent -dealloc; a nil
// URL answers NULL.
- (id)initWithURL:(id)url {
    [self dealloc];
    if (url == nil) {
        return nil;
    }
    return (id)OCFReadStreamCreateWithFile(NULL, (OCFURLRef)url);
}

// Native -[__NSCFInputStream open], 0x1c01b51d0. Non-external.
- (void)open {
    OCFReadStreamOpen((OCFReadStreamRef)self);
}

// Native -[__NSCFInputStream close], 0x1c01b51d4. Non-external.
- (void)close {
    OCFReadStreamClose((OCFReadStreamRef)self);
}

// Native -[__NSCFInputStream delegate], 0x1c01b51d8. Non-external. The client info is the weak delegate
// wrapper; its retained delegate is autoreleased.
- (id)delegate {
    id wrapper = (id)_OCFReadStreamGetClient((OCFReadStreamRef)self);
    return [[wrapper retainedDelegate] autorelease];
}

// Native -[__NSCFInputStream setDelegate:], 0x1c01b51f0. Non-external. A delegate is wrapped in a new
// __NSCFStreamWeakDelegateWrapper that the client context retains with CFRetain and CFRelease; the local
// reference is released after the client is set. nil clears the client.
- (void)setDelegate:(id)delegate {
    if (delegate == nil) {
        OCFReadStreamSetClient((OCFReadStreamRef)self, 0, NULL, NULL);
        return;
    }
    id wrapper = [[(id)weakDelegateWrapperClass alloc] initWithDelegate:delegate];
    OCFStreamClientContext context = {
        0,
        wrapper,
        (void *(*)(void *))OCFRetain,
        (void (*)(void *))OCFRelease,
        NULL,
    };
    OCFReadStreamSetClient(
        (OCFReadStreamRef)self, kInputStreamDelegateEvents, _inputStreamCallbackFunc, &context
    );
    [wrapper release];
}

// Native _inputStreamCallbackFunc, 0x1c01b52d0. Non-external. The client callback: the info is the weak
// delegate wrapper, whose -retainedDelegate is sent -stream:handleEvent: with the stream and the event when
// it responds (objc_opt_respondsToSelector; the selector reference 0x2387b3136 lies outside the extracted
// image and is bound by the send that follows, confidence: medium), then released.
static void _inputStreamCallbackFunc(OCFReadStreamRef stream, OCFStreamEventType type, void *info) {
    id delegate = [(id)info retainedDelegate];
    if ([delegate respondsToSelector:@selector(stream:handleEvent:)]) {
        [delegate stream:(id)stream handleEvent:(NSUInteger)type];
    }
    [delegate release];
}

// Native -[__NSCFInputStream propertyForKey:], 0x1c01b5328. Non-external.
- (id)propertyForKey:(id)key {
    return [(id)OCFReadStreamCopyProperty((OCFReadStreamRef)self, (OCFStreamPropertyKey)key) autorelease];
}

// Native -[__NSCFInputStream setProperty:forKey:], 0x1c01b5340. Non-external.
- (BOOL)setProperty:(id)property forKey:(id)key {
    return OCFReadStreamSetProperty((OCFReadStreamRef)self, (OCFStreamPropertyKey)key, property) != 0;
}

// Native -[__NSCFInputStream scheduleInRunLoop:forMode:], 0x1c01b5360. Non-external. A nil run loop does
// nothing.
- (void)scheduleInRunLoop:(id)runLoop forMode:(id)mode {
    if (runLoop == nil) {
        return;
    }
    OCFReadStreamScheduleWithRunLoop((OCFReadStreamRef)self, [runLoop getCFRunLoop], (OCFRunLoopMode)mode);
}

// Native -[__NSCFInputStream removeFromRunLoop:forMode:], 0x1c01b539c. Non-external. A nil run loop does
// nothing.
- (void)removeFromRunLoop:(id)runLoop forMode:(id)mode {
    if (runLoop == nil) {
        return;
    }
    OCFReadStreamUnscheduleFromRunLoop((OCFReadStreamRef)self, [runLoop getCFRunLoop], (OCFRunLoopMode)mode);
}

// Native -[__NSCFInputStream streamStatus], 0x1c01b53d8. Non-external.
- (NSUInteger)streamStatus {
    return (NSUInteger)OCFReadStreamGetStatus((OCFReadStreamRef)self);
}

// Native -[__NSCFInputStream streamError], 0x1c01b53dc. Non-external.
- (id)streamError {
    return [(id)OCFReadStreamCopyError((OCFReadStreamRef)self) autorelease];
}

// Native -[__NSCFInputStream read:maxLength:], 0x1c01b53f0. Non-external.
- (NSInteger)read:(uint8_t *)buffer maxLength:(NSUInteger)length {
    return OCFReadStreamRead((OCFReadStreamRef)self, buffer, (OCFIndex)length);
}

// Native -[__NSCFInputStream getBuffer:length:], 0x1c01b53fc. Non-external. The buffer pointer is stored only
// when bytes were read; answers whether any were.
- (BOOL)getBuffer:(uint8_t **)buffer length:(NSUInteger *)length {
    const UInt8 *bytes = OCFReadStreamGetBuffer((OCFReadStreamRef)self, 0, (OCFIndex *)length);
    NSUInteger count = *length;
    if (count != 0) {
        *buffer = (uint8_t *)bytes;
    }
    return count != 0;
}

// Native -[__NSCFInputStream hasBytesAvailable], 0x1c01b543c. Non-external.
- (BOOL)hasBytesAvailable {
    return OCFReadStreamHasBytesAvailable((OCFReadStreamRef)self) != 0;
}

// Native -[__NSCFInputStream _setCFClientFlags:callback:context:], 0x1c01b5458. Non-external.
- (BOOL)_setCFClientFlags:(OCFOptionFlags)flags
                 callback:(OCFReadStreamClientCallBack)callback
                  context:(OCFStreamClientContext *)context {
    return OCFReadStreamSetClient((OCFReadStreamRef)self, flags, callback, context) != 0;
}

// Native -[__NSCFInputStream _scheduleInCFRunLoop:forMode:], 0x1c01b5480. Non-external.
- (void)_scheduleInCFRunLoop:(OCFRunLoopRef)runLoop forMode:(OCFRunLoopMode)mode {
    OCFReadStreamScheduleWithRunLoop((OCFReadStreamRef)self, runLoop, mode);
}

// Native -[__NSCFInputStream _unscheduleFromCFRunLoop:forMode:], 0x1c01b548c. Non-external.
- (void)_unscheduleFromCFRunLoop:(OCFRunLoopRef)runLoop forMode:(OCFRunLoopMode)mode {
    OCFReadStreamUnscheduleFromRunLoop((OCFReadStreamRef)self, runLoop, mode);
}

@end

@implementation __ONSCFOutputStream

// Native -[__NSCFOutputStream isEqual:], 0x1c01b5498. Non-external. nil answers NO and the receiver itself
// YES before _CFNonObjCEqual.
- (BOOL)isEqual:(id)object {
    if (object == nil) {
        return NO;
    }
    if (self == object) {
        return YES;
    }
    return _OCFNonObjCEqual(self, object) != 0;
}

// Native -[__NSCFOutputStream hash], 0x1c01b54d4. Non-external.
- (NSUInteger)hash {
    return _OCFNonObjCHash(self);
}

// Native -[__NSCFOutputStream retain], 0x1c01b54d8. Non-external.
- (id)retain {
    return (id)_OCFNonObjCRetain(self);
}

// Native -[__NSCFOutputStream release], 0x1c01b54dc. Non-external.
- (oneway void)release {
    _OCFNonObjCRelease(self);
}

// Native -[__NSCFOutputStream _tryRetain], 0x1c01b54e0. Non-external.
- (BOOL)_tryRetain {
    return _OCFTryRetain(self) != NULL;
}

// Native -[__NSCFOutputStream _isDeallocating], 0x1c01b54fc. Non-external.
- (BOOL)_isDeallocating {
    return _OCFIsDeallocating(self) != 0;
}

// Native -[__NSCFOutputStream retainCount], 0x1c01b5518. Non-external.
- (NSUInteger)retainCount {
    return (NSUInteger)OCFGetRetainCount(self);
}

// Native +[__NSCFOutputStream automaticallyNotifiesObserversForKey:], 0x1c01b551c. Non-external.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native +[__NSCFOutputStream allocWithZone:], 0x1c01b5524. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    [self doesNotRecognizeSelector:_cmd];
    return nil;
}

// Native -[__NSCFOutputStream initToMemory], 0x1c01b5540. Non-external. The receiver is sent -dealloc; the
// buffers come from the default allocator.
- (id)initToMemory {
    [self dealloc];
    return (id)OCFWriteStreamCreateWithAllocatedBuffers(NULL, kOCFAllocatorDefault);
}

// Native -[__NSCFOutputStream initToBuffer:capacity:], 0x1c01b5564. Non-external. The receiver is sent
// -dealloc.
- (id)initToBuffer:(uint8_t *)buffer capacity:(NSUInteger)capacity {
    [self dealloc];
    return (id)OCFWriteStreamCreateWithBuffer(NULL, buffer, (OCFIndex)capacity);
}

// Native -[__NSCFOutputStream initToFileAtPath:append:], 0x1c01b5594. Non-external. Without a file URL for
// the path nil is answered and the receiver is left as is; otherwise -initWithURL:append: with the URL, which
// is released.
- (id)initToFileAtPath:(id)path append:(BOOL)shouldAppend {
    id url = [[ONSURL alloc] initFileURLWithPath:path];
    if (url == nil) {
        return nil;
    }
    id stream = [self initWithURL:url append:shouldAppend];
    [url release];
    return stream;
}

// Native -[__NSCFOutputStream initWithURL:append:], 0x1c01b55f8. Non-external. The receiver is sent -dealloc;
// a nil URL answers NULL; appending sets kCFStreamPropertyAppendToFile to kCFBooleanTrue.
- (id)initWithURL:(id)url append:(BOOL)shouldAppend {
    [self dealloc];
    if (url == nil) {
        return nil;
    }
    OCFWriteStreamRef stream = OCFWriteStreamCreateWithFile(NULL, (OCFURLRef)url);
    if (shouldAppend) {
        OCFWriteStreamSetProperty(stream, kOCFStreamPropertyAppendToFile, kOCFBooleanTrue);
    }
    return (id)stream;
}

// Native -[__NSCFOutputStream open], 0x1c01b5658. Non-external.
- (void)open {
    OCFWriteStreamOpen((OCFWriteStreamRef)self);
}

// Native -[__NSCFOutputStream close], 0x1c01b565c. Non-external.
- (void)close {
    OCFWriteStreamClose((OCFWriteStreamRef)self);
}

// Native -[__NSCFOutputStream delegate], 0x1c01b5660. Non-external.
- (id)delegate {
    id wrapper = (id)_OCFWriteStreamGetClient((OCFWriteStreamRef)self);
    return [[wrapper retainedDelegate] autorelease];
}

// Native -[__NSCFOutputStream setDelegate:], 0x1c01b5678. Non-external. As the input stream's, with the
// output events.
- (void)setDelegate:(id)delegate {
    if (delegate == nil) {
        OCFWriteStreamSetClient((OCFWriteStreamRef)self, 0, NULL, NULL);
        return;
    }
    id wrapper = [[(id)weakDelegateWrapperClass alloc] initWithDelegate:delegate];
    OCFStreamClientContext context = {
        0,
        wrapper,
        (void *(*)(void *))OCFRetain,
        (void (*)(void *))OCFRelease,
        NULL,
    };
    OCFWriteStreamSetClient(
        (OCFWriteStreamRef)self, kOutputStreamDelegateEvents, _outputStreamCallbackFunc, &context
    );
    [wrapper release];
}

// Native _outputStreamCallbackFunc, 0x1c01b5758. Non-external. The client callback: the info is the weak
// delegate wrapper, whose -retainedDelegate is sent -stream:handleEvent: with the stream and the event when
// it responds (objc_opt_respondsToSelector; the selector reference 0x2387b3136 lies outside the extracted
// image and is bound by the send that follows, confidence: medium), then released.
static void _outputStreamCallbackFunc(OCFWriteStreamRef stream, OCFStreamEventType type, void *info) {
    id delegate = [(id)info retainedDelegate];
    if ([delegate respondsToSelector:@selector(stream:handleEvent:)]) {
        [delegate stream:(id)stream handleEvent:(NSUInteger)type];
    }
    [delegate release];
}

// Native -[__NSCFOutputStream propertyForKey:], 0x1c01b57b0. Non-external.
- (id)propertyForKey:(id)key {
    return [(id)OCFWriteStreamCopyProperty((OCFWriteStreamRef)self, (OCFStreamPropertyKey)key) autorelease];
}

// Native -[__NSCFOutputStream setProperty:forKey:], 0x1c01b57c8. Non-external.
- (BOOL)setProperty:(id)property forKey:(id)key {
    return OCFWriteStreamSetProperty((OCFWriteStreamRef)self, (OCFStreamPropertyKey)key, property) != 0;
}

// Native -[__NSCFOutputStream scheduleInRunLoop:forMode:], 0x1c01b57e8. Non-external. A nil run loop does
// nothing.
- (void)scheduleInRunLoop:(id)runLoop forMode:(id)mode {
    if (runLoop == nil) {
        return;
    }
    OCFWriteStreamScheduleWithRunLoop((OCFWriteStreamRef)self, [runLoop getCFRunLoop], (OCFRunLoopMode)mode);
}

// Native -[__NSCFOutputStream removeFromRunLoop:forMode:], 0x1c01b5824. Non-external. A nil run loop does
// nothing.
- (void)removeFromRunLoop:(id)runLoop forMode:(id)mode {
    if (runLoop == nil) {
        return;
    }
    OCFWriteStreamUnscheduleFromRunLoop(
        (OCFWriteStreamRef)self, [runLoop getCFRunLoop], (OCFRunLoopMode)mode
    );
}

// Native -[__NSCFOutputStream streamStatus], 0x1c01b5860. Non-external.
- (NSUInteger)streamStatus {
    return (NSUInteger)OCFWriteStreamGetStatus((OCFWriteStreamRef)self);
}

// Native -[__NSCFOutputStream streamError], 0x1c01b5864. Non-external.
- (id)streamError {
    return [(id)OCFWriteStreamCopyError((OCFWriteStreamRef)self) autorelease];
}

// Native -[__NSCFOutputStream write:maxLength:], 0x1c01b5878. Non-external.
- (NSInteger)write:(const uint8_t *)buffer maxLength:(NSUInteger)length {
    return OCFWriteStreamWrite((OCFWriteStreamRef)self, buffer, (OCFIndex)length);
}

// Native -[__NSCFOutputStream hasSpaceAvailable], 0x1c01b5884. Non-external.
- (BOOL)hasSpaceAvailable {
    return OCFWriteStreamCanAcceptBytes((OCFWriteStreamRef)self) != 0;
}

// Native -[__NSCFOutputStream _setCFClientFlags:callback:context:], 0x1c01b58a0. Non-external.
- (BOOL)_setCFClientFlags:(OCFOptionFlags)flags
                 callback:(OCFWriteStreamClientCallBack)callback
                  context:(OCFStreamClientContext *)context {
    return OCFWriteStreamSetClient((OCFWriteStreamRef)self, flags, callback, context) != 0;
}

// Native -[__NSCFOutputStream _scheduleInCFRunLoop:forMode:], 0x1c01b58c8. Non-external.
- (void)_scheduleInCFRunLoop:(OCFRunLoopRef)runLoop forMode:(OCFRunLoopMode)mode {
    OCFWriteStreamScheduleWithRunLoop((OCFWriteStreamRef)self, runLoop, mode);
}

// Native -[__NSCFOutputStream _unscheduleFromCFRunLoop:forMode:], 0x1c01b58d4. Non-external.
- (void)_unscheduleFromCFRunLoop:(OCFRunLoopRef)runLoop forMode:(OCFRunLoopMode)mode {
    OCFWriteStreamUnscheduleFromRunLoop((OCFWriteStreamRef)self, runLoop, mode);
}

@end

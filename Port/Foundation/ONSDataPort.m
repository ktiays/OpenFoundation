//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// NSData and NSMutableData entry points for the wasm port. Foundation's concrete data classes are not
// restored (they are outside the consumers' scope), but restored code reaches the abstract classes:
// -[NSString UTF8String] for strings over 165 characters and -dataUsingEncoding: (+dataWithLength:,
// -initWithLength:, -mutableBytes), NSJSONSerialization (+dataWithBytesNoCopy:length:freeWhenDone:,
// -appendBytes:length:), the archivers (+dataWithBytes:length:, +data) and path handling
// (+dataWithBytesNoCopy:length:). Each is a simple implementation over the restored CFData, whose instances
// are the bridged __NSCFData (a subclass of NSMutableData that implements -bytes, -length, -mutableBytes and
// -setLength:):
//
// - Initializers release the abstract receiver and answer a new CFData: immutable for NSData, mutable for
//   NSMutableData. Class factories autorelease the initializer's result.
// - A no-copy initializer that frees its bytes adopts them with kCFAllocatorMalloc as the deallocator (they
//   come from malloc), else kCFAllocatorNull.
// - Mutating methods are CFData calls on the receiver, which is a mutable __NSCFData.
//
// Manual reference counting, as the port's other Foundation units.

#import "CoreFoundation/Objective-C/OCFNSData.h"
#import "CoreFoundation/Objective-C/OCFNSMutableData.h"

#include "CoreFoundation/Base/OCFBase.h"
#include "CoreFoundation/Collections/OCFData.h"

#include <objc/runtime.h>
#include <stdlib.h>

static BOOL isMutableDataReceiver(id receiver) {
    return [receiver isKindOfClass:objc_lookUpClass(OF_NAME_STRING(ONSMutableData))];
}

static id createData(BOOL mutableData, const void *bytes, NSUInteger length) {
    if (mutableData) {
        OCFMutableDataRef data = OCFDataCreateMutable(NULL, 0);
        if (length != 0) {
            OCFDataAppendBytes(data, (const UInt8 *)bytes, (OCFIndex)length);
        }
        return (id)data;
    }
    return (id)OCFDataCreate(NULL, (const UInt8 *)bytes, (OCFIndex)length);
}

@interface ONSData (OpenFoundationPortData)
@end

@implementation ONSData (OpenFoundationPortData)

+ (id)data {
    return [[[self alloc] initWithBytes:NULL length:0] autorelease];
}

+ (id)dataWithBytes:(const void *)bytes length:(NSUInteger)length {
    return [[[self alloc] initWithBytes:bytes length:length] autorelease];
}

+ (id)dataWithBytesNoCopy:(void *)bytes length:(NSUInteger)length {
    return [[[self alloc] initWithBytesNoCopy:bytes length:length freeWhenDone:YES] autorelease];
}

+ (id)dataWithBytesNoCopy:(void *)bytes length:(NSUInteger)length freeWhenDone:(BOOL)freeWhenDone {
    return [[[self alloc] initWithBytesNoCopy:bytes length:length freeWhenDone:freeWhenDone] autorelease];
}

+ (id)dataWithData:(id)data {
    return [[[self alloc] initWithData:data] autorelease];
}

- (id)init {
    return [self initWithBytes:NULL length:0];
}

- (id)initWithBytes:(const void *)bytes length:(NSUInteger)length {
    BOOL mutableData = isMutableDataReceiver(self);
    [self release];
    return createData(mutableData, bytes, length);
}

- (id)initWithBytesNoCopy:(void *)bytes length:(NSUInteger)length {
    return [self initWithBytesNoCopy:bytes length:length freeWhenDone:YES];
}

- (id)initWithBytesNoCopy:(void *)bytes length:(NSUInteger)length freeWhenDone:(BOOL)freeWhenDone {
    if (isMutableDataReceiver(self)) {
        // A mutable data owns growable storage: copy, then release the bytes as the caller asked.
        id data = [self initWithBytes:bytes length:length];
        if (freeWhenDone) {
            free(bytes);
        }
        return data;
    }
    [self release];
    return (id)OCFDataCreateWithBytesNoCopy(
        NULL, (const UInt8 *)bytes, (OCFIndex)length, freeWhenDone ? kOCFAllocatorMalloc : kOCFAllocatorNull
    );
}

- (id)initWithData:(id)data {
    return [self initWithBytes:OCFDataGetBytePtr((OCFDataRef)data)
                        length:(NSUInteger)OCFDataGetLength((OCFDataRef)data)];
}

@end

@interface ONSMutableData (OpenFoundationPortData)
@end

@implementation ONSMutableData (OpenFoundationPortData)

+ (id)dataWithCapacity:(NSUInteger)capacity {
    return [[[self alloc] initWithCapacity:capacity] autorelease];
}

+ (id)dataWithLength:(NSUInteger)length {
    return [[[self alloc] initWithLength:length] autorelease];
}

- (id)initWithCapacity:(NSUInteger)capacity {
    [self release];
    return (id)OCFDataCreateMutable(NULL, 0);
}

// CFDataSetLength zero-fills the bytes it adds.
- (id)initWithLength:(NSUInteger)length {
    [self release];
    OCFMutableDataRef data = OCFDataCreateMutable(NULL, 0);
    OCFDataSetLength(data, (OCFIndex)length);
    return (id)data;
}

- (void)appendBytes:(const void *)bytes length:(NSUInteger)length {
    OCFDataAppendBytes((OCFMutableDataRef)self, (const UInt8 *)bytes, (OCFIndex)length);
}

- (void)appendData:(id)data {
    OCFDataAppendBytes(
        (OCFMutableDataRef)self, OCFDataGetBytePtr((OCFDataRef)data), OCFDataGetLength((OCFDataRef)data)
    );
}

- (void)increaseLengthBy:(NSUInteger)extraLength {
    OCFDataSetLength((OCFMutableDataRef)self, OCFDataGetLength((OCFDataRef)self) + (OCFIndex)extraLength);
}

@end

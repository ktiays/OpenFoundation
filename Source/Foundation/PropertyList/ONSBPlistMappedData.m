//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/PropertyList/ONSBPlistMappedData.h"
#import "Foundation/Archiving/ONSArchivingPrivate.h"
#import "Foundation/Error/ONSError.h"
#import "Foundation/Error/ONSErrorConstants.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#import "CoreFoundation/Objective-C/OCFNSDictionary.h"
#include "CoreFoundation/PlugIn/OCFBundleStrings.h"

#include <assert.h>
#include <sys/mman.h>

// Native source name NSBPlistMappedClasses.m (assertion at 0x1c051aacc).
// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. iOS Device 24A446: unread.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FB-bundle-paths-plist.
// Uniqued shared-cache GOT bindings (medium): NSData 0x22227fab8 and NSDictionary 0x22227fad0.

extern char mappedDataBaseClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSData));

// Native __NSCreateBPlistMappedDataFromURL, 0x1c051a710. External.
OF_EXPORT OCFDataRef __ONSCreateBPlistMappedDataFromURL(
    OCFURLRef url, OCFIndex (^registerMappedData)(OCFDataRef data), OCFErrorRef *error
) {
    void *pool = objc_autoreleasePoolPush();
    id mappingError = nil;
    _ONSBPlistMappedData *data = [[_ONSBPlistMappedData alloc] initWithFileURL:(id)url error:&mappingError];
    if (data != nil) {
        [data setMappingIndex:registerMappedData((OCFDataRef)data)];
    } else if (error != NULL) {
        *error = (OCFErrorRef)objc_retain(mappingError);
    }
    objc_autoreleasePoolPop(pool);
    return (OCFDataRef)data;
}

@implementation _ONSBPlistMappedData

// Native -[_NSBPlistMappedData initWithBinaryPlistData:size:trailer:offset:marker:], 0x1c051a7ec.
// Non-external. Hex-Rays shows Q0 as the result of the trailer copy. The receiver stays in X0.
- (id)initWithBinaryPlistData:(void *)bytes
                         size:(ONSUInteger)length
                      trailer:(BinaryPlistTrailer)trailer
                       offset:(ONSUInteger)offset
                       marker:(uint8_t)marker {
    self = [super init];
    if (self != nil) {
        ptr = bytes;
        size = length;
        mappingIndex = LONG_MAX;
        bplistTrailer = trailer;
        bplistOffset = offset;
        bplistMarker = marker;
    }
    return self;
}

// Native -[_NSBPlistMappedData dealloc], 0x1c051a8c0. Non-external.
- (void)dealloc {
    if (ptr != NULL && size != 0) {
        munmap(ptr, size);
    }
    [super dealloc];
}

// Native -[_NSBPlistMappedData initWithFileURL:error:], 0x1c051a950. Non-external.
// The private NSData read selector is unread. The assembly passes the trailer by value to the initializer
// and uses the exported NSCocoaErrorDomain and NSDebugDescriptionErrorKey objects.
- (id)initWithFileURL:(id)url error:(id *)error {
    id<StringPartBMessages> dataClass = (id)mappedDataBaseClass;
    void *bytes = NULL;
    ONSUInteger length = 0;
    BOOL mapped = NO;
    if (![dataClass _readBytesFromPath:[url path]
                             maxLength:LONG_MAX
                                 bytes:&bytes
                                length:&length
                                didMap:&mapped
                               options:8
                        reportProgress:NO
                                 error:error]) {
        [self release];
        return nil;
    }

    BinaryPlistTrailer trailer = {0};
    uint64_t offset = 0;
    uint8_t marker = 0;
    if (!__OCFBinaryPlistGetTopLevelInfo(bytes, length, &marker, &offset, &trailer)) {
        munmap(bytes, length);
        if (error != NULL) {
            id key = ONSDebugDescriptionErrorKey;
            id reason = (id)OF_STR("File is not binary plist");
            id info = [ONSDictionary dictionaryWithObjects:&reason forKeys:&key count:1];
            *error = [ONSError errorWithDomain:ONSCocoaErrorDomain code:256 userInfo:info];
        }
        [self release];
        return nil;
    }

    if (!mapped) {
        __assert_rtn(
            "-[_NSBPlistMappedData initWithFileURL:error:]", "NSBPlistMappedClasses.m", 81, "mapped"
        );
    }
    return [self initWithBinaryPlistData:bytes size:length trailer:trailer offset:offset marker:marker];
}

// Native -[_NSBPlistMappedData setMappingIndex:], 0x1c051aad4. Non-external.
- (void)setMappingIndex:(ONSUInteger)index {
    mappingIndex = index;
}

// Native -[_NSBPlistMappedData mappingIndex], 0x1c051aae4. Non-external.
- (ONSUInteger)mappingIndex {
    return mappingIndex;
}

// Native -[_NSBPlistMappedData bytes], 0x1c051aaf4. Non-external.
- (const void *)bytes {
    return ptr;
}

// Native -[_NSBPlistMappedData length], 0x1c051ab04. Non-external.
- (ONSUInteger)length {
    return size;
}

// Native -[_NSBPlistMappedData _bplistObjectsRangeEnd], 0x1c051ab14. Non-external.
- (ONSUInteger)_bplistObjectsRangeEnd {
    return bplistTrailer.offsetTableOffset - 1;
}

@end

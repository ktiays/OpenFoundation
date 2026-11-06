//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Preferences/OCFPreferences.h"

#include <stdbool.h>
#include <stddef.h>
#include <xpc/xpc.h>

// The CFPrefsBuffers.m functions of this restoration (Preferences/OCFPrefsBuffers.m). Hidden natively
// (private externals); signatures from the native register use. The size acceptability is 0 (acceptable) up
// to 3 MiB, 1 (large) up to 4 MiB and 2 (too large) above. The byte counts accumulate the encoded sizes.
OF_EXTERN int _CFPrefsDomainSizeAcceptabilityForByteCount(size_t byteCount);

// Native _CFPrefsSizeOfValue (kind 0: one value; kind 1: an XPC dictionary of values) and
// _CFPrefsDecodeKeyValuePairFromXPCMessage (the key +1 or released, the value retained when requested).
OF_EXTERN size_t _CFPrefsSizeOfValue(xpc_object_t value, int kind);

OF_EXTERN bool _CFPrefsDecodeKeyValuePairFromXPCMessage(
    xpc_object_t message, xpc_object_t *outKey, xpc_object_t *outValue, int *outMultiple
);

OF_EXTERN bool _CFPrefsEncodeValueIntoMessage(
    xpc_object_t message, const char *key, OCFPropertyListRef value, size_t *byteCount
);

OF_EXTERN bool _CFPrefsEncodeKeyValuePairsIntoMessage(
    xpc_object_t message, OCFStringRef *keys, OCFPropertyListRef *values, OCFIndex count, size_t *byteCount
);

// C2 supplies the native allocator callbacks at 0x1c0199ffc and 0x1c019a0c8.
OF_EXTERN void *allocatePurgeable(OCFIndex size, OCFOptionFlags hint, void *info);
OF_EXTERN void deallocatePurgeable(void *pointer, void *info);

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native CFPDDataBuffer, class_ro_t 0x2227bf818. NSObject subclass, instance size 8.
@interface OCFPDDataBuffer : NSObject
+ (instancetype)newBufferFromFile:(int)fd allowMappingIfSafe:(BOOL)allowMapping;
+ (instancetype)newBufferFromPropertyList:(OCFPropertyListRef)plist;
- (OCFDataRef)copyCFData;
- (xpc_object_t)copyXPCData;
- (OCFPropertyListRef)copyPropertyListWithMutability:(OCFOptionFlags)mutability error:(OCFErrorRef *)error;
- (BOOL)validatePlist;
- (void)quicklyValidatePlistAndOnFailureInvokeBlock:(void (^)(void))block;
- (BOOL)beginAccessing;
- (void)endAccessing;
- (BOOL)purgable;
- (const UInt8 *)bytes;
- (OCFIndex)length;
@end

// Native CFPDPurgeableBuffer, class_ro_t 0x2227bf9e8. Size 26, ivars at 8, 16, 24 and 25.
// The Mach-O type strings are outside the extracted image. The handle, flags and allocation size are
// typed from the reads and writes in the captured methods and allocator callbacks.
@interface OCFPDPurgeableBuffer : OCFPDDataBuffer {
    @package
    OCFDataRef handle;
    size_t allocSize;
    unsigned char safe;
    unsigned char usedMalloc;
}
- (instancetype)initWithFileDescriptor:(int)fd size:(size_t)size;
- (instancetype)initWithPropertyList:(OCFPropertyListRef)plist;
@end
#endif

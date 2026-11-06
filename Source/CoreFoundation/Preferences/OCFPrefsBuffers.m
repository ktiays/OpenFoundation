//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Preferences/OCFPrefsBuffers.h"
#import "CoreFoundation/Preferences/OCFXPCBridge.h"
#include "CoreFoundation/PropertyList/OCFBinaryPList.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <mach/mach.h>
#include <os/log.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mach/vm_page_size.h>
#include <malloc/malloc.h>
#include <string.h>
#include <strings.h>

// Inferred organization: _CFPrefsDomainSizeAcceptabilityForByteCount (0x1c0199024) through
// _CFPrefsEncodeKeyValuePairsIntoMessage (0x1c01995ac) lie together in the iOS Simulator __text section, and
// the assertion of _CFPrefsEncodeKeyValuePairsIntoMessage names "CFPrefsBuffers.m". _CFPrefsSizeOfValue
// (0x1c0199048, its block 0x1c01991b8) and _CFPrefsDecodeKeyValuePairFromXPCMessage (0x1c0199218) of the same
// unit were read from Local/Evidence/cf-core/iphone-simulator-corpus-delta12.json. The XPC type objects are
// read through GOT slots outside the extracted image; their bindings (0x22241b970 XPC_TYPE_BOOL, 0x22241b988
// XPC_TYPE_DATA, 0x22241b9e8 XPC_TYPE_NULL, 0x22241ba28 XPC_TYPE_STRING) are inferred from the alphabetical,
// 8-byte-stride order of the image's libxpc data imports, as in Preferences/OCFPrefsDaemon.m: confidence
// medium. The unit is Objective-C (its @finally clauses rethrow with objc_exception_rethrow), compiled
// with manual reference counting. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Local/Scratch/cf-core-prefs-daemon/REPORT.md (main checkout; integration reading pending).

// Constants as the native call sites pass them: kCFStringEncodingUTF8, kCFPropertyListBinaryFormat_v1_0
// (200) and the builtin type ID of CFString (7).
#define kUTF8Encoding ((OCFStringEncoding)0x08000100)
#define kBinaryPropertyListFormat ((OCFIndex)200)
#define kStringTypeID ((OCFTypeID)7)

// Native _CFPrefsDomainSizeAcceptabilityForByteCount, 0x1c0199024. Hidden (was a private external). 0 up to
// 0x300000 bytes, 1 up to 0x400000, 2 above.
int _CFPrefsDomainSizeAcceptabilityForByteCount(size_t byteCount) {
    int acceptability = byteCount < 0x400001 ? 1 : 2;
    return byteCount > 0x300000 ? acceptability : 0;
}

// Native _CFPrefsSizeOfValue, 0x1c0199048, with ____CFPrefsSizeOfValue_block_invoke (0x1c01991b8, descriptor
// 0x22279db88). Hidden (was a private external). The approximate encoded size of an XPC value, accumulated in
// a __block counter: kind 1 sums, over an XPC dictionary, each entry's key length plus its value's size (kind
// 0), skipping entries of size 0; kind 0 answers 1 for a bool, the length of a string or data, and 0 for a
// null or any other type; any other kind answers 0.
size_t _CFPrefsSizeOfValue(xpc_object_t value, int kind) {
    __block size_t size = 0;
    if (kind == 1) {
        xpc_dictionary_apply(value, ^bool(const char *key, xpc_object_t entry) {
          size_t entrySize = _CFPrefsSizeOfValue(entry, 0);
          if (entrySize != 0) {
              size += strlen(key) + entrySize;
          }
          return true;
        });
    } else if (kind == 0) {
        xpc_type_t type = xpc_get_type(value);
        if (type == XPC_TYPE_BOOL) {
            size += 1;
        } else if (type == XPC_TYPE_STRING) {
            size += xpc_string_get_length(value);
        } else if (type == XPC_TYPE_NULL) {
            size = 0;
        } else if (type == XPC_TYPE_DATA) {
            size += xpc_data_get_length(value);
        }
    }
    return size;
}

// Native _CFPrefsDecodeKeyValuePairFromXPCMessage, 0x1c0199218. Hidden (was a private external). A message
// with "MultiKeySet" set carries its pairs in "Value" and answers the key string "MultipleKeys" (created, +1)
// with multiple 1; otherwise "Key" (retained) and "Value" with multiple 0. A missing value answers false. On
// success the key is stored (+1) when outKey is given and released otherwise, the value is retained into
// outValue when given, and the multiple flag stored when outMultiple is given.
// CoreFoundation defect: natively the retained "Key" leaks when "Value" is missing (the early return skips
// its release); established from the assembly (0x1c01992b8 retains, 0x1c01992cc returns): confidence
// verified. This restoration releases it on that path.
bool _CFPrefsDecodeKeyValuePairFromXPCMessage(
    xpc_object_t message, xpc_object_t *outKey, xpc_object_t *outValue, int *outMultiple
) {
    xpc_object_t key;
    xpc_object_t value;
    int multiple;
    if (xpc_dictionary_get_bool(message, "MultiKeySet")) {
        value = xpc_dictionary_get_value(message, "Value");
        if (value == NULL) {
            return false;
        }
        key = xpc_string_create("MultipleKeys");
        multiple = 1;
    } else {
        key = xpc_dictionary_get_value(message, "Key");
        if (key == NULL) {
            return false;
        }
        xpc_retain(key);
        value = xpc_dictionary_get_value(message, "Value");
        if (value == NULL) {
            xpc_release(key);
            return false;
        }
        multiple = 0;
    }
    if (outKey != NULL) {
        *outKey = key;
    } else {
        xpc_release(key);
    }
    if (outValue != NULL) {
        xpc_retain(value);
        *outValue = value;
    }
    if (outMultiple != NULL) {
        *outMultiple = multiple;
    }
    return true;
}

// Native _CFPrefsEncodeValueIntoMessage, 0x1c0199300. Hidden (was a private external). A NULL key or value
// encodes nothing (false). Booleans are XPC bools (size 1); kCFNull is an XPC null (true, size not counted);
// a string is stored as a C string when it converts to UTF-8 within 1,024 bytes, otherwise through
// _CFXPCCreateXPCObjectFromCFObject (false when that fails), its size being its length either way; any other
// value is stored as binary property list data (its size the data length). A value that cannot be
// serialized is passed, with the key as a string, to _CFPrefsValidateValueForKey (which reports it), the key
// string being released in a @finally (region bounds inferred from the rethrow structure: confidence
// medium), and false is answered. On success with a nonzero size, the size is added to *byteCount when given.
bool _CFPrefsEncodeValueIntoMessage(
    xpc_object_t message, const char *key, OCFPropertyListRef value, size_t *byteCount
) {
    if (key == NULL || value == NULL) {
        return false;
    }
    bool encoded;
    OCFIndex size;
    if (value == kOCFBooleanTrue || value == kOCFBooleanFalse) {
        xpc_dictionary_set_bool(message, key, value == kOCFBooleanTrue);
        size = 1;
        encoded = true;
    } else if (value == kOCFNull) {
        xpc_object_t null = xpc_null_create();
        xpc_dictionary_set_value(message, key, null);
        xpc_release(null);
        return true;
    } else if (OCFGetTypeID(value) == kStringTypeID) {
        char buffer[1024];
        bzero(buffer, sizeof(buffer));
        if (OCFStringGetCString(value, buffer, 1024, kUTF8Encoding)) {
            xpc_dictionary_set_string(message, key, buffer);
            encoded = true;
        } else {
            xpc_object_t object = _OCFXPCCreateXPCObjectFromCFObject(value);
            encoded = object != NULL;
            if (object != NULL) {
                xpc_dictionary_set_value(message, key, object);
                xpc_release(object);
            }
        }
        size = OCFStringGetLength(value);
    } else {
        OCFDataRef data =
            OCFPropertyListCreateData(kOCFAllocatorSystemDefault, value, kBinaryPropertyListFormat, 0, NULL);
        if (data == NULL) {
            OCFStringRef keyString = OCFStringCreateWithBytes(
                kOCFAllocatorSystemDefault, (const UInt8 *)key, strlen(key), kUTF8Encoding, false
            );
            @try {
                _CFPrefsValidateValueForKey(value, keyString);
            } @finally {
                if (keyString != NULL) {
                    OCFRelease(keyString);
                }
            }
            return false;
        }
        xpc_dictionary_set_data(message, key, OCFDataGetBytePtr(data), OCFDataGetLength(data));
        size = OCFDataGetLength(data);
        OCFRelease(data);
        encoded = true;
    }
    if (byteCount != NULL && encoded && size != 0) {
        *byteCount += size;
        return true;
    }
    return encoded;
}

// Native _CFPrefsEncodeKeyValuePairsIntoMessage, 0x1c01995ac, with its .cold.1 (0x1c0308048, the assertion
// "count > 0", CFPrefsBuffers.m line 152). Hidden (was a private external). One pair is encoded as "Key" and
// "Value" (a NULL value as kCFNull; the value only when the key encoded). Several pairs go into a nested XPC
// dictionary keyed by each key's UTF-8 form (at most 1,026 bytes), NULL keys skipped; after the first
// failure no further pair is encoded. The nested dictionary is stored under "Value" with "MultiKeySet" set,
// and released in a @finally (region bounds inferred from the rethrow structure: confidence medium). The
// accumulated size is stored into *byteCount when given. Answers whether the last attempt succeeded.
bool _CFPrefsEncodeKeyValuePairsIntoMessage(
    xpc_object_t message, OCFStringRef *keys, OCFPropertyListRef *values, OCFIndex count, size_t *byteCount
) {
    if (count <= 0) {
        __assert_rtn("_CFPrefsEncodeKeyValuePairsIntoMessage", "CFPrefsBuffers.m", 152, "count > 0");
    }
    size_t size = 0;
    bool encoded;
    if (count == 1) {
        if (_CFPrefsEncodeValueIntoMessage(message, "Key", *keys, &size)) {
            OCFPropertyListRef value = *values != NULL ? *values : kOCFNull;
            encoded = _CFPrefsEncodeValueIntoMessage(message, "Value", value, &size);
        } else {
            encoded = false;
        }
    } else {
        xpc_object_t pairs = xpc_dictionary_create(NULL, NULL, 0);
        encoded = true;
        @try {
            for (OCFIndex remaining = count; remaining != 0; --remaining) {
                OCFStringRef key = *keys++;
                if (key != NULL) {
                    char buffer[1026];
                    bzero(buffer, sizeof(buffer));
                    if (encoded && OCFStringGetCString(key, buffer, 1026, kUTF8Encoding)) {
                        OCFPropertyListRef value = *values != NULL ? *values : kOCFNull;
                        encoded = _CFPrefsEncodeValueIntoMessage(pairs, buffer, value, &size);
                    } else {
                        encoded = false;
                    }
                }
                ++values;
            }
            xpc_dictionary_set_bool(message, "MultiKeySet", true);
            xpc_dictionary_set_value(message, "Value", pairs);
        } @finally {
            xpc_release(pairs);
        }
    }
    if (byteCount != NULL) {
        *byteCount = size;
    }
    return encoded;
}

// libsystem os_variant SPI, also used in the daemon source.
extern bool os_variant_has_internal_diagnostics(const char *subsystem);

@implementation OCFPDDataBuffer

// Native +[CFPDDataBuffer newBufferFromFile:allowMappingIfSafe:], 0x1c0199804. Listed method.
// The iOS Simulator path always uses the purgeable buffer and ignores allowMappingIfSafe (kept).
+ (instancetype)newBufferFromFile:(int)fd allowMappingIfSafe:(BOOL)allowMapping {
    (void)allowMapping;
    struct stat status = {0};
    if (fstat(fd, &status) != 0 || status.st_size < 1) {
        return nil;
    }
    return [[OCFPDPurgeableBuffer alloc] initWithFileDescriptor:fd size:(size_t)status.st_size];
}

// Native +[CFPDDataBuffer newBufferFromPropertyList:], 0x1c01998a8. Listed method.
+ (instancetype)newBufferFromPropertyList:(OCFPropertyListRef)plist {
    return [[OCFPDPurgeableBuffer alloc] initWithPropertyList:plist];
}

// Native -[CFPDDataBuffer copyCFData], 0x1c01998d4. Listed method.
- (OCFDataRef)copyCFData {
    const UInt8 *bytes = [self bytes];
    OCFIndex length = [self length];
    return OCFDataCreate(kOCFAllocatorSystemDefault, bytes, length);
}

// Native -[CFPDDataBuffer copyXPCData], 0x1c0199920. Listed method.
- (xpc_object_t)copyXPCData {
    const UInt8 *bytes = [self bytes];
    OCFIndex length = [self length];
    return xpc_data_create(bytes, (size_t)length);
}

// Native -[CFPDDataBuffer copyPropertyListWithMutability:error:], 0x1c0199954. Listed method.
// Folds .cold.1 (0x1c0308070), OUTLINED_FUNCTION_0_8 (0x1c019a4e0) and
// OUTLINED_FUNCTION_1_6 (0x1c019a4e8) into the log.
- (OCFPropertyListRef)copyPropertyListWithMutability:(OCFOptionFlags)mutability error:(OCFErrorRef *)error {
    const UInt8 *bytes = [self bytes];
    OCFIndex length = [self length];
    OCFDataRef data =
        OCFDataCreateWithBytesNoCopy(kOCFAllocatorSystemDefault, bytes, length, kOCFAllocatorNull);
    OCFPropertyListRef plist =
        OCFPropertyListCreateWithData(kOCFAllocatorSystemDefault, data, mutability, NULL, error);
    OCFRelease(data);
    if (plist != NULL && OCFGetTypeID(plist) != 18) {
        os_log_error(
            _CFPrefsDaemonLog(),
            "Found non-dictionary property list, which is invalid for preferences. Ignoring it."
        );
        OCFRelease(plist);
        plist = NULL;
        if (error != NULL) {
            *error = NULL;
        }
    }
    return plist;
}

// Native -[CFPDDataBuffer validatePlist], 0x1c0199a34. Listed method.
- (BOOL)validatePlist {
    const UInt8 *bytes = [self bytes];
    OCFIndex length = [self length];
    OCFDataRef data =
        OCFDataCreateWithBytesNoCopy(kOCFAllocatorSystemDefault, bytes, length, kOCFAllocatorNull);
    OCFTypeID type = 0;
    bool valid = _CFPropertyListValidateData(data, &type);
    OCFRelease(data);
    return valid && type == 18;
}

// Native -[CFPDDataBuffer quicklyValidatePlistAndOnFailureInvokeBlock:], 0x1c0199aec. Listed method.
// Folds .cold.1 (0x1c03080a8) and the log helpers at 0x1c019a4e0 and 0x1c019a4e8.
- (void)quicklyValidatePlistAndOnFailureInvokeBlock:(void (^)(void))block {
    if ((uint64_t)[self length] >= 8) {
        const char *prefix = (const char *)[self bytes];
        if (strncmp(prefix, "bplist00", 8) == 0 || strncmp(prefix, "<?xml", 5) == 0) {
            return;
        }
    }
    block();
    const UInt8 *bytes = [self bytes];
    OCFIndex length = [self length];
    OCFDataRef data =
        OCFDataCreateWithBytesNoCopy(kOCFAllocatorSystemDefault, bytes, length, kOCFAllocatorNull);
    if (os_variant_has_internal_diagnostics("com.apple.defaults")) {
        os_log_t log = os_log_create("com.apple.defaults", "diagnostics");
        os_log_error(log, "bad plist data: %{public}@", (id)data);
        [(id)log release];
    }
    OCFRelease(data);
}

// Native -[CFPDDataBuffer beginAccessing], 0x1c0199bf4. Listed base method.
- (BOOL)beginAccessing {
    return YES;
}

// Native -[CFPDDataBuffer endAccessing], 0x1c0199bfc. Listed, natively empty.
- (void)endAccessing {
}

// Native -[CFPDDataBuffer purgable], 0x1c0199c00. Listed base method.
- (BOOL)purgable {
    return NO;
}

// Native -[CFPDDataBuffer bytes], 0x1c0308030. Listed abstract method.
- (const UInt8 *)bytes {
    abort();
}

// Native -[CFPDDataBuffer length], 0x1c030803c. Listed abstract method.
- (OCFIndex)length {
    abort();
}

@end

@implementation OCFPDPurgeableBuffer

// Native -[CFPDPurgeableBuffer initWithFileDescriptor:size:], 0x1c0199d60. Listed method.
// Folds .cold.1 (0x1c03080e8), .cold.2 (0x1c0308154) and the log helpers at 0x1c019a4e0 and 0x1c019a4e8.
// GOT 0x22241a5b0 is the page size (binding confidence: medium).
- (instancetype)initWithFileDescriptor:(int)fd size:(size_t)size {
    if (fd < 0 || size == 0) {
        [self release];
        return nil;
    }
    self = [super init];
    if (self == nil) {
        return nil;
    }
    OCFAllocatorRef allocator;
    if (size < vm_page_size / 4) {
        allocator = OCFRetain(kOCFAllocatorSystemDefault);
        usedMalloc = 1;
    } else {
        OCFAllocatorContext context = {
            .info = self, .allocate = allocatePurgeable, .deallocate = deallocatePurgeable
        };
        allocator = OCFAllocatorCreate(kOCFAllocatorSystemDefault, &context);
    }
    if (allocator == NULL) {
        [self release];
        return nil;
    }
    UInt8 *bytes = OCFAllocatorAllocateTyped(allocator, (OCFIndex)size, 0x611C48F6, 0);
    if (bytes == NULL) {
        OCFRelease(allocator);
        [self release];
        return nil;
    }
    OCFMutableDataRef data =
        _CFDataCreateFixedMutableWithBuffer(kOCFAllocatorSystemDefault, (OCFIndex)size, bytes, allocator);
    if (data == NULL) {
        OCFAllocatorDeallocate(allocator, bytes);
        OCFRelease(allocator);
        [self release];
        return nil;
    }
    OCFRelease(allocator);
    OCFDataSetLength(data, (OCFIndex)size);
    UInt8 *destination = OCFDataGetMutableBytePtr(data);
    size_t consumed = 0;
    for (;;) {
        ssize_t count = read(fd, destination + consumed, size - consumed);
        if (count == -1) {
            if (errno == EINTR) {
                continue;
            }
            int savedError = errno;
            char path[1024] = {0};
            fcntl(fd, F_GETPATH, path);
            os_log_t log = _CFPrefsDaemonLog();
            if (os_log_type_enabled(log, OS_LOG_TYPE_FAULT)) {
                os_log_fault(
                    log,
                    "couldn't read preferences file %{private}s due to %{public}s",
                    path,
                    strerror(savedError)
                );
            }
            handle = data;
            [self release];
            return nil;
        }
        if (count == 0) {
            break;
        }
        consumed += (size_t)count;
    }
    handle = data;
    if (consumed == size) {
        safe = 1;
        return self;
    }
    char path[1024] = {0};
    fcntl(fd, F_GETPATH, path);
    os_log_fault(
        _CFPrefsDaemonLog(),
        "read an unexpected amount of data from %{private}s. This likely means that someone wrote directly "
        "to that file behind cfprefsd's back, which is unsupported.",
        path
    );
    [self release];
    return nil;
}

// Native -[CFPDPurgeableBuffer initWithPropertyList:], 0x1c019a144. Listed method.
- (instancetype)initWithPropertyList:(OCFPropertyListRef)plist {
    self = [super init];
    if (self == nil) {
        return nil;
    }
    // Native ___44-[CFPDPurgeableBuffer initWithPropertyList:]_block_invoke, 0x1c019a220. Non-external.
    OCFDataRef data = __CFBinaryPlistCreateDataUsingExternalBufferAllocator(
        plist,
        0,
        0,
        ^OCFAllocatorRef(size_t size) {
          if (size < vm_page_size / 4) {
              usedMalloc = 1;
              return OCFRetain(kOCFAllocatorSystemDefault);
          }
          OCFAllocatorContext context = {
              .info = self, .allocate = allocatePurgeable, .deallocate = deallocatePurgeable
          };
          return OCFAllocatorCreate(kOCFAllocatorSystemDefault, &context);
        },
        NULL
    );
    if (data == NULL) {
        [self release];
        return nil;
    }
    handle = data;
    safe = 1;
    return self;
}

// Native -[CFPDPurgeableBuffer bytes], 0x1c019a2f0. Listed method. Folds .cold.1 at 0x1c03081d4.
- (const UInt8 *)bytes {
    if ((safe & 1) == 0) {
        __assert_rtn("-[CFPDPurgeableBuffer bytes]", "CFPrefsBuffers.m", 451, "safe");
    }
    return OCFDataGetBytePtr(handle);
}

// Native -[CFPDPurgeableBuffer length], 0x1c019a30c. Listed method.
- (OCFIndex)length {
    return OCFDataGetLength(handle);
}

// Native -[CFPDPurgeableBuffer beginAccessing], 0x1c019a314. Listed method. Folds .cold.1 at 0x1c03081fc.
- (BOOL)beginAccessing {
    if (usedMalloc & 1) {
        return YES;
    }
    if (safe == 1) {
        __assert_rtn("-[CFPDPurgeableBuffer beginAccessing]", "CFPrefsBuffers.m", 463, "!safe");
    }
    int state = VM_PURGABLE_NONVOLATILE;
    vm_map_t task = mach_task_self();
    const UInt8 *bytes = OCFDataGetBytePtr(handle);
    kern_return_t result = vm_purgable_control(task, (vm_address_t)bytes, VM_PURGABLE_SET_STATE, &state);
    safe = (result | state) == 0;
    return safe;
}

// Native -[CFPDPurgeableBuffer endAccessing], 0x1c019a3c4. Listed method.
- (void)endAccessing {
    if ((usedMalloc & 1) == 0 && safe == 1) {
        int state = VM_PURGABLE_VOLATILE;
        vm_map_t task = mach_task_self();
        const UInt8 *bytes = OCFDataGetBytePtr(handle);
        vm_purgable_control(task, (vm_address_t)bytes, VM_PURGABLE_SET_STATE, &state);
        safe = 0;
    }
}

// Native -[CFPDPurgeableBuffer purgable], 0x1c019a458. Listed method.
- (BOOL)purgable {
    return (safe & 1) == 0;
}

// The runtime-invoked -dealloc at 0x1c019a468 is outside the captured closure.
@end

// CFPDPurgeableBuffer allocator context, fields read by the callbacks. The C1-prefs-sources methods
// own the Objective-C class. The callback fields are at 0x08, 0x10 and 0x19 in the native loads/stores.
typedef struct {
    uintptr_t isa;
    OCFDataRef data;
    vm_size_t size;
    uint8_t unreadAt24;
    uint8_t usesMalloc;
} PurgeableBufferAllocatorContext;

// Native allocatePurgeable, 0x1c0199ffc. Non-external natively; hidden for the C1 buffer methods.
// iOS Device divergence: unread. The GOT bindings for vm_page_size and mach_task_self_ have medium
// confidence (0x22241a5b0 and 0x222419e80). A nonpositive request still asks VM for one page.
void *allocatePurgeable(OCFIndex size, OCFOptionFlags hint, void *info) {
    (void)hint;
    PurgeableBufferAllocatorContext *context = info;
    vm_address_t address = 0;
    vm_size_t pageSize = vm_page_size;
    vm_size_t request = size > 0 ? (vm_size_t)size : pageSize;
    vm_size_t alignedSize = (pageSize + request - 1) & -pageSize;
    if (vm_allocate(mach_task_self(), &address, alignedSize, (int)0xf0000003u) != KERN_SUCCESS) {
        context->usesMalloc = 1;
        context->size = (vm_size_t)size;
        return malloc_type_malloc((size_t)size, 0x802473f7);
    }
    context->size = alignedSize;
    return (void *)address;
}

// Native deallocatePurgeable, 0x1c019a0c8, with .cold.1 at 0x1c0308194. Non-external natively;
// hidden for the C1 buffer methods. iOS Device divergence: unread. Failed vm_deallocate is logged (kept).
void deallocatePurgeable(void *pointer, void *info) {
    PurgeableBufferAllocatorContext *context = info;
    if (context->usesMalloc == 1) {
        free(pointer);
        return;
    }
    vm_map_t task = mach_task_self();
    const uint8_t *bytes = OCFDataGetBytePtr(context->data);
    kern_return_t error = vm_deallocate(task, (vm_address_t)bytes, context->size);
    if (error != KERN_SUCCESS) {
        os_log_fault(
            _CFPrefsDaemonLog(),
            "couldn't deallocate purgable buffer. Leaking it to avoid crashing. Error was %{public}d",
            error
        );
    }
}

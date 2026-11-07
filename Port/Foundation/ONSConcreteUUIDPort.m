//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// __NSConcreteUUID for the wasm port: Source/Foundation/UUID/UUID.swift translated to Objective-C (manual
// reference counting), member for member. The Swift restoration cites the native functions (Foundation
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03, iOS Simulator 24A434); each method below names the Swift member it
// translates. Differences: -debugDescription spells the receiver's address with %p instead of Swift's
// UnsafePointer description, and the CocoaError a decoding failure bridges to is built directly as the
// NSError _bridgeErrorToNSError produces (NSCocoaErrorDomain, the code, the debug description).

#import "Foundation/Base/ONSPlatformShims.h"
#import "Foundation/Error/ONSError.h"
#import "Foundation/Error/ONSErrorConstants.h"
#import "Foundation/String/ONSString.h"
#import "Foundation/UUID/ONSUUIDSwiftSupport.h"

#include <objc/runtime.h>
#include <string.h>

@protocol ConcreteUUIDMessages
+ (id)dictionaryWithObject:(id)object forKey:(id)key;
@end

@interface __ONSConcreteUUID : _ONSUUIDBridge {
    uuid_t _storage;
}
@end

@implementation __ONSConcreteUUID

static id decodingError(NSInteger code, const char *description) {
    id info = [(id)objc_getClass(OF_NAME_STRING(ONSDictionary))
        dictionaryWithObject:[ONSString stringWithUTF8String:description]
                      forKey:ONSDebugDescriptionErrorKey];
    return [ONSError errorWithDomain:ONSCocoaErrorDomain code:code userInfo:info];
}

// static supportsSecureCoding.getter.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// init(): random bytes, then -[_NSUUIDBridge init].
- (id)init {
    _foundation_uuid_generate_random(_storage);
    return [super init];
}

// init(coder:).
- (id)initWithCoder:(ONSCoder *)coder {
    if (![coder allowsKeyedCoding]) {
        [coder failWithError:decodingError(4864, "Cannot be decoded without keyed coding")];
        [self release];
        return nil;
    }
    NSUInteger length = 0;
    const unsigned char *bytes = [coder decodeBytesForKey:@"NS.uuidbytes" returnedLength:&length];
    if (bytes == NULL) {
        if ([ONSUUID _compatibilityBehavior]) {
            memset(_storage, 0, sizeof(_storage));
            return [super init];
        }
        [coder failWithError:decodingError(4865, "UUID bytes not found in archive")];
        [self release];
        return nil;
    }
    if (length != 16) {
        if ([ONSUUID _compatibilityBehavior]) {
            memset(_storage, 0, sizeof(_storage));
            return [super init];
        }
        [coder failWithError:decodingError(4864, "UUID bytes were not the expected length")];
        [self release];
        return nil;
    }
    memcpy(_storage, bytes, sizeof(_storage));
    return [super init];
}

// encode(with:).
- (void)encodeWithCoder:(ONSCoder *)coder {
    [coder encodeBytes:_storage length:16 forKey:@"NS.uuidbytes"];
}

// init(uuidString:): UUID.init(uuidString:) parses the string's UTF-8 with _foundation_uuid_parse.
- (id)initWithUUIDString:(ONSString *)string {
    const char *utf8 = string != nil ? [(id)string UTF8String] : "";
    if (utf8 == NULL || _foundation_uuid_parse(utf8, _storage) != 0) {
        if ([ONSUUID _compatibilityBehavior]) {
            memset(_storage, 0, sizeof(_storage));
            return [super init];
        }
        [self release];
        return nil;
    }
    return [super init];
}

// init(uuidBytes:): the 16 bytes, or zero for a null pointer.
- (id)initWithUUIDBytes:(const unsigned char *)bytes {
    if (bytes != NULL) {
        memcpy(_storage, bytes, sizeof(_storage));
    } else {
        memset(_storage, 0, sizeof(_storage));
    }
    return [super init];
}

// getBytes(_:).
- (void)getUUIDBytes:(unsigned char *)bytes {
    memcpy(bytes, _storage, sizeof(_storage));
}

// uuidString.getter: UUID.uuidString, the upper-case form of _foundation_uuid_unparse_upper.
- (ONSString *)UUIDString {
    uuid_string_t buffer;
    memset(buffer, 0, sizeof(buffer));
    _foundation_uuid_unparse_upper(_storage, buffer);
    return [ONSString stringWithUTF8String:buffer];
}

// description.getter: -UUIDString sent as a message.
- (ONSString *)description {
    return [(ONSUUID *)self UUIDString];
}

// debugDescription.getter: "<__NSConcreteUUID address> UUIDString".
- (ONSString *)debugDescription {
    return [ONSString stringWithFormat:@"<%s %p> %@",
                                       class_getName(object_getClass(self)),
                                       self,
                                       [(ONSUUID *)self UUIDString]];
}

// classForCoder.getter: the NSUUID class.
- (Class)classForCoder {
    return [ONSUUID class];
}

@end

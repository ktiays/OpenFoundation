//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Archiving/ONSCoder.h"
#import "Foundation/UUID/ONSUUID.h"

// Swift-only message interfaces of NSCoder that __NSConcreteUUID (UUID/UUID.swift) sends. The category
// declares the selectors and defines nothing; each message is an indirect edge into the coder's own class.
// iOS Simulator Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (the selectors of
// the captured objc_msgSend stubs).
@interface ONSCoder (UUIDSwiftMessages)
- (BOOL)allowsKeyedCoding;
- (const unsigned char *)decodeBytesForKey:(id)key returnedLength:(NSUInteger *)lengthp;
- (void)encodeBytes:(const unsigned char *)bytes length:(NSUInteger)length forKey:(id)key;
- (void)failWithError:(id)error;
@end

// Swift-only redeclarations of NSObject's description and debugDescription with the restored string class, so
// that the overrides of __NSConcreteUUID have one type in both naming modes (the prefixed build imports the
// objc/NSObject.h declarations as String, the system-names build as NSString). They define nothing.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-property-type"
@interface ONSUUID (UUIDSwiftDescriptions)
@property(readonly, copy) ONSString *description;
@property(readonly, copy) ONSString *debugDescription;
@end
#pragma clang diagnostic pop

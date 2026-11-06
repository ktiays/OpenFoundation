//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "Foundation/Base/ONSFoundationSupport.h"
#include "Foundation/Error/ONSErrorConstants.h"

#ifdef __OBJC__
#import <objc/NSObject.h>

@class ONSString;
@class ONSDictionary;
@class ONSSet;

// Foundation's NSCFError (__DATA,__objc_data 0x22e6d7920, was a private external), the NSError subclass of
// the toll-free bridged errors, declared in ONSCFError.h.
@class ONSCFError;

// Native class NSError (__DATA,__objc_data 0x22e6d78d0, external); class_ro_t 0x22284d070: instance start 8,
// size 40; ivars _reserved (offset 8, "^v"), _code (16, "q"), _domain (24, "@\"NSString\"") and _userInfo
// (32,
// "@\"NSDictionary\""), each eight bytes with eight-byte alignment, their offset symbols
// _OBJC_IVAR_$_NSError._reserved ... (iOS Simulator 24A434, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified). Its superclass (0x22e6b6088) lies outside the
// image; NSObject is the SDK hierarchy (confidence: medium). The protocol list (0x23d8923a9) and the property
// list (0x23d8923f9) lie outside the extracted image (unread); no protocol is adopted here.
__attribute__((visibility("default")))
// Native ivar-offset symbols are non-external in nm. @package preserves hidden linkage while allowing
// same-image helpers to use the read layout; the original source access-specifier spelling is unread.
// NS_SWIFT_SENDABLE, as the SDK declares NSError.
__attribute__((swift_attr("@Sendable")))
@interface ONSError : NSObject {
    @package
    void *_reserved;
    NSInteger _code;
    ONSString *_domain;
    ONSDictionary *_userInfo;
}

+ (id)errorWithDomain:(id)domain code:(NSInteger)code userInfo:(id)dict;
- (id)initWithDomain:(id)domain code:(NSInteger)code userInfo:(id)dict;
- (id)initWithCoder:(id)coder;
// Typed with the string class so the Swift error types (Error/CocoaError.swift) bridge it directly.
- (ONSString *)domain;
- (NSInteger)code;
- (id)userInfo;
- (id)localizedDescription;
- (id)_callStackReturnAddresses;

@end

// Native category NSError(FoundationSwiftAdditions); the method restored by Error/ONSError.m.
@interface ONSError (FoundationSwiftAdditions)
+ (id)_cocoaErrorWithCode:(NSInteger)code
                     path:(id)path
                      url:(id)url
               underlying:(id)underlying
                  variant:(id)variant
                   source:(id)source
              destination:(id)destination
         debugDescription:(id)debugDescription;
@end

#endif

//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/Archiving/ONSObjCRuntimeNames.h"
#import "Foundation/FileManager/ONSSearchPaths.h"
#import "Foundation/Number/ONSNumber.h"
#import "Foundation/String/ONSAllocateObject.h"
#import "Foundation/String/ONSCharacterSet.h"
#import "Foundation/String/ONSLog.h"
#import "Foundation/String/ONSMutableString.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSString.h"
#import "Foundation/String/ONSStringOtherEncodings.h"

#include <OpenCoreFoundation/Base/OCFBase.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>
#include <os/log.h>
#include <stdint.h>

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native NSString, class reference 0x22289f3d8; FS-string. Message interfaces only, no layout claim.
@interface ONSString (UnrestoredBaseMessages)
- (id)copy;
@end

// Native NSCharacterSet, class reference 0x22289eca0; FC-sets-attributed.

// Native class names looked up during process startup. These Foundation and CoreFoundation classes
// have their own restoration scope; no instances or method declarations are required here.
@class ONSCountedSet;
@class ONSNumberFormatter;
@class ONSDateFormatter;
@class ONSAffineTransform;

#endif

#ifdef __OBJC__
// Native NSMutableString class reference 0x22289f118 (FS-string); message declarations only.

// Native NSNumber class reference 0x22289f158 (FN-number-value); only its class object is used here.

#import "Foundation/Bundle/ONSBundle.h"

#endif

// Native ___CFInitialize, 0x1c01e7bd4, external; CoreFoundation runtime integration.
OF_EXPORT void __OCFInitialize(void);

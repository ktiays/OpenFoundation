//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/objc.h>

// Classes the type checks name (class symbols and runtime names), so that the prefix tool maps them. NSArray,
// NSData, NSDictionary, NSSet, NSOrderedSet and NSTimeZone are classes of the CoreFoundation image; NSString
// is the bridged string class.
#ifdef __OBJC__
@class ONSData;
@class ONSDictionary;
@class ONSSet;
@class ONSOrderedSet;
@class ONSString;
@class ONSTimeZone;
#endif

// The _NSIsNS* type checks of the Objective-C runtime glue (Objective-C/OCFNSTypeChecks.m); external
// natively. _NSIsNSNumber and _NSIsNSCFConstantString of the same native cluster are declared in
// Objective-C/OCFNSObject.h.
OF_EXPORT BOOL _ONSIsNSArray(id object);
OF_EXPORT BOOL _ONSIsNSData(id object);
OF_EXPORT BOOL _ONSIsNSDictionary(id object);
OF_EXPORT BOOL _ONSIsNSOrderedSet(id object);
OF_EXPORT BOOL _ONSIsNSSet(id object);
OF_EXPORT BOOL _ONSIsNSString(id object);
OF_EXPORT BOOL _ONSIsNSTimeZone(id object);
OF_EXPORT BOOL _ONSIsNSIndexSet(id object);

OF_EXPORT BOOL _ONSIsNSValue(id object);
OF_EXPORT BOOL _ONSIsNSURL(id object);
OF_EXPORT BOOL _ONSIsNSAttributedString(id object);

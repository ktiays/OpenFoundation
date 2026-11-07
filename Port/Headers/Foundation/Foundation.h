//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// The Foundation umbrella header for OpenFoundation's wasm32 Foundation, in the SDK's shape. Only the classes,
// methods, functions and constants the port's consumers use are declared; Port/Headers/README.md lists how they
// are checked and which of them OpenFoundation does not implement yet.

#include <CoreFoundation/CoreFoundation.h>

#import <Foundation/NSObjCRuntime.h>
#import <Foundation/NSZone.h>
#import <Foundation/NSObject.h>
#import <Foundation/NSRange.h>
#import <Foundation/NSEnumerator.h>
#import <Foundation/NSString.h>
#import <Foundation/NSCharacterSet.h>
#import <Foundation/NSArray.h>
#import <Foundation/NSDictionary.h>
#import <Foundation/NSSet.h>
#import <Foundation/NSValue.h>
#import <Foundation/NSData.h>
#import <Foundation/NSDate.h>
#import <Foundation/NSNull.h>
#import <Foundation/NSError.h>
#import <Foundation/FoundationErrors.h>
#import <Foundation/NSException.h>
#import <Foundation/NSNotification.h>
#import <Foundation/NSURL.h>
#import <Foundation/NSBundle.h>
#import <Foundation/NSCoder.h>
#import <Foundation/NSKeyedArchiver.h>
#import <Foundation/NSPropertyList.h>
#import <Foundation/NSAutoreleasePool.h>
#import <Foundation/NSRunLoop.h>
#import <Foundation/NSTimer.h>
#import <Foundation/NSProcessInfo.h>
#import <Foundation/NSUUID.h>
#import <Foundation/NSScanner.h>
#import <Foundation/NSPointerFunctions.h>
#import <Foundation/NSHashTable.h>
#import <Foundation/NSPredicate.h>
#import <Foundation/NSPathUtilities.h>
#import <Foundation/NSFileManager.h>
#import <Foundation/NSFormatter.h>
#import <Foundation/NSDateFormatter.h>
#import <Foundation/NSAttributedString.h>
#import <Foundation/NSXPCConnection.h>
#import <Foundation/NSKeyValueCoding.h>
#import <Foundation/NSKeyValueObserving.h>
#import <Foundation/NSThread.h>

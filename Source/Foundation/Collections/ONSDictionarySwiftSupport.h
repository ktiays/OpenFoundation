//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import <objc/NSObject.h>

// The messages the Swift dictionary bridge (Collections/Dictionary+ObjectiveCBridge.swift) sends to an
// Objective-C dictionary, whatever its class: the native code sends them to the object as it comes, so the
// protocol types the receiver and defines nothing. The name is a local descriptive name.
@protocol DictionaryBridgingMessages
- (NSUInteger)count;
- (void)_getObjects:(id *)objects andKeys:(id *)keys count:(NSUInteger)count;
@end

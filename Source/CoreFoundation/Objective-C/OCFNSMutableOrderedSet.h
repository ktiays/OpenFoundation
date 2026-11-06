//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Objective-C/OCFNSOrderedSetHelpers.h"
#include "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#include "CoreFoundation/Objective-C/OCFRequireConcreteImplementation.h"

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

#ifdef __OBJC__
#import "CoreFoundation/Objective-C/OCFNSOrderedSet.h"

// Native class NSMutableOrderedSet (__DATA,__objc_data 0x22e6cc7d0, external), an NSOrderedSet subclass;
// class_ro_t 0x2227bf528: flags 0, instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The method lists
// __OBJC_$_INSTANCE_METHODS_NSMutableOrderedSet (0x1c031c950, 68 methods) and
// __OBJC_$_CLASS_METHODS_NSMutableOrderedSet (0x1c0319540, 1 method) name exactly the methods restored in
// Objective-C/OCFNSMutableOrderedSet.m; the image defines no category on the class (no
// "-[NSMutableOrderedSet(" symbol in the nm listing). The class_ro_t method list pointer is a relative list
// of lists that Local/Scratch/cf-core/macho.py does not decode; the lists were read at their nm addresses.
__attribute__((visibility("default")))
@interface ONSMutableOrderedSet : ONSOrderedSet
@end
#endif

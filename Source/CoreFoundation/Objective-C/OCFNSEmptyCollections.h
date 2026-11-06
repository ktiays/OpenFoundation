//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <objc/objc.h>

// The empty-collection singletons that Objective-C/OCFNSEmptyCollections.m defines.

// Native __NSArray0__struct (__DATA_CONST,__const_cfobj2 0x2227cc250, external): the empty immutable array, a
// 16-byte static object whose isa is __NSArray0 (0x22e6cc550) followed by a zero word (read with
// Local/Scratch/cf-core/macho.py). Its layout is spelled as bytes here; the definition is a record under an
// assembler label.
OF_EXPORT char __ONSArray0__struct[];

// Native __NSArray0__ (__DATA_CONST,__const 0x222429080, external): a pointer to the empty immutable array
// __NSArray0__struct (the word holds 0x2227cc250; read with Local/Scratch/cf-core/macho.py), which
// -[__NSPlaceholderArray initWithObjects:count:] loads and retains.
OF_EXPORT id const __ONSArray0__;

// Native __NSSet0__, 0x22e6d3ea0 (__DATA,__common, was a private external): the empty immutable set, a
// pointer +[__NSPlaceholderSet initialize] stores. Read, never retained, by -initWithObjects: and
// +setWithObjects:; retained by +newSetWithObjects:count:.
#ifdef __OBJC__
OF_EXTERN id __NSSet0__;
#endif

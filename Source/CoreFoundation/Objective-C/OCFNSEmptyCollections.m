//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSEmptyCollections.h"

#include <stdint.h>

// Inferred organization: the empty immutable array object __NSArray0__struct (__DATA_CONST,__const_cfobj2
// 0x2227cc250), its address __NSArray0__ (__DATA_CONST,__const 0x222429080) and the empty immutable set
// pointer __NSSet0__ (__DATA,__common 0x22e6d3ea0) are data of the __NSArray0 and __NSSetI units, which are
// restored elsewhere (Objective-C/OCFNSArray0.m holds the __NSArray0 methods; the __NSSetI unit is family
// I's) without them; family J defined the first and last here, following the batch's assignment, and family
// L-collection-leftovers __NSArray0__. The native source paths are not in the evidence. iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (read with Local/Scratch/cf-core/macho.py
// and Local/Evidence/cf-core/iphone-simulator-nm.txt); iOS Device 24A446: unread. Reading: not yet written;
// the evidence is the static data at the addresses above.

// The __NSArray0 class symbol, the empty array's isa (class restored in Objective-C/OCFNSArray0.m).
extern char emptyArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSArray0));

// Native __NSArray0__struct, 0x2227cc250 (__DATA_CONST,__const_cfobj2, 16 bytes). External: the empty
// immutable array, an object whose isa is __NSArray0, then a zero word. Objective-C/OCFNSEmptyCollections.h
// declares it as a byte array (__ONSArray0__struct); the definition carries that symbol through an assembler
// label, so this record is the object the declaration names. The restoration keeps it in the default constant
// section rather than __const_cfobj2.
__attribute__((visibility("default"))) const struct {
    const void *isa;
    uintptr_t unused;
} emptyArrayObject __asm(OF_SYMBOL("") OF_NAME_STRING(__ONSArray0__struct)) = {emptyArrayClass, 0};

// Native __NSArray0__, 0x222429080 (__DATA_CONST,__const, 8 bytes). External: the address of the empty
// immutable array __NSArray0__struct (the word holds 0x2227cc250, read with Local/Scratch/cf-core/macho.py).
// Defined by family L-collection-leftovers, which owns this unit in batch 6.
id const __ONSArray0__ = (id)&emptyArrayObject;

// Native __NSSet0__, 0x22e6d3ea0 (__DATA,__common). Hidden (was a private external): the empty immutable set,
// zero until +[__NSPlaceholderSet initialize] (family I) stores it.
id __NSSet0__;

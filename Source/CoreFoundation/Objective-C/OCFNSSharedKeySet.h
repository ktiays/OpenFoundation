//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Objective-C/OCFNSDictionary.h"

#ifdef __OBJC__
// __NSSKGraph, the graph the perfect hash is built on (Objective-C/OCFNSSKGraph.m); its methods are messages.
#import "CoreFoundation/Objective-C/OCFNSSKGraph.h"

// Classes outside this unit that the NSSharedKeySet methods reference by name (class symbols and runtime
// names), so that the prefix tool maps them.
@class ONSKeyedUnarchiver;
@class ONSMutableArray;
@class ONSMutableString;
@class ONSString;
@class ONSNumber;
@class ONSDate;
@class ONSURL;

// Native class NSSharedKeySet (__DATA,__objc_data 0x22e6ced50, external), an NSObject subclass (superclass
// pointer 0x22e6b6088, libobjc's NSObject, outside the extracted image); class_ro_t 0x2227c6d10: instance
// start 8, size 73; ivars (instance variable list 0x2227c6b30, offsets from their _OBJC_IVAR_$ symbols, which
// were private externals) _g at 8 (8 bytes), _select at 16 (1 byte), _rankTable at 24 (8), _M at 32 (4),
// _factor at 36 (4), _numKey at 40 (4), _seeds at 48 (8), _keys at 56 (8), _subSharedKeySet at 64 (8) and
// _algorithmType at 72 (1) (iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified for names, offsets and sizes). The type encodings, the property list (0x2227c6c78, nine
// properties), the class property list (0x2227c6ad0) and the three adopted protocols (0x2227c6aa8) name
// strings that lie outside the extracted image (unread): the types are read from the methods' use
// (confidence: medium), and the property attributes from the accessors' code (the subSharedKeySet accessors
// call objc_getProperty and objc_setProperty_atomic: atomic, retain; the others are plain loads and stores).
// The adopted protocols are not declared. The ivars are @private so that their offset symbols stay hidden, as
// natively (were private externals).
__attribute__((visibility("default")))
@interface ONSSharedKeySet : NSObject {
    @private
    uint8_t *_g;
    uint8_t _select;
    void *_rankTable;
    uint32_t _M;
    uint32_t _factor;
    uint32_t _numKey;
    uint32_t *_seeds;
    id *_keys;
    ONSSharedKeySet *_subSharedKeySet;
    uint8_t _algorithmType;
}

@property uint8_t *g;
@property uint8_t select;
@property void *rankTable;
@property uint32_t M;
@property uint32_t factor;
@property uint32_t numKey;
@property uint32_t *seeds;
@property id *keys;
@property(retain) ONSSharedKeySet *subSharedKeySet;

+ (id)keySetWithKeys:(id)keys;
+ (BOOL)supportsSecureCoding;

- (id)initWithKeys:(id *)keys count:(NSUInteger)count;
- (id)initWithCoder:(id)coder;
- (void)encodeWithCoder:(id)coder;
- (id)copyWithZone:(struct _NSZone *)zone;
- (NSUInteger)count;
- (BOOL)isEmpty;
- (NSUInteger)maximumIndex;
- (id)allKeys;
- (NSUInteger)indexForKey:(id)key;
- (id)keyAtIndex:(NSUInteger)index;
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length;
- (NSUInteger)keySetCount;
- (id)debugDescription;

@end
#endif

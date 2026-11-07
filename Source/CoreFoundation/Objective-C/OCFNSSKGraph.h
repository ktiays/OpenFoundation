//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

// The 32-byte edge record of __NSSKGraph's _edges array (malloc_type_calloc of _numE records): the edge's
// index, its two vertices, and the vertex it was peeled from (NSNotFound until -isAcyclic queues it)
// (confidence: verified for positions and widths, read from -addEdgeWithH1:withH2: and -isAcyclic; the
// native type encoding lies outside the extracted image). SKGraphEdge and its field names are local
// descriptive names.
typedef struct {
    NSUInteger index;
    NSUInteger h1;
    NSUInteger h2;
    NSUInteger peelVertex;
} SKGraphEdge;

OF_ABI_ASSERT(sizeof(SKGraphEdge) == 32, "The __NSSKGraph edge record is 32 bytes.");

// The 24-byte vertex record of __NSSKGraph's _vertices array (malloc_type_calloc of _numV records): the
// number of live incident edges, the capacity of the incident-edge list, and the list itself (edge indices,
// terminated by NSNotFound; removed entries hold _dump) (confidence: verified for positions and widths;
// the native type encoding lies outside the extracted image). SKGraphVertex and its field names are local
// descriptive names.
typedef struct {
    NSUInteger degree;
    NSUInteger capacity;
    NSUInteger *edges;
} SKGraphVertex;

OF_ABI_ASSERT(sizeof(SKGraphVertex) == 24, "The __NSSKGraph vertex record is 24 bytes.");

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native class __NSSKGraph (__DATA,__objc_data 0x22e6ceda0, was a private external), the hypergraph
// NSSharedKeySet peels to build its perfect hash, an NSObject subclass (superclass pointer 0x22e6b6088,
// libobjc's NSObject, outside the extracted image); class_ro_t 0x2227c6e88: flags 0x10, instance start 8,
// size 64; ivars (instance variable list 0x2227c6da0, all eight-byte aligned and eight bytes wide, offsets
// from their _OBJC_IVAR_$ symbols, which were private externals) _edges at 8, _vertices at 16, _numV at 24,
// _numE at 32, _currE at 40, _dump at 48 and _l_end at 56 (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for names, offsets and sizes). The type
// encodings and the method type strings lie outside the extracted image (unread): the types are read from
// the methods' use (confidence: medium; every method reads its integer arguments as 64-bit registers). The
// ivars are @private so that their offset symbols stay hidden, as natively. Its methods are restored in
// Objective-C/OCFNSSKGraph.m.
@interface __ONSSKGraph : NSObject {
    @private
    SKGraphEdge *_edges;
    SKGraphVertex *_vertices;
    NSUInteger _numV;
    NSUInteger _numE;
    NSUInteger _currE;
    NSUInteger _dump;
    NSUInteger _l_end;
}

- (id)initWithNumberOfVertices:(NSUInteger)vertices numberOfEdges:(NSUInteger)edges;
- (BOOL)addEdgeWithH1:(NSUInteger)h1 withH2:(NSUInteger)h2;
- (BOOL)isEmpty;
- (NSUInteger)numOfEdges;
- (void **)isAcyclic;

@end
#endif

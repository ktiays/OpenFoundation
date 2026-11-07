//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSSKGraph.h"

#include <malloc/malloc.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: the __NSSKGraph methods (0x1c02dad34-0x1c02db337) lie together in the iOS Simulator
// __text section right after the NSSharedKeySet unit (which ends at 0x1c02dad33); the class has its own
// translation unit, in native order. It has no statics and no C functions. Objective-C compiled with manual
// reference counting (no retain or release; -dealloc reaches super through objc_msgSendSuper2). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta12.json (family P-collections-url); the class_ro_t
// read with Local/Scratch/cf-core/macho.py. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The terminator of a vertex's incident-edge list and the "not peeled" mark of an edge (0x7FFFFFFFFFFFFFFF).
static const NSUInteger kGraphNotFound = LONG_MAX;

// The growth of a vertex's incident-edge list -addEdgeWithH1:withH2: inlines for each endpoint: an empty
// vertex gets a new list of four entries (malloc_type_calloc(4, sizeof(NSUInteger), 0x100004000313F17)) whose
// first entry is NSNotFound (the capacity and the list stored as one pair); a list whose capacity minus one
// equals the count is replaced by one of twice the capacity (the same type ID), the old entries copied
// (memcpy of the old capacity's bytes) and the old list freed. Then the current edge index is stored at the
// count, the count incremented, and NSNotFound stored after it. appendIncidentEdge is a local descriptive
// name. CoreFoundation defect (kept): a vertex whose count is 0 but whose list exists (only after -isAcyclic
// has peeled it) gets a new list and the old one is lost; -addEdgeWithH1:withH2: is not sent after -isAcyclic
// by the image's only client, NSSharedKeySet (confidence: verified for the code path, medium for the
// reachability).
static inline void appendIncidentEdge(SKGraphVertex *vertex, NSUInteger edgeIndex) {
    NSUInteger *list;
    if (vertex->degree == 0) {
        list = (NSUInteger *)malloc_type_calloc(4, sizeof(NSUInteger), 0x100004000313F17);
        vertex->capacity = 4;
        vertex->edges = list;
        list[0] = kGraphNotFound;
    } else if (vertex->capacity - 1 == vertex->degree) {
        NSUInteger capacity = vertex->capacity << 1;
        list = (NSUInteger *)malloc_type_calloc(capacity, sizeof(NSUInteger), 0x100004000313F17);
        memcpy(list, vertex->edges, vertex->capacity * sizeof(NSUInteger));
        free(vertex->edges);
        vertex->capacity = capacity;
        vertex->edges = list;
    } else {
        list = vertex->edges;
    }
    list[vertex->degree] = edgeIndex;
    NSUInteger degree = vertex->degree + 1;
    vertex->degree = degree;
    list[degree] = kGraphNotFound;
}

@implementation __ONSSKGraph

// Native -[__NSSKGraph initWithNumberOfVertices:numberOfEdges:], 0x1c02dad34. Non-external. After -[NSObject
// init] (objc_msgSendSuper2; selector reference 0x2387aa0ea, outside the extracted image; -init inferred,
// confidence medium), a non-nil object gets the vertex records (malloc_type_calloc(vertices, 0x18,
// 0x1010040A79CA2DE)) and the edge records (malloc_type_calloc(edges, 0x20, 0x1000040E0EAB150)), then
// _numV and _numE, _dump = edges + 1 with _l_end = 0, and _currE = 0, in that store order. The callocs'
// results are not checked.
- (id)initWithNumberOfVertices:(NSUInteger)vertices numberOfEdges:(NSUInteger)edges {
    self = [super init];
    if (self != nil) {
        _vertices = (SKGraphVertex *)malloc_type_calloc(vertices, 0x18, 0x1010040A79CA2DE);
        _edges = (SKGraphEdge *)malloc_type_calloc(edges, 0x20, 0x1000040E0EAB150);
        _numV = vertices;
        _numE = edges;
        _dump = edges + 1;
        _l_end = 0;
        _currE = 0;
    }
    return self;
}

// Native -[__NSSKGraph dealloc], 0x1c02dae08. Non-external. The edge records are freed, then each vertex's
// incident-edge list that is not NULL, from the last vertex to the first (_vertices read again for each),
// then the vertex records; then super's -dealloc.
- (void)dealloc {
    free(_edges);
    for (NSUInteger index = _numV; index != 0; index--) {
        NSUInteger *list = _vertices[index - 1].edges;
        if (list != NULL) {
            free(list);
        }
    }
    free(_vertices);
    [super dealloc];
}

// Native -[__NSSKGraph addEdgeWithH1:withH2:], 0x1c02daebc. Non-external. The edge record at _currE gets the
// index _currE, h1, h2 and NSNotFound as its peel vertex (_currE read again before each of the last two
// stores); the edge is appended to h1's incident list, then to h2's (the growth above); _currE is
// incremented and YES answered. No bound is checked.
- (BOOL)addEdgeWithH1:(NSUInteger)h1 withH2:(NSUInteger)h2 {
    SKGraphEdge *edges = _edges;
    SKGraphVertex *vertices = _vertices;
    edges[_currE].index = _currE;
    edges[_currE].h1 = h1;
    edges[_currE].h2 = h2;
    edges[_currE].peelVertex = kGraphNotFound;
    appendIncidentEdge(&vertices[h1], _currE);
    appendIncidentEdge(&_vertices[h2], _currE);
    _currE = _currE + 1;
    return YES;
}

// Native -[__NSSKGraph isEmpty], 0x1c02db07c. Non-external. Whether _l_end equals _numE (every edge peeled).
- (BOOL)isEmpty {
    return _l_end == _numE;
}

// Native -[__NSSKGraph numOfEdges], 0x1c02db090. Non-external. _numE.
- (NSUInteger)numOfEdges {
    return _numE;
}

// The removal -isAcyclic inlines for each endpoint of a dequeued edge: the endpoint's incident list is
// scanned while the index is below its capacity (read again each step); an entry equal to the edge's index
// (read again each step) is replaced by _dump and the vertex's count decremented; the scan stops at the
// NSNotFound terminator. removeIncidentEdge is a local descriptive name.
static inline void removeIncidentEdge(__ONSSKGraph *graph, SKGraphVertex *vertex, const SKGraphEdge *edge);

// The queueing -isAcyclic inlines for each endpoint left with a single other edge: the first entry of its
// incident list that is not _dump (the scan bounded by the capacity; none leaves everything unchanged)
// names an edge; one not yet queued (peel vertex NSNotFound) is queued and its peel vertex set to the
// endpoint (the edge records read again from _edges); either way that entry becomes _dump and the endpoint's
// count drops (the vertex records are those -isAcyclic read once before its walk). queueLastIncidentEdge is a
// local descriptive name.
static inline void queueLastIncidentEdge(
    __ONSSKGraph *graph,
    SKGraphVertex *vertices,
    NSUInteger vertexIndex,
    SKGraphEdge **queue,
    NSUInteger *queued
);

// Native -[__NSSKGraph isAcyclic], 0x1c02db098. Non-external. The peeling order (malloc_type_calloc(_numE, 8,
// 0x2004093837F09), answered; the caller frees it) and a work queue (the same call, freed before return).
// First every edge with an endpoint of count 1 (h1 tested first, then h2) is queued with that endpoint as
// its peel vertex (_vertices read once per edge, _numE again each step). Then, for each queued edge in
// queue order (the queue grows while it is walked): it is appended to the order at _l_end (incremented);
// each endpoint whose count is 2 before the removals is noted; the edge is removed from h1's list, then
// from h2's (h2 read again from the edge); then the noted h1 endpoint, then the noted h2 endpoint, has its
// remaining edge queued. The answer does not say whether the graph is acyclic: -isEmpty does.
- (void **)isAcyclic {
    void **order = (void **)malloc_type_calloc(_numE, 8, 0x2004093837F09);
    SKGraphEdge **queue = (SKGraphEdge **)malloc_type_calloc(_numE, 8, 0x2004093837F09);
    if (_numE != 0) {
        NSUInteger queued = 0;
        NSUInteger index = 0;
        do {
            SKGraphVertex *vertices = _vertices;
            SKGraphEdge *edge = &_edges[index];
            NSUInteger vertex = edge->h1;
            bool leaf = vertices[vertex].degree == 1;
            if (!leaf) {
                vertex = edge->h2;
                leaf = vertices[vertex].degree == 1;
            }
            if (leaf) {
                edge->peelVertex = vertex;
                queue[queued++] = edge;
            }
            index++;
        } while (index < _numE);
        if (queued != 0) {
            NSUInteger position = 0;
            SKGraphVertex *vertices = _vertices;
            do {
                SKGraphEdge *edge = queue[position];
                order[_l_end++] = edge;
                NSUInteger h1 = edge->h1;
                NSUInteger h2 = edge->h2;
                NSUInteger first = vertices[h1].degree == 2 ? h1 : kGraphNotFound;
                NSUInteger second = vertices[h2].degree == 2 ? h2 : kGraphNotFound;
                removeIncidentEdge(self, &vertices[h1], edge);
                removeIncidentEdge(self, &vertices[edge->h2], edge);
                if (first != kGraphNotFound) {
                    queueLastIncidentEdge(self, vertices, first, queue, &queued);
                }
                if (second != kGraphNotFound) {
                    queueLastIncidentEdge(self, vertices, second, queue, &queued);
                }
                position++;
            } while (position < queued);
        }
    }
    free(queue);
    return order;
}

static inline void removeIncidentEdge(__ONSSKGraph *graph, SKGraphVertex *vertex, const SKGraphEdge *edge) {
    if (vertex->capacity == 0) {
        return;
    }
    NSUInteger *list = vertex->edges;
    NSUInteger slot = 0;
    do {
        NSUInteger entry = list[slot];
        if (entry == edge->index) {
            list[slot] = graph->_dump;
            vertex->degree -= 1;
            entry = list[slot];
        }
        if (entry == kGraphNotFound) {
            return;
        }
        slot++;
    } while (slot < vertex->capacity);
}

static inline void queueLastIncidentEdge(
    __ONSSKGraph *graph,
    SKGraphVertex *vertices,
    NSUInteger vertexIndex,
    SKGraphEdge **queue,
    NSUInteger *queued
) {
    SKGraphVertex *vertex = &vertices[vertexIndex];
    NSUInteger remaining = vertex->capacity;
    if (remaining == 0) {
        return;
    }
    NSUInteger *entry = vertex->edges;
    NSUInteger dump = graph->_dump;
    while (*entry == dump) {
        entry++;
        if (--remaining == 0) {
            return;
        }
    }
    SKGraphEdge *other = &graph->_edges[*entry];
    if (other->peelVertex == kGraphNotFound) {
        queue[(*queued)++] = other;
        other->peelVertex = vertexIndex;
        dump = graph->_dump;
    }
    *entry = dump;
    vertex->degree -= 1;
}

@end

//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSSharedKeySet.h"
#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/Error/OCFError.h"

#include <dispatch/dispatch.h>
#include <malloc/malloc.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: the NSSharedKeySet methods (0x1c02d87a0-0x1c02dad33) lie together in the iOS
// Simulator __text section after the NSSharedKeyDictionary unit, with the unit's C functions among them
// (_compareHashesAscending at 0x1c02d9608 after -initWithKeys:count:, ___archiveIsCorrupt, listed as
// ___archiveIsCorrupt_0, at 0x1c02d9e38 and the -initWithCoder: block at 0x1c02d9ed0 after -initWithCoder:);
// the class has its own translation unit, in native order. The unit's statics: the -initWithCoder: once token
// and property-list class set (__DATA,__bss, listed as _initWithCoder:.onceToken_0 and .oPlistClasses_0; one
// of the two _initWithCoder:.onceToken/.oPlistClasses pairs at 0x22e6d3be0-0x22e6d3bf0) and the
// -countByEnumeratingWithState:objects:count: const_mu (listed as const_mu_8; value 1 like every const_mu of
// the image, read with Local/Scratch/cf-core/macho.py; its address, presumably 0x1c046de00 after the
// -debugDescription bit masks 0x1c046dde0-0x1c046ddff, confidence: medium). Objective-C compiled with manual
// reference counting (objc_retain, objc_release, objc_autorelease) and Objective-C exceptions (the @try of
// -initWithCoder:). Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json. Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// Class symbols sent to directly (class references off_2227CE3B8, off_2227CE2A0, off_2227CE360, and
// _OBJC_CLASS_$___NSSKGraph addressed directly); NSException is dictionaryExceptionClass
// (Objective-C/OCFNSDictionary.h).
extern char sharedKeySetSetClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSSet));
extern char sharedKeySetArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSArray));
extern char sharedKeySetMutableArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableArray));
extern char sharedKeySetDictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDictionary));
extern char sharedKeySetGraphClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(__ONSSKGraph));

// Selectors this unit sends that no included header declares (the coders', the collections' and the
// strings'), declared for the message sends only; the protocol is never adopted and no protocol object is
// emitted. The graph's selectors are declared by Objective-C/OCFNSSKGraph.h.
@protocol SharedKeySetMessages

- (BOOL)allowsKeyedCoding;
- (void)encodeInt64:(int64_t)value forKey:(id)key;
- (void)encodeInt32:(int32_t)value forKey:(id)key;
- (void)encodeInteger:(NSInteger)value forKey:(id)key;
- (void)encodeBytes:(const uint8_t *)bytes length:(NSUInteger)length forKey:(id)key;
- (void)encodeObject:(id)object forKey:(id)key;
- (int64_t)decodeInt64ForKey:(id)key;
- (id)decodeObjectOfClass:(Class)aClass forKey:(id)key;
- (id)decodeObjectOfClasses:(id)classes forKey:(id)key;
- (void)failWithError:(id)error;
- (id)error;
- (id)allowedClasses;
- (void)replaceObject:(id)object withObject:(id)newObject;
- (id)arrayWithObjects:(const id *)objects count:(NSUInteger)count;
- (id)arrayWithArray:(id)array;
- (void)addObjectsFromArray:(id)array;
- (id)setByAddingObjectsFromSet:(id)set;
- (void)removeObject:(id)object;
- (id)setWithObjects:(id)firstObject, ...;
- (id)objectAtIndexedSubscript:(NSUInteger)index;
- (id)dictionaryWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count;
- (id)array;
- (id)_initByAdoptingBuffer:(id *)buffer count:(NSUInteger)count size:(NSUInteger)size;
- (id)string;
- (void)appendFormat:(id)format, ...;
- (id)stringWithCharacters:(const UniChar *)characters length:(NSUInteger)length;
- (id)stringWithFormat:(id)format, ...;

@end

// The -initWithKeys:count: fast-enumeration record of a key and its hash (16 bytes, malloc_type_malloc type
// 0x1080040FC6463CF), sorted by _compareHashesAscending. KeyHashPair is a local descriptive name.
typedef struct {
    id key;
    NSUInteger hash;
} KeyHashPair;

// -initWithKeys:count: reads the graph's edge records (SKGraphEdge, Objective-C/OCFNSSKGraph.h) directly
// through the order -[__NSSKGraph isAcyclic] answers, using the low 32 bits of h1, h2 and peelVertex.

// The raise of the native temporary id buffers (the new_id_array pattern) for a count with any of its top
// four bits set: NSGenericException, the reason (default allocator) released before the throw.
// raiseTemporaryBufferOverflow is a local descriptive name for the inlined code.
static inline __attribute__((noreturn)) void raiseTemporaryBufferOverflow(unsigned long count) {
    OCFStringRef reason = OCFStringCreateWithFormat(
        NULL,
        NULL,
        OF_STR(
            "*** attempt to create a temporary id buffer which is too large or with a negative count (%lu) "
            "-- "
            "possibly data is corrupt"
        ),
        count
    );
    id exception = [(id)dictionaryExceptionClass exceptionWithName:(id)ONSGenericException
                                                            reason:(id)reason
                                                          userInfo:nil];
    OCFRelease(reason);
    objc_exception_throw(exception);
}

// A 32-bit rotation, the EXTR of the hash mixing. rotateLeft is a local descriptive name.
static inline uint32_t rotateLeft(uint32_t value, unsigned bits) {
    return (value << bits) | (value >> (32 - bits));
}

// The final mixing every hash site inlines (Jenkins' lookup3 final(a, b, c), the order of the native
// EOR/EXTR/SUB sequence): b and c are answered. finalMix is a local descriptive name.
static inline void finalMix(uint32_t a, uint32_t b, uint32_t c, uint32_t *outB, uint32_t *outC) {
    c ^= b;
    c -= rotateLeft(b, 14);
    a ^= c;
    a -= rotateLeft(c, 11);
    b ^= a;
    b -= rotateLeft(a, 25);
    c ^= b;
    c -= rotateLeft(b, 16);
    a ^= c;
    a -= rotateLeft(c, 4);
    b ^= a;
    b -= rotateLeft(a, 14);
    c ^= b;
    c -= rotateLeft(b, 24);
    *outB = b;
    *outC = c;
}

// The prime search -initWithKeys:count: inlines twice: from half the vertex count, the odd number above it,
// then (unless half is 1 or 2) the first candidate, stepping by 2, that is not a multiple of 3 and is below
// 25 or has no divisor 6k - 1 or 6k + 1 up to its square root; a candidate of 1 or less ends the search at
// the next one. sharedKeySetPrime is a local descriptive name.
static inline uint32_t sharedKeySetPrime(uint32_t vertices) {
    uint32_t half = vertices >> 1;
    uint32_t candidate = (half + 1) | 1;
    if (half - 3 > 0xFFFFFFFDU) {
        return candidate;
    }
    for (;;) {
        if (candidate * 0xAAAAAAABU >= 0x55555556U) {
            if (candidate < 25) {
                return candidate;
            }
            uint32_t divisor = 11;
            for (;;) {
                if (candidate % (divisor - 6) == 0) {
                    break;
                }
                if (candidate % (divisor - 4) == 0) {
                    break;
                }
                uint32_t square = divisor * divisor;
                divisor += 6;
                if (square > candidate) {
                    return candidate;
                }
            }
        }
        uint32_t next = candidate + 2;
        if (candidate <= 1) {
            return next;
        }
        candidate = next;
    }
}

static int compareHashesAscending(const void *first, const void *second);

static id __archiveIsCorrupt(id reason);

@implementation ONSSharedKeySet

// Native +[NSSharedKeySet keySetWithKeys:], 0x1c02d87a0. Non-external. A nil argument throws
// NSInvalidArgumentException "%@: keys cannot be nil", one failing _NSIsNSArray "%@: keys must be a kind of
// NSArray instead of '%@'" (the __CFExceptionProem of the receiver and selector, then the argument's class;
// kCFAllocatorSystemDefault; autoreleased through _CFAutoreleasePoolAddObject). Otherwise the keys are read
// into a temporary id buffer (the new_id_array pattern: a count with any of its top four bits set throws; on
// the stack up to 0x100 entries, otherwise _CFCreateArrayStorage, freed afterwards) with -getObjects:range:,
// and the answer is [[self alloc] initWithKeys:count:], autoreleased.
+ (id)keySetWithKeys:(id)keys {
    OCFStringRef reason;
    if (keys == nil) {
        reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault, NULL, OF_STR("%@: keys cannot be nil"), __CFExceptionProem(self, _cmd)
        );
    } else if (!_ONSIsNSArray(keys)) {
        OCFStringRef proem = __CFExceptionProem(self, _cmd);
        Class keysClass = [keys class];
        reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("%@: keys must be a kind of NSArray instead of '%@'"),
            proem,
            keysClass
        );
    } else {
        NSUInteger count = [keys count];
        if ((count >> 60) != 0) {
            raiseTemporaryBufferOverflow(count);
        }
        NSUInteger allocationCount = count > 1 ? count : 1;
        id stackKeys[count < 0x101 ? allocationCount : 1];
        size_t storageSize = 0;
        id *heapKeys = NULL;
        id *buffer = stackKeys;
        if (count >= 0x101) {
            buffer = (id *)_OCFCreateArrayStorage(allocationCount, false, &storageSize);
            heapKeys = buffer;
        }
        [keys getObjects:buffer range:(OCFRange){0, (OCFIndex)count}];
        id keySet = [[[self alloc] initWithKeys:buffer count:count] autorelease];
        free(heapKeys);
        return keySet;
    }
    objc_exception_throw([(id)dictionaryExceptionClass
        exceptionWithName:(id)ONSInvalidArgumentException
                   reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                 userInfo:nil]);
}

// Native -[NSSharedKeySet initWithKeys:count:], 0x1c02d89c4. Non-external. Builds the minimal perfect hash of
// the keys (a random two-vertex hypergraph over -hash, peeled acyclic by __NSSKGraph). After [super init]
// (nil answers nil): no keys set _numKey to 0 and answer; a count with any of its top four bits set throws
// (the new_id_array raise). The unique keys and their hashes: a single key is taken as is; otherwise the keys
// are uniqued through [[NSSet alloc] initWithObjects:count:], each paired with its -hash (fast enumeration, a
// 32-bit pair index), sorted by hash (qsort, _compareHashesAscending), and a key whose hash equals the
// previous pair's goes to the duplicates (the sub key set) instead. The unique key and duplicate buffers live
// on the stack up to 0x100 keys, otherwise in two _CFCreateArrayStorage blocks (the unique keys first); the
// hashes in malloc_type_malloc(8 * count, 0x100004000313F17). _numKey takes the unique count, _algorithmType
// 1; the vertex count M is twice the prime above ceil(unique * 2.125) / 2 (_factor the prime), _select the
// rank width for M (0 up to 0xFE, 1 up to 0xFFFE, else 2), _seeds malloc_type_calloc(2, 4,
// 0x100004052888210). Up to 1000 attempts: both seeds from arc4random_buf; from the second attempt the factor
// grows by 0.125 (FCVTPU: the ceiling) and M, _factor and _select are recomputed; a new __NSSKGraph gets one
// edge per unique hash (the vertices 2 (c % factor) and 2 (b % factor) + 1 of lookup3's final mix over the
// seeds and the 64-bit hash); -isAcyclic answers the peeling order (malloc'd, freed here) and -isEmpty
// whether the graph emptied. 1000 failures, or an empty graph without an order, release the graph and self
// and answer nil. Otherwise the vertex values g (malloc_type_calloc(M, 1, 0x100004077774924), all 2) are
// assigned in reverse peeling order, the rank table (malloc_type_calloc of M entries of 1, 2 or 4 bytes:
// 0x100004077774924, 0x1000040BDFB0063, 0x100004052888210) counts the assigned vertices, unassigned vertices
// become 0, and g is packed into bits (malloc_type_calloc(M / 8 + 1, 1, 0x100004077774924); full bytes from
// bit 7 down, then the remaining vertices from M - 1 down into the byte at M / 8). _keys
// (malloc_type_calloc(unique, 8, 0x80040B8603338)) receives each unique key's -copyWithZone: NULL at its rank
// (lookup by _algorithmType: 1 adds the hash's high word, 0 does not, any other value is BRK #1; an unknown
// _select is BRK #1). Duplicates make the sub key set with
// [[[self class] alloc] initWithKeys:count:], stored without a further retain; nil releases self and answers
// nil. Finally the order is freed, the graph released, and the hashes and both storage blocks freed.
- (id)initWithKeys:(id *)keys count:(NSUInteger)count {
    self = [super init];
    if (self == nil) {
        return nil;
    }
    if (count == 0) {
        _numKey = 0;
        return self;
    }
    if ((count >> 60) != 0) {
        raiseTemporaryBufferOverflow(count);
    }
    id uniqueStack[count < 0x101 ? count : 1];
    id *uniqueKeys = uniqueStack;
    size_t uniqueSize = 0;
    void *uniqueHeap = NULL;
    if (count > 0x100) {
        uniqueHeap = _OCFCreateArrayStorage(count, false, &uniqueSize);
        uniqueKeys = (id *)uniqueHeap;
    }
    id duplicateStack[count < 0x101 ? count : 1];
    size_t duplicateSize = 0;
    void *duplicateHeap = NULL;
    if (count > 0x100) {
        duplicateHeap = _OCFCreateArrayStorage(count, false, &duplicateSize);
    }
    id *duplicateKeys = count < 0x101 ? duplicateStack : (id *)duplicateHeap;
    NSUInteger *hashes = (NSUInteger *)malloc_type_malloc(count * sizeof(NSUInteger), 0x100004000313F17);
    NSUInteger duplicateCount;
    NSUInteger uniqueCount;
    if (count == 1) {
        id key = keys[0];
        uniqueKeys[0] = key;
        hashes[0] = [key hash];
        duplicateCount = 0;
        uniqueCount = 1;
    } else {
        id set = [[(Class)sharedKeySetSetClass alloc] initWithObjects:keys count:count];
        NSUInteger setCount = [set count];
        KeyHashPair *pairs =
            (KeyHashPair *)malloc_type_malloc(setCount * sizeof(KeyHashPair), 0x1080040FC6463CF);
        uint32_t pairIndex = 0;
        for (id key in set) {
            KeyHashPair *pair = &pairs[pairIndex];
            pair->key = key;
            pair->hash = [key hash];
            pairIndex += 1;
        }
        [set release];
        qsort(pairs, setCount, sizeof(KeyHashPair), compareHashesAscending);
        uniqueKeys[0] = pairs[0].key;
        hashes[0] = pairs[0].hash;
        duplicateCount = 0;
        uniqueCount = 1;
        if (setCount >= 2) {
            uint32_t next = 2;
            uint32_t current = 1;
            for (;;) {
                KeyHashPair *pair = &pairs[current];
                if (pair->hash == pairs[current - 1].hash) {
                    duplicateKeys[duplicateCount] = pair->key;
                    duplicateCount += 1;
                } else {
                    uniqueKeys[uniqueCount] = pair->key;
                    hashes[uniqueCount] = pair->hash;
                    uniqueCount += 1;
                }
                current = next;
                if (setCount <= next++) {
                    break;
                }
            }
        }
        free(pairs);
    }
    _numKey = (uint32_t)uniqueCount;
    _algorithmType = 1;
    uint32_t prime = sharedKeySetPrime((uint32_t)ceil((double)(uint32_t)uniqueCount * 2.125));
    uint32_t vertices = prime << 1;
    _M = vertices;
    _factor = prime;
    if (vertices <= 0xFE) {
        _select = 0;
    } else {
        _select = vertices <= 0xFFFE ? 1 : 2;
    }
    _seeds = (uint32_t *)malloc_type_calloc(2, 4, 0x100004052888210);
    uint32_t edgeCount = (uint32_t)uniqueCount;
    double factor = 2.125;
    uint32_t attempt = 0;
    id graph;
    void **order;
    for (;;) {
        arc4random_buf(_seeds, 4);
        arc4random_buf(_seeds + 1, 4);
        if (attempt != 0) {
            factor = factor + 0.125;
            uint32_t retryPrime = sharedKeySetPrime((uint32_t)ceil(factor * (double)_numKey));
            uint32_t retryVertices = retryPrime << 1;
            uint8_t retrySelect = retryVertices >= 0xFFFF ? 2 : 1;
            if (retryVertices < 0xFF) {
                retrySelect = 0;
            }
            _M = retryVertices;
            _factor = retryPrime;
            _select = retrySelect;
        }
        graph = [[(Class)sharedKeySetGraphClass alloc] initWithNumberOfVertices:_M numberOfEdges:edgeCount];
        NSUInteger *hash = hashes;
        for (NSUInteger remaining = edgeCount; remaining != 0; remaining--) {
            uint64_t value = *hash++;
            uint32_t base = _seeds[0] + 0xDEADBEF7;
            uint32_t b;
            uint32_t c;
            finalMix(base + (uint32_t)value, base + (uint32_t)(value >> 32), base + _seeds[1], &b, &c);
            uint32_t modulus = _factor;
            [graph addEdgeWithH1:(c % modulus) << 1 withH2:1 | ((b % modulus) << 1)];
        }
        order = [graph isAcyclic];
        if ([graph isEmpty]) {
            break;
        }
        free(order);
        [graph release];
        if (++attempt == 1000) {
            graph = nil;
            goto failure;
        }
    }
    if (order == NULL) {
        goto failure;
    }
    {
        NSUInteger edges = [graph numOfEdges];
        uint8_t *g = (uint8_t *)malloc_type_calloc(_M, 1, 0x100004077774924);
        memset(g, 2, _M);
        for (; edges != 0; edges--) {
            SKGraphEdge *edge = (SKGraphEdge *)order[edges - 1];
            uint8_t assigned;
            if ((uint32_t)edge->h1 == (uint32_t)edge->peelVertex) {
                assigned = g[(uint32_t)edge->h2] & 1;
            } else {
                assigned = 1 & ~g[(uint32_t)edge->h1];
            }
            g[(uint32_t)edge->peelVertex] = assigned;
        }
        _g = g;
        uint32_t vertexCount = _M;
        void *rank;
        switch (_select) {
            case 2: {
                uint32_t *table = (uint32_t *)malloc_type_calloc(vertexCount, 4, 0x100004052888210);
                rank = table;
                uint32_t counter = 0;
                for (NSUInteger index = 0; index < vertexCount; index++) {
                    table[index] = counter;
                    if (g[index] != 2) {
                        counter += 1;
                    }
                }
                break;
            }
            case 1: {
                uint16_t *table = (uint16_t *)malloc_type_calloc(vertexCount, 2, 0x1000040BDFB0063);
                rank = table;
                uint32_t counter = 0;
                for (NSUInteger index = 0; index < vertexCount; index++) {
                    table[index] = (uint16_t)counter;
                    if (g[index] != 2) {
                        counter += 1;
                    }
                }
                break;
            }
            case 0: {
                uint8_t *table = (uint8_t *)malloc_type_calloc(vertexCount, 1, 0x100004077774924);
                rank = table;
                uint32_t counter = 0;
                for (NSUInteger index = 0; index < vertexCount; index++) {
                    table[index] = (uint8_t)counter;
                    if (g[index] != 2) {
                        counter += 1;
                    }
                }
                break;
            }
            default:
                rank = NULL;
                break;
        }
        for (NSUInteger index = vertexCount; index != 0; index--) {
            if (g[index - 1] == 2) {
                g[index - 1] = 0;
            }
        }
        uint8_t *bits = (uint8_t *)malloc_type_calloc((vertexCount >> 3) + 1, 1, 0x100004077774924);
        uint64_t fullBytes = vertexCount & 0xFFFFFFF8;
        uint32_t tailBase = 0;
        if ((uint32_t)fullBytes != 0) {
            uint64_t position = 0;
            do {
                uint32_t byte = g[position + 7] != 0 ? 2 : 0;
                for (uint64_t bit = position + 6; bit > position; bit--) {
                    byte = (byte | (g[bit] != 0)) << 1;
                }
                byte |= g[position] != 0;
                bits[position >> 3] = (uint8_t)byte;
                position += 8;
            } while (position < fullBytes);
            tailBase = (uint32_t)position;
        }
        uint32_t tail = g[(uint32_t)(vertexCount - 1)] != 0;
        uint32_t tailIndex = vertexCount - 2;
        if (tailIndex >= tailBase) {
            tail = (g[tailIndex] != 0) | (tail << 1);
            if (tailIndex != 0) {
                uint64_t bit = tailIndex;
                uint32_t limit = vertexCount - 3;
                while (limit >= tailBase) {
                    tail = (g[bit - 1] != 0) | (tail << 1);
                    limit -= 1;
                    bit -= 1;
                    if (bit == 0) {
                        break;
                    }
                }
            }
        }
        bits[vertexCount >> 3] = (uint8_t)tail;
        free(g);
        _g = bits;
        _rankTable = rank;
        _keys = (id *)malloc_type_calloc(uniqueCount, 8, 0x80040B8603338);
        for (uint32_t index = 0;;) {
            uint32_t base;
            uint32_t a;
            uint32_t b;
            uint32_t c;
            uint64_t value = hashes[index];
            if (_algorithmType == 1) {
                base = _seeds[0] + 0xDEADBEF7;
                c = base + _seeds[1];
                b = base + (uint32_t)(value >> 32);
            } else if (_algorithmType == 0) {
                base = _seeds[0] + 0xDEADBEF3;
                c = base + _seeds[1];
                b = base;
            } else {
                __builtin_trap();
            }
            a = base + (uint32_t)value;
            finalMix(a, b, c, &b, &c);
            uint32_t modulus = _factor;
            uint32_t vertexPair[2];
            vertexPair[0] = (c % modulus) << 1;
            vertexPair[1] = 1 | ((b % modulus) << 1);
            uint32_t first = (_g[vertexPair[0] >> 3] >> (vertexPair[0] & 6));
            uint32_t second = (_g[vertexPair[1] >> 3] >> (vertexPair[1] & 7));
            uint32_t vertex = vertexPair[(second ^ first) & 1];
            NSUInteger slot;
            switch (_select) {
                case 2:
                    slot = ((uint32_t *)_rankTable)[vertex];
                    break;
                case 1:
                    slot = ((uint16_t *)_rankTable)[vertex];
                    break;
                case 0:
                    slot = ((uint8_t *)_rankTable)[vertex];
                    break;
                default:
                    __builtin_trap();
            }
            _keys[slot] = [uniqueKeys[index] copyWithZone:NULL];
            if (uniqueCount <= ++index) {
                break;
            }
        }
        if (duplicateCount != 0) {
            id subKeySet = [[[self class] alloc] initWithKeys:duplicateKeys count:duplicateCount];
            if (subKeySet != nil) {
                _subSharedKeySet = subKeySet;
            } else {
                [self release];
                self = nil;
            }
        }
        free(order);
        goto cleanup;
    }
failure:
    [graph release];
    [self release];
    self = nil;
    graph = nil;
cleanup:
    [graph release];
    free(hashes);
    free(duplicateHeap);
    free(uniqueHeap);
    return self;
}

// Native _compareHashesAscending, 0x1c02d9608. Non-external. The qsort comparator of -initWithKeys:count::
// the hashes (the words at 8) compared unsigned, answering 1, -1 or 0.
static int compareHashesAscending(const void *first, const void *second) {
    NSUInteger firstHash = ((const KeyHashPair *)first)->hash;
    NSUInteger secondHash = ((const KeyHashPair *)second)->hash;
    if (firstHash < secondHash) {
        return -1;
    }
    return firstHash > secondHash;
}

// Native -[NSSharedKeySet init], 0x1c02d9620. Non-external. -initWithKeys:NULL count:0 (a tail call).
- (id)init {
    return [self initWithKeys:NULL count:0];
}

// Native -[NSSharedKeySet copyWithZone:], 0x1c02d962c. Non-external. The receiver, retained.
- (id)copyWithZone:(struct _NSZone *)zone {
    return [self retain];
}

// Native -[NSSharedKeySet encodeWithCoder:], 0x1c02d9630. Non-external. A coder that does not allow keyed
// coding throws NSInvalidArgumentException "%@: this object can only be encoded by a keyed coder". NS.numKey
// (-encodeInt64:forKey:); with keys also NS.select (-encodeInt32:forKey:), NS.factor, NS.seed0, NS.seed1 and
// NS.M (-encodeInt64:forKey:), NS.rankTable (-encodeBytes:length:forKey:, M entries of the _select width; an
// unknown _select encodes none), NS.g (M / 8 + 1 bytes), NS.subskset (-encodeObject:forKey:), NS.keys (an
// NSArray of the keys) and NS.algorithmType (bit 0, -encodeInteger:forKey:, a tail call).
- (void)encodeWithCoder:(id)coder {
    if (![coder allowsKeyedCoding]) {
        OCFStringRef reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("%@: this object can only be encoded by a keyed coder"),
            __CFExceptionProem(self, _cmd)
        );
        objc_exception_throw([(id)dictionaryExceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                     userInfo:nil]);
    }
    [coder encodeInt64:_numKey forKey:(id)OF_STR("NS.numKey")];
    if (_numKey == 0) {
        return;
    }
    [coder encodeInt32:_select forKey:(id)OF_STR("NS.select")];
    [coder encodeInt64:_factor forKey:(id)OF_STR("NS.factor")];
    [coder encodeInt64:_seeds[0] forKey:(id)OF_STR("NS.seed0")];
    [coder encodeInt64:_seeds[1] forKey:(id)OF_STR("NS.seed1")];
    [coder encodeInt64:_M forKey:(id)OF_STR("NS.M")];
    switch (_select) {
        case 2:
            [coder encodeBytes:_rankTable length:(NSUInteger)_M << 2 forKey:(id)OF_STR("NS.rankTable")];
            break;
        case 1:
            [coder encodeBytes:_rankTable length:(NSUInteger)_M << 1 forKey:(id)OF_STR("NS.rankTable")];
            break;
        case 0:
            [coder encodeBytes:_rankTable length:_M forKey:(id)OF_STR("NS.rankTable")];
            break;
        default:
            break;
    }
    [coder encodeBytes:_g length:((NSUInteger)_M >> 3) + 1 forKey:(id)OF_STR("NS.g")];
    [coder encodeObject:_subSharedKeySet forKey:(id)OF_STR("NS.subskset")];
    [coder encodeObject:[(Class)sharedKeySetArrayClass arrayWithObjects:_keys count:_numKey]
                 forKey:(id)OF_STR("NS.keys")];
    [coder encodeInteger:_algorithmType & 1 forKey:(id)OF_STR("NS.algorithmType")];
}

// Native +[NSSharedKeySet supportsSecureCoding], 0x1c02d982c. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSSharedKeySet initWithCoder:], 0x1c02d9834. Non-external. Decoding rebuilds the key set from its
// keys. Each failure makes its reason (the __CFExceptionProem of the receiver and selector, or the bare
// message for the key checks; kCFAllocatorSystemDefault; autoreleased through _CFAutoreleasePoolAddObject),
// releases the receiver, sends -failWithError: the ___archiveIsCorrupt error and answers nil: a coder that
// does not allow keyed coding ("%@: this object can only be decoded by a keyed coder"); NS.numKey of
// 0x7FFFFFFF or more (signed; "%@: extreme number of keys"); a zero low word of it answers the receiver as
// is. For an NSKeyedUnarchiver (objc_getClass) a placeholder [NSSharedKeySet new] (nil: "%@: unable to
// unarchive - memory failure") replaces the receiver in the unarchiver while NS.subskset is decoded
// (-decodeObjectOfClass: NSSharedKeySet), inside an @try whose @finally puts the receiver back
// (-replaceObject:withObject: of the placeholder) and releases the placeholder; a sub key set of another
// class ("%@: unexpected shared key set class: %@") or none with a coder -error ("%@: unable to unarchive -
// invalid class") fails, otherwise it is stored retained (unless a tagged pointer). Then NS.keys is decoded
// with the coder's -allowedClasses plus NSArray, NSString, NSNumber, NSDate and NSURL (a set made once,
// dispatch_once) less NSSharedKeySet; nil with a coder -error fails ("%@: unable to unarchive - invalid
// class"); an NSMutableArray ("unexpected mutable keys (%@)"), a non-array ("unexpected keys (%@)"), a count
// other than the low word of NS.numKey ("key count mismatch %lu vs %u") or an NSArray or NSDictionary key
// ("unexpected recursive keys (%@)", checked from the last) fails. The answer is +keySetWithKeys: of the keys
// plus the sub key set's -allKeys, retained; the receiver is released. Defect kept: on the failures inside
// the @try the receiver is released before the
// @finally passes it to -replaceObject:withObject: (whether the unarchiver still holds it is unread).
- (id)initWithCoder:(id)coder {
    OCFStringRef reason;
    if (![coder allowsKeyedCoding]) {
        reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("%@: this object can only be decoded by a keyed coder"),
            __CFExceptionProem(self, _cmd)
        );
        goto fail;
    }
    BOOL isUnarchiver = [coder isKindOfClass:objc_getClass(OF_NAME_STRING(ONSKeyedUnarchiver))];
    id unarchiver = isUnarchiver ? coder : nil;
    int64_t numKey = [coder decodeInt64ForKey:(id)OF_STR("NS.numKey")];
    if (numKey >= 0x7FFFFFFF) {
        reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("%@: extreme number of keys"),
            __CFExceptionProem(self, _cmd)
        );
        goto fail;
    }
    if ((uint32_t)numKey == 0) {
        return self;
    }
    id placeholder = nil;
    if (isUnarchiver) {
        placeholder = [ONSSharedKeySet new];
        if (placeholder == nil) {
            reason = OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault,
                NULL,
                OF_STR("%@: unable to unarchive - memory failure"),
                __CFExceptionProem(self, _cmd)
            );
            goto fail;
        }
        [coder replaceObject:self withObject:placeholder];
    }
    BOOL decoded;
    @try {
        id subKeySet = [coder decodeObjectOfClass:[ONSSharedKeySet class] forKey:(id)OF_STR("NS.subskset")];
        if (subKeySet != nil) {
            if ([subKeySet class] == [ONSSharedKeySet class]) {
                _subSharedKeySet = subKeySet;
                if ((intptr_t)subKeySet >= 0) {
                    [subKeySet retain];
                }
                decoded = YES;
            } else {
                OCFStringRef proem = __CFExceptionProem(self, _cmd);
                Class subKeySetClass = [subKeySet class];
                OCFStringRef failure = OCFStringCreateWithFormat(
                    kOCFAllocatorSystemDefault,
                    NULL,
                    OF_STR("%@: unexpected shared key set class: %@"),
                    proem,
                    subKeySetClass
                );
                id failureReason = _OCFAutoreleasePoolAddObject(NULL, failure);
                [self release];
                [coder failWithError:__archiveIsCorrupt(failureReason)];
                decoded = NO;
            }
        } else if ([coder error] != nil) {
            OCFStringRef failure = OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault,
                NULL,
                OF_STR("%@: unable to unarchive - invalid class"),
                __CFExceptionProem(self, _cmd)
            );
            id failureReason = _OCFAutoreleasePoolAddObject(NULL, failure);
            [self release];
            [coder failWithError:__archiveIsCorrupt(failureReason)];
            decoded = NO;
        } else {
            decoded = YES;
            _subSharedKeySet = nil;
        }
    } @finally {
        if (placeholder != nil) {
            [unarchiver replaceObject:placeholder withObject:self];
            [placeholder release];
        }
    }
    if (!decoded) {
        return nil;
    }
    Class arrayClass = objc_lookUpClass(OF_NAME_STRING(ONSArray));
    Class dictionaryClass = objc_lookUpClass(OF_NAME_STRING(ONSDictionary));
    // Native _initWithCoder:.onceToken (listed as .onceToken_0) and _initWithCoder:.oPlistClasses (listed as
    // .oPlistClasses_0), __DATA,__bss, non-external.
    static dispatch_once_t onceToken;
    static id oPlistClasses;
    dispatch_once(&onceToken, ^{
      // Native ___32-[NSSharedKeySet initWithCoder:]_block_invoke, 0x1c02d9ed0. Non-external. The captured
      // NSArray class, then NSString, NSNumber, NSDate and NSURL (objc_lookUpClass), as +[NSSet
      // setWithObjects:], retained after the store.
      oPlistClasses =
          [[(Class)sharedKeySetSetClass setWithObjects:arrayClass,
                                                       objc_lookUpClass(OF_NAME_STRING(ONSString)),
                                                       objc_lookUpClass(OF_NAME_STRING(ONSNumber)),
                                                       objc_lookUpClass(OF_NAME_STRING(ONSDate)),
                                                       objc_lookUpClass(OF_NAME_STRING(ONSURL)),
                                                       nil] retain];
    });
    id classes = [[[coder allowedClasses] setByAddingObjectsFromSet:oPlistClasses] mutableCopy];
    [classes removeObject:[ONSSharedKeySet class]];
    id keys = [coder decodeObjectOfClasses:classes forKey:(id)OF_STR("NS.keys")];
    [classes release];
    if (keys == nil && [coder error] != nil) {
        reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("%@: unable to unarchive - invalid class"),
            __CFExceptionProem(self, _cmd)
        );
        goto fail;
    }
    if ([keys isKindOfClass:[(Class)sharedKeySetMutableArrayClass class]]) {
        reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault, NULL, OF_STR("unexpected mutable keys (%@)"), [keys class]
        );
        goto fail;
    }
    if (!_ONSIsNSArray(keys)) {
        reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault, NULL, OF_STR("unexpected keys (%@)"), [keys class]
        );
        goto fail;
    }
    NSUInteger keyCount = [keys count];
    if (keyCount != (uint32_t)numKey) {
        reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("key count mismatch %lu vs %u"),
            [keys count],
            (uint32_t)numKey
        );
        goto fail;
    }
    for (NSUInteger index = keyCount; index != 0; index--) {
        id key = [keys objectAtIndexedSubscript:index - 1];
        if ([key isKindOfClass:arrayClass] || [key isKindOfClass:dictionaryClass]) {
            reason = OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault, NULL, OF_STR("unexpected recursive keys (%@)"), [key class]
            );
            goto fail;
        }
    }
    id allKeys = [(Class)sharedKeySetMutableArrayClass arrayWithArray:keys];
    [allKeys addObjectsFromArray:[_subSharedKeySet allKeys]];
    [self release];
    return [[ONSSharedKeySet keySetWithKeys:allKeys] retain];
fail:
    {
        id failureReason = _OCFAutoreleasePoolAddObject(NULL, reason);
        [self release];
        [coder failWithError:__archiveIsCorrupt(failureReason)];
        return nil;
    }
}

// Native ___archiveIsCorrupt (listed as ___archiveIsCorrupt_0), 0x1c02d9e38. Non-external. An autoreleased
// CFError of kCFErrorDomainCocoa, code 4864, whose user info is +[NSDictionary
// dictionaryWithObjects:forKeys:count:] of the reason under kCFErrorLocalizedDescriptionKey.
static id __archiveIsCorrupt(id reason) {
    OCFStringRef domain = kOCFErrorDomainCocoa;
    id key = (id)kOCFErrorLocalizedDescriptionKey;
    id userInfo = [(Class)sharedKeySetDictionaryClass dictionaryWithObjects:&reason forKeys:&key count:1];
    return [(id)OCFErrorCreate(NULL, domain, 0x1300, (OCFDictionaryRef)userInfo) autorelease];
}

// Native -[NSSharedKeySet count], 0x1c02d9f68. Non-external. The sum of _numKey over the receiver and its
// chain of sub key sets (0 for nil).
- (NSUInteger)count {
    NSUInteger count = 0;
    for (ONSSharedKeySet *keySet = self; keySet != nil; keySet = keySet->_subSharedKeySet) {
        count += keySet->_numKey;
    }
    return count;
}

// Native -[NSSharedKeySet isEmpty], 0x1c02d9f88. Non-external. Whether _numKey is 0 (the sub key sets are not
// read).
- (BOOL)isEmpty {
    return _numKey == 0;
}

// Native -[NSSharedKeySet maximumIndex], 0x1c02d9f98. Non-external. -count minus 1.
- (NSUInteger)maximumIndex {
    return [self count] - 1;
}

// Native -[NSSharedKeySet allKeys], 0x1c02d9fb0. Non-external. No keys answer +[NSArray array] (a tail call).
// Otherwise the keys of the receiver and its sub key sets (memmove, in chain order, a 32-bit running count)
// fill a temporary id buffer of -count entries (the new_id_array pattern); a heap buffer has its keys
// retained and is adopted by [[NSArray alloc] _initByAdoptingBuffer:count:size:], a stack buffer is copied by
// -initWithObjects:count:; the array is autoreleased.
- (id)allKeys {
    if (_numKey == 0) {
        return [(Class)sharedKeySetArrayClass array];
    }
    NSUInteger count = [self count];
    if ((count >> 60) != 0) {
        raiseTemporaryBufferOverflow(count);
    }
    NSUInteger allocationCount = count > 1 ? count : 1;
    id stackKeys[count < 0x101 ? allocationCount : 1];
    size_t storageSize = 0;
    id *heapKeys = NULL;
    id *buffer = stackKeys;
    if (count >= 0x101) {
        buffer = (id *)_OCFCreateArrayStorage(allocationCount, false, &storageSize);
        heapKeys = buffer;
    }
    uint32_t filled = 0;
    ONSSharedKeySet *keySet = self;
    do {
        memmove(buffer + filled, keySet->_keys, (NSUInteger)keySet->_numKey * sizeof(id));
        filled = keySet->_numKey + filled;
        keySet = keySet->_subSharedKeySet;
    } while (keySet != nil);
    id array;
    if (heapKeys != NULL) {
        for (uint32_t index = 0; index < filled; index++) {
            [heapKeys[index] retain];
        }
        array = [[(Class)sharedKeySetArrayClass alloc] _initByAdoptingBuffer:heapKeys
                                                                       count:filled
                                                                        size:filled];
    } else {
        array = [[(Class)sharedKeySetArrayClass alloc] initWithObjects:buffer count:filled];
    }
    return [array autorelease];
}

// Native -[NSSharedKeySet indexForKey:], 0x1c02da194. Non-external. NSNotFound when _numKey is 0; otherwise
// the key's -hash is looked up in the receiver and then each sub key set, skipping a set without keys:
// lookup3's final mix over the set's seeds (_algorithmType 1 also mixes the hash's high word, 0 does not, any
// other value is BRK #1) gives the vertices 2 (c % factor) and 2 (b % factor) + 1; either at or above M skips
// the set; the vertex chosen by the packed bits (bit h1 & 6 of byte h1 / 8 xor bit h2 & 7 of byte h2 / 8) at
// or above M, an unknown _select, a rank at or above _numKey or a nil key there skips it. A key that is the
// same object or is -isEqual: (sent to the argument) answers its rank plus the keys of the sets before (a
// 32-bit running count); otherwise NSNotFound.
- (NSUInteger)indexForKey:(id)key {
    if (_numKey == 0) {
        return 0x7FFFFFFFFFFFFFFF;
    }
    uint64_t hash = [key hash];
    uint32_t offset = 0;
    ONSSharedKeySet *keySet = self;
    do {
        uint32_t keyCount = keySet->_numKey;
        if (keyCount != 0) {
            uint32_t base;
            uint32_t b;
            uint32_t c;
            uint32_t modulus = keySet->_factor;
            if (keySet->_algorithmType == 1) {
                base = keySet->_seeds[0] + 0xDEADBEF7;
                c = base + keySet->_seeds[1];
                b = base + (uint32_t)(hash >> 32);
            } else if (keySet->_algorithmType == 0) {
                base = keySet->_seeds[0] + 0xDEADBEF3;
                c = base + keySet->_seeds[1];
                b = base;
            } else {
                __builtin_trap();
            }
            finalMix(base + (uint32_t)hash, b, c, &b, &c);
            uint32_t bModulo = b % modulus;
            uint32_t vertexPair[2];
            vertexPair[0] = (c % modulus) << 1;
            vertexPair[1] = 1 | (bModulo << 1);
            uint32_t vertices = keySet->_M;
            if (vertexPair[0] < vertices && vertexPair[1] < vertices) {
                uint8_t *bits = keySet->_g;
                uint32_t first = bits[vertexPair[0] >> 3] >> (vertexPair[0] & 6);
                uint32_t second = bits[(bModulo << 1) >> 3] >> (vertexPair[1] & 7);
                uint32_t vertex = vertexPair[(second ^ first) & 1];
                if (vertex < vertices) {
                    NSUInteger rank;
                    BOOL known = YES;
                    switch (keySet->_select) {
                        case 2:
                            rank = ((uint32_t *)keySet->_rankTable)[vertex];
                            break;
                        case 1:
                            rank = ((uint16_t *)keySet->_rankTable)[vertex];
                            break;
                        case 0:
                            rank = ((uint8_t *)keySet->_rankTable)[vertex];
                            break;
                        default:
                            rank = 0;
                            known = NO;
                            break;
                    }
                    if (known && rank < keyCount) {
                        id candidate = keySet->_keys[rank];
                        if (candidate != nil) {
                            if (key == candidate || [key isEqual:candidate]) {
                                return rank + offset;
                            }
                            keyCount = keySet->_numKey;
                        }
                    }
                }
            }
        }
        offset = keyCount + offset;
        keySet = keySet->_subSharedKeySet;
    } while (keySet != nil);
    return 0x7FFFFFFFFFFFFFFF;
}

// Native -[NSSharedKeySet keyAtIndex:], 0x1c02da3b4. Non-external. NSNotFound, an empty receiver (_numKey is
// read first) or a nil receiver answer nil; otherwise the index walks the chain of key sets, each consuming
// its _numKey keys, and nil past the last.
- (id)keyAtIndex:(NSUInteger)index {
    if (index == 0x7FFFFFFFFFFFFFFF || _numKey == 0 || self == nil) {
        return nil;
    }
    ONSSharedKeySet *keySet = self;
    do {
        NSUInteger keyCount = keySet->_numKey;
        if (index < keyCount) {
            return keySet->_keys[index];
        }
        index -= keyCount;
        keySet = keySet->_subSharedKeySet;
    } while (keySet != nil);
    return nil;
}

// Native -[NSSharedKeySet countByEnumeratingWithState:objects:count:], 0x1c02da3fc. Non-external.
// state->state is the next index; at -count nothing is stored and 0 answered. Otherwise up to the length the
// keys
// (-keyAtIndex:) are written to the buffer, then the state, the items pointer (the buffer) and the mutations
// pointer (the constant const_mu, 1) are stored; the answer is the number written.
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length {
    // Native countByEnumeratingWithState:objects:count:.const_mu (listed as .const_mu_8), __TEXT,__const.
    static const unsigned long const_mu = 1;
    DictionaryEnumerationState *enumerationState = state;
    NSUInteger count = [self count];
    NSUInteger index = enumerationState->state;
    if (index == count) {
        return 0;
    }
    NSUInteger written = 0;
    if (length != 0 && index < count) {
        do {
            buffer[written] = [self keyAtIndex:index];
            index += 1;
            written += 1;
        } while (written < length && index < count);
    }
    enumerationState->state = index;
    enumerationState->itemsPtr = (void **)buffer;
    enumerationState->mutationsPtr = (unsigned long *)&const_mu;
    return written;
}

// Native -[NSSharedKeySet hash], 0x1c02da4a8. Non-external. -count (a tail call).
- (NSUInteger)hash {
    return [self count];
}

// Native -[NSSharedKeySet isEqual:], 0x1c02da4ac. Non-external. The receiver itself answers YES; nil, an
// object not of the receiver's class, a different -count or -keySetCount, or -allKeys arrays of different
// counts answer NO. Every key of each side must have an index in the other (-indexForKey:, fast enumeration)
// and each side must enumerate -count keys; the -maximumIndex values must match, and for every index up to it
// the argument's key at that index must be -isEqual: to the receiver's key at the receiver's index of it. The
// answer is then whether both or neither -isEmpty (the two results xor-ed, inverted).
- (BOOL)isEqual:(id)other {
    if (self == other) {
        return YES;
    }
    if (other == nil || ![other isKindOfClass:[self class]]) {
        return NO;
    }
    NSUInteger count = [self count];
    if (count != [other count]) {
        return NO;
    }
    if ([self keySetCount] != [other keySetCount]) {
        return NO;
    }
    id selfKeys = [self allKeys];
    id otherKeys = [other allKeys];
    if ([selfKeys count] != [otherKeys count]) {
        return NO;
    }
    NSUInteger enumerated = 0;
    for (id key in selfKeys) {
        enumerated += 1;
        if ([other indexForKey:key] == 0x7FFFFFFFFFFFFFFF) {
            return NO;
        }
    }
    if (enumerated != count) {
        return NO;
    }
    enumerated = 0;
    for (id key in otherKeys) {
        enumerated += 1;
        if ([self indexForKey:key] == 0x7FFFFFFFFFFFFFFF) {
            return NO;
        }
    }
    if (enumerated != count) {
        return NO;
    }
    NSUInteger maximumIndex = [self maximumIndex];
    if (maximumIndex != [other maximumIndex]) {
        return NO;
    }
    NSUInteger index = 0;
    do {
        id key = [other keyAtIndex:index];
        if (![key isEqual:[self keyAtIndex:[self indexForKey:key]]]) {
            return NO;
        }
        index += 1;
    } while (index <= maximumIndex);
    BOOL selfEmpty = [self isEmpty];
    BOOL otherEmpty = [other isEmpty];
    return (BOOL)((selfEmpty ^ otherEmpty) ^ 1);
}

// Native -[NSSharedKeySet dealloc], 0x1c02da78c. Non-external. Frees the rank table, the packed bits and the
// seeds (when set), releases the keys from the last (unless nil or tagged pointers) and frees them, releases
// the sub key set (unless nil or a tagged pointer), then super.
- (void)dealloc {
    if (_rankTable != NULL) {
        free(_rankTable);
    }
    if (_g != NULL) {
        free(_g);
    }
    if (_seeds != NULL) {
        free(_seeds);
    }
    if (_keys != NULL) {
        if (_numKey != 0) {
            for (NSUInteger index = _numKey; index != 0; index--) {
                id key = _keys[index - 1];
                if ((intptr_t)key >= 1) {
                    [key release];
                }
            }
        }
        free(_keys);
    }
    if ((intptr_t)_subSharedKeySet >= 1) {
        [_subSharedKeySet release];
    }
    [super dealloc];
}

// Native -[NSSharedKeySet keySetCount], 0x1c02da86c. Non-external. The number of key sets in the chain (0 for
// nil).
- (NSUInteger)keySetCount {
    NSUInteger count = 0;
    for (ONSSharedKeySet *keySet = self; keySet != nil; keySet = keySet->_subSharedKeySet) {
        count += 1;
    }
    return count;
}

// Native -[NSSharedKeySet debugDescription], 0x1c02da888. Non-external. A +[NSMutableString string]
// (objc_lookUpClass) listing the class and address, numKey, M, factor, each key's class, address and hash,
// the seeds, the rank table with its select width name and entries, the algorithm type, g, the packed bits as
// groups of eight binary digits (bit 7 first, the masks 0x80-0x01 of 0x1c046dde0, each group an +[NSString
// stringWithCharacters:length:] of 8, M / 8 + 1 groups) and the sub key set.
- (id)debugDescription {
    id description = [(Class)objc_lookUpClass(OF_NAME_STRING(ONSMutableString)) string];
    [description appendFormat:(id)OF_STR("<%@: %p>\n"), [self class], self];
    [description appendFormat:(id)OF_STR("  numKey = %u\n"), _numKey];
    [description appendFormat:(id)OF_STR("  M = %u (should always be even)\n"), _M];
    [description appendFormat:(id)OF_STR("  factor = %u (should always be prime)\n"), _factor];
    [description appendFormat:(id)OF_STR("  keys = [\n")];
    for (NSUInteger index = 0; index < _numKey; index++) {
        id key = _keys[index];
        Class keyClass = [key class];
        NSUInteger keyHash = [key hash];
        [description appendFormat:(id)OF_STR("    <%@: %p>, 0x%lx\n"), keyClass, key, keyHash];
    }
    [description appendFormat:(id)OF_STR("  ]\n")];
    [description appendFormat:(id)OF_STR("  seeds[0] = 0x%xu\n"), _seeds[0]];
    [description appendFormat:(id)OF_STR("  seeds[1] = 0x%xu\n"), _seeds[1]];
    uint8_t width = _select;
    OCFStringRef widthName = width == 0   ? OF_STR("unsigned char")
                             : width == 1 ? OF_STR("unsigned short")
                             : width == 2 ? OF_STR("unsigned int")
                                          : OF_STR("unknown");
    [description
        appendFormat:(id)OF_STR("  ranktable = %p (select = %u (%@)) [\n"), _rankTable, width, widthName];
    switch (_select) {
        case 0:
            for (NSUInteger index = 0; index < _M; index++) {
                [description appendFormat:(id)OF_STR("    %d - %u\n"), index, ((uint8_t *)_rankTable)[index]];
            }
            break;
        case 1:
            for (NSUInteger index = 0; index < _M; index++) {
                [description
                    appendFormat:(id)OF_STR("    %d - %u\n"), index, ((uint16_t *)_rankTable)[index]];
            }
            break;
        case 2:
            for (NSUInteger index = 0; index < _M; index++) {
                [description
                    appendFormat:(id)OF_STR("    %d - %u\n"), index, ((uint32_t *)_rankTable)[index]];
            }
            break;
        default:
            break;
    }
    [description appendFormat:(id)OF_STR("  ]\n")];
    [description appendFormat:(id)OF_STR("  algorithmType: %@\n"),
                              _algorithmType == 0 ? OF_STR("clamp_hash_to_32") : OF_STR("use_full_hash_64")];
    [description appendFormat:(id)OF_STR("  g = %p\n"), _g];
    [description appendFormat:(id)OF_STR("    packed bits (binary):\n      ")];
    // Native xmmword_1C046DDE0 and xmmword_1C046DDF0 (__TEXT,__const): the bit masks of the eight digits.
    static const uint32_t masks[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
    NSUInteger byteIndex = 0;
    do {
        uint8_t byte = _g[byteIndex];
        UniChar digits[8];
        for (unsigned digit = 0; digit < 8; digit++) {
            digits[digit] = (byte & masks[digit]) == 0 ? '0' : '1';
        }
        [description
            appendFormat:(id)OF_STR("%@ "),
                         [(Class)objc_lookUpClass(OF_NAME_STRING(ONSString)) stringWithCharacters:digits
                                                                                           length:8]];
    } while (byteIndex++ < (NSUInteger)(_M >> 3));
    [description appendFormat:(id)OF_STR("\n")];
    if (_subSharedKeySet != nil) {
        Class mutableStringClass = objc_lookUpClass(OF_NAME_STRING(ONSMutableString));
        [description appendFormat:(id)OF_STR("  subSharedKeySet = %@\n"),
                                  [mutableStringClass stringWithFormat:(id)OF_STR("<%@: %p>"),
                                                                       [_subSharedKeySet class],
                                                                       _subSharedKeySet]];
    } else {
        [description appendFormat:(id)OF_STR("  subSharedKeySet = nil\n")];
    }
    return description;
}

// Native -[NSSharedKeySet g] (0x1c02daca0), -setG: (0x1c02daca8), -select (0x1c02dacb0), -setSelect:
// (0x1c02dacb8), -rankTable (0x1c02dacc0), -setRankTable: (0x1c02dacc8), -M (0x1c02dacd0), -setM:
// (0x1c02dacd8), -factor (0x1c02dace0), -setFactor: (0x1c02dace8), -numKey (0x1c02dacf0), -setNumKey:
// (0x1c02dacf8), -seeds (0x1c02dad00), -setSeeds: (0x1c02dad08), -keys (0x1c02dad10), -setKeys:
// (0x1c02dad18), -subSharedKeySet (0x1c02dad20: objc_getProperty at offset 0x40, atomic) and
// -setSubSharedKeySet: (0x1c02dad2c: objc_setProperty_atomic at offset 0x40). Non-external. The synthesized
// accessors of the properties.
@synthesize g = _g;
@synthesize select = _select;
@synthesize rankTable = _rankTable;
@synthesize M = _M;
@synthesize factor = _factor;
@synthesize numKey = _numKey;
@synthesize seeds = _seeds;
@synthesize keys = _keys;
@synthesize subSharedKeySet = _subSharedKeySet;

@end

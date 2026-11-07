//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Notification/OCFXNotificationRegistrar.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Notification/OCFNotificationCenter.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <Block.h>
#include <assert.h>
#include <malloc/malloc.h>
#include <notify.h>
#include <objc/objc.h>
#include <os/lock.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <xpc/xpc.h>

// Inferred organization: the CFXNotification registrar functions (CFXNotificationHandlerCopy, 0x1c02bb134,
// through ____CFXNotificationRegistrarRecycleObserverRegistration_block_invoke, 0x1c02be5e8) are one
// contiguous cluster of the iOS Simulator __text section and are kept in one translation unit; the native
// source path is not in the evidence. Functions of the cluster outside the misc-services task list (the add,
// remove, remote-token and distributed-connection entries, CFXNotificationRegistrarGetTypeID, GetOptions,
// IsEmpty, UpdateDistributedConnections and __CFXNotificationRegistrarAddCachedName) are not restored here.
// Unless a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified (assembly read in full; the pseudocode was a guide only). iOS Device 24A446: unread.
// Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json.

// libobjc entry points the native code calls; supplied, never reimplemented. The SDK installs no declarations
// of the weak-reference functions.
extern id objc_loadWeakRetained(id *location);
extern void objc_destroyWeak(id *location);
extern void objc_release(id value);
extern id objc_storeWeakOrNil(id *location, id object);
extern void objc_moveWeak(id *to, id *from);

// Observer flags (the observer record's flags word): 0x400 a weak observer, 0x800 a name registration for any
// object, 0x2000 a distributed connection in the context word, 0x4000 a notify token in the context word,
// 0x8000 an invalidated record, 0x20000 a block handler, 0x40000 a selector handler, 0x80000 a C callback.
enum {
    kObserverWeak = 0x400,
    kObserverNameOnly = 0x800,
    kObserverDistributedConnection = 0x2000,
    kObserverNotifyToken = 0x4000,
    kObserverInvalid = 0x8000,
    kHandlerBlock = 0x20000,
    kHandlerSelector = 0x40000,
    kHandlerCallback = 0x80000,
};

// A hash table of record indices: bucketCount buckets, each the head index of a chain (-1 when empty), and
// the number of records it holds.
typedef struct {
    int32_t *buckets;
    uint32_t bucketCount;
    uint32_t count;
} RegistrarTable;

// A doubly linked list of observer record indices.
typedef struct {
    int32_t head;
    uint32_t count;
} ObserverList;

// A name record (40 bytes): chain links, the name (a reference counted in the name pool), its CFHash and its
// children: an object table, or for the any-object names an observer list.
typedef struct {
    int32_t previous;
    int32_t next;
    OCFStringRef name;
    uint64_t hash;

    union {
        RegistrarTable objects;
        ObserverList observers;
    } children;
} NameRecord;

_Static_assert(sizeof(NameRecord) == 40, "The native name record is 40 bytes.");

// An object record (32 bytes): chain links, the object (a pooled string when the registrar compares objects
// with CFEqual), its observers and the index of its name record (-1 for the any-name object table).
typedef struct {
    int32_t previous;
    int32_t next;
    OCFTypeRef object;
    ObserverList observers;
    int32_t name;
    uint32_t reserved;
} ObjectRecord;

OF_ABI_ASSERT(sizeof(ObjectRecord) == 32, "The native object record is 32 bytes.");

// An observer record (56 bytes): list links, the observer (weak storage under kObserverWeak), the handler,
// the context word, the generation of the token, the flags and the parent index (an object record, a name
// record under kObserverNameOnly, or -1 for the any-name any-object list).
typedef struct {
    int32_t previous;
    int32_t next;
    void *observer;
    NotificationHandler handler;
    uint64_t context;
    uint32_t generation;
    uint32_t flags;
    int32_t parent;
    uint32_t reserved;
} ObserverRecord;

OF_ABI_ASSERT(sizeof(ObserverRecord) == 56, "The native observer record is 56 bytes.");

// A record pool: the records, the number in use, the number allocated and the head of the free list.
typedef struct {
    NameRecord *records;
    uint32_t used;
    uint32_t capacity;
    int32_t freeHead;
    uint32_t reserved;
} NamePool;

typedef struct {
    ObjectRecord *records;
    uint32_t used;
    uint32_t capacity;
    int32_t freeHead;
    uint32_t reserved;
} ObjectPool;

typedef struct {
    ObserverRecord *records;
    uint32_t used;
    uint32_t capacity;
    int32_t freeHead;
    uint32_t reserved;
} ObserverPool;

// The registrar instance (176 bytes). The CF object at 0x58 is released by the deallocator; the functions
// that store it are outside this restoration (unread).
struct __OCFXNotificationRegistrar {
    OCFRuntimeBase base;
    os_unfair_lock lock;
    uint32_t reserved;
    uint64_t options;
    RegistrarTable names;
    RegistrarTable objects;
    RegistrarTable anyObjectNames;
    ObserverList anyObservers;
    OCFTypeRef retained;
    NamePool namePool;
    ObjectPool objectPool;
    ObserverPool observerPool;
    uint32_t nextGeneration;
    uint32_t reserved2;
};

OF_ABI_ASSERT(sizeof(struct __OCFXNotificationRegistrar) == 0xb0, "The native registrar is 176 bytes.");
OF_ABI_ASSERT(
    __builtin_offsetof(struct __OCFXNotificationRegistrar, namePool) == 0x60, "The name pool is at 0x60."
);
OF_ABI_ASSERT(
    __builtin_offsetof(struct __OCFXNotificationRegistrar, observerPool) == 0x90,
    "The observer pool is at 0x90."
);

// The disposal list (48 bytes) collects CF objects and blocks to release after the registrar lock is dropped:
// four inline items, then an overflow array.
typedef struct {
    uint32_t count;
    uint32_t capacity;
    OCFTypeRef items[4];
    OCFTypeRef *overflow;
} DisposalList;

OF_ABI_ASSERT(sizeof(DisposalList) == 48, "The native disposal list is 48 bytes.");

// Options: bit 0 sorts the found registrations by token, bit 1 compares names and objects with CFEqual and
// pools object strings (the distributed and Darwin centers).
enum {
    kRegistrarSorts = 1,
    kRegistrarComparesObjects = 2,
};

static void _CFXNotificationHandlerRelease(NotificationHandler *handler, uint32_t flags, DisposalList *list);
static void _CFXNotificationDisposalListRelease(DisposalList *list);
static NameRecord *_CFXNotificationRegistrarFindName(
    OCFXNotificationRegistrarRef registrar,
    RegistrarTable *table,
    OCFStringRef name,
    uint64_t hash,
    int32_t *index
);
static ObjectRecord *_CFXNotificationRegistrarFindObject(
    OCFXNotificationRegistrarRef registrar,
    RegistrarTable *table,
    OCFTypeRef object,
    uint64_t hash,
    Boolean compares,
    int32_t *index
);
static OCFStringRef CFCachedStringCopyString(OCFStringRef *cached);
static void _CFXNotificationRegistrarFindObservers(
    OCFXNotificationRegistrarRef registrar,
    ObserverList *list,
    OCFStringRef name,
    OCFTypeRef object,
    NotificationRegistrationBuffer *buffer,
    int32_t **invalid,
    uint32_t *invalidCount,
    uint64_t *invalidCapacity,
    bool *invalidOnHeap
);
static void _CFXNotificationRegistrarInvalidateObserver(
    OCFXNotificationRegistrarRef registrar,
    uint32_t index,
    int32_t keepName,
    int32_t keepObject,
    DisposalList *list
);
static int _registrar_compare(const void *first, const void *second);
#ifdef __BLOCKS__
static void _CFXNotificationRegistrarEnumerateNameTable(
    OCFXNotificationRegistrarRef registrar,
    RegistrarTable *table,
    Boolean observerTables,
    void (^block)(const NotificationRegistration *registration)
);
static void _CFXNotificationRegistrarEnumerateObjectTable(
    OCFXNotificationRegistrarRef registrar,
    RegistrarTable *table,
    OCFStringRef name,
    void (^block)(const NotificationRegistration *registration)
);
static void _CFXNotificationRegistrarEnumerateObserverTable(
    OCFXNotificationRegistrarRef registrar,
    ObserverList *list,
    OCFStringRef name,
    OCFTypeRef object,
    void (^block)(const NotificationRegistration *registration)
);
#endif
static void __CFXNotificationDisposalListAddItem(DisposalList *list, OCFTypeRef item);
static void _CFXNotificationRegistrarRemoveCachedName(OCFStringRef *cached);
static OCFMutableDictionaryRef _CFXNotificationRegistrarGetNamePool(void);
static Boolean CFCachedStringEqual(OCFStringRef *cached, OCFStringRef string);
static Boolean _CFXNotificationRegistrarRecycleObserverRegistration(
    OCFXNotificationRegistrarRef registrar, ObserverList *list, uint32_t index, DisposalList *disposal
);
static void _CFXNotificationRegistrarRecycleNameRegistration(
    OCFXNotificationRegistrarRef registrar, RegistrarTable *table, int32_t index, Boolean keepChildren
);
static Boolean _CFXNotificationRegistrarRecycleObjectRegistration(
    OCFXNotificationRegistrarRef registrar, RegistrarTable *table, int32_t index, Boolean compares
);
static void _CFXNotificationRegistrarRemoveNames(
    OCFXNotificationRegistrarRef registrar,
    RegistrarTable *table,
    Boolean observerTables,
    OCFStringRef name,
    uint64_t nameHash,
    OCFTypeRef object,
    uint64_t objectHash,
    Boolean compares,
    void *observer
);
static Boolean _CFXNotificationRegistrarRemoveObjects(
    OCFXNotificationRegistrarRef registrar,
    RegistrarTable *table,
    OCFTypeRef object,
    uint64_t hash,
    Boolean compares,
    void *observer
);
static Boolean _CFXNotificationRegistrarRemoveObservers(
    OCFXNotificationRegistrarRef registrar, ObserverList *list, void *observer
);
static ObserverRecord *_CFXNotificationRegistrarAddObserver(
    OCFXNotificationRegistrarRef registrar,
    ObserverList *list,
    void *callout,
    OCFTypeRef context,
    int32_t parent,
    void *observer,
    uint32_t *index,
    OCFOptionFlags flags,
    uint64_t token,
    int32_t nameIndex,
    int32_t objectIndex,
    DisposalList *disposal
);
static ObjectRecord *_CFXNotificationRegistrarAddObject(
    OCFXNotificationRegistrarRef registrar,
    RegistrarTable *table,
    OCFTypeRef object,
    uint64_t hash,
    int32_t *index,
    int32_t nameIndex,
    Boolean compares
);
static NameRecord *_CFXNotificationRegistrarAddName(
    OCFXNotificationRegistrarRef registrar,
    RegistrarTable *table,
    OCFStringRef name,
    uint64_t hash,
    int32_t *index,
    Boolean observerTables
);
static void _CFXNotificationRegisteredObserverSetDistributedConnection(
    OCFXNotificationRegistrarRef registrar, uint32_t generation, uint32_t index, xpc_object_t connection
);
static void _CFXNotificationRegistrarAddCachedName(OCFStringRef name, OCFStringRef *cached);

// Native __CFXNotificationRegistrarNamePoolLock, 0x22e6d3b08, and __CFXNotificationRegistrarNamePool,
// 0x22e6d3b10 (__DATA,__bss). Non-external. The pool maps each registered name to its use count.
static os_unfair_lock __CFXNotificationRegistrarNamePoolLock = OS_UNFAIR_LOCK_INIT;
static OCFMutableDictionaryRef __CFXNotificationRegistrarNamePool;

// The object hash of a registrar that does not compare objects: bits 5-11 of the pointer (UBFX #5, #7).
static inline uint64_t pointerHash(OCFTypeRef object) {
    return ((uintptr_t)object >> 5) & 0x7f;
}

// Native CFXNotificationHandlerCopy, 0x1c02bb134. External. The handler is zeroed; a block handler copies the
// block (_Block_copy) and retains the context when each is not NULL; a selector or C callback handler stores
// the callout only.
void OCFXNotificationHandlerCopy(
    NotificationHandler *handler, void *callout, OCFTypeRef context, uint32_t flags
) {
    handler->callout = NULL;
    handler->context = NULL;
    if ((flags & kHandlerBlock) != 0) {
        if (callout != NULL) {
            handler->callout = _Block_copy(callout);
        }
        if (context != NULL) {
            handler->context = OCFRetain(context);
        }
    } else if ((flags & kHandlerSelector) != 0 || (flags & kHandlerCallback) != 0) {
        handler->callout = callout;
    }
}

// Native CFXNotificationHandlerRelease, 0x1c02bb18c. External. Through a zeroed stack disposal list.
void OCFXNotificationHandlerRelease(NotificationHandler *handler, uint32_t flags) {
    DisposalList list = {0};
    _CFXNotificationHandlerRelease(handler, flags, &list);
    _CFXNotificationDisposalListRelease(&list);
}

// Native __CFXNotificationHandlerRelease, 0x1c02bb1ec. Non-external. A block handler queues its block and its
// context (each when not NULL) on the disposal list; the handler is zeroed in every case.
static void _CFXNotificationHandlerRelease(NotificationHandler *handler, uint32_t flags, DisposalList *list) {
    if ((flags & kHandlerBlock) != 0) {
        if (handler->callout != NULL) {
            __CFXNotificationDisposalListAddItem(list, handler->callout);
        }
        if (handler->context != NULL) {
            __CFXNotificationDisposalListAddItem(list, handler->context);
        }
    }
    handler->callout = NULL;
    handler->context = NULL;
}

// Native __CFXNotificationDisposalListRelease, 0x1c02bb234. Non-external. CFRelease on the inline items (up
// to four), then on the overflow items, re-reading the count after each release.
// CoreFoundation defect (kept): the overflow array is never freed here or elsewhere in the cluster, so a list
// that grew past four items leaks it; confidence: verified from the assembly of every function that touches
// the list.
static void _CFXNotificationDisposalListRelease(DisposalList *list) {
    if (list->count == 0) {
        return;
    }
    uint64_t position = 1;
    uint32_t count;
    uint64_t limit;
    do {
        OCFRelease(list->items[position - 1]);
        count = list->count;
        limit = count < 4 ? count : 4;
    } while (position++ < limit);
    if (count >= 5) {
        uint64_t index = 0;
        do {
            OCFRelease(list->overflow[index]);
            index++;
        } while (index + 4 < list->count);
    }
}

// Native __CFXNotificationRegistrarDeallocate, 0x1c02bb2b4. Non-external; the class's finalize callback.
// Frees the object table of the first name record of each name bucket, removes every pooled name (and, for a
// registrar that pools objects, every pooled object string), invalidates the handlers of the live observers
// (destroying weak observers first) and frees the pools and bucket arrays, then releases the CF object at
// 0x58 (a tail call).
// CoreFoundation defect (kept): only the head record of each name bucket has its object table freed; the
// object tables of the other names in a chain leak; confidence: verified (the loop reads the bucket head and
// never follows the chain).
static void __CFXNotificationRegistrarDeallocate(OCFTypeRef cf) {
    OCFXNotificationRegistrarRef registrar = (OCFXNotificationRegistrarRef)cf;
    uint64_t count = registrar->names.bucketCount;
    if ((uint32_t)count != 0) {
        for (uint64_t bucket = 0; bucket < count; bucket++) {
            int32_t index = registrar->names.buckets[bucket];
            if (index != -1) {
                void *objects = registrar->namePool.records[index].children.objects.buckets;
                if (objects != NULL) {
                    free(objects);
                    count = registrar->names.bucketCount;
                }
            }
        }
    }
    if (registrar->namePool.records != NULL) {
        uint64_t capacity = registrar->namePool.capacity;
        if ((uint32_t)capacity != 0) {
            for (uint64_t index = 0; index < capacity; index++) {
                NameRecord *record = &registrar->namePool.records[index];
                if (record->name != NULL) {
                    _CFXNotificationRegistrarRemoveCachedName(&record->name);
                    capacity = registrar->namePool.capacity;
                }
            }
        }
        free(registrar->namePool.records);
    }
    if (registrar->objectPool.records != NULL) {
        if ((registrar->options & kRegistrarComparesObjects) != 0) {
            uint64_t capacity = registrar->objectPool.capacity;
            if ((uint32_t)capacity != 0) {
                for (uint64_t index = 0; index < capacity; index++) {
                    ObjectRecord *record = &registrar->objectPool.records[index];
                    if (record->object != NULL) {
                        _CFXNotificationRegistrarRemoveCachedName((OCFStringRef *)&record->object);
                        capacity = registrar->objectPool.capacity;
                    }
                }
            }
        }
        free(registrar->objectPool.records);
    }
    uint64_t capacity = registrar->observerPool.capacity;
    if ((uint32_t)capacity != 0) {
        for (uint64_t index = 0; index < capacity; index++) {
            ObserverRecord *record = &registrar->observerPool.records[index];
            uint32_t flags = record->flags;
            if ((flags & kObserverInvalid) == 0) {
                if ((flags & kObserverWeak) != 0) {
                    objc_destroyWeak((id *)&record->observer);
                    flags = record->flags;
                }
                DisposalList list = {0};
                _CFXNotificationHandlerRelease(&record->handler, flags, &list);
                _CFXNotificationDisposalListRelease(&list);
                capacity = registrar->observerPool.capacity;
            }
        }
    }
    free(registrar->observerPool.records);
    if (registrar->names.buckets != NULL) {
        free(registrar->names.buckets);
    }
    if (registrar->objects.buckets != NULL) {
        free(registrar->objects.buckets);
    }
    if (registrar->anyObjectNames.buckets != NULL) {
        free(registrar->anyObjectNames.buckets);
    }
    if (registrar->retained != NULL) {
        OCFRelease(registrar->retained);
    }
}

// Native __CFXNotificationRegistrarClass, 0x2227a9d38 (__DATA_CONST,__const). Hidden (was a private
// external). Version 1, name "CFXNotificationRegistrar", the finalize callback only.
const OCFRuntimeClass __CFXNotificationRegistrarClass = {
    1,
    "CFXNotificationRegistrar",
    NULL,
    NULL,
    __CFXNotificationRegistrarDeallocate,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    0,
};

// Native CFXNotificationRegistrarCreate, 0x1c02bb4c4. External. A type-70 instance with 0xa0 extra bytes;
// stores the options, zeroes the lock, sets the three free-list heads and the any-observer list head to -1
// and stores 1 in the generation counter with release semantics (STLR).
OCFXNotificationRegistrarRef OCFXNotificationRegistrarCreate(OCFAllocatorRef allocator, uint64_t options) {
    OCFXNotificationRegistrarRef registrar =
        (OCFXNotificationRegistrarRef)_OCFRuntimeCreateInstance(allocator, 70, 0xa0, NULL);
    if (registrar != NULL) {
        registrar->options = options;
        registrar->lock = (os_unfair_lock)OS_UNFAIR_LOCK_INIT;
        registrar->namePool.freeHead = -1;
        registrar->objectPool.freeHead = -1;
        registrar->observerPool.freeHead = -1;
        registrar->anyObservers.head = -1;
        __atomic_store_n(&registrar->nextGeneration, 1, __ATOMIC_RELEASE);
    }
    return registrar;
}

// Native CFXNotificationRegistrarGetOptions, 0x1c02bb51c. External. Reading:
// Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3; iOS Device divergence: unread).
uint64_t OCFXNotificationRegistrarGetOptions(OCFXNotificationRegistrarRef registrar) {
    return registrar->options;
}

// Native _CFXNotificationRegistrationDispose, 0x1c02bb524. External. Releases the handler (with the record's
// flags), the object when releaseObject is set and it is not NULL, the retained weak observer (flag 0x400,
// objc_release) and the name when not NULL (a tail call).
void _OCFXNotificationRegistrationDispose(NotificationRegistration *registration, Boolean releaseObject) {
    DisposalList list = {0};
    _CFXNotificationHandlerRelease(&registration->handler, (uint32_t)registration->flags, &list);
    _CFXNotificationDisposalListRelease(&list);
    if (releaseObject && registration->object != NULL) {
        OCFRelease(registration->object);
    }
    if ((registration->flags & kObserverWeak) != 0) {
        objc_release((id)registration->observer);
    }
    if (registration->name != NULL) {
        OCFRelease(registration->name);
    }
}

// Native _CFXNotificationRegistrationBufferDestroy, 0x1c02bb5ec. External. Disposes every record (releasing
// objects when the buffer copies them) and frees the records unless they are the inline storage that follows
// the header.
void _OCFXNotificationRegistrationBufferDestroy(NotificationRegistrationBuffer *buffer) {
    if (buffer->count != 0) {
        uint64_t index = 0;
        do {
            _OCFXNotificationRegistrationDispose(&buffer->records[index], buffer->copiesObjects);
            index++;
        } while (index < buffer->count);
    }
    if (buffer->records != (NotificationRegistration *)(buffer + 1)) {
        free(buffer->records);
    }
}

// Native CFXNotificationRegistrarFind, 0x1c02bb664. External.
// The buffer's copiesObjects is the options' bit 1. A name equal to kCFNotificationAnyName (or NULL) becomes
// that constant; with bit 1 an object equal to kCFNotificationAnyObject (or NULL) becomes that constant. The
// name hash is CFHash; the object hash is CFHash with bit 1, else bits 5-11 of the pointer (the pseudocode
// drops this; the assembly settles it). Under the lock, observers are collected from: the name's object
// entry, the any-name object entry, the name's any-object entry and the any-name any-object list, in that
// order. Weak observers that are gone are collected in a 32-entry stack array (grown on the heap) and
// invalidated before the unlock; the disposal list is released after it. With option bit 0 the records are
// sorted by token (qsort, 64-byte elements).
void OCFXNotificationRegistrarFind(
    OCFXNotificationRegistrarRef registrar,
    OCFStringRef name,
    OCFTypeRef object,
    NotificationRegistrationBuffer *buffer
) {
    int32_t invalidStorage[32];
    int32_t *invalid = invalidStorage;
    uint32_t invalidCount = 0;
    uint64_t invalidCapacity = 32;
    bool invalidOnHeap = false;
    Boolean compares = (registrar->options >> 1) & 1;
    buffer->copiesObjects = compares;
    OCFStringRef anyName = kOCFNotificationAnyName;
    OCFStringRef searchName = anyName;
    if (name != NULL) {
        if (name == anyName) {
            searchName = name;
        } else {
            searchName = OCFEqual(name, anyName) ? anyName : name;
        }
    }
    uint64_t nameHash;
    uint64_t objectHash;
    if (object == NULL) {
        object = kOCFNotificationAnyObject;
        nameHash = OCFHash(searchName);
        objectHash = compares ? OCFHash(object) : pointerHash(object);
    } else if (compares) {
        OCFTypeRef anyObject = kOCFNotificationAnyObject;
        if (object != anyObject) {
            object = OCFEqual(object, anyObject) ? anyObject : object;
        }
        nameHash = OCFHash(searchName);
        objectHash = OCFHash(object);
    } else {
        nameHash = OCFHash(searchName);
        objectHash = pointerHash(object);
    }
    os_unfair_lock_lock(&registrar->lock);
    if (searchName != anyName) {
        NameRecord *nameRecord =
            _CFXNotificationRegistrarFindName(registrar, &registrar->names, searchName, nameHash, NULL);
        if (nameRecord != NULL) {
            ObjectRecord *objectRecord = _CFXNotificationRegistrarFindObject(
                registrar, &nameRecord->children.objects, object, objectHash, compares, NULL
            );
            if (objectRecord != NULL) {
                OCFStringRef copied = CFCachedStringCopyString(&nameRecord->name);
                _CFXNotificationRegistrarFindObservers(
                    registrar,
                    &objectRecord->observers,
                    copied,
                    objectRecord->object,
                    buffer,
                    &invalid,
                    &invalidCount,
                    &invalidCapacity,
                    &invalidOnHeap
                );
                OCFRelease(copied);
            }
        }
    }
    OCFTypeRef anyObject = kOCFNotificationAnyObject;
    if (object != anyObject) {
        ObjectRecord *objectRecord = _CFXNotificationRegistrarFindObject(
            registrar, &registrar->objects, object, objectHash, compares, NULL
        );
        if (objectRecord != NULL) {
            _CFXNotificationRegistrarFindObservers(
                registrar,
                &objectRecord->observers,
                anyName,
                objectRecord->object,
                buffer,
                &invalid,
                &invalidCount,
                &invalidCapacity,
                &invalidOnHeap
            );
        }
    }
    NameRecord *nameRecord =
        _CFXNotificationRegistrarFindName(registrar, &registrar->anyObjectNames, searchName, nameHash, NULL);
    if (nameRecord != NULL) {
        OCFStringRef copied = CFCachedStringCopyString(&nameRecord->name);
        _CFXNotificationRegistrarFindObservers(
            registrar,
            &nameRecord->children.observers,
            copied,
            anyObject,
            buffer,
            &invalid,
            &invalidCount,
            &invalidCapacity,
            &invalidOnHeap
        );
        OCFRelease(copied);
    }
    _CFXNotificationRegistrarFindObservers(
        registrar,
        &registrar->anyObservers,
        anyName,
        anyObject,
        buffer,
        &invalid,
        &invalidCount,
        &invalidCapacity,
        &invalidOnHeap
    );
    DisposalList list = {0};
    for (uint64_t remaining = invalidCount, position = 0; remaining != 0; remaining--, position++) {
        _CFXNotificationRegistrarInvalidateObserver(registrar, (uint32_t)invalid[position], -1, -1, &list);
    }
    os_unfair_lock_unlock(&registrar->lock);
    _CFXNotificationDisposalListRelease(&list);
    if ((registrar->options & kRegistrarSorts) != 0) {
        qsort(buffer->records, buffer->count, sizeof(NotificationRegistration), _registrar_compare);
    }
    if (invalidOnHeap) {
        free(invalid);
    }
}

// Native __CFXNotificationRegistrarFindName, 0x1c02bb9a8. Non-external. The chain of bucket hash %
// bucketCount; a record matches when its hash is equal and its pooled name is CFEqual to the name. The next
// index is read before the comparison. The index is stored only on a match and when requested.
static NameRecord *_CFXNotificationRegistrarFindName(
    OCFXNotificationRegistrarRef registrar,
    RegistrarTable *table,
    OCFStringRef name,
    uint64_t hash,
    int32_t *index
) {
    uint32_t bucketCount = table->bucketCount;
    if (bucketCount == 0) {
        return NULL;
    }
    uint32_t current = (uint32_t)table->buckets[hash % bucketCount];
    while (current != 0xffffffff) {
        NameRecord *record = &registrar->namePool.records[current];
        uint32_t next = (uint32_t)record->next;
        if (record->hash == hash && CFCachedStringEqual(&record->name, name)) {
            if (index != NULL) {
                *index = (int32_t)current;
            }
            return record;
        }
        current = next;
    }
    return NULL;
}

// Native __CFXNotificationRegistrarFindObject, 0x1c02bba60. Non-external. A record matches when its object is
// the same pointer or, when compares is set, CFEqual(object, record object); the next index is read after the
// comparisons.
static ObjectRecord *_CFXNotificationRegistrarFindObject(
    OCFXNotificationRegistrarRef registrar,
    RegistrarTable *table,
    OCFTypeRef object,
    uint64_t hash,
    Boolean compares,
    int32_t *index
) {
    uint32_t bucketCount = table->bucketCount;
    if (bucketCount == 0) {
        return NULL;
    }
    int32_t current = table->buckets[hash % bucketCount];
    if (current == -1) {
        return NULL;
    }
    do {
        ObjectRecord *record = &registrar->objectPool.records[(uint32_t)current];
        if (record->object == object || (compares && OCFEqual(object, record->object))) {
            if (index != NULL) {
                *index = current;
            }
            return record;
        }
        current = record->next;
    } while (current != -1);
    return NULL;
}

// Native CFCachedStringCopyString, 0x1c02bbafc. Non-external. CFRetain of the pooled string under the
// name-pool lock.
static OCFStringRef CFCachedStringCopyString(OCFStringRef *cached) {
    os_unfair_lock_lock(&__CFXNotificationRegistrarNamePoolLock);
    OCFStringRef string = OCFRetain(*cached);
    os_unfair_lock_unlock(&__CFXNotificationRegistrarNamePoolLock);
    return string;
}

// Native __CFXNotificationRegistrarFindObservers, 0x1c02bbb40. Non-external.
// For each observer of the list: the buffer grows to malloc_good_size((count + 1) * 64) bytes when full (a
// realloc, type 0xD28E4C11, of heap records; a malloc, type 0x70791A5B, and a copy of the inline ones; the
// capacity is the size / 64 as 32 bits). A weak observer is loaded retained; when it is gone its index is
// appended to the invalid list (grown the same way in 4-byte entries with the same type IDs) and no record is
// added. Otherwise the record gets the observer, the copied handler, a copy of the name, the object (a copy
// when the buffer copies objects), the flags, the token (index, generation << 32) and the context word. The
// invalid-list state is read on entry and written back on return.
static void _CFXNotificationRegistrarFindObservers(
    OCFXNotificationRegistrarRef registrar,
    ObserverList *list,
    OCFStringRef name,
    OCFTypeRef object,
    NotificationRegistrationBuffer *buffer,
    int32_t **invalid,
    uint32_t *invalidCount,
    uint64_t *invalidCapacity,
    bool *invalidOnHeap
) {
    int32_t *invalidIndices = *invalid;
    uint64_t invalidUsed = *invalidCount;
    uint64_t invalidLimit = *invalidCapacity;
    bool onHeap = *invalidOnHeap;
    uint32_t index = (uint32_t)list->head;
    if (index != 0xffffffff) {
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        do {
            ObserverRecord *record = &registrar->observerPool.records[index];
            uint32_t next = (uint32_t)record->next;
            uint32_t count = buffer->count;
            NotificationRegistration *records;
            if (count + 1 > buffer->capacity) {
                size_t size = malloc_good_size((uint64_t)(count + 1) << 6);
                if (buffer->records != (NotificationRegistration *)(buffer + 1)) {
                    records = malloc_type_realloc(buffer->records, size & 0x3FFFFFFFC0, 0xD28E4C11);
                } else {
                    records = malloc_type_malloc(size & 0x3FFFFFFFC0, 0x70791A5B);
                    memcpy(records, buffer->records, (size_t)buffer->count << 6);
                }
                buffer->capacity = (uint32_t)(size >> 6);
                buffer->records = records;
                count = buffer->count;
            } else {
                records = buffer->records;
            }
            NotificationRegistration *entry = &records[count];
            bool added = true;
            if ((record->flags & kObserverWeak) != 0) {
                id observer = objc_loadWeakRetained((id *)&record->observer);
                if (observer == NULL) {
                    added = false;
                    uint64_t needed = (uint32_t)(invalidUsed + 1);
                    if (invalidLimit < needed) {
                        size_t size = malloc_good_size(needed << 2);
                        if (onHeap) {
                            invalidIndices =
                                malloc_type_realloc(invalidIndices, size & 0x3FFFFFFFC, 0xD28E4C11);
                        } else {
                            int32_t *grown = malloc_type_malloc(size & 0x3FFFFFFFC, 0x70791A5B);
                            memcpy(grown, invalidIndices, (size_t)(uint32_t)invalidUsed << 2);
                            onHeap = true;
                            invalidIndices = grown;
                        }
                        invalidLimit = size >> 2;
                    }
                    invalidIndices[(uint32_t)invalidUsed] = (int32_t)index;
                    invalidUsed = needed;
                } else {
                    entry->observer = observer;
                    count = buffer->count;
                }
            } else {
                entry->observer = record->observer;
            }
            if (added) {
                buffer->count = count + 1;
                OCFXNotificationHandlerCopy(
                    &entry->handler, record->handler.callout, record->handler.context, record->flags
                );
                entry->name = OCFStringCreateCopy(allocator, name);
                OCFTypeRef copied = object;
                if (buffer->copiesObjects) {
                    copied = OCFStringCreateCopy(allocator, (OCFStringRef)object);
                }
                entry->object = copied;
                entry->flags = record->flags;
                entry->token = (uint64_t)index | ((uint64_t)record->generation << 32);
                entry->context = record->context;
            }
            index = next;
        } while (index != 0xffffffff);
    }
    *invalid = invalidIndices;
    *invalidCount = (uint32_t)invalidUsed;
    *invalidCapacity = invalidLimit;
    *invalidOnHeap = onHeap;
}

// Native __CFXNotificationRegistrarInvalidateObserver, 0x1c02bbdcc. Non-external.
// An index at or beyond the observer pool's capacity is ignored. A name-only observer (0x800) leaves its
// name's observer list; an emptied name other than keepName is then recycled from the any-object name table.
// An invalid record (0x8000) is ignored. An observer without an object (-1) leaves the any-name any-object
// list. Otherwise (object index below the object pool's capacity) it leaves its object's list; an emptied
// object other than keepObject is recycled from the any-name object table, or from its name's object table,
// and an emptied name other than keepName is then recycled from the name table. The name pool's records
// pointer is read before the observer is recycled.
static void _CFXNotificationRegistrarInvalidateObserver(
    OCFXNotificationRegistrarRef registrar,
    uint32_t index,
    int32_t keepName,
    int32_t keepObject,
    DisposalList *list
) {
    if (index >= registrar->observerPool.capacity) {
        return;
    }
    ObserverRecord *record = &registrar->observerPool.records[index];
    uint32_t flags = record->flags;
    if ((flags & kObserverNameOnly) != 0) {
        int32_t nameIndex = record->parent;
        NameRecord *nameRecord = &registrar->namePool.records[(uint32_t)nameIndex];
        Boolean empty = _CFXNotificationRegistrarRecycleObserverRegistration(
            registrar, &nameRecord->children.observers, index, list
        );
        if (nameIndex == keepName || !empty) {
            return;
        }
        _CFXNotificationRegistrarRecycleNameRegistration(
            registrar, &registrar->anyObjectNames, nameIndex, true
        );
        return;
    }
    if ((flags & kObserverInvalid) != 0) {
        return;
    }
    int32_t objectIndex = record->parent;
    if (objectIndex == -1) {
        _CFXNotificationRegistrarRecycleObserverRegistration(
            registrar, &registrar->anyObservers, index, list
        );
        return;
    }
    if ((uint32_t)objectIndex >= registrar->objectPool.capacity) {
        return;
    }
    Boolean compares = (registrar->options >> 1) & 1;
    ObjectRecord *objectRecord = &registrar->objectPool.records[(uint32_t)objectIndex];
    int32_t nameIndex = objectRecord->name;
    if (nameIndex == -1) {
        Boolean empty = _CFXNotificationRegistrarRecycleObserverRegistration(
            registrar, &objectRecord->observers, index, list
        );
        if (objectIndex == keepObject || !empty) {
            return;
        }
        _CFXNotificationRegistrarRecycleObjectRegistration(
            registrar, &registrar->objects, objectIndex, compares
        );
        return;
    }
    NameRecord *names = registrar->namePool.records;
    Boolean empty = _CFXNotificationRegistrarRecycleObserverRegistration(
        registrar, &objectRecord->observers, index, list
    );
    if (objectIndex == keepObject || !empty) {
        return;
    }
    empty = _CFXNotificationRegistrarRecycleObjectRegistration(
        registrar, &names[(uint32_t)nameIndex].children.objects, objectIndex, compares
    );
    if (nameIndex == keepName || !empty) {
        return;
    }
    _CFXNotificationRegistrarRecycleNameRegistration(registrar, &registrar->names, nameIndex, false);
}

// Native __registrar_compare, 0x1c02bbf7c. Non-external; the qsort comparator: the tokens as unsigned 64-bit
// values, 1, -1 or 0.
static int _registrar_compare(const void *first, const void *second) {
    uint64_t left = ((const NotificationRegistration *)first)->token;
    uint64_t right = ((const NotificationRegistration *)second)->token;
    if (left < right) {
        return -1;
    }
    return left > right ? 1 : 0;
}

// Native CFXNotificationRegistrarFindToken, 0x1c02bbf94. External.
// The buffer's copiesObjects is the options' bit 1. Under the lock, a token whose index is below the observer
// pool's capacity, whose generation matches and whose record is valid yields one registration, built in a
// zeroed stack record: the observer (loaded retained when weak; a weak observer that is gone is invalidated
// and nothing is found); for a name-only observer the pooled name and kCFNotificationAnyObject (retained when
// objects are copied); for an observer without an object no name (NULL, as natively) and
// kCFNotificationAnyObject (retained when copied); otherwise the object's name (kCFNotificationAnyName,
// retained, without one) and the object (a string copy when copied). Then the handler copy, the flags and the
// token; the context word stays 0. After the unlock and the release of a zeroed disposal list, a found record
// is copied to the first buffer record and the count is 1, else 0.
Boolean OCFXNotificationRegistrarFindToken(
    OCFXNotificationRegistrarRef registrar, uint64_t token, NotificationRegistrationBuffer *buffer
) {
    Boolean compares = (registrar->options >> 1) & 1;
    buffer->copiesObjects = compares;
    NotificationRegistration entry = {0};
    Boolean found = false;
    DisposalList list;
    os_unfair_lock_lock(&registrar->lock);
    uint32_t index = (uint32_t)token;
    if (registrar->observerPool.capacity > index) {
        ObserverRecord *record = &registrar->observerPool.records[index];
        if (record->generation == (uint32_t)(token >> 32) && (record->flags & kObserverInvalid) == 0) {
            bool present = true;
            if ((record->flags & kObserverWeak) != 0) {
                id observer = objc_loadWeakRetained((id *)&record->observer);
                if (observer == NULL) {
                    present = false;
                    list = (DisposalList){0};
                    _CFXNotificationRegistrarInvalidateObserver(registrar, index, -1, -1, &list);
                } else {
                    entry.observer = observer;
                }
            } else {
                entry.observer = record->observer;
            }
            if (present) {
                int32_t parent = record->parent;
                if ((record->flags & kObserverNameOnly) != 0) {
                    OCFTypeRef anyObject = kOCFNotificationAnyObject;
                    if (compares) {
                        anyObject = OCFRetain(anyObject);
                    }
                    entry.object = anyObject;
                    entry.name =
                        CFCachedStringCopyString(&registrar->namePool.records[(uint32_t)parent].name);
                } else if (parent == -1) {
                    OCFTypeRef anyObject = kOCFNotificationAnyObject;
                    if (compares) {
                        anyObject = OCFRetain(anyObject);
                    }
                    entry.object = anyObject;
                } else {
                    ObjectRecord *objectRecord = &registrar->objectPool.records[(uint32_t)parent];
                    if (objectRecord->name == -1) {
                        entry.name = OCFRetain(kOCFNotificationAnyName);
                    } else {
                        entry.name = CFCachedStringCopyString(
                            &registrar->namePool.records[(uint32_t)objectRecord->name].name
                        );
                    }
                    if (!compares) {
                        entry.object = objectRecord->object;
                    } else {
                        entry.object = OCFStringCreateCopy(
                            kOCFAllocatorSystemDefault, (OCFStringRef)objectRecord->object
                        );
                    }
                }
                OCFXNotificationHandlerCopy(
                    &entry.handler, record->handler.callout, record->handler.context, record->flags
                );
                entry.flags = record->flags;
                entry.token = (uint64_t)index | ((uint64_t)record->generation << 32);
                found = true;
                list = (DisposalList){0};
            }
        } else {
            list = (DisposalList){0};
        }
    } else {
        list = (DisposalList){0};
    }
    os_unfair_lock_unlock(&registrar->lock);
    _CFXNotificationDisposalListRelease(&list);
    if (found) {
        buffer->records[0] = entry;
        buffer->count = 1;
    } else {
        buffer->count = 0;
    }
    return found;
}

#ifdef __BLOCKS__
// Native CFXNotificationRegistrarRemove, 0x1c02bc1d8. External. A NULL or equal-to-any name becomes
// kCFNotificationAnyName, a NULL object kCFNotificationAnyObject (an object equal to it only when the
// registrar compares objects). Both hashes are full CFHash values; an uncompared object hashes by pointer.
// Under the lock the name table is searched first; the any-name case also clears the any-name object table,
// and an any-object removal clears the any-object name table and, for any name, the any-observer list.
void OCFXNotificationRegistrarRemove(
    OCFXNotificationRegistrarRef registrar, OCFStringRef name, OCFTypeRef object, void *observer
) {
    uint8_t options = (uint8_t)registrar->options;
    OCFStringRef anyName = kOCFNotificationAnyName;
    if (name == NULL) {
        name = anyName;
    } else if (name == anyName) {
        anyName = name;
    } else if (OCFEqual(name, anyName)) {
        name = anyName;
    }
    Boolean compares = (options >> 1) & 1;
    uint64_t nameHash;
    uint64_t objectHash;
    if (object == NULL) {
        object = kOCFNotificationAnyObject;
        nameHash = OCFHash(name);
        if (!compares) {
            objectHash = pointerHash(object);
        } else {
            objectHash = OCFHash(object);
        }
    } else if (!compares) {
        nameHash = OCFHash(name);
        objectHash = pointerHash(object);
    } else {
        OCFTypeRef anyObject = kOCFNotificationAnyObject;
        if (object != anyObject && OCFEqual(object, anyObject)) {
            object = anyObject;
        }
        nameHash = OCFHash(name);
        objectHash = OCFHash(object);
    }
    os_unfair_lock_lock(&registrar->lock);
    _CFXNotificationRegistrarRemoveNames(
        registrar, &registrar->names, false, name, nameHash, object, objectHash, compares, observer
    );
    if (name == anyName) {
        _CFXNotificationRegistrarRemoveObjects(
            registrar, &registrar->objects, object, objectHash, compares, observer
        );
        if (object == kOCFNotificationAnyObject) {
            _CFXNotificationRegistrarRemoveNames(
                registrar,
                &registrar->anyObjectNames,
                true,
                name,
                nameHash,
                object,
                objectHash,
                compares,
                observer
            );
            _CFXNotificationRegistrarRemoveObservers(registrar, &registrar->anyObservers, observer);
        }
    } else if (object == kOCFNotificationAnyObject) {
        _CFXNotificationRegistrarRemoveNames(
            registrar,
            &registrar->anyObjectNames,
            true,
            name,
            nameHash,
            object,
            objectHash,
            compares,
            observer
        );
    }
    os_unfair_lock_unlock(&registrar->lock);
}

// Native __CFXNotificationRegistrarRemoveNames, 0x1c02bc3c4. Non-external. For the any name, every name of
// the table: its observers (an any-object table) or its objects are removed, and a name left empty is
// recycled (keeping its children for an observer table). For another name, the first record of its bucket
// chain equal to it whose children become empty is recycled, then the walk stops.
static void _CFXNotificationRegistrarRemoveNames(
    OCFXNotificationRegistrarRef registrar,
    RegistrarTable *table,
    Boolean observerTables,
    OCFStringRef name,
    uint64_t nameHash,
    OCFTypeRef object,
    uint64_t objectHash,
    Boolean compares,
    void *observer
) {
    uint64_t bucketCount = table->bucketCount;
    if ((uint32_t)bucketCount == 0) {
        return;
    }
    if (name == kOCFNotificationAnyName) {
        for (uint64_t bucket = 0; bucket != bucketCount; bucket++) {
            int32_t index = table->buckets[bucket];
            while (index != -1) {
                NameRecord *record = &registrar->namePool.records[(uint32_t)index];
                int32_t next = record->next;
                if (observerTables) {
                    if (_CFXNotificationRegistrarRemoveObservers(
                            registrar, &record->children.observers, observer
                        )) {
                        _CFXNotificationRegistrarRecycleNameRegistration(registrar, table, index, true);
                    }
                } else if (
                    _CFXNotificationRegistrarRemoveObjects(
                        registrar, &record->children.objects, object, objectHash, compares, observer
                    )
                ) {
                    _CFXNotificationRegistrarRecycleNameRegistration(registrar, table, index, false);
                }
                index = next;
            }
        }
        return;
    }
    int32_t index = table->buckets[nameHash % bucketCount];
    while (index != -1) {
        NameRecord *record = &registrar->namePool.records[(uint32_t)index];
        int32_t next = record->next;
        if (CFCachedStringEqual(&record->name, name)) {
            if (observerTables) {
                if (_CFXNotificationRegistrarRemoveObservers(
                        registrar, &record->children.observers, observer
                    )) {
                    _CFXNotificationRegistrarRecycleNameRegistration(registrar, table, index, true);
                    return;
                }
            } else if (
                _CFXNotificationRegistrarRemoveObjects(
                    registrar, &record->children.objects, object, objectHash, compares, observer
                )
            ) {
                _CFXNotificationRegistrarRecycleNameRegistration(registrar, table, index, false);
                return;
            }
        }
        index = next;
    }
}

// Native __CFXNotificationRegistrarRemoveObjects, 0x1c02bc588. Non-external. The any object walks the whole
// table, recycling each object whose observers become empty; another object walks its bucket chain (pointer
// equality, or CFEqual when the registrar compares) and stops at the first record it recycles, answering
// true. Otherwise the result is whether the table is now empty.
static Boolean _CFXNotificationRegistrarRemoveObjects(
    OCFXNotificationRegistrarRef registrar,
    RegistrarTable *table,
    OCFTypeRef object,
    uint64_t hash,
    Boolean compares,
    void *observer
) {
    uint64_t bucketCount = table->bucketCount;
    if ((uint32_t)bucketCount == 0) {
        return table->count == 0;
    }
    if (object == kOCFNotificationAnyObject) {
        for (uint64_t bucket = 0; bucket != bucketCount; bucket++) {
            uint64_t index = (uint32_t)table->buckets[bucket];
            while (index != 0xffffffff) {
                ObjectRecord *record = &registrar->objectPool.records[index];
                uint64_t next = (uint32_t)record->next;
                if (_CFXNotificationRegistrarRemoveObservers(registrar, &record->observers, observer)) {
                    _CFXNotificationRegistrarRecycleObjectRegistration(
                        registrar, table, (int32_t)index, compares
                    );
                }
                index = next;
            }
        }
        return table->count == 0;
    }
    int32_t index = table->buckets[hash % bucketCount];
    while (index != -1) {
        ObjectRecord *record = &registrar->objectPool.records[(uint32_t)index];
        int32_t next = record->next;
        OCFTypeRef stored = record->object;
        if ((stored == object || (compares && OCFEqual(object, stored))) &&
            _CFXNotificationRegistrarRemoveObservers(registrar, &record->observers, observer) &&
            _CFXNotificationRegistrarRecycleObjectRegistration(registrar, table, index, compares)) {
            return true;
        }
        index = next;
    }
    return table->count == 0;
}

// Native __CFXNotificationRegistrarRemoveObservers, 0x1c02bc6e8. Non-external. Every record of the list whose
// observer matches (any observer for NULL) is recycled onto a stack disposal list; the result is whether the
// list is empty. CoreFoundation defect: when a recycle empties the list, the native code (0x1c02bc788 to
// 0x1c02bc78c) returns without releasing the disposal list, leaking the handler block or context the recycle
// queued on it. Confidence: verified. Release the list on that path too.
static Boolean _CFXNotificationRegistrarRemoveObservers(
    OCFXNotificationRegistrarRef registrar, ObserverList *list, void *observer
) {
    DisposalList disposal;
    memset(&disposal, 0, sizeof(disposal));
    uint32_t index = (uint32_t)list->head;
    while (index != 0xffffffff) {
        ObserverRecord *record = &registrar->observerPool.records[index];
        uint32_t next = (uint32_t)record->next;
        if ((observer == NULL || record->observer == observer) &&
            _CFXNotificationRegistrarRecycleObserverRegistration(registrar, list, index, &disposal)) {
            _CFXNotificationDisposalListRelease(&disposal);
            return true;
        }
        index = next;
    }
    _CFXNotificationDisposalListRelease(&disposal);
    return list->count == 0;
}

// Native CFXNotificationRegistrarRemoveToken, 0x1c02bc7c4. External. A nonzero token whose index is below the
// observer pool's capacity and whose generation matches invalidates that observer (keeping no name or
// object), then the disposal list is released after the lock.
void OCFXNotificationRegistrarRemoveToken(OCFXNotificationRegistrarRef registrar, uint64_t token) {
    if (token == 0) {
        return;
    }
    DisposalList disposal;
    memset(&disposal, 0, sizeof(disposal));
    os_unfair_lock_lock(&registrar->lock);
    uint32_t index = (uint32_t)token;
    if (registrar->observerPool.capacity > index &&
        registrar->observerPool.records[index].generation == (uint32_t)(token >> 32)) {
        _CFXNotificationRegistrarInvalidateObserver(registrar, index, -1, -1, &disposal);
    }
    os_unfair_lock_unlock(&registrar->lock);
    _CFXNotificationDisposalListRelease(&disposal);
}

// Native CFXNotificationRegistrarAdd, 0x1c02bc880. External. The name and object are normalized as in Remove
// (object equality to the any object only when the registrar compares); a NULL observer becomes
// kCFNotificationAnyObserver. The generation counter (0xa8) is incremented atomically outside the lock and
// becomes the token's high half. A named registration goes through the name table (its object table, or for
// the any object the any-object name table with flag 0x800), an any-name one through the any-name object
// table or, for any object too, the any-observer list. The result is the generation << 32 | the observer
// index.
uint64_t OCFXNotificationRegistrarAdd(
    OCFXNotificationRegistrarRef registrar,
    OCFStringRef name,
    OCFTypeRef object,
    void *observer,
    OCFOptionFlags flags,
    void *callout,
    OCFTypeRef context
) {
    uint8_t options = (uint8_t)registrar->options;
    OCFStringRef anyName = kOCFNotificationAnyName;
    if (name == NULL) {
        name = anyName;
    } else if (name != anyName && OCFEqual(name, anyName)) {
        name = anyName;
    }
    Boolean compares = (options >> 1) & 1;
    if (object == NULL) {
        object = kOCFNotificationAnyObject;
    } else if (
        compares && object != kOCFNotificationAnyObject && OCFEqual(object, kOCFNotificationAnyObject)
    ) {
        object = kOCFNotificationAnyObject;
    }
    if (observer == NULL) {
        observer = (void *)kOCFNotificationAnyObserver;
    }
    uint64_t nameHash = OCFHash(name);
    uint64_t objectHash = compares ? OCFHash(object) : pointerHash(object);
    uint64_t token = (uint64_t)__atomic_fetch_add(&registrar->nextGeneration, 1, __ATOMIC_SEQ_CST) << 32;
    int32_t objectIndex = -1;
    int32_t nameIndex = -1;
    uint32_t observerIndex = 0xffffffff;
    DisposalList disposal;
    memset(&disposal, 0, sizeof(disposal));
    os_unfair_lock_lock(&registrar->lock);
    if (name != anyName) {
        if (object != kOCFNotificationAnyObject) {
            NameRecord *record = _CFXNotificationRegistrarAddName(
                registrar, &registrar->names, name, nameHash, &nameIndex, false
            );
            int32_t keepName = nameIndex;
            ObjectRecord *objectRecord = _CFXNotificationRegistrarAddObject(
                registrar, &record->children.objects, object, objectHash, &objectIndex, keepName, compares
            );
            _CFXNotificationRegistrarAddObserver(
                registrar,
                &objectRecord->observers,
                callout,
                context,
                objectIndex,
                observer,
                &observerIndex,
                flags,
                token,
                keepName,
                objectIndex,
                &disposal
            );
        } else {
            NameRecord *record = _CFXNotificationRegistrarAddName(
                registrar, &registrar->anyObjectNames, name, nameHash, &nameIndex, true
            );
            _CFXNotificationRegistrarAddObserver(
                registrar,
                &record->children.observers,
                callout,
                context,
                nameIndex,
                observer,
                &observerIndex,
                flags | kObserverNameOnly,
                token,
                nameIndex,
                -1,
                &disposal
            );
        }
    } else if (object != kOCFNotificationAnyObject) {
        ObjectRecord *objectRecord = _CFXNotificationRegistrarAddObject(
            registrar, &registrar->objects, object, objectHash, &objectIndex, -1, compares
        );
        _CFXNotificationRegistrarAddObserver(
            registrar,
            &objectRecord->observers,
            callout,
            context,
            objectIndex,
            observer,
            &observerIndex,
            flags,
            token,
            -1,
            objectIndex,
            &disposal
        );
    } else {
        _CFXNotificationRegistrarAddObserver(
            registrar,
            &registrar->anyObservers,
            callout,
            context,
            -1,
            observer,
            &observerIndex,
            flags,
            token,
            -1,
            -1,
            &disposal
        );
    }
    os_unfair_lock_unlock(&registrar->lock);
    _CFXNotificationDisposalListRelease(&disposal);
    return token | observerIndex;
}

// Native _CFXNotificationRegistrarGrowObserverPool, inlined into __CFXNotificationRegistrarAddObserver
// (0x1c02bcbbc to 0x1c02bce80; CFXNotificationRegistrar.c line 1123). The capacity grows to
// malloc_good_size((capacity + 1) * 56) / 56 records in a new allocation: each old record is copied (its
// reserved word is not); a weak observer moves with objc_moveWeak, and one whose weak storage is already zero
// is collected, cleared and invalidated after the copy (skipping an index equal to the object index, as
// natively, and keeping the name and object being added to). The new records join the free list, flags 0x8000
// and parent -1, with their context and generation words left as allocated. A capacity of 0xffffffff is not
// grown.
static inline void _CFXNotificationRegistrarGrowObserverPool(
    OCFXNotificationRegistrarRef registrar, int32_t nameIndex, int32_t objectIndex, DisposalList *disposal
) {
    uint32_t capacity = registrar->observerPool.capacity;
    if (capacity == 0xffffffff) {
        return;
    }
    size_t size = malloc_good_size((size_t)capacity * 56 + 56);
    uint64_t newCapacity = size / 56;
    uint64_t oldCount = registrar->observerPool.capacity;
    ObserverRecord *old = registrar->observerPool.records;
    ObserverRecord *records = malloc_type_malloc((uint32_t)newCapacity * 56, 0x70791A5B);
    uint32_t inlineZeroed[32];
    memset(inlineZeroed, 0, sizeof(inlineZeroed));
    if ((uint32_t)oldCount == 0) {
        registrar->observerPool.records = records;
        free(old);
    } else {
        uint32_t *zeroed = inlineZeroed;
        uint64_t zeroedCapacity = 32;
        uint32_t zeroedCount = 0;
        for (uint32_t index = 0; index != (uint32_t)oldCount; index++) {
            ObserverRecord *from = &old[index];
            ObserverRecord *to = &records[index];
            if ((from->flags & kObserverWeak) == 0) {
                to->observer = from->observer;
            } else if (from->observer != NULL) {
                objc_moveWeak((id *)&to->observer, (id *)&from->observer);
            } else {
                if (zeroedCount + 1 > (uint32_t)zeroedCapacity) {
                    uint32_t grown = (uint32_t)zeroedCapacity + 1;
                    if (zeroed == inlineZeroed) {
                        uint32_t *heap = malloc_type_malloc((size_t)grown * 4, 0x70791A5B);
                        memcpy(heap, zeroed, (size_t)(uint32_t)zeroedCapacity * 4);
                        zeroed = heap;
                    } else {
                        zeroed = malloc_type_realloc(zeroed, (size_t)grown * 4, 0xD28E4C11);
                    }
                    zeroedCapacity = grown;
                }
                zeroed[zeroedCount] = index;
                to->observer = NULL;
                zeroedCount++;
            }
            to->previous = from->previous;
            to->next = from->next;
            to->handler = from->handler;
            to->context = from->context;
            to->generation = from->generation;
            to->flags = from->flags;
            to->parent = from->parent;
        }
        registrar->observerPool.records = records;
        free(old);
        for (uint32_t item = 0; item != zeroedCount; item++) {
            if ((int32_t)zeroed[item] != objectIndex) {
                _CFXNotificationRegistrarInvalidateObserver(
                    registrar, zeroed[item], nameIndex, objectIndex, disposal
                );
            }
        }
        if (zeroed != inlineZeroed) {
            free(zeroed);
        }
    }
    uint32_t count = registrar->observerPool.capacity;
    for (uint64_t index = oldCount;
         (uint32_t)oldCount < (uint32_t)newCapacity && index != (uint32_t)newCapacity;
         index++) {
        ObserverRecord *record = &records[index];
        record->handler.context = NULL;
        record->observer = NULL;
        record->handler.callout = NULL;
        record->flags = kObserverInvalid;
        record->parent = -1;
        ObserverRecord *pool = registrar->observerPool.records;
        int32_t freeHead = registrar->observerPool.freeHead;
        if (freeHead != -1) {
            pool[(uint32_t)freeHead].previous = (int32_t)index;
        }
        pool[index].previous = -1;
        pool[index].next = freeHead;
        count = registrar->observerPool.capacity + 1;
        registrar->observerPool.capacity = count;
        registrar->observerPool.freeHead = (int32_t)index;
    }
    if (count != (uint32_t)newCapacity) {
        __assert_rtn(
            "_CFXNotificationRegistrarGrowObserverPool",
            "CFXNotificationRegistrar.c",
            1123,
            "registrar->observerPool.capacity == newCapacity"
        );
    }
}

// Native __CFXNotificationRegistrarAddObserver, 0x1c02bcb20 (assertion .cold.2 0x1c0312020, line 2235; the
// pool growth's .cold.1 0x1c0311ff8). Non-external. A record is taken from the free list (growing the pool
// when it is empty), its index stored, the handler copied, the token's generation and the parent index
// stored, the observer stored weakly under flag 0x400 (strongly, with the flag cleared, when
// objc_storeWeakOrNil refuses it), the context word cleared and the record linked at the head of the list.
// Without a handler type flag (0xe0000) the native code asserts.
static ObserverRecord *_CFXNotificationRegistrarAddObserver(
    OCFXNotificationRegistrarRef registrar,
    ObserverList *list,
    void *callout,
    OCFTypeRef context,
    int32_t parent,
    void *observer,
    uint32_t *index,
    OCFOptionFlags flags,
    uint64_t token,
    int32_t nameIndex,
    int32_t objectIndex,
    DisposalList *disposal
) {
    if ((flags & 0xe0000) == 0) {
        __assert_rtn(
            "_CFXNotificationRegistrarAddObserver",
            "CFXNotificationRegistrar.c",
            2235,
            "(options & kCFXNotificationValidHandlers) != 0"
        );
    }
    if (registrar->observerPool.freeHead == -1) {
        _CFXNotificationRegistrarGrowObserverPool(registrar, nameIndex, objectIndex, disposal);
    }
    uint32_t freeIndex = (uint32_t)registrar->observerPool.freeHead;
    ObserverRecord *records = registrar->observerPool.records;
    if (freeIndex != 0xffffffff) {
        int32_t previous = records[freeIndex].previous;
        int32_t next = records[freeIndex].next;
        registrar->observerPool.freeHead = next;
        if (previous != -1) {
            records[(uint32_t)previous].next = next;
        }
        if (next != -1) {
            records[(uint32_t)next].previous = previous;
        }
    }
    registrar->observerPool.used = registrar->observerPool.used + 1;
    ObserverRecord *record = &records[freeIndex];
    *index = freeIndex;
    OCFXNotificationHandlerCopy(&record->handler, callout, context, (uint32_t)flags);
    record->generation = (uint32_t)(token >> 32);
    record->parent = parent;
    if ((flags & kObserverWeak) == 0) {
        record->observer = observer;
    } else if (objc_storeWeakOrNil((id *)&record->observer, (id)observer) == nil) {
        record->observer = observer;
        flags &= ~(OCFOptionFlags)kObserverWeak;
    }
    record->flags = (uint32_t)flags;
    record->context = 0;
    uint32_t added = *index;
    int32_t head = list->head;
    if (head != -1) {
        registrar->observerPool.records[(uint32_t)head].previous = (int32_t)added;
    }
    record->previous = -1;
    record->next = head;
    list->head = (int32_t)added;
    list->count = list->count + 1;
    return record;
}

// Native _CFXNotificationObjectTableRehash, inlined into __CFXNotificationRegistrarAddObject (0x1c02bd104 to
// 0x1c02bd258; assertion .cold.1 0x1c0312048, line 2048). The table needs malloc_good_size(4 * max(1,
// floor(log2(count + 1)))) / 4 buckets; a smaller table moves to a new array of that many (0xff-filled)
// buckets, each record re-hashed (CFHash of the stored object when the registrar compares, else the pointer
// hash) and linked at the head of its new bucket.
static inline uint32_t *_CFXNotificationObjectTableRehash(
    OCFXNotificationRegistrarRef registrar, RegistrarTable *table, Boolean compares
) {
    uint32_t bits = 31 - __builtin_clz(table->count + 1);
    size_t size = malloc_good_size((size_t)(bits > 1 ? bits : 1) * 4);
    uint64_t bucketCount = size >> 2;
    if ((uint32_t)bucketCount == 0) {
        __assert_rtn(
            "_CFXNotificationObjectTableRehash", "CFXNotificationRegistrar.c", 2048, "newBucketCount != 0"
        );
    }
    int32_t *old = table->buckets;
    if (table->bucketCount >= (uint32_t)bucketCount) {
        return (uint32_t *)old;
    }
    int32_t *buckets = malloc_type_malloc(size & 0x3FFFFFFFC, 0x70791A5B);
    if (old == NULL) {
        table->buckets = buckets;
        table->bucketCount = (uint32_t)bucketCount;
        memset(buckets, 0xff, size & 0x3FFFFFFFC);
        return (uint32_t *)buckets;
    }
    memset(buckets, 0xff, size & 0x3FFFFFFFC);
    uint64_t divisor = (uint32_t)(size >> 2);
    for (uint64_t bucket = 0; bucket < table->bucketCount; bucket++) {
        uint64_t index = (uint32_t)old[bucket];
        while (index != 0xffffffff) {
            ObjectRecord *record = &registrar->objectPool.records[index];
            uint64_t hash = compares ? OCFHash(record->object) : pointerHash(record->object);
            uint64_t next = (uint32_t)record->next;
            int32_t head = buckets[hash % divisor];
            record->previous = -1;
            record->next = head;
            if (head != -1) {
                registrar->objectPool.records[(uint32_t)head].previous = (int32_t)index;
            }
            buckets[hash % divisor] = (int32_t)index;
            index = next;
        }
    }
    table->buckets = buckets;
    table->bucketCount = (uint32_t)bucketCount;
    free(old);
    return (uint32_t *)table->buckets;
}

// Native __CFXNotificationRegistrarAddObject, 0x1c02bcf7c. Non-external. An existing record is returned with
// its index; otherwise the object pool grows by malloc_good_size((capacity + 1) * 32) / 32 records (realloc,
// the new ones on the free list with no name, a NULL object and an empty observer list), a record is taken
// from the free list, its object stored (a pooled string when the registrar compares) and its name index set,
// the table rehashed when too small and the record linked at the head of its bucket.
static ObjectRecord *_CFXNotificationRegistrarAddObject(
    OCFXNotificationRegistrarRef registrar,
    RegistrarTable *table,
    OCFTypeRef object,
    uint64_t hash,
    int32_t *index,
    int32_t nameIndex,
    Boolean compares
) {
    ObjectRecord *found =
        _CFXNotificationRegistrarFindObject(registrar, table, object, hash, compares, index);
    if (found != NULL) {
        return found;
    }
    ObjectRecord *records;
    int32_t freeIndex = registrar->objectPool.freeHead;
    if (freeIndex == -1) {
        size_t size = malloc_good_size((size_t)(uint32_t)(registrar->objectPool.capacity + 1) * 32);
        uint64_t capacity = registrar->objectPool.capacity;
        if (registrar->objectPool.records != NULL) {
            records = malloc_type_realloc(registrar->objectPool.records, size & 0x1FFFFFFFE0, 0xD28E4C11);
        } else {
            records = malloc_type_malloc(size & 0x1FFFFFFFE0, 0x70791A5B);
        }
        registrar->objectPool.capacity = (uint32_t)(size >> 5);
        registrar->objectPool.records = records;
        for (; (uint32_t)capacity < (uint32_t)(size >> 5); capacity++) {
            int32_t freeHead = registrar->objectPool.freeHead;
            if (freeHead != -1) {
                records[(uint32_t)freeHead].previous = (int32_t)capacity;
            }
            records[capacity].previous = -1;
            records[capacity].next = freeHead;
            registrar->objectPool.freeHead = (int32_t)capacity;
            records[capacity].name = -1;
            records[capacity].object = NULL;
            records[capacity].observers.head = -1;
            records[capacity].observers.count = 0;
        }
        freeIndex = registrar->objectPool.freeHead;
    } else {
        records = registrar->objectPool.records;
    }
    if (freeIndex != -1) {
        int32_t previous = records[(uint32_t)freeIndex].previous;
        int32_t next = records[(uint32_t)freeIndex].next;
        registrar->objectPool.freeHead = next;
        if (previous != -1) {
            records[(uint32_t)previous].next = next;
        }
        if (next != -1) {
            records[(uint32_t)next].previous = previous;
        }
    }
    registrar->objectPool.used = registrar->objectPool.used + 1;
    *index = freeIndex;
    ObjectRecord *record = &records[(uint32_t)freeIndex];
    record->name = -1;
    record->observers.head = -1;
    record->observers.count = 0;
    if (compares) {
        _CFXNotificationRegistrarAddCachedName((OCFStringRef)object, (OCFStringRef *)&record->object);
    } else {
        record->object = object;
    }
    record->name = nameIndex;
    int32_t added = *index;
    int32_t *buckets = (int32_t *)_CFXNotificationObjectTableRehash(registrar, table, compares);
    uint64_t slot = hash % table->bucketCount;
    int32_t head = buckets[slot];
    if (head != -1) {
        registrar->objectPool.records[(uint32_t)head].previous = added;
    }
    record->previous = -1;
    record->next = head;
    buckets[slot] = added;
    table->count = table->count + 1;
    return record;
}

// Native _CFXNotificationNameTableRehash, inlined into __CFXNotificationRegistrarAddName (0x1c02bd460 to
// 0x1c02bd5c0; assertion .cold.1 0x1c0312070, line 1979). The bucket count follows the object table's rule; a
// smaller table moves to a new 0xff-filled array: each record is unlinked from its old chain (rewriting the
// old bucket head and the neighbors) and linked at the head of its new bucket by its stored hash.
static inline int32_t *
_CFXNotificationNameTableRehash(OCFXNotificationRegistrarRef registrar, RegistrarTable *table) {
    uint32_t bits = 31 - __builtin_clz(table->count + 1);
    size_t size = malloc_good_size((size_t)(bits > 1 ? bits : 1) * 4);
    uint64_t bucketCount = size >> 2;
    if ((uint32_t)bucketCount == 0) {
        __assert_rtn(
            "_CFXNotificationNameTableRehash", "CFXNotificationRegistrar.c", 1979, "newBucketCount != 0"
        );
    }
    int32_t *old = table->buckets;
    if (table->bucketCount >= (uint32_t)bucketCount) {
        return old;
    }
    int32_t *buckets = malloc_type_malloc(size & 0x3FFFFFFFC, 0x70791A5B);
    if (old == NULL) {
        table->buckets = buckets;
        table->bucketCount = (uint32_t)bucketCount;
        memset(buckets, 0xff, size & 0x3FFFFFFFC);
        return buckets;
    }
    memset(buckets, 0xff, size & 0x3FFFFFFFC);
    old = table->buckets;
    uint64_t oldCount = table->bucketCount;
    uint64_t divisor = (uint32_t)(size >> 2);
    for (uint64_t bucket = 0; bucket < table->bucketCount; bucket++) {
        int32_t index = old[bucket];
        while (index != -1) {
            NameRecord *records = registrar->namePool.records;
            NameRecord *record = &records[(uint32_t)index];
            uint64_t hash = record->hash;
            int32_t next = record->next;
            if (old != NULL) {
                uint64_t slot = hash % oldCount;
                int32_t previous = record->previous;
                if (old[slot] == index) {
                    old[slot] = next;
                }
                if (previous != -1) {
                    records[(uint32_t)previous].next = next;
                }
                if (next != -1) {
                    records[(uint32_t)next].previous = previous;
                }
            }
            if (buckets != NULL) {
                uint64_t slot = hash % divisor;
                int32_t head = buckets[slot];
                if (head != -1) {
                    records[(uint32_t)head].previous = index;
                }
                record->previous = -1;
                record->next = head;
                buckets[slot] = index;
            }
            index = next;
        }
    }
    table->buckets = buckets;
    table->bucketCount = (uint32_t)bucketCount;
    free(old);
    return table->buckets;
}

// Native __CFXNotificationRegistrarAddName, 0x1c02bd2bc. Non-external. An existing record is returned with
// its index; otherwise the name pool grows by malloc_good_size((capacity + 1) * 40) / 40 records (realloc;
// the new ones on the free list with the name, both child words and the count cleared, the hash left as
// allocated), a record is taken from the free list, its name pooled, its children set to an empty observer
// list (an any-object name table) or an empty object table, the table rehashed when too small, the hash
// stored, and a record with an index other than -1 linked at the head of its bucket when the table has
// buckets.
static NameRecord *_CFXNotificationRegistrarAddName(
    OCFXNotificationRegistrarRef registrar,
    RegistrarTable *table,
    OCFStringRef name,
    uint64_t hash,
    int32_t *index,
    Boolean observerTables
) {
    NameRecord *found = _CFXNotificationRegistrarFindName(registrar, table, name, hash, index);
    if (found != NULL) {
        return found;
    }
    NameRecord *records;
    if (registrar->namePool.freeHead == -1) {
        size_t size = malloc_good_size((size_t)(uint32_t)(registrar->namePool.capacity + 1) * 40);
        uint64_t newCapacity = size / 40;
        uint64_t capacity = registrar->namePool.capacity;
        size_t bytes = ((uint64_t)(uint32_t)newCapacity * 4 + (uint32_t)newCapacity) * 8;
        if (registrar->namePool.records != NULL) {
            records = malloc_type_realloc(registrar->namePool.records, bytes, 0xD28E4C11);
        } else {
            records = malloc_type_malloc(bytes, 0x70791A5B);
        }
        registrar->namePool.capacity = (uint32_t)newCapacity;
        registrar->namePool.records = records;
        for (; (uint32_t)capacity < (uint32_t)newCapacity; capacity++) {
            NameRecord *pool = registrar->namePool.records;
            int32_t freeHead = registrar->namePool.freeHead;
            if (freeHead != -1) {
                pool[(uint32_t)freeHead].previous = (int32_t)capacity;
            }
            pool[capacity].previous = -1;
            pool[capacity].next = freeHead;
            registrar->namePool.freeHead = (int32_t)capacity;
            pool[capacity].name = NULL;
            pool[capacity].children.objects.bucketCount = 0;
            pool = registrar->namePool.records;
            pool[capacity].children.objects.buckets = NULL;
            registrar->namePool.records[capacity].children.objects.count = 0;
        }
    }
    records = registrar->namePool.records;
    int32_t freeIndex = registrar->namePool.freeHead;
    if (freeIndex != -1) {
        int32_t previous = records[(uint32_t)freeIndex].previous;
        int32_t next = records[(uint32_t)freeIndex].next;
        registrar->namePool.freeHead = next;
        if (previous != -1) {
            records[(uint32_t)previous].next = next;
        }
        if (next != -1) {
            records[(uint32_t)next].previous = previous;
        }
    }
    registrar->namePool.used = registrar->namePool.used + 1;
    *index = freeIndex;
    NameRecord *record = &records[(uint32_t)freeIndex];
    _CFXNotificationRegistrarAddCachedName(name, &record->name);
    if (observerTables) {
        record->children.observers.head = -1;
        record->children.observers.count = 0;
    } else {
        record->children.objects.buckets = NULL;
        record->children.objects.bucketCount = 0;
        record->children.objects.count = 0;
    }
    int32_t added = *index;
    int32_t *buckets = _CFXNotificationNameTableRehash(registrar, table);
    uint64_t bucketCount = table->bucketCount;
    record->hash = hash;
    if (added != -1 && buckets != NULL) {
        uint64_t slot = hash % bucketCount;
        int32_t head = buckets[slot];
        if (head != -1) {
            registrar->namePool.records[(uint32_t)head].previous = added;
        }
        NameRecord *linked = &registrar->namePool.records[(uint32_t)added];
        linked->previous = -1;
        linked->next = head;
        buckets[slot] = added;
    }
    table->count = table->count + 1;
    return record;
}

// Native CFXNotificationRegisteredObserverSetNotifyToken, 0x1c02bda00. External. For a token whose index is
// below the observer pool's capacity and whose generation matches, flag 0x4000 is set and the notify token
// stored in the context word's low 32 bits.
void OCFXNotificationRegisteredObserverSetNotifyToken(
    OCFXNotificationRegistrarRef registrar, uint64_t token, int notifyToken
) {
    os_unfair_lock_lock(&registrar->lock);
    uint32_t index = (uint32_t)token;
    if (registrar->observerPool.capacity > index) {
        ObserverRecord *record = &registrar->observerPool.records[index];
        if (record->generation == (uint32_t)(token >> 32)) {
            record->flags |= kObserverNotifyToken;
            *(uint32_t *)&record->context = (uint32_t)notifyToken;
        }
    }
    os_unfair_lock_unlock(&registrar->lock);
}

// Native CFXNotificationRegisteredObserverSetDistributedConnection, 0x1c02bda70. External. Under the lock,
// for an index below the observer pool's capacity.
void OCFXNotificationRegisteredObserverSetDistributedConnection(
    OCFXNotificationRegistrarRef registrar, uint64_t token, xpc_object_t connection
) {
    os_unfair_lock_lock(&registrar->lock);
    if (registrar->observerPool.capacity > (uint32_t)token) {
        _CFXNotificationRegisteredObserverSetDistributedConnection(
            registrar, (uint32_t)(token >> 32), (uint32_t)token, connection
        );
    }
    os_unfair_lock_unlock(&registrar->lock);
}

// Native __CFXNotificationRegisteredObserverSetDistributedConnection, 0x1c02bdac8. Non-external. When the
// generation matches, flag 0x2000 is set and a different connection replaces the context word (the old one
// released, the new one retained).
static void _CFXNotificationRegisteredObserverSetDistributedConnection(
    OCFXNotificationRegistrarRef registrar, uint32_t generation, uint32_t index, xpc_object_t connection
) {
    ObserverRecord *record = &registrar->observerPool.records[index];
    if (record->generation != generation) {
        return;
    }
    record->flags |= kObserverDistributedConnection;
    xpc_object_t current = (xpc_object_t)record->context;
    if (current != connection) {
        if (current != NULL) {
            xpc_release(current);
        }
        record->context = (uint64_t)xpc_retain(connection);
    }
}

// Native CFXNotificationRegistrarEnumerate, 0x1c02bdb28. External. Under the lock: the name table (object
// tables), the any-name object table, the any-object name table (observer lists) and the any-name any-object
// list.
void OCFXNotificationRegistrarEnumerate(
    OCFXNotificationRegistrarRef registrar, void (^block)(const NotificationRegistration *registration)
) {
    os_unfair_lock_lock(&registrar->lock);
    _CFXNotificationRegistrarEnumerateNameTable(registrar, &registrar->names, false, block);
    OCFStringRef anyName = kOCFNotificationAnyName;
    _CFXNotificationRegistrarEnumerateObjectTable(registrar, &registrar->objects, anyName, block);
    _CFXNotificationRegistrarEnumerateNameTable(registrar, &registrar->anyObjectNames, true, block);
    _CFXNotificationRegistrarEnumerateObserverTable(
        registrar, &registrar->anyObservers, anyName, kOCFNotificationAnyObject, block
    );
    os_unfair_lock_unlock(&registrar->lock);
}

// Native __CFXNotificationRegistrarEnumerateNameTable, 0x1c02bdbc4. Non-external. Every chain of every
// bucket; each name is copied from the pool for its children and released after them.
static void _CFXNotificationRegistrarEnumerateNameTable(
    OCFXNotificationRegistrarRef registrar,
    RegistrarTable *table,
    Boolean observerTables,
    void (^block)(const NotificationRegistration *registration)
) {
    if (table->bucketCount == 0) {
        return;
    }
    OCFTypeRef anyObject = kOCFNotificationAnyObject;
    for (uint64_t bucket = 0; bucket < table->bucketCount; bucket++) {
        uint32_t index = (uint32_t)table->buckets[bucket];
        while (index != 0xffffffff) {
            NameRecord *record = &registrar->namePool.records[index];
            index = (uint32_t)record->next;
            OCFStringRef name = CFCachedStringCopyString(&record->name);
            if (observerTables) {
                _CFXNotificationRegistrarEnumerateObserverTable(
                    registrar, &record->children.observers, name, anyObject, block
                );
            } else {
                _CFXNotificationRegistrarEnumerateObjectTable(
                    registrar, &record->children.objects, name, block
                );
            }
            OCFRelease(name);
        }
    }
}

// Native __CFXNotificationRegistrarEnumerateObjectTable, 0x1c02bdca8. Non-external.
static void _CFXNotificationRegistrarEnumerateObjectTable(
    OCFXNotificationRegistrarRef registrar,
    RegistrarTable *table,
    OCFStringRef name,
    void (^block)(const NotificationRegistration *registration)
) {
    if (table->bucketCount == 0) {
        return;
    }
    for (uint64_t bucket = 0; bucket < table->bucketCount; bucket++) {
        uint32_t index = (uint32_t)table->buckets[bucket];
        while (index != 0xffffffff) {
            ObjectRecord *record = &registrar->objectPool.records[index];
            index = (uint32_t)record->next;
            _CFXNotificationRegistrarEnumerateObserverTable(
                registrar, &record->observers, name, record->object, block
            );
        }
    }
}

// Native __CFXNotificationRegistrarEnumerateObserverTable, 0x1c02bdd38. Non-external. Each observer is passed
// as a zeroed stack registration filled with the name, the object, the stored observer pointer (not loaded
// for weak observers), the flags, the handler (not copied), the token and the context word.
static void _CFXNotificationRegistrarEnumerateObserverTable(
    OCFXNotificationRegistrarRef registrar,
    ObserverList *list,
    OCFStringRef name,
    OCFTypeRef object,
    void (^block)(const NotificationRegistration *registration)
) {
    uint32_t index = (uint32_t)list->head;
    while (index != 0xffffffff) {
        ObserverRecord *record = &registrar->observerPool.records[index];
        uint32_t next = (uint32_t)record->next;
        NotificationRegistration entry = {0};
        entry.observer = record->observer;
        entry.handler = record->handler;
        entry.name = name;
        entry.object = object;
        entry.flags = record->flags;
        entry.token = (uint64_t)index | ((uint64_t)record->generation << 32);
        entry.context = record->context;
        block(&entry);
        index = next;
    }
}
#endif

// Native CFXNotificationRegistrarIsEmpty, 0x1c02bdf5c. External. Under the lock: no name, any-name object,
// any-object name or any observer registration.
Boolean OCFXNotificationRegistrarIsEmpty(OCFXNotificationRegistrarRef registrar) {
    os_unfair_lock_lock(&registrar->lock);
    Boolean empty = registrar->objects.count + registrar->names.count + registrar->anyObjectNames.count +
                        registrar->anyObservers.count ==
                    0;
    os_unfair_lock_unlock(&registrar->lock);
    return empty;
}

// Native CFXNotificationRegistrarValidateToken, 0x1c02bdfac. External. Under the lock: true when the index is
// below the observer pool's capacity, the generation matches and the record is not invalid.
Boolean OCFXNotificationRegistrarValidateToken(OCFXNotificationRegistrarRef registrar, uint64_t token) {
    os_unfair_lock_lock(&registrar->lock);
    Boolean valid = false;
    if (registrar->observerPool.capacity > (uint32_t)token) {
        ObserverRecord *record = &registrar->observerPool.records[(uint32_t)token];
        if (record->generation == (uint32_t)(token >> 32)) {
            valid = (record->flags & kObserverInvalid) == 0;
        }
    }
    os_unfair_lock_unlock(&registrar->lock);
    return valid;
}

// Native ___CFXNotificationDisposalListAddItem, 0x1c02be01c. Non-external. The first four items go inline;
// later ones go to the overflow array, which grows to exactly the number of overflow items (malloc type
// 0x80040B8603338 on first use, realloc type 0xC5716605 after).
// CoreFoundation defect (fixed): the native realloc passes the item count as the byte size (MOV X1, X21), so
// from the second overflow item on the store overruns the reallocated block; confidence: verified. The
// restoration passes count * 8 bytes. Divergence: the reallocated size only.
static void __CFXNotificationDisposalListAddItem(DisposalList *list, OCFTypeRef item) {
    uint32_t count = list->count;
    if (count <= 3) {
        list->items[count] = item;
    } else {
        uint32_t needed = count - 3;
        OCFTypeRef *overflow = list->overflow;
        if (list->capacity < needed) {
            if (overflow != NULL) {
                overflow = malloc_type_realloc(overflow, (size_t)needed * sizeof(OCFTypeRef), 0xC5716605);
            } else {
                overflow = malloc_type_malloc((size_t)needed << 3, 0x80040B8603338);
            }
            list->overflow = overflow;
            list->capacity = needed;
            count = list->count;
        }
        overflow[count - 4] = item;
    }
    list->count = count + 1;
}

// Native __CFXNotificationRegistrarRemoveCachedName, 0x1c02be0b8. Non-external. Under the name-pool lock, the
// name's use count is decremented (the entry is removed at zero) and the reference is cleared.
static void _CFXNotificationRegistrarRemoveCachedName(OCFStringRef *cached) {
    os_unfair_lock_lock(&__CFXNotificationRegistrarNamePoolLock);
    OCFMutableDictionaryRef pool = _CFXNotificationRegistrarGetNamePool();
    uintptr_t uses = (uintptr_t)OCFDictionaryGetValue(pool, *cached);
    if (uses - 1 == 0) {
        OCFDictionaryRemoveValue(pool, *cached);
    } else {
        OCFDictionarySetValue(pool, *cached, (const void *)(uses - 1));
    }
    *cached = NULL;
    os_unfair_lock_unlock(&__CFXNotificationRegistrarNamePoolLock);
}

// Native __CFXNotificationRegistrarGetNamePool, 0x1c02be11c. Non-external. Asserts the name-pool lock and
// creates the pool on first use: a mutable dictionary of the system default allocator with the copy-string
// key callbacks and no value callbacks.
static OCFMutableDictionaryRef _CFXNotificationRegistrarGetNamePool(void) {
    os_unfair_lock_assert_owner(&__CFXNotificationRegistrarNamePoolLock);
    if (__CFXNotificationRegistrarNamePool == NULL) {
        __CFXNotificationRegistrarNamePool = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, &kOCFCopyStringDictionaryKeyCallBacks, NULL
        );
    }
    return __CFXNotificationRegistrarNamePool;
}

// Native CFCachedStringEqual, 0x1c02be170. Non-external. Under the name-pool lock: false for a cleared
// reference, else CFEqual.
static Boolean CFCachedStringEqual(OCFStringRef *cached, OCFStringRef string) {
    os_unfair_lock_lock(&__CFXNotificationRegistrarNamePoolLock);
    Boolean equal;
    if (*cached != NULL) {
        equal = OCFEqual(*cached, string) != 0;
    } else {
        equal = false;
    }
    os_unfair_lock_unlock(&__CFXNotificationRegistrarNamePoolLock);
    return equal;
}

// Native ___block_literal_global_41 (the barrier of the unregister message), whose invoke is
// ____CFXNotificationRegistrarRecycleObserverRegistration_block_invoke (0x1c02be5e8, non-external): a tail
// call of _CFSuddenTerminationEnable.
#ifdef __BLOCKS__
static void (^const recycleBarrierBlock)(void) = ^{
  _OCFSuddenTerminationEnable();
};
#endif

// Native __CFXNotificationRegistrarRecycleObserverRegistration, 0x1c02be1cc. Non-external.
// With a list and an index other than -1 the record is unlinked (the head moves to the next index). For a
// registrar of option bit 1: a distributed observer (0x2000) with a connection sends {method: "unregister",
// version: 1, tokens: [index | generation << 32]} with sudden termination disabled until the barrier,
// releases the message, the array and the connection and clears the word; a notify observer (0x4000) cancels
// a nonzero token and clears it (32 bits). Then the handler is released onto the disposal list, a weak
// observer is destroyed, the observer word is cleared, the flags become 0x8000 and the list count is
// decremented (the list is dereferenced even when NULL, as natively). An index other than -1 is pushed on the
// free list. The pool's use count is decremented; the result is whether the list is now empty.
static Boolean _CFXNotificationRegistrarRecycleObserverRegistration(
    OCFXNotificationRegistrarRef registrar, ObserverList *list, uint32_t index, DisposalList *disposal
) {
    ObserverRecord *records = registrar->observerPool.records;
    ObserverRecord *record = &records[index];
    if (list != NULL && index != 0xffffffff) {
        int32_t previous = record->previous;
        int32_t next = record->next;
        if ((uint32_t)list->head == index) {
            list->head = next;
        }
        if (previous != -1) {
            records[(uint32_t)previous].next = next;
        }
        if (next != -1) {
            records[(uint32_t)next].previous = previous;
        }
    }
    if ((registrar->options & kRegistrarComparesObjects) != 0) {
        uint32_t flags = record->flags;
        if ((flags & kObserverDistributedConnection) != 0) {
            if (record->context != 0) {
                xpc_object_t tokens = xpc_array_create(NULL, 0);
                xpc_array_set_uint64(
                    tokens, XPC_ARRAY_APPEND, (uint64_t)index | ((uint64_t)record->generation << 32)
                );
                xpc_object_t message = xpc_dictionary_create(NULL, NULL, 0);
                xpc_dictionary_set_string(message, "method", "unregister");
                xpc_dictionary_set_uint64(message, "version", 1);
                xpc_dictionary_set_value(message, "tokens", tokens);
                _OCFSuddenTerminationDisable();
                xpc_connection_send_message((xpc_connection_t)record->context, message);
                xpc_connection_send_barrier((xpc_connection_t)record->context, recycleBarrierBlock);
                xpc_release(message);
                xpc_release(tokens);
                xpc_release((xpc_object_t)record->context);
            }
            record->context = 0;
        } else if ((flags & kObserverNotifyToken) != 0) {
            int token = (int)(uint32_t)record->context;
            if (token != 0) {
                notify_cancel(token);
            }
            *(uint32_t *)&record->context = 0;
        }
    }
    _CFXNotificationHandlerRelease(&record->handler, record->flags, disposal);
    if ((record->flags & kObserverWeak) != 0) {
        objc_destroyWeak((id *)&record->observer);
    }
    record->observer = NULL;
    record->flags = kObserverInvalid;
    list->count = list->count - 1;
    if (index != 0xffffffff) {
        ObserverRecord *pool = registrar->observerPool.records;
        int32_t freeHead = registrar->observerPool.freeHead;
        if (freeHead != -1) {
            pool[(uint32_t)freeHead].previous = (int32_t)index;
        }
        pool[index].previous = -1;
        pool[index].next = freeHead;
        registrar->observerPool.freeHead = (int32_t)index;
    }
    uint32_t remaining = list->count;
    registrar->observerPool.used = registrar->observerPool.used - 1;
    return remaining == 0;
}

// Native __CFXNotificationRegistrarRecycleNameRegistration, 0x1c02be3c8. Non-external. With an index other
// than -1 and buckets, the record is unlinked from its bucket (hash % bucketCount). The pooled name is
// removed; unless keepChildren is set the object table's buckets are freed and cleared. The table count is
// decremented, an index other than -1 is pushed on the free list, and the pool's use count is decremented.
static void _CFXNotificationRegistrarRecycleNameRegistration(
    OCFXNotificationRegistrarRef registrar, RegistrarTable *table, int32_t index, Boolean keepChildren
) {
    NameRecord *records = registrar->namePool.records;
    NameRecord *record = &records[(uint32_t)index];
    if (index != -1 && table->buckets != NULL) {
        uint64_t bucket = record->hash % table->bucketCount;
        int32_t previous = record->previous;
        int32_t next = record->next;
        if (table->buckets[bucket] == index) {
            table->buckets[bucket] = next;
        }
        if (previous != -1) {
            records[(uint32_t)previous].next = next;
        }
        if (next != -1) {
            records[(uint32_t)next].previous = previous;
        }
    }
    if (record->name != NULL) {
        _CFXNotificationRegistrarRemoveCachedName(&record->name);
    }
    if (!keepChildren && record->children.objects.buckets != NULL) {
        free(record->children.objects.buckets);
        record->children.objects.buckets = NULL;
    }
    table->count = table->count - 1;
    if (index != -1) {
        NameRecord *pool = registrar->namePool.records;
        int32_t freeHead = registrar->namePool.freeHead;
        if (freeHead != -1) {
            pool[(uint32_t)freeHead].previous = index;
        }
        pool[(uint32_t)index].previous = -1;
        pool[(uint32_t)index].next = freeHead;
        registrar->namePool.freeHead = index;
    }
    registrar->namePool.used = registrar->namePool.used - 1;
}

// Native __CFXNotificationRegistrarRecycleObjectRegistration, 0x1c02be4e0. Non-external. The object's hash is
// CFHash when compares is set (the pool pointer is re-read after it), else bits 5-11 of the pointer. With
// buckets the record is unlinked from its bucket. A pooled object string (compares) is removed; the object
// word is cleared, a nonzero table count is decremented, the index is pushed on the free list and the pool's
// use count is decremented. The result is whether the table is now empty.
static Boolean _CFXNotificationRegistrarRecycleObjectRegistration(
    OCFXNotificationRegistrarRef registrar, RegistrarTable *table, int32_t index, Boolean compares
) {
    uint32_t position = (uint32_t)index;
    ObjectRecord *records = registrar->objectPool.records;
    ObjectRecord *record = &records[position];
    uint64_t hash;
    if (compares) {
        hash = OCFHash(record->object);
        records = registrar->objectPool.records;
    } else {
        hash = pointerHash(record->object);
    }
    if (table->buckets != NULL) {
        uint64_t bucket = hash % table->bucketCount;
        int32_t previous = records[position].previous;
        int32_t next = records[position].next;
        if (table->buckets[bucket] == index) {
            table->buckets[bucket] = next;
        }
        if (previous != -1) {
            records[(uint32_t)previous].next = next;
        }
        if (next != -1) {
            records[(uint32_t)next].previous = previous;
        }
    }
    if (compares) {
        _CFXNotificationRegistrarRemoveCachedName((OCFStringRef *)&record->object);
    }
    record->object = NULL;
    if (table->count != 0) {
        table->count = table->count - 1;
    }
    ObjectRecord *pool = registrar->objectPool.records;
    int32_t freeHead = registrar->objectPool.freeHead;
    if (freeHead != -1) {
        pool[(uint32_t)freeHead].previous = index;
    }
    pool[position].previous = -1;
    pool[position].next = freeHead;
    registrar->objectPool.freeHead = index;
    registrar->objectPool.used = registrar->objectPool.used - 1;
    return table->count == 0;
}

// Native __CFXNotificationRegistrarAddCachedName, 0x1c02be5ec. Non-external. Under the name pool's lock: a
// name already pooled (CFDictionaryGetKeyIfPresent) has its use count incremented and the pooled key stored;
// otherwise the name joins with a count of 1 and the pooled key is looked up again.
static void _CFXNotificationRegistrarAddCachedName(OCFStringRef name, OCFStringRef *cached) {
    const void *key = NULL;
    os_unfair_lock_lock(&__CFXNotificationRegistrarNamePoolLock);
    OCFMutableDictionaryRef pool = _CFXNotificationRegistrarGetNamePool();
    if (OCFDictionaryGetKeyIfPresent(pool, name, &key)) {
        const char *count = OCFDictionaryGetValue(pool, key);
        OCFDictionarySetValue(pool, key, count + 1);
    } else {
        OCFDictionarySetValue(pool, name, (const void *)1);
        OCFDictionaryGetKeyIfPresent(pool, name, &key);
    }
    *cached = (OCFStringRef)key;
    os_unfair_lock_unlock(&__CFXNotificationRegistrarNamePoolLock);
}

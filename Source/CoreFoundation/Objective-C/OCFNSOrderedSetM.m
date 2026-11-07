//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSOrderedSetM.h"
#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Objective-C/OCFNSArray.h"
#include "CoreFoundation/Objective-C/OCFNSMutableArray.h"
#include "CoreFoundation/Objective-C/OCFNSOrderedSetPlaceholder.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <alloca.h>
#include <mach-o/loader.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <os/lock.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// Inferred organization: the __NSOrderedSetM unit spans 0x1c02360d0-0x1c0238abf of the iOS Simulator __text
// section: the __NSOrderedSetM methods with __NSOrderedSetM_new (0x1c0237948) after +allocWithZone: and
// ___RELEASE_OBJECTS_IN_THE_ORDEREDSET__ (0x1c0237edc) after -dealloc, the __NSFrozenOrderedSetM methods
// (0x1c0237f28-0x1c02389bb), then the copy-on-write callbacks cow_copy_instance, cow_copy_storage and
// cow_set_cow (0x1c02389bc, 0x1c0238a20, 0x1c0238a84, listed with an _0 suffix: the ordered set unit's own
// copies; the array, set and dictionary units have theirs), the CFBasicHash callbacks
// __NSOrderedSetEquateKeys (0x1c0238a98) and __NSOrderedSetHashKey (0x1c0238ab0), and OUTLINED_FUNCTION_0_30
// (0x1c0238ab4: __cow_failure with "Attempting to mutate immutable collection!"; folded into its callers).
// The methods' .cold.1 pieces lie in the cold region (0x1c030cda4-0x1c030cdf3) and are folded into their
// methods. Following the owner's decision, the class has its own translation unit, which keeps the native
// order (the C functions, which reach the instance variables, are defined inside the @implementation; the
// __NSFrozenOrderedSetM methods are a second @implementation at their native position, and the C functions
// after them follow it outside both, reaching the public instance variables). The
// unit's statics: ___NSOrderedSetMCB (0x2227a5c50) and ___NSOrderedSet_cowCallbacks (0x2227a5c30), read with
// Local/Scratch/cf-core/macho.py. Objective-C compiled with manual reference counting (objc_retain,
// objc_release). Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta11.json (family
// K-mutable-ordered-set; __NSOrderedSetM_new from iphone-simulator-corpus-delta10.json; the
// __NSFrozenOrderedSetM methods and their .cold.1 from iphone-simulator-corpus-delta12.json, family
// P-collections-url). Unless a definition
// says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified
// (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).
//
// Argument errors raise as the other bridged collection classes do: an os_log pack of the format's buffer
// size built on the stack, the reason made with kCFAllocatorSystemDefault and autoreleased through
// _CFAutoreleasePoolAddObject, +[NSException exceptionWithName:reason:userInfo:osLogPack:size:] with no user
// info thrown. The first format argument is the method's name for the mutation methods
// ("-[__NSOrderedSetM insertObject:atIndex:]", spelled with the class's selected name) and the name of the
// inlined native helper for the others ("_oset_objectAtIndex", "_oset_getObjectsRange",
// "_oset_enumerateObjectsWithOptionsUsingBlock", literal strings natively; the helpers have no symbols of
// their own).

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// libobjc's objc_retain, objc_release and objc_opt_self, which the native image imports (objc_retain_x8,
// objc_release_x8, objc_release_x1 and objc_release_x23 are register variants); the SDK installs no
// declarations for them. Supplied, never reimplemented.
extern id objc_retain(id object);
extern void objc_release(id object);
extern Class objc_opt_self(id object);

// The NSException class symbol (off_2227CE300), sent to directly.
extern char exceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));

// The __NSOrderedSetM class symbol (loaded through the __SCR___NSOrderedSetM GOT slot) and the offsets of its
// ivars cow and storage, defined by this unit's @implementation; __NSOrderedSetM_new reaches them through
// the symbols, as natively.
extern char orderedSetMClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSOrderedSetM));
extern const int32_t orderedSetMCowOffset __asm(OF_SYMBOL("OBJC_IVAR_$_")
                                                    OF_NAME_STRING(__ONSOrderedSetM) ".cow");
extern const int32_t orderedSetMStorageOffset __asm(OF_SYMBOL("OBJC_IVAR_$_")
                                                        OF_NAME_STRING(__ONSOrderedSetM) ".storage");

// The __NSFrozenOrderedSetM class symbol (___SCR___NSFrozenOrderedSetM) and the offsets of its ivars cow and
// storage (_OBJC_IVAR_$___NSFrozenOrderedSetM.cow, .storage), read by -copy, -copyWithZone:,
// __RELEASE_OBJECTS_IN_THE_ORDEREDSET__ and cow_copy_instance, defined by this unit's second
// @implementation.
extern char frozenOrderedSetMClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSFrozenOrderedSetM));
extern const int32_t frozenOrderedSetMCowOffset __asm(OF_SYMBOL("OBJC_IVAR_$_")
                                                          OF_NAME_STRING(__ONSFrozenOrderedSetM) ".cow");
extern const int32_t
    frozenOrderedSetMStorageOffset __asm(OF_SYMBOL("OBJC_IVAR_$_")
                                             OF_NAME_STRING(__ONSFrozenOrderedSetM) ".storage");

// The messages this unit sends that the shared declarations do not carry; declared for the sends only.
@protocol OrderedSetMMessages

+ (id)exceptionWithName:(id)name
                 reason:(id)reason
               userInfo:(id)userInfo
              osLogPack:(void *)pack
                   size:(size_t)size;
- (NSUInteger)indexOfObjectIdenticalTo:(id)object;

@end

#define methodName(selectorText) "-[" OF_NAME_STRING(__ONSOrderedSetM) " " selectorText "]"

#define RAISE_LOGGED_EXCEPTION(name, bufferSize, format, ...)                                                \
    do {                                                                                                     \
        size_t packSize = _os_log_pack_size(bufferSize);                                                     \
        void *pack = alloca((packSize + 15) & ~(size_t)15);                                                  \
        uint8_t *packBuffer = _os_log_pack_fill(pack, packSize, 0, &__dso_handle, format);                   \
        __builtin_os_log_format(packBuffer, format, __VA_ARGS__);                                            \
        OCFStringRef reason =                                                                                \
            OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR(format), __VA_ARGS__);        \
        objc_exception_throw([(id)exceptionClass                                                             \
            exceptionWithName:(id)(name)                                                                     \
                       reason:_OCFAutoreleasePoolAddObject(NULL, reason)                                     \
                     userInfo:nil                                                                            \
                    osLogPack:pack                                                                           \
                         size:packSize]);                                                                    \
    } while (0)

// The index check failure (NSRangeException): "for empty ordered set" (buffer 0x16) for a count of 0, else
// "[0 .. %lu]" (buffer 0x20) with the count minus one.
#define RAISE_INDEX_EXCEPTION(method, index, count)                                                          \
    do {                                                                                                     \
        NSUInteger boundCount = (count);                                                                     \
        if (boundCount == 0) {                                                                               \
            RAISE_LOGGED_EXCEPTION(                                                                          \
                ONSRangeException,                                                                           \
                0x16,                                                                                        \
                "*** %s: index %lu beyond bounds for empty ordered set",                                     \
                method,                                                                                      \
                (unsigned long)(index)                                                                       \
            );                                                                                               \
        }                                                                                                    \
        RAISE_LOGGED_EXCEPTION(                                                                              \
            ONSRangeException,                                                                               \
            0x20,                                                                                            \
            "*** %s: index %lu beyond bounds [0 .. %lu]",                                                    \
            method,                                                                                          \
            (unsigned long)(index),                                                                          \
            (unsigned long)boundCount - 1                                                                    \
        );                                                                                                   \
    } while (0)

// The ThreadSanitizer hooks the methods inline: load the hook, and when it is set, call it with the object,
// the method's return address and __CFTSANTagMutableOrderedSet. orderedSetMTSANRead and orderedSetMTSANWrite
// are local descriptive names.
static inline void orderedSetMTSANRead(const void *object, void *returnAddress) {
    void (*hook)(const void *, void *, void *) =
        (void (*)(const void *, void *, void *))__cf_tsanReadFunction;
    if (hook != NULL) {
        hook(object, returnAddress, __OCFTSANTagMutableOrderedSet);
    }
}

static inline void orderedSetMTSANWrite(const void *object, void *returnAddress) {
    void (*hook)(const void *, void *, void *) =
        (void (*)(const void *, void *, void *))__cf_tsanWriteFunction;
    if (hook != NULL) {
        hook(object, returnAddress, __OCFTSANTagMutableOrderedSet);
    }
}

static void cow_copy_instance(id source, int kind, id copy, int mode);

static bool cow_copy_storage(id object);

static void cow_set_cow(id object, CopyOnWriteBlock *cow);

static void __RELEASE_OBJECTS_IN_THE_ORDEREDSET__(id object, int kind);

static Boolean __NSOrderedSetEquateKeys(const void *collection, const void *stack);

static OCFHashCode __NSOrderedSetHashKey(const void *stack);

// Native ___NSOrderedSet_cowCallbacks, 0x2227a5c30 (__DATA_CONST,__const). Non-external. The ordered set
// unit's cow_copy_instance, cow_copy_storage and cow_set_cow, and __RELEASE_OBJECTS_IN_THE_ORDEREDSET__ as
// the cleanup slot (read with Local/Scratch/cf-core/macho.py).
static const CopyOnWriteCallbacks __NSOrderedSet_cowCallbacks = {
    cow_copy_instance,
    cow_copy_storage,
    cow_set_cow,
    __RELEASE_OBJECTS_IN_THE_ORDEREDSET__,
};

// Native __NSOrderedSetMCB, 0x2227a5c50 (__DATA_CONST,__const, non-external): the ten CFBasicHash callbacks
// of the mutable ordered set's hash, all NULL except equateKeys (__NSOrderedSetEquateKeys, 0x1c0238a98) and
// hashKey (__NSOrderedSetHashKey, 0x1c0238ab0) (read with Local/Scratch/cf-core/macho.py; confidence:
// verified).
static const BasicHashCallbacks __NSOrderedSetMCB = {
    .retainValue = NULL,
    .retainKey = NULL,
    .releaseValue = NULL,
    .releaseKey = NULL,
    .equateValues = NULL,
    .equateKeys = __NSOrderedSetEquateKeys,
    .hashKey = __NSOrderedSetHashKey,
    .getIndirectKey = NULL,
    .copyValueDescription = NULL,
    .copyKeyDescription = NULL,
};

// The copy-on-write prologue the mutating methods inline: the control block is loaded with acquire; under
// its lock a block without a mutable reference crashes (the methods' .cold.1: OUTLINED_FUNCTION_0_30,
// __cow_failure with "Attempting to mutate immutable collection!"); unless this ordered set is the block's
// only reference, its mutable count drops and __cow_mutate_slow gives the ordered set its own storage. There
// is no mutation counter. mutateOrderedSetStorage is a local descriptive name.
static inline void mutateOrderedSetStorage(__ONSOrderedSetM *orderedSet) {
    CopyOnWriteBlock *block = __atomic_load_n(&orderedSet->cow, __ATOMIC_ACQUIRE);
    if (block == NULL) {
        return;
    }
    os_unfair_lock_lock(&block->lock);
    uint16_t mutableReferences = block->mutableReferences;
    if (mutableReferences == 0) {
        _cow_failure(OF_STR("Attempting to mutate immutable collection!"));
    }
    uint16_t remaining = mutableReferences - 1;
    if (remaining != 0 || block->immutableReferences != 0) {
        block->mutableReferences = remaining;
        _cow_mutate_slow((id)orderedSet, block, &__NSOrderedSet_cowCallbacks);
    }
    os_unfair_lock_unlock(&block->lock);
}

// The control block a copy shares: a receiver without one gets one (__cow_create mode 1, installed with a
// compare-and-swap from NULL, acquire-release; the loser's block is freed). ensureCopyOnWriteBlock is a
// local descriptive name.
static inline void ensureCopyOnWriteBlock(__ONSOrderedSetM *orderedSet) {
    if (__atomic_load_n(&orderedSet->cow, __ATOMIC_ACQUIRE) == NULL) {
        CopyOnWriteBlock *created = _cow_create((id)orderedSet, 1);
        CopyOnWriteBlock *expected = NULL;
        if (!__atomic_compare_exchange_n(
                &orderedSet->cow, &expected, created, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            )) {
            free(created);
        }
    }
}

@implementation __ONSOrderedSetM

// Native -[__NSOrderedSetM count], 0x1c02360d0. Non-external. CFBasicHashGetCount of the hash (a tail call).
- (NSUInteger)count {
    return (NSUInteger)CFBasicHashGetCount(storage.hash);
}

// Native -[__NSOrderedSetM indexOfObject:], 0x1c02360e0. Non-external. The read hook; nil, or a NULL hash,
// answers NSNotFound. Otherwise CFBasicHashFindBucket of the object: a zero count answers NSNotFound, a match
// the array's -indexOfObjectIdenticalTo: of the bucket's value (the stored object).
- (NSUInteger)indexOfObject:(id)object {
    orderedSetMTSANRead(self, __builtin_return_address(0));
    if (object == nil || storage.hash == NULL) {
        return kNotFound;
    }
    BasicHashBucket bucket = CFBasicHashFindBucket(storage.hash, (uintptr_t)object);
    if (bucket.count == 0) {
        return kNotFound;
    }
    return [storage.array indexOfObjectIdenticalTo:(id)bucket.value];
}

// Native -[__NSOrderedSetM objectAtIndex:], 0x1c02361b0. Non-external. The read hook; an index below the
// hash's count (unsigned) answers the array's -objectAtIndex: (a tail call); otherwise the index raise with
// "_oset_objectAtIndex" as the name.
- (id)objectAtIndex:(NSUInteger)index {
    orderedSetMTSANRead(self, __builtin_return_address(0));
    NSUInteger count = (NSUInteger)CFBasicHashGetCount(storage.hash);
    if (index >= count) {
        RAISE_INDEX_EXCEPTION("_oset_objectAtIndex", index, count);
    }
    return [storage.array objectAtIndex:index];
}

// Native -[__NSOrderedSetM insertObject:atIndex:], 0x1c02363c0, with .cold.1 (0x1c030cda4). Non-external.
// Order: the write hook, the copy-on-write prologue, a nil object ("object cannot be nil", buffer 0xC), the
// index check against the hash's count (bit 63 or above the count). The object is added to the hash as its
// own key and value; when CFBasicHashAddValue accepts it (it was absent), the array's -insertObject:atIndex:
// follows (a tail call); otherwise nothing changes.
- (void)insertObject:(id)object atIndex:(NSUInteger)index {
    orderedSetMTSANWrite(self, __builtin_return_address(0));
    mutateOrderedSetStorage(self);
    if (object == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: object cannot be nil",
            methodName("insertObject:atIndex:")
        );
    }
    NSUInteger count = (NSUInteger)CFBasicHashGetCount(storage.hash);
    if ((NSInteger)index < 0 || count < index) {
        RAISE_INDEX_EXCEPTION(methodName("insertObject:atIndex:"), index, count);
    }
    if (CFBasicHashAddValue(storage.hash, (uintptr_t)object, (uintptr_t)object)) {
        [storage.array insertObject:object atIndex:index];
    }
}

// Native -[__NSOrderedSetM removeObjectAtIndex:], 0x1c0236764, with .cold.1 (0x1c030cdb0). Non-external. The
// write hook, the copy-on-write prologue, the index check (bit 63 or at/above the hash's count); the array's
// object at the index is removed from the hash (CFBasicHashRemoveValue), then the array's
// -removeObjectAtIndex: (a tail call).
- (void)removeObjectAtIndex:(NSUInteger)index {
    orderedSetMTSANWrite(self, __builtin_return_address(0));
    mutateOrderedSetStorage(self);
    NSUInteger count = (NSUInteger)CFBasicHashGetCount(storage.hash);
    if ((NSInteger)index < 0 || count <= index) {
        RAISE_INDEX_EXCEPTION(methodName("removeObjectAtIndex:"), index, count);
    }
    id object = [storage.array objectAtIndex:index];
    CFBasicHashRemoveValue(storage.hash, (uintptr_t)object);
    [storage.array removeObjectAtIndex:index];
}

// Native -[__NSOrderedSetM removeAllObjects], 0x1c02369e8, with .cold.1 (0x1c030cdbc). Non-external. The
// write hook, the copy-on-write prologue, the array's -removeAllObjects, then CFBasicHashRemoveAllValues (a
// tail call).
- (void)removeAllObjects {
    orderedSetMTSANWrite(self, __builtin_return_address(0));
    mutateOrderedSetStorage(self);
    [storage.array removeAllObjects];
    CFBasicHashRemoveAllValues(storage.hash);
}

// The replacement -replaceObjectAtIndex:withObject: and -setObject:atIndex: inline once the index is checked:
// the array's object at the index is read; when it is the object itself, or does not answer -isEqual: YES
// (bit 0; sent to the stored object) to it, CFBasicHashFindBucket of the new object runs and a nonzero count
// (the object is already held, at this index or another) leaves everything unchanged. Otherwise the old
// object is removed from the hash, the new one added, and the array's -replaceObjectAtIndex:withObject: sent.
// replaceOrderedSetObject is a local descriptive name.
static inline void replaceOrderedSetObject(__ONSOrderedSetM *orderedSet, NSUInteger index, id object) {
    id existing = [orderedSet->storage.array objectAtIndex:index];
    if (existing == object || !([existing isEqual:object] & 1)) {
        BasicHashBucket bucket = CFBasicHashFindBucket(orderedSet->storage.hash, (uintptr_t)object);
        if (bucket.count != 0) {
            return;
        }
    }
    CFBasicHashRemoveValue(orderedSet->storage.hash, (uintptr_t)existing);
    CFBasicHashAddValue(orderedSet->storage.hash, (uintptr_t)object, (uintptr_t)object);
    [orderedSet->storage.array replaceObjectAtIndex:index withObject:object];
}

// Native -[__NSOrderedSetM replaceObjectAtIndex:withObject:], 0x1c0236aa0, with .cold.1 (0x1c030cdc8).
// Non-external. The write hook, the copy-on-write prologue, a nil object ("object cannot be nil", buffer
// 0xC), the index check (bit 63 or at/above the hash's count), then the replacement.
- (void)replaceObjectAtIndex:(NSUInteger)index withObject:(id)object {
    orderedSetMTSANWrite(self, __builtin_return_address(0));
    mutateOrderedSetStorage(self);
    if (object == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: object cannot be nil",
            methodName("replaceObjectAtIndex:withObject:")
        );
    }
    NSUInteger count = (NSUInteger)CFBasicHashGetCount(storage.hash);
    if ((NSInteger)index < 0 || count <= index) {
        RAISE_INDEX_EXCEPTION(methodName("replaceObjectAtIndex:withObject:"), index, count);
    }
    replaceOrderedSetObject(self, index, object);
}

// Native -[__NSOrderedSetM _mutate], 0x1c0236e60. Non-external. No copy-on-write prologue: the array is sent
// -_mutate, then the write hook is called with the array (not the receiver), the method's return address and
// the tag (a tail call).
- (void)_mutate {
    void *returnAddress = __builtin_return_address(0);
    id array = storage.array;
    [array _mutate];
    orderedSetMTSANWrite(array, returnAddress);
}

// Native +[__NSOrderedSetM automaticallyNotifiesObserversForKey:], 0x1c0236ec4. Non-external. Answers NO.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSOrderedSetM containsObject:], 0x1c0236ecc. Non-external. The read hook; nil or a NULL hash
// answers NO; otherwise whether CFBasicHashFindBucket's count is nonzero.
- (BOOL)containsObject:(id)object {
    orderedSetMTSANRead(self, __builtin_return_address(0));
    if (object == nil || storage.hash == NULL) {
        return NO;
    }
    BasicHashBucket bucket = CFBasicHashFindBucket(storage.hash, (uintptr_t)object);
    return bucket.count != 0;
}

// Native -[__NSOrderedSetM countForObject:], 0x1c0236f88. Non-external. The body of -containsObject:,
// answering 1 or 0.
- (NSUInteger)countForObject:(id)object {
    orderedSetMTSANRead(self, __builtin_return_address(0));
    if (object == nil || storage.hash == NULL) {
        return 0;
    }
    BasicHashBucket bucket = CFBasicHashFindBucket(storage.hash, (uintptr_t)object);
    return bucket.count != 0;
}

// Native -[__NSOrderedSetM enumerateObjectsWithOptions:usingBlock:], 0x1c0237044. Non-external. The read
// hook; a nil block raises ("*** %s: block cannot be nil", buffer 0xC, with
// "_oset_enumerateObjectsWithOptionsUsingBlock" as the name); otherwise the array's
// -enumerateObjectsWithOptions:usingBlock: with the same arguments (a tail call).
- (void)enumerateObjectsWithOptions:(NSUInteger)options
                         usingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block {
    orderedSetMTSANRead(self, __builtin_return_address(0));
    if (block == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: block cannot be nil",
            "_oset_enumerateObjectsWithOptionsUsingBlock"
        );
    }
    [storage.array enumerateObjectsWithOptions:options usingBlock:block];
}

// Native -[__NSOrderedSetM getObjects:range:], 0x1c02371a8. Non-external. The read hook; the range check
// against the hash's count (location or length with bit 63 set, or past the count; "for empty ordered set",
// buffer 0x20, or "[0 .. %lu]", buffer 0x2A), then the buffer checks (a NULL buffer with a nonzero length, a
// length of 2^61 or more; buffer 0x16 each), all with "_oset_getObjectsRange" as the name; then the array's
// -getObjects:range: (a tail call).
- (void)getObjects:(id *)objects range:(OCFRange)range {
    orderedSetMTSANRead(self, __builtin_return_address(0));
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    NSUInteger count = (NSUInteger)CFBasicHashGetCount(storage.hash);
    if ((NSInteger)location < 0 || (NSInteger)length < 0 || count < location + length) {
        if (count == 0) {
            RAISE_LOGGED_EXCEPTION(
                ONSRangeException,
                0x20,
                "*** %s: range {%lu, %lu} extends beyond bounds for empty ordered set",
                "_oset_getObjectsRange",
                (unsigned long)location,
                (unsigned long)length
            );
        }
        RAISE_LOGGED_EXCEPTION(
            ONSRangeException,
            0x2A,
            "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",
            "_oset_getObjectsRange",
            (unsigned long)location,
            (unsigned long)length,
            (unsigned long)count - 1
        );
    }
    if (objects == NULL && length != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "_oset_getObjectsRange",
            (unsigned long)length
        );
    }
    if ((OF_WORD_SHIFT(length, 61)) != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "_oset_getObjectsRange",
            (unsigned long)length
        );
    }
    [storage.array getObjects:objects range:range];
}

// Native -[__NSOrderedSetM setObject:atIndex:], 0x1c023752c, with .cold.1 (0x1c030cdd4). Non-external. The
// write hook, the copy-on-write prologue, a nil object ("object cannot be nil", buffer 0xC), the index check
// (bit 63 or above the hash's count). At the count the object is added to the hash and, when accepted,
// inserted at the end of the array (-insertObject:atIndex:, a tail call); below it the replacement of
// -replaceObjectAtIndex:withObject:.
- (void)setObject:(id)object atIndex:(NSUInteger)index {
    orderedSetMTSANWrite(self, __builtin_return_address(0));
    mutateOrderedSetStorage(self);
    if (object == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException, 0xC, "*** %s: object cannot be nil", methodName("setObject:atIndex:")
        );
    }
    NSUInteger count = (NSUInteger)CFBasicHashGetCount(storage.hash);
    if ((NSInteger)index < 0 || count < index) {
        RAISE_INDEX_EXCEPTION(methodName("setObject:atIndex:"), index, count);
    }
    if (index == count) {
        if (CFBasicHashAddValue(storage.hash, (uintptr_t)object, (uintptr_t)object)) {
            [storage.array insertObject:object atIndex:index];
        }
        return;
    }
    replaceOrderedSetObject(self, index, object);
}

// Native +[__NSOrderedSetM allocWithZone:], 0x1c0237944. Non-external. The mutable placeholder (a tail call).
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSOrderedSetMutablePlaceholder();
}

// Native __NSOrderedSetM_new, 0x1c0237948. Hidden (was a private external). A CFBasicHash is created with
// kCFAllocatorSystemDefault, flags 0x7000 and __NSOrderedSetMCB (NULL answers nil), then a
// _CFCreateArrayStorage(count, not zeroed) buffer (NULL releases the hash and answers nil). With objects and
// a non-zero count, each object is added to the hash as key and value; an object CFBasicHashAddValue accepts
// (not already present) is retained (objc_retain, no tagged-pointer test) unless bit 0 of noRetain is set and
// appended to the buffer (read again from objects). The buffer is adopted by a new NSMutableArray
// (objc_alloc,
// -_initByAdoptingBuffer:count:size: with the kept count and the storage size). A nil array releases the kept
// objects from the last to the first (when retained and the count is at least one), frees the buffer,
// releases the hash and answers nil. Otherwise the object is __CFAllocateObject of +[__NSOrderedSetM self]
// with no extra bytes; nil releases the hash and the array and answers nil; else the storage takes the hash
// and the array (one pair store) and cow is cleared with a release store.
id __NSOrderedSetM_new(const id *objects, NSUInteger count, BOOL noRetain) {
    BasicHashRef hash = CFBasicHashCreate(kOCFAllocatorSystemDefault, 0x7000, &__NSOrderedSetMCB);
    if (hash == NULL) {
        return nil;
    }
    size_t size = 0;
    id *buffer = (id *)_OCFCreateArrayStorage(count, false, &size);
    if (buffer == NULL) {
        OCFRelease(hash);
        return nil;
    }
    NSUInteger kept = 0;
    if (objects != NULL && count != 0) {
        do {
            if (CFBasicHashAddValue(hash, (uintptr_t)*objects, (uintptr_t)*objects)) {
                if (!(noRetain & 1)) {
                    objc_retain(*objects);
                }
                buffer[kept++] = *objects;
            }
            objects++;
        } while (--count != 0);
    }
    id array = [[ONSMutableArray alloc] _initByAdoptingBuffer:buffer count:kept size:size];
    if (array == nil) {
        if (!(noRetain & 1) && (NSInteger)kept >= 1) {
            do {
                objc_release(buffer[--kept]);
            } while (kept != 0);
        }
        free(buffer);
        OCFRelease(hash);
        return nil;
    }
    id set = __CFAllocateObject([(Class)orderedSetMClass self], 0);
    if (set == nil) {
        OCFRelease(hash);
        objc_release(array);
        return nil;
    }
    void **storage = (void **)((uint8_t *)set + orderedSetMStorageOffset);
    storage[0] = hash;
    storage[1] = array;
    __atomic_store_n((uintptr_t *)((uint8_t *)set + orderedSetMCowOffset), 0, __ATOMIC_RELEASE);
    return set;
}

// Native -[__NSOrderedSetM copy], 0x1c0237adc. Non-external. The read hook; the receiver's control block is
// ensured; the copy is __CFAllocateObject(__NSFrozenOrderedSetM, 0) (objc_opt_self of its class reference),
// sharing the storage through __cow_copy (kind 1, mode 0). No small-count path.
- (id)copy {
    orderedSetMTSANRead(self, __builtin_return_address(0));
    ensureCopyOnWriteBlock(self);
    id copy = __CFAllocateObject(objc_opt_self((id)frozenOrderedSetMClass), 0);
    _cow_copy(self, 1, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSOrderedSet_cowCallbacks, copy, 0);
    return copy;
}

// Native -[__NSOrderedSetM copyWithZone:], 0x1c0237ba4. Non-external. The body of -copy; the zone is not
// read.
- (id)copyWithZone:(struct _NSZone *)zone {
    orderedSetMTSANRead(self, __builtin_return_address(0));
    ensureCopyOnWriteBlock(self);
    id copy = __CFAllocateObject(objc_opt_self((id)frozenOrderedSetMClass), 0);
    _cow_copy(self, 1, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSOrderedSet_cowCallbacks, copy, 0);
    return copy;
}

// Native -[__NSOrderedSetM mutableCopy], 0x1c0237c6c. Non-external. As -copy, with the copy
// __CFAllocateObject(__NSOrderedSetM, 0) (objc_opt_self of ___SCR___NSOrderedSetM) and __cow_copy mode 1.
- (id)mutableCopy {
    orderedSetMTSANRead(self, __builtin_return_address(0));
    ensureCopyOnWriteBlock(self);
    id copy = __CFAllocateObject(objc_opt_self((id)orderedSetMClass), 0);
    _cow_copy(self, 1, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSOrderedSet_cowCallbacks, copy, 1);
    return copy;
}

// Native -[__NSOrderedSetM mutableCopyWithZone:], 0x1c0237d34. Non-external. The body of -mutableCopy; the
// zone is not read.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    orderedSetMTSANRead(self, __builtin_return_address(0));
    ensureCopyOnWriteBlock(self);
    id copy = __CFAllocateObject(objc_opt_self((id)orderedSetMClass), 0);
    _cow_copy(self, 1, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSOrderedSet_cowCallbacks, copy, 1);
    return copy;
}

// Native -[__NSOrderedSetM dealloc], 0x1c0237dfc, with its .cold.1 (0x1c030cde0). Non-external. With a
// control block: under its lock a block without a mutable reference crashes (__cow_failure with "Attempting
// to over-release (mutable)"); the mutable count drops, and when no reference remains (after the unlock) the
// storage is released and the block freed. Without one the storage is released. Then super.
- (void)dealloc {
    CopyOnWriteBlock *block = __atomic_load_n(&cow, __ATOMIC_ACQUIRE);
    if (block != NULL) {
        os_unfair_lock_lock(&block->lock);
        uint16_t mutableReferences = block->mutableReferences;
        if (mutableReferences == 0) {
            _cow_failure(OF_STR("Attempting to over-release (mutable)"));
        }
        uint16_t immutableReferences = block->immutableReferences;
        mutableReferences -= 1;
        block->mutableReferences = mutableReferences;
        uint16_t remaining = mutableReferences | immutableReferences;
        os_unfair_lock_unlock(&block->lock);
        if (remaining == 0) {
            __RELEASE_OBJECTS_IN_THE_ORDEREDSET__(self, 1);
            free(block);
        }
    } else {
        __RELEASE_OBJECTS_IN_THE_ORDEREDSET__(self, 1);
    }
    [super dealloc];
}

// Native ___RELEASE_OBJECTS_IN_THE_ORDEREDSET__, 0x1c0237edc. Non-external; the cleanup slot of
// ___NSOrderedSet_cowCallbacks. Kind 1 is a __NSOrderedSetM, any other a __NSFrozenOrderedSetM (its storage
// ivar offset). The hash is CFReleased when not NULL and the array released (objc_release, a tail call, also
// for a NULL hash); the storage is not cleared.
static void __RELEASE_OBJECTS_IN_THE_ORDEREDSET__(id object, int kind) {
    int32_t offset = kind == 1 ? orderedSetMStorageOffset : frozenOrderedSetMStorageOffset;
    OrderedSetMStorage *orderedSetStorage = (OrderedSetMStorage *)((char *)object + offset);
    BasicHashRef hash = orderedSetStorage->hash;
    id array = orderedSetStorage->array;
    if (hash != NULL) {
        OCFRelease(hash);
    }
    objc_release(array);
}

@end

// Native position: the __NSFrozenOrderedSetM methods lie between ___RELEASE_OBJECTS_IN_THE_ORDEREDSET__ and
// cow_copy_instance. They read the storage through the class's own ivar offsets and, unlike the
// __NSOrderedSetM methods, call no ThreadSanitizer hook except in -mutableCopy and -mutableCopyWithZone:.
@implementation __ONSFrozenOrderedSetM

// Native +[__NSFrozenOrderedSetM automaticallyNotifiesObserversForKey:], 0x1c0237f28. Non-external. Answers
// NO.
+ (BOOL)automaticallyNotifiesObserversForKey:(id)key {
    return NO;
}

// Native -[__NSFrozenOrderedSetM count], 0x1c0237f30. Non-external. CFBasicHashGetCount of the hash (a tail
// call).
- (NSUInteger)count {
    return (NSUInteger)CFBasicHashGetCount(storage.hash);
}

// Native -[__NSFrozenOrderedSetM indexOfObject:], 0x1c0237f40. Non-external. nil, or a NULL hash, answers
// NSNotFound. Otherwise CFBasicHashFindBucket of the object: a zero count answers NSNotFound, a match the
// array's -indexOfObjectIdenticalTo: of the bucket's value.
- (NSUInteger)indexOfObject:(id)object {
    if (object == nil || storage.hash == NULL) {
        return kNotFound;
    }
    BasicHashBucket bucket = CFBasicHashFindBucket(storage.hash, (uintptr_t)object);
    if (bucket.count == 0) {
        return kNotFound;
    }
    return [storage.array indexOfObjectIdenticalTo:(id)bucket.value];
}

// Native -[__NSFrozenOrderedSetM objectAtIndex:], 0x1c0237fe0. Non-external. An index below the hash's
// count (unsigned) answers the array's -objectAtIndex: (a tail call); otherwise the index raise with
// "_oset_objectAtIndex" as the name.
- (id)objectAtIndex:(NSUInteger)index {
    NSUInteger count = (NSUInteger)CFBasicHashGetCount(storage.hash);
    if (index >= count) {
        RAISE_INDEX_EXCEPTION("_oset_objectAtIndex", index, count);
    }
    return [storage.array objectAtIndex:index];
}

// Native -[__NSFrozenOrderedSetM containsObject:], 0x1c02381c0. Non-external. nil or a NULL hash answers
// NO; otherwise whether CFBasicHashFindBucket's count is nonzero.
- (BOOL)containsObject:(id)object {
    if (object == nil || storage.hash == NULL) {
        return NO;
    }
    BasicHashBucket bucket = CFBasicHashFindBucket(storage.hash, (uintptr_t)object);
    return bucket.count != 0;
}

// Native -[__NSFrozenOrderedSetM countForObject:], 0x1c0238244. Non-external. The body of -containsObject:,
// answering 1 or 0.
- (NSUInteger)countForObject:(id)object {
    if (object == nil || storage.hash == NULL) {
        return 0;
    }
    BasicHashBucket bucket = CFBasicHashFindBucket(storage.hash, (uintptr_t)object);
    return bucket.count != 0;
}

// Native -[__NSFrozenOrderedSetM enumerateObjectsWithOptions:usingBlock:], 0x1c02382c8. Non-external. A nil
// block raises ("*** %s: block cannot be nil", buffer 0xC, with "_oset_enumerateObjectsWithOptionsUsingBlock"
// as the name); otherwise the array's -enumerateObjectsWithOptions:usingBlock: with the same arguments (a
// tail call).
- (void)enumerateObjectsWithOptions:(NSUInteger)options
                         usingBlock:(void (^)(id object, NSUInteger index, BOOL *stop))block {
    if (block == nil) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: block cannot be nil",
            "_oset_enumerateObjectsWithOptionsUsingBlock"
        );
    }
    [storage.array enumerateObjectsWithOptions:options usingBlock:block];
}

// Native -[__NSFrozenOrderedSetM getObjects:range:], 0x1c02383ec. Non-external. The checks of
// -[__NSOrderedSetM getObjects:range:] in the same order and with the same buffers, all with
// "_oset_getObjectsRange" as the name, then the array's -getObjects:range: (a tail call).
- (void)getObjects:(id *)objects range:(OCFRange)range {
    NSUInteger location = (NSUInteger)range.location;
    NSUInteger length = (NSUInteger)range.length;
    NSUInteger count = (NSUInteger)CFBasicHashGetCount(storage.hash);
    if ((NSInteger)location < 0 || (NSInteger)length < 0 || count < location + length) {
        if (count == 0) {
            RAISE_LOGGED_EXCEPTION(
                ONSRangeException,
                0x20,
                "*** %s: range {%lu, %lu} extends beyond bounds for empty ordered set",
                "_oset_getObjectsRange",
                (unsigned long)location,
                (unsigned long)length
            );
        }
        RAISE_LOGGED_EXCEPTION(
            ONSRangeException,
            0x2A,
            "*** %s: range {%lu, %lu} extends beyond bounds [0 .. %lu]",
            "_oset_getObjectsRange",
            (unsigned long)location,
            (unsigned long)length,
            (unsigned long)count - 1
        );
    }
    if (objects == NULL && length != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "_oset_getObjectsRange",
            (unsigned long)length
        );
    }
    if ((OF_WORD_SHIFT(length, 61)) != 0) {
        RAISE_LOGGED_EXCEPTION(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "_oset_getObjectsRange",
            (unsigned long)length
        );
    }
    [storage.array getObjects:objects range:range];
}

// Native +[__NSFrozenOrderedSetM allocWithZone:], 0x1c0238740. Non-external. The immutable placeholder (a
// tail call).
+ (id)allocWithZone:(struct _NSZone *)zone {
    return __NSOrderedSetImmutablePlaceholder();
}

// Native -[__NSFrozenOrderedSetM dealloc], 0x1c0238744, with its .cold.1 (0x1c030cdf4). Non-external. With a
// control block: under its lock a block without an immutable reference crashes (__cow_failure with
// "Attempting to over-release (immutable)"); the immutable count drops, and when neither count remains
// (after the unlock) the storage is released (__RELEASE_OBJECTS_IN_THE_ORDEREDSET__, kind 0) and the block
// freed. Without one the storage is released. Then super's -dealloc.
- (void)dealloc {
    CopyOnWriteBlock *block = __atomic_load_n(&cow, __ATOMIC_ACQUIRE);
    if (block != NULL) {
        os_unfair_lock_lock(&block->lock);
        uint16_t immutableReferences = block->immutableReferences;
        if (immutableReferences == 0) {
            _cow_failure(OF_STR("Attempting to over-release (immutable)"));
        }
        uint16_t mutableReferences = block->mutableReferences;
        immutableReferences -= 1;
        block->immutableReferences = immutableReferences;
        uint16_t remaining = mutableReferences | immutableReferences;
        os_unfair_lock_unlock(&block->lock);
        if (remaining == 0) {
            __RELEASE_OBJECTS_IN_THE_ORDEREDSET__(self, 0);
            free(block);
        }
    } else {
        __RELEASE_OBJECTS_IN_THE_ORDEREDSET__(self, 0);
    }
    [super dealloc];
}

// Native -[__NSFrozenOrderedSetM copy], 0x1c0238824. Non-external. The receiver retained (objc_retain, a tail
// call).
- (id)copy {
    return objc_retain(self);
}

// Native -[__NSFrozenOrderedSetM copyWithZone:], 0x1c0238828. Non-external. The receiver retained
// (objc_retain, a tail call).
- (id)copyWithZone:(struct _NSZone *)zone {
    return objc_retain(self);
}

// Native -[__NSFrozenOrderedSetM mutableCopy], 0x1c023882c. Non-external. The read hook (with the
// __CFTSANTagMutableOrderedSet tag); a receiver without a control block gets one (__cow_create mode 0,
// installed with a compare-and-swap from NULL, acquire-release; the loser's block is freed). The copy is
// __CFAllocateObject(__NSOrderedSetM, 0) (objc_opt_self of ___SCR___NSOrderedSetM), sharing the storage
// through __cow_copy (kind 0, mode 1).
- (id)mutableCopy {
    orderedSetMTSANRead(self, __builtin_return_address(0));
    if (__atomic_load_n(&cow, __ATOMIC_ACQUIRE) == NULL) {
        CopyOnWriteBlock *created = _cow_create(self, 0);
        CopyOnWriteBlock *expected = NULL;
        if (!__atomic_compare_exchange_n(
                &cow, &expected, created, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            )) {
            free(created);
        }
    }
    id copy = __CFAllocateObject(objc_opt_self((id)orderedSetMClass), 0);
    _cow_copy(self, 0, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSOrderedSet_cowCallbacks, copy, 1);
    return copy;
}

// Native -[__NSFrozenOrderedSetM mutableCopyWithZone:], 0x1c02388f4. Non-external. The body of -mutableCopy;
// the zone is not read.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    orderedSetMTSANRead(self, __builtin_return_address(0));
    if (__atomic_load_n(&cow, __ATOMIC_ACQUIRE) == NULL) {
        CopyOnWriteBlock *created = _cow_create(self, 0);
        CopyOnWriteBlock *expected = NULL;
        if (!__atomic_compare_exchange_n(
                &cow, &expected, created, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE
            )) {
            free(created);
        }
    }
    id copy = __CFAllocateObject(objc_opt_self((id)orderedSetMClass), 0);
    _cow_copy(self, 0, __atomic_load_n(&cow, __ATOMIC_ACQUIRE), &__NSOrderedSet_cowCallbacks, copy, 1);
    return copy;
}

@end

// Native cow_copy_instance, 0x1c02389bc (listed as _cow_copy_instance_0). Non-external; the copyInstance slot
// of ___NSOrderedSet_cowCallbacks. The source's control block (cow of a __NSOrderedSetM for kind 1, else of a
// __NSFrozenOrderedSetM; load-acquire) is stored with release into the copy's cow (the __NSOrderedSetM ivar
// for mode 1, else the frozen class's), then the 16-byte storage is copied the same way (one 128-bit load and
// store).
static void cow_copy_instance(id source, int kind, id copy, int mode) {
    int32_t sourceCowOffset = kind == 1 ? orderedSetMCowOffset : frozenOrderedSetMCowOffset;
    int32_t sourceStorageOffset = kind == 1 ? orderedSetMStorageOffset : frozenOrderedSetMStorageOffset;
    CopyOnWriteBlock *block =
        __atomic_load_n((CopyOnWriteBlock **)((char *)source + sourceCowOffset), __ATOMIC_ACQUIRE);
    int32_t copyCowOffset = mode == 1 ? orderedSetMCowOffset : frozenOrderedSetMCowOffset;
    int32_t copyStorageOffset = mode == 1 ? orderedSetMStorageOffset : frozenOrderedSetMStorageOffset;
    __atomic_store_n((CopyOnWriteBlock **)((char *)copy + copyCowOffset), block, __ATOMIC_RELEASE);
    *(OrderedSetMStorage *)((char *)copy + copyStorageOffset) =
        *(const OrderedSetMStorage *)((const char *)source + sourceStorageOffset);
}

// Native cow_copy_storage, 0x1c0238a20 (listed as _cow_copy_storage_0). Non-external; the copyStorage slot.
// The array is sent -mutableCopyWithZone: NULL; nil answers false. The hash is copied with
// CFBasicHashCreateCopy(NULL, hash); NULL answers false. Otherwise both copies replace the storage (one pair
// store) and true is answered.
// CoreFoundation defect (kept): when CFBasicHashCreateCopy fails, the array copy is not released (assembly
// 0x1c0238a60; confidence: verified); the path is an allocation failure only.
static bool cow_copy_storage(id object) {
    OrderedSetMStorage *orderedSetStorage = &((__ONSOrderedSetM *)object)->storage;
    BasicHashRef hash = orderedSetStorage->hash;
    id array = [orderedSetStorage->array mutableCopyWithZone:NULL];
    if (array == nil) {
        return false;
    }
    BasicHashRef hashCopy = CFBasicHashCreateCopy(NULL, hash);
    if (hashCopy == NULL) {
        return false;
    }
    orderedSetStorage = &((__ONSOrderedSetM *)object)->storage;
    orderedSetStorage->hash = hashCopy;
    orderedSetStorage->array = array;
    return true;
}

// Native cow_set_cow, 0x1c0238a84 (listed as _cow_set_cow_0). Non-external; the setCow slot. The control
// block stored into cow with release.
static void cow_set_cow(id object, CopyOnWriteBlock *block) {
    __atomic_store_n(&((__ONSOrderedSetM *)object)->cow, block, __ATOMIC_RELEASE);
}

// Native __NSOrderedSetEquateKeys, 0x1c0238a98. Non-external; the equateKeys callback of __NSOrderedSetMCB.
// The first key sent -isEqual: with the second; the BOOL answered as is.
static Boolean __NSOrderedSetEquateKeys(const void *collection, const void *stack) {
    return [(id)collection isEqual:(id)stack];
}

// Native __NSOrderedSetHashKey, 0x1c0238ab0. Non-external; the hashKey callback of __NSOrderedSetMCB. The key
// sent -hash (a tail call).
static OCFHashCode __NSOrderedSetHashKey(const void *stack) {
    return (OCFHashCode)[(id)stack hash];
}

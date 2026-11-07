//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSRLEArray.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#include "CoreFoundation/Objective-C/OCFNSException.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <malloc/malloc.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdlib.h>
#include <string.h>

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified unless noted. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.
// The shared-cache NSException class and NSRangeException name bindings have medium confidence.

extern char rleArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSRLEArray));
extern char mutableRLEArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableRLEArray));
extern char rleStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSString));
extern char rleMutableStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableString));
extern char rleExceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));

// Native ___NSRLEArrayLock. Non-external. Serializes the cached index and location pair.
static os_unfair_lock __NSRLEArrayLock = OS_UNFAIR_LOCK_INIT;
static const malloc_type_id_t kRunArrayMallocType = 0x1000040B4E0BC51;

static NSUInteger blockForLocation(NSRefCountedRunArray *list, NSUInteger location, ONSRange *range);

// The captured methods inline this exception construction at each bounds check.
static inline __attribute__((noreturn)) void raiseRunArrayBoundsException(id self, SEL cmd) {
    const char *name = _NSNameOfClass(objc_opt_class(self));
    const char *selector = sel_getName(cmd);
    id reason = [(id)rleStringClass stringWithFormat:(id)OF_STR("%s %s: Out of bounds"), name, selector];
    objc_exception_throw([(id)rleExceptionClass exceptionWithName:(id)ONSRangeException
                                                           reason:reason
                                                         userInfo:nil]);
}

@implementation ONSRLEArray

// Native -[NSRLEArray init], 0x1c04c6534. Non-external.
- (id)init {
    return [self initWithRefCountedRunArray:NULL];
}

// Native -[NSRLEArray initWithRefCountedRunArray:], 0x1c04c653c. Non-external.
- (id)initWithRefCountedRunArray:(NSRefCountedRunArray *)list {
    self = [super init];
    if (list != NULL) {
        theList = list;
        os_unfair_lock_lock(&list->lock);
        ++theList->references;
        os_unfair_lock_unlock(&theList->lock);
    } else {
        [self _makeNewListFrom:NULL];
    }
    return self;
}

// Native -[NSRLEArray copyWithZone:], 0x1c04c65e4. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return objc_retain(self);
}

// Native -[NSRLEArray mutableCopyWithZone:], 0x1c04c65e8. Non-external.
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    return [[(id)mutableRLEArrayClass allocWithZone:zone] initWithRefCountedRunArray:theList];
}

// Native -[NSRLEArray dealloc], 0x1c04c6614. Non-external.
- (void)dealloc {
    os_unfair_lock_lock(&theList->lock);
    if (theList->references > 1) {
        --theList->references;
        os_unfair_lock_unlock(&theList->lock);
    } else {
        os_unfair_lock_unlock(&theList->lock);
        for (NSUInteger i = 0; i < theList->used; ++i) {
            objc_release(theList->runs[i].object);
        }
        free(theList);
    }
    [super dealloc];
}

// Native -[NSRLEArray _makeNewListFrom:], 0x1c04c66e8. Non-external.
- (void)_makeNewListFrom:(NSRefCountedRunArray *)source {
    uint32_t capacity = source != NULL ? source->used : 2;
    theList = malloc_type_malloc(sizeof(*theList) + capacity * sizeof(NSRunArrayItem), kRunArrayMallocType);
    theList->lock = OS_UNFAIR_LOCK_INIT;
    theList->capacity = capacity;
    if (source != NULL) {
        for (NSUInteger i = 0; i < source->used; ++i) {
            id object = source->runs[i].object;
            theList->runs[i].length = source->runs[i].length;
            object = objc_retain(object);
            theList->runs[i].object = object;
        }
        theList->used = source->used;
        os_unfair_lock_lock(&__NSRLEArrayLock);
        theList->cachedIndex = source->cachedIndex;
        theList->cachedLocation = source->cachedLocation;
        os_unfair_lock_unlock(&__NSRLEArrayLock);
        theList->length = source->length;
        --source->references;
    } else {
        theList->cachedIndex = 0;
        theList->used = 0;
        theList->length = 0;
        theList->cachedLocation = 0;
    }
    theList->references = 1;
}

// Native -[NSRLEArray objectAtIndex:effectiveRange:], 0x1c04c67f4. Non-external.
- (id)objectAtIndex:(NSUInteger)index effectiveRange:(ONSRange *)range {
    NSRefCountedRunArray *list = theList;
    if (index >= list->length) {
        raiseRunArrayBoundsException(self, _cmd);
    }
    NSUInteger run = blockForLocation(list, index, range);
    return list->runs[run].object;
}

// Native _blockForLocation, 0x1c04c689c. Non-external.
static NSUInteger blockForLocation(NSRefCountedRunArray *list, NSUInteger location, ONSRange *range) {
    os_unfair_lock_lock(&__NSRLEArrayLock);
    NSUInteger offset = list->cachedLocation;
    NSUInteger index = 0;
    if (location > (offset >> 1)) {
        index = list->cachedIndex;
    } else {
        offset = 0;
    }
    os_unfair_lock_unlock(&__NSRLEArrayLock);
    if (offset > location) {
        do {
            --index;
            offset -= list->runs[index].length;
        } while (index != 0 && offset > location);
    } else {
        while (offset + list->runs[index].length <= location) {
            offset += list->runs[index].length;
            ++index;
        }
    }
    os_unfair_lock_lock(&__NSRLEArrayLock);
    list->cachedLocation = offset;
    list->cachedIndex = (uint32_t)index;
    os_unfair_lock_unlock(&__NSRLEArrayLock);
    if (range != NULL) {
        *range = (ONSRange){offset, list->runs[index].length};
    }
    return index;
}

// Native -[NSRLEArray objectAtIndex:effectiveRange:runIndex:], 0x1c04c6994. Non-external.
- (id)objectAtIndex:(NSUInteger)index effectiveRange:(ONSRange *)range runIndex:(NSUInteger *)runIndex {
    if (index >= theList->length) {
        raiseRunArrayBoundsException(self, _cmd);
    }
    NSUInteger run = blockForLocation(theList, index, range);
    if (runIndex != NULL) {
        *runIndex = run;
    }
    return theList->runs[run].object;
}

// Native -[NSRLEArray count], 0x1c04c6a80. Non-external.
- (NSUInteger)count {
    return theList->length;
}

// Native -[NSRLEArray description], 0x1c04c6a8c. Non-external.
- (id)description {
    ONSMutableString *description = [(id)rleMutableStringClass stringWithCapacity:100];
    [description appendFormat:(id)OF_STR("Length %ld (%d blocks, %d used, block %d is at %ld)\n"),
                              theList->length,
                              theList->capacity,
                              theList->used,
                              theList->cachedIndex,
                              theList->cachedLocation];
    for (NSUInteger i = 0; i < theList->used; ++i) {
        id object = theList->runs[i].object;
        [description appendFormat:(id)OF_STR(" %ld %p %@\n"), theList->runs[i].length, object, object];
    }
    return description;
}

@end

@implementation ONSMutableRLEArray

// Native -[NSMutableRLEArray copyWithZone:], 0x1c04c6b50. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return [[(id)rleArrayClass allocWithZone:zone] initWithRefCountedRunArray:theList];
}

// Native -[NSMutableRLEArray _setBlockCapacity:], 0x1c04c6b7c. Non-external.
- (void)_setBlockCapacity:(NSUInteger)capacity {
    if (capacity <= 1) {
        capacity = 1;
    }
    NSUInteger previousCapacity = theList->capacity;
    if (capacity > previousCapacity || capacity < (previousCapacity >> 1)) {
        NSUInteger units = (capacity + 3) / 3;
        theList = __OCFSafelyReallocateTyped(theList, (units << 6) | 40, kRunArrayMallocType, NULL);
        theList->capacity = (uint32_t)(4 * units);
    }
}

// Native -[NSMutableRLEArray insertObject:range:], 0x1c04c6bf8. Non-external.
- (void)insertObject:(id)object range:(ONSRange)range {
    NSRefCountedRunArray *oldList = theList;
    if (range.location > oldList->length) {
        raiseRunArrayBoundsException(self, _cmd);
    }
    if (range.length == 0) {
        return;
    }
    os_unfair_lock_lock(&theList->lock);
    if (theList->references >= 2) {
        [self _makeNewListFrom:theList];
    }
    os_unfair_lock_unlock(&oldList->lock);
    if (range.location == theList->length) {
        if (range.location == 0) {
            [self _setBlockCapacity:1];
            id retained = objc_retain(object);
            theList->runs[0] = (NSRunArrayItem){range.length, retained};
            ++theList->used;
        } else {
            uint32_t count = theList->used;
            NSRunArrayItem *last = &theList->runs[count - 1];
            if (object == last->object) {
                last->length += range.length;
                if (theList->cachedIndex >= count) {
                    theList->cachedLocation += range.length;
                }
            } else {
                [self _setBlockCapacity:count + 1];
                id retained = objc_retain(object);
                theList->runs[theList->used] = (NSRunArrayItem){range.length, retained};
                ++theList->used;
            }
        }
    } else {
        ONSRange existing = {0, 0};
        NSUInteger run = blockForLocation(theList, range.location, &existing);
        if (object == theList->runs[run].object) {
            theList->runs[run].length += range.length;
        } else if (
            run != 0 && existing.location == range.location && object == theList->runs[run - 1].object
        ) {
            theList->runs[run - 1].length += range.length;
            if (run <= theList->cachedIndex) {
                theList->cachedLocation += range.length;
            }
        } else if (existing.location != range.location) {
            [self _setBlockCapacity:theList->used + 2];
            memmove(
                &theList->runs[run + 2], &theList->runs[run], (theList->used - run) * sizeof(NSRunArrayItem)
            );
            id retained = objc_retain(object);
            theList->runs[run + 1].object = retained;
            theList->runs[run + 1].length = range.length;
            NSUInteger prefix = range.location - existing.location;
            theList->runs[run].length = prefix;
            id original = objc_retain(theList->runs[run].object);
            theList->runs[run + 2] = (NSRunArrayItem){existing.length - prefix, original};
            theList->used += 2;
        } else {
            [self _setBlockCapacity:theList->used + 1];
            memmove(
                &theList->runs[run + 1], &theList->runs[run], (theList->used - run) * sizeof(NSRunArrayItem)
            );
            id retained = objc_retain(object);
            theList->runs[run] = (NSRunArrayItem){range.length, retained};
            ++theList->used;
        }
    }
    theList->length += range.length;
}

// Native -[NSMutableRLEArray replaceObjectsInRange:withObject:length:], 0x1c04c6f08. Non-external.
// The native replacement path checks bounds but ignores a zero-length removal, even when length is nonzero.
- (void)replaceObjectsInRange:(ONSRange)range withObject:(id)object length:(NSUInteger)length {
    NSRefCountedRunArray *oldList = theList;
    if (range.location > oldList->length || range.length > oldList->length - range.location) {
        raiseRunArrayBoundsException(self, _cmd);
    }
    if (range.length == 0) {
        return;
    }
    id replacement = length != 0 ? object : nil;
    os_unfair_lock_lock(&theList->lock);
    if (theList->references >= 2) {
        [self _makeNewListFrom:theList];
    }
    ONSRange existing = {0, 0};
    os_unfair_lock_unlock(&oldList->lock);
    NSUInteger run = blockForLocation(theList, range.location, &existing);
    theList->length -= range.length;
    NSUInteger removed = existing.location + existing.length - range.location;
    if (removed > range.length) {
        removed = range.length;
    }
    theList->runs[run].length -= removed;
    if (theList->runs[run].length == 0) {
        objc_release(theList->runs[run].object);
    }
    NSUInteger firstEmpty = run + (theList->runs[run].length != 0);
    NSUInteger remaining = range.length - removed;
    while (remaining != 0) {
        ++run;
        NSUInteger runLength = theList->runs[run].length;
        NSUInteger deleted = remaining < runLength ? remaining : runLength;
        theList->runs[run].length = runLength - deleted;
        if (runLength <= remaining) {
            objc_release(theList->runs[run].object);
        }
        remaining -= deleted;
    }
    NSUInteger lastEmpty = run == 0 ? 0 : run - (theList->runs[run].length != 0);
    if (firstEmpty > lastEmpty) {
        if (replacement != nil) {
            [self insertObject:replacement range:(ONSRange){range.location, length}];
        }
        return;
    }
    if (firstEmpty != 0 && firstEmpty == theList->cachedIndex) {
        theList->cachedLocation -= theList->runs[firstEmpty - 1].length;
        --theList->cachedIndex;
    }
    id pending = object;
    NSUInteger following = lastEmpty + 1;
    BOOL joined = NO;
    if (replacement == nil) {
        pending = nil;
        joined = firstEmpty != 0;
    } else if (firstEmpty != 0 && theList->runs[firstEmpty - 1].object == replacement) {
        theList->runs[firstEmpty - 1].length += length;
        theList->length += length;
        joined = YES;
    } else if (following < theList->used && theList->runs[following].object == replacement) {
        theList->runs[following].length += length;
        theList->length += length;
        joined = firstEmpty != 0;
        pending = nil;
    } else if (firstEmpty < theList->used) {
        id retained = objc_retain(object);
        theList->runs[firstEmpty++] = (NSRunArrayItem){length, retained};
        theList->length += length;
        pending = nil;
    }
    if (joined) {
        if (following < theList->used &&
            theList->runs[firstEmpty - 1].object == theList->runs[following].object) {
            theList->runs[firstEmpty - 1].length += theList->runs[following].length;
            objc_release(theList->runs[following].object);
            lastEmpty = following;
        }
        pending = nil;
    }
    if (firstEmpty <= lastEmpty) {
        following = lastEmpty + 1;
        memmove(
            &theList->runs[firstEmpty],
            &theList->runs[following],
            (theList->used - following) * sizeof(NSRunArrayItem)
        );
        theList->used += (uint32_t)(firstEmpty - following);
        [self _setBlockCapacity:theList->used];
    }
    if (pending != nil) {
        [self insertObject:pending range:(ONSRange){range.location, length}];
    }
}

// Native -[NSMutableRLEArray deleteObjectsInRange:], 0x1c04c72b4. Non-external.
- (void)deleteObjectsInRange:(ONSRange)range {
    [self replaceObjectsInRange:range withObject:nil length:0];
}

@end

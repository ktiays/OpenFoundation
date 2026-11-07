//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Number/ONSValue.h"
#include "CoreFoundation/OFPortHooks.h"

#import "Foundation/Archiving/ONSArchivingPrivate.h"
#import "Foundation/Number/ONSPlaceholderNumber.h"
#import "Foundation/Number/ONSValueGeometry.h"
#import "Foundation/String/ONSStringFromRange.h"
#import "Foundation/Number/ONSUnrestoredInterfaces.h"

#include "CoreFoundation/OFPortHooks.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFSet.h"
#include "CoreFoundation/Number/OCFNumber.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"
#include "CoreFoundation/String/OCFString.h"

#include <malloc/malloc.h>
#include <math.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <os/lock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: __NSNewValue and matchTypeEncoding (0x1c05f77f4-0x1c05f8003), the NSConcreteValue
// and NSWeakObjectValue methods (round 2), the NSValue methods (0x1c05f8d40-0x1c05f9eef)
// with newDecodedValue (0x1c05f9ef4), the NSNumber methods (0x1c05fa40c-0x1c05fd80f) with newDecodedNumber
// (0x1c05fccd4) and newNumber (0x1c05fd3a0), the NSPlaceholderValue and NSPlaceholderNumber methods
// (0x1c05fd810-0x1c05fdfeb) and the type-information callbacks and mungeUnionsAndStructures
// (0x1c05fdfec-0x1c05fe0df) lie together in the iOS Simulator __text section and form one native unit; the
// methods that only request a concrete implementation (-[NSValue objCType], -[NSValue
// initWithBytes:objCType:], the -[NSNumber initWith...:] family and three NSPlaceholderValue methods,
// 0x1c0fe7098-0x1c0fe745f) lie in a separate region of the same section and are restored inside their classes
// here. The native source path is not in the evidence; the unit lives under Number/ with the family.
// Objective-C compiled with manual reference counting (objc_retain, objc_release, objc_autorelease). Unless a
// definition says otherwise: Foundation, iOS Simulator 24A434, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Source/Foundation/Number/Reading.md.
//
// Shared-cache GOT slots outside the extracted image are bound by use and slot order (confidence: medium):
// 0x222280b50 kCFAllocatorDefault, 0x222280b70 kCFAllocatorSystemDefault, 0x222280b80 kCFBooleanFalse,
// 0x222280b88 kCFBooleanTrue (alphabetical import order, consistent with 0x222280b58 kCFAllocatorMalloc and
// 0x222280b68 kCFAllocatorNull at the image's no-copy string call sites); 0x22227fae0 the NSException class,
// 0x22227fb20 the NSMutableData class (-initWithLength:, -mutableBytes); 0x22227f500
// NSInvalidArgumentException, 0x22227f4f8 NSInternalInconsistencyException (the image's other raises through
// these slots); 0x222409408 objc_debug_tag60_permutations, 0x222409418 objc_debug_taggedpointer_mask,
// 0x222409420 objc_debug_taggedpointer_obfuscator, as in CoreFoundation/Runtime/OCFRuntime.c. The selector
// strings 0x2387c85eb and 0x2387aef89 that -getValue: and -getValue:size: test are bound to "getValue:size:"
// and "getValue:" by the logic of the two methods (confidence: medium).
//
// Exception regions were checked through IDA against __unwind_info and __gcc_except_tab (see Reading.md).
// matchTypeEncoding's landing pads compare the type selector with 1; its indirect typed entry is bound to
// NSException (GOT 0x22227fba0; confidence: medium). The other sizing catches have a null catch-all entry
// (@catch (...), including foreign exceptions).

// libobjc interfaces (supplied, never reimplemented); read for each encode, as natively.
extern uintptr_t objc_debug_taggedpointer_obfuscator;
extern const uint8_t objc_debug_tag60_permutations[8];
extern uintptr_t objc_debug_taggedpointer_mask;

// Class objects the unit loads directly (class references and GOT slots).
extern char valueClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSValue));
extern char numberClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSNumber));
extern char placeholderValueClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSPlaceholderValue));
extern char placeholderNumberClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSPlaceholderNumber));
extern char concreteValueClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSConcreteValue));
extern char weakObjectValueClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSWeakObjectValue));
extern char stringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSString));
extern char exceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));
extern char mutableDataClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableData));
extern char errorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSError));
extern char keyedArchiverClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSKeyedArchiver));
extern char keyedUnarchiverClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSKeyedUnarchiver));

static BOOL matchTypeEncoding(const char *type1, const char *type2, NSUInteger size);
static OCFHashCode cachedObjCTypeInfoHash(const void *value);
static Boolean cachedObjCTypeInfoIsEqual(const void *value1, const void *value2);
static BOOL mungeUnionsAndStructures(const char **cursor);
static id newDecodedValue(id coder, struct _NSZone *zone);
static id newDecodedNumber(id coder);
static id newNumber(unsigned char type, const void *bytes);

// Native __NSCachedTypeInfoRect64 (0x2227d7a38), Point64 (0x2227d7a48), Size64 (0x2227d7a58), Range64
// (0x2227d7a68), CGAffineTransform64 (0x2227d7a78), UIEdgeInsets64 (0x2227d7a88), NSEdgeInsets64
// (0x2227d7a98), NSDirectionalEdgeInsets64 (0x2227d7aa8) and UIOffset64 (0x2227d7ab8), __DATA_CONST,__const,
// non-external: the size, then the cstring the native code compares against (0x20, 0x10, 0x10, 0x10, 0x30,
// 0x20, 0x20, 0x20 and 0x10 bytes).
static const CachedTypeInfo _NSCachedTypeInfoRect64 = {0x20, "{CGRect={CGPoint=dd}{CGSize=dd}}"};
static const CachedTypeInfo _NSCachedTypeInfoPoint64 = {0x10, "{CGPoint=dd}"};
static const CachedTypeInfo _NSCachedTypeInfoSize64 = {0x10, "{CGSize=dd}"};
static const CachedTypeInfo _NSCachedTypeInfoRange64 = {0x10, "{_NSRange=QQ}"};
static const CachedTypeInfo _NSCachedTypeInfoCGAffineTransform64 = {0x30, "{CGAffineTransform=dddddd}"};
static const CachedTypeInfo _NSCachedTypeInfoUIEdgeInsets64 = {0x20, "{UIEdgeInsets=dddd}"};
static const CachedTypeInfo _NSCachedTypeInfoNSEdgeInsets64 = {0x20, "{NSEdgeInsets=dddd}"};
static const CachedTypeInfo _NSCachedTypeInfoNSDirectionalEdgeInsets64 = {
    0x20, "{NSDirectionalEdgeInsets=dddd}"
};
static const CachedTypeInfo _NSCachedTypeInfoUIOffset64 = {0x10, "{UIOffset=dd}"};

// Native __MergedGlobals.24, 0x22e76cd60 (__DATA,__bss, non-external): the lock at offset 0 and the set of
// allocated type-information records at offset 8 (qword_22E76CD68). typeInfoLock and typeInfoSet are local
// descriptive names.
static os_unfair_lock typeInfoLock = OS_UNFAIR_LOCK_INIT;
static OCFMutableSetRef typeInfoSet;

// Native __NSConcreteValueClass, 0x22e775b10 (__DATA,__common, was a private external).
// Foundation defect: __NSNewValue 0x1c05f77f4 reads/writes this cache without synchronization at
// 0x1c05f7cc4/0x1c05f7cd8; -[NSValue isEqual:] 0x1c05f93f4 reads it at 0x1c05f9420. Concurrent
// first use races (confidence: verified). All restored accesses use relaxed atomics, preserving ABI and
// the native initialization/message order. Future readers must also access this cache atomically.
_Atomic(uintptr_t) __NSConcreteValueClass;

// Native __NSNumberTaggedPointersDisabled, 0x22e6fccd0 (__DATA,__bss, non-external): set by +[NSNumber
// initialize] when tagged pointers are unavailable or the executable predates the tagged numbers.
static uint8_t _NSNumberTaggedPointersDisabled;

// Native __placeholderValue (0x2227d7ad8) and __placeholderNumber (0x2227d7ac8), __DATA_CONST,__const,
// non-external: the class, then the zone (NULL). +[NSValue allocWithZone:] and +[NSNumber allocWithZone:]
// answer them; newNumber and the decoders send them -initWith...:. PlaceholderObject is a local descriptive
// name.
typedef struct {
    const void *isa;
    struct _NSZone *zone;
} PlaceholderObject;

static const PlaceholderObject __placeholderValue = {placeholderValueClass, NULL};
static const PlaceholderObject __placeholderNumber = {placeholderNumberClass, NULL};

// The bad-objCType raise the NSNumber methods inline: NSInternalInconsistencyException with "%@: bad
// objCType:
// \"%s\"", the method proem and the receiver's -objCType, in that order. raiseBadObjCType is a local
// descriptive name.
static __attribute__((noreturn, always_inline)) inline void raiseBadObjCType(id self, SEL _cmd) {
    id proem = _ONSMethodExceptionProem(self, _cmd);
    id reason =
        [(id)stringClass stringWithFormat:(id)OF_STR("%@: bad objCType: \"%s\""), proem, [self objCType]];
    objc_exception_throw([(id)exceptionClass exceptionWithName:(id)ONSInternalInconsistencyException
                                                        reason:reason
                                                      userInfo:nil]);
}

// libobjc's inline tagged-pointer encoder (_objc_encodeTaggedPointer, split tagged pointers): XOR with the
// obfuscator; unless the result is an extended tag with all of bits 63, 62 and 0-2 set (then the plain
// pointer is answered), the basic tag in bits 0-2 is permuted through objc_debug_tag60_permutations. The
// NSNumber constructors inline it. encodeTaggedNumber is a local descriptive name.
static __attribute__((always_inline)) inline id encodeTaggedNumber(uintptr_t pointer) {
    uintptr_t value = objc_debug_taggedpointer_obfuscator ^ pointer;
    if ((~value & UINT64_C(0xC000000000000007)) == 0) {
        return (id)pointer;
    }
    return (id)((value & ~(uintptr_t)7) | objc_debug_tag60_permutations[value & 7]);
}

// These always-inlined expressions preserve the FCVTZS/FCVTZU instructions in the accessor and
// formatting captures, including saturation, NaNs and FPSR flags. C casts have undefined behavior outside
// their integer range. The input register width remains S for float and D for double; no widening occurs.
// They are instruction spellings within the native functions, not additional restored function boundaries.
static __attribute__((always_inline)) inline int32_t floatToInt32(float value) {
    OF_PORT_RETURN(floatToInt32, value);
    int32_t result;
    OF_PORT_NATIVE(__asm__ volatile("fcvtzs %w0, %s1" : "=r"(result) : "w"(value) : "memory"));
    return result;
}

static __attribute__((always_inline)) inline uint32_t floatToUInt32(float value) {
    OF_PORT_RETURN(floatToUInt32, value);
    uint32_t result;
    OF_PORT_NATIVE(__asm__ volatile("fcvtzu %w0, %s1" : "=r"(result) : "w"(value) : "memory"));
    return result;
}

static __attribute__((always_inline)) inline int64_t floatToInt64(float value) {
    OF_PORT_RETURN(floatToInt64, value);
    int64_t result;
    OF_PORT_NATIVE(__asm__ volatile("fcvtzs %x0, %s1" : "=r"(result) : "w"(value) : "memory"));
    return result;
}

static __attribute__((always_inline)) inline uint64_t floatToUInt64(float value) {
    OF_PORT_RETURN(floatToUInt64, value);
    uint64_t result;
    OF_PORT_NATIVE(__asm__ volatile("fcvtzu %x0, %s1" : "=r"(result) : "w"(value) : "memory"));
    return result;
}

static __attribute__((always_inline)) inline int32_t doubleToInt32(double value) {
    OF_PORT_RETURN(doubleToInt32, value);
    int32_t result;
    OF_PORT_NATIVE(__asm__ volatile("fcvtzs %w0, %d1" : "=r"(result) : "w"(value) : "memory"));
    return result;
}

static __attribute__((always_inline)) inline uint32_t doubleToUInt32(double value) {
    OF_PORT_RETURN(doubleToUInt32, value);
    uint32_t result;
    OF_PORT_NATIVE(__asm__ volatile("fcvtzu %w0, %d1" : "=r"(result) : "w"(value) : "memory"));
    return result;
}

static __attribute__((always_inline)) inline int64_t doubleToInt64(double value) {
    OF_PORT_RETURN(doubleToInt64, value);
    int64_t result;
    OF_PORT_NATIVE(__asm__ volatile("fcvtzs %x0, %d1" : "=r"(result) : "w"(value) : "memory"));
    return result;
}

static __attribute__((always_inline)) inline uint64_t doubleToUInt64(double value) {
    OF_PORT_RETURN(doubleToUInt64, value);
    uint64_t result;
    OF_PORT_NATIVE(__asm__ volatile("fcvtzu %x0, %d1" : "=r"(result) : "w"(value) : "memory"));
    return result;
}

// Native __NSNewValue, 0x1c05f77f4. Non-external. The nine fixed geometry encodings (and their 32-bit float
// and unsigned forms, widened to the 64-bit layout through a stack copy) use the static records and special
// flags 1-4 and 10-14; any other encoding is looked up in the set under the lock, by encoding, and a record
// is allocated (malloc type 0x1050040606e6c2f, the encoding copied after the two words) and inserted under
// the lock unless another thread inserted one first. The set is created outside the lock (system default
// allocator, equal and hash callbacks only) and released if another thread installed one. The object is an
// NSConcreteValue (cached in __NSConcreteValueClass through objc_opt_self) with the value size as extra
// bytes, sent -init; when that answers an object, the record and the special flags are stored and the value
// bytes moved into the indexed ivars. An exception from sizing the encoding, or a failed record allocation,
// answers nil.
static id _NSNewValue(const void *value, const char *type, struct _NSZone *zone) {
    double converted[6] = {0};
    const CachedTypeInfo *info;
    unsigned long long specialFlags;
    if (strcmp(type, "{_NSRange=II}") == 0) {
        uint32_t range[2];
        memcpy(range, value, sizeof(range));
        uint64_t range64[2] = {range[0], range[1]};
        memcpy(converted, range64, sizeof(range64));
        info = &_NSCachedTypeInfoRange64;
        value = converted;
        specialFlags = 4;
    } else if (strcmp(type, "{_NSRange=QQ}") == 0) {
        info = &_NSCachedTypeInfoRange64;
        specialFlags = 4;
    } else if (strcmp(type, "{CGPoint=ff}") == 0) {
        float floats[2];
        memcpy(floats, value, sizeof(floats));
        converted[0] = floats[0];
        converted[1] = floats[1];
        info = &_NSCachedTypeInfoPoint64;
        value = converted;
        specialFlags = 1;
    } else if (strcmp(type, "{CGPoint=dd}") == 0) {
        info = &_NSCachedTypeInfoPoint64;
        specialFlags = 1;
    } else if (strcmp(type, "{CGSize=ff}") == 0) {
        float floats[2];
        memcpy(floats, value, sizeof(floats));
        converted[0] = floats[0];
        converted[1] = floats[1];
        info = &_NSCachedTypeInfoSize64;
        value = converted;
        specialFlags = 2;
    } else if (strcmp(type, "{CGSize=dd}") == 0) {
        info = &_NSCachedTypeInfoSize64;
        specialFlags = 2;
    } else if (strcmp(type, "{CGRect={CGPoint=ff}{CGSize=ff}}") == 0) {
        float floats[4];
        memcpy(floats, value, sizeof(floats));
        converted[0] = floats[0];
        converted[1] = floats[1];
        converted[2] = floats[2];
        converted[3] = floats[3];
        info = &_NSCachedTypeInfoRect64;
        value = converted;
        specialFlags = 3;
    } else if (strcmp(type, "{CGRect={CGPoint=dd}{CGSize=dd}}") == 0) {
        info = &_NSCachedTypeInfoRect64;
        specialFlags = 3;
    } else if (strcmp(type, "{CGAffineTransform=ffffff}") == 0) {
        float floats[6];
        memcpy(floats, value, sizeof(floats));
        converted[0] = floats[0];
        converted[1] = floats[1];
        converted[2] = floats[2];
        converted[3] = floats[3];
        converted[4] = floats[4];
        converted[5] = floats[5];
        info = &_NSCachedTypeInfoCGAffineTransform64;
        value = converted;
        specialFlags = 10;
    } else if (strcmp(type, "{CGAffineTransform=dddddd}") == 0) {
        info = &_NSCachedTypeInfoCGAffineTransform64;
        specialFlags = 10;
    } else if (strcmp(type, "{UIEdgeInsets=ffff}") == 0) {
        float floats[4];
        memcpy(floats, value, sizeof(floats));
        converted[0] = floats[0];
        converted[1] = floats[1];
        converted[2] = floats[2];
        converted[3] = floats[3];
        info = &_NSCachedTypeInfoUIEdgeInsets64;
        value = converted;
        specialFlags = 11;
    } else if (strcmp(type, "{UIEdgeInsets=dddd}") == 0) {
        info = &_NSCachedTypeInfoUIEdgeInsets64;
        specialFlags = 11;
    } else if (strcmp(type, "{NSEdgeInsets=ffff}") == 0) {
        float floats[4];
        memcpy(floats, value, sizeof(floats));
        converted[0] = floats[0];
        converted[1] = floats[1];
        converted[2] = floats[2];
        converted[3] = floats[3];
        info = &_NSCachedTypeInfoNSEdgeInsets64;
        value = converted;
        specialFlags = 12;
    } else if (strcmp(type, "{NSEdgeInsets=dddd}") == 0) {
        info = &_NSCachedTypeInfoNSEdgeInsets64;
        specialFlags = 12;
    } else if (strcmp(type, "{NSDirectionalEdgeInsets=ffff}") == 0) {
        float floats[4];
        memcpy(floats, value, sizeof(floats));
        converted[0] = floats[0];
        converted[1] = floats[1];
        converted[2] = floats[2];
        converted[3] = floats[3];
        info = &_NSCachedTypeInfoNSDirectionalEdgeInsets64;
        value = converted;
        specialFlags = 14;
    } else if (strcmp(type, "{NSDirectionalEdgeInsets=dddd}") == 0) {
        info = &_NSCachedTypeInfoNSDirectionalEdgeInsets64;
        specialFlags = 14;
    } else if (strcmp(type, "{UIOffset=ff}") == 0) {
        float floats[2];
        memcpy(floats, value, sizeof(floats));
        converted[0] = floats[0];
        converted[1] = floats[1];
        info = &_NSCachedTypeInfoUIOffset64;
        value = converted;
        specialFlags = 13;
    } else if (strcmp(type, "{UIOffset=dd}") == 0) {
        info = &_NSCachedTypeInfoUIOffset64;
        specialFlags = 13;
    } else {
        CachedTypeInfo key = {0, type};
        os_unfair_lock_lock(&typeInfoLock);
        OCFMutableSetRef set = typeInfoSet;
        if (set == NULL) {
            os_unfair_lock_unlock(&typeInfoLock);
            OCFSetCallBacks callBacks;
            memset(&callBacks, 0, offsetof(OCFSetCallBacks, equal));
            callBacks.equal = cachedObjCTypeInfoIsEqual;
            callBacks.hash = cachedObjCTypeInfoHash;
            set = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, &callBacks);
            os_unfair_lock_lock(&typeInfoLock);
            if (typeInfoSet != NULL) {
                OCFRelease(set);
                set = typeInfoSet;
            } else {
                typeInfoSet = set;
            }
        }
        info = OCFSetGetValue(set, &key);
        os_unfair_lock_unlock(&typeInfoLock);
        if (info == NULL) {
            NSUInteger size = 0;
            @try {
                ONSGetSizeAndAlignment(type, &size, NULL);
            } @catch (...) {
                return nil;
            }
            size_t length = strlen(type);
            char *record = malloc_type_malloc(length + 17, 0x1050040606E6C2FULL);
            if (record == NULL) {
                return nil;
            }
            ((CachedTypeInfo *)record)->size = size;
            ((CachedTypeInfo *)record)->type = record + 16;
            strlcpy(record + 16, type, length + 1);
            os_unfair_lock_lock(&typeInfoLock);
            info = OCFSetGetValue(typeInfoSet, &key);
            if (info != NULL) {
                free(record);
            } else {
                OCFSetSetValue(typeInfoSet, record);
                info = (const CachedTypeInfo *)record;
            }
            os_unfair_lock_unlock(&typeInfoLock);
        }
        specialFlags = 0;
    }
    Class cls = (Class)atomic_load_explicit(&__NSConcreteValueClass, memory_order_relaxed);
    if (cls == Nil) {
        cls = (Class)objc_opt_self((id)concreteValueClass);
        atomic_store_explicit(&__NSConcreteValueClass, (uintptr_t)cls, memory_order_relaxed);
    }
    NSUInteger size = info->size;
    id object = [ONSAllocateObject(cls, size, zone) init];
    if (object != nil) {
        *(const CachedTypeInfo **)((uint8_t *)(void *)object + concreteValueTypeInfoOffset) = info;
        *(uint64_t *)((uint8_t *)(void *)object + concreteValueSpecialFlagsOffset) = specialFlags;
        memmove(object_getIndexedIvars(object), value, size);
    }
    return object;
}

// Native matchTypeEncoding, 0x1c05f7eb8. Non-external. NULL encodings never match; identical pointers or
// equal strings match. Otherwise both encodings must have the expected size and equal alignments (an
// exception while sizing answers NO), and then the encodings are walked together, mungeUnionsAndStructures
// skipping aggregate names and brackets on each side, until one ends or a character differs.
static BOOL matchTypeEncoding(const char *type1, const char *type2, NSUInteger size) {
    const char *cursor2 = type2;
    const char *cursor1 = type1;
    if (type1 == NULL || type2 == NULL) {
        return NO;
    }
    if (type1 == type2 || strcmp(type1, type2) == 0) {
        return YES;
    }
    NSUInteger alignment1;
    NSUInteger size1;
    NSUInteger alignment2;
    NSUInteger size2;
    @try {
        alignment1 = 0;
        size1 = 0;
        ONSGetSizeAndAlignment(type1, &size1, &alignment1);
        alignment2 = 0;
        size2 = 0;
        ONSGetSizeAndAlignment(type2, &size2, &alignment2);
    } @catch (ONSException *exception) {
        return NO;
    }
    if (size1 != size2 || size1 != size || alignment1 != alignment2) {
        return NO;
    }
    if (*cursor1 != 0) {
        do {
            if (*cursor2 == 0) {
                break;
            }
            if (!mungeUnionsAndStructures(&cursor1)) {
                return NO;
            }
            if (!mungeUnionsAndStructures(&cursor2)) {
                return NO;
            }
            if (*cursor1 != *cursor2) {
                return NO;
            }
            if (*cursor1 == 0) {
                break;
            }
            cursor1++;
            cursor2++;
        } while (*cursor1 != 0);
    }
    return YES;
}

// NSConcreteValue round-2 definitions share the native unit with __NSNewValue and matchTypeEncoding.
// Reading: Foundation/Number/Reading.md, round 2. iOS Device divergence: iOS Device 24A446 remains unread.
static id _stringFromEdgeInsets(ONSConcreteValue *value);

@implementation ONSConcreteValue

// OF_STR creates the restored constant string without an Objective-C literal.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"

// Native +[NSConcreteValue initialize], 0x1c05f7700. Non-external.
+ (void)initialize {
    if (self != objc_opt_class((id)concreteValueClass)) {
        return;
    }
    NSUInteger size = 0;
    ONSGetSizeAndAlignment("{CGRect={CGPoint=dd}{CGSize=dd}}", &size, NULL);
    if (size != 32) {
        ONSLog((id)OF_STR("*** Inconsistency in NSValue type strings for NSRect types!"));
    }
    size = 0;
    ONSGetSizeAndAlignment("{CGPoint=dd}", &size, NULL);
    if (size != 16) {
        ONSLog((id)OF_STR("*** Inconsistency in NSValue type strings for NSPoint types!"));
    }
    size = 0;
    ONSGetSizeAndAlignment("{CGSize=dd}", &size, NULL);
    if (size != 16) {
        ONSLog((id)OF_STR("*** Inconsistency in NSValue type strings for NSSize types!"));
    }
}

#pragma clang diagnostic pop

// Native -[NSConcreteValue getValue:], 0x1c05f7d78. Non-external.
- (void)getValue:(void *)value {
    const void *bytes = object_getIndexedIvars(self);
    memmove(value, bytes, ((const CachedTypeInfo *)typeInfo)->size);
}

// Native -[NSConcreteValue getValue:size:], 0x1c05f7db4. Non-external.
// The throw terminates the function. The next method in the pseudocode is a separate entry.
- (void)getValue:(void *)value size:(NSUInteger)size {
    if (size == ((const CachedTypeInfo *)typeInfo)->size) {
        memmove(value, object_getIndexedIvars(self), size);
        return;
    }
    const char *encoding = [self objCType];
    id reason = [(id)stringClass
        stringWithFormat:
            (id)OF_STR("Cannot get value with size %zu. The type encoded as %s is expected to be %zu bytes"),
            size,
            encoding,
            ((const CachedTypeInfo *)typeInfo)->size];
    objc_exception_throw([(id)exceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                        reason:reason
                                                      userInfo:nil]);
}

// Native -[NSConcreteValue objCType], 0x1c05f7e64. Non-external.
- (const char *)objCType {
    return ((const CachedTypeInfo *)typeInfo)->type;
}

// Native -[NSConcreteValue _value], 0x1c05f7e78. Non-external.
- (void *)_value {
    return object_getIndexedIvars(self);
}

// Native -[NSConcreteValue _matchType:size:strict:], 0x1c05f7e7c. Non-external.
- (BOOL)_matchType:(const char *)type size:(NSUInteger)size strict:(BOOL)strict {
    const CachedTypeInfo *info = typeInfo;
    if (size != info->size) {
        return NO;
    }
    return !strict || matchTypeEncoding(info->type, type, size);
}

// Native -[NSConcreteValue isEqualToValue:], 0x1c05f8004. Non-external.
- (BOOL)isEqualToValue:(id)value {
    if (self == value) {
        return YES;
    }
    const char *type = ((const CachedTypeInfo *)typeInfo)->type;
    if (strcmp(type, [value objCType]) != 0) {
        return NO;
    }
    size_t size = ((const CachedTypeInfo *)typeInfo)->size;
    Class ownClass = objc_opt_class(self);
    if (ownClass == objc_opt_class(value) ||
        objc_opt_isKindOfClass(
            value, (Class)atomic_load_explicit(&__NSConcreteValueClass, memory_order_relaxed)
        )) {
        const void *bytes = object_getIndexedIvars(self);
        return memcmp(bytes, [value _value], size) == 0;
    }
    unsigned char stackBuffer[1024];
    void *buffer = size < 0x401 ? (void *)stackBuffer : malloc_type_malloc(size, 0x100004077774924ULL);
    [value getValue:buffer];
    BOOL equal = memcmp(object_getIndexedIvars(self), buffer, size) == 0;
    if (size >= 0x401) {
        free(buffer);
    }
    return equal;
}

// Native -[NSConcreteValue hash], 0x1c05f8174. Non-external.
- (NSUInteger)hash {
    const unsigned char *bytes = object_getIndexedIvars(self);
    size_t size = ((const CachedTypeInfo *)typeInfo)->size;
    NSUInteger hash = 0;
    if (size <= 7) {
        for (size_t i = 0; i < size; ++i) {
            hash = (hash << 8) | bytes[i];
        }
    } else {
        size_t words = (size < 32 ? size : 32) / 8;
        for (size_t i = 0; i < words; ++i) {
            uint64_t word;
            memcpy(&word, bytes + i * 8, sizeof(word));
            hash ^= word;
        }
    }
    return hash;
}

// Native -[NSConcreteValue copyWithZone:], 0x1c05f81fc. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    if (ONSShouldRetainWithZone(self, zone)) {
        return objc_retain(self);
    }
    const void *bytes = object_getIndexedIvars(self);
    return _NSNewValue(bytes, ((const CachedTypeInfo *)typeInfo)->type, zone);
}

// Native -[NSConcreteValue description], 0x1c05f8264. Non-external.
// The range length is passed in X1 to NSStringFromRange. Hex-Rays only represents X0.
- (id)description {
    uint64_t flags = _specialFlags;
    if (flags == 0) {
        const char *type = [self objCType];
        if (strcmp(type, "{CGRect={CGPoint=dd}{CGSize=dd}}") == 0) {
            flags = 3;
        } else if (strcmp(type, "{CGPoint=dd}") == 0) {
            flags = 1;
        } else if (strcmp(type, "{CGSize=dd}") == 0) {
            flags = 2;
        } else if (strcmp(type, "{_NSRange=QQ}") == 0) {
            flags = 4;
        } else if (
            strcmp(type, "{CGAffineTransform=ffffff}") == 0 || strcmp(type, "{CGAffineTransform=dddddd}") == 0
        ) {
            flags = 10;
        } else if (strcmp(type, "{UIEdgeInsets=ffff}") == 0 || strcmp(type, "{UIEdgeInsets=dddd}") == 0) {
            flags = 11;
        } else if (strcmp(type, "{NSEdgeInsets=ffff}") == 0 || strcmp(type, "{NSEdgeInsets=dddd}") == 0) {
            flags = 12;
        } else if (
            strcmp(type, "{NSDirectionalEdgeInsets=ffff}") == 0 ||
            strcmp(type, "{NSDirectionalEdgeInsets=dddd}") == 0
        ) {
            flags = 14;
        } else if (strcmp(type, "{UIOffset=ff}") == 0 || strcmp(type, "{UIOffset=dd}") == 0) {
            flags = 13;
        }
    }
    id detail;
    id format;
    switch (flags) {
        case 1:
            detail = ONSStringFromPoint([self pointValue]);
            format = (id)OF_STR("NSPoint: %@");
            break;
        case 2:
            detail = ONSStringFromSize([self sizeValue]);
            format = (id)OF_STR("NSSize: %@");
            break;
        case 3:
            detail = ONSStringFromRect([self rectValue]);
            format = (id)OF_STR("NSRect: %@");
            break;
        case 4:
            detail = ONSStringFromRange([self rangeValue]);
            format = (id)OF_STR("NSRange: %@");
            break;
        case 10: {
            double transform[6] = {0};
            [self getValue:transform];
            detail = [(id)stringClass stringWithFormat:(id)OF_STR("{{%.*g, %.*g, %.*g, %.*g}, {%.*g, %.*g}}"),
                                                       17,
                                                       transform[0],
                                                       17,
                                                       transform[1],
                                                       17,
                                                       transform[2],
                                                       17,
                                                       transform[3],
                                                       17,
                                                       transform[4],
                                                       17,
                                                       transform[5]];
            format = (id)OF_STR("CGAffineTransform: %@");
            break;
        }
        case 11:
            detail = _stringFromEdgeInsets(self);
            format = (id)OF_STR("UIEdgeInsets: %@");
            break;
        case 12:
            detail = _stringFromEdgeInsets(self);
            format = (id)OF_STR("NSEdgeInsets: %@");
            break;
        case 13: {
            double offset[2] = {0};
            [self getValue:offset];
            detail =
                [(id)stringClass stringWithFormat:(id)OF_STR("{%.*g, %.*g}"), 17, offset[0], 17, offset[1]];
            format = (id)OF_STR("UIOffset: %@");
            break;
        }
        case 14:
            detail = _stringFromEdgeInsets(self);
            format = (id)OF_STR("NSDirectionalEdgeInsets: %@");
            break;
        default:
            return [super description];
    }
    return [(id)stringClass stringWithFormat:format, detail];
}

// Native __stringFromEdgeInsets, 0x1c05f8608. Non-external.
static id _stringFromEdgeInsets(ONSConcreteValue *value) {
    double insets[4] = {0};
    [value getValue:insets];
    return [(id)stringClass stringWithFormat:(id)OF_STR("{%.*g, %.*g, %.*g, %.*g}"),
                                             17,
                                             insets[0],
                                             17,
                                             insets[1],
                                             17,
                                             insets[2],
                                             17,
                                             insets[3]];
}

// Native +[NSConcreteValue supportsSecureCoding], 0x1c05f869c. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSConcreteValue encodeWithCoder:], 0x1c05f86a4. Non-external.
// The range length returned in X1 is encoded first, then location. The throw ends the failure path.
- (void)encodeWithCoder:(id)coder {
    if (_specialFlags == 0 || ![coder allowsKeyedCoding]) {
        [super encodeWithCoder:coder];
        return;
    }
    [coder encodeInteger:(NSInteger)_specialFlags forKey:(id)OF_STR("NS.special")];
    switch (_specialFlags) {
        case 1:
            [coder encodePoint:[self pointValue] forKey:(id)OF_STR("NS.pointval")];
            return;
        case 2:
            [coder encodeSize:[self sizeValue] forKey:(id)OF_STR("NS.sizeval")];
            return;
        case 3:
            [coder encodeRect:[self rectValue] forKey:(id)OF_STR("NS.rectval")];
            return;
        case 4: {
            ONSRange range = [self rangeValue];
            [coder encodeObject:[(id)numberClass numberWithUnsignedInteger:range.length]
                         forKey:(id)OF_STR("NS.rangeval.length")];
            [coder encodeObject:[(id)numberClass numberWithUnsignedInteger:range.location]
                         forKey:(id)OF_STR("NS.rangeval.location")];
            return;
        }
        case 10: {
            double transform[6] = {0};
            [self getValue:transform];
            [coder encodeDouble:transform[0] forKey:(id)OF_STR("NS.atval.a")];
            [coder encodeDouble:transform[1] forKey:(id)OF_STR("NS.atval.b")];
            [coder encodeDouble:transform[2] forKey:(id)OF_STR("NS.atval.c")];
            [coder encodeDouble:transform[3] forKey:(id)OF_STR("NS.atval.d")];
            [coder encodeDouble:transform[4] forKey:(id)OF_STR("NS.atval.tx")];
            [coder encodeDouble:transform[5] forKey:(id)OF_STR("NS.atval.ty")];
            return;
        }
        case 11:
        case 12: {
            double insets[4] = {0};
            [self getValue:insets];
            [coder encodeDouble:insets[0] forKey:(id)OF_STR("NS.edgeval.top")];
            [coder encodeDouble:insets[1] forKey:(id)OF_STR("NS.edgeval.left")];
            [coder encodeDouble:insets[2] forKey:(id)OF_STR("NS.edgeval.bottom")];
            [coder encodeDouble:insets[3] forKey:(id)OF_STR("NS.edgeval.right")];
            return;
        }
        case 13: {
            double offset[2] = {0};
            [self getValue:offset];
            [coder encodeDouble:offset[0] forKey:(id)OF_STR("NS.offset.h")];
            [coder encodeDouble:offset[1] forKey:(id)OF_STR("NS.offset.v")];
            return;
        }
        case 14: {
            double insets[4] = {0};
            [self getValue:insets];
            [coder encodeDouble:insets[0] forKey:(id)OF_STR("NS.dirEdgeVal.top")];
            [coder encodeDouble:insets[1] forKey:(id)OF_STR("NS.dirEdgeVal.leading")];
            [coder encodeDouble:insets[2] forKey:(id)OF_STR("NS.dirEdgeVal.bottom")];
            [coder encodeDouble:insets[3] forKey:(id)OF_STR("NS.dirEdgeVal.trailing")];
            return;
        }
        default: {
            id reason = [(id)stringClass
                stringWithFormat:(id)OF_STR("cannot encode special type '%ld'"), (long)_specialFlags];
            objc_exception_throw([(id)exceptionClass exceptionWithName:(id)ONSInternalInconsistencyException
                                                                reason:reason
                                                              userInfo:nil]);
        }
    }
}

@end

@implementation ONSValue

// Native +[NSValue allocWithZone:], 0x1c05f8d40. Non-external. NSValue itself (its self class reference,
// ___SCR_NSValue) answers the static placeholder; a subclass gets NSAllocateObject with no extra bytes.
+ (id)allocWithZone:(struct _NSZone *)zone {
    if (self == (Class)valueClass) {
        return (id)&__placeholderValue;
    }
    return ONSAllocateObject(self, 0, zone);
}

// Native -[NSValue init], 0x1c05f8d64. Non-external. An instance whose class is NSValue itself requests a
// concrete implementation and continues; then super's -init.
- (id)init {
    if (objc_opt_class(self) == (Class)valueClass) {
        ONSRequestConcreteImplementation(self, _cmd, (Class)valueClass);
    }
    return [super init];
}

// Native +[NSValue value:withObjCType:], 0x1c05f8dfc. Non-external; a tail call of
// +valueWithBytes:objCType:.
+ (id)value:(const void *)value withObjCType:(const char *)type {
    return [self valueWithBytes:value objCType:type];
}

// Native +[NSValue valueWithBytes:objCType:], 0x1c05f8e00. Non-external. __NSNewValue with no zone,
// autoreleased; the receiver is not consulted.
+ (id)valueWithBytes:(const void *)value objCType:(const char *)type {
    return objc_autorelease(_NSNewValue(value, type, NULL));
}

// Native -[NSValue getValue:], 0x1c05f8e20. Non-external. Unless the class overrides -getValue:size: and not
// -getValue: (compared through +instanceMethodForSelector: on the receiver's class and on NSValue), a
// concrete implementation is requested (a tail call); otherwise the size of -objCType goes to
// -getValue:size:.
- (void)getValue:(void *)value {
    Class cls = objc_opt_class(self);
    IMP sizedImplementation = [cls instanceMethodForSelector:@selector(getValue:size:)];
    if (sizedImplementation == [(Class)valueClass instanceMethodForSelector:@selector(getValue:size:)]) {
        ONSRequestConcreteImplementation(self, _cmd, (Class)valueClass);
        return;
    }
    IMP implementation = [cls instanceMethodForSelector:@selector(getValue:)];
    if (implementation != [(Class)valueClass instanceMethodForSelector:@selector(getValue:)]) {
        ONSRequestConcreteImplementation(self, _cmd, (Class)valueClass);
        return;
    }
    NSUInteger size = 0;
    ONSGetSizeAndAlignment([self objCType], &size, NULL);
    [self getValue:value size:size];
}

// Native -[NSValue getValue:size:], 0x1c05f8f5c. Non-external. Unless the class overrides -getValue: and not
// -getValue:size:, a concrete implementation is requested (a tail call); otherwise a size equal to that of
// -objCType goes to -getValue:, and any other raises NSInvalidArgumentException.
- (void)getValue:(void *)value size:(NSUInteger)size {
    Class cls = objc_opt_class(self);
    IMP implementation = [cls instanceMethodForSelector:@selector(getValue:)];
    if (implementation == [(Class)valueClass instanceMethodForSelector:@selector(getValue:)]) {
        ONSRequestConcreteImplementation(self, _cmd, (Class)valueClass);
        return;
    }
    IMP sizedImplementation = [cls instanceMethodForSelector:@selector(getValue:size:)];
    if (sizedImplementation != [(Class)valueClass instanceMethodForSelector:@selector(getValue:size:)]) {
        ONSRequestConcreteImplementation(self, _cmd, (Class)valueClass);
        return;
    }
    NSUInteger expectedSize = 0;
    const char *type = [self objCType];
    ONSGetSizeAndAlignment(type, &expectedSize, NULL);
    if (size != expectedSize) {
        id reason = [(id)stringClass
            stringWithFormat:
                (id)OF_STR(
                    "Cannot get value with size %zu. The type encoded as %s is expected to be %zu "
                    "bytes"
                ),
                size,
                type,
                expectedSize];
        objc_exception_throw([(id)exceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                            reason:reason
                                                          userInfo:nil]);
    }
    [self getValue:value];
}

// Native -[NSValue _matchType:size:], 0x1c05f90e8. Non-external; a tail call of -_matchType:size:strict:NO.
- (BOOL)_matchType:(const char *)type size:(NSUInteger)size {
    return [self _matchType:type size:size strict:NO];
}

// Native -[NSValue _matchType:size:strict:], 0x1c05f90f0. Non-external. The size of -objCType must equal the
// size; then a non-strict match succeeds, and a strict one compares the encodings (matchTypeEncoding, the
// receiver's encoding first).
- (BOOL)_matchType:(const char *)type size:(NSUInteger)size strict:(BOOL)strict {
    const char *ownType = [self objCType];
    NSUInteger ownSize = 0;
    ONSGetSizeAndAlignment(ownType, &ownSize, NULL);
    BOOL result = size != ownSize ? NO : !strict;
    if (strict && size == ownSize) {
        return matchTypeEncoding(ownType, type, size);
    }
    return result;
}

// Native -[NSValue copyWithZone:], 0x1c05f9194. Non-external. The bytes go through a stack buffer of 1024
// bytes, or a heap buffer (malloc type 0x100004077774924) above that, into __NSNewValue with the zone. Folded
// sizing-exception handler 0x1c05f9278 (null catch-all type entry); returns nil.
- (id)copyWithZone:(struct _NSZone *)zone {
    const char *type = [self objCType];
    NSUInteger size = 0;
    @try {
        ONSGetSizeAndAlignment(type, &size, NULL);
    } @catch (...) {
        return nil;
    }
    uint64_t stackBuffer[128];
    stackBuffer[0] = size;
    void *buffer = size < 0x401 ? (void *)stackBuffer : malloc_type_malloc(size, 0x100004077774924ULL);
    [self getValue:buffer];
    id copy = _NSNewValue(buffer, type, zone);
    if (size > 0x400) {
        free(buffer);
    }
    return copy;
}

// Native -[NSValue isEqualToValue:], 0x1c05f9290. Non-external. The same object is equal; otherwise the
// encodings must be equal strings and the value bytes (through two 1024-byte stack buffers, or two heap
// buffers above that) equal under memcmp. Folded sizing-exception handler 0x1c05f93e0 (null catch-all type
// entry); returns NO.
- (BOOL)isEqualToValue:(id)value {
    if (self == value) {
        return YES;
    }
    const char *type = [self objCType];
    if (strcmp(type, [value objCType]) != 0) {
        return NO;
    }
    NSUInteger size = 0;
    @try {
        ONSGetSizeAndAlignment(type, &size, NULL);
    } @catch (...) {
        return NO;
    }
    uint64_t stackBuffer1[128];
    stackBuffer1[0] = size;
    uint64_t stackBuffer2[128];
    void *buffer1;
    void *buffer2;
    if (size < 0x401) {
        buffer2 = stackBuffer2;
        buffer1 = stackBuffer1;
    } else {
        buffer1 = malloc_type_malloc(size, 0x100004077774924ULL);
        buffer2 = malloc_type_malloc(size, 0x100004077774924ULL);
    }
    [self getValue:buffer1];
    [value getValue:buffer2];
    BOOL equal = memcmp(buffer1, buffer2, size) == 0;
    if (size > 0x400) {
        free(buffer1);
        free(buffer2);
    }
    return equal;
}

// Native -[NSValue isEqual:], 0x1c05f93f4. Non-external. The same object is equal and nil is not; an object
// of exactly the cached NSConcreteValue class, or any object _NSIsNSValue accepts, goes to -isEqualToValue:.
- (BOOL)isEqual:(id)object {
    if (self == object) {
        return YES;
    }
    if (object == nil) {
        return NO;
    }
    if (objc_opt_class(object) ==
        (Class)atomic_load_explicit(&__NSConcreteValueClass, memory_order_relaxed)) {
        return [self isEqualToValue:object];
    }
    if (_ONSIsNSValue(object)) {
        return [self isEqualToValue:object];
    }
    return NO;
}

// Native -[NSValue hash], 0x1c05f9464. Non-external. Up to seven bytes hash as a big-endian number; longer
// values XOR their first min(size, 32) / 8 eight-byte words. Values above 1024 bytes are read into a heap
// buffer. Folded sizing-exception handler 0x1c05f9580 (null catch-all type entry); returns 0.
- (NSUInteger)hash {
    const char *type = [self objCType];
    NSUInteger size = 0;
    @try {
        ONSGetSizeAndAlignment(type, &size, NULL);
    } @catch (...) {
        return 0;
    }
    uint64_t stackBuffer[128];
    stackBuffer[0] = size;
    const uint64_t *words;
    if (size >= 0x401) {
        uint64_t *heapBuffer = malloc_type_malloc(size, 0x100004077774924ULL);
        [self getValue:heapBuffer];
        words = heapBuffer;
    } else {
        [self getValue:stackBuffer];
        if (size <= 7) {
            NSUInteger hash = 0;
            const uint8_t *bytes = (const uint8_t *)stackBuffer;
            while (size != 0) {
                hash = *bytes++ | (hash << 8);
                size--;
            }
            return hash;
        }
        words = stackBuffer;
    }
    NSUInteger hash = 0;
    NSUInteger remaining = (size < 0x20 ? size : 0x20) + 8;
    const uint64_t *word = words;
    do {
        hash ^= *word++;
        remaining -= 8;
    } while (remaining > 15);
    if (size >= 0x401) {
        free((void *)words);
    }
    return hash;
}

// Native -[NSValue description], 0x1c05f9598. Non-external. The bytes are read into an NSMutableData of the
// encoding's size (objc_allocWithZone, -initWithLength:), whose -description is answered; the data is
// released (not autoreleased). Folded sizing-exception handler 0x1c05f963c (null catch-all type entry);
// returns nil.
- (id)description {
    const char *type = [self objCType];
    NSUInteger size = 0;
    @try {
        ONSGetSizeAndAlignment(type, &size, NULL);
    } @catch (...) {
        return nil;
    }
    id data = [objc_allocWithZone((Class)mutableDataClass) initWithLength:size];
    [self getValue:[data mutableBytes]];
    id description = [data description];
    objc_release(data);
    return description;
}

// Native +[NSValue valueWithWeakObject:], 0x1c05f96b4. Non-external. An NSWeakObjectValue (objc_alloc,
// -initWithObject:), autoreleased.
+ (id)valueWithWeakObject:(id)object {
    return objc_autorelease([objc_alloc((Class)weakObjectValueClass) initWithObject:object]);
}

// Native -[NSValue nonretainedObjectValue], 0x1c05f96e4. Non-external. A value whose encoding is the size of
// "^v" (8 bytes, non-strict) answers its pointer word; any other nil.
- (id)nonretainedObjectValue {
    if (![self _matchType:"^v" size:8]) {
        return nil;
    }
    id object = nil;
    [self getValue:&object];
    return object;
}

// Native -[NSValue weakObjectValue], 0x1c05f9764. Non-external. A value holds no weak object: nil.
- (id)weakObjectValue {
    return nil;
}

// Native +[NSValue valueWithPoint:], 0x1c05f984c. Non-external. +valueWithBytes:objCType: sent to NSValue
// (not the receiver) with "{CGPoint=dd}".
+ (id)valueWithPoint:(ValuePoint)point {
    ValuePoint bytes = point;
    return [(id)valueClass valueWithBytes:&bytes objCType:"{CGPoint=dd}"];
}

// Native +[NSValue valueWithSize:], 0x1c05f98ac. Non-external. As +valueWithPoint: with "{CGSize=dd}".
+ (id)valueWithSize:(ValueSize)size {
    ValueSize bytes = size;
    return [(id)valueClass valueWithBytes:&bytes objCType:"{CGSize=dd}"];
}

// Native +[NSValue valueWithRect:], 0x1c05f990c. Non-external. As +valueWithPoint: with
// "{CGRect={CGPoint=dd}{CGSize=dd}}".
+ (id)valueWithRect:(ValueRect)rect {
    ValueRect bytes = rect;
    return [(id)valueClass valueWithBytes:&bytes objCType:"{CGRect={CGPoint=dd}{CGSize=dd}}"];
}

// Native +[NSValue valueWithRange:], 0x1c05f9970. Non-external. As +valueWithPoint: with "{_NSRange=QQ}".
+ (id)valueWithRange:(ONSRange)range {
    ONSRange bytes = range;
    return [(id)valueClass valueWithBytes:&bytes objCType:"{_NSRange=QQ}"];
}

// Native -[NSValue pointValue], 0x1c05f9a34. Non-external. A strict match of "{CGPoint=dd}" (16 bytes) reads
// the point; otherwise zero.
- (ValuePoint)pointValue {
    ValuePoint result = {0.0, 0.0};
    if ([self _matchType:"{CGPoint=dd}" size:16 strict:YES]) {
        ValuePoint point = {0.0, 0.0};
        [self getValue:&point];
        result = point;
    }
    return result;
}

// Native -[NSValue sizeValue], 0x1c05f9ab8. Non-external. As -pointValue with "{CGSize=dd}".
- (ValueSize)sizeValue {
    ValueSize result = {0.0, 0.0};
    if ([self _matchType:"{CGSize=dd}" size:16 strict:YES]) {
        ValueSize size = {0.0, 0.0};
        [self getValue:&size];
        result = size;
    }
    return result;
}

// Native -[NSValue rectValue], 0x1c05f9b3c. Non-external. As -pointValue with
// "{CGRect={CGPoint=dd}{CGSize=dd}}" (32 bytes).
- (ValueRect)rectValue {
    ValueRect result = {{0.0, 0.0}, {0.0, 0.0}};
    if ([self _matchType:"{CGRect={CGPoint=dd}{CGSize=dd}}" size:32 strict:YES]) {
        ValueRect rect = {{0.0, 0.0}, {0.0, 0.0}};
        [self getValue:&rect];
        result = rect;
    }
    return result;
}

// Native -[NSValue rangeValue], 0x1c05f9bd0. Non-external. As -pointValue with "{_NSRange=QQ}".
- (ONSRange)rangeValue {
    if ([self _matchType:"{_NSRange=QQ}" size:16 strict:YES]) {
        ONSRange range = {0, 0};
        [self getValue:&range];
        return range;
    }
    ONSRange empty = {0, 0};
    return empty;
}

// Native -[NSValue classForCoder], 0x1c05f9cec. Non-external. NSValue (its self class reference).
- (Class)classForCoder {
    return (Class)valueClass;
}

// Native -[NSValue encodeWithCoder:], 0x1c05f9cf8. Non-external. The encoding is sized (an exception there is
// replaced by NSInvalidArgumentException "cannot encode '%s'-typed value: %@"), a buffer chosen (1024 stack
// bytes or a heap buffer), a "^v" encoding raises NSInvalidArgumentException "cannot encode (void *) value:
// %@", and the bytes are read and encoded as the encoding string ("*") followed by the value. Folded
// catch-cleanup/termination range 0x1c05f9e90-0x1c05f9eaf, read with the native LSDA.
- (void)encodeWithCoder:(id)coder {
    const char *type = [self objCType];
    const char *typeReference = type;
    NSUInteger size = 0;
    @try {
        ONSGetSizeAndAlignment(type, &size, NULL);
    } @catch (...) {
        id reason =
            [(id)stringClass stringWithFormat:(id)OF_STR("cannot encode '%s'-typed value: %@"), type, self];
        objc_exception_throw([(id)exceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                            reason:reason
                                                          userInfo:nil]);
    }
    uint64_t stackBuffer[128];
    stackBuffer[0] = size;
    void *buffer = size < 0x401 ? (void *)stackBuffer : malloc_type_malloc(size, 0x100004077774924ULL);
    if (type[0] == '^' && type[1] == 'v') {
        id reason = [(id)stringClass stringWithFormat:(id)OF_STR("cannot encode (void *) value: %@"), self];
        objc_exception_throw([(id)exceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                            reason:reason
                                                          userInfo:nil]);
    }
    BOOL heap = size > 0x400;
    [self getValue:buffer];
    [coder encodeValueOfObjCType:"*" at:&typeReference];
    [coder encodeValueOfObjCType:typeReference at:buffer];
    if (heap) {
        free(buffer);
    }
}

// Native +[NSValue supportsSecureCoding], 0x1c05f9eb0. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSValue initWithCoder:], 0x1c05f9eb8. Non-external. The receiver's zone is read, the receiver
// released, and newDecodedValue answers (a tail call).
- (id)initWithCoder:(id)coder {
    struct _NSZone *zone = [self zone];
    objc_release(self);
    return newDecodedValue(coder, zone);
}

// Native -[NSValue objCType], 0x1c0fe7098. Non-external. Requests a concrete implementation of NSValue and
// answers NULL.
- (const char *)objCType {
    ONSRequestConcreteImplementation(self, _cmd, (Class)valueClass);
    return NULL;
}

// Native -[NSValue initWithBytes:objCType:], 0x1c0fe70b8. Non-external. Requests a concrete object and
// answers the receiver.
- (id)initWithBytes:(const void *)value objCType:(const char *)type {
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

@end

// Native newDecodedValue, 0x1c05f9ef4. Non-external. A keyed coder with "NS.special" decodes that int: 1
// point, 2 size, 3 rect (NSValue's +valueWith...: of the coder's -decode...ForKey:, retained), 4 range
// (length, then location, each an NSNumber object's -unsignedIntegerValue), 10 affine transform, 11
// UIEdgeInsets, 12 NSEdgeInsets, 13 UIOffset and 14 NSDirectionalEdgeInsets (doubles into a stack record,
// __NSNewValue with no zone); any other raises NSInternalInconsistencyException "cannot decode special type
// '%lu'" with the value sign-extended. Otherwise the encoding string is decoded ("*", 8 bytes); NULL or empty
// answers nil, and the value is decoded into a zeroed 1024-byte stack buffer, or a calloc'ed one above that
// (nil if that fails), and passed to __NSNewValue with the zone; the heap buffer and the encoding string are
// freed. An exception from sizing the encoding answers nil. Foundation defect: newDecodedValue 0x1c05f9ef4
// leaks the decoded encoding at the empty-string branch 0x1c05fa014, allocation failure 0x1c05fa054 and
// sizing-exception landing pad 0x1c05fa3f4 (confidence: verified). The restoration frees that encoding on
// these exits.
static id newDecodedValue(id coder, struct _NSZone *zone) {
    double record[128];
    if ([coder allowsKeyedCoding] && [coder containsValueForKey:(id)OF_STR("NS.special")]) {
        int special = [coder decodeIntForKey:(id)OF_STR("NS.special")];
        switch (special) {
            case 1: {
                ValuePoint point = [coder decodePointForKey:(id)OF_STR("NS.pointval")];
                return objc_retain([(id)valueClass valueWithPoint:point]);
            }
            case 2: {
                ValueSize size = [coder decodeSizeForKey:(id)OF_STR("NS.sizeval")];
                return objc_retain([(id)valueClass valueWithSize:size]);
            }
            case 3: {
                ValueRect rect = [coder decodeRectForKey:(id)OF_STR("NS.rectval")];
                return objc_retain([(id)valueClass valueWithRect:rect]);
            }
            case 4: {
                NSUInteger length =
                    [[coder decodeObjectOfClass:objc_opt_class((id)numberClass)
                                         forKey:(id)OF_STR("NS.rangeval.length")] unsignedIntegerValue];
                NSUInteger location =
                    [[coder decodeObjectOfClass:objc_opt_class((id)numberClass)
                                         forKey:(id)OF_STR("NS.rangeval.location")] unsignedIntegerValue];
                ONSRange range = {location, length};
                return objc_retain([(id)valueClass valueWithRange:range]);
            }
            case 10:
                record[0] = [coder decodeDoubleForKey:(id)OF_STR("NS.atval.a")];
                record[1] = [coder decodeDoubleForKey:(id)OF_STR("NS.atval.b")];
                record[2] = [coder decodeDoubleForKey:(id)OF_STR("NS.atval.c")];
                record[3] = [coder decodeDoubleForKey:(id)OF_STR("NS.atval.d")];
                record[4] = [coder decodeDoubleForKey:(id)OF_STR("NS.atval.tx")];
                record[5] = [coder decodeDoubleForKey:(id)OF_STR("NS.atval.ty")];
                return _NSNewValue(record, "{CGAffineTransform=dddddd}", NULL);
            case 11:
                record[0] = [coder decodeDoubleForKey:(id)OF_STR("NS.edgeval.top")];
                record[1] = [coder decodeDoubleForKey:(id)OF_STR("NS.edgeval.left")];
                record[2] = [coder decodeDoubleForKey:(id)OF_STR("NS.edgeval.bottom")];
                record[3] = [coder decodeDoubleForKey:(id)OF_STR("NS.edgeval.right")];
                return _NSNewValue(record, "{UIEdgeInsets=dddd}", NULL);
            case 12:
                record[0] = [coder decodeDoubleForKey:(id)OF_STR("NS.edgeval.top")];
                record[1] = [coder decodeDoubleForKey:(id)OF_STR("NS.edgeval.left")];
                record[2] = [coder decodeDoubleForKey:(id)OF_STR("NS.edgeval.bottom")];
                record[3] = [coder decodeDoubleForKey:(id)OF_STR("NS.edgeval.right")];
                return _NSNewValue(record, "{NSEdgeInsets=dddd}", NULL);
            case 13:
                record[0] = [coder decodeDoubleForKey:(id)OF_STR("NS.offset.h")];
                record[1] = [coder decodeDoubleForKey:(id)OF_STR("NS.offset.v")];
                return _NSNewValue(record, "{UIOffset=dd}", NULL);
            case 14:
                record[0] = [coder decodeDoubleForKey:(id)OF_STR("NS.dirEdgeVal.top")];
                record[1] = [coder decodeDoubleForKey:(id)OF_STR("NS.dirEdgeVal.leading")];
                record[2] = [coder decodeDoubleForKey:(id)OF_STR("NS.dirEdgeVal.bottom")];
                record[3] = [coder decodeDoubleForKey:(id)OF_STR("NS.dirEdgeVal.trailing")];
                return _NSNewValue(record, "{NSDirectionalEdgeInsets=dddd}", NULL);
            default: {
                id reason = [(id)stringClass
                    stringWithFormat:(id)OF_STR("cannot decode special type '%lu'"), (long)special];
                objc_exception_throw([(id)exceptionClass
                    exceptionWithName:(id)ONSInternalInconsistencyException
                               reason:reason
                             userInfo:nil]);
            }
        }
    }
    bzero(record, 0x400);
    char *typePtr = NULL;
    [coder decodeValueOfObjCType:"*" at:&typePtr size:8];
    if (typePtr == NULL) {
        return nil;
    }
    if (*typePtr == 0) {
        free(typePtr);
        return nil;
    }
    NSUInteger size = 0;
    @try {
        ONSGetSizeAndAlignment(typePtr, &size, NULL);
    } @catch (...) {
        free(typePtr);
        return nil;
    }
    void *buffer;
    if (size < 0x401) {
        buffer = record;
    } else {
        buffer = malloc_type_calloc(size, 1, 0x100004077774924ULL);
        if (buffer == NULL) {
            free(typePtr);
            return nil;
        }
    }
    [coder decodeValueOfObjCType:typePtr at:buffer size:size];
    id value = _NSNewValue(buffer, typePtr, zone);
    if (buffer != record) {
        free(buffer);
    }
    free(typePtr);
    return value;
}

// The type characters every NSNumber accessor dispatches on, in the native comparison tree: 'B' and 'c' read
// -charValue, 'C' -unsignedCharValue, 's' -shortValue, 'S' -unsignedShortValue, 'i' -intValue, 'I'
// -unsignedIntValue, 'l' -longValue, 'L' -unsignedLongValue, 'q' -longLongValue, 'Q' -unsignedLongLongValue,
// 'f' -floatValue and 'd' -doubleValue; any other raises the bad-objCType exception. Each result is converted
// with its native integer conversion, floating values to the narrow integer types through
// int (FCVTZS W) as natively. A receiver whose type matches the accessor sends the same accessor again (a
// tail call), as natively, so an abstract subclass that overrides only -objCType recurses.

@implementation ONSNumber

// Native +[NSNumber initialize], 0x1c05fa40c. Non-external. For NSNumber itself, tagged numbers are disabled
// when libobjc has no tagged pointers (objc_debug_taggedpointer_mask zero) or the executable is not linked on
// or after version 7.
+ (void)initialize {
    if (self == (Class)numberClass &&
        (objc_debug_taggedpointer_mask == 0 || !_OCFExecutableLinkedOnOrAfter(7))) {
        _NSNumberTaggedPointersDisabled = 1;
    }
}

// Native +[NSNumber allocWithZone:], 0x1c05fa458. Non-external. NSNumber itself answers the static
// placeholder; a subclass gets NSAllocateObject with no extra bytes.
+ (id)allocWithZone:(struct _NSZone *)zone {
    if (self == (Class)numberClass) {
        return (id)&__placeholderNumber;
    }
    return ONSAllocateObject(self, 0, zone);
}

// Native +[NSNumber numberWithChar:], 0x1c05fa480. Non-external. For NSNumber with tagged numbers enabled, a
// tagged number (the sign-extended value shifted left 7, tag bits 0x8000000000000003); otherwise
// objc_allocWithZone of the receiver, -initWithChar:, autoreleased.
+ (id)numberWithChar:(char)value {
    if (self != (Class)numberClass || (_NSNumberTaggedPointersDisabled & 1) != 0) {
        return objc_autorelease([objc_allocWithZone((Class)self) initWithChar:value]);
    }
    return encodeTaggedNumber(((uintptr_t)(intptr_t)value << 7) | UINT64_C(0x8000000000000003));
}

// Native +[NSNumber numberWithUnsignedChar:], 0x1c05fa510. Non-external. As +numberWithChar: with the
// zero-extended value and tag bits 0x800000000000000B.
+ (id)numberWithUnsignedChar:(unsigned char)value {
    if (self != (Class)numberClass || (_NSNumberTaggedPointersDisabled & 1) != 0) {
        return objc_autorelease([objc_allocWithZone((Class)self) initWithUnsignedChar:value]);
    }
    return encodeTaggedNumber(((uintptr_t)value << 7) | UINT64_C(0x800000000000000B));
}

// Native +[NSNumber numberWithShort:], 0x1c05fa5b0. Non-external. Sign-extended value, tag bits
// 0x800000000000000B.
+ (id)numberWithShort:(short)value {
    if (self != (Class)numberClass || (_NSNumberTaggedPointersDisabled & 1) != 0) {
        return objc_autorelease([objc_allocWithZone((Class)self) initWithShort:value]);
    }
    return encodeTaggedNumber(((uintptr_t)(intptr_t)value << 7) | UINT64_C(0x800000000000000B));
}

// Native +[NSNumber numberWithUnsignedShort:], 0x1c05fa650. Non-external. Zero-extended value, tag bits
// 0x8000000000000013.
+ (id)numberWithUnsignedShort:(unsigned short)value {
    if (self != (Class)numberClass || (_NSNumberTaggedPointersDisabled & 1) != 0) {
        return objc_autorelease([objc_allocWithZone((Class)self) initWithUnsignedShort:value]);
    }
    return encodeTaggedNumber(((uintptr_t)value << 7) | UINT64_C(0x8000000000000013));
}

// Native +[NSNumber numberWithInt:], 0x1c05fa6f0. Non-external. Sign-extended value, tag bits
// 0x8000000000000013.
+ (id)numberWithInt:(int)value {
    if (self != (Class)numberClass || (_NSNumberTaggedPointersDisabled & 1) != 0) {
        return objc_autorelease([objc_allocWithZone((Class)self) initWithInt:value]);
    }
    return encodeTaggedNumber(((uintptr_t)(intptr_t)value << 7) | UINT64_C(0x8000000000000013));
}

// Native +[NSNumber numberWithUnsignedInt:], 0x1c05fa790. Non-external. Zero-extended value, tag bits
// 0x800000000000001B.
+ (id)numberWithUnsignedInt:(unsigned int)value {
    if (self != (Class)numberClass || (_NSNumberTaggedPointersDisabled & 1) != 0) {
        return objc_autorelease([objc_allocWithZone((Class)self) initWithUnsignedInt:value]);
    }
    return encodeTaggedNumber(((uintptr_t)value << 7) | UINT64_C(0x800000000000001B));
}

// Native +[NSNumber numberWithInteger:], 0x1c05fa830. Non-external. Tagged only for values in
// [-(2^55 - 1), 2^55 - 1] (value + 0xFF80000000000000 unsigned at least 0xFF00000000000001); tag bits
// 0x800000000000001B.
+ (id)numberWithInteger:(NSInteger)value {
    if ((uint64_t)value + UINT64_C(0xFF80000000000000) < UINT64_C(0xFF00000000000001) ||
        self != (Class)numberClass || (_NSNumberTaggedPointersDisabled & 1) != 0) {
        return objc_autorelease([objc_allocWithZone((Class)self) initWithInteger:value]);
    }
    return encodeTaggedNumber(((uintptr_t)value << 7) | UINT64_C(0x800000000000001B));
}

// Native +[NSNumber numberWithUnsignedInteger:], 0x1c05fa8d0. Non-external. Tagged only for values below
// 2^55; tag bits 0x800000000000001B.
+ (id)numberWithUnsignedInteger:(NSUInteger)value {
    if ((OF_WORD_SHIFT(value, 55)) != 0 || self != (Class)numberClass ||
        (_NSNumberTaggedPointersDisabled & 1) != 0) {
        return objc_autorelease([objc_allocWithZone((Class)self) initWithUnsignedInteger:value]);
    }
    return encodeTaggedNumber(((uintptr_t)value << 7) | UINT64_C(0x800000000000001B));
}

// Native +[NSNumber numberWithLong:], 0x1c05fa970. Non-external. As +numberWithInteger:.
+ (id)numberWithLong:(long)value {
    if ((uint64_t)value + UINT64_C(0xFF80000000000000) < UINT64_C(0xFF00000000000001) ||
        self != (Class)numberClass || (_NSNumberTaggedPointersDisabled & 1) != 0) {
        return objc_autorelease([objc_allocWithZone((Class)self) initWithLong:value]);
    }
    return encodeTaggedNumber(((uintptr_t)value << 7) | UINT64_C(0x800000000000001B));
}

// Native +[NSNumber numberWithUnsignedLong:], 0x1c05faa10. Non-external. As +numberWithUnsignedInteger:.
+ (id)numberWithUnsignedLong:(unsigned long)value {
    if ((OF_WORD_SHIFT(value, 55)) != 0 || self != (Class)numberClass ||
        (_NSNumberTaggedPointersDisabled & 1) != 0) {
        return objc_autorelease([objc_allocWithZone((Class)self) initWithUnsignedLong:value]);
    }
    return encodeTaggedNumber(((uintptr_t)value << 7) | UINT64_C(0x800000000000001B));
}

// Native +[NSNumber numberWithLongLong:], 0x1c05faab0. Non-external. As +numberWithInteger:.
+ (id)numberWithLongLong:(long long)value {
    if ((uint64_t)value + UINT64_C(0xFF80000000000000) < UINT64_C(0xFF00000000000001) ||
        self != (Class)numberClass || (_NSNumberTaggedPointersDisabled & 1) != 0) {
        return objc_autorelease([objc_allocWithZone((Class)self) initWithLongLong:value]);
    }
    return encodeTaggedNumber(((uintptr_t)value << 7) | UINT64_C(0x800000000000001B));
}

// Native +[NSNumber numberWithUnsignedLongLong:], 0x1c05fab50. Non-external. As +numberWithUnsignedInteger:.
+ (id)numberWithUnsignedLongLong:(unsigned long long)value {
    if ((value >> 55) != 0 || self != (Class)numberClass || (_NSNumberTaggedPointersDisabled & 1) != 0) {
        return objc_autorelease([objc_allocWithZone((Class)self) initWithUnsignedLongLong:value]);
    }
    return encodeTaggedNumber(((uintptr_t)value << 7) | UINT64_C(0x800000000000001B));
}

// Native +[NSNumber numberWithFloat:], 0x1c05fabe8. Non-external. For NSNumber with tagged numbers enabled, a
// value that converts exactly to a 64-bit integer (FCVTZS, then SCVTF back and compared; NaN fails) is tagged
// (tag bits 0x8000000000000023) when that integer is nonzero and within [-(2^55 - 1), 2^55 - 1], or zero with
// a clear sign bit; -0.0 and every other value get objc_allocWithZone, -initWithFloat:, autoreleased.
+ (id)numberWithFloat:(float)value {
    if (self == (Class)numberClass && (_NSNumberTaggedPointersDisabled & 1) == 0) {
        int64_t integer = floatToInt64(value);
        if (value == (float)integer) {
            uint32_t bits;
            memcpy(&bits, &value, sizeof(bits));
            if (integer != 0
                    ? (uint64_t)integer + UINT64_C(0xFF80000000000000) >= UINT64_C(0xFF00000000000001)
                    : (bits >> 31) == 0) {
                return encodeTaggedNumber(((uintptr_t)integer << 7) | UINT64_C(0x8000000000000023));
            }
        }
    }
    return objc_autorelease([objc_allocWithZone((Class)self) initWithFloat:value]);
}

// Native +[NSNumber numberWithDouble:], 0x1c05faca8. Non-external. As +numberWithFloat: in double (the
// exactness test converts in the vector register, FCVTZS D then SCVTF D), tag bits 0x800000000000002B;
// -initWithDouble: otherwise.
+ (id)numberWithDouble:(double)value {
    if (self == (Class)numberClass && (_NSNumberTaggedPointersDisabled & 1) == 0) {
        int64_t integer = doubleToInt64(value);
        if (value == (double)integer) {
            uint64_t bits;
            memcpy(&bits, &value, sizeof(bits));
            if (integer != 0
                    ? (uint64_t)integer + UINT64_C(0xFF80000000000000) >= UINT64_C(0xFF00000000000001)
                    : (bits >> 63) == 0) {
                return encodeTaggedNumber(((uintptr_t)integer << 7) | UINT64_C(0x800000000000002B));
            }
        }
    }
    return objc_autorelease([objc_allocWithZone((Class)self) initWithDouble:value]);
}

// Native +[NSNumber numberWithBool:], 0x1c05fad6c. Non-external. The CFBoolean constants, neither retained
// nor autoreleased; the receiver is not consulted.
+ (id)numberWithBool:(BOOL)value {
    return (id)(value ? kOCFBooleanTrue : kOCFBooleanFalse);
}

// Native -[NSNumber boolValue], 0x1c05fad8c. Non-external. The integer types ('B', 'C', 'I', 'L', 'S', 'c',
// 'i', 'l', 's') test -longValue, 'Q' and 'q' -longLongValue, 'd' and 'f' -doubleValue (unequal to 0.0, so
// NaN is YES); any other type raises the bad-objCType exception.
- (BOOL)boolValue {
    switch (*[self objCType]) {
        case 'B':
        case 'C':
        case 'I':
        case 'L':
        case 'S':
        case 'c':
        case 'i':
        case 'l':
        case 's':
            return [self longValue] != 0;
        case 'd':
        case 'f':
            return [self doubleValue] != 0.0;
        case 'Q':
        case 'q':
            return [self longLongValue] != 0;
        default:
            raiseBadObjCType(self, _cmd);
    }
}

// Native -[NSNumber integerValue], 0x1c05fae94. Non-external; a tail call of -longValue.
- (NSInteger)integerValue {
    return [self longValue];
}

// Native -[NSNumber unsignedIntegerValue], 0x1c05fae98. Non-external; a tail call of -unsignedLongValue.
- (NSUInteger)unsignedIntegerValue {
    return [self unsignedLongValue];
}

// Native -[NSNumber charValue], 0x1c05fae9c. Non-external. See the accessor note above.
- (char)charValue {
    switch (*[self objCType]) {
        case 'B':
        case 'c':
            return [self charValue];
        case 'C':
            return (char)[self unsignedCharValue];
        case 'I':
            return (char)[self unsignedIntValue];
        case 'L':
            return (char)[self unsignedLongValue];
        case 'Q':
            return (char)[self unsignedLongLongValue];
        case 'S':
            return (char)[self unsignedShortValue];
        case 'd':
            return (char)doubleToInt32([self doubleValue]);
        case 'f':
            return (char)floatToInt32([self floatValue]);
        case 'i':
            return (char)[self intValue];
        case 'l':
            return (char)[self longValue];
        case 'q':
            return (char)[self longLongValue];
        case 's':
            return (char)[self shortValue];
        default:
            raiseBadObjCType(self, _cmd);
    }
}

// Native -[NSNumber unsignedCharValue], 0x1c05fb058. Non-external. See the accessor note above.
- (unsigned char)unsignedCharValue {
    switch (*[self objCType]) {
        case 'B':
        case 'c':
            return (unsigned char)[self charValue];
        case 'C':
            return [self unsignedCharValue];
        case 'I':
            return (unsigned char)[self unsignedIntValue];
        case 'L':
            return (unsigned char)[self unsignedLongValue];
        case 'Q':
            return (unsigned char)[self unsignedLongLongValue];
        case 'S':
            return (unsigned char)[self unsignedShortValue];
        case 'd':
            return (unsigned char)doubleToInt32([self doubleValue]);
        case 'f':
            return (unsigned char)floatToInt32([self floatValue]);
        case 'i':
            return (unsigned char)[self intValue];
        case 'l':
            return (unsigned char)[self longValue];
        case 'q':
            return (unsigned char)[self longLongValue];
        case 's':
            return (unsigned char)[self shortValue];
        default:
            raiseBadObjCType(self, _cmd);
    }
}

// Native -[NSNumber shortValue], 0x1c05fb214. Non-external. See the accessor note above.
- (short)shortValue {
    switch (*[self objCType]) {
        case 'B':
        case 'c':
            return (short)[self charValue];
        case 'C':
            return (short)[self unsignedCharValue];
        case 'I':
            return (short)[self unsignedIntValue];
        case 'L':
            return (short)[self unsignedLongValue];
        case 'Q':
            return (short)[self unsignedLongLongValue];
        case 'S':
            return (short)[self unsignedShortValue];
        case 'd':
            return (short)doubleToInt32([self doubleValue]);
        case 'f':
            return (short)floatToInt32([self floatValue]);
        case 'i':
            return (short)[self intValue];
        case 'l':
            return (short)[self longValue];
        case 'q':
            return (short)[self longLongValue];
        case 's':
            return [self shortValue];
        default:
            raiseBadObjCType(self, _cmd);
    }
}

// Native -[NSNumber unsignedShortValue], 0x1c05fb3d0. Non-external. See the accessor note above.
- (unsigned short)unsignedShortValue {
    switch (*[self objCType]) {
        case 'B':
        case 'c':
            return (unsigned short)[self charValue];
        case 'C':
            return (unsigned short)[self unsignedCharValue];
        case 'I':
            return (unsigned short)[self unsignedIntValue];
        case 'L':
            return (unsigned short)[self unsignedLongValue];
        case 'Q':
            return (unsigned short)[self unsignedLongLongValue];
        case 'S':
            return [self unsignedShortValue];
        case 'd':
            return (unsigned short)doubleToInt32([self doubleValue]);
        case 'f':
            return (unsigned short)floatToInt32([self floatValue]);
        case 'i':
            return (unsigned short)[self intValue];
        case 'l':
            return (unsigned short)[self longValue];
        case 'q':
            return (unsigned short)[self longLongValue];
        case 's':
            return (unsigned short)[self shortValue];
        default:
            raiseBadObjCType(self, _cmd);
    }
}

// Native -[NSNumber intValue], 0x1c05fb58c. Non-external. See the accessor note above.
- (int)intValue {
    switch (*[self objCType]) {
        case 'B':
        case 'c':
            return (int)[self charValue];
        case 'C':
            return (int)[self unsignedCharValue];
        case 'I':
            return (int)[self unsignedIntValue];
        case 'L':
            return (int)[self unsignedLongValue];
        case 'Q':
            return (int)[self unsignedLongLongValue];
        case 'S':
            return (int)[self unsignedShortValue];
        case 'd':
            return doubleToInt32([self doubleValue]);
        case 'f':
            return floatToInt32([self floatValue]);
        case 'i':
            return [self intValue];
        case 'l':
            return (int)[self longValue];
        case 'q':
            return (int)[self longLongValue];
        case 's':
            return (int)[self shortValue];
        default:
            raiseBadObjCType(self, _cmd);
    }
}

// Native -[NSNumber unsignedIntValue], 0x1c05fb750. Non-external. See the accessor note above; floating
// values convert unsigned (FCVTZU W).
- (unsigned int)unsignedIntValue {
    switch (*[self objCType]) {
        case 'B':
        case 'c':
            return (unsigned int)[self charValue];
        case 'C':
            return (unsigned int)[self unsignedCharValue];
        case 'I':
            return [self unsignedIntValue];
        case 'L':
            return (unsigned int)[self unsignedLongValue];
        case 'Q':
            return (unsigned int)[self unsignedLongLongValue];
        case 'S':
            return (unsigned int)[self unsignedShortValue];
        case 'd':
            return doubleToUInt32([self doubleValue]);
        case 'f':
            return floatToUInt32([self floatValue]);
        case 'i':
            return (unsigned int)[self intValue];
        case 'l':
            return (unsigned int)[self longValue];
        case 'q':
            return (unsigned int)[self longLongValue];
        case 's':
            return (unsigned int)[self shortValue];
        default:
            raiseBadObjCType(self, _cmd);
    }
}

// Native -[NSNumber longValue], 0x1c05fb914. Non-external. See the accessor note above; floating values
// convert signed in 64 bits (FCVTZS X).
- (long)longValue {
    switch (*[self objCType]) {
        case 'B':
        case 'c':
            return (long)[self charValue];
        case 'C':
            return (long)[self unsignedCharValue];
        case 'I':
            return (long)[self unsignedIntValue];
        case 'L':
            return (long)[self unsignedLongValue];
        case 'Q':
            return (long)[self unsignedLongLongValue];
        case 'S':
            return (long)[self unsignedShortValue];
        case 'd':
            return (long)doubleToInt64([self doubleValue]);
        case 'f':
            return (long)floatToInt64([self floatValue]);
        case 'i':
            return (long)[self intValue];
        case 'l':
            return [self longValue];
        case 'q':
            return (long)[self longLongValue];
        case 's':
            return (long)[self shortValue];
        default:
            raiseBadObjCType(self, _cmd);
    }
}

// Native -[NSNumber unsignedLongValue], 0x1c05fbaf8. Non-external. See the accessor note above; floating
// values convert unsigned in 64 bits (FCVTZU X).
- (unsigned long)unsignedLongValue {
    switch (*[self objCType]) {
        case 'B':
        case 'c':
            return (unsigned long)[self charValue];
        case 'C':
            return (unsigned long)[self unsignedCharValue];
        case 'I':
            return (unsigned long)[self unsignedIntValue];
        case 'L':
            return [self unsignedLongValue];
        case 'Q':
            return (unsigned long)[self unsignedLongLongValue];
        case 'S':
            return (unsigned long)[self unsignedShortValue];
        case 'd':
            return (unsigned long)doubleToUInt64([self doubleValue]);
        case 'f':
            return (unsigned long)floatToUInt64([self floatValue]);
        case 'i':
            return (unsigned long)[self intValue];
        case 'l':
            return (unsigned long)[self longValue];
        case 'q':
            return (unsigned long)[self longLongValue];
        case 's':
            return (unsigned long)[self shortValue];
        default:
            raiseBadObjCType(self, _cmd);
    }
}

// Native -[NSNumber doubleValue], 0x1c05fbcdc. Non-external. See the accessor note above (SCVTF for the
// signed types, UCVTF for the unsigned ones, FCVT for float).
- (double)doubleValue {
    switch (*[self objCType]) {
        case 'B':
        case 'c':
            return (double)[self charValue];
        case 'C':
            return (double)[self unsignedCharValue];
        case 'I':
            return (double)[self unsignedIntValue];
        case 'L':
            return (double)[self unsignedLongValue];
        case 'Q':
            return (double)[self unsignedLongLongValue];
        case 'S':
            return (double)[self unsignedShortValue];
        case 'd':
            return [self doubleValue];
        case 'f':
            return (double)[self floatValue];
        case 'i':
            return (double)[self intValue];
        case 'l':
            return (double)[self longValue];
        case 'q':
            return (double)[self longLongValue];
        case 's':
            return (double)[self shortValue];
        default:
            raiseBadObjCType(self, _cmd);
    }
}

// Native -[NSNumber floatValue], 0x1c05fbea0. Non-external. See the accessor note above (SCVTF S, UCVTF S,
// and FCVT S from double).
- (float)floatValue {
    switch (*[self objCType]) {
        case 'B':
        case 'c':
            return (float)[self charValue];
        case 'C':
            return (float)[self unsignedCharValue];
        case 'I':
            return (float)[self unsignedIntValue];
        case 'L':
            return (float)[self unsignedLongValue];
        case 'Q':
            return (float)[self unsignedLongLongValue];
        case 'S':
            return (float)[self unsignedShortValue];
        case 'd':
            return (float)[self doubleValue];
        case 'f':
            return [self floatValue];
        case 'i':
            return (float)[self intValue];
        case 'l':
            return (float)[self longValue];
        case 'q':
            return (float)[self longLongValue];
        case 's':
            return (float)[self shortValue];
        default:
            raiseBadObjCType(self, _cmd);
    }
}

// Native -[NSNumber longLongValue], 0x1c05fc064. Non-external. See the accessor note above (FCVTZS X).
- (long long)longLongValue {
    switch (*[self objCType]) {
        case 'B':
        case 'c':
            return (long long)[self charValue];
        case 'C':
            return (long long)[self unsignedCharValue];
        case 'I':
            return (long long)[self unsignedIntValue];
        case 'L':
            return (long long)[self unsignedLongValue];
        case 'Q':
            return (long long)[self unsignedLongLongValue];
        case 'S':
            return (long long)[self unsignedShortValue];
        case 'd':
            return (long long)doubleToInt64([self doubleValue]);
        case 'f':
            return (long long)floatToInt64([self floatValue]);
        case 'i':
            return (long long)[self intValue];
        case 'l':
            return (long long)[self longValue];
        case 'q':
            return [self longLongValue];
        case 's':
            return (long long)[self shortValue];
        default:
            raiseBadObjCType(self, _cmd);
    }
}

// Native -[NSNumber unsignedLongLongValue], 0x1c05fc248. Non-external. See the accessor note above (FCVTZU
// X).
- (unsigned long long)unsignedLongLongValue {
    switch (*[self objCType]) {
        case 'B':
        case 'c':
            return (unsigned long long)[self charValue];
        case 'C':
            return (unsigned long long)[self unsignedCharValue];
        case 'I':
            return (unsigned long long)[self unsignedIntValue];
        case 'L':
            return (unsigned long long)[self unsignedLongValue];
        case 'Q':
            return [self unsignedLongLongValue];
        case 'S':
            return (unsigned long long)[self unsignedShortValue];
        case 'd':
            return (unsigned long long)doubleToUInt64([self doubleValue]);
        case 'f':
            return (unsigned long long)floatToUInt64([self floatValue]);
        case 'i':
            return (unsigned long long)[self intValue];
        case 'l':
            return (unsigned long long)[self longValue];
        case 'q':
            return (unsigned long long)[self longLongValue];
        case 's':
            return (unsigned long long)[self shortValue];
        default:
            raiseBadObjCType(self, _cmd);
    }
}

// Native -[NSNumber compare:], 0x1c05fc42c. Non-external. nil raises NSInvalidArgumentException "%@: nil
// argument"; the receiver itself is the same. Both -objCType messages are sent (the argument's first); when
// either type is 'd' or 'f' the -doubleValue results compare, a NaN ordering below every number and equal to
// another NaN (a NaN receiver answers NSOrderedDescending against a negative-signed argument,
// NSOrderedAscending otherwise; a NaN argument answers by the receiver's sign). Two 'Q' numbers compare
// unsigned; otherwise a 'Q' receiver at or above 2^63 is greater, a 'Q' argument at or above 2^63 is greater,
// and -longLongValue compares signed.
- (NSInteger)compare:(id)otherNumber {
    if (otherNumber == nil) {
        id proem = _ONSMethodExceptionProem(self, _cmd);
        id reason = [(id)stringClass stringWithFormat:(id)OF_STR("%@: nil argument"), proem];
        objc_exception_throw([(id)exceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                            reason:reason
                                                          userInfo:nil]);
    }
    if (self == otherNumber) {
        return 0;
    }
    unsigned char otherType = (unsigned char)*[otherNumber objCType];
    unsigned char ownType = (unsigned char)*[self objCType];
    if ((otherType & 0xFD) == 'd' || (ownType & 0xFD) == 'd') {
        double ownValue = [self doubleValue];
        double otherValue = [otherNumber doubleValue];
        if (isnan(ownValue)) {
            int64_t otherBits;
            memcpy(&otherBits, &otherValue, sizeof(otherBits));
            NSInteger result = otherBits <= -1 ? 1 : -1;
            return isnan(otherValue) ? 0 : result;
        }
        if (isnan(otherValue)) {
            int64_t ownBits;
            memcpy(&ownBits, &ownValue, sizeof(ownBits));
            return (ownBits >> 63) | 1;
        }
        if (ownValue < otherValue) {
            return -1;
        }
        return otherValue < ownValue;
    }
    if (otherType == 'Q' && ownType == 'Q') {
        unsigned long long ownValue = [self unsignedLongLongValue];
        unsigned long long otherValue = [otherNumber unsignedLongLongValue];
        if (ownValue < otherValue) {
            return -1;
        }
        return ownValue > otherValue;
    }
    if (ownType == 'Q' && ([self unsignedLongLongValue] >> 63) != 0) {
        return 1;
    }
    if (otherType == 'Q' && ([otherNumber unsignedLongLongValue] >> 63) != 0) {
        return -1;
    }
    long long ownValue = [self longLongValue];
    long long otherValue = [otherNumber longLongValue];
    if (ownValue < otherValue) {
        return -1;
    }
    return ownValue > otherValue;
}

// Native -[NSNumber isEqualToNumber:], 0x1c05fc5e0. Non-external. The same object is equal; otherwise
// -compare: must answer NSOrderedSame.
- (BOOL)isEqualToNumber:(id)number {
    if (self == number) {
        return YES;
    }
    return [self compare:number] == 0;
}

// Native -[NSNumber isEqual:], 0x1c05fc60c. Non-external. The same object is equal; an object _NSIsNSNumber
// accepts goes to -isEqualToNumber: (a tail call); anything else is not.
- (BOOL)isEqual:(id)object {
    if (self == object) {
        return YES;
    }
    if (!_ONSIsNSNumber(object)) {
        return NO;
    }
    return [self isEqualToNumber:object];
}

// Native -[NSNumber hash], 0x1c05fc658. Non-external. The signed integer types ('B', 'c', 'i', 'l', 's') hash
// |-integerValue| times 2654435761; the unsigned ones ('C', 'I', 'L', 'S') -unsignedIntegerValue times
// 2654435761 below 2^63; above that, 'Q' (always) and 'q' (|-longLongValue|, negated after the conversion) go
// through the double hash, as does every other type with |-doubleValue|: the value rounded to the nearest
// integer (floor of value + 0.5) is reduced modulo 2^64 (FMSUB), converted unsigned and multiplied by
// 2654435761, and the fraction times 2^64 is added when positive or its magnitude subtracted when negative.
- (NSUInteger)hash {
    unsigned char type = (unsigned char)*[self objCType];
    double value;
    switch (type) {
        case 'B':
        case 'c':
        case 'i':
        case 'l':
        case 's': {
            NSInteger integer = [self integerValue];
            NSUInteger magnitude = integer < 0 ? -(NSUInteger)integer : (NSUInteger)integer;
            return magnitude * 2654435761U;
        }
        case 'C':
        case 'I':
        case 'L':
        case 'S': {
            NSUInteger unsignedInteger = [self unsignedIntegerValue];
            if ((OF_WORD_SHIFT(unsignedInteger, 63)) == 0) {
                return unsignedInteger * 2654435761U;
            }
            value = (double)unsignedInteger;
            break;
        }
        case 'Q':
            value = (double)[self unsignedLongLongValue];
            break;
        case 'q': {
            long long integer = [self longLongValue];
            value = (double)integer;
            if (integer < 0) {
                value = -value;
            }
            break;
        }
        default:
            value = [self doubleValue];
            if (value < 0.0) {
                value = -value;
            }
            break;
    }
    double integral = floor(value + 0.5);
    double fraction = (value - integral) * 0x1p64;
    double reduced = fma(-trunc(integral * 0x1p-64), 0x1p64, integral);
    NSUInteger hash = (NSUInteger)doubleToUInt64(reduced) * 2654435761U;
    if (fraction < 0.0) {
        hash -= (NSUInteger)doubleToUInt64(fabs(fraction));
    } else if (fraction > 0.0) {
        hash += (NSUInteger)doubleToUInt64(fraction);
    }
    return hash;
}

// Native -[NSNumber descriptionWithLocale:], 0x1c05fc7c4. Non-external. NSString's
// objc_allocWithZone/-initWithFormat:locale: with "%d" ('B', 'c', 'i'), "%u" ('C', 'I'), "%hi", "%hu", "%ld",
// "%lu", "%lld", "%llu", "%0.7g" (float) or "%0.16g" (double), autoreleased; any other type raises the
// bad-objCType exception.
- (id)descriptionWithLocale:(id)locale {
    switch (*[self objCType]) {
        case 'B':
        case 'c':
            return objc_autorelease([objc_allocWithZone((Class)stringClass)
                initWithFormat:(id)OF_STR("%d")
                        locale:locale, (int)[self charValue]]);
        case 'C':
            return objc_autorelease([objc_allocWithZone((Class)stringClass)
                initWithFormat:(id)OF_STR("%u")
                        locale:locale, (unsigned int)[self unsignedCharValue]]);
        case 'I':
            return objc_autorelease([objc_allocWithZone((Class)stringClass)
                initWithFormat:(id)OF_STR("%u")
                        locale:locale, [self unsignedIntValue]]);
        case 'L':
            return objc_autorelease([objc_allocWithZone((Class)stringClass)
                initWithFormat:(id)OF_STR("%lu")
                        locale:locale, [self unsignedLongValue]]);
        case 'Q':
            return objc_autorelease([objc_allocWithZone((Class)stringClass)
                initWithFormat:(id)OF_STR("%llu")
                        locale:locale, [self unsignedLongLongValue]]);
        case 'S':
            return objc_autorelease([objc_allocWithZone((Class)stringClass)
                initWithFormat:(id)OF_STR("%hu")
                        locale:locale, (int)[self unsignedShortValue]]);
        case 'd':
            return objc_autorelease([objc_allocWithZone((Class)stringClass)
                initWithFormat:(id)OF_STR("%0.16g")
                        locale:locale, [self doubleValue]]);
        case 'f':
            return objc_autorelease([objc_allocWithZone((Class)stringClass)
                initWithFormat:(id)OF_STR("%0.7g")
                        locale:locale, (double)[self floatValue]]);
        case 'i':
            return objc_autorelease([objc_allocWithZone((Class)stringClass)
                initWithFormat:(id)OF_STR("%d")
                        locale:locale, [self intValue]]);
        case 'l':
            return objc_autorelease([objc_allocWithZone((Class)stringClass)
                initWithFormat:(id)OF_STR("%ld")
                        locale:locale, [self longValue]]);
        case 'q':
            return objc_autorelease([objc_allocWithZone((Class)stringClass)
                initWithFormat:(id)OF_STR("%lld")
                        locale:locale, [self longLongValue]]);
        case 's':
            return objc_autorelease([objc_allocWithZone((Class)stringClass)
                initWithFormat:(id)OF_STR("%hi")
                        locale:locale, (int)[self shortValue]]);
        default:
            raiseBadObjCType(self, _cmd);
    }
}

// Native -[NSNumber description], 0x1c05fcac0. Non-external; -descriptionWithLocale: with nil (a tail call).
- (id)description {
    return [self descriptionWithLocale:nil];
}

// Native -[NSNumber redactedDescription], 0x1c05fcac8. Non-external; -descriptionWithLocale: with nil.
- (id)redactedDescription {
    return [self descriptionWithLocale:nil];
}

// Native -[NSNumber stringValue], 0x1c05fcad0. Non-external; -descriptionWithLocale: with nil.
- (id)stringValue {
    return [self descriptionWithLocale:nil];
}

// Native -[NSNumber classForCoder], 0x1c05fcad8. Non-external. NSNumber.
- (Class)classForCoder {
    return (Class)numberClass;
}

// Native -[NSNumber _allowsDirectEncoding], 0x1c05fcae4. Non-external.
- (BOOL)_allowsDirectEncoding {
    return YES;
}

// Native +[NSNumber supportsSecureCoding], 0x1c05fcaec. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSNumber initWithCoder:], 0x1c05fcaf4. Non-external. The receiver's -zone is sent (its result
// unused), the coder asked -allowsKeyedCoding, and the receiver released. A non-keyed coder goes to
// newDecodedNumber (a tail call). A keyed coder whose class is exactly NSKeyedUnarchiver (objc_opt_class
// against objc_opt_self), or that contains "NS.number", decodes that property list: a number is retained and
// answered; anything else fails the coder with NSError's +_readCorruptErrorWithFormat: "Decoded object is not
// a number" and answers nil. Otherwise "NS.boolval", "NS.intval" and "NS.dblval" are tried in order, each
// sending the static NSNumber placeholder the matching -initWith...: (a tail call); none raises
// NSInternalInconsistencyException.
// Foundation defect: the native failure path sends objc_release to the receiver a second time (0x1c05fcbc8,
// after the release at 0x1c05fcb20), over-releasing it; confidence: verified. The restoration releases it
// once.
- (id)initWithCoder:(id)coder {
    (void)[self zone];
    BOOL keyed = [coder allowsKeyedCoding];
    objc_release(self);
    if (!keyed) {
        return newDecodedNumber(coder);
    }
    if (objc_opt_class(coder) == (Class)objc_opt_self((id)keyedUnarchiverClass) ||
        [coder containsValueForKey:(id)OF_STR("NS.number")]) {
        id number = [coder _decodePropertyListForKey:(id)OF_STR("NS.number")];
        if (_ONSIsNSNumber(number)) {
            return objc_retain(number);
        }
        [coder failWithError:[(id)errorClass
                                 _readCorruptErrorWithFormat:(id)OF_STR("Decoded object is not a number")]];
        return nil;
    }
    if ([coder containsValueForKey:(id)OF_STR("NS.boolval")]) {
        return [(id)&__placeholderNumber initWithBool:[coder decodeBoolForKey:(id)OF_STR("NS.boolval")]];
    }
    if ([coder containsValueForKey:(id)OF_STR("NS.intval")]) {
        return [(id)&__placeholderNumber initWithLongLong:[coder decodeInt64ForKey:(id)OF_STR("NS.intval")]];
    }
    if ([coder containsValueForKey:(id)OF_STR("NS.dblval")]) {
        return [(id)&__placeholderNumber initWithDouble:[coder decodeDoubleForKey:(id)OF_STR("NS.dblval")]];
    }
    objc_exception_throw([(id)exceptionClass
        exceptionWithName:(id)ONSInternalInconsistencyException
                   reason:(id)OF_STR("*** -[NSNumber initWithCoder:]: unknown number type")
                 userInfo:nil]);
}

// Native newDecodedNumber, 0x1c05fccd4. Non-external. The encoding string is decoded ("*", 8 bytes); NULL
// answers nil. A one-character 'l' or 'L' is rewritten in place to 'i' or 'I' (the 32-bit archive forms). The
// value is decoded into a 1024-byte stack buffer, or a heap buffer above that (nil if the allocation fails,
// after freeing the encoding), and newNumber builds the number. An empty encoding, or one newNumber does not
// know, raises NSInternalInconsistencyException "*** -[NSNumber initWithCoder:] bad objCType: \"%@\"" (the
// encoding through +stringWithUTF8String:, freed before the raise). An exception from sizing answers nil.
// Foundation defect: newDecodedNumber 0x1c05fccd4 leaks the decoded encoding through the sizing-exception
// landing pad 0x1c05fce44 (confidence: verified). The restoration frees it there.
static id newDecodedNumber(id coder) {
    char *typePtr = NULL;
    [coder decodeValueOfObjCType:"*" at:&typePtr size:8];
    if (typePtr == NULL) {
        return nil;
    }
    id number = nil;
    if (typePtr[0] != 0 && typePtr[1] == 0) {
        if (typePtr[0] == 'l') {
            typePtr[0] = 'i';
        } else if (typePtr[0] == 'L') {
            typePtr[0] = 'I';
        }
        uint64_t stackBuffer[128];
        NSUInteger size = 0;
        @try {
            ONSGetSizeAndAlignment(typePtr, &size, NULL);
        } @catch (...) {
            free(typePtr);
            return nil;
        }
        stackBuffer[0] = size;
        void *buffer;
        if (size < 0x401) {
            buffer = stackBuffer;
        } else {
            buffer = malloc_type_malloc(size, 0x100004077774924ULL);
            if (buffer == NULL) {
                free(typePtr);
                return nil;
            }
        }
        [coder decodeValueOfObjCType:typePtr at:buffer size:size];
        number = newNumber((unsigned char)*typePtr, buffer);
        if (buffer != stackBuffer) {
            free(buffer);
        }
        if (number != nil) {
            free(typePtr);
            return number;
        }
    }
    id typeString = [(id)stringClass stringWithUTF8String:typePtr];
    id reason = [(id)stringClass
        stringWithFormat:(id)OF_STR("*** -[NSNumber initWithCoder:] bad objCType: \"%@\""), typeString];
    free(typePtr);
    objc_exception_throw([(id)exceptionClass exceptionWithName:(id)ONSInternalInconsistencyException
                                                        reason:reason
                                                      userInfo:nil]);
}

// Native -[NSNumber encodeWithCoder:], 0x1c05fce8c. Non-external. A non-keyed coder gets the -objCType string
// (a one-character 'l' or 'L' widened to "q" or "Q") and the -getValue: bytes as with NSValue, including the
// sizing exception replacement and the "^v" raise. A keyed coder whose class is exactly NSKeyedArchiver
// (object_getClass) gets -_encodePropertyList:forKey: "NS.number"; otherwise a CFBoolean encodes
// "NS.boolval", the integer types ('C', 'I', 'L', 'Q', 'S', 'c', 'i', 'l', 'q', 's') -longLongValue as
// "NS.intval", 'd' and 'f' -doubleValue as "NS.dblval", a 'B' -boolValue as "NS.boolval", and any other type
// nothing.
// Folded catch-cleanup/termination range 0x1c05fd230-0x1c05fd24f, read with the native LSDA.
- (void)encodeWithCoder:(id)coder {
    if (![coder allowsKeyedCoding]) {
        const char *type = [self objCType];
        const char *typeReference = type;
        if (type[0] != 0 && type[1] == 0) {
            if (type[0] == 'l') {
                type = "q";
                typeReference = type;
            } else if (type[0] == 'L') {
                type = "Q";
                typeReference = type;
            }
        }
        NSUInteger size = 0;
        @try {
            ONSGetSizeAndAlignment(type, &size, NULL);
        } @catch (...) {
            id reason = [(id)stringClass
                stringWithFormat:(id)OF_STR("cannot encode '%s'-typed value: %@"), type, self];
            objc_exception_throw([(id)exceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                                reason:reason
                                                              userInfo:nil]);
        }
        uint64_t stackBuffer[128];
        stackBuffer[0] = size;
        void *buffer = size < 0x401 ? (void *)stackBuffer : malloc_type_malloc(size, 0x100004077774924ULL);
        if (type[0] == '^' && type[1] == 'v') {
            id reason =
                [(id)stringClass stringWithFormat:(id)OF_STR("cannot encode (void *) value: %@"), self];
            objc_exception_throw([(id)exceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                                reason:reason
                                                              userInfo:nil]);
        }
        BOOL heap = size > 0x400;
        [self getValue:buffer];
        [coder encodeValueOfObjCType:"*" at:&typeReference];
        [coder encodeValueOfObjCType:typeReference at:buffer];
        if (heap) {
            free(buffer);
        }
        return;
    }
    if (object_getClass(coder) == (Class)keyedArchiverClass) {
        [coder _encodePropertyList:self forKey:(id)OF_STR("NS.number")];
        return;
    }
    if (OCFBooleanGetTypeID() == OCFGetTypeID((OCFTypeRef)self)) {
        [coder encodeBool:OCFBooleanGetValue((OCFBooleanRef)self) != 0 forKey:(id)OF_STR("NS.boolval")];
        return;
    }
    switch (*[self objCType]) {
        case 'C':
        case 'I':
        case 'L':
        case 'Q':
        case 'S':
        case 'c':
        case 'i':
        case 'l':
        case 'q':
        case 's':
            [coder encodeInt64:[self longLongValue] forKey:(id)OF_STR("NS.intval")];
            return;
        case 'd':
        case 'f':
            [coder encodeDouble:[self doubleValue] forKey:(id)OF_STR("NS.dblval")];
            return;
        case 'B':
            [coder encodeBool:[self boolValue] forKey:(id)OF_STR("NS.boolval")];
            return;
        default:
            return;
    }
}

// Native -[NSNumber copyWithZone:], 0x1c05fd250. Non-external. The -getValue: bytes (1024 stack bytes, or a
// heap buffer above that) go to newNumber with the first type character; no number raises the bad-objCType
// exception. The zone is not used. Foundation defect: -[NSNumber copyWithZone:] 0x1c05fd250 jumps to the
// raise at 0x1c05fd334 without freeing its heap buffer when newNumber returns nil (confidence: verified). The
// restoration frees that buffer before constructing the exception. Folded sizing-exception handler
// 0x1c05fd388 (null catch-all type entry); returns nil.
- (id)copyWithZone:(struct _NSZone *)zone {
    const char *type = [self objCType];
    NSUInteger size = 0;
    @try {
        ONSGetSizeAndAlignment(type, &size, NULL);
    } @catch (...) {
        return nil;
    }
    uint64_t stackBuffer[128];
    stackBuffer[0] = size;
    void *buffer = size < 0x401 ? (void *)stackBuffer : malloc_type_malloc(size, 0x100004077774924ULL);
    [self getValue:buffer];
    id copy = newNumber((unsigned char)*type, buffer);
    if (copy == nil) {
        if (size >= 0x401) {
            free(buffer);
        }
        id proem = _ONSMethodExceptionProem(self, _cmd);
        id reason = [(id)stringClass stringWithFormat:(id)OF_STR("%@: bad objCType: \"%s\""), proem, type];
        objc_exception_throw([(id)exceptionClass exceptionWithName:(id)ONSInternalInconsistencyException
                                                            reason:reason
                                                          userInfo:nil]);
    }
    if (size >= 0x401) {
        free(buffer);
    }
    return copy;
}

// Native newNumber, 0x1c05fd3a0. Non-external. The static NSNumber placeholder's -initWith...: for the type
// character, reading the value from the bytes at its width ('B' and 'C' a byte, 'c' a signed byte, 's'/'S'
// sixteen bits, 'i'/'I' and 'f' thirty-two, 'l', 'L', 'q', 'Q' and 'd' sixty-four); any other type answers
// nil.
static id newNumber(unsigned char type, const void *bytes) {
    id placeholder = (id)&__placeholderNumber;
    switch (type) {
        case 'B': {
            uint8_t value;
            memcpy(&value, bytes, sizeof(value));
            return [placeholder initWithBool:value];
        }
        case 'C': {
            uint8_t value;
            memcpy(&value, bytes, sizeof(value));
            return [placeholder initWithUnsignedChar:value];
        }
        case 'I': {
            uint32_t value;
            memcpy(&value, bytes, sizeof(value));
            return [placeholder initWithUnsignedInt:value];
        }
        case 'L': {
            uint64_t value;
            memcpy(&value, bytes, sizeof(value));
            return [placeholder initWithUnsignedLong:value];
        }
        case 'Q': {
            uint64_t value;
            memcpy(&value, bytes, sizeof(value));
            return [placeholder initWithUnsignedLongLong:value];
        }
        case 'S': {
            uint16_t value;
            memcpy(&value, bytes, sizeof(value));
            return [placeholder initWithUnsignedShort:value];
        }
        case 'c': {
            char value;
            memcpy(&value, bytes, sizeof(value));
            return [placeholder initWithChar:value];
        }
        case 'd': {
            double value;
            memcpy(&value, bytes, sizeof(value));
            return [placeholder initWithDouble:value];
        }
        case 'f': {
            float value;
            memcpy(&value, bytes, sizeof(value));
            return [placeholder initWithFloat:value];
        }
        case 'i': {
            int32_t value;
            memcpy(&value, bytes, sizeof(value));
            return [placeholder initWithInt:value];
        }
        case 'l': {
            int64_t value;
            memcpy(&value, bytes, sizeof(value));
            return [placeholder initWithLong:value];
        }
        case 'q': {
            int64_t value;
            memcpy(&value, bytes, sizeof(value));
            return [placeholder initWithLongLong:value];
        }
        case 's': {
            int16_t value;
            memcpy(&value, bytes, sizeof(value));
            return [placeholder initWithShort:value];
        }
        default:
            return nil;
    }
}

// Native -[NSNumber _getCString:length:multiplier:], 0x1c05fd510. Non-external. The value, converted to
// double, is multiplied by the multiplier (multiplier first) and converted back (to int through FCVTZS W for
// 'B', 'c', 'C', 's', 'S' and 'i'; FCVTZU W for 'I'; 64-bit signed or unsigned for 'l', 'q', 'L', 'Q'; float
// for 'f'), then printed with snprintf into the buffer with the sign-extended length ("%d", "%u", "%hi",
// "%hu", "%ld",
// "%lu", "%lld", "%llu", "%0.7g" or "%0.16g"; the "%hi" and "%hu" arguments are spelled with their short
// types, which printf applies to the promoted int natively); the result is whether the output fit (the count
// below the length). The 'C' path uses signed FCVTZS W before printing unsigned; the 's'/'S' paths also
// convert in 32 bits and apply the short length modifier (kept). Any other type raises the bad-objCType
// exception.
- (BOOL)_getCString:(char *)buffer length:(int)length multiplier:(double)multiplier {
    // Foundation defect: the sign extension at 0x1c05fd57c (and the other snprintf paths) makes a
    // negative int length an enormous size_t, allowing an out-of-bounds write (confidence: verified).
    // Pass zero capacity for a negative length; retain all getter calls and the native signed comparison.
    size_t capacity = length < 0 ? 0 : (size_t)length;
    int count;
    switch (*[self objCType]) {
        case 'B':
        case 'c':
            count =
                snprintf(buffer, capacity, "%d", (int)doubleToInt32(multiplier * (double)[self charValue]));
            break;
        case 'C':
            count = snprintf(
                buffer, capacity, "%u", (int)doubleToInt32(multiplier * (double)[self unsignedCharValue])
            );
            break;
        case 'I':
            count = snprintf(
                buffer,
                capacity,
                "%u",
                (unsigned int)doubleToUInt32(multiplier * (double)[self unsignedIntValue])
            );
            break;
        case 'L':
            count = snprintf(
                buffer,
                capacity,
                "%lu",
                (unsigned long)doubleToUInt64(multiplier * (double)[self unsignedLongValue])
            );
            break;
        case 'Q':
            count = snprintf(
                buffer,
                capacity,
                "%llu",
                (unsigned long long)doubleToUInt64(multiplier * (double)[self unsignedLongLongValue])
            );
            break;
        case 'S':
            count = snprintf(
                buffer,
                capacity,
                "%hu",
                (unsigned short)doubleToInt32(multiplier * (double)[self unsignedShortValue])
            );
            break;
        case 'd':
            count = snprintf(buffer, capacity, "%0.16g", multiplier * [self doubleValue]);
            break;
        case 'f':
            count =
                snprintf(buffer, capacity, "%0.7g", (double)(float)(multiplier * (double)[self floatValue]));
            break;
        case 'i':
            count =
                snprintf(buffer, capacity, "%d", (int)doubleToInt32(multiplier * (double)[self intValue]));
            break;
        case 'l':
            count =
                snprintf(buffer, capacity, "%ld", (long)doubleToInt64(multiplier * (double)[self longValue]));
            break;
        case 'q':
            count = snprintf(
                buffer, capacity, "%lld", (long long)doubleToInt64(multiplier * (double)[self longLongValue])
            );
            break;
        case 's':
            count = snprintf(
                buffer, capacity, "%hi", (short)doubleToInt32(multiplier * (double)[self shortValue])
            );
            break;
        default:
            raiseBadObjCType(self, _cmd);
    }
    return count < length;
}

// Native -[NSNumber initWithChar:], 0x1c0fe70e4. Non-external.
// Requests a concrete object and answers the receiver.
- (id)initWithChar:(char)value {
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

// Native -[NSNumber initWithUnsignedChar:], 0x1c0fe7108. Non-external.
// Requests a concrete object and answers the receiver.
- (id)initWithUnsignedChar:(unsigned char)value {
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

// Native -[NSNumber initWithShort:], 0x1c0fe712c. Non-external.
// Requests a concrete object and answers the receiver.
- (id)initWithShort:(short)value {
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

// Native -[NSNumber initWithUnsignedShort:], 0x1c0fe7150. Non-external.
// Requests a concrete object and answers the receiver.
- (id)initWithUnsignedShort:(unsigned short)value {
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

// Native -[NSNumber initWithInt:], 0x1c0fe7174. Non-external.
// Requests a concrete object and answers the receiver.
- (id)initWithInt:(int)value {
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

// Native -[NSNumber initWithUnsignedInt:], 0x1c0fe7198. Non-external.
// Requests a concrete object and answers the receiver.
- (id)initWithUnsignedInt:(unsigned int)value {
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

// Native -[NSNumber initWithInteger:], 0x1c0fe71bc. Non-external.
// Requests a concrete object and answers the receiver.
- (id)initWithInteger:(NSInteger)value {
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

// Native -[NSNumber initWithUnsignedInteger:], 0x1c0fe71e0. Non-external.
// Requests a concrete object and answers the receiver.
- (id)initWithUnsignedInteger:(NSUInteger)value {
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

// Native -[NSNumber initWithLong:], 0x1c0fe7204. Non-external.
// Requests a concrete object and answers the receiver.
- (id)initWithLong:(long)value {
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

// Native -[NSNumber initWithUnsignedLong:], 0x1c0fe7228. Non-external.
// Requests a concrete object and answers the receiver.
- (id)initWithUnsignedLong:(unsigned long)value {
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

// Native -[NSNumber initWithLongLong:], 0x1c0fe724c. Non-external.
// Requests a concrete object and answers the receiver.
- (id)initWithLongLong:(long long)value {
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

// Native -[NSNumber initWithUnsignedLongLong:], 0x1c0fe7270. Non-external.
// Requests a concrete object and answers the receiver.
- (id)initWithUnsignedLongLong:(unsigned long long)value {
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

// Native -[NSNumber initWithFloat:], 0x1c0fe7294. Non-external.
// Requests a concrete object and answers the receiver.
- (id)initWithFloat:(float)value {
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

// Native -[NSNumber initWithDouble:], 0x1c0fe72b8. Non-external.
// Requests a concrete object and answers the receiver.
- (id)initWithDouble:(double)value {
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

// Native -[NSNumber initWithBool:], 0x1c0fe72dc. Non-external.
// Requests a concrete object and answers the receiver.
- (id)initWithBool:(BOOL)value {
    _ONSRequestConcreteObject(self, _cmd);
    return self;
}

@end

// The placeholder's lifetime methods do nothing, and super's -dealloc is never reached, so the
// missing-super-call diagnostic is silenced for this implementation. In the system-name build NSLog is the
// compiler's builtin format function, which rejects the constant-string macro as a non-literal format; the
// native call passes the constant string, so that diagnostic is silenced here as well.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"
#pragma clang diagnostic ignored "-Wformat-security"

@implementation ONSPlaceholderValue

// Native -objc_retain(NSPlaceholderValue), 0x1c05fd814. Non-external. Answers the receiver (RET).
- (id)retain {
    return self;
}

// Native -[NSPlaceholderValue retainCount], 0x1c05fd818. Non-external. All ones.
- (NSUInteger)retainCount {
    return (NSUInteger)-1;
}

// Native -objc_release(NSPlaceholderValue), 0x1c05fd820. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[NSPlaceholderValue dealloc], 0x1c05fd834. Non-external. Does nothing.
- (void)dealloc {
}

// Native -[NSPlaceholderValue init], 0x1c05fd838. Non-external. Answers nil.
- (id)init {
    return nil;
}

// Native -[NSPlaceholderValue initWithBytes:objCType:], 0x1c05fd840. Non-external. __NSNewValue with the
// placeholder's zone (a tail call).
- (id)initWithBytes:(const void *)value objCType:(const char *)type {
    return _NSNewValue(value, type, zoneForInstance);
}

// Native +[NSPlaceholderValue supportsSecureCoding], 0x1c05fd854. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSPlaceholderValue initWithCoder:], 0x1c05fd85c. Non-external. newDecodedValue with the
// placeholder's zone (a tail call).
- (id)initWithCoder:(id)coder {
    return newDecodedValue(coder, zoneForInstance);
}

// Native -[NSPlaceholderValue objCType], 0x1c0fe7308. Non-external. Logs "Did you forget to nest alloc and
// init?", then requests a concrete implementation of NSNumber when the receiver's class is
// NSPlaceholderNumber (objc_opt_class against objc_opt_self) and of NSValue otherwise (the self class
// references); answers NULL.
- (const char *)objCType {
    ONSLog((id)OF_STR("Did you forget to nest alloc and init?"));
    Class cls = objc_opt_class(self);
    Class absClass =
        cls == (Class)objc_opt_self((id)placeholderNumberClass) ? (Class)numberClass : (Class)valueClass;
    ONSRequestConcreteImplementation(self, _cmd, absClass);
    return NULL;
}

// Native -[NSPlaceholderValue getValue:], 0x1c0fe7380. Non-external. As -objCType, ending in the request (a
// tail call).
- (void)getValue:(void *)value {
    ONSLog((id)OF_STR("Did you forget to nest alloc and init?"));
    Class cls = objc_opt_class(self);
    Class absClass =
        cls == (Class)objc_opt_self((id)placeholderNumberClass) ? (Class)numberClass : (Class)valueClass;
    ONSRequestConcreteImplementation(self, _cmd, absClass);
}

// Native -[NSPlaceholderValue getValue:size:], 0x1c0fe73f0. Non-external. As -getValue:.
- (void)getValue:(void *)value size:(NSUInteger)size {
    ONSLog((id)OF_STR("Did you forget to nest alloc and init?"));
    Class cls = objc_opt_class(self);
    Class absClass =
        cls == (Class)objc_opt_self((id)placeholderNumberClass) ? (Class)numberClass : (Class)valueClass;
    ONSRequestConcreteImplementation(self, _cmd, absClass);
}

@end

#pragma clang diagnostic pop

// The private 128-bit CFNumber value (type 17, kCFNumberSInt128Type): the high signed word, then the low
// unsigned word. NumberSInt128 is a local descriptive name.
typedef struct {
    int64_t high;
    uint64_t low;
} NumberSInt128;

@implementation ONSPlaceholderNumber

// Native -[NSPlaceholderNumber init], 0x1c05fd870. Non-external. Answers nil.
- (id)init {
    return nil;
}

// Native -[NSPlaceholderNumber initWithChar:], 0x1c05fd878. Non-external. CFNumberCreate
// (kCFAllocatorDefault) of the value as kCFNumberSInt8Type; the placeholder is not released.
- (id)initWithChar:(char)value {
    char number = value;
    return (id)OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberSInt8Type, &number);
}

// Native -[NSPlaceholderNumber initWithUnsignedChar:], 0x1c05fd8d8. Non-external. kCFNumberSInt16Type.
- (id)initWithUnsignedChar:(unsigned char)value {
    int16_t number = value;
    return (id)OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberSInt16Type, &number);
}

// Native -[NSPlaceholderNumber initWithShort:], 0x1c05fd938. Non-external. kCFNumberSInt16Type.
- (id)initWithShort:(short)value {
    int16_t number = value;
    return (id)OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberSInt16Type, &number);
}

// Native -[NSPlaceholderNumber initWithUnsignedShort:], 0x1c05fd998. Non-external. kCFNumberSInt32Type.
- (id)initWithUnsignedShort:(unsigned short)value {
    int32_t number = value;
    return (id)OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberSInt32Type, &number);
}

// Native -[NSPlaceholderNumber initWithInt:], 0x1c05fd9f8. Non-external. kCFNumberSInt32Type.
- (id)initWithInt:(int)value {
    int32_t number = value;
    return (id)OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberSInt32Type, &number);
}

// Native -[NSPlaceholderNumber initWithUnsignedInt:], 0x1c05fda58. Non-external. kCFNumberSInt64Type.
- (id)initWithUnsignedInt:(unsigned int)value {
    int64_t number = value;
    return (id)OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberSInt64Type, &number);
}

// Native -[NSPlaceholderNumber initWithInteger:], 0x1c05fdabc. Non-external. kCFNumberSInt64Type.
- (id)initWithInteger:(NSInteger)value {
    int64_t number = value;
    return (id)OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberSInt64Type, &number);
}

// Native -[NSPlaceholderNumber initWithUnsignedInteger:], 0x1c05fdb1c. Non-external. Below 2^63
// kCFNumberSInt64Type; otherwise the private 128-bit type 17 (kCFNumberSInt128Type), a zero high word
// followed by the value as the low word (NumberSInt128).
- (id)initWithUnsignedInteger:(NSUInteger)value {
    if ((int64_t)value < 0) {
        NumberSInt128 number = {0, value};
        return (id)OCFNumberCreate(kOCFAllocatorDefault, (OCFNumberType)17, &number);
    }
    int64_t number = (int64_t)value;
    return (id)OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberSInt64Type, &number);
}

// Native -[NSPlaceholderNumber initWithLong:], 0x1c05fdb9c. Non-external. kCFNumberSInt64Type.
- (id)initWithLong:(long)value {
    int64_t number = value;
    return (id)OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberSInt64Type, &number);
}

// Native -[NSPlaceholderNumber initWithUnsignedLong:], 0x1c05fdbfc. Non-external. As
// -initWithUnsignedInteger:.
- (id)initWithUnsignedLong:(unsigned long)value {
    if ((int64_t)value < 0) {
        NumberSInt128 number = {0, value};
        return (id)OCFNumberCreate(kOCFAllocatorDefault, (OCFNumberType)17, &number);
    }
    int64_t number = (int64_t)value;
    return (id)OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberSInt64Type, &number);
}

// Native -[NSPlaceholderNumber initWithLongLong:], 0x1c05fdc7c. Non-external. kCFNumberSInt64Type.
- (id)initWithLongLong:(long long)value {
    int64_t number = value;
    return (id)OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberSInt64Type, &number);
}

// Native -[NSPlaceholderNumber initWithUnsignedLongLong:], 0x1c05fdcdc. Non-external. As
// -initWithUnsignedInteger:.
- (id)initWithUnsignedLongLong:(unsigned long long)value {
    if ((int64_t)value < 0) {
        NumberSInt128 number = {0, value};
        return (id)OCFNumberCreate(kOCFAllocatorDefault, (OCFNumberType)17, &number);
    }
    int64_t number = (int64_t)value;
    return (id)OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberSInt64Type, &number);
}

// Native -[NSPlaceholderNumber initWithFloat:], 0x1c05fdd5c. Non-external. kCFNumberFloatType.
- (id)initWithFloat:(float)value {
    float number = value;
    return (id)OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberFloatType, &number);
}

// Native -[NSPlaceholderNumber initWithDouble:], 0x1c05fddbc. Non-external. kCFNumberDoubleType.
- (id)initWithDouble:(double)value {
    double number = value;
    return (id)OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberDoubleType, &number);
}

// Native -[NSPlaceholderNumber initWithBool:], 0x1c05fde1c. Non-external. CFRetain of kCFBooleanTrue or
// kCFBooleanFalse (a tail call).
- (id)initWithBool:(BOOL)value {
    return (id)OCFRetain(value ? kOCFBooleanTrue : kOCFBooleanFalse);
}

// Native +[NSPlaceholderNumber supportsSecureCoding], 0x1c05fde3c. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSPlaceholderNumber initWithCoder:], 0x1c05fde44. Non-external. As -[NSNumber initWithCoder:]
// without the zone read and the initial release: a non-keyed coder goes to newDecodedNumber; a coder whose
// class is exactly NSKeyedUnarchiver (object_getClass), or that contains "NS.number", decodes the property
// list (a non-number fails the coder and releases the receiver); otherwise "NS.boolval", "NS.intval" and
// "NS.dblval" go to the receiver's own -initWith...:, and none releases the receiver and raises
// NSInternalInconsistencyException.
- (id)initWithCoder:(id)coder {
    if (![coder allowsKeyedCoding]) {
        return newDecodedNumber(coder);
    }
    if (object_getClass(coder) == (Class)keyedUnarchiverClass ||
        [coder containsValueForKey:(id)OF_STR("NS.number")]) {
        id number = [coder _decodePropertyListForKey:(id)OF_STR("NS.number")];
        if (_ONSIsNSNumber(number)) {
            return objc_retain(number);
        }
        [coder failWithError:[(id)errorClass
                                 _readCorruptErrorWithFormat:(id)OF_STR("Decoded object is not a number")]];
        objc_release(self);
        return nil;
    }
    if ([coder containsValueForKey:(id)OF_STR("NS.boolval")]) {
        return [self initWithBool:[coder decodeBoolForKey:(id)OF_STR("NS.boolval")]];
    }
    if ([coder containsValueForKey:(id)OF_STR("NS.intval")]) {
        return [self initWithLongLong:[coder decodeInt64ForKey:(id)OF_STR("NS.intval")]];
    }
    if ([coder containsValueForKey:(id)OF_STR("NS.dblval")]) {
        return [self initWithDouble:[coder decodeDoubleForKey:(id)OF_STR("NS.dblval")]];
    }
    objc_release(self);
    objc_exception_throw([(id)exceptionClass
        exceptionWithName:(id)ONSInternalInconsistencyException
                   reason:(id)OF_STR("*** -[NSPlaceholderNumber initWithCoder:]: unknown number type")
                 userInfo:nil]);
}

@end

// Native cachedObjCTypeInfoHash, 0x1c05fdfec. Non-external. CFStringHashISOLatin1CString of the record's
// encoding with its strlen.
static OCFHashCode cachedObjCTypeInfoHash(const void *value) {
    const char *type = ((const CachedTypeInfo *)value)->type;
    return OCFStringHashISOLatin1CString((const uint8_t *)type, (OCFIndex)strlen(type));
}

// Native cachedObjCTypeInfoIsEqual, 0x1c05fe018. Non-external. strcmp of the two records' encodings.
static Boolean cachedObjCTypeInfoIsEqual(const void *value1, const void *value2) {
    return strcmp(((const CachedTypeInfo *)value1)->type, ((const CachedTypeInfo *)value2)->type) == 0;
}

// Native mungeUnionsAndStructures, 0x1c05fe03c. Non-external. Skips from the cursor past aggregate openings
// and closings: at '(' or '{' the scan runs to the '=' (the cursor moves past it) or to the matching close
// character first (the cursor moves past the opening only), failing (NO, the cursor unchanged) at the end of
// the string; a
// ')' or '}' is skipped. The first other character is stored back and YES answered.
static BOOL mungeUnionsAndStructures(const char **cursor) {
    const char *position = *cursor;
    for (;;) {
        unsigned char character = (unsigned char)*position;
        if (character == '(' || character == '{') {
            unsigned char close = character == '(' ? ')' : '}';
            const char *scan = position;
            for (;;) {
                if (character == 0) {
                    return NO;
                }
                if (character == '=') {
                    position = scan + 1;
                    break;
                }
                character = (unsigned char)*++scan;
                if (character == close) {
                    position = position + 1;
                    break;
                }
            }
            continue;
        }
        if (character == ')' || character == '}') {
            position = position + 1;
            continue;
        }
        *cursor = position;
        return YES;
    }
}

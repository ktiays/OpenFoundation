//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Archiving/ONSKeyedArchiver.h"

#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Collections/OCFSet.h"
#include "CoreFoundation/Number/OCFNumber.h"
#import "CoreFoundation/Objective-C/OCFNSArray.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#include "CoreFoundation/Stream/OCFStream.h"
#import "Foundation/Archiving/ONSArchivingPrivate.h"
#import "Foundation/Archiving/ONSKeyedUnarchiver.h"
#import "Foundation/Archiving/ONSObjCRuntimeNames.h"

#include <objc/objc-exception.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. MRC ownership, acquire loads and release stores follow the native operations.
// iOS Device divergence: the iOS Device 24A446 bodies remain unread.
// Reading: Source/Foundation/Archiving/Reading.md.

// Native __warnArchiverCompat, 0x22e6fcba1, one byte shared by both classes' +initialize methods.
uint8_t _warnArchiverCompat;
// Native __MergedGlobals.43: lock at 0x22e76c5e8 and class-name map at 0x22e76c5f0.
static os_unfair_lock classNameMapLock = OS_UNFAIR_LOCK_INIT;
static OCFMutableDictionaryRef classNameMap;

// Native generic_keys, 0x2227d5530: forty constant-string references, "$0" through "$39". IDA static-data
// reading also verifies each pointed-to string's 0x7c8 info word and byte length.
static const OCFConstantString kGenericKeyStrings[40] = {
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$0", 2},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$1", 2},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$2", 2},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$3", 2},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$4", 2},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$5", 2},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$6", 2},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$7", 2},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$8", 2},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$9", 2},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$10", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$11", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$12", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$13", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$14", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$15", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$16", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$17", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$18", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$19", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$20", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$21", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$22", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$23", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$24", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$25", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$26", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$27", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$28", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$29", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$30", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$31", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$32", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$33", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$34", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$35", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$36", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$37", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$38", 3},
    {(uintptr_t)__OCFConstantStringClassReference, 0x7c8, "$39", 3},
};
const OCFStringRef generic_keys[40] = {
    (OCFStringRef)&kGenericKeyStrings[0],  (OCFStringRef)&kGenericKeyStrings[1],
    (OCFStringRef)&kGenericKeyStrings[2],  (OCFStringRef)&kGenericKeyStrings[3],
    (OCFStringRef)&kGenericKeyStrings[4],  (OCFStringRef)&kGenericKeyStrings[5],
    (OCFStringRef)&kGenericKeyStrings[6],  (OCFStringRef)&kGenericKeyStrings[7],
    (OCFStringRef)&kGenericKeyStrings[8],  (OCFStringRef)&kGenericKeyStrings[9],
    (OCFStringRef)&kGenericKeyStrings[10], (OCFStringRef)&kGenericKeyStrings[11],
    (OCFStringRef)&kGenericKeyStrings[12], (OCFStringRef)&kGenericKeyStrings[13],
    (OCFStringRef)&kGenericKeyStrings[14], (OCFStringRef)&kGenericKeyStrings[15],
    (OCFStringRef)&kGenericKeyStrings[16], (OCFStringRef)&kGenericKeyStrings[17],
    (OCFStringRef)&kGenericKeyStrings[18], (OCFStringRef)&kGenericKeyStrings[19],
    (OCFStringRef)&kGenericKeyStrings[20], (OCFStringRef)&kGenericKeyStrings[21],
    (OCFStringRef)&kGenericKeyStrings[22], (OCFStringRef)&kGenericKeyStrings[23],
    (OCFStringRef)&kGenericKeyStrings[24], (OCFStringRef)&kGenericKeyStrings[25],
    (OCFStringRef)&kGenericKeyStrings[26], (OCFStringRef)&kGenericKeyStrings[27],
    (OCFStringRef)&kGenericKeyStrings[28], (OCFStringRef)&kGenericKeyStrings[29],
    (OCFStringRef)&kGenericKeyStrings[30], (OCFStringRef)&kGenericKeyStrings[31],
    (OCFStringRef)&kGenericKeyStrings[32], (OCFStringRef)&kGenericKeyStrings[33],
    (OCFStringRef)&kGenericKeyStrings[34], (OCFStringRef)&kGenericKeyStrings[35],
    (OCFStringRef)&kGenericKeyStrings[36], (OCFStringRef)&kGenericKeyStrings[37],
    (OCFStringRef)&kGenericKeyStrings[38], (OCFStringRef)&kGenericKeyStrings[39],
};

static void _encodeObject(ONSKeyedArchiver *coder, id key, id object, int conditional);
static void addValueToTopContainerE(ONSKeyedArchiver *coder, const void *key, const void *value);
static void _encodeInt32(ONSKeyedArchiver *coder, const void *key, int32_t value);
static void _encodeInt64(ONSKeyedArchiver *coder, const void *key, int64_t value);
static void _encodeFloat(ONSKeyedArchiver *coder, const void *key, float value);
static void _encodeDouble(ONSKeyedArchiver *coder, const void *key, double value);
static void _encodeBytes(ONSKeyedArchiver *coder, const uint8_t *bytes, OCFIndex length, const void *key);
static void
_compatEncodeValueOfObjCType(ONSKeyedArchiver *coder, int type, const void *address, SEL selector);
static OCFKeyedArchiverUIDRef
__NSKeyedArchiverUIDCreateCached(OCFAllocatorRef allocator, uint32_t uid, ONSKeyedArchiver *coder);
static const void *__karetain(OCFAllocatorRef allocator, const void *object);
static void __karelease(OCFAllocatorRef allocator, const void *object);

// Native _NSCoderCheckPotentiallyMangledClassNameAndWarn, 0x1c0556fbc. Non-external (was a private external).
// Only a positive responds-to-selector answer is cached; operation is the 32-bit archiving XOR 1 (kept).
void _NSCoderCheckPotentiallyMangledClassNameAndWarn(Class cls, int archiving) {
    static _Atomic(uint8_t) warningMethodIsAvailable;
    if (cls == Nil) {
        return;
    }
    const char *name = class_getName(cls);
    if (name[0] != '_' || name[1] != 'T' || name[2] == '\0') {
        return;
    }
    if (!(atomic_load_explicit(&warningMethodIsAvailable, memory_order_acquire) & 1)) {
        if (!objc_opt_respondsToSelector(
                (id)archivingUnarchiverClass,
                sel_getUid("_swift_checkClassAndWarnForKeyedArchiving:operation:")
            )) {
            return;
        }
        atomic_store_explicit(&warningMethodIsAvailable, 1, memory_order_release);
    }
    [(id)archivingUnarchiverClass _swift_checkClassAndWarnForKeyedArchiving:cls operation:archiving ^ 1];
}

@implementation ONSKeyedArchiver

// Native +[NSKeyedArchiver initialize], 0x1c0557058. Non-external.
+ (void)initialize {
    if (self == (id)archivingArchiverClass) {
        if (getenv("NSWarnAboutOldStyleArchiverMethods") != NULL) {
            _warnArchiverCompat = 1;
        }
        [(id)archivingArchiverClass setClassName:(id)OF_STR("NSLocalTimeZone")
                                        forClass:objc_lookUpClass(OF_NAME_STRING(__ONSLocalTimeZone))];
        [(id)archivingArchiverClass setClassName:(id)OF_STR("NSLocalTimeZone")
                                        forClass:objc_lookUpClass(OF_NAME_STRING(__ONSLocalTimeZoneI))];
    }
}

// Native -[NSKeyedArchiver initRequiringSecureCoding:], 0x1c0557174. Non-external.
- (id)initRequiringSecureCoding:(BOOL)requiresSecureCoding {
    id coder = [self init];
    [coder setRequiresSecureCoding:requiresSecureCoding];
    return coder;
}

// Native +[NSKeyedArchiver archivedDataWithRootObject:requiringSecureCoding:error:], 0x1c05571a4.
// Non-external. The normal path leaves error untouched. Folded exception landing pads
// 0x1c0557254-0x1c0557388 (LSDA 0x1c14a1de0) catch id and convert it to error code 4866;
// finally calls finishEncoding and releases the coder. Hex-Rays omits this error path.
+ (id)archivedDataWithRootObject:(id)object
           requiringSecureCoding:(BOOL)requiresSecureCoding
                           error:(id *)error {
    id coder = [objc_alloc(self) initRequiringSecureCoding:requiresSecureCoding];
    id data;
    @try {
        [coder encodeObject:object forKey:(id)OF_STR("root")];
        data = [coder encodedData];
    } @catch (id exception) {
        if (error != NULL) {
            id underlyingError = nil;
            if (objc_opt_isKindOfClass(exception, (Class)archivingExceptionClass)) {
                [self __categorizeException:exception intoError:&underlyingError];
            }
            id key;
            id value;
            if (underlyingError != nil) {
                key = (id)ONSUnderlyingErrorKey;
                value = underlyingError;
            } else {
                key = (id)ONSDebugDescriptionErrorKey;
                value = [(id)archivingStringClass
                    stringWithFormat:(id)OF_STR("Caught exception during archival: %@"),
                                     [exception debugDescription]];
            }
            id userInfo = [(id)archivingDictionaryClass dictionaryWithObjects:&value forKeys:&key count:1];
            *error = [(id)archivingErrorClass errorWithDomain:(id)ONSCocoaErrorDomain
                                                         code:4866
                                                     userInfo:userInfo];
        }
        data = nil;
    } @finally {
        [coder finishEncoding];
        objc_release(coder);
    }
    return data;
}

// Native -[NSKeyedArchiver init], 0x1c055738c. Non-external.
- (id)init {
    id data = objc_autorelease(objc_opt_new((id)archivingMutableDataClass));
    return [self _initWithOutput:data];
}

// Native -[NSKeyedArchiver _initWithOutput:], 0x1c05573c0. Non-external.
// No super-init call occurs. Initial flags are 0xC80000; capacities are 1024, 256 and 16.
- (id)_initWithOutput:(id)output {
    _stream = (void *)OCFRetain(output);
    atomic_store_explicit(&_flags, 0xc80000, memory_order_release);
    _delegate = nil;
    _containers = nil;
    _objects = nil;
    _objRefMap = NULL;
    _replacementMap = NULL;
    _classNameMap = NULL;
    _conditionals = NULL;
    _classes = NULL;
    _visited = NULL;
    _cache = NULL;
    _cacheSize = 0;
    _estimatedCount = 0;
    _containers = objc_opt_new((id)archivingMutableArrayClass);
    _objects = objc_opt_new((id)archivingMutableArrayClass);
    _OCFArraySetCapacity((OCFMutableArrayRef)_objects, 1024);
    [_objects addObject:(id)OF_STR("$null")];
    OCFDictionaryKeyCallBacks callbacks = {0, __karetain, __karelease, NULL, NULL, NULL};
    _objRefMap = OCFDictionaryCreateMutable(NULL, 0, &callbacks, NULL);
    _OCFDictionarySetCapacity(_objRefMap, 256);
    _conditionals = OCFDictionaryCreateMutable(NULL, 0, &callbacks, NULL);
    id container = objc_opt_new((id)archivingMutableDictionaryClass);
    _OCFDictionarySetCapacity((OCFMutableDictionaryRef)container, 16);
    [_containers addObject:container];
    ++_estimatedCount;
    objc_release(container);
    atomic_store_explicit(
        &_flags, atomic_load_explicit(&_flags, memory_order_acquire) & ~1ULL, memory_order_release
    );
    _genericKey = 0;
    return self;
}

// Native ___karetain, 0x1c055758c. Non-external.
static const void *__karetain(OCFAllocatorRef allocator, const void *object) {
    (void)allocator;
    return OCFRetain(object);
}

// Native ___karelease, 0x1c0557594. Non-external.
static void __karelease(OCFAllocatorRef allocator, const void *object) {
    (void)allocator;
    OCFRelease(object);
}

// Native -[NSKeyedArchiver dealloc], 0x1c055777c. Non-external.
// Foundation defect: native 0x1c0557800-0x1c055780c advances a 32-bit index. Use a 64-bit index
// to release a cache whose repaired capacity may reach 2^32 (see __NSKeyedArchiverUIDCreateCached).
- (void)dealloc {
    if (!(atomic_load_explicit(&_flags, memory_order_acquire) & 2)) {
        os_log_t log = _NSOSLog();
        if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
            id proem = _ONSMethodExceptionProem(self, _cmd);
            uint8_t buffer[12];
            __builtin_os_log_format(
                buffer,
                "%@: warning: NSKeyedArchiver deallocated without having had -finishEncoding called on it.",
                proem
            );
            _os_log_error_impl(
                &__dso_handle,
                log,
                OS_LOG_TYPE_ERROR,
                "%@: warning: NSKeyedArchiver deallocated without having had -finishEncoding called on it.",
                buffer,
                sizeof(buffer)
            );
        }
    }
    for (uint64_t index = 0; index < _cacheSize; ++index) {
        if (_cache[index] != NULL) {
            OCFRelease(_cache[index]);
        }
    }
    if (_cache != NULL) {
        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, (void *)_cache);
    }
    if (_stream != NULL) {
        OCFRelease(_stream);
    }
    objc_release(_containers);
    objc_release(_objects);
    if (_objRefMap != NULL) {
        OCFRelease(_objRefMap);
    }
    if (_replacementMap != NULL) {
        OCFRelease(_replacementMap);
    }
    if (_classNameMap != NULL) {
        OCFRelease(_classNameMap);
    }
    if (_conditionals != NULL) {
        OCFRelease(_conditionals);
    }
    if (_classes != NULL) {
        OCFRelease(_classes);
    }
    if (_visited != NULL) {
        OCFRelease(_visited);
    }
    [super dealloc];
}

// Native -[NSKeyedArchiver description], 0x1c0557960. Non-external.
- (id)description {
    return [super description];
}

// Native -[NSKeyedArchiver allowsKeyedCoding], 0x1c05579c0. Non-external.
- (BOOL)allowsKeyedCoding {
    return YES;
}

// Native -[NSKeyedArchiver setDelegate:], 0x1c05579d0. Non-external.
- (void)setDelegate:(id)delegate {
    _delegate = delegate;
}

// Native -[NSKeyedArchiver delegate], 0x1c05579e0. Non-external.
- (id)delegate {
    return _delegate;
}

// Native -[NSKeyedArchiver outputFormat], 0x1c0557a78. Non-external.
- (NSUInteger)outputFormat {
    return atomic_load_explicit(&_flags, memory_order_acquire) >> 16;
}

// Native +[NSKeyedArchiver setClassName:forClass:], 0x1c0557a90. Non-external.
+ (void)setClassName:(id)name forClass:(Class)cls {
    os_unfair_lock_lock(&classNameMapLock);
    OCFMutableDictionaryRef map = classNameMap;
    if (map == NULL) {
        map = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, NULL, &kOCFTypeDictionaryValueCallBacks
        );
        classNameMap = map;
    }
    if (name != nil) {
        OCFDictionarySetValue(map, cls, name);
    } else {
        OCFDictionaryRemoveValue(map, cls);
    }
    os_unfair_lock_unlock(&classNameMapLock);
}

// Native -[NSKeyedArchiver setClassName:forClass:], 0x1c0557b18. Non-external.
- (void)setClassName:(id)name forClass:(Class)cls {
    OCFMutableDictionaryRef map = _classNameMap;
    if (map == NULL) {
        map = OCFDictionaryCreateMutable(
            kOCFAllocatorSystemDefault, 0, NULL, &kOCFTypeDictionaryValueCallBacks
        );
        _classNameMap = map;
    }
    if (name != nil) {
        OCFDictionarySetValue(map, cls, name);
    } else {
        OCFDictionaryRemoveValue(map, cls);
    }
}

// Native +[NSKeyedArchiver classNameForClass:], 0x1c0557b94. Non-external.
+ (id)classNameForClass:(Class)cls {
    os_unfair_lock_lock(&classNameMapLock);
    id name = classNameMap != NULL ? (id)OCFDictionaryGetValue(classNameMap, cls) : nil;
    os_unfair_lock_unlock(&classNameMapLock);
    return name;
}

// Native -[NSKeyedArchiver classNameForClass:], 0x1c0557bec. Non-external.
- (id)classNameForClass:(Class)cls {
    return _classNameMap != NULL ? (id)OCFDictionaryGetValue(_classNameMap, cls) : nil;
}

// Native -[NSKeyedArchiver finishEncoding], 0x1c0557c08. Non-external.
- (void)finishEncoding {
    int32_t version = 100000;
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        return;
    }
    if (_delegate != nil && objc_opt_respondsToSelector(_delegate, sel_getUid("archiverWillFinish:"))) {
        [(id)_delegate archiverWillFinish:self];
    }
    atomic_store_explicit(
        &_flags, atomic_load_explicit(&_flags, memory_order_acquire) | 2, memory_order_release
    );
    OCFAllocatorRef allocator = kOCFAllocatorDefault;
    OCFNumberRef number = OCFNumberCreate(allocator, kOCFNumberSInt32Type, &version);
    OCFMutableDictionaryRef archive = OCFDictionaryCreateMutable(
        allocator, 8, &kOCFTypeDictionaryKeyCallBacks, &kOCFTypeDictionaryValueCallBacks
    );
    id name = ONSStringFromClass(objc_opt_class(self));
    OCFDictionarySetValue(archive, OF_STR("$archiver"), name);
    OCFDictionarySetValue(archive, OF_STR("$version"), number);
    OCFDictionarySetValue(archive, OF_STR("$objects"), _objects);
    const void *top = OCFArrayGetValueAtIndex((OCFArrayRef)_containers, 0);
    OCFDictionarySetValue(archive, OF_STR("$top"), top);
    OCFRelease(number);
    _estimatedCount += 12;
    if ([self outputFormat] == 100) {
        OCFDataRef data = _OCFPropertyListCreateXMLDataWithExtras(allocator, archive);
        if (data == NULL) {
            id reason =
                [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: could not create XML data"),
                                                           _ONSMethodExceptionProem(self, _cmd)];
            objc_exception_throw([ONSException exceptionWithName:(id)ONSInternalInconsistencyException
                                                          reason:reason
                                                        userInfo:nil]);
        }
        OCFTypeID type = OCFGetTypeID(_stream);
        OCFTypeID dataType = OCFDataGetTypeID();
        void *stream = _stream;
        const uint8_t *bytes = OCFDataGetBytePtr(data);
        OCFIndex length = OCFDataGetLength(data);
        if (type == dataType) {
            OCFDataAppendBytes(stream, bytes, length);
        } else {
            OCFWriteStreamWrite(stream, bytes, length);
        }
        OCFRelease(data);
    } else if ([self outputFormat] == 200) {
        __OCFBinaryPlistWriteToStreamWithOptions(archive, _stream, _estimatedCount, 0);
    }
    OCFRelease(archive);
    if (_delegate != nil && objc_opt_respondsToSelector(_delegate, sel_getUid("archiverDidFinish:"))) {
        [(id)_delegate archiverDidFinish:self];
    }
}

// Native -[NSKeyedArchiver encodedData], 0x1c0557ed4. Non-external.
- (id)encodedData {
    if (!_ONSIsNSData((id)_stream)) {
        return [(id)archivingDataClass data];
    }
    if (!(atomic_load_explicit(&_flags, memory_order_acquire) & 2)) {
        [self finishEncoding];
    }
    return objc_autorelease(objc_retain((id)_stream));
}

// Native -[NSKeyedArchiver replaceObject:withObject:], 0x1c0557f3c. Non-external.
- (void)replaceObject:(id)object withObject:(id)replacement {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: archive already finished, %@"),
                                                               _ONSMethodExceptionProem(self, _cmd),
                                                               (id)OF_STR("cannot replace")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (object != replacement) {
        if (_delegate != nil &&
            objc_opt_respondsToSelector(_delegate, sel_getUid("archiver:willReplaceObject:withObject:"))) {
            [(id)_delegate archiver:self willReplaceObject:object withObject:replacement];
        }
        const void *reference = NULL;
        if (OCFDictionaryGetValueIfPresent(_objRefMap, object, &reference) &&
            (uint32_t)(uintptr_t)reference != UINT32_MAX) {
            OCFDictionaryRemoveValue(_objRefMap, object);
            OCFDictionarySetValue(_objRefMap, replacement, reference);
        }
    }
    if (_replacementMap == NULL) {
        _replacementMap = OCFDictionaryCreateMutable(kOCFAllocatorSystemDefault, 0, NULL, NULL);
        _OCFDictionarySetCapacity(_replacementMap, 128);
    }
    if (_visited == NULL) {
        _visited = OCFSetCreateMutable(kOCFAllocatorSystemDefault, 0, NULL);
        _OCFSetSetCapacity(_visited, 128);
    }
    OCFSetAddValue(_visited, object);
    if (object != replacement) {
        OCFDictionarySetValue(_replacementMap, object, replacement);
    }
}

// Native -[NSKeyedArchiver encodeObject:forKey:], 0x1c055811c. Non-external.
// Hex-Rays truncates self and key to int; assembly passes X0/X1 pointers and W3 = 0.
- (void)encodeObject:(id)object forKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: archive already finished, %@"),
                                                               _ONSMethodExceptionProem(self, _cmd),
                                                               (id)OF_STR("cannot encode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [(id)OF_STR("$") stringByAppendingString:key];
    }
    _encodeObject(self, key, object, 0);
}

// Native _encodeObject, 0x1c05581fc. Non-external. Class placeholders, replacement maps, delegate calls,
// class-name translations and container changes follow the native order; a changed top container bypasses
// the class-record generation. Class references in maps are 32-bit indices zero-extended to pointer width.
static void _encodeObject(ONSKeyedArchiver *coder, id key, id object, int conditional) {
    id original = object;
    BOOL visited = NO;
    if (coder->_visited != NULL) {
        visited = OCFSetContainsValue(coder->_visited, object) != 0;
    }
    BOOL replaced = NO;
    if (coder->_replacementMap != NULL) {
        replaced = OCFDictionaryContainsKey(coder->_replacementMap, object) != 0;
    }
    if (!visited && !replaced) {
        id replacement = [object replacementObjectForKeyedArchiver:coder];
        if (replacement != nil && coder->_delegate != nil &&
            objc_opt_respondsToSelector(coder->_delegate, sel_getUid("archiver:willEncodeObject:"))) {
            replacement = [(id)coder->_delegate archiver:coder willEncodeObject:replacement];
        }
        [coder replaceObject:object withObject:replacement];
        object = replacement;
    } else if (replaced) {
        const void *replacement = original;
        OCFDictionaryGetValueIfPresent(coder->_replacementMap, object, &replacement);
        object = (id)replacement;
    }
    Class cls = [object classForKeyedArchiver];
    if (cls == Nil) {
        cls = objc_opt_class(object);
    }
    if (cls != Nil && [coder requiresSecureCoding]) {
        [coder validateClassSupportsSecureCoding:cls];
    }
    const void *mapped = NULL;
    if (object == nil || (OCFDictionaryGetValueIfPresent(coder->_objRefMap, object, &mapped) &&
                          (uint32_t)(uintptr_t)mapped != UINT32_MAX)) {
        uint32_t reference = object == nil ? 0 : (uint32_t)(uintptr_t)mapped;
        OCFKeyedArchiverUIDRef uid = __NSKeyedArchiverUIDCreateCached(kOCFAllocatorDefault, reference, coder);
        addValueToTopContainerE(coder, key, uid);
        OCFRelease(uid);
        return;
    }
    mapped = NULL;
    uint32_t reference;
    if (OCFDictionaryGetValueIfPresent(coder->_conditionals, object, &mapped)) {
        OCFDictionaryRemoveValue(coder->_conditionals, object);
        reference = (uint32_t)(uintptr_t)mapped;
    } else {
        [coder->_objects addObject:(id)OF_STR("$null")];
        reference = (uint32_t)[coder->_objects count] - 1;
    }
    if (conditional != 0) {
        OCFDictionaryAddValue(coder->_conditionals, object, (const void *)(uintptr_t)reference);
    } else {
        OCFDictionarySetValue(coder->_objRefMap, object, (const void *)(uintptr_t)reference);
    }
    OCFAllocatorRef allocator = kOCFAllocatorDefault;
    OCFKeyedArchiverUIDRef uid = __NSKeyedArchiverUIDCreateCached(allocator, reference, coder);
    addValueToTopContainerE(coder, key, uid);
    OCFRelease(uid);
    if (conditional & 1) {
        return;
    }
    Class codedClass = cls;
    if (class_getName(cls)[0] == '%') {
        codedClass = objc_lookUpClass(class_getName(cls) + 1);
    }
    if (codedClass != Nil) {
        id name = [coder classNameForClass:codedClass];
        if (name == nil) {
            name = [(id)objc_opt_class(coder) classNameForClass:codedClass];
        }
        if (name != nil) {
            codedClass = ONSClassFromString(name);
        }
    }
    _NSCoderCheckPotentiallyMangledClassNameAndWarn(codedClass, 1);
    BOOL scalar;
    if (codedClass == __NSStringClass) {
        scalar = [object length] != 5 || [object compare:(id)OF_STR("$null")] != 0;
    } else {
        scalar = codedClass == (Class)archivingNumberClass || codedClass == (Class)archivingDataClass;
    }
    if (scalar) {
        OCFArraySetValueAtIndex((OCFMutableArrayRef)coder->_objects, reference, object);
    } else {
        uint64_t savedGenericKey = coder->_genericKey;
        coder->_genericKey = 0;
        id container = objc_opt_new((id)archivingMutableDictionaryClass);
        _OCFDictionarySetCapacity((OCFMutableDictionaryRef)container, 16);
        [coder->_containers addObject:container];
        ++coder->_estimatedCount;
        objc_release(container);
        atomic_store_explicit(
            &coder->_flags,
            atomic_load_explicit(&coder->_flags, memory_order_acquire) & ~1ULL,
            memory_order_release
        );
        NSInteger top = (NSInteger)[coder->_containers count] - 1;
        id before = top < 0 ? nil : [coder->_containers objectAtIndexedSubscript:top];
        [object encodeWithCoder:coder];
        top = (NSInteger)[coder->_containers count] - 1;
        id encoded = top < 0 ? nil : [coder->_containers objectAtIndexedSubscript:top];
        if (before == encoded) {
            if (class_getName(cls)[0] == '%') {
                cls = objc_lookUpClass(class_getName(cls) + 1);
            }
            id codedName = nil;
            if (cls != Nil) {
                id name = [coder classNameForClass:cls];
                if (name == nil) {
                    name = [(id)objc_opt_class(coder) classNameForClass:cls];
                }
                if (name != nil) {
                    cls = ONSClassFromString(name);
                    codedName = objc_retain(name);
                }
            }
            BOOL omitHierarchy = NO;
            BOOL omitHints = NO;
            Class hierarchy = cls;
            if (cls == Nil) {
                id cachedClass =
                    coder->_classes != NULL ? (id)OCFDictionaryGetValue(coder->_classes, codedName) : nil;
                omitHierarchy = YES;
                hierarchy = Nil;
                if (cachedClass != nil) {
                    cls = (Class)cachedClass;
                    omitHints = !class_isMetaClass(object_getClass(cachedClass));
                    if (!omitHints) {
                        _NSCoderCheckPotentiallyMangledClassNameAndWarn(cls, 1);
                    }
                } else {
                    if (coder->_classes == NULL) {
                        coder->_classes = OCFDictionaryCreateMutable(
                            kOCFAllocatorSystemDefault,
                            0,
                            &kOCFCopyStringDictionaryKeyCallBacks,
                            &kOCFTypeDictionaryValueCallBacks
                        );
                    }
                    id placeholder = objc_opt_new((id)archivingObjectClass);
                    OCFDictionarySetValue(coder->_classes, codedName, placeholder);
                    objc_release(placeholder);
                    cls = (Class)placeholder;
                    omitHints = YES;
                }
            } else {
                _NSCoderCheckPotentiallyMangledClassNameAndWarn(cls, 1);
            }
            mapped = NULL;
            BOOL hasReference = cls != Nil &&
                                OCFDictionaryGetValueIfPresent(coder->_objRefMap, cls, &mapped) &&
                                (uint32_t)(uintptr_t)mapped != UINT32_MAX;
            if (cls == Nil || hasReference) {
                uint32_t classReference = cls == Nil ? 0 : (uint32_t)(uintptr_t)mapped;
                uid = __NSKeyedArchiverUIDCreateCached(allocator, classReference, coder);
                addValueToTopContainerE(coder, OF_STR("$class"), uid);
                OCFRelease(uid);
                objc_release(codedName);
            } else {
                mapped = NULL;
                uint32_t classReference;
                if (OCFDictionaryGetValueIfPresent(coder->_conditionals, cls, &mapped)) {
                    OCFDictionaryRemoveValue(coder->_conditionals, cls);
                    classReference = (uint32_t)(uintptr_t)mapped;
                } else {
                    [coder->_objects addObject:(id)OF_STR("$null")];
                    classReference = (uint32_t)[coder->_objects count] - 1;
                }
                OCFDictionarySetValue(coder->_objRefMap, cls, (const void *)(uintptr_t)classReference);
                uid = __NSKeyedArchiverUIDCreateCached(allocator, classReference, coder);
                addValueToTopContainerE(coder, OF_STR("$class"), uid);
                OCFRelease(uid);
                id classContainer = objc_opt_new((id)archivingMutableDictionaryClass);
                _OCFDictionarySetCapacity((OCFMutableDictionaryRef)classContainer, 16);
                [coder->_containers addObject:classContainer];
                ++coder->_estimatedCount;
                objc_release(classContainer);
                atomic_store_explicit(
                    &coder->_flags,
                    atomic_load_explicit(&coder->_flags, memory_order_acquire) & ~1ULL,
                    memory_order_release
                );
                if (codedName == nil) {
                    codedName = NSCopyStringFromClass(cls);
                }
                addValueToTopContainerE(coder, OF_STR("$classname"), codedName);
                objc_release(codedName);
                if (!omitHierarchy) {
                    id classes = objc_opt_new((id)archivingMutableArrayClass);
                    for (Class current = hierarchy; current != Nil; current = [(id)current superclass]) {
                        id name = NSCopyStringFromClass(current);
                        OCFArrayAppendValue((OCFMutableArrayRef)classes, name);
                        objc_release(name);
                    }
                    addValueToTopContainerE(coder, OF_STR("$classes"), classes);
                    objc_release(classes);
                }
                id hints = omitHints ? nil : [(id)cls classFallbacksForKeyedArchiver];
                if ([hints count] != 0) {
                    for (id name in hints) {
                        Class hint = ONSClassFromString(name);
                        if (hint != Nil) {
                            _NSCoderCheckPotentiallyMangledClassNameAndWarn(hint, 1);
                        }
                    }
                    addValueToTopContainerE(coder, OF_STR("$classhints"), hints);
                }
                top = (NSInteger)[coder->_containers count] - 1;
                id classRecord = top < 0 ? nil : [coder->_containers objectAtIndexedSubscript:top];
                OCFArraySetValueAtIndex((OCFMutableArrayRef)coder->_objects, classReference, classRecord);
                NSInteger count = [coder->_containers count];
                [coder->_containers removeObjectAtIndex:count - 1];
                if (count - 1 >= 1) {
                    OCFTypeID type = OCFGetTypeID([coder->_containers objectAtIndexedSubscript:count - 2]);
                    uint64_t flags = atomic_load_explicit(&coder->_flags, memory_order_acquire);
                    atomic_store_explicit(
                        &coder->_flags, type == 19 ? flags | 1 : flags & ~1ULL, memory_order_release
                    );
                }
            }
        }
        OCFArraySetValueAtIndex((OCFMutableArrayRef)coder->_objects, reference, encoded);
        NSInteger count = [coder->_containers count];
        [coder->_containers removeObjectAtIndex:count - 1];
        if (count - 1 >= 1) {
            OCFTypeID type = OCFGetTypeID([coder->_containers objectAtIndexedSubscript:count - 2]);
            uint64_t flags = atomic_load_explicit(&coder->_flags, memory_order_acquire);
            atomic_store_explicit(&coder->_flags, (flags & ~1ULL) + (type == 19), memory_order_release);
        }
        coder->_genericKey = savedGenericKey;
    }
    if (coder->_delegate != nil &&
        objc_opt_respondsToSelector(coder->_delegate, sel_getUid("archiver:didEncodeObject:"))) {
        [(id)coder->_delegate archiver:coder didEncodeObject:object];
    }
}

// Native -[NSKeyedArchiver encodeConditionalObject:forKey:], 0x1c0558c0c. Non-external.
- (void)encodeConditionalObject:(id)object forKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: archive already finished, %@"),
                                                               _ONSMethodExceptionProem(self, _cmd),
                                                               (id)OF_STR("cannot encode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [(id)OF_STR("$") stringByAppendingString:key];
    }
    _encodeObject(self, key, object, 1);
}

// Native addValueToTopContainerE, 0x1c0558fa4. Non-external. Duplicate keys still add two to the estimate.
static void addValueToTopContainerE(ONSKeyedArchiver *coder, const void *key, const void *value) {
    NSInteger top = (NSInteger)[coder->_containers count] - 1;
    id container = top < 0 ? nil : [coder->_containers objectAtIndexedSubscript:top];
    if (atomic_load_explicit(&coder->_flags, memory_order_acquire) & 1) {
        OCFArrayAppendValue((OCFMutableArrayRef)container, value);
        ++coder->_estimatedCount;
    } else {
        OCFIndex count = OCFDictionaryGetCount((OCFDictionaryRef)container);
        OCFDictionarySetValue((OCFMutableDictionaryRef)container, key, value);
        coder->_estimatedCount += 2;
        if (count == OCFDictionaryGetCount((OCFDictionaryRef)container)) {
            os_log_t log = _NSOSLog();
            if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
                uint8_t buffer[12];
#define DUPLICATE_KEY_WARNING                                                                                \
    "*** NSKeyedArchiver warning: replacing existing value for key '%@'; probable duplication of encoding "  \
    "keys in class hierarchy"
                __builtin_os_log_format(buffer, DUPLICATE_KEY_WARNING, (id)key);
                _os_log_error_impl(
                    &__dso_handle, log, OS_LOG_TYPE_ERROR, DUPLICATE_KEY_WARNING, buffer, sizeof(buffer)
                );
#undef DUPLICATE_KEY_WARNING
            }
        }
    }
}

// Native -[NSKeyedArchiver encodeBool:forKey:], 0x1c05590d0. Non-external.
- (void)encodeBool:(BOOL)value forKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: archive already finished, %@"),
                                                               _ONSMethodExceptionProem(self, _cmd),
                                                               (id)OF_STR("cannot encode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [(id)OF_STR("$") stringByAppendingString:key];
    }
    addValueToTopContainerE(self, key, value ? kOCFBooleanTrue : kOCFBooleanFalse);
}

// Native -[NSKeyedArchiver encodeInt:forKey:], 0x1c05591c4. Non-external.
- (void)encodeInt:(int)value forKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: archive already finished, %@"),
                                                               _ONSMethodExceptionProem(self, _cmd),
                                                               (id)OF_STR("cannot encode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    OCFNumberRef number = OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberIntType, &value);
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [(id)OF_STR("$") stringByAppendingString:key];
    }
    addValueToTopContainerE(self, key, number);
    OCFRelease(number);
}

// Native -[NSKeyedArchiver encodeInt32:forKey:], 0x1c05592f4. Non-external.
// Assembly preserves the scalar argument across key escaping; decompiler register-width artifacts are not
// used.
- (void)encodeInt32:(int32_t)value forKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: archive already finished, %@"),
                                                               _ONSMethodExceptionProem(self, _cmd),
                                                               (id)OF_STR("cannot encode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [(id)OF_STR("$") stringByAppendingString:key];
    }
    _encodeInt32(self, key, value);
}

// Native __encodeInt32, 0x1c05593d0. Non-external.
static void _encodeInt32(ONSKeyedArchiver *coder, const void *key, int32_t value) {
    OCFNumberRef number = OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberSInt32Type, &value);
    addValueToTopContainerE(coder, key, number);
    OCFRelease(number);
}

// Native -[NSKeyedArchiver encodeInt64:forKey:], 0x1c0559464. Non-external.
// Assembly preserves the scalar argument across key escaping; decompiler register-width artifacts are not
// used.
- (void)encodeInt64:(int64_t)value forKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: archive already finished, %@"),
                                                               _ONSMethodExceptionProem(self, _cmd),
                                                               (id)OF_STR("cannot encode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [(id)OF_STR("$") stringByAppendingString:key];
    }
    _encodeInt64(self, key, value);
}

// Native __encodeInt64, 0x1c0559540. Non-external.
static void _encodeInt64(ONSKeyedArchiver *coder, const void *key, int64_t value) {
    OCFNumberRef number = OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberSInt64Type, &value);
    addValueToTopContainerE(coder, key, number);
    OCFRelease(number);
}

// Native -[NSKeyedArchiver encodeFloat:forKey:], 0x1c05595d4. Non-external.
// Assembly preserves the scalar argument across key escaping; decompiler register-width artifacts are not
// used.
- (void)encodeFloat:(float)value forKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: archive already finished, %@"),
                                                               _ONSMethodExceptionProem(self, _cmd),
                                                               (id)OF_STR("cannot encode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [(id)OF_STR("$") stringByAppendingString:key];
    }
    _encodeFloat(self, key, value);
}

// Native __encodeFloat, 0x1c05596b0. Non-external.
static void _encodeFloat(ONSKeyedArchiver *coder, const void *key, float value) {
    OCFNumberRef number = OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberFloat32Type, &value);
    addValueToTopContainerE(coder, key, number);
    OCFRelease(number);
}

// Native -[NSKeyedArchiver encodeDouble:forKey:], 0x1c0559744. Non-external.
// Assembly preserves the scalar argument across key escaping; decompiler register-width artifacts are not
// used.
- (void)encodeDouble:(double)value forKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: archive already finished, %@"),
                                                               _ONSMethodExceptionProem(self, _cmd),
                                                               (id)OF_STR("cannot encode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [(id)OF_STR("$") stringByAppendingString:key];
    }
    _encodeDouble(self, key, value);
}

// Native __encodeDouble, 0x1c0559820. Non-external.
static void _encodeDouble(ONSKeyedArchiver *coder, const void *key, double value) {
    OCFNumberRef number = OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberFloat64Type, &value);
    addValueToTopContainerE(coder, key, number);
    OCFRelease(number);
}

// Native -[NSKeyedArchiver encodeBytes:length:forKey:], 0x1c05598b4. Non-external.
- (void)encodeBytes:(const uint8_t *)bytes length:(NSUInteger)length forKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: archive already finished, %@"),
                                                               _ONSMethodExceptionProem(self, _cmd),
                                                               (id)OF_STR("cannot encode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (bytes == NULL && length != 0) {
        id reason = [(id)archivingStringClass
            stringWithFormat:
                (id)OF_STR("%@: argument inconsistency: bytes pointer is null but length (%ld) is not zero"),
                _ONSMethodExceptionProem(self, _cmd),
                length];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [(id)OF_STR("$") stringByAppendingString:key];
    }
    _encodeBytes(self, bytes, length, key);
}

// Native __encodeBytes, 0x1c05599e0. Non-external.
static void _encodeBytes(ONSKeyedArchiver *coder, const uint8_t *bytes, OCFIndex length, const void *key) {
    if (bytes != NULL) {
        OCFDataRef data = OCFDataCreate(kOCFAllocatorDefault, bytes, length);
        addValueToTopContainerE(coder, key, data);
        OCFRelease(data);
    } else {
        addValueToTopContainerE(coder, key, OF_STR("$null"));
    }
}

// Native -[NSKeyedArchiver _encodePropertyList:forKey:], 0x1c0559a54. Non-external.
- (void)_encodePropertyList:(id)plist forKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: archive already finished, %@"),
                                                               _ONSMethodExceptionProem(self, _cmd),
                                                               (id)OF_STR("cannot encode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [(id)OF_STR("$") stringByAppendingString:key];
    }
    addValueToTopContainerE(self, key, plist);
}

// Native -[NSKeyedArchiver encodeObject:], 0x1c0559b30. Non-external.
- (void)encodeObject:(id)object {
    if (_warnArchiverCompat == 1) {
        ONSLog(
            (id)OF_STR("%@: warning: old archiving compatibility method called"),
            _ONSMethodExceptionProem(self, _cmd)
        );
    }
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: archive already finished, %@"),
                                                               _ONSMethodExceptionProem(self, _cmd),
                                                               (id)OF_STR("cannot encode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    uint64_t index = _genericKey;
    _genericKey = index + 1;
    id key = index > 39 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), index]
                        : (id)generic_keys[index];
    _encodeObject(self, key, object, 0);
}

// Native -[NSKeyedArchiver encodeConditionalObject:], 0x1c0559c48. Non-external.
- (void)encodeConditionalObject:(id)object {
    if (_warnArchiverCompat == 1) {
        ONSLog(
            (id)OF_STR("%@: warning: old archiving compatibility method called"),
            _ONSMethodExceptionProem(self, _cmd)
        );
    }
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: archive already finished, %@"),
                                                               _ONSMethodExceptionProem(self, _cmd),
                                                               (id)OF_STR("cannot encode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    uint64_t index = _genericKey;
    _genericKey = index + 1;
    id key = index > 39 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), index]
                        : (id)generic_keys[index];
    _encodeObject(self, key, object, 1);
}

// Native -[NSKeyedArchiver encodeBycopyObject:], 0x1c0559d60. Non-external.
// Tail message send; X2 (the object) is preserved, although Hex-Rays omits it.
- (void)encodeBycopyObject:(id)object {
    [self encodeObject:object];
}

// Native -[NSKeyedArchiver encodeBytes:length:], 0x1c0559d70. Non-external.
- (void)encodeBytes:(const void *)bytes length:(NSUInteger)length {
    if (_warnArchiverCompat == 1) {
        ONSLog(
            (id)OF_STR("%@: warning: old archiving compatibility method called"),
            _ONSMethodExceptionProem(self, _cmd)
        );
    }
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: archive already finished, %@"),
                                                               _ONSMethodExceptionProem(self, _cmd),
                                                               (id)OF_STR("cannot encode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    uint64_t index = _genericKey;
    _genericKey = index + 1;
    id key = index > 39 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), index]
                        : (id)generic_keys[index];
    _encodeBytes(self, bytes, length, key);
}

// Native -[NSKeyedArchiver encodeValuesOfObjCTypes:], 0x1c0559e8c. Non-external.
// The types pointer advances one byte for each address, as in the native compatibility method (kept).
- (void)encodeValuesOfObjCTypes:(const char *)types, ... {
    if (_warnArchiverCompat == 1) {
        ONSLog(
            (id)OF_STR("%@: warning: old archiving compatibility method called"),
            _ONSMethodExceptionProem(self, _cmd)
        );
    }
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: archive already finished, %@"),
                                                               _ONSMethodExceptionProem(self, _cmd),
                                                               (id)OF_STR("cannot encode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (types == NULL) {
        id reason = [(id)archivingStringClass
            stringWithFormat:(id)OF_STR("%@: null types pointer"), _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    va_list arguments;
    va_start(arguments, types);
    for (; *types != '\0'; ++types) {
        const void *address = va_arg(arguments, const void *);
        if (address == NULL) {
            id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: null address pointer"),
                                                                   _ONSMethodExceptionProem(self, _cmd)];
            objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                          reason:reason
                                                        userInfo:nil]);
        }
        [self encodeValueOfObjCType:types at:address];
    }
    va_end(arguments);
}

// Native -[NSKeyedArchiver encodeValueOfObjCType:at:], 0x1c055a024. Non-external.
- (void)encodeValueOfObjCType:(const char *)type at:(const void *)address {
    if (_warnArchiverCompat == 1) {
        ONSLog(
            (id)OF_STR("%@: warning: old archiving compatibility method called"),
            _ONSMethodExceptionProem(self, _cmd)
        );
    }
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: archive already finished, %@"),
                                                               _ONSMethodExceptionProem(self, _cmd),
                                                               (id)OF_STR("cannot encode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (type == NULL || address == NULL) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: null type or address pointer"),
                                                               _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (*type != '[') {
        if (*type == '{') {
            id reason = [(id)archivingStringClass
                stringWithFormat:(id)OF_STR("%@: this archiver cannot encode structs"),
                                 _ONSMethodExceptionProem(self, _cmd)];
            objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                          reason:reason
                                                        userInfo:nil]);
        }
        _compatEncodeValueOfObjCType(self, (signed char)*type, address, _cmd);
        return;
    }
    const unsigned char *element = (const unsigned char *)type + 1;
    uint64_t count = 0;
    while ((unsigned int)(*element - '0') <= 9) {
        count = count * 10 + *element - '0';
        ++element;
    }
    if (count == 0) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: array count is missing or zero"),
                                                       _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (*element == ']') {
        id reason = [(id)archivingStringClass
            stringWithFormat:(id)OF_STR("%@: array type is missing"), _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    [self encodeArrayOfObjCType:(const char *)element count:count at:address];
}

// Native __compatEncodeValueOfObjCType, 0x1c055a228. Non-external.
// The c/C and s/S pairs both sign-extend. i/I/l/L all read 32 bits on this arm64 image (kept).
static void
_compatEncodeValueOfObjCType(ONSKeyedArchiver *coder, int type, const void *address, SEL selector) {
    switch (type) {
        case 'c':
        case 'C': {
            uint64_t index = coder->_genericKey;
            coder->_genericKey = index + 1;
            id key = index > 39 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), index]
                                : (id)generic_keys[index];
            int8_t value;
            memcpy(&value, address, sizeof(value));
            _encodeInt32(coder, key, value);
            return;
        }
        case 's':
        case 'S': {
            uint64_t index = coder->_genericKey;
            coder->_genericKey = index + 1;
            id key = index > 39 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), index]
                                : (id)generic_keys[index];
            int16_t value;
            memcpy(&value, address, sizeof(value));
            _encodeInt32(coder, key, value);
            return;
        }
        case 'i':
        case 'I':
        case 'l':
        case 'L': {
            uint64_t index = coder->_genericKey;
            coder->_genericKey = index + 1;
            id key = index > 39 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), index]
                                : (id)generic_keys[index];
            int32_t value;
            memcpy(&value, address, sizeof(value));
            _encodeInt32(coder, key, value);
            return;
        }
        case 'q':
        case 'Q': {
            uint64_t index = coder->_genericKey;
            coder->_genericKey = index + 1;
            id key = index > 39 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), index]
                                : (id)generic_keys[index];
            int64_t value;
            memcpy(&value, address, sizeof(value));
            _encodeInt64(coder, key, value);
            return;
        }
        case 'f': {
            uint64_t index = coder->_genericKey;
            coder->_genericKey = index + 1;
            id key = index > 39 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), index]
                                : (id)generic_keys[index];
            float value;
            memcpy(&value, address, sizeof(value));
            _encodeFloat(coder, key, value);
            return;
        }
        case 'd': {
            uint64_t index = coder->_genericKey;
            coder->_genericKey = index + 1;
            id key = index > 39 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), index]
                                : (id)generic_keys[index];
            double value;
            memcpy(&value, address, sizeof(value));
            _encodeDouble(coder, key, value);
            return;
        }
        case 'B': {
            uint64_t index = coder->_genericKey;
            coder->_genericKey = index + 1;
            id key = index > 39 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), index]
                                : (id)generic_keys[index];
            uint8_t value = *(const uint8_t *)address;
            addValueToTopContainerE(coder, key, value ? kOCFBooleanTrue : kOCFBooleanFalse);
            return;
        }
        case '@': {
            uint64_t index = coder->_genericKey;
            coder->_genericKey = index + 1;
            id key = index > 39 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), index]
                                : (id)generic_keys[index];
            _encodeObject(coder, key, *(id const *)address, 0);
            return;
        }
        case '#': {
            Class cls = *(Class const *)address;
            _NSCoderCheckPotentiallyMangledClassNameAndWarn(cls, 1);
            uint64_t index = coder->_genericKey;
            coder->_genericKey = index + 1;
            id key = index > 39 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), index]
                                : (id)generic_keys[index];
            _encodeObject(coder, key, ONSStringFromClass(cls), 0);
            return;
        }
        case ':': {
            id name = NSCopyStringFromSelector(*(SEL const *)address);
            uint64_t index = coder->_genericKey;
            coder->_genericKey = index + 1;
            id key = index > 39 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), index]
                                : (id)generic_keys[index];
            _encodeObject(coder, key, name, 0);
            objc_release(name);
            return;
        }
        case '%': {
            ONSLog(
                (id)OF_STR("%@: warning: NXAtom being encoded -- will not be decoded uniqued"),
                _ONSMethodExceptionProem(coder, selector)
            );
        }
            __attribute__((fallthrough));
        case '*': {
            uint64_t index = coder->_genericKey;
            coder->_genericKey = index + 1;
            id key = index > 39 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), index]
                                : (id)generic_keys[index];
            id name = [(id)archivingStringClass stringWithUTF8String:*(const char *const *)address];
            _encodeObject(coder, key, name, 0);
            return;
        }
        default: {
            id reason =
                [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unknown type encoding ('%c')"),
                                                           _ONSMethodExceptionProem(coder, selector),
                                                           type];
            objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                          reason:reason
                                                        userInfo:nil]);
        }
    }
}

// Native -[NSKeyedArchiver encodeArrayOfObjCType:count:at:], 0x1c055a7e4. Non-external.
// Folded cleanup landing pads 0x1c055a9b0-0x1c055a9d8; LSDA 0x1c14a1e2c.
- (void)encodeArrayOfObjCType:(const char *)type count:(NSUInteger)count at:(const void *)address {
    if (_warnArchiverCompat == 1) {
        ONSLog(
            (id)OF_STR("%@: warning: old archiving compatibility method called"),
            _ONSMethodExceptionProem(self, _cmd)
        );
    }
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: archive already finished, %@"),
                                                               _ONSMethodExceptionProem(self, _cmd),
                                                               (id)OF_STR("cannot encode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    id reason = nil;
    if (type == NULL || address == NULL) {
        reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: null type or address pointer"),
                                                            _ONSMethodExceptionProem(self, _cmd)];
    } else if (count == 0) {
        reason = [(id)archivingStringClass
            stringWithFormat:(id)OF_STR("%@: count is zero"), _ONSMethodExceptionProem(self, _cmd)];
    } else if (*type == '(' || *type == '{' || *type == '[') {
        reason = [(id)archivingStringClass
            stringWithFormat:(id)OF_STR("%@: unsupported type \"%s\" for array encoding"),
                             _ONSMethodExceptionProem(self, _cmd),
                             type];
    }
    if (reason != nil) {
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    id array = [objc_allocWithZone((Class)archivingOldStyleArrayClass) initWithObjCType:*type
                                                                                  count:count
                                                                                     at:address];
    @try {
        [self encodeObject:array];
    } @finally {
        objc_release(array);
    }
}

// Native -[NSKeyedArchiver versionForClassName:], 0x1c055a9dc. Non-external.
- (NSInteger)versionForClassName:(id)name {
    Class cls = ONSClassFromString(name);
    return cls != Nil ? [(id)cls version] : INT64_MAX;
}

// Native -[NSKeyedArchiver requiresSecureCoding], 0x1c055aa0c. Non-external.
- (BOOL)requiresSecureCoding {
    return (atomic_load_explicit(&_flags, memory_order_acquire) & 4) != 0;
}

// Native -[NSKeyedArchiver setRequiresSecureCoding:], 0x1c055aa24. Non-external.
- (void)setRequiresSecureCoding:(BOOL)requiresSecureCoding {
    uint64_t flags = atomic_load_explicit(&_flags, memory_order_acquire);
    atomic_store_explicit(&_flags, (flags & ~4ULL) | (requiresSecureCoding ? 4 : 0), memory_order_release);
}

@end

// Native ___NSKeyedArchiverUIDCreateCached, 0x1c0563d14. Non-external.
// Foundation defect: 0x1c0563d58-0x1c0563d70 chooses max(uid, doubled 32-bit capacity), then
// 0x1c0563dcc indexes cache[uid]. A sparse uid at or beyond the doubled capacity reads/writes past the
// allocation. Reserve uid + 1 entries and grow in 64 bits; ordinary sequential growth is unchanged. The
// matching dealloc loop is widened above. No destructive reference probe is run.
static OCFKeyedArchiverUIDRef
__NSKeyedArchiverUIDCreateCached(OCFAllocatorRef allocator, uint32_t uid, ONSKeyedArchiver *coder) {
    uint64_t oldSize = coder->_cacheSize;
    if (coder->_cache == NULL || oldSize <= uid) {
        uint64_t size = oldSize == 0 ? 256 : oldSize * 2;
        if (size <= uid) {
            size = (uint64_t)uid + 1;
        }
        coder->_cacheSize = size;
        coder->_cache = __OCFSafelyReallocateWithAllocatorTyped(
            kOCFAllocatorSystemDefault,
            (void *)coder->_cache,
            size * sizeof(*coder->_cache),
            0x6004044c4a2dfULL,
            0,
            nil
        );
        bzero((void *)(coder->_cache + oldSize), (coder->_cacheSize - oldSize) * sizeof(*coder->_cache));
    }
    OCFKeyedArchiverUIDRef object = coder->_cache[uid];
    if (object == NULL) {
        object = _OCFKeyedArchiverUIDCreate(allocator, uid);
        coder->_cache[uid] = object;
    }
    return OCFRetain(object);
}

@implementation _ONSKeyedCoderOldStyleArray (Encoding)

// Native -[_NSKeyedCoderOldStyleArray encodeWithCoder:], 0x1c0563738. Non-external.
// This method shares the native file-local compatibility encoder.
- (void)encodeWithCoder:(id)coder {
    ONSKeyedArchiver *archiver = coder;
    [archiver encodeInt32:(int32_t)_count forKey:(id)OF_STR("NS.count")];
    [archiver encodeInt32:(int32_t)_size forKey:(id)OF_STR("NS.size")];
    [archiver encodeInt32:(int32_t)_type forKey:(id)OF_STR("NS.type")];
    SEL selector = sel_getUid("encodeArrayOfObjCType:count:at:");
    for (NSUInteger index = 0; index < _count; ++index) {
        _compatEncodeValueOfObjCType(archiver, _type, (const char *)_addr + _size * index, selector);
    }
}

@end

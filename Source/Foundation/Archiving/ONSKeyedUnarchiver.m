//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Archiving/ONSKeyedUnarchiver.h"

#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFData.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Number/OCFNumber.h"
#import "CoreFoundation/Objective-C/OCFNSArray.h"
#import "CoreFoundation/Objective-C/OCFNSDictionary.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#include "CoreFoundation/PropertyList/OCFKeyedArchiverUID.h"
#import "Foundation/Archiving/ONSArchivingPrivate.h"
#import "Foundation/Archiving/ONSKeyedArchiver.h"
#import "Foundation/Archiving/ONSObjCRuntimeNames.h"
#import "Foundation/Archiving/ONSRuntimeIssuesLog.h"

#import "Foundation/Number/ONSUnrestoredInterfaces.h"

#include <alloca.h>
#include <float.h>
#include <malloc/malloc.h>
#include <math.h>
#include <objc/objc-exception.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. MRC ownership and native acquire/release atomic operations are preserved.
// iOS Device divergence: the iOS Device 24A446 bodies remain unread.
// Reading: Source/Foundation/Archiving/Reading.md.

// The verified helper overlay is an access to another unit's object, not a definition of that class.
#define UNARCHIVER_HELPER(coder) ((KeyedUnarchiverHelperFields *)(void *)(coder)->_helper)

// Native __MergedGlobals.43: class-name lock 0x22e76c5ec, map 0x22e76c5f8, method-initializer token
// 0x22e76c600, three IMPs at 0x22e76c608. These do not overlap the archiver's fields.
static os_unfair_lock nameClassMapLock = OS_UNFAIR_LOCK_INIT;
static OCFMutableDictionaryRef nameClassMap;
static dispatch_once_t qword_22E76C600;
IMP keyedUnarchiverBaseImplementations[3];
static dispatch_once_t _decodeObjectReference_onceToken;
static void YOU_HAVE_A_CYCLE_IN_YOUR_OBJECT_GRAPH__THIS_ISNT_A_FOUNDATION_BUG__SEE_ERROR_MESSAGE_(void);

static bool _decodeOffsetAndRef(ONSKeyedUnarchiver *coder, id key, uint64_t *offset, uint64_t *reference);
static bool _decodeObjectBinary(ONSKeyedUnarchiver *coder, uint32_t reference, id key, id *result);
static bool _decodeObjectXML(ONSKeyedUnarchiver *coder, const void *key, id *result);
static bool decodingCyclesIsAllowedForCoder(ONSKeyedUnarchiver *coder);
static bool
_readInt(id coder, const uint8_t *marker, const uint8_t *limit, uint64_t *value, const uint8_t **next);
static bool _getUIDFromData(id coder, const uint8_t *marker, const uint8_t *limit, uint64_t *value);
static bool _decodeBool(ONSKeyedUnarchiver *coder, const void *key, bool *result);
static bool _decodeInt32(ONSKeyedUnarchiver *coder, const void *key, int32_t *result);
static bool _decodeInt64(ONSKeyedUnarchiver *coder, const void *key, uint64_t *result);
static bool _decodeFloat(ONSKeyedUnarchiver *coder, const void *key, float *result);
static bool _decodeDouble(ONSKeyedUnarchiver *coder, const void *key, double *result);
static const uint8_t *_decodeBytes(ONSKeyedUnarchiver *coder, const void *key, NSUInteger *length);
static bool _compatDecodeValueOfObjCType(ONSKeyedUnarchiver *coder, int type, void *address, SEL selector);
static bool
_decodeObjectReference(ONSKeyedUnarchiver *coder, NSUInteger format, id key, uint32_t reference, id *result);
static id coderAllocateClassForUnarchival(ONSKeyedUnarchiver *coder, Class cls, bool *allowed);
static bool _coderValidateClassValidToMessageForUnarchiving(Class cls);
static bool _decodeObject(ONSKeyedUnarchiver *coder, const void *key, id *result);

@implementation ONSKeyedUnarchiver

// Native +[NSKeyedUnarchiver initialize], 0x1c055abf0. Non-external.
+ (void)initialize {
    if (self == (id)archivingUnarchiverClass) {
        if (getenv("NSWarnAboutOldStyleArchiverMethods") != NULL) {
            _warnArchiverCompat = 1;
        }
        Class cls = objc_lookUpClass(OF_NAME_STRING(__ONSLocalTimeZone));
        [(id)archivingUnarchiverClass setClass:cls forClassName:(id)OF_STR("NSLocalTimeZone")];
    }
}

// Native -[NSKeyedUnarchiver init], 0x1c055ad40. Non-external. self is released before the exception.
- (id)init {
    id reason =
        [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: cannot use -init for initialization"),
                                                   _ONSMethodExceptionProem(self, _cmd)];
    objc_release(self);
    objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                  reason:reason
                                                userInfo:nil]);
}

// Native -[NSKeyedUnarchiver _initWithStream:data:topDict:], 0x1c055ada0. Non-external. None of the three
// explicit arguments is read. There is no super-init call.
- (id)_initWithStream:(void *)stream data:(id)data topDict:(id)topDict {
    _objRefMap = OCFDictionaryCreateMutable(NULL, 0, NULL, NULL);
    _OCFDictionarySetCapacity(_objRefMap, 128);
    _tmpRefObjMap = OCFDictionaryCreateMutable(kOCFAllocatorSystemDefault, 0, NULL, NULL);
    _OCFDictionarySetCapacity(_tmpRefObjMap, 16);
    _refObjMap = OCFDictionaryCreateMutable(NULL, 0, NULL, &kOCFTypeDictionaryValueCallBacks);
    _OCFDictionarySetCapacity(_refObjMap, 128);
    return self;
}

// Native -[NSKeyedUnarchiver _initForReadingFromData:error:throwLegacyExceptions:], 0x1c055ae40.
// Non-external. XML fallback begins at 120 bytes, not at the binary minimum. The byte diagnostic sign-extends
// each of its eight bytes. Header signatures compare overlapping eight-byte words at offsets 2 and 5.
// Folded cleanup landing pads 0x1c055b884-0x1c055b92c; LSDA 0x1c14a1e60. Self cleanup encloses
// helper allocation and the meter cleanup; a throwing invalidate still reaches self cleanup on failure.
- (id)_initForReadingFromData:(id)data error:(id *)error throwLegacyExceptions:(BOOL)throwLegacyExceptions {
    id versionObject = nil;
    int32_t version = -1;
    uint8_t marker = 0;
    uint64_t rootOffset = 0;
    uint64_t versionOffset = 0;
    uint64_t topOffset = 0;
    BinaryPlistTrailer trailer = {0};
    _data = NULL;
    atomic_store_explicit(&_flags, 16, memory_order_release);
    _delegate = nil;
    _containers = NULL;
    _objects = NULL;
    _objRefMap = NULL;
    _replacementMap = NULL;
    _nameClassMap = NULL;
    _tmpRefObjMap = NULL;
    _refObjMap = NULL;
    _genericKey = 0;
    id meter = nil;
    id result = nil;
    @try {
        _helper = objc_alloc_init((Class)archivingHelperClass);
        _offsetObjMap = NULL;
        atomic_store_explicit(
            &_flags, atomic_load_explicit(&_flags, memory_order_acquire) | 4, memory_order_release
        );
        atomic_store_explicit(
            &_flags, atomic_load_explicit(&_flags, memory_order_acquire) & ~16ULL, memory_order_release
        );
        if (data == nil) {
            if (error != NULL) {
                id description = [objc_alloc((Class)archivingStringClass)
                    initWithFormat:(id)OF_STR("%@: data is NULL"), _ONSMethodExceptionProem(self, _cmd)];
                id key = (id)ONSDebugDescriptionErrorKey;
                id dictionary = [(id)archivingDictionaryClass dictionaryWithObjects:&description
                                                                            forKeys:&key
                                                                              count:1];
                *error = [(id)archivingErrorClass errorWithDomain:(id)ONSCocoaErrorDomain
                                                             code:4864
                                                         userInfo:dictionary];
                objc_release(description);
            }
            return nil;
        }
        @try {
            meter = [objc_alloc((Class)archivingPerformanceMeterClass) initWithTarget:self];
            const uint8_t *bytes = [data bytes];
            uint64_t length = [data length];
            if (length == 0 || bytes == NULL) {
                if (throwLegacyExceptions) {
                    [self setDecodingFailurePolicy:0];
                }
                [self __failWithExceptionName:(id)ONSInvalidArgumentException
                                    errorCode:4864
                                       format:
                                           (id)OF_STR(
                                               "%@: data is empty; did you forget to send -finishEncoding to "
                                               "the NSKeyedArchiver?"
                                           ),
                                           _ONSMethodExceptionProem(self, _cmd)];
                goto copyError;
            }
            if (length >= 13 && bytes[1] == 11) {
                uint64_t first;
                uint64_t second;
                memcpy(&first, bytes + 2, 8);
                memcpy(&second, bytes + 5, 8);
                if ((first == 0x7274736465707974ULL && second == 0x6d61657274736465ULL) ||
                    (first == 0x79746d6165727473ULL && second == 0x64657079746d6165ULL)) {
                    if (throwLegacyExceptions) {
                        [self setDecodingFailurePolicy:0];
                    }
                    [self
                        __failWithExceptionName:(id)ONSInvalidArgumentException
                                      errorCode:4864
                                         format:
                                             (id)OF_STR(
                                                 "%@: non-keyed archive cannot be decoded by " OF_NAME_STRING(
                                                     ONSKeyedUnarchiver
                                                 )
                                             ),
                                             _ONSMethodExceptionProem(self, _cmd)];
                    goto copyError;
                }
            }
            if (length < 8) {
                goto incomprehensible;
            }
            if (__OCFBinaryPlistGetTopLevelInfo(bytes, length, &marker, &rootOffset, &trailer) &&
                __OCFBinaryPlistGetOffsetForValueFromDictionary3(
                    bytes,
                    length,
                    rootOffset,
                    &trailer,
                    OF_STR("$version"),
                    NULL,
                    &versionOffset,
                    NULL,
                    _offsetObjMap
                ) &&
                __OCFBinaryPlistCreateObject(
                    bytes,
                    length,
                    versionOffset,
                    &trailer,
                    NULL,
                    0,
                    _offsetObjMap,
                    (OCFPropertyListRef *)&versionObject
                ) &&
                objc_autorelease(versionObject) != nil) {
                OCFTypeID type = OCFGetTypeID(versionObject);
                if (type == OCFNumberGetTypeID() &&
                    OCFNumberGetValue((OCFNumberRef)versionObject, kOCFNumberSInt32Type, &version) &&
                    version == 100000) {
                    if (!__OCFBinaryPlistGetOffsetForValueFromDictionary3(
                            bytes,
                            length,
                            rootOffset,
                            &trailer,
                            OF_STR("$top"),
                            NULL,
                            &topOffset,
                            NULL,
                            _offsetObjMap
                        )) {
                        goto corruptComponents;
                    }
                    if (!__OCFBinaryPlistGetOffsetForValueFromDictionary3(
                            bytes,
                            length,
                            rootOffset,
                            &trailer,
                            OF_STR("$objects"),
                            NULL,
                            &rootOffset,
                            NULL,
                            _offsetObjMap
                        )) {
                        goto corruptComponents;
                    }
                    if ((bytes[rootOffset] & 0xf0) != 0xa0) {
                        if (error != NULL) {
                            id description = [objc_alloc((Class)archivingStringClass)
                                initWithFormat:
                                    (id)OF_STR(
                                        "%@: data is corrupt or archive is missing key components or "
                                        "object is too large to decode"
                                    ),
                                    _ONSMethodExceptionProem(self, _cmd)];
                            id key = (id)ONSDebugDescriptionErrorKey;
                            id dictionary = [(id)archivingDictionaryClass dictionaryWithObjects:&description
                                                                                        forKeys:&key
                                                                                          count:1];
                            *error = [(id)archivingErrorClass errorWithDomain:(id)ONSCocoaErrorDomain
                                                                         code:4864
                                                                     userInfo:dictionary];
                            objc_release(description);
                        }
                        goto finish;
                    }
                    _offsetData =
                        OCFAllocatorAllocateTyped(kOCFAllocatorSystemDefault, 48, 0x1000040eed21634ULL, 0);
                    _offsetData->trailer = trailer;
                    _offsetData->containerOffset = topOffset;
                    _offsetData->objectsOffset = rootOffset;
                    _data = (void *)OCFRetain(data);
                    _bytes = (const char *)bytes;
                    _len = length;
                    _objRefMap = OCFDictionaryCreateMutable(NULL, 0, NULL, NULL);
                    _tmpRefObjMap = OCFDictionaryCreateMutable(kOCFAllocatorSystemDefault, 0, NULL, NULL);
                    _refObjMap = OCFDictionaryCreateMutable(NULL, 0, NULL, &kOCFTypeDictionaryValueCallBacks);
                    _OCFDictionarySetCapacity(_objRefMap, trailer.numObjects / 5);
                    _OCFDictionarySetCapacity(_refObjMap, trailer.numObjects / 5);
                    _offsetObjMap =
                        OCFDictionaryCreateMutable(NULL, 0, NULL, &kOCFTypeDictionaryValueCallBacks);
                    result = self;
                    goto finish;
                }
            }
            if (length < 120) {
                goto incomprehensible;
            }
            OCFPropertyListRef archive =
                OCFPropertyListCreateWithData(kOCFAllocatorSystemDefault, (OCFDataRef)data, 1, NULL, NULL);
            if (archive == NULL) {
                versionObject = nil;
                goto incomprehensible;
            }
            OCFTypeID dictionaryType = OCFDictionaryGetTypeID();
            if (dictionaryType != OCFGetTypeID(archive)) {
                versionObject = nil;
                goto invalidVersion;
            }
            versionObject = (id)OCFDictionaryGetValue(archive, OF_STR("$version"));
            if (versionObject == nil || OCFGetTypeID(versionObject) != OCFNumberGetTypeID() ||
                !OCFNumberGetValue((OCFNumberRef)versionObject, kOCFNumberSInt32Type, &version) ||
                version != 100000) {
                goto invalidVersion;
            }
            const void *top = OCFDictionaryGetValue(archive, OF_STR("$top"));
            const void *objects = OCFDictionaryGetValue(archive, OF_STR("$objects"));
            if (top == NULL || objects == NULL || OCFGetTypeID(objects) != 19) {
                OCFRelease(archive);
                goto corruptComponents;
            }
            _containers = OCFArrayCreateMutable(NULL, 0, NULL);
            OCFRetain(top);
            OCFArrayAppendValue(_containers, top);
            OCFTypeID topType = OCFGetTypeID(top);
            uint64_t flags = atomic_load_explicit(&_flags, memory_order_acquire);
            atomic_store_explicit(&_flags, (flags & ~1ULL) + (topType == 19), memory_order_release);
            _objects = (void *)OCFRetain(objects);
            [self _initWithStream:NULL data:nil topDict:(id)archive];
            OCFRelease(archive);
            result = self;
            goto finish;
        invalidVersion:
            OCFRelease(archive);
            if (throwLegacyExceptions) {
                [self setDecodingFailurePolicy:0];
            }
            [self __failWithExceptionName:(id)ONSInvalidArgumentException
                                errorCode:4864
                                   format:(id)OF_STR("%@: incomprehensible archive version (%d)"),
                                          _ONSMethodExceptionProem(self, _cmd),
                                          version];
            goto copyError;
        corruptComponents:
            if (throwLegacyExceptions) {
                [self setDecodingFailurePolicy:0];
            }
            [self __failWithExceptionName:(id)ONSInconsistentArchiveException
                                errorCode:4864
                                   format:(id)OF_STR(
                                              "%@: data is corrupt or archive is missing key components or "
                                              "object is too large to decode"
                                          ),
                                          _ONSMethodExceptionProem(self, _cmd)];
            goto copyError;
        incomprehensible:
            {
                signed char prefix[8] = {0};
                [data getBytes:prefix range:(OCFRange){0, length < 8 ? length : 8}];
                if (throwLegacyExceptions) {
                    [self setDecodingFailurePolicy:0];
                }
                [self
                    __failWithExceptionName:(id)ONSInvalidArgumentException
                                  errorCode:4864
                                     format:(id)OF_STR(
                                                "%@: incomprehensible archive (0x%x, 0x%x, 0x%x, 0x%x, 0x%x, "
                                                "0x%x, 0x%x, 0x%x)"
                                            ),
                                            _ONSMethodExceptionProem(self, _cmd),
                                            (long)prefix[0],
                                            (long)prefix[1],
                                            (long)prefix[2],
                                            (long)prefix[3],
                                            (long)prefix[4],
                                            (long)prefix[5],
                                            (long)prefix[6],
                                            (long)prefix[7]];
            }
        copyError:
            if (error != NULL) {
                *error = objc_autorelease(objc_retain([self error]));
            }
        finish:
            (void)0;
        } @finally {
            if (meter != nil) {
                [meter invalidate];
                objc_release(meter);
            }
        }
    } @finally {
        if (result == nil) {
            objc_release(self);
        }
    }
    return result;
}

// Native -[NSKeyedUnarchiver initForReadingFromData:error:], 0x1c055b930. Non-external.
- (id)initForReadingFromData:(id)data error:(id *)error {
    return [self _initForReadingFromData:data error:error throwLegacyExceptions:NO];
}

// Native +[NSKeyedUnarchiver unarchivedObjectOfClass:fromData:error:], 0x1c055b938. Non-external.
// Folded cleanup landing pads 0x1c055b9e8-0x1c055ba28; LSDA 0x1c14a1ef0.
+ (id)unarchivedObjectOfClass:(Class)cls fromData:(id)data error:(id *)error {
    id classes = [objc_alloc((Class)archivingSetClass) initWithObjects:(const id *)&cls count:1];
    id result;
    @try {
        result = [self unarchivedObjectOfClasses:classes fromData:data error:error];
    } @finally {
        objc_release(classes);
    }
    return result;
}

// Native +[NSKeyedUnarchiver unarchivedObjectOfClasses:fromData:error:], 0x1c055bc3c. Non-external.
// Folded cleanup landing pads 0x1c055bccc-0x1c055bcf8; LSDA 0x1c14a1f9c.
+ (id)unarchivedObjectOfClasses:(id)classes fromData:(id)data error:(id *)error {
    id coder = [objc_alloc(self) initForReadingFromData:data error:error];
    if (coder == nil) {
        return nil;
    }
    [coder setRequiresSecureCoding:YES];
    [coder setDecodingFailurePolicy:1];
    id result;
    @try {
        result = [coder decodeTopLevelObjectOfClasses:classes forKey:(id)OF_STR("root") error:error];
    } @finally {
        objc_release(coder);
    }
    return result;
}

// Native -[NSKeyedUnarchiver dealloc], 0x1c055c894. Non-external. The raw container array owns no entries;
// each popped entry has a separate retain that is released before the remaining container's type is read.
- (void)dealloc {
    if (_data != NULL) {
        OCFRelease(_data);
    }
    if (_containers != NULL) {
        OCFIndex count;
        while ((count = OCFArrayGetCount(_containers)) >= 1) {
            if (OCFArrayGetValueAtIndex(_containers, count - 1) == NULL) {
                break;
            }
            count = OCFArrayGetCount(_containers);
            const void *value = OCFArrayGetValueAtIndex(_containers, count - 1);
            OCFArrayRemoveValueAtIndex(_containers, count - 1);
            OCFRelease(value);
            if (count > 1) {
                const void *top = OCFArrayGetValueAtIndex(_containers, count - 2);
                OCFTypeID type = OCFGetTypeID(top);
                uint64_t flags = atomic_load_explicit(&_flags, memory_order_acquire);
                atomic_store_explicit(&_flags, (flags & ~1ULL) + (type == 19), memory_order_release);
            }
        }
        OCFRelease(_containers);
    }
    if (_objects != NULL) {
        OCFRelease(_objects);
    }
    if (_objRefMap != NULL) {
        OCFRelease(_objRefMap);
    }
    if (_replacementMap != NULL) {
        OCFRelease(_replacementMap);
    }
    if (_nameClassMap != NULL) {
        OCFRelease(_nameClassMap);
    }
    if (_tmpRefObjMap != NULL) {
        OCFRelease(_tmpRefObjMap);
    }
    if (_refObjMap != NULL) {
        OCFRelease(_refObjMap);
    }
    if (_offsetData != NULL) {
        OCFAllocatorDeallocate(kOCFAllocatorSystemDefault, _offsetData);
    }
    objc_release(_helper);
    if (_offsetObjMap != NULL) {
        OCFRelease(_offsetObjMap);
    }
    [super dealloc];
}

// Native -[NSKeyedUnarchiver description], 0x1c055ca98. Non-external.
- (id)description {
    return [super description];
}

// Native -[NSKeyedUnarchiver allowsKeyedCoding], 0x1c055caf8. Non-external.
- (BOOL)allowsKeyedCoding {
    return YES;
}

// Native -[NSKeyedUnarchiver setDelegate:], 0x1c055cb08. Non-external. Assign, with no retain or weak store.
- (void)setDelegate:(id)delegate {
    _delegate = delegate;
}

// Native -[NSKeyedUnarchiver delegate], 0x1c055cb18. Non-external.
- (id)delegate {
    return _delegate;
}

// Native +[NSKeyedUnarchiver setClass:forClassName:], 0x1c055cb28. Non-external.
+ (void)setClass:(Class)cls forClassName:(id)name {
    os_unfair_lock_lock(&nameClassMapLock);
    OCFMutableDictionaryRef map = nameClassMap;
    if (map == NULL) {
        map = OCFDictionaryCreateMutable(kOCFAllocatorDefault, 0, &kOCFTypeDictionaryKeyCallBacks, NULL);
        nameClassMap = map;
    }
    if (cls != Nil) {
        OCFDictionarySetValue(map, name, cls);
    } else {
        OCFDictionaryRemoveValue(map, name);
    }
    os_unfair_lock_unlock(&nameClassMapLock);
}

// Native -[NSKeyedUnarchiver setClass:forClassName:], 0x1c055cbb0. Non-external.
- (void)setClass:(Class)cls forClassName:(id)name {
    OCFMutableDictionaryRef map = _nameClassMap;
    if (map == NULL) {
        map = OCFDictionaryCreateMutable(kOCFAllocatorDefault, 0, &kOCFTypeDictionaryKeyCallBacks, NULL);
        _nameClassMap = map;
    }
    if (cls != Nil) {
        OCFDictionarySetValue(map, name, cls);
    } else {
        OCFDictionaryRemoveValue(map, name);
    }
}

// Native +[NSKeyedUnarchiver classForClassName:], 0x1c055cc2c. Non-external.
+ (Class)classForClassName:(id)name {
    os_unfair_lock_lock(&nameClassMapLock);
    Class cls = nameClassMap != NULL ? (Class)OCFDictionaryGetValue(nameClassMap, name) : Nil;
    os_unfair_lock_unlock(&nameClassMapLock);
    return cls;
}

// Native -[NSKeyedUnarchiver classForClassName:], 0x1c055cc84. Non-external.
- (Class)classForClassName:(id)name {
    return _nameClassMap != NULL ? (Class)OCFDictionaryGetValue(_nameClassMap, name) : Nil;
}

// Native -[NSKeyedUnarchiver finishDecoding], 0x1c055cd38. Non-external.
- (void)finishDecoding {
    if (!(atomic_load_explicit(&_flags, memory_order_acquire) & 2)) {
        if (_delegate != nil && objc_opt_respondsToSelector(_delegate, sel_getUid("unarchiverWillFinish:"))) {
            [(id)_delegate unarchiverWillFinish:self];
        }
        atomic_store_explicit(
            &_flags, atomic_load_explicit(&_flags, memory_order_acquire) | 2, memory_order_release
        );
        if (_delegate != nil && objc_opt_respondsToSelector(_delegate, sel_getUid("unarchiverDidFinish:"))) {
            [(id)_delegate unarchiverDidFinish:self];
        }
    }
}

// Native -[NSKeyedUnarchiver _temporaryMapReplaceObject:withObject:], 0x1c055cde8. Non-external.
// Assembly/pseudocode disagreement: 0x1c055cea0-0x1c055ced4 makes TWO alloca buffers, not the single aliased
// buffer printed by Hex-Rays. The first matching value alone is replaced (kept).
- (void)_temporaryMapReplaceObject:(id)object withObject:(id)replacement {
    if (_tmpRefObjMap == NULL) {
        return;
    }
    OCFIndex count = OCFDictionaryGetCount(_tmpRefObjMap);
    size_t bytes = (uint64_t)count * sizeof(void *);
    const void **keys;
    const void **values;
    if (bytes <= 512) {
        if (count <= 0) {
            OCFDictionaryGetKeysAndValues(_tmpRefObjMap, NULL, NULL);
            return;
        }
        keys = alloca((bytes + 15) & ~(size_t)15);
        bzero(keys, bytes);
        values = alloca((bytes + 15) & ~(size_t)15);
        bzero(values, bytes);
    } else {
        keys = malloc_type_malloc(bytes, 0x100004000313f17ULL);
        values = malloc_type_malloc(bytes, 0x80040b8603338ULL);
    }
    OCFDictionaryGetKeysAndValues(_tmpRefObjMap, keys, values);
    for (OCFIndex index = 0; index < count; ++index) {
        if (values[index] == object) {
            OCFDictionarySetValue(_tmpRefObjMap, keys[index], replacement);
            break;
        }
    }
    if (bytes >= 513) {
        free(values);
        free(keys);
    }
}

// Native -[NSKeyedUnarchiver replaceObject:withObject:], 0x1c055cfa8. Non-external. The decompiler's
// swiftself annotation is spurious: X0 is self, X1 _cmd, X2/X3 the two objects.
- (void)replaceObject:(id)object withObject:(id)replacement {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot replace")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (object != replacement) {
        [self _temporaryMapReplaceObject:object withObject:replacement];
    }
}

// Native -[NSKeyedUnarchiver _replaceObject:withObject:], 0x1c055d024. Non-external.
- (void)_replaceObject:(id)object withObject:(id)replacement {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot replace")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (object != replacement) {
        if (_delegate != nil &&
            objc_opt_respondsToSelector(_delegate, sel_getUid("unarchiver:willReplaceObject:withObject:"))) {
            [(id)_delegate unarchiver:self willReplaceObject:object withObject:replacement];
        }
        if (_objRefMap != NULL) {
            const void *reference = OCFDictionaryGetValue(_objRefMap, object);
            OCFDictionaryRemoveValue(_objRefMap, object);
            OCFDictionarySetValue(_objRefMap, replacement, reference);
            OCFDictionarySetValue(_refObjMap, reference, replacement);
        }
        [self _temporaryMapReplaceObject:object withObject:replacement];
    }
    if (_replacementMap == NULL) {
        OCFDictionaryKeyCallBacks callbacks = {
            0, kOCFTypeDictionaryKeyCallBacks.retain, kOCFTypeDictionaryKeyCallBacks.release, NULL, NULL, NULL
        };
        _replacementMap = OCFDictionaryCreateMutable(kOCFAllocatorDefault, 0, &callbacks, NULL);
        _OCFDictionarySetCapacity(
            _replacementMap, _containers != NULL ? 128 : _offsetData->trailer.numObjects / 5
        );
    }
    OCFDictionarySetValue(_replacementMap, object, replacement);
}

// Native -[NSKeyedUnarchiver containsValueForKey:], 0x1c055d220. Non-external. The XML-array form consumes
// the first element even for this existence query; only one leading '$' is stripped (kept).
- (BOOL)containsValueForKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot decode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [key substringFromIndex:1];
    }
    if (_containers != NULL) {
        OCFIndex count = OCFArrayGetCount(_containers);
        const void *container = OCFArrayGetValueAtIndex(_containers, count - 1);
        id value;
        if (OCFGetTypeID(container) == 19) {
            value = (id)OCFArrayGetValueAtIndex(container, 0);
            objc_autorelease(objc_retain(value));
            OCFArrayRemoveValueAtIndex((OCFMutableArrayRef)container, 0);
        } else {
            value = (id)OCFDictionaryGetValue(container, key);
            objc_autorelease(objc_retain(value));
        }
        return value != nil;
    }
    return __OCFBinaryPlistGetOffsetForValueFromDictionary3(
        (const uint8_t *)_bytes,
        _len,
        _offsetData->containerOffset,
        &_offsetData->trailer,
        key,
        NULL,
        NULL,
        NULL,
        _offsetObjMap
    );
}

// Native -[NSKeyedUnarchiver _validateAllowedClassesContainsClass:forKey:], 0x1c055d3a4. Non-external.
- (BOOL)_validateAllowedClassesContainsClass:(Class)cls forKey:(id)key {
    if (cls == Nil) {
        id reason = [(id)archivingStringClass
            stringWithFormat:(id)OF_STR("Attempt to decode an object with no class for key '%@'."), key];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    id classes = [self allowedClasses];
    uint64_t strictFlags = atomic_load_explicit(&_flags, memory_order_acquire);
    uint64_t explicitFlags = atomic_load_explicit(&_flags, memory_order_acquire);
    BOOL valid = [self _walkAllowedClassListLookingForClass:cls
                                                     forKey:key
                                             allowedClasses:classes
                                          strictModeEnabled:(strictFlags & 32) != 0
                            alwaysEnforceExplicitSubclasses:(explicitFlags & 64) != 0
                                          specialCaseObject:NO];
    if (!valid && UNARCHIVER_HELPER(self)->decodeError == nil) {
        [self __failWithUnexpectedClass:cls forKey:key allowedClasses:classes];
    }
    return valid;
}

// Native -[NSKeyedUnarchiver _validateAllowedClassesContainsClassOfReference:forKey:], 0x1c055d494.
// Non-external. The class-list walker distinguishes a rejected collection element from a rejected root.
- (BOOL)_validateAllowedClassesContainsClassOfReference:(id)object forKey:(id)key {
    Class invalidElement = Nil;
    id classes = [self allowedClasses];
    uint64_t strictFlags = atomic_load_explicit(&_flags, memory_order_acquire);
    uint64_t explicitFlags = atomic_load_explicit(&_flags, memory_order_acquire);
    BOOL valid = [self _walkAllowedClassListValidatingInstance:object
                                                        forKey:key
                                                allowedClasses:classes
                                             strictModeEnabled:(strictFlags & 32) != 0
                               alwaysEnforceExplicitSubclasses:(explicitFlags & 64) != 0
                                             specialCaseObject:NO
                                           invalidElementClass:&invalidElement];
    if (!valid && UNARCHIVER_HELPER(self)->decodeError == nil) {
        if (invalidElement != Nil) {
            [self __failWithUnexpectedElementClass:invalidElement forContainerKey:key allowedClasses:classes];
        } else {
            [self __failWithUnexpectedClass:[object classForCoder] forKey:key allowedClasses:classes];
        }
    }
    return valid;
}

// Native -[NSKeyedUnarchiver _validatePropertyListClass:forKey:], 0x1c055d5a0. Non-external; objc_direct.
- (BOOL)_validatePropertyListClass:(Class)cls forKey:(id)key {
    if (cls == Nil) {
        id reason = [(id)archivingStringClass
            stringWithFormat:(id)OF_STR("Attempt to decode an object with no class for key '%@'."), key];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    id classes = [self allowedClasses];
    uint64_t firstFlags = atomic_load_explicit(&_flags, memory_order_acquire);
    if ([self _walkAllowedClassListLookingForClass:cls
                                            forKey:key
                                    allowedClasses:classes
                                 strictModeEnabled:(firstFlags & 32) != 0
                   alwaysEnforceExplicitSubclasses:NO
                                 specialCaseObject:NO] ||
        UNARCHIVER_HELPER(self)->decodeError != nil) {
        return YES;
    }
    bool exceptions = _os_feature_enabled_impl("ArchivingExceptions", "NSKeyedUnarchiverPlistTypeExceptions");
    bool linked = dyld_program_sdk_at_least((OCFBuildVersion){UINT32_MAX, 0x07e50901});
    uint64_t secondFlags = atomic_load_explicit(&_flags, memory_order_acquire);
    uint64_t forced = (firstFlags & 32) | (secondFlags & 128);
    BOOL enforce = forced != 0 || (exceptions & linked);
    if (linked && forced == 0) {
        [self _warnAboutPlistType:cls forKey:key missingInAllowedClasses:classes];
    }
    if (!enforce ||
        (forced == 0 &&
         _os_feature_enabled_impl("ArchivingExceptions", "NSKeyedUnarchiverPlistTypeGrandfathering") &&
         [self _shouldExemptProcessBasedOnArchivingExceptionRules:
                   "/System/Library/CoreServices/ArchivingExceptions.bundle/PlistTypeExceptions.plist"])) {
        return YES;
    }
    id description = [self __allowedClassesDescriptionForClass:cls];
    id allowed = [self _allowedClassesDescriptionForClasses:classes];
    id actualKey = key != nil ? key : (id)OF_STR("(no key, possibly an argument to a message)");
    os_log_t log = _NSRuntimeIssuesLog();
    if (os_log_type_enabled(log, OS_LOG_TYPE_FAULT)) {
        uint8_t buffer[32];
#define IMPLICIT_PLIST_WARNING                                                                               \
    "implicit decoding of plist types, like `[unarchiver decodeObjectOfClass:[NSString class] forKey:...]` " \
    "allowing a "                                                                                            \
    "decode of a `NSNumber` are no longer allowed. Please ensure you list all the expected types including " \
    "core plist "                                                                                            \
    "types like `NSString`, `NSData`, and `NSNumber` when decoding. The missing class is %{public}@ for "    \
    "key '%{public}@'\n"                                                                                     \
    "Allowed classes are:\n %@"
        __builtin_os_log_format(buffer, IMPLICIT_PLIST_WARNING, description, actualKey, allowed);
        _os_log_fault_impl(
            &__dso_handle, log, OS_LOG_TYPE_FAULT, IMPLICIT_PLIST_WARNING, buffer, sizeof(buffer)
        );
#undef IMPLICIT_PLIST_WARNING
    }
    [self __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                        errorCode:4864
                           format:
                               (id)OF_STR(
                                   "value for key '%@' was of unexpected class %@.\nAllowed classes are:\n %@"
                               ),
                               actualKey,
                               description,
                               allowed];
    return NO;
}

// Native -[NSKeyedUnarchiver decodeObjectForKey:], 0x1c055d7d0. Non-external.
- (id)decodeObjectForKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot decode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (UNARCHIVER_HELPER(self)->decodeError != nil) {
        return nil;
    }
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [key substringFromIndex:1];
    }
    id result;
    _decodeObject(self, key, &result);
    return result;
}

// Native -[NSKeyedUnarchiver decodeObjectOfClass:forKey:], 0x1c055d8ec. Non-external.
- (id)decodeObjectOfClass:(Class)cls forKey:(id)key {
    if (UNARCHIVER_HELPER(self)->decodeError != nil) {
        return nil;
    }
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 4) {
        id classes = cls != Nil ? [(id)archivingSetClass setWithObject:(id)cls] : nil;
        return [self decodeObjectOfClasses:classes forKey:key];
    }
    return [super decodeObjectOfClass:cls forKey:key];
}

// Native -[NSKeyedUnarchiver decodeObjectOfClasses:forKey:], 0x1c055d9d4. Non-external.
- (id)decodeObjectOfClasses:(id)classes forKey:(id)key {
    if (UNARCHIVER_HELPER(self)->decodeError != nil) {
        return nil;
    }
    if (!(atomic_load_explicit(&_flags, memory_order_acquire) & 4)) {
        return [super decodeObjectOfClasses:classes forKey:key];
    }
    if (classes == nil) {
        return [self decodeObjectForKey:key];
    }
    for (id cls in classes) {
        if (!object_isClass(cls)) {
            id reason = [(id)archivingStringClass
                stringWithFormat:(id)OF_STR("%@ object in classes '%@' is not an Objective-C class!"),
                                 _ONSMethodExceptionProem(self, _cmd),
                                 cls];
            objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                          reason:reason
                                                        userInfo:nil]);
        }
    }
    [UNARCHIVER_HELPER(self)->allowedClasses addObject:classes];
    id result = [self decodeObjectForKey:key];
    [UNARCHIVER_HELPER(self)->allowedClasses removeLastObject];
    return result;
}

// Native -[NSKeyedUnarchiver _decodeObjectOfClass:], 0x1c055dbf4. Non-external.
- (id)_decodeObjectOfClass:(Class)cls {
    if (UNARCHIVER_HELPER(self)->decodeError != nil) {
        return nil;
    }
    if (!(atomic_load_explicit(&_flags, memory_order_acquire) & 4)) {
        return [self decodeObject];
    }
    if (cls == Nil || !object_isClass((id)cls)) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@ '%@' is not an Objective-C class!"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       cls];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    id classes = [objc_alloc((Class)archivingSetClass) initWithObjects:(id)cls, nil];
    [UNARCHIVER_HELPER(self)->allowedClasses addObject:classes];
    objc_release(classes);
    if (_warnArchiverCompat == 1) {
        ONSLog(
            (id)OF_STR("%@: warning: old archiving compatibility method called"),
            _ONSMethodExceptionProem(self, _cmd)
        );
    }
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot decode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (UNARCHIVER_HELPER(self)->decodeError != nil) {
        return nil;
    }
    int32_t index = _genericKey;
    _genericKey = (uint32_t)index + 1;
    id key = (uint32_t)index > 39
                 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), (int64_t)index]
                 : (id)generic_keys[index];
    id result;
    _decodeObject(self, key, &result);
    [UNARCHIVER_HELPER(self)->allowedClasses removeLastObject];
    return result;
}

// Native -[NSKeyedUnarchiver _decodeCollectionOfClass:allowedClasses:forKey:], 0x1c055de40. Non-external.
// objc_direct; X1 is the collection class, X2 the allowed-class set, X3 the key. The helper flags
// and allowed-class stack are cleaned before an exception is rethrown. Folded landing pads
// 0x1c055e034-0x1c055e07c; LSDA 0x1c14a2118.
- (id)_decodeCollectionOfClass:(Class)cls allowedClasses:(id)classes forKey:(id)key {
    if (UNARCHIVER_HELPER(self)->decodeError != nil) {
        return nil;
    }
    if (!(atomic_load_explicit(&_flags, memory_order_acquire) & 4)) {
        id reason = [(id)archivingStringClass
            stringWithFormat:(id)OF_STR("%@: This method only supports secure coding."),
                             _ONSMethodExceptionProem(
                                 self, sel_getUid("_decodeCollectionOfClass:allowedClasses:forKey:")
                             )];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    KeyedUnarchiverHelperFields *helper = UNARCHIVER_HELPER(self);
    [helper->allowedClasses addObject:[classes setByAddingObject:(id)cls]];
    helper->expectsUnnestedCollection = true;
    helper->decodedCollection = false;
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [key substringFromIndex:1];
    }
    id result = nil;
    atomic_store_explicit(
        &_flags, atomic_load_explicit(&_flags, memory_order_acquire) | 8, memory_order_release
    );
    void *pool = objc_autoreleasePoolPush();
    @try {
        if (_containers != NULL) {
            _decodeObjectXML(self, key, &result);
        } else {
            uint64_t reference = 0;
            uint64_t offset = 0;
            if (_decodeOffsetAndRef(self, key, &offset, &reference)) {
                _decodeObjectBinary(self, (uint32_t)reference, key, &result);
            }
        }
        objc_autoreleasePoolPop(pool);
    } @finally {
        helper->expectsUnnestedCollection = false;
        helper->decodedCollection = false;
        [helper->allowedClasses removeLastObject];
    }
    return objc_autorelease(result);
}

// Native __decodeOffsetAndRef, 0x1c055e080. Non-external.
static bool _decodeOffsetAndRef(ONSKeyedUnarchiver *coder, id key, uint64_t *offset, uint64_t *reference) {
    if (!__OCFBinaryPlistGetOffsetForValueFromDictionary3(
            (const uint8_t *)coder->_bytes,
            coder->_len,
            coder->_offsetData->containerOffset,
            &coder->_offsetData->trailer,
            key,
            NULL,
            offset,
            NULL,
            coder->_offsetObjMap
        )) {
        return false;
    }
    if (_getUIDFromData(
            coder,
            (const uint8_t *)coder->_bytes + *offset,
            (const uint8_t *)coder->_bytes + coder->_len - 1,
            reference
        )) {
        return *reference != 0;
    }
    os_log_t log = _NSRuntimeIssuesLog();
    if (os_log_type_enabled(log, OS_LOG_TYPE_FAULT)) {
        id proem = _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:"));
        uint8_t buffer[22];
#define NONOBJECT_WARNING                                                                                    \
    "%{public}@: value for key (%{public}@) is not an object. This will become an error in the future."
        __builtin_os_log_format(buffer, NONOBJECT_WARNING, proem, key);
        _os_log_fault_impl(&__dso_handle, log, OS_LOG_TYPE_FAULT, NONOBJECT_WARNING, buffer, sizeof(buffer));
#undef NONOBJECT_WARNING
    }
    return false;
}

// Native _decodeObjectBinary, 0x1c055e1a8. Non-external. Binary object/class indices are 32-bit even though
// the UID reader returns 64 bits. Error paths, class-hint validation and replacement ownership remain
// distinct from the XML decoder below. Pointer sums are unsigned so the native carry tests are preserved.
// Folded landing pads 0x1c055eda0-0x1c055eeac; LSDA 0x1c14a2164. Only initWithCoder: catches id
// for error categorization. awakeAfterUsingCoder: has a separate catch-all for temporary-map removal.
static bool _decodeObjectBinary(ONSKeyedUnarchiver *coder, uint32_t reference, id key, id *result) {
    if (_decodeObjectReference(coder, 200, key, reference, result)) {
        return *result != nil;
    }
    uint64_t objectOffset = 0;
    id object = nil;
    uintptr_t base;
    uint64_t length;
    uintptr_t position;
    if (!__OCFBinaryPlistGetOffsetForValueFromArray2(
            (const uint8_t *)coder->_bytes,
            coder->_len,
            coder->_offsetData->objectsOffset,
            &coder->_offsetData->trailer,
            reference,
            &objectOffset,
            coder->_offsetObjMap
        ) ||
        __builtin_add_overflow(base = (uintptr_t)coder->_bytes, objectOffset, &position) ||
        base + (length = coder->_len) - 1 <= position) {
        [coder
            __failWithExceptionName:(id)ONSInconsistentArchiveException
                          errorCode:4864
                             format:(id)OF_STR("%@: object reference beyond limit or other data corruption"),
                                    _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:"))];
        goto failed;
    }
    unsigned int kind = *(const uint8_t *)position >> 4;
    if (kind < 5 || kind - 5 < 2) {
        __OCFBinaryPlistCreateObject(
            (const uint8_t *)base,
            length,
            objectOffset,
            &coder->_offsetData->trailer,
            NULL,
            0,
            coder->_offsetObjMap,
            (OCFPropertyListRef *)&object
        );
        if (kind >= 5 && object != nil && OCFEqual(OF_STR("$null"), object)) {
            OCFRelease(object);
            *result = nil;
            return true;
        }
        if (object != nil && (atomic_load_explicit(&coder->_flags, memory_order_acquire) & 4) &&
            ![coder _validatePropertyListClass:[object classForCoder] forKey:key]) {
            OCFRelease(object);
            goto failed;
        }
        *result = object;
        return true;
    }
    uint64_t classUIDOffset = 0;
    if (!__OCFBinaryPlistGetOffsetForValueFromDictionary3(
            (const uint8_t *)base,
            length,
            objectOffset,
            &coder->_offsetData->trailer,
            OF_STR("$class"),
            NULL,
            &classUIDOffset,
            NULL,
            coder->_offsetObjMap
        ) ||
        __builtin_add_overflow((uintptr_t)coder->_bytes, classUIDOffset, &position) ||
        (uintptr_t)coder->_bytes + coder->_len - 1 < position) {
        [coder __failWithExceptionName:(id)ONSInconsistentArchiveException
                             errorCode:4864
                                format:(id)OF_STR("%@: missing class information for object"),
                                       _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:"))];
        goto failed;
    }
    uint64_t classReference = 0;
    _getUIDFromData(
        coder, (const uint8_t *)position, (const uint8_t *)coder->_bytes + coder->_len - 1, &classReference
    );
    if (classReference == 0) {
        [coder __failWithExceptionName:(id)ONSInconsistentArchiveException
                             errorCode:4864
                                format:(id)OF_STR("%@: null class for object"),
                                       _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:"))];
        goto failed;
    }
    uint64_t classOffset = 0;
    if (!__OCFBinaryPlistGetOffsetForValueFromArray2(
            (const uint8_t *)coder->_bytes,
            coder->_len,
            coder->_offsetData->objectsOffset,
            &coder->_offsetData->trailer,
            (uint32_t)classReference,
            &classOffset,
            coder->_offsetObjMap
        ) ||
        __builtin_add_overflow((uintptr_t)coder->_bytes, classOffset, &position) ||
        (uintptr_t)coder->_bytes + coder->_len - 1 < position) {
        [coder __failWithExceptionName:(id)ONSInconsistentArchiveException
                             errorCode:4864
                                format:(id)OF_STR("%@: bogus class information for object"),
                                       _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:"))];
        goto failed;
    }
    uint64_t nameOffset = 0;
    if (!__OCFBinaryPlistGetOffsetForValueFromDictionary3(
            (const uint8_t *)coder->_bytes,
            coder->_len,
            classOffset,
            &coder->_offsetData->trailer,
            OF_STR("$classname"),
            NULL,
            &nameOffset,
            NULL,
            coder->_offsetObjMap
        ) ||
        __builtin_add_overflow((uintptr_t)coder->_bytes, nameOffset, &position) ||
        (uintptr_t)coder->_bytes + coder->_len - 1 < position) {
        [coder __failWithExceptionName:(id)ONSInconsistentArchiveException
                             errorCode:4864
                                format:(id)OF_STR("%@: missing class name for class"),
                                       _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:"))];
        goto failed;
    }
    id className = nil;
    if (!__OCFBinaryPlistCreateObject(
            (const uint8_t *)coder->_bytes,
            coder->_len,
            nameOffset,
            &coder->_offsetData->trailer,
            kOCFAllocatorDefault,
            0,
            coder->_offsetObjMap,
            (OCFPropertyListRef *)&className
        )) {
        [coder __failWithExceptionName:(id)ONSInconsistentArchiveException
                             errorCode:4864
                                format:(id)OF_STR("%@: corrupt data or object is too large to decode"),
                                       _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:"))];
        goto failed;
    }
    if (!_ONSIsNSString(className)) {
        if (className != nil) {
            OCFRelease(className);
        }
        [coder __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                             errorCode:4864
                                format:(id)OF_STR(
                                           "%@: cannot decode object of class (%@) for key (%@); the archive "
                                           "appears to be corrupt (non-string class name)"
                                       ),
                                       _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:")),
                                       className,
                                       key];
        goto failed;
    }
    uint64_t hintsOffset = 0;
    uint64_t hierarchyOffset = 0;
    if (!__OCFBinaryPlistGetOffsetForValueFromDictionary3(
            (const uint8_t *)coder->_bytes,
            coder->_len,
            classOffset,
            &coder->_offsetData->trailer,
            OF_STR("$classhints"),
            NULL,
            &hintsOffset,
            NULL,
            coder->_offsetObjMap
        )) {
        hintsOffset = 0;
    }
    if (!__OCFBinaryPlistGetOffsetForValueFromDictionary3(
            (const uint8_t *)coder->_bytes,
            coder->_len,
            classOffset,
            &coder->_offsetData->trailer,
            OF_STR("$classes"),
            NULL,
            &hierarchyOffset,
            NULL,
            coder->_offsetObjMap
        )) {
        hierarchyOffset = 0;
    }
    id failureReason = nil;
    Class cls = [coder classForClassName:className];
    if (cls == Nil) {
        cls = [(id)objc_opt_class(coder) classForClassName:className];
    }
    if (cls == Nil) {
        cls = ONSClassFromString(className);
    }
    if (cls != Nil) {
        goto checkClassName;
    }
    id hints = nil;
    if (hintsOffset != 0) {
        if (!__OCFBinaryPlistCreateObject(
                (const uint8_t *)coder->_bytes,
                coder->_len,
                hintsOffset,
                &coder->_offsetData->trailer,
                NULL,
                0,
                coder->_offsetObjMap,
                (OCFPropertyListRef *)&hints
            ) ||
            !_ONSIsNSArray(hints)) {
            failureReason = [(id)archivingStringClass
                stringWithFormat:(id)OF_STR("%@: data is corrupt or object is too large to decode"),
                                 _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:"))];
            if (hints != nil) {
                OCFRelease(hints);
            }
            goto classFailure;
        }
    }
    for (id name in hints) {
        if (!_ONSIsNSString(name)) {
            failureReason = [(id)archivingStringClass
                stringWithFormat:(id)OF_STR("%@: data is corrupt or object is too large to decode"),
                                 _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:"))];
            OCFRelease(hints);
            goto classFailure;
        }
        cls = ONSClassFromString(name);
        if (cls != Nil) {
            break;
        }
    }
    if (hints != nil) {
        OCFRelease(hints);
    }
    if (cls != Nil) {
        goto checkClassName;
    }
    if (coder->_delegate != nil &&
        objc_opt_respondsToSelector(
            coder->_delegate, sel_getUid("unarchiver:cannotDecodeObjectOfClassName:originalClasses:")
        )) {
        id hierarchy = nil;
        if (hierarchyOffset != 0) {
            if (!__OCFBinaryPlistCreateObject(
                    (const uint8_t *)coder->_bytes,
                    coder->_len,
                    hierarchyOffset,
                    &coder->_offsetData->trailer,
                    NULL,
                    0,
                    coder->_offsetObjMap,
                    (OCFPropertyListRef *)&hierarchy
                ) ||
                !_ONSIsNSArray(hierarchy)) {
                failureReason = [(id)archivingStringClass
                    stringWithFormat:(id)OF_STR("%@: data is corrupt or object is too large to decode"),
                                     _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:"))];
                if (hierarchy != nil) {
                    OCFRelease(hierarchy);
                }
                goto classFailure;
            }
        }
        cls = [(id)coder->_delegate unarchiver:coder
                 cannotDecodeObjectOfClassName:className
                               originalClasses:hierarchy];
        if (hierarchy != nil) {
            OCFRelease(hierarchy);
        }
        if (cls != Nil) {
            goto validateClass;
        }
    }
    goto classForUnarchiver;
checkClassName:
    if (![coder->_helper classNameAllowed:cls]) {
        id proem = _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:"));
        failureReason = [(id)archivingStringClass
            stringWithFormat:(id)OF_STR(
                                 "%@: data to unarchive contains class (%@) which has not been allowed"
                             ),
                             proem,
                             ONSStringFromClass(cls)];
        goto classFailure;
    }
validateClass:
    if (!_coderValidateClassValidToMessageForUnarchiving(cls)) {
        failureReason = [(id)archivingStringClass
            stringWithFormat:(id)OF_STR("%@: unexpected non-objectlike class (%@)"),
                             _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:")),
                             className];
        goto classFailure;
    }
classForUnarchiver:
    cls = [(id)cls classForKeyedUnarchiver];
    if (cls == Nil) {
        goto classFailure;
    }
    if (className != nil) {
        OCFRelease(className);
    }
    if ([coder requiresSecureCoding]) {
        if (key != nil) {
            if (![coder validateAllowedClass:cls forKey:key]) {
                goto failed;
            }
        } else {
            [coder validateClassSupportsSecureCoding:cls];
        }
    }
    _NSCoderCheckPotentiallyMangledClassNameAndWarn(cls, 0);
    KeyedUnarchiverHelperFields *helper = UNARCHIVER_HELPER(coder);
    if ([coder _isPossibleNestedCollectionClass:cls] && helper->expectsUnnestedCollection == true) {
        if (helper->decodedCollection == true) {
            helper->expectsUnnestedCollection = false;
            helper->decodedCollection = false;
            [coder __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                                 errorCode:4864
                                    format:(id)OF_STR(
                                               "%1$@: value for key (%2$@) contains too many nested (%3$@)s"
                                           ),
                                           _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:")),
                                           key,
                                           className];
            goto failed;
        }
        helper->decodedCollection = true;
    }
    bool allowed;
    object = coderAllocateClassForUnarchival(coder, cls, &allowed);
    if (!allowed) {
        goto failed;
    }
    int32_t savedGenericKey = coder->_genericKey;
    coder->_genericKey = 0;
    uint64_t savedOffset = coder->_offsetData->containerOffset;
    coder->_offsetData->containerOffset = objectOffset;
    OCFDictionarySetValue(coder->_tmpRefObjMap, (const void *)(uintptr_t)reference, object);
    @try {
        object = [object initWithCoder:coder];
    } @catch (id exception) {
        if (![(id)cls instancesRespondToSelector:sel_getUid("initWithCoder:")]) {
            objc_release(object);
        }
        OCFDictionaryRemoveValue(coder->_tmpRefObjMap, (const void *)(uintptr_t)reference);
        if (objc_opt_isKindOfClass(exception, objc_opt_class((id)archivingExceptionClass)) &&
            [coder decodingFailurePolicy] != 0) {
            id error = nil;
            if ([(id)objc_opt_class(coder) __categorizeException:exception intoError:&error]) {
                [coder __setError:error];
                *result = nil;
                return false;
            }
        }
        @throw;
    } @
    try {
        object = [object awakeAfterUsingCoder:coder];
    } @catch (...) {
        OCFDictionaryRemoveValue(coder->_tmpRefObjMap, (const void *)(uintptr_t)reference);
        @throw;
    }
    UNARCHIVER_HELPER(coder)->lastRef = reference;
    coder->_offsetData->containerOffset = savedOffset;
    coder->_genericKey = savedGenericKey;
    if (object != nil) {
        id replacement = object;
        if (coder->_replacementMap != NULL &&
            OCFDictionaryGetValueIfPresent(coder->_replacementMap, object, (const void **)&replacement)) {
            if (object != replacement) {
                objc_release(object);
                objc_retain(replacement);
            }
        } else {
            objc_retain(object);
            if (coder->_delegate != nil &&
                objc_opt_respondsToSelector(coder->_delegate, sel_getUid("unarchiver:didDecodeObject:"))) {
                replacement = [(id)coder->_delegate unarchiver:coder didDecodeObject:object];
            }
            if (replacement != nil) {
                [coder _replaceObject:object withObject:replacement];
            } else {
                replacement = objc_retain(object);
            }
            objc_release(object);
        }
        object = replacement;
        OCFDictionarySetValue(coder->_objRefMap, object, (const void *)(uintptr_t)reference);
        OCFDictionarySetValue(coder->_refObjMap, (const void *)(uintptr_t)reference, object);
    }
    OCFDictionaryRemoveValue(coder->_tmpRefObjMap, (const void *)(uintptr_t)reference);
    *result = object;
    return true;
classFailure:
    if (className != nil) {
        OCFRelease(className);
    }
    if (failureReason != nil) {
        [coder __failWithExceptionName:(id)ONSInconsistentArchiveException
                             errorCode:4864
                                format:(id)OF_STR("%@"), failureReason];
    } else {
        [coder __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                             errorCode:4864
                                format:(id)OF_STR(
                                           "%1$@: cannot decode object of class (%2$@) for key (%3$@) "
                                           "because no class named \"%2$@\" was found; "
                                           "the class needs to be defined in source code or linked in from a "
                                           "library (ensure the class is part of the correct target). "
                                           "If the class was renamed, use setClassName:forClass: to add a "
                                           "class translation mapping to NSKeyedUnarchiver"
                                       ),
                                       _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:")),
                                       className,
                                       key];
    }
failed:
    *result = nil;
    return false;
}

// Native _decodeObjectXML, 0x1c055eeb0. Non-external. UID zero answers false, while an objects-table
// "$null" string answers true with a nil object. XML does not validate every class-hint element as a string;
// the calls to NSClassFromString remain as read. Repeated CFGetTypeID calls are intentional.
// Folded landing pads 0x1c055f874-0x1c055f97c; LSDA 0x1c14a21dc. initWithCoder: catches id for
// error categorization; awakeAfterUsingCoder: separately catches all for temporary-map removal.
static bool _decodeObjectXML(ONSKeyedUnarchiver *coder, const void *key, id *result) {
    OCFIndex count = OCFArrayGetCount(coder->_containers);
    const void *container = OCFArrayGetValueAtIndex(coder->_containers, count - 1);
    id uid;
    if (OCFGetTypeID(container) == 19) {
        uid = (id)OCFArrayGetValueAtIndex(container, 0);
        objc_autorelease(objc_retain(uid));
        OCFArrayRemoveValueAtIndex((OCFMutableArrayRef)container, 0);
    } else {
        uid = (id)OCFDictionaryGetValue(container, key);
        objc_autorelease(objc_retain(uid));
    }
    id format = nil;
    if (uid == nil) {
        goto failed;
    }
    OCFTypeID uidType = OCFGetTypeID(uid);
    if (uidType != _OCFKeyedArchiverUIDGetTypeID()) {
        format = (id)OF_STR("%@: value is not a reference as expected");
        goto archiveFailure;
    }
    uint32_t reference = _OCFKeyedArchiverUIDGetValue((OCFKeyedArchiverUIDRef)uid);
    if (reference == 0) {
        goto failed;
    }
    if (_decodeObjectReference(coder, 100, (id)key, reference, result)) {
        return *result != nil;
    }
    if (OCFArrayGetCount(coder->_objects) <= reference) {
        format = (id)OF_STR("%@: invalid object reference");
        goto archiveFailure;
    }
    id object = (id)OCFArrayGetValueAtIndex(coder->_objects, reference);
    if (object == nil) {
        format = (id)OF_STR("%@: wrong object format");
        goto archiveFailure;
    }
    if (OCFGetTypeID(object) == 7 && OCFEqual(OF_STR("$null"), object)) {
        *result = nil;
        return true;
    }
    if (OCFGetTypeID(object) == 7 || OCFGetTypeID(object) == 21 || OCFGetTypeID(object) == 22 ||
        OCFGetTypeID(object) == 20) {
        if ((atomic_load_explicit(&coder->_flags, memory_order_acquire) & 4) &&
            ![coder _validatePropertyListClass:[object classForCoder] forKey:(id)key]) {
            goto failed;
        }
        *result = objc_retain(object);
        return true;
    }
    if (OCFGetTypeID(object) != 18) {
        format = (id)OF_STR("%@: wrong object format format");
        goto archiveFailure;
    }
    const void *classUID = OCFDictionaryGetValue((OCFDictionaryRef)object, OF_STR("$class"));
    if (classUID == NULL) {
        format = (id)OF_STR("%@: missing class information for object or wrong format");
        goto archiveFailure;
    }
    uidType = OCFGetTypeID(classUID);
    if (uidType != _OCFKeyedArchiverUIDGetTypeID()) {
        format = (id)OF_STR("%@: missing class information for object or wrong format");
        goto archiveFailure;
    }
    uint32_t classReference = _OCFKeyedArchiverUIDGetValue(classUID);
    if (classReference == 0) {
        format = (id)OF_STR("%@: null class for object");
        goto archiveFailure;
    }
    if (OCFArrayGetCount(coder->_objects) <= classReference) {
        format = (id)OF_STR("%@: invalid object reference");
        goto archiveFailure;
    }
    OCFDictionaryRef classRecord = OCFArrayGetValueAtIndex(coder->_objects, classReference);
    if (classRecord == NULL || OCFGetTypeID(classRecord) != 18) {
        format = (id)OF_STR("%@: wrong class container format");
        goto archiveFailure;
    }
    id className = (id)OCFDictionaryGetValue(classRecord, OF_STR("$classname"));
    if (className == nil) {
        format = (id)OF_STR("%@: missing class name for class");
        goto archiveFailure;
    }
    if (!_ONSIsNSString(className)) {
        [coder __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                             errorCode:4864
                                format:(id)OF_STR(
                                           "%@: cannot decode object of class (%@) for key (%@); the archive "
                                           "appears to be corrupt (non-string class name)"
                                       ),
                                       _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:")),
                                       className,
                                       key];
        goto failed;
    }
    id hints = (id)OCFDictionaryGetValue(classRecord, OF_STR("$classhints"));
    id hierarchy = (id)OCFDictionaryGetValue(classRecord, OF_STR("$classes"));
    if ((hints != nil && !_ONSIsNSArray(hints)) || (hierarchy != nil && !_ONSIsNSArray(hierarchy))) {
        format = (id)OF_STR("%@: wrong class format");
        goto archiveFailure;
    }
    Class cls = [coder classForClassName:className];
    if (cls == Nil) {
        cls = [(id)objc_opt_class(coder) classForClassName:className];
    }
    if (cls == Nil) {
        cls = ONSClassFromString(className);
    }
    if (cls == Nil) {
        for (id name in hints) {
            cls = ONSClassFromString(name);
            if (cls != Nil) {
                break;
            }
        }
        if (cls == Nil) {
            if (coder->_delegate != nil &&
                objc_opt_respondsToSelector(
                    coder->_delegate, sel_getUid("unarchiver:cannotDecodeObjectOfClassName:originalClasses:")
                )) {
                cls = [(id)coder->_delegate unarchiver:coder
                         cannotDecodeObjectOfClassName:className
                                       originalClasses:hierarchy];
                if (cls != Nil) {
                    goto validateClass;
                }
            }
            goto classForUnarchiver;
        }
    }
    if (![coder->_helper classNameAllowed:cls]) {
        id proem = _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:"));
        id name = ONSStringFromClass(cls);
        [coder
            __failWithExceptionName:(id)ONSInvalidArgumentException
                          errorCode:4864
                             format:(id)OF_STR(
                                        "%@: data to unarchive contains class (%@) which has not been allowed"
                                    ),
                                    proem,
                                    name];
        goto missingClass;
    }
validateClass:
    if (!_coderValidateClassValidToMessageForUnarchiving(cls)) {
        id proem = _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:"));
        id name = ONSStringFromClass(cls);
        [coder __failWithExceptionName:(id)ONSInvalidArgumentException
                             errorCode:4864
                                format:(id)OF_STR("%@: unexpected non-objectlike class (%@)"), proem, name];
        goto missingClass;
    }
classForUnarchiver:
    cls = [(id)cls classForKeyedUnarchiver];
    if (cls == Nil) {
        goto missingClass;
    }
    if ([coder requiresSecureCoding]) {
        if (key != NULL) {
            if (![coder validateAllowedClass:cls forKey:(id)key]) {
                goto failed;
            }
        } else {
            [coder validateClassSupportsSecureCoding:cls];
        }
    }
    _NSCoderCheckPotentiallyMangledClassNameAndWarn(cls, 0);
    KeyedUnarchiverHelperFields *helper = UNARCHIVER_HELPER(coder);
    if ([coder _isPossibleNestedCollectionClass:cls] && helper->expectsUnnestedCollection == true) {
        if (helper->decodedCollection == true) {
            helper->expectsUnnestedCollection = false;
            helper->decodedCollection = false;
            [coder __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                                 errorCode:4864
                                    format:(id)OF_STR(
                                               "%1$@: value for key (%2$@) contains too many nested (%3$@)s"
                                           ),
                                           _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:")),
                                           key,
                                           className];
            goto failed;
        }
        helper->decodedCollection = true;
    }
    bool allowed;
    id allocated = coderAllocateClassForUnarchival(coder, cls, &allowed);
    if (!allowed) {
        goto failed;
    }
    int32_t savedGenericKey = coder->_genericKey;
    coder->_genericKey = 0;
    OCFRetain(object);
    OCFArrayAppendValue(coder->_containers, object);
    OCFTypeID type = OCFGetTypeID(object);
    uint64_t flags = atomic_load_explicit(&coder->_flags, memory_order_acquire);
    atomic_store_explicit(&coder->_flags, (flags & ~1ULL) + (type == 19), memory_order_release);
    OCFDictionarySetValue(coder->_tmpRefObjMap, (const void *)(uintptr_t)reference, allocated);
    id decoded;
    @try {
        decoded = [allocated initWithCoder:coder];
    } @catch (id exception) {
        if (![(id)cls instancesRespondToSelector:sel_getUid("initWithCoder:")]) {
            objc_release(allocated);
        }
        OCFDictionaryRemoveValue(coder->_tmpRefObjMap, (const void *)(uintptr_t)reference);
        if (objc_opt_isKindOfClass(exception, objc_opt_class((id)archivingExceptionClass)) &&
            [coder decodingFailurePolicy] != 0) {
            id error = nil;
            if ([(id)objc_opt_class(coder) __categorizeException:exception intoError:&error]) {
                [coder __setError:error];
                *result = nil;
                return false;
            }
        }
        @throw;
    } @
    try {
        decoded = [decoded awakeAfterUsingCoder:coder];
    } @catch (...) {
        OCFDictionaryRemoveValue(coder->_tmpRefObjMap, (const void *)(uintptr_t)reference);
        @throw;
    }
    UNARCHIVER_HELPER(coder)->lastRef = reference;
    count = OCFArrayGetCount(coder->_containers);
    const void *top = OCFArrayGetValueAtIndex(coder->_containers, count - 1);
    OCFArrayRemoveValueAtIndex(coder->_containers, count - 1);
    OCFRelease(top);
    if (count >= 2) {
        const void *previous = OCFArrayGetValueAtIndex(coder->_containers, count - 2);
        type = OCFGetTypeID(previous);
        flags = atomic_load_explicit(&coder->_flags, memory_order_acquire);
        atomic_store_explicit(&coder->_flags, (flags & ~1ULL) + (type == 19), memory_order_release);
    }
    coder->_genericKey = savedGenericKey;
    if (decoded != nil) {
        id replacement = decoded;
        if (coder->_replacementMap != NULL &&
            OCFDictionaryGetValueIfPresent(coder->_replacementMap, decoded, (const void **)&replacement)) {
            if (decoded != replacement) {
                objc_release(decoded);
                objc_retain(replacement);
            }
        } else {
            objc_retain(decoded);
            if (coder->_delegate != nil &&
                objc_opt_respondsToSelector(coder->_delegate, sel_getUid("unarchiver:didDecodeObject:"))) {
                replacement = [(id)coder->_delegate unarchiver:coder didDecodeObject:decoded];
            }
            if (replacement != nil) {
                [coder _replaceObject:decoded withObject:replacement];
            } else {
                replacement = objc_retain(decoded);
            }
            objc_release(decoded);
        }
        decoded = replacement;
        OCFDictionarySetValue(coder->_objRefMap, decoded, (const void *)(uintptr_t)reference);
        OCFDictionarySetValue(coder->_refObjMap, (const void *)(uintptr_t)reference, decoded);
    }
    OCFDictionaryRemoveValue(coder->_tmpRefObjMap, (const void *)(uintptr_t)reference);
    *result = decoded;
    return true;
missingClass:
    [coder __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                         errorCode:4864
                            format:(id)OF_STR(
                                       "%1$@: cannot decode object of class (%2$@) for key (%3$@) because no "
                                       "class named \"%2$@\" was found; "
                                       "the class needs to be defined in source code or linked in from a "
                                       "library (ensure the class is part of the correct target). "
                                       "If the class was renamed, use setClassName:forClass: to add a class "
                                       "translation mapping to NSKeyedUnarchiver"
                                   ),
                                   _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:")),
                                   className,
                                   key];
    goto failed;
archiveFailure:
    [coder
        __failWithExceptionName:(id)ONSInconsistentArchiveException
                      errorCode:4864
                         format:format, _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:"))];
failed:
    *result = nil;
    return false;
}

// Native -[NSKeyedUnarchiver decodeArrayOfObjectsOfClasses:forKey:], 0x1c055f980. Non-external.
- (id)decodeArrayOfObjectsOfClasses:(id)classes forKey:(id)key {
    id proem = _ONSMethodExceptionProem(self, _cmd);
    if (![self _validateDecodeCollectionAllowedClassesRequirementsWithClasses:classes
                                                      forMethodExceptionProem:proem]) {
        return nil;
    }
    id result = [self _decodeCollectionOfClass:(Class)archivingArrayClass allowedClasses:classes forKey:key];
    if (result != nil && !_ONSIsNSArray(result) && [self __decoderEnforceCollectionType]) {
        [self __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                            errorCode:4864
                               format:(id)OF_STR("value for key '%@' was not an NSArray (got %@)"),
                                      key,
                                      ONSStringFromClass([result classForCoder])];
        return nil;
    }
    return result;
}

// Native -[NSKeyedUnarchiver allowedClasses], 0x1c055fb64. Non-external.
- (id)allowedClasses {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 4) {
        return [UNARCHIVER_HELPER(self)->allowedClasses lastObject];
    }
    return [super allowedClasses];
}

// Native -[NSKeyedUnarchiver requiresSecureCoding], 0x1c055fc0c. Non-external.
- (BOOL)requiresSecureCoding {
    return (atomic_load_explicit(&_flags, memory_order_acquire) & 4) != 0;
}

// Native -[NSKeyedUnarchiver setRequiresSecureCoding:], 0x1c055fc24. Non-external.
- (void)setRequiresSecureCoding:(BOOL)requiresSecureCoding {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 8) {
        id reason = [(id)archivingStringClass
            stringWithFormat:(id)OF_STR("%@: unarchiver has started; cannot change secure coding state"),
                             _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    uint64_t flags = atomic_load_explicit(&_flags, memory_order_acquire);
    atomic_store_explicit(&_flags, (flags & ~4ULL) | (requiresSecureCoding ? 4 : 0), memory_order_release);
}

// Native -[NSKeyedUnarchiver _strictSecureDecodingEnabled], 0x1c055fcf0. Non-external.
- (BOOL)_strictSecureDecodingEnabled {
    return (atomic_load_explicit(&_flags, memory_order_acquire) & 32) != 0;
}

// Native -[NSKeyedUnarchiver _allowDecodingCyclesInSecureMode], 0x1c055fd08. Non-external. LDSETAL.
- (void)_allowDecodingCyclesInSecureMode {
    atomic_fetch_or_explicit(&_flags, 256, memory_order_acq_rel);
}

// Native decodingCyclesIsAllowedForCoder, 0x1c055fd24. Non-external. Three distinct acquire loads precede
// the trust restriction call; merging the loads would change the native observation order.
static bool decodingCyclesIsAllowedForCoder(ONSKeyedUnarchiver *coder) {
    uint64_t secureFlags = atomic_load_explicit(&coder->_flags, memory_order_acquire);
    uint64_t strictFlags = atomic_load_explicit(&coder->_flags, memory_order_acquire);
    uint64_t cycleFlags = atomic_load_explicit(&coder->_flags, memory_order_acquire);
    unsigned int trust = _NSCoderGetTrustRestrictions();
    if (strictFlags & 32) {
        return false;
    }
    return (((cycleFlags << 16) & 0x1000000) | ((secureFlags >> 2) & 1)) != 1 || trust != 0;
}

// Native -[NSKeyedUnarchiver error], 0x1c055fdf4. Non-external. No autorelease or retain.
- (id)error {
    return UNARCHIVER_HELPER(self)->decodeError;
}

// Native -[NSKeyedUnarchiver __setError:], 0x1c055fe08. Non-external.
- (void)__setError:(id)error {
    [_helper setDecodeError:error];
}

// Native -[NSKeyedUnarchiver decodingFailurePolicy], 0x1c055fe18. Non-external.
- (NSInteger)decodingFailurePolicy {
    uint32_t flags = (uint32_t)atomic_load_explicit(&_flags, memory_order_acquire);
    return ~(flags >> 4) & 1;
}

// Native -[NSKeyedUnarchiver setDecodingFailurePolicy:], 0x1c055fe38. Non-external.
- (void)setDecodingFailurePolicy:(NSInteger)policy {
    id reason = nil;
    if ((uint64_t)policy >= 2) {
        reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: invalid decoding error policy %lu"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       policy];
    } else if (atomic_load_explicit(&_flags, memory_order_acquire) & 8) {
        reason = [(id)archivingStringClass
            stringWithFormat:(id)OF_STR("%@: unarchiver has started; cannot change decoding error policy"),
                             _ONSMethodExceptionProem(self, _cmd)];
    }
    if (reason != nil) {
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    uint64_t flags = atomic_load_explicit(&_flags, memory_order_acquire);
    atomic_store_explicit(&_flags, (flags & ~16ULL) | (policy == 0 ? 16 : 0), memory_order_release);
}

// Native __readInt, 0x1c0560720. Non-external.
// Widths 16 through 128 accumulate modulo 2^64; encodings 8 through 15 advance by their full width
// but return zero (kept). The end test deliberately requires marker + width + 1 <= limit.
static bool
_readInt(id coder, const uint8_t *marker, const uint8_t *limit, uint64_t *value, const uint8_t **next) {
    uint8_t tag = *marker;
    if ((tag & 0xf0) != 0x10) {
        return false;
    }
    unsigned int exponent = tag & 15;
    uint64_t width = 1ULL << exponent;
    uintptr_t end;
    if (__builtin_add_overflow((uintptr_t)marker, width + 1, &end) || (uintptr_t)limit < end) {
        [coder __failWithExceptionName:(id)ONSInconsistentArchiveException
                             errorCode:4864
                                format:(id)OF_STR("overflow while attempting to read %lu bytes (int)"),
                                       width + 1];
        return false;
    }
    const uint8_t *bytes = marker + 1;
    uint64_t result = 0;
    if ((uint8_t)width == 1) {
        result = *bytes;
    } else if ((uint8_t)width == 2) {
        uint16_t word;
        memcpy(&word, bytes, 2);
        result = __builtin_bswap16(word);
    } else if ((uint8_t)width == 4) {
        uint32_t word;
        memcpy(&word, bytes, 4);
        result = __builtin_bswap32(word);
    } else if ((uint8_t)width == 8) {
        uint64_t word;
        memcpy(&word, bytes, 8);
        result = __builtin_bswap64(word);
    } else if (exponent <= 7) {
        uint64_t remaining = (uint8_t)width;
        do {
            result = *bytes++ | (result << 8);
        } while (--remaining != 0);
    }
    *value = result;
    if (next != NULL) {
        *next = (const uint8_t *)end;
    }
    return true;
}

// Native __getUIDFromData, 0x1c056082c. Non-external.
// The initial signed-byte comparison admits exactly markers 0x80..0x8f. Values longer than eight
// bytes retain their low 64 bits. The bound is marker + payload width + 1 <= limit (kept).
static bool _getUIDFromData(id coder, const uint8_t *marker, const uint8_t *limit, uint64_t *value) {
    int tag = *(const int8_t *)marker;
    if (tag > -113) {
        return false;
    }
    unsigned int low = tag & 15;
    uint64_t extent = low + 2;
    uintptr_t end;
    if (__builtin_add_overflow((uintptr_t)marker, extent, &end) || (uintptr_t)limit < end) {
        [coder
            __failWithExceptionName:(id)ONSInconsistentArchiveException
                          errorCode:4864
                             format:(id)OF_STR("overflow while attempting to read %lu bytes (uid)"), extent];
        return false;
    }
    const uint8_t *bytes = marker + 1;
    uint64_t result = 0;
    if (low == 0) {
        result = *bytes;
    } else if (low == 1) {
        uint16_t word;
        memcpy(&word, bytes, 2);
        result = __builtin_bswap16(word);
    } else if (low == 3) {
        uint32_t word;
        memcpy(&word, bytes, 4);
        result = __builtin_bswap32(word);
    } else if (low == 7) {
        uint64_t word;
        memcpy(&word, bytes, 8);
        result = __builtin_bswap64(word);
    } else {
        unsigned int remaining = low + 1;
        do {
            result = *bytes++ | (result << 8);
        } while (--remaining != 0);
    }
    *value = result;
    return true;
}

// Native -[NSKeyedUnarchiver decodeBoolForKey:], 0x1c0560910. Non-external.
- (BOOL)decodeBoolForKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot decode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (UNARCHIVER_HELPER(self)->decodeError != nil) {
        return NO;
    }
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [key substringFromIndex:1];
    }
    bool result;
    _decodeBool(self, key, &result);
    return result & 1;
}

// Native __decodeBool, 0x1c0560a30. Non-external.
static bool _decodeBool(ONSKeyedUnarchiver *coder, const void *key, bool *result) {
    atomic_store_explicit(
        &coder->_flags, atomic_load_explicit(&coder->_flags, memory_order_acquire) | 8, memory_order_release
    );
    if (coder->_containers != NULL) {
        OCFIndex count = OCFArrayGetCount(coder->_containers);
        const void *container = OCFArrayGetValueAtIndex(coder->_containers, count - 1);
        id value;
        if (OCFGetTypeID(container) == 19) {
            value = (id)OCFArrayGetValueAtIndex(container, 0);
            objc_autorelease(objc_retain(value));
            OCFArrayRemoveValueAtIndex((OCFMutableArrayRef)container, 0);
        } else {
            value = (id)OCFDictionaryGetValue(container, key);
            objc_autorelease(objc_retain(value));
        }
        if (value == nil) {
            *result = false;
            return false;
        }
        if (OCFGetTypeID(value) == 21) {
            *result = OCFBooleanGetValue((OCFBooleanRef)value) != 0;
            return true;
        }
    } else {
        uint64_t offset = 0;
        if (!__OCFBinaryPlistGetOffsetForValueFromDictionary3(
                (const uint8_t *)coder->_bytes,
                coder->_len,
                coder->_offsetData->containerOffset,
                &coder->_offsetData->trailer,
                key,
                NULL,
                &offset,
                NULL,
                coder->_offsetObjMap
            )) {
            *result = false;
            return false;
        }
        uint8_t marker = ((const uint8_t *)coder->_bytes)[offset];
        if (marker == 9 || marker == 8) {
            *result = marker == 9;
            return true;
        }
    }
    [coder __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                         errorCode:4864
                            format:(id)OF_STR("%@: value for key (%@) is not a boolean"),
                                   _ONSMethodExceptionProem(coder, sel_getUid("decodeBoolForKey:")),
                                   key];
    *result = false;
    return false;
}

// Native -[NSKeyedUnarchiver decodeIntForKey:], 0x1c0560c1c. Non-external.
- (int)decodeIntForKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot decode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (UNARCHIVER_HELPER(self)->decodeError != nil) {
        return 0;
    }
    atomic_store_explicit(
        &_flags, atomic_load_explicit(&_flags, memory_order_acquire) | 8, memory_order_release
    );
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [key substringFromIndex:1];
    }
    uint64_t result;
    bool success = _decodeInt64(self, key, &result);
    int value = (int)result;
    if (success && result - 0x80000000ULL <= 0xfffffffeffffffffULL) {
        [self __failWithExceptionName:(id)ONSRangeException
                            errorCode:4864
                               format:(id)OF_STR(
                                          "%@: value (%lld) for key (%@) too large to fit in a native integer"
                                      ),
                                      _ONSMethodExceptionProem(self, sel_getUid("decodeIntForKey:")),
                                      result,
                                      key];
    }
    return value;
}

// Native -[NSKeyedUnarchiver decodeInt32ForKey:], 0x1c0560db4. Non-external.
- (int32_t)decodeInt32ForKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot decode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (UNARCHIVER_HELPER(self)->decodeError != nil) {
        return 0;
    }
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [key substringFromIndex:1];
    }
    int32_t result;
    _decodeInt32(self, key, &result);
    return result;
}

// Native __decodeInt32, 0x1c0560ed0. Non-external.
static bool _decodeInt32(ONSKeyedUnarchiver *coder, const void *key, int32_t *result) {
    uint64_t value;
    bool success = _decodeInt64(coder, key, &value);
    int32_t narrowed = (int32_t)value;
    if (success && value != (uint64_t)(int64_t)narrowed) {
        [coder
            __failWithExceptionName:(id)ONSRangeException
                          errorCode:4864
                             format:(id)OF_STR(
                                        "%@: value (%lld) for key (%@) too large to fit in a 32-bit integer"
                                    ),
                                    _ONSMethodExceptionProem(coder, sel_getUid("decodeInt32ForKey:")),
                                    value,
                                    key];
        success = false;
    }
    *result = narrowed;
    return success;
}

// Native -[NSKeyedUnarchiver decodeInt64ForKey:], 0x1c0560f9c. Non-external.
- (int64_t)decodeInt64ForKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot decode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (UNARCHIVER_HELPER(self)->decodeError != nil) {
        return 0;
    }
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [key substringFromIndex:1];
    }
    uint64_t result;
    _decodeInt64(self, key, &result);
    return (int64_t)result;
}

// Native __decodeInt64, 0x1c05610b8. Non-external.
static bool _decodeInt64(ONSKeyedUnarchiver *coder, const void *key, uint64_t *result) {
    atomic_store_explicit(
        &coder->_flags, atomic_load_explicit(&coder->_flags, memory_order_acquire) | 8, memory_order_release
    );
    uint64_t value = 0;
    if (coder->_containers != NULL) {
        OCFIndex count = OCFArrayGetCount(coder->_containers);
        const void *container = OCFArrayGetValueAtIndex(coder->_containers, count - 1);
        id value;
        if (OCFGetTypeID(container) == 19) {
            value = (id)OCFArrayGetValueAtIndex(container, 0);
            objc_autorelease(objc_retain(value));
            OCFArrayRemoveValueAtIndex((OCFMutableArrayRef)container, 0);
        } else {
            value = (id)OCFDictionaryGetValue(container, key);
            objc_autorelease(objc_retain(value));
        }
        if (value == nil) {
            *result = 0;
            return false;
        }
        if (OCFGetTypeID(value) == 22 && !OCFNumberIsFloatType((OCFNumberRef)value)) {
            uint64_t integer = 0;
            bool success = OCFNumberGetValue((OCFNumberRef)value, kOCFNumberSInt64Type, &integer) != 0;
            *result = integer;
            return success;
        }
    } else {
        uint64_t offset = 0;
        if (!__OCFBinaryPlistGetOffsetForValueFromDictionary3(
                (const uint8_t *)coder->_bytes,
                coder->_len,
                coder->_offsetData->containerOffset,
                &coder->_offsetData->trailer,
                key,
                NULL,
                &offset,
                NULL,
                coder->_offsetObjMap
            )) {
            *result = 0;
            return false;
        }
        if (_readInt(
                coder,
                (const uint8_t *)coder->_bytes + offset,
                (const uint8_t *)coder->_bytes + coder->_len - 1,
                &value,
                NULL
            )) {
            *result = value;
            return true;
        }
    }
    [coder __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                         errorCode:4864
                            format:(id)OF_STR("%@: value for key (%@) is not an integer number"),
                                   _ONSMethodExceptionProem(coder, sel_getUid("decodeInt64ForKey:")),
                                   key];
    *result = 0;
    return false;
}

// Native -[NSKeyedUnarchiver decodeFloatForKey:], 0x1c05612c8. Non-external.
- (float)decodeFloatForKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot decode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (UNARCHIVER_HELPER(self)->decodeError != nil) {
        return 0.0f;
    }
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [key substringFromIndex:1];
    }
    float result;
    _decodeFloat(self, key, &result);
    return result;
}

// Native __decodeFloat, 0x1c05613e0. Non-external.
// NaNs and infinities are accepted. The maximum finite threshold is exactly 0x47efffffe0000000
// (FLT_MAX converted to double); conversion to float occurs even when an error is reported.
static bool _decodeFloat(ONSKeyedUnarchiver *coder, const void *key, float *result) {
    double value;
    bool success = _decodeDouble(coder, key, &value);
    if (success && value == value) {
        uint64_t bits;
        memcpy(&bits, &value, sizeof(bits));
        if ((bits & 0x7fffffffffffffffULL) != 0x7ff0000000000000ULL && fabs(value) > (double)FLT_MAX) {
            [coder __failWithExceptionName:(id)ONSRangeException
                                 errorCode:4864
                                    format:(id)OF_STR(
                                               "%@: value (%f) for key (%@) too large to fit in 32-bit float"
                                           ),
                                           _ONSMethodExceptionProem(coder, sel_getUid("decodeFloatForKey:")),
                                           value,
                                           key];
            success = false;
        }
    }
    *result = (float)value;
    return success;
}

// Native -[NSKeyedUnarchiver decodeDoubleForKey:], 0x1c05614e0. Non-external.
- (double)decodeDoubleForKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot decode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (UNARCHIVER_HELPER(self)->decodeError != nil) {
        return 0.0;
    }
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [key substringFromIndex:1];
    }
    double result;
    _decodeDouble(self, key, &result);
    return result;
}

// Native __decodeDouble, 0x1c05615f8. Non-external.
// Bounds require the pointer after the payload to be no greater than the supplied last-byte
// limit, as in the native code. Overflow reports the archive error and then the float-type error.
static bool _decodeDouble(ONSKeyedUnarchiver *coder, const void *key, double *result) {
    atomic_store_explicit(
        &coder->_flags, atomic_load_explicit(&coder->_flags, memory_order_acquire) | 8, memory_order_release
    );
    double value = 0.0;
    if (coder->_containers != NULL) {
        OCFIndex count = OCFArrayGetCount(coder->_containers);
        const void *container = OCFArrayGetValueAtIndex(coder->_containers, count - 1);
        id value;
        if (OCFGetTypeID(container) == 19) {
            value = (id)OCFArrayGetValueAtIndex(container, 0);
            objc_autorelease(objc_retain(value));
            OCFArrayRemoveValueAtIndex((OCFMutableArrayRef)container, 0);
        } else {
            value = (id)OCFDictionaryGetValue(container, key);
            objc_autorelease(objc_retain(value));
        }
        if (value == nil) {
            *result = 0.0;
            return false;
        }
        if (OCFGetTypeID(value) == 22 && OCFNumberIsFloatType((OCFNumberRef)value)) {
            double number = 0.0;
            OCFNumberGetValue((OCFNumberRef)value, kOCFNumberFloat64Type, &number);
            *result = number;
            return true;
        }
    } else {
        uint64_t offset = 0;
        if (!__OCFBinaryPlistGetOffsetForValueFromDictionary3(
                (const uint8_t *)coder->_bytes,
                coder->_len,
                coder->_offsetData->containerOffset,
                &coder->_offsetData->trailer,
                key,
                NULL,
                &offset,
                NULL,
                coder->_offsetObjMap
            )) {
            *result = 0.0;
            return false;
        }
        const uint8_t *marker = (const uint8_t *)coder->_bytes + offset;
        uintptr_t limit = (uintptr_t)coder->_bytes + coder->_len - 1;
        uint8_t tag = *marker;
        if (tag == 35 || tag == 34) {
            uint64_t extent = tag == 35 ? 9 : 5;
            uintptr_t end;
            if (!__builtin_add_overflow((uintptr_t)marker, extent, &end) && limit >= end) {
                if (tag == 35) {
                    uint64_t bits;
                    memcpy(&bits, marker + 1, sizeof(bits));
                    bits = __builtin_bswap64(bits);
                    memcpy(&value, &bits, sizeof(value));
                } else {
                    uint32_t bits;
                    memcpy(&bits, marker + 1, sizeof(bits));
                    bits = __builtin_bswap32(bits);
                    float number;
                    memcpy(&number, &bits, sizeof(number));
                    value = number;
                }
                *result = value;
                return true;
            }
            [coder __failWithExceptionName:(id)ONSInconsistentArchiveException
                                 errorCode:4864
                                    format:(id)OF_STR("overflow while attempting to read %lu bytes (float)"),
                                           extent];
        }
    }
    [coder __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                         errorCode:4864
                            format:(id)OF_STR("%@: value for key (%@) is not a 64-bit float"),
                                   _ONSMethodExceptionProem(coder, sel_getUid("decodeDoubleForKey:")),
                                   key];
    *result = 0.0;
    return false;
}

// Native -[NSKeyedUnarchiver decodeBytesForKey:returnedLength:], 0x1c0561868. Non-external.
- (const uint8_t *)decodeBytesForKey:(id)key returnedLength:(NSUInteger *)length {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot decode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (UNARCHIVER_HELPER(self)->decodeError != nil) {
        if (length != NULL) {
            *length = 0;
        }
        return NULL;
    }
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [key substringFromIndex:1];
    }
    return _decodeBytes(self, key, length);
}

// Native __decodeBytes, 0x1c0561974. Non-external.
static const uint8_t *_decodeBytes(ONSKeyedUnarchiver *coder, const void *key, NSUInteger *length) {
    atomic_store_explicit(
        &coder->_flags, atomic_load_explicit(&coder->_flags, memory_order_acquire) | 8, memory_order_release
    );
    if (coder->_containers != NULL) {
        OCFIndex count = OCFArrayGetCount(coder->_containers);
        const void *container = OCFArrayGetValueAtIndex(coder->_containers, count - 1);
        id value;
        if (OCFGetTypeID(container) == 19) {
            value = (id)OCFArrayGetValueAtIndex(container, 0);
            objc_autorelease(objc_retain(value));
            OCFArrayRemoveValueAtIndex((OCFMutableArrayRef)container, 0);
        } else {
            value = (id)OCFDictionaryGetValue(container, key);
            objc_autorelease(objc_retain(value));
        }
        if (length != NULL) {
            *length = 0;
        }
        if (value == nil || (OCFGetTypeID(value) == 7 && OCFEqual(OF_STR("$null"), value))) {
            return NULL;
        }
        if (OCFGetTypeID(value) != 20) {
            goto notBytes;
        }
        if (length != NULL) {
            *length = OCFDataGetLength((OCFDataRef)value);
        }
        return OCFDataGetBytePtr((OCFDataRef)value);
    }
    uint64_t offset = 0;
    if (length != NULL) {
        *length = 0;
    }
    if (!__OCFBinaryPlistGetOffsetForValueFromDictionary3(
            (const uint8_t *)coder->_bytes,
            coder->_len,
            coder->_offsetData->containerOffset,
            &coder->_offsetData->trailer,
            key,
            NULL,
            &offset,
            NULL,
            coder->_offsetObjMap
        )) {
        return NULL;
    }
    uintptr_t base = (uintptr_t)coder->_bytes;
    const uint8_t *marker = (const uint8_t *)(base + offset);
    uint8_t tag = *marker;
    if ((tag & 0xf0) != 0x40) {
        if ((tag & 0xf0) == 0x50 && (tag & 15) == 5 && marker[1] == '$' && marker[2] == 'n' &&
            marker[3] == 'u' && marker[4] == 'l' && marker[5] == 'l') {
            return NULL;
        }
        goto notBytes;
    }
    uint64_t count = tag & 15;
    const uint8_t *bytes = marker + 1;
    if (count == 15) {
        count = 0;
        if (!_readInt(coder, bytes, (const uint8_t *)(base + coder->_len - 1), &count, &bytes)) {
            goto corrupt;
        }
        if (count & 0x8000000000000000ULL) {
            [coder __failWithExceptionName:(id)ONSInconsistentArchiveException
                                 errorCode:4864
                                    format:(id)OF_STR("%@: data bytes is too long to decode (%qd)"),
                                           _ONSMethodExceptionProem(
                                               coder, sel_getUid("decodeBytesForKey:returnedLength:")
                                           ),
                                           count];
            return NULL;
        }
        base = (uintptr_t)coder->_bytes;
    }
    if ((uintptr_t)bytes < base) {
        goto corrupt;
    }
    uintptr_t position = (uintptr_t)bytes - base;
    if (coder->_len < position || count > coder->_len - position) {
        goto corrupt;
    }
    if (length != NULL) {
        *length = count;
    }
    return bytes;
notBytes:
    [coder __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                         errorCode:4864
                            format:(id)OF_STR("%@: value for key (%@) is not a blob of bytes"),
                                   _ONSMethodExceptionProem(
                                       coder, sel_getUid("decodeBytesForKey:returnedLength:")
                                   ),
                                   key];
    return NULL;
corrupt:
    [coder __failWithExceptionName:(id)ONSInconsistentArchiveException
                         errorCode:4864
                            format:(id)OF_STR("%@: data is corrupt"),
                                   _ONSMethodExceptionProem(
                                       coder, sel_getUid("decodeBytesForKey:returnedLength:")
                                   )];
    return NULL;
}

// Native -[NSKeyedUnarchiver _decodePropertyListForKey:], 0x1c0561cdc. Non-external.
// Unlike public scalar/object wrappers, this method does not first test the helper decode error.
- (id)_decodePropertyListForKey:(id)key {
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot decode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    atomic_store_explicit(
        &_flags, atomic_load_explicit(&_flags, memory_order_acquire) | 8, memory_order_release
    );
    if (key != nil && OCFStringGetLength((OCFStringRef)key) >= 1 &&
        OCFStringGetCharacterAtIndex((OCFStringRef)key, 0) == '$') {
        key = [key substringFromIndex:1];
    }
    if (_containers != NULL) {
        OCFIndex count = OCFArrayGetCount(_containers);
        const void *container = OCFArrayGetValueAtIndex(_containers, count - 1);
        id value;
        if (OCFGetTypeID(container) == 19) {
            value = (id)OCFArrayGetValueAtIndex(container, 0);
            objc_autorelease(objc_retain(value));
            OCFArrayRemoveValueAtIndex((OCFMutableArrayRef)container, 0);
        } else {
            value = (id)OCFDictionaryGetValue(container, key);
            objc_autorelease(objc_retain(value));
        }
        return value;
    }
    id value = nil;
    uint64_t offset = 0;
    if (__OCFBinaryPlistGetOffsetForValueFromDictionary3(
            (const uint8_t *)_bytes,
            _len,
            _offsetData->containerOffset,
            &_offsetData->trailer,
            key,
            NULL,
            &offset,
            NULL,
            _offsetObjMap
        )) {
        if (!__OCFBinaryPlistCreateObject(
                (const uint8_t *)_bytes,
                _len,
                offset,
                &_offsetData->trailer,
                NULL,
                0,
                _offsetObjMap,
                (OCFPropertyListRef *)&value
            )) {
            [self __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                                errorCode:4864
                                   format:(id)OF_STR("%@: data is corrupt or object is too large to decode"),
                                          _ONSMethodExceptionProem(self, _cmd)];
            return nil;
        }
    }
    return objc_autorelease(value);
}

// Native -[NSKeyedUnarchiver _containsNextUnkeyedObject], 0x1c0561f44. Non-external.
// The generic-key counter is not incremented; the XML array existence query still removes its first element
// (kept).
- (BOOL)_containsNextUnkeyedObject {
    int32_t index = _genericKey;
    id key = (uint32_t)index > 39
                 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), (int64_t)index]
                 : (id)generic_keys[index];
    if (key == nil) {
        return NO;
    }
    if (_containers == NULL) {
        return __OCFBinaryPlistGetOffsetForValueFromDictionary3(
            (const uint8_t *)_bytes,
            _len,
            _offsetData->containerOffset,
            &_offsetData->trailer,
            key,
            NULL,
            NULL,
            NULL,
            _offsetObjMap
        );
    }
    OCFIndex count = OCFArrayGetCount(_containers);
    const void *container = OCFArrayGetValueAtIndex(_containers, count - 1);
    id value;
    if (OCFGetTypeID(container) == 19) {
        value = (id)OCFArrayGetValueAtIndex(container, 0);
        objc_autorelease(objc_retain(value));
        OCFArrayRemoveValueAtIndex((OCFMutableArrayRef)container, 0);
    } else {
        value = (id)OCFDictionaryGetValue(container, key);
        objc_autorelease(objc_retain(value));
    }
    return value != nil;
}

// Native -[NSKeyedUnarchiver decodeObject], 0x1c05620b0. Non-external.
- (id)decodeObject {
    if (_warnArchiverCompat == 1) {
        ONSLog(
            (id)OF_STR("%@: warning: old archiving compatibility method called"),
            _ONSMethodExceptionProem(self, _cmd)
        );
    }
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot decode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (UNARCHIVER_HELPER(self)->decodeError != nil) {
        return nil;
    }
    int32_t index = _genericKey;
    _genericKey = (uint32_t)index + 1;
    id key = (uint32_t)index > 39
                 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), (int64_t)index]
                 : (id)generic_keys[index];
    id result;
    _decodeObject(self, key, &result);
    return result;
}

// Native -[NSKeyedUnarchiver decodeDataObject], 0x1c0562210. Non-external.
- (id)decodeDataObject {
    return [self decodeObject];
}

// Native -[NSKeyedUnarchiver decodeBytesWithReturnedLength:], 0x1c0562214. Non-external.
- (void *)decodeBytesWithReturnedLength:(NSUInteger *)length {
    if (_warnArchiverCompat == 1) {
        ONSLog(
            (id)OF_STR("%@: warning: old archiving compatibility method called"),
            _ONSMethodExceptionProem(self, _cmd)
        );
    }
    NSUInteger count = 0;
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot decode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (UNARCHIVER_HELPER(self)->decodeError != nil) {
        if (length != NULL) {
            *length = 0;
        }
        return NULL;
    }
    int32_t index = _genericKey;
    _genericKey = (uint32_t)index + 1;
    id key = (uint32_t)index > 39
                 ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), (int64_t)index]
                 : (id)generic_keys[index];
    const uint8_t *bytes = _decodeBytes(self, key, &count);
    if (length != NULL) {
        *length = count;
    }
    if (bytes != NULL) {
        id data = [(id)archivingMutableDataClass dataWithBytes:bytes length:count];
        if (data != nil) {
            return [data mutableBytes];
        }
        [self __failWithExceptionName:(id)ONSMallocException
                            errorCode:4864
                               format:(id)OF_STR("%@: unable to allocate memory for length (%ld)"),
                                      _ONSMethodExceptionProem(
                                          self, sel_getUid("decodeBytesWithReturnedLength:")
                                      ),
                                      count];
    }
    return NULL;
}

// Native -[NSKeyedUnarchiver decodeValuesOfObjCTypes:], 0x1c05623f0. Non-external.
// The types pointer advances one byte per vararg, even for a composite encoding (kept).
- (void)decodeValuesOfObjCTypes:(const char *)types, ... {
    if (_warnArchiverCompat == 1) {
        ONSLog(
            (id)OF_STR("%@: warning: old archiving compatibility method called"),
            _ONSMethodExceptionProem(self, _cmd)
        );
    }
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot decode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
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
        void *address = va_arg(arguments, void *);
        if (address == NULL) {
            id reason = [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: null address pointer"),
                                                                   _ONSMethodExceptionProem(self, _cmd)];
            objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                          reason:reason
                                                        userInfo:nil]);
        }
        [self decodeValueOfObjCType:types at:address];
    }
    va_end(arguments);
}

// Native -[NSKeyedUnarchiver decodeValueOfObjCType:at:], 0x1c0562588. Non-external.
- (void)decodeValueOfObjCType:(const char *)type at:(void *)address {
    if (_warnArchiverCompat == 1) {
        ONSLog(
            (id)OF_STR("%@: warning: old archiving compatibility method called"),
            _ONSMethodExceptionProem(self, _cmd)
        );
    }
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot decode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (UNARCHIVER_HELPER(self)->decodeError != nil) {
        return;
    }
    atomic_store_explicit(
        &_flags, atomic_load_explicit(&_flags, memory_order_acquire) | 8, memory_order_release
    );
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
                stringWithFormat:(id)OF_STR("%@: this unarchiver cannot decode structs"),
                                 _ONSMethodExceptionProem(self, _cmd)];
            objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                          reason:reason
                                                        userInfo:nil]);
        }
        _compatDecodeValueOfObjCType(self, (signed char)*type, address, _cmd);
        return;
    }
    const unsigned char *element = (const unsigned char *)type + 1;
    uint64_t count = 0;
    while ((unsigned int)(*element - '0') <= 9) {
        count = count * 10 + *element - '0';
        ++element;
    }
    if (count == 0) {
        [self __failWithExceptionName:(id)ONSInvalidArgumentException
                            errorCode:4864
                               format:(id)OF_STR("%@: array count is missing or zero"),
                                      _ONSMethodExceptionProem(self, _cmd)];
    } else if (*element == ']') {
        [self __failWithExceptionName:(id)ONSInvalidArgumentException
                            errorCode:4864
                               format:(id)OF_STR("%@: array type is missing"),
                                      _ONSMethodExceptionProem(self, _cmd)];
    } else {
        [self decodeArrayOfObjCType:(const char *)element count:count at:address];
    }
}

// Native __compatDecodeValueOfObjCType, 0x1c05627e4. Non-external.
// c/C and s/S narrow as signed values. Assembly STRB/STRH stores the narrowed raw value even
// after range failure; the decompiler bool * must not normalize a char to 0 or 1. l/L use 32 bits (kept).
static bool _compatDecodeValueOfObjCType(ONSKeyedUnarchiver *coder, int type, void *address, SEL selector) {
    switch (type) {
        case 'B': {
            int32_t index = coder->_genericKey;
            coder->_genericKey = (uint32_t)index + 1;
            id key = (uint32_t)index > 39
                         ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), (int64_t)index]
                         : (id)generic_keys[index];
            return _decodeBool(coder, key, (bool *)address);
        }
        case 'i':
        case 'I':
        case 'l':
        case 'L': {
            int32_t index = coder->_genericKey;
            coder->_genericKey = (uint32_t)index + 1;
            id key = (uint32_t)index > 39
                         ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), (int64_t)index]
                         : (id)generic_keys[index];
            return _decodeInt32(coder, key, (int32_t *)address);
        }
        case 'q':
        case 'Q': {
            int32_t index = coder->_genericKey;
            coder->_genericKey = (uint32_t)index + 1;
            id key = (uint32_t)index > 39
                         ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), (int64_t)index]
                         : (id)generic_keys[index];
            return _decodeInt64(coder, key, (uint64_t *)address);
        }
        case 'f': {
            int32_t index = coder->_genericKey;
            coder->_genericKey = (uint32_t)index + 1;
            id key = (uint32_t)index > 39
                         ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), (int64_t)index]
                         : (id)generic_keys[index];
            return _decodeFloat(coder, key, (float *)address);
        }
        case 'd': {
            int32_t index = coder->_genericKey;
            coder->_genericKey = (uint32_t)index + 1;
            id key = (uint32_t)index > 39
                         ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), (int64_t)index]
                         : (id)generic_keys[index];
            return _decodeDouble(coder, key, (double *)address);
        }
        case 'c':
        case 'C': {
            int32_t index = coder->_genericKey;
            coder->_genericKey = (uint32_t)index + 1;
            id key = (uint32_t)index > 39
                         ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), (int64_t)index]
                         : (id)generic_keys[index];
            int32_t value;
            bool success = _decodeInt32(coder, key, &value);
            int8_t narrowed = (int8_t)value;
            if (success && value != (int32_t)narrowed) {
                [coder
                    __failWithExceptionName:(id)ONSRangeException
                                  errorCode:4864
                                     format:(id)OF_STR(
                                                "%@: value (%d) for key (%@) too large to fit in a char"
                                            ),
                                            _ONSMethodExceptionProem(coder, sel_getUid("decodeInt32ForKey:")),
                                            value,
                                            key];
                success = false;
            }
            memcpy(address, &narrowed, sizeof(narrowed));
            return success;
        }
        case 's':
        case 'S': {
            int32_t index = coder->_genericKey;
            coder->_genericKey = (uint32_t)index + 1;
            id key = (uint32_t)index > 39
                         ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), (int64_t)index]
                         : (id)generic_keys[index];
            int32_t value;
            bool success = _decodeInt32(coder, key, &value);
            int16_t narrowed = (int16_t)value;
            if (success && value != (int32_t)narrowed) {
                [coder
                    __failWithExceptionName:(id)ONSRangeException
                                  errorCode:4864
                                     format:(id)OF_STR(
                                                "%@: value (%d) for key (%@) too large to fit in a short"
                                            ),
                                            _ONSMethodExceptionProem(coder, sel_getUid("decodeInt32ForKey:")),
                                            value,
                                            key];
                success = false;
            }
            memcpy(address, &narrowed, sizeof(narrowed));
            return success;
        }
        case '@': {
            int32_t index = coder->_genericKey;
            coder->_genericKey = (uint32_t)index + 1;
            id key = (uint32_t)index > 39
                         ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), (int64_t)index]
                         : (id)generic_keys[index];
            if (!_decodeObject(coder, key, (id *)address)) {
                return false;
            }
            objc_retain(*(id *)address);
            return true;
        }
        case '#': {
            id escaped = [(id)OF_STR("$") mutableCopy];
            int32_t index = coder->_genericKey;
            coder->_genericKey = (uint32_t)index + 1;
            id key = (uint32_t)index > 39
                         ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), (int64_t)index]
                         : (id)generic_keys[index];
            [escaped appendString:key];
            id name = [coder decodeObjectOfClass:objc_opt_class((id)archivingStringClass) forKey:escaped];
            objc_release(escaped);
            if (_ONSIsNSString(name)) {
                id helper = coder->_helper;
                Class cls = ONSClassFromString(name);
                if (![helper classNameAllowed:cls]) {
                    [coder
                        __failWithExceptionName:(id)ONSInvalidArgumentException
                                      errorCode:4864
                                         format:(id)OF_STR(
                                                    "%@: data to unarchive contains class (%@) which has not "
                                                    "been allowed"
                                                ),
                                                _ONSMethodExceptionProem(coder, sel_getUid("decodeObject")),
                                                name];
                    return false;
                }
                _NSCoderCheckPotentiallyMangledClassNameAndWarn(cls, 0);
                *(Class *)address = cls;
                return true;
            }
            *(Class *)address = Nil;
            if (name != nil) {
                [coder
                    __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                                  errorCode:4864
                                     format:(id)OF_STR(
                                                "%@: Found non-string object (%@) where expecting class name"
                                            ),
                                            _ONSMethodExceptionProem(coder, sel_getUid("decodeObject")),
                                            name];
            }
            return false;
        }
        case ':': {
            id escaped = [(id)OF_STR("$") mutableCopy];
            int32_t index = coder->_genericKey;
            coder->_genericKey = (uint32_t)index + 1;
            id key = (uint32_t)index > 39
                         ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), (int64_t)index]
                         : (id)generic_keys[index];
            [escaped appendString:key];
            id name = [coder decodeObjectOfClass:objc_opt_class((id)archivingStringClass) forKey:escaped];
            objc_release(escaped);
            if (_ONSIsNSString(name)) {
                *(SEL *)address = ONSSelectorFromString(name);
                return true;
            }
            *(SEL *)address = NULL;
            if (name != nil) {
                [coder __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                                     errorCode:4864
                                        format:(id)OF_STR(
                                                   "%@: Found non-string object (%@) where expecting SEL name"
                                               ),
                                               _ONSMethodExceptionProem(coder, sel_getUid("decodeObject")),
                                               name];
            }
            return false;
        }
        case '%': {
            ONSLog(
                (id)OF_STR("%@: warning: NXAtom being decoded -- will not be uniqued"),
                _ONSMethodExceptionProem(coder, selector)
            );
        }
            __attribute__((fallthrough));
        case '*': {
            id escaped = [(id)OF_STR("$") mutableCopy];
            int32_t index = coder->_genericKey;
            coder->_genericKey = (uint32_t)index + 1;
            id key = (uint32_t)index > 39
                         ? [(id)archivingStringClass stringWithFormat:(id)OF_STR("$%ld"), (int64_t)index]
                         : (id)generic_keys[index];
            [escaped appendString:key];
            id name = [coder decodeObjectOfClass:objc_opt_class((id)archivingStringClass) forKey:escaped];
            objc_release(escaped);
            if (_ONSIsNSString(name)) {
                *(char **)address = strdup([name cString]);
                return true;
            }
            *(char **)address = NULL;
            if (name != nil) {
                [coder __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                                     errorCode:4864
                                        format:(id)OF_STR(
                                                   "%@: Found non-string object (%@) where expecting C string"
                                               ),
                                               _ONSMethodExceptionProem(coder, sel_getUid("decodeObject")),
                                               name];
            }
            return false;
        }
        default: {
            [coder __failWithExceptionName:(id)ONSInvalidArgumentException
                                 errorCode:4864
                                    format:(id)OF_STR("%@: unknown type encoding ('%c')"),
                                           _ONSMethodExceptionProem(coder, selector),
                                           type];
            return false;
        }
    }
}

// Native -[NSKeyedUnarchiver decodeArrayOfObjCType:count:at:], 0x1c05630a4. Non-external.
- (void)decodeArrayOfObjCType:(const char *)type count:(NSUInteger)count at:(void *)address {
    Class arrayClass = objc_opt_class((id)archivingOldStyleArrayClass);
    if (_warnArchiverCompat == 1) {
        ONSLog(
            (id)OF_STR("%@: warning: old archiving compatibility method called"),
            _ONSMethodExceptionProem(self, _cmd)
        );
    }
    if (atomic_load_explicit(&_flags, memory_order_acquire) & 2) {
        id reason =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("%@: unarchive already finished, %@"),
                                                       _ONSMethodExceptionProem(self, _cmd),
                                                       (id)OF_STR("cannot decode anything more")];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidUnarchiveOperationException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (UNARCHIVER_HELPER(self)->decodeError != nil) {
        return;
    }
    atomic_store_explicit(
        &_flags, atomic_load_explicit(&_flags, memory_order_acquire) | 8, memory_order_release
    );
    if (type == NULL || address == NULL) {
        [self __failWithExceptionName:(id)ONSInvalidArgumentException
                            errorCode:4864
                               format:(id)OF_STR("%@: null type or address pointer"),
                                      _ONSMethodExceptionProem(self, _cmd)];
        return;
    }
    if (count == 0) {
        [self __failWithExceptionName:(id)ONSInvalidArgumentException
                            errorCode:4864
                               format:(id)OF_STR("%@: count is zero"), _ONSMethodExceptionProem(self, _cmd)];
        return;
    }
    id array = [self _decodeObjectOfClass:arrayClass];
    if (objc_opt_isKindOfClass(array, arrayClass)) {
        [array fillObjCType:*type count:count at:address];
    } else {
        [self __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                            errorCode:4864
                               format:(id)OF_STR(
                                          "%@: encoded entity at this point is not an array of ObjC type"
                                      ),
                                      _ONSMethodExceptionProem(self, _cmd)];
    }
}

// Native -[NSKeyedUnarchiver versionForClassName:], 0x1c0563298. Non-external.
- (NSInteger)versionForClassName:(id)name {
    Class cls = ONSClassFromString(name);
    return cls != Nil ? [(id)cls version] : LONG_MAX;
}

@end

// Native __decodeObjectReference, 0x1c0563e4c. Non-external.
static bool
_decodeObjectReference(ONSKeyedUnarchiver *coder, NSUInteger format, id key, uint32_t reference, id *result) {
    *result = nil;
    BOOL secure = [coder requiresSecureCoding];
    id object = nil;
    if (!OCFDictionaryGetValueIfPresent(
            coder->_tmpRefObjMap, (const void *)(uintptr_t)reference, (const void **)&object
        )) {
        if (!OCFDictionaryGetValueIfPresent(
                coder->_refObjMap, (const void *)(uintptr_t)reference, (const void **)&object
            )) {
            return false;
        }
        if (secure && ![coder _validateAllowedClassesContainsClassOfReference:object forKey:key]) {
            return true;
        }
        *result = objc_retain(object);
        return true;
    }
    if (!decodingCyclesIsAllowedForCoder(coder)) {
        uint64_t flags = atomic_load_explicit(&coder->_flags, memory_order_acquire);
        if (!(flags & 32)) {
            const char *feature = format == 200 ? "NSKeyedUnarchiverCyclesInSecureModeBinRaiseError"
                                                : "NSKeyedUnarchiverCyclesInSecureModeXMLRaiseError";
            if (!_os_feature_enabled_impl("Foundation", feature)) {
                if (_decodeObjectReference_onceToken != -1) {
                    // Native ____decodeObjectReference_block_invoke, 0x1c0564228. Non-external.
                    dispatch_once(&_decodeObjectReference_onceToken, ^{
                      YOU_HAVE_A_CYCLE_IN_YOUR_OBJECT_GRAPH__THIS_ISNT_A_FOUNDATION_BUG__SEE_ERROR_MESSAGE_();
                    });
                }
                goto validate;
            }
        }
        [coder __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                             errorCode:4864
                                format:(id)OF_STR("%@: object cycles are disallowed"),
                                       _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:"))];
        return true;
    }
validate:
    if (secure && ![coder _validateAllowedClassesContainsClass:[object classForCoder] forKey:key]) {
        return true;
    }
    *result = objc_retain(object);
    return true;
}

// Native _YOU_HAVE_A_CYCLE_IN_YOUR_OBJECT_GRAPH__THIS_ISNT_A_FOUNDATION_BUG__SEE_ERROR_MESSAGE_,
// 0x1c056422c. Non-external.
static void YOU_HAVE_A_CYCLE_IN_YOUR_OBJECT_GRAPH__THIS_ISNT_A_FOUNDATION_BUG__SEE_ERROR_MESSAGE_(void) {
    os_log_t log = _NSOSLog();
    if (os_log_type_enabled(log, OS_LOG_TYPE_FAULT)) {
        uint8_t buffer[2] = {0, 0};
        _os_log_fault_impl(
            &__dso_handle,
            log,
            OS_LOG_TYPE_FAULT,
            "Decoder with secure coding enabled is decoding a cycle. See rdar://105473537 for help\n",
            buffer,
            sizeof(buffer)
        );
    }
}

// Native _coderAllocateClassForUnarchival, 0x1c0564004. Non-external.
// Only the first IMP differing from the cached NSObject implementations is tested against the
// immediate superclass; the other two selectors are not then inspected (kept).
static id coderAllocateClassForUnarchival(ONSKeyedUnarchiver *coder, Class cls, bool *allowed) {
    *allowed = true;
    if ([coder requiresSecureCoding]) {
        if (qword_22E76C600 != -1) {
            // Native ___coderAllocateClassForUnarchival_block_invoke, 0x1c05642e8. Non-external.
            // The selectors at 0x2387c317f, 0x2387c315b and 0x2387ada3f match the parent's comparison order.
            dispatch_once(&qword_22E76C600, ^{
              keyedUnarchiverBaseImplementations[0] = class_getMethodImplementation(
                  (Class)archivingObjectClass, sel_getUid("classForKeyedArchiver")
              );
              keyedUnarchiverBaseImplementations[1] =
                  class_getMethodImplementation((Class)archivingObjectClass, sel_getUid("classForArchiver"));
              keyedUnarchiverBaseImplementations[2] =
                  class_getMethodImplementation((Class)archivingObjectClass, sel_getUid("classForCoder"));
            });
        }
        SEL selectors[] = {
            sel_getUid("classForKeyedArchiver"), sel_getUid("classForArchiver"), sel_getUid("classForCoder")
        };
        Class superclass = class_getSuperclass(cls);
        for (unsigned int index = 0; index != 3; ++index) {
            SEL selector = selectors[index];
            IMP implementation = class_getMethodImplementation(cls, selector);
            if (implementation != keyedUnarchiverBaseImplementations[index]) {
                if (superclass != (Class)archivingObjectClass &&
                    implementation == class_getMethodImplementation(superclass, selector)) {
                    *allowed = false;
                }
                break;
            }
        }
    }
    if (*allowed) {
        return objc_allocWithZone(cls);
    }
    os_log_t log = _NSOSLog();
    if (os_log_type_enabled(log, OS_LOG_TYPE_FAULT)) {
        uint8_t buffer[22];
#define INHERITED_CODER_CLASS_WARNING                                                                        \
    "Class '%{public}@' unexpectedly found in possibly-altered archive. -[%{public}@ classForCoder] is "     \
    "inherited from "                                                                                        \
    "a superclass and should likely have replaced this object at encode time. To be allowed in archives, "   \
    "it should "                                                                                             \
    "override -classForCoder itself."
        __builtin_os_log_format(buffer, INHERITED_CODER_CLASS_WARNING, (id)cls, (id)cls);
        _os_log_fault_impl(
            &__dso_handle, log, OS_LOG_TYPE_FAULT, INHERITED_CODER_CLASS_WARNING, buffer, sizeof(buffer)
        );
#undef INHERITED_CODER_CLASS_WARNING
    }
    [coder __failWithExceptionName:(id)ONSInconsistentArchiveException
                         errorCode:4864
                            format:(id)OF_STR(
                                       "%@: class '%@' unexpectedly found in possibly-altered archive. -[%@ "
                                       "classForCoder] is inherited "
                                       "from a superclass and should likely have replaced this object at "
                                       "encode time. To be allowed in archives, it "
                                       "should override -classForCoder itself"
                                   ),
                                   _ONSMethodExceptionProem(coder, sel_getUid("decodeObjectForKey:")),
                                   cls,
                                   cls];
    return nil;
}

// Native __coderValidateClassValidToMessageForUnarchiving, 0x1c056428c. Non-external.
static bool _coderValidateClassValidToMessageForUnarchiving(Class cls) {
    return class_respondsToSelector(cls, sel_getUid("methodSignatureForSelector:")) &&
           class_respondsToSelector(cls, sel_getUid("doesNotRecognizeSelector:")) &&
           class_respondsToSelector(cls, sel_getUid("forwardInvocation:"));
}

// Native __decodeObject, 0x1c0fddb78. Non-external.
// The inner decode returns an owned result across the private pool. Pop the pool before autoreleasing
// that result into the caller pool, on success or a normal failure.
static bool _decodeObject(ONSKeyedUnarchiver *coder, const void *key, id *result) {
    id object = nil;
    atomic_store_explicit(
        &coder->_flags, atomic_load_explicit(&coder->_flags, memory_order_acquire) | 8, memory_order_release
    );
    void *pool = objc_autoreleasePoolPush();
    bool success;
    if (coder->_containers != NULL) {
        success = _decodeObjectXML(coder, key, &object);
    } else {
        uint64_t reference = 0;
        uint64_t offset = 0;
        success = _decodeOffsetAndRef(coder, (id)key, &offset, &reference);
        if (success) {
            success = _decodeObjectBinary(coder, (uint32_t)reference, (id)key, &object);
        }
    }
    objc_autoreleasePoolPop(pool);
    *result = objc_autorelease(object);
    return success;
}

@implementation _ONSKeyedCoderOldStyleArray (Decoding)

// Native -[_NSKeyedCoderOldStyleArray initWithCoder:], 0x1c05637dc. Non-external.
// This method shares the native file-local compatibility decoder.
- (id)initWithCoder:(id)coder {
    ONSKeyedUnarchiver *unarchiver = coder;
    int32_t count = [unarchiver decodeInt32ForKey:(id)OF_STR("NS.count")];
    if (count < 0) {
        id proem = _ONSMethodExceptionProem(self, _cmd);
        objc_release(self);
        [unarchiver __failWithExceptionName:(id)OF_STR("NSArchiverArchiveInconsistency")
                                  errorCode:4864
                                     format:(id)OF_STR("%@: unable to decode array with negative count (%d)"),
                                            proem,
                                            count];
        return nil;
    }
    _count = (NSUInteger)count;
    int32_t archivedSize = [unarchiver decodeInt32ForKey:(id)OF_STR("NS.size")];
    if (archivedSize < 0) {
        id proem = _ONSMethodExceptionProem(self, _cmd);
        objc_release(self);
        [unarchiver __failWithExceptionName:(id)OF_STR("NSArchiverArchiveInconsistency")
                                  errorCode:4864
                                     format:(id)OF_STR("%@: unable to decode array with negative size (%d)"),
                                            proem,
                                            archivedSize];
        return nil;
    }
    _size = (NSUInteger)archivedSize;
    int32_t type = [unarchiver decodeInt32ForKey:(id)OF_STR("NS.type")];
    if (type != (char)type) {
        id proem = _ONSMethodExceptionProem(self, _cmd);
        objc_release(self);
        [unarchiver __failWithExceptionName:(id)OF_STR("NSArchiverArchiveInconsistency")
                                  errorCode:4864
                                     format:(id)OF_STR("%@: unable to decode array with invalid type (%d)"),
                                            proem,
                                            type];
        return nil;
    }
    _type = (char)type;
    NSUInteger size = 0;
    NSUInteger alignment = 0;
    char encoding[2] = {(char)type, '\0'};
    ONSGetSizeAndAlignment(encoding, &size, &alignment);
    _size = size > alignment ? size : alignment;
    _addr = malloc_type_calloc(_count, _size, 0x8617DF4E);
    if (_addr == NULL) {
        id proem = _ONSMethodExceptionProem(self, _cmd);
        objc_release(self);
        [unarchiver __failWithExceptionName:(id)ONSMallocException
                                  errorCode:4864
                                     format:(id)OF_STR(
                                                "%@: unable to allocate memory for size (%ld) and count (%ld)"
                                            ),
                                            proem,
                                            (NSInteger)archivedSize,
                                            (NSInteger)count];
        return nil;
    }
    _decoded = YES;
    SEL selector = sel_getUid("decodeArrayOfObjCType:count:at:");
    for (NSUInteger index = 0; index < _count; ++index) {
        if (!_compatDecodeValueOfObjCType(unarchiver, _type, (char *)_addr + _size * index, selector)) {
            id proem = _ONSMethodExceptionProem(self, _cmd);
            objc_release(self);
            if ([unarchiver error] == nil) {
                [unarchiver __failWithExceptionName:(id)OF_STR("NSArchiverArchiveInconsistency")
                                          errorCode:4864
                                             format:(id)OF_STR(
                                                        "%@: unable to decode element in array of size (%ld) "
                                                        "and count (%ld)"
                                                    ),
                                                    proem,
                                                    (NSInteger)archivedSize,
                                                    (NSInteger)count];
            }
            return nil;
        }
    }
    return self;
}

@end

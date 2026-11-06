//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Archiving/ONSCoder.h"

#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Collections/OCFSet.h"
#import "CoreFoundation/Objective-C/OCFNSArray.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "CoreFoundation/Objective-C/OCFNSSet.h"
#import "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#import "Foundation/Archiving/ONSArchivingPrivate.h"
#import "Foundation/Archiving/ONSObjCRuntimeNames.h"
#import "Foundation/Archiving/ONSRuntimeIssuesLog.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSStringOtherEncodings.h"

#include <dlfcn.h>
#include <objc/objc-exception.h>
#include <string.h>
#include <unistd.h>

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. These are MRC units: explicit objc_retain/release/autorelease and __block object
// copy/dispose operations are preserved. iOS Device divergence: the iOS Device 24A446 bodies remain unread.
// Reading: Source/Foundation/Archiving/Reading.md.

// Native NSInvalidArchiveOperationException, 0x2227d54a0, and NSInvalidUnarchiveOperationException,
// 0x2227d54a8. External. Their pointed-to constant-string records are 0x22282b450 and 0x22282b470.
static const OCFConstantString kInvalidArchiveString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(ONSInvalidArchiveOperationException),
    sizeof(OF_NAME_STRING(ONSInvalidArchiveOperationException)) - 1
};
static const OCFConstantString kInvalidUnarchiveString = {
    (uintptr_t)__OCFConstantStringClassReference,
    0x7c8,
    OF_NAME_STRING(ONSInvalidUnarchiveOperationException),
    sizeof(OF_NAME_STRING(ONSInvalidUnarchiveOperationException)) - 1
};
const OCFStringRef ONSInvalidArchiveOperationException = (OCFStringRef)&kInvalidArchiveString;
const OCFStringRef ONSInvalidUnarchiveOperationException = (OCFStringRef)&kInvalidUnarchiveString;

// Native zero-initialized trust state (0x22e76c144), once token (0x22e76c148), collection-class set
// (0x22e76c160, token 0x22e76c168) and explicit-subclass map (0x22e76c170, token 0x22e76c178).
unsigned int dword_22E76C144;
static dispatch_once_t qword_22E76C148;
id qword_22E76C160;
static dispatch_once_t qword_22E76C168;
OCFMutableDictionaryRef qword_22E76C170;
static dispatch_once_t qword_22E76C178;
// The exemption result is process-wide: the first supplied path is used for all subsequent calls (kept).
static dispatch_once_t qword_22E76C150;
static uint8_t byte_22E76C141;
static uint8_t _MergedGlobals_79;

// Native ____NSCoderGetTrustRestrictions_block_invoke, 0x1c04dc3e4. Non-external.
// The call through the status function receives the task in X0. Hex-Rays omits that argument.
static void ___NSCoderGetTrustRestrictions_block_invoke(const void *block) {
    (void)block;
    void *security = dlopen("/System/Library/Frameworks/Security.framework/Security", RTLD_FIRST);
    if (security != NULL) {
        OCFTypeRef (*createTask)(OCFAllocatorRef) = dlsym(security, "SecTaskCreateFromSelf");
        uint32_t (*getStatus)(OCFTypeRef) = dlsym(security, "SecTaskGetCodeSignStatus");
        OCFTypeRef (*copySigningIdentifier)(OCFTypeRef, OCFErrorRef *) =
            dlsym(security, "SecTaskCopySigningIdentifier");
        if (createTask != NULL && getStatus != NULL && copySigningIdentifier != NULL) {
            OCFTypeRef task = createTask(kOCFAllocatorDefault);
            uint32_t status = getStatus(task);
            if ((status & 0x04000001) == 0x04000001) {
                dword_22E76C144 = 0;
            } else {
                OCFTypeRef identifier = copySigningIdentifier(task, NULL);
                if (identifier != NULL) {
                    dword_22E76C144 = _ONSCoderIsAppleSigningIdentifier((id)identifier) ? 1 : 2;
                    OCFRelease(identifier);
                }
            }
            OCFRelease(task);
            return;
        }
    }
    os_log_t log = _NSOSLog();
    if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
        uint8_t payload[2] = {0, 0};
        _os_log_error_impl(
            &__dso_handle,
            log,
            OS_LOG_TYPE_ERROR,
            "NSCoder couldn't load a Security symbol.",
            payload,
            sizeof(payload)
        );
    }
}

static const ArchivingBlockDescriptor kTrustRestrictionsDescriptor = {0x50000000, 32, "v8@?0"};
// Native block literal at 0x22281c688, descriptor at 0x2227d03f8.
static const ArchivingGlobalBlock kTrustRestrictionsBlock = {
    _NSConcreteGlobalBlock,
    0x50000000,
    0,
    ___NSCoderGetTrustRestrictions_block_invoke,
    &kTrustRestrictionsDescriptor
};

// Native _explicitSubclassExceptionKeys and _explicitSubclassExceptionObjects,
// 0x2227d07d0 and 0x2227d0870. Non-external arrays of class objects.
static const void *const _explicitSubclassExceptionKeys[] = {
    archivingStringClass,
    archivingMutableStringClass,
    archivingLocalizedStringClass,
    archivingAttributedStringClass,
    archivingMutableAttributedStringClass,
    archivingArrayClass,
    archivingMutableArrayClass,
    archivingDictionaryClass,
    archivingMutableDictionaryClass,
    archivingDataClass,
    archivingMutableDataClass,
    archivingSetClass,
    archivingMutableSetClass,
    archivingOrderedSetClass,
    archivingMutableOrderedSetClass,
    archivingCountedSetClass,
    archivingIndexSetClass,
    archivingMutableIndexSetClass,
    archivingHashTableClass,
    archivingPointerArrayClass,
};
static const void *const _explicitSubclassExceptionObjects[] = {
    archivingMutableStringClass,
    archivingStringClass,
    archivingMutableStringClass,
    archivingMutableAttributedStringClass,
    archivingAttributedStringClass,
    archivingMutableArrayClass,
    archivingArrayClass,
    archivingMutableDictionaryClass,
    archivingDictionaryClass,
    archivingMutableDataClass,
    archivingDataClass,
    archivingMutableSetClass,
    archivingSetClass,
    archivingMutableOrderedSetClass,
    archivingOrderedSetClass,
    archivingMutableSetClass,
    archivingMutableIndexSetClass,
    archivingIndexSetClass,
    archivingHashTableClass,
    archivingPointerArrayClass,
};

// Native ____explicitSubclassExceptions_block_invoke, 0x1c04de1bc. Non-external.
static dispatch_block_t const kSubclassExceptionsBlock = ^{
  OCFAllocatorRef allocator = kOCFAllocatorDefault;
  OCFDictionaryRef initial = OCFDictionaryCreate(
      allocator,
      (const void **)_explicitSubclassExceptionKeys,
      (const void **)_explicitSubclassExceptionObjects,
      20,
      NULL,
      NULL
  );
  qword_22E76C170 = OCFDictionaryCreateMutableCopy(allocator, 21, initial);
  OCFRelease(initial);

  OCFMutableDictionaryRef exceptions = qword_22E76C170;
  Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSSharedKeyDictionary)));
  OCFDictionaryAddValue(exceptions, cls, archivingMutableDictionaryClass);
  exceptions = qword_22E76C170;
  cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(__ONSCFString)));
  OCFDictionaryAddValue(exceptions, cls, archivingMutableStringClass);
  exceptions = qword_22E76C170;
  cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(__ONSCFConstantString)));
  OCFDictionaryAddValue(exceptions, cls, archivingMutableStringClass);
};

@interface ONSCoder (DecodingMessages)
- (id)error;
- (int64_t)decodeInt64ForKey:(id)key;
- (void)decodeValueOfObjCType:(const char *)type at:(void *)address size:(NSUInteger)size;
- (void)decodeArrayOfObjCType:(const char *)type count:(NSUInteger)count at:(void *)address;
@end

// GOT 0x22227FB20, bound to NSMutableData; the class is owned by the data restoration.
@protocol CoderMutableDataMessages
+ (id)dataWithLength:(NSUInteger)length;
@end

@implementation ONSCoder

// Native -[NSCoder decodeObject], 0x1c04daed8. Non-external.
- (id)decodeObject {
    if ([self error] != nil) {
        return nil;
    }
    id object = nil;
    [self decodeValueOfObjCType:"@" at:&object size:8];
    return objc_autorelease(object);
}

// Native -[NSCoder decodeBytesWithReturnedLength:], 0x1c04db378. Non-external.
- (void *)decodeBytesWithReturnedLength:(NSUInteger *)lengthp {
    unsigned int length = 0;
    [self decodeValueOfObjCType:"I" at:&length size:4];
    id data = [(Class<CoderMutableDataMessages>)archivingMutableDataClass dataWithLength:length];
    void *bytes = [data mutableBytes];
    [self decodeArrayOfObjCType:"c" count:length at:bytes];
    *lengthp = length;
    return bytes;
}

// Native -[NSCoder allowsKeyedCoding], 0x1c04db4bc. Non-external.
- (BOOL)allowsKeyedCoding {
    return NO;
}

// Native -[NSCoder decodeIntegerForKey:], 0x1c04db56c. Non-external.
- (NSInteger)decodeIntegerForKey:(id)key {
    return [self decodeInt64ForKey:key];
}

// Native -[NSCoder decodeObjectOfClass:forKey:], 0x1c04db634. Non-external.
- (id)decodeObjectOfClass:(Class)cls forKey:(id)key {
    return [self decodeObjectOfClasses:[(id)archivingSetClass setWithObject:cls] forKey:key];
}

// Native -[NSCoder _validateDecodeCollectionAllowedClassesRequirementsWithClasses:forMethodExceptionProem:],
// 0x1c04db734. Non-external (was a private external); objc_direct.
// Folded byref cleanup landing pads 0x1c04db98c-0x1c04db9ac; LSDA 0x1c149fdb0.
- (BOOL)_validateDecodeCollectionAllowedClassesRequirementsWithClasses:(id)classes
                                               forMethodExceptionProem:(id)proem {
    for (Class cls in classes) {
        if (!object_isClass((id)cls)) {
            id reason = [(id)archivingStringClass
                stringWithFormat:(id)OF_STR("%@ object in class list '%@' is not an Objective-C class!"),
                                 proem,
                                 cls];
            objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                          reason:reason
                                                        userInfo:nil]);
        }
        if (cls == (Class)archivingObjectClass &&
            ![self _warnAboutNSObjectInAllowedClassesWithException:YES]) {
            return NO;
        }
    }
    // Native __Block_byref_object_copy__1, 0x1c0fdc1f8, and __Block_byref_object_dispose__1, 0x1c0fdc208:
    // the compiler's MRC __block id cell uses _Block_object_assign/dispose with flags 0x83, at offset 40.
    __block id description = nil;
    // Native ___98-[NSCoder _validateDecodeCollectionAllowedClassesRequirementsWithClasses:
    // forMethodExceptionProem:]_block_invoke, 0x1c04dc1ac. Non-external.
    [classes enumerateObjectsUsingBlock:^(id cls, BOOL *stop) {
      (void)stop;
      if ([self _isPossibleNestedCollectionClass:cls]) {
          id item = [(id)archivingStringClass
              stringWithFormat:(id)OF_STR("\n\t\t%@"), [self __allowedClassesDescriptionForClass:cls]];
          if (description != nil) {
              [description appendString:item];
          } else {
              description =
                  [objc_alloc((Class)archivingMutableStringClass) initWithFormat:(id)OF_STR("\n\t(%@"), item];
          }
      }
    }];
    BOOL valid = description == nil;
    if (description != nil) {
        [description appendString:(id)OF_STR("\n\t)\n")];
        id reason = [(id)archivingStringClass
            stringWithFormat:(id)OF_STR(
                                 "%@: This method only supports decoding non-nested collections. Please "
                                 "remove the following or use "
                                 "'-decodeObjectOfClasses: forKey:' instead: %@"
                             ),
                             proem,
                             description];
        objc_release(description);
        [self __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                            errorCode:4864
                               format:(id)OF_STR("%@"), reason];
    }
    return valid;
}

// Native -[NSCoder _isPossibleNestedCollectionClass:], 0x1c04dbc1c. Private external; objc_direct.
- (BOOL)_isPossibleNestedCollectionClass:(Class)cls {
    while (cls != Nil) {
        if (cls == (Class)archivingObjectClass) {
            return NO;
        }
        if (qword_22E76C168 != -1) {
            // Native ____collectionClasses_block_invoke, 0x1c04de0cc. Non-external.
            // Shared-cache class bindings, confidence: medium. The second NSMutableSet is kept.
            dispatch_once(&qword_22E76C168, ^{
              id set = objc_alloc((Class)archivingSetClass);
              id classes[] = {
                  (id)archivingArrayClass,
                  (id)archivingMutableArrayClass,
                  (id)archivingDictionaryClass,
                  (id)archivingMutableDictionaryClass,
                  (id)archivingMutableSetClass,
                  (id)archivingSetClass,
                  (id)archivingMutableOrderedSetClass,
                  (id)archivingOrderedSetClass,
                  (id)archivingMutableSetClass,
                  (id)archivingHashTableClass,
                  (id)archivingPointerArrayClass,
              };
              qword_22E76C160 = [set initWithArray:[(id)archivingArrayClass arrayWithObjects:classes
                                                                                       count:11]];
            });
        }
        if ([qword_22E76C160 containsObject:(id)cls]) {
            return YES;
        }
        cls = class_getSuperclass(cls);
    }
    return NO;
}

// Native -[NSCoder _warnAboutNSObjectInAllowedClassesWithException:], 0x1c04dc030. Non-external;
// objc_direct. The logging arguments are evaluated only after the log-enabled check.
- (BOOL)_warnAboutNSObjectInAllowedClassesWithException:(BOOL)exception {
    id info = [self __decoderInfoForAllowedClassesWarning];
    os_log_t log = _NSRuntimeIssuesLog();
    if (os_log_type_enabled(log, OS_LOG_TYPE_FAULT)) {
        id proem = _ONSMethodExceptionProem(self, sel_getUid("validateAllowedClass:forKey:"));
        id classes = [self _allowedClassesDescriptionForClasses:[self allowedClasses]];
        uint8_t buffer[32];
#define OBJECT_CLASSES_WARNING                                                                               \
    "%{public}@: NSSecureCoding allowed classes list contains [NSObject class], which bypasses security by " \
    "allowing any Objective-C class to be implicitly decoded. Consider reducing the scope of allowed "       \
    "classes "                                                                                               \
    "during decoding by listing only the classes you expect to decode, or a more specific base class than "  \
    "NSObject. This will become an error in the future. Allowed class list: %{public}@%{public}@"
        __builtin_os_log_format(buffer, OBJECT_CLASSES_WARNING, proem, classes, info);
        _os_log_fault_impl(
            &__dso_handle, log, OS_LOG_TYPE_FAULT, OBJECT_CLASSES_WARNING, buffer, sizeof(buffer)
        );
#undef OBJECT_CLASSES_WARNING
    }
    if (!exception || !dyld_program_sdk_at_least((OCFBuildVersion){UINT32_MAX, 0x07e50901})) {
        return YES;
    }
    if (qword_22E76C148 != -1) {
        dispatch_once(&qword_22E76C148, (dispatch_block_t)(void *)&kTrustRestrictionsBlock);
    }
    if (dword_22E76C144 == 2) {
        return YES;
    }
    id proem = _ONSMethodExceptionProem(self, sel_getUid("validateAllowedClass:forKey:"));
    id classes = [self _allowedClassesDescriptionForClasses:[self allowedClasses]];
    [self __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                        errorCode:4864
                           format:(id)OF_STR(
                                      "%@: NSSecureCoding allowed classes list contains '[NSObject class]', "
                                      "which bypasses "
                                      "security by allowing any Objective-C class to be implicitly decoded. "
                                      "Consider reducing the scope of "
                                      "allowed classes during decoding by listing only the classes you "
                                      "expect to decode, or a more specific "
                                      "base class than NSObject. Allowed class list: %@%@"
                                  ),
                                  proem,
                                  classes,
                                  info];
    return NO;
}

// Native -[NSCoder __allowedClassesDescriptionForClass:], 0x1c04dc258. Private external; objc_direct.
- (id)__allowedClassesDescriptionForClass:(Class)cls {
    id bundle = [(id)archivingBundleClass bundleForClass:cls];
    id path = [[bundle bundleURL] path];
    return [(id)archivingStringClass stringWithFormat:(id)OF_STR("'%@' (%p) [%@]"), cls, cls, path];
}

// Native __NSCoderIsAppleSigningIdentifier, 0x1c04dc2b8. External.
// The assembly returns a Boolean and supplies a range starting at zero to the encoding method.
BOOL _ONSCoderIsAppleSigningIdentifier(id identifier) {
    char bytes[76] = {0};
    ONSRange range = {0, [identifier length]};
    if (![identifier getBytes:bytes
                    maxLength:sizeof(bytes)
                   usedLength:NULL
                     encoding:1
                      options:0
                        range:range
               remainingRange:NULL]) {
        return NO;
    }
    bytes[75] = 0;
    size_t length = 0;
    unsigned char byte;
    do {
        byte = (unsigned char)bytes[length++];
    } while ((byte >= 'A' && byte <= 'Z') || (byte >= '0' && byte <= '9'));
    if (length == 1) {
        return strncmp(bytes, "com.apple.", 10) == 0;
    }
    return byte == '.' && strncmp(bytes + length, "com.apple.", 10) == 0;
}

// Native _NSCoderGetTrustRestrictions, 0x1c04dc3a4. Non-external (was a private external).
unsigned int _NSCoderGetTrustRestrictions(void) {
    if (qword_22E76C148 != -1) {
        dispatch_once(&qword_22E76C148, (dispatch_block_t)(void *)&kTrustRestrictionsBlock);
    }
    return dword_22E76C144;
}

// Native -[NSCoder _walkAllowedClassListLookingForClass:forKey:allowedClasses:strictModeEnabled:
// alwaysEnforceExplicitSubclasses:specialCaseObject:], 0x1c04dc528. Private external; objc_direct.
- (BOOL)_walkAllowedClassListLookingForClass:(Class)cls
                                      forKey:(id)key
                              allowedClasses:(id)classes
                           strictModeEnabled:(BOOL)strictModeEnabled
             alwaysEnforceExplicitSubclasses:(BOOL)alwaysEnforceExplicitSubclasses
                           specialCaseObject:(BOOL)specialCaseObject {
    if (object_isClass(classes)) {
        classes = [(id)archivingSetClass setWithObject:classes];
    }
    if (classes == nil) {
        return NO;
    }
    if (!object_isClass((id)cls)) {
        id proem = _ONSMethodExceptionProem(self, sel_getUid("validateAllowedClass:forKey:"));
        id reason = [(id)archivingStringClass
            stringWithFormat:(id)OF_STR("%@ object in class list '%@' is not an Objective-C class!"),
                             proem,
                             cls];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (strictModeEnabled) {
        return [classes containsObject:(id)cls];
    }
    BOOL warned = [classes containsObject:(id)archivingObjectClass];
    if (warned && ![self _warnAboutNSObjectInAllowedClassesWithException:!specialCaseObject]) {
        return NO;
    }
    for (Class current = cls; current != Nil; current = class_getSuperclass(current)) {
        if (!object_isClass((id)current)) {
            id proem = _ONSMethodExceptionProem(self, sel_getUid("validateAllowedClass:forKey:"));
            id reason = [(id)archivingStringClass
                stringWithFormat:(id)OF_STR("%@ object in class list '%@' is not an Objective-C class!"),
                                 proem,
                                 cls];
            objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                          reason:reason
                                                        userInfo:nil]);
        }
        if ([classes containsObject:(id)current]) {
            return YES;
        }
        BOOL explicitSubclasses = NO;
        if (objc_opt_respondsToSelector(
                (id)current, sel_getUid("_subclassesMustBeExplicitlyMentionedWhenDecoded")
            )) {
            explicitSubclasses = [(id)current _subclassesMustBeExplicitlyMentionedWhenDecoded];
        }
        BOOL allowCollectionSubclasses = NO;
        if (!alwaysEnforceExplicitSubclasses) {
            allowCollectionSubclasses =
                !_os_feature_enabled_impl("ArchivingExceptions", "NSKeyedUnarchiverCollectionSubclasses");
        }
        bool linked = dyld_program_sdk_at_least((OCFBuildVersion){UINT32_MAX, 0x07e50901});
        if ((linked & explicitSubclasses) != 1) {
            continue;
        }
        SEL classForCoder = sel_getUid("classForCoder");
        IMP base = class_getMethodImplementation((Class)archivingObjectClass, classForCoder);
        IMP implementation = class_getMethodImplementation(current, classForCoder);
        if (object_getClass((id)current) == (Class)archivingObjectClass) {
            continue;
        }
        if (implementation == base) {
            Method method = class_getInstanceMethod(current, classForCoder);
            if (method == class_getInstanceMethod((Class)archivingObjectClass, classForCoder)) {
                continue;
            }
        }
        Class superclass = class_getSuperclass(current);
        if (implementation == class_getMethodImplementation(superclass, classForCoder)) {
            Method method = class_getInstanceMethod(current, classForCoder);
            if (method == class_getInstanceMethod(superclass, classForCoder)) {
                continue;
            }
        }
        if (qword_22E76C178 != -1) {
            dispatch_once(&qword_22E76C178, kSubclassExceptionsBlock);
        }
        const void *first = OCFDictionaryGetValue(qword_22E76C170, current);
        if (first != NULL) {
            if ([classes containsObject:(id)first]) {
                return YES;
            }
            if (qword_22E76C178 != -1) {
                dispatch_once(&qword_22E76C178, kSubclassExceptionsBlock);
            }
            const void *second = OCFDictionaryGetValue(qword_22E76C170, first);
            if (second != NULL) {
                BOOL allowed = [classes containsObject:(id)second];
                if (alwaysEnforceExplicitSubclasses || allowed) {
                    return allowed;
                }
            } else if (alwaysEnforceExplicitSubclasses) {
                return NO;
            }
        } else if (alwaysEnforceExplicitSubclasses) {
            return NO;
        }
        if (qword_22E76C148 != -1) {
            dispatch_once(&qword_22E76C148, (dispatch_block_t)(void *)&kTrustRestrictionsBlock);
        }
        if (dword_22E76C144 == 2) {
            continue;
        }
        if (!warned) {
            id info = [self __decoderInfoForAllowedClassesWarning];
            id bundle = [(id)archivingBundleClass bundleForClass:cls];
            id path = [[bundle bundleURL] path];
            id description =
                [(id)archivingStringClass stringWithFormat:(id)OF_STR("'%@' (%p) [%@]"), cls, cls, path];
            os_log_t log = _NSRuntimeIssuesLog();
            if (os_log_type_enabled(log, OS_LOG_TYPE_FAULT)) {
                uint8_t buffer[62];
#define COLLECTION_SUBCLASS_WARNING                                                                          \
    "Attempted to decode a collection type '%{public}@' (subclass of '%{public}@') for key '%{public}@'. "   \
    "'%{public}@' requires its subclasses to be explicitly added to the allowed classes list but it is not " \
    "present. Allowing this has been a source of security issues. Please ensure you meant this type to be "  \
    "in archives: %{public}@%{public}@"
                __builtin_os_log_format(
                    buffer,
                    COLLECTION_SUBCLASS_WARNING,
                    (id)current,
                    (id)superclass,
                    key,
                    (id)current,
                    description,
                    info
                );
                _os_log_fault_impl(
                    &__dso_handle, log, OS_LOG_TYPE_FAULT, COLLECTION_SUBCLASS_WARNING, buffer, sizeof(buffer)
                );
#undef COLLECTION_SUBCLASS_WARNING
            }
        }
        if (!(specialCaseObject || allowCollectionSubclasses)) {
            return [self
                _shouldExemptProcessBasedOnArchivingExceptionRules:
                    "/System/Library/CoreServices/ArchivingExceptions.bundle/CollectionSubclasses.plist"];
        }
        warned = YES;
    }
    return NO;
}

// Native -[NSCoder _shouldExemptProcessBasedOnArchivingExceptionRules:], 0x1c04dc9fc. Private external;
// objc_direct. The first path wins even when a different archiving exception rule later calls this method.
- (BOOL)_shouldExemptProcessBasedOnArchivingExceptionRules:(const char *)path {
    // Native ___62-[NSCoder _shouldExemptProcessBasedOnArchivingExceptionRules:]_block_invoke,
    // 0x1c04dcde0. Non-external. The sandbox filter is SANDBOX_CHECK_NO_REPORT | SANDBOX_FILTER_PATH.
    dispatch_once(&qword_22E76C150, ^{
      id names = nil;
      BOOL allowAll = sandbox_check(getpid(), "file-read-data", SANDBOX_CHECK_NO_REPORT | 1, path) != 0;
      if (!allowAll) {
          id file = [(id)archivingStringClass stringWithCString:path encoding:4];
          names = [objc_alloc((Class)archivingArrayClass) initWithContentsOfFile:file];
          allowAll = names == nil;
      }
      if (allowAll) {
          byte_22E76C141 = 1;
          os_log_t log = _NSOSLog();
          if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
              uint8_t buffer[2] = {0, 0};
              _os_log_error_impl(
                  &__dso_handle,
                  log,
                  OS_LOG_TYPE_ERROR,
                  "No ArchivingExceptions available / readable. Allowing all.",
                  buffer,
                  sizeof(buffer)
              );
          }
      } else {
          id info = [(id)archivingProcessInfoClass processInfo];
          byte_22E76C141 = [names containsObject:[info processName]];
      }
      objc_release(names);
      uint8_t exempt = 1;
      if ((byte_22E76C141 & 1) == 0) {
          if (qword_22E76C148 != -1) {
              dispatch_once(&qword_22E76C148, (dispatch_block_t)(void *)&kTrustRestrictionsBlock);
          }
          exempt = byte_22E76C141 | (dword_22E76C144 == 2);
      }
      _MergedGlobals_79 = exempt & 1;
    });
    return _MergedGlobals_79 & 1;
}

// Native -[NSCoder _walkAllowedClassListValidatingInstance:forKey:allowedClasses:strictModeEnabled:
// alwaysEnforceExplicitSubclasses:specialCaseObject:invalidElementClass:], 0x1c04dcaf4. Private external;
// objc_direct. Work and visited collections have zero callbacks and do not retain their elements.
- (BOOL)_walkAllowedClassListValidatingInstance:(id)instance
                                         forKey:(id)key
                                 allowedClasses:(id)allowedClasses
                              strictModeEnabled:(BOOL)strictModeEnabled
                alwaysEnforceExplicitSubclasses:(BOOL)alwaysEnforceExplicitSubclasses
                              specialCaseObject:(BOOL)specialCaseObject
                            invalidElementClass:(Class *)invalidElementClass {
    if (![self _walkAllowedClassListLookingForClass:[instance classForCoder]
                                             forKey:key
                                     allowedClasses:allowedClasses
                                  strictModeEnabled:strictModeEnabled
                    alwaysEnforceExplicitSubclasses:alwaysEnforceExplicitSubclasses
                                  specialCaseObject:specialCaseObject]) {
        return NO;
    }
    if (![self _enforceCollectionTyping] ||
        (!_ONSIsNSArray(instance) && !_ONSIsNSSet(instance) && !_ONSIsNSOrderedSet(instance) &&
         !_ONSIsNSDictionary(instance))) {
        return YES;
    }
    OCFSetCallBacks setCallbacks = {0};
    OCFMutableSetRef visited = OCFSetCreateMutable(kOCFAllocatorDefault, 32, &setCallbacks);
    OCFArrayCallBacks arrayCallbacks = {0};
    OCFMutableArrayRef work = OCFArrayCreateMutable(kOCFAllocatorDefault, 32, &arrayCallbacks);
    OCFArrayAppendValue(work, instance);
    BOOL valid = YES;
    OCFIndex count;
    while ((count = OCFArrayGetCount(work)) != 0) {
        OCFIndex insertion = count - 1;
        id object = (id)OCFArrayGetValueAtIndex(work, insertion);
        OCFArrayRemoveValueAtIndex(work, insertion);
        if (OCFSetContainsValue(visited, object)) {
            continue;
        }
        OCFSetAddValue(visited, object);
        Class cls = [object classForCoder];
        if (![(id)self _validateAllowedClassesContainsClass:cls forKey:key]) {
            valid = NO;
            if (invalidElementClass != NULL) {
                *invalidElementClass = cls;
            }
            break;
        }
        if (!_ONSIsNSArray(object) && !_ONSIsNSSet(object) && !_ONSIsNSOrderedSet(object) &&
            !_ONSIsNSDictionary(object)) {
            continue;
        }
        BOOL dictionary = _ONSIsNSDictionary(object);
        ArchivingEnumerationState state = {0};
        id buffer[128];
        NSUInteger batch;
        while ((batch = [object countByEnumeratingWithState:&state objects:buffer count:128]) != 0) {
            OCFArrayReplaceValues(work, (OCFRange){insertion, 0}, (const void **)state.itemsPtr, batch);
            insertion += batch;
            if (dictionary) {
                for (NSUInteger index = 0; index != batch; ++index) {
                    buffer[index] = [object objectForKey:state.itemsPtr[index]];
                }
                OCFArrayReplaceValues(work, (OCFRange){insertion, 0}, (const void **)buffer, batch);
                insertion += batch;
            }
        }
    }
    OCFRelease(visited);
    OCFRelease(work);
    return valid;
}

// Native -[NSCoder __descriptionForUnexpectedClass:forKey:allowedClasses:], 0x1c04dd0d0. Private external;
// objc_direct. The class description and allowed-class descriptions are evaluated in that order.
- (id)__descriptionForUnexpectedClass:(Class)cls forKey:(id)key allowedClasses:(id)classes {
    id actualKey = key != nil ? key : (id)OF_STR("(no key, possibly an argument to a message)");
    id bundle = [(id)archivingBundleClass bundleForClass:cls];
    id path = [[bundle bundleURL] path];
    id description = [(id)archivingStringClass stringWithFormat:(id)OF_STR("'%@' (%p) [%@]"), cls, cls, path];
    id allowed = [self _allowedClassesDescriptionForClasses:classes];
    return [(id)archivingStringClass
        stringWithFormat:(id)OF_STR(
                             "value for key '%@' was of unexpected class %@.\nAllowed classes are:\n %@"
                         ),
                         actualKey,
                         description,
                         allowed];
}

// Native -[NSCoder _allowedClassesDescriptionForClasses:], 0x1c04dd184. Private external; objc_direct.
- (id)_allowedClassesDescriptionForClasses:(id)classes {
    id descriptions = objc_autorelease(objc_alloc_init((Class)archivingMutableSetClass));
    // Native ___48-[NSCoder _allowedClassesDescriptionForClasses:]_block_invoke, 0x1c04dd2f4. Non-external.
    [classes enumerateObjectsUsingBlock:^(id cls, BOOL *stop) {
      (void)stop;
      [descriptions addObject:[self __allowedClassesDescriptionForClass:cls]];
    }];
    return descriptions;
}

// Native -[NSCoder __descriptionForUnexpectedElementClass:forContainerKey:allowedClasses:], 0x1c04dd240.
// Private external; objc_direct.
- (id)__descriptionForUnexpectedElementClass:(Class)cls forContainerKey:(id)key allowedClasses:(id)classes {
    id actualKey = key != nil ? key : (id)OF_STR("(no key, possibly an argument to a message)");
    id bundle = [(id)archivingBundleClass bundleForClass:cls];
    id path = [[bundle bundleURL] path];
    id description = [(id)archivingStringClass stringWithFormat:(id)OF_STR("'%@' (%p) [%@]"), cls, cls, path];
    id allowed = [self _allowedClassesDescriptionForClasses:classes];
    return [(id)archivingStringClass
        stringWithFormat:(id)OF_STR(
                             "collection for key '%@' recursively contained an element of unexpected class "
                             "%@.\nAllowed classes are:\n %@"
                         ),
                         actualKey,
                         description,
                         allowed];
}

// Native -[NSCoder _warnAboutPlistType:forKey:missingInAllowedClasses:], 0x1c04dd31c. Private external;
// objc_direct. This native path uses _NSOSLog, not _NSRuntimeIssuesLog.
- (void)_warnAboutPlistType:(Class)type forKey:(id)key missingInAllowedClasses:(id)classes {
    os_log_t log = _NSOSLog();
    if (os_log_type_enabled(log, OS_LOG_TYPE_FAULT)) {
        id proem = _ONSMethodExceptionProem(self, sel_getUid("validateAllowedClass:forKey:"));
        id bundle = [(id)archivingBundleClass bundleForClass:type];
        id path = [[bundle bundleURL] path];
        id description =
            [(id)archivingStringClass stringWithFormat:(id)OF_STR("'%@' (%p) [%@]"), type, type, path];
        id allowed = [self _allowedClassesDescriptionForClasses:classes];
        id info = [self __decoderInfoForAllowedClassesWarning];
        uint8_t buffer[52];
#define PLIST_CLASS_WARNING                                                                                  \
    "%{public}@ allowed unarchiving safe plist type '%{public}@' for key '%{public}@', even though it was "  \
    "not "                                                                                                   \
    "explicitly included in the client allowed classes set: '%{public}@'. This will be disallowed in the "   \
    "future.%{public}@"
        __builtin_os_log_format(buffer, PLIST_CLASS_WARNING, proem, description, key, allowed, info);
        _os_log_fault_impl(
            &__dso_handle, log, OS_LOG_TYPE_FAULT, PLIST_CLASS_WARNING, buffer, sizeof(buffer)
        );
#undef PLIST_CLASS_WARNING
    }
}

// Native -[NSCoder __failWithUnexpectedClass:forKey:allowedClasses:], 0x1c04ddb30. Private external;
// objc_direct.
- (void)__failWithUnexpectedClass:(Class)cls forKey:(id)key allowedClasses:(id)classes {
    [self __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                        errorCode:4864
                           format:(id)OF_STR("%@"),
                                  [self __descriptionForUnexpectedClass:cls
                                                                 forKey:key
                                                         allowedClasses:classes]];
}

// Native -[NSCoder __failWithUnexpectedElementClass:forContainerKey:allowedClasses:], 0x1c04ddb84.
// Private external; objc_direct.
- (void)__failWithUnexpectedElementClass:(Class)cls forContainerKey:(id)key allowedClasses:(id)classes {
    [self __failWithExceptionName:(id)ONSInvalidUnarchiveOperationException
                        errorCode:4864
                           format:(id)OF_STR("%@"),
                                  [self __descriptionForUnexpectedElementClass:cls
                                                               forContainerKey:key
                                                                allowedClasses:classes]];
}

// Native -[NSCoder versionForClassName:], 0x1c0fe26fc. Non-external.
- (NSInteger)versionForClassName:(id)className {
    Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSCoder)));
    ONSRequestConcreteImplementation(self, _cmd, cls);
    return 0;
}

// Native -[NSCoder decodeObjectForKey:], 0x1c0fe2774. Non-external.
- (id)decodeObjectForKey:(id)key {
    Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSCoder)));
    ONSRequestConcreteImplementation(self, _cmd, cls);
    return nil;
}

// Native -[NSCoder containsValueForKey:], 0x1c0fe2974. Non-external.
- (BOOL)containsValueForKey:(id)key {
    Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSCoder)));
    ONSRequestConcreteImplementation(self, _cmd, cls);
    return NO;
}

// Native -[NSCoder decodeBoolForKey:], 0x1c0fe29b4. Non-external.
- (BOOL)decodeBoolForKey:(id)key {
    Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSCoder)));
    ONSRequestConcreteImplementation(self, _cmd, cls);
    return NO;
}

// Native -[NSCoder decodeInt64ForKey:], 0x1c0fe2a74. Non-external.
- (int64_t)decodeInt64ForKey:(id)key {
    Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSCoder)));
    ONSRequestConcreteImplementation(self, _cmd, cls);
    return 0;
}

// Native -[NSCoder decodeDoubleForKey:], 0x1c0fe2af4. Non-external.
- (double)decodeDoubleForKey:(id)key {
    Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSCoder)));
    ONSRequestConcreteImplementation(self, _cmd, cls);
    return 0.0;
}

// Native -[NSCoder decodeBytesForKey:returnedLength:], 0x1c0fe2b34. Non-external.
- (const uint8_t *)decodeBytesForKey:(id)key returnedLength:(NSUInteger *)lengthp {
    Class cls = ONSClassFromString((id)OF_STR(OF_NAME_STRING(ONSCoder)));
    ONSRequestConcreteImplementation(self, _cmd, cls);
    return NULL;
}

@end

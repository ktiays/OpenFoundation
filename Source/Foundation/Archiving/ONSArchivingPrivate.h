//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/String/OCFString.h"
#import "Foundation/Archiving/ONSCoder.h"
#import "Foundation/Archiving/ONSUnrestoredInterfaces.h"

#include <dispatch/dispatch.h>
#include <mach-o/loader.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <os/lock.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Native shared state, Foundation 24A434 UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified
// offsets and widths. iOS Device 24A446: unread. Reading: Source/Foundation/Archiving/Reading.md.
OF_EXTERN uint8_t _warnArchiverCompat;
OF_EXTERN const OCFStringRef generic_keys[40];

// Shared with the six undefined initializer blocks declared in ONSUnrestoredInterfaces.h. Keep these names
// and declarations available for their restoration; these variables are not replacement implementations.
OF_EXTERN unsigned int dword_22E76C144;
OF_EXTERN id qword_22E76C160;
OF_EXTERN OCFMutableDictionaryRef qword_22E76C170;
OF_EXTERN IMP keyedUnarchiverBaseImplementations[3]; // __MergedGlobals.43 + 0x20, 0x28, 0x30.
OF_EXTERN os_log_t qword_22E76C930;

// Block literal and descriptor read in IDA at 0x2227d0790 and 0x2227d03f8. The initializer blocks have no
// captures, flags 0x50000000 (global and signature), and literal size 32. These descriptive types encode
// static native data only; block invokes remain native function boundaries.
typedef struct {
    uintptr_t reserved;
    uintptr_t size;
    const char *signature;
} ArchivingBlockDescriptor;

typedef struct {
    void *isa;
    int32_t flags;
    int32_t reserved;
    void (*invoke)(const void *block);
    const ArchivingBlockDescriptor *descriptor;
} ArchivingGlobalBlock;

extern void *_NSConcreteGlobalBlock[];
extern struct mach_header __dso_handle;
extern id objc_retain(id object);
extern void objc_release(id object);
extern id objc_autorelease(id object);
extern id objc_allocWithZone(Class cls);
extern void *objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void *pool);
extern BOOL objc_opt_respondsToSelector(id object, SEL selector);
extern Class objc_opt_class(id object);
extern BOOL objc_opt_isKindOfClass(id object, Class cls);
extern bool _os_feature_enabled_impl(const char *domain, const char *feature);
extern bool dyld_program_sdk_at_least(OCFBuildVersion version);
extern int sandbox_check(pid_t pid, const char *operation, int filter, ...);
extern const int SANDBOX_CHECK_NO_REPORT;

// NSFastEnumerationState's 64-byte ABI as the native methods use it. A local spelling, not an exported type.
typedef struct {
    unsigned long state;
    id *itemsPtr;
    unsigned long *mutationsPtr;
    unsigned long extra[5];
} ArchivingEnumerationState;

// Verified _NSKeyedUnarchiverHelper ivar offsets from class_ro_t 0x222857400. This overlay does not define
// the foreign class or choose its allocation size (native instanceSize 42; runtime allocation rounds up).
typedef struct {
    Class isa;
    id allowed;
    uint32_t lastRef;
    id allowedClasses;
    id decodeError;
    bool expectsUnnestedCollection;
    bool decodedCollection;
} KeyedUnarchiverHelperFields;

_Static_assert(offsetof(KeyedUnarchiverHelperFields, lastRef) == 16, "The last reference is at offset 16.");
_Static_assert(
    offsetof(KeyedUnarchiverHelperFields, allowedClasses) == 24, "Allowed classes are at offset 24."
);
_Static_assert(offsetof(KeyedUnarchiverHelperFields, decodeError) == 32, "The decode error is at offset 32.");
_Static_assert(
    offsetof(KeyedUnarchiverHelperFields, expectsUnnestedCollection) == 40, "Collection flags start at 40."
);
_Static_assert(
    offsetof(KeyedUnarchiverHelperFields, decodedCollection) == 41, "The second collection flag is at 41."
);

// The existing owner headers do not yet declare these messages on every receiver's static type. This is a
// declaration of the observed message boundary; it provides no behavior on NSObject.
@interface NSObject (ArchivingMessageInterfaces)
+ (id)dictionaryWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count;
- (void)addObject:(id)object;
- (void)removeObjectAtIndex:(NSUInteger)index;
- (void)removeLastObject;
- (id)lastObject;
- (id)objectForKey:(id)key;
- (id)setByAddingObject:(id)object;
+ (id)setWithObject:(id)object;
- (id)initWithObjects:(id)firstObject, ...;
- (BOOL)_validateAllowedClassesContainsClass:(Class)cls forKey:(id)key;
- (id)decodeObjectOfClass:(Class)cls forKey:(id)key;
- (id)decodeObjectOfClasses:(id)classes forKey:(id)key;
- (id)classForCoder;
- (id)classForKeyedArchiver;
+ (Class)classForKeyedUnarchiver;
+ (id)classFallbacksForKeyedArchiver;
+ (BOOL)_subclassesMustBeExplicitlyMentionedWhenDecoded;
- (id)replacementObjectForKeyedArchiver:(id)archiver;
- (void)encodeWithCoder:(id)coder;
- (id)initWithCoder:(id)coder;
- (id)awakeAfterUsingCoder:(id)coder;
- (void)archiverWillFinish:(id)archiver;
- (void)archiverDidFinish:(id)archiver;
- (void)archiver:(id)archiver willReplaceObject:(id)object withObject:(id)replacement;
- (id)archiver:(id)archiver willEncodeObject:(id)object;
- (void)archiver:(id)archiver didEncodeObject:(id)object;
- (void)unarchiverWillFinish:(id)unarchiver;
- (void)unarchiverDidFinish:(id)unarchiver;
- (void)unarchiver:(id)unarchiver willReplaceObject:(id)object withObject:(id)replacement;
- (id)unarchiver:(id)unarchiver didDecodeObject:(id)object;
- (Class)unarchiver:(id)unarchiver cannotDecodeObjectOfClassName:(id)name originalClasses:(id)classes;
+ (void)_swift_checkClassAndWarnForKeyedArchiving:(Class)cls operation:(int)operation;
- (id)initWithContentsOfFile:(id)path;
- (id)path;
+ (NSInteger)version;
- (id)stringByAppendingString:(id)string;
- (id)substringFromIndex:(NSUInteger)index;
- (id)mutableCopy;
- (void)appendString:(id)string;
- (void)enumerateObjectsUsingBlock:(void (^)(id object, BOOL *stop))block;
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)count;
+ (id)data;
+ (id)dataWithBytes:(const void *)bytes length:(NSUInteger)length;
- (const void *)bytes;
- (void *)mutableBytes;
- (NSUInteger)length;
- (void)getBytes:(void *)bytes range:(OCFRange)range;
@end

// Direct class-object bindings preserve the native distinction between loading a class reference and
// sending +class (which user subclasses may override).
extern char archivingStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSString));
extern char archivingMutableStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableString));
extern char archivingNumberClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSNumber));
extern char archivingExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char archivingErrorClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSError));
extern char archivingBundleClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSBundle));
extern char archivingProcessInfoClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSProcessInfo));
extern char archivingArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSArray));
extern char archivingMutableArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableArray));
extern char archivingMutableSetClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableSet));
extern char archivingSetClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSSet));
extern char archivingDictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDictionary));
extern char archivingMutableDictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableDictionary));
extern char archivingDataClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSData));
extern char archivingMutableDataClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableData));
extern char archivingObjectClass[] __asm("_OBJC_CLASS_$_NSObject");
extern char archivingArchiverClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSKeyedArchiver));
extern char archivingUnarchiverClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSKeyedUnarchiver));
extern char archivingHelperClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(_ONSKeyedUnarchiverHelper));
extern char archivingPerformanceMeterClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(_ONSPerformanceMeter));
extern char archivingOldStyleArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(_ONSKeyedCoderOldStyleArray));
@class __ONSLocalTimeZone;
@class __ONSLocalTimeZoneI;

extern id objc_alloc(Class cls);
extern id objc_alloc_init(Class cls);
extern id objc_opt_new(id cls);

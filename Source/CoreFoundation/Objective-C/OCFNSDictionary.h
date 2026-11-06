//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"
#include "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#include "CoreFoundation/Objective-C/OCFRequireConcreteImplementation.h"
#include "CoreFoundation/Objective-C/OCFNSCollectionsShouldCopy.h"

#include <alloca.h>
#include <mach-o/loader.h>
#include <objc/NSObjCRuntime.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Shared declarations of the dictionary classes that live in CoreFoundation (NSDictionary,
// NSMutableDictionary, NSConstantDictionary, __NSDictionary0, __NSDictionaryI, __NSDictionaryM,
// __NSSingleEntryDictionaryI) and of their C helpers, iOS Simulator 24A434, arm64, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC. Each class has its own translation unit under Objective-C/; see each
// unit's header comment for its organization. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet on this branch).

// The ThreadSanitizer check every dictionary method inlines: load the hook, and when it is set, call it with
// the object, the method's return address (X30 natively) and __CFTSANTagMutableDictionary. The same check as
// tsanRead and tsanWrite of Objective-C/OCFBridgedClasses.h, which this header cannot include while that
// header still declares the placeholder ONSDictionary interfaces; dictionaryTSANRead and dictionaryTSANWrite
// are local descriptive names.
static inline void dictionaryTSANRead(const void *object, void *returnAddress) {
    void (*hook)(const void *, void *, void *) =
        (void (*)(const void *, void *, void *))__cf_tsanReadFunction;
    if (hook != NULL) {
        hook(object, returnAddress, __OCFTSANTagMutableDictionary);
    }
}

static inline void dictionaryTSANWrite(const void *object, void *returnAddress) {
    void (*hook)(const void *, void *, void *) =
        (void (*)(const void *, void *, void *))__cf_tsanWriteFunction;
    if (hook != NULL) {
        hook(object, returnAddress, __OCFTSANTagMutableDictionary);
    }
}

// The layout of Foundation's NSFastEnumerationState (Foundation's type; the methods spell the state as
// void *, as the bridged classes do, and cast to this layout). DictionaryEnumerationState is a local
// descriptive name.
typedef struct {
    unsigned long state;
    void **itemsPtr;
    unsigned long *mutationsPtr;
    unsigned long extra[5];
} DictionaryEnumerationState;

// Native __CFExceptionProem (was a private external), also declared by Objective-C/OCFBridgedClasses.h; see
// the note on dictionaryTSANRead.

// libsystem_trace's os_log pack interfaces (private SPI without SDK declarations); supplied, never
// reimplemented.
extern size_t _os_log_pack_size(size_t formatBufferSize);
extern uint8_t *
_os_log_pack_fill(void *pack, size_t size, int savedErrno, const void *dso, const char *format);
extern struct mach_header __dso_handle;

// The NSException class symbol (_OBJC_CLASS_$_NSException), sent to directly.
extern char dictionaryExceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));

// The raise the dictionary methods inline for an invalid argument: an os_log pack of the format buffer size
// on the stack (_os_log_pack_size, the size rounded up to 16 bytes, _os_log_pack_fill with errno 0 and the
// image's header), the arguments encoded by the compiler (__builtin_os_log_format), the reason formatted with
// kCFAllocatorSystemDefault, then +[NSException exceptionWithName:reason:userInfo:osLogPack:size:] with the
// reason autoreleased through _CFAutoreleasePoolAddObject and no user info, thrown. The arguments are
// evaluated twice, as the native code reloads them; pass only values without side effects.
#define DICTIONARY_RAISE_WITH_LOG_PACK(exceptionName, bufferSize, format, ...)                               \
    do {                                                                                                     \
        size_t packSize = _os_log_pack_size(bufferSize);                                                     \
        void *pack = alloca((packSize + 15) & ~(size_t)15);                                                  \
        uint8_t *packBuffer = _os_log_pack_fill(pack, packSize, 0, &__dso_handle, format);                   \
        __builtin_os_log_format(packBuffer, format, __VA_ARGS__);                                            \
        OCFStringRef packReason =                                                                            \
            OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR(format), __VA_ARGS__);        \
        id packException = ((id (*)(id, SEL, id, id, id, void *, size_t))objc_msgSend)(                      \
            (id)dictionaryExceptionClass,                                                                    \
            sel_getUid("exceptionWithName:reason:userInfo:osLogPack:size:"),                                 \
            (id)(exceptionName),                                                                             \
            _OCFAutoreleasePoolAddObject(NULL, packReason),                                                  \
            nil,                                                                                             \
            pack,                                                                                            \
            packSize                                                                                         \
        );                                                                                                   \
        objc_exception_throw(packException);                                                                 \
    } while (0)

// Interfaces of families not yet restored on this branch
// Native _NSIsNSArray (0x1c029e0b4), _NSIsNSData (0x1c029e128), _NSIsNSDictionary (0x1c029e274) and
// _NSIsNSString (0x1c029e544), external, in the Objective-C runtime glue beside _NSIsNSNumber
// (Objective-C/OCFNSObject.m).
// Native __CFRequireConcreteImplementation, 0x1c030fa8c (was a private external): raises for a method an
// abstract class leaves to its concrete subclasses; the abstract class (looked up by name through
// __CFLookUpClass), the receiver and the selector.
// Native __NSCollectionsShouldCopy, 0x1c0287374 (was a private external): whether -copyWithZone: of the
// collection copies its items; the collection is the only argument the callers pass (confidence: medium for
// the parameter list).

#ifdef __OBJC__
#import <objc/NSObject.h>

// Classes outside this family that the dictionary methods reference by name (class symbols and runtime
// names), so that the prefix tool maps them.
@class ONSException;
@class ONSSet;
@class __ONSPlaceholderDictionary;
@class __ONSFrozenDictionaryM;
@class __ONSDictionaryObjectEnumerator;
@class __ONSConstantDictionaryEnumerator;
@class __ONSEnumerator0;
@class __ONSSingleObjectEnumerator;
@class __ONSFastEnumerationEnumerator;
@class ONSSharedKeySet;
@class ONSSharedKeyDictionary;

// Selectors the dictionary methods send to objects of classes outside this family (or to their own objects
// through id), declared for the message sends only; no class adopts the protocol and no protocol object is
// emitted. Foundation's NSString,
// NSArray, NSSet and NSEnumerator parameter and result types are spelled id; NSRange as OCFRange.
@protocol DictionaryClassMessages

- (id)copyWithZone:(struct _NSZone *)zone;
- (id)mutableCopyWithZone:(struct _NSZone *)zone;
- (NSUInteger)count;
- (id)objectForKey:(id)key;
- (id)keyEnumerator;
- (id)objectEnumerator;
- (id)nextObject;
- (void)getObjects:(id *)objects andKeys:(id *)keys count:(NSUInteger)count;
- (void)getObjects:(id *)objects range:(OCFRange)range;
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)length;
- (void)setObject:(id)object forKey:(id)key;
- (void)removeObjectForKey:(id)key;
- (void)removeAllObjects;
- (id)initWithObject:(id)object;
- (id)initWithObjects:(const id *)objects count:(NSUInteger)count;
- (id)initWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count;
- (id)initWithCapacity:(NSUInteger)capacity;
- (id)initWithDictionary:(id)dictionary copyItems:(BOOL)flag;
- (NSUInteger)length;
- (BOOL)isEqualToString:(id)string;
- (BOOL)isEqualToNumber:(id)number;
- (id)objectAtIndex:(NSUInteger)index;
- (void)addObject:(id)object;
- (id)descriptionWithLocale:(id)locale indent:(NSUInteger)level;
- (void)willChangeValueForKey:(id)key;
- (void)didChangeValueForKey:(id)key;
- (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;

@end

// Native class NSDictionary (__DATA,__objc_data 0x22e6cde50, external), an NSObject subclass; class_ro_t
// 0x2227c3c98: instance start and size 8, no ivars (iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified). The superclass pointer 0x22e6b6088 lies
// outside the extracted image (libobjc's NSObject, as every root-level CoreFoundation class). Its methods are
// defined by Objective-C/OCFNSDictionary.m.
__attribute__((visibility("default")))
@interface ONSDictionary : NSObject
@end
#endif

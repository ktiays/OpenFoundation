//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSPlaceholderDictionary.h"

// Inferred organization: the __NSPlaceholderDictionary methods (0x1c02f2f6c-0x1c02f3d3f) lie together in the
// iOS Simulator __text section, with __NSDictionaryImmutablePlaceholder and __NSDictionaryMutablePlaceholder
// (0x1c02f337c, 0x1c02f3388) among them and the __NSCFDictionaryCreate* helpers after them; its trapping
// +allocWithZone: (0x1c03171b8) and -dealloc (0x1c03171bc) lie in the cold region. The C helpers and the two
// static placeholder instances were restored earlier in Collections/OCFNSCFDictionary.c, which keeps them;
// following the owner's decision the class's methods have this translation unit, which reaches the instances
// through their hidden native names. The unit's static _initWithContentsOfURL:.__cls_NSURL (one of the two
// __DATA,__bss words of that name, 0x22e6d26e8 or 0x22e6d3db8; which one belongs to this method is unread)
// is the function-local static of -initWithContentsOfURL:. Objective-C compiled with manual reference
// counting (objc_retain). Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json and
// iphone-simulator-corpus-delta11.json (family L-collection-leftovers: -retain, listed by instructions).
// Unless a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// libobjc's objc_retain, which the native image imports; the SDK installs no declaration for it.
extern id objc_retain(id object);

// The NSDictionary class symbol (class reference off_2227CE2F8), sent +newWithContentsOf:immutable:.
extern char placeholderDictionaryNSDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                               OF_NAME_STRING(ONSDictionary));

// The __NSDictionaryI, __NSFrozenDictionaryM and __NSDictionaryM class symbols, loaded natively through their
// GOT slots (___SCR___NSDictionaryI, ___SCR___NSFrozenDictionaryM, ___SCR___NSDictionaryM) and compared with
// the class of the argument of -initWithDictionary:copyItems:.
extern char placeholderDictionaryIClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSDictionaryI));
extern char placeholderFrozenDictionaryMClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                          OF_NAME_STRING(__ONSFrozenDictionaryM));
extern char placeholderDictionaryMClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSDictionaryM));

// Selectors this unit sends that no included header declares, for the message sends only.
@protocol PlaceholderDictionaryMessages

+ (id)newWithContentsOf:(id)location immutable:(BOOL)immutable;

@end

// The superclass method -initWithDictionary:copyItems: sends to super (NSDictionary's, defined by
// Objective-C/OCFNSDictionary.m, which declares no interface for it); declared only.
@interface ONSMutableDictionary (PlaceholderDictionarySuperclassMessages)

- (id)initWithDictionary:(id)dictionary copyItems:(BOOL)flag;

@end

// The raise every primitive method of the placeholder inlines: the class looked up by name (__CFLookUpClass
// of "NSDictionary" for the immutable placeholder, "NSMutableDictionary" for the mutable one, "<unknown>"
// otherwise), its __CFFullMethodName with the receiver and selector, and the kind ("immutable", "mutable" or
// "<unknown>") form the reason "*** %@: method sent to an uninitialized %s dictionary object" (made with
// kCFAllocatorSystemDefault and autoreleased through _CFAutoreleasePoolAddObject);
// +[NSException exceptionWithName:reason:userInfo:] with NSInvalidArgumentException and no user info is
// thrown. raiseUninitializedPlaceholder is a local descriptive name for the inlined code.
static inline __attribute__((always_inline, noreturn)) void raiseUninitializedPlaceholder(id self, SEL _cmd) {
    const char *kind = self == (id)&__mutablePlaceholderDictionary ? "mutable" : "<unknown>";
    const char *className =
        self == (id)&__mutablePlaceholderDictionary ? OF_NAME_STRING(ONSMutableDictionary) : "<unknown>";
    if (self == (id)&__immutablePlaceholderDictionary) {
        kind = "immutable";
        className = OF_NAME_STRING(ONSDictionary);
    }
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFStringRef reason = OCFStringCreateWithFormat(
        allocator,
        NULL,
        OF_STR("*** %@: method sent to an uninitialized %s dictionary object"),
        __CFFullMethodName((Class)__CFLookUpClass(className), self, _cmd),
        kind
    );
    objc_exception_throw([(id)dictionaryExceptionClass
        exceptionWithName:(id)ONSInvalidArgumentException
                   reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                 userInfo:nil]);
}

@implementation __ONSPlaceholderDictionary

// Native -[__NSPlaceholderDictionary count], 0x1c02f2f6c. Non-external. Raises
// (raiseUninitializedPlaceholder).
- (NSUInteger)count {
    raiseUninitializedPlaceholder(self, _cmd);
}

// Native -[__NSPlaceholderDictionary objectForKey:], 0x1c02f303c. Non-external. The body of -count.
- (id)objectForKey:(id)key {
    raiseUninitializedPlaceholder(self, _cmd);
}

// Native -[__NSPlaceholderDictionary keyEnumerator], 0x1c02f310c. Non-external. The body of -count.
- (id)keyEnumerator {
    raiseUninitializedPlaceholder(self, _cmd);
}

// Native -[__NSPlaceholderDictionary setObject:forKey:], 0x1c02f31dc. Non-external. The body of -count.
- (void)setObject:(id)object forKey:(id)key {
    raiseUninitializedPlaceholder(self, _cmd);
}

// Native -[__NSPlaceholderDictionary removeObjectForKey:], 0x1c02f32ac. Non-external. The body of -count.
- (void)removeObjectForKey:(id)key {
    raiseUninitializedPlaceholder(self, _cmd);
}

// Native -[__NSPlaceholderDictionary initWithObjects:forKeys:count:], 0x1c02f3394. Non-external. A NULL keys
// array with a nonzero count raises "pointer to objects array is NULL but length is %lu", a count with any of
// its top three bits set "count (%lu) of objects array is ridiculous"; then (with a nonzero count) a nil key
// raises "attempt to insert nil object from objects[%lu]" (the key's index; the native message names the
// objects array for the keys as well), a NULL objects array raises the NULL-pointer message and a nil object
// the nil message with its index (os_log pack buffer 0x16 each: the method name and the number). The
// immutable placeholder answers the empty dictionary __NSDictionary0__ retained (count 0),
// __NSSingleEntryDictionaryI_new(key, object, 1) (count 1) or __NSDictionaryI_new(keys, objects, NULL, count,
// 1); the mutable placeholder __NSDictionaryM_new(keys, objects, count, 3) (all tail calls); any other
// receiver traps (BRK #1).
- (id)initWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count {
    if (keys == NULL && count != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: pointer to objects array is NULL but length is %lu",
            "-[" OF_NAME_STRING(__ONSPlaceholderDictionary) " initWithObjects:forKeys:count:]",
            count
        );
    }
    if ((OF_WORD_SHIFT(count, 61)) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: count (%lu) of objects array is ridiculous",
            "-[" OF_NAME_STRING(__ONSPlaceholderDictionary) " initWithObjects:forKeys:count:]",
            count
        );
    }
    if (count != 0) {
        for (NSUInteger index = 0; index != count; index++) {
            if (keys[index] == nil) {
                DICTIONARY_RAISE_WITH_LOG_PACK(
                    ONSInvalidArgumentException,
                    0x16,
                    "*** %s: attempt to insert nil object from objects[%lu]",
                    "-[" OF_NAME_STRING(__ONSPlaceholderDictionary) " initWithObjects:forKeys:count:]",
                    index
                );
            }
        }
        if (objects == NULL) {
            DICTIONARY_RAISE_WITH_LOG_PACK(
                ONSInvalidArgumentException,
                0x16,
                "*** %s: pointer to objects array is NULL but length is %lu",
                "-[" OF_NAME_STRING(__ONSPlaceholderDictionary) " initWithObjects:forKeys:count:]",
                count
            );
        }
        for (NSUInteger index = 0; index != count; index++) {
            if (objects[index] == nil) {
                DICTIONARY_RAISE_WITH_LOG_PACK(
                    ONSInvalidArgumentException,
                    0x16,
                    "*** %s: attempt to insert nil object from objects[%lu]",
                    "-[" OF_NAME_STRING(__ONSPlaceholderDictionary) " initWithObjects:forKeys:count:]",
                    index
                );
            }
        }
    }
    if (self == (id)&__immutablePlaceholderDictionary) {
        if (count == 1) {
            return (id)__NSSingleEntryDictionaryI_new(keys[0], objects[0], 1);
        }
        if (count == 0) {
            return objc_retain((id)__ONSDictionary0__);
        }
        return (id)__NSDictionaryI_new((const void **)keys, (const void **)objects, NULL, (OCFIndex)count, 1);
    }
    if (self != (id)&__mutablePlaceholderDictionary) {
        __builtin_trap();
    }
    return (id)__NSDictionaryM_new((const void **)keys, (const void **)objects, (OCFIndex)count, 3);
}

// Native -[__NSPlaceholderDictionary initWithDictionary:copyItems:], 0x1c02f3778. Non-external. Without
// copied items, a dictionary whose class (objc_opt_class) is __NSDictionaryI, __NSDictionaryM or
// __NSFrozenDictionaryM is answered as its -mutableCopyWithZone: NULL by the mutable placeholder and its
// -copyWithZone: NULL by the immutable one (tail calls); everything else goes to super.
- (id)initWithDictionary:(id)dictionary copyItems:(BOOL)flag {
    if (!flag) {
        Class dictionaryClass = [dictionary class];
        if (dictionaryClass == (Class)placeholderDictionaryIClass ||
            dictionaryClass == (Class)placeholderDictionaryMClass ||
            dictionaryClass == (Class)placeholderFrozenDictionaryMClass) {
            if (self == (id)&__mutablePlaceholderDictionary) {
                return [(id<DictionaryClassMessages>)dictionary mutableCopyWithZone:NULL];
            }
            if (self == (id)&__immutablePlaceholderDictionary) {
                return [(id<DictionaryClassMessages>)dictionary copyWithZone:NULL];
            }
        }
    }
    return [super initWithDictionary:dictionary copyItems:flag];
}

// Native -[__NSPlaceholderDictionary initWithCapacity:], 0x1c02f38c8. Non-external. A capacity with any of
// its top three bits set raises "capacity (%lu) is ridiculous" (os_log pack buffer 0x16). The mutable
// placeholder answers __NSDictionaryM_new(NULL, NULL, capacity, 3) (a tail call); the immutable placeholder
// sends itself -doesNotRecognizeSelector: with the selector, and that or any other receiver traps (BRK #1).
- (id)initWithCapacity:(NSUInteger)capacity {
    if ((OF_WORD_SHIFT(capacity, 61)) != 0) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0x16,
            "*** %s: capacity (%lu) is ridiculous",
            "-[" OF_NAME_STRING(__ONSPlaceholderDictionary) " initWithCapacity:]",
            capacity
        );
    }
    if (self == (id)&__immutablePlaceholderDictionary) {
        [(id)&__immutablePlaceholderDictionary doesNotRecognizeSelector:_cmd];
        __builtin_trap();
    }
    if (self != (id)&__mutablePlaceholderDictionary) {
        __builtin_trap();
    }
    return (id)__NSDictionaryM_new(NULL, NULL, (OCFIndex)capacity, 3);
}

// Native -[__NSPlaceholderDictionary init], 0x1c02f3a30. Non-external. The immutable placeholder sends itself
// -initWithObjects:forKeys:count: (NULL, NULL, 0), the mutable one -initWithCapacity: 0 (tail calls); any
// other receiver traps (BRK #1). The designated initializers are reached through the placeholder object,
// which the diagnostic cannot see.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-designated-initializers"

- (id)init {
    if (self == (id)&__immutablePlaceholderDictionary) {
        return [(id)&__immutablePlaceholderDictionary initWithObjects:NULL forKeys:NULL count:0];
    }
    if (self != (id)&__mutablePlaceholderDictionary) {
        __builtin_trap();
    }
    return [(id)&__mutablePlaceholderDictionary initWithCapacity:0];
}

#pragma clang diagnostic pop

// Native -[__NSPlaceholderDictionary initWithContentsOfFile:], 0x1c02f3a7c. Non-external. A path that is not
// nil and not an NSString (_NSIsNSString, bit 0) raises "string argument is not an NSString" (os_log pack
// buffer 0xC); otherwise +[NSDictionary newWithContentsOf:immutable:] with the path and whether the receiver
// is the immutable placeholder (a tail call).
- (id)initWithContentsOfFile:(id)path {
    if (path != nil && !_ONSIsNSString(path)) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: string argument is not an NSString",
            "-[" OF_NAME_STRING(__ONSPlaceholderDictionary) " initWithContentsOfFile:]"
        );
    }
    return (id)[(id<PlaceholderDictionaryMessages>)(id)placeholderDictionaryNSDictionaryClass
        newWithContentsOf:path
                immutable:self == (id)&__immutablePlaceholderDictionary];
}

// Native -[__NSPlaceholderDictionary initWithContentsOfURL:], 0x1c02f3bc0. Non-external. The NSURL class is
// looked up once (objc_lookUpClass("NSURL"), cached in the static __cls_NSURL with release semantics and read
// with acquire; a lookup answering NULL is repeated on the next call). A URL that is not nil and not
// -isKindOfClass: NSURL (objc_opt_isKindOfClass) raises "url argument is not an NSURL" (os_log pack buffer
// 0xC); otherwise +[NSDictionary newWithContentsOf:immutable:] as -initWithContentsOfFile: (a tail call).
- (id)initWithContentsOfURL:(id)url {
    static Class __cls_NSURL;
    if (__atomic_load_n((void **)&__cls_NSURL, __ATOMIC_ACQUIRE) == NULL) {
        __atomic_store_n(
            (void **)&__cls_NSURL, (void *)objc_lookUpClass(OF_NAME_STRING(ONSURL)), __ATOMIC_RELEASE
        );
    }
    if (url != nil && ![url isKindOfClass:(Class)__atomic_load_n((void **)&__cls_NSURL, __ATOMIC_ACQUIRE)]) {
        DICTIONARY_RAISE_WITH_LOG_PACK(
            ONSInvalidArgumentException,
            0xC,
            "*** %s: url argument is not an NSURL",
            "-[" OF_NAME_STRING(__ONSPlaceholderDictionary) " initWithContentsOfURL:]"
        );
    }
    return (id)[(id<PlaceholderDictionaryMessages>)(id)placeholderDictionaryNSDictionaryClass
        newWithContentsOf:url
                immutable:self == (id)&__immutablePlaceholderDictionary];
}

// Native -[__NSPlaceholderDictionary retain], 0x1c02f3d30. Non-external. The receiver (a bare RET; the
// capture lists its one instruction, IDA has no function there).
- (instancetype)retain {
    return self;
}

// Native -[__NSPlaceholderDictionary release], 0x1c02f3d34. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[__NSPlaceholderDictionary retainCount], 0x1c02f3d38. Non-external.
- (NSUInteger)retainCount {
    return UINTPTR_MAX;
}

// Native +[__NSPlaceholderDictionary allocWithZone:], 0x1c03171b8 (cold region). Non-external. A bare BRK #1.
+ (id)allocWithZone:(struct _NSZone *)zone {
    __builtin_trap();
}

// Native -[__NSPlaceholderDictionary dealloc], 0x1c03171bc (cold region). Non-external. A bare BRK #1; super
// is never reached, so the missing-super-call diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"

- (void)dealloc {
    __builtin_trap();
}

#pragma clang diagnostic pop

@end

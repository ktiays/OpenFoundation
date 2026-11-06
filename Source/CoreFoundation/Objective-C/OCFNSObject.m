//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Objective-C/OCFBridgedClasses.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <dlfcn.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// Inferred organization: __CFZombifyNSObject (0x1c029d448), __CFOAInitializeNSObject (0x1c029d4a4),
// +[NSObject(NSObject) load] (0x1c029d5d8), couldNotInstantiate (0x1c029d618), _CFAutoreleasePoolAddObject
// (0x1c029d7e4), the autorelease-pool wrappers (0x1c029dca0, 0x1c029dcc8), _NSIsNSNumber (0x1c029e3d0),
// _NSIsNSCFConstantString
// (0x1c029e780), getAtomTarget (0x1c029e944), __CFExceptionProem (0x1c029ec78), __CFAllocateObject
// (0x1c029eca4) and __CFLookUpClass (0x1c029ed14) lie together in the iOS Simulator __text section, among
// Objective-C methods; the unit is Objective-C, compiled with manual
// reference counting. The native source path is not in the evidence. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly); iOS Device
// 24A446: unread.
//
// Selector names: the native code passes selector string addresses of the shared cache (0x2387...). They are
// named here from the image's relative method lists (selector offsets from the base 0x2387a9f48, fixed by the
// 80 "dealloc" entries and consistent for every entry; confidence: verified) or, for selectors the image does
// not implement, from the objc_msgSend$ stubs that load the same address (confidence: verified) or by use
// (confidence: medium, said at the use).
//

// libobjc and dyld entry points the native unit imports whose declarations the SDK does not install
// (objc-internal.h, dyld_priv.h); supplied, never reimplemented.
extern id _objc_rootAutorelease(id object);
extern IMP object_getMethodImplementation(id object, SEL name);
extern Class objc_opt_class(id object);
extern Class objc_opt_self(id object);
extern BOOL objc_opt_isKindOfClass(id object, Class cls);
extern void _objc_msgForward(void);
extern bool dyld_program_sdk_at_least(OCFBuildVersion version);
extern uintptr_t objc_debug_taggedpointer_obfuscator;
extern const uint8_t objc_debug_tag60_permutations[8];

// The bridged exception class and the malloc exception name (ONSMallocException, external natively as
// NSMallocException).
extern char exceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));

// dyld_program_sdk_at_least's argument at both native uses: the version set 0x07E30901 (2019.9.1) on every
// platform (0xFFFFFFFF), MOV X0, #0x7E30901FFFFFFFF.
static const OCFBuildVersion kFall2019VersionSet = {0xFFFFFFFF, 0x07E30901};

static void *exceptionWithReason(OCFStringRef name, id reason) {
    return ((id (*)(id, SEL, OCFStringRef, id, id))objc_msgSend)(
        (id)exceptionClass, sel_getUid("exceptionWithName:reason:userInfo:"), name, reason, nil
    );
}

// Native __CFZombifyNSObject, 0x1c029d448. Hidden (was a private external). Exchanges NSObject's dealloc
// (0x2387a9f83) and __dealloc_zombie (0x2387ab447) implementations (a tail call).
void __CFZombifyNSObject(void) {
    Class cls = objc_lookUpClass("NSObject");
    Method dealloc = class_getInstanceMethod(cls, sel_getUid("dealloc"));
    Method zombie = class_getInstanceMethod(cls, sel_getUid("__dealloc_zombie"));
    method_exchangeImplementations(dealloc, zombie);
}

// Native __CFOAInitializeNSObject, 0x1c029d4a4. Hidden (was a private external). Exchanges NSObject's
// retain/__retain_OA, release/__release_OA, autorelease/__autorelease_OA and _tryRetain/___tryRetain_OA
// instance methods, then the metaclass's allocWithZone:/__allocWithZone_OA: (the last a tail call).
void __CFOAInitializeNSObject(void) {
    Class cls = objc_lookUpClass("NSObject");
    method_exchangeImplementations(
        class_getInstanceMethod(cls, sel_getUid("retain")),
        class_getInstanceMethod(cls, sel_getUid("__retain_OA"))
    );
    method_exchangeImplementations(
        class_getInstanceMethod(cls, sel_getUid("release")),
        class_getInstanceMethod(cls, sel_getUid("__release_OA"))
    );
    method_exchangeImplementations(
        class_getInstanceMethod(cls, sel_getUid("autorelease")),
        class_getInstanceMethod(cls, sel_getUid("__autorelease_OA"))
    );
    method_exchangeImplementations(
        class_getInstanceMethod(cls, sel_getUid("_tryRetain")),
        class_getInstanceMethod(cls, sel_getUid("___tryRetain_OA"))
    );
    Class metaclass = object_getClass((id)cls);
    method_exchangeImplementations(
        class_getInstanceMethod(metaclass, sel_getUid("allocWithZone:")),
        class_getInstanceMethod(metaclass, sel_getUid("__allocWithZone_OA:"))
    );
}

// Native couldNotInstantiate, 0x1c029d618. Non-external natively, the bad-alloc handler that +[NSObject
// load] installs; hidden here because only the system-name build compiles that method. Throws an
// NSMallocException whose reason is the autoreleased "*** -[NSObject allocWithZone:]: attempt to allocate
// object of class '%s' failed" (kCFAllocatorSystemDefault); the pool argument of _CFAutoreleasePoolAddObject
// is the string itself (X0 is left unchanged), which is never read.
__attribute__((noreturn)) id couldNotInstantiate(Class cls) {
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFStringRef string = OCFStringCreateWithFormat(
        allocator,
        NULL,
        OF_STR("*** -[NSObject allocWithZone:]: attempt to allocate object of class '%s' failed"),
        class_getName(cls)
    );
    id reason = _OCFAutoreleasePoolAddObject((void *)string, string);
    objc_exception_throw(exceptionWithReason(ONSMallocException, reason));
}

// Native +[NSObject(NSObject) load], 0x1c029d5d8, with .cold.1 (0x1c030fb44: dispatch_once of load.once and
// ___block_literal_global_35). Non-external. When libobjc's objc_debug_taggedpointer_mask is nonzero, the
// block runs once; then couldNotInstantiate becomes libobjc's bad-alloc handler. The method is a category on
// libobjc's shared root class, so, as for Foundation's +[NSObject(NSObject) load], only the isolated
// system-name build compiles it (coexistence must not alter the system class). Reading:
// Local/Evidence/cf-core/iphone-simulator-corefoundation-link3.json (round 3; iOS Device divergence: unread).
#if OF_SYSTEM_NAMES
extern uintptr_t objc_debug_taggedpointer_mask;
extern void _objc_registerTaggedPointerClass(unsigned int tag, Class cls);
extern void _objc_setBadAllocHandler(id (*handler)(Class));
extern bool dyld_program_sdk_at_least(OCFBuildVersion version);

@interface NSObject (NSObject)
@end

@implementation NSObject (NSObject)

+ (void)load {
    static dispatch_once_t once;
    if (objc_debug_taggedpointer_mask != 0) {
        // Native ___26+[NSObject(NSObject) load]_block_invoke, 0x1c030fa3c. Non-external. For programs built
        // with at least the fall 2019 SDK set (platform 0xffffffff, version 0x07e30901), the indirect tagged
        // pointer string class takes tag 0.
        dispatch_once(&once, ^{
          static const OCFBuildVersion fall2019 = {0xFFFFFFFF, 0x07E30901};
          if (dyld_program_sdk_at_least(fall2019)) {
              _objc_registerTaggedPointerClass(0, objc_lookUpClass("NSIndirectTaggedPointerString"));
          }
        });
    }
    _objc_setBadAllocHandler(couldNotInstantiate);
}

@end
#endif

// Native _CFAutoreleasePoolAddObject, 0x1c029d7e4. External. The pool argument is not read. With allocation
// statistics on (__CFOASafe), event 3 is recorded for the object; it is then autoreleased with
// _objc_rootAutorelease (a tail call). The NOP after the move has no effect.
id _OCFAutoreleasePoolAddObject(void *pool, OCFTypeRef object) {
    (void)pool;
    if (__OCFOASafe == 1) {
        __OCFRecordAllocationEvent(3, (void *)object, 0, 0, 0);
    }
    return _objc_rootAutorelease((id)object);
}

// libobjc entry points (re-exported by the native image); supplied, never reimplemented.
extern void *objc_autoreleasePoolPush(void);

extern void objc_autoreleasePoolPop(void *context);

// Native _CFAutoreleasePoolPush, iOS Simulator 24A434, 0x1c029dca0, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified. External. The NOP after the call has no effect.
// iOS Device 24A446: unread.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.
void *_OCFAutoreleasePoolPush(void) {
    return objc_autoreleasePoolPush();
}

// Native _CFAutoreleasePoolPop, iOS Simulator 24A434, 0x1c029dcc8, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified. External. Returns its argument (MOV X0, X19 after the call); the NOPs have no effect.
// iOS Device 24A446: unread.
// Reading: Documentation/Reference/Research/RuntimePrerequisites.md.
void *_OCFAutoreleasePoolPop(void *context) {
    objc_autoreleasePoolPop(context);
    return context;
}

// Native __NSNumberClass, 0x22e6d3818 (__DATA,__bss). Non-external; the NSNumber class, looked up on first
// use.
static Class _NSNumberClass;

// Native _NSIsNSNumber, 0x1c029e3d0 (Local/Evidence/cf-core/iphone-simulator-corpus-delta5.json). External.
// A tagged pointer answers whether its tag is 3 (the inline decoding: the obfuscator, read as a 32-bit word,
// is skipped for pointers with bits 63, 62 and 0-2 set; the low three bits are found in the seven-entry
// permutation table, objc_debug_tag60_permutations by use of the GOT slot 0x222409408; confidence: medium for
// the slot bindings). Otherwise NSNumber (OF_NAME_STRING(ONSNumber), the prefixed class in the prefixed
// build) is looked up with objc_getClass on first use (a failed lookup answers false and is retried on the
// next call); nil answers false. The object's isNSNumber__ implementation (0x2387b0337) is called (a tail
// call) unless it is the forwarding implementation (the GOT slot 0x222409138, _objc_msgForward by use;
// confidence: medium), in which case the answer is whether the object is a kind of NSNumber.
BOOL _ONSIsNSNumber(id object) {
    if ((intptr_t)object < 0) {
        uintptr_t pointer = (uintptr_t)object;
        uint32_t obfuscator = (pointer & 0xC000000000000007) == 0xC000000000000007
                                  ? 0
                                  : (uint32_t)objc_debug_taggedpointer_obfuscator;
        uintptr_t value = (obfuscator ^ (uint32_t)pointer) & 7;
        uintptr_t index = 0;
        while (value != objc_debug_tag60_permutations[index]) {
            index++;
            if (index == 7) {
                break;
            }
        }
        return (index & 7) == 3;
    }
    Class cls = _NSNumberClass;
    if (cls == Nil) {
        cls = objc_getClass(OF_NAME_STRING(ONSNumber));
        _NSNumberClass = cls;
        if (cls == Nil) {
            return NO;
        }
    }
    if (object == nil) {
        return NO;
    }
    SEL selector = sel_getUid("isNSNumber__");
    IMP implementation = object_getMethodImplementation(object, selector);
    if (implementation != (IMP)_objc_msgForward) {
        return ((BOOL (*)(id, SEL))implementation)(object, selector);
    }
    return objc_opt_isKindOfClass(object, cls) & 1;
}

// Native _NSIsNSCFConstantString, 0x1c029e780. External. NULL answers false. The object's
// isNSCFConstantString__ implementation (0x2387b02f9) is called (a tail call) unless it is the forwarding
// implementation (the GOT slot 0x222409138, taken to be _objc_msgForward by use; confidence: medium), in
// which case the answer is whether the object is a kind of the constant-string class.
BOOL _ONSIsNSCFConstantString(id object) {
    if (object == nil) {
        return NO;
    }
    SEL selector = sel_getUid("isNSCFConstantString__");
    IMP implementation = object_getMethodImplementation(object, selector);
    if (implementation != (IMP)_objc_msgForward) {
        return ((BOOL (*)(id, SEL))implementation)(object, selector);
    }
    Class cls = objc_opt_class((id)__OCFConstantStringClassReference);
    return objc_opt_isKindOfClass(object, cls) & 1;
}

// Native __natoms (0x22e6d3848, __DATA,__bss, 4 bytes), __atom_buckets_0 (0x22e6d3850, __DATA,__bss, 16
// pointers) and __atom_buckets (0x22e6cfe28, __DATA,__data, 24 pointers whose first word is the address of
// __atom_buckets_0 and the rest zero), read with Local/Scratch/cf-core/macho.py. Non-external; only
// getAtomTarget reads them on this branch (the code that registers atoms is not restored). The IDA capture
// (Local/Evidence/cf-core/iphone-simulator-corpus-delta6.json) shows __natoms as 0xffffffffffffffff, IDA's
// display of an uninitialized __bss word; the loader zero-fills __bss, so the count starts at 0.
static int32_t __natoms;
static id __atom_buckets_0[16];
static id *__atom_buckets[24] = {__atom_buckets_0};

// Native getAtomTarget, 0x1c029e944, with getAtomTarget.cold.1 (0x1c030fb58). Hidden (was a private
// external). Executables built with the fall 2019 SDKs or later answer nil. Otherwise the tagged pointer is
// decoded (the obfuscator is skipped for 0xC000000000000007 patterns; 52 payload bits for extended tags, else
// 60, after the tag), its index (payload >> 4) must not exceed __natoms (BRK #1), and the target is read from
// the atom bucket flsl((payload >> 8) + 1) - 1 at index + (0xFFFFFFFF << (bucket + 3)) + 16 (32-bit
// arithmetic, sign-extended).
id getAtomTarget(id object) {
    if (dyld_program_sdk_at_least(kFall2019VersionSet)) {
        return nil;
    }
    uintptr_t pointer = (uintptr_t)object;
    uintptr_t obfuscator = (~pointer & 0xC000000000000007) == 0 ? 0 : objc_debug_taggedpointer_obfuscator;
    uintptr_t value = obfuscator ^ pointer;
    uintptr_t mask = (~(uint32_t)value & 7) == 0 ? 0xFFFFFFFFFFFFF : 0xFFFFFFFFFFFFFFF;
    uintptr_t payload = mask & (value >> 3);
    if ((uintptr_t)(intptr_t)__natoms < payload >> 4) {
        __builtin_trap();
    }
    uint32_t index = (uint32_t)(payload >> 4);
    int bucket = flsl((long)((payload >> 8) + 1));
    int32_t slot = (int32_t)(index + ((uint32_t)0xFFFFFFFF << ((bucket + 3) & 31)) + 16);
    return __atom_buckets[bucket - 1][slot];
}

// Native __CFFullMethodName, 0x1c029eb84. Hidden (was a private external). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta3.json. Formats "%c[%s %s]" with asprintf: '+' when
// the object is the class itself and '-' otherwise, the class name ("(null class)" for Nil) and the selector
// name ("(null selector)" for NULL). A NULL buffer answers NULL; otherwise the UTF-8 string
// (kCFAllocatorSystemDefault) is autoreleased through _CFAutoreleasePoolAddObject, whose pool argument is the
// string itself (X0 is left unchanged), and the buffer is freed.
OCFStringRef __CFFullMethodName(Class cls, const void *object, SEL selector) {
    char *cString = NULL;
    int kind = (const void *)cls == object ? '+' : '-';
    const char *className = cls != Nil ? class_getName(cls) : "(null class)";
    const char *selectorName = selector != NULL ? sel_getName(selector) : "(null selector)";
    asprintf(&cString, "%c[%s %s]", kind, className, selectorName);
    if (cString == NULL) {
        return NULL;
    }
    OCFStringRef string =
        OCFStringCreateWithCString(kOCFAllocatorSystemDefault, cString, kOCFStringEncodingUTF8);
    OCFStringRef result = (OCFStringRef)_OCFAutoreleasePoolAddObject((void *)string, string);
    free(cString);
    return result;
}

// Native __CFExceptionProem, 0x1c029ec78. Hidden (was a private external). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta2.json. A tail call of __CFFullMethodName with the
// object's class (objc_opt_class), the object and the selector.
OCFStringRef __CFExceptionProem(const void *object, SEL selector) {
    Class cls = objc_opt_class((id)object);
    return __CFFullMethodName(cls, object, selector);
}

// Native __CFAllocateObject, 0x1c029eca4, with __CFAllocateObject.cold.1 (0x1c030fbf8). Hidden (was a private
// external). class_createInstance with the extra bytes; a failure throws an NSMallocException ("***
// NSAllocateObject(): attempt to allocate object failed"). With allocation statistics on (__CFOASafe), the
// object's class name becomes its last allocation event name.
id __CFAllocateObject(Class cls, size_t extraBytes) {
    id object = class_createInstance(cls, extraBytes);
    if (object == nil) {
        objc_exception_throw(exceptionWithReason(
            ONSMallocException, (id)OF_STR("*** NSAllocateObject(): attempt to allocate object failed")
        ));
    }
    if (__OCFOASafe == 1) {
        __OCFSetLastAllocationEventName(object, object_getClassName(object));
    }
    return object;
}

// The root class whose class reference __CFLookUpClass reads through the GOT slot 0x222409078, outside the
// extracted image; it is bound to NSObject (libobjc's root class) by use: objc_opt_self of it becomes the
// superclass of the NSString class pair; confidence: medium.
extern char rootObjectClass[] __asm("_OBJC_CLASS_$_NSObject");

// Native __CFLookUpClass, 0x1c029ed14. Hidden (was a private external). Evidence:
// Local/Evidence/cf-core/iphone-simulator-corpus-delta2.json. objc_lookUpClass; when the class is absent,
// Foundation is opened (dlopen mode 5, RTLD_LAZY | RTLD_LOCAL; the handle is not kept) and a failed open is
// logged at level 3 with the class name and dlerror(); the lookup is repeated. When it still fails and the
// name is NSString or NSMutableString, both classes are created and registered (NSString under the root
// class, NSMutableString under NSString, 0x20 extra bytes each) and the lookup is answered by a tail call;
// any other name answers NULL. The class names are the mapped runtime names (OF_NAME_STRING) of the restored
// string classes.
uintptr_t __CFLookUpClass(const char *name) {
    Class cls = objc_lookUpClass(name);
    if (cls != Nil) {
        return (uintptr_t)cls;
    }
    if (dlopen("/System/Library/Frameworks/Foundation.framework/Foundation", RTLD_LAZY | RTLD_LOCAL) ==
        NULL) {
        const char *error = dlerror();
        OCFLog(
            3,
            OF_STR(
                "*** Error: Could not load Foundation for class %s; error is '%s'; if this seems "
                "unreasonable, perhaps seatbelt has prevented the load due to file access limitations on "
                "the process"
            ),
            name,
            error
        );
    }
    cls = objc_lookUpClass(name);
    if (cls != Nil) {
        return (uintptr_t)cls;
    }
    if (strcmp(name, OF_NAME_STRING(ONSString)) != 0 && strcmp(name, OF_NAME_STRING(ONSMutableString)) != 0) {
        return 0;
    }
    Class root = objc_opt_self((id)rootObjectClass);
    Class stringClass = objc_allocateClassPair(root, OF_NAME_STRING(ONSString), 0x20);
    objc_registerClassPair(stringClass);
    Class mutableStringClass = objc_allocateClassPair(stringClass, OF_NAME_STRING(ONSMutableString), 0x20);
    objc_registerClassPair(mutableStringClass);
    return (uintptr_t)objc_lookUpClass(name);
}

// libsystem_blocks and libobjc interfaces the native function imports (Block_private.h, objc-internal.h);
// supplied, never reimplemented.
typedef struct {
    size_t size;
    void (*retain)(const void *object);
    void (*release)(const void *object);
    void (*destructInstance)(const void *object);
} BlockCallbacksRR;

extern void _Block_use_RR2(const BlockCallbacksRR *callbacks);
extern Class objc_initializeClassPair(Class superclass, const char *name, Class cls, Class metaclass);
extern id objc_retain(id object);
extern void objc_release(id object);
extern void *objc_destructInstance(id object);
extern void *_NSConcreteAutoBlock[32];
extern void *_NSConcreteFinalizingBlock[32];
extern void *_NSConcreteGlobalBlock[32];
extern void *_NSConcreteMallocBlock[32];
extern void *_NSConcreteStackBlock[32];
extern void *_NSConcreteWeakBlockVariable[32];

extern char blockClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSBlock));

// Native __CFMakeNSBlockClasses' callbacks, 0x2227a9ba0 (__DATA_CONST,__const, 32 bytes): the size 0x20 and
// three function addresses outside the image, taken to be objc_retain, objc_release and objc_destructInstance
// (the image's imports of those names and the libclosure field order; confidence: medium).
static const BlockCallbacksRR blockCallbacks = {
    0x20,
    (void (*)(const void *))(void (*)(void))objc_retain,
    (void (*)(const void *))(void (*)(void))objc_release,
    (void (*)(const void *))(void (*)(void))objc_destructInstance,
};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

// Native __CFMakeNSBlockClasses, 0x1c02b536c. Hidden (was a private external); it lies among the NSBlock
// methods and joins this unit by the track's assignment. When libobjc already has __NSStackBlock__, the
// stack, malloc, auto and global block classes get NSBlock (objc_opt_class) as their superclass. Otherwise
// __NSStackBlock__, __NSMallocBlock__, __NSAutoBlock__, __NSFinalizingBlock__, __NSGlobalBlock__ and
// __NSBlockVariable__ are initialized over the class storage of libsystem_blocks' _NSConcrete* arrays (the
// metaclass at 128 bytes in), with the classes without the trailing underscores as superclasses, and then
// registered in that order. The GOT slots 0x222417df8-0x222417e20 are bound to _NSConcreteAutoBlock,
// _NSConcreteFinalizingBlock, _NSConcreteGlobalBlock, _NSConcreteMallocBlock, _NSConcreteStackBlock and
// _NSConcreteWeakBlockVariable by the class each initializes and the alphabetical slot order (confidence:
// medium). Finally the copied callbacks are installed with _Block_use_RR2.
void __CFMakeNSBlockClasses(void) {
    if (objc_lookUpClass("__NSStackBlock__") != Nil) {
        Class superclass = objc_opt_class((id)blockClass);
        class_setSuperclass(objc_lookUpClass("__NSStackBlock__"), superclass);
        class_setSuperclass(objc_lookUpClass("__NSMallocBlock__"), superclass);
        class_setSuperclass(objc_lookUpClass("__NSAutoBlock__"), superclass);
        class_setSuperclass(objc_lookUpClass("__NSGlobalBlock__"), superclass);
    } else {
        Class stack = (Class)_NSConcreteStackBlock;
        objc_initializeClassPair(
            objc_lookUpClass(OF_NAME_STRING(__ONSStackBlock)),
            "__NSStackBlock__",
            stack,
            (Class)(_NSConcreteStackBlock + 16)
        );
        Class mallocBlock = (Class)_NSConcreteMallocBlock;
        objc_initializeClassPair(
            objc_lookUpClass(OF_NAME_STRING(__ONSMallocBlock)),
            "__NSMallocBlock__",
            mallocBlock,
            (Class)(_NSConcreteMallocBlock + 16)
        );
        Class automatic = (Class)_NSConcreteAutoBlock;
        objc_initializeClassPair(
            objc_lookUpClass(OF_NAME_STRING(__ONSAutoBlock)),
            "__NSAutoBlock__",
            automatic,
            (Class)(_NSConcreteAutoBlock + 16)
        );
        Class finalizing = (Class)_NSConcreteFinalizingBlock;
        objc_initializeClassPair(
            objc_lookUpClass(OF_NAME_STRING(__ONSFinalizingBlock)),
            "__NSFinalizingBlock__",
            finalizing,
            (Class)(_NSConcreteFinalizingBlock + 16)
        );
        Class global = (Class)_NSConcreteGlobalBlock;
        objc_initializeClassPair(
            objc_lookUpClass(OF_NAME_STRING(__ONSGlobalBlock)),
            "__NSGlobalBlock__",
            global,
            (Class)(_NSConcreteGlobalBlock + 16)
        );
        Class variable = (Class)_NSConcreteWeakBlockVariable;
        objc_initializeClassPair(
            objc_lookUpClass("__NSBlockVariable"),
            "__NSBlockVariable__",
            variable,
            (Class)(_NSConcreteWeakBlockVariable + 16)
        );
        objc_registerClassPair(stack);
        objc_registerClassPair(mallocBlock);
        objc_registerClassPair(automatic);
        objc_registerClassPair(finalizing);
        objc_registerClassPair(global);
        objc_registerClassPair(variable);
    }
    BlockCallbacksRR callbacks = blockCallbacks;
    _Block_use_RR2(&callbacks);
}

#pragma clang diagnostic pop

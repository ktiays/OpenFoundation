//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// The wasm port's __CFMakeNSBlockClasses (Port/hooks.tsv). The restored function (native 0x1c02b536c) takes
// one of two paths: when libobjc already has block classes, NSBlock becomes their superclass; otherwise it
// builds __NSStackBlock__ and the others over libsystem_blocks' class storage, with __NSStackBlock,
// __NSMallocBlock, __NSAutoBlock and __NSGlobalBlock as their superclasses. objc4's wasm port compiles its
// own concrete block classes (ObjC4WasmStackBlock, ObjC4WasmHeapBlock, ObjC4WasmGlobalBlock under
// ObjC4WasmBlock) and makes the _NSConcrete*Block symbols aliases of them. The port gives each of those
// classes the restored superclass of the second path, so a block resolves retain, release and copy the way
// the native hierarchy does (__NSMallocBlock__ -> __NSMallocBlock -> NSBlock): -[__NSMallocBlock retain] is
// _Block_copy,
// -[__NSGlobalBlock retain] answers the receiver. Making NSBlock their direct superclass, as the first path
// does for libobjc's own classes, would leave a heap block with NSObject's -retain, which treats the block as
// a side-table object (an unaligned atomic access on wasm). objc4 aliases _NSConcreteAutoBlock to its heap
// class, which therefore keeps __NSMallocBlock. The same callbacks are then installed (objc_retain,
// objc_release, objc_destructInstance).

#import <objc/runtime.h>

#include <OpenCoreFoundation/OCFBuild.h>

#include <OFPortHookDeclarations.h>
#include <stddef.h>

typedef struct {
    size_t size;
    void (*retain)(const void *object);
    void (*release)(const void *object);
    void (*destructInstance)(const void *object);
} BlockCallbacksRR;

extern void _Block_use_RR2(const BlockCallbacksRR *callbacks);
extern id objc_retain(id object);
extern void objc_release(id object);
extern void *objc_destructInstance(id object);
extern char _NSConcreteStackBlock[];
extern char _NSConcreteMallocBlock[];
extern char _NSConcreteAutoBlock[];
extern char _NSConcreteGlobalBlock[];

static void retainBlockObject(const void *object) {
    objc_retain((id)object);
}

static void releaseBlockObject(const void *object) {
    objc_release((id)object);
}

static void destructBlockObject(const void *object) {
    objc_destructInstance((id)object);
}

static const BlockCallbacksRR blockCallbacks = {
    sizeof(BlockCallbacksRR),
    retainBlockObject,
    releaseBlockObject,
    destructBlockObject,
};

// Gives one of objc4's concrete block classes the named restored superclass.
static void setBlockSuperclass(char *concreteClass, const char *superclassName) {
    Class cls = (Class)(void *)concreteClass;
    Class superclass = objc_lookUpClass(superclassName);
    if (superclass == Nil || class_getSuperclass(cls) == superclass) {
        return;
    }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    class_setSuperclass(cls, superclass);
#pragma clang diagnostic pop
}

void OFPort_makeBlockClasses(void) {
    setBlockSuperclass(_NSConcreteStackBlock, OF_NAME_STRING(__ONSStackBlock));
    setBlockSuperclass(_NSConcreteMallocBlock, OF_NAME_STRING(__ONSMallocBlock));
    if (_NSConcreteAutoBlock != _NSConcreteMallocBlock) {
        setBlockSuperclass(_NSConcreteAutoBlock, OF_NAME_STRING(__ONSAutoBlock));
    }
    setBlockSuperclass(_NSConcreteGlobalBlock, OF_NAME_STRING(__ONSGlobalBlock));
    _Block_use_RR2(&blockCallbacks);
}

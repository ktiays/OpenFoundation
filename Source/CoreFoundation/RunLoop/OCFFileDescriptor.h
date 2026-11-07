//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/RunLoop/OCFRunLoop.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"

#include <dispatch/dispatch.h>
#include <stdint.h>

// Public CFFileDescriptor interface, iOS Simulator SDK CFFileDescriptor.h, spelled with the OCF prefix (the
// subset this unit defines or reads).
typedef int OCFFileDescriptorNativeDescriptor;
typedef struct __OCFFileDescriptor *OCFFileDescriptorRef;

enum : OCFOptionFlags {
    kOCFFileDescriptorReadCallBack = 1UL << 0,
    kOCFFileDescriptorWriteCallBack = 1UL << 1,
};

typedef void (*OCFFileDescriptorCallBack)(OCFFileDescriptorRef f, OCFOptionFlags callBackTypes, void *info);

typedef struct {
    OCFIndex version;
    void *info;
    void *(*retain)(void *info);
    void (*release)(void *info);
    OCFStringRef (*copyDescription)(void *info);
} OCFFileDescriptorContext;

// The CFFileDescriptor object (type 60, 0x60 extra bytes), iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for every offset named here (the accesses of
// RunLoop/OCFFileDescriptor.c), unread for the bytes marked unread. Info bit 1 closes the descriptor on
// invalidation. The state is 0 while valid, 1 while invalidating and 2 once invalid. The read and write
// descriptors are F_DUPFD_CLOEXEC duplicates (minimum 3) watched by the read and write dispatch sources; the
// four bytes after the sources mark each source suspended and each fired. The context is the client's copy
// with its info retained. The type and field names are local descriptive spellings.
struct __OCFFileDescriptor {
    OCFRuntimeBase base;
    int32_t state;
    OCFFileDescriptorNativeDescriptor fd;
    int32_t readFd;
    int32_t writeFd;
    dispatch_source_t readSource;
    dispatch_source_t writeSource;
    uint8_t readSuspended;
    uint8_t writeSuspended;
    uint8_t readFired;
    uint8_t writeFired;
    uint32_t unread0;
    OCFRunLoopSourceRef rls;
    OCFFileDescriptorCallBack callout;
    OCFFileDescriptorContext context;
};

OF_ABI_ASSERT(
    sizeof(struct __OCFFileDescriptor) == 0x70, "The native CFFileDescriptor object is 0x70 bytes."
);

OF_EXPORT OCFTypeID OCFFileDescriptorGetTypeID(void);
OF_EXPORT OCFFileDescriptorRef OCFFileDescriptorCreate(
    OCFAllocatorRef allocator,
    OCFFileDescriptorNativeDescriptor fd,
    Boolean closeOnInvalidate,
    OCFFileDescriptorCallBack callout,
    const OCFFileDescriptorContext *context
);
OF_EXPORT void OCFFileDescriptorEnableCallBacks(OCFFileDescriptorRef f, OCFOptionFlags callBackTypes);
OF_EXPORT void OCFFileDescriptorInvalidate(OCFFileDescriptorRef f);
OF_EXPORT Boolean OCFFileDescriptorIsValid(OCFFileDescriptorRef f);
OF_EXPORT OCFRunLoopSourceRef
OCFFileDescriptorCreateRunLoopSource(OCFAllocatorRef allocator, OCFFileDescriptorRef f, OCFIndex order);

// Native _CFFileDescriptorInduceFakeReadCallBack, 0x1c019b030. Hidden (was a private external).
OF_EXTERN void _CFFileDescriptorInduceFakeReadCallBack(OCFFileDescriptorRef f);

// The class record (type 60), defined by RunLoop/OCFFileDescriptor.c for the builtin table.
OF_EXTERN const OCFRuntimeClass __CFFileDescriptorClass;

//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// The wasm port's hook functions (Source/CoreFoundation/OFPortHooks.h, Port/hooks.tsv).

#include <OFPortHookDeclarations.h>

#include "CoreFoundation/String/OCFString.h"

#include <objc/message.h>
#include <malloc/malloc.h>
#include <objc/runtime.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int32_t OFPort_floatToInt32(float value) {
    return __builtin_wasm_trunc_saturate_s_i32_f32(value);
}

uint32_t OFPort_floatToUInt32(float value) {
    return __builtin_wasm_trunc_saturate_u_i32_f32(value);
}

int64_t OFPort_floatToInt64(float value) {
    return __builtin_wasm_trunc_saturate_s_i64_f32(value);
}

uint64_t OFPort_floatToUInt64(float value) {
    return __builtin_wasm_trunc_saturate_u_i64_f32(value);
}

int32_t OFPort_doubleToInt32(double value) {
    return __builtin_wasm_trunc_saturate_s_i32_f64(value);
}

uint32_t OFPort_doubleToUInt32(double value) {
    return __builtin_wasm_trunc_saturate_u_i32_f64(value);
}

int64_t OFPort_doubleToInt64(double value) {
    return __builtin_wasm_trunc_saturate_s_i64_f64(value);
}

uint64_t OFPort_doubleToUInt64(double value) {
    return __builtin_wasm_trunc_saturate_u_i64_f64(value);
}

dispatch_source_t OFPort_memoryPressureSource(dispatch_queue_t queue) {
    return dispatch_source_create(DISPATCH_SOURCE_TYPE_DATA_OR, 0, 0, queue);
}

uint16_t OFPort_direntNameLength(const struct dirent *entry) {
    return (uint16_t)strlen(entry->d_name);
}

int OFPort_probeEnabled(void) {
    return 0;
}

static char *alignCursor(char *cursor, uintptr_t alignment) {
    return (char *)(((uintptr_t)cursor + alignment - 1) & ~(alignment - 1));
}

int64_t OFPort_formatReadInt8(char **cursor) {
    int32_t value;
    memcpy(&value, *cursor, sizeof(value));
    *cursor += 4;
    return (int8_t)value;
}

int64_t OFPort_formatReadInt16(char **cursor) {
    int32_t value;
    memcpy(&value, *cursor, sizeof(value));
    *cursor += 4;
    return (int16_t)value;
}

int64_t OFPort_formatReadInt32(char **cursor) {
    int32_t value;
    memcpy(&value, *cursor, sizeof(value));
    *cursor += 4;
    return value;
}

int64_t OFPort_formatReadInt64(char **cursor) {
    *cursor = alignCursor(*cursor, 8);
    int64_t value;
    memcpy(&value, *cursor, sizeof(value));
    *cursor += 8;
    return value;
}

int64_t OFPort_formatReadDoubleBits(char **cursor) {
    return OFPort_formatReadInt64(cursor);
}

int64_t OFPort_formatReadPointer(char **cursor) {
    uintptr_t value;
    memcpy(&value, *cursor, sizeof(value));
    *cursor += sizeof(value);
    return (int64_t)value;
}

int OFPort_formatArgumentSlotSize(void) {
    return 4;
}

uint8_t OFPort_commpageKdebugEnabled(void) {
    return 0;
}

uint8_t OFPort_commpageActiveCPUs(void) {
    long count = sysconf(_SC_NPROCESSORS_ONLN);
    return (uint8_t)(count < 1 ? 1 : (count > 255 ? 255 : count));
}

int OFPort_formatIntegerSize(unsigned character) {
    return character == 'j' || character == 'q' ? 4 : 0;
}

int OFPort_formatLongSize(const uint8_t *cformat, const uint16_t *uformat, int32_t position) {
    if (position < 2) {
        return 0;
    }
    unsigned first = cformat != NULL ? cformat[position - 2] : uformat[position - 2];
    unsigned second = cformat != NULL ? cformat[position - 1] : uformat[position - 1];
    return first == 'l' && second == 'l' ? 4 : 0;
}

void OFPort_scanBufferFetch(
    void *string, unsigned short *characters, unsigned long location, unsigned long count
) {
    typedef struct {
        unsigned long location;
        unsigned long length;
    } ScanRange;

    ScanRange range = {location, count};
    ((void (*)(void *, SEL, unsigned short *, ScanRange))objc_msgSend)(
        string, sel_getUid("getCharacters:range:"), characters, range
    );
}

const void *OFPort_indirectASCIITaggedPointerString(const uint8_t *bytes, long length) {
    (void)bytes;
    if (length == 0) {
        return kOCFEmptyString;
    }
    return NULL;
}

void *OFPort_mallocAllocatorAllocate(long size, unsigned long hint, void *info) {
    (void)hint;
    (void)info;
    return malloc((size_t)size);
}

void *OFPort_mallocAllocatorReallocate(void *pointer, long size, unsigned long hint, void *info) {
    (void)hint;
    (void)info;
    return realloc(pointer, (size_t)size);
}

void OFPort_mallocAllocatorDeallocate(void *pointer, void *info) {
    (void)info;
    free(pointer);
}

void *OFPort_mallocAllocatorTypedAllocate(long size, uint64_t typeID, unsigned long hint, void *info) {
    (void)hint;
    (void)info;
    return malloc_type_malloc((size_t)size, typeID);
}

void *OFPort_mallocAllocatorTypedReallocate(
    void *pointer, long size, uint64_t typeID, unsigned long hint, void *info
) {
    (void)hint;
    (void)info;
    return malloc_type_realloc(pointer, (size_t)size, typeID);
}

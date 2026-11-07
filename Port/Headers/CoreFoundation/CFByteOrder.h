//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// Byte order conversions, inline as in the SDK. wasm32 is little-endian.

#include <CoreFoundation/CFBase.h>

CF_EXTERN_C_BEGIN

enum __CFByteOrder {
    CFByteOrderUnknown,
    CFByteOrderLittleEndian,
    CFByteOrderBigEndian
};
typedef CFIndex CFByteOrder;

CF_INLINE CFByteOrder CFByteOrderGetCurrent(void) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return CFByteOrderLittleEndian;
#else
    return CFByteOrderBigEndian;
#endif
}

CF_INLINE uint16_t CFSwapInt16(uint16_t arg) {
    return __builtin_bswap16(arg);
}

CF_INLINE uint32_t CFSwapInt32(uint32_t arg) {
    return __builtin_bswap32(arg);
}

CF_INLINE uint64_t CFSwapInt64(uint64_t arg) {
    return __builtin_bswap64(arg);
}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define __CF_BIG(swap, value) swap(value)
    #define __CF_LITTLE(swap, value) (value)
#else
    #define __CF_BIG(swap, value) (value)
    #define __CF_LITTLE(swap, value) swap(value)
#endif

CF_INLINE uint16_t CFSwapInt16BigToHost(uint16_t arg) {
    return __CF_BIG(CFSwapInt16, arg);
}

CF_INLINE uint32_t CFSwapInt32BigToHost(uint32_t arg) {
    return __CF_BIG(CFSwapInt32, arg);
}

CF_INLINE uint64_t CFSwapInt64BigToHost(uint64_t arg) {
    return __CF_BIG(CFSwapInt64, arg);
}

CF_INLINE uint16_t CFSwapInt16HostToBig(uint16_t arg) {
    return __CF_BIG(CFSwapInt16, arg);
}

CF_INLINE uint32_t CFSwapInt32HostToBig(uint32_t arg) {
    return __CF_BIG(CFSwapInt32, arg);
}

CF_INLINE uint64_t CFSwapInt64HostToBig(uint64_t arg) {
    return __CF_BIG(CFSwapInt64, arg);
}

CF_INLINE uint16_t CFSwapInt16LittleToHost(uint16_t arg) {
    return __CF_LITTLE(CFSwapInt16, arg);
}

CF_INLINE uint32_t CFSwapInt32LittleToHost(uint32_t arg) {
    return __CF_LITTLE(CFSwapInt32, arg);
}

CF_INLINE uint64_t CFSwapInt64LittleToHost(uint64_t arg) {
    return __CF_LITTLE(CFSwapInt64, arg);
}

CF_INLINE uint16_t CFSwapInt16HostToLittle(uint16_t arg) {
    return __CF_LITTLE(CFSwapInt16, arg);
}

CF_INLINE uint32_t CFSwapInt32HostToLittle(uint32_t arg) {
    return __CF_LITTLE(CFSwapInt32, arg);
}

CF_INLINE uint64_t CFSwapInt64HostToLittle(uint64_t arg) {
    return __CF_LITTLE(CFSwapInt64, arg);
}

#undef __CF_BIG
#undef __CF_LITTLE

typedef struct {
    uint32_t v;
} CFSwappedFloat32;

typedef struct {
    uint64_t v;
} CFSwappedFloat64;

CF_INLINE CFSwappedFloat32 CFConvertFloat32HostToSwapped(Float32 arg) {
    union {
        Float32 value;
        CFSwappedFloat32 swapped;
    } result;
    result.value = arg;
    result.swapped.v = CFSwapInt32(result.swapped.v);
    return result.swapped;
}

CF_INLINE Float32 CFConvertFloat32SwappedToHost(CFSwappedFloat32 arg) {
    union {
        Float32 value;
        CFSwappedFloat32 swapped;
    } result;
    result.swapped = arg;
    result.swapped.v = CFSwapInt32(result.swapped.v);
    return result.value;
}

CF_INLINE CFSwappedFloat64 CFConvertFloat64HostToSwapped(Float64 arg) {
    union {
        Float64 value;
        CFSwappedFloat64 swapped;
    } result;
    result.value = arg;
    result.swapped.v = CFSwapInt64(result.swapped.v);
    return result.swapped;
}

CF_INLINE Float64 CFConvertFloat64SwappedToHost(CFSwappedFloat64 arg) {
    union {
        Float64 value;
        CFSwappedFloat64 swapped;
    } result;
    result.swapped = arg;
    result.swapped.v = CFSwapInt64(result.swapped.v);
    return result.value;
}

CF_EXTERN_C_END

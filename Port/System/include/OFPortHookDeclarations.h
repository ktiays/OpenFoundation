//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// The wasm port's hook functions (Source/CoreFoundation/OFPortHooks.h; Port/hooks.tsv lists every site).

#include <stdbool.h>
#include <stdint.h>

// dyld_program_sdk_at_least called with the packed 64-bit dyld_build_version_t: arm64 passes the packed form
// and the structure form alike in one register, wasm passes the structure indirectly, so the port has one
// implementation per form.
bool OFPort_programSDKAtLeastPacked(uint64_t version);

#include <dirent.h>
#include <dispatch/dispatch.h>

// arm64's FCVTZS/FCVTZU (saturating, NaN to zero): wasm's saturating truncations have the same results.
int32_t OFPort_floatToInt32(float value);
uint32_t OFPort_floatToUInt32(float value);
int64_t OFPort_floatToInt64(float value);
uint64_t OFPort_floatToUInt64(float value);
int32_t OFPort_doubleToInt32(double value);
uint32_t OFPort_doubleToUInt32(double value);
int64_t OFPort_doubleToInt64(double value);
uint64_t OFPort_doubleToUInt64(double value);

// A memory-pressure dispatch source: the port has no memory-pressure events, so the source never fires.
dispatch_source_t OFPort_memoryPressureSource(dispatch_queue_t queue);

// Darwin's struct dirent carries the name's length; musl's does not.
uint16_t OFPort_direntNameLength(const struct dirent *entry);

// DTrace is-enabled sites: the port has no DTrace, so no probe is enabled (the native MOV X0, #0).
int OFPort_probeEnabled(void);

// CFString formatting's argument fetch (__CFStringAppendFormatCore): the native code walks a Darwin arm64
// va_list, eight-byte slots; wasm32's variadic buffer packs int, long and pointers in four bytes and aligns
// long long and double to eight. Each reader returns the value and advances the cursor.
int64_t OFPort_formatReadInt8(char **cursor);
int64_t OFPort_formatReadInt16(char **cursor);
int64_t OFPort_formatReadInt32(char **cursor);
int64_t OFPort_formatReadInt64(char **cursor);
int64_t OFPort_formatReadDoubleBits(char **cursor);
int64_t OFPort_formatReadPointer(char **cursor);
// The slot an argument the format does not read occupies: four bytes, the size of int and pointers.
int OFPort_formatArgumentSlotSize(void);

// Darwin's commpage, read at fixed addresses (0xfffffc342, the kdebug enable byte; 0xfffffc034, the active
// processor count). wasm has no commpage: kdebug is off and the processor count comes from sysconf.
uint8_t OFPort_commpageKdebugEnabled(void);
uint8_t OFPort_commpageActiveCPUs(void);

// CFString formatting's integer size codes: 4 is a 64-bit argument and 0 an int. LP64 gives long, ptrdiff_t
// and size_t the 64-bit code; on wasm32 they are int-sized (ll, q and j stay 64-bit). formatIntegerSize
// answers the j, q, t and z modifiers, which the native code handles under one case label, from the modifier
// character. formatLongSize reads the modifier the parser has just passed: "ll" when the two characters
// before the cursor are both 'l'.
int OFPort_formatIntegerSize(unsigned character);
int OFPort_formatLongSize(const uint8_t *cformat, const uint16_t *uformat, int32_t position);

// __CFMakeNSBlockClasses: objc4's wasm port defines _NSConcrete*Block as its own block classes rather than
// libsystem_blocks' 32-word class storage, so the native path that builds __NSStackBlock__ and the others
// over that storage would overwrite neighboring data. The port gives objc4's classes the superclasses that
// path gives __NSStackBlock__ and the others (__NSStackBlock, __NSMallocBlock, __NSGlobalBlock), then
// installs the block callbacks.
void OFPort_makeBlockClasses(void);

// _NSScanLongLongFromString and _NSScanDoubleFromString fill their character buffer with
// -getCharacters:range: through an objc_msgSend cast that passes the range as two NSUInteger arguments.
// arm64 passes an NSRange in the same two registers; wasm32 passes the structure indirectly, so the cast does
// not match the method's type and call_indirect traps. The port sends the range as the structure.
void OFPort_scanBufferFetch(
    void *string, unsigned short *characters, unsigned long location, unsigned long count
);

// _CFStringCreateIndirectASCIITaggedPointerString: objc4 supports tagged pointer objects only where pointers
// are 64 bits, and the native encoding stores the length in bits 50 and up. The port answers the native
// function's own results for the empty string (the constant empty string) and for a string it cannot tag
// (NULL, after which callers make an ordinary string).
const void *OFPort_indirectASCIITaggedPointerString(const uint8_t *bytes, long length);

// kCFAllocatorMalloc's context callbacks: the native table stores libsystem's malloc, realloc and free and
// the typed malloc_type_malloc and malloc_type_realloc, which CoreFoundation calls with the callback types'
// extra hint, type and info arguments. arm64 ignores the extra registers; wasm's call_indirect traps on the
// mismatched type. The port stores functions of the callback types (CFIndex is long, CFOptionFlags unsigned
// long) that call the same allocation functions.
void *OFPort_mallocAllocatorAllocate(long size, unsigned long hint, void *info);
void *OFPort_mallocAllocatorReallocate(void *pointer, long size, unsigned long hint, void *info);
void OFPort_mallocAllocatorDeallocate(void *pointer, void *info);
void *OFPort_mallocAllocatorTypedAllocate(long size, uint64_t typeID, unsigned long hint, void *info);
void *OFPort_mallocAllocatorTypedReallocate(
    void *pointer, long size, uint64_t typeID, unsigned long hint, void *info
);

// A Darwin static TSD key read directly with pthread_getspecific (CoreFoundation's key 55): the port maps
// static key numbers to keys it creates (pthread_key_init_np, Port/System/Source/DarwinExtras.c).
void *OFPort_staticKeyGetSpecific(unsigned long key);

// __CFInitialize stores _CFAutoreleasePoolPop, which returns its argument, in libdispatch's
// void (*)(void *) pool hook: arm64 ignores the unused return register, wasm's call_indirect traps on the
// signature mismatch. The port stores a function of the hook's exact type that calls it.
void (*OFPort_autoreleasePoolPopHook(void))(void *);

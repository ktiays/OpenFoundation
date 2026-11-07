//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// libmalloc zones for the wasm port (Port/System/Source/Malloc.c): one default zone over Emscripten's
// allocator, with Darwin's zone structure so that zone-based allocators call through it as on Darwin.

#include <PortDarwin.h>
#include <mach/mach_types.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

typedef struct _malloc_zone_t malloc_zone_t;
typedef struct malloc_introspection_t malloc_introspection_t;

typedef struct {
    uintptr_t address;
    size_t size;
} vm_range_t;

struct _malloc_zone_t {
    void *reserved1;
    void *reserved2;
    size_t (*size)(struct _malloc_zone_t *zone, const void *pointer);
    void *(*malloc)(struct _malloc_zone_t *zone, size_t size);
    void *(*calloc)(struct _malloc_zone_t *zone, size_t count, size_t size);
    void *(*valloc)(struct _malloc_zone_t *zone, size_t size);
    void (*free)(struct _malloc_zone_t *zone, void *pointer);
    void *(*realloc)(struct _malloc_zone_t *zone, void *pointer, size_t size);
    void (*destroy)(struct _malloc_zone_t *zone);
    const char *zone_name;
    unsigned (*batch_malloc)(struct _malloc_zone_t *zone, size_t size, void **results, unsigned count);
    void (*batch_free)(struct _malloc_zone_t *zone, void **pointers, unsigned count);
    malloc_introspection_t *introspect;
    unsigned version;
    void *(*memalign)(struct _malloc_zone_t *zone, size_t alignment, size_t size);
    void (*free_definite_size)(struct _malloc_zone_t *zone, void *pointer, size_t size);
    size_t (*pressure_relief)(struct _malloc_zone_t *zone, size_t goal);
    bool (*claimed_address)(struct _malloc_zone_t *zone, void *pointer);
};

// Zones created at run time draw from the same allocator as the default zone (a zone's memory is freed by
// whichever zone frees it, and malloc_zone_from_ptr answers the default zone); each has its own name.
malloc_zone_t *malloc_create_zone(vm_size_t startSize, unsigned flags);
void malloc_destroy_zone(malloc_zone_t *zone);
malloc_zone_t *malloc_default_zone(void);
malloc_zone_t *malloc_zone_from_ptr(const void *pointer);
void *malloc_zone_malloc(malloc_zone_t *zone, size_t size);
void *malloc_zone_calloc(malloc_zone_t *zone, size_t count, size_t size);
void *malloc_zone_realloc(malloc_zone_t *zone, void *pointer, size_t size);
void *malloc_zone_memalign(malloc_zone_t *zone, size_t alignment, size_t size);
void *malloc_zone_valloc(malloc_zone_t *zone, size_t size);
void malloc_zone_free(malloc_zone_t *zone, void *pointer);
const char *malloc_get_zone_name(malloc_zone_t *zone);
void malloc_set_zone_name(malloc_zone_t *zone, const char *name);
void *malloc_type_zone_malloc(malloc_zone_t *zone, size_t size, malloc_type_id_t typeID);
void *malloc_type_zone_calloc(malloc_zone_t *zone, size_t count, size_t size, malloc_type_id_t typeID);
void *malloc_type_zone_realloc(malloc_zone_t *zone, void *pointer, size_t size, malloc_type_id_t typeID);
void *malloc_type_zone_memalign(malloc_zone_t *zone, size_t alignment, size_t size, malloc_type_id_t typeID);
void *malloc_type_zone_valloc(malloc_zone_t *zone, size_t size, malloc_type_id_t typeID);
size_t malloc_size(const void *pointer);
size_t malloc_good_size(size_t size);

__END_DECLS

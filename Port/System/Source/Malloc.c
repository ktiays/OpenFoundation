//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// The default malloc zone of the wasm port (Port/System/include/malloc/malloc.h).

#include <malloc/malloc.h>

#include <stdlib.h>
#include <string.h>

size_t malloc_usable_size(void *pointer);

static size_t zoneSize(malloc_zone_t *zone, const void *pointer) {
    (void)zone;
    return malloc_usable_size((void *)pointer);
}

static void *zoneMalloc(malloc_zone_t *zone, size_t size) {
    (void)zone;
    return malloc(size);
}

static void *zoneCalloc(malloc_zone_t *zone, size_t count, size_t size) {
    (void)zone;
    return calloc(count, size);
}

static void *zoneValloc(malloc_zone_t *zone, size_t size) {
    (void)zone;
    return aligned_alloc(4096, (size + 4095) & ~(size_t)4095);
}

static void zoneFree(malloc_zone_t *zone, void *pointer) {
    (void)zone;
    free(pointer);
}

static void *zoneRealloc(malloc_zone_t *zone, void *pointer, size_t size) {
    (void)zone;
    return realloc(pointer, size);
}

static void zoneDestroy(malloc_zone_t *zone) {
    (void)zone;
}

static void *zoneMemalign(malloc_zone_t *zone, size_t alignment, size_t size) {
    (void)zone;
    void *memory = NULL;
    return posix_memalign(&memory, alignment < sizeof(void *) ? sizeof(void *) : alignment, size) == 0
               ? memory
               : NULL;
}

static void zoneFreeDefiniteSize(malloc_zone_t *zone, void *pointer, size_t size) {
    (void)zone;
    (void)size;
    free(pointer);
}

static malloc_zone_t gDefaultZone = {
    .size = zoneSize,
    .malloc = zoneMalloc,
    .calloc = zoneCalloc,
    .valloc = zoneValloc,
    .free = zoneFree,
    .realloc = zoneRealloc,
    .destroy = zoneDestroy,
    .zone_name = "DefaultMallocZone",
    .version = 10,
    .memalign = zoneMemalign,
    .free_definite_size = zoneFreeDefiniteSize,
};

malloc_zone_t *malloc_default_zone(void) {
    return &gDefaultZone;
}

malloc_zone_t *malloc_create_zone(vm_size_t startSize, unsigned flags) {
    (void)startSize;
    (void)flags;
    malloc_zone_t *zone = malloc(sizeof(malloc_zone_t));
    if (zone == NULL) {
        return NULL;
    }
    *zone = gDefaultZone;
    zone->zone_name = NULL;
    return zone;
}

// Darwin frees every block of the zone with it; blocks here belong to the shared allocator, so only the zone
// record goes.
void malloc_destroy_zone(malloc_zone_t *zone) {
    if (zone != NULL && zone != &gDefaultZone) {
        free(zone);
    }
}

malloc_zone_t *malloc_zone_from_ptr(const void *pointer) {
    return pointer != NULL ? &gDefaultZone : NULL;
}

void *malloc_zone_malloc(malloc_zone_t *zone, size_t size) {
    return zone->malloc(zone, size);
}

void *malloc_zone_calloc(malloc_zone_t *zone, size_t count, size_t size) {
    return zone->calloc(zone, count, size);
}

void *malloc_zone_realloc(malloc_zone_t *zone, void *pointer, size_t size) {
    return zone->realloc(zone, pointer, size);
}

void *malloc_zone_memalign(malloc_zone_t *zone, size_t alignment, size_t size) {
    return zone->memalign(zone, alignment, size);
}

void *malloc_zone_valloc(malloc_zone_t *zone, size_t size) {
    return zone->valloc(zone, size);
}

void malloc_zone_free(malloc_zone_t *zone, void *pointer) {
    zone->free(zone, pointer);
}

const char *malloc_get_zone_name(malloc_zone_t *zone) {
    return zone->zone_name;
}

void malloc_set_zone_name(malloc_zone_t *zone, const char *name) {
    zone->zone_name = name;
}

void *malloc_type_zone_malloc(malloc_zone_t *zone, size_t size, malloc_type_id_t typeID) {
    (void)typeID;
    return malloc_zone_malloc(zone, size);
}

void *malloc_type_zone_calloc(malloc_zone_t *zone, size_t count, size_t size, malloc_type_id_t typeID) {
    (void)typeID;
    return malloc_zone_calloc(zone, count, size);
}

void *malloc_type_zone_realloc(malloc_zone_t *zone, void *pointer, size_t size, malloc_type_id_t typeID) {
    (void)typeID;
    return malloc_zone_realloc(zone, pointer, size);
}

void *malloc_type_zone_memalign(malloc_zone_t *zone, size_t alignment, size_t size, malloc_type_id_t typeID) {
    (void)typeID;
    return malloc_zone_memalign(zone, alignment, size);
}

void *malloc_type_zone_valloc(malloc_zone_t *zone, size_t size, malloc_type_id_t typeID) {
    (void)typeID;
    return malloc_zone_valloc(zone, size);
}

size_t malloc_size(const void *pointer) {
    return pointer != NULL ? malloc_usable_size((void *)pointer) : 0;
}

// Darwin's tiny allocator rounds to 16 bytes, its small allocator to 512.
size_t malloc_good_size(size_t size) {
    if (size <= 1008) {
        return size == 0 ? 16 : (size + 15) & ~(size_t)15;
    }
    return (size + 511) & ~(size_t)511;
}

//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSZone.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"

#include <malloc/malloc.h>

// Inferred organization: NSZoneMalloc (0x1c0593e6c) lies in the Foundation image's NSZone functions;
// following the FS-string ownership it has its own unit here. iOS Simulator 24A434, arm64, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full,
// iphone-simulator-foundation-link.json). iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Foundation/String/ONSStringReview.md.

// Native NSZoneMalloc, 0x1c0593e6c. External. A NULL zone is malloc_default_zone(); the allocation is a tail
// call of malloc_type_zone_malloc with type ID 0x409734D7. An NSZone is a malloc zone in this image.
void *ONSZoneMalloc(struct _NSZone *zone, ONSUInteger size) {
    malloc_zone_t *mallocZone = (malloc_zone_t *)zone;
    if (mallocZone == NULL) {
        mallocZone = malloc_default_zone();
    }
    return malloc_type_zone_malloc(mallocZone, size, 0x409734D7);
}

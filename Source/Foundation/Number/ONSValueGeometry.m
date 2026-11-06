//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Number/ONSValueGeometry.h"

#import "Foundation/String/ONSString.h"

#include <malloc/malloc.h>

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Foundation/Number/Reading.md, round 2. Organization by use in the NSValue cluster.

// Native NSStringFromPoint, 0x1c0519ecc. External.
id ONSStringFromPoint(ValuePoint point) {
    return [ONSString stringWithFormat:(id)OF_STR("{%.*g, %.*g}"), 17, point.x, 17, point.y];
}

// Native NSStringFromSize, 0x1c0519f64. External.
id ONSStringFromSize(ValueSize size) {
    return [ONSString stringWithFormat:(id)OF_STR("{%.*g, %.*g}"), 17, size.width, 17, size.height];
}

// Native NSStringFromRect, 0x1c0519ffc. External.
id ONSStringFromRect(ValueRect rect) {
    return [ONSString stringWithFormat:(id)OF_STR("{{%.*g, %.*g}, {%.*g, %.*g}}"),
                                       17,
                                       rect.origin.x,
                                       17,
                                       rect.origin.y,
                                       17,
                                       rect.size.width,
                                       17,
                                       rect.size.height];
}

// Native NSShouldRetainWithZone, 0x1c0593f64. External.
BOOL ONSShouldRetainWithZone(id object, struct _NSZone *requestedZone) {
    malloc_zone_t *objectZone = malloc_zone_from_ptr(object);
    if (requestedZone != NULL) {
        return objectZone == (malloc_zone_t *)requestedZone;
    }

    malloc_zone_t *defaultZone = malloc_default_zone();
    return objectZone == NULL || objectZone == defaultZone;
}

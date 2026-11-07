//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Date/OCFTimeZoneEntryPoints.h"

#include "CoreFoundation/Objective-C/OCFNSObject.h"

#include <math.h>
#include <objc/message.h>
#include <objc/runtime.h>

// Inferred organization: CFTimeZoneGetTypeID (0x1c02acf24, family J), CFTimeZoneCopyDefault (0x1c02acfa8),
// CFTimeZoneCreateWithTimeIntervalFromGMT (0x1c02ad154) and CFTimeZoneGetName (0x1c02ad218) belong to the
// CFTimeZone unit, whose CFTimeZoneGetSecondsFromGMT (0x1c02ad220) Date/OCFTimeZone.c restores; that file
// belongs to another family, so these have their own translation unit, in native address order. In this image
// CFTimeZone is a thin layer over NSTimeZone (an Objective-C class of the image, loaded from the class
// reference 0x2227ce3d8). The native source path is not in the evidence. Unless a definition says otherwise:
// iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in
// full); iOS Device 24A446: unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json and, for CFTimeZoneGetTypeID,
// Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json.

// libobjc's objc_retain (no SDK declaration in MRC); supplied.
extern id objc_retain(id value);

// The class the entry points message (NSTimeZone, an Objective-C class of this image, not restored on this
// branch; Date/OCFTimeZone.h declares it for the naming switch).
extern char timeZoneClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSTimeZone));

// Native CFTimeZoneGetTypeID, 0x1c02acf24. External. The builtin type ID 48 (MOV W0, #0x30).
OCFTypeID OCFTimeZoneGetTypeID(void) {
    return 48;
}

// Native CFTimeZoneCopyDefault, 0x1c02acfa8. External. Inside an autorelease pool, +[NSTimeZone
// defaultTimeZone] is retained with CFRetain unless it is nil.
OCFTimeZoneRef OCFTimeZoneCopyDefault(void) {
    void *pool = _OCFAutoreleasePoolPush();
    id timeZone = ((id (*)(id, SEL))objc_msgSend)((id)timeZoneClass, sel_getUid("defaultTimeZone"));
    OCFTimeZoneRef result = NULL;
    if (timeZone != nil) {
        result = (OCFTimeZoneRef)OCFRetain(timeZone);
    }
    _OCFAutoreleasePoolPop(pool);
    return result;
}

// Native CFTimeZoneCreateWithTimeIntervalFromGMT, 0x1c02ad154. External. Inside an autorelease pool,
// +[NSTimeZone _timeZoneWithSecondsFromGMT:] with the interval rounded toward negative infinity to a 64-bit
// integer (FCVTMS; floor and the saturating conversion compile to the same result on arm64), retained with
// objc_retain (also for nil); the allocator is not read.
OCFTimeZoneRef OCFTimeZoneCreateWithTimeIntervalFromGMT(OCFAllocatorRef allocator, OCFTimeInterval ti) {
    (void)allocator;
    void *pool = _OCFAutoreleasePoolPush();
    int64_t seconds = (int64_t)floor(ti);
    id timeZone = ((id (*)(id, SEL, int64_t))objc_msgSend)(
        (id)timeZoneClass, sel_getUid("_timeZoneWithSecondsFromGMT:"), seconds
    );
    OCFTimeZoneRef result = (OCFTimeZoneRef)objc_retain(timeZone);
    _OCFAutoreleasePoolPop(pool);
    return result;
}

// Native CFTimeZoneGetName, 0x1c02ad218. External; a tail call of -name.
OCFStringRef OCFTimeZoneGetName(OCFTimeZoneRef tz) {
    return ((OCFStringRef (*)(id, SEL))objc_msgSend)((id)tz, sel_getUid("name"));
}

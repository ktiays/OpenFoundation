//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Locale/OCFCalendarEntryPoints.h"

#include <objc/message.h>
#include <objc/runtime.h>

// Inferred organization: the CFCalendar entry points restored here lie in one iOS Simulator __text region
// (CFCalendarGetTypeID 0x1c01ced18 through CFCalendarSetMinimumDaysInFirstWeek 0x1c01ceda0, then
// CFCalendarCreateDateFromComponents 0x1c01cfa00 and CFCalendarCreateDateComponentsFromDate 0x1c01cfa7c
// further along the same unit), with the unit's other entry points (outside the scope) between them; they are
// defined in native address order. In this image CFCalendar is a thin layer over NSCalendar (an Objective-C
// class of the image, loaded from the class reference 0x2227ce2b8); the CFCalendar unit's other restored
// functions are in Locale/OCFCalendar.c, which belongs to another family. The native source path is not in
// the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full); iOS Device 24A446:
// unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json and, for CFCalendarGetTypeID,
// _CFCalendarCreateCopy, CFCalendarSetTimeZone and the two component conversions,
// Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json.

// libobjc's objc_alloc, objc_retain and objc_release (no SDK declarations in MRC); supplied.
extern id objc_alloc(Class cls);
extern id objc_retain(id value);
extern void objc_release(id value);

// The class the entry points message (NSCalendar, restored in Objective-C/OCFNSCalendar.m).
extern char calendarClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSCalendar));

// The NSDateComponents class reference (off_2227CE2E0), restored in Objective-C/OCFNSDateComponents.m.
extern char calendarDateComponentsClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSDateComponents));

// Native CFCalendarGetTypeID, 0x1c01ced18. External. The builtin type ID 49 (MOV W0, #0x31).
OCFTypeID OCFCalendarGetTypeID(void) {
    return 49;
}

// Native CFCalendarCreateWithIdentifier, 0x1c01ced3c. External; a tail call of
// +[NSCalendar _newCalendarWithIdentifier:] (the allocator is not read).
OCFCalendarRef OCFCalendarCreateWithIdentifier(OCFAllocatorRef allocator, OCFCalendarIdentifier identifier) {
    (void)allocator;
    return ((OCFCalendarRef (*)(id, SEL, OCFCalendarIdentifier))objc_msgSend)(
        (id)calendarClass, sel_getUid("_newCalendarWithIdentifier:"), identifier
    );
}

// Native _CFCalendarCreateCopy, 0x1c01ced4c. Hidden (was a private external); a tail call of -copy on the
// calendar (the allocator is not read).
OCFCalendarRef _CFCalendarCreateCopy(OCFAllocatorRef allocator, OCFCalendarRef calendar) {
    (void)allocator;
    return ((OCFCalendarRef (*)(id, SEL))objc_msgSend)((id)calendar, sel_getUid("copy"));
}

// Native CFCalendarGetIdentifier, 0x1c01ced54. External; a tail call of -calendarIdentifier.
OCFCalendarIdentifier OCFCalendarGetIdentifier(OCFCalendarRef calendar) {
    return ((OCFCalendarIdentifier (*)(id, SEL))objc_msgSend)((id)calendar, sel_getUid("calendarIdentifier"));
}

// Native CFCalendarSetTimeZone, 0x1c01ced88. External; a tail call of -setTimeZone:.
void OCFCalendarSetTimeZone(OCFCalendarRef calendar, OCFTimeZoneRef tz) {
    ((void (*)(id, SEL, OCFTimeZoneRef))objc_msgSend)((id)calendar, sel_getUid("setTimeZone:"), tz);
}

// Native CFCalendarGetFirstWeekday, 0x1c01ced90. External; a tail call of -firstWeekday.
OCFIndex OCFCalendarGetFirstWeekday(OCFCalendarRef calendar) {
    return ((OCFIndex (*)(id, SEL))objc_msgSend)((id)calendar, sel_getUid("firstWeekday"));
}

// Native CFCalendarSetFirstWeekday, 0x1c01ced94. External; a tail call of -setFirstWeekday:.
void OCFCalendarSetFirstWeekday(OCFCalendarRef calendar, OCFIndex wkdy) {
    ((void (*)(id, SEL, OCFIndex))objc_msgSend)((id)calendar, sel_getUid("setFirstWeekday:"), wkdy);
}

// Native CFCalendarGetMinimumDaysInFirstWeek, 0x1c01ced9c. External; a tail call of -minimumDaysInFirstWeek.
OCFIndex OCFCalendarGetMinimumDaysInFirstWeek(OCFCalendarRef calendar) {
    return ((OCFIndex (*)(id, SEL))objc_msgSend)((id)calendar, sel_getUid("minimumDaysInFirstWeek"));
}

// Native CFCalendarSetMinimumDaysInFirstWeek, 0x1c01ceda0. External; a tail call of
// -setMinimumDaysInFirstWeek:.
void OCFCalendarSetMinimumDaysInFirstWeek(OCFCalendarRef calendar, OCFIndex mwd) {
    ((void (*)(id, SEL, OCFIndex))objc_msgSend)((id)calendar, sel_getUid("setMinimumDaysInFirstWeek:"), mwd);
}

// Native CFCalendarCreateDateFromComponents, 0x1c01cfa00. External. A new NSDateComponents (objc_alloc)
// initialized with -_initWithCFDateComponents: (a copy of the components), the calendar's
// -dateFromComponents: result retained (objc_retain), the NSDateComponents released; the allocator is not
// read.
OCFDateRef OCFCalendarCreateDateFromComponents(
    OCFAllocatorRef allocator, OCFCalendarRef calendar, OCFDateComponentsRef dateComp
) {
    (void)allocator;
    id components = objc_alloc((Class)calendarDateComponentsClass);
    components = ((id (*)(id, SEL, OCFDateComponentsRef))objc_msgSend)(
        components, sel_getUid("_initWithCFDateComponents:"), dateComp
    );
    id date =
        ((id (*)(id, SEL, id))objc_msgSend)((id)calendar, sel_getUid("dateFromComponents:"), components);
    OCFDateRef result = (OCFDateRef)objc_retain(date);
    objc_release(components);
    return result;
}

// Native CFCalendarCreateDateComponentsFromDate, 0x1c01cfa7c. External. The calendar's
// -components:fromDate: result's -_dateComponents, retained with CFRetain (a tail call); the allocator is not
// read.
OCFDateComponentsRef OCFCalendarCreateDateComponentsFromDate(
    OCFAllocatorRef allocator, OCFCalendarRef calendar, OCFCalendarUnit unitFlags, OCFDateRef date
) {
    (void)allocator;
    id components = ((id (*)(id, SEL, OCFCalendarUnit, OCFDateRef))objc_msgSend)(
        (id)calendar, sel_getUid("components:fromDate:"), unitFlags, date
    );
    OCFDateComponentsRef dateComp =
        ((OCFDateComponentsRef (*)(id, SEL))objc_msgSend)(components, sel_getUid("_dateComponents"));
    return (OCFDateComponentsRef)OCFRetain(dateComp);
}

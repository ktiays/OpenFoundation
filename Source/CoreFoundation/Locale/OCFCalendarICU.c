//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Locale/OCFCalendarICU.h"

#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Date/OCFTimeZoneEntryPoints.h"
#include "CoreFoundation/Locale/OCFLocaleComponents.h"
#include "CoreFoundation/Locale/OCFLocaleEntryPoints.h"
#include "CoreFoundation/String/OCFString.h"

#include <stdint.h>

// Inferred organization: __CFCalendarCreateUCalendar (0x1c02aa958) lies in the iOS Simulator __text section
// after the CFCalendar notification functions that Locale/OCFCalendar.c restores (0x1c02a8118-0x1c02a833c);
// that file belongs to another family, so this function has its own translation unit. The native source path
// is not in the evidence. iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full); iOS Device 24A446: unread. Reading: not yet written; the evidence is the
// assembly in Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json.

// ICU (libicucore, linked by the native image); supplied, never reimplemented. Declared here because the SDK
// installs no ucal.h. UCalendarType and UErrorCode are 32-bit enumerations; UCalendar is opaque.
extern void *ucal_open(const UniChar *zoneID, int32_t len, const char *locale, int32_t type, int32_t *status);

// Native __CFCalendarCreateUCalendar, 0x1c02aa958. Hidden (was a private external).
// With a calendar identifier, the locale identifier is rebuilt with kCFLocaleCalendarIdentifierKey set to it
// (components and identifier with kCFAllocatorSystemDefault). The locale identifier's UTF-8 C string is its
// CFStringGetCStringPtr or a copy into a 512-byte stack buffer; when the copy fails the result is NULL. The
// time zone name's first min(length, 512) UTF-16 units are copied into a 1024-byte buffer and passed with
// that count to ucal_open (UCAL_DEFAULT, 0; the status starts at zero and is not read). The rebuilt
// identifier is released on both paths.
void *__CFCalendarCreateUCalendar(OCFStringRef calendarID, OCFStringRef localeID, OCFTimeZoneRef tz) {
    if (calendarID != NULL) {
        OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
        OCFDictionaryRef components = OCFLocaleCreateComponentsFromLocaleIdentifier(allocator, localeID);
        OCFMutableDictionaryRef mutableComponents = OCFDictionaryCreateMutableCopy(allocator, 0, components);
        OCFDictionarySetValue(mutableComponents, kOCFLocaleCalendarIdentifierKey, calendarID);
        localeID = OCFLocaleCreateLocaleIdentifierFromComponents(allocator, mutableComponents);
        OCFRelease(mutableComponents);
        OCFRelease(components);
    }
    char buffer[512];
    const char *cLocaleID = OCFStringGetCStringPtr(localeID, kOCFStringEncodingUTF8);
    if (cLocaleID == NULL) {
        cLocaleID = buffer;
        if (!OCFStringGetCString(localeID, buffer, 512, kOCFStringEncodingUTF8)) {
            if (calendarID != NULL) {
                OCFRelease(localeID);
            }
            return NULL;
        }
    }
    UniChar zoneName[512];
    OCFStringRef name = OCFTimeZoneGetName(tz);
    OCFIndex length = OCFStringGetLength(name);
    OCFIndex zoneLength = (uint64_t)length < 0x200 ? length : 0x200;
    OCFStringGetCharacters(name, (OCFRange){0, zoneLength}, zoneName);
    int32_t status = 0;
    void *calendar = ucal_open(zoneName, (int32_t)zoneLength, cLocaleID, 0, &status);
    if (calendarID != NULL) {
        OCFRelease(localeID);
    }
    return calendar;
}

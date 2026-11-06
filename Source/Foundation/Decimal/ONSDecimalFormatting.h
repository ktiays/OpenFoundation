//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenFoundation/Decimal/ONSDecimal.h>

#include "CoreFoundation/Locale/OCFLocaleKeys.h"

#include <string.h>

#ifdef __OBJC__
OF_EXTERN id _ONSDecimalString(const ONSDecimal *number, id locale);
#endif

// Native Locale metadata 0x2227ea0d8, Foundation iOS Simulator 24A434,
// UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03. The selected module supplies the metadata.
// The value witness table records a size and stride of 16, aligned to eight bytes.
#if OF_SYSTEM_NAMES
extern const unsigned char decimalLocaleMetadataSymbol[] __asm("_$s10Foundation6LocaleVN");
#else
extern const unsigned char decimalLocaleMetadataSymbol[] __asm("_$s14OpenFoundation6LocaleVN");
#endif

static inline const void *decimalLocaleTypeMetadata(void) {
    return decimalLocaleMetadataSymbol;
}

extern const void *swift_getObjectType(const void *object);

// The witness at byte 0xe0 of the class-bound existential's witness table takes the
// object metadata in x0, the witness table in x1, and the borrowed object in Swift's context register.
// It returns an owned Optional<String> in x0 and x1. Copy those words into uninitialized Swift storage.
typedef struct {
    uintptr_t words[2];
} DecimalLocaleStringResult;

typedef DecimalLocaleStringResult (*DecimalLocaleSeparatorWitness)(
    const void *type, const void *table, const void *object __attribute__((swift_context))
) __attribute__((swiftcall));

static inline void decimalLocaleReadSeparator(const void *locale, void *result) {
    const void *fields[2];
    memcpy(fields, locale, sizeof(fields));
    const void *type = swift_getObjectType(fields[0]);
    DecimalLocaleSeparatorWitness witness;
    memcpy(&witness, (const unsigned char *)fields[1] + 0xe0, sizeof(witness));
    DecimalLocaleStringResult separator = witness(type, fields[1], fields[0]);
    memcpy(result, &separator, sizeof(separator));
}

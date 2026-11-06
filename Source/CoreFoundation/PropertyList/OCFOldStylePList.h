//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/PropertyList/OCFPropertyList.h"
#include "CoreFoundation/StringEncoding/OCFStringEncodingConverter.h"

#include <stdbool.h>
#include <stdint.h>

// The old-style (OpenStep) property-list and strings-file parser context, iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for every offset (the stores of
// __CFCreateOldStylePropertyListOrStringsFile into its stack context at SP+0x10 and the accesses of the
// parse functions): pointers to UTF-16 units for the first unit (0), the current position (8) and the end
// (16); the pending parse error (24), replaced or cleared by the parse functions; the allocator of the
// created objects (32); the mutability option truncated to 32 bits (40, a 32-bit store after an 8-byte zero
// store, read with 32-bit loads and compared with 2, kCFPropertyListMutableContainersAndLeaves); four bytes
// that only that zero store writes (44); and the set that uniques immutable strings (48). The context is 56
// bytes. OldStylePlistParseContext and its field names are local descriptive spellings.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md (leaf helpers); the parse
// functions: Local/Scratch/cf-core/tasks/plist.txt (sub-track reading; no versioned reading yet).
typedef struct {
    const uint16_t *begin;
    const uint16_t *curr;
    const uint16_t *end;
    OCFErrorRef error;
    OCFAllocatorRef allocator;
    uint32_t mutabilityOption;
    uint32_t unused;
    OCFMutableSetRef stringSet;
} OldStylePlistParseContext;

_Static_assert(sizeof(OldStylePlistParseContext) == 56, "The native parse context is 56 bytes.");
_Static_assert(__builtin_offsetof(OldStylePlistParseContext, error) == 24, "Error at 24.");
_Static_assert(__builtin_offsetof(OldStylePlistParseContext, allocator) == 32, "Allocator at 32.");
_Static_assert(__builtin_offsetof(OldStylePlistParseContext, mutabilityOption) == 40, "Option at 40.");
_Static_assert(__builtin_offsetof(OldStylePlistParseContext, stringSet) == 48, "String set at 48.");

// Original non-external names are preserved; no process-shared symbol is exported.
OF_EXTERN bool advanceToNonSpace(OldStylePlistParseContext *context);

OF_EXTERN uint32_t lineNumberStrings(const OldStylePlistParseContext *context);

// The old-style parser entry point (non-external, was a private external). The definition (iOS Simulator
// 24A434, 0x1c02d4b0c, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC) confirms every parameter: guessedEncoding
// is read as 32 bits, option is truncated to 32 bits and *format receives a 64-bit store.
OF_EXTERN OCFTypeRef __CFCreateOldStylePropertyListOrStringsFile(
    OCFAllocatorRef allocator,
    OCFDataRef xmlData,
    OCFStringRef originalString,
    OCFStringEncoding guessedEncoding,
    OCFOptionFlags option,
    OCFErrorRef *outError,
    OCFPropertyListFormat *format
);

// The string-uniquing helper of this unit (non-external natively): called with the context, the first unit
// and the unit count; its result is returned to the parse functions' callers as a new reference.
OF_EXTERN OCFStringRef
_uniqueStringForCharacters(OldStylePlistParseContext *context, const UniChar *base, OCFIndex length);

//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdbool.h>
#include <stdint.h>

// Leading fields of the old-style (OpenStep) property-list and strings-file parser context as
// lineNumberStrings and advanceToNonSpace read them, iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for these three 64-bit pointers to
// UTF-16 units: the first unit (offset 0), the current position (offset 8, the only field written, by
// advanceToNonSpace) and the end (offset 16). Later fields are unread and not declared; the producer
// (__CFCreateOldStylePropertyListOrStringsFile) is unread. OldStylePlistParseContext and its field
// names are local descriptive spellings.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
typedef struct {
    const uint16_t *begin;
    const uint16_t *curr;
    const uint16_t *end;
} OldStylePlistParseContext;

// Original non-external names are preserved; no process-shared symbol is exported.
OF_EXTERN bool advanceToNonSpace(OldStylePlistParseContext *context);

OF_EXTERN uint32_t lineNumberStrings(const OldStylePlistParseContext *context);

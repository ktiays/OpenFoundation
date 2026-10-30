//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdbool.h>
#include <stdint.h>

// Leading fields of the XML property-list parser context as lineNumber reads them, iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified for these three 64-bit
// pointers: the first byte (offset 0), the current position (offset 8) and the end (offset 16). Later
// fields are unread and not declared; the producer (__CFPropertyListCreateFromUTF8Data) is unread.
// XMLPlistParseContext and its field names are local descriptive spellings.
// Reading: Documentation/Reference/Research/CoreFoundationLeafHelpers.md.
typedef struct {
    const uint8_t *begin;
    const uint8_t *curr;
    const uint8_t *end;
} XMLPlistParseContext;

// Original names are preserved. __CFPropertyListKeyBufferMatchesFilterBuffer was a private external
// and lineNumber is non-external; no process-shared symbol is exported.
OF_EXTERN bool __CFPropertyListKeyBufferMatchesFilterBuffer(
    const char *key, OCFIndex keyLength, const char *filter, OCFIndex filterLength
);

OF_EXTERN uint32_t lineNumber(const XMLPlistParseContext *context);

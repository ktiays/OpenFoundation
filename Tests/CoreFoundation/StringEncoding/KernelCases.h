//
//  Created by ktiays on 2026/10/29.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/StringEncoding/OCFSingleByte.h"
#include "CoreFoundation/StringEncoding/OCFUTF8.h"

#include <stddef.h>

// Native __CFToUTF8 / __CFToUTF8Len: iOS Simulator 24A434,
// 0x1c0168fe4 / 0x1c0169564, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// iOS Device 24A446, 0x180650f64 / 0x18072755c,
// UUID 9426FC9A-B36C-3096-B836-D17D6744A75B; confidence: verified.
typedef struct {
    const char *name;
    uint16_t input[8];
    OCFIndex count;
    uint32_t flags;
    OCFIndex capacity;
    OCFIndex consumed;
    OCFIndex used;
    uint8_t bytes[24];
} UTF8KernelCase;

static const UTF8KernelCase kUTF8KernelCases[] = {
    {"empty", {0}, 0, 0x0, 0, 0, 0, {0}},
    {"ASCII and zero", {0x0041, 0x0000}, 2, 0x0, 2, 2, 2, {0x41, 0x00}},
    {"two-byte cent", {0x00a2}, 1, 0x0, 2, 1, 2, {0xc2, 0xa2}},
    {"three-byte euro", {0x20ac}, 1, 0x0, 3, 1, 3, {0xe2, 0x82, 0xac}},
    {"supplementary pair", {0xd83d, 0xde00}, 2, 0x0, 4, 2, 4, {0xf0, 0x9f, 0x98, 0x80}},
    {"short supplementary destination", {0xd83d, 0xde00}, 2, 0x0, 3, 0, 0, {0}},
    {"short BMP destination", {0x20ac}, 1, 0x0, 2, 0, 0, {0}},
    {"mixed prefix before euro", {0x0041, 0x20ac, 0xd83d, 0xde00, 0x005a}, 5, 0x0, 3, 1, 1, {0x41}},
    {"mixed prefix before pair",
     {0x0041, 0x20ac, 0xd83d, 0xde00, 0x005a},
     5,
     0x0,
     4,
     2,
     4,
     {0x41, 0xe2, 0x82, 0xac}},
    {"mixed prefix after pair",
     {0x0041, 0x20ac, 0xd83d, 0xde00, 0x005a},
     5,
     0x0,
     8,
     4,
     8,
     {0x41, 0xe2, 0x82, 0xac, 0xf0, 0x9f, 0x98, 0x80}},
    {"mixed complete",
     {0x0041, 0x20ac, 0xd83d, 0xde00, 0x005a},
     5,
     0x0,
     9,
     5,
     9,
     {0x41, 0xe2, 0x82, 0xac, 0xf0, 0x9f, 0x98, 0x80, 0x5a}},
    {"length-only pair", {0xd83d, 0xde00}, 2, 0x0, 0, 2, 4, {0}},
    {"strict high surrogate", {0xd800}, 1, 0x0, 8, 0, 0, {0}},
    {"strict low surrogate", {0xdc00}, 1, 0x0, 8, 0, 0, {0}},
    {"permitted high surrogate", {0xd800}, 1, 0x80, 3, 1, 3, {0xed, 0xa0, 0x80}},
    {"permitted low surrogate", {0xdc00}, 1, 0x80, 3, 1, 3, {0xed, 0xb0, 0x80}},
    {"strict unpaired prefix", {0xd800, 0x0041}, 2, 0x0, 8, 0, 0, {0}},
    {"permitted unpaired prefix", {0xd800, 0x0041}, 2, 0x80, 4, 2, 4, {0xed, 0xa0, 0x80, 0x41}},
    {"BMP noncharacter", {0xffff}, 1, 0x0, 3, 1, 3, {0xef, 0xbf, 0xbf}},
    {"BOM copied once", {0xfeff}, 1, 0xffffffff, 3, 1, 3, {0xef, 0xbb, 0xbf}},
    {"length-only strict failure", {0x0041, 0xd800}, 2, 0x0, 0, 1, 1, {0}},
    {"length-only permitted surrogate", {0x0041, 0xd800}, 2, 0x80, 0, 2, 4, {0}},
};

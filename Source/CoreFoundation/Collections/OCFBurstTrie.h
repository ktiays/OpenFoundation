//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Collections/OCFDictionary.h"

#include <OpenCoreFoundation/Base/OCFBase.h>

#include <stdbool.h>
#include <stdint.h>

// The private CFBurstTrie interface (native externals; no SDK header). Signatures from the native register
// use of the restored functions and their callers.
typedef struct __OCFBurstTrie *OCFBurstTrieRef;

OF_EXPORT OCFBurstTrieRef OCFBurstTrieCreateWithOptions(OCFDictionaryRef options);
OF_EXPORT OCFBurstTrieRef OCFBurstTrieCreate(void);
OF_EXPORT Boolean OCFBurstTrieAddUTF8StringWithWeight(
    OCFBurstTrieRef trie, const UInt8 *key, OCFIndex length, uint32_t weight, uint32_t payload
);
OF_EXPORT Boolean
OCFBurstTrieAddUTF8String(OCFBurstTrieRef trie, const UInt8 *key, OCFIndex length, uint32_t payload);
OF_EXPORT Boolean
OCFBurstTrieContainsUTF8String(OCFBurstTrieRef trie, const UInt8 *key, OCFIndex length, uint32_t *payload);
OF_EXPORT void OCFBurstTrieRelease(OCFBurstTrieRef trie);

// The cursor of file-mapped tries (struct __OCFBurstTrieCursor, 33 bytes, defined in OCFBurstTrie.c). Callers
// allocate it themselves; CFBurstTrieContainsUTF8String keeps a zeroed one on its stack.
typedef struct __OCFBurstTrieCursor *OCFBurstTrieCursorRef;

OF_EXPORT Boolean OCFBurstTrieSetCursorForBytes(
    OCFBurstTrieRef trie, OCFBurstTrieCursorRef cursor, const UInt8 *bytes, OCFIndex length
);
OF_EXPORT Boolean
OCFBurstTrieCursorAdvanceForBytes(OCFBurstTrieCursorRef cursor, const UInt8 *bytes, OCFIndex length);

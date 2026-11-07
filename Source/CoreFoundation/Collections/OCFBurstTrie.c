//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Collections/OCFBurstTrie.h"

#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <malloc/malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

// Inferred organization: CFBurstTrieCreateWithOptions (0x1c02cf74c) through finalizeCFBurstTrie (0x1c02d2e28)
// are contiguous in the iOS Simulator __text section, with CFBurstTrieAddUTF8StringWithWeight.cold.1
// (0x1c03122cc), and form the native burst-trie unit; the native source path is not in the evidence. This
// translation unit holds, in native order, the in-memory group and the file-mapped lookup, cursor and
// traversal group that the in-memory entry points reach (burstTrieMappedFind, CFBurstTrieSetCursorForBytes,
// CFBurstTrieCursorAdvanceForBytes, advanceMapCursor, the mapped and compact mapped find and traverse
// functions, advanceCursorMappedLevel, burstTrieCompactTrieMappedFind and burstTrieMappedPageFind). No
// restored function creates a mapped trie (CFBurstTrieCreateFromFile and CFBurstTrieCreateFromMapBytes are
// outside the scope); the mapped group is reached only for a trie whose map base is set. Unless a definition
// says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified; iOS
// Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationCollections.md (to be
// written from this unit's comments).

// A level holds 256 tagged slots (low bits 1: a child level, 2: a list of nodes) and the weight and payload
// of the key ending here (0x808 bytes). The trie (0x830 bytes) is its root level followed by the map base
// (0x808), the map size (0x810, 32-bit), the flags (0x818, 32-bit; bit 3 file-based, bits 3-4 cursor
// capable), the key count (0x81c), the list limit before a list bursts (0x820), the reference count (0x824)
// and the mapped-by-file byte (0x828). A list node holds the next node, the weight, the payload, the
// remaining key length (16-bit) and the remaining key bytes, zero terminated. Field names are local
// descriptive spellings.
typedef struct {
    uintptr_t slots[256];
    uint32_t weight;
    uint32_t payload;
} BurstTrieLevel;

typedef struct BurstTrieListNode {
    struct BurstTrieListNode *next;
    uint32_t weight;
    uint32_t payload;
    uint16_t length;
    uint8_t string[];
} BurstTrieListNode;

struct __OCFBurstTrie {
    BurstTrieLevel root;
    void *mapBase;
    uint32_t mapSize;
    uint32_t unread0;
    uint32_t cflags;
    uint32_t count;
    uint32_t containerSize;
    uint32_t retain;
    uint8_t mappedByFile;
};

OF_ABI_ASSERT(sizeof(BurstTrieLevel) == 0x808, "A burst-trie level is 0x808 bytes.");
OF_ABI_ASSERT(__builtin_offsetof(BurstTrieListNode, string) == 0x12, "List node keys start at 0x12.");
OF_ABI_ASSERT(__builtin_offsetof(struct __OCFBurstTrie, mappedByFile) == 0x828, "Mapped byte at 0x828.");
_Static_assert(sizeof(struct __OCFBurstTrie) <= 0x830, "The trie is a 0x830-byte allocation.");

// The traversal cursor of in-memory tries, on traverseCFBurstTrieWithCursor's stack: the current slot
// (tagged) at 0x10, the matched prefix length at 0x18, the searched key's length at 0x1c, the searched key at
// 0x20 and the key built so far at 0x28. Local descriptive spelling.
typedef struct {
    uint8_t unread0[16];
    uintptr_t next;
    uint32_t prefixLength;
    uint32_t targetLength;
    const UInt8 *key;
    uint8_t keyBuffer[4104];
} BurstTrieTraversalCursor;

// The cursor of mapped tries at the same stack slot: the current slot word (offset and tag) at 0x10, the
// searched key's length at 0x14, the matched prefix length at 0x18, the searched key at 0x1c (unaligned) and
// the key built so far at 0x24 (findCFBurstTrieMappedLevel and the functions it reaches). Bytes 0x00-0x0f are
// zeroed by traverseCFBurstTrieWithCursor and otherwise unread. Local descriptive spelling.
typedef struct __attribute__((packed)) {
    uint8_t unread0[16];
    uint32_t offset;
    uint32_t length;
    uint32_t prefixLength;
    const UInt8 *key;
    uint8_t keyBuffer[4108];
} BurstTrieMappedTraversalCursor;

OF_ABI_ASSERT(__builtin_offsetof(BurstTrieMappedTraversalCursor, keyBuffer) == 0x24, "Mapped key at 0x24.");

// The file-mapped trie, as the mapped group reads it through the map base (offsets from the base, all
// little-endian 32-bit words unless stated):
// - Header: 0x00 the magic 0xBABEFACE (tested only by CFBurstTrieContainsUTF8String), 0x04 the root offset
//   (a slot word of a mapped level). The rest of the header is unread.
// - Slot word: an offset from the map base with the tag in its low two bits: 0 empty, 1 mapped level, 2 page,
//   3 compact level.
// - Mapped level: 256 slot words indexed by the key byte (0x000-0x3ff), then two words. burstTrieMappedFind
//   tests 0x400 for nonzero and answers 0x404 as the payload; the traversal and cursor functions answer 0x400
//   as the payload and never read 0x404.
// - Compact level: a 256-bit presence bitmap (four 64-bit words, 0x00-0x1f, bit n of word n / 64 for byte n),
//   then the slot words of the present bytes in byte order, indexed by the number of present bytes below.
//   burstTrieCompactTrieMappedFind tests 0x20 for nonzero, answers 0x24 as the payload and reads the slot
//   words from 0x28; the traversal and cursor functions answer 0x20 as the payload and read the slot words
//   from 0x24. The native readers disagree by one word; each function below follows its own assembly.
// - Page: 0x00 the page size, then entries from 0x04. burstTrieMappedPageFind and findCFBurstTrieMappedPage
//   walk while the entry offset (from 0x04) is below the size; advanceMapCursor walks while it is below the
//   size minus 4. Entries are plain when flags bit 3 is clear: the key length (16-bit), the payload at 2
//   (unaligned) and the key bytes at 6. With flags bit 3 set they are prefix-compressed: the number of bytes
//   shared with the previous entry's key (8-bit), the suffix length at 1 (16-bit, unaligned), the payload at
//   3 (unaligned) and the suffix at 7.
// The trie's flags at 0x818: bit 3 selects prefix-compressed pages (and is the file-based trie of
// traverseCFBurstTrieWithCursor's message); bit 4 enables cursor advance over plain pages; either one enables
// the cursor (CFBurstTrieSetCursorForBytes).

// The cursor of mapped tries (CFBurstTrieCursor, 33 bytes, packed): the current slot word at 0x00, the entry
// offset within the current page at 0x04, the number of key bytes matched in that entry at 0x08, the payload
// at 0x0c, the positioned-on-an-entry byte at 0x10, the cursor kind at 0x11 (64-bit, unaligned; 1 for a
// mapped trie, the only kind the mapped group sets or accepts) and the trie at 0x19 (unaligned). Field names
// are local descriptive spellings.
struct __attribute__((packed)) __OCFBurstTrieCursor {
    uint32_t slot;
    uint32_t entryOffset;
    uint32_t matched;
    uint32_t payload;
    uint8_t inPage;
    uint64_t kind;
    OCFBurstTrieRef trie;
};

OF_ABI_ASSERT(sizeof(struct __OCFBurstTrieCursor) == 0x21, "The mapped cursor is 33 bytes.");
_Static_assert(__builtin_offsetof(struct __OCFBurstTrieCursor, kind) == 0x11, "The cursor kind is at 0x11.");
_Static_assert(__builtin_offsetof(struct __OCFBurstTrieCursor, trie) == 0x19, "The cursor trie is at 0x19.");

static inline uint16_t readMapU16(const uint8_t *address) {
    uint16_t value;
    memcpy(&value, address, sizeof(value));
    return value;
}

static inline uint32_t readMapU32(const uint8_t *address) {
    uint32_t value;
    memcpy(&value, address, sizeof(value));
    return value;
}

static inline uint64_t readMapU64(const uint8_t *address) {
    uint64_t value;
    memcpy(&value, address, sizeof(value));
    return value;
}

typedef Boolean (*BurstTrieTraversalCallback)(
    void *context, const UInt8 *key, uint32_t payload, Boolean exact
);

enum {
    kBurstTrieSlotLevel = 1,
    kBurstTrieSlotList = 2,
};

static Boolean burstTrieMappedFind(
    const uint8_t *level,
    const uint8_t *map,
    const UInt8 *key,
    uint32_t length,
    uint32_t *payload,
    Boolean prefixCompressed
);
static void traverseCFBurstTrieWithCursor(
    OCFBurstTrieRef trie,
    const UInt8 *key,
    uint32_t length,
    Boolean prefix,
    void *context,
    BurstTrieTraversalCallback callback
);
static Boolean containsKey(void *context, const UInt8 *key, uint32_t payload, Boolean exact);
static BurstTrieListNode *
makeCFBurstTrieListNode(const UInt8 *string, uint32_t length, uint32_t weight, uint32_t payload);
static void
findCFBurstTrieList(BurstTrieTraversalCursor *cursor, void *context, BurstTrieTraversalCallback callback);
static void traverseCFBurstTrieLevel(
    BurstTrieLevel *level,
    BurstTrieTraversalCursor *cursor,
    Boolean prefix,
    void *context,
    BurstTrieTraversalCallback callback
);
static void finalizeCFBurstTrie(BurstTrieLevel *level);
static Boolean
advanceMapCursor(OCFBurstTrieRef trie, OCFBurstTrieCursorRef cursor, const UInt8 *bytes, OCFIndex length);
static void findCFBurstTrieMappedLevel(
    OCFBurstTrieRef trie,
    BurstTrieMappedTraversalCursor *cursor,
    Boolean prefix,
    void *context,
    BurstTrieTraversalCallback callback
);
static void findCFBurstTrieCompactMappedLevel(
    OCFBurstTrieRef trie,
    BurstTrieMappedTraversalCursor *cursor,
    Boolean prefix,
    void *context,
    BurstTrieTraversalCallback callback
);
static void findCFBurstTrieMappedPage(
    OCFBurstTrieRef trie,
    BurstTrieMappedTraversalCursor *cursor,
    void *context,
    BurstTrieTraversalCallback callback
);
static void traverseCFBurstTrieMappedLevel(
    OCFBurstTrieRef trie,
    const uint8_t *level,
    BurstTrieMappedTraversalCursor *cursor,
    Boolean prefix,
    void *context,
    BurstTrieTraversalCallback callback
);
static void traverseCFBurstTrieCompactMappedLevel(
    OCFBurstTrieRef trie,
    const uint8_t *level,
    BurstTrieMappedTraversalCursor *cursor,
    Boolean prefix,
    void *context,
    BurstTrieTraversalCallback callback
);
static Boolean advanceCursorMappedLevel(
    OCFBurstTrieRef trie, OCFBurstTrieCursorRef cursor, const UInt8 *bytes, OCFIndex length
);
static Boolean burstTrieCompactTrieMappedFind(
    const uint8_t *level,
    const uint8_t *map,
    const UInt8 *key,
    uint32_t length,
    uint32_t *payload,
    Boolean prefixCompressed
);
static Boolean burstTrieMappedPageFind(
    const uint8_t *page, const UInt8 *key, uint32_t length, uint32_t *payload, Boolean prefixCompressed
);

// Native CFBurstTrieCreateWithOptions, 0x1c02cf74c. External. A zeroed trie (malloc_type_calloc, type
// 0x1010040873DE71C; not checked) with the list limit 256, or the options' "ContainerSize" (read as an int)
// when it lies in 3-4095, and a reference count of 1 (a release store).
OCFBurstTrieRef OCFBurstTrieCreateWithOptions(OCFDictionaryRef options) {
    OCFBurstTrieRef trie = malloc_type_calloc(1, 0x830, 0x1010040873DE71C);
    trie->containerSize = 0x100;
    const void *value = NULL;
    if (OCFDictionaryGetValueIfPresent(options, OF_STR("ContainerSize"), &value)) {
        int containerSize = 0;
        OCFNumberGetValue(value, kOCFNumberIntType, &containerSize);
        trie->containerSize = (uint32_t)containerSize - 3 < 0xFFD ? (uint32_t)containerSize : 0x100;
    }
    __atomic_store_n(&trie->retain, 1, __ATOMIC_RELEASE);
    return trie;
}

// Native CFBurstTrieCreate, 0x1c02cf820. External. A trie with "ContainerSize" 256: the number and a mutable
// dictionary (capacity 1, no callbacks) are made with kCFAllocatorDefault and released afterwards.
OCFBurstTrieRef OCFBurstTrieCreate(void) {
    int containerSize = 0x100;
    OCFNumberRef number = OCFNumberCreate(kOCFAllocatorDefault, kOCFNumberIntType, &containerSize);
    OCFMutableDictionaryRef options = OCFDictionaryCreateMutable(kOCFAllocatorDefault, 1, NULL, NULL);
    OCFDictionarySetValue(options, OF_STR("ContainerSize"), number);
    OCFBurstTrieRef trie = OCFBurstTrieCreateWithOptions(options);
    OCFRelease(number);
    OCFRelease(options);
    return trie;
}

// Native CFBurstTrieAddUTF8StringWithWeight, 0x1c02cfdcc, with .cold.1 (0x1c03122cc, the crash "list is
// NULL"). External. Keys of 4096 bytes or more, mapped tries and a zero payload answer false. The key
// descends through child levels; ending at a level adds the weight to it and sets the payload (a new key when
// the weight was 0). An empty slot gets a one-node list with the remaining bytes; a list adds the weight to
// the node with the same remaining bytes or appends a node, and when the walk passed more nodes than the list
// limit the list bursts into a new level (calloc 0x808, type 0x100004078B9B834): each node moves one byte
// down (a node with no bytes left becomes the level's weight and payload) and is freed. New keys increase the
// trie's count.
Boolean OCFBurstTrieAddUTF8StringWithWeight(
    OCFBurstTrieRef trie, const UInt8 *key, OCFIndex length, uint32_t weight, uint32_t payload
) {
    if (length >= 0x1000 || trie->mapBase != NULL || payload == 0) {
        return false;
    }
    BurstTrieLevel *level = &trie->root;
    uint32_t index = 0;
    uintptr_t slot = 0;
    if ((uint32_t)length != 0) {
        for (;;) {
            slot = level->slots[key[index]];
            if ((slot & 3) != kBurstTrieSlotLevel) {
                break;
            }
            level = (BurstTrieLevel *)(slot & ~(uintptr_t)3);
            ++index;
            if ((uint32_t)length == index) {
                goto atLevel;
            }
        }
        if ((slot & 3) != kBurstTrieSlotList) {
            BurstTrieListNode *node =
                makeCFBurstTrieListNode(key + index + 1, ~index + (uint32_t)length, weight, payload);
            level->slots[key[index]] = (uintptr_t)node | kBurstTrieSlotList;
            ++trie->count;
            return true;
        }
        BurstTrieListNode *list = (BurstTrieListNode *)(slot & ~(uintptr_t)3);
        if (list == NULL) {
            crashWithMessage("list is NULL");
        }
        const UInt8 *rest = key + index + 1;
        uint32_t last = (uint32_t)length - 1;
        uint64_t restLength = (uint64_t)last - index;
        uint32_t walked = 1;
        bool added;
        BurstTrieListNode *node = list;
        for (;;) {
            if (last - node->length == index && memcmp(rest, node->string, (size_t)restLength) == 0) {
                added = false;
                node->weight += weight;
                node->payload = payload;
                break;
            }
            ++walked;
            if (node->next == NULL) {
                node->next = makeCFBurstTrieListNode(rest, (uint32_t)restLength, weight, payload);
                added = true;
                break;
            }
            node = node->next;
        }
        if (walked > trie->containerSize) {
            BurstTrieLevel *burst = malloc_type_calloc(1, 0x808, 0x100004078B9B834);
            BurstTrieListNode *current = list;
            do {
                uint32_t nodeWeight = current->weight;
                uint32_t nodePayload = current->payload;
                if (current->length != 0) {
                    uintptr_t head = burst->slots[current->string[0]];
                    BurstTrieListNode *moved = makeCFBurstTrieListNode(
                        current->string + 1, (uint32_t)current->length - 1, nodeWeight, nodePayload
                    );
                    moved->weight = nodeWeight;
                    moved->next = (BurstTrieListNode *)(head & ~(uintptr_t)3);
                    burst->slots[current->string[0]] = (uintptr_t)moved | kBurstTrieSlotList;
                } else {
                    burst->weight = nodeWeight;
                    burst->payload = nodePayload;
                }
                BurstTrieListNode *next = current->next;
                free(current);
                current = next;
            } while (current != NULL);
            slot = (uintptr_t)burst | kBurstTrieSlotLevel;
        }
        level->slots[key[index]] = slot;
        if (added) {
            ++trie->count;
        }
        return true;
    }
atLevel:
    {
        uint32_t previous = level->weight;
        level->weight = previous + weight;
        level->payload = payload;
        if (previous == 0) {
            ++trie->count;
        }
        return true;
    }
}

// Native CFBurstTrieAddUTF8String, 0x1c02d0000. External. CFBurstTrieAddUTF8StringWithWeight with weight 1 (a
// tail call).
Boolean OCFBurstTrieAddUTF8String(OCFBurstTrieRef trie, const UInt8 *key, OCFIndex length, uint32_t payload) {
    return OCFBurstTrieAddUTF8StringWithWeight(trie, key, length, 1, payload);
}

// Native CFBurstTrieContainsUTF8String, 0x1c02d02e8. External. Keys longer than 1023 bytes answer false. A
// map with the magic 0xBABEFACE is searched by burstTrieMappedFind from the offset in its second word (a tail
// call, the key length truncated to 32 bits, the prefix-compressed flag from bit 3 of the flags); another map
// with cursor flags (bits 3-4) goes through a zeroed cursor (CFBurstTrieSetCursorForBytes answering true; the
// cursor kind must be 1 and its payload nonzero). Otherwise the in-memory traversal with containsKey records
// the exact match's payload; *payload receives a nonzero payload.
Boolean
OCFBurstTrieContainsUTF8String(OCFBurstTrieRef trie, const UInt8 *key, OCFIndex length, uint32_t *payload) {
    if (length > 0x3FF) {
        return false;
    }
    const uint32_t *map = trie->mapBase;
    if (map != NULL) {
        uint32_t flags = trie->cflags;
        if (map[0] == 0xBABEFACE) {
            return burstTrieMappedFind(
                (const uint8_t *)map + map[1],
                (const uint8_t *)map,
                key,
                (uint32_t)length,
                payload,
                (Boolean)((flags >> 3) & 1)
            );
        }
        if ((flags & 0x18) != 0) {
            struct __OCFBurstTrieCursor cursor;
            memset(&cursor, 0, sizeof(cursor));
            if (!OCFBurstTrieSetCursorForBytes(trie, &cursor, key, length)) {
                return false;
            }
            if (cursor.kind != 1) {
                return false;
            }
            uint32_t found = cursor.payload;
            if (found == 0) {
                return false;
            }
            if (payload != NULL) {
                *payload = found;
            }
            return true;
        }
    }
    uint32_t found = 0;
    traverseCFBurstTrieWithCursor(trie, key, (uint32_t)length, true, &found, containsKey);
    if (payload != NULL && found != 0) {
        *payload = found;
    }
    return found != 0;
}

// Native burstTrieMappedFind, 0x1c02d0600. Non-external. The key (its 32-bit length) descends through mapped
// levels; a key ending at a level answers whether the level's word at 0x400 is nonzero and stores the word at
// 0x404 through payload when payload is set. An empty slot answers false; a page continues with
// burstTrieMappedPageFind and a compact level with burstTrieCompactTrieMappedFind (tail calls, with the bytes
// after the slot's byte and the prefix-compressed flag that arrives in W5).
static Boolean burstTrieMappedFind(
    const uint8_t *level,
    const uint8_t *map,
    const UInt8 *key,
    uint32_t length,
    uint32_t *payload,
    Boolean prefixCompressed
) {
    if (length != 0) {
        uint32_t counter = 1 - length;
        const UInt8 *rest = key + 1;
        for (;;) {
            uint32_t slot = readMapU32(level + (size_t)rest[-1] * 4);
            uint32_t tag = slot & 3;
            if (tag != 1) {
                if (tag == 0) {
                    return false;
                }
                if (tag == 2) {
                    return burstTrieMappedPageFind(
                        map + (slot & ~3u), rest, -counter, payload, prefixCompressed
                    );
                }
                return burstTrieCompactTrieMappedFind(
                    map + (slot & ~3u), map, rest, -counter, payload, prefixCompressed
                );
            }
            level = map + (slot & ~3u);
            ++counter;
            ++rest;
            if (counter == 1) {
                break;
            }
        }
    }
    if (readMapU32(level + 0x400) == 0) {
        return false;
    }
    if (payload != NULL) {
        *payload = readMapU32(level + 0x404);
    }
    return true;
}

// Native CFBurstTrieSetCursorForBytes, 0x1c02d069c. External. A trie without a map, a negative length or
// flags without bits 3-4 answer false and leave the cursor alone. Otherwise the cursor gets the trie (0x19),
// kind 1 (0x11), the root slot word from the map's second word (0x00), and bytes 0x04-0x10 are zeroed (two
// 64-bit stores, at 0x09 then 0x04); no bytes or a zero length answer true, others answer
// CFBurstTrieCursorAdvanceForBytes (a tail call).
Boolean OCFBurstTrieSetCursorForBytes(
    OCFBurstTrieRef trie, OCFBurstTrieCursorRef cursor, const UInt8 *bytes, OCFIndex length
) {
    const uint8_t *map = trie->mapBase;
    if (map == NULL) {
        return false;
    }
    if (length < 0) {
        return false;
    }
    if ((trie->cflags & 0x18) == 0) {
        return false;
    }
    cursor->trie = trie;
    cursor->kind = 1;
    cursor->slot = readMapU32(map + 4);
    cursor->inPage = 0;
    cursor->payload = 0;
    cursor->matched = 0;
    cursor->entryOffset = 0;
    if (bytes == NULL || length == 0) {
        return true;
    }
    return OCFBurstTrieCursorAdvanceForBytes(cursor, bytes, length);
}

// Native traverseCFBurstTrieWithCursor, 0x1c02d0724. Non-external. A file-based map (flags bit 3) writes
// "Please use CFBurstTrieCursorRef API for file based trie.\n" to stderr; another map runs
// findCFBurstTrieMappedLevel from the root offset. In memory the key descends through child levels, the bytes
// copied into the cursor's key buffer: a list continues with findCFBurstTrieList (the prefix length one past
// the byte), an empty slot ends. At the key's level its payload is reported as an exact match; a callback
// answering true, or a prefix search whose prefix covers the key, ends; otherwise the level is traversed.
static void traverseCFBurstTrieWithCursor(
    OCFBurstTrieRef trie,
    const UInt8 *key,
    uint32_t length,
    Boolean prefix,
    void *context,
    BurstTrieTraversalCallback callback
) {
    if (trie->mapBase != NULL) {
        if ((trie->cflags & 8) != 0) {
            fwrite("Please use CFBurstTrieCursorRef API for file based trie.\n", 0x39, 1, stderr);
            return;
        }
        BurstTrieMappedTraversalCursor cursor;
        memset(&cursor, 0, 0x200);
        cursor.key = key;
        cursor.offset = ((const uint32_t *)trie->mapBase)[1];
        cursor.length = length;
        cursor.prefixLength = 0;
        findCFBurstTrieMappedLevel(trie, &cursor, prefix, context, callback);
        return;
    }
    BurstTrieTraversalCursor cursor;
    memset(&cursor, 0, 0x200);
    uintptr_t slot = (uintptr_t)trie | kBurstTrieSlotLevel;
    cursor.next = slot;
    cursor.key = key;
    cursor.targetLength = length;
    for (uint32_t index = 0; index != length; ++index) {
        uint8_t character = key[index];
        slot = ((BurstTrieLevel *)(slot & ~(uintptr_t)3))->slots[character];
        cursor.next = slot;
        cursor.keyBuffer[index] = character;
        if ((slot & 3) != kBurstTrieSlotLevel) {
            if ((slot & 3) == kBurstTrieSlotList) {
                cursor.prefixLength = index + 1;
                findCFBurstTrieList(&cursor, context, callback);
            }
            return;
        }
    }
    cursor.prefixLength = length;
    BurstTrieLevel *level = (BurstTrieLevel *)(slot & ~(uintptr_t)3);
    uint32_t depth = length;
    uint32_t target = length;
    if (level->payload != 0) {
        if (callback(context, cursor.keyBuffer, level->payload, true)) {
            return;
        }
        depth = cursor.prefixLength;
        target = cursor.targetLength;
    }
    if (prefix && depth == target) {
        return;
    }
    traverseCFBurstTrieLevel(level, &cursor, prefix, context, callback);
}

// Native containsKey, 0x1c02d096c. Non-external; the traversal callback of CFBurstTrieContainsUTF8String. An
// exact match stores its payload; the answer is the exact flag (the whole register).
static Boolean containsKey(void *context, const UInt8 *key, uint32_t payload, Boolean exact) {
    (void)key;
    if (exact) {
        *(uint32_t *)context = payload;
    }
    return exact;
}

// Native CFBurstTrieRelease, 0x1c02d0cd8. External. The last release (an acquire-release decrement from 1)
// unmaps a file-mapped trie (munmap of the 32-bit map size), finalizes an in-memory one and frees the trie.
void OCFBurstTrieRelease(OCFBurstTrieRef trie) {
    if (__atomic_fetch_add(&trie->retain, (uint32_t)-1, __ATOMIC_ACQ_REL) != 1) {
        return;
    }
    if (trie->mapBase != NULL) {
        if (trie->mappedByFile == 1) {
            munmap(trie->mapBase, trie->mapSize);
        }
    } else {
        finalizeCFBurstTrie(&trie->root);
    }
    free(trie);
}

// Native CFBurstTrieCursorAdvanceForBytes, 0x1c02d0d38. External. A cursor whose kind is not 1 answers false.
// Otherwise its first 16 bytes and the byte at 0x10 are saved, advanceMapCursor runs with the cursor's trie,
// and when it answers false (bit 0 clear) the saved bytes are restored and the answer is false.
Boolean OCFBurstTrieCursorAdvanceForBytes(OCFBurstTrieCursorRef cursor, const UInt8 *bytes, OCFIndex length) {
    if (cursor->kind != 1) {
        return false;
    }
    uint8_t inPage = cursor->inPage;
    uint8_t saved[16];
    memcpy(saved, cursor, sizeof(saved));
    if ((advanceMapCursor(cursor->trie, cursor, bytes, length) & 1) != 0) {
        return true;
    }
    cursor->inPage = inPage;
    memcpy(cursor, saved, sizeof(saved));
    return false;
}

// Native advanceMapCursor, 0x1c02d0f0c. Non-external. By the tag of the cursor's slot word:
// - 0 (only when the word equals the root offset; another answers false) and 1: advanceCursorMappedLevel.
// - 3, a compact level (no bytes or a negative length answer false): no bytes left stores the word at 0x20 as
//   the payload and answers true; an absent first byte answers false; otherwise the slot word of the byte
//   (from 0x24) becomes the cursor's and the rest of the bytes advance recursively.
// - 2, a page (no bytes or a negative length answer false; a page of size 4 answers false): with flags bit 3
//   the prefix-compressed entries are walked byte by byte, the cursor keeping the entry offset and the
//   number of suffix bytes matched; with bit 4 (and bit 3 clear) the plain entries are searched for one
//   that extends the bytes matched so far by the new bytes (entries are in byte order: an entry ordering
//   after the bytes ends the search); with neither the answer is false. A match answers true, storing the
//   entry's payload when the whole entry is matched (0 otherwise). With no bytes the payload is the first
//   entry's when the cursor's matched count equals the first entry's length, whatever entry the cursor is on.
static Boolean
advanceMapCursor(OCFBurstTrieRef trie, OCFBurstTrieCursorRef cursor, const UInt8 *bytes, OCFIndex length) {
    uint32_t slot = cursor->slot;
    uint32_t tag = slot & 3;
    if (tag <= 1) {
        if (tag == 0 && slot != readMapU32((const uint8_t *)trie->mapBase + 4)) {
            return false;
        }
        return advanceCursorMappedLevel(trie, cursor, bytes, length);
    }
    if (tag != 2) {
        if (bytes == NULL || length < 0) {
            return false;
        }
        const uint8_t *level = (const uint8_t *)trie->mapBase + (slot & ~3u);
        if (length == 0) {
            cursor->payload = readMapU32(level + 0x20);
            return true;
        }
        uint32_t character = bytes[0];
        uint64_t word = readMapU64(level + (size_t)(character >> 6) * 8);
        uint64_t bit = (uint64_t)1 << (character & 63);
        if ((bit & word) == 0) {
            return false;
        }
        uint32_t count = 0;
        if (character >= 0x40) {
            for (uint64_t index = 0; index != (character >> 6); ++index) {
                count += (uint32_t)__builtin_popcountll(readMapU64(level + index * 8));
            }
        }
        count += (uint32_t)__builtin_popcountll((bit - 1) & word);
        cursor->slot = readMapU32(level + (size_t)count * 4 + 0x24);
        return advanceMapCursor(trie, cursor, bytes + 1, length - 1);
    }
    if (bytes == NULL || length < 0) {
        return false;
    }
    const uint8_t *page = (const uint8_t *)trie->mapBase + (slot & ~3u);
    uint32_t end = readMapU32(page) - 4;
    if (end == 0) {
        return false;
    }
    const uint8_t *entries = page + 4;
    uint32_t flags = trie->cflags;
    if ((flags & 8) != 0) {
        uint8_t inPage = cursor->inPage;
        if (length == 0) {
            bool atFirst;
            uint32_t matched;
            if ((inPage & 1) != 0) {
                matched = cursor->matched;
                atFirst = cursor->entryOffset == 0;
            } else {
                bool empty = false;
                cursor->entryOffset = 0;
                cursor->matched = 0;
                if (page[4] == 0) {
                    empty = readMapU16(page + 5) == 0;
                }
                matched = 0;
                atFirst = true;
                cursor->inPage = empty;
            }
            cursor->payload = 0;
            uint32_t suffixLength = readMapU16(page + 5);
            if (!(atFirst && matched == 0 && suffixLength == 0) && matched != suffixLength) {
                return true;
            }
            cursor->payload = readMapU32(page + 7);
            return true;
        }
        uint32_t offset = 0;
        uint32_t depth = 0;
        uint32_t matched = 0;
        OCFIndex index = 0;
        for (;;) {
            uint32_t character = bytes[index];
            bool scan = true;
            bool store = true;
            if ((inPage & 1) != 0) {
                offset = cursor->entryOffset;
                const uint8_t *entry = entries + offset;
                if (offset == 0 && readMapU16(entry + 1) == 0) {
                    offset = 0;
                    depth = 0;
                    store = false;
                } else {
                    uint32_t position = cursor->matched;
                    uint32_t current = entry[position + 7];
                    if (current == character) {
                        matched = position + 1;
                        scan = false;
                    } else if (current > character) {
                        cursor->matched = position + 1;
                        return false;
                    } else {
                        depth = position + entry[0];
                        offset = offset + readMapU16(entry + 1) + 7;
                    }
                }
            } else {
                offset = 0;
                depth = 0;
            }
            if (scan) {
                if (store) {
                    cursor->entryOffset = offset;
                }
                for (;;) {
                    if (offset >= readMapU32(page) - 4) {
                        return false;
                    }
                    const uint8_t *entry = entries + offset;
                    uint32_t shared = entry[0];
                    if (depth > shared) {
                        return false;
                    }
                    uint32_t suffixLength = readMapU16(entry + 1);
                    if (depth < shared) {
                        offset = offset + suffixLength + 7;
                    } else if (suffixLength == 0) {
                        offset = offset + 7;
                    } else {
                        uint32_t first = entry[7];
                        if (first > character) {
                            return false;
                        }
                        if (first == character) {
                            matched = 1;
                            break;
                        }
                        offset = offset + suffixLength + 7;
                    }
                    cursor->entryOffset = offset;
                }
            }
            cursor->matched = matched;
            inPage = 1;
            cursor->inPage = 1;
            if (++index == length) {
                break;
            }
        }
        const uint8_t *entry = entries + offset;
        cursor->payload = 0;
        uint32_t suffixLength = readMapU16(entry + 1);
        if (!((offset | matched) == 0 && suffixLength == 0) && matched != suffixLength) {
            return true;
        }
        cursor->payload = readMapU32(entry + 3);
        return true;
    }
    if ((flags & 0x10) == 0) {
        return false;
    }
    uint8_t inPage = cursor->inPage;
    if (length == 0) {
        bool atFirst;
        uint32_t matched;
        if ((inPage & 1) != 0) {
            cursor->payload = 0;
            atFirst = cursor->entryOffset == 0;
            matched = cursor->matched;
        } else {
            cursor->payload = 0;
            cursor->entryOffset = 0;
            cursor->matched = 0;
            cursor->inPage = readMapU16(page + 4) == 0;
            atFirst = true;
            matched = 0;
        }
        uint32_t firstLength = readMapU16(page + 4);
        if (!(atFirst && matched == 0 && firstLength == 0) && matched != firstLength) {
            return true;
        }
        cursor->payload = readMapU32(page + 6);
        return true;
    }
    uint32_t offset;
    uint32_t matchedBefore;
    const uint8_t *previous;
    if ((inPage & 1) != 0) {
        offset = cursor->entryOffset;
        matchedBefore = cursor->matched;
        previous = page + offset + 0xA;
    } else {
        matchedBefore = 0;
        previous = NULL;
        offset = cursor->entryOffset;
    }
    if (offset >= end) {
        return false;
    }
    uint64_t total = (uint64_t)length + matchedBefore;
    for (;;) {
        const uint8_t *entry = entries + offset;
        uint32_t entryLength = readMapU16(entry);
        if (entryLength == 0) {
            offset = offset + 6;
        } else {
            if (entryLength <= matchedBefore) {
                return false;
            }
            uint32_t rest = entryLength - matchedBefore;
            int order = 0;
            if (matchedBefore != 0) {
                order = memcmp(entry + 6, previous, matchedBefore);
                if (order > 0) {
                    return false;
                }
            }
            if (order == 0) {
                const uint8_t *tail = entry + matchedBefore + 6;
                if (total > entryLength) {
                    if (memcmp(tail, bytes, rest) > 0) {
                        return false;
                    }
                } else {
                    order = memcmp(tail, bytes, (size_t)length);
                    if (order > 0) {
                        return false;
                    }
                    if (order == 0) {
                        cursor->inPage = 1;
                        cursor->matched = (uint32_t)total;
                        cursor->payload = 0;
                        if (total != entryLength) {
                            return true;
                        }
                        cursor->payload = readMapU32(entry + 2);
                        return true;
                    }
                }
            }
            offset = offset + entryLength + 6;
        }
        cursor->entryOffset = offset;
        if (offset >= end) {
            return false;
        }
    }
}

// Native makeCFBurstTrieListNode, 0x1c02d1944. Non-external. A zeroed node of length + 0x13 bytes
// (malloc_type_calloc, type 0x10200401AC14DB1; not checked) holding the bytes, a terminating zero, the 16-bit
// length, the weight and the payload.
static BurstTrieListNode *
makeCFBurstTrieListNode(const UInt8 *string, uint32_t length, uint32_t weight, uint32_t payload) {
    BurstTrieListNode *node = malloc_type_calloc(1, (size_t)length + 0x13, 0x10200401AC14DB1);
    memcpy(node->string, string, length);
    node->string[length] = 0;
    node->next = NULL;
    node->length = (uint16_t)length;
    node->weight = weight;
    node->payload = payload;
    return node;
}

// Native findCFBurstTrieMappedLevel, 0x1c02d19d0. Non-external. From the cursor's slot word (a mapped level)
// the searched key descends from the matched prefix length: each byte's slot word becomes the cursor's, the
// prefix length grows by one and the byte is copied into the key buffer. An empty slot ends; a page continues
// with findCFBurstTrieMappedPage and a compact level with findCFBurstTrieCompactMappedLevel (tail calls). At
// the key's level the word at 0x400, when nonzero, is reported as the payload, exact when the searched length
// is at least the prefix length the call started with; a callback answering true (bit 0), or a prefix search
// whose prefix length equals the searched length, ends; otherwise traverseCFBurstTrieMappedLevel (a tail
// call). The map base is reloaded from the trie for each level.
static void findCFBurstTrieMappedLevel(
    OCFBurstTrieRef trie,
    BurstTrieMappedTraversalCursor *cursor,
    Boolean prefix,
    void *context,
    BurstTrieTraversalCallback callback
) {
    uint32_t length = cursor->length;
    uint32_t start = cursor->prefixLength;
    uint32_t end = start > length ? start : length;
    const uint8_t *level = (const uint8_t *)trie->mapBase + (cursor->offset & ~3u);
    uint32_t depth = start;
    if (start < length) {
        const UInt8 *key = cursor->key + start;
        uint64_t remaining = (uint64_t)end - start;
        uint64_t position = start;
        for (;;) {
            uint8_t character = *key++;
            uint32_t slot = readMapU32(level + (size_t)character * 4);
            cursor->offset = slot;
            ++depth;
            cursor->prefixLength = depth;
            cursor->keyBuffer[position] = character;
            uint32_t tag = slot & 3;
            if (tag != 1) {
                if (tag == 0) {
                    return;
                }
                if (tag == 2) {
                    findCFBurstTrieMappedPage(trie, cursor, context, callback);
                    return;
                }
                findCFBurstTrieCompactMappedLevel(trie, cursor, prefix, context, callback);
                return;
            }
            level = (const uint8_t *)trie->mapBase + (slot & ~3u);
            ++position;
            if (--remaining == 0) {
                break;
            }
        }
    }
    uint32_t payload = readMapU32(level + 0x400);
    if (payload != 0) {
        if ((callback(context, cursor->keyBuffer, payload, length >= start) & 1) != 0) {
            return;
        }
        length = cursor->length;
        depth = cursor->prefixLength;
    }
    if (prefix != 0 && depth == length) {
        return;
    }
    traverseCFBurstTrieMappedLevel(trie, level, cursor, prefix, context, callback);
}

// Native findCFBurstTrieCompactMappedLevel, 0x1c02d1b30. Non-external. As findCFBurstTrieMappedLevel from a
// compact level: each byte grows the prefix length and is copied into the key buffer before its presence bit
// is tested (an absent byte ends); the byte's slot word (from 0x24, after the present bytes below it) becomes
// the cursor's. A mapped level continues with findCFBurstTrieMappedLevel and a page with
// findCFBurstTrieMappedPage (tail calls); at the key's compact level the word at 0x20 is the reported payload
// and the traversal is traverseCFBurstTrieCompactMappedLevel.
static void findCFBurstTrieCompactMappedLevel(
    OCFBurstTrieRef trie,
    BurstTrieMappedTraversalCursor *cursor,
    Boolean prefix,
    void *context,
    BurstTrieTraversalCallback callback
) {
    uint32_t length = cursor->length;
    uint32_t start = cursor->prefixLength;
    uint32_t end = start > length ? start : length;
    const uint8_t *level = (const uint8_t *)trie->mapBase + (cursor->offset & ~3u);
    uint32_t depth = start;
    if (start < length) {
        const UInt8 *key = cursor->key;
        uint64_t index = start;
        for (;;) {
            uint32_t character = key[index];
            uint64_t next = index + 1;
            cursor->prefixLength = (uint32_t)next;
            cursor->keyBuffer[index] = (uint8_t)character;
            uint64_t word = readMapU64(level + (size_t)(character >> 6) * 8);
            uint64_t bit = (uint64_t)1 << (character & 63);
            if ((word & bit) == 0) {
                return;
            }
            uint32_t count = 0;
            if (character >= 0x40) {
                for (uint64_t wordIndex = 0; wordIndex != (character >> 6); ++wordIndex) {
                    count += (uint32_t)__builtin_popcountll(readMapU64(level + wordIndex * 8));
                }
            }
            count += (uint32_t)__builtin_popcountll((bit - 1) & word);
            uint32_t slot = readMapU32(level + (size_t)count * 4 + 0x24);
            uint32_t tag = slot & 3;
            cursor->offset = slot;
            if (tag != 3) {
                if (tag == 0) {
                    return;
                }
                if (tag == 1) {
                    findCFBurstTrieMappedLevel(trie, cursor, prefix, context, callback);
                    return;
                }
                findCFBurstTrieMappedPage(trie, cursor, context, callback);
                return;
            }
            level = (const uint8_t *)trie->mapBase + (slot & ~3u);
            index = next;
            if (next == end) {
                break;
            }
        }
        depth = (uint32_t)index;
    }
    uint32_t payload = readMapU32(level + 0x20);
    if (payload != 0) {
        if ((callback(context, cursor->keyBuffer, payload, length >= start) & 1) != 0) {
            return;
        }
        length = cursor->length;
        depth = cursor->prefixLength;
    }
    if (prefix != 0 && depth == length) {
        return;
    }
    traverseCFBurstTrieCompactMappedLevel(trie, level, cursor, prefix, context, callback);
}

// Native findCFBurstTrieMappedPage, 0x1c02d1cf0. Non-external. The entries of the cursor's page are walked
// while their offset is below the page size. The rest of the searched key is its length minus the prefix
// length (signed), at least 0 when needed. Plain entries (flags byte bit 3 clear): an entry at least as long
// as the needed rest whose first needed bytes equal the key after the prefix is copied after the prefix into
// the key buffer, zero terminated, and a nonzero payload is reported (exact when the entry's length equals
// the needed rest). Prefix-compressed entries (bit 3 set): a 256-byte stack buffer (not initialized) rebuilds
// each entry's shared bytes from the previous entry's suffix (copied when the entry shares more bytes than
// the previous one); an entry whose whole length is at least the needed rest is matched, when the rest is
// positive, by its shared bytes and its suffix against the key after the prefix, then the shared bytes and
// the suffix are copied after the prefix, zero terminated, and a nonzero payload is reported (exact when the
// whole length equals the needed rest). A callback answering true (bit 0) ends.
static void findCFBurstTrieMappedPage(
    OCFBurstTrieRef trie,
    BurstTrieMappedTraversalCursor *cursor,
    void *context,
    BurstTrieTraversalCallback callback
) {
    const uint8_t *page = (const uint8_t *)trie->mapBase + (cursor->offset & ~3u);
    uint32_t size = readMapU32(page);
    int32_t remaining = (int32_t)(cursor->length - cursor->prefixLength);
    int32_t needed = remaining < 1 ? 0 : remaining;
    const uint8_t *entries = page + 4;
    if ((((const uint8_t *)&trie->cflags)[0] & 8) == 0) {
        if (size == 0) {
            return;
        }
        uint32_t offset = 0;
        do {
            const uint8_t *entry = entries + offset;
            uint32_t entryLength = readMapU16(entry);
            if (needed <= (int32_t)entryLength) {
                uint32_t depth = cursor->prefixLength;
                if (memcmp(entry + 6, cursor->key + depth, (size_t)(int64_t)needed) == 0) {
                    memcpy(cursor->keyBuffer + depth, entry + 6, entryLength);
                    cursor->keyBuffer[(uint32_t)(depth + readMapU16(entry))] = 0;
                    uint32_t payload = readMapU32(entry + 2);
                    if (payload != 0 &&
                        (callback(context, cursor->keyBuffer, payload, needed == (int32_t)entryLength) & 1) !=
                            0) {
                        return;
                    }
                }
            }
            offset = offset + readMapU16(entry) + 6;
        } while (offset < size);
        return;
    }
    if (size == 0) {
        return;
    }
    uint8_t buffer[256];
    const uint8_t *previous = NULL;
    uint32_t offset = 0;
    do {
        const uint8_t *entry = entries + offset;
        uint32_t suffixLength = readMapU16(entry + 1);
        uint32_t shared = entry[0];
        if (previous != NULL) {
            uint32_t previousShared = previous[0];
            if (shared > previousShared) {
                memcpy(buffer + previousShared, previous + 7, shared - previousShared);
            }
        }
        int32_t total = (int32_t)(shared + suffixLength);
        if (total >= needed) {
            uint32_t depth = cursor->prefixLength;
            shared = entry[0];
            bool matches = true;
            if (remaining > 0) {
                const UInt8 *searched = cursor->key + depth;
                if (memcmp(buffer, searched, shared) != 0 ||
                    memcmp(entry + 7, searched + shared, (uint32_t)(cursor->length - depth - shared)) != 0) {
                    matches = false;
                }
            }
            if (matches) {
                uint8_t *destination = cursor->keyBuffer + depth;
                memcpy(destination, buffer, shared);
                memcpy(destination + entry[0], entry + 7, readMapU16(entry + 1));
                cursor->keyBuffer[(uint32_t)(depth + entry[0] + readMapU16(entry + 1))] = 0;
                uint32_t payload = readMapU32(entry + 3);
                if (payload != 0 &&
                    (callback(context, cursor->keyBuffer, payload, total == needed) & 1) != 0) {
                    return;
                }
            }
        }
        previous = entry;
        offset = offset + readMapU16(entry + 1) + 7;
    } while (offset < size);
}

// Native traverseCFBurstTrieMappedLevel, 0x1c02d1f44. Non-external. Every slot word of the mapped level, in
// byte order, extends the key buffer by its byte (the prefix length one past the level's). A page becomes the
// cursor's slot word and continues with findCFBurstTrieMappedPage; a mapped level reports its word at 0x400
// and a compact level its word at 0x20, when nonzero, as the payload (exact when the searched length equals
// the new prefix length), and unless the callback answered true (bit 0) or a prefix search reached the
// searched length (both end the traversal) is traversed in turn.
static void traverseCFBurstTrieMappedLevel(
    OCFBurstTrieRef trie,
    const uint8_t *level,
    BurstTrieMappedTraversalCursor *cursor,
    Boolean prefix,
    void *context,
    BurstTrieTraversalCallback callback
) {
    uint32_t depth = cursor->prefixLength;
    uint32_t next = depth + 1;
    cursor->keyBuffer[depth] = 0;
    for (uint64_t index = 0; index != 0x100; ++index) {
        uint32_t slot = readMapU32(level + index * 4);
        uint32_t tag = slot & 3;
        cursor->prefixLength = next;
        cursor->keyBuffer[depth] = (uint8_t)index;
        if (tag > 1) {
            if (tag == 2) {
                cursor->offset = slot;
                cursor->keyBuffer[next] = 0;
                findCFBurstTrieMappedPage(trie, cursor, context, callback);
                continue;
            }
            const uint8_t *child = (const uint8_t *)trie->mapBase + (slot & ~3u);
            uint32_t payload = readMapU32(child + 0x20);
            uint32_t reached = next;
            if (payload != 0) {
                if ((callback(context, cursor->keyBuffer, payload, cursor->length == next) & 1) != 0) {
                    return;
                }
                reached = cursor->prefixLength;
            }
            if (prefix != 0 && reached == cursor->length) {
                return;
            }
            traverseCFBurstTrieCompactMappedLevel(trie, child, cursor, prefix, context, callback);
        } else if (tag != 0) {
            const uint8_t *child = (const uint8_t *)trie->mapBase + (slot & ~3u);
            uint32_t payload = readMapU32(child + 0x400);
            uint32_t reached = next;
            if (payload != 0) {
                if ((callback(context, cursor->keyBuffer, payload, cursor->length == next) & 1) != 0) {
                    return;
                }
                reached = cursor->prefixLength;
            }
            if (prefix != 0 && reached == cursor->length) {
                return;
            }
            traverseCFBurstTrieMappedLevel(trie, child, cursor, prefix, context, callback);
        }
    }
}

// Native traverseCFBurstTrieCompactMappedLevel, 0x1c02d20d4. Non-external. Every byte of the compact level,
// in byte order, first resets the prefix length to the level's; a present byte (its slot word from 0x24,
// after the present bytes below it) extends the key buffer by the byte (the prefix length one past the
// level's). A page and a compact level continue as in traverseCFBurstTrieMappedLevel; a mapped level is
// traversed with traverseCFBurstTrieMappedLevel directly, without reporting its payload or testing the prefix
// search.
static void traverseCFBurstTrieCompactMappedLevel(
    OCFBurstTrieRef trie,
    const uint8_t *level,
    BurstTrieMappedTraversalCursor *cursor,
    Boolean prefix,
    void *context,
    BurstTrieTraversalCallback callback
) {
    uint32_t depth = cursor->prefixLength;
    cursor->keyBuffer[depth] = 0;
    const uint8_t *slots = level + 0x24;
    uint32_t next = depth + 1;
    for (uint32_t index = 0; index != 0x100; ++index) {
        uint32_t wordIndex = index >> 6;
        uint64_t word = readMapU64(level + (size_t)wordIndex * 8);
        cursor->prefixLength = depth;
        uint64_t bit = (uint64_t)1 << (index & 63);
        if ((word & bit) == 0) {
            continue;
        }
        uint32_t count = 0;
        if (index >= 0x40) {
            uint64_t counted = 0;
            do {
                count += (uint32_t)__builtin_popcountll(readMapU64(level + counted * 8));
                ++counted;
            } while (counted < wordIndex);
        }
        count += (uint32_t)__builtin_popcountll(word & (bit - 1));
        uint32_t slot = readMapU32(slots + (size_t)count * 4);
        uint32_t tag = slot & 3;
        cursor->prefixLength = next;
        cursor->keyBuffer[depth] = (uint8_t)index;
        if (tag > 1) {
            if (tag == 2) {
                cursor->offset = slot;
                cursor->keyBuffer[next] = 0;
                findCFBurstTrieMappedPage(trie, cursor, context, callback);
                continue;
            }
            const uint8_t *child = (const uint8_t *)trie->mapBase + (slot & ~3u);
            uint32_t payload = readMapU32(child + 0x20);
            uint32_t reached = next;
            if (payload != 0) {
                if ((callback(context, cursor->keyBuffer, payload, cursor->length == next) & 1) != 0) {
                    return;
                }
                reached = cursor->prefixLength;
            }
            if (prefix != 0 && reached == cursor->length) {
                return;
            }
            traverseCFBurstTrieCompactMappedLevel(trie, child, cursor, prefix, context, callback);
        } else if (tag != 0) {
            traverseCFBurstTrieMappedLevel(
                trie, (const uint8_t *)trie->mapBase + (slot & ~3u), cursor, prefix, context, callback
            );
        }
    }
}

// Native findCFBurstTrieList, 0x1c02d2298. Non-external. Each node of the cursor's list whose length covers
// the rest of the searched key (searched length minus prefix length, at least 0) and whose bytes start with
// it is copied after the prefix into the key buffer (zero terminated) and becomes the cursor's node; a node
// with a payload is reported, exact when its length equals the rest, and a callback answering true ends.
static void
findCFBurstTrieList(BurstTrieTraversalCursor *cursor, void *context, BurstTrieTraversalCallback callback) {
    int32_t remaining = (int32_t)(cursor->targetLength - cursor->prefixLength);
    int32_t needed = remaining < 1 ? 0 : remaining;
    for (BurstTrieListNode *node = (BurstTrieListNode *)(cursor->next & ~(uintptr_t)3); node != NULL;
         node = node->next) {
        uint32_t nodeLength = node->length;
        if (needed > (int32_t)nodeLength) {
            continue;
        }
        uint32_t depth = cursor->prefixLength;
        if (remaining > 0 && memcmp(node->string, cursor->key + depth, (size_t)remaining) != 0) {
            continue;
        }
        memcpy(cursor->keyBuffer + depth, node->string, nodeLength);
        cursor->keyBuffer[(uint32_t)(depth + node->length)] = 0;
        cursor->next = (uintptr_t)node;
        if (node->payload != 0 &&
            callback(context, cursor->keyBuffer, node->payload, needed == (int32_t)nodeLength)) {
            return;
        }
    }
}

// Native traverseCFBurstTrieLevel, 0x1c02d2374. Non-external. Every slot of the level, in byte order, extends
// the key buffer by its byte (the prefix length one past the level's). A list continues with
// findCFBurstTrieList; a child level reports its payload (exact when the prefix length equals the searched
// length) and, unless the callback answered true (which ends the traversal) or a prefix search reached the
// searched length (also an end), is traversed in turn.
static void traverseCFBurstTrieLevel(
    BurstTrieLevel *level,
    BurstTrieTraversalCursor *cursor,
    Boolean prefix,
    void *context,
    BurstTrieTraversalCallback callback
) {
    uint32_t depth = cursor->prefixLength;
    uint32_t next = depth + 1;
    cursor->keyBuffer[depth] = 0;
    for (uintptr_t index = 0; index != 0x100; ++index) {
        uintptr_t slot = level->slots[index];
        cursor->prefixLength = next;
        cursor->keyBuffer[depth] = (uint8_t)index;
        if ((slot & 3) == kBurstTrieSlotList) {
            cursor->next = slot;
            cursor->keyBuffer[next] = 0;
            findCFBurstTrieList(cursor, context, callback);
        } else if ((slot & 3) == kBurstTrieSlotLevel) {
            BurstTrieLevel *child = (BurstTrieLevel *)(slot & ~(uintptr_t)3);
            uint32_t reached = next;
            if (child->payload != 0) {
                if (callback(context, cursor->keyBuffer, child->payload, cursor->targetLength == next)) {
                    return;
                }
                reached = cursor->prefixLength;
            }
            if (prefix && reached == cursor->targetLength) {
                return;
            }
            traverseCFBurstTrieLevel(child, cursor, prefix, context, callback);
        }
    }
}

// Native advanceCursorMappedLevel, 0x1c02d2474. Non-external. No bytes or a negative length answer false. At
// the cursor's mapped level a first byte's slot word becomes the cursor's and the rest advances with
// advanceMapCursor (a tail call); with no bytes left the level's word at 0x400 becomes the payload and the
// answer is true.
static Boolean advanceCursorMappedLevel(
    OCFBurstTrieRef trie, OCFBurstTrieCursorRef cursor, const UInt8 *bytes, OCFIndex length
) {
    if (bytes == NULL || length < 0) {
        return false;
    }
    const uint8_t *level = (const uint8_t *)trie->mapBase + (cursor->slot & ~3u);
    if (length != 0) {
        cursor->slot = readMapU32(level + (size_t)bytes[0] * 4);
        return advanceMapCursor(trie, cursor, bytes + 1, length - 1);
    }
    cursor->payload = readMapU32(level + 0x400);
    return true;
}

// Native burstTrieCompactTrieMappedFind, 0x1c02d24bc. Non-external. The key descends through compact levels:
// an absent byte answers false, a present byte's slot word (from 0x28, after the present bytes below it)
// leads to the next compact level; a key ending at a compact level answers whether its word at 0x20 is
// nonzero and stores the word at 0x24 through payload when payload is set. An empty slot answers false; a
// mapped level continues with burstTrieMappedFind and a page with burstTrieMappedPageFind (tail calls, with
// the bytes after the slot's byte; the prefix-compressed flag passes through in W5 and W4).
static Boolean burstTrieCompactTrieMappedFind(
    const uint8_t *level,
    const uint8_t *map,
    const UInt8 *key,
    uint32_t length,
    uint32_t *payload,
    Boolean prefixCompressed
) {
    while (length != 0) {
        uint32_t character = key[0];
        uint64_t word = readMapU64(level + (size_t)(character >> 6) * 8);
        uint64_t bit = (uint64_t)1 << (character & 63);
        if ((bit & word) == 0) {
            return false;
        }
        uint32_t count = 0;
        if (character >= 0x40) {
            for (uint64_t index = 0; index != (character >> 6); ++index) {
                count += (uint32_t)__builtin_popcountll(readMapU64(level + index * 8));
            }
        }
        count += (uint32_t)__builtin_popcountll((bit - 1) & word);
        uint32_t slot = readMapU32(level + (size_t)count * 4 + 0x28);
        uint32_t tag = slot & 3;
        if (tag != 3) {
            if (tag == 0) {
                return false;
            }
            if (tag == 1) {
                return burstTrieMappedFind(
                    map + (slot & ~3u), map, key + 1, length - 1, payload, prefixCompressed
                );
            }
            return burstTrieMappedPageFind(
                map + (slot & ~3u), key + 1, length - 1, payload, prefixCompressed
            );
        }
        level = map + (slot & ~3u);
        ++key;
        --length;
    }
    if (readMapU32(level + 0x20) == 0) {
        return false;
    }
    if (payload != NULL) {
        *payload = readMapU32(level + 0x24);
    }
    return true;
}

// Native burstTrieMappedPageFind, 0x1c02d25b8. Non-external. The page's entries are walked while their offset
// is below the page size. Plain entries (prefix-compressed flag bit 0 clear) match a key of the same length
// and bytes. Prefix-compressed entries match when the shared count plus the suffix length (16-bit) equals the
// length, the 256-byte stack buffer (not initialized) holds the key's shared bytes and the suffix holds the
// rest; after each mismatch the entry's suffix is copied into the buffer after its shared bytes, at most 255
// minus the shared count bytes and at most the length minus the shared count (an unsigned minimum, so the
// copy may extend past the suffix into the following map bytes). A match stores its payload through payload
// when payload is set and answers true.
static Boolean burstTrieMappedPageFind(
    const uint8_t *page, const UInt8 *key, uint32_t length, uint32_t *payload, Boolean prefixCompressed
) {
    uint32_t size = readMapU32(page);
    const uint8_t *entries = page + 4;
    if ((prefixCompressed & 1) != 0) {
        if (size == 0) {
            return false;
        }
        uint8_t buffer[256];
        uint32_t offset = 0;
        do {
            const uint8_t *entry = entries + offset;
            uint32_t shared = entry[0];
            uint32_t whole = readMapU16(entry + 1) + shared;
            if (length == (uint16_t)whole && memcmp(buffer, key, shared) == 0 &&
                memcmp(entry + 7, key + shared, length - shared) == 0) {
                if (payload != NULL) {
                    *payload = readMapU32(entry + 3);
                }
                return true;
            }
            whole &= 0xFFFF;
            uint32_t limit = shared ^ 0xFF;
            uint32_t rest = length - shared;
            memcpy(buffer + shared, entry + 7, limit < rest ? limit : rest);
            offset = offset - shared + whole + 7;
        } while (offset < size);
        return false;
    }
    if (size == 0) {
        return false;
    }
    uint32_t offset = 0;
    do {
        const uint8_t *entry = entries + offset;
        uint32_t entryLength = readMapU16(entry);
        if (length == entryLength && memcmp(entry + 6, key, length) == 0) {
            if (payload != NULL) {
                *payload = readMapU32(entry + 2);
            }
            return true;
        }
        offset = offset + entryLength + 6;
    } while (offset < size);
    return false;
}

// Native finalizeCFBurstTrie, 0x1c02d2e28. Non-external. Every list of the level is freed node by node and
// every child level finalized and freed; the level itself is not freed.
static void finalizeCFBurstTrie(BurstTrieLevel *level) {
    for (uintptr_t index = 0; index != 0x100; ++index) {
        uintptr_t slot = level->slots[index];
        if ((slot & 3) == kBurstTrieSlotList) {
            BurstTrieListNode *node = (BurstTrieListNode *)(slot & ~(uintptr_t)3);
            do {
                BurstTrieListNode *next = node->next;
                free(node);
                node = next;
            } while (node != NULL);
        } else if ((slot & 3) == kBurstTrieSlotLevel) {
            finalizeCFBurstTrie((BurstTrieLevel *)(slot & ~(uintptr_t)3));
            free((void *)(level->slots[index] & ~(uintptr_t)3));
        }
    }
}

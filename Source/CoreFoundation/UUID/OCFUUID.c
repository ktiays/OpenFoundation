//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/UUID/OCFUUID.h"

#include "CoreFoundation/Base/OCFHashBytes.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/String/OCFString.h"

#include <os/lock.h>
#include <stdint.h>

// Inferred organization: the CFUUID functions (__CFUUIDDeallocate, 0x1c0231c10, through __CFhashUUIDBytes,
// 0x1c0232fd0) are contiguous in the iOS Simulator __text section and are kept in one translation unit; the
// native source path is not in the evidence. The unit's other entry points (CFUUIDGetTypeID, CFUUIDCreate,
// CFUUIDCreateWithBytes, CFUUIDGetConstantUUIDWithBytes, CFUUIDGetUUIDBytes, CFUUIDCreateFromUUIDBytes and
// __CFUUIDCreate_block_invoke) are outside this restoration. Unless a definition says otherwise: iOS
// Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full);
// iOS Device 24A446: unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-core-corpus.json.

// libsystem_platform's os_unfair_lock_lock_with_options (private SPI); every native call passes 0x10000.
extern void os_unfair_lock_lock_with_options(os_unfair_lock_t lock, uint32_t options);
static const uint32_t kUnfairLockDataSynchronization = 0x10000;

static void __CFUUIDDeallocate(OCFTypeRef cf);
static OCFStringRef __CFUUIDCopyFormattingDescription(OCFTypeRef cf, const void *formatOptions);
static OCFStringRef __CFUUIDCopyDescription(OCFTypeRef cf);
static Boolean __CFisEqualUUIDBytes(const void *ptr1, const void *ptr2);
static OCFHashCode __CFhashUUIDBytes(const void *ptr);

// Native __CFUUIDClass, 0x2227a5a20. Version 0, name "CFUUID", finalize, formatting and debug descriptions.
const OCFRuntimeClass __CFUUIDClass = {
    0,
    "CFUUID",
    NULL,
    NULL,
    __CFUUIDDeallocate,
    NULL,
    NULL,
    __CFUUIDCopyFormattingDescription,
    __CFUUIDCopyDescription,
    NULL,
    NULL,
    0,
};

// Native _uniquedUUIDs (__uniquedUUIDs, 0x22e6d3218) and _uniquedUUIDsLock (__uniquedUUIDsLock, 0x22e6d3220),
// __DATA,__bss, non-external: the table of live UUIDs keyed by their bytes, and its lock.
static OCFMutableDictionaryRef _uniquedUUIDs;
static os_unfair_lock _uniquedUUIDsLock;

// Native __CFUUIDDeallocate, 0x1c0231c10. Non-external. Under the lock, the UUID's bytes are removed from the
// table when the table exists.
static void __CFUUIDDeallocate(OCFTypeRef cf) {
    os_unfair_lock_lock_with_options(&_uniquedUUIDsLock, kUnfairLockDataSynchronization);
    if (_uniquedUUIDs != NULL) {
        OCFDictionaryRemoveValue(_uniquedUUIDs, &((const struct __OCFUUID *)cf)->bytes);
    }
    os_unfair_lock_unlock(&_uniquedUUIDsLock);
}

// Native __CFUUIDCopyFormattingDescription, 0x1c0231c58. Non-external. The UUID string, created with the
// UUID's allocator (a tail call); the format options are not read.
static OCFStringRef __CFUUIDCopyFormattingDescription(OCFTypeRef cf, const void *formatOptions) {
    (void)formatOptions;
    return OCFUUIDCreateString(OCFGetAllocator(cf), (OCFUUIDRef)cf);
}

// Native __CFUUIDCopyDescription, 0x1c0231c7c. Non-external. "<CFUUID %p> %@" with the UUID and its string
// (created with the UUID's allocator, released afterwards), formatted with kCFAllocatorSystemDefault.
static OCFStringRef __CFUUIDCopyDescription(OCFTypeRef cf) {
    OCFStringRef string = OCFUUIDCreateString(OCFGetAllocator(cf), (OCFUUIDRef)cf);
    OCFStringRef description =
        OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR("<CFUUID %p> %@"), cf, string);
    OCFRelease(string);
    return description;
}

// Native CFUUIDGetTypeID, 0x1c0231ce0. External. The builtin type ID 34.
OCFTypeID OCFUUIDGetTypeID(void) {
    return 34;
}

// Native __CFUUIDCreateWithBytesPrimitive, 0x1c0231ea0, with its block _____CFUUIDCreateWithBytesPrimitive_
// block_invoke (0x1c0232e60, non-external; descriptor 0x2227a5ab0, captures: the __block result, the
// allocator, the sixteen bytes and isConst, in that layout). Non-external. The stack block is invoked
// directly while the lock is held. The block looks the bytes up in the table (when it exists); a UUID found
// is retained unless isConst. Otherwise a type-34 instance with 16 extra bytes is created (no category), the
// bytes are copied into it, and it is stored in the table (created on first use with
// kCFAllocatorSystemDefault, capacity 0, byte-comparing keys and CFCopyDescription/CFEqual values; the
// callback tables 0x2227a5ad0 and 0x2227a5b00 are copied to the stack first) under its own bytes. A failed
// creation answers NULL. With isConst the result is made immortal (also a found one).
static OCFUUIDRef
__CFUUIDCreateWithBytesPrimitive(OCFAllocatorRef allocator, OCFUUIDBytes bytes, Boolean isConst) {
    __block struct __OCFUUID *uuid = NULL;
    os_unfair_lock_lock_with_options(&_uniquedUUIDsLock, kUnfairLockDataSynchronization);
    ^{
      if (_uniquedUUIDs != NULL) {
          uuid = (struct __OCFUUID *)OCFDictionaryGetValue(_uniquedUUIDs, &bytes);
      } else {
          uuid = NULL;
      }
      if (uuid != NULL) {
          if (!isConst) {
              OCFRetain(uuid);
          }
      } else {
          uuid = (struct __OCFUUID *)_OCFRuntimeCreateInstance(allocator, 34, 16, NULL);
          if (uuid == NULL) {
              return;
          }
          uuid->bytes = bytes;
          struct __OCFUUID *created = uuid;
          OCFDictionaryKeyCallBacks keyCallBacks = {
              0, NULL, NULL, NULL, __CFisEqualUUIDBytes, __CFhashUUIDBytes
          };
          OCFDictionaryValueCallBacks valueCallBacks = {0, NULL, NULL, OCFCopyDescription, OCFEqual};
          if (_uniquedUUIDs == NULL) {
              _uniquedUUIDs =
                  OCFDictionaryCreateMutable(kOCFAllocatorSystemDefault, 0, &keyCallBacks, &valueCallBacks);
          }
          OCFDictionarySetValue(_uniquedUUIDs, &created->bytes, created);
      }
      if (isConst) {
          __CFRuntimeSetImmortal(uuid);
      }
    }();
    os_unfair_lock_unlock(&_uniquedUUIDsLock);
    return uuid;
}

// Native CFUUIDCreateFromString, 0x1c023200c. External. NULL for a NULL or empty string. At most 100
// characters are read into a stack buffer. Characters before the first hexadecimal digit ('0'-'9', or a
// letter whose value with bit 5 cleared is 'A'-'F') are skipped. Each of the sixteen bytes is read from the
// next two characters when both exist (a character that is not '0'-'9', 'a'-'f' or 'A'-'F', tested in that
// order, makes the whole byte 0); a byte without two characters is 0 and does not advance. One character is
// skipped unconditionally after bytes 3, 5, 7 and 9 (the dashes). The bytes are made into a UUID with
// __CFUUIDCreateWithBytesPrimitive (not constant). The native code unrolls each byte's two-character loop.
OCFUUIDRef OCFUUIDCreateFromString(OCFAllocatorRef alloc, OCFStringRef uuidStr) {
    if (uuidStr == NULL) {
        return NULL;
    }
    UniChar chars[100];
    OCFIndex len = OCFStringGetLength(uuidStr);
    if (len >= 101) {
        len = 100;
        OCFStringGetCharacters(uuidStr, (OCFRange){0, 100}, chars);
    } else {
        if (len == 0) {
            return NULL;
        }
        OCFStringGetCharacters(uuidStr, (OCFRange){0, len}, chars);
    }
    OCFIndex i = 0;
    for (; i < len; i++) {
        UniChar c = chars[i];
        if ((uint32_t)c - 0x3a > 0xfffffff5 || ((uint32_t)c & 0xffffffdf) - 0x47 > 0xfffffff9) {
            break;
        }
    }
    OCFUUIDBytes bytes;
    uint8_t *byte = (uint8_t *)&bytes;
#define READ_A_BYTE(index)                                                                                   \
    do {                                                                                                     \
        uint8_t value = 0;                                                                                   \
        if (i + 1 < len) {                                                                                   \
            for (OCFIndex j = 0; j < 2; j++) {                                                               \
                uint32_t ch = chars[i + j];                                                                  \
                uint8_t digit;                                                                               \
                if (ch - '0' < 10) {                                                                         \
                    digit = (uint8_t)(ch - '0');                                                             \
                } else if (ch - 'a' < 6) {                                                                   \
                    digit = (uint8_t)(ch - 'a' + 10);                                                        \
                } else if (ch - 'A' <= 5) {                                                                  \
                    digit = (uint8_t)(ch - 'A' + 10);                                                        \
                } else {                                                                                     \
                    value = 0;                                                                               \
                    break;                                                                                   \
                }                                                                                            \
                value = (uint8_t)(digit | (value << 4));                                                     \
            }                                                                                                \
            i += 2;                                                                                          \
        }                                                                                                    \
        byte[index] = value;                                                                                 \
    } while (0)
    READ_A_BYTE(0);
    READ_A_BYTE(1);
    READ_A_BYTE(2);
    READ_A_BYTE(3);
    i++;
    READ_A_BYTE(4);
    READ_A_BYTE(5);
    i++;
    READ_A_BYTE(6);
    READ_A_BYTE(7);
    i++;
    READ_A_BYTE(8);
    READ_A_BYTE(9);
    i++;
    READ_A_BYTE(10);
    READ_A_BYTE(11);
    READ_A_BYTE(12);
    READ_A_BYTE(13);
    READ_A_BYTE(14);
    READ_A_BYTE(15);
#undef READ_A_BYTE
    return __CFUUIDCreateWithBytesPrimitive(alloc, bytes, false);
}

// Native CFUUIDCreateString, 0x1c02329d8. External. A UUID that is not a type-34 object is
// _CFAssertMismatchedTypeID(34, its type). A mutable string (no maximum length) receives the bytes as
// upper-case hexadecimal digits (a nibble below 10 is '0' + nibble, else nibble + 0x37), high nibble first,
// in the groups 8-4-4-4-12, appended as 9, 5, 5, 5 and 12 characters (each of the first four with its dash).
OCFStringRef OCFUUIDCreateString(OCFAllocatorRef alloc, OCFUUIDRef uuid) {
    OCFTypeID typeID = _CFGetNonObjCTypeID(uuid);
    if (typeID != 34) {
        _CFAssertMismatchedTypeID(34, typeID);
    }
    OCFMutableStringRef string = OCFStringCreateMutable(alloc, 0);
    const uint8_t *byte = (const uint8_t *)&uuid->bytes;
    UniChar buffer[12];
#define HEX_DIGIT(nibble) ((UniChar)((nibble) < 10 ? ((nibble) | 0x30) : ((nibble) + 0x37)))
#define STORE_BYTE(position, index)                                                                          \
    do {                                                                                                     \
        uint32_t high = (uint32_t)byte[index] >> 4;                                                          \
        uint32_t low = (uint32_t)byte[index] & 0xf;                                                          \
        buffer[(position)] = HEX_DIGIT(high);                                                                \
        buffer[(position) + 1] = HEX_DIGIT(low);                                                             \
    } while (0)
    STORE_BYTE(0, 0);
    STORE_BYTE(2, 1);
    STORE_BYTE(4, 2);
    STORE_BYTE(6, 3);
    buffer[8] = '-';
    OCFStringAppendCharacters(string, buffer, 9);
    STORE_BYTE(0, 4);
    STORE_BYTE(2, 5);
    buffer[4] = '-';
    OCFStringAppendCharacters(string, buffer, 5);
    STORE_BYTE(0, 6);
    STORE_BYTE(2, 7);
    buffer[4] = '-';
    OCFStringAppendCharacters(string, buffer, 5);
    STORE_BYTE(0, 8);
    STORE_BYTE(2, 9);
    buffer[4] = '-';
    OCFStringAppendCharacters(string, buffer, 5);
    STORE_BYTE(0, 10);
    STORE_BYTE(2, 11);
    STORE_BYTE(4, 12);
    STORE_BYTE(6, 13);
    STORE_BYTE(8, 14);
    STORE_BYTE(10, 15);
    OCFStringAppendCharacters(string, buffer, 12);
#undef STORE_BYTE
#undef HEX_DIGIT
    return string;
}

// Native CFUUIDGetUUIDBytes, 0x1c0232e20. External. A UUID that is not a type-34 object is
// _CFAssertMismatchedTypeID(34, its type); the sixteen bytes at offset 16 are answered in X0/X1.
OCFUUIDBytes OCFUUIDGetUUIDBytes(OCFUUIDRef uuid) {
    OCFTypeID typeID = _CFGetNonObjCTypeID(uuid);
    if (typeID != 34) {
        _CFAssertMismatchedTypeID(34, typeID);
    }
    return uuid->bytes;
}

// Native CFUUIDCreateFromUUIDBytes, 0x1c0232e58. External. A tail call of __CFUUIDCreateWithBytesPrimitive,
// not constant.
OCFUUIDRef OCFUUIDCreateFromUUIDBytes(OCFAllocatorRef alloc, OCFUUIDBytes bytes) {
    return __CFUUIDCreateWithBytesPrimitive(alloc, bytes, false);
}

// Native __CFisEqualUUIDBytes, 0x1c0232fb8. Non-external; the table's key equality: both 8-byte halves equal.
static Boolean __CFisEqualUUIDBytes(const void *ptr1, const void *ptr2) {
    const uint64_t *first = ptr1;
    const uint64_t *second = ptr2;
    return first[0] == second[0] && first[1] == second[1];
}

// Native __CFhashUUIDBytes, 0x1c0232fd0. Non-external; the table's key hash: CFHashBytes over the sixteen
// bytes (a tail call).
static OCFHashCode __CFhashUUIDBytes(const void *ptr) {
    return OCFHashBytes((const uint8_t *)ptr, 16);
}

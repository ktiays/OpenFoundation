//
//  Created by ktiays on 2026/11/05.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/String/OCFStringUText.h"
#include "CoreFoundation/String/OCFAttributedString.h"

#include <malloc/malloc.h>
#include <stdbool.h>
#include <stdlib.h>

// Inferred organization: the CFString UText provider spans 0x1c0232fd8-0x1c0233de7 of the iOS Simulator
// CoreFoundation __text section: CFStringOpenUText, __CFStringUTextInit, the attributed-string callbacks, the
// shared close, doAccess, __pinIndex and doExtract, then the string callbacks. The two function tables
// __CFStringFuncs (0x2227a5b98) and __CFAttributedStringFuncs (0x2227a5b28) and the chunk sizes _bufferSizes
// (0x1c03a4240) are its data. The scope closure reached only CFStringOpenUText and __CFStringUTextInit; the
// callbacks are reached through the tables (resolved by reading). Unless a definition says otherwise: iOS
// Simulator 24A434, CoreFoundation UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly
// read in full). iOS Device 24A446: unread.
//
// The UText chunk is the string's own UTF-16 buffer when CFStringGetCharactersPtr gives one, otherwise a
// 1024-byte buffer in q, refilled in chunks whose size grows (32 ... 512 characters) while accesses stay
// within 16 characters of the previous chunk (the count kept in c). p holds the mutable string that wraps the
// replacement characters; providerProperties bit 0x20 marks a context owned (copied) by a deep clone.

// ICU's error codes as the provider sets them.
static const int32_t kIllegalArgumentError = 1;
static const int32_t kIndexOutOfBoundsError = 8;
static const int32_t kBufferOverflowError = 15;
static const int32_t kStringNotTerminatedWarning = -124;

// ICU's UTextFuncs, with the layout of the native tables: tableSize 0x70, three reserved words, then the
// callbacks. The member names are ICU's; the struct name is local.
typedef struct {
    int32_t tableSize;
    int32_t reserved1;
    int32_t reserved2;
    int32_t reserved3;
    ProviderUText *(*clone)(ProviderUText *dest, const ProviderUText *src, int8_t deep, int32_t *status);
    int64_t (*nativeLength)(ProviderUText *ut);
    int8_t (*access)(ProviderUText *ut, int64_t nativeIndex, int8_t forward);
    int32_t (*extract)(
        ProviderUText *ut,
        int64_t nativeStart,
        int64_t nativeLimit,
        uint16_t *dest,
        int32_t destCapacity,
        int32_t *status
    );
    int32_t (*replace)(
        ProviderUText *ut,
        int64_t nativeStart,
        int64_t nativeLimit,
        const uint16_t *replacementText,
        int32_t replacmentLength,
        int32_t *status
    );
    void (*copy)(
        ProviderUText *ut,
        int64_t nativeStart,
        int64_t nativeLimit,
        int64_t nativeDest,
        int8_t move,
        int32_t *status
    );
    void *mapOffsetToNative;
    void *mapNativeIndexToUTF16;
    void (*close)(ProviderUText *ut);
    void *spare1;
    void *spare2;
    void *spare3;
} ProviderUTextFuncs;

_Static_assert(sizeof(ProviderUTextFuncs) == 0x70, "The UText function table is 0x70 bytes.");

// libicucore (supplied).
extern ProviderUText *utext_setup(ProviderUText *ut, int32_t extraSpace, int32_t *status);

static ProviderUText *
__CFStringClone(ProviderUText *dest, const ProviderUText *src, int8_t deep, int32_t *status);
static int64_t __CFAttributedStringLength(ProviderUText *ut);
static int8_t __CFAttributedStringAccess(ProviderUText *ut, int64_t index, int8_t forward);
static int32_t __CFAttributedStringExtract(
    ProviderUText *ut, int64_t start, int64_t limit, uint16_t *dest, int32_t capacity, int32_t *status
);
static int32_t __CFAttributedStringReplace(
    ProviderUText *ut, int64_t start, int64_t limit, const uint16_t *text, int32_t length, int32_t *status
);
static void __CFAttributedStringCopy(
    ProviderUText *ut, int64_t start, int64_t limit, int64_t destination, int8_t move, int32_t *status
);
static void __CFStringClose(ProviderUText *ut);
static int64_t __CFStringLength(ProviderUText *ut);
static int8_t __CFStringAccess(ProviderUText *ut, int64_t index, int8_t forward);
static int32_t __CFStringExtract(
    ProviderUText *ut, int64_t start, int64_t limit, uint16_t *dest, int32_t capacity, int32_t *status
);
static int32_t __CFStringReplace(
    ProviderUText *ut, int64_t start, int64_t limit, const uint16_t *text, int32_t length, int32_t *status
);
static void __CFStringCopy(
    ProviderUText *ut, int64_t start, int64_t limit, int64_t destination, int8_t move, int32_t *status
);

// Native __CFStringFuncs, 0x2227a5b98 (__DATA_CONST,__const). Non-external.
static const ProviderUTextFuncs __CFStringFuncs = {
    0x70,
    0,
    0,
    0,
    __CFStringClone,
    __CFStringLength,
    __CFStringAccess,
    __CFStringExtract,
    __CFStringReplace,
    __CFStringCopy,
    NULL,
    NULL,
    __CFStringClose,
    NULL,
    NULL,
    NULL,
};

// Native __CFAttributedStringFuncs, 0x2227a5b28 (__DATA_CONST,__const). Non-external.
static const ProviderUTextFuncs __CFAttributedStringFuncs = {
    0x70,
    0,
    0,
    0,
    __CFStringClone,
    __CFAttributedStringLength,
    __CFAttributedStringAccess,
    __CFAttributedStringExtract,
    __CFAttributedStringReplace,
    __CFAttributedStringCopy,
    NULL,
    NULL,
    __CFStringClose,
    NULL,
    NULL,
    NULL,
};

// Native _bufferSizes, 0x1c03a4240 (__TEXT,__const). Non-external. The chunk sizes, in characters, indexed by
// the count of nearby accesses (0 to 5).
static const int16_t _bufferSizes[6] = {32, 32, 64, 128, 256, 512};

static int8_t doAccess(ProviderUText *ut, int64_t index, int8_t forward, OCFStringRef string);
static OCFIndex
__pinIndex(OCFIndex index, OCFIndex length, OCFStringRef string, int8_t forward, int8_t adjust);
static int32_t doExtract(
    int64_t start, int64_t limit, uint16_t *dest, int32_t capacity, int32_t *status, OCFStringRef string
);

// Native CFStringOpenUText, 0x1c0232fd8. External.
ProviderUText *OCFStringOpenUText(ProviderUText *ut, OCFStringRef string) {
    return __CFStringUTextInit(ut, 0, string);
}

// Native __CFStringUTextInit, 0x1c0232fe4. Hidden (was a private external).
ProviderUText *__CFStringUTextInit(ProviderUText *ut, uint32_t properties, const void *string) {
    int32_t status = 0;
    if (string == NULL) {
        return NULL;
    }
    ProviderUText *result = utext_setup(ut, 0, &status);
    if (status > 0) {
        return NULL;
    }
    result->providerProperties = (int32_t)properties;
    result->context = string;
    result->pFuncs = properties < 0x10 ? &__CFStringFuncs : &__CFAttributedStringFuncs;
    return result;
}

// Native __CFStringClone, 0x1c02330a8. Non-external. The clone starts with an empty chunk at the source's
// chunk start; a deep clone owns a mutable copy of the context (properties | 0x28).
static ProviderUText *
__CFStringClone(ProviderUText *dest, const ProviderUText *src, int8_t deep, int32_t *status) {
    if (*status > 0) {
        return NULL;
    }
    ProviderUText *clone = utext_setup(dest, 0, status);
    if (*status > 0) {
        return clone;
    }
    int32_t properties = src->providerProperties;
    clone->providerProperties = properties;
    int64_t start = src->chunkNativeStart;
    clone->chunkNativeStart = start;
    clone->chunkNativeLimit = start;
    clone->nativeIndexingLimit = (int32_t)start;
    clone->chunkOffset = 0;
    clone->pFuncs = (properties & 0x10) == 0 ? &__CFStringFuncs : &__CFAttributedStringFuncs;
    if (*status > 0) {
        return clone;
    }
    if (deep) {
        const void *copy;
        if ((properties & 0x10) != 0) {
            copy = OCFAttributedStringCreateMutableCopy(kOCFAllocatorSystemDefault, 0, src->context);
        } else {
            copy = OCFStringCreateMutableCopy(kOCFAllocatorSystemDefault, 0, src->context);
        }
        clone->context = copy;
        clone->providerProperties |= 0x28;
    } else {
        clone->context = src->context;
    }
    return clone;
}

// Native __CFAttributedStringLength, 0x1c0233194. Non-external.
static int64_t __CFAttributedStringLength(ProviderUText *ut) {
    return OCFAttributedStringGetLength(ut->context);
}

// Native __CFAttributedStringAccess, 0x1c023319c. Non-external.
static int8_t __CFAttributedStringAccess(ProviderUText *ut, int64_t index, int8_t forward) {
    return doAccess(ut, index, forward, OCFAttributedStringGetString(ut->context));
}

// Native __CFAttributedStringExtract, 0x1c02331e0. Non-external.
static int32_t __CFAttributedStringExtract(
    ProviderUText *ut, int64_t start, int64_t limit, uint16_t *dest, int32_t capacity, int32_t *status
) {
    return doExtract(start, limit, dest, capacity, status, OCFAttributedStringGetString(ut->context));
}

// Native __CFAttributedStringReplace, 0x1c023323c. Non-external. As __CFStringReplace, on the attributed
// string; both indexes are pinned forward and backward as there.
static int32_t __CFAttributedStringReplace(
    ProviderUText *ut, int64_t start, int64_t limit, const uint16_t *text, int32_t length, int32_t *status
) {
    OCFMutableAttributedStringRef attributedString = (OCFMutableAttributedStringRef)ut->context;
    OCFStringRef string = OCFAttributedStringGetString(attributedString);
    OCFIndex oldLength = OCFAttributedStringGetLength(attributedString);
    if (*status > 0) {
        return 0;
    }
    if (text == NULL && length != 0) {
        *status = kIllegalArgumentError;
        return 0;
    }
    if (start > limit) {
        *status = kIndexOutOfBoundsError;
        return 0;
    }
    OCFIndex pinnedStart = __pinIndex(start, oldLength, string, 1, 1);
    OCFIndex pinnedLimit = __pinIndex(limit, oldLength, string, 0, 1);
    OCFMutableStringRef replacement = (OCFMutableStringRef)ut->p;
    if (replacement != NULL) {
        OCFStringSetExternalCharactersNoCopy(replacement, (uint16_t *)text, length, length);
    } else {
        replacement = OCFStringCreateMutableWithExternalCharactersNoCopy(
            kOCFAllocatorSystemDefault, (uint16_t *)text, length, length, kOCFAllocatorNull
        );
        ut->p = replacement;
    }
    OCFAttributedStringReplaceString(
        attributedString, (OCFRange){pinnedStart, pinnedLimit - pinnedStart}, replacement
    );
    if (ut->chunkNativeLimit > pinnedStart || ut->chunkContents != ut->q) {
        ut->chunkNativeLimit = 0;
        ut->nativeIndexingLimit = 0;
        ut->chunkNativeStart = 0;
        ut->chunkOffset = 0;
        ut->chunkLength = 0;
        ut->chunkContents = NULL;
    }
    OCFIndex delta = OCFAttributedStringGetLength(attributedString) - oldLength;
    doAccess(ut, delta + pinnedLimit, 1, OCFAttributedStringGetString(attributedString));
    return (int32_t)delta;
}

// Native __CFAttributedStringCopy, 0x1c02333e0. Non-external. Copies (or moves) the attributed substring to
// destination between begin and end editing; the chunk is kept when it is the provider's buffer and ends
// before the change.
static void __CFAttributedStringCopy(
    ProviderUText *ut, int64_t start, int64_t limit, int64_t destination, int8_t move, int32_t *status
) {
    OCFMutableAttributedStringRef attributedString = (OCFMutableAttributedStringRef)ut->context;
    OCFStringRef string = OCFAttributedStringGetString(attributedString);
    OCFIndex length = OCFAttributedStringGetLength(attributedString);
    if (*status > 0) {
        return;
    }
    if (start > limit || (start < destination && destination < limit)) {
        *status = kIndexOutOfBoundsError;
        return;
    }
    OCFIndex pinnedStart = __pinIndex(start, length, string, 1, 1);
    OCFIndex pinnedLimit = __pinIndex(limit, length, string, 0, 1);
    OCFIndex copied = pinnedLimit - pinnedStart;
    OCFAttributedStringRef substring = OCFAttributedStringCreateWithSubstring(
        kOCFAllocatorSystemDefault, attributedString, (OCFRange){pinnedStart, copied}
    );
    OCFAttributedStringBeginEditing(attributedString);
    OCFAttributedStringReplaceAttributedString(attributedString, (OCFRange){destination, 0}, substring);
    OCFRelease(substring);
    if (move) {
        pinnedStart = destination < pinnedStart ? pinnedLimit : pinnedStart;
        OCFAttributedStringReplaceString(attributedString, (OCFRange){pinnedStart, copied}, OF_STR(""));
    }
    OCFAttributedStringEndEditing(attributedString);
    OCFIndex changed = pinnedStart < destination ? pinnedStart : destination;
    bool keep = false;
    if (ut->chunkContents == ut->q) {
        keep = move ? ut->chunkNativeLimit <= changed : ut->chunkNativeLimit <= destination;
    }
    if (!keep) {
        ut->chunkNativeLimit = 0;
        ut->nativeIndexingLimit = 0;
        ut->chunkNativeStart = 0;
        ut->chunkOffset = 0;
        ut->chunkLength = 0;
        ut->chunkContents = NULL;
    }
    doAccess(ut, copied + destination, 1, OCFAttributedStringGetString(attributedString));
}

// Native __CFStringClose, 0x1c02335a4. Non-external. Releases the replacement wrapper, frees the chunk buffer
// and releases an owned context.
static void __CFStringClose(ProviderUText *ut) {
    if (ut->p != NULL) {
        OCFRelease(ut->p);
        ut->p = NULL;
    }
    if (ut->q != NULL) {
        free((void *)ut->q);
        ut->q = NULL;
    }
    if ((ut->providerProperties & 0x20) != 0) {
        OCFRelease(ut->context);
        ut->context = NULL;
    }
}

// Native doAccess, 0x1c02335f4. Non-external. Makes the chunk hold index (clamped to the string): a hit sets
// the offset only. The string's own buffer becomes the chunk (windowed at 0x7FFFFFFF characters); otherwise a
// chunk of _bufferSizes[c] characters after (forward) or before index is copied into q, its ends pinned off
// surrogate pairs. Returns whether a character is available in the direction asked.
static int8_t doAccess(ProviderUText *ut, int64_t index, int8_t forward, OCFStringRef string) {
    OCFIndex length = OCFStringGetLength(string);
    OCFIndex bounded = index < length ? index : length;
    OCFIndex clamped = index < 0 ? 0 : bounded;
    int64_t limit = ut->chunkNativeLimit;
    if (forward) {
        if (clamped < limit && clamped >= ut->chunkNativeStart) {
            ut->chunkOffset = (int32_t)(clamped - ut->chunkNativeStart);
            return 1;
        }
        if (clamped >= length && limit == length) {
            ut->chunkOffset = ut->chunkLength;
            return 0;
        }
    } else {
        if (clamped <= limit && clamped > ut->chunkNativeStart) {
            ut->chunkOffset = (int32_t)(clamped - ut->chunkNativeStart);
            return 1;
        }
        if (clamped == 0 && ut->chunkNativeStart == 0) {
            ut->chunkOffset = 0;
            return 0;
        }
    }
    const uint16_t *characters = OCFStringGetCharactersPtr(string);
    ut->chunkContents = characters;
    if (characters != NULL) {
        int64_t start;
        if (clamped > 0x7FFFFFFE) {
            start = forward ? bounded : clamped - 0x7FFFFFFF;
            ut->chunkNativeStart = start;
            ut->chunkOffset = forward ? 0 : 0x7FFFFFFF;
            ut->chunkContents = characters + start;
        } else {
            start = 0;
            ut->chunkNativeStart = 0;
            ut->chunkOffset = (int32_t)clamped;
        }
        int64_t available = length - start;
        if (available > 0x7FFFFFFE) {
            ut->chunkNativeLimit = start + 0x7FFFFFFF;
            ut->chunkLength = 0x7FFFFFFF;
            ut->nativeIndexingLimit = 0x7FFFFFFF;
        } else {
            ut->chunkNativeLimit = length;
            ut->chunkLength = (int32_t)available;
            ut->nativeIndexingLimit = (int32_t)available;
        }
        return 1;
    }
    uint16_t *buffer = (uint16_t *)ut->q;
    int32_t nearby;
    if (buffer != NULL) {
        int64_t distance = forward ? clamped - ut->chunkNativeLimit : ut->chunkNativeStart - clamped;
        if ((uint64_t)(distance + 16) <= 32) {
            nearby = ut->c;
            if ((uint32_t)nearby <= 4) {
                ++nearby;
                ut->c = nearby;
            }
        } else {
            nearby = 0;
            ut->c = 0;
        }
    } else {
        buffer = malloc_type_malloc(0x400, 0x1000040BDFB0063);
        ut->q = buffer;
        if (buffer == NULL) {
            return 0;
        }
        nearby = 0;
        ut->c = 0;
    }
    ut->chunkContents = buffer;
    OCFIndex chunkStart;
    int64_t chunkLength;
    if (forward) {
        ut->chunkNativeStart = clamped;
        OCFIndex end = clamped + _bufferSizes[nearby];
        ut->chunkNativeLimit = end;
        if (end > length) {
            ut->chunkNativeLimit = length;
            end = length;
        } else if (end < length) {
            end = __pinIndex(end, length, string, forward, 1);
            ut->chunkNativeLimit = end;
            clamped = ut->chunkNativeStart;
        }
        chunkStart = clamped;
        chunkLength = end - clamped;
        ut->chunkOffset = 0;
        ut->chunkLength = (int32_t)chunkLength;
    } else {
        OCFIndex end = clamped < length ? clamped : length;
        ut->chunkNativeLimit = end;
        OCFIndex start = end - _bufferSizes[nearby];
        ut->chunkNativeStart = start;
        if (start < 0) {
            chunkStart = 0;
            ut->chunkNativeStart = 0;
        } else if (end == _bufferSizes[nearby]) {
            chunkStart = 0;
        } else {
            chunkStart = __pinIndex(start, length, string, 0, 1);
            ut->chunkNativeStart = chunkStart;
            end = ut->chunkNativeLimit;
        }
        chunkLength = end - chunkStart;
        ut->chunkOffset = (int32_t)chunkLength;
        ut->chunkLength = (int32_t)chunkLength;
    }
    OCFStringGetCharacters(
        string, (OCFRange){chunkStart, (int32_t)chunkLength}, (uint16_t *)ut->chunkContents
    );
    ut->nativeIndexingLimit = ut->chunkLength;
    return 1;
}

// Native __pinIndex, 0x1c02338b0. Non-external. Clamps index to the string; with adjust, an index on the low
// half of a surrogate pair moves before the pair (forward) or after it.
static OCFIndex
__pinIndex(OCFIndex index, OCFIndex length, OCFStringRef string, int8_t forward, int8_t adjust) {
    if (index < 0) {
        return 0;
    }
    if (index > length) {
        return length;
    }
    if (index == length || index == 0 || !adjust) {
        return index;
    }
    if ((OCFStringGetCharacterAtIndex(string, index) & 0xFC00) != 0xDC00) {
        return index;
    }
    uint16_t previous = OCFStringGetCharacterAtIndex(string, index - 1);
    OCFIndex moved = forward ? index - 1 : index + 1;
    return (previous & 0xFC00) != 0xD800 ? index : moved;
}

// Native doExtract, 0x1c023394c. Non-external. Copies the pinned range (at most capacity characters) into
// dest, NUL-terminating when room remains; a range of 0 or of at least capacity characters reports
// U_STRING_NOT_TERMINATED_WARNING when it equals the capacity and U_BUFFER_OVERFLOW_ERROR otherwise (native,
// including the empty range).
static int32_t doExtract(
    int64_t start, int64_t limit, uint16_t *dest, int32_t capacity, int32_t *status, OCFStringRef string
) {
    OCFIndex length = OCFStringGetLength(string);
    if (*status > 0) {
        return 0;
    }
    if (capacity < 0 || (dest == NULL && capacity != 0)) {
        *status = kIllegalArgumentError;
        return 0;
    }
    if (start < 0 || limit < start || limit - start >= 0x80000000LL) {
        *status = kIndexOutOfBoundsError;
        return 0;
    }
    OCFIndex pinnedStart = __pinIndex(start, length, string, 1, 1);
    int64_t extracted = __pinIndex(limit, length, string, 1, 1) - pinnedStart;
    if (dest != NULL && capacity != 0) {
        int32_t count = capacity < (int32_t)extracted ? capacity : (int32_t)extracted;
        OCFStringGetCharacters(string, (OCFRange){pinnedStart, count}, dest);
    }
    int32_t current = *status;
    if (current > 0) {
        return (int32_t)extracted;
    }
    if (extracted >= 1 && extracted < (int64_t)(uint32_t)capacity) {
        dest[extracted] = 0;
        if (current != kStringNotTerminatedWarning) {
            return (int32_t)extracted;
        }
        *status = 0;
        return (int32_t)extracted;
    }
    *status = extracted == (int64_t)(uint32_t)capacity ? kStringNotTerminatedWarning : kBufferOverflowError;
    return (int32_t)extracted;
}

// Native __CFStringLength, 0x1c0233a9c. Non-external.
static int64_t __CFStringLength(ProviderUText *ut) {
    return OCFStringGetLength(ut->context);
}

// Native __CFStringAccess, 0x1c0233aa4. Non-external.
static int8_t __CFStringAccess(ProviderUText *ut, int64_t index, int8_t forward) {
    return doAccess(ut, index, forward, ut->context);
}

// Native __CFStringExtract, 0x1c0233aac. Non-external.
static int32_t __CFStringExtract(
    ProviderUText *ut, int64_t start, int64_t limit, uint16_t *dest, int32_t capacity, int32_t *status
) {
    return doExtract(start, limit, dest, capacity, status, ut->context);
}

// Native __CFStringReplace, 0x1c0233acc. Non-external. Replaces the pinned range with the characters (wrapped
// without copying in the reused p string), resets a chunk that is not the provider's buffer or that reaches
// past the start, and reloads the chunk after the replacement. Returns the change in length.
static int32_t __CFStringReplace(
    ProviderUText *ut, int64_t start, int64_t limit, const uint16_t *text, int32_t length, int32_t *status
) {
    OCFMutableStringRef string = (OCFMutableStringRef)ut->context;
    OCFIndex oldLength = OCFStringGetLength(string);
    if (*status > 0) {
        return 0;
    }
    if (text == NULL && length != 0) {
        *status = kIllegalArgumentError;
        return 0;
    }
    if (start > limit) {
        *status = kIndexOutOfBoundsError;
        return 0;
    }
    OCFIndex pinnedStart = __pinIndex(start, oldLength, string, 1, 1);
    OCFIndex pinnedLimit = __pinIndex(limit, oldLength, string, 0, 1);
    OCFMutableStringRef replacement = (OCFMutableStringRef)ut->p;
    if (replacement != NULL) {
        OCFStringSetExternalCharactersNoCopy(replacement, (uint16_t *)text, length, length);
    } else {
        replacement = OCFStringCreateMutableWithExternalCharactersNoCopy(
            kOCFAllocatorSystemDefault, (uint16_t *)text, length, length, kOCFAllocatorNull
        );
        ut->p = replacement;
    }
    OCFStringReplace(string, (OCFRange){pinnedStart, pinnedLimit - pinnedStart}, replacement);
    if (ut->chunkNativeLimit > pinnedStart || ut->chunkContents != ut->q) {
        ut->chunkNativeLimit = 0;
        ut->nativeIndexingLimit = 0;
        ut->chunkNativeStart = 0;
        ut->chunkOffset = 0;
        ut->chunkLength = 0;
        ut->chunkContents = NULL;
    }
    OCFIndex delta = OCFStringGetLength(string) - oldLength;
    doAccess(ut, delta + pinnedLimit, 1, string);
    return (int32_t)delta;
}

// Native __CFStringCopy, 0x1c0233c54. Non-external. Inserts the substring at destination (deleting the source
// for a move); the chunk is kept when it is the provider's buffer and ends before the change.
static void __CFStringCopy(
    ProviderUText *ut, int64_t start, int64_t limit, int64_t destination, int8_t move, int32_t *status
) {
    OCFMutableStringRef string = (OCFMutableStringRef)ut->context;
    OCFIndex length = OCFStringGetLength(string);
    if (*status > 0) {
        return;
    }
    if (start > limit || (start < destination && destination < limit)) {
        *status = kIndexOutOfBoundsError;
        return;
    }
    OCFIndex pinnedStart = __pinIndex(start, length, string, 1, 1);
    OCFIndex pinnedLimit = __pinIndex(limit, length, string, 0, 1);
    OCFIndex copied = pinnedLimit - pinnedStart;
    OCFStringRef substring =
        OCFStringCreateWithSubstring(kOCFAllocatorSystemDefault, string, (OCFRange){pinnedStart, copied});
    OCFStringInsert(string, destination, substring);
    OCFRelease(substring);
    bool keep = false;
    if (move) {
        OCFIndex deleted = destination < pinnedStart ? pinnedLimit : pinnedStart;
        OCFStringDelete(string, (OCFRange){deleted, copied});
        if (ut->chunkContents == ut->q) {
            OCFIndex changed = deleted < destination ? deleted : destination;
            keep = ut->chunkNativeLimit <= changed;
        }
    } else if (ut->chunkContents == ut->q) {
        keep = ut->chunkNativeLimit <= destination;
    }
    if (!keep) {
        ut->chunkNativeLimit = 0;
        ut->nativeIndexingLimit = 0;
        ut->chunkNativeStart = 0;
        ut->chunkOffset = 0;
        ut->chunkLength = 0;
        ut->chunkContents = NULL;
    }
    doAccess(ut, copied + destination, 1, string);
}

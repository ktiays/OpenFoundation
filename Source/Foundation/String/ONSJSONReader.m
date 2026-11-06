//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSJSONReader.h"
#import "Foundation/String/ONSJSONSerialization.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#import "Foundation/Collections/ONSObjectArray.h"
#import "CoreFoundation/Objective-C/OCFNSArray.h"
#import "CoreFoundation/Objective-C/OCFNSMutableArray.h"
#import "CoreFoundation/Objective-C/OCFNSMutableDictionary.h"
#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFArray.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <errno.h>
#include <malloc/malloc.h>
#include <math.h>
#include <string.h>
#include <xlocale.h>

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: medium. The JSON5 string decoder and error exits require a second assembly review.
// iOS Device divergence: iOS Device 24A446
// remains unread. Reading: Local/Scratch/cf-core-r2-fg/Reading.md. The native retains/releases use MRC.

// The state allocated in -parseData:options:error:, size 0x2038. Field meanings and offsets follow
// its loads and stores and the cache index stride of 32 in _setInCache.
typedef struct {
    const uint8_t *bytes;
    NSUInteger length;
    id string;
    uint32_t hash;
} JSONStringCacheEntry;

typedef struct {
    const uint8_t *bytes;
    NSUInteger options;
    NSUInteger length;
    NSUInteger cursor;
    NSUInteger depth;
    NSUInteger line;
    NSUInteger lineStart;
    JSONStringCacheEntry cache[256];
} JSONReaderState;

_Static_assert(sizeof(JSONReaderState) == 0x2038, "Native JSON reader state size.");
_Static_assert(offsetof(JSONReaderState, cache) == 56, "Native JSON reader cache offset.");
_Static_assert(sizeof(JSONStringCacheEntry) == 32, "Native JSON reader cache entry stride.");

extern bool dyld_program_sdk_at_least(OCFBuildVersion version);
extern char jsonStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSString));
extern char jsonMutableStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableString));
extern char jsonNullClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSNull));
extern char jsonDictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDictionary));
extern char jsonMutableDictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableDictionary));
extern char jsonArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSArray));
extern char jsonMutableArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableArray));

@protocol JSONNumberMessages
- (id)initWithUnsignedLongLong:(unsigned long long)value;
- (id)initWithString:(id)string;
@end

@protocol JSONReaderMessages <JSONCollectionMessages>
+ (id)null;
- (id)initWithObjects:(const id *)objects count:(NSUInteger)count;
- (id)initWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count;
- (const void *)bytes;
- (NSUInteger)length;
- (id)dataUsingEncoding:(NSUInteger)encoding;
@end

static BOOL skipJSON5Comment(JSONReaderState *state, BOOL singleLine, id *error);
static id newJSONObject(JSONReaderState *state, BOOL assumeDictionary, id *error);
static id newJSONValue(JSONReaderState *state, id *error);
static id newJSONString(JSONReaderState *state, BOOL allowUnquoted, id *error);
static id newJSON5HexNumber(JSONReaderState *state, id *error);
static id newJSONNumber(JSONReaderState *state, id *error);
static id newJSON5InfinityOrNaN(JSONReaderState *state, id *error);
static id newJSONNumberCreateRoundTripping(JSONReaderState *state, NSUInteger start, id number);

// Inlined error construction in the native reader. The index and the column use the byte cursor.
static inline void jsonReadError(JSONReaderState *state, id *error, id message) {
    if (error) {
        NSUInteger column = state->cursor >= state->lineStart ? state->cursor - state->lineStart : 0;
        id description = [[ONSString alloc]
            initWithFormat:(id)OF_STR("%@ around line %lu, column %lu."), message, state->line, column];
        id index = [[ONSNumber alloc] initWithUnsignedInteger:state->cursor];
        id keys[] = {ONSDebugDescriptionErrorKey, (id)OF_STR("NSJSONSerializationErrorIndex")};
        id values[] = {description, index};
        id info = [(id<JSONReaderMessages>)jsonDictionaryClass dictionaryWithObjects:values
                                                                             forKeys:keys
                                                                               count:2];
        *error = [ONSError errorWithDomain:ONSCocoaErrorDomain code:3840 userInfo:info];
        objc_release(description);
        objc_release(index);
    }
}

// Native skipJSONWhitespace, 0x1c0631250. Non-external.
static BOOL skipJSONWhitespace(JSONReaderState *state, id *error, BOOL allowsEOF) {
    BOOL json5 = (state->options & 8) != 0;
    while (state->cursor < state->length) {
        NSUInteger cursor = state->cursor;
        unsigned byte = state->bytes[cursor];
        if (byte == ' ' || byte == '\t' || (json5 && (byte == '\v' || byte == '\f' || byte == 160))) {
            ++state->cursor;
        } else if (byte == '\r' || byte == '\n') {
            ++state->cursor;
            if (byte == '\r' && state->cursor < state->length && state->bytes[state->cursor] == '\n') {
                ++state->cursor;
            }
            ++state->line;
            state->lineStart = state->cursor;
        } else if (
            json5 && byte == '/' && cursor + 1 < state->length &&
            (state->bytes[cursor + 1] == '/' || state->bytes[cursor + 1] == '*')
        ) {
            BOOL singleLine = state->bytes[cursor + 1] == '/';
            state->cursor = cursor + 2;
            if (!skipJSON5Comment(state, singleLine, error)) {
                return NO;
            }
        } else {
            return YES;
        }
    }
    state->cursor = state->length;
    if (allowsEOF) {
        return YES;
    }
    jsonReadError(state, error, (id)OF_STR("Unexpected end of file"));
    return NO;
}

// Native newJSONObject, 0x1c0631534. Non-external.
static id newJSONObject(JSONReaderState *state, BOOL assumeDictionary, id *error) {
    BOOL braced = !assumeDictionary || state->bytes[state->cursor] == '{';
    Class mutableClass = (Class)jsonMutableDictionaryClass;
    Class immutableClass = (Class)jsonDictionaryClass;
    if (braced) {
        if (state->cursor + 1 >= state->length) {
            jsonReadError(state, error, (id)OF_STR("Unexpected end of file during JSON parse."));
            return nil;
        }
        ++state->cursor;
        if (!skipJSONWhitespace(state, error, NO)) {
            return nil;
        }
        if (state->bytes[state->cursor] == '}') {
            ++state->cursor;
            return objc_alloc_init((state->options & 1) ? mutableClass : immutableClass);
        }
    }

    id stackKeys[32], stackValues[32];
    id *keys = stackKeys;
    id *values = stackValues;
    NSUInteger capacity = 32;
    NSUInteger count = 0;
    id dictionary = nil;
    for (;;) {
        BOOL json5 = (state->options & 8) != 0;
        if (!json5 && state->bytes[state->cursor] != '"') {
            jsonReadError(state, error, (id)OF_STR("No string key for value in object"));
            break;
        }
        id key = newJSONString(state, json5, error);
        if (!key) {
            break;
        }
        if (!skipJSONWhitespace(state, error, NO)) {
            objc_release(key);
            break;
        }
        if (state->bytes[state->cursor] != ':') {
            jsonReadError(state, error, (id)OF_STR("No value for key in object"));
            objc_release(key);
            break;
        }
        if (state->cursor + 1 >= state->length) {
            jsonReadError(state, error, (id)OF_STR("Unexpected end of file during JSON parse."));
            objc_release(key);
            break;
        }
        ++state->cursor;
        if (!skipJSONWhitespace(state, error, NO)) {
            objc_release(key);
            break;
        }
        id value = newJSONValue(state, error);
        if (!value) {
            objc_release(key);
            break;
        }
        if (count >= capacity) {
            capacity *= 2;
            id *newKeys;
            id *newValues;
            if (keys == stackKeys) {
                newValues = ONSAllocateObjectArray(capacity);
                newKeys = ONSAllocateObjectArray(capacity);
                if (newValues && newKeys) {
                    memmove(newValues, values, count * sizeof(id));
                    memmove(newKeys, keys, count * sizeof(id));
                }
            } else {
                newKeys = ONSReallocateObjectArray(keys, capacity);
                newValues = ONSReallocateObjectArray(values, capacity);
            }
            if (!newKeys || !newValues) {
                // Foundation defect: newJSONObject (0x1c0631534), confidence: verified. If only one
                // reallocation succeeds, native frees that buffer and then releases through stale
                // pointers. Preserve the live buffers until after releasing the collected pairs.
                if (keys == stackKeys) {
                    free(newKeys);
                    free(newValues);
                } else {
                    keys = newKeys ? newKeys : keys;
                    values = newValues ? newValues : values;
                }
                objc_release(key);
                objc_release(value);
                jsonReadError(state, error, (id)OF_STR("Memory exhausted during parse of dictionary."));
                break;
            }
            keys = newKeys;
            values = newValues;
        }
        keys[count] = key;
        values[count++] = value;

        BOOL finished;
        if (braced) {
            if (!skipJSONWhitespace(state, error, NO)) {
                break;
            }
            unsigned delimiter = state->bytes[state->cursor];
            if (delimiter == ',') {
                if (state->cursor + 1 >= state->length) {
                    jsonReadError(state, error, (id)OF_STR("Unexpected end of file during JSON parse."));
                    break;
                }
                ++state->cursor;
                if (!skipJSONWhitespace(state, error, NO)) {
                    break;
                }
                finished = state->bytes[state->cursor] == '}';
            } else if (delimiter == '}') {
                finished = YES;
            } else {
                jsonReadError(state, error, (id)OF_STR("Badly formed object"));
                break;
            }
        } else {
            finished = !skipJSONWhitespace(state, error, YES) || state->cursor >= state->length;
            if (!finished && state->bytes[state->cursor] != ',') {
                jsonReadError(state, error, (id)OF_STR("Badly formed object"));
                break;
            }
            if (!finished) {
                if (state->cursor + 1 >= state->length) {
                    finished = YES;
                } else {
                    ++state->cursor;
                    finished = !skipJSONWhitespace(state, error, YES) || state->cursor >= state->length;
                }
            }
        }
        if (finished) {
            if (state->options & 1) {
                dictionary = [(id<JSONReaderMessages>)objc_alloc(mutableClass) initWithObjects:values
                                                                                       forKeys:keys
                                                                                         count:count];
            } else if (dyld_program_sdk_at_least((OCFBuildVersion){UINT32_MAX, 0x07e00901})) {
                dictionary = [(id<JSONReaderMessages>)objc_alloc(immutableClass) initWithObjects:values
                                                                                         forKeys:keys
                                                                                           count:count];
            } else {
                dictionary = (id)OCFDictionaryCreate(
                    kOCFAllocatorDefault,
                    (const void **)keys,
                    (const void **)values,
                    count,
                    &kOCFTypeDictionaryKeyCallBacks,
                    &kOCFTypeDictionaryValueCallBacks
                );
            }
            break;
        }
    }
    for (NSUInteger i = 0; i < count; ++i) {
        objc_release(values[i]);
        objc_release(keys[i]);
    }
    if (keys != stackKeys) {
        free(values);
        free(keys);
    }
    ++state->cursor;
    return dictionary;
}

// Native newJSONValue, 0x1c0631fbc. Non-external.
static id newJSONValue(JSONReaderState *state, id *error) {
    if (state->depth >= 513) {
        jsonReadError(state, error, (id)OF_STR("Too many nested arrays or dictionaries"));
        return nil;
    }

    const uint8_t *bytes = state->bytes;
    NSUInteger cursor = state->cursor;
    unsigned byte = bytes[cursor];
    unsigned next = cursor + 1 < state->length ? bytes[cursor + 1] : 0;
    BOOL hexPrefix = cursor + 2 < state->length && (bytes[cursor + 2] & 0xdf) == 'X';
    id failure = nil;
    id result = nil;
    void *pool = objc_autoreleasePoolPush();

    BOOL json5 = (state->options & 8) != 0;
    if (byte == '"' || (json5 && byte == '\'')) {
        result = newJSONString(state, NO, &failure);
    } else if (byte == '{') {
        ++state->depth;
        result = newJSONObject(state, NO, &failure);
        --state->depth;
    } else if (byte == '[') {
        ++state->depth;
        if (state->cursor + 1 >= state->length) {
            jsonReadError(state, &failure, (id)OF_STR("Unexpected end of file during JSON parse."));
            goto finishArray;
        }
        ++state->cursor;
        if (!skipJSONWhitespace(state, &failure, NO)) {
            goto finishArray;
        }
        Class arrayClass = (Class)((state->options & 1) ? jsonMutableArrayClass : jsonArrayClass);
        if (state->bytes[state->cursor] == ']') {
            ++state->cursor;
            result = objc_alloc_init(arrayClass);
            goto finishArray;
        }

        id stack[32];
        id *objects = stack;
        NSUInteger count = 0;
        NSUInteger capacity = 32;
        for (;;) {
            id object = newJSONValue(state, &failure);
            if (!object) {
                break;
            }
            if (!skipJSONWhitespace(state, &failure, NO)) {
                objc_release(object);
                break;
            }
            unsigned delimiter = state->bytes[state->cursor];
            if (delimiter != ',' && delimiter != ']') {
                jsonReadError(state, &failure, (id)OF_STR("Badly formed array"));
                objc_release(object);
                break;
            }
            if (delimiter == ',') {
                if (state->cursor + 1 >= state->length) {
                    jsonReadError(state, &failure, (id)OF_STR("Unexpected end of file during JSON parse."));
                    objc_release(object);
                    break;
                }
                ++state->cursor;
            }
            if (count >= capacity) {
                capacity *= 2;
                id *larger = objects == stack ? ONSAllocateObjectArray(capacity)
                                              : ONSReallocateObjectArray(objects, capacity);
                if (!larger) {
                    // Foundation defect: newJSONValue (0x1c0631fbc), confidence: verified.
                    // Native frees the old heap buffer at 0x1c0632a3c before releasing its objects.
                    // Keep the buffer live until the common cleanup has released those objects.
                    objc_release(object);
                    jsonReadError(state, &failure, (id)OF_STR("Memory exhausted during parse of array."));
                    break;
                }
                if (objects == stack) {
                    memmove(larger, stack, count * sizeof(id));
                }
                objects = larger;
            }
            objects[count++] = object;
            if (!skipJSONWhitespace(state, &failure, NO)) {
                break;
            }
            if (delimiter == ']' || state->bytes[state->cursor] == ']') {
                if ((state->options & 1) ||
                    dyld_program_sdk_at_least((OCFBuildVersion){UINT32_MAX, 0x07e00901})) {
                    result = [(id<JSONReaderMessages>)objc_alloc(arrayClass) initWithObjects:objects
                                                                                       count:count];
                } else {
                    result = (id)OCFArrayCreate(
                        kOCFAllocatorDefault, (const void **)objects, count, &kOCFTypeArrayCallBacks
                    );
                }
                break;
            }
        }
        for (NSUInteger i = 0; i < count; ++i) {
            objc_release(objects[i]);
        }
        if (objects != stack) {
            free(objects);
        }
        ++state->cursor;

    finishArray:
        --state->depth;
    } else if (byte == 't' || byte == 'f' || byte == 'n') {
        const char *literal = byte == 't' ? "true" : byte == 'f' ? "false" : "null";
        NSUInteger count = byte == 'f' ? 5 : 4;
        if (cursor + count > state->length) {
            jsonReadError(state, &failure, (id)OF_STR("Unexpected end of file during JSON parse."));
        } else if (memcmp(bytes + cursor + 1, literal + 1, count - 1) != 0) {
            id reason = byte == 't'   ? (id)OF_STR("Something looked like a 'true' but wasn't")
                        : byte == 'f' ? (id)OF_STR("Something looked like a 'false' but wasn't")
                                      : (id)OF_STR("Something looked like a 'null' but wasn't");
            jsonReadError(state, &failure, reason);
        } else {
            state->cursor = cursor + count - 1;
            id object = byte == 't'   ? (id)kOCFBooleanTrue
                        : byte == 'f' ? (id)kOCFBooleanFalse
                                      : [(id<JSONReaderMessages>)jsonNullClass null];
            ++state->cursor;
            result = objc_retain(object);
        }
    } else if (byte >= '0' && byte <= '9') {
        result = json5 && (next & 0xdf) == 'X' ? newJSON5HexNumber(state, &failure)
                                               : newJSONNumber(state, &failure);
    } else if (byte == '-') {
        if (json5 && next == '0' && hexPrefix) {
            result = newJSON5HexNumber(state, &failure);
        } else if ((json5 && next == 'I') || next == 'N') {
            result = newJSON5InfinityOrNaN(state, &failure);
        } else {
            result = newJSONNumber(state, &failure);
        }
    } else if (json5 && byte == '+') {
        if (cursor + 1 >= state->length) {
            jsonReadError(state, &failure, (id)OF_STR("Unexpected end of file during JSON parse."));
        } else {
            ++state->cursor;
        }
        if (next == '0' && hexPrefix) {
            result = newJSON5HexNumber(state, &failure);
        } else if (next == 'N' || next == 'I') {
            result = newJSON5InfinityOrNaN(state, &failure);
        } else {
            result = newJSONNumber(state, &failure);
        }
    } else if (json5 && (byte == 'I' || byte == 'N')) {
        result = newJSON5InfinityOrNaN(state, &failure);
    } else if (json5 && byte == '.') {
        result = newJSONNumber(state, &failure);
    } else {
        jsonReadError(state, &failure, (id)OF_STR("Invalid value"));
    }
    if (failure) {
        objc_retain(failure);
    }
    objc_autoreleasePoolPop(pool);
    if (error && failure) {
        *error = failure;
    }
    objc_autorelease(failure);
    return result;
}

@implementation _ONSJSONReader

// Native -[_NSJSONReader parseData:options:error:], 0x1c0632bfc. Non-external.
- (id)parseData:(id<JSONReaderMessages>)data options:(NSUInteger)options error:(id *)error {
    Class dictionaryClass = (Class)((options & 1) ? jsonMutableDictionaryClass : jsonDictionaryClass);
    if ([data length] == 0) {
        if (options & 0x10) {
            return objc_autorelease(objc_alloc_init(dictionaryClass));
        }
        if (error) {
            id key = ONSDebugDescriptionErrorKey;
            id value = (id)OF_STR("Unable to parse empty data.");
            id info = [(id<JSONReaderMessages>)jsonDictionaryClass dictionaryWithObjects:&value
                                                                                 forKeys:&key
                                                                                   count:1];
            *error = [ONSError errorWithDomain:ONSCocoaErrorDomain code:3840 userInfo:info];
        }
        return nil;
    }

    const uint8_t *bytes = [data bytes];
    NSUInteger length = [data length];
    NSUInteger skip = 0;
    NSUInteger encoding = 0;
    if (length >= 5 && bytes[0] == 0xfe && bytes[1] == 0xff && bytes[2] == 0 && bytes[3] == 0xff) {
        encoding = 0x9c000100;
        skip = 4;
    } else if (length >= 5 && bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 0xfe && bytes[3] == 0xff) {
        encoding = 0x98000100;
        skip = 4;
    } else if (length >= 3 && bytes[0] == 0xfe && bytes[1] == 0xff) {
        encoding = 0x90000100;
        skip = 2;
    } else if (length >= 3 && bytes[0] == 0xff && bytes[1] == 0xfe) {
        encoding = 0x94000100;
        skip = 2;
    } else if (length >= 4 && bytes[0] == 0xef && bytes[1] == 0xbb && bytes[2] == 0xbf) {
        skip = 3;
    } else if (length >= 5 && bytes[0] == 0) {
        if (bytes[1] != 0 && bytes[2] == 0 && bytes[3] != 0) {
            encoding = 0x90000100;
        } else if (bytes[1] == 0 && bytes[2] == 0 && bytes[3] != 0) {
            encoding = 0x98000100;
        }
    } else if (length >= 5 && bytes[1] == 0 && bytes[3] == 0) {
        encoding = bytes[2] != 0 ? 0x94000100 : 0x9c000100;
    }

    if (encoding) {
        const uint8_t *encodedBytes = [data bytes];
        NSUInteger encodedLength = [data length];
        id<JSONReaderMessages> string = objc_autorelease([[ONSString alloc] initWithBytes:encodedBytes + skip
                                                                                   length:encodedLength - skip
                                                                                 encoding:encoding]);
        if (!string) {
            if (error) {
                id key = ONSDebugDescriptionErrorKey;
                id value = (id)OF_STR(
                    "Unable to convert data to a string using the detected encoding. The data may be corrupt."
                );
                id info = [(id<JSONReaderMessages>)jsonDictionaryClass dictionaryWithObjects:&value
                                                                                     forKeys:&key
                                                                                       count:1];
                *error = [ONSError errorWithDomain:ONSCocoaErrorDomain code:3840 userInfo:info];
            }
            return nil;
        }
        data = [string dataUsingEncoding:4];
        skip = 0;
    }

    JSONReaderState *state = malloc_type_calloc(1, sizeof(*state), 0x109004015bca0ec);
    state->bytes = [data bytes];
    state->length = [data length];
    state->line = 1;
    state->cursor = skip;
    state->options = options;
    BOOL assumeDictionary = (options & 0x10) != 0;
    id object = nil;
    if (!skipJSONWhitespace(state, error, assumeDictionary) || state->cursor >= state->length) {
        if (assumeDictionary) {
            object = objc_alloc_init(dictionaryClass);
        } else {
            jsonReadError(state, error, (id)OF_STR("JSON text did not have any content"));
        }
    } else if (assumeDictionary) {
        object = newJSONObject(state, YES, error);
    } else if ((options & 4) || (state->bytes[state->cursor] | 0x20) == 0x7b) {
        object = newJSONValue(state, error);
    } else {
        jsonReadError(
            state,
            error,
            (id)OF_STR("JSON text did not start with array or object and option to allow fragments not set.")
        );
    }
    if (object && state->cursor < state->length) {
        BOOL validTail = skipJSONWhitespace(state, error, YES);
        if (!validTail || state->cursor != state->length) {
            if (validTail) {
                jsonReadError(state, error, (id)OF_STR("Garbage at end"));
            }
            objc_release(object);
            object = nil;
        }
    }
    for (NSUInteger i = 0; i < 256; ++i) {
        if (state->cache[i].string) {
            objc_release(state->cache[i].string);
        }
    }
    free(state);
    return objc_autorelease(object);
}

@end

// Native skipJSON5Comment, 0x1c0634d5c. Non-external.
static BOOL skipJSON5Comment(JSONReaderState *state, BOOL singleLine, id *error) {
    if (singleLine) {
        while (state->cursor < state->length) {
            unsigned byte = state->bytes[state->cursor++];
            if (byte == '\n' || byte == '\r') {
                if (byte == '\r' && state->cursor < state->length && state->bytes[state->cursor] == '\n') {
                    ++state->cursor;
                }
                ++state->line;
                state->lineStart = state->cursor;
                return YES;
            }
        }
        state->cursor = state->length;
        return YES;
    }
    while (state->cursor + 1 < state->length) {
        if (state->bytes[state->cursor] == '*' && state->bytes[state->cursor + 1] == '/') {
            state->cursor += 2;
            return YES;
        }
        ++state->cursor;
    }
    state->cursor = state->length;
    jsonReadError(state, error, (id)OF_STR("Unterminated block comment"));
    return NO;
}

static id
_setInCache(JSONStringCacheEntry *cache, id string, uint32_t hash, const uint8_t *bytes, NSUInteger length);
static BOOL parseJSONUnicodePointAtLocation(
    JSONReaderState *state, NSUInteger start, NSUInteger end, uint16_t *point, id *error
);

extern void *uset_openPattern(const uint16_t *pattern, int32_t length, int32_t *status);
extern int8_t uset_contains(const void *set, int32_t point);
extern int32_t utf8_back1SafeBody(const uint8_t *bytes, int32_t start, int32_t index);

@protocol JSONStringMessages
- (NSUInteger)length;
- (void)getCharacters:(uint16_t *)buffer;
- (id)initWithBytesNoCopy:(void *)bytes
                   length:(NSUInteger)length
                 encoding:(NSUInteger)encoding
             freeWhenDone:(BOOL)freeWhenDone;
- (BOOL)getBytes:(void *)bytes
         maxLength:(NSUInteger)length
        usedLength:(NSUInteger *)usedLength
          encoding:(NSUInteger)encoding
           options:(NSUInteger)options
             range:(ONSRange)range
    remainingRange:(ONSRange *)remaining;
@end

// The shared hex-digit comparisons in the native reader's string and number units are inlined.
static inline int jsonHexDigit(unsigned byte) {
    if (byte - '0' <= 9) {
        return (int)byte - '0';
    }
    if ((byte | 0x20) - 'a' <= 5) {
        return (int)(byte | 0x20) - 'a' + 10;
    }
    return -1;
}

// Native newJSONString, 0x1c0634f3c. Non-external.
static id newJSONString(JSONReaderState *state, BOOL allowUnquoted, id *error) {
    const uint8_t *bytes = state->bytes;
    NSUInteger start = state->cursor;
    NSUInteger length = state->length;
    unsigned quote = bytes[start];
    BOOL json5 = (state->options & 8) != 0;
    BOOL quoted = quote == '"' || (json5 && quote == '\'');
    if (quoted) {
        ++start;
    } else if (!allowUnquoted || !json5) {
        jsonReadError(state, error, (id)OF_STR("String without surrounding quotes"));
        return nil;
    } else if ((quote | 0x20) - 'a' > 25 && quote != '$' && quote != '\\' && quote != '_') {
        NSUInteger position = start;
        if ((int8_t)bytes[position] <= -65) {
            position = (NSUInteger)utf8_back1SafeBody(bytes, 0, (int32_t)position);
        }
        uint32_t codePoint = bytes[position++];
        if (codePoint >= 0x80) {
            uint32_t lead = codePoint;
            NSUInteger continuation = lead < 0xe0 ? 1 : lead < 0xf0 ? 2 : 3;
            uint32_t minimum = continuation == 1 ? 0x80 : continuation == 2 ? 0x800 : 0x10000;
            if (lead < 0xc2 || lead > 0xf4 || position + continuation > length) {
                goto invalidUTF8;
            }
            codePoint = lead & (0x7f >> (continuation + 1));
            while (continuation--) {
                unsigned byte = bytes[position++];
                if ((byte ^ 0x80) > 0x3f) {
                    goto invalidUTF8;
                }
                codePoint = (codePoint << 6) | (byte & 0x3f);
            }
            if (codePoint < minimum || codePoint > 0x10ffff || (codePoint >= 0xd800 && codePoint <= 0xdfff)) {
                goto invalidUTF8;
            }
        }
        static dispatch_once_t once;
        static void *identifierInitials;
        // Native ___newJSONString_block_invoke, 0x1c06369c4. Non-external.
        dispatch_once(&once, ^{
          id<JSONStringMessages> pattern = (id)OF_STR("[[:Lu:][:Ll:][:Lt:][:Lm:][:Lo:][:Nl:]]");
          NSUInteger length = [pattern length];
          uint16_t characters[length];
          [pattern getCharacters:characters];
          int32_t status = 0;
          identifierInitials = uset_openPattern(characters, (int32_t)[pattern length], &status);
          if (status > 0) {
              _NSJSONCrashMessage = "Unable to open unicode pattern";
              __builtin_debugtrap();
          }
        });
        if (!uset_contains(identifierInitials, (int32_t)codePoint)) {
            jsonReadError(state, error, (id)OF_STR("Disallowed first character in JSON5 object key"));
            return nil;
        }
    }

    if (start >= length) {
        jsonReadError(state, error, (id)OF_STR("Unexpected end of file during string parse"));
        return nil;
    }

    BOOL escapes = NO;
    uint32_t hash = 2166136261u;
    NSUInteger end = start;
    while (end < length) {
        unsigned byte = bytes[end];
        if (byte < 0x20) {
            if (!quoted && (byte == '\r' || byte == '\n')) {
                ++state->line;
                state->lineStart = end;
                if (byte == '\r' && end + 1 < length && bytes[end + 1] == '\n') {
                    ++state->lineStart;
                }
                break;
            }
            state->cursor = end;
            jsonReadError(state, error, (id)OF_STR("Unescaped control character"));
            return nil;
        }
        hash = (hash ^ byte) * 16777619u;
        if (byte == '\\') {
            unsigned escaped = end + 1 < length ? bytes[end + 1] : 0;
            end += 2;
            escapes = YES;
            if (escaped == 'u') {
                end += 4;
            } else if (json5 && escaped == 'x') {
                end += 2;
            } else if (json5 && (escaped == '\n' || escaped == '\r')) {
                if (escaped == '\r' && end < length && bytes[end] == '\n') {
                    ++end;
                }
                ++state->line;
                state->lineStart = end;
            }
        } else {
            if ((quoted && byte == quote) ||
                (!quoted &&
                 (byte == ':' || (byte & 0x7f) == 0x20 ||
                  (byte == '/' && end + 1 < length && (bytes[end + 1] == '/' || bytes[end + 1] == '*'))))) {
                break;
            }
            ++end;
        }
    }
    if (end >= length) {
        jsonReadError(state, error, (id)OF_STR("Unterminated string"));
        return nil;
    }
    if (end == start) {
        state->cursor = start + 1;
        return objc_alloc_init((Class)((state->options & 2) ? jsonMutableStringClass : jsonStringClass));
    }

    NSUInteger byteCount = end - start;
    JSONStringCacheEntry *cached = &state->cache[(uint8_t)hash];
    if (cached->hash == hash && cached->length == byteCount &&
        memcmp(cached->bytes, bytes + start, byteCount) == 0 && cached->string) {
        state->cursor = end + quoted;
        return (state->options & 2) ? [cached->string mutableCopy] : objc_retain(cached->string);
    }

    id string;
    if (!escapes) {
        string = [[ONSString alloc] initWithBytes:bytes + start length:byteCount encoding:4];
        if (!string) {
            jsonReadError(state, error, (id)OF_STR("Unable to convert data to string"));
            return nil;
        }
    } else {
        NSUInteger capacity = byteCount;
        uint8_t *decoded = malloc_type_malloc(capacity, 0x100004077774924);
        NSUInteger used = 0;
        for (NSUInteger cursor = start; cursor < end; ++cursor) {
            if (used >= capacity) {
                capacity = capacity * 2 <= 6 ? 6 : capacity * 2;
                __block bool failed = false;
                // Native ___newJSONString_block_invoke_2, 0x1c0636ae4. Non-external.
                decoded = __OCFSafelyReallocateTyped(decoded, capacity, 511909056, ^(void *old, bool *stop) {
                  free(old);
                  *stop = true;
                  failed = true;
                });
                if (failed) {
                    jsonReadError(state, error, (id)OF_STR("Memory exhausted during parse of string."));
                    return nil;
                }
            }
            unsigned byte = bytes[cursor];
            if (byte != '\\') {
                decoded[used++] = byte;
                continue;
            }

            id reason = nil;
            if (cursor + 1 >= end) {
                reason = (id)OF_STR("Unfinished control character");
            } else {
                unsigned escaped = bytes[cursor + 1];
                switch (escaped) {
                    case '"':
                    case '/':
                    case '\\':
                        byte = escaped;
                        break;
                    case 'b':
                        byte = '\b';
                        break;
                    case 'f':
                        byte = '\f';
                        break;
                    case 'n':
                        byte = '\n';
                        break;
                    case 'r':
                        byte = '\r';
                        break;
                    case 't':
                        byte = '\t';
                        break;
                    case '\'':
                    case '\n':
                    case '\r':
                        if (!json5) {
                            reason = (id)OF_STR("Invalid escape sequence");
                        } else if (escaped == '\r' || escaped == '\n') {
                            if (escaped == '\r' && cursor + 2 < end - 1 && bytes[cursor + 2] == '\n') {
                                ++cursor;
                            }
                            byte = '\n';
                        } else {
                            byte = '\'';
                        }
                        break;
                    case '0':
                        reason = json5 ? (id)OF_STR("Unsupported escaped null")
                                       : (id)OF_STR("Invalid escape sequence");
                        break;
                    case 'x':
                        if (!json5) {
                            reason = (id)OF_STR("Invalid escape sequence");
                        } else {
                            int high = jsonHexDigit(bytes[cursor + 2]);
                            int low = jsonHexDigit(bytes[cursor + 3]);
                            if (high < 0 || low < 0) {
                                reason = (id)OF_STR("Unable to convert hex ('x') escape sequence character");
                            } else if (!high && !low) {
                                reason = (id)OF_STR("Unsupported escaped (hex) null");
                            } else {
                                byte = 16 * high + low;
                                cursor += 2;
                            }
                        }
                        break;
                    case 'u': {
                        uint16_t points[2];
                        if (!parseJSONUnicodePointAtLocation(state, cursor + 2, end - 1, &points[0], error)) {
                            free(decoded);
                            return nil;
                        }
                        if (json5 && points[0] == 0) {
                            reason = (id)OF_STR("Unsupported escaped (unicode) null");
                            break;
                        }
                        BOOL high = (points[0] & 0xfc00) == 0xd800;
                        if (high) {
                            if (cursor + 11 >= end) {
                                reason = (id)OF_STR(
                                    "Unexpected end of file during string parse (expected low-surrogate code "
                                    "point but did not find one)."
                                );
                                break;
                            }
                            if (bytes[cursor + 6] != '\\' || bytes[cursor + 7] != 'u') {
                                reason = (id)OF_STR("Missing low code point in surrogate pair");
                                break;
                            }
                            if (!parseJSONUnicodePointAtLocation(
                                    state, cursor + 8, end - 1, &points[1], error
                                )) {
                                free(decoded);
                                return nil;
                            }
                            if ((points[1] & 0xfc00) != 0xdc00) {
                                reason = (id)OF_STR("Invalid surrogate pair in unicode escape sequence");
                                break;
                            }
                        }
                        id<JSONStringMessages> unicode =
                            [(id<JSONStringMessages>)[ONSString alloc] initWithBytesNoCopy:points
                                                                                    length:high ? 4 : 2
                                                                                  encoding:0x94000100
                                                                              freeWhenDone:NO];
                        NSUInteger written = 0;
                        BOOL converted = [unicode getBytes:decoded + used
                                                 maxLength:6
                                                usedLength:&written
                                                  encoding:4
                                                   options:0
                                                     range:(ONSRange){0, [unicode length]}
                                            remainingRange:NULL];
                        objc_release(unicode);
                        if (!converted) {
                            reason = high ? (id)OF_STR(
                                                "Unable to convert hex escape sequence (with high character) "
                                                "to UTF8-encoded character"
                                            )
                                          : (id)OF_STR(
                                                "Unable to convert hex escape sequence (no high character) "
                                                "to UTF8-encoded character."
                                            );
                            break;
                        }
                        used += written;
                        cursor += high ? 11 : 5;
                        continue;
                    }
                    default:
                        reason = (id)OF_STR("Invalid escape sequence");
                        break;
                }
                if (!reason) {
                    decoded[used++] = byte;
                    ++cursor;
                }
            }
            if (reason) {
                if (cursor + 1 < end) {
                    state->cursor = cursor;
                }
                jsonReadError(state, error, reason);
                free(decoded);
                return nil;
            }
        }
        id<JSONStringMessages> allocated =
            (id<JSONStringMessages>)objc_alloc((Class)((state->options & 2) ? jsonMutableStringClass
                                                                            : jsonStringClass));
        string = [allocated initWithBytesNoCopy:decoded length:used encoding:4 freeWhenDone:YES];
        if (!string) {
            jsonReadError(state, error, (id)OF_STR("Unable to convert data to string"));
            free(decoded);
            return nil;
        }
    }
    state->cursor = end + quoted;
    _setInCache(state->cache, string, hash, bytes + start, byteCount);
    return string;

invalidUTF8:
    jsonReadError(state, error, (id)OF_STR("Unparseable UTF8 character"));
    return nil;
}

// Native _setInCache, 0x1c0636a88. Non-external.
static id
_setInCache(JSONStringCacheEntry *cache, id string, uint32_t hash, const uint8_t *bytes, NSUInteger length) {
    JSONStringCacheEntry *entry = &cache[(uint8_t)hash];
    if (entry->string) {
        objc_release(entry->string);
    }
    id retained = objc_retain(string);
    entry->length = length;
    entry->string = retained;
    entry->hash = hash;
    entry->bytes = bytes;
    return retained;
}

// Native parseJSONUnicodePointAtLocation, 0x1c0636b20. Non-external.
static BOOL parseJSONUnicodePointAtLocation(
    JSONReaderState *state, NSUInteger start, NSUInteger end, uint16_t *point, id *error
) {
    if (start + 3 > end) {
        state->cursor = start;
        jsonReadError(state, error, (id)OF_STR("Early end of unicode escape sequence"));
        return NO;
    }
    uint16_t result = 0;
    for (NSUInteger i = 0; i < 4; ++i) {
        int digit = jsonHexDigit(state->bytes[start + i]);
        if (digit < 0) {
            state->cursor = start + i;
            jsonReadError(state, error, (id)OF_STR("Invalid hex digit in unicode escape sequence"));
            return NO;
        }
        result += (unsigned)digit << (12 - 4 * i);
    }
    *point = result;
    return YES;
}

// Native newJSON5HexNumber, 0x1c0636d04. Non-external.
static id newJSON5HexNumber(JSONReaderState *state, id *error) {
    NSUInteger start = state->cursor;
    const uint8_t *bytes = state->bytes;
    BOOL negative = bytes[start] == '-';
    if (negative) {
        if (start + 1 >= state->length) {
            jsonReadError(state, error, (id)OF_STR("Number with minus sign but no digits"));
            return nil;
        }
        ++state->cursor;
    }
    if (bytes[state->cursor] != '0') {
        jsonReadError(state, error, (id)OF_STR("Hex number without leading zero"));
        return nil;
    }
    if (state->cursor + 1 >= state->length) {
        jsonReadError(state, error, (id)OF_STR("Unexpected end of file during JSON parse."));
        jsonReadError(state, error, (id)OF_STR("Hex number without next 'x'"));
        return nil;
    }
    ++state->cursor;
    if ((bytes[state->cursor] | 0x20) != 'x') {
        jsonReadError(state, error, (id)OF_STR("Hex number without 'x'"));
        return nil;
    }
    if (state->cursor + 1 >= state->length) {
        jsonReadError(state, error, (id)OF_STR("Unexpected end of file during JSON parse."));
        jsonReadError(state, error, (id)OF_STR("Hex number without next digit (EoF)"));
        return nil;
    }
    NSUInteger digits = ++state->cursor;
    if (jsonHexDigit(bytes[digits]) < 0) {
        jsonReadError(state, error, (id)OF_STR("Hex number without next digit"));
        return nil;
    }
    do {
        ++state->cursor;
    } while (state->cursor < state->length && jsonHexDigit(bytes[state->cursor]) >= 0);

    uint64_t value = 0;
    uint64_t factor = 1;
    for (NSUInteger i = state->cursor; i > digits;) {
        unsigned digit = (unsigned)jsonHexDigit(bytes[--i]);
        uint64_t contribution;
        BOOL overflow = __builtin_mul_overflow(factor, digit, &contribution);
        overflow |= __builtin_add_overflow(value, contribution, &value);
        if (i > digits) {
            overflow |= __builtin_mul_overflow(factor, 16, &factor);
        }
        if (overflow) {
            jsonReadError(state, error, (id)OF_STR("Hex number with underflow or overflow"));
            return nil;
        }
    }
    if (negative && value > (UINT64_C(1) << 63)) {
        jsonReadError(state, error, (id)OF_STR("Hex number with underflow"));
        return nil;
    }
    id number = negative ? (id)[[ONSNumber alloc] initWithLongLong:(long long)(0 - value)]
                         : [(id<JSONNumberMessages>)[ONSNumber alloc] initWithUnsignedLongLong:value];
    return newJSONNumberCreateRoundTripping(state, start, number);
}

// Native newJSONNumber, 0x1c0637378. Non-external.
static id newJSONNumber(JSONReaderState *state, id *error) {
    NSUInteger start = state->cursor;
    NSUInteger end = state->length;
    const uint8_t *bytes = state->bytes;
    BOOL negative = bytes[start] == '-';
    BOOL json5 = (state->options & 8) != 0;
    if (negative) {
        if (start + 1 >= end) {
            jsonReadError(state, error, (id)OF_STR("Number with minus sign but no digits"));
            return nil;
        }
        ++state->cursor;
        unsigned first = bytes[state->cursor];
        if (first - '0' > 9 && !(json5 && first == '.')) {
            jsonReadError(state, error, (id)OF_STR("Number with minus sign but no digits"));
            return nil;
        }
    }
    NSUInteger digits = state->cursor;
    unsigned first = bytes[digits];
    if (first == '0') {
        ++state->cursor;
        if (state->cursor == end) {
            return newJSONNumberCreateRoundTripping(state, start, [[ONSNumber alloc] initWithLongLong:0]);
        }
        if ((unsigned)bytes[state->cursor] - '0' <= 9) {
            jsonReadError(state, error, (id)OF_STR("Number with leading zero"));
            return nil;
        }
    } else if (first - '0' <= 9) {
        do {
            ++state->cursor;
        } while (state->cursor < end && (unsigned)bytes[state->cursor] - '0' <= 9);
    } else {
        if (first != '.' || !json5) {
            jsonReadError(state, error, (id)OF_STR("Malformed number"));
            return nil;
        }
        if (state->cursor + 1 >= end) {
            jsonReadError(state, error, (id)OF_STR("Decimal point before EoF"));
            return nil;
        }
        if ((unsigned)bytes[state->cursor + 1] - '0' > 9) {
            jsonReadError(state, error, (id)OF_STR("Decimal point with no digits"));
            return nil;
        }
    }

    NSUInteger integerEnd = state->cursor;
    BOOL fraction = NO;
    if (state->cursor < end && bytes[state->cursor] == '.') {
        if (state->cursor + 1 < end || json5) {
            ++state->cursor;
        }
        if (!json5 && (bytes[state->cursor] == '.' || (unsigned)bytes[state->cursor] - '0' > 9)) {
            jsonReadError(state, error, (id)OF_STR("Number with decimal point but no additional digits"));
            return nil;
        }
        while (state->cursor < end && (unsigned)bytes[state->cursor] - '0' <= 9) {
            fraction = YES;
            ++state->cursor;
        }
    }

    NSUInteger exponentSize = 0;
    if (state->cursor < end && (bytes[state->cursor] | 0x20) == 'e') {
        if (state->cursor + 1 >= end) {
            jsonReadError(state, error, (id)OF_STR("Number with exponent followed by EoF"));
            return nil;
        }
        ++state->cursor;
        unsigned sign = bytes[state->cursor];
        exponentSize = 1;
        if (sign == '+' || sign == '-') {
            if (state->cursor + 1 >= end) {
                jsonReadError(state, error, (id)OF_STR("Number with '+' or '-' but no additional digits"));
                return nil;
            }
            ++state->cursor;
            ++exponentSize;
            if ((unsigned)bytes[state->cursor] - '0' > 9) {
                jsonReadError(state, error, (id)OF_STR("Number with '+' or '-' but no additional digits"));
                return nil;
            }
        } else if (sign - '0' > 9) {
            jsonReadError(state, error, (id)OF_STR("Number with 'e' but no additional digits"));
            return nil;
        }
        do {
            ++state->cursor;
            ++exponentSize;
            if (exponentSize == 6) {
                state->cursor = start;
                jsonReadError(state, error, (id)OF_STR("Number wound up as NaN"));
                return nil;
            }
        } while (state->cursor < end && (unsigned)bytes[state->cursor] - '0' <= 9);
    } else if (!fraction) {
        uint64_t value = 0;
        uint64_t factor = 1;
        BOOL overflow = NO;
        for (NSUInteger i = integerEnd; i > digits;) {
            unsigned digit = bytes[--i] - '0';
            uint64_t contribution;
            overflow = __builtin_mul_overflow(factor, digit, &contribution);
            overflow |= __builtin_add_overflow(value, contribution, &value);
            if (i > digits) {
                overflow |= __builtin_mul_overflow(factor, 10, &factor);
            }
            if (overflow) {
                break;
            }
        }
        if (!overflow && (!negative || value <= (UINT64_C(1) << 63))) {
            id number = negative ? (id)[[ONSNumber alloc] initWithLongLong:(long long)(0 - value)]
                                 : [(id<JSONNumberMessages>)[ONSNumber alloc] initWithUnsignedLongLong:value];
            return newJSONNumberCreateRoundTripping(state, start, number);
        }
    }

    NSUInteger count = state->cursor - start;
    if (!count) {
        jsonReadError(state, error, (id)OF_STR("Invalid number"));
        return nil;
    }
    NSUInteger limit = negative ? 25 : 24;
    if (!fraction) {
        limit -= 2;
    } else if (count < (negative ? 4 : 3) || bytes[digits] != '0' || bytes[digits + 1] != '.') {
        --limit;
    }
    id number = nil;
    if (count <= exponentSize + limit - 5) {
        char buffer[32];
        memcpy(buffer, bytes + start, count);
        buffer[count] = '\0';
        int savedError = errno;
        errno = 0;
        char *endPointer = NULL;
        double value = strtod_l(buffer, &endPointer, NULL);
        if (!((value == 0.0 && *endPointer) || (value == INFINITY && errno == ERANGE))) {
            number = [[ONSNumber alloc] initWithDouble:value];
        }
        errno = savedError;
    }
    if (!number) {
        *error = nil;
        id<JSONStringMessages> allocated = (id<JSONStringMessages>)[ONSString alloc];
        id string = [allocated initWithBytesNoCopy:(void *)(bytes + start)
                                            length:count
                                          encoding:1
                                      freeWhenDone:NO];
        ONSDecimalNumber *decimal =
            (ONSDecimalNumber *)[(id<JSONNumberMessages>)[ONSDecimalNumber alloc] initWithString:string];
        objc_release(string);
        if (decimal) {
            ONSDecimal value = [decimal decimalValue];
            uint16_t flags;
            memcpy(&flags, &value, sizeof(flags));
            if ((flags & 0x1f00) != 0x1000) {
                return newJSONNumberCreateRoundTripping(state, start, decimal);
            }
        }
        state->cursor = start;
        jsonReadError(state, error, (id)OF_STR("Number wound up as NaN"));
        objc_release(decimal);
        return nil;
    }
    return newJSONNumberCreateRoundTripping(state, start, number);
}

// Native newJSON5InfinityOrNaN, 0x1c0637d20. Non-external.
static id newJSON5InfinityOrNaN(JSONReaderState *state, id *error) {
    const uint8_t *bytes = state->bytes;
    unsigned sign = bytes[state->cursor];
    BOOL negative = sign == '-';
    if (negative || sign == '+') {
        if (state->cursor + 1 >= state->length) {
            jsonReadError(
                state, error, (id)OF_STR("Partial negative infinity or NaN around character %lu (EoF).")
            );
            return nil;
        }
        ++state->cursor;
    }
    NSUInteger start = state->cursor;
    double value;
    if (bytes[start] == 'N') {
        if (start + 2 >= state->length) {
            jsonReadError(state, error, (id)OF_STR("Partial NaN around character %lu (EoF)."));
            return nil;
        }
        state->cursor = start + 2;
        if (bytes[start + 1] != 'a' || bytes[start + 2] != 'N') {
            state->cursor = start;
            jsonReadError(state, error, (id)OF_STR("Invalid NaN around character %lu (EoF)."));
            return nil;
        }
        value = negative ? -NAN : NAN;
    } else if (bytes[start] == 'I') {
        if (start + 7 >= state->length) {
            jsonReadError(state, error, (id)OF_STR("Partial infinity around character %lu (EoF)."));
            return nil;
        }
        state->cursor = start + 7;
        if (strncmp((const char *)(bytes + start), "Infinity", 8) != 0) {
            state->cursor = start;
            jsonReadError(state, error, (id)OF_STR("Invalid infinity around character %lu (EoF)."));
            return nil;
        }
        value = negative ? -INFINITY : INFINITY;
    } else {
        jsonReadError(state, error, (id)OF_STR("Invalid Infinity or NaN"));
        return nil;
    }
    ++state->cursor;
    return [[ONSNumber alloc] initWithDouble:value];
}

// Native newJSONNumberCreateRoundTripping, 0x1c0638060. Non-external.
static id newJSONNumberCreateRoundTripping(JSONReaderState *state, NSUInteger start, id number) {
    if (!number || !(state->options & 0x1000)) {
        return number;
    }
    id representation = [[ONSString alloc] initWithBytes:state->bytes + start
                                                  length:state->cursor - start
                                                encoding:4];
    if (!representation) {
        _NSJSONCrashMessage = "Unable to allocate string from UTF-8 JSON data.";
        __builtin_debugtrap();
    }
    id wrapped = [[_ONSJSONRoundTrippingNumber alloc] initWithNumber:number representation:representation];
    objc_release(number);
    objc_release(representation);
    return wrapped;
}

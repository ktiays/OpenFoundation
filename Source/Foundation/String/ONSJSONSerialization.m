//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSJSONSerialization.h"
#import "Foundation/String/ONSJSONReader.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "Foundation/String/ONSString.h"
#import "CoreFoundation/Objective-C/OCFNSArray.h"
#import "CoreFoundation/Objective-C/OCFNSData.h"
#import "CoreFoundation/Objective-C/OCFNSDictionary.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "CoreFoundation/Objective-C/OCFNSLocale.h"
#include "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#include "CoreFoundation/Collections/OCFSortFunctions.h"
#include "CoreFoundation/Collections/OCFNSCFDictionary.h"
#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"
#include "CoreFoundation/String/OCFString.h"

#include <objc/objc-exception.h>
#include <stdlib.h>
#include <math.h>
#include <malloc/malloc.h>
#include <objc/runtime.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <xlocale.h>

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified except shared-cache GOT bindings (medium). iOS Device divergence: iOS Device 24A446
// remains unread. Reading: Local/Scratch/cf-core-r2-fg/Reading.md. The native retains/releases use MRC.

// Native JSON crash-reason slot, 0x22e6fc8d0, zero-fill and non-external.
const char *volatile _NSJSONCrashMessage = NULL;

extern char jsonDataClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSData));
extern char jsonDictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDictionary));
extern char jsonLocaleClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSLocale));

extern char jsonDecimalNumberClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDecimalNumber));
extern char jsonNullClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSNull));
extern char
    jsonRoundTrippingNumberClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(_ONSJSONRoundTrippingNumber));

@protocol LocaleMethods
+ (id)systemLocale;
@end

static BOOL _writeJSONValue(_ONSJSONWriter *writer, id object, NSUInteger depth, NSUInteger options);
static BOOL _writeJSONString(_ONSJSONWriter *writer, OCFStringRef string, NSUInteger options);
static BOOL _writeJSONNumber(_ONSJSONWriter *writer, ONSNumber *number);
static BOOL _writeJSONObject(
    _ONSJSONWriter *writer, id<JSONCollectionMessages> object, NSUInteger depth, NSUInteger options
);
static BOOL _writeJSONArray(_ONSJSONWriter *writer, ONSArray *array, NSUInteger depth, NSUInteger options);
static BOOL _convertJSONString(_ONSJSONWriter *writer, OCFStringRef string, NSUInteger options, BOOL ascii);

extern void objc_release(id object);
extern void objc_setProperty_atomic(id object, SEL selector, id value, ptrdiff_t offset);

@protocol JSONWriterStringMessages
- (BOOL)getBytes:(void *)buffer
         maxLength:(NSUInteger)maximum
        usedLength:(NSUInteger *)used
          encoding:(NSUInteger)encoding
           options:(NSUInteger)options
             range:(ONSRange)range
    remainingRange:(ONSRange *)remaining;
@end

@implementation _ONSJSONWriter

// Native -[_NSJSONWriter init], 0x1c06306a0. Non-external.
- (id)init {
    self = [super init];
    if (self != nil) {
        tempBuffer = NULL;
        tempBufferLen = 0;
        totalDataWritten = 0;
    }
    return self;
}

// Native -[_NSJSONWriter dealloc], 0x1c063070c. Non-external.
- (void)dealloc {
    objc_release(_failure);
    if (tempBuffer != NULL) {
        free(tempBuffer);
    }
    if (dataBuffer != NULL && freeDataBuffer) {
        free(dataBuffer);
    }
    [super dealloc];
}

// Native -[_NSJSONWriter dataWithRootObject:options:], 0x1c06307a4. Non-external.
- (id)dataWithRootObject:(id)object options:(NSUInteger)options {
    dataBuffer = malloc_type_malloc(4096, 0x100004077774924);
    dataBufferLen = 4096;
    dataLen = 0;
    freeDataBuffer = true;
    kind = 1;
    if (!_writeJSONValue(self, object, 0, options)) {
        return nil;
    }
    freeDataBuffer = false;
    return [(id<JSONCollectionMessages>)jsonDataClass dataWithBytesNoCopy:dataBuffer
                                                                   length:dataLen
                                                             freeWhenDone:YES];
}

// Native -[_NSJSONWriter resizeTemporaryBuffer:], 0x1c0630bd4. Non-external.
- (void)resizeTemporaryBuffer:(NSUInteger)size {
    if (tempBufferLen < size && tempBufferLen < 8192) {
        NSUInteger capacity = size < 256 ? 256 : size;
        if (capacity >= 8192) {
            capacity = 8192;
        }
        if (tempBuffer) {
            free(tempBuffer);
        }
        tempBuffer = malloc_type_malloc(capacity, 0x100004077774924);
        tempBufferLen = capacity;
    }
}

// Native -[_NSJSONWriter appendString:range:], 0x1c0630c48. Non-external.
// Assembly puts the writer in x0 for both _appendBytes calls and calls the buffer resize directly.
// The conversion's own return value is discarded, as in the capture.
- (BOOL)appendString:(id)string range:(ONSRange)range {
    [self resizeTemporaryBuffer:range.length];
    id<JSONWriterStringMessages> source = string;
    ONSRange remaining = {0, 0};
    NSUInteger used = 0;
    [source getBytes:tempBuffer
             maxLength:tempBufferLen
            usedLength:&used
              encoding:4
               options:0
                 range:range
        remainingRange:&remaining];
    BOOL appended = _appendBytes(self, tempBuffer, used);
    if (!appended || remaining.length == 0) {
        return appended;
    }
    do {
        [source getBytes:tempBuffer
                 maxLength:tempBufferLen
                usedLength:&used
                  encoding:4
                   options:0
                     range:remaining
            remainingRange:&remaining];
        appended = _appendBytes(self, tempBuffer, used);
    } while (remaining.length != 0 && appended);
    return appended;
}

// Native -[_NSJSONWriter failure], 0x1c0630eac. Non-external.
- (ONSError *)failure {
    return objc_getProperty(self, _cmd, 80, YES);
}

// Native -[_NSJSONWriter setFailure:], 0x1c0630eb8. Non-external.
- (void)setFailure:(ONSError *)error {
    objc_setProperty_atomic(self, _cmd, error, (char *)&_failure - (char *)self);
}

@end

// Native _writeJSONValue, 0x1c0630848. Non-external. The assembly preserves depth/options in calls
// to _writeJSONString, _writeJSONObject and _writeJSONArray, which Hex-Rays omits.
static BOOL _writeJSONValue(_ONSJSONWriter *writer, id object, NSUInteger depth, NSUInteger options) {
    if (_ONSIsNSString(object)) {
        return _writeJSONString(writer, (OCFStringRef)object, options);
    }
    if (objc_opt_isKindOfClass(object, (id)jsonDecimalNumberClass)) {
        if (object) {
            ONSDecimal decimal = [(ONSDecimalNumber *)object decimalValue];
            uint16_t flags;
            memcpy(&flags, &decimal, sizeof(flags));
            if ((flags & 0x1f00) == 0x1000) {
                objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                              reason:(id)OF_STR("NaN number in JSON write")
                                                            userInfo:nil]);
            }
        }
        ONSString *string = [object stringValue];
        return [writer appendString:string range:(ONSRange){0, [string length]}];
    }
    if (_ONSIsNSNumber(object)) {
        OCFTypeID type = OCFGetTypeID(object);
        if (type == OCFBooleanGetTypeID()) {
            if ([object boolValue]) {
                return _appendBytes(writer, "true", 4);
            }
            return _appendBytes(writer, "false", 5);
        }
        return _writeJSONNumber(writer, object);
    }
    if (_ONSIsNSDictionary(object)) {
        return _writeJSONObject(writer, object, depth, options);
    }
    if (_ONSIsNSArray(object)) {
        return _writeJSONArray(writer, object, depth, options);
    }
    if (!objc_opt_isKindOfClass(object, (id)jsonNullClass)) {
        id reason = [ONSString
            stringWithFormat:(id)OF_STR("Invalid type in JSON write (%@)"), objc_opt_class(object)];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    return _appendBytes(writer, "null", 4);
}

// Native _appendBytes, 0x1c0630d48. Non-external. Hidden across the FG and FO units.
// Hex-Rays presents the scratch W8 as an extra parameter. Only kind 1 and kind 2 are set on
// the captured writer entry paths. The other path returns the incoming W8 bit (unread callers).
BOOL _appendBytes(_ONSJSONWriter *writer, const char *bytes, size_t length) {
    if (writer->kind == 1) {
        NSUInteger required = writer->dataLen + length;
        if (required >= writer->dataBufferLen) {
            NSUInteger capacity = writer->dataBufferLen * 2;
            if (required > capacity) {
                capacity = required;
            }
            writer->dataBuffer = __OCFSafelyReallocateTyped(writer->dataBuffer, capacity, 0x81d20417, NULL);
            writer->dataBufferLen = capacity;
        }
        memcpy(writer->dataBuffer + writer->dataLen, bytes, length);
        writer->dataLen += length;
        return YES;
    }
    if (writer->kind != 2) {
        __builtin_unreachable();
    }
    unsigned int emptyWrites = 0;
    NSInteger errorCode;
    while (length) {
        NSInteger written = [writer->outputStream write:(const uint8_t *)bytes maxLength:length];
        if (written < 0) {
            errorCode = 512;
            goto failed;
        }
        if (written == 0) {
            if (++emptyWrites >= 11) {
                errorCode = 640;
                goto failed;
            }
            pthread_yield_np();
        } else {
            bytes += written;
            writer->totalDataWritten += written;
            length -= written;
        }
    }
    return YES;

failed:
    [writer setFailure:[ONSError errorWithDomain:ONSCocoaErrorDomain
                                            code:errorCode
                                        userInfo:(id)&__ONSDictionary0__struct]];
    return NO;
}

@implementation ONSJSONSerialization

// Native +[NSJSONSerialization allocWithZone:], 0x1c063340c. Non-external.
+ (id)allocWithZone:(struct _NSZone *)zone {
    id proem = _ONSMethodExceptionProem(self, _cmd);
    id reason = [ONSString
        stringWithFormat:(id)OF_STR("%@: Do not create instances of NSJSONSerialization in this release"),
                         proem];
    objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                  reason:reason
                                                userInfo:nil]);
}

// Native -[NSJSONSerialization init], 0x1c0633460. Non-external.
- (id)init {
    id proem = _ONSMethodExceptionProem(self, _cmd);
    id reason = [ONSString
        stringWithFormat:(id)OF_STR("%@: Do not create instances of NSJSONSerialization in this release"),
                         proem];
    objc_release(self);
    objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                  reason:reason
                                                userInfo:nil]);
}

// Native +[NSJSONSerialization dataWithJSONObject:options:error:], 0x1c06334d4. Non-external.
+ (id)dataWithJSONObject:(id)object options:(NSUInteger)options error:(id *)error {
    if (!object) {
        id reason = [ONSString
            stringWithFormat:(id)OF_STR("%@: value parameter is nil"), _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (!(options & 4) && !_ONSIsNSArray(object) && !_ONSIsNSDictionary(object)) {
        id reason = [ONSString stringWithFormat:(id)OF_STR("%@: Invalid top-level type in JSON write"),
                                                _ONSMethodExceptionProem(self, _cmd)];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    _ONSJSONWriter *writer = objc_autorelease([[_ONSJSONWriter alloc] init]);
    id data = [writer dataWithRootObject:object options:options];
    if (!data && error) {
        *error = [writer failure];
    }
    return data;
}

// Native +[NSJSONSerialization JSONObjectWithData:options:error:], 0x1c06335e4. Non-external.
+ (id)JSONObjectWithData:(id)data options:(NSUInteger)options error:(id *)error {
    if (data && (options & 0x14) != 0x14) {
        _ONSJSONReader *reader = objc_autorelease([[_ONSJSONReader alloc] init]);
        return [reader parseData:data options:options error:error];
    }
    id reason = data ? (id)OF_STR(
                           "NSJSONReadingAssumeTopLevelDictionary and NSJSONReadingAllowFragments cannot be "
                           "set at the same time"
                       )
                     : (id)OF_STR("data parameter is nil");
    objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                  reason:reason
                                                userInfo:nil]);
}

@end

// Native _writeJSONString, 0x1c06338e8. Non-external. The assembly supplies options and an ASCII-first
// flag, then retries from the original data length with UTF-8.
static BOOL _writeJSONString(_ONSJSONWriter *writer, OCFStringRef string, NSUInteger options) {
    NSUInteger start = writer->kind == 1 ? writer->dataLen : 0;
    if (!_convertJSONString(writer, string, options, YES)) {
        writer->dataLen = start;
        if (!_convertJSONString(writer, string, options, NO)) {
            id description =
                [ONSString stringWithFormat:(id)OF_STR("The string %p failed to convert to UTF8"), string];
            id info = [(id<JSONCollectionMessages>)jsonDictionaryClass
                dictionaryWithObjectsAndKeys:description, (id)kOCFErrorDebugDescriptionKey, nil];
            [writer setFailure:[ONSError errorWithDomain:ONSCocoaErrorDomain code:3852 userInfo:info]];
            return NO;
        }
    }
    if (writer->kind == 2) {
        if (!_appendBytes(writer, writer->dataBuffer, writer->dataLen)) {
            return NO;
        }
        writer->dataLen = 0;
    }
    return YES;
}

// Native _writeJSONNumber, 0x1c0633a10. Non-external.
static BOOL _writeJSONNumber(_ONSJSONWriter *writer, ONSNumber *number) {
    if (object_getClass(number) == (Class)jsonRoundTrippingNumberClass) {
        const char *bytes = [(ONSString *)[number stringValue] UTF8String];
        if (!bytes) {
            _NSJSONCrashMessage = "Unable to allocate UTF-8 representation of JSON number.";
            __builtin_debugtrap();
        }
        return _appendBytes(writer, bytes, strlen(bytes));
    }

    char temporary[32];
    char *buffer = NULL;
    if (writer->kind == 2) {
        memset(temporary, 0, sizeof(temporary));
        buffer = temporary;
    } else if (writer->kind == 1) {
        NSUInteger required = writer->dataLen + 33;
        if (required >= writer->dataBufferLen) {
            NSUInteger capacity = writer->dataBufferLen * 2;
            if (required > capacity) {
                capacity = required;
            }
            writer->dataBuffer = __OCFSafelyReallocateTyped(writer->dataBuffer, capacity, 0x81d20417, NULL);
            writer->dataBufferLen = capacity;
        }
        buffer = writer->dataBuffer + writer->dataLen;
    }

    int length;
    switch ((unsigned char)*[number objCType]) {
        case 'B':
        case 'c':
            length = snprintf_l(buffer, 32, NULL, "%d", [number charValue]);
            break;
        case 'C':
            length = snprintf_l(buffer, 32, NULL, "%u", (unsigned int)[number unsignedCharValue]);
            break;
        case 's':
            length = snprintf_l(buffer, 32, NULL, "%hi", [number shortValue]);
            break;
        case 'S':
            length = snprintf_l(buffer, 32, NULL, "%hu", [number unsignedShortValue]);
            break;
        case 'i':
            length = snprintf_l(buffer, 32, NULL, "%d", [number intValue]);
            break;
        case 'I':
            length = snprintf_l(buffer, 32, NULL, "%u", [number unsignedIntValue]);
            break;
        case 'l':
            length = snprintf_l(buffer, 32, NULL, "%ld", [number longValue]);
            break;
        case 'L':
            length = snprintf_l(buffer, 32, NULL, "%lu", [number unsignedLongValue]);
            break;
        case 'q':
            length = snprintf_l(buffer, 32, NULL, "%lld", [number longLongValue]);
            break;
        case 'Q':
            length = snprintf_l(buffer, 32, NULL, "%llu", [number unsignedLongLongValue]);
            break;
        case 'd':
        case 'f': {
            double value = [number doubleValue];
            if (isnan(value)) {
                objc_exception_throw([ONSException
                    exceptionWithName:(id)ONSInvalidArgumentException
                               reason:(id)OF_STR("Invalid number value (NaN) in JSON write")
                             userInfo:nil]);
            }
            if (fabs(value) == INFINITY) {
                objc_exception_throw([ONSException
                    exceptionWithName:(id)ONSInvalidArgumentException
                               reason:(id)OF_STR("Invalid number value (infinite) in JSON write")
                             userInfo:nil]);
            }
            length = snprintf_l(buffer, 32, NULL, "%0.*g", 17, [number doubleValue]);
            break;
        }
        default: {
            id reason = [ONSString
                stringWithFormat:(id)OF_STR("Invalid number type in JSON write (%c)"), *[number objCType]];
            objc_exception_throw([ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                                          reason:reason
                                                        userInfo:nil]);
        }
    }
    if (length < 1) {
        return NO;
    }
    if (length < 32) {
        if (writer->kind == 2) {
            return _appendBytes(writer, buffer, length);
        }
        if (writer->kind == 1) {
            writer->dataLen += length;
        }
        return YES;
    }
    ONSString *string = [number stringValue];
    return [writer appendString:string range:(ONSRange){0, [string length]}];
}

// Native _writeJSONObject, 0x1c0633e3c. Non-external.
static BOOL _writeJSONObject(
    _ONSJSONWriter *writer, id<JSONCollectionMessages> object, NSUInteger depth, NSUInteger options
) {
    BOOL pretty = (options & 1) != 0;
    char *indent = NULL;
    if (pretty) {
        indent = __builtin_alloca(2 * depth + 2);
        bzero(indent, 2 * depth + 2);
        memset(indent, ' ', 2 * depth + 2);
    }
    if (!_appendBytes(writer, "{", 1) || (pretty && !_appendBytes(writer, "\n", 1))) {
        return NO;
    }

    NSUInteger count = [object count];
    if (count) {
        __block NSUInteger written = 1;
        __block BOOL success = YES;
        __unsafe_unretained _ONSJSONWriter *target = writer;

        // Native ____writeJSONObject_block_invoke, 0x1c0634884 (FO). Non-external.
        // The assembly passes the writer options to _writeJSONString and depth + 1 to _writeJSONValue.
        void (^writePair)(id, id, BOOL *) = ^(id key, id value, BOOL *stop) {
          if (success && pretty) {
              success = _appendBytes(target, indent, 2 * depth + 2);
          }
          if (success) {
              success = _writeJSONString(target, (OCFStringRef)key, options);
          }
          if (success && pretty) {
              success = _appendBytes(target, " ", 1);
          }
          if (success) {
              success = _appendBytes(target, ":", 1);
          }
          if (success && pretty) {
              success = _appendBytes(target, " ", 1);
          }
          if (success) {
              success = _writeJSONValue(target, value, depth + 1, options);
          }
          if (success && written < count) {
              success = _appendBytes(target, ",", 1);
              if (success && pretty) {
                  success = _appendBytes(target, "\n", 1);
              }
          }
          if (success) {
              ++written;
          } else {
              *stop = YES;
          }
        };

        if (options & 2) {
            id stackKeys[128];
            id stackObjects[128];
            OCFIndex stackIndexes[128];
            id *keys = stackKeys;
            id *objects = stackObjects;
            OCFIndex *indexes = stackIndexes;
            if (count >= 129) {
                keys = malloc_type_malloc(sizeof(id) * count, 0x80040b8603338);
                objects = malloc_type_malloc(sizeof(id) * count, 0x80040b8603338);
                indexes = malloc_type_malloc(sizeof(OCFIndex) * count, 0x100004000313f17);
            }
            [(id<JSONCollectionMessages>)object getObjects:objects andKeys:keys count:count];
            // The selector stub at 0x1c0634094 spells isNSString__; Hex-Rays drops the suffix.
            for (NSUInteger i = 0; i < count; ++i) {
                if (![(id)keys[i] isNSString__]) {
                    if (count >= 129) {
                        free(keys);
                        free(objects);
                        free(indexes);
                    }
                    objc_exception_throw([ONSException
                        exceptionWithName:(id)ONSInvalidArgumentException
                                   reason:(id)OF_STR("Invalid (non-string) key in JSON dictionary")
                                 userInfo:nil]);
                }
            }
            for (NSUInteger i = 0; i < count; ++i) {
                indexes[i] = i;
            }

            // Native ____writeJSONObject_block_invoke_2, 0x1c0634b24 (FO). Non-external.
            OCFSortIndexes(indexes, count, 0, ^OCFComparisonResult(OCFIndex first, OCFIndex second) {
              ONSString *key = keys[first];
              NSUInteger length = [key length];
              id locale = [(id<LocaleMethods>)jsonLocaleClass systemLocale];
              return (OCFComparisonResult)[key compare:keys[second]
                                               options:577
                                                 range:(ONSRange){0, length}
                                                locale:locale];
            });
            BOOL stop = NO;
            for (NSUInteger i = 0; i < count; ++i) {
                OCFIndex index = indexes[i];
                writePair(keys[index], objects[index], &stop);
                if (stop) {
                    break;
                }
            }
            if (count >= 129) {
                free(keys);
                free(objects);
                free(indexes);
            }
        } else {
            // Native ____writeJSONObject_block_invoke_3, 0x1c0634b80 (FO). Non-external.
            void (^__unsafe_unretained pair)(id, id, BOOL *) = writePair;
            [(id<JSONCollectionMessages>)object
                enumerateKeysAndObjectsUsingBlock:^(id key, id value, BOOL *stop) {
                  if (!_ONSIsNSString(key)) {
                      objc_exception_throw([ONSException
                          exceptionWithName:(id)ONSInvalidArgumentException
                                     reason:(id)OF_STR("Invalid (non-string) key in JSON dictionary")
                                   userInfo:nil]);
                  }
                  pair(key, value, stop);
                }];
        }
        if (!success) {
            return NO;
        }
    }

    if (pretty && (!_appendBytes(writer, "\n", 1) || (depth && !_appendBytes(writer, indent, 2 * depth)))) {
        return NO;
    }
    return _appendBytes(writer, "}", 1);
}

// Native _writeJSONArray, 0x1c063431c. Non-external.
static BOOL _writeJSONArray(_ONSJSONWriter *writer, ONSArray *array, NSUInteger depth, NSUInteger options) {
    BOOL pretty = (options & 1) != 0;
    char *indent = NULL;
    if (pretty) {
        indent = __builtin_alloca(2 * depth + 2);
        bzero(indent, 2 * depth + 2);
        memset(indent, ' ', 2 * depth + 2);
    }
    if (!_appendBytes(writer, "[", 1) || (pretty && !_appendBytes(writer, "\n", 1))) {
        return NO;
    }

    __block BOOL success = YES;
    __unsafe_unretained _ONSJSONWriter *target = writer;
    // Native ____writeJSONArray_block_invoke, 0x1c0634bf8 (FO). Non-external.
    [array enumerateObjectsUsingBlock:^(id value, NSUInteger index, BOOL *stop) {
      if (index) {
          if (!_appendBytes(target, ",", 1)) {
              success = NO;
          }
          if (success && pretty) {
              success = _appendBytes(target, "\n", 1);
          }
      }
      if (success && pretty) {
          success = _appendBytes(target, indent, 2 * depth + 2);
      }
      if (success) {
          success = _writeJSONValue(target, value, depth + 1, options);
      }
      if (!success) {
          *stop = YES;
      }
    }];

    if (!success ||
        (pretty && (!_appendBytes(writer, "\n", 1) || (depth && !_appendBytes(writer, indent, 2 * depth))))) {
        return NO;
    }
    return _appendBytes(writer, "]", 1);
}

// Native _convertJSONString, 0x1c063451c. Non-external.
static BOOL _convertJSONString(_ONSJSONWriter *writer, OCFStringRef string, NSUInteger options, BOOL ascii) {
    OCFIndex length = OCFStringGetLength(string);
    OCFStringEncoding encoding = ascii ? 0 : kOCFStringEncodingUTF8;
    const char *direct = OCFStringGetCStringPtr(string, encoding);
    const unsigned char *bytes = (const unsigned char *)direct;
    if (!direct) {
        if (writer->tempBufferLen < (NSUInteger)length) {
            [writer resizeTemporaryBuffer:length];
        }
        bytes = (const unsigned char *)writer->tempBuffer;
    }

    NSUInteger required = writer->dataLen + 6 * length + 3;
    if (required >= writer->dataBufferLen) {
        NSUInteger capacity = writer->dataBufferLen * 2;
        if (required > capacity) {
            capacity = required;
        }
        writer->dataBuffer = __OCFSafelyReallocateTyped(writer->dataBuffer, capacity, 0x81d20417, NULL);
        writer->dataBufferLen = capacity;
    }
    writer->dataBuffer[writer->dataLen++] = '"';

    OCFIndex location = 0;
    while (location < length) {
        OCFIndex count = 0;
        if (direct) {
            location = length;
            count = length;
        } else {
            OCFRange range = {location, length - location};
            OCFIndex converted = OCFStringGetBytes(
                string, range, encoding, 0, false, (uint8_t *)bytes, writer->tempBufferLen, &count
            );
            if (!converted) {
                return NO;
            }
            location += converted;
        }

        for (OCFIndex i = 0; i < count; ++i) {
            unsigned char byte = bytes[i];
            if (ascii && byte >= 128) {
                return NO;
            }
            if (byte == '\\' || byte == '"' || (byte == '/' && !(options & 8))) {
                writer->dataBuffer[writer->dataLen++] = '\\';
                writer->dataBuffer[writer->dataLen++] = byte;
            } else if (byte > 0x1f) {
                writer->dataBuffer[writer->dataLen++] = byte;
            } else {
                writer->dataBuffer[writer->dataLen++] = '\\';
                switch (byte) {
                    case 8:
                        writer->dataBuffer[writer->dataLen++] = 'b';
                        break;
                    case 9:
                        writer->dataBuffer[writer->dataLen++] = 't';
                        break;
                    case 10:
                        writer->dataBuffer[writer->dataLen++] = 'n';
                        break;
                    case 12:
                        writer->dataBuffer[writer->dataLen++] = 'f';
                        break;
                    case 13:
                        writer->dataBuffer[writer->dataLen++] = 'r';
                        break;
                    default:
                        writer->dataBuffer[writer->dataLen++] = 'u';
                        snprintf_l(writer->dataBuffer + writer->dataLen, 5, NULL, "%04x", (unsigned int)byte);
                        writer->dataLen += 4;
                        break;
                }
            }
        }
    }
    writer->dataBuffer[writer->dataLen++] = '"';
    return YES;
}

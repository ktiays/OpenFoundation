//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFObjCTypeEncoding.h"

#include "CoreFoundation/String/OCFString.h"

#include <limits.h>
#include <objc/message.h>
#include <objc/objc-exception.h>
#include <objc/runtime.h>

// Inferred organization: Objective-C type encoding, iOS Simulator 24A434,
// UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC. Confidence: verified for the parser;
// iOS Device divergence: unread. Reading: Local/Evidence/cf-core/iphone-simulator-corpus-delta13.json.

extern char exceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));

static const char *__NSGetSizeAndAlignment(
    const char *type,
    unsigned long *size,
    unsigned long *alignment,
    const char *encoding,
    uint64_t options,
    uint64_t depth,
    uint64_t arrayProduct
);

// Native NSGetSizeAndAlignment, 0x1c0288550. External.
const char *ONSGetSizeAndAlignment(const char *type, unsigned long *size, unsigned long *alignment) {
    return __NSGetSizeAndAlignment(type, size, alignment, type, 1, 0, 1);
}

// Balanced delimiter scan, inlined in the native pointer, object, array and aggregate cases.
static inline const char *closingDelimiter(const char *start, unsigned char open, unsigned char close) {
    uint64_t nesting = 0;
    for (const char *p = start; *p != '\0'; ++p) {
        if ((unsigned char)*p == close) {
            if (nesting == 0) {
                return p;
            }
            --nesting;
        } else if ((unsigned char)*p == open) {
            ++nesting;
        }
    }
    return NULL;
}

// Native __NSGetSizeAndAlignment, 0x1c0288564. Non-external.
// The assembly's jump table settles the qualifiers and primitive encodings. The array diagnostics use
// unsigned carry and multiplication overflow. Parsing an opaque aggregate still scans its contents as
// encodings if it has no '='. The final '+' skip for a struct or union is bounded by its scanned close.
static const char *__NSGetSizeAndAlignment(
    const char *type,
    unsigned long *size,
    unsigned long *alignment,
    const char *encoding,
    uint64_t options,
    uint64_t depth,
    uint64_t arrayProduct
) {
    if (alignment != NULL) {
        *alignment = 0;
    }
    if (size != NULL) {
        *size = 0;
    }

    const char *p = type;
    OCFStringRef reason;
    for (;;) {
        const char *token = p;
        unsigned char code = (unsigned char)*p++;
        unsigned long width;
        switch (code) {
            case '+':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case 'N':
            case 'O':
            case 'R':
            case 'V':
            case 'n':
            case 'o':
            case 'r':
                continue;
            case '"':
                while (*p != '"' && *p != '\0') {
                    ++p;
                }
                if (*p == '\0') {
                    reason = OCFStringCreateWithFormat(
                        kOCFAllocatorSystemDefault,
                        NULL,
                        OF_STR("NSGetSizeAndAlignment(): unterminated ivar name spec at '%s' in '%s'"),
                        type,
                        encoding
                    );
                    goto throwException;
                }
                ++p;
                continue;
            case '\0':
                return token;
            case 'v':
                return p + (*p == '+');
            case 'B':
            case 'C':
            case 'c':
                width = 1;
                break;
            case 'S':
            case 's':
                width = 2;
                break;
            case 'I':
            case 'L':
            case 'f':
            case 'i':
            case 'l':
                width = 4;
                break;
            case '#':
            case '*':
            case ':':
            case 'D':
            case 'Q':
            case 'd':
            case 'q':
                width = 8;
                break;
            case 'T':
            case 't':
                width = 16;
                break;
            case '@':
                if (alignment != NULL) {
                    *alignment = 8;
                }
                if (size != NULL) {
                    *size = 8;
                }
                if (*p == '"') {
                    ++p;
                    while (*p != '"' && *p != '\0') {
                        ++p;
                    }
                    if (*p == '\0') {
                        reason = OCFStringCreateWithFormat(
                            kOCFAllocatorSystemDefault,
                            NULL,
                            OF_STR(
                                "NSGetSizeAndAlignment(): malformed extended class info spec at '%s' in '%s'"
                            ),
                            type,
                            encoding
                        );
                        goto throwException;
                    }
                    ++p;
                } else if (*p == '?') {
                    ++p;
                    if (*p == '<') {
                        p = closingDelimiter(p + 1, '<', '>');
                        if (p == NULL) {
                            reason = OCFStringCreateWithFormat(
                                kOCFAllocatorSystemDefault,
                                NULL,
                                OF_STR(
                                    "NSGetSizeAndAlignment(): malformed extended block spec at '%s' in '%s'"
                                ),
                                type,
                                encoding
                            );
                            goto throwException;
                        }
                        ++p;
                    }
                }
                return p + (*p == '+');
            case '^': {
                if (depth >= 512) {
                    goto excessiveDepth;
                }
                if (*p == '?' || *p == 'v') {
                    ++p;
                } else if (*p == '(' || *p == '[' || *p == '{') {
                    unsigned char open = (unsigned char)*p++;
                    unsigned char close = open == '(' ? ')' : open == '[' ? ']' : '}';
                    p = closingDelimiter(p, open, close);
                    if (p != NULL) {
                        ++p;
                    }
                } else {
                    p = __NSGetSizeAndAlignment(p, NULL, NULL, encoding, options, depth + 1, arrayProduct);
                }
                if (p == NULL) {
                    reason = OCFStringCreateWithFormat(
                        kOCFAllocatorSystemDefault,
                        NULL,
                        OF_STR("NSGetSizeAndAlignment(): malformed pointed-to-type spec at '%s' in '%s'"),
                        type,
                        encoding
                    );
                    goto throwException;
                }
                width = 8;
                break;
            }
            case '[': {
                if (depth >= 512) {
                    goto excessiveDepth;
                }
                const char *close = closingDelimiter(p, '[', ']');
                if (close == NULL || p == close) {
                    goto malformedArray;
                }
                uint64_t count = 0;
                while ((unsigned)(*p - '0') <= 9) {
                    if (count >= 0x19999999) {
                        reason = OCFStringCreateWithFormat(
                            kOCFAllocatorSystemDefault,
                            NULL,
                            OF_STR(
                                "NSGetSizeAndAlignment(): malformed array spec at '%s' in '%s' -- size is "
                                "too large"
                            ),
                            type,
                            encoding
                        );
                        goto throwException;
                    }
                    count = 10 * count + (unsigned)(*p++ - '0');
                }
                if (p == close) {
                    goto malformedArray;
                }
                uint64_t totalCount;
                if (__builtin_mul_overflow(count, arrayProduct, &totalCount)) {
                    reason = OCFStringCreateWithFormat(
                        kOCFAllocatorSystemDefault,
                        NULL,
                        OF_STR(
                            "NSGetSizeAndAlignment(): cumulative nesting of arrays has reached ridiculous "
                            "proportions at '%s' in '%s'"
                        ),
                        type,
                        encoding
                    );
                    goto throwException;
                }
                @try {
                    unsigned long elementSize = 0;
                    unsigned long elementAlignment = 0;
                    const char *end = __NSGetSizeAndAlignment(
                        p, &elementSize, &elementAlignment, encoding, options, depth + 1, totalCount
                    );
                    if (elementSize == 0) {
                        reason = OCFStringCreateWithFormat(
                            kOCFAllocatorSystemDefault,
                            NULL,
                            OF_STR(
                                "NSGetSizeAndAlignment(): malformed array spec or array too large at '%s' in "
                                "'%s'"
                            ),
                            type,
                            encoding
                        );
                    } else if (end == NULL || end > close || *end != ']') {
                        reason = OCFStringCreateWithFormat(
                            kOCFAllocatorSystemDefault,
                            NULL,
                            OF_STR("NSGetSizeAndAlignment(): malformed array spec at '%s' in '%s'"),
                            type,
                            encoding
                        );
                    } else {
                        if (alignment != NULL) {
                            *alignment = elementAlignment;
                        }
                        if (size == NULL) {
                            ++end;
                            return end + (*end == '+');
                        }
                        unsigned long unit = elementAlignment > 1 ? elementAlignment : 1;
                        bool overflow = elementSize > ULONG_MAX - unit;
                        unsigned long stride = overflow ? 0 : ((elementSize + unit - 1) / unit) * unit;
                        if (overflow || count > ULONG_MAX / stride) {
                            reason = OCFStringCreateWithFormat(
                                kOCFAllocatorSystemDefault,
                                NULL,
                                OF_STR("NSGetSizeAndAlignment(): array too large at '%s' in '%s'"),
                                type,
                                encoding
                            );
                        } else if (totalCount > ULONG_MAX / stride) {
                            reason = OCFStringCreateWithFormat(
                                kOCFAllocatorSystemDefault,
                                NULL,
                                OF_STR(
                                    "NSGetSizeAndAlignment(): cumulative nested array has reached ridiculous "
                                    "size at '%s' in '%s'"
                                ),
                                type,
                                encoding
                            );
                        } else {
                            *size = count * stride;
                            ++end;
                            return end + (*end == '+');
                        }
                    }
                    id autoreleased = _OCFAutoreleasePoolAddObject(NULL, reason);
                    id exception = ((id (*)(id, SEL, OCFStringRef, id, id))objc_msgSend)(
                        (id)exceptionClass,
                        sel_getUid("exceptionWithName:reason:userInfo:"),
                        ONSInvalidArgumentException,
                        autoreleased,
                        nil
                    );
                    objc_exception_throw(exception);
                } @catch (id exception) {
                    (void)exception;
                    @throw;
                }
            }
            case '(':
            case '{': {
                if (depth >= 512) {
                    goto excessiveDepth;
                }
                bool structure = code == '{';
                const char *close = closingDelimiter(p, code, structure ? '}' : ')');
                if (close == NULL) {
                    reason = OCFStringCreateWithFormat(
                        kOCFAllocatorSystemDefault,
                        NULL,
                        OF_STR("NSGetSizeAndAlignment(): malformed %s spec at '%s' in '%s'"),
                        structure ? "struct" : "union",
                        type,
                        encoding
                    );
                    goto throwException;
                }
                for (const char *name = p; *name != '\0' && *name != ')' && *name != '}'; ++name) {
                    if (*name == '=') {
                        p = name + 1;
                        break;
                    }
                }
                unsigned long totalSize = 0;
                unsigned long maxAlignment = 0;
                while (p != NULL && p < close) {
                    unsigned long memberSize;
                    unsigned long memberAlignment;
                    p = __NSGetSizeAndAlignment(
                        p, &memberSize, &memberAlignment, encoding, options, depth + 1, arrayProduct
                    );
                    if (maxAlignment < memberAlignment) {
                        maxAlignment = memberAlignment;
                    }
                    if (structure) {
                        unsigned long unit = memberAlignment > 1 ? memberAlignment : 1;
                        if (totalSize > ~unit - memberSize) {
                            reason = OCFStringCreateWithFormat(
                                kOCFAllocatorSystemDefault,
                                NULL,
                                OF_STR("NSGetSizeAndAlignment(): struct too large at '%s' in '%s'"),
                                type,
                                encoding
                            );
                            goto throwException;
                        }
                        totalSize = ((totalSize + unit - 1) / unit) * unit + memberSize;
                    } else if (totalSize < memberSize) {
                        totalSize = memberSize;
                    }
                }
                const char *end = p;
                if (end != NULL && *end != '\0') {
                    ++end;
                }
                if (alignment != NULL) {
                    *alignment = maxAlignment;
                }
                if (size != NULL) {
                    unsigned long unit = maxAlignment > 1 ? maxAlignment : 1;
                    if (totalSize > ULONG_MAX - unit) {
                        reason = OCFStringCreateWithFormat(
                            kOCFAllocatorSystemDefault,
                            NULL,
                            OF_STR("NSGetSizeAndAlignment(): struct or union too large at '%s' in '%s'"),
                            type,
                            encoding
                        );
                        goto throwException;
                    }
                    *size = ((totalSize + unit - 1) / unit) * unit;
                }
                return end != NULL && end < close && *end == '+' ? end + 1 : end;
            }
            default:
                reason = OCFStringCreateWithFormat(
                    kOCFAllocatorSystemDefault,
                    NULL,
                    OF_STR("NSGetSizeAndAlignment(): unsupported type encoding spec '%c' at '%s' in '%s'"),
                    (int)(int8_t)code,
                    type,
                    encoding
                );
                goto throwException;
        }
        if (alignment != NULL) {
            *alignment = width;
        }
        if (size != NULL) {
            *size = width;
        }
        return p + (*p == '+');
    }

excessiveDepth:
    reason = OCFStringCreateWithFormat(
        kOCFAllocatorSystemDefault,
        NULL,
        OF_STR("NSGetSizeAndAlignment(): type encoding string '%s' contains too many nesting levels"),
        encoding
    );
    goto throwException;
malformedArray:
    reason = OCFStringCreateWithFormat(
        kOCFAllocatorSystemDefault,
        NULL,
        OF_STR("NSGetSizeAndAlignment(): malformed array spec at '%s' in '%s'"),
        type,
        encoding
    );
throwException:;
    id autoreleased = _OCFAutoreleasePoolAddObject(NULL, reason);
    id exception = ((id (*)(id, SEL, OCFStringRef, id, id))objc_msgSend)(
        (id)exceptionClass,
        sel_getUid("exceptionWithName:reason:userInfo:"),
        ONSInvalidArgumentException,
        autoreleased,
        nil
    );
    objc_exception_throw(exception);
}

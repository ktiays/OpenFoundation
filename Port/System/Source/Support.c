//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Darwin's C-locale rune table, ASL to stderr, and libuuid for the wasm port.

#include <asl.h>
#include <runetype.h>
#include <uuid/uuid.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

void arc4random_buf(void *buffer, size_t size);

_RuneLocale _DefaultRuneLocale = {
    .__magic = "RuneMagi",
    .__encoding = "NONE",
};

__attribute__((constructor)) static void initializeRuneLocale(void) {
    for (int character = 0; character < 256; character++) {
        uint32_t type = 0;
        if (character < 0x80) {
            if (isalpha(character)) {
                type |= _CTYPE_A;
            }
            if (iscntrl(character)) {
                type |= _CTYPE_C;
            }
            if (isdigit(character)) {
                type |= _CTYPE_D;
            }
            if (isgraph(character)) {
                type |= _CTYPE_G;
            }
            if (islower(character)) {
                type |= _CTYPE_L;
            }
            if (ispunct(character)) {
                type |= _CTYPE_P;
            }
            if (isspace(character)) {
                type |= _CTYPE_S;
            }
            if (isupper(character)) {
                type |= _CTYPE_U;
            }
            if (isxdigit(character)) {
                type |= _CTYPE_X;
            }
            if (isblank(character)) {
                type |= _CTYPE_B;
            }
            if (isprint(character)) {
                type |= _CTYPE_R;
            }
            // Darwin keeps a digit's value in the low byte.
            if (isdigit(character)) {
                type |= (uint32_t)(character - '0');
            } else if (isxdigit(character)) {
                type |= (uint32_t)(tolower(character) - 'a' + 10);
            }
        }
        _DefaultRuneLocale.__runetype[character] = type;
        _DefaultRuneLocale.__maplower[character] = character < 0x80 ? tolower(character) : character;
        _DefaultRuneLocale.__mapupper[character] = character < 0x80 ? toupper(character) : character;
    }
}

unsigned long __maskrune(int32_t character, unsigned long mask) {
    if (character >= 0 && character < 256) {
        return _DefaultRuneLocale.__runetype[character] & mask;
    }
    unsigned long type = 0;
    if (iswalpha((wint_t)character)) {
        type |= _CTYPE_A;
    }
    if (iswdigit((wint_t)character)) {
        type |= _CTYPE_D;
    }
    if (iswspace((wint_t)character)) {
        type |= _CTYPE_S;
    }
    if (iswupper((wint_t)character)) {
        type |= _CTYPE_U;
    }
    if (iswlower((wint_t)character)) {
        type |= _CTYPE_L;
    }
    if (iswpunct((wint_t)character)) {
        type |= _CTYPE_P;
    }
    if (iswprint((wint_t)character)) {
        type |= _CTYPE_R;
    }
    return type & mask;
}

struct asl_object_s {
    char *message;
};

asl_object_t asl_open(const char *identifier, const char *facility, uint32_t options) {
    (void)identifier;
    (void)facility;
    (void)options;
    return calloc(1, sizeof(struct asl_object_s));
}

asl_object_t asl_new(uint32_t type) {
    (void)type;
    return calloc(1, sizeof(struct asl_object_s));
}

int asl_set(asl_object_t object, const char *key, const char *value) {
    if (object != NULL && key != NULL && strcmp(key, ASL_KEY_MSG) == 0) {
        free(object->message);
        object->message = value != NULL ? strdup(value) : NULL;
    }
    return 0;
}

int asl_send(asl_object_t client, asl_object_t message) {
    (void)client;
    if (message != NULL && message->message != NULL) {
        fprintf(stderr, "%s\n", message->message);
    }
    return 0;
}

void asl_free(asl_object_t object) {
    if (object != NULL) {
        free(object->message);
        free(object);
    }
}

void asl_close(asl_object_t object) {
    asl_free(object);
}

void uuid_clear(uuid_t uu) {
    memset(uu, 0, sizeof(uuid_t));
}

int uuid_compare(const uuid_t uu1, const uuid_t uu2) {
    return memcmp(uu1, uu2, sizeof(uuid_t));
}

void uuid_copy(uuid_t destination, const uuid_t source) {
    memcpy(destination, source, sizeof(uuid_t));
}

void uuid_generate_random(uuid_t out) {
    arc4random_buf(out, sizeof(uuid_t));
    out[6] = (unsigned char)((out[6] & 0x0F) | 0x40);
    out[8] = (unsigned char)((out[8] & 0x3F) | 0x80);
}

void uuid_generate(uuid_t out) {
    uuid_generate_random(out);
}

int uuid_is_null(const uuid_t uu) {
    for (size_t index = 0; index < sizeof(uuid_t); index++) {
        if (uu[index] != 0) {
            return 0;
        }
    }
    return 1;
}

static int hexValue(char character) {
    if (character >= '0' && character <= '9') {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f') {
        return character - 'a' + 10;
    }
    if (character >= 'A' && character <= 'F') {
        return character - 'A' + 10;
    }
    return -1;
}

int uuid_parse(const char *in, uuid_t uu) {
    if (strlen(in) != 36) {
        return -1;
    }
    int byte = 0;
    for (int index = 0; index < 36;) {
        if (index == 8 || index == 13 || index == 18 || index == 23) {
            if (in[index] != '-') {
                return -1;
            }
            index++;
            continue;
        }
        int high = hexValue(in[index]);
        int low = hexValue(in[index + 1]);
        if (high < 0 || low < 0) {
            return -1;
        }
        uu[byte++] = (unsigned char)(high << 4 | low);
        index += 2;
    }
    return 0;
}

static void unparse(const uuid_t uu, char *out, const char *digits) {
    int position = 0;
    for (int index = 0; index < 16; index++) {
        if (index == 4 || index == 6 || index == 8 || index == 10) {
            out[position++] = '-';
        }
        out[position++] = digits[uu[index] >> 4];
        out[position++] = digits[uu[index] & 0xF];
    }
    out[position] = '\0';
}

void uuid_unparse_lower(const uuid_t uu, char *out) {
    unparse(uu, out, "0123456789abcdef");
}

void uuid_unparse_upper(const uuid_t uu, char *out) {
    unparse(uu, out, "0123456789ABCDEF");
}

void uuid_unparse(const uuid_t uu, char *out) {
    uuid_unparse_upper(uu, out);
}

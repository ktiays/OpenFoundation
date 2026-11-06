//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Preferences/OCFPreferencesDescription.h"

#include <assert.h>
#include <malloc/malloc.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: CFPreferencesCopyKeyDescription (0x1c0231340, and its blocks 0x1c023153c and
// 0x1c023157c), CFPreferencesClientDescriptionForMessage (0x1c023160c), CFPreferencesRedactedName
// (0x1c023176c), CFPreferencesRedactedUserIdentifier (0x1c0231820) and CFPreferencesRedactedContainerPath
// (0x1c0231894) lie in one iOS Simulator __text region, restored here in native order. The native source path
// is not in the evidence; the file is named for the cluster. The language is not established (the code is
// C-callable and sends no messages); it is compiled as C. Unless a definition says otherwise: iOS Simulator
// 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full; constant
// strings resolved from the ADRP/ADD encodings); iOS Device 24A446: unread. Reading:
// Local/Scratch/cf-core-prefs-daemon/REPORT.md (main checkout); the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta11.json and, for CFPreferencesCopyKeyDescription,
// CFPreferencesRedactedUserIdentifier and CFPreferencesRedactedContainerPath,
// iphone-simulator-corpus-delta12.json.

// libxpc's xpc_connection_get_audit_token and libproc's proc_name (no iOS Simulator SDK declarations);
// supplied, never reimplemented.
extern void xpc_connection_get_audit_token(xpc_connection_t connection, audit_token_t *token);
extern int proc_name(int pid, void *buffer, uint32_t buffersize);

// Native CFPreferencesCopyKeyDescription, 0x1c0231340, with __CFPreferencesCopyKeyDescription_block_invoke
// (0x1c023153c, descriptor 0x22279db88) and _block_invoke_2 (0x1c023157c, descriptor 0x2227a5948). Hidden
// (was a private external). A malloc'd C string (the caller frees it): an XPC string's copy (strdup); for an
// XPC dictionary, "[" followed by each key and ", " and closed with "]" after the last separator (the
// buffer is calloc'd, malloc type 0x5f368ddd, with the summed key lengths plus 2 each, plus 3; the native
// write index is a 32-bit __block int starting at 1); "no key" otherwise, NULL included. CoreFoundation
// defect: the write offset wraps at 2 GiB (ADD W at 0x1c02315c8 and 0x1c02315f0; LDRSW at 0x1c02315d8 then
// writes before the allocation). Confidence: verified. Use a size_t offset alongside the already-native
// size_t allocation count, preserving the formatted bytes without that overwrite.
char *CFPreferencesCopyKeyDescription(xpc_object_t key) {
    if (key != NULL) {
        xpc_type_t type = xpc_get_type(key);
        if (type == XPC_TYPE_STRING) {
            return strdup(xpc_string_get_string_ptr(key));
        }
        if (type == XPC_TYPE_DICTIONARY) {
            __block size_t size = 0;
            xpc_dictionary_apply(key, ^bool(const char *name, xpc_object_t value) {
              (void)value;
              size += strlen(name) + 2;
              return true;
            });
            char *description = malloc_type_calloc(1, size + 3, 0x5F368DDD);
            description[0] = '[';
            __block size_t index = 1;
            xpc_dictionary_apply(key, ^bool(const char *name, xpc_object_t value) {
              (void)value;
              size_t length = strlen(name);
              strncpy(description + index, name, length);
              index += length;
              memcpy(description + index, ", ", 2);
              index += 2;
              return true;
            });
            description[index] = ']';
            return description;
        }
    }
    return strdup("no key");
}

// Native CFPreferencesClientDescriptionForMessage, 0x1c023160c, with .cold.1 (0x1c030cd10: the assertion of
// the inlined _CFPrefsConnectionForMessage, "conn && xpc_get_type(conn) == XPC_TYPE_CONNECTION",
// CFXPreferences_Internal.h line 427). Hidden (was a private external).
// No message gives pid -1. Otherwise the audit token is the message's "CFPreferencesAuditToken" data when it
// is exactly 32 bytes long, else the token of the message's remote connection, or of its "connection" value
// (which must be an XPC connection). The pid is the token's sixth word. For pid -1, or when proc_name (into a
// zeroed 64-byte buffer) fails, the description is "%d" with the pid; otherwise "%s[%d]" with the name and
// the pid. The string (system default allocator) is autoreleased.
OCFStringRef CFPreferencesClientDescriptionForMessage(xpc_object_t message) {
    audit_token_t token;
    pid_t pid = -1;
    if (message != NULL) {
        memset(&token, 0, sizeof(token));
        size_t length = 0;
        const void *data = xpc_dictionary_get_data(message, "CFPreferencesAuditToken", &length);
        if (data != NULL && length == 32) {
            memcpy(&token, data, sizeof(token));
        } else {
            xpc_connection_t connection = xpc_dictionary_get_remote_connection(message);
            if (connection == NULL) {
                connection = xpc_dictionary_get_value(message, "connection");
                if (connection == NULL || xpc_get_type(connection) != XPC_TYPE_CONNECTION) {
                    __assert_rtn(
                        "_CFPrefsConnectionForMessage",
                        "CFXPreferences_Internal.h",
                        427,
                        "conn && xpc_get_type(conn) == XPC_TYPE_CONNECTION"
                    );
                }
            }
            xpc_connection_get_audit_token(connection, &token);
        }
        pid = (pid_t)token.val[5];
    }
    char name[64];
    memset(name, 0, sizeof(name));
    OCFStringRef description;
    if (pid != -1 && proc_name(pid, name, 64) >= 0) {
        description =
            OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR("%s[%d]"), name, pid);
    } else {
        description = OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR("%d"), pid);
    }
    return OCFAutorelease(description);
}

// Native CFPreferencesRedactedName, 0x1c023176c. Hidden (was a private external).
// With at most one character, "{%ld}" and the length. Otherwise the first character, then: for exactly two
// characters "%C{1}" (the first character only); for more, "%C{%ld}%C" with the first character, the length
// minus two and the last character. Made with the system default allocator and autoreleased (CFAutorelease,
// a tail call).
OCFStringRef CFPreferencesRedactedName(OCFStringRef name) {
    OCFIndex length = OCFStringGetLength(name);
    OCFStringRef redacted;
    if (length - 1 <= 0) {
        redacted = OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR("{%ld}"), length);
    } else {
        UniChar first = OCFStringGetCharacterAtIndex(name, 0);
        OCFIndex hidden = length - 2;
        if (hidden == 0) {
            redacted = OCFStringCreateWithFormat(kOCFAllocatorSystemDefault, NULL, OF_STR("%C{1}"), first);
        } else {
            UniChar last = OCFStringGetCharacterAtIndex(name, length - 1);
            redacted = OCFStringCreateWithFormat(
                kOCFAllocatorSystemDefault, NULL, OF_STR("%C{%ld}%C"), first, hidden, last
            );
        }
    }
    return OCFAutorelease(redacted);
}

// Native CFPreferencesRedactedUserIdentifier, 0x1c0231820. Hidden (was a private external). NULL,
// kCFPreferencesCurrentUser, kCFPreferencesAnyUser (CFEqual with the constants' values) and names shorter
// than 3 characters answer themselves; any other name is redacted by CFPreferencesRedactedName (a tail
// call).
OCFStringRef CFPreferencesRedactedUserIdentifier(OCFStringRef user) {
    if (user == NULL || OCFEqual(user, kOCFPreferencesCurrentUser) ||
        OCFEqual(user, kOCFPreferencesAnyUser) || OCFStringGetLength(user) < 3) {
        return user;
    }
    return CFPreferencesRedactedName(user);
}

// Native CFPreferencesRedactedContainerPath.allowedComponents (__DATA_CONST,__const 0x2227a5968, five C
// string pointers, read with Local/Scratch/cf-core/macho.py). Non-external: the path components shown
// unredacted.
static const char *const allowedComponents[5] = {"Users", "Library", "Containers", "Data", "Preferences"};

// Native CFPreferencesRedactedContainerPath, 0x1c0231894. Hidden (was a private external). NULL answers
// NULL. The path is converted to UTF-8 in a calloc'd buffer (the maximum size plus one, malloc type
// 0x5c5092c1; a failed conversion frees it and answers NULL) and split at "/" (strsep). Into a new mutable
// string (default allocator), every component but the one at the buffer's start is preceded by "/"; an empty
// component adds nothing, one character "{1}", two characters "%c{1}" with the first, a listed component
// "%s" itself, and any other "%c{%zu}%c" with its first character, its length minus two and its last
// character (characters as signed bytes). The buffer is freed and the string autoreleased.
OCFStringRef CFPreferencesRedactedContainerPath(OCFStringRef path) {
    if (path == NULL) {
        return NULL;
    }
    OCFIndex maximum = OCFStringGetMaximumSizeForEncoding(OCFStringGetLength(path), 0x08000100);
    char *buffer = malloc_type_calloc(1, (size_t)maximum + 1, 0x5C5092C1);
    if (!OCFStringGetCString(path, buffer, maximum + 1, 0x08000100)) {
        free(buffer);
        return NULL;
    }
    OCFMutableStringRef redacted = OCFStringCreateMutable(kOCFAllocatorDefault, 0);
    char *remaining = buffer;
    for (char *component = strsep(&remaining, "/"); component != NULL; component = strsep(&remaining, "/")) {
        size_t length = strlen(component);
        if (component != buffer) {
            OCFStringAppend(redacted, OF_STR("/"));
        }
        if (length == 0) {
            continue;
        }
        if (length == 1) {
            OCFStringAppend(redacted, OF_STR("{1}"));
        } else if (length == 2) {
            OCFStringAppendFormat(redacted, NULL, OF_STR("%c{1}"), (long)(signed char)component[0]);
        } else {
            size_t index = 0;
            while (strcmp(component, allowedComponents[index]) != 0) {
                if (++index == 5) {
                    break;
                }
            }
            if (index == 5) {
                OCFStringAppendFormat(
                    redacted,
                    NULL,
                    OF_STR("%c{%zu}%c"),
                    (long)(signed char)component[0],
                    length - 2,
                    (long)(signed char)component[length - 1]
                );
            } else {
                OCFStringAppendFormat(redacted, NULL, OF_STR("%s"), component);
            }
        }
    }
    free(buffer);
    return OCFAutorelease(redacted);
}

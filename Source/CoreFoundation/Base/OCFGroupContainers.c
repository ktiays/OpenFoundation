//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Base/OCFGroupContainers.h"

#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Collections/OCFDictionary.h"
#include "CoreFoundation/Preferences/OCFXPCBridge.h"
#include "CoreFoundation/String/OCFString.h"

#include <os/log.h>
#include <stdlib.h>
#include <string.h>
#include <xpc/xpc.h>

// Inferred organization: _CFCreateContainerURLForSecurityApplicationIdentifierGroupIdentifierAndUser
// (0x1c0235d0c, with its .cold.1 0x1c030cd4c) and ____CFLookUpGroupPathForUser_block_invoke (0x1c0236078) lie
// after CFCopySearchPathForDirectoriesInDomains (0x1c0235b88, Base/OCFPlatform.c, another family's file) and
// before _CFCreateContainerURLForSecurityApplicationGroupIdentifier (0x1c023604c, outside the evidence) in
// the iOS Simulator __text section; following the batch's ownership rule, the cluster has its own translation
// unit here, named for it. The native source path and language are not in the evidence; it is compiled as
// C. Unless a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified (assembly read in full); iOS Device 24A446: unread. Reading: not yet written; the
// evidence is the assembly in Local/Evidence/cf-core/iphone-simulator-corpus-delta11.json.

// libsystem_containermanager's query interface (container_private.h, not in the SDK); supplied, never
// reimplemented. Signatures from the native register use. The default persona unique string is read from
// the GOT slot 0x222418e50, outside the extracted image; its binding to CONTAINER_PERSONA_CURRENT is inferred
// from the interface (confidence low).
typedef struct ContainerQuery *ContainerQueryRef;
typedef const struct ContainerObject *ContainerObjectRef;
extern const char *CONTAINER_PERSONA_CURRENT;
extern ContainerQueryRef container_query_create(void);
extern void container_query_set_class(ContainerQueryRef query, uint64_t containerClass);
extern void container_query_set_identifiers(ContainerQueryRef query, xpc_object_t identifiers);
extern void container_query_set_persona_unique_string(ContainerQueryRef query, const char *persona);
extern void container_query_operation_set_flags(ContainerQueryRef query, uint64_t flags);
extern void container_query_set_uid(ContainerQueryRef query, uid_t uid);
extern bool
container_query_iterate_results_sync(ContainerQueryRef query, bool (^block)(ContainerObjectRef object));
extern void *container_query_get_last_error(ContainerQueryRef query);
extern char *container_error_copy_unlocalized_description(void *error);
extern void container_query_free(ContainerQueryRef query);
extern const char *container_get_identifier(ContainerObjectRef object);
extern const char *container_get_path(ContainerObjectRef object);

// libdispatch's voucher_get_current_persona and libsystem_kernel's kpersona_info (not in the SDK); supplied.
// The native caller passes a 348-byte zeroed kpersona_info with version 2 and reads the persona type (offset
// 8) and the persona unique string (offset 88); the remaining fields are spelled as their bytes (unread).
// PersonaInfo and its field names are local descriptive spellings.
typedef struct {
    uint32_t version;
    uint32_t personaID;
    int personaType;
    uint8_t unread[76];
    char uniqueString[260];
} PersonaInfo;

_Static_assert(sizeof(PersonaInfo) == 348, "The native persona buffer is 348 bytes.");

extern int voucher_get_current_persona(void);
extern int kpersona_info(uint32_t personaID, PersonaInfo *info);

// Native _CFCreateContainerURLForSecurityApplicationIdentifierGroupIdentifierAndUser, 0x1c0235d0c, with
// .cold.1 (0x1c030cd4c, the error log) and the inlined _CFLookUpGroupPathForUser with its block
// ____CFLookUpGroupPathForUser_block_invoke (0x1c0236078, descriptor 0x2227a5c10, flags 0x40000000: the
// __block dictionary). External.
// The identifier is converted to a C string (its UTF-8 pointer, else in a zeroed 1,024-byte buffer with its
// fastest encoding; NULL when that fails or there is no identifier). A new XPC dictionary collects the
// results. The persona unique string is CONTAINER_PERSONA_CURRENT unless the current voucher persona (when
// not -1) is a type-2 persona with a nonempty unique string and kpersona_info succeeds. A container query of
// class 7, with the identifier as an XPC string (when present), that persona, operation flags 0x100000001
// and the user, maps each result's identifier to its path. When the iteration fails, the error description
// is logged (_CFOSLog, error level: "Error getting app group containers for %s: %s"), freed, and the
// results released and cleared. The query is freed; the results are converted to CF and released. The
// answer is a directory URL (POSIX style, system default allocator) for the group identifier's path, when
// present; the converted dictionary is released.
OCFURLRef _OCFCreateContainerURLForSecurityApplicationIdentifierGroupIdentifierAndUser(
    OCFStringRef identifier, OCFStringRef groupIdentifier, uid_t user
) {
    char buffer[1024];
    bzero(buffer, sizeof(buffer));
    const char *identifierString = NULL;
    if (identifier != NULL) {
        identifierString = OCFStringGetCStringPtr(identifier, 0x08000100);
        if (identifierString == NULL) {
            OCFStringEncoding encoding = OCFStringGetFastestEncoding(identifier);
            identifierString = OCFStringGetCString(identifier, buffer, 1024, encoding) ? buffer : NULL;
        }
    }
    __block xpc_object_t paths = xpc_dictionary_create(NULL, NULL, 0);
    const char *persona = CONTAINER_PERSONA_CURRENT;
    int currentPersona = voucher_get_current_persona();
    PersonaInfo info;
    memset(&info, 0, sizeof(info));
    info.version = 2;
    if (currentPersona != -1) {
        int status = kpersona_info((uint32_t)currentPersona, &info);
        if (info.uniqueString[0] != 0 && info.personaType == 2 && status == 0) {
            persona = info.uniqueString;
        }
    }
    ContainerQueryRef query = container_query_create();
    container_query_set_class(query, 7);
    if (identifierString != NULL) {
        xpc_object_t identifiers = xpc_string_create(identifierString);
        container_query_set_identifiers(query, identifiers);
        xpc_release(identifiers);
    }
    container_query_set_persona_unique_string(query, persona);
    container_query_operation_set_flags(query, 0x100000001);
    container_query_set_uid(query, user);
    bool succeeded = container_query_iterate_results_sync(query, ^bool(ContainerObjectRef object) {
      xpc_dictionary_set_string(paths, container_get_identifier(object), container_get_path(object));
      return true;
    });
    if (!succeeded) {
        char *description =
            container_error_copy_unlocalized_description(container_query_get_last_error(query));
        os_log_error(
            _CFOSLog(), "Error getting app group containers for %s: %s", identifierString, description
        );
        free(description);
        xpc_release(paths);
        paths = NULL;
    }
    container_query_free(query);
    xpc_object_t results = paths;
    if (results == NULL) {
        return NULL;
    }
    OCFDictionaryRef dictionary = _OCFXPCCreateCFObjectFromXPCObject(results);
    xpc_release(results);
    if (dictionary == NULL) {
        return NULL;
    }
    OCFStringRef path = OCFDictionaryGetValue(dictionary, groupIdentifier);
    OCFURLRef url = NULL;
    if (path != NULL) {
        url = OCFURLCreateWithFileSystemPath(kOCFAllocatorSystemDefault, path, 0, true);
    }
    OCFRelease(dictionary);
    return url;
}

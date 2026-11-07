//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// Apple System Log for the wasm port: messages go to stderr.

#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

typedef struct asl_object_s *asl_object_t;
typedef asl_object_t aslclient;
typedef asl_object_t aslmsg;

#define ASL_TYPE_MSG 0
#define ASL_TYPE_QUERY 1
#define ASL_LEVEL_EMERG 0
#define ASL_LEVEL_ALERT 1
#define ASL_LEVEL_CRIT 2
#define ASL_LEVEL_ERR 3
#define ASL_LEVEL_WARNING 4
#define ASL_LEVEL_NOTICE 5
#define ASL_LEVEL_INFO 6
#define ASL_LEVEL_DEBUG 7
#define ASL_KEY_MSG "Message"
#define ASL_KEY_LEVEL "Level"
#define ASL_KEY_FACILITY "Facility"
#define ASL_KEY_SENDER "Sender"
#define ASL_OPT_STDERR 0x00000001
#define ASL_OPT_NO_DELAY 0x00000002
#define ASL_OPT_NO_REMOTE 0x00000004

asl_object_t asl_open(const char *identifier, const char *facility, uint32_t options);
asl_object_t asl_new(uint32_t type);
int asl_set(asl_object_t object, const char *key, const char *value);
int asl_send(asl_object_t client, asl_object_t message);
void asl_free(asl_object_t object);
void asl_close(asl_object_t object);

__END_DECLS

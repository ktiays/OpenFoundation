//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// Quality-of-service classes for the wasm port: the values Darwin uses; a thread's class is always the
// main thread's.

#include <pthread.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

typedef unsigned int qos_class_t;

#define QOS_CLASS_USER_INTERACTIVE 0x21u
#define QOS_CLASS_USER_INITIATED 0x19u
#define QOS_CLASS_DEFAULT 0x15u
#define QOS_CLASS_UTILITY 0x11u
#define QOS_CLASS_BACKGROUND 0x09u
#define QOS_CLASS_UNSPECIFIED 0x00u
#define QOS_MIN_RELATIVE_PRIORITY (-15)

qos_class_t qos_class_self(void);
qos_class_t qos_class_main(void);
int pthread_attr_set_qos_class_np(pthread_attr_t *attributes, qos_class_t qosClass, int relativePriority);
int pthread_attr_get_qos_class_np(pthread_attr_t *attributes, qos_class_t *qosClass, int *relativePriority);
int pthread_set_qos_class_self_np(qos_class_t qosClass, int relativePriority);
int pthread_get_qos_class_np(pthread_t thread, qos_class_t *qosClass, int *relativePriority);

__END_DECLS

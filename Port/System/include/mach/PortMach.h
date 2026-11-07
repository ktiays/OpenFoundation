//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// The Mach interface of the wasm port: ports, port sets, send rights, mach_msg(), mk_timer and Mach time, all
// within one process (Port/System/Source/Mach.c). Types, constants and calling conventions are the Darwin
// ones the iOS SDK declares, so that the restored sources compile and behave as on Darwin for in-process
// messages. Messages carry bytes only: port rights inside message bodies are not transferred.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

typedef int kern_return_t;
typedef int integer_t;
typedef unsigned int natural_t;
typedef natural_t mach_port_name_t;
typedef mach_port_name_t mach_port_t;
typedef mach_port_t task_t;
typedef mach_port_t task_name_t;
typedef mach_port_t thread_t;
typedef mach_port_t thread_act_t;
typedef mach_port_t ipc_space_t;
typedef mach_port_t host_t;
typedef mach_port_t vm_map_t;
typedef mach_port_name_t *mach_port_name_array_t;
typedef natural_t mach_port_right_t;
typedef natural_t mach_port_type_t;
typedef natural_t mach_port_urefs_t;
typedef integer_t mach_port_delta_t;
typedef natural_t mach_port_seqno_t;
typedef natural_t mach_port_mscount_t;
typedef natural_t mach_port_msgcount_t;
typedef natural_t mach_port_rights_t;
typedef uint64_t mach_port_context_t;
typedef int mach_port_flavor_t;
typedef integer_t *mach_port_info_t;
typedef natural_t mach_msg_type_number_t;
typedef int mach_error_t;
typedef int vm_prot_t;
typedef uintptr_t vm_offset_t;
typedef uintptr_t vm_size_t;
typedef uintptr_t vm_address_t;
typedef uint64_t mach_vm_address_t;
typedef uint64_t mach_vm_size_t;
typedef integer_t cpu_type_t;
typedef integer_t cpu_subtype_t;
typedef natural_t task_flavor_t;
typedef integer_t *task_info_t;

typedef struct {
    unsigned int val[8];
} audit_token_t;

#define KERN_SUCCESS 0
#define KERN_INVALID_ADDRESS 1
#define KERN_PROTECTION_FAILURE 2
#define KERN_NO_SPACE 3
#define KERN_INVALID_ARGUMENT 4
#define KERN_FAILURE 5
#define KERN_RESOURCE_SHORTAGE 6
#define KERN_NOT_RECEIVER 7
#define KERN_NO_ACCESS 8
#define KERN_ALREADY_IN_SET 11
#define KERN_NOT_IN_SET 12
#define KERN_NAME_EXISTS 13
#define KERN_ABORTED 14
#define KERN_INVALID_NAME 15
#define KERN_INVALID_TASK 16
#define KERN_INVALID_RIGHT 17
#define KERN_INVALID_VALUE 18
#define KERN_UREFS_OVERFLOW 19
#define KERN_INVALID_CAPABILITY 20
#define KERN_NOT_SUPPORTED 46
#define KERN_OPERATION_TIMED_OUT 49

#define MACH_PORT_NULL ((mach_port_t)0)
#define MACH_PORT_DEAD ((mach_port_t)~0u)
#define MACH_PORT_VALID(name) (((name) != MACH_PORT_NULL) && ((name) != MACH_PORT_DEAD))

#define MACH_PORT_RIGHT_SEND ((mach_port_right_t)0)
#define MACH_PORT_RIGHT_RECEIVE ((mach_port_right_t)1)
#define MACH_PORT_RIGHT_SEND_ONCE ((mach_port_right_t)2)
#define MACH_PORT_RIGHT_PORT_SET ((mach_port_right_t)3)
#define MACH_PORT_RIGHT_DEAD_NAME ((mach_port_right_t)4)

#define MACH_PORT_TYPE(right)                                                                                \
    ((mach_port_type_t)(((mach_port_type_t)1) << ((right) + ((mach_port_right_t)16))))
#define MACH_PORT_TYPE_NONE ((mach_port_type_t)0)
#define MACH_PORT_TYPE_SEND MACH_PORT_TYPE(MACH_PORT_RIGHT_SEND)
#define MACH_PORT_TYPE_RECEIVE MACH_PORT_TYPE(MACH_PORT_RIGHT_RECEIVE)
#define MACH_PORT_TYPE_SEND_ONCE MACH_PORT_TYPE(MACH_PORT_RIGHT_SEND_ONCE)
#define MACH_PORT_TYPE_PORT_SET MACH_PORT_TYPE(MACH_PORT_RIGHT_PORT_SET)
#define MACH_PORT_TYPE_DEAD_NAME MACH_PORT_TYPE(MACH_PORT_RIGHT_DEAD_NAME)
#define MACH_PORT_TYPE_SEND_RECEIVE (MACH_PORT_TYPE_SEND | MACH_PORT_TYPE_RECEIVE)

typedef struct mach_port_limits {
    mach_port_msgcount_t mpl_qlimit;
} mach_port_limits_t;

#define MACH_PORT_QLIMIT_DEFAULT ((mach_port_msgcount_t)5)
#define MACH_PORT_QLIMIT_MAX ((mach_port_msgcount_t)1024)

typedef struct mach_port_options {
    uint32_t flags;
    mach_port_limits_t mpl;

    union {
        uint64_t reserved[2];
        mach_port_name_t work_interval_port;
        mach_port_name_t service_port_info;
    };
} mach_port_options_t;

typedef mach_port_options_t *mach_port_options_ptr_t;

#define MPO_CONTEXT_AS_GUARD 0x01
#define MPO_QLIMIT 0x02
#define MPO_TEMPOWNER 0x04
#define MPO_IMPORTANCE_RECEIVER 0x08
#define MPO_INSERT_SEND_RIGHT 0x10
#define MPO_STRICT 0x20
#define MPO_DENAP_RECEIVER 0x40
#define MPO_IMMOVABLE_RECEIVE 0x80

typedef unsigned int mach_msg_bits_t;
typedef natural_t mach_msg_size_t;
typedef integer_t mach_msg_id_t;
typedef natural_t mach_msg_timeout_t;
typedef integer_t mach_msg_option_t;
typedef kern_return_t mach_msg_return_t;
typedef unsigned int mach_msg_type_name_t;
typedef natural_t mach_msg_priority_t;

typedef struct {
    mach_msg_bits_t msgh_bits;
    mach_msg_size_t msgh_size;
    mach_port_t msgh_remote_port;
    mach_port_t msgh_local_port;
    mach_port_name_t msgh_voucher_port;
    mach_msg_id_t msgh_id;
} mach_msg_header_t;

typedef struct {
    mach_msg_size_t msgh_descriptor_count;
} mach_msg_body_t;

typedef unsigned int mach_msg_trailer_type_t;
typedef unsigned int mach_msg_trailer_size_t;

typedef struct {
    mach_msg_trailer_type_t msgh_trailer_type;
    mach_msg_trailer_size_t msgh_trailer_size;
} mach_msg_trailer_t;

typedef struct {
    mach_msg_header_t header;
    mach_msg_body_t body;
} mach_msg_base_t;

#define MACH_MSG_TIMEOUT_NONE ((mach_msg_timeout_t)0)

#define MACH_MSGH_BITS_ZERO 0x00000000
#define MACH_MSGH_BITS_REMOTE_MASK 0x0000001f
#define MACH_MSGH_BITS_LOCAL_MASK 0x00001f00
#define MACH_MSGH_BITS_VOUCHER_MASK 0x001f0000
#define MACH_MSGH_BITS_PORTS_MASK                                                                            \
    (MACH_MSGH_BITS_REMOTE_MASK | MACH_MSGH_BITS_LOCAL_MASK | MACH_MSGH_BITS_VOUCHER_MASK)
#define MACH_MSGH_BITS_COMPLEX 0x80000000U
#define MACH_MSGH_BITS(remote, local) ((remote) | ((local) << 8))
#define MACH_MSGH_BITS_SET(remote, local, voucher, other)                                                    \
    (MACH_MSGH_BITS(remote, local) | ((voucher) << 16) | ((other) & ~MACH_MSGH_BITS_PORTS_MASK))
#define MACH_MSGH_BITS_REMOTE(bits) ((bits) & MACH_MSGH_BITS_REMOTE_MASK)
#define MACH_MSGH_BITS_LOCAL(bits) (((bits) & MACH_MSGH_BITS_LOCAL_MASK) >> 8)
#define MACH_MSGH_BITS_VOUCHER(bits) (((bits) & MACH_MSGH_BITS_VOUCHER_MASK) >> 16)
#define MACH_MSGH_BITS_OTHER(bits) ((bits) & ~MACH_MSGH_BITS_PORTS_MASK)

#define MACH_MSG_TYPE_MOVE_RECEIVE 16
#define MACH_MSG_TYPE_MOVE_SEND 17
#define MACH_MSG_TYPE_MOVE_SEND_ONCE 18
#define MACH_MSG_TYPE_COPY_SEND 19
#define MACH_MSG_TYPE_MAKE_SEND 20
#define MACH_MSG_TYPE_MAKE_SEND_ONCE 21
#define MACH_MSG_TYPE_COPY_RECEIVE 22
#define MACH_MSG_TYPE_PORT_NAME 15
#define MACH_MSG_TYPE_PORT_SEND MACH_MSG_TYPE_MOVE_SEND
#define MACH_MSG_TYPE_PORT_SEND_ONCE MACH_MSG_TYPE_MOVE_SEND_ONCE

#define MACH_MSG_OPTION_NONE 0x00000000
#define MACH_SEND_MSG 0x00000001
#define MACH_RCV_MSG 0x00000002
#define MACH_RCV_LARGE 0x00000004
#define MACH_RCV_LARGE_IDENTITY 0x00000008
#define MACH_SEND_TIMEOUT 0x00000010
#define MACH_SEND_OVERRIDE 0x00000020
#define MACH_SEND_INTERRUPT 0x00000040
#define MACH_SEND_NOTIFY 0x00000080
#define MACH_RCV_TIMEOUT 0x00000100
#define MACH_RCV_INTERRUPT 0x00000400
#define MACH_RCV_VOUCHER 0x00000800
#define MACH_RCV_TRAILER_TYPE(x) (((x) & 0xf) << 28)
#define MACH_RCV_TRAILER_ELEMENTS(x) (((x) & 0xf) << 24)
#define MACH_MSG_TRAILER_FORMAT_0 0
#define MACH_RCV_TRAILER_NULL 0
#define MACH_RCV_TRAILER_SEQNO 1
#define MACH_RCV_TRAILER_SENDER 2
#define MACH_RCV_TRAILER_AUDIT 3
#define MACH_RCV_TRAILER_CTX 4
#define MACH_RCV_TRAILER_AV 7
#define MACH_RCV_TRAILER_LABELS 8
#define MACH_MSG_TRAILER_MINIMUM_SIZE sizeof(mach_msg_trailer_t)
#define MAX_TRAILER_SIZE ((mach_msg_size_t)68)

#define MACH_MSG_SUCCESS 0x00000000
#define MACH_MSG_MASK 0x00003e00
#define MACH_MSG_IPC_SPACE 0x00002000
#define MACH_MSG_VM_SPACE 0x00001000
#define MACH_MSG_IPC_KERNEL 0x00000800
#define MACH_MSG_VM_KERNEL 0x00000400
#define MACH_SEND_IN_PROGRESS 0x10000001
#define MACH_SEND_INVALID_DATA 0x10000002
#define MACH_SEND_INVALID_DEST 0x10000003
#define MACH_SEND_TIMED_OUT 0x10000004
#define MACH_SEND_INVALID_VOUCHER 0x10000005
#define MACH_SEND_INTERRUPTED 0x10000007
#define MACH_SEND_MSG_TOO_SMALL 0x10000008
#define MACH_SEND_INVALID_REPLY 0x10000009
#define MACH_SEND_INVALID_RIGHT 0x1000000a
#define MACH_SEND_INVALID_NOTIFY 0x1000000b
#define MACH_SEND_INVALID_MEMORY 0x1000000c
#define MACH_SEND_NO_BUFFER 0x1000000d
#define MACH_SEND_TOO_LARGE 0x1000000e
#define MACH_SEND_INVALID_TYPE 0x1000000f
#define MACH_SEND_INVALID_HEADER 0x10000010
#define MACH_RCV_IN_PROGRESS 0x10004001
#define MACH_RCV_INVALID_NAME 0x10004002
#define MACH_RCV_TIMED_OUT 0x10004003
#define MACH_RCV_TOO_LARGE 0x10004004
#define MACH_RCV_INTERRUPTED 0x10004005
#define MACH_RCV_PORT_CHANGED 0x10004006
#define MACH_RCV_INVALID_NOTIFY 0x10004007
#define MACH_RCV_INVALID_DATA 0x10004008
#define MACH_RCV_PORT_DIED 0x10004009
#define MACH_RCV_IN_SET 0x1000400a
#define MACH_RCV_HEADER_ERROR 0x1000400b
#define MACH_RCV_BODY_ERROR 0x1000400c
#define MACH_RCV_INVALID_TYPE 0x1000400d
#define MACH_RCV_SCATTER_SMALL 0x1000400e
#define MACH_RCV_INVALID_TRAILER 0x1000400f

extern mach_port_t mach_task_self_;
#define mach_task_self() mach_task_self_
#define current_task() mach_task_self()

mach_port_t mach_thread_self(void);
mach_port_t mach_host_self(void);

kern_return_t mach_port_allocate(ipc_space_t task, mach_port_right_t right, mach_port_name_t *name);
kern_return_t mach_port_construct(
    ipc_space_t task, mach_port_options_ptr_t options, mach_port_context_t context, mach_port_name_t *name
);
kern_return_t mach_port_destruct(
    ipc_space_t task, mach_port_name_t name, mach_port_delta_t srdelta, mach_port_context_t guard
);
kern_return_t mach_port_deallocate(ipc_space_t task, mach_port_name_t name);
kern_return_t mach_port_destroy(ipc_space_t task, mach_port_name_t name);
kern_return_t
mach_port_mod_refs(ipc_space_t task, mach_port_name_t name, mach_port_right_t right, mach_port_delta_t delta);
kern_return_t
mach_port_get_refs(ipc_space_t task, mach_port_name_t name, mach_port_right_t right, mach_port_urefs_t *refs);
kern_return_t mach_port_insert_right(
    ipc_space_t task, mach_port_name_t name, mach_port_t poly, mach_msg_type_name_t polyPoly
);
kern_return_t mach_port_insert_member(ipc_space_t task, mach_port_name_t name, mach_port_name_t pset);
kern_return_t mach_port_extract_member(ipc_space_t task, mach_port_name_t name, mach_port_name_t pset);
kern_return_t mach_port_move_member(ipc_space_t task, mach_port_name_t member, mach_port_name_t after);
kern_return_t mach_port_type(ipc_space_t task, mach_port_name_t name, mach_port_type_t *ptype);
kern_return_t mach_port_get_context(ipc_space_t task, mach_port_name_t name, mach_port_context_t *context);
kern_return_t mach_port_set_context(ipc_space_t task, mach_port_name_t name, mach_port_context_t context);
kern_return_t mach_port_request_notification(
    ipc_space_t task,
    mach_port_name_t name,
    integer_t msgid,
    mach_port_mscount_t sync,
    mach_port_t notify,
    mach_msg_type_name_t notifyPoly,
    mach_port_t *previous
);

mach_msg_return_t mach_msg(
    mach_msg_header_t *msg,
    mach_msg_option_t option,
    mach_msg_size_t sendSize,
    mach_msg_size_t receiveLimit,
    mach_port_name_t receiveName,
    mach_msg_timeout_t timeout,
    mach_port_name_t notify
);
void mach_msg_destroy(mach_msg_header_t *msg);

const char *mach_error_string(mach_error_t errorValue);
void mach_error(const char *string, mach_error_t errorValue);

typedef struct mach_timebase_info {
    uint32_t numer;
    uint32_t denom;
} mach_timebase_info_data_t, *mach_timebase_info_t;

kern_return_t mach_timebase_info(mach_timebase_info_t info);
uint64_t mach_absolute_time(void);
uint64_t mach_approximate_time(void);
uint64_t mach_continuous_time(void);
uint64_t mach_continuous_approximate_time(void);

mach_port_name_t mk_timer_create(void);
kern_return_t mk_timer_destroy(mach_port_name_t name);
kern_return_t mk_timer_arm(mach_port_name_t name, uint64_t expireTime);
kern_return_t
mk_timer_arm_leeway(mach_port_name_t name, uint64_t flags, uint64_t expireTime, uint64_t leeway);
kern_return_t mk_timer_cancel(mach_port_name_t name, uint64_t *resultTime);

typedef struct mk_timer_expire_msg {
    mach_msg_header_t header;
    uint64_t unused[3];
} mk_timer_expire_msg_t;

extern vm_size_t vm_page_size;
extern vm_size_t vm_page_mask;
extern int vm_page_shift;
#define VM_FLAGS_FIXED 0x0000
#define VM_FLAGS_ANYWHERE 0x0001
#define VM_PROT_NONE ((vm_prot_t)0x00)
#define VM_PROT_READ ((vm_prot_t)0x01)
#define VM_PROT_WRITE ((vm_prot_t)0x02)
#define VM_PROT_EXECUTE ((vm_prot_t)0x04)
#define VM_PROT_DEFAULT (VM_PROT_READ | VM_PROT_WRITE)

kern_return_t vm_allocate(vm_map_t task, vm_address_t *address, vm_size_t size, int flags);
kern_return_t vm_deallocate(vm_map_t task, vm_address_t address, vm_size_t size);
kern_return_t vm_copy(vm_map_t task, vm_address_t source, vm_size_t size, vm_address_t destination);
kern_return_t mach_vm_allocate(vm_map_t task, mach_vm_address_t *address, mach_vm_size_t size, int flags);
kern_return_t mach_vm_deallocate(vm_map_t task, mach_vm_address_t address, mach_vm_size_t size);

#define TASK_BASIC_INFO 5
#define TASK_AUDIT_TOKEN 15
#define TASK_AUDIT_TOKEN_COUNT (sizeof(audit_token_t) / sizeof(natural_t))
#define TASK_VM_INFO 22
kern_return_t
task_info(task_name_t task, task_flavor_t flavor, task_info_t info, mach_msg_type_number_t *count);

// Mach vouchers travel with no message on the port: adopting one changes nothing.
typedef struct voucher_mach_msg_state_s *voucher_mach_msg_state_t;
#define VOUCHER_MACH_MSG_STATE_UNCHANGED ((voucher_mach_msg_state_t)~0ul)
voucher_mach_msg_state_t voucher_mach_msg_adopt(mach_msg_header_t *msg);
void voucher_mach_msg_revert(voucher_mach_msg_state_t state);

__END_DECLS

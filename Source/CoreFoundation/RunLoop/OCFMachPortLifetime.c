//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/RunLoop/OCFMachPortLifetime.h"

#include "CoreFoundation/Collections/OCFSet.h"
#include "CoreFoundation/String/OCFString.h"

#include <dispatch/dispatch.h>
#include <mach/mach.h>
#include <os/lock.h>
#include <os/log.h>
#include <stdlib.h>
#include <string.h>

// Inferred organization: __CFMachPortDeallocationFailure (0x1c019d9b8) through _cfmp_hash (0x1c019e370), the
// mach port lifetime bookkeeping (_cfmp_cleanup, _cfmp_record_deallocation and their record set), lie in one
// cluster of the iOS Simulator __text section between the path utilities and __NSSingleEntryDictionaryI; they
// form their own translation unit here. The native source path is not in the evidence. Restored:
// _CFMachPortDeallocationFailure, _cfmp_cleanup, _cfmp_record_deallocation (with its .cold.1, 0x1c03082b4,
// and _OUTLINED_FUNCTION_0_10, 0x1c019e390, the dispatch_once of the record set, folded into it),
// _cfmp_log_failure (with its .cold.1, 0x1c03082d4), the record set's once block and its callbacks. Not
// restored (not in the evidence): _cfmp_record_intent_to_invalidate (0x1c019dbb0), _cfmp_source_invalidated
// (0x1c019dc8c), _cfmp_source_record_deadness (0x1c019dec8), _cfmp_record_nsmachport_is_interested
// (0x1c019dfa0) and _cfmp_record_nsmachport_deallocation (0x1c019e060), with their .cold entries. Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full; pseudocode used as a guide only); iOS Device 24A446: unread. Reading: not
// yet written; evidence Local/Evidence/cf-core/iphone-simulator-corpus-delta4.json (_cfmp_cleanup) and
// iphone-simulator-corpus-delta5.json (the rest).

extern struct mach_header __dso_handle;

// The os_log format literals, placed in __TEXT,__oslogstring like the compiler-generated native ones.
#define OSLOG_STRING __attribute__((section("__TEXT,__oslogstring,cstring_literals")))
OSLOG_STRING static const char recordFailureFormat[] =
    "*** %{public}s, break on '_CFMachPortDeallocationFailure' to debug: {p:%{private}d c:%d is:%d "
    "<i:%d,d:%d> "
    "s:%d,r:%d nsi:%d,nss:%d,nsr:%d - ic:%d,ip:%d}";
OSLOG_STRING static const char nullRecordFailureFormat[] =
    "*** %{public}s, break on '_CFMachPortDeallocationFailure' to debug: {null - ic:%d,ip:%d}";

static const void *_cfmp_deallocation_record_retain(OCFAllocatorRef allocator, const void *value);
static void _cfmp_deallocation_record_release(OCFAllocatorRef allocator, const void *value);
static OCFStringRef _cfmp_copy_description(const void *value);
static Boolean _cfmp_equal(const void *value1, const void *value2);
static OCFHashCode _cfmp_hash(const void *value);

// Native _cfmp_records_lock, 0x22e6d2840 (__DATA,__bss). Non-external; guards the record set.
static os_unfair_lock _cfmp_records_lock;

// Native _CFMachPortDeallocationFailure, 0x1c019d9b8. Hidden (was a private external). Returns at once; it
// exists as a breakpoint target. The empty volatile assembly emits no instruction; with noinline it keeps the
// call that _cfmp_log_failure makes, as natively.
__attribute__((noinline)) void _CFMachPortDeallocationFailure(void) {
    __asm__ __volatile__("");
}

// Native _cfmp_cleanup, 0x1c019d9bc. Hidden (was a private external). Each right is dropped from
// mach_task_self_ (read at each call) in this order: flag bit 4, a receive right reference
// (mach_port_mod_refs, right 1, delta -1); bit 3, a send right (mach_port_deallocate); then, with bit 5, bit
// 7 (receive reference) and bit 6 (send right; a tail call). The results are ignored.
void _cfmp_cleanup(MachPortDeallocationRecord record) {
    uint8_t flags = record.flags;
    if ((flags & 0x10) != 0) {
        mach_port_mod_refs(mach_task_self_, record.port, MACH_PORT_RIGHT_RECEIVE, -1);
    }
    if ((flags & 0x08) != 0) {
        mach_port_deallocate(mach_task_self_, record.port);
    }
    if ((flags & 0x20) == 0) {
        return;
    }
    if ((flags & 0x80) != 0) {
        mach_port_mod_refs(mach_task_self_, record.port, MACH_PORT_RIGHT_RECEIVE, -1);
    }
    if ((flags & 0x40) != 0) {
        mach_port_deallocate(mach_task_self_, record.port);
    }
}

// Native _cfmp_records.oGuard and _cfmp_records.oRecords, 0x22e6d2850 and 0x22e6d2848 (__DATA,__bss), the
// statics of the inlined _cfmp_records (no native symbol; the name is the statics'), with the once block
// ____cfmp_records_block_invoke, 0x1c019e1d4, which copies the set callbacks at 0x22279de58
// (__DATA_CONST,__const, after ___block_literal_global: version 0, the record retain, release, description,
// equality and hash; the table's words, read with macho.py) and creates the set with the default allocator
// (NULL) and capacity 16.
static OCFMutableSetRef _cfmp_records(void) {
    static dispatch_once_t oGuard;
    static OCFMutableSetRef oRecords;
    dispatch_once(&oGuard, ^{
      OCFSetCallBacks callBacks = {
          0,
          _cfmp_deallocation_record_retain,
          _cfmp_deallocation_record_release,
          _cfmp_copy_description,
          _cfmp_equal,
          _cfmp_hash,
      };
      oRecords = OCFSetCreateMutable(NULL, 16, &callBacks);
    });
    return oRecords;
}

// Native _cfmp_record_deallocation, 0x1c019da60, with _cfmp_record_deallocation.cold.1 (0x1c03082b4, the
// branch to _OUTLINED_FUNCTION_0_10, the record set's dispatch_once). Hidden (was a private external). Port
// 0 does nothing. The record set is read before its lock is taken; the lookup key is the port and type with
// zero flags. A recorded port without an intent to invalidate (flag bit 2 clear) keeps its record: bits 2-4
// are replaced by the send (bit 3) and receive (bit 4) flags and bit 1 (deallocated) is set. A port with an
// intent to invalidate leaves the set with its port, type and 16-bit word, its flags keeping bits 0-2 and
// 5-7 and taking the send and receive flags in bits 3 and 4. A port without a record gets the passed port,
// type 0 and only the send and receive flags. In the last two cases, after the unlock, the rights are dropped
// with _cfmp_cleanup.
void _cfmp_record_deallocation(int32_t type, mach_port_t port, bool bit2, bool bit1) {
    if (port == 0) {
        return;
    }
    OCFMutableSetRef records = _cfmp_records();
    os_unfair_lock_lock(&_cfmp_records_lock);
    MachPortDeallocationRecord probe;
    memset(&probe, 0, sizeof(probe));
    probe.port = port;
    probe.type = (uint8_t)type;
    MachPortDeallocationRecord *found = (MachPortDeallocationRecord *)OCFSetGetValue(records, &probe);
    MachPortDeallocationRecord record;
    if (found != NULL) {
        uint8_t flags = found->flags;
        if ((flags & 0x04) == 0) {
            found->flags = (uint8_t)((flags & 0xE3) | ((bit2 & 1) << 3) | ((bit1 & 1) << 4) | 0x02);
            os_unfair_lock_unlock(&_cfmp_records_lock);
            return;
        }
        record.port = found->port;
        record.type = found->type;
        record.flags = (uint8_t)((flags & 0xE7) | ((bit2 & 1) << 3) | ((bit1 & 1) << 4));
        record.extra = found->extra;
        OCFSetRemoveValue(records, found);
    } else {
        record.port = port;
        record.type = 0;
        record.flags = (uint8_t)(((bit2 & 1) << 3) | ((bit1 & 1) << 4));
        record.extra = 0;
    }
    os_unfair_lock_unlock(&_cfmp_records_lock);
    _cfmp_cleanup(record);
}

// Native _cfmp_log_failure, 0x1c019dd8c, with _cfmp_log_failure.cold.1 (0x1c03082d4). Non-external. Whether
// errors are enabled for the default log (the GOT slot 0x22241b4d8, taken to be OS_LOG_DEFAULT as in
// Base/OCFUtilities.c; confidence: medium) is asked first. When enabled, a record is logged with its port
// (private), type and the eight flag bits (in the order 0, 2, 1, 3, 4, 5, 6, 7), then the two integers
// (84-byte buffer, 13 arguments, summary byte 3); without a record (.cold.1), only the two integers, the
// first as its low byte (24-byte buffer, 3 arguments, summary byte 2). _CFMachPortDeallocationFailure is
// called in every case.
static void
_cfmp_log_failure(const char *message, const MachPortDeallocationRecord *record, uint8_t ic, int32_t ip) {
    bool enabled = os_log_type_enabled(OS_LOG_DEFAULT, OS_LOG_TYPE_ERROR);
    if (record != NULL) {
        if (enabled) {
            uint8_t flags = record->flags;
            int32_t values[12] = {
                (int32_t)record->port,
                record->type,
                flags & 1,
                (flags >> 2) & 1,
                (flags >> 1) & 1,
                (flags >> 3) & 1,
                (flags >> 4) & 1,
                (flags >> 5) & 1,
                (flags >> 6) & 1,
                flags >> 7,
                ic,
                ip,
            };
            uint8_t buffer[0x54];
            buffer[0] = 0x03;
            buffer[1] = 0x0D;
            buffer[2] = 0x22;
            buffer[3] = 0x08;
            memcpy(&buffer[4], &message, 8);
            for (int idx = 0; idx < 12; idx++) {
                buffer[12 + idx * 6] = idx == 0 ? 0x01 : 0x00;
                buffer[13 + idx * 6] = 0x04;
                memcpy(&buffer[14 + idx * 6], &values[idx], 4);
            }
            _os_log_error_impl(
                &__dso_handle, OS_LOG_DEFAULT, OS_LOG_TYPE_ERROR, recordFailureFormat, buffer, sizeof(buffer)
            );
        }
    } else if (enabled) {
        int32_t lowByte = ic;
        uint8_t buffer[0x18];
        buffer[0] = 0x02;
        buffer[1] = 0x03;
        buffer[2] = 0x22;
        buffer[3] = 0x08;
        memcpy(&buffer[4], &message, 8);
        buffer[12] = 0x00;
        buffer[13] = 0x04;
        memcpy(&buffer[14], &lowByte, 4);
        buffer[18] = 0x00;
        buffer[19] = 0x04;
        memcpy(&buffer[20], &ip, 4);
        _os_log_error_impl(
            &__dso_handle, OS_LOG_DEFAULT, OS_LOG_TYPE_ERROR, nullRecordFailureFormat, buffer, sizeof(buffer)
        );
    }
    _CFMachPortDeallocationFailure();
}

// Native _cfmp_deallocation_record_retain, 0x1c019e248, with _cfmp_deallocation_record_retain.cold.1
// (0x1c0308348). Non-external; the set's retain. A record already in a set (flag bit 0) crashes with "refcnt
// overflow"; otherwise bit 0 is set and the record itself is the retained value.
static const void *_cfmp_deallocation_record_retain(OCFAllocatorRef allocator, const void *value) {
    (void)allocator;
    MachPortDeallocationRecord *record = (MachPortDeallocationRecord *)value;
    uint8_t flags = record->flags;
    if ((flags & 0x01) != 0) {
        crashWithMessage("refcnt overflow");
    }
    record->flags = flags | 0x01;
    return record;
}

// Native _cfmp_deallocation_record_release, 0x1c019e26c. Non-external; the set's release. A record not
// marked as in a set (flag bit 0 clear) is logged with "Freeing a record not in the set", its type and its
// port; the record is freed (a tail call).
static void _cfmp_deallocation_record_release(OCFAllocatorRef allocator, const void *value) {
    (void)allocator;
    const MachPortDeallocationRecord *record = value;
    if ((record->flags & 0x01) == 0) {
        _cfmp_log_failure("Freeing a record not in the set", record, record->type, (int32_t)record->port);
    }
    free((void *)record);
}

// Native _cfmp_copy_description, 0x1c019e2ac. Non-external; the set's description. NULL gives the constant
// "{null}" (0x2227aeac0, not retained); a record gives "{p:%d c:%d is:%d <i:%d,d:%d> s:%d,r:%d
// nsi:%d,nss:%d,nsr:%d}" (0x2227aeae0) with the port, the type and the flag bits 0, 2, 1, 3, 4, 5, 6 and 7,
// in the default allocator.
static OCFStringRef _cfmp_copy_description(const void *value) {
    const MachPortDeallocationRecord *record = value;
    if (record == NULL) {
        return OF_STR("{null}");
    }
    uint8_t flags = record->flags;
    return OCFStringCreateWithFormat(
        NULL,
        NULL,
        OF_STR("{p:%d c:%d is:%d <i:%d,d:%d> s:%d,r:%d nsi:%d,nss:%d,nsr:%d}"),
        record->port,
        record->type,
        flags & 1,
        (flags >> 2) & 1,
        (flags >> 1) & 1,
        (flags >> 3) & 1,
        (flags >> 4) & 1,
        (flags >> 5) & 1,
        (flags >> 6) & 1,
        flags >> 7
    );
}

// Native _cfmp_equal, 0x1c019e328. Non-external; the set's equality. The same pointer is equal; otherwise
// both records must exist and have the same port and type.
static Boolean _cfmp_equal(const void *value1, const void *value2) {
    if (value1 == value2) {
        return true;
    }
    if (value1 == NULL || value2 == NULL) {
        return false;
    }
    const MachPortDeallocationRecord *record1 = value1;
    const MachPortDeallocationRecord *record2 = value2;
    return record1->port == record2->port && record1->type == record2->type;
}

// Native _cfmp_hash, 0x1c019e370. Non-external; the set's hash. NULL hashes to 0; a record to the 32-bit
// (port << 8 | type) times 0x9E3779B1, an unsigned 32 x 32 -> 64-bit product.
static OCFHashCode _cfmp_hash(const void *value) {
    const MachPortDeallocationRecord *record = value;
    if (record == NULL) {
        return 0;
    }
    uint32_t key = (record->port << 8) | record->type;
    return (OCFHashCode)((uint64_t)key * 0x9E3779B1u);
}

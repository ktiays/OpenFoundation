//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Base/OCFBase.h>

#include "CoreFoundation/Runtime/OCFRuntime.h"

#include <mach/mach.h>
#include <stdbool.h>
#include <stdint.h>

// A mach port deallocation record of the iOS Simulator image (24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC), passed by value in one register and kept, as an 8-byte heap block,
// in the unit's record set: the port name (bits 0-31), the record type (bits 32-39; 0 for CFMachPort), the
// flag byte (bits 40-47) and a 16-bit word (bits 48-63). Of the flag byte, bit 0 marks a record in the set
// ("is"), bit 1 a recorded deallocation ("d"), bit 2 a recorded intent to invalidate ("i"), bits 3 and 4 a
// send right and a receive right reference to drop ("s", "r"), bit 5 a second pair that applies ("nsi") and
// bits 6 and 7 its send right and receive right reference ("nss", "nsr"); the letters are those of
// _cfmp_copy_description and _cfmp_log_failure. Confidence: verified for the widths and the bits named (their
// meaning by use and by the description letters). The type and field names are local descriptive spellings.
typedef struct {
    mach_port_t port;
    uint8_t type;
    uint8_t flags;
    uint16_t extra;
} MachPortDeallocationRecord;

_Static_assert(sizeof(MachPortDeallocationRecord) == 8, "A deallocation record is one register.");

// Native _CFMachPortDeallocationFailure, 0x1c019d9b8. Hidden (was a private external); the breakpoint target
// _cfmp_log_failure names.
OF_EXTERN void _CFMachPortDeallocationFailure(void);

// Native _cfmp_cleanup, 0x1c019d9bc. Hidden (was a private external).
OF_EXTERN void _cfmp_cleanup(MachPortDeallocationRecord record);

// Native _cfmp_record_deallocation, 0x1c019da60. Hidden (was a private external): the record type (0 for
// CFMachPort), the port name and the send and receive flags (flag bits 3 and 4; bits 2 and 1 of the port's
// info word at __CFMachPortDeallocate's call).
OF_EXTERN void _cfmp_record_deallocation(int32_t type, mach_port_t port, bool bit2, bool bit1);

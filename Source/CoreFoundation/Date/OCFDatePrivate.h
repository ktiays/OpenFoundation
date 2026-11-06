//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/Date/OCFDate.h>

#include <stdint.h>

// Private interfaces of the CFDate unit (Date/OCFDate.c) that the installed Date/OCFDate.h does not carry.

#ifdef __OBJC__
@class __ONSTaggedDate;
#endif

// The TSR conversions (all were private externals): to and from time-stamp-register ticks with the rates that
// Date/OCFDate.h declares. __CFTSRToDispatchTime answers a dispatch_time_t (uint64_t), spelled uint64_t here
// so that this header needs no libdispatch include.
OF_EXTERN uint64_t __CFTimeIntervalToTSR(OCFTimeInterval ti);
OF_EXTERN OCFTimeInterval __CFTSRToTimeInterval(uint64_t tsr);
OF_EXTERN OCFTimeInterval __CFTimeIntervalUntilTSR(uint64_t tsr);
OF_EXTERN uint64_t __CFTSRToNanoseconds(uint64_t tsr);
OF_EXTERN uint64_t __CFTSRToDispatchTime(uint64_t tsr);

// Non-external natively; hidden here because their users are restored elsewhere: the tagged-date switch, the
// global block of +[__NSPlaceholderDate initialize] (a local descriptive name) and the system clock
// notification callback (its parameters are unread).
OF_EXTERN uint8_t __NSDateTaggedPointersDisabled;
#ifdef __BLOCKS__
typedef void (^PlaceholderDateInitializeBlock)(void);
OF_EXTERN PlaceholderDateInitializeBlock const placeholderDateInitializeBlock;
#endif
OF_EXTERN void __systemClockChangedNotification(void);

// Native NSSystemClockDidChangeNotification, 0x2227a82f0. External.
OF_EXPORT const OCFStringRef ONSSystemClockDidChangeNotification;

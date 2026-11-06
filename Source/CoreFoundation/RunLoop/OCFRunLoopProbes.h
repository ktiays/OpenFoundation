//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

// The DTrace provider "CFRunLoop" of the iOS Simulator image (24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC): its __TEXT,__dof_CFRunLoop section (2,404 bytes) declares fourteen
// probes with their functions, probe names, native argument types and offsets, and attributes 1_1_0 (private,
// private, unknown) for the provider, module, function, name and arguments. Each probe site is a NOP and each
// is-enabled site a MOV X0, #0 in the native code, the forms ld64 gives to the probe and is-enabled
// references below. Confidence: verified for the provider, probe names, argument types, attributes and the
// sites; medium for the spelling of the stability and typedef references, which follow the dtrace -h header
// form and leave no trace in the image besides the DOF they produce. The probes this unit fires are declared.
#define OCF_RUNLOOP_PROBE_STABILITY "___dtrace_stability$CFRunLoop$v1$1_1_0_1_1_0_1_1_0_1_1_0_1_1_0"
#define OCF_RUNLOOP_PROBE_TYPEDEFS "___dtrace_typedefs$CFRunLoop$v2"

#define OCF_RUNLOOP_PROBE(call)                                                                              \
    do {                                                                                                     \
        __asm__ volatile(".reference " OCF_RUNLOOP_PROBE_TYPEDEFS);                                          \
        call;                                                                                                \
        __asm__ volatile(".reference " OCF_RUNLOOP_PROBE_STABILITY);                                         \
    } while (0)

#define OCF_RUNLOOP_PROBE_ENABLED(call)                                                                      \
    ({                                                                                                       \
        int enabled = call;                                                                                  \
        __asm__ volatile("");                                                                                \
        enabled;                                                                                             \
    })

// next_timer_armed (unsigned long), in __CFArmNextTimerInMode.
extern void __dtrace_probe$CFRunLoop$next_timer_armed$v1$756e7369676e6564206c6f6e67(unsigned long);
extern int __dtrace_isenabled$CFRunLoop$next_timer_armed$v1(void);

#define CFRUNLOOP_NEXT_TIMER_ARMED(arg0)                                                                     \
    OCF_RUNLOOP_PROBE(__dtrace_probe$CFRunLoop$next_timer_armed$v1$756e7369676e6564206c6f6e67(arg0))
#define CFRUNLOOP_NEXT_TIMER_ARMED_ENABLED()                                                                 \
    OCF_RUNLOOP_PROBE_ENABLED(__dtrace_isenabled$CFRunLoop$next_timer_armed$v1())

// poll, sleep and wakeup (unsigned long), in __CFRunLoopServiceMachPort.
extern void __dtrace_probe$CFRunLoop$poll$v1(void);
extern void __dtrace_probe$CFRunLoop$sleep$v1(void);
extern void __dtrace_probe$CFRunLoop$wakeup$v1$756e7369676e6564206c6f6e67(unsigned long);

#define CFRUNLOOP_POLL() OCF_RUNLOOP_PROBE(__dtrace_probe$CFRunLoop$poll$v1())
#define CFRUNLOOP_SLEEP() OCF_RUNLOOP_PROBE(__dtrace_probe$CFRunLoop$sleep$v1())
#define CFRUNLOOP_WAKEUP(arg0)                                                                               \
    OCF_RUNLOOP_PROBE(__dtrace_probe$CFRunLoop$wakeup$v1$756e7369676e6564206c6f6e67(arg0))

// source_fired (void *, char *, char *), in __CFRunLoopDoSource1.
extern void
__dtrace_probe$CFRunLoop$source_fired$v1$766f6964202a$63686172202a$63686172202a(void *, char *, char *);
extern int __dtrace_isenabled$CFRunLoop$source_fired$v1(void);

#define CFRUNLOOP_SOURCE_FIRED(arg0, arg1, arg2)                                                             \
    OCF_RUNLOOP_PROBE(                                                                                       \
        __dtrace_probe$CFRunLoop$source_fired$v1$766f6964202a$63686172202a$63686172202a(arg0, arg1, arg2)    \
    )
#define CFRUNLOOP_SOURCE_FIRED_ENABLED()                                                                     \
    OCF_RUNLOOP_PROBE_ENABLED(__dtrace_isenabled$CFRunLoop$source_fired$v1())

// timer_fired (void *, unsigned long, char *, char *, void *), in __CFRunLoopDoTimer.
extern void
__dtrace_probe$CFRunLoop$timer_fired$v1$766f6964202a$756e7369676e6564206c6f6e67$63686172202a$63686172202a$766f6964202a(
    void *, unsigned long, char *, char *, void *
);
extern int __dtrace_isenabled$CFRunLoop$timer_fired$v1(void);

#define CFRUNLOOP_TIMER_FIRED(arg0, arg1, arg2, arg3, arg4)                                                                     \
    OCF_RUNLOOP_PROBE(                                                                                                          \
        __dtrace_probe$CFRunLoop$timer_fired$v1$766f6964202a$756e7369676e6564206c6f6e67$63686172202a$63686172202a$766f6964202a( \
            arg0, arg1, arg2, arg3, arg4                                                                                        \
        )                                                                                                                       \
    )
#define CFRUNLOOP_TIMER_FIRED_ENABLED()                                                                      \
    OCF_RUNLOOP_PROBE_ENABLED(__dtrace_isenabled$CFRunLoop$timer_fired$v1())

// timer_created (void *, unsigned long, unsigned long, long, char *, char *, void *), in
// CFRunLoopTimerCreate.
extern void
__dtrace_probe$CFRunLoop$timer_created$v1$766f6964202a$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67$6c6f6e67$63686172202a$63686172202a$766f6964202a(
    void *, unsigned long, unsigned long, long, char *, char *, void *
);
extern int __dtrace_isenabled$CFRunLoop$timer_created$v1(void);

#define CFRUNLOOP_TIMER_CREATED(arg0, arg1, arg2, arg3, arg4, arg5, arg6)                                                                                             \
    OCF_RUNLOOP_PROBE(                                                                                                                                                \
        __dtrace_probe$CFRunLoop$timer_created$v1$766f6964202a$756e7369676e6564206c6f6e67$756e7369676e6564206c6f6e67$6c6f6e67$63686172202a$63686172202a$766f6964202a( \
            arg0, arg1, arg2, arg3, arg4, arg5, arg6                                                                                                                  \
        )                                                                                                                                                             \
    )
#define CFRUNLOOP_TIMER_CREATED_ENABLED()                                                                    \
    OCF_RUNLOOP_PROBE_ENABLED(__dtrace_isenabled$CFRunLoop$timer_created$v1())

// timer_rescheduled (void *, unsigned long, long, char *, char *, void *), in CFRunLoopTimerSetNextFireDate.
extern void
__dtrace_probe$CFRunLoop$timer_rescheduled$v1$766f6964202a$756e7369676e6564206c6f6e67$6c6f6e67$63686172202a$63686172202a$766f6964202a(
    void *, unsigned long, long, char *, char *, void *
);
extern int __dtrace_isenabled$CFRunLoop$timer_rescheduled$v1(void);

#define CFRUNLOOP_TIMER_RESCHEDULED(arg0, arg1, arg2, arg3, arg4, arg5)                                                                        \
    OCF_RUNLOOP_PROBE(                                                                                                                         \
        __dtrace_probe$CFRunLoop$timer_rescheduled$v1$766f6964202a$756e7369676e6564206c6f6e67$6c6f6e67$63686172202a$63686172202a$766f6964202a( \
            arg0, arg1, arg2, arg3, arg4, arg5                                                                                                 \
        )                                                                                                                                      \
    )
#define CFRUNLOOP_TIMER_RESCHEDULED_ENABLED()                                                                \
    OCF_RUNLOOP_PROBE_ENABLED(__dtrace_isenabled$CFRunLoop$timer_rescheduled$v1())

// The wake-up reasons, in __CFRunLoopRun (wakeup_for_timeout in its timeout timer's event handler block).
extern void __dtrace_probe$CFRunLoop$wakeup_for_dispatch$v1(void);
extern void __dtrace_probe$CFRunLoop$wakeup_for_nothing$v1(void);
extern void __dtrace_probe$CFRunLoop$wakeup_for_source$v1(void);
extern void __dtrace_probe$CFRunLoop$wakeup_for_timeout$v1(void);
extern void __dtrace_probe$CFRunLoop$wakeup_for_timer$v1(void);
extern void __dtrace_probe$CFRunLoop$wakeup_for_wakeup$v1(void);

#define CFRUNLOOP_WAKEUP_FOR_DISPATCH() OCF_RUNLOOP_PROBE(__dtrace_probe$CFRunLoop$wakeup_for_dispatch$v1())
#define CFRUNLOOP_WAKEUP_FOR_NOTHING() OCF_RUNLOOP_PROBE(__dtrace_probe$CFRunLoop$wakeup_for_nothing$v1())
#define CFRUNLOOP_WAKEUP_FOR_SOURCE() OCF_RUNLOOP_PROBE(__dtrace_probe$CFRunLoop$wakeup_for_source$v1())
#define CFRUNLOOP_WAKEUP_FOR_TIMEOUT() OCF_RUNLOOP_PROBE(__dtrace_probe$CFRunLoop$wakeup_for_timeout$v1())
#define CFRUNLOOP_WAKEUP_FOR_TIMER() OCF_RUNLOOP_PROBE(__dtrace_probe$CFRunLoop$wakeup_for_timer$v1())
#define CFRUNLOOP_WAKEUP_FOR_WAKEUP() OCF_RUNLOOP_PROBE(__dtrace_probe$CFRunLoop$wakeup_for_wakeup$v1())

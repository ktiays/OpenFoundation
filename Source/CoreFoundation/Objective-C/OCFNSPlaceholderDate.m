//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSPlaceholderDate.h"

#import "CoreFoundation/Date/OCFDatePrivate.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "CoreFoundation/String/OCFString.h"

#include <assert.h>
#include <dispatch/dispatch.h>
#include <math.h>
#include <objc/objc-exception.h>
#include <time.h>

// Inferred organization: the __NSPlaceholderDate methods (+initialize 0x1c02801ac through
// -timeIntervalSinceReferenceDate 0x1c02804dc, with the .cold fragments at 0x1c030f114-0x1c030f158) lie
// between the __NSTaggedDate and NSDate methods of NSDate.m (the file name of the unit's assertions), which
// Objective-C/OCFNSDate.m restores (family E-misc); following the owner's decision, the class has its own
// translation unit here, and that unit defines the __immutablePlaceholderDate and decodeTaggedTimeInterval it
// uses hidden (see Objective-C/OCFNSPlaceholderDate.h). The
// +initialize block's invoke is restored in Date/OCFDate.c. Objective-C, manual reference counting. Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full; constants read with Local/Scratch/cf-core/macho.py; constant strings
// resolved from the ADRP/ADD encodings); iOS Device 24A446: unread. Reading: not yet written; the evidence is
// the assembly in Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json.

// libobjc's tagged-pointer tables (objc-internal.h; supplied). The GOT slots 0x222409420 and 0x222409408 lie
// outside the image and are bound by use, as Objective-C/OCFNSDate.m binds them (confidence: medium).
extern uintptr_t objc_debug_taggedpointer_obfuscator;
extern const uint8_t objc_debug_tag60_permutations[8];
extern Class objc_opt_self(id object);

// The concrete date class (__NSDate, the class reference ___SCR___NSDate), restored in
// Objective-C/OCFNSDate.m.
extern char concreteDateClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(__ONSDate));

// Native initialize.static_init, 0x22e6d3728 (__DATA,__bss). Non-external: the once predicate of
// +[__NSPlaceholderDate initialize].
static dispatch_once_t initializeStaticInit;

// The distant dates' intervals (qword_1C046C2F0 and qword_1C046C2F8): 63113904000.0 (bits 0x422d63c37f000000)
// and -63114076800.0 (bits 0xc22d63c8c5000000).
static const double kDistantFutureInterval = 63113904000.0;
static const double kDistantPastInterval = -63114076800.0;

@implementation __ONSPlaceholderDate

// Native +[__NSPlaceholderDate initialize], 0x1c02801ac, with .cold.1 (0x1c030f114: dispatch_once of
// initialize.static_init with ___block_literal_global_32, whose invoke Date/OCFDate.c restores as
// placeholderDateInitializeBlock). Non-external. The once runs unless the predicate is already ~0.
+ (void)initialize {
    dispatch_once(&initializeStaticInit, placeholderDateInitializeBlock);
}

// Native -[__NSPlaceholderDate initWithTimeIntervalSinceReferenceDate:], 0x1c028026c, with .cold.1
// (0x1c030f130) and .cold.2 (0x1c030f158). Non-external.
// A receiver other than the immutable placeholder answers nil. A NaN interval becomes the default NaN
// (0x7ff8000000000000). The distant-future and distant-past intervals answer the constant dates (the
// external pointers _NSDateDistantFuture and _NSDateDistantPast, when not NULL). Unless tagged dates are
// disabled, the interval is encoded (encodeTaggedTimeInterval, inlined): 0.0 as 0 and -0.0 as all ones;
// otherwise bit 63 moves to bit 59, the mantissa stays and the exponent is kept as a 7-bit field
// (exponent + 0x11, sign-extended, must give back exponent - 0x3EF), a zero result or an exponent outside
// that range not being encodable (a zero interval reaching the encoder fails the assertion "timeInterval !=
// 0", NSDate.m line 204). An encoded value must decode to the same interval (the assertion "ti ==
// decodeTaggedTimeInterval(encodedBits)" of __NSTaggedDate_new, NSDate.m line 268); the tagged pointer is
// 0x8000000000000006 | encoded << 3, obfuscated (objc_debug_taggedpointer_obfuscator) with its tag permuted
// (objc_debug_tag60_permutations), unless the obfuscated value has bits 63, 62 and 0-2 set, which leaves the
// plain value. An interval that cannot be tagged gets a new __NSDate (objc_opt_self, __CFAllocateObject with
// no extra bytes) storing the interval at offset 8.
- (id)initWithTimeIntervalSinceReferenceDate:(double)ti {
    if ((const void *)self != (const void *)__immutablePlaceholderDate) {
        return nil;
    }
    double interval = ti == ti ? ti : __builtin_nan("");
    id result;
    if (interval == kDistantFutureInterval) {
        result = (id)_ONSDateDistantFuture;
    } else if (interval == kDistantPastInterval) {
        result = (id)_ONSDateDistantPast;
    } else {
        if (__NSDateTaggedPointersDisabled & 1) {
            goto allocate;
        }
        uint64_t bits;
        __builtin_memcpy(&bits, &interval, sizeof(bits));
        uint64_t encoded;
        if (bits == 0) {
            encoded = 0;
        } else if (bits == 0x8000000000000000) {
            encoded = UINT64_MAX;
        } else {
            if (interval == 0.0) {
                __assert_rtn("encodeTaggedTimeInterval", "NSDate.m", 204, "timeInterval != 0");
            }
            uint64_t signAndMantissa = (bits & 0xFFFFFFFFFFFFF) | ((bits >> 63) << 59);
            uint32_t biased = (uint32_t)(bits >> 52) + 0x11;
            encoded = signAndMantissa | ((uint64_t)(biased & 0x7F) << 52);
            int64_t exponent = (int64_t)((int8_t)(uint8_t)(biased << 1) >> 1) + 0x3EF;
            if (exponent != (int64_t)((bits >> 52) & 0x7FF) || encoded == 0) {
                goto allocate;
            }
        }
        if (interval != decodeTaggedTimeInterval(encoded)) {
            __assert_rtn(
                "__NSTaggedDate_new", "NSDate.m", 268, "ti == decodeTaggedTimeInterval(encodedBits)"
            );
        }
        uintptr_t value = 0x8000000000000006 | (encoded << 3);
        uintptr_t obfuscated = objc_debug_taggedpointer_obfuscator ^ value;
        if ((obfuscated & 0xC000000000000007) == 0xC000000000000007) {
            result = (id)value;
        } else {
            uintptr_t tag = objc_debug_tag60_permutations[obfuscated & 7];
            result = (id)((obfuscated & 0xFFFFFFFFFFFFFFF8) | tag);
        }
    }
    if (result != nil) {
        return result;
    }
allocate:;
    id date = __CFAllocateObject(objc_opt_self((id)concreteDateClass), 0);
    *(double *)((uint8_t *)date + 8) = interval;
    return date;
}

// Native -[__NSPlaceholderDate init], 0x1c02803c4. Non-external. -initWithTimeIntervalSinceReferenceDate:
// with the current absolute time (clock_gettime(CLOCK_REALTIME) into a zeroed timespec; fma(nanoseconds,
// 1e-9, seconds + -978307200.0)), a tail call.
- (id)init {
    struct timespec now = {0, 0};
    clock_gettime(CLOCK_REALTIME, &now);
    double seconds = (double)now.tv_sec + -978307200.0;
    return [self initWithTimeIntervalSinceReferenceDate:fma((double)now.tv_nsec, 1e-09, seconds)];
}

// Native -[__NSPlaceholderDate retain], 0x1c02804c8. Non-external. The receiver (RET).
- (instancetype)retain {
    return self;
}

// Native -[__NSPlaceholderDate release], 0x1c02804cc. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[__NSPlaceholderDate retainCount], 0x1c02804d0. Non-external.
- (NSUInteger)retainCount {
    return UINTPTR_MAX;
}

// Native -[__NSPlaceholderDate dealloc], 0x1c02804d8. Non-external. Does nothing (RET); super is never
// reached, so the missing-super-call diagnostic is silenced here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-missing-super-calls"

- (void)dealloc {
}

#pragma clang diagnostic pop

// Native -[__NSPlaceholderDate timeIntervalSinceReferenceDate], 0x1c02804dc. Non-external.
// Raises NSInvalidArgumentException (+[NSException exceptionWithName:reason:userInfo:], objc_exception_throw)
// with "*** %@: method sent to an uninitialized date object" (the constant string 0x2227b8520) and the
// __CFFullMethodName of NSDate (looked up by name), the receiver and the selector; the reason string (system
// default allocator) is added to the current autorelease pool, and there is no user info.
- (double)timeIntervalSinceReferenceDate {
    OCFAllocatorRef allocator = kOCFAllocatorSystemDefault;
    OCFStringRef methodName =
        __CFFullMethodName((Class)__CFLookUpClass(OF_NAME_STRING(ONSDate)), (const void *)self, _cmd);
    OCFStringRef reason = OCFStringCreateWithFormat(
        allocator, NULL, OF_STR("*** %@: method sent to an uninitialized date object"), methodName
    );
    id exception = [ONSException exceptionWithName:(id)ONSInvalidArgumentException
                                            reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                                          userInfo:nil];
    objc_exception_throw(exception);
}

@end

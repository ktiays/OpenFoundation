//
//  Created by ktiays on 2026/10/30.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/Runtime/RuntimeTraces.h"
#include "Support/Trace.h"

#include <math.h>
#include <stdlib.h>

// Scenarios for the core track's runtime group (Documentation/Reference/Research/RuntimePrerequisites.md).
// Every value is observed through public API; deallocation is observed through a logging allocator.

enum {
    kBlockCapacity = 256,
};

static const void *gBlocks[kBlockCapacity];
static size_t gBlockCount;
static int gContextInfo;

static size_t blockIndex(const void *block) {
    for (size_t index = 0; index < gBlockCount; ++index) {
        if (gBlocks[index] == block) {
            return index + 1;
        }
    }
    return 0;
}

static const void *contextRetain(const void *info) {
    traceLine("context retain %s", info == &gContextInfo ? "info" : "other");
    return info;
}

static void contextRelease(const void *info) {
    traceLine("context release %s", info == &gContextInfo ? "info" : "other");
}

static void *contextAllocate(CFIndex size, CFOptionFlags hint, void *info) {
    void *block = malloc((size_t)size);
    if (gBlockCount < kBlockCapacity) {
        gBlocks[gBlockCount++] = block;
    }
    traceLine(
        "allocate block#%zu size %ld hint 0x%lx %s",
        gBlockCount,
        (long)size,
        (unsigned long)hint,
        info == &gContextInfo ? "info" : "other"
    );
    return block;
}

static void contextDeallocate(void *block, void *info) {
    size_t index = blockIndex(block);
    traceLine("deallocate block#%zu %s", index, info == &gContextInfo ? "info" : "other");
    if (index != 0) {
        // A freed block's address can be reused; keep its index but stop matching it.
        gBlocks[index - 1] = (const void *)(uintptr_t)1;
    }
    free(block);
}

static CFAllocatorRef createLoggingAllocator(void) {
    gBlockCount = 0;
    CFAllocatorContext context = {
        .version = 0,
        .info = &gContextInfo,
        .retain = contextRetain,
        .release = contextRelease,
        .allocate = contextAllocate,
        .deallocate = contextDeallocate,
    };
    return CFAllocatorCreate(kCFAllocatorDefault, &context);
}

static void traceType(const char *label, CFTypeRef object, CFTypeID expected) {
    CFTypeID typeID = CFGetTypeID(object);
    traceLine(
        "%s %s type %s getter %s tagged %d",
        label,
        traceObject(object),
        traceTypeID(typeID),
        typeID == expected ? "equal" : "different",
        traceIsTagged(object)
    );
}

static CFNumberRef number64(int64_t value) {
    return CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &value);
}

static CFNumberRef numberDouble(double value) {
    return CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat64Type, &value);
}

static void typeIDs(void) {
    traceScenario("runtime.type-ids");
    const UInt8 bytes[4] = {1, 2, 3, 4};
    CFStringRef shortString = CFStringCreateWithCString(kCFAllocatorDefault, "ab", kCFStringEncodingASCII);
    CFStringRef longString = CFStringCreateWithCString(
        kCFAllocatorDefault, "a string that is too long for any tagged representation", kCFStringEncodingASCII
    );
    CFNumberRef small = number64(7);
    CFNumberRef large = number64(INT64_C(1) << 62);
    CFNumberRef half = numberDouble(0.5);
    CFDateRef epoch = CFDateCreate(kCFAllocatorDefault, 0);
    CFDateRef later = CFDateCreate(kCFAllocatorDefault, 1000000000.125);
    CFDataRef data = CFDataCreate(kCFAllocatorDefault, bytes, 4);
    CFArrayRef emptyArray = CFArrayCreate(kCFAllocatorDefault, NULL, 0, &kCFTypeArrayCallBacks);
    CFArrayRef secondEmptyArray = CFArrayCreate(kCFAllocatorDefault, NULL, 0, &kCFTypeArrayCallBacks);
    const void *values[2] = {small, half};
    CFArrayRef array = CFArrayCreate(kCFAllocatorDefault, values, 2, &kCFTypeArrayCallBacks);
    CFDictionaryRef dictionary = CFDictionaryCreate(
        kCFAllocatorDefault, NULL, NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks
    );
    CFSetRef set = CFSetCreate(kCFAllocatorDefault, NULL, 0, &kCFTypeSetCallBacks);
    traceType("constant-string", CFSTR("constant"), CFStringGetTypeID());
    traceType("short-string", shortString, CFStringGetTypeID());
    traceType("long-string", longString, CFStringGetTypeID());
    traceType("number-7", small, CFNumberGetTypeID());
    traceType("number-2^62", large, CFNumberGetTypeID());
    traceType("number-0.5", half, CFNumberGetTypeID());
    traceType("boolean-true", kCFBooleanTrue, CFBooleanGetTypeID());
    traceType("null", kCFNull, CFNullGetTypeID());
    traceType("date-0", epoch, CFDateGetTypeID());
    traceType("date-1e9.125", later, CFDateGetTypeID());
    traceType("data", data, CFDataGetTypeID());
    traceType("empty-array", emptyArray, CFArrayGetTypeID());
    traceType("second-empty-array", secondEmptyArray, CFArrayGetTypeID());
    traceType("array", array, CFArrayGetTypeID());
    traceType("dictionary", dictionary, CFDictionaryGetTypeID());
    traceType("set", set, CFSetGetTypeID());
    traceType("allocator-default", CFAllocatorGetDefault(), CFAllocatorGetTypeID());
    traceType("allocator-system-default", kCFAllocatorSystemDefault, CFAllocatorGetTypeID());
    traceType("allocator-malloc", kCFAllocatorMalloc, CFAllocatorGetTypeID());
    traceType("allocator-null", kCFAllocatorNull, CFAllocatorGetTypeID());
    traceLine("default-is-system-default %d", CFAllocatorGetDefault() == kCFAllocatorSystemDefault);
    CFTypeRef objects[] = {
        shortString,
        longString,
        small,
        large,
        half,
        epoch,
        later,
        data,
        emptyArray,
        secondEmptyArray,
        array,
        dictionary,
        set
    };
    for (size_t index = 0; index < sizeof(objects) / sizeof(objects[0]); ++index) {
        CFRelease(objects[index]);
    }
}

static void retainCounts(void) {
    traceScenario("runtime.retain-counts");
    CFNumberRef large = number64(INT64_C(1) << 62);
    traceLine("heap-number %s count %ld", traceObject(large), (long)CFGetRetainCount(large));
    CFRetain(large);
    CFRetain(large);
    traceLine("after two retains %ld", (long)CFGetRetainCount(large));
    CFRelease(large);
    traceLine("after one release %ld", (long)CFGetRetainCount(large));
    CFRelease(large);
    CFRelease(large);
    CFNumberRef small = number64(7);
    traceLine("tagged-number count %ld", (long)CFGetRetainCount(small));
    CFRelease(small);
    traceLine("constant-string count %ld", (long)CFGetRetainCount(CFSTR("constant")));
    traceLine("boolean-true count %ld", (long)CFGetRetainCount(kCFBooleanTrue));
    traceLine("null count %ld", (long)CFGetRetainCount(kCFNull));
    traceLine("allocator-system-default count %ld", (long)CFGetRetainCount(kCFAllocatorSystemDefault));
    CFTypeRef retained = CFRetain(CFSTR("constant"));
    traceLine("retain returns argument %d", retained == CFSTR("constant"));
    CFRelease(retained);
}

static void hashes(void) {
    traceScenario("runtime.hashes");
    const int64_t integers[] = {0, 1, -1, 7, INT64_C(1) << 40, INT64_C(1) << 62, INT64_MIN, INT64_MAX};
    for (size_t index = 0; index < sizeof(integers) / sizeof(integers[0]); ++index) {
        CFNumberRef number = number64(integers[index]);
        traceLine("number %lld hash 0x%lx", (long long)integers[index], (unsigned long)CFHash(number));
        CFRelease(number);
    }
    const double reals[] = {0.0, -0.0, 0.5, 1.0, -1.5, 1e300, INFINITY, -INFINITY, NAN};
    for (size_t index = 0; index < sizeof(reals) / sizeof(reals[0]); ++index) {
        CFNumberRef number = numberDouble(reals[index]);
        traceLine("real %a hash 0x%lx", reals[index], (unsigned long)CFHash(number));
        CFRelease(number);
    }
    const char *strings[] = {
        "", "a", "ab", "abc", "constant", "a string that is too long for any tagged representation"
    };
    for (size_t index = 0; index < sizeof(strings) / sizeof(strings[0]); ++index) {
        CFStringRef string =
            CFStringCreateWithCString(kCFAllocatorDefault, strings[index], kCFStringEncodingASCII);
        traceLine("string \"%s\" hash 0x%lx", strings[index], (unsigned long)CFHash(string));
        CFRelease(string);
    }
    traceLine("constant-string hash 0x%lx", (unsigned long)CFHash(CFSTR("constant")));
    const UInt8 bytes[5] = {0, 1, 2, 3, 255};
    for (CFIndex length = 0; length <= 5; ++length) {
        CFDataRef data = CFDataCreate(kCFAllocatorDefault, bytes, length);
        traceLine("data length %ld hash 0x%lx", (long)length, (unsigned long)CFHash(data));
        CFRelease(data);
    }
    CFNumberRef one = number64(1);
    CFNumberRef two = number64(2);
    const void *values[2] = {one, two};
    CFArrayRef array = CFArrayCreate(kCFAllocatorDefault, values, 2, &kCFTypeArrayCallBacks);
    traceLine("array [1, 2] hash 0x%lx", (unsigned long)CFHash(array));
    CFDictionaryRef dictionary = CFDictionaryCreate(
        kCFAllocatorDefault,
        values,
        values,
        2,
        &kCFTypeDictionaryKeyCallBacks,
        &kCFTypeDictionaryValueCallBacks
    );
    traceLine("dictionary {1: 1, 2: 2} hash 0x%lx", (unsigned long)CFHash(dictionary));
    CFRelease(dictionary);
    CFRelease(array);
    CFRelease(two);
    CFRelease(one);
}

static void allocatorLifecycle(void) {
    traceScenario("runtime.allocator-lifecycle");
    CFAllocatorRef allocator = createLoggingAllocator();
    traceLine(
        "allocator %s type %s count %ld",
        traceObject(allocator),
        traceTypeID(CFGetTypeID(allocator)),
        (long)CFGetRetainCount(allocator)
    );
    const UInt8 bytes[4] = {1, 2, 3, 4};
    CFDataRef data = CFDataCreate(allocator, bytes, 4);
    traceLine(
        "data %s allocator %s data-count %ld allocator-count %ld",
        traceObject(data),
        traceObject(CFGetAllocator(data)),
        (long)CFGetRetainCount(data),
        (long)CFGetRetainCount(allocator)
    );
    traceLine(
        "data bytes %d %d %d %d",
        CFDataGetBytePtr(data)[0],
        CFDataGetBytePtr(data)[1],
        CFDataGetBytePtr(data)[2],
        CFDataGetBytePtr(data)[3]
    );
    traceLine("release data");
    CFRelease(data);
    traceForget(data);
    traceLine("after data allocator-count %ld", (long)CFGetRetainCount(allocator));
    traceLine("release allocator");
    CFRelease(allocator);
    traceForget(allocator);
    traceLine("done");
}

static const void *arrayRetain(CFAllocatorRef allocator, const void *value) {
    (void)allocator;
    traceLine("array retain %d", *(const int *)value);
    return value;
}

static void arrayRelease(CFAllocatorRef allocator, const void *value) {
    (void)allocator;
    traceLine("array release %d", *(const int *)value);
}

static Boolean arrayEqual(const void *left, const void *right) {
    traceLine("array equal %d %d", *(const int *)left, *(const int *)right);
    return *(const int *)left == *(const int *)right;
}

static void callbackOrder(void) {
    traceScenario("runtime.callback-order");
    static const int tokens[4] = {10, 20, 30, 20};
    const CFArrayCallBacks callbacks = {
        .version = 0,
        .retain = arrayRetain,
        .release = arrayRelease,
        .equal = arrayEqual,
    };
    const void *values[3] = {&tokens[0], &tokens[1], &tokens[2]};
    traceLine("create");
    CFArrayRef array = CFArrayCreate(kCFAllocatorDefault, values, 3, &callbacks);
    traceLine("count %ld", (long)CFArrayGetCount(array));
    traceLine("contains 20");
    traceLine("result %d", CFArrayContainsValue(array, CFRangeMake(0, 3), &tokens[3]));
    traceLine("release");
    CFRelease(array);
    traceLine("done");
}

static void autorelease(void) {
    traceScenario("runtime.autorelease");
    CFAllocatorRef allocator = createLoggingAllocator();
    const UInt8 bytes[2] = {9, 8};
    void *pool = objc_autoreleasePoolPush();
    CFDataRef data = CFDataCreate(allocator, bytes, 2);
    traceLine("data %s count %ld", traceObject(data), (long)CFGetRetainCount(data));
    CFTypeRef returned = CFAutorelease(data);
    traceLine("autorelease returns argument %d count %ld", returned == data, (long)CFGetRetainCount(data));
    traceLine("pop");
    objc_autoreleasePoolPop(pool);
    traceForget(data);
    traceLine("release allocator");
    CFRelease(allocator);
    traceLine("done");
}

void runRuntimeTraces(void) {
    typeIDs();
    retainCounts();
    hashes();
    allocatorLifecycle();
    callbackOrder();
    autorelease();
}

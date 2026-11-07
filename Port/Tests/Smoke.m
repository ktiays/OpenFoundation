//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// A smoke test of the wasm port: CF objects, Foundation objects, an exception raised by a restored method,
// a run-loop timer and a block on the main dispatch queue drained by the run loop, then regressions for
// wasm32 defects found by a consumer (mutable array growth, external string contents, the scanners' range
// argument, indirect tagged strings, block retain, NSData entry points, constant floating-point literals).
// Exit status 0 on success.

#import "CoreFoundation/Collections/OCFArray.h"
#import "CoreFoundation/Collections/OCFDictionary.h"
#import "CoreFoundation/Date/OCFDate.h"
#import "CoreFoundation/Number/OCFNumber.h"
#import "CoreFoundation/Objective-C/OCFNSArray.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "CoreFoundation/RunLoop/OCFRunLoop.h"
#import "CoreFoundation/String/OCFString.h"
#import "Foundation/Archiving/ONSObjCRuntimeNames.h"
#import "Foundation/Number/ONSNumber.h"
#import "Foundation/String/ONSString.h"

#include <dispatch/dispatch.h>
#include <objc/runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CFSTR(literal) OF_STR(literal)

// The messages the test sends, as Foundation declares them (the restored headers declare only what each unit
// sends).
@protocol SmokeMessages
- (const char *)UTF8String;
- (NSUInteger)count;
- (id)objectAtIndex:(NSUInteger)index;
- (id)name;
- (id)reason;
- (BOOL)isEqual:(id)object;
+ (id)arrayWithObjects:(id)firstObject, ...;
+ (id)array;
- (void)addObject:(id)object;
- (id)mutableCopy;
- (NSUInteger)length;
- (id)componentsSeparatedByString:(id)separator;
- (id)componentsJoinedByString:(id)separator;
- (id)stringByAppendingString:(id)string;
+ (id)stringWithCharacters:(const unsigned short *)characters length:(NSUInteger)length;
- (unsigned short)characterAtIndex:(NSUInteger)index;
- (NSInteger)integerValue;
- (double)doubleValue;
- (id)dataUsingEncoding:(NSUInteger)encoding;
+ (id)dataWithLength:(NSUInteger)length;
- (const void *)bytes;
- (void *)mutableBytes;
- (id)description;
- (id)retain;
- (void)release;
- (id)copy;
- (const char *)objCType;
+ (id)dictionaryWithObject:(id)object forKey:(id)key;
@end

static int failures;

static bool check(bool condition, const char *what) {
    printf("%s %s\n", condition ? "ok  " : "FAIL", what);
    if (!condition) {
        failures++;
    }
    return condition;
}

static const char *cString(CFStringRef string, char *buffer, size_t size) {
    if (string == NULL || !CFStringGetCString(string, buffer, (CFIndex)size, kCFStringEncodingUTF8)) {
        return "(null)";
    }
    return buffer;
}

static int timerFired;

static void timerCallback(CFRunLoopTimerRef timer, void *info) {
    (void)timer;
    (void)info;
    timerFired++;
}

int main(void) {
    char buffer[256];

    CFStringRef hello = CFStringCreateWithCString(NULL, "Hello, wasm", kCFStringEncodingUTF8);
    check(hello != NULL && CFStringGetLength(hello) == 11, "CFStringCreateWithCString and CFStringGetLength");

    CFStringRef formatted = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@ %d %.2f"), hello, 42, 3.14159);
    const char *formattedText = cString(formatted, buffer, sizeof(buffer));
    if (!check(
            strcmp(formattedText, "Hello, wasm 42 3.14") == 0, "CFStringCreateWithFormat with %@, %d and %.2f"
        )) {
        printf("     got \"%s\"\n", formattedText);
    }

    int value = 7;
    CFNumberRef number = CFNumberCreate(NULL, kCFNumberIntType, &value);
    int readBack = 0;
    check(CFNumberGetValue(number, kCFNumberIntType, &readBack) && readBack == 7, "CFNumber round trip");

    const void *values[] = {hello, number};
    CFArrayRef array = CFArrayCreate(NULL, values, 2, &kCFTypeArrayCallBacks);
    check(CFArrayGetCount(array) == 2 && CFArrayGetValueAtIndex(array, 1) == number, "CFArray");

    const void *keys[] = {CFSTR("greeting")};
    CFDictionaryRef dictionary = CFDictionaryCreate(
        NULL, keys, (const void **)&hello, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks
    );
    check(CFDictionaryGetValue(dictionary, CFSTR("greeting")) == hello, "CFDictionary with a constant key");

    id string = [NSString stringWithFormat:@"%@-%ld", @"objc", (long)5];
    const char *utf8 = [string UTF8String];
    if (!check(
            utf8 != NULL && strcmp(utf8, "objc-5") == 0, "+[NSString stringWithFormat:] and -UTF8String"
        )) {
        printf("     got \"%s\"\n", utf8 != NULL ? utf8 : "(null)");
    }

    id objects = [NSArray arrayWithObjects:@"a", @"b", nil];
    check([objects count] == 2, "+[NSArray arrayWithObjects:]");

    bool caughtPlain = false;
    @try {
        @throw objects;
    } @catch (id thrown) {
        caughtPlain = thrown == objects;
    }
    check(caughtPlain, "@throw and @catch of an object");

    bool caught = false;
    @try {
        [objects objectAtIndex:10];
    } @catch (id exception) {
        caught = [[exception name] isEqual:@"NSRangeException"];
        printf("     reason: %s\n", [[exception reason] UTF8String]);
    }
    check(caught, "NSRangeException from -[NSArray objectAtIndex:] beyond bounds");

    CFRunLoopTimerRef timer =
        CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent() + 0.05, 0, 0, 0, timerCallback, NULL);
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer, kCFRunLoopDefaultMode);
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1.0, true);
    check(timerFired == 1, "CFRunLoopTimer fires within CFRunLoopRunInMode");

    __block bool blockRan = false;
    dispatch_async(dispatch_get_main_queue(), ^{
      blockRan = true;
    });
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1.0, true);
    check(blockRan, "dispatch_async to the main queue drained by the run loop");

    // Regressions: wasm32 defects found by a consumer.
    id mutableArray = [(id)objc_getClass("NSMutableArray") array];
    for (int index = 0; index < 10; index++) {
        [mutableArray addObject:[NSString stringWithFormat:@"%d", index]];
    }
    check(
        [mutableArray count] == 10 && [[mutableArray objectAtIndex:9] isEqual:@"9"] &&
            [[mutableArray objectAtIndex:2] isEqual:@"2"],
        "-[NSMutableArray addObject:] past the initial capacity"
    );
    id copied = [[NSArray arrayWithObjects:@"a", @"b", @"c", @"d", @"e", nil] mutableCopy];
    check([[copied objectAtIndex:4] isEqual:@"e"], "-mutableCopy of a five-element array");
    [copied release];
    id components = [@"a,b,c" componentsSeparatedByString:@","];
    check(
        [components count] == 3 && [[components objectAtIndex:2] isEqual:@"c"],
        "-componentsSeparatedByString:"
    );

    id appended = [[NSString stringWithFormat:@"Hello %@", @"World"] stringByAppendingString:@"!"];
    check(
        [appended length] == 12 && strcmp([appended UTF8String], "Hello World!") == 0,
        "-stringByAppendingString: (external no-copy contents)"
    );

    char *mallocBytes = malloc(6);
    memcpy(mallocBytes, "wasm32", 6);
    CFStringRef external = CFStringCreateWithBytesNoCopy(
        NULL, (const UInt8 *)mallocBytes, 6, kCFStringEncodingASCII, false, kCFAllocatorMalloc
    );
    check(CFStringGetLength(external) == 6, "CFStringCreateWithBytesNoCopy with kCFAllocatorMalloc");
    CFRelease(external);
    @autoreleasepool {
        [[NSString stringWithFormat:@"%d", 42] stringByAppendingString:@" appended"];
    }
    check(true, "freeing strings with malloc'd external contents (kCFAllocatorMalloc deallocate)");

    unsigned short characters[1200];
    for (int index = 0; index < 1200; index++) {
        characters[index] = (unsigned short)('a' + index % 26);
    }
    id longString = [NSString stringWithCharacters:characters length:1200];
    check(
        [longString length] == 1200 && [longString characterAtIndex:1199] == characters[1199],
        "+stringWithCharacters:length: with 1,200 ASCII characters (external eight-bit contents)"
    );
    check(strlen([longString UTF8String]) == 1200, "-UTF8String of a 1,200-character string (NSMutableData)");
    check([[longString dataUsingEncoding:4] length] == 1200, "-dataUsingEncoding:");
    id zeroed = [(id)objc_getClass("NSMutableData") dataWithLength:16];
    check(
        [zeroed length] == 16 && ((const unsigned char *)[zeroed bytes])[15] == 0,
        "+[NSMutableData dataWithLength:]"
    );

    check([@"  42" integerValue] == 42, "-[NSString integerValue] (scanner range argument)");
    check([@"2.5" doubleValue] == 2.5, "-[NSString doubleValue] (scanner range argument)");

    id selectorName = NSStringFromSelector(@selector(integerValue));
    check([selectorName isEqual:@"integerValue"], "NSStringFromSelector (no indirect tagged string)");

    __block int captured = 0;
    void (^stackBlock)(void) = ^{
      captured++;
    };
    id heapBlock = [(id)stackBlock copy];
    [heapBlock retain];
    [heapBlock release];
    ((void (^)(void))heapBlock)();
    [heapBlock release];
    check(captured == 1, "-retain and -release of a heap block (__NSMallocBlock)");

    bool regexInitialized = true;
    @try {
        [(id)objc_getClass("NSRegularExpression") class];
    } @catch (id exception) {
        regexInitialized = false;
    }
    check(regexInitialized, "+[NSRegularExpression initialize] (+setVersion:)");

    id quoted = [[(id)objc_getClass("NSDictionary") dictionaryWithObject:@"a-b" forKey:@"k"] description];
    check(strstr([quoted UTF8String], "\"a-b\"") != NULL, "a quoted string in a collection description");

    id constantDouble = @2.5;
    id constantFloat = @1.5f;
    check(
        [constantDouble doubleValue] == 2.5 && [constantDouble objCType][0] == 'd' &&
            [constantFloat doubleValue] == 1.5 && [constantFloat objCType][0] == 'f',
        "constant floating-point number literals"
    );

    printf("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}

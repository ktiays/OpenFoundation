//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// The methods CoreFoundation adds to the root class NSObject (and NSThread's call-stack accessors), which are
// not restored yet. objc4's root class defers them to CoreFoundation and stops the process without it. The
// port is an isolated process (system names), so the category may replace them; each is a simple
// implementation:
//
// - -doesNotRecognizeSelector: raises NSInvalidArgumentException with CoreFoundation's reason format.
// - -methodSignatureForSelector: answers the signature of an implemented method from its type encoding.
// - -forwardInvocation: treats the message as unrecognized.
// - -description answers "<Class: address>"; +description answers the class name.
// - +version and +setVersion: (Foundation's NSObject archiving version) read and set the class version with
//   class_getVersion and class_setVersion; +[NSRegularExpression initialize] sends +setVersion:.
// - +[NSThread callStackReturnAddresses] and +callStackSymbols answer empty arrays: wasm frames cannot be
//   walked from C.

#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"
#import "Foundation/String/ONSString.h"
#import "Foundation/Thread/ONSThread.h"

#include <objc/runtime.h>

@protocol PortNSObjectMessages
+ (id)signatureWithObjCTypes:(const char *)types;
+ (id)array;
- (SEL)selector;
@end

static void raiseUnrecognized(id object, SEL selector, BOOL isClass) {
    Class cls = isClass ? (Class)object : object_getClass(object);
    id reason = [ONSString stringWithFormat:@"%c[%s %s]: unrecognized selector sent to %s %p",
                                            isClass ? '+' : '-',
                                            class_getName(cls),
                                            sel_getName(selector),
                                            isClass ? "class" : "instance",
                                            object];
    @throw [ONSException exceptionWithName:(id)ONSInvalidArgumentException reason:reason userInfo:nil];
}

static id signatureForMethod(Method method) {
    if (method == NULL) {
        return nil;
    }
    return [(id)objc_lookUpClass(OF_NAME_STRING(ONSMethodSignature))
        signatureWithObjCTypes:method_getTypeEncoding(method)];
}

@interface NSObject (OpenFoundationPort)
@end

@implementation NSObject (OpenFoundationPort)

+ (void)doesNotRecognizeSelector:(SEL)selector {
    raiseUnrecognized(self, selector, YES);
}

- (void)doesNotRecognizeSelector:(SEL)selector {
    raiseUnrecognized(self, selector, NO);
}

+ (id)methodSignatureForSelector:(SEL)selector {
    return signatureForMethod(class_getClassMethod(self, selector));
}

- (id)methodSignatureForSelector:(SEL)selector {
    return signatureForMethod(class_getInstanceMethod(object_getClass(self), selector));
}

+ (id)instanceMethodSignatureForSelector:(SEL)selector {
    return signatureForMethod(class_getInstanceMethod(self, selector));
}

+ (void)forwardInvocation:(id)invocation {
    raiseUnrecognized(self, [invocation selector], YES);
}

- (void)forwardInvocation:(id)invocation {
    raiseUnrecognized(self, [invocation selector], NO);
}

+ (id)description {
    return [ONSString stringWithUTF8String:class_getName(self)];
}

- (id)description {
    return [ONSString stringWithFormat:@"<%s: %p>", class_getName(object_getClass(self)), self];
}

+ (id)debugDescription {
    return [self description];
}

- (id)debugDescription {
    return [self description];
}

+ (NSInteger)version {
    return class_getVersion(self);
}

+ (void)setVersion:(NSInteger)version {
    class_setVersion(self, (int)version);
}

@end

@interface ONSThread (OpenFoundationPortCallStack)
@end

@implementation ONSThread (OpenFoundationPortCallStack)

+ (id)callStackReturnAddresses {
    return [(id)objc_lookUpClass(OF_NAME_STRING(ONSArray)) array];
}

+ (id)callStackSymbols {
    return [(id)objc_lookUpClass(OF_NAME_STRING(ONSArray)) array];
}

@end

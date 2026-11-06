//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSString.h"

// The NSPlaceholderString unit (String/ONSPlaceholderString.m). Its method overrides are inherited
// declarations (String/ONSString.h).

// Native __NSDescriptionWithLocaleFunc (0x1c05d8df8) and __NSDescriptionWithStringProxyFunc (0x1c05d8e60),
// external: the copy-description and context-description callbacks -[NSPlaceholderString
// initWithFormat:locale:arguments:] passes to _CFStringCreateWithFormatAndArgumentsAux2 (the
// CopyDescriptionFunction and ContextDescriptionFunction types of String/OCFStringFormat.h). Each answers a
// retained string or NULL.
OF_EXPORT OCFStringRef _ONSDescriptionWithLocaleFunc(void *object, const void *locale);
OF_EXPORT OCFStringRef _ONSDescriptionWithStringProxyFunc(
    void *object, const void *context, const void *patternString, bool evaluate, bool *handled
);

#ifdef __OBJC__
// Selectors -[NSPlaceholderString initWithFormat:locale:arguments:] sends to NSAttributedString objects,
// declared for the message sends only; no protocol object is emitted.

#endif

#ifdef __OBJC__
// Native class NSPlaceholderString (__DATA,__objc_data 0x22e6dd5a0, external), an NSString subclass;
// class_ro_t 0x22285f8f0: flags 0, instance start and size 8, no ivars. Its methods override NSString's
// initializers and NSObject's reference counting; their declarations are inherited.
__attribute__((visibility("default")))
@interface ONSPlaceholderString : ONSString
@end
#endif

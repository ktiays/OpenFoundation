//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>

#include <objc/objc.h>

// The class and selector name conversions of the native NSObjCRuntime cluster (0x1c057e81c-0x1c057ebc8 in the
// iOS Simulator Foundation __text section) that the keyed archivers call; defined in
// Archiving/ONSObjCRuntimeNames.m. SDK signatures for the external functions (NSString spelled id).
OF_EXPORT id ONSStringFromSelector(SEL selector);
OF_EXPORT SEL ONSSelectorFromString(id aSelectorName);
OF_EXPORT id ONSStringFromClass(Class aClass);
OF_EXPORT Class ONSClassFromString(id aClassName);

// Private (were private externals): +1 strings over the runtime's own name bytes (no copy, not freed).
OF_EXTERN id NSCopyStringFromSelector(SEL selector);
OF_EXTERN id NSCopyStringFromClass(Class cls);

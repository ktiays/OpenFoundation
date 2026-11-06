//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>

#include <OpenCoreFoundation/Base/OCFBase.h>

// Native __NSCoderGetTrustRestrictions, 0x1c04dc3a4 (was a private external; defined in
// Archiving/ONSCoder.m): the process's archive trust restriction level, computed once.
OF_EXTERN unsigned int _NSCoderGetTrustRestrictions(void);

// Native NSInvalidArchiveOperationException 0x2227d54a0 and NSInvalidUnarchiveOperationException
// 0x2227d54a8, external constant-string references.
OF_EXPORT const OCFStringRef ONSInvalidArchiveOperationException;
OF_EXPORT const OCFStringRef ONSInvalidUnarchiveOperationException;

#ifdef __OBJC__
#import <objc/NSObject.h>

// Native __NSCoderIsAppleSigningIdentifier, 0x1c04dc2b8. External.
OF_EXPORT BOOL _ONSCoderIsAppleSigningIdentifier(id identifier);

// Native class NSCoder (__DATA,__objc_data 0x22e6d6ae8, external), an NSObject subclass; class_ro_t
// 0x222849ea0: instance start and size 8, no ivars (iOS Simulator 24A434, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified). Its method list lies in the shared cache
// outside the extracted image. Only the methods the scope reaches are declared: the thirteen restored here
// are direct methods (objc_direct: called with BL, no _cmd argument, a nil receiver answers zero), absent
// from the runtime method list. iOS Device 24A446: unread. Reading: Source/Foundation/Archiving/Reading.md.
__attribute__((visibility("default")))
@interface ONSCoder : NSObject

- (BOOL)_validateDecodeCollectionAllowedClassesRequirementsWithClasses:(id)classes
                                               forMethodExceptionProem:(id)proem __attribute__((objc_direct));
- (BOOL)_isPossibleNestedCollectionClass:(Class)cls __attribute__((objc_direct));
- (BOOL)_warnAboutNSObjectInAllowedClassesWithException:(BOOL)exception __attribute__((objc_direct));
- (id)__allowedClassesDescriptionForClass:(Class)cls __attribute__((objc_direct));
- (BOOL)_walkAllowedClassListLookingForClass:(Class)cls
                                      forKey:(id)key
                              allowedClasses:(id)allowedClasses
                           strictModeEnabled:(BOOL)strictModeEnabled
             alwaysEnforceExplicitSubclasses:(BOOL)alwaysEnforceExplicitSubclasses
                           specialCaseObject:(BOOL)specialCaseObject __attribute__((objc_direct));
- (BOOL)_shouldExemptProcessBasedOnArchivingExceptionRules:(const char *)path __attribute__((objc_direct));
- (BOOL)_walkAllowedClassListValidatingInstance:(id)instance
                                         forKey:(id)key
                                 allowedClasses:(id)allowedClasses
                              strictModeEnabled:(BOOL)strictModeEnabled
                alwaysEnforceExplicitSubclasses:(BOOL)alwaysEnforceExplicitSubclasses
                              specialCaseObject:(BOOL)specialCaseObject
                            invalidElementClass:(Class *)invalidElementClass __attribute__((objc_direct));
- (id)__descriptionForUnexpectedClass:(Class)cls
                               forKey:(id)key
                       allowedClasses:(id)allowedClasses __attribute__((objc_direct));
- (id)_allowedClassesDescriptionForClasses:(id)classes __attribute__((objc_direct));
- (id)__descriptionForUnexpectedElementClass:(Class)cls
                             forContainerKey:(id)key
                              allowedClasses:(id)allowedClasses __attribute__((objc_direct));
- (void)_warnAboutPlistType:(Class)type
                     forKey:(id)key
    missingInAllowedClasses:(id)allowedClasses __attribute__((objc_direct));
- (void)__failWithUnexpectedClass:(Class)cls
                           forKey:(id)key
                   allowedClasses:(id)allowedClasses __attribute__((objc_direct));
- (void)__failWithUnexpectedElementClass:(Class)cls
                         forContainerKey:(id)key
                          allowedClasses:(id)allowedClasses __attribute__((objc_direct));

@end
#endif

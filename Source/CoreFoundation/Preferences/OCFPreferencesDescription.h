//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Preferences/OCFPreferences.h"

#include <xpc/xpc.h>

// The CFPreferences description helpers (CFPreferencesCopyKeyDescription 0x1c0231340 through
// CFPreferencesRedactedContainerPath 0x1c0231894 in the iOS Simulator __text section), restored in
// Preferences/OCFPreferencesDescription.c.

// Native CFPreferencesCopyKeyDescription, 0x1c0231340. Hidden (was a private external). A malloc'd
// description of an XPC key (a string, or a dictionary of keys as "[a, b, ]"), "no key" otherwise; the
// caller frees it.
OF_EXTERN char *CFPreferencesCopyKeyDescription(xpc_object_t key);

// Native CFPreferencesClientDescriptionForMessage, 0x1c023160c. Hidden (was a private external). The
// client of a request message as "<process name>[<pid>]" (or "<pid>"), autoreleased.
OF_EXTERN OCFStringRef CFPreferencesClientDescriptionForMessage(xpc_object_t message);

// Native CFPreferencesRedactedName, 0x1c023176c. Hidden (was a private external). A name shortened to its
// first and last characters around the count of the others, autoreleased.
OF_EXTERN OCFStringRef CFPreferencesRedactedName(OCFStringRef name);

// Native CFPreferencesRedactedUserIdentifier, 0x1c0231820, and CFPreferencesRedactedContainerPath,
// 0x1c0231894. Hidden (were private externals). The user unchanged when it is NULL, a preferences user
// constant or shorter than 3 characters, else redacted (autoreleased); the container path with each
// component redacted except the listed ones (autoreleased), NULL when it is NULL or not convertible.
OF_EXTERN OCFStringRef CFPreferencesRedactedUserIdentifier(OCFStringRef user);

OF_EXTERN OCFStringRef CFPreferencesRedactedContainerPath(OCFStringRef path);

//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/Notification/OCFNotificationCenter.h"

// These native CoreFoundation exports are reached by the notification center family. Their definitions
// belong to the CoreFoundation notification subsystem.
OF_EXPORT const NotificationConfiguration _kOCFXNotificationConfigurationCustomLocalConfiguration;

// The four dependent notification records are in other Foundation units. The registration list only
// takes their addresses. Their initialization functions are outside this family's capture.
OF_EXTERN const DependentNotification _NSUbiquityIdentityDidChangeDependentNotification;
OF_EXTERN const DependentNotification _NSBundleDidLoadDependentNotification;
OF_EXTERN const DependentNotification _NSProcessInfoThermalStateDidChangeDependentNotification;
OF_EXTERN const DependentNotification _NSProcessInfoPowerStateDidChangeDependentNotification;

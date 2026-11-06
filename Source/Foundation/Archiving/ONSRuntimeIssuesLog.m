//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Archiving/ONSRuntimeIssuesLog.h"

#import "Foundation/Archiving/ONSArchivingPrivate.h"

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. iOS Device divergence: _NSRuntimeIssuesLog's iOS Device 24A446 body remains unread.
// Reading: Source/Foundation/Archiving/Reading.md.

// Native qword_22E76C930 and qword_22E76C938: zero-initialized handle and dispatch_once token.
os_log_t qword_22E76C930;
static dispatch_once_t qword_22E76C938;

// Native _NSRuntimeIssuesLog, 0x1c0593960. Non-external (was a private external).
os_log_t _NSRuntimeIssuesLog(void) {
    if (qword_22E76C938 != -1) {
        // Native ____NSRuntimeIssuesLog_block_invoke, 0x1c05939a0. Non-external.
        dispatch_once(&qword_22E76C938, ^{
          qword_22E76C930 = os_log_create("com.apple.runtime-issues", "Foundation");
        });
    }
    return qword_22E76C930;
}

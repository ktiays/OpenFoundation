//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Preferences/OCFPrefsPendingKVONotification.h"

// Inferred organization: -[_CFPrefsPendingKVONotification dealloc] (0x1c02ea090) lies in the iOS Simulator
// __text section just before applyBlockApplier (0x1c02ea124), the first function of the _CFXPreferences unit
// that Preferences/OCFXPreferences.m restores, and the class's direct setters (0x1c0315b24-0x1c0315b4c) lie
// with that unit's direct methods in the cold region; following the owner's decision, the class has its own
// translation unit. Objective-C compiled with manual reference counting (objc_release in -dealloc). The
// native source path is not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full); iOS Device 24A446:
// unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json.
//
// The super send of -dealloc passes the selector address 0x2387a9f83.

@implementation _OCFPrefsPendingKVONotification

// Native -[_CFPrefsPendingKVONotification setSource:] (0x1c0315b24), -setOldValues: (0x1c0315b38) and
// -setValues: (0x1c0315b4c). Hidden (were private externals); synthesized direct setters: nothing for a nil
// receiver, otherwise objc_setProperty_atomic with the ivar's offset (a tail call). The synthesized direct
// getters have no native counterpart (inlined into their callers natively).
@synthesize source;
@synthesize oldValues;
@synthesize values;

// Native -[_CFPrefsPendingKVONotification dealloc], 0x1c02ea090. Non-external. The source, old values and
// values are released in that order (objc_release, so NULL is allowed); super's -dealloc.
- (void)dealloc {
    [source release];
    [(id)oldValues release];
    [(id)values release];
    [super dealloc];
}

@end

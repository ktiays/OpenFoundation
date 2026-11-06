//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSPlaceholderLocale.h"

#import "CoreFoundation/Locale/OCFLocale.h"

// Inferred organization: the _NSPlaceholderLocale methods (0x1c02ad470-0x1c02ad47c) lie in the iOS Simulator
// __text section just before the NSLocale methods that Objective-C/OCFNSLocale.m restores; following the
// owner's decision, the class has its own translation unit. Objective-C, manual reference counting (the
// unit's MRC is inherited from the other Objective-C units; these methods do not establish it). The native
// source path is not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full); iOS Device 24A446:
// unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json.

@implementation _ONSPlaceholderLocale

// Native -[_NSPlaceholderLocale retain], 0x1c02ad470. Non-external. The receiver (RET).
- (instancetype)retain {
    return self;
}

// Native -[_NSPlaceholderLocale release], 0x1c02ad474. Non-external. Does nothing.
- (oneway void)release {
}

// Native -[_NSPlaceholderLocale autorelease], 0x1c02ad478. Non-external. The receiver (RET).
- (instancetype)autorelease {
    return self;
}

// Native -[_NSPlaceholderLocale initWithLocaleIdentifier:], 0x1c02ad47c. Non-external. nil for a nil
// identifier; otherwise CFLocaleCreate with the system default allocator (a tail call). The placeholder
// receiver is not released.
- (id)initWithLocaleIdentifier:(id)string {
    if (string == nil) {
        return nil;
    }
    return (id)OCFLocaleCreate(kOCFAllocatorSystemDefault, (OCFLocaleIdentifier)string);
}

@end

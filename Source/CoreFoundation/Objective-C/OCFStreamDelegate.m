//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFStreamDelegate.h"

// Inferred organization: the _CFStreamDelegate methods (0x1c01febc4-0x1c01fed00) lie together in the iOS
// Simulator __text section among the CFStream functions that Stream/OCFStream.c restores (another family's
// file); following the owner's decision, the class has its own translation unit. Objective-C compiled with
// manual reference counting. The native source path is not in the evidence. Unless a definition says
// otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly
// read in full); iOS Device 24A446: unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json.
//
// The super sends pass the selector addresses 0x2387aa0ea (init) and 0x2387a9f83 (dealloc).

@implementation _OCFStreamDelegate

// Native -[_CFStreamDelegate initWithStreamEvents:callback:context:], 0x1c01febc4. Non-external. After
// super's -init, the event mask and callback are stored and the 40-byte context copied; when the copy has
// both retain and release callbacks, its info is replaced by retain(info).
- (id)initWithStreamEvents:(OCFOptionFlags)streamEvents
                  callback:(void (*)(void *stream, OCFStreamEventType type, void *info))callback
                   context:(OCFStreamClientContext *)context {
    self = [super init];
    if (self != nil) {
        _flags = streamEvents;
        _cb = callback;
        _client = *context;
        if (_client.retain != NULL && _client.release != NULL) {
            _client.info = _client.retain(_client.info);
        }
    }
    return self;
}

// Native -[_CFStreamDelegate dealloc], 0x1c01fec80. Non-external. A release callback with an info releases
// it; then super's -dealloc.
- (void)dealloc {
    if (_client.release != NULL && _client.info != NULL) {
        _client.release(_client.info);
    }
    [super dealloc];
}

// Native -[_CFStreamDelegate stream:handleEvent:], 0x1c01fed00. Non-external. An event in the mask calls the
// callback with the stream, the event and the context's info (a tail call).
- (void)stream:(id)stream handleEvent:(NSUInteger)eventCode {
    if ((_flags & eventCode) != 0) {
        _cb(stream, eventCode, _client.info);
    }
}

@end

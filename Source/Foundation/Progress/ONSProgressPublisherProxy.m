//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Progress/ONSProgressPublisherProxy.h"
#include "CoreFoundation/String/OCFString.h"

// Inferred organization: the NSProgressPublisherProxy methods lie at 0x1c04b8f00-0x1c04b9203 of the iOS
// Simulator Foundation __text section, with the NSProgress publishing classes. Objective-C compiled with
// manual reference counting. Unless a definition says otherwise: iOS Simulator 24A434, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full). iOS Device 24A446:
// unread.

extern id objc_retain(id object);
extern void objc_release(id object);

extern char progressProxyStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSString));

// The messages the unit sends to objects of other classes, declared for the sends only.
@protocol ProgressPublisherProxyMessages
+ (id)stringWithFormat:(id)format, ...;
- (id)stringByAppendingString:(id)string;
- (id)copy;
@end

@implementation ONSProgressPublisherProxy

// Native -[NSProgressPublisherProxy initWithForwarder:onConnection:publisherID:values:], 0x1c04b8f00.
// Non-external. The values are copied; the other arguments retained.
- (id)initWithForwarder:(id)forwarder
           onConnection:(id)connection
            publisherID:(id)publisherID
                 values:(id)values {
    self = [super init];
    if (self != nil) {
        _forwarder = objc_retain(forwarder);
        _publisherID = objc_retain(publisherID);
        _connection = objc_retain(connection);
        _values = [values copy];
    }
    return self;
}

// Native -[NSProgressPublisherProxy dealloc], 0x1c04b8fb8. Non-external. _itemLocation is not released here.
- (void)dealloc {
    objc_release(_lowerCaseCategoryName);
    objc_release(_publisherID);
    objc_release(_forwarder);
    objc_release(_connection);
    objc_release(_values);
    [super dealloc];
}

// Native -[NSProgressPublisherProxy descriptionWithIndenting:], 0x1c04b917c. Non-external.
- (id)descriptionWithIndenting:(id)indenting {
    id deeper = [indenting stringByAppendingString:(id)OF_STR("    ")];
    return [(id)progressProxyStringClass
        stringWithFormat:(id)OF_STR("%@<%@ %p> forwarder: %@, publisher ID: %@\n%@location: %p"),
                         indenting,
                         [self class],
                         self,
                         _forwarder,
                         _publisherID,
                         deeper,
                         _itemLocation];
}

// Native -[NSProgressPublisherProxy description], 0x1c04b91f8. Non-external.
- (id)description {
    return [self descriptionWithIndenting:(id)OF_STR("")];
}

@end

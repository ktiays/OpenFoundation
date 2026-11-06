//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSUserDefaults.h"

#import "CoreFoundation/String/OCFString.h"
#import "CoreFoundation/URL/OCFURL.h"

// Inferred organization: the NSUserDefaults methods of this image (0x1c02ea13c-0x1c02ea20c) lie together in
// the iOS Simulator __text section among the preferences code; following the owner's decision, the class has
// its own translation unit. Objective-C compiled with manual reference counting. The native source path is
// not in the evidence. Unless a definition says otherwise: iOS Simulator 24A434, UUID
// D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence: verified (assembly read in full); iOS Device 24A446:
// unread. Reading: not yet written; the evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-corpus-delta8.json.

@implementation ONSUserDefaults

// Native -[NSUserDefaults _identifier], 0x1c02ea13c. Non-external; the ivar, unretained.
- (id)_identifier {
    return (id)_identifier_;
}

// Native -[NSUserDefaults _setIdentifier:], 0x1c02ea144. Non-external. The old identifier is released when
// set; the new one is a copy (CFStringCreateCopy, kCFAllocatorSystemDefault) of the argument or, for nil, of
// the constant string "kCFPreferencesCurrentApplication".
- (void)_setIdentifier:(id)identifier {
    if (_identifier_ != NULL) {
        OCFRelease(_identifier_);
    }
    OCFStringRef source = identifier != nil ? (OCFStringRef)identifier
                                            : OF_STR(
                                                  "kCFPreferences"
                                                  "CurrentApplication"
                                              );
    _identifier_ = OCFStringCreateCopy(kOCFAllocatorSystemDefault, source);
}

// Native -[NSUserDefaults _container], 0x1c02ea194. Non-external; the ivar, unretained.
- (id)_container {
    return (id)_container_;
}

// Native -[NSUserDefaults _setContainer:], 0x1c02ea19c. Non-external. A URL releases the old container (when
// set) and stores its POSIX path (CFURLCopyFileSystemPath, style 0); nil releases and clears an existing
// container and leaves an empty one alone.
- (void)_setContainer:(id)url {
    if (url != nil) {
        if (_container_ != NULL) {
            OCFRelease(_container_);
        }
        _container_ = OCFURLCopyFileSystemPath((OCFURLRef)url, kOCFURLPOSIXPathStyle);
    } else if (_container_ != NULL) {
        OCFRelease(_container_);
        _container_ = NULL;
    }
}

// Native -[NSUserDefaults _observingCFPreferences], 0x1c02ea1ec. Non-external; whether _kvo_ is set.
- (BOOL)_observingCFPreferences {
    return _kvo_ != nil;
}

// Native -[NSUserDefaults _kvo], 0x1c02ea1fc. Non-external; the ivar, unretained.
- (id)_kvo {
    return _kvo_;
}

// Native -[NSUserDefaults _setKVO:], 0x1c02ea204. Non-external; stored unretained.
- (void)_setKVO:(id)kvo {
    _kvo_ = kvo;
}

// Native -[NSUserDefaults volatileDomainNames], 0x1c02ea20c. Non-external; the synthesized atomic getter,
// objc_getProperty(self, _cmd, 0x20, YES) (a tail call). The property's attributes lie outside the extracted
// image (unread; any atomic retain or copy property has this getter).
@synthesize volatileDomainNames = _volatileDomainNames;

@end

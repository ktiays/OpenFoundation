//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSMutableStringProxyForMutableAttributedString.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified unless noted. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.

@implementation ONSMutableStringProxyForMutableAttributedString

// Native -[NSMutableStringProxyForMutableAttributedString initWithMutableAttributedString:], 0x1c04c6388.
// Non-external.
- (instancetype)initWithMutableAttributedString:(ONSMutableAttributedString *)string {
    self = [super init];
    if (self != nil) {
        mutableAttributedString = objc_retain(string);
    }
    return self;
}

// Native -[NSMutableStringProxyForMutableAttributedString dealloc], 0x1c04c6410. Non-external.
- (void)dealloc {
    objc_release(mutableAttributedString);
    [super dealloc];
}

// Native -[NSMutableStringProxyForMutableAttributedString length], 0x1c04c648c. Non-external.
- (ONSUInteger)length {
    return [(ONSString *)[mutableAttributedString string] length];
}

// Native -[NSMutableStringProxyForMutableAttributedString characterAtIndex:], 0x1c04c64ac. Non-external.
- (unsigned short)characterAtIndex:(ONSUInteger)index {
    return [(ONSString *)[mutableAttributedString string] characterAtIndex:index];
}

// Native -[NSMutableStringProxyForMutableAttributedString getCharacters:range:], 0x1c04c64dc.
// Non-external.
- (void)getCharacters:(unsigned short *)buffer range:(ONSRange)range {
    [(ONSString *)[mutableAttributedString string] getCharacters:buffer range:range];
}

// Native -[NSMutableStringProxyForMutableAttributedString replaceCharactersInRange:withString:],
// 0x1c04c6524. Non-external. The assembly forwards both range words and the replacement string.
- (void)replaceCharactersInRange:(ONSRange)range withString:(id)string {
    [mutableAttributedString replaceCharactersInRange:range withString:string];
}

@end

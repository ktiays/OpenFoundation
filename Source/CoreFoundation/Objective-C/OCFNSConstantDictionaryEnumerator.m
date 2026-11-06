//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSConstantDictionaryEnumerator.h"

// Inferred organization: the two __NSConstantDictionaryEnumerator methods (0x1c01f8418-0x1c01f851f) lie
// together in the iOS Simulator __text section; following the owner's decision, the class has its own
// translation unit. Objective-C compiled with manual reference counting (the methods establish none: the
// dictionary is stored unretained). Evidence: Local/Evidence/cf-core/iphone-simulator-corpus-delta10.json.
// Unless a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// The offsets of NSConstantDictionary's _count, _keys and _objects ivars (their external _OBJC_IVAR_$
// symbols, read with LDRSW), which -nextObject reads; the ivars are not visible outside that class's
// interface, so the offset symbols are read as natively.
extern const int32_t
    constantEnumeratorCountOffset __asm("_OBJC_IVAR_$_" OF_NAME_STRING(ONSConstantDictionary) "._count");
extern const int32_t
    constantEnumeratorKeysOffset __asm("_OBJC_IVAR_$_" OF_NAME_STRING(ONSConstantDictionary) "._keys");
extern const int32_t
    constantEnumeratorObjectsOffset __asm("_OBJC_IVAR_$_" OF_NAME_STRING(ONSConstantDictionary) "._objects");

@implementation __ONSConstantDictionaryEnumerator

// Native -[__NSConstantDictionaryEnumerator initWithConstantDictionary:enumerateKeys:], 0x1c01f8418.
// Non-external. [super init], then (without a nil check) the dictionary (not retained), index 0 and the
// keys flag are stored.
- (id)initWithConstantDictionary:(ONSConstantDictionary *)dictionary enumerateKeys:(BOOL)enumerateKeys {
    self = [super init];
    _underlyingDictionary = dictionary;
    _currentIndex = 0;
    _isForKeys = enumerateKeys;
    return self;
}

// Native -[__NSConstantDictionaryEnumerator nextObject], 0x1c01f84ac. Non-external. Below the dictionary's
// count, the key (when _isForKeys is nonzero) or the object at the current index, and the index advances;
// otherwise nil.
- (id)nextObject {
    char *dictionary = (char *)_underlyingDictionary;
    NSUInteger index = _currentIndex;
    if (index >= *(NSUInteger *)(dictionary + constantEnumeratorCountOffset)) {
        return nil;
    }
    int32_t offset = _isForKeys ? constantEnumeratorKeysOffset : constantEnumeratorObjectsOffset;
    id object = (*(id **)(dictionary + offset))[index];
    _currentIndex = index + 1;
    return object;
}

@end

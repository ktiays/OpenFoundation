//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSDictionarySharedKeySet.h"

// Inferred organization: +[NSDictionary(NSSharedKeySetDictionary) sharedKeySetForKeys:] (0x1c02a7530) and
// +[NSMutableDictionary(NSSharedKeySetDictionary) dictionaryWithSharedKeySet:] (0x1c02a7608) lie together in
// the iOS Simulator __text section, after the OUTLINED_FUNCTION pieces of the preceding unit and before
// _CFListFormatterGetTypeID; the two categories form their own translation unit here (the native source path
// is not in the evidence). Objective-C, manual reference counting (inherited from the dictionary units; these
// methods neither retain nor release). iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified (assembly read in full, iphone-simulator-corpus-delta8.json). iOS Device 24A446:
// unread. Reading: Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

// Class symbols sent to directly: NSSharedKeySet (class reference off_2227CE3C0, and ___SCR_NSSharedKeySet
// for the kind test), NSMutableDictionary (off_2227CE378) and NSSharedKeyDictionary (addressed directly).
extern char sharedKeySetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSSharedKeySet));
extern char sharedKeyDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                 OF_NAME_STRING(ONSSharedKeyDictionary));
extern char mutableDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSMutableDictionary));

// Selectors this unit sends that no included header declares, declared for the message sends only; the
// protocol is never adopted and no protocol object is emitted.
@protocol SharedKeySetDictionaryMessages

+ (id)keySetWithKeys:(id)keys;
+ (id)sharedKeyDictionaryWithKeySet:(id)keySet;
+ (id)dictionaryWithCapacity:(NSUInteger)numItems;
- (BOOL)isEmpty;

@end

@implementation ONSDictionary (NSSharedKeySetDictionary)

// Native +[NSDictionary(NSSharedKeySetDictionary) sharedKeySetForKeys:], 0x1c02a7530. Non-external. A nil
// argument, or one failing _NSIsNSArray, throws NSInvalidArgumentException with a reason made from the
// __CFExceptionProem of the receiver and selector (kCFAllocatorSystemDefault), autoreleased through
// _CFAutoreleasePoolAddObject; otherwise +[NSSharedKeySet keySetWithKeys:] (a tail call).
+ (id)sharedKeySetForKeys:(id)keys {
    OCFStringRef reason;
    if (keys == nil) {
        reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault, NULL, OF_STR("%@: keys cannot be nil"), __CFExceptionProem(self, _cmd)
        );
    } else {
        if (_ONSIsNSArray(keys)) {
            return [(Class)sharedKeySetClass keySetWithKeys:keys];
        }
        reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("%@: keys must be a kind of NSArray"),
            __CFExceptionProem(self, _cmd)
        );
    }
    id exception = [(id)dictionaryExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                            reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                                                          userInfo:nil];
    objc_exception_throw(exception);
}

@end

@implementation ONSMutableDictionary (NSSharedKeySetDictionary)

// Native +[NSMutableDictionary(NSSharedKeySetDictionary) dictionaryWithSharedKeySet:], 0x1c02a7608.
// Non-external. A nil key set, or one that is not a kind of NSSharedKeySet, throws NSInvalidArgumentException
// (the second reason names the argument's -class, sent after __CFExceptionProem); an empty key set (-isEmpty)
// answers +[NSMutableDictionary dictionaryWithCapacity:0], any other
// +[NSSharedKeyDictionary sharedKeyDictionaryWithKeySet:] (tail calls).
+ (id)dictionaryWithSharedKeySet:(id)keySet {
    OCFStringRef reason;
    if (keySet == nil) {
        reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("%@: keyset cannot be nil"),
            __CFExceptionProem(self, _cmd)
        );
    } else {
        if ([keySet isKindOfClass:(Class)sharedKeySetClass]) {
            if ([keySet isEmpty]) {
                return [(Class)mutableDictionaryClass dictionaryWithCapacity:0];
            }
            return [(Class)sharedKeyDictionaryClass sharedKeyDictionaryWithKeySet:keySet];
        }
        OCFStringRef proem = __CFExceptionProem(self, _cmd);
        Class keySetClass = [keySet class];
        reason = OCFStringCreateWithFormat(
            kOCFAllocatorSystemDefault,
            NULL,
            OF_STR("%@: keyset must be an object created by +sharedKeySetForKeys: instead of '%@'"),
            proem,
            keySetClass
        );
    }
    id exception = [(id)dictionaryExceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                                            reason:_OCFAutoreleasePoolAddObject(NULL, reason)
                                                          userInfo:nil];
    objc_exception_throw(exception);
}

@end

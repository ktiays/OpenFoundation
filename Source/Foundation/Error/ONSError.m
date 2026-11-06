//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Error/ONSError.h"
#import "Foundation/Error/ONSErrorKeyedArchivalErrors.h"

#include "CoreFoundation/Base/OCFUtilities.h"
#include "CoreFoundation/Error/OCFError.h"
#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#include "CoreFoundation/String/OCFString.h"

#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <os/log.h>
#include <stdatomic.h>

// Inferred organization: the NSError methods (0x1c0502fd8-0x1c05047e8) and ___validateKnownUserInfoKeyTypes
// lie together in the iOS Simulator __text section; the native source path is not in the evidence.
// Objective-C compiled with manual reference counting natively (explicit objc_retain, objc_release and
// objc_autorelease calls at the points MRC places them). Unless a definition says otherwise: iOS Simulator
// 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in
// full); iOS Device 24A446: unread. Reading: Local/Scratch/cf-core-fd-misc/Reading.md; original assembly in
// Local/Evidence/cf-core/iphone-simulator-foundation-link.json (family FM-misc).
//
// Class references that lie outside the extracted image are loaded from the shared cache's uniqued GOT slots;
// their bindings follow from the selectors sent and from the alphabetical order of those slots (confidence:
// medium): 0x22227fad0 NSDictionary, 0x22227fae0 NSException, 0x22227fb88 NSURL, 0x22227f500
// NSInvalidArgumentException.

// The classes this unit references by symbol: NSError itself where the native code loads its class reference
// without a +class message (___SCR_NSError, 0x22289ee20), NSString (Foundation's, the GOT slot 0x22289f3d8),
// NSCFError
// (__DATA,__objc_data 0x22e6d7920, Foundation's NSError subclass of the toll-free bridged errors, was a
// private external; not restored on this branch), NSDictionary, NSException and NSURL (CoreFoundation's).
extern char stringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSString));
extern char errorClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSError));
extern char cfErrorClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSCFError));
extern char dictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDictionary));
extern char exceptionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSException));
extern char urlClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSURL));
extern char errorMutableDictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableDictionary));
extern char errorArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSArray));

// Selectors this unit sends to objects of classes outside it (NSString, NSDictionary, NSArray, NSException,
// NSBundle, NSCoder and NSSet), declared for the message sends only; no protocol object is emitted.
@protocol ErrorMessages

+ (id)stringWithFormat:(id)format, ...;
+ (id)dictionary;
+ (id)exceptionWithName:(id)name reason:(id)reason userInfo:(id)userInfo;
- (id)objectForKey:(id)key;
- (NSUInteger)count;
- (id)localizedStringForKey:(id)key value:(id)value table:(id)table;
- (BOOL)allowsKeyedCoding;
- (void)encodeInteger:(NSInteger)value forKey:(id)key;
- (void)encodeObject:(id)object forKey:(id)key;
- (void)encodeValuesOfObjCTypes:(const char *)types, ...;
- (id)allowedClasses;
- (id)setByAddingObjectsFromSet:(id)set;
- (BOOL)containsValueForKey:(id)key;
- (NSInteger)decodeIntegerForKey:(id)key;
- (id)error;
- (id)decodeObjectOfClass:(Class)cls forKey:(id)key;
- (id)decodeObjectOfClasses:(id)classes forKey:(id)key;
- (void)decodeValuesOfObjCTypes:(const char *)types, ...;
- (void)failWithError:(id)error;
- (id)copy;
- (BOOL)isEqual:(id)object;
- (NSUInteger)hash;
- (id)allocWithZone:(struct _NSZone *)zone;
- (void)setObject:(id)object forKeyedSubscript:(id)key;
+ (id)arrayWithObjects:(const id *)objects count:(NSUInteger)count;

@end

static id _validateKnownUserInfoKeyTypes(id userInfo);

@implementation ONSError

// Native -[NSError init], 0x1c0502fd8. External class, non-external method. The first call only (the byte
// init.warned) logs an error-level message through _NSOSLog when that type is enabled (one os_log buffer of
// two zero bytes); then [super init] (a tail call).
- (id)init {
    // Foundation defect: native 0x1c0503000-0x1c050300c reads/writes init.warned without
    // synchronization. A relaxed exchange removes that global data race and retains one-time logging.
    static _Atomic(bool) warned;
    if (!atomic_exchange_explicit(&warned, true, memory_order_relaxed)) {
        os_log_t log = _NSOSLog();
        os_log_error(
            log,
            "-[NSError init] called; this results in an invalid NSError instance. It will raise an exception "
            "in a "
            "future release. Please call errorWithDomain:code:userInfo: or initWithDomain:code:userInfo:. "
            "This "
            "message shown only once."
        );
    }
    return [super init];
}

// Native -[NSError initWithDomain:code:userInfo:], 0x1c0503098. A nil domain releases the receiver and throws
// NSInvalidArgumentException ("Invalid domain=nil in -[%@ %s]", the receiver's class and the selector name).
// Otherwise [super init]; when it is not nil, the copies of the domain and user info and the code are stored
// and the call-stack return addresses are cleared (_CFErrorSetCallStackReturnAddresses with NULL).
- (id)initWithDomain:(id)domain code:(NSInteger)code userInfo:(id)dict {
    if (domain == nil) {
        id reason = [(id)stringClass
            stringWithFormat:(id)OF_STR("Invalid domain=nil in -[%@ %s]"), [self class], sel_getName(_cmd)];
        [self release];
        @throw [(id)exceptionClass exceptionWithName:(id)ONSInvalidArgumentException
                                              reason:reason
                                            userInfo:nil];
    }
    self = [super init];
    if (self != nil) {
        id domainCopy = [domain copy];
        _code = code;
        _domain = domainCopy;
        _userInfo = [dict copy];
        _OCFErrorSetCallStackReturnAddresses((OCFErrorRef)self, NULL);
    }
    return self;
}

// Native +[NSError errorWithDomain:code:userInfo:], 0x1c05031ac. objc_alloc, the initializer, then
// autoreleased (a tail call).
+ (id)errorWithDomain:(id)domain code:(NSInteger)code userInfo:(id)dict {
    return [[[self alloc] initWithDomain:domain code:code userInfo:dict] autorelease];
}

// Native -[NSError dealloc], 0x1c05031ec. Releases the domain (offset 24) and the user info (offset 32), then
// [super dealloc].
- (void)dealloc {
    [_domain release];
    [_userInfo release];
    [super dealloc];
}

// Native -[NSError code], 0x1c0503268.
- (NSInteger)code {
    return _code;
}

// Native -[NSError domain], 0x1c0503270. Retained and autoreleased (a tail call).
- (id)domain {
    return [[_domain retain] autorelease];
}

// Native -[NSError userInfo], 0x1c0503288. The ivar is retained and autoreleased; when it is not nil the ivar
// is reloaded and returned. Otherwise, for executables linked on or after version 8
// (_CFExecutableLinkedOnOrAfter), an empty dictionary (+[NSDictionary dictionary], a tail call); otherwise
// nil.
- (id)userInfo {
    if ([[_userInfo retain] autorelease] != nil) {
        return _userInfo;
    }
    if (_OCFExecutableLinkedOnOrAfter(8)) {
        return [(id)dictionaryClass dictionary];
    }
    return nil;
}

// Native -[NSError _callStackReturnAddresses], 0x1c05032e0. _CFErrorCopyCallStackReturnAddresses on the
// receiver (passed through x0), autoreleased (a tail call).
- (id)_callStackReturnAddresses {
    return [(id)_OCFErrorCopyCallStackReturnAddresses(self) autorelease];
}

// Native -[NSError isEqual:], 0x1c05032f4. The receiver itself is equal and nil is not; the argument must be
// a kind of NSError (the class reference ___SCR_NSError); then equal codes, equal domains (-isEqual:) and
// identical user infos, or user infos equal by -isEqual: (a tail call).
- (BOOL)isEqual:(id)object {
    if (self == object) {
        return YES;
    }
    if (object == nil) {
        return NO;
    }
    if (![object isKindOfClass:(Class)errorClass]) {
        return NO;
    }
    if ([self code] != [object code]) {
        return NO;
    }
    if (![[self domain] isEqual:[object domain]]) {
        return NO;
    }
    id userInfo = [self userInfo];
    id otherUserInfo = [object userInfo];
    if (userInfo == otherUserInfo) {
        return YES;
    }
    return [userInfo isEqual:otherUserInfo];
}

// Native -[NSError hash], 0x1c05033bc. The domain's hash plus the code.
- (NSUInteger)hash {
    NSUInteger domainHash = [[self domain] hash];
    return domainHash + [self code];
}

// Native -[NSError localizedDescription], 0x1c050389c. The user info's "NSLocalizedDescription" (a literal,
// not NSLocalizedDescriptionKey) copied and autoreleased; else _CFErrorCreateLocalizedDescription,
// autoreleased; else the Foundation bundle's "FoundationErrors" format "The operation couldn\U2019t be
// completed. (%1$@ error %2$ld.)" (the backslash escape is literal ASCII in the key; value "") applied to the
// domain and code.
- (id)localizedDescription {
    id description = [[self userInfo] objectForKey:(id)OF_STR("NSLocalizedDescription")];
    if (description != nil) {
        return [[description copy] autorelease];
    }
    id created = (id)_OCFErrorCreateLocalizedDescription((OCFErrorRef)self);
    if (created != nil) {
        return [created autorelease];
    }
    id stringClassObject = (id)stringClass;
    id format = [_ONSFoundationBundle()
        localizedStringForKey:(id)OF_STR("The operation couldn\\U2019t be completed. (%1$@ error %2$ld.)")
                        value:(id)OF_STR("")
                        table:(id)OF_STR("FoundationErrors")];
    id domain = [self domain];
    NSInteger code = [self code];
    return [stringClassObject stringWithFormat:format, domain, code];
}

// Native -[NSError description], 0x1c0503da4. _CFErrorCreateDebugDescription on the receiver, autoreleased (a
// tail call).
- (id)description {
    return [(id)_OCFErrorCreateDebugDescription((OCFErrorRef)self) autorelease];
}

// Native -[NSError redactedDescription], 0x1c0503dcc. _CFErrorCreateRedactedDescription on the receiver,
// autoreleased (a tail call).
- (id)redactedDescription {
    return [(id)_OCFErrorCreateRedactedDescription(self) autorelease];
}

// Native -[NSError copyWithZone:], 0x1c0503de0. An empty user info (count 0) is copied as nil. For a receiver
// whose class is NSCFError, a new NSError allocated in the zone is initialized with the domain, code and user
// info (a tail call). Otherwise an instance of the receiver's class is allocated in the zone and its ivars
// are stored directly: the domain's copy, the code, the user info's copy, then the receiver's call-stack
// return addresses.
- (id)copyWithZone:(struct _NSZone *)zone {
    id userInfo = [self userInfo];
    if ([userInfo count] == 0) {
        userInfo = nil;
    }
    Class cls = [self class];
    if (cls == (Class)cfErrorClass) {
        id copy = [(id)errorClass allocWithZone:zone];
        id domain = [self domain];
        NSInteger code = [self code];
        return [copy initWithDomain:domain code:code userInfo:userInfo];
    }
    ONSError *copy = [(id)cls allocWithZone:zone];
    copy->_domain = [[self domain] copy];
    copy->_code = [self code];
    copy->_userInfo = [userInfo copy];
    _OCFErrorSetCallStackReturnAddresses((OCFErrorRef)copy, (OCFArrayRef)[self _callStackReturnAddresses]);
    return copy;
}

// Native -[NSError encodeWithCoder:], 0x1c0503ec8. An empty user info (count 0) is encoded as nil. A keyed
// coder gets "NSCode" (integer), "NSDomain" and "NSUserInfo"; any other coder gets the values of "i@@" (the
// code truncated to int, the domain, the user info).
- (void)encodeWithCoder:(id)coder {
    id userInfo = [self userInfo];
    if ([userInfo count] == 0) {
        userInfo = nil;
    }
    if ([coder allowsKeyedCoding]) {
        [coder encodeInteger:[self code] forKey:(id)OF_STR("NSCode")];
        [coder encodeObject:[self domain] forKey:(id)OF_STR("NSDomain")];
        [coder encodeObject:userInfo forKey:(id)OF_STR("NSUserInfo")];
        return;
    }
    int code = (int)[self code];
    id domain = [self domain];
    [coder encodeValuesOfObjCTypes:"i@@", &code, &domain, &userInfo];
}

// Native +[NSError supportsSecureCoding], 0x1c0503ffc.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSError initWithCoder:], 0x1c0504004. [super init] (its result is not tested), then the allowed
// classes: __NSPropertyListClasses_ForArchival_ErrorAndException added to the coder's -allowedClasses. A
// keyed coder must contain "NSCode"; the code, the domain (of class NSString, retained) and the user info (of
// the allowed classes, retained) are decoded in turn, each followed by a check of the coder's -error (set:
// the receiver is released and nil returned); a nil domain fails. Any other coder decodes "i@@" into a zeroed
// int code and the domain and user-info ivars. A user info that is not nil must be a dictionary
// (_NSIsNSDictionary) whose known keys hold values of the expected types (_validateKnownUserInfoKeyTypes). A
// failure builds a corrupt-archive error (+_readCorruptErrorWithFormat: on NSError, the method proem first),
// releases the receiver, hands the error to -failWithError: and returns nil.
- (id)initWithCoder:(id)coder {
    self = [super init];
    id plistClasses = __ONSPropertyListClasses_ForArchival_ErrorAndException();
    id allowedClasses = [[coder allowedClasses] setByAddingObjectsFromSet:plistClasses];
    id error;
    if ([coder allowsKeyedCoding]) {
        if (![coder containsValueForKey:(id)OF_STR("NSCode")]) {
            error = [ONSError _readCorruptErrorWithFormat:(id)OF_STR("%@: missing error code"),
                                                          _ONSMethodExceptionProem(self, _cmd)];
            goto fail;
        }
        _code = [coder decodeIntegerForKey:(id)OF_STR("NSCode")];
        if ([coder error] != nil) {
            goto release;
        }
        _domain = [[coder decodeObjectOfClass:[(id)stringClass class] forKey:(id)OF_STR("NSDomain")] retain];
        if ([coder error] != nil) {
            goto release;
        }
        if (_domain == nil) {
            error = [ONSError _readCorruptErrorWithFormat:(id)OF_STR("%@: missing domain string"),
                                                          _ONSMethodExceptionProem(self, _cmd)];
            goto fail;
        }
        _userInfo = [[coder decodeObjectOfClasses:allowedClasses forKey:(id)OF_STR("NSUserInfo")] retain];
        if ([coder error] != nil) {
            goto release;
        }
    } else {
        int code = 0;
        [coder decodeValuesOfObjCTypes:"i@@", &code, &_domain, &_userInfo];
        _code = code;
    }
    if (_userInfo == nil) {
        return self;
    }
    if (!_ONSIsNSDictionary(_userInfo)) {
        error =
            [ONSError _readCorruptErrorWithFormat:(id)OF_STR(
                                                      "%@: userInfo dictionary found in archive was not a "
                                                      "dictionary"
                                                  ),
                                                  _ONSMethodExceptionProem(self, _cmd)];
        goto fail;
    }
    if (_userInfo == nil) {
        return self;
    }
    id key = _validateKnownUserInfoKeyTypes(_userInfo);
    if (key == nil) {
        return self;
    }
    error = [ONSError _readCorruptErrorWithFormat:(id)OF_STR(
                                                      "%@: userInfo dictionary found in archive contained an "
                                                      "invalid value type for key %@"
                                                  ),
                                                  _ONSMethodExceptionProem(self, _cmd),
                                                  key];
fail:
    [self release];
    [coder failWithError:error];
    return nil;
release:
    [self release];
    return nil;
}

// Native -[NSError _cfTypeID], 0x1c05047e8. CFErrorGetTypeID (a tail call).
- (OCFTypeID)_cfTypeID {
    return OCFErrorGetTypeID();
}

@end

// Native __validateKnownUserInfoKeyTypes, 0x1c0504274. Non-external. Returns the first known user-info key
// whose value has an unexpected type, or nil. In order: "NSUnderlyingError" (a kind of [NSError class]);
// "NSLocalizedDescription", "NSLocalizedFailureReason" and "NSLocalizedRecoverySuggestion" (strings,
// _NSIsNSString); "NSLocalizedRecoveryOptions" (an array, _NSIsNSArray, of strings); "NSHelpAnchor" and
// "NSDebugDescription" (strings); "NSStringEncoding" (a number, _NSIsNSNumber); "NSURL" (a kind of [NSURL
// class]); "NSFilePath", "NSSourceFilePathErrorKey" and "NSDestinationFilePath" (strings);
// "NSMultipleUnderlyingErrorsKey" and "NSDetailedErrors" (arrays of kinds of [NSError class]); and
// "NSUserStringVariant" (an array of strings, or a string). Absent keys pass. The arrays are fast-enumerated
// (sixteen objects per batch).
static id _validateKnownUserInfoKeyTypes(id userInfo) {
    id value = [userInfo objectForKey:(id)OF_STR("NSUnderlyingError")];
    if (value != nil && ![value isKindOfClass:[ONSError class]]) {
        return (id)OF_STR("NSUnderlyingError");
    }
    value = [userInfo objectForKey:(id)OF_STR("NSLocalizedDescription")];
    if (value != nil && !_ONSIsNSString(value)) {
        return (id)OF_STR("NSLocalizedDescription");
    }
    value = [userInfo objectForKey:(id)OF_STR("NSLocalizedFailureReason")];
    if (value != nil && !_ONSIsNSString(value)) {
        return (id)OF_STR("NSLocalizedFailureReason");
    }
    value = [userInfo objectForKey:(id)OF_STR("NSLocalizedRecoverySuggestion")];
    if (value != nil && !_ONSIsNSString(value)) {
        return (id)OF_STR("NSLocalizedRecoverySuggestion");
    }
    value = [userInfo objectForKey:(id)OF_STR("NSLocalizedRecoveryOptions")];
    if (value != nil) {
        if (!_ONSIsNSArray(value)) {
            return (id)OF_STR("NSLocalizedRecoveryOptions");
        }
        for (id option in value) {
            if (!_ONSIsNSString(option)) {
                return (id)OF_STR("NSLocalizedRecoveryOptions");
            }
        }
    }
    value = [userInfo objectForKey:(id)OF_STR("NSHelpAnchor")];
    if (value != nil && !_ONSIsNSString(value)) {
        return (id)OF_STR("NSHelpAnchor");
    }
    value = [userInfo objectForKey:(id)OF_STR("NSDebugDescription")];
    if (value != nil && !_ONSIsNSString(value)) {
        return (id)OF_STR("NSDebugDescription");
    }
    value = [userInfo objectForKey:(id)OF_STR("NSStringEncoding")];
    if (value != nil && !_ONSIsNSNumber(value)) {
        return (id)OF_STR("NSStringEncoding");
    }
    // The key "NSURL" is spelled as two adjacent literals: it is a user-info key, never a runtime class name,
    // and must keep its native value in both naming modes.
    value = [userInfo objectForKey:(id)OF_STR(
                                       "NS"
                                       "URL"
                                   )];
    if (value != nil && ![value isKindOfClass:[(id)urlClass class]]) {
        return (id)OF_STR(
            "NS"
            "URL"
        );
    }
    value = [userInfo objectForKey:(id)OF_STR("NSFilePath")];
    if (value != nil && !_ONSIsNSString(value)) {
        return (id)OF_STR("NSFilePath");
    }
    value = [userInfo objectForKey:(id)OF_STR("NSSourceFilePathErrorKey")];
    if (value != nil && !_ONSIsNSString(value)) {
        return (id)OF_STR("NSSourceFilePathErrorKey");
    }
    value = [userInfo objectForKey:(id)OF_STR("NSDestinationFilePath")];
    if (value != nil && !_ONSIsNSString(value)) {
        return (id)OF_STR("NSDestinationFilePath");
    }
    value = [userInfo objectForKey:(id)OF_STR("NSMultipleUnderlyingErrorsKey")];
    if (value != nil) {
        if (!_ONSIsNSArray(value)) {
            return (id)OF_STR("NSMultipleUnderlyingErrorsKey");
        }
        for (id error in value) {
            if (![error isKindOfClass:[ONSError class]]) {
                return (id)OF_STR("NSMultipleUnderlyingErrorsKey");
            }
        }
    }
    value = [userInfo objectForKey:(id)OF_STR("NSDetailedErrors")];
    if (value != nil) {
        if (!_ONSIsNSArray(value)) {
            return (id)OF_STR("NSDetailedErrors");
        }
        for (id error in value) {
            if (![error isKindOfClass:[ONSError class]]) {
                return (id)OF_STR("NSDetailedErrors");
            }
        }
    }
    value = [userInfo objectForKey:(id)OF_STR("NSUserStringVariant")];
    if (value == nil) {
        return nil;
    }
    if (!_ONSIsNSArray(value)) {
        if (!_ONSIsNSString(value)) {
            return (id)OF_STR("NSUserStringVariant");
        }
        return nil;
    }
    for (id variant in value) {
        if (!_ONSIsNSString(variant)) {
            return (id)OF_STR("NSUserStringVariant");
        }
    }
    return nil;
}

@implementation ONSError (FoundationSwiftAdditions)

// Native
// +[NSError(FoundationSwiftAdditions)
// _cocoaErrorWithCode:path:url:underlying:variant:source:destination:debugDescription:], 0x1c05048cc.
// Non-external method. An autoreleased NSCocoaErrorDomain error whose user info holds each non-nil argument:
// NSFilePath, NSURL, NSUnderlyingError, NSUserStringVariant (a one-element array of the variant),
// NSSourceFilePathErrorKey, NSDestinationFilePath and NSDebugDescription, in that order. The user info is a
// CoreFoundation NSMutableDictionary (objc_alloc_init; uniqued GOT slot 0x22227fb28) and the array an NSArray
// (+arrayWithObjects:count:, slot 0x22227fa90); confidence: medium for the bindings.
+ (id)_cocoaErrorWithCode:(NSInteger)code
                     path:(id)path
                      url:(id)url
               underlying:(id)underlying
                  variant:(id)variant
                   source:(id)source
              destination:(id)destination
         debugDescription:(id)debugDescription {
    id userInfo = [[(id)errorMutableDictionaryClass alloc] init];
    if (path != nil) {
        [userInfo setObject:path forKeyedSubscript:(id)OF_STR("NSFilePath")];
    }
    if (url != nil) {
        // The user-info key "NSURL", spelled as two literals (never a runtime class name).
        [userInfo setObject:url
            forKeyedSubscript:(id)OF_STR(
                                  "NS"
                                  "URL"
                              )];
    }
    if (underlying != nil) {
        [userInfo setObject:underlying forKeyedSubscript:(id)OF_STR("NSUnderlyingError")];
    }
    if (variant != nil) {
        id variants[1] = {variant};
        [userInfo setObject:[(id)errorArrayClass arrayWithObjects:variants count:1]
            forKeyedSubscript:(id)OF_STR("NSUserStringVariant")];
    }
    if (source != nil) {
        [userInfo setObject:source forKeyedSubscript:(id)OF_STR("NSSourceFilePathErrorKey")];
    }
    if (destination != nil) {
        [userInfo setObject:destination forKeyedSubscript:(id)OF_STR("NSDestinationFilePath")];
    }
    if (debugDescription != nil) {
        [userInfo setObject:debugDescription forKeyedSubscript:(id)OF_STR("NSDebugDescription")];
    }
    id error = [[ONSError alloc] initWithDomain:(id)OF_STR(OF_NAME_STRING(ONSCocoaErrorDomain))
                                           code:code
                                       userInfo:userInfo];
    objc_release(userInfo);
    return objc_autorelease(error);
}

@end

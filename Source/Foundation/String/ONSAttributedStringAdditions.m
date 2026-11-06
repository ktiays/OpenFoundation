//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSAttributedStringAdditions.h"
#import "Foundation/String/ONSAttributedStringInternal.h"
#include "CoreFoundation/Number/OCFNumber.h"
#include "CoreFoundation/String/OCFAttributedString.h"

// Inferred organization: categories of the base attributed-string unit, CF additions and port-coding
// unit. Evidence: Local/Evidence/cf-core/iphone-simulator-foundation-link.json, FC-sets-attributed. iOS
// Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified.
// iOS Device divergence: iOS Device 24A446 remains unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md.

// Native __NSAttributedStringMayRequireIntentResolutionKey, 0x2227d0480, non-external: a pointer to its
// own storage, used by address as an associated-object key (static data read from the pinned Mach-O).
static const void *const _NSAttributedStringMayRequireIntentResolutionKey =
    &_NSAttributedStringMayRequireIntentResolutionKey;

// Native object at 0x22288aa58, non-external: NSConstantIntegerNumber {isa, "i", 0}, 24 bytes. The dirtying
// methods compare values with this object through isEqual:. Its type string and full record were read
// from static data; no replacement NSNumber allocation or tagged-pointer value is fitted here.
static const struct {
    const void *isa;
    const char *type;
    int64_t value;
} kNoInlinePresentationIntent = {attributedStringConstantIntegerClass, "i", 0};

@implementation ONSAttributedString (_NSPresentationIntentDirtying)

// Native -[NSAttributedString(_NSPresentationIntentDirtying) _mayRequireIntentResolution], 0x1c04c481c.
// Non-external.
- (BOOL)_mayRequireIntentResolution {
    return objc_getAssociatedObject(self, &_NSAttributedStringMayRequireIntentResolutionKey) != nil;
}

// Native -[NSAttributedString(_NSPresentationIntentDirtying) _markRequiringIntentResolution], 0x1c04c4840.
// Non-external. Native policy 0x303 (OBJC_ASSOCIATION_COPY); the shared-cache 0x2222807e8 binding to
// __kCFBooleanTrue has medium confidence (nm confirms the import; the slot is outside this image).
- (void)_markRequiringIntentResolution {
    objc_setAssociatedObject(
        self,
        &_NSAttributedStringMayRequireIntentResolutionKey,
        (id)&__kOCFBooleanTrue,
        (objc_AssociationPolicy)0x303
    );
}

// Native -[NSAttributedString(_NSPresentationIntentDirtying)
// _willRequireIntentResolutionWhenContainingAttributes:], 0x1c04c486c. Non-external.
- (BOOL)_willRequireIntentResolutionWhenContainingAttributes:(id)attributes {
    id value = [attributes objectForKeyedSubscript:ONSInlinePresentationIntentAttributeName];
    if (value != nil && ![value isEqual:(id)&kNoInlinePresentationIntent]) {
        return YES;
    }
    return [attributes objectForKeyedSubscript:ONSPresentationIntentAttributeName] != nil;
}

// Native -[NSAttributedString(_NSPresentationIntentDirtying)
// _willRequireIntentResolutionWhenContainingAttribute:value:], 0x1c04c48d4. Non-external. Compare the
// attribute name with the inline-intent name even when value is nil; compare the presentation name next.
- (BOOL)_willRequireIntentResolutionWhenContainingAttribute:(id)attribute value:(id)value {
    BOOL inlineIntent = [attribute isEqual:ONSInlinePresentationIntentAttributeName];
    if (value != nil && inlineIntent && ![value isEqual:(id)&kNoInlinePresentationIntent]) {
        return YES;
    }
    BOOL presentationIntent = [attribute isEqual:ONSPresentationIntentAttributeName];
    return value != nil ? presentationIntent : NO;
}

@end

@implementation ONSAttributedString (NSCFAdditions)

// Native -[NSAttributedString(NSCFAdditions) _cfTypeID], 0x1c04d1d44. Non-external; tail-call.
- (unsigned long)_cfTypeID {
    return OCFAttributedStringGetTypeID();
}

// Native -[NSAttributedString(NSCFAdditions) _createAttributedSubstringWithRange:], 0x1c04d1d48.
// Non-external.
- (id)_createAttributedSubstringWithRange:(ONSRange)range {
    return objc_retain([self attributedSubstringFromRange:range]);
}

@end

@implementation ONSAttributedString (NSAttributedStringPortCoding)

// Native -[NSAttributedString(NSAttributedStringPortCoding) replacementObjectForPortCoder:],
// 0x1c064e3a0. Non-external. Native RET preserves the receiver in X0; Hex-Rays prints a void function.
- (id)replacementObjectForPortCoder:(id)coder {
    (void)coder;
    return self;
}

@end

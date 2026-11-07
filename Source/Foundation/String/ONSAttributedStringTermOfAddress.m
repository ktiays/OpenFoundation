//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSAttributedStringTermOfAddress.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"

#import <objc/NSObject.h>

// Inferred organization: preferredTermOfAddressForLanguageFromList (0x1c05c4ba4) lies in Foundation's
// NSTermOfAddress unit, far from the inflection unit that calls it; the native unit is its own translation
// unit, so the function gets this file (the String/ONSAttributedString* path is this family's ownership). It
// is the only function of that unit in this family's scope. Objective-C compiled with manual reference
// counting. Evidence: Local/Evidence/cf-core/iphone-simulator-foundation-link.json (family
// FC-sets-attributed). iOS Simulator 24A434, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md.

// NSTermOfAddress (declared in the header) is referenced by its class symbol.
extern char termOfAddressClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSTermOfAddress));

// The messages the function sends to the terms of address, declared for the sends only; no protocol object
// is emitted.
@protocol TermOfAddressMessages

- (BOOL)_isSupportedInLanguage:(id)languageIdentifier;
- (id)_withExtrapolatedPronouns;

@end

// Native preferredTermOfAddressForLanguageFromList, 0x1c05c4ba4. Was a private external. The first element
// that is an NSTermOfAddress supporting the language and whose -_withExtrapolatedPronouns is not nil gives
// the result; nil when none does.
id preferredTermOfAddressForLanguageFromList(id languageIdentifier, id list) {
    for (id term in list) {
        if (objc_opt_isKindOfClass((id)(term), (id)(objc_opt_class((id)((Class)termOfAddressClass)))) &&
            [term _isSupportedInLanguage:languageIdentifier]) {
            id extrapolated = [term _withExtrapolatedPronouns];
            if (extrapolated != nil) {
                return extrapolated;
            }
        }
    }
    return nil;
}

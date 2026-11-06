//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Base/ONSPropertyListClasses.h"

#include <dispatch/dispatch.h>

// Inferred organization: __NSPropertyListClasses_ForArchival_ErrorAndException (0x1c05a6b0c) and its block
// (0x1c05a6b4c) lie together in the iOS Simulator __text section after the NSProcessInfo methods; the native
// source path is not in the evidence. iOS Simulator 24A434, arm64, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full); iOS Device 24A446:
// unread. Reading: Local/Scratch/cf-core-fd-misc/Reading.md; original assembly in
// Local/Evidence/cf-core/iphone-simulator-foundation-link.json (family FM-misc).

// Native once predicate qword_22e76c9c8 and set __MergedGlobals_105.
static dispatch_once_t propertyListClassesOnce;
static id propertyListClassesForErrorAndException;
extern Class objc_opt_class(id object);
extern char setClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSSet));
extern char dictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDictionary));
extern char arrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSArray));
extern char stringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSString));
extern char numberClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSNumber));
extern char dataClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSData));
extern char dateClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDate));
extern char urlClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSURL));

@protocol PropertyListClassMessages
- (id)initWithObjects:(id)first, ...;
@end

// Native __NSPropertyListClasses_ForArchival_ErrorAndException, 0x1c05a6b0c. External. The once block runs
// unless the predicate is already -1, then the set is returned.
id __ONSPropertyListClasses_ForArchival_ErrorAndException(void) {
    dispatch_once(&propertyListClassesOnce, ^{
      // Native _____NSPropertyListClasses_ForArchival_ErrorAndException_block_invoke, 0x1c05a6b4c.
      // Non-external. Uniqued GOT bindings (medium): set 0x22227fb68, dictionary 0x22227fad0,
      // array 0x22227fa90, data 0x22227fab8, date 0x22227fac0, URL 0x22227fb88.
      id allocated = [(id)setClass alloc];
      Class dictionary = objc_opt_class((id)dictionaryClass);
      Class array = objc_opt_class((id)arrayClass);
      Class string = objc_opt_class((id)stringClass);
      Class number = objc_opt_class((id)numberClass);
      Class data = objc_opt_class((id)dataClass);
      Class date = objc_opt_class((id)dateClass);
      Class url = objc_opt_class((id)urlClass);
      propertyListClassesForErrorAndException =
          [allocated initWithObjects:dictionary, array, string, number, data, date, url, nil];
    });
    return propertyListClassesForErrorAndException;
}

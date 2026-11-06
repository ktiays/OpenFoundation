//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Base/ONSFoundationBundle.h"

#include <dispatch/dispatch.h>
#include "CoreFoundation/String/OCFString.h"

// Inferred organization: _NSFoundationBundle (0x1c05a1480) lies among Foundation's platform helpers of the
// iOS Simulator __text section (its block, _NSFoundationResourceDirectory and _NSIsPList follow); the native
// source path is not in the evidence. iOS Simulator 24A434, arm64, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full); iOS Device 24A446:
// unread. Reading: Local/Scratch/cf-core-fd-misc/Reading.md; original evidence is the assembly in
// Local/Evidence/cf-core/iphone-simulator-foundation-link.json (family FM-misc).

// Native once predicate __MergedGlobals_102 and bundle qword_22e76c980. The invoke was independently read
// through IDA on 2026/11/03; Local/Scratch/cf-core-fd-misc/additional-ida.json retains the capture.
static dispatch_once_t foundationBundleOnce;
static id foundationBundle;
extern id objc_opt_self(id object);
extern char bundleClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSBundle));
extern char stringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSString));

@protocol FoundationBundleMessages
+ (id)bundleForClass:(Class)cls;
+ (id)mainBundle;
+ (id)bundleWithPath:(id)path;
- (id)stringByAppendingPathComponent:(id)path;
@end

// Native _NSFoundationBundle, 0x1c05a1480. External. The once block runs unless the predicate is already -1,
// then the bundle is returned.
id _ONSFoundationBundle(void) {
    dispatch_once(&foundationBundleOnce, ^{
      // Native ____NSFoundationBundle_block_invoke, 0x1c05a14c0. Non-external.
      id cls = objc_opt_self((id)stringClass);
      foundationBundle = [(id)bundleClass bundleForClass:(Class)cls];
      if (foundationBundle != [(id)bundleClass mainBundle]) {
          return;
      }
      foundationBundle =
          [[(id)bundleClass bundleWithPath:(id)OF_STR("/System/Frameworks/Foundation.framework")] retain];
      if (foundationBundle != nil && foundationBundle != [(id)bundleClass mainBundle]) {
          return;
      }
      id paths = ONSSearchPathForDirectoriesInDomains(5, 8, YES);
      for (id path in paths) {
          id candidate = [path stringByAppendingPathComponent:(id)OF_STR("Frameworks/Foundation.framework")];
          foundationBundle = [[(id)bundleClass bundleWithPath:candidate] retain];
          if (foundationBundle != nil) {
              return;
          }
      }
    });
    return foundationBundle;
}

//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/FileManager/ONSSearchPaths.h"
#import "Foundation/FileManager/ONSUnrestoredInterfaces.h"
#import "Foundation/Archiving/ONSArchivingPrivate.h"
#import "CoreFoundation/Objective-C/OCFNSArray.h"

#include <dispatch/dispatch.h>
#include <string.h>

extern id objc_opt_self(id object);
extern char searchPathFileManagerClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSFileManager));

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified. iOS Device 24A446: comparison pending.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FB-bundle-paths-plist.
// Uniqued GOT binding (medium): NSArray 0x22227fa90.

static dispatch_once_t directorySearchOnce;
static BOOL copySearchPathsForAdobe;

// Native NSSearchPathForDirectoriesInDomains, 0x1c05920b8. External.
// Hex-Rays reports failed variable allocation. The assembly's Adobe compatibility branch tail-calls
// initWithArray: and leaves the copy retained (kept).
id ONSSearchPathForDirectoriesInDomains(NSUInteger directory, NSUInteger domains, BOOL expandTilde) {
    id paths = _NSSearchPathsForDirectoryInDomain(directory, domains, expandTilde);
    dispatch_once(&directorySearchOnce, ^{
      // Native ___NSSearchPathForDirectoriesInDomains_block_invoke, 0x1c0592134. Non-external.
      copySearchPathsForAdobe = !dyld_program_sdk_at_least((OCFBuildVersion){UINT32_MAX, 0x07e80400}) &&
                                _OCFAppVersionCheck(OF_STR("com.adobe.AdobeAcroCEF"), NULL);
    });
    if (copySearchPathsForAdobe) {
        return [[ONSArray alloc] initWithArray:paths];
    }
    objc_autorelease(objc_retain(paths));
    return paths;
}

// Objective-C calls inlined into the Swift specialization at 0x1c08d9e1c. Keep the dynamic sends and
// ownership in Objective-C so the Swift iterator can use the same filesystem-string bridge in both modes.
// Foundation divergence: no such native function. Natively the Swift code converts the path with
// String._bridgeToObjectiveC() and messages the file manager itself; the Swift String to NSString bridge
// is not restored, so the conversion and the sends stay in this Objective-C helper until it is.
const void *copySearchPathFileSystemString(const char *path) {
    id cls = objc_opt_self((id)searchPathFileManagerClass);
    id manager = objc_retain([(Class)cls defaultManager]);
    id string = objc_retain([manager stringWithFileSystemRepresentation:path length:strlen(path)]);
    objc_release(manager);
    return string;
}

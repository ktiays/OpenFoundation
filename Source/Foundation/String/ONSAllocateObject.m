//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSAllocateObject.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#import "CoreFoundation/Objective-C/OCFNSException.h"
#import "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/Runtime/OCFRuntime.h"

#import "Foundation/Base/ONSInitializePlatform.h"

#include <objc/objc-exception.h>
#include <objc/runtime.h>
#include <stdio.h>
#include <stdlib.h>

// Inferred organization: NSAllocateObject (0x1c0582f54) and NSDeallocateObject (0x1c0582ff8) lie in the
// Foundation image's NSObject functions; following the FS-string ownership they have their own unit here.
// Objective-C compiled with manual reference counting. iOS Simulator 24A434, arm64, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full,
// iphone-simulator-foundation-link.json) except the GOT bindings below. iOS Device divergence: iOS Device
// 24A446 remains unread. Reading: Foundation/String/ONSStringReview.md.

// Native NSAllocateObject, 0x1c0582f54. External. class_createInstance(aClass, extraBytes); the zone is not
// read. When the byte behind GOT slot 0x2222806D8 (bound as CoreFoundation's __CFOASafe, confidence: medium)
// equals 1, the allocation event is named after object_getClassName. A failed allocation raises the exception
// named by GOT slot 0x22227F5D8 (bound as NSMallocException, confidence: medium) from the class behind
// 0x22227FAE0 (NSException, confidence: medium).
id ONSAllocateObject(Class aClass, ONSUInteger extraBytes, struct _NSZone *zone) {
    (void)zone;
    id object = class_createInstance(aClass, extraBytes);
    if (object == nil) {
        id reason = [ONSString
            stringWithFormat:(id)OF_STR(
                                 "*** NSAllocateObject(): attempt to allocate object of class '%s' failed"
                             ),
                             class_getName(aClass)];
        objc_exception_throw([ONSException exceptionWithName:(id)ONSMallocException
                                                      reason:reason
                                                    userInfo:nil]);
    }
    if (__OCFOASafe == 1) {
        __OCFSetLastAllocationEventName(object, object_getClassName(object));
    }
    return object;
}

// Native NSDeallocateObject, 0x1c0582ff8. External. With NSZombieEnabled the object becomes an instance of
// _NSZombie_<class> (duplicated from _NSZombie_ on first use; the class names are the runtime's own,
// unprefixed, as in CoreFoundation's zombie support): disposed under NSDeallocateZombies, otherwise
// destructed and kept. Without zombies the object is disposed.
void ONSDeallocateObject(id object) {
    if (object == nil) {
        return;
    }
    if (ONSZombieEnabled == 1) {
        Class cls = object_getClass(object);
        char *name = NULL;
        asprintf(&name, "_NSZombie_%s", class_getName(cls));
        Class zombie = objc_lookUpClass(name);
        if (zombie == Nil) {
            zombie = objc_duplicateClass(objc_lookUpClass("_NSZombie_"), name, 0);
        }
        free(name);
        if (ONSDeallocateZombies == 1) {
            object_setClass(object, zombie);
            object_dispose(object);
        } else {
            objc_destructInstance(object);
            object_setClass(object, zombie);
        }
    } else {
        object_dispose(object);
    }
}

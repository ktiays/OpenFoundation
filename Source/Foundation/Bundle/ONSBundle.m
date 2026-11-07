//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/Bundle/ONSBundle.h"
#import "Foundation/Bundle/ONSBundleTables.h"
#import "Foundation/Bundle/ONSBundleDidLoadNotificationPayloadArray.h"
#import "Foundation/Bundle/ONSUnrestoredInterfaces.h"
#import "Foundation/Archiving/ONSArchivingPrivate.h"
#import "Foundation/Archiving/ONSObjCRuntimeNames.h"
#import "Foundation/Error/ONSError.h"
#import "Foundation/Notification/ONSNotification.h"
#import "Foundation/String/ONSFileUtilities.h"
#import "Foundation/String/ONSLog.h"
#import "Foundation/String/ONSPathUtilities.h"
#import "Foundation/String/ONSRuntimeSupport.h"
#import "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#import "CoreFoundation/Objective-C/OCFNSDictionary.h"
#include "CoreFoundation/Base/OCFPlatform.h"
#include "CoreFoundation/PlugIn/OCFBundleExecutable.h"
#include "CoreFoundation/PlugIn/OCFBundleLocalization.h"
#include "CoreFoundation/PlugIn/OCFBundleMain.h"
#include "CoreFoundation/PlugIn/OCFBundleResources.h"
#include "CoreFoundation/PlugIn/OCFBundleStrings.h"

#include <assert.h>
#include <malloc/malloc.h>
#include <unistd.h>
#include <objc/objc-exception.h>
#include <sys/stat.h>
#include <string.h>

// Native NSBundle.m (assertion at 0x1c04cf9b8), iOS Simulator 24A434, arm64, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified. iOS Device 24A446: comparison pending.
// Reading: Local/Evidence/cf-core/iphone-simulator-foundation-link-delta1.json, FB-bundle-paths-plist.
// GOT bindings (medium): NSMutableArray 0x22227fb18, NSDictionary 0x22227fad0, NSURL 0x22227fb88,
// and kCFBundleIdentifierKey 0x222280ba8.

extern struct mach_header __dso_handle;
extern id objc_opt_self(id object);
extern const char *dyld_image_path_containing_address(const void *address);
extern void
_dyld_register_for_bulk_image_loads(void (*callback)(unsigned int, const struct mach_header *const[]));
extern char nativeBundleClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSBundle));
extern char bundleURLClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSURL));

static Class _NSBundleGetPrincipalClassFromInfoDict(ONSBundle *bundle);
static void _NSBundleInitializePrincipalClass(Class principalClass);
static void _NSBundleImagesLoadedCallback(unsigned int count, const struct mach_header *const headers[]);
static id normalizedPath(id path);

static os_unfair_lock mainBundleLock;
static ONSBundle *mainBundle;
static _Atomic(bool) emitBundleDidLoadNotifications;
// Native __MergedGlobals.21 + 1. The setter NSBundleDisableCodeLoading is outside the declared scope.
static volatile BOOL bundleLoadingDisabled;
static dispatch_once_t bundleLoadCodeOnce;

typedef struct __attribute__((packed)) {
    uint32_t header;
    id object;
} BundleLogObject;

typedef struct __attribute__((packed)) {
    uint32_t header;
    id first;
    uint16_t secondHeader;
    id second;
} BundleLogTwoObjects;

typedef struct __attribute__((packed)) {
    uint32_t header;
    id first;
    uint16_t secondHeader;
    id second;
    uint16_t thirdHeader;
    id third;
} BundleLogThreeObjects;

@interface ONSBundle ()
- (OCFBundleRef)_cfBundle;
- (OCFBundleRef)_cfBundleIfPresent;
- (BOOL)loadAndReturnError:(id *)error;
- (BOOL)unload;
- (BOOL)isLoaded;
- (id)_initWithPath:(id)path unique:(BOOL)unique options:(uint64_t)options;
- (id)bundlePath;
- (id)infoDictionary;
- (id)localizedInfoDictionary;
- (id)bundleIdentifier;
+ (id)loadedBundles;
+ (id)preferredLocalizationsFromArray:(id)localizations;
- (id)localizations;
@end

// Native __NSInitBundleDidLoadNotification, 0x1c04cd304. Non-external.
// +[NSNotificationCenter load] registers this callback through __nsDependentNotifications.
// The coordinator can fold it into the registration unit once both families are integrated.
void _NSInitBundleDidLoadNotification(void) {
    atomic_store_explicit(&emitBundleDidLoadNotifications, true, memory_order_release);
}

@implementation ONSBundle

// Native -[NSBundle _cfBundle], 0x1c04cd754. Non-external.
- (OCFBundleRef)_cfBundle {
    if (atomic_load_explicit(&_cfBundle, memory_order_acquire) == NULL && _resolvedPath != nil) {
        OCFURLRef url =
            OCFURLCreateWithFileSystemPath(NULL, (OCFStringRef)_resolvedPath, kOCFURLPOSIXPathStyle, true);
        OCFBundleRef bundle = OCFBundleCreate(NULL, url);
        if (bundle != NULL) {
            OCFBundleRef expected = NULL;
            if (!atomic_compare_exchange_strong(&_cfBundle, &expected, bundle)) {
                OCFRelease(bundle);
            }
        }
        OCFRelease(url);
    }
    return atomic_load_explicit(&_cfBundle, memory_order_acquire);
}

// Native -[NSBundle _cfBundleIfPresent], 0x1c04cd7d4. Non-external.
- (OCFBundleRef)_cfBundleIfPresent {
    return atomic_load_explicit(&_cfBundle, memory_order_acquire);
}

// Native -[NSBundle URLForResource:withExtension:subdirectory:localization:], 0x1c04cdf00. Non-external.
- (id)URLForResource:(id)name
       withExtension:(id)extension
        subdirectory:(id)subpath
        localization:(id)localization {
    if ([name length] == 0 && [extension length] == 0) {
        return nil;
    }
    OCFBundleRef bundle = [self _cfBundle];
    return objc_autorelease(
        bundle == NULL ? nil
                       : (id)OCFBundleCopyResourceURLForLocalization(
                             bundle,
                             (OCFStringRef)name,
                             (OCFStringRef)extension,
                             (OCFStringRef)subpath,
                             (OCFStringRef)localization
                         )
    );
}

// Native -[NSBundle URLForResource:withExtension:subdirectory:], 0x1c04cdfa8. Non-external.
- (id)URLForResource:(id)name withExtension:(id)extension subdirectory:(id)subpath {
    if ([name length] == 0 && [extension length] == 0) {
        return nil;
    }
    OCFBundleRef bundle = [self _cfBundle];
    return objc_autorelease(
        bundle == NULL ? nil
                       : (id)OCFBundleCopyResourceURL(
                             bundle, (OCFStringRef)name, (OCFStringRef)extension, (OCFStringRef)subpath
                         )
    );
}

// Native -[NSBundle URLForResource:withExtension:], 0x1c04ce07c. Non-external.
- (id)URLForResource:(id)name withExtension:(id)extension {
    return [self URLForResource:name withExtension:extension subdirectory:nil];
}

// Native -[NSBundle URLsForResourcesWithExtension:subdirectory:], 0x1c04ce210. Non-external.
- (id)URLsForResourcesWithExtension:(id)extension subdirectory:(id)subpath {
    OCFBundleRef bundle = [self _cfBundle];
    return objc_autorelease(
        bundle == NULL
            ? nil
            : (id)OCFBundleCopyResourceURLsOfType(bundle, (OCFStringRef)extension, (OCFStringRef)subpath)
    );
}

// Native -[NSBundle localizedStringForKey:value:table:], 0x1c04ce244. Non-external.
- (id)localizedStringForKey:(id)key value:(id)value table:(id)table {
    OCFBundleRef bundle = [self _cfBundle];
    return objc_autorelease(
        bundle == NULL ? nil
                       : (id)OCFBundleCopyLocalizedString(
                             bundle, (OCFStringRef)key, (OCFStringRef)value, (OCFStringRef)table
                         )
    );
}

// Native -[NSBundle localizedStringForKey:value:table:localization:], 0x1c04ce288. Non-external.
- (id)localizedStringForKey:(id)key value:(id)value table:(id)table localization:(id)localization {
    OCFBundleRef bundle = [self _cfBundle];
    return objc_autorelease(
        bundle == NULL ? nil
                       : (id)OCFBundleCopyLocalizedStringForLocalization(
                             bundle,
                             (OCFStringRef)key,
                             (OCFStringRef)value,
                             (OCFStringRef)table,
                             (OCFStringRef)localization
                         )
    );
}

// Native -[NSBundle localizedStringsForTable:localization:], 0x1c04ce36c. Non-external.
- (id)localizedStringsForTable:(id)table localization:(id)localization {
    OCFBundleRef bundle = [self _cfBundle];
    return objc_autorelease(
        bundle == NULL ? nil
                       : (id)OCFBundleCopyLocalizedStringTableForLocalization(
                             bundle, (OCFStringRef)table, (OCFStringRef)localization
                         )
    );
}

// Native __NSFrameworkPathFromLibraryPath, 0x1c04ce4b4. Was a private external.
// The sandbox check receives the filesystem representation on the stack, omitted by Hex-Rays.
id _NSFrameworkPathFromLibraryPath(id path) {
    if (path == nil) {
        return nil;
    }
    id directory = [[path _stringByResolvingSymlinksInPathUsingCache:YES] stringByDeletingLastPathComponent];
    id extension = [directory pathExtension];
    if ([extension compare:(id)OF_STR("framework") options:0 range:(ONSRange){0, [extension length]}] != 0) {
        id parent = [directory stringByDeletingLastPathComponent];
        id last = [parent lastPathComponent];
        if ([last compare:(id)OF_STR("Versions") options:0 range:(ONSRange){0, [last length]}] != 0) {
            return nil;
        }
        directory = [parent stringByDeletingLastPathComponent];
        extension = [directory pathExtension];
        if ([extension compare:(id)OF_STR("framework") options:0
                         range:(ONSRange){0, [extension length]}] != 0 &&
            [extension compare:(id)OF_STR("axbundle") options:0
                         range:(ONSRange){0, [extension length]}] != 0) {
            return nil;
        }
    }
    struct stat status = {0};
    char representation[1026];
    if (![directory getFileSystemRepresentation:representation maxLength:1024] ||
        stat(representation, &status) != 0) {
        return nil;
    }
    if (access(representation, R_OK) != 0) {
        sandbox_check(getpid(), "file-read-data", 1, representation);
        return nil;
    }
    return S_ISDIR(status.st_mode) ? directory : nil;
}

// Native __NSBundlePathFromExecutablePath, 0x1c04ce664. Was a private external.
// The sandbox check receives the filesystem representation on the stack, omitted by Hex-Rays.
id _NSBundlePathFromExecutablePath(id path) {
    id directory = [[path _stringByResolvingSymlinksInPathUsingCache:YES] stringByDeletingLastPathComponent];
    id parent = [directory stringByDeletingLastPathComponent];
    id last = [parent lastPathComponent];
    BOOL hasContainer = [last compare:(id)OF_STR("Contents") options:0
                                range:(ONSRange){0, [last length]}] == 0;
    if (!hasContainer && [last compare:(id)OF_STR("Executables")
                               options:0
                                 range:(ONSRange){0, [last length]}] == 0) {
        parent = [parent stringByDeletingLastPathComponent];
        last = [parent lastPathComponent];
        hasContainer = [last compare:(id)OF_STR("Support Files")
                             options:0
                               range:(ONSRange){0, [last length]}] == 0;
    }
    if (hasContainer) {
        id candidate = [parent stringByDeletingLastPathComponent];
        struct stat status = {0};
        char representation[1026];
        if ([candidate getFileSystemRepresentation:representation maxLength:1024] &&
            stat(representation, &status) == 0) {
            if (access(representation, R_OK) != 0) {
                sandbox_check(getpid(), "file-read-data", 1, representation);
            } else if (S_ISDIR(status.st_mode)) {
                return candidate;
            }
        }
    }
    return directory;
}

// Native -[NSBundle load], 0x1c04ce7f4. Non-external.
- (BOOL)load {
    return [self loadAndReturnError:NULL];
}

// Native -[NSBundle loadAndReturnError:], 0x1c04ce7fc. Non-external.
// The first image header is passed to dyld_image_path_containing_address in X0. Hex-Rays omits X0.
- (BOOL)loadAndReturnError:(id *)error {
    os_unfair_lock_lock(&_lock);
    if ((atomic_load_explicit(&_flags, memory_order_acquire) & 0x4000000) != 0) {
        os_unfair_lock_unlock(&_lock);
        return YES;
    }

    id notifications = atomic_load_explicit(&emitBundleDidLoadNotifications, memory_order_acquire)
                           ? [ONSMutableArray array]
                           : nil;
    void *pool = objc_autoreleasePoolPush();
    (void)atomic_load_explicit(&_flags, memory_order_acquire);
    OCFBundleRef bundle = [self _cfBundle];
    OCFErrorRef loadError = NULL;
    BOOL loaded = NO;
    if (bundle == NULL) {
        os_log_t log = _NSOSLog();
        if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
            BundleLogObject payload = {138412290, [self bundleURL]};
            _os_log_error_impl(
                &__dso_handle,
                log,
                OS_LOG_TYPE_ERROR,
                "NSBundle %@ loading failed because the file does not exist",
                (uint8_t *)&payload,
                sizeof(payload)
            );
        }
        if (error != NULL) {
            *error = [[ONSError alloc] initWithDomain:ONSCocoaErrorDomain code:4 userInfo:nil];
        }
    }
    if (bundleLoadingDisabled) {
        os_log_t log = _NSOSLog();
        if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
            BundleLogObject payload = {138412290, [self bundleURL]};
            _os_log_error_impl(
                &__dso_handle,
                log,
                OS_LOG_TYPE_ERROR,
                "NSBundle %@ loading failed because bundle loading is disabled",
                (uint8_t *)&payload,
                sizeof(payload)
            );
        }
        if (error != NULL) {
            *error = (id)_OCFBundleCreateError(OCFGetAllocator(bundle), bundle, 3584);
        }
    } else if (bundle != NULL) {
        dispatch_once(&bundleLoadCodeOnce, ^{
          // Native ____NSBundleLoadCode_block_invoke, 0x1c04d1018. Non-external.
          _dyld_register_for_bulk_image_loads(_NSBundleImagesLoadedCallback);
        });
        _OCFSetTSD(37, (void *)1, NULL);
        loaded = _OCFBundleLoadExecutableAndReturnError(bundle, true, error != NULL ? &loadError : NULL);
        if (loaded) {
            const struct mach_header **headers = _OCFGetTSD(37);
            if ((uintptr_t)headers >= 2) {
                for (const struct mach_header **cursor = headers; *cursor != NULL; ++cursor) {
                    const struct mach_header *header = *cursor;
                    const char *name = dyld_image_path_containing_address(header);
                    id nameString = _NSFileSystemRepresentationString(name, strlen(name));
                    id path = _NSFrameworkPathFromLibraryPath(nameString);
                    if (path == nil) {
                        path = _NSBundlePathFromExecutablePath(nameString);
                    }
                    if (path == nil) {
                        continue;
                    }
                    ONSBundle *loadedBundle = objc_autorelease([[ONSBundle alloc] initWithPath:path]);
                    if (loadedBundle == nil) {
                        continue;
                    }
                    if ((atomic_load_explicit(&loadedBundle->_flags, memory_order_acquire) & 0xf0000) ==
                        0x40000) {
                        atomic_fetch_or(&loadedBundle->_flags, 0x4000000);
                    }
                    if (loadedBundle != self) {
                        os_unfair_lock_lock(&loadedBundle->_lock);
                    }
                    if (loadedBundle->_firstClassName == nil) {
                        // This byref variable emits ___Block_byref_object_copy__0 (0x1c0fdc1dc)
                        // and ___Block_byref_object_dispose__0 (0x1c0fdc1ec), with flags 131.
                        __block id firstClassName = nil;
                        objc_enumerateClasses(header, NULL, nil, Nil, ^(Class cls, BOOL *stop) {
                          // Native ____NSBundleAddClassesInImage_block_invoke, 0x1c04d10a8. Non-external.
                          firstClassName = objc_retain(ONSStringFromClass(cls));
                          *stop = YES;
                        });
                        loadedBundle->_firstClassName = firstClassName;
                    }
                    if (loadedBundle != self) {
                        os_unfair_lock_unlock(&loadedBundle->_lock);
                    }
                    if (notifications != nil) {
                        id classes =
                            [[_ONSBundleDidLoadNotificationPayloadArray alloc] initWithHeader:header];
                        id key = (id)OF_STR("NSLoadedClasses");
                        id info = [ONSDictionary dictionaryWithObjects:&classes forKeys:&key count:1];
                        [notifications
                            addObject:[ONSNotification
                                          notificationWithName:(id)OF_STR("NSBundleDidLoadNotification")
                                                        object:loadedBundle
                                                      userInfo:info]];
                        objc_release(classes);
                    }
                }
            }
        } else if (error != NULL) {
            *error = (id)loadError;
            os_log_t log = _NSOSLog();
            if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
                BundleLogTwoObjects payload = {138412546, [self bundleURL], 2112, *error};
                _os_log_error_impl(
                    &__dso_handle,
                    log,
                    OS_LOG_TYPE_ERROR,
                    "NSBundle %@ loading failed because of an error %@",
                    (uint8_t *)&payload,
                    sizeof(payload)
                );
            }
        }

        void *headers = _OCFGetTSD(37);
        _OCFSetTSD(37, NULL, NULL);
        if ((uintptr_t)headers >= 2) {
            free(headers);
        }
    }

    if (loaded) {
        atomic_fetch_or(&_flags, 0x4000000);
    }
    objc_autoreleasePoolPop(pool);
    if (!loaded && error != NULL) {
        objc_autorelease(*error);
    }
    Class principal = _NSBundleGetPrincipalClassFromInfoDict(self);
    os_unfair_lock_unlock(&_lock);
    _NSBundleInitializePrincipalClass(principal);
    if (loaded && [notifications count] != 0) {
        id center = [ONSNotificationCenter defaultCenter];
        for (id notification in notifications) {
            [center postNotification:notification];
        }
    }
    return loaded;
}

// Native __NSBundleGetPrincipalClassFromInfoDict, 0x1c04ceec8. Non-external.
static Class _NSBundleGetPrincipalClassFromInfoDict(ONSBundle *bundle) {
    if ((atomic_load_explicit(&bundle->_flags, memory_order_acquire) & 0x4000000) == 0) {
        os_log_t log = _NSOSLog();
        if (os_log_type_enabled(log, OS_LOG_TYPE_DEBUG)) {
            BundleLogObject payload = {138412290, [bundle bundleURL]};
            _os_log_debug_impl(
                &__dso_handle,
                log,
                OS_LOG_TYPE_DEBUG,
                "NSBundle %@ principal class is nil because the bundle is not loaded",
                (uint8_t *)&payload,
                sizeof(payload)
            );
        }
        return Nil;
    }

    id info = [bundle infoDictionary];
    id key = (id)OF_STR("NSPrincipalClass");
    id name = [info objectForKey:key];
    id firstClassName = bundle->_firstClassName;
    if (name != nil) {
        Class principal = ONSClassFromString(name);
        if (principal != Nil) {
            return principal;
        }
        if (firstClassName != nil) {
            os_log_t log = _NSOSLog();
            if (os_log_type_enabled(log, OS_LOG_TYPE_DEBUG)) {
                BundleLogThreeObjects payload = {
                    138412802, [bundle bundleURL], 2112, firstClassName, 2112, name
                };
                _os_log_debug_impl(
                    &__dso_handle,
                    log,
                    OS_LOG_TYPE_DEBUG,
                    "NSBundle %@ principal class is using first class name %@ because the "
                    "bundle's principal class name %@ is not present",
                    (uint8_t *)&payload,
                    sizeof(payload)
                );
            }
        }
    } else if (firstClassName != nil) {
        os_log_t log = _NSOSLog();
        if (os_log_type_enabled(log, OS_LOG_TYPE_DEBUG)) {
            BundleLogTwoObjects payload = {138412546, [bundle bundleURL], 2112, firstClassName};
            _os_log_debug_impl(
                &__dso_handle,
                log,
                OS_LOG_TYPE_DEBUG,
                "NSBundle %@ principal class is using first class name %@ because the bundle "
                "has no principal class name",
                (uint8_t *)&payload,
                sizeof(payload)
            );
        }
    } else {
        id path = [[bundle bundlePath] _stringByResolvingSymlinksInPathUsingCache:NO];
        ONSBundle *existing = [(__ONSBundleTables *)[__ONSBundleTables bundleTables] bundleForPath:path];
        if (existing != nil && existing != bundle) {
            os_unfair_lock_lock(&existing->_lock);
            id fallback = objc_retain(existing->_firstClassName);
            os_unfair_lock_unlock(&existing->_lock);
            if (fallback != nil) {
                [info setObject:fallback forKey:key];
                Class principal = ONSClassFromString(fallback);
                objc_release(fallback);
                return principal;
            }
        }
    }
    if (firstClassName == nil) {
        os_log_t log = _NSOSLog();
        if (os_log_type_enabled(log, OS_LOG_TYPE_DEBUG)) {
            BundleLogObject payload = {138412290, [bundle bundleURL]};
            _os_log_debug_impl(
                &__dso_handle,
                log,
                OS_LOG_TYPE_DEBUG,
                "NSBundle %@ principal class is nil because all fallbacks have failed",
                (uint8_t *)&payload,
                sizeof(payload)
            );
        }
        return Nil;
    }
    [info setObject:firstClassName forKey:key];
    return ONSClassFromString(firstClassName);
}

// Native __NSBundleInitializePrincipalClass, 0x1c04cf13c. Non-external.
// Selector "self" read at shared-cache address 0x2387aa2c2. Reading: ida-bundle-selectors.json.
static void _NSBundleInitializePrincipalClass(Class principalClass) {
    if (principalClass != Nil && class_respondsToSelector(object_getClass(principalClass), @selector(self))) {
        objc_opt_self(principalClass);
    }
}

// Native -[NSBundle unload], 0x1c04cf180. Non-external.
- (BOOL)unload {
    OCFBundleRef bundle = [self _cfBundleIfPresent];
    if (bundle == NULL) {
        return NO;
    }
    OCFBundleUnloadExecutable(bundle);
    if (OCFBundleIsExecutableLoaded(bundle)) {
        return NO;
    }
    if ((atomic_load_explicit(&_flags, memory_order_acquire) & 0x4000000) != 0) {
        atomic_fetch_and(&_flags, UINT64_C(0xfffffffff9ffffff));
    }
    return YES;
}

// Native -[NSBundle isLoaded], 0x1c04cf1d8. Non-external.
- (BOOL)isLoaded {
    if ((atomic_load_explicit(&_flags, memory_order_acquire) & 0x4000000) != 0) {
        return YES;
    }
    OCFBundleRef bundle = [self _cfBundleIfPresent];
    return bundle != NULL && OCFBundleIsExecutableLoaded(bundle);
}

// Native __NSBundleDeallocatingImmortalBundle, 0x1c04cf210. Was a private external.
// Debugger breakpoint hook. The entire native body is RET.
__attribute__((noinline)) void _NSBundleDeallocatingImmortalBundle(void) {
    __asm__ volatile("");
}

// Native -[NSBundle dealloc], 0x1c04cf214. Non-external.
- (void)dealloc {
    if ((atomic_load_explicit(&_flags, memory_order_acquire) & 0x4000000) != 0) {
        [self unload];
    }
    BOOL isStatic = (atomic_load_explicit(&_flags, memory_order_acquire) & 0xd0000) == 0x10000;
    BOOL isImmortal = !isStatic && (atomic_load_explicit(&_flags, memory_order_acquire) & 8) != 0;
    if (isStatic || isImmortal) {
        os_log_t log = _NSOSLog();
        if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
            struct __attribute__((packed)) {
                uint32_t header;
                id proem;
                uint16_t addressHeader;
                uintptr_t address;
                uint16_t identifierHeader;
                id identifier;
                uint16_t urlHeader;
                id url;
            } payload = {
                138413058,
                _ONSMethodExceptionProem(self, _cmd),
                2048,
                (uintptr_t)self,
                2112,
                [self bundleIdentifier],
                2112,
                [self bundleURL]
            };

            const char *message =
                isStatic ? "%@: attempt to deallocate static bundle - break on "
                           "_NSBundleDeallocatingStaticBundle to debug. "
                           "This bundle %p would have been overreleased, but will instead be preserved.\n"
                           "\tBundle Identifier: %@\n\tBundle URL: %@"
                         : "%@: attempting to deallocate an immortal bundle - break on "
                           "_NSBundleDeallocatingImmortalBundle "
                           "to debug. This bundle %p has been overreleased.\n\tBundle Identifier: "
                           "%@\n\tBundle URL: %@";
            _os_log_error_impl(
                &__dso_handle, log, OS_LOG_TYPE_ERROR, message, (uint8_t *)&payload, sizeof(payload)
            );
        }
        _NSBundleDeallocatingImmortalBundle();
        if (ONSZombieEnabled) {
            __builtin_arm_trap(1);
        }
        if (isStatic) {
            return;
        }
    }

    if (atomic_load_explicit(&_cfBundle, memory_order_acquire) != NULL) {
        objc_release((id)atomic_load_explicit(&_cfBundle, memory_order_acquire));
    }
    objc_release(_resolvedPath);
    objc_release(_initialPath);
    objc_release(_firstClassName);
    if (_attributedStringTable != nil) {
        objc_release(_attributedStringTable);
    }
    [super dealloc];
}

// Native +[NSBundle bundleWithPath:], 0x1c04cf450. Non-external.
+ (id)bundleWithPath:(id)path {
    return objc_autorelease([objc_allocWithZone((Class)self) initWithPath:path]);
}

// Native -[NSBundle initWithPath:], 0x1c04cf6f0. Non-external.
- (id)initWithPath:(id)path {
    return [self _initWithPath:path unique:NO options:0];
}

// Native -[NSBundle _initWithPath:unique:options:], 0x1c04cf6fc. Non-external.
- (id)_initWithPath:(id)path unique:(BOOL)unique options:(uint64_t)options {
    id normalized = normalizedPath(path);
    if (!unique) {
        id cached = [(__ONSBundleTables *)[__ONSBundleTables bundleTables] bundleForPath:normalized];
        if (cached != nil) {
            objc_release(self);
            return objc_retain(cached);
        }
    }

    id resolved = [normalized _stringByResolvingSymlinksInPathUsingCache:!unique];
    if (resolved == nil || [resolved isEqual:(id)OF_STR("")]) {
        os_log_t log = _NSOSLog();
        if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
            BundleLogObject payload = {138412290, path};
            _os_log_error_impl(
                &__dso_handle,
                log,
                OS_LOG_TYPE_ERROR,
                "NSBundle %@ initWithPath failed because the resolved path is empty or nil",
                (uint8_t *)&payload,
                sizeof(payload)
            );
        }
        objc_release(self);
        return nil;
    }

    if (!unique) {
        id cached = [(__ONSBundleTables *)[__ONSBundleTables bundleTables] bundleForPath:resolved];
        if (cached != nil) {
            objc_release(self);
            return objc_retain(cached);
        }
        struct stat status = {0};
        char representation[1026];
        if (![resolved getFileSystemRepresentation:representation maxLength:1024] ||
            stat(representation, &status) != 0) {
            objc_release(self);
            return objc_retain(nil);
        }
        if (access(representation, R_OK) != 0) {
            sandbox_check(getpid(), "file-read-data", 1, representation);
            objc_release(self);
            return objc_retain(nil);
        }
        if (!S_ISDIR(status.st_mode)) {
            objc_release(self);
            return objc_retain(nil);
        }
    }

    atomic_fetch_or(&_flags, [resolved hasSuffix:(id)OF_STR(".framework")] ? 0x40000 : 0x20000);
    _initialPath = [normalized copy];
    _resolvedPath = [resolved copy];
    _lock = OS_UNFAIR_LOCK_INIT;
    if (unique) {
        OCFURLRef url =
            OCFURLCreateWithFileSystemPath(NULL, (OCFStringRef)_resolvedPath, kOCFURLPOSIXPathStyle, true);
        atomic_store(&_cfBundle, _OCFBundleCreateUniqueWithOptions(NULL, url, NULL, options));
        OCFRelease(url);
        return self;
    }

    if (options != 0) {
        __assert_rtn("-[NSBundle _initWithPath:unique:options:]", "NSBundle.m", 1180, "options == 0");
    }
    __ONSBundleTables *tables = [__ONSBundleTables bundleTables];
    uint64_t type = atomic_load_explicit(&_flags, memory_order_acquire) & 0xf0000;
    id existing = [tables addBundleIfNeeded:self forPath:resolved withType:type isImmortal:YES];
    if (existing == nil) {
        atomic_fetch_or(&_flags, 8);
        return self;
    }
    objc_release(self);
    return objc_retain(existing);
}

// Native _normalizedPath, 0x1c04cf9f4. Non-external.
static id normalizedPath(id path) {
    id prefix = (id)OF_STR("/private/preboot/Cryptexes/OS/");
    if ([path hasPrefix:prefix]) {
        return [path substringFromIndex:[prefix length] - 1];
    }
    prefix = (id)OF_STR("/System/Cryptexes/OS/");
    if ([path hasPrefix:prefix]) {
        return [path substringFromIndex:[prefix length] - 1];
    }
    return path;
}

// Native -[NSBundle bundlePath], 0x1c04cfa74. Non-external.
- (id)bundlePath {
    if (!_OCFExecutableLinkedOnOrAfter(7)) {
        id initial = objc_autorelease([_initialPath copy]);
        if (initial != nil) {
            return initial;
        }
    }

    OCFBundleRef bundle = [self _cfBundle];
    if (bundle != NULL) {
        OCFURLRef url = OCFBundleCopyBundleURL(bundle);
        if (url != NULL) {
            id path = [(id)url path];
            OCFRelease(url);
            return path;
        }
    }
    id fallback = objc_autorelease([_resolvedPath copy]);
    os_log_t log = _NSOSLog();
    if (os_log_type_enabled(log, OS_LOG_TYPE_ERROR)) {
        struct __attribute__((packed)) {
            uint32_t header;
            uintptr_t bundle;
            uint16_t pathHeader;
            id path;
        } payload = {134218242, (uintptr_t)self, 2112, fallback};

        _os_log_error_impl(
            &__dso_handle,
            log,
            OS_LOG_TYPE_ERROR,
            "This bundle %p does not have a valid path. Using backstop: %@",
            (uint8_t *)&payload,
            sizeof(payload)
        );
    }
    return fallback;
}

// Native -[NSBundle bundleURL], 0x1c04cfb5c. Non-external.
- (id)bundleURL {
    if (!_OCFExecutableLinkedOnOrAfter(7)) {
        id initial = objc_autorelease([_initialPath copy]);
        if (initial != nil) {
            return objc_autorelease([(id<BundleClassMessages>)objc_allocWithZone((Class)bundleURLClass)
                initFileURLWithPath:initial
                        isDirectory:YES]);
        }
    }
    OCFBundleRef bundle = [self _cfBundle];
    return bundle == NULL ? nil : objc_autorelease((id)OCFBundleCopyBundleURL(bundle));
}

// Native -[NSBundle resourceURL], 0x1c04cfc18. Non-external.
- (id)resourceURL {
    return objc_autorelease((id)OCFBundleCopyResourcesDirectoryURL([self _cfBundle]));
}

// Native -[NSBundle executableURL], 0x1c04cfc7c. Non-external.
- (id)executableURL {
    return objc_autorelease((id)OCFBundleCopyExecutableURL([self _cfBundle]));
}

// Native -[NSBundle URLForAuxiliaryExecutable:], 0x1c04cfda8. Non-external.
- (id)URLForAuxiliaryExecutable:(id)name {
    OCFBundleRef bundle = [self _cfBundle];
    return objc_autorelease(
        bundle == NULL ? nil : (id)OCFBundleCopyAuxiliaryExecutableURL(bundle, (OCFStringRef)name)
    );
}

// Native -[NSBundle bundleIdentifier], 0x1c04cff74. Non-external.
- (id)bundleIdentifier {
    id identifier = [[self infoDictionary] objectForKey:(id)kOCFBundleIdentifierKey];
    return _ONSIsNSString(identifier) ? identifier : nil;
}

// Native -[NSBundle versionNumber], 0x1c04cffb0. Non-external.
- (uint32_t)versionNumber {
    OCFBundleRef bundle = [self _cfBundle];
    return bundle == NULL ? 0 : OCFBundleGetVersionNumber(bundle);
}

// Native -[NSBundle classNamed:], 0x1c04d0108. Non-external.
- (Class)classNamed:(id)className {
    [self load];
    Class cls = ONSClassFromString(className);
    return [(Class)objc_opt_class(self) bundleForClass:cls] == self ? cls : Nil;
}

// Native +[NSBundle bundleForClass:], 0x1c04d01ac. Non-external.
// Selector bundleForClass (no colon) read at shared-cache address 0x2387c29a1.
+ (id)bundleForClass:(Class)cls {
    if (cls != Nil) {
        if (class_respondsToSelector(object_getClass(cls), @selector(bundleForClass))) {
            id custom = [(id<BundleClassMessages>)cls bundleForClass];
            if (objc_opt_isKindOfClass(custom, (Class)nativeBundleClass) && custom != nil) {
                return custom;
            }
        }
        id cached = [(__ONSBundleTables *)[__ONSBundleTables bundleTables] bundleForClass:cls];
        if (cached != nil) {
            return cached;
        }
        const char *image = class_getImageName(cls);
        if (image != NULL) {
            id imagePath = normalizedPath(_NSFileSystemRepresentationString(image, strnlen(image, 1026)));
            if (imagePath != nil) {
                id path = _NSFrameworkPathFromLibraryPath(imagePath);
                if (path == nil) {
                    path = _NSBundlePathFromExecutablePath(imagePath);
                }
                if (path != nil) {
                    ONSBundle *bundle = objc_autorelease([[ONSBundle alloc] initWithPath:path]);
                    if (bundle != nil) {
                        atomic_fetch_or(&bundle->_flags, 0x4000000);
                        atomic_fetch_or(&bundle->_flags, 0x30000);
                        atomic_fetch_or(&bundle->_flags, 8);
                        [(__ONSBundleTables *)[__ONSBundleTables bundleTables] addBundle:bundle
                                                                                 forPath:nil
                                                                                withType:0x30000
                                                                                forClass:cls
                                                                              isImmortal:YES];
                        return bundle;
                    }
                }
            }
        }
    }
    return [ONSBundle mainBundle];
}

// Native -[NSBundle infoDictionary], 0x1c04d02ec. Non-external.
- (id)infoDictionary {
    OCFBundleRef bundle = [self _cfBundle];
    return bundle == NULL ? nil : objc_autorelease(objc_retain((id)OCFBundleGetInfoDictionary(bundle)));
}

// Native -[NSBundle localizedInfoDictionary], 0x1c04d0314. Non-external.
- (id)localizedInfoDictionary {
    OCFBundleRef bundle = [self _cfBundle];
    return bundle == NULL ? nil : objc_autorelease(objc_retain((id)OCFBundleGetLocalInfoDictionary(bundle)));
}

// Native -[NSBundle objectForInfoDictionaryKey:], 0x1c04d033c. Non-external.
- (id)objectForInfoDictionaryKey:(id)key {
    id object = [[self localizedInfoDictionary] objectForKey:key];
    return object != nil ? object : [[self infoDictionary] objectForKey:key];
}

// Native +[NSBundle loadedBundles], 0x1c04d03a8. Non-external.
+ (id)loadedBundles {
    return [(__ONSBundleTables *)[__ONSBundleTables bundleTables] loadedBundles];
}

// Native +[NSBundle mainBundle], 0x1c04d05e4. Non-external.
+ (id)mainBundle {
    os_unfair_lock_lock(&mainBundleLock);
    ONSBundle *result = objc_retain(mainBundle);
    if (result == nil) {
        os_unfair_lock_unlock(&mainBundleLock);
        OCFBundleRef bundle = OCFBundleGetMainBundle();
        if (bundle != NULL) {
            OCFURLRef url = OCFBundleCopyBundleURL(bundle);
            OCFStringRef path = OCFURLCopyFileSystemPath(url, kOCFURLPOSIXPathStyle);
            OCFRelease(url);
            if (path != NULL) {
                result = [[self alloc] initWithPath:(id)path];
                OCFRelease(path);
            }
        }
        os_unfair_lock_lock(&mainBundleLock);
        if (mainBundle != nil) {
            objc_release(result);
            result = objc_retain(mainBundle);
        } else if (result != nil) {
            objc_retain((id)bundle);
            atomic_store(&result->_cfBundle, bundle);
            atomic_fetch_or(&result->_flags, 0x10000);
            atomic_fetch_or(&result->_flags, 0x4000000);
            if (result->_resolvedPath != nil) {
                objc_autorelease(result->_initialPath);
                result->_initialPath = [result->_resolvedPath copy];
            }
            [(__ONSBundleTables *)[__ONSBundleTables bundleTables] removeBundle:result
                                                                        forPath:nil
                                                                           type:0x20000];
            mainBundle = objc_retain(result);
        }
    }
    os_unfair_lock_unlock(&mainBundleLock);
    return objc_autorelease(result);
}

// Native +[NSBundle debugDescription], 0x1c04d0734. Non-external.
+ (id)debugDescription {
    return [[self loadedBundles] description];
}

// Native -[NSBundle description], 0x1c04d0748. Non-external.
- (id)description {
    Class cls = objc_opt_class(self);
    id path = [self bundlePath];
    id status = [self isLoaded] ? (id)OF_STR("") : (id)OF_STR("not yet ");
    return [ONSString stringWithFormat:(id)OF_STR("%@ <%@> (%@loaded)"), cls, path, status];
}

// Native -[NSBundle localizations], 0x1c04d07dc. Non-external.
- (id)localizations {
    OCFBundleRef bundle = [self _cfBundle];
    return objc_autorelease(bundle == NULL ? nil : (id)OCFBundleCopyBundleLocalizations(bundle));
}

// Native -[NSBundle preferredLocalizations], 0x1c04d07f8. Non-external.
- (id)preferredLocalizations {
    return [ONSBundle preferredLocalizationsFromArray:[self localizations]];
}

// Native -[NSBundle developmentLocalization], 0x1c04d0824. Non-external.
- (id)developmentLocalization {
    OCFBundleRef bundle = [self _cfBundle];
    return bundle == NULL ? nil : objc_autorelease(objc_retain((id)OCFBundleGetDevelopmentRegion(bundle)));
}

// Native +[NSBundle preferredLocalizationsFromArray:], 0x1c04d084c. Non-external.
+ (id)preferredLocalizationsFromArray:(id)localizations {
    return objc_autorelease(
        localizations == nil ? nil
                             : (id)OCFBundleCopyPreferredLocalizationsFromArray((OCFArrayRef)localizations)
    );
}

// Native +[NSBundle preferredLocalizationsFromArray:forPreferences:], 0x1c04d0870. Non-external.
+ (id)preferredLocalizationsFromArray:(id)localizations forPreferences:(id)preferences {
    return objc_autorelease(
        localizations == nil ? nil
                             : (id)OCFBundleCopyLocalizationsForPreferences(
                                   (OCFArrayRef)localizations, (OCFArrayRef)preferences
                               )
    );
}

// Native -[NSBundle executableArchitectures], 0x1c04d09b4. Non-external.
- (id)executableArchitectures {
    OCFBundleRef bundle = [self _cfBundle];
    return objc_autorelease(bundle == NULL ? nil : (id)OCFBundleCopyExecutableArchitectures(bundle));
}

// Native __NSBundleImagesLoadedCallback, 0x1c04d1024. Non-external.
static void _NSBundleImagesLoadedCallback(unsigned int count, const struct mach_header *const headers[]) {
    if (_OCFGetTSD(37) == (void *)1) {
        const struct mach_header **images = malloc_type_calloc(count + 1, sizeof(*images), 0x6004044c4a2df);
        memcpy(images, headers, count * sizeof(*images));
        images[count] = NULL;
        _OCFSetTSD(37, images, NULL);
    }
}

// Native -[NSBundle loadForcingGlobalAndReturnError:], 0x1c0fe2188. Non-external.
- (BOOL)loadForcingGlobalAndReturnError:(id *)error {
    ONSRequestConcreteImplementation(self, _cmd, objc_opt_class((id)nativeBundleClass));
    return NO;
}

// Native -[NSBundle
// _localizedStringForKey:value:table:bundleLocalizations:localization:preventMarkdownParsing:], 0x1c0fe2348.
// Non-external.
- (id)_localizedStringForKey:(id)key
                       value:(id)value
                       table:(id)table
         bundleLocalizations:(id)bundleLocalizations
                localization:(id)localization
      preventMarkdownParsing:(BOOL)preventMarkdownParsing {
    ONSRequestConcreteImplementation(self, _cmd, objc_opt_class((id)nativeBundleClass));
    return nil;
}

@end

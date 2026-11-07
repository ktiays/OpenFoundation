//
//  Created by ktiays on 2026/11/07.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation definitions the wasm port supplies in place of trimmed, Swift-implemented or unrestored native
// code (Port/Trim.tsv; owner decision, 2026-11-07: a trimmed part that turns out to be needed gets a simple
// implementation). Each definition states what it stands for and how far it goes.

#import "CoreFoundation/Notification/OCFNotificationCenter.h"
#import "CoreFoundation/Objective-C/OCFNSMutableSet.h"
#import "Foundation/FileManager/ONSFileManager.h"
#import "Foundation/Number/ONSDecimalNumber.h"
#import "Foundation/String/ONSAttributeDescriptor.h"
#import "Foundation/String/ONSCharacterSet.h"
#import "Foundation/String/ONSInflectionRule.h"
#import "Foundation/String/ONSMorphology.h"
#import "Foundation/String/ONSMorphologyPronoun.h"
#import "Foundation/String/ONSPresentationIntent.h"
#import "Foundation/String/ONSString.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"

#include "CoreFoundation/Collections/OCFDictionary.h"
#include <dirent.h>
#include <errno.h>
#include <objc/runtime.h>
#include <os/lock.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sysdir.h>
#include <unistd.h>

@protocol PortDefinitionMessages
+ (id)array;
+ (id)arrayWithObjects:(const id *)objects count:(NSUInteger)count;
+ (id)dictionary;
- (id)objectEnumerator;
- (void)addObject:(id)object;
- (id)objectForKey:(id)key;
- (void)setObject:(id)object forKey:(id)key;
- (void)removeObjectForKey:(id)key;
- (void)removeAllObjects;
- (const char *)fileSystemRepresentation;
- (id)stringByDeletingLastPathComponent;
@end

#pragma mark - Constants

// Attribute names and notification names of families that are not restored; their values are their names.
id const ONSInflectionAgreementArgumentAttributeName = @"NSInflectionAgreementArgument";
id const ONSInflectionAgreementConceptAttributeName = @"NSInflectionAgreementConcept";
id const ONSInflectionReferentConceptAttributeName = @"NSInflectionReferentConcept";
id const ONSInflectionRuleAttributeName = @"NSInflect";
id const _ONSInflectionAlternativeAttributeName = @"NSInflectionAlternative";
id const ONSLocalizedNumberFormatAttributeName = @"NSLocalizedNumberFormat";

id const NSUbiquityIdentityDidChangeNotification = @"NSUbiquityIdentityDidChangeNotification";
id const NSBundleDidLoadNotification = @"NSBundleDidLoadNotification";
id const NSProcessInfoThermalStateDidChangeNotification = @"NSProcessInfoThermalStateDidChangeNotification";
id const NSProcessInfoPowerStateDidChangeNotification = @"NSProcessInfoPowerStateDidChangeNotification";

// The dependent notifications NSNotificationCenter consults when an observer registers: each names its
// notification and has no sub-notification, so a registration installs nothing further (the native records
// observe the system sources these notifications follow, which the port does not have).
const DependentNotification _NSUbiquityIdentityDidChangeDependentNotification = {
    .name = (const OCFStringRef *)&NSUbiquityIdentityDidChangeNotification,
};
const DependentNotification _NSBundleDidLoadDependentNotification = {
    .name = (const OCFStringRef *)&NSBundleDidLoadNotification,
};
const DependentNotification _NSProcessInfoThermalStateDidChangeDependentNotification = {
    .name = (const OCFStringRef *)&NSProcessInfoThermalStateDidChangeNotification,
};
const DependentNotification _NSProcessInfoPowerStateDidChangeDependentNotification = {
    .name = (const OCFStringRef *)&NSProcessInfoPowerStateDidChangeNotification,
};

#pragma mark - Swift units

// LocaleNotifications.swift: the generation of the locale notification cache, an atomic integer that starts
// at 1 and that nothing in the port advances.
static _Atomic NSInteger gLocaleNotificationGeneration = 1;

NSInteger _localeNotificationCount(void) {
    return atomic_load_explicit(&gLocaleNotificationGeneration, memory_order_relaxed);
}

// DarwinSearchPaths.swift's _NSSearchPathsForDirectoryInDomain: the sysdir enumeration's paths (the port's
// sysdir has none), with a leading tilde left as is. Autoreleased.
// Swift calling convention, as Source/Foundation/FileManager/ONSUnrestoredInterfaces.h declares it: wasm
// signatures include the convention's context and error parameters.
id __attribute__((swiftcall))
_NSSearchPathsForDirectoryInDomain(NSUInteger directory, NSUInteger domains, BOOL expandTilde) {
    (void)expandTilde;
    id paths = [(id)objc_getClass(OF_NAME_STRING(ONSMutableArray)) array];
    sysdir_search_path_enumeration_state state =
        sysdir_start_search_path_enumeration_private((uint32_t)directory, (uint32_t)(domains & 0xc0f));
    char path[0x402];
    while ((state = sysdir_get_next_search_path_enumeration(state, path)) != 0) {
        [paths addObject:[ONSString stringWithUTF8String:path]];
    }
    return paths;
}

// _NSSwiftCharacterSet: the Swift character set the feature flags select. Every flag is off on the port, so
// no instance is created; the class exists for the references to it.
@interface _ONSSwiftCharacterSet : ONSCharacterSet
@end

@implementation _ONSSwiftCharacterSet
@end

// _NSFileManagerBridge: the Swift FileManager core NSFileManager forwards to, implemented over POSIX for the
// requests NSFileManager sends. Errors are reported by the result only (no NSError is built).
@interface _ONSFileManagerBridge : NSObject
@end

@implementation _ONSFileManagerBridge

- (id)initWithFileManager:(ONSFileManager *)manager {
    (void)manager;
    return [super init];
}

static const char *posixPath(id path) {
    return path != nil ? [path fileSystemRepresentation] : NULL;
}

- (BOOL)fileExistsAtPath:(id)path {
    struct stat information;
    const char *representation = posixPath(path);
    return representation != NULL && stat(representation, &information) == 0;
}

- (BOOL)moveItemAtPath:(id)source toPath:(id)destination options:(NSUInteger)options error:(id *)error {
    (void)options;
    if (error != NULL) {
        *error = nil;
    }
    const char *from = posixPath(source);
    const char *to = posixPath(destination);
    return from != NULL && to != NULL && rename(from, to) == 0;
}

static bool removeRecursively(const char *path) {
    struct stat information;
    if (lstat(path, &information) != 0) {
        return false;
    }
    if (!S_ISDIR(information.st_mode)) {
        return unlink(path) == 0;
    }
    DIR *directory = opendir(path);
    if (directory == NULL) {
        return false;
    }
    struct dirent *entry;
    bool succeeded = true;
    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        size_t length = strlen(path) + strlen(entry->d_name) + 2;
        char *child = malloc(length);
        snprintf(child, length, "%s/%s", path, entry->d_name);
        succeeded = removeRecursively(child) && succeeded;
        free(child);
    }
    closedir(directory);
    return rmdir(path) == 0 && succeeded;
}

- (BOOL)removeItemAtPath:(id)path error:(id *)error {
    if (error != NULL) {
        *error = nil;
    }
    const char *representation = posixPath(path);
    return representation != NULL && removeRecursively(representation);
}

- (BOOL)createDirectoryAtPath:(id)path
    withIntermediateDirectories:(BOOL)intermediates
                     attributes:(id)attributes
                          error:(id *)error {
    (void)attributes;
    if (error != NULL) {
        *error = nil;
    }
    const char *representation = posixPath(path);
    if (representation == NULL) {
        return NO;
    }
    if (!intermediates) {
        return mkdir(representation, 0777) == 0;
    }
    char *copy = strdup(representation);
    for (char *cursor = copy + 1; *cursor != '\0'; cursor++) {
        if (*cursor == '/') {
            *cursor = '\0';
            mkdir(copy, 0777);
            *cursor = '/';
        }
    }
    int result = mkdir(copy, 0777);
    free(copy);
    return result == 0 || errno == EEXIST;
}

- (id)temporaryDirectory {
    return @"/tmp";
}

- (id)currentDirectoryPath {
    char buffer[1024];
    return getcwd(buffer, sizeof(buffer)) != NULL ? [ONSString stringWithUTF8String:buffer] : nil;
}

- (BOOL)setAttributes:(id)attributes ofItemAtPath:(id)path error:(id *)error {
    (void)attributes;
    if (error != NULL) {
        *error = nil;
    }
    return [self fileExistsAtPath:path];
}

@end

#pragma mark - Unrestored classes

// NSCountedSet: a mutable set that counts its members, over a CFDictionary from member to count (simple
// implementation of the unrestored class; CA's display link and process monitor use it).
@interface ONSCountedSet : ONSMutableSet {
    OCFMutableDictionaryRef _counts;
}
@end

@implementation ONSCountedSet

- (id)init {
    return [self initWithCapacity:0];
}

- (id)initWithCapacity:(NSUInteger)capacity {
    self = [super init];
    if (self != nil) {
        _counts = OCFDictionaryCreateMutable(
            kOCFAllocatorDefault, (OCFIndex)capacity, &kOCFTypeDictionaryKeyCallBacks, NULL
        );
    }
    return self;
}

- (void)dealloc {
    if (_counts != NULL) {
        OCFRelease(_counts);
    }
    [super dealloc];
}

- (NSUInteger)count {
    return (NSUInteger)OCFDictionaryGetCount(_counts);
}

- (id)member:(id)object {
    const void *key = NULL;
    return object != nil && OCFDictionaryGetKeyIfPresent(_counts, object, &key) ? (id)key : nil;
}

- (NSUInteger)countForObject:(id)object {
    return object != nil ? (NSUInteger)(uintptr_t)OCFDictionaryGetValue(_counts, object) : 0;
}

- (id)objectEnumerator {
    OCFIndex count = OCFDictionaryGetCount(_counts);
    const void **keys = malloc((size_t)(count > 0 ? count : 1) * sizeof(void *));
    OCFDictionaryGetKeysAndValues(_counts, keys, NULL);
    id array = [(id)objc_getClass(OF_NAME_STRING(ONSArray)) arrayWithObjects:(const id *)keys
                                                                       count:(NSUInteger)count];
    free(keys);
    return [array objectEnumerator];
}

- (void)addObject:(id)object {
    if (object != nil) {
        uintptr_t count = (uintptr_t)OCFDictionaryGetValue(_counts, object);
        OCFDictionarySetValue(_counts, object, (const void *)(count + 1));
    }
}

- (void)removeObject:(id)object {
    if (object == nil) {
        return;
    }
    uintptr_t count = (uintptr_t)OCFDictionaryGetValue(_counts, object);
    if (count > 1) {
        OCFDictionarySetValue(_counts, object, (const void *)(count - 1));
    } else if (count == 1) {
        OCFDictionaryRemoveValue(_counts, object);
    }
}

- (void)removeAllObjects {
    OCFDictionaryRemoveAllValues(_counts);
}

@end

// NSCache: a thread-safe key-value store without eviction (simple implementation of the unrestored class).
@implementation ONSCache {
    os_unfair_lock _lock;
    id _storage;
    id _name;
    id _delegate;
    NSUInteger _countLimit;
    NSUInteger _totalCostLimit;
    BOOL _evictsObjectsWithDiscardedContent;
}

- (id)init {
    self = [super init];
    if (self != nil) {
        _lock = OS_UNFAIR_LOCK_INIT;
        _storage = [[(id)objc_getClass(OF_NAME_STRING(ONSMutableDictionary)) dictionary] retain];
        _evictsObjectsWithDiscardedContent = YES;
    }
    return self;
}

- (void)dealloc {
    [_storage release];
    [_name release];
    [super dealloc];
}

- (id)objectForKey:(id)key {
    os_unfair_lock_lock(&_lock);
    id object = [[_storage objectForKey:key] retain];
    os_unfair_lock_unlock(&_lock);
    return [object autorelease];
}

- (void)setObject:(id)object forKey:(id)key {
    [self setObject:object forKey:key cost:0];
}

- (void)setObject:(id)object forKey:(id)key cost:(NSUInteger)cost {
    (void)cost;
    if (key == nil) {
        return;
    }
    os_unfair_lock_lock(&_lock);
    if (object != nil) {
        [_storage setObject:object forKey:key];
    } else {
        [_storage removeObjectForKey:key];
    }
    os_unfair_lock_unlock(&_lock);
}

- (void)removeObjectForKey:(id)key {
    os_unfair_lock_lock(&_lock);
    [_storage removeObjectForKey:key];
    os_unfair_lock_unlock(&_lock);
}

- (void)removeAllObjects {
    os_unfair_lock_lock(&_lock);
    [_storage removeAllObjects];
    os_unfair_lock_unlock(&_lock);
}

- (id)name {
    return _name;
}

- (void)setName:(id)name {
    id old = _name;
    _name = [name copy];
    [old release];
}

- (id)delegate {
    return _delegate;
}

- (void)setDelegate:(id)delegate {
    _delegate = delegate;
}

- (NSUInteger)countLimit {
    return _countLimit;
}

- (void)setCountLimit:(NSUInteger)limit {
    _countLimit = limit;
}

- (NSUInteger)totalCostLimit {
    return _totalCostLimit;
}

- (void)setTotalCostLimit:(NSUInteger)limit {
    _totalCostLimit = limit;
}

- (BOOL)evictsObjectsWithDiscardedContent {
    return _evictsObjectsWithDiscardedContent;
}

- (void)setEvictsObjectsWithDiscardedContent:(BOOL)evicts {
    _evictsObjectsWithDiscardedContent = evicts;
}

@end

// Classes of trimmed families (Port/Trim.tsv) and of CFNetwork (unbound on Apple builds too) that kept code
// references by class symbol. They exist so that the references resolve; they implement nothing, so a
// message they do not inherit raises an unrecognized-selector exception.
@implementation ONSDecimalNumber
@end

@implementation ONSInflectionRule
@end

@implementation ONSInflectionRuleExplicit
@end

@implementation ONSMorphology
@end

@implementation ONSMorphologyPronoun
@end

@implementation ONSPresentationIntent
@end

@interface ONSURLRequest : NSObject
@end

@implementation ONSURLRequest
@end

@interface ONSHTTPURLResponse : NSObject
@end

@implementation ONSHTTPURLResponse
@end

@interface ONSURLConnection : NSObject
@end

@implementation ONSURLConnection
@end

// Attribute descriptors of attribute kinds whose families are not restored; they behave as the base
// descriptor.
@interface _ONSNumericAttributeDescriptor : _ONSAttributeDescriptor
@end

@implementation _ONSNumericAttributeDescriptor
@end

@interface _ONSStringAttributeDescriptor : _ONSAttributeDescriptor
@end

@implementation _ONSStringAttributeDescriptor
@end

@interface _ONSMarkdownStringAttributeDescriptor : _ONSAttributeDescriptor
@end

@implementation _ONSMarkdownStringAttributeDescriptor
@end

@interface _ONSNumberFormattingAttributeDescriptor : _ONSAttributeDescriptor
@end

@implementation _ONSNumberFormattingAttributeDescriptor
@end

@interface _ONSAssumedFallbackInflectionAttributeDescriptor : _ONSAttributeDescriptor
@end

@implementation _ONSAssumedFallbackInflectionAttributeDescriptor
@end

// Foundation/XPC/ONSXPCClasses.m (trimmed) defines the once block NSXPCCoder runs before its first use to
// initialize the XPC classes; the port has no XPC classes, so the block does nothing.
dispatch_block_t const xpcInitializeClassesBlock = ^{
};

//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "CoreFoundation/Objective-C/OCFNSURL.h"

#include <dispatch/dispatch.h>

// The check every resource method inlines after reading -_cfurl: the immutable placeholder (+__unurl) means
// the receiver was never initialized, which raises NSGenericException "%@: object was not initialized" with
// the full method name (the .cold.1 fragments). checkInitialized is a local descriptive name.
static inline void checkInitialized(id self, SEL _cmd, id url) {
    if (url == [ONSURL __unurl]) {
        Class cls = (Class)__CFLookUpClass(OF_NAME_STRING(ONSURL));
        _CFThrowFormattedException(
            ONSGenericException, OF_STR("%@: object was not initialized"), __CFFullMethodName(cls, self, _cmd)
        );
    }
}

// Inferred organization: NSURL's CoreFoundation methods (0x1c02b4534-0x1c02b4fe4) lie together in the iOS
// Simulator __text section, before the NSBlock methods; following the owner's decision, the class has its own
// translation unit. Objective-C, manual reference counting (objc_retain and objc_autorelease). The
// fourteen methods that reach .cold.1 fragments (+__unurl, with its block, and every method that compares
// -_cfurl with it) are restored from the IDA capture of the fragments (iphone-simulator-corpus-delta2.json).
// The "object was not initialized" fragments end in OUTLINED_FUNCTION_0_52 (0x1c02b4fe8: the receiver and
// selector kept, NSGenericException loaded, __CFLookUpClass("NSURL")), _2_32 (0x1c02b501c:
// __CFFullMethodName(class, receiver, selector)) and _1_38 (0x1c02b5008: the raise with "%@: object was not
// initialized"), read in iphone-simulator-corpus-delta3.json (confidence: verified). Unless
// a definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified (assembly read in full). iOS Device 24A446: unread. Reading:
// Documentation/Reference/Research/CoreFoundationBridgedClasses.md (not written yet).

@implementation ONSURL

// Native +[NSURL __unurl], 0x1c02b4534, with its .cold.1 (0x1c0311cc0, the dispatch_once call) and its
// function statics __unurl.static_init (__DATA,__bss 0x22e6d39d8) and __unurl.immutablePlaceholderNSURL
// (0x22e6d39d0). Non-external. Native ___16+[NSURL __unurl]_block_invoke, 0x1c02b4564 (non-external, the
// global block ___block_literal_global_38): the placeholder is CFURLCreateWithString(kCFAllocatorDefault,
// "__immutablePlaceholderNSURL", NULL).
+ (id)__unurl {
    static dispatch_once_t static_init;
    static id immutablePlaceholderNSURL;
    dispatch_once(&static_init, ^{
      immutablePlaceholderNSURL =
          (id)OCFURLCreateWithString(kOCFAllocatorDefault, OF_STR("__immutablePlaceholderNSURL"), NULL);
    });
    return immutablePlaceholderNSURL;
}

// Native +[NSURL supportsSecureCoding], 0x1c02b4598. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSURL getResourceValue:forKey:error:], 0x1c02b45a0, with its .cold.1 (0x1c0311cd4).
// Non-external. Inside the resource-cache access bracket. On success a nonnull value is autoreleased in
// place; on failure a returned error is (an error pointer of NULL answers NO without touching it).
- (BOOL)getResourceValue:(id *)value forKey:(id)key error:(id *)error {
    OCFURLRef url = (OCFURLRef)[self _cfurl];
    checkInitialized(self, _cmd, (id)url);
    if (url == NULL) {
        return NO;
    }
    __OCFURLBeginResourcePropertyCacheAccess(url);
    Boolean copied = OCFURLCopyResourcePropertyForKey(url, (OCFStringRef)key, value, (OCFErrorRef *)error);
    BOOL result = copied != 0;
    __OCFURLEndResourcePropertyCacheAccess(url);
    if (!copied) {
        if (error == NULL) {
            return NO;
        }
        *error = [*error autorelease];
        return result;
    }
    if (*value == nil) {
        return YES;
    }
    *value = [*value autorelease];
    return result;
}

// Native -[NSURL resourceValuesForKeys:error:], 0x1c02b466c, with its .cold.1 (0x1c0311cf4). Non-external.
// The dictionary is autoreleased; on failure a returned error is autoreleased in place.
- (id)resourceValuesForKeys:(id)keys error:(id *)error {
    OCFURLRef url = (OCFURLRef)[self _cfurl];
    checkInitialized(self, _cmd, (id)url);
    if (url == NULL) {
        return nil;
    }
    __OCFURLBeginResourcePropertyCacheAccess(url);
    OCFDictionaryRef values =
        OCFURLCopyResourcePropertiesForKeys(url, (OCFArrayRef)keys, (OCFErrorRef *)error);
    __OCFURLEndResourcePropertyCacheAccess(url);
    if (values != NULL) {
        return [(id)values autorelease];
    }
    if (error != NULL) {
        *error = [*error autorelease];
    }
    return nil;
}

// Native -[NSURL setResourceValue:forKey:error:], 0x1c02b4724, with its .cold.1 (0x1c0311d14). Non-external.
- (BOOL)setResourceValue:(id)value forKey:(id)key error:(id *)error {
    OCFURLRef url = (OCFURLRef)[self _cfurl];
    checkInitialized(self, _cmd, (id)url);
    if (url == NULL) {
        return NO;
    }
    __OCFURLBeginResourcePropertyCacheAccess(url);
    Boolean set = OCFURLSetResourcePropertyForKey(url, (OCFStringRef)key, value, (OCFErrorRef *)error);
    BOOL result = set != 0;
    __OCFURLEndResourcePropertyCacheAccess(url);
    if (error != NULL && !set) {
        *error = [*error autorelease];
        result = NO;
    }
    return result;
}

// Native -[NSURL setResourceValues:error:], 0x1c02b47e0, with its .cold.1 (0x1c0311d34). Non-external.
- (BOOL)setResourceValues:(id)keyedValues error:(id *)error {
    OCFURLRef url = (OCFURLRef)[self _cfurl];
    checkInitialized(self, _cmd, (id)url);
    if (url == NULL) {
        return NO;
    }
    __OCFURLBeginResourcePropertyCacheAccess(url);
    Boolean set =
        OCFURLSetResourcePropertiesForKeys(url, (OCFDictionaryRef)keyedValues, (OCFErrorRef *)error);
    BOOL result = set != 0;
    __OCFURLEndResourcePropertyCacheAccess(url);
    if (error != NULL && !set) {
        *error = [*error autorelease];
        result = NO;
    }
    return result;
}

// Native -[NSURL removeCachedResourceValueForKey:], 0x1c02b4894, with its .cold.1 (0x1c0311d54).
// Non-external.
- (void)removeCachedResourceValueForKey:(id)key {
    OCFURLRef url = (OCFURLRef)[self _cfurl];
    checkInitialized(self, _cmd, (id)url);
    if (url == NULL) {
        return;
    }
    __OCFURLBeginResourcePropertyCacheAccess(url);
    OCFURLClearResourcePropertyCacheForKey(url, (OCFStringRef)key);
    __OCFURLEndResourcePropertyCacheAccess(url);
}

// Native -[NSURL removeAllCachedResourceValues], 0x1c02b4914, with its .cold.1 (0x1c0311d74). Non-external.
- (void)removeAllCachedResourceValues {
    OCFURLRef url = (OCFURLRef)[self _cfurl];
    checkInitialized(self, _cmd, (id)url);
    if (url == NULL) {
        return;
    }
    __OCFURLBeginResourcePropertyCacheAccess(url);
    OCFURLClearResourcePropertyCache(url);
    __OCFURLEndResourcePropertyCacheAccess(url);
}

// Native -[NSURL setTemporaryResourceValue:forKey:], 0x1c02b498c, with its .cold.1 (0x1c0311d94).
// Non-external.
- (void)setTemporaryResourceValue:(id)value forKey:(id)key {
    OCFURLRef url = (OCFURLRef)[self _cfurl];
    checkInitialized(self, _cmd, (id)url);
    if (url == NULL) {
        return;
    }
    __OCFURLBeginResourcePropertyCacheAccess(url);
    OCFURLSetTemporaryResourcePropertyForKey(url, (OCFStringRef)key, value);
    __OCFURLEndResourcePropertyCacheAccess(url);
}

// Native -[NSURL isFileReferenceURL], 0x1c02b4a20, with its .cold.1 (0x1c0311db4). Non-external. No cache
// access bracket.
- (BOOL)isFileReferenceURL {
    OCFURLRef url = (OCFURLRef)[self _cfurl];
    checkInitialized(self, _cmd, (id)url);
    if (url == NULL) {
        return NO;
    }
    return OCFURLIsFileReferenceURL(url) != 0;
}

// Native -[NSURL fileReferenceURL], 0x1c02b4a8c, with its .cold.1 (0x1c0311dd4). Non-external. Inside the
// bracket, the URL's allocator and a second -_cfurl make the reference URL (no error), autoreleased.
- (id)fileReferenceURL {
    OCFURLRef url = (OCFURLRef)[self _cfurl];
    checkInitialized(self, _cmd, (id)url);
    if (url == NULL) {
        return nil;
    }
    __OCFURLBeginResourcePropertyCacheAccess(url);
    OCFAllocatorRef allocator = OCFGetAllocator(url);
    OCFURLRef referenceURL = OCFURLCreateFileReferenceURL(allocator, (OCFURLRef)[self _cfurl], NULL);
    __OCFURLEndResourcePropertyCacheAccess(url);
    if (referenceURL == NULL) {
        return nil;
    }
    return [(id)referenceURL autorelease];
}

// Native -[NSURL filePathURL], 0x1c02b4b34, with its .cold.1 (0x1c0311df4). Non-external. As
// -fileReferenceURL, through CFURLCreateFilePathURL.
- (id)filePathURL {
    OCFURLRef url = (OCFURLRef)[self _cfurl];
    checkInitialized(self, _cmd, (id)url);
    if (url == NULL) {
        return nil;
    }
    __OCFURLBeginResourcePropertyCacheAccess(url);
    OCFAllocatorRef allocator = OCFGetAllocator(url);
    OCFURLRef pathURL = OCFURLCreateFilePathURL(allocator, (OCFURLRef)[self _cfurl], NULL);
    __OCFURLEndResourcePropertyCacheAccess(url);
    if (pathURL == NULL) {
        return nil;
    }
    return [(id)pathURL autorelease];
}

// Native -[NSURL _resolveFlags], 0x1c02b4bdc. Non-external. Only file URLs carry flags: a file reference URL
// reads them from its query, a path URL from the path of its absolute URL (autoreleased). The flag word's
// width is 32 bits by the callers' use (confidence: medium).
- (unsigned int)_resolveFlags {
    if (![self isFileURL]) {
        return 0;
    }
    if ([self isFileReferenceURL]) {
        if ([self query] == nil) {
            return 0;
        }
        return [[self query] _queryResolveFlags];
    }
    OCFURLRef absoluteURL = OCFURLCopyAbsoluteURL((OCFURLRef)[self _cfurl]);
    if (absoluteURL == NULL) {
        return 0;
    }
    id path = [(id)OCFURLCopyPath(absoluteURL) autorelease];
    OCFRelease(absoluteURL);
    if (path == nil) {
        return 0;
    }
    return [path _pathResolveFlags];
}

// Native -[NSURL _URLByInsertingResolveFlags:], 0x1c02b4c6c. Non-external. -isFileURL is sent first in every
// case. Without flags, or for a non-file URL, or when the string cannot be rewritten, the receiver is
// answered (retained and autoreleased); otherwise a URL made from the absolute string with the flags inserted
// into its query (file reference URLs) or its path.
- (id)_URLByInsertingResolveFlags:(unsigned int)flags {
    BOOL isFileURL = [self isFileURL];
    id result;
    if (flags != 0 && isFileURL) {
        BOOL isFileReferenceURL = [self isFileReferenceURL];
        id absoluteString = [self absoluteString];
        id string = isFileReferenceURL ? [absoluteString _urlStringByInsertingQueryResolveFlags:flags]
                                       : [absoluteString _urlStringByInsertingPathResolveFlags:flags];
        if (string != nil) {
            result = (id)OCFURLCreateWithString(kOCFAllocatorDefault, (OCFStringRef)string, NULL);
            return [result autorelease];
        }
    }
    result = [self retain];
    return [result autorelease];
}

// Native -[NSURL _URLByRemovingResolveFlags], 0x1c02b4cec. Non-external. A file URL whose absolute string
// loses its flags answers a new URL; otherwise the receiver (retained and autoreleased).
- (id)_URLByRemovingResolveFlags {
    id result;
    if ([self isFileURL]) {
        id string = [[self absoluteString] _urlStringByRemovingResolveFlags];
        if (string != nil) {
            result = (id)OCFURLCreateWithString(kOCFAllocatorDefault, (OCFStringRef)string, NULL);
            return [result autorelease];
        }
    }
    result = [self retain];
    return [result autorelease];
}

// Native -[NSURL bookmarkDataWithOptions:includingResourceValuesForKeys:relativeToURL:error:], 0x1c02b4d40,
// with its .cold.1 (0x1c0311e14). Non-external. The relative URL is sent -_cfurl after the allocator is read;
// the data is autoreleased; on failure a nonnull returned error is autoreleased in place.
- (id)bookmarkDataWithOptions:(NSUInteger)options
    includingResourceValuesForKeys:(id)keys
                     relativeToURL:(id)relativeURL
                             error:(id *)error {
    OCFURLRef url = (OCFURLRef)[self _cfurl];
    checkInitialized(self, _cmd, (id)url);
    if (url == NULL) {
        return nil;
    }
    __OCFURLBeginResourcePropertyCacheAccess(url);
    OCFAllocatorRef allocator = OCFGetAllocator(url);
    OCFDataRef data = OCFURLCreateBookmarkData(
        allocator, url, options, (OCFArrayRef)keys, (OCFURLRef)[relativeURL _cfurl], (OCFErrorRef *)error
    );
    __OCFURLEndResourcePropertyCacheAccess(url);
    if (data != NULL) {
        return [(id)data autorelease];
    }
    if (error != NULL && *error != nil) {
        *error = [*error autorelease];
    }
    return nil;
}

// Native +[NSURL resourceValuesForKeys:fromBookmarkData:], 0x1c02b4e30. Non-external.
+ (id)resourceValuesForKeys:(id)keys fromBookmarkData:(id)bookmarkData {
    OCFDictionaryRef values = OCFURLCreateResourcePropertiesForKeysFromBookmarkData(
        kOCFAllocatorDefault, (OCFArrayRef)keys, (OCFDataRef)bookmarkData
    );
    if (values == NULL) {
        return nil;
    }
    return [(id)values autorelease];
}

// Native +[NSURL writeBookmarkData:toURL:options:error:], 0x1c02b4e64. Non-external. On failure a returned
// error is autoreleased in place.
+ (BOOL)writeBookmarkData:(id)bookmarkData
                    toURL:(id)bookmarkFileURL
                  options:(NSUInteger)options
                    error:(id *)error {
    Boolean written = OCFURLWriteBookmarkDataToFile(
        (OCFDataRef)bookmarkData, (OCFURLRef)bookmarkFileURL, options, (OCFErrorRef *)error
    );
    if (error != NULL && !written && *error != nil) {
        *error = [*error autorelease];
    }
    return written != 0;
}

// Native +[NSURL bookmarkDataWithContentsOfURL:error:], 0x1c02b4eb8. Non-external. On failure a returned
// error is autoreleased in place.
+ (id)bookmarkDataWithContentsOfURL:(id)bookmarkFileURL error:(id *)error {
    OCFDataRef data = OCFURLCreateBookmarkDataFromFile(
        kOCFAllocatorDefault, (OCFURLRef)bookmarkFileURL, (OCFErrorRef *)error
    );
    if (data != NULL) {
        return [(id)data autorelease];
    }
    if (error != NULL && *error != nil) {
        *error = [*error autorelease];
    }
    return nil;
}

// Native -[NSURL startAccessingSecurityScopedResource], 0x1c02b4f14, with its .cold.1 (0x1c0311e34).
// Non-external.
- (BOOL)startAccessingSecurityScopedResource {
    OCFURLRef url = (OCFURLRef)[self _cfurl];
    checkInitialized(self, _cmd, (id)url);
    if (url == NULL) {
        return NO;
    }
    return OCFURLStartAccessingSecurityScopedResource(url) != 0;
}

// Native -[NSURL stopAccessingSecurityScopedResource], 0x1c02b4f80, with its .cold.1 (0x1c0311e54).
// Non-external. A tail call.
- (void)stopAccessingSecurityScopedResource {
    OCFURLRef url = (OCFURLRef)[self _cfurl];
    checkInitialized(self, _cmd, (id)url);
    if (url == NULL) {
        return;
    }
    OCFURLStopAccessingSecurityScopedResource(url);
}

@end

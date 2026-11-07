//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSAttributedStringFromMarkdownCreatorConcrete.h"
#import "Foundation/String/ONSAttributeDescriptor.h"
#import "Foundation/String/ONSAttributedStringInternal.h"
#import "Foundation/String/ONSMutableString.h"
#import "Foundation/String/ONSUnrestoredInterfaces.h"
#include "CoreFoundation/Objective-C/OCFNSTypeChecks.h"
#include "CoreFoundation/Objective-C/OCFNSDictionary.h"
#include "CoreFoundation/String/OCFString.h"

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified unless noted. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.

extern char markdownCreatorDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSDictionary));
extern char markdownCreatorMutableDictionaryClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                              OF_NAME_STRING(ONSMutableDictionary));
extern char markdownCreatorErrorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSError));
extern char markdownCreatorJSONClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSJSONSerialization));
extern char markdownCreatorAttributeDescriptorClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                                OF_NAME_STRING(_ONSAttributeDescriptor));

@protocol MarkdownCreatorMessages <NSObject>
- (id)initWithString:(id)string attributes:(id)attributes;
- (void)enumerateKeysAndObjectsUsingBlock:(void (^)(id key, id value, BOOL *stop))block;
- (void)setObject:(id)value forKeyedSubscript:(id)key;
@end

@implementation _ONSAttributedStringFromMarkdownCreatorConcrete

// Native -[_NSAttributedStringFromMarkdownCreatorConcrete init], 0x1c06018dc. Non-external.
- (instancetype)init {
    self = [super init];
    if (self != nil) {
        _result = objc_alloc_init((Class)attributedStringMutableBaseClass);
    }
    return self;
}

// Native -[_NSAttributedStringFromMarkdownCreatorConcrete dealloc], 0x1c0601960. Non-external.
- (void)dealloc {
    objc_release(_result);
    [super dealloc];
}

// Native -[_NSAttributedStringFromMarkdownCreatorConcrete result], 0x1c06019d4. Non-external.
- (id)result {
    return objc_autorelease(objc_retain(_result));
}

// Native -[_NSAttributedStringFromMarkdownCreatorConcrete appendString:with:], 0x1c06019ec. Non-external.
- (void)appendString:(id)string with:(id)attributes {
    id value =
        [(id<MarkdownCreatorMessages>)objc_alloc((Class)attributedStringBaseClass) initWithString:string
                                                                                       attributes:attributes];
    [_result appendAttributedString:value];
    objc_release(value);
}

// Native -[_NSAttributedStringFromMarkdownCreatorConcrete appendString:], 0x1c0601a9c. Non-external.
- (void)appendString:(id)string {
    [(ONSMutableString *)[_result mutableString] appendString:string];
}

// Native -[_NSAttributedStringFromMarkdownCreatorConcrete parseExtendedAttributesFromData:error:],
// 0x1c0601ac4. Non-external. NSDictionary 0x22227fad0 and NSMutableDictionary 0x22227fb28
// are shared-cache bindings (confidence: medium).
- (id)parseExtendedAttributesFromData:(id)data error:(id *)error {
    id underlyingError = nil;
    id json = [(id)markdownCreatorJSONClass JSONObjectWithData:data options:24 error:&underlyingError];
    if (json != nil && _ONSIsNSDictionary(json)) {
        id<MarkdownCreatorMessages> attributes =
            objc_alloc_init((Class)markdownCreatorMutableDictionaryClass);
        // Native ___attributesFromKnownAttributeKeys_block_invoke, 0x1c0602df8. Non-external.
        [(id<MarkdownCreatorMessages>)json enumerateKeysAndObjectsUsingBlock:^(id key, id value, BOOL *stop) {
          (void)stop;
          _ONSAttributeDescriptor *descriptor =
              [(id)markdownCreatorAttributeDescriptorClass descriptorForAttributeKey:key];
          if (descriptor != nil) {
              value = [descriptor attributeValueFromValue:value];
              if (value == nil) {
                  return;
              }
              key = [descriptor name];
          }
          [attributes setObject:value forKeyedSubscript:key];
        }];
        return objc_autorelease(attributes);
    }
    if (error != NULL) {
        id key;
        id value;
        if (json != nil) {
            key = (id)OF_STR("NSDebugDescription");
            value = (id)OF_STR("JSON result was not a dictionary");
        } else {
            key = (id)OF_STR("NSUnderlyingError");
            value = underlyingError;
        }
        id info = [(id)markdownCreatorDictionaryClass dictionaryWithObjects:&value forKeys:&key count:1];
        *error = [(id)markdownCreatorErrorClass errorWithDomain:(id)OF_STR(
                                                                    "NS"
                                                                    "CocoaErrorDomain"
                                                                )
                                                           code:259
                                                       userInfo:info];
    }
    return nil;
}

// Native -[_NSAttributedStringFromMarkdownCreatorConcrete finalizeWithLanguageIdentifier:],
// 0x1c0601c4c. Non-external.
- (void)finalizeWithLanguageIdentifier:(id)language {
    if (language != nil) {
        ONSMutableAttributedString *result = _result;
        ONSRange range = {0, [result length]};
        // Native ___81-[_NSAttributedStringFromMarkdownCreatorConcrete
        // finalizeWithLanguageIdentifier:]_block_invoke, 0x1c0601d08. Non-external.
        [result enumerateAttribute:(id)OF_STR("NSLanguage")
                           inRange:range
                           options:0x100000
                        usingBlock:^(id value, ONSRange run, BOOL *stop) {
                          (void)stop;
                          if (value == nil) {
                              [_result addAttribute:(id)OF_STR("NSLanguage") value:language range:run];
                          }
                        }];
    }
}

@end

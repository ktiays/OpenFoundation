//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSSimpleAttributeDictionary.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#include "CoreFoundation/Objective-C/OCFNSMutableDictionary.h"

#include <dispatch/dispatch.h>
#include <objc/runtime.h>

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified unless noted. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.

extern char
    simpleAttributeDictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSSimpleAttributeDictionary));
extern char simpleAttributeDictionaryEnumeratorClass[] __asm(
    "_OBJC_CLASS_$_" OF_NAME_STRING(ONSSimpleAttributeDictionaryEnumerator)
);
extern char attributeMutableDictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableDictionary));

static Class attributeDictionaryClass;
static id emptyAttributeDictionary;
static dispatch_once_t emptyAttributeDictionaryOnceToken;

// Native __NSAttributeDictionaryClass, 0x1c04c4b50. Non-external.
// Foundation defect: concurrent first lookups race on the cached pointer. Use relaxed atomic
// accesses, preserving both native stores and the fallback to NSSimpleAttributeDictionary.
Class _NSAttributeDictionaryClass(void) {
    Class result;
    __atomic_load(&attributeDictionaryClass, &result, __ATOMIC_RELAXED);
    if (result == Nil) {
        result = objc_lookUpClass(OF_NAME_STRING(ONSAttributeDictionary));
        __atomic_store(&attributeDictionaryClass, &result, __ATOMIC_RELAXED);
        if (result == Nil) {
            result = (Class)objc_opt_self((id)simpleAttributeDictionaryClass);
            __atomic_store(&attributeDictionaryClass, &result, __ATOMIC_RELAXED);
        }
    }
    return result;
}

// Declaration for the super send to the existing CoreFoundation NSDictionary implementation.
@interface ONSDictionary (AttributeDictionaryCopying)
- (id)mutableCopyWithZone:(struct _NSZone *)zone;
@end

@implementation ONSSimpleAttributeDictionary

// Native +[NSSimpleAttributeDictionary emptyAttributeDictionary], 0x1c04c72c0. Non-external.
+ (id)emptyAttributeDictionary {
    // Native ___55+[NSSimpleAttributeDictionary emptyAttributeDictionary]_block_invoke, 0x1c04c7300.
    dispatch_once(&emptyAttributeDictionaryOnceToken, ^{
      Class cls = (Class)objc_opt_self((id)simpleAttributeDictionaryClass);
      ONSSimpleAttributeDictionary *dictionary = ONSAllocateObject(cls, 0, NULL);
      emptyAttributeDictionary = dictionary;
      dictionary->numElements = 0;
    });
    return emptyAttributeDictionary;
}

// Native +[NSSimpleAttributeDictionary newWithDictionary:], 0x1c04c733c. Non-external.
+ (id)newWithDictionary:(id)dictionary {
    if (dictionary != nil) {
        id cls = objc_opt_self((id)simpleAttributeDictionaryClass);
        if (objc_opt_isKindOfClass(dictionary, cls)) {
            return objc_retain(dictionary);
        }
    }
    NSUInteger count = [(id<DictionaryClassMessages>)dictionary count];
    if (count == 0) {
        return objc_retain([self emptyAttributeDictionary]);
    }
    ONSEnumerator *enumerator = [dictionary keyEnumerator];
    Class cls = (Class)objc_opt_self((id)simpleAttributeDictionaryClass);
    ONSSimpleAttributeDictionary *result =
        ONSAllocateObject(cls, sizeof(NSSimpleAttributeDictionaryElement) * (count - 1), NULL);
    result->numElements = (uint32_t)count;
    for (id key = [enumerator nextObject]; key != nil; key = [enumerator nextObject]) {
        NSUInteger hash = [key hash];
        NSUInteger slot = hash % count;
        id value = [dictionary objectForKey:key];
        while (result->elements[slot].key != nil) {
            slot = slot + 1 == count ? 0 : slot + 1;
        }
        result->elements[slot].hash = hash;
        result->elements[slot].key = [key copyWithZone:NULL];
        result->elements[slot].value = objc_retain(value);
    }
    return result;
}

// Native -[NSSimpleAttributeDictionary newWithKey:object:], 0x1c04c74c8. Non-external.
// Hex-Rays leaves the last retain argument uninitialized. The assembly retains the object argument.
- (id)newWithKey:(id)key object:(id)object {
    NSUInteger slot = [self slotForKey:key];
    uint32_t count = numElements;
    if (slot == NSIntegerMax) {
        ++count;
    } else {
        id old = elements[slot].value;
        if (old == object || [old isEqual:object]) {
            return objc_retain(self);
        }
    }
    Class cls = (Class)objc_opt_self((id)simpleAttributeDictionaryClass);
    ONSSimpleAttributeDictionary *result =
        ONSAllocateObject(cls, sizeof(NSSimpleAttributeDictionaryElement) * ((NSUInteger)count - 1), NULL);
    result->numElements = count;
    if (slot == NSIntegerMax) {
        for (NSUInteger i = 0; i < count; ++i) {
            NSSimpleAttributeDictionaryElement item;
            if (i == numElements) {
                item = (NSSimpleAttributeDictionaryElement){[key hash], key, object};
            } else {
                item = elements[i];
            }
            NSUInteger destination = item.hash % count;
            while (result->elements[destination].key != nil) {
                destination = destination + 1 == count ? 0 : destination + 1;
            }
            result->elements[destination].hash = item.hash;
            result->elements[destination].key = [item.key copyWithZone:NULL];
            result->elements[destination].value = objc_retain(item.value);
        }
    } else {
        for (NSUInteger i = 0; i < numElements; ++i) {
            if (i != slot) {
                result->elements[i].hash = elements[i].hash;
                result->elements[i].key = [elements[i].key copyWithZone:NULL];
                result->elements[i].value = objc_retain(elements[i].value);
            }
        }
        result->elements[slot].hash = [key hash];
        result->elements[slot].key = [key copyWithZone:NULL];
        result->elements[slot].value = objc_retain(object);
    }
    return result;
}

// Native -[NSSimpleAttributeDictionary count], 0x1c04c770c. Non-external.
- (NSUInteger)count {
    return numElements;
}

// Native -[NSSimpleAttributeDictionary slotForKey:], 0x1c04c771c. Non-external.
- (NSUInteger)slotForKey:(id)key {
    if (numElements == 0) {
        return NSIntegerMax;
    }
    for (NSUInteger i = 0; i < numElements; ++i) {
        if (elements[i].key == key) {
            return i;
        }
    }
    NSUInteger hash = [key hash];
    NSUInteger start = hash % numElements;
    NSUInteger slot = start;
    do {
        if (elements[slot].hash == hash && [elements[slot].key isEqual:key]) {
            return slot;
        }
        slot = slot + 1 == numElements ? 0 : slot + 1;
    } while (slot != start);
    return NSIntegerMax;
}

// Native -[NSSimpleAttributeDictionary objectForKey:], 0x1c04c77f0. Non-external.
- (id)objectForKey:(id)key {
    NSUInteger slot = [self slotForKey:key];
    return slot != NSIntegerMax ? elements[slot].value : nil;
}

// Native -[NSSimpleAttributeDictionary keyEnumerator], 0x1c04c783c. Non-external.
- (id)keyEnumerator {
    return objc_autorelease([objc_allocWithZone((Class)simpleAttributeDictionaryEnumeratorClass)
        initWithAttributeDictionary:self]);
}

// Native -[NSSimpleAttributeDictionary dealloc], 0x1c04c786c. Non-external.
- (void)dealloc {
    for (NSUInteger i = 0; i < numElements; ++i) {
        objc_release(elements[i].key);
        objc_release(elements[i].value);
    }
    [super dealloc];
}

// Native -[NSSimpleAttributeDictionary mutableCopyWithZone:], 0x1c04c7924. Non-external.
// NSMutableDictionary at 0x22227fb28 is a shared-cache binding (confidence: medium).
- (id)mutableCopyWithZone:(struct _NSZone *)zone {
    if (numElements >= 41) {
        return [super mutableCopyWithZone:zone];
    }
    id values[40];
    id keys[40];
    for (NSUInteger i = 0; i < numElements; ++i) {
        keys[i] = elements[i].key;
        values[i] = elements[i].value;
    }
    return [[(id)attributeMutableDictionaryClass allocWithZone:zone] initWithObjects:values
                                                                             forKeys:keys
                                                                               count:numElements];
}

// Native -[NSSimpleAttributeDictionary copyWithZone:], 0x1c04c7a00. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return objc_retain(self);
}

@end

@implementation ONSSimpleAttributeDictionaryEnumerator

// Native -[NSSimpleAttributeDictionaryEnumerator initWithAttributeDictionary:], 0x1c04c7a04. Non-external.
- (id)initWithAttributeDictionary:(ONSSimpleAttributeDictionary *)attributes {
    self = [super init];
    if (self != nil) {
        dictionary = objc_retain(attributes);
        nextElement = 0;
    }
    return self;
}

// Native -[NSSimpleAttributeDictionaryEnumerator nextObject], 0x1c04c7a98. Non-external.
- (id)nextObject {
    NSUInteger index = nextElement;
    if (index == dictionary->numElements) {
        return nil;
    }
    nextElement = index + 1;
    return dictionary->elements[index].key;
}

// Native -[NSSimpleAttributeDictionaryEnumerator dealloc], 0x1c04c7ae8. Non-external.
- (void)dealloc {
    objc_release(dictionary);
    [super dealloc];
}

@end

//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSPresentationIntent.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#include "CoreFoundation/Objective-C/OCFNSArray.h"
#include "CoreFoundation/Objective-C/OCFNSException.h"
#include "CoreFoundation/Objective-C/OCFNSSet.h"
#include "CoreFoundation/String/OCFString.h"

#include <objc/objc-exception.h>

// iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03;
// confidence: verified unless noted. iOS Device divergence: iOS Device 24A446 remains unread.
// Reading: Source/Foundation/String/FoundationAttributedStorageReading.md.

extern char presentationIntentClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSPresentationIntent));
extern char presentationIntentArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSArray));
extern char presentationIntentMutableArrayClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                            OF_NAME_STRING(ONSMutableArray));
extern char presentationIntentNumberClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSNumber));
extern char presentationIntentSetClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSSet));
extern char presentationIntentStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSString));
extern char presentationIntentMutableStringClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                             OF_NAME_STRING(ONSMutableString));
extern char presentationIntentExceptionClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_") OF_NAME_STRING(ONSException));
extern char presentationIntentConstantIntegerClass[] __asm(OF_SYMBOL("OBJC_CLASS_$_")
                                                               OF_NAME_STRING(ONSConstantIntegerNumber));

// The Objective-C integer literals at 0x22288bbe0, 0x22288bbf8 and 0x22288bc10.
static const struct {
    const void *isa;
    const char *encoding;
    int64_t value;
} kAlignmentLeft = {presentationIntentConstantIntegerClass, "q", 0},
  kAlignmentRight = {presentationIntentConstantIntegerClass, "q", 2},
  kAlignmentCenter = {presentationIntentConstantIntegerClass, "q", 1};

@protocol PresentationIntentCoder
- (BOOL)requiresSecureCoding;
- (NSInteger)decodeIntegerForKey:(id)key;
- (id)decodeObjectOfClass:(Class)cls forKey:(id)key;
- (id)decodeArrayOfObjectsOfClass:(Class)cls forKey:(id)key;
- (id)decodeObjectOfClasses:(id)classes forKey:(id)key;
- (void)encodeInteger:(NSInteger)value forKey:(id)key;
- (void)encodeObject:(id)value forKey:(id)key;
@end

@implementation ONSPresentationIntent

// Native -[NSPresentationIntent _init], 0x1c068dd2c. Non-external.
- (id)_init {
    return [super init];
}

// Native -[NSPresentationIntent dealloc], 0x1c068dd8c. Non-external.
- (void)dealloc {
    objc_release(_parentIntent);
    objc_release(_languageHint);
    objc_release(_columnAlignments);
    [super dealloc];
}

// Native -[NSPresentationIntent hash], 0x1c068de10. Non-external.
- (NSUInteger)hash {
    NSUInteger hash = _intentKind ^ _identity ^ _headerLevel ^ _indentationLevel;
    hash ^= [_languageHint hash];
    hash ^= _columnCount;
    NSUInteger table = _ordinal ^ _column ^ _row;
    return hash ^ table ^ [_columnAlignments hash];
}

// Native -[NSPresentationIntent isEqual:], 0x1c068de74. Non-external.
- (BOOL)isEqual:(id)object {
    if (object == self) {
        return YES;
    }
    if (!objc_opt_isKindOfClass(object, (id)objc_opt_class((id)presentationIntentClass))) {
        return NO;
    }
    ONSPresentationIntent *other = object;
    NSInteger identity = [other identity];
    return identity == [self identity] && [self isEquivalentToPresentationIntent:other];
}

// Native -[NSPresentationIntent isEquivalentToPresentationIntent:], 0x1c068df00. Non-external.
- (BOOL)isEquivalentToPresentationIntent:(ONSPresentationIntent *)other {
    if (other == self) {
        return YES;
    }
    if (!objc_opt_isKindOfClass(other, (id)objc_opt_class((id)presentationIntentClass))) {
        return NO;
    }
    ONSString *language = [self languageHint];
    ONSString *otherLanguage = [other languageHint];
    ONSPresentationIntent *parent = [self parentIntent];
    ONSPresentationIntent *otherParent = [other parentIntent];
    ONSArray *alignments = [self columnAlignments];
    ONSArray *otherAlignments = [other columnAlignments];
    NSInteger kind = [other intentKind];
    if (kind != [self intentKind] || (language != otherLanguage && ![language isEqual:otherLanguage])) {
        return NO;
    }
    NSInteger headerLevel = [other headerLevel];
    if (headerLevel != [self headerLevel]) {
        return NO;
    }
    NSInteger ordinal = [other ordinal];
    if (ordinal != [self ordinal]) {
        return NO;
    }
    NSInteger indentationLevel = [other indentationLevel];
    if (indentationLevel != [self indentationLevel]) {
        return NO;
    }
    if (alignments != otherAlignments) {
        otherAlignments = [other columnAlignments];
        if (![otherAlignments isEqual:[self columnAlignments]]) {
            return NO;
        }
    }
    NSInteger columnCount = [other columnCount];
    if (columnCount != [self columnCount]) {
        return NO;
    }
    NSInteger column = [other column];
    if (column != [self column]) {
        return NO;
    }
    NSInteger row = [other row];
    if (row != [self row]) {
        return NO;
    }
    return parent == otherParent || [parent isEquivalentToPresentationIntent:otherParent];
}

// Native -[NSPresentationIntent description], 0x1c068e0ec. Non-external.
// NSMutableArray at shared-cache slot 0x22227fb18 has medium confidence.
- (id)description {
    ONSMutableString *result = objc_autorelease([objc_alloc((Class)presentationIntentMutableStringClass)
        initWithFormat:(id)OF_STR("<%@ %p>"), objc_opt_class(self), self]);
    [result appendString:(id)OF_STR(": ")];
    id kinds[] = {
        (id)OF_STR("Paragraph"),
        (id)OF_STR("Header"),
        (id)OF_STR("OrderedList"),
        (id)OF_STR("UnorderedList"),
        (id)OF_STR("ListItem"),
        (id)OF_STR("CodeBlock"),
        (id)OF_STR("BlockQuote"),
        (id)OF_STR("ThematicBreak"),
        (id)OF_STR("Table"),
        (id)OF_STR("TableHeaderRow"),
        (id)OF_STR("TableRow"),
        (id)OF_STR("TableCell"),
    };
    NSUInteger kind = _intentKind;
    [result appendString:kinds[kind <= 11 ? kind : 0]];
    [result appendFormat:(id)OF_STR(" (id %ld)"), _identity];
    switch (_intentKind) {
        case 1:
            [result appendFormat:(id)OF_STR(" (%ld) indent %ld"), _headerLevel, _indentationLevel];
            break;
        case 2:
        case 3:
            [result appendFormat:(id)OF_STR(" indent %ld"), _indentationLevel];
            break;
        case 4:
            [result appendFormat:(id)OF_STR(" ordinal %ld, indent %ld"), _ordinal, _indentationLevel];
            break;
        case 5:
            [result appendFormat:(id)OF_STR(" (%@) indent %ld"),
                                 _languageHint ?: (id)OF_STR("no hint"),
                                 _indentationLevel];
            break;
        case 8: {
            ONSArray *alignments = _columnAlignments;
            NSInteger columnCount = _columnCount;
            id description = (id)OF_STR("none");
            if (alignments != nil && [alignments count] != 0) {
                ONSMutableArray *names = [(id)presentationIntentMutableArrayClass array];
                for (id alignment in alignments) {
                    id name;
                    if ([alignment isEqual:(id)&kAlignmentLeft]) {
                        name = (id)OF_STR("left");
                    } else if ([alignment isEqual:(id)&kAlignmentRight]) {
                        name = (id)OF_STR("right");
                    } else if ([alignment isEqual:(id)&kAlignmentCenter]) {
                        name = (id)OF_STR("center");
                    } else {
                        name = (id)OF_STR("unknown");
                    }
                    [names addObject:name];
                }
                description = [names componentsJoinedByString:(id)OF_STR(",")];
            }
            [result appendFormat:(id)OF_STR(" %ld columns, alignment %@"), columnCount, description];
            break;
        }
        case 10:
            [result appendFormat:(id)OF_STR(" # %ld"), _row];
            break;
        case 11:
            [result appendFormat:(id)OF_STR(" # %ld"), _column];
            break;
        default:
            break;
    }
    if (_parentIntent != nil) {
        [result appendString:(id)OF_STR("\n ")];
        [result appendString:[_parentIntent description]];
    }
    return result;
}

// Native -[NSPresentationIntent copyWithZone:], 0x1c068e444. Non-external.
- (id)copyWithZone:(struct _NSZone *)zone {
    return objc_retain(self);
}

// Native +[NSPresentationIntent supportsSecureCoding], 0x1c068e448. Non-external.
+ (BOOL)supportsSecureCoding {
    return YES;
}

// Native -[NSPresentationIntent initWithCoder:], 0x1c068e450. Non-external.
// NSArray at 0x22227fa90 and NSSet at 0x22227fb68 are shared-cache bindings (confidence: medium).
- (id)initWithCoder:(id<PresentationIntentCoder>)coder {
    self = [self _init];
    if (self != nil) {
        _intentKind = [coder decodeIntegerForKey:(id)OF_STR("NS.intentKind")];
        Class intentClass = objc_opt_class((id)presentationIntentClass);
        _parentIntent = objc_retain([coder decodeObjectOfClass:intentClass
                                                        forKey:(id)OF_STR("NS.parentIntent")]);
        _identity = [coder decodeIntegerForKey:(id)OF_STR("NS.identity")];
        _ordinal = [coder decodeIntegerForKey:(id)OF_STR("NS.ordinal")];
        id alignments;
        if ([coder requiresSecureCoding]) {
            Class numberClass = objc_opt_class((id)presentationIntentNumberClass);
            alignments = [coder decodeArrayOfObjectsOfClass:numberClass
                                                     forKey:(id)OF_STR("NS.columnAlignments")];
        } else {
            id classes[2];
            classes[0] = (id)objc_opt_class((id)presentationIntentArrayClass);
            classes[1] = (id)objc_opt_class((id)presentationIntentNumberClass);
            id array = [(id)presentationIntentArrayClass arrayWithObjects:classes count:2];
            id set = [(id)presentationIntentSetClass setWithArray:array];
            alignments = [coder decodeObjectOfClasses:set forKey:(id)OF_STR("NS.columnAlignments")];
        }
        _columnAlignments = objc_retain(alignments);
        _columnCount = [coder decodeIntegerForKey:(id)OF_STR("NS.columnCount")];
        _headerLevel = [coder decodeIntegerForKey:(id)OF_STR("NS.headerLevel")];
        Class stringClass = objc_opt_class((id)presentationIntentStringClass);
        _languageHint = objc_retain([coder decodeObjectOfClass:stringClass
                                                        forKey:(id)OF_STR("NS.languageHint")]);
        _column = [coder decodeIntegerForKey:(id)OF_STR("NS.column")];
        _row = [coder decodeIntegerForKey:(id)OF_STR("NS.row")];
        _indentationLevel = [coder decodeIntegerForKey:(id)OF_STR("NS.indentationLevel")];
    }
    return self;
}

// Native -[NSPresentationIntent encodeWithCoder:], 0x1c068e644. Non-external.
- (void)encodeWithCoder:(id<PresentationIntentCoder>)coder {
    [coder encodeInteger:[self intentKind] forKey:(id)OF_STR("NS.intentKind")];
    [coder encodeObject:[self parentIntent] forKey:(id)OF_STR("NS.parentIntent")];
    [coder encodeInteger:[self identity] forKey:(id)OF_STR("NS.identity")];
    [coder encodeInteger:[self ordinal] forKey:(id)OF_STR("NS.ordinal")];
    [coder encodeObject:[self columnAlignments] forKey:(id)OF_STR("NS.columnAlignments")];
    [coder encodeInteger:[self columnCount] forKey:(id)OF_STR("NS.columnCount")];
    [coder encodeInteger:[self headerLevel] forKey:(id)OF_STR("NS.headerLevel")];
    [coder encodeObject:[self languageHint] forKey:(id)OF_STR("NS.languageHint")];
    [coder encodeInteger:[self column] forKey:(id)OF_STR("NS.column")];
    [coder encodeInteger:[self row] forKey:(id)OF_STR("NS.row")];
    [coder encodeInteger:[self indentationLevel] forKey:(id)OF_STR("NS.indentationLevel")];
}

// Native +[NSPresentationIntent paragraphIntentWithIdentity:nestedInsideIntent:], 0x1c068e790.
// Non-external.
+ (id)paragraphIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent {
    ONSPresentationIntent *intent = [objc_alloc((Class)presentationIntentClass) _init];
    intent->_identity = identity;
    intent->_intentKind = 0;
    intent->_parentIntent = objc_retain(parent);
    return objc_autorelease(intent);
}

// Native +[NSPresentationIntent headerIntentWithIdentity:level:nestedInsideIntent:], 0x1c068e7e4.
// Non-external.
+ (id)headerIntentWithIdentity:(NSInteger)identity level:(NSInteger)level nestedInsideIntent:(id)parent {
    ONSPresentationIntent *intent = [objc_alloc((Class)presentationIntentClass) _init];
    intent->_identity = identity;
    intent->_intentKind = 1;
    intent->_headerLevel = level;
    intent->_indentationLevel = parent != nil ? [(ONSPresentationIntent *)parent indentationLevel] : 0;
    intent->_parentIntent = objc_retain(parent);
    return objc_autorelease(intent);
}

// Native +[NSPresentationIntent codeBlockIntentWithIdentity:languageHint:nestedInsideIntent:], 0x1c068e85c.
// Non-external.
+ (id)codeBlockIntentWithIdentity:(NSInteger)identity
                     languageHint:(id)language
               nestedInsideIntent:(id)parent {
    ONSPresentationIntent *intent = [objc_alloc((Class)presentationIntentClass) _init];
    intent->_identity = identity;
    intent->_intentKind = 5;
    intent->_languageHint = [language copy];
    intent->_indentationLevel = parent != nil ? [(ONSPresentationIntent *)parent indentationLevel] : 0;
    intent->_parentIntent = objc_retain(parent);
    return objc_autorelease(intent);
}

// Native +[NSPresentationIntent thematicBreakIntentWithIdentity:nestedInsideIntent:], 0x1c068e8dc.
// Non-external.
+ (id)thematicBreakIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent {
    ONSPresentationIntent *intent = [objc_alloc((Class)presentationIntentClass) _init];
    intent->_identity = identity;
    intent->_intentKind = 7;
    intent->_parentIntent = objc_retain(parent);
    return objc_autorelease(intent);
}

// Native +[NSPresentationIntent orderedListIntentWithIdentity:nestedInsideIntent:], 0x1c068e934.
// Non-external.
+ (id)orderedListIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent {
    ONSPresentationIntent *intent = [objc_alloc((Class)presentationIntentClass) _init];
    intent->_identity = identity;
    intent->_intentKind = 2;
    intent->_indentationLevel = parent != nil ? [(ONSPresentationIntent *)parent indentationLevel] : 0;
    intent->_parentIntent = objc_retain(parent);
    return objc_autorelease(intent);
}

// Native +[NSPresentationIntent unorderedListIntentWithIdentity:nestedInsideIntent:], 0x1c068e9a4.
// Non-external.
+ (id)unorderedListIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent {
    ONSPresentationIntent *intent = [objc_alloc((Class)presentationIntentClass) _init];
    intent->_identity = identity;
    intent->_intentKind = 3;
    intent->_indentationLevel = parent != nil ? [(ONSPresentationIntent *)parent indentationLevel] : 0;
    intent->_parentIntent = objc_retain(parent);
    return objc_autorelease(intent);
}

// Native +[NSPresentationIntent listItemIntentWithIdentity:ordinal:nestedInsideIntent:], 0x1c068ea14.
// Non-external.
+ (id)listItemIntentWithIdentity:(NSInteger)identity
                         ordinal:(NSInteger)ordinal
              nestedInsideIntent:(id)parent {
    ONSPresentationIntent *intent = [objc_alloc((Class)presentationIntentClass) _init];
    intent->_intentKind = 4;
    intent->_identity = identity;
    intent->_ordinal = ordinal;
    intent->_indentationLevel =
        parent != nil ? (NSInteger)((NSUInteger)[(ONSPresentationIntent *)parent indentationLevel] + 1) : 0;
    intent->_parentIntent = objc_retain(parent);
    return objc_autorelease(intent);
}

// Native +[NSPresentationIntent blockQuoteIntentWithIdentity:nestedInsideIntent:], 0x1c068ea8c.
// Non-external.
+ (id)blockQuoteIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent {
    ONSPresentationIntent *intent = [objc_alloc((Class)presentationIntentClass) _init];
    intent->_identity = identity;
    intent->_intentKind = 6;
    intent->_indentationLevel =
        parent != nil ? (NSInteger)((NSUInteger)[(ONSPresentationIntent *)parent indentationLevel] + 1) : 0;
    intent->_parentIntent = objc_retain(parent);
    return objc_autorelease(intent);
}

// Native +[NSPresentationIntent tableIntentWithIdentity:columnCount:alignments:nestedInsideIntent:],
// 0x1c068eb00. Non-external.
+ (id)tableIntentWithIdentity:(NSInteger)identity
                  columnCount:(NSInteger)count
                   alignments:(id)alignments
           nestedInsideIntent:(id)parent {
    if ((NSUInteger)count != [(ONSArray *)alignments count]) {
        id proem = _ONSMethodExceptionProem(self, _cmd);
        id reason = [(id)presentationIntentStringClass
            stringWithFormat:(id)OF_STR("%@: column count does not match count of alignments"), proem];
        objc_exception_throw([(id)presentationIntentExceptionClass
            exceptionWithName:(id)ONSInvalidArgumentException
                       reason:reason
                     userInfo:nil]);
    }
    ONSPresentationIntent *intent = [objc_alloc((Class)presentationIntentClass) _init];
    intent->_identity = identity;
    intent->_intentKind = 8;
    intent->_parentIntent = objc_retain(parent);
    intent->_columnCount = count;
    intent->_columnAlignments = [alignments copy];
    return objc_autorelease(intent);
}

// Native +[NSPresentationIntent tableRowIntentWithIdentity:row:nestedInsideIntent:], 0x1c068ebe4.
// Non-external.
+ (id)tableRowIntentWithIdentity:(NSInteger)identity row:(NSInteger)row nestedInsideIntent:(id)parent {
    ONSPresentationIntent *intent = [objc_alloc((Class)presentationIntentClass) _init];
    intent->_identity = identity;
    intent->_intentKind = 10;
    intent->_row = row;
    intent->_parentIntent = objc_retain(parent);
    return objc_autorelease(intent);
}

// Native +[NSPresentationIntent tableHeaderRowIntentWithIdentity:nestedInsideIntent:], 0x1c068ec44.
// Non-external.
+ (id)tableHeaderRowIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent {
    ONSPresentationIntent *intent = [objc_alloc((Class)presentationIntentClass) _init];
    intent->_identity = identity;
    intent->_intentKind = 9;
    intent->_parentIntent = objc_retain(parent);
    return objc_autorelease(intent);
}

// Native +[NSPresentationIntent tableCellIntentWithIdentity:column:nestedInsideIntent:], 0x1c068ec9c.
// Non-external.
+ (id)tableCellIntentWithIdentity:(NSInteger)identity column:(NSInteger)column nestedInsideIntent:(id)parent {
    ONSPresentationIntent *intent = [objc_alloc((Class)presentationIntentClass) _init];
    intent->_identity = identity;
    intent->_intentKind = 11;
    intent->_column = column;
    intent->_parentIntent = objc_retain(parent);
    return objc_autorelease(intent);
}

// Native -[NSPresentationIntent intentKind], 0x1c068ecfc. Non-external.
@synthesize intentKind = _intentKind;

// Native -[NSPresentationIntent parentIntent], 0x1c068ed04. Non-external.
@synthesize parentIntent = _parentIntent;

// Native -[NSPresentationIntent identity], 0x1c068ed10. Non-external.
@synthesize identity = _identity;

// Native -[NSPresentationIntent ordinal], 0x1c068ed18. Non-external.
@synthesize ordinal = _ordinal;

// Native -[NSPresentationIntent columnAlignments], 0x1c068ed20. Non-external.
@synthesize columnAlignments = _columnAlignments;

// Native -[NSPresentationIntent columnCount], 0x1c068ed28. Non-external.
@synthesize columnCount = _columnCount;

// Native -[NSPresentationIntent headerLevel], 0x1c068ed30. Non-external.
@synthesize headerLevel = _headerLevel;

// Native -[NSPresentationIntent languageHint], 0x1c068ed38. Non-external.
@synthesize languageHint = _languageHint;

// Native -[NSPresentationIntent column], 0x1c068ed44. Non-external.
@synthesize column = _column;

// Native -[NSPresentationIntent row], 0x1c068ed4c. Non-external.
@synthesize row = _row;

// Native -[NSPresentationIntent indentationLevel], 0x1c068ed54. Non-external.
@synthesize indentationLevel = _indentationLevel;

@end

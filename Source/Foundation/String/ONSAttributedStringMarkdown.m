//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSAttributedStringMarkdown.h"
#import "Foundation/Collections/ONSUnrestoredInterfaces.h"
#include "CoreFoundation/Collections/OCFNSCFDictionary.h"
#include "CoreFoundation/String/OCFString.h"

#include <Block.h>
#include <alloca.h>
#include <dispatch/dispatch.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// Inferred organization: the Markdown unit spans 0x1c0601568-0x1c0603233 of the iOS Simulator Foundation
// __text section: -[NSAttributedString(_NSAttributedStringFromMarkdown)
// initWithMarkdownString:options:baseURL:error:] (0x1c0601568), newAttributedStringFromMarkdown
// (0x1c060177c), _NSAttributedStringFromMarkdownWithCreator (0x1c0601d2c; the native functions between them
// are outside the scope), appendNodeText (0x1c0602e84), appendStringWithSourcePosition (0x1c0603024) and
// attributesForIntents (0x1c06030e0); the C functions follow the category's @implementation in native order.
// Objective-C compiled with manual reference counting (objc_retain, objc_release, objc_autorelease; no ARC
// entry points). Evidence: Local/Evidence/cf-core/iphone-simulator-foundation-link.json (family
// FC-sets-attributed). Unless a definition says otherwise: iOS Simulator 24A434, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified (assembly read in full). iOS Device 24A446:
// unread. Reading: Source/Foundation/Collections/FoundationSetsAttributedReading.md.
//
// Class references through the shared cache's GOT (slots outside the extracted image, identified from their
// use; confidence: medium): 0x22227FB18 NSMutableArray (+arrayWithObject:, +array), 0x22227FAD0 NSDictionary
// (+dictionaryWithObjects:forKeys:count:), 0x22227FAB8 NSData (-initWithBytesNoCopy:length:freeWhenDone:),
// 0x22227FB88 NSURL (-initWithString:relativeToURL:), and 0x2222807B8 the empty dictionary singleton
// __NSDictionary0__struct (loaded once and used as the object itself). The selector 0x2387B599C (the
// respondsToSelector: argument before -finalizeWithLanguageIdentifier:) lies outside the image and is read
// as finalizeWithLanguageIdentifier: (confidence: medium).

// Node types, iterator events, list types and delimiters of libcmark-gfm as the native code compares them
// (values from the assembly; the names are local descriptive spellings of the library's enumerators).
enum {
    markdownEventDone = 1,
    markdownEventEnter = 2,
    markdownEventExit = 3,
};

enum {
    markdownNodeBlockQuote = 0x8002,
    markdownNodeList = 0x8003,
    markdownNodeItem = 0x8004,
    markdownNodeCodeBlock = 0x8005,
    markdownNodeHTMLBlock = 0x8006,
    markdownNodeCustomBlock = 0x8007,
    markdownNodeHeading = 0x8009,
    markdownNodeThematicBreak = 0x800A,
    markdownNodeText = 0xC001,
    markdownNodeSoftBreak = 0xC002,
    markdownNodeLineBreak = 0xC003,
    markdownNodeCode = 0xC004,
    markdownNodeHTMLInline = 0xC005,
    markdownNodeCustomInline = 0xC006,
    markdownNodeEmphasis = 0xC007,
    markdownNodeStrong = 0xC008,
    markdownNodeLink = 0xC009,
    markdownNodeImage = 0xC00A,
    markdownNodeAttribute = 0xC00C,
};

// The inline presentation intent bits the unit sets (NSInlinePresentationIntent: emphasized 1, strongly
// emphasized 2, code 4, strikethrough 0x20, soft break 0x40, line break 0x80, inline HTML 0x100, block HTML
// 0x200).

// The UTF-8 offset cache -[NSAttributedStringMarkdownSourcePosition setOffsetsFromUTF8:size:usingCache:]
// reads and updates (48 bytes on the creator's stack, the first four words zero and the last two 1 at the
// start; the fields belong to that method's class, outside this family, and are unread).
// MarkdownOffsetCache is a local descriptive name.

extern char errorClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSError));
extern char stringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSString));
extern char mutableStringClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableString));
extern char numberClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSNumber));
extern char dictionaryClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSDictionary));
extern char mutableArrayClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSMutableArray));
extern char dataClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSData));
extern char urlClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSURL));
extern char presentationIntentClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSPresentationIntent));
extern char
    parsingOptionsClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSAttributedStringMarkdownParsingOptions));
extern char
    sourcePositionClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSAttributedStringMarkdownSourcePosition));
extern char creatorConcreteClass[] __asm(
    "_OBJC_CLASS_$_" OF_NAME_STRING(_ONSAttributedStringFromMarkdownCreatorConcrete)
);
extern char constantIntegerNumberClass[] __asm("_OBJC_CLASS_$_" OF_NAME_STRING(ONSConstantIntegerNumber));

// The messages the unit sends (to the creator, the parsing options, presentation intents, source positions,
// strings, collections, URLs, data and errors), declared for the sends only; no protocol object is emitted.
@protocol MarkdownMessages <NSObject>

- (id)result;
- (void)appendString:(id)string with:(id)attributes;
- (void)finalizeWithLanguageIdentifier:(id)languageIdentifier;
- (id)parseExtendedAttributesFromData:(id)data error:(id *)error;
- (NSInteger)interpretedSyntax;
- (id)languageCode;
- (BOOL)appliesSourcePositionAttributes;
- (BOOL)allowsExtendedAttributes;
- (NSInteger)failurePolicy;
+ (id)paragraphIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent;
+ (id)blockQuoteIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent;
+ (id)orderedListIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent;
+ (id)unorderedListIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent;
+ (id)listItemIntentWithIdentity:(NSInteger)identity ordinal:(NSInteger)ordinal nestedInsideIntent:(id)parent;
+ (id)codeBlockIntentWithIdentity:(NSInteger)identity
                     languageHint:(id)languageHint
               nestedInsideIntent:(id)parent;
+ (id)headerIntentWithIdentity:(NSInteger)identity level:(NSInteger)level nestedInsideIntent:(id)parent;
+ (id)thematicBreakIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent;
+ (id)tableIntentWithIdentity:(NSInteger)identity
                  columnCount:(NSInteger)columnCount
                   alignments:(id)alignments
           nestedInsideIntent:(id)parent;
+ (id)tableHeaderRowIntentWithIdentity:(NSInteger)identity nestedInsideIntent:(id)parent;
+ (id)tableRowIntentWithIdentity:(NSInteger)identity row:(NSInteger)row nestedInsideIntent:(id)parent;
+ (id)tableCellIntentWithIdentity:(NSInteger)identity column:(NSInteger)column nestedInsideIntent:(id)parent;
- (id)parentIntent;
- (id)initWithStartLine:(NSInteger)startLine
            startColumn:(NSInteger)startColumn
                endLine:(NSInteger)endLine
              endColumn:(NSInteger)endColumn;
- (void)setOffsetsFromUTF8:(const char *)bytes size:(size_t)size usingCache:(MarkdownOffsetCache *)cache;
- (id)initWithBytesNoCopy:(const void *)bytes
                   length:(NSUInteger)length
                 encoding:(NSUInteger)encoding
             freeWhenDone:(BOOL)freeWhenDone;
- (id)initWithBytes:(const void *)bytes length:(NSUInteger)length encoding:(NSUInteger)encoding;
- (id)initWithUTF8String:(const char *)string;
- (id)initWithBytesNoCopy:(const void *)bytes length:(NSUInteger)length freeWhenDone:(BOOL)freeWhenDone;
- (id)initWithString:(id)string relativeToURL:(id)baseURL;
- (NSUInteger)length;
- (unsigned short)characterAtIndex:(NSUInteger)index;
- (void)replaceCharactersInRange:(ONSRange)range withString:(id)string;
- (void)appendString:(id)string;
- (NSUInteger)count;
- (id)lastObject;
- (void)addObject:(id)object;
- (void)removeLastObject;
- (void)setObject:(id)object forKey:(id)key;
- (void)setObject:(id)object forKeyedSubscript:(id)key;
- (void)addEntriesFromDictionary:(id)dictionary;
+ (id)array;
+ (id)arrayWithObject:(id)object;
+ (id)numberWithUnsignedInteger:(NSUInteger)value;
+ (id)dictionaryWithObjects:(const id *)objects forKeys:(const id *)keys count:(NSUInteger)count;
+ (id)errorWithDomain:(id)domain code:(NSInteger)code userInfo:(id)userInfo;

@end

// ICU (libicucore, linked by the native image), supplied: u_hasBinaryProperty with UCHAR_BIDI_CONTROL (2).
// UChar32 and UProperty are int-sized; UBool is int8_t.
extern int8_t u_hasBinaryProperty(int32_t c, int32_t which);

// The NSConstantIntegerNumber literals the table alignments are built from (__DATA_CONST,__objc_intobj,
// read with Local/Scratch/cf-core-fc/fmacho.py): {the class, the encoding "q", the value}. 0x22288B1F0 is
// 0 (left, the default), 0x22288B1C0 is 1 ('c', center) and 0x22288B1D8 is 2 ('r', right). The records are
// emitted as plain constant data here (the section is the compiler's choice for literals).
// MarkdownConstantIntegerNumber is a local descriptive name.
typedef struct {
    const void *isa;
    const char *encoding;
    long long value;
} MarkdownConstantIntegerNumber;

static const MarkdownConstantIntegerNumber markdownAlignmentLeft = {constantIntegerNumberClass, "q", 0};
static const MarkdownConstantIntegerNumber markdownAlignmentCenter = {constantIntegerNumberClass, "q", 1};
static const MarkdownConstantIntegerNumber markdownAlignmentRight = {constantIntegerNumberClass, "q", 2};

// The UTF-16 constant strings of the thematic break ("⸻", 0x222837CD0) and the image placeholder
// ("￼", 0x222837CF0), flags 0x7d0; OF_STR spells ASCII literals only.
static const uint16_t markdownThreeEmDashCharacters[] = {0x2E3B, 0};
static const uint16_t markdownObjectReplacementCharacters[] = {0xFFFC, 0};

static const OCFConstantString markdownThreeEmDash = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7d0, markdownThreeEmDashCharacters, 1
};

static const OCFConstantString markdownObjectReplacement = {
    (uintptr_t)__OCFConstantStringClassReference, 0x7d0, markdownObjectReplacementCharacters, 1
};

// The global block of the creator's dispatch_once (Block ABI; local descriptive type names).
// Native _NSAttributedStringFromMarkdownWithCreator.onceToken, 0x22e6fcce0 (__DATA,__bss, non-external).
static dispatch_once_t markdownCreatorOnceToken;

static id
newAttributedStringFromMarkdown(const char *bytes, size_t length, id options, id baseURL, id *error);
static BOOL appendNodeText(
    id creator,
    id options,
    cmark_node *node,
    NSUInteger inlineIntent,
    id intent,
    id attributes,
    const char *bytes,
    size_t length,
    MarkdownOffsetCache *cache,
    id *error
);
static void appendStringWithSourcePosition(
    id creator, id string, NSUInteger inlineIntent, id intent, id attributes, id position
);
static id attributesForIntents(NSUInteger inlineIntent, id intent, id attributes);

@implementation ONSAttributedString (_NSAttributedStringFromMarkdown)

// Native -[NSAttributedString(_NSAttributedStringFromMarkdown)
// initWithMarkdownString:options:baseURL:error:], 0x1c0601568. Self is released first; the result comes from
// newAttributedStringFromMarkdown (+1). The string's UTF-8 length is measured with CFStringGetBytes
// (kCFStringEncodingUTF8, 0x08000100, no loss byte, not external); a result below 1 answers nil without an
// error. Up to 0x80 bytes go to a zeroed stack buffer (rounded up to 16 bytes), more to malloc_type_malloc
// (type 0x100004077774924, freed after). A second conversion that yields nothing answers nil with
// NSCocoaErrorDomain 259 (NSFileReadCorruptFileError) stored through error when it is not NULL.
- (id)initWithMarkdownString:(id)markdownString options:(id)options baseURL:(id)baseURL error:(id *)error {
    objc_release((id)(self));
    OCFIndex stringLength = (OCFIndex)[(id<MarkdownMessages>)markdownString length];
    OCFIndex usedLength = 0;
    if (OCFStringGetBytes(
            (OCFStringRef)markdownString,
            (OCFRange){0, stringLength},
            0x08000100,
            0,
            false,
            NULL,
            0,
            &usedLength
        ) < 1) {
        return nil;
    }
    OCFIndex bufferLength = usedLength;
    if ((uint64_t)bufferLength > 0x80) {
        uint8_t *buffer = malloc_type_malloc((size_t)bufferLength, 0x100004077774924ULL);
        if (OCFStringGetBytes(
                (OCFStringRef)markdownString,
                (OCFRange){0, stringLength},
                0x08000100,
                0,
                false,
                buffer,
                usedLength,
                NULL
            ) <= 0) {
            free(buffer);
            goto failed;
        }
        id result = newAttributedStringFromMarkdown(
            (const char *)buffer, (size_t)usedLength, options, baseURL, error
        );
        free(buffer);
        return result;
    }
    uint8_t *buffer;
    if (bufferLength != 0) {
        buffer = alloca(((size_t)bufferLength + 15) & ~(size_t)15);
        bzero(buffer, (size_t)bufferLength);
    } else {
        buffer = NULL;
    }
    if (OCFStringGetBytes(
            (OCFStringRef)markdownString,
            (OCFRange){0, stringLength},
            0x08000100,
            0,
            false,
            buffer,
            bufferLength,
            NULL
        ) >= 1) {
        return newAttributedStringFromMarkdown(
            (const char *)buffer, (size_t)usedLength, options, baseURL, error
        );
    }
failed:
    if (error == NULL) {
        return nil;
    }
    *error = [(id<MarkdownMessages>)(id)errorClass errorWithDomain:ONSCocoaErrorDomain code:259 userInfo:nil];
    return nil;
}

@end

// Native newAttributedStringFromMarkdown, 0x1c060177c. Non-external. A
// _NSAttributedStringFromMarkdownCreatorConcrete (objc_alloc_init) is filled by
// _NSAttributedStringFromMarkdownWithCreator; on success its -result retained (+1), else nil. The creator is
// released.
static id
newAttributedStringFromMarkdown(const char *bytes, size_t length, id options, id baseURL, id *error) {
    id creator = objc_alloc_init((Class)((Class)creatorConcreteClass));
    id result;
    if (_ONSAttributedStringFromMarkdownWithCreator(creator, bytes, length, options, baseURL, error)) {
        result = objc_retain((id)([(id<MarkdownMessages>)creator result]));
    } else {
        result = nil;
    }
    objc_release((id)(creator));
    return result;
}

// Native _NSAttributedStringFromMarkdownWithCreator, 0x1c0601d2c. External.
//
// The once block runs first (unread invoke). The parser options are 0x40000 for interpreted syntax 1 and
// 0xC0000 for 2, else 0; the "table", "autolink", "strikethrough" and "extended-attrs" extensions are
// attached when found. A NULL document answers NO with NSCocoaErrorDomain 259 stored through error when it
// is not NULL. Otherwise nil options become a new autoreleased NSAttributedStringMarkdownParsingOptions, the
// attribute stack starts as an NSMutableArray holding the empty dictionary, the offset cache as {0, 0, 0, 0,
// 1, 1} and the ten list ordinals as 1, and the iterator's events are walked:
// - custom block and paragraph (only for interpreted syntax 0, full): enter makes a paragraph intent, exit
//   returns to the parent;
// - block quote, list item, heading (levels 1-6 only), table, table row and table cell: enter makes the
//   intent (an item takes the current list's ordinal; a row its row number, resetting the column; a cell
//   its column), exit returns to the parent (an item also advances its list's ordinal);
// - list: enter pushes the top attributes with "NSListItemDelimiter" (")" or "." for an ordered list by its
//   delimiter, "*", "+" or "-" for a bullet list by its marker, none otherwise), opens a list level (up to
//   nine; its ordinal is the list's start for an ordered list, else 1) and makes the ordered or unordered
//   list intent (none for another list type); exit closes the level (not below 0), returns to the parent
//   and pops the attributes;
// - code block: a code block intent with the fence info (nil when empty) as the language hint, its text
//   appended, then the parent;
// - thematic break: "⸻" with a thematic break intent, then the parent;
// - text, custom inline, code (0x4), inline HTML (0x100) and HTML block (0x200): the node's text appended;
// - soft and line breaks: " " (0x40) and "\n" (0x80);
// - emphasis (0x1), strong (0x2) and strikethrough (0x20) set the bit on enter and clear it on exit;
// - link and image (enter only): the top attributes plus "NSLink" or "NSImageURL" (an NSURL of the URL
//   relative to the base URL, after removing the Bidi_Control characters; only for a non-empty URL) and
//   "NSAlternateDescription" (the title, when not empty) are pushed; the node is marked (user data 1) and the
//   events up to its exit are consumed, collecting the text of the entered or leaf nodes and the source
//   range from the first node's start to the last node's end; an image without text appends "￼",
//   otherwise the text is appended (with an NSAttributedStringMarkdownSourcePosition when source positions
//   apply and every coordinate is at least 1); the attributes are popped;
// - extended attributes (only when allowed): enter pushes the top attributes plus the parsed attributes,
//   or, when parsing fails, the top attributes again (a nonzero failure policy) or answers NO with the
//   parser's error (policy 0); exit pops.
// Each new intent takes the next identity (starting at 1). At the end the creator is sent
// -finalizeWithLanguageIdentifier: with the options' language code when it responds, and the answer is YES.
// The iterator, the document and the parser are freed on every path after the document exists.
BOOL _ONSAttributedStringFromMarkdownWithCreator(
    id creator, const char *bytes, size_t length, id options, id baseURL, id *error
) {
    // Native ____NSAttributedStringFromMarkdownWithCreator_block_invoke, 0x1c0602df4. Non-external.
    // The native tail branch goes through libcmark-gfm's delay-init stub.
    dispatch_once(&markdownCreatorOnceToken, ^{
      cmark_gfm_core_extensions_ensure_registered();
    });
    NSInteger syntax = [(id<MarkdownMessages>)options interpretedSyntax];
    int parserOptions = syntax == 1 ? 0x40000 : 0;
    if (syntax == 2) {
        parserOptions = 0xC0000;
    }
    cmark_parser *parser = cmark_parser_new(parserOptions);
    cmark_syntax_extension *extension = cmark_find_syntax_extension("table");
    if (extension != NULL) {
        cmark_parser_attach_syntax_extension(parser, extension);
    }
    extension = cmark_find_syntax_extension("autolink");
    if (extension != NULL) {
        cmark_parser_attach_syntax_extension(parser, extension);
    }
    extension = cmark_find_syntax_extension("strikethrough");
    if (extension != NULL) {
        cmark_parser_attach_syntax_extension(parser, extension);
    }
    extension = cmark_find_syntax_extension("extended-attrs");
    if (extension != NULL) {
        cmark_parser_attach_syntax_extension(parser, extension);
    }
    cmark_parser_feed(parser, bytes, length);
    cmark_node *document = cmark_parser_finish(parser);
    if (document == NULL) {
        // Foundation defect: natively the parser is not freed on this path (0x1c0601f14 returns without
        // cmark_parser_free; confidence: verified), an unintended leak. Divergence: the parser is freed
        // before the error is stored.
        cmark_parser_free(parser);
        if (error != NULL) {
            *error = [(id<MarkdownMessages>)(id)errorClass errorWithDomain:ONSCocoaErrorDomain
                                                                      code:259
                                                                  userInfo:nil];
        }
        return NO;
    }
    cmark_iter *iter = cmark_iter_new(document);
    if (options == nil) {
        options = objc_autorelease((id)(objc_alloc_init((Class)((Class)parsingOptionsClass))));
    }
    id stack = [(id<MarkdownMessages>)(id)mutableArrayClass arrayWithObject:(id)&__ONSDictionary0__struct];
    MarkdownOffsetCache cache = {{0, 0, 0, 0, 1, 1}};
    NSInteger ordinals[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    BOOL result;
    int event = cmark_iter_next(iter);
    if (event != markdownEventDone) {
        NSUInteger inlineIntent = 0;
        id intent = nil;
        NSUInteger depth = 0;
        NSInteger column = 0;
        NSInteger row = 0;
        NSInteger identity = 1;
        id presentationIntent = (id)presentationIntentClass;
        do {
            cmark_node *node = cmark_iter_get_node(iter);
            int type = cmark_node_get_type(node);
            if ((uint32_t)(type - markdownNodeCustomBlock) <= 1) {
                if ([(id<MarkdownMessages>)options interpretedSyntax] == 0) {
                    if (event == markdownEventExit) {
                        intent = [(id<MarkdownMessages>)intent parentIntent];
                    } else if (event == markdownEventEnter) {
                        intent =
                            [(id<MarkdownMessages>)presentationIntent paragraphIntentWithIdentity:identity
                                                                               nestedInsideIntent:intent];
                        identity++;
                    }
                }
            } else if (type == markdownNodeBlockQuote) {
                if (event == markdownEventExit) {
                    intent = [(id<MarkdownMessages>)intent parentIntent];
                } else if (event == markdownEventEnter) {
                    intent = [(id<MarkdownMessages>)presentationIntent blockQuoteIntentWithIdentity:identity
                                                                                 nestedInsideIntent:intent];
                    identity++;
                }
            } else if (type == markdownNodeList) {
                if (event == markdownEventExit) {
                    depth = depth == 0 ? 0 : depth - 1;
                    intent = [(id<MarkdownMessages>)intent parentIntent];
                    [(id<MarkdownMessages>)stack removeLastObject];
                } else if (event == markdownEventEnter) {
                    int listType = cmark_node_get_list_type(node);
                    id delimiter = nil;
                    BOOL noDelimiter = YES;
                    if (listType == 2) {
                        int delim = cmark_node_get_list_delim(node);
                        if (delim == 2) {
                            delimiter = (id)OF_STR(")");
                            noDelimiter = NO;
                        } else if (delim == 1) {
                            delimiter = (id)OF_STR(".");
                            noDelimiter = NO;
                        }
                    } else if (listType == 1) {
                        int marker = cmark_node_get_list_marker(node);
                        if (marker == 3) {
                            delimiter = (id)OF_STR("*");
                            noDelimiter = NO;
                        } else if (marker == 2) {
                            delimiter = (id)OF_STR("+");
                            noDelimiter = NO;
                        } else if (marker == 1) {
                            delimiter = (id)OF_STR("-");
                            noDelimiter = NO;
                        }
                    }
                    id attributes = [[(id<MarkdownMessages>)stack lastObject] mutableCopy];
                    if (!noDelimiter) {
                        [(id<MarkdownMessages>)attributes setObject:delimiter
                                                  forKeyedSubscript:(id)OF_STR("NSListItemDelimiter")];
                    }
                    [(id<MarkdownMessages>)stack addObject:attributes];
                    objc_release((id)(attributes));
                    if (depth <= 8) {
                        depth++;
                        if (listType == 2) {
                            ordinals[depth] = (NSInteger)cmark_node_get_list_start(node);
                        } else {
                            ordinals[depth] = 1;
                        }
                    }
                    if (listType == 2) {
                        intent =
                            [(id<MarkdownMessages>)presentationIntent orderedListIntentWithIdentity:identity
                                                                                 nestedInsideIntent:intent];
                        identity++;
                    } else if (listType == 1) {
                        intent =
                            [(id<MarkdownMessages>)presentationIntent unorderedListIntentWithIdentity:identity
                                                                                   nestedInsideIntent:intent];
                        identity++;
                    }
                }
            } else if (type == markdownNodeItem) {
                if (event == markdownEventExit) {
                    intent = [(id<MarkdownMessages>)intent parentIntent];
                    ordinals[depth]++;
                } else if (event == markdownEventEnter) {
                    intent =
                        [(id<MarkdownMessages>)presentationIntent listItemIntentWithIdentity:identity
                                                                                     ordinal:ordinals[depth]
                                                                          nestedInsideIntent:intent];
                    identity++;
                }
            } else if (type == markdownNodeCodeBlock) {
                const char *fenceInfo = cmark_node_get_fence_info(node);
                id languageHint = nil;
                if (fenceInfo != NULL && fenceInfo[0] != 0) {
                    languageHint = [(id<MarkdownMessages>)objc_alloc((Class)((Class)stringClass))
                        initWithUTF8String:fenceInfo];
                }
                id codeIntent =
                    [(id<MarkdownMessages>)presentationIntent codeBlockIntentWithIdentity:identity
                                                                             languageHint:languageHint
                                                                       nestedInsideIntent:intent];
                objc_release((id)(languageHint));
                if (!appendNodeText(
                        creator,
                        options,
                        node,
                        inlineIntent,
                        codeIntent,
                        [(id<MarkdownMessages>)stack lastObject],
                        bytes,
                        length,
                        &cache,
                        error
                    )) {
                    goto failure;
                }
                identity++;
                intent = [(id<MarkdownMessages>)codeIntent parentIntent];
            } else if (type == markdownNodeHTMLBlock) {
                if (!appendNodeText(
                        creator,
                        options,
                        node,
                        inlineIntent | 0x200,
                        intent,
                        [(id<MarkdownMessages>)stack lastObject],
                        bytes,
                        length,
                        &cache,
                        error
                    )) {
                    goto failure;
                }
            } else if (type == markdownNodeHeading) {
                int level = cmark_node_get_heading_level(node);
                if ((uint32_t)(level - 1) <= 5) {
                    if (event == markdownEventExit) {
                        intent = [(id<MarkdownMessages>)intent parentIntent];
                    } else if (event == markdownEventEnter) {
                        intent = [(id<MarkdownMessages>)presentationIntent
                            headerIntentWithIdentity:identity
                                               level:(NSInteger)(uint32_t)level
                                  nestedInsideIntent:intent];
                        identity++;
                    }
                }
            } else if (type == markdownNodeThematicBreak) {
                id breakIntent =
                    [(id<MarkdownMessages>)presentationIntent thematicBreakIntentWithIdentity:identity
                                                                           nestedInsideIntent:intent];
                [(id<MarkdownMessages>)creator
                    appendString:(id)&markdownThreeEmDash
                            with:attributesForIntents(
                                     inlineIntent, breakIntent, [(id<MarkdownMessages>)stack lastObject]
                                 )];
                intent = [(id<MarkdownMessages>)breakIntent parentIntent];
                identity++;
            } else if (type == markdownNodeText || type == markdownNodeCustomInline) {
                if (!appendNodeText(
                        creator,
                        options,
                        node,
                        inlineIntent,
                        intent,
                        [(id<MarkdownMessages>)stack lastObject],
                        bytes,
                        length,
                        &cache,
                        error
                    )) {
                    goto failure;
                }
            } else if (type == markdownNodeSoftBreak) {
                [(id<MarkdownMessages>)creator
                    appendString:(id)OF_STR(" ")
                            with:attributesForIntents(
                                     inlineIntent | 0x40, intent, [(id<MarkdownMessages>)stack lastObject]
                                 )];
                inlineIntent &= ~(NSUInteger)0x40;
            } else if (type == markdownNodeLineBreak) {
                [(id<MarkdownMessages>)creator
                    appendString:(id)OF_STR("\n")
                            with:attributesForIntents(
                                     inlineIntent | 0x80, intent, [(id<MarkdownMessages>)stack lastObject]
                                 )];
                inlineIntent &= ~(NSUInteger)0x80;
            } else if (type == markdownNodeCode) {
                if (!appendNodeText(
                        creator,
                        options,
                        node,
                        inlineIntent | 0x4,
                        intent,
                        [(id<MarkdownMessages>)stack lastObject],
                        bytes,
                        length,
                        &cache,
                        error
                    )) {
                    goto failure;
                }
            } else if (type == markdownNodeHTMLInline) {
                if (!appendNodeText(
                        creator,
                        options,
                        node,
                        inlineIntent | 0x100,
                        intent,
                        [(id<MarkdownMessages>)stack lastObject],
                        bytes,
                        length,
                        &cache,
                        error
                    )) {
                    goto failure;
                }
            } else if (type == markdownNodeEmphasis) {
                if (event == markdownEventExit) {
                    inlineIntent &= ~(NSUInteger)0x1;
                } else if (event == markdownEventEnter) {
                    inlineIntent |= 0x1;
                }
            } else if (type == markdownNodeStrong) {
                if (event == markdownEventExit) {
                    inlineIntent &= ~(NSUInteger)0x2;
                } else if (event == markdownEventEnter) {
                    inlineIntent |= 0x2;
                }
            } else if (type == CMARK_NODE_STRIKETHROUGH) {
                if (event == markdownEventExit) {
                    inlineIntent &= ~(NSUInteger)0x20;
                } else if (event == markdownEventEnter) {
                    inlineIntent |= 0x20;
                }
            } else if ((uint32_t)(type - markdownNodeLink) <= 1) {
                if (event == markdownEventEnter) {
                    id attributes = [[(id<MarkdownMessages>)stack lastObject] mutableCopy];
                    const char *url = cmark_node_get_url(node);
                    if (url != NULL) {
                        id urlString = [(id<MarkdownMessages>)objc_alloc((Class)((Class)stringClass))
                            initWithBytesNoCopy:url
                                         length:strlen(url)
                                       encoding:4
                                   freeWhenDone:NO];
                        if ([(id<MarkdownMessages>)urlString length] != 0) {
                            id cleaned = nil;
                            if ([(id<MarkdownMessages>)urlString length] != 0) {
                                NSUInteger removed = 0;
                                NSUInteger index = 0;
                                do {
                                    if (u_hasBinaryProperty(
                                            [(id<MarkdownMessages>)urlString characterAtIndex:index], 2
                                        ) != 0) {
                                        if (cleaned == nil) {
                                            cleaned = [urlString mutableCopy];
                                        }
                                        [(id<MarkdownMessages>)cleaned
                                            replaceCharactersInRange:(ONSRange){index - removed, 1}
                                                          withString:(id)OF_STR("")];
                                        removed++;
                                    }
                                    index++;
                                } while (index < [(id<MarkdownMessages>)urlString length]);
                            }
                            if (cleaned == nil) {
                                cleaned = objc_retain((id)(urlString));
                            }
                            id linkURL = [(id<MarkdownMessages>)objc_alloc((Class)((Class)urlClass))
                                initWithString:cleaned
                                 relativeToURL:baseURL];
                            if (linkURL != nil) {
                                [(id<MarkdownMessages>)attributes
                                    setObject:linkURL
                                       forKey:type == markdownNodeImage ? (id)OF_STR("NSImageURL")
                                                                        : (id)OF_STR("NSLink")];
                                objc_release((id)(linkURL));
                            }
                            objc_release((id)(cleaned));
                        }
                        objc_release((id)(urlString));
                    }
                    const char *title = cmark_node_get_title(node);
                    if (title != NULL) {
                        id titleString = [(id<MarkdownMessages>)objc_alloc((Class)((Class)stringClass))
                            initWithBytes:title
                                   length:strlen(title)
                                 encoding:4];
                        if ([(id<MarkdownMessages>)titleString length] != 0) {
                            [(id<MarkdownMessages>)attributes setObject:titleString
                                                                 forKey:(id)OF_STR("NSAlternateDescription")];
                        }
                        objc_release((id)(titleString));
                    }
                    [(id<MarkdownMessages>)stack addObject:attributes];
                    objc_release((id)(attributes));
                    id text = objc_alloc_init((Class)((Class)mutableStringClass));
                    cmark_node_set_user_data(node, (void *)1);
                    int startLine = -1;
                    int startColumn = -1;
                    int endLine = -1;
                    int endColumn = -1;
                    int innerEvent = cmark_iter_next(iter);
                    while (innerEvent != markdownEventDone) {
                        cmark_node *inner = cmark_iter_get_node(iter);
                        if (startLine == -1) {
                            startLine = cmark_node_get_start_line(inner);
                            startColumn = cmark_node_get_start_column(inner);
                        }
                        if (cmark_node_get_user_data(inner) == (void *)1) {
                            break;
                        }
                        endLine = cmark_node_get_end_line(inner);
                        endColumn = cmark_node_get_end_column(inner);
                        if (innerEvent != markdownEventExit) {
                            const char *literal = cmark_node_get_literal(inner);
                            if (literal != NULL) {
                                id literalString =
                                    [(id<MarkdownMessages>)objc_alloc((Class)((Class)stringClass))
                                        initWithBytesNoCopy:literal
                                                     length:strlen(literal)
                                                   encoding:4
                                               freeWhenDone:NO];
                                if ([(id<MarkdownMessages>)literalString length] != 0) {
                                    [(id<MarkdownMessages>)text appendString:literalString];
                                }
                                objc_release((id)(literalString));
                            }
                        }
                        innerEvent = cmark_iter_next(iter);
                    }
                    NSUInteger textLength = [(id<MarkdownMessages>)text length];
                    if (type == markdownNodeImage && textLength == 0) {
                        [(id<MarkdownMessages>)creator
                            appendString:(id)&markdownObjectReplacement
                                    with:attributesForIntents(
                                             inlineIntent, intent, [(id<MarkdownMessages>)stack lastObject]
                                         )];
                    } else if (
                        [(id<MarkdownMessages>)options appliesSourcePositionAttributes] && startLine >= 1 &&
                        startColumn >= 1 && endLine >= 1 && endColumn >= 1
                    ) {
                        id position = [(id<MarkdownMessages>)objc_alloc((Class)((Class)sourcePositionClass))
                            initWithStartLine:(NSInteger)(uint32_t)startLine
                                  startColumn:(NSInteger)(uint32_t)startColumn
                                      endLine:(NSInteger)(uint32_t)endLine
                                    endColumn:(NSInteger)(uint32_t)endColumn];
                        [(id<MarkdownMessages>)position setOffsetsFromUTF8:bytes
                                                                      size:length
                                                                usingCache:&cache];
                        appendStringWithSourcePosition(
                            creator,
                            text,
                            inlineIntent,
                            intent,
                            [(id<MarkdownMessages>)stack lastObject],
                            position
                        );
                        objc_release((id)(position));
                    } else {
                        [(id<MarkdownMessages>)creator
                            appendString:text
                                    with:attributesForIntents(
                                             inlineIntent, intent, [(id<MarkdownMessages>)stack lastObject]
                                         )];
                    }
                    objc_release((id)(text));
                    [(id<MarkdownMessages>)stack removeLastObject];
                }
            } else if (type == markdownNodeAttribute) {
                if ([(id<MarkdownMessages>)options allowsExtendedAttributes]) {
                    if (event == markdownEventExit) {
                        [(id<MarkdownMessages>)stack removeLastObject];
                    } else if (event == markdownEventEnter) {
                        const char *attributeText = cmark_node_get_attributes(node);
                        id data = [(id<MarkdownMessages>)objc_alloc((Class)((Class)dataClass))
                            initWithBytesNoCopy:attributeText
                                         length:strlen(attributeText)
                                   freeWhenDone:NO];
                        id parseError = nil;
                        id parsed =
                            [(id<MarkdownMessages>)creator parseExtendedAttributesFromData:data
                                                                                     error:&parseError];
                        objc_release((id)(data));
                        if (parsed != nil) {
                            id attributes = [[(id<MarkdownMessages>)stack lastObject] mutableCopy];
                            [(id<MarkdownMessages>)attributes addEntriesFromDictionary:parsed];
                            [(id<MarkdownMessages>)stack addObject:attributes];
                            objc_release((id)(attributes));
                        } else if ([(id<MarkdownMessages>)options failurePolicy] != 0) {
                            [(id<MarkdownMessages>)stack addObject:[(id<MarkdownMessages>)stack lastObject]];
                        } else {
                            if (error != NULL) {
                                *error = parseError;
                            }
                            goto failure;
                        }
                    }
                }
            } else if (type == CMARK_NODE_TABLE) {
                if (event == markdownEventExit) {
                    intent = [(id<MarkdownMessages>)intent parentIntent];
                } else if (event == markdownEventEnter) {
                    uint16_t columns = cmark_gfm_extensions_get_table_columns(node);
                    uint8_t *alignments = cmark_gfm_extensions_get_table_alignments(node);
                    id alignmentObjects = nil;
                    if (alignments != NULL && columns != 0) {
                        alignmentObjects = [(id<MarkdownMessages>)(id)mutableArrayClass array];
                        for (NSUInteger remaining = columns; remaining != 0; remaining--) {
                            uint8_t alignment = *alignments++;
                            const MarkdownConstantIntegerNumber *object =
                                alignment == 'r' ? &markdownAlignmentRight : &markdownAlignmentLeft;
                            if (alignment == 'c') {
                                object = &markdownAlignmentCenter;
                            }
                            [(id<MarkdownMessages>)alignmentObjects addObject:(id)object];
                        }
                    }
                    intent =
                        [(id<MarkdownMessages>)presentationIntent tableIntentWithIdentity:identity
                                                                              columnCount:(NSInteger)columns
                                                                               alignments:alignmentObjects
                                                                       nestedInsideIntent:intent];
                    row = 0;
                    identity++;
                }
            } else if (type == CMARK_NODE_TABLE_ROW) {
                if (event == markdownEventExit) {
                    intent = [(id<MarkdownMessages>)intent parentIntent];
                } else if (event == markdownEventEnter) {
                    if (cmark_gfm_extensions_get_table_row_is_header(node) != 0) {
                        intent = [(id<MarkdownMessages>)presentationIntent
                            tableHeaderRowIntentWithIdentity:identity
                                          nestedInsideIntent:intent];
                    } else {
                        intent = [(id<MarkdownMessages>)presentationIntent tableRowIntentWithIdentity:identity
                                                                                                  row:row
                                                                                   nestedInsideIntent:intent];
                    }
                    identity++;
                    row++;
                    column = 0;
                }
            } else if (type == CMARK_NODE_TABLE_CELL) {
                if (event == markdownEventExit) {
                    intent = [(id<MarkdownMessages>)intent parentIntent];
                } else if (event == markdownEventEnter) {
                    intent = [(id<MarkdownMessages>)presentationIntent tableCellIntentWithIdentity:identity
                                                                                            column:column
                                                                                nestedInsideIntent:intent];
                    column++;
                    identity++;
                }
            }
            event = cmark_iter_next(iter);
        } while (event != markdownEventDone);
    }
    if (objc_opt_respondsToSelector((id)(creator), (@selector(finalizeWithLanguageIdentifier:)))) {
        [(id<MarkdownMessages>)creator
            finalizeWithLanguageIdentifier:[(id<MarkdownMessages>)options languageCode]];
    }
    result = YES;
    goto cleanup;
failure:
    result = NO;
cleanup:
    cmark_iter_free(iter);
    cmark_node_free(document);
    cmark_parser_free(parser);
    return result;
}

// Native appendNodeText, 0x1c0602e84. Non-external. A node without literal text appends nothing (YES). The
// literal becomes an NSString without copying (UTF-8, not freed); a string that cannot be made answers YES
// for a nonzero failure policy, else NO with NSCocoaErrorDomain 259 stored through error when it is not NULL.
// With source positions, an NSAttributedStringMarkdownSourcePosition is allocated first and initialized with
// the node's start line and column and end line and column (sign-extended), then given the UTF-8 offsets;
// the string is appended with it (nil without source positions), and both are released.
static BOOL appendNodeText(
    id creator,
    id options,
    cmark_node *node,
    NSUInteger inlineIntent,
    id intent,
    id attributes,
    const char *bytes,
    size_t length,
    MarkdownOffsetCache *cache,
    id *error
) {
    const char *literal = cmark_node_get_literal(node);
    if (literal == NULL) {
        return YES;
    }
    id string =
        [(id<MarkdownMessages>)objc_alloc((Class)((Class)stringClass)) initWithBytesNoCopy:literal
                                                                                    length:strlen(literal)
                                                                                  encoding:4
                                                                              freeWhenDone:NO];
    if (string == nil) {
        if ([(id<MarkdownMessages>)options failurePolicy] != 0) {
            return YES;
        }
        if (error == NULL) {
            return NO;
        }
        *error = [(id<MarkdownMessages>)(id)errorClass errorWithDomain:ONSCocoaErrorDomain
                                                                  code:259
                                                              userInfo:nil];
        return NO;
    }
    id position;
    if ([(id<MarkdownMessages>)options appliesSourcePositionAttributes]) {
        id allocated = objc_alloc((Class)((Class)sourcePositionClass));
        NSInteger startLine = (NSInteger)cmark_node_get_start_line(node);
        NSInteger startColumn = (NSInteger)cmark_node_get_start_column(node);
        NSInteger endLine = (NSInteger)cmark_node_get_end_line(node);
        NSInteger endColumn = (NSInteger)cmark_node_get_end_column(node);
        position = [(id<MarkdownMessages>)allocated initWithStartLine:startLine
                                                          startColumn:startColumn
                                                              endLine:endLine
                                                            endColumn:endColumn];
        [(id<MarkdownMessages>)position setOffsetsFromUTF8:bytes size:length usingCache:cache];
    } else {
        position = nil;
    }
    appendStringWithSourcePosition(creator, string, inlineIntent, intent, attributes, position);
    objc_release((id)(position));
    objc_release((id)(string));
    return YES;
}

// Native appendStringWithSourcePosition, 0x1c0603024. Non-external. With a position, a mutable copy of the
// attributes gains "NSMarkdownSourcePosition" and is used (then released after the append); otherwise the
// attributes as given. The creator is sent -appendString:with: with attributesForIntents' dictionary.
static void appendStringWithSourcePosition(
    id creator, id string, NSUInteger inlineIntent, id intent, id attributes, id position
) {
    if (position != nil) {
        id withPosition = [attributes mutableCopy];
        [(id<MarkdownMessages>)withPosition setObject:position forKey:(id)OF_STR("NSMarkdownSourcePosition")];
        [(id<MarkdownMessages>)creator appendString:string
                                               with:attributesForIntents(inlineIntent, intent, withPosition)];
        objc_release((id)(withPosition));
    } else {
        [(id<MarkdownMessages>)creator appendString:string
                                               with:attributesForIntents(inlineIntent, intent, attributes)];
    }
}

// Native attributesForIntents, 0x1c06030e0. Non-external. The base dictionary holds
// "NSInlinePresentationIntent" (a constant string) as an NSNumber of the inline intent when it is nonzero,
// and NSPresentationIntentAttributeName (the variable) with the intent when it is not nil (both: the inline
// intent first); with neither it is the empty dictionary singleton. Nonempty extra attributes are merged into
// an autoreleased mutable copy; otherwise the base dictionary is returned.
static id attributesForIntents(NSUInteger inlineIntent, id intent, id attributes) {
    id base;
    if (inlineIntent != 0 && intent != nil) {
        id keys[2];
        id objects[2];
        keys[0] = (id)OF_STR("NSInlinePresentationIntent");
        objects[0] = [(id<MarkdownMessages>)(id)numberClass numberWithUnsignedInteger:inlineIntent];
        keys[1] = ONSPresentationIntentAttributeName;
        objects[1] = intent;
        base = [(id<MarkdownMessages>)(id)dictionaryClass dictionaryWithObjects:objects forKeys:keys count:2];
    } else if (intent != nil) {
        id key = ONSPresentationIntentAttributeName;
        id object = intent;
        base = [(id<MarkdownMessages>)(id)dictionaryClass dictionaryWithObjects:&object forKeys:&key count:1];
    } else if (inlineIntent != 0) {
        id key = (id)OF_STR("NSInlinePresentationIntent");
        id object = [(id<MarkdownMessages>)(id)numberClass numberWithUnsignedInteger:inlineIntent];
        base = [(id<MarkdownMessages>)(id)dictionaryClass dictionaryWithObjects:&object forKeys:&key count:1];
    } else {
        base = (id)&__ONSDictionary0__struct;
    }
    if (attributes == nil) {
        return base;
    }
    if ([(id<MarkdownMessages>)attributes count] != 0) {
        id merged = [base mutableCopy];
        [(id<MarkdownMessages>)merged addEntriesFromDictionary:attributes];
        return objc_autorelease((id)(merged));
    }
    return base;
}

//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#import "Foundation/String/ONSAttributedString.h"
#import "Foundation/String/ONSCharacterSet.h"

#include <OpenCoreFoundation/OCFBuild.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>
#include <stddef.h>
#include <stdint.h>

// Shared declarations of the Markdown unit (String/ONSAttributedStringMarkdown.m), iOS Simulator 24A434,
// arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified unless a declaration says
// otherwise. iOS Device 24A446: unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md.

// libcmark-gfm (/usr/lib/libcmark-gfm.dylib, an LC_LOAD_DYLIB with the delay-init option in the native
// Foundation image): an external library the unit calls, supplied and never reimplemented. The native call
// sites go through the linker's delay-init stubs ($delayInitStub) and load helpers ($loadHelper_x8); the
// library's own symbols are what they bind. The SDK installs no header for the library, so the interfaces the
// unit calls are declared here: the names are the library's exported symbols, the signatures follow the
// callers' register use (32-bit values where the native code reads W registers) and the library's
// published interface (cmark-gfm.h and cmark-gfm-core-extensions.h), which this project neither imports nor
// reproduces. The node types, events, list types and delimiters are passed as int.
typedef struct cmark_parser cmark_parser;
typedef struct cmark_node cmark_node;
typedef struct cmark_iter cmark_iter;
typedef struct cmark_syntax_extension cmark_syntax_extension;

extern void cmark_gfm_core_extensions_ensure_registered(void);
extern cmark_parser *cmark_parser_new(int options);
extern cmark_syntax_extension *cmark_find_syntax_extension(const char *name);
extern int cmark_parser_attach_syntax_extension(cmark_parser *parser, cmark_syntax_extension *extension);
extern void cmark_parser_feed(cmark_parser *parser, const char *buffer, size_t length);
extern cmark_node *cmark_parser_finish(cmark_parser *parser);
extern void cmark_parser_free(cmark_parser *parser);
extern cmark_iter *cmark_iter_new(cmark_node *root);
extern int cmark_iter_next(cmark_iter *iter);
extern cmark_node *cmark_iter_get_node(cmark_iter *iter);
extern void cmark_iter_free(cmark_iter *iter);
extern void cmark_node_free(cmark_node *node);
extern int cmark_node_get_type(cmark_node *node);
extern const char *cmark_node_get_literal(cmark_node *node);
extern const char *cmark_node_get_url(cmark_node *node);
extern const char *cmark_node_get_title(cmark_node *node);
extern const char *cmark_node_get_fence_info(cmark_node *node);
extern const char *cmark_node_get_attributes(cmark_node *node);
extern int cmark_node_get_heading_level(cmark_node *node);
extern int cmark_node_get_list_type(cmark_node *node);
extern int cmark_node_get_list_delim(cmark_node *node);
extern int cmark_node_get_list_marker(cmark_node *node);
extern int cmark_node_get_list_start(cmark_node *node);
extern int cmark_node_get_start_line(cmark_node *node);
extern int cmark_node_get_start_column(cmark_node *node);
extern int cmark_node_get_end_line(cmark_node *node);
extern int cmark_node_get_end_column(cmark_node *node);
extern void *cmark_node_get_user_data(cmark_node *node);
extern int cmark_node_set_user_data(cmark_node *node, void *user_data);
extern uint16_t cmark_gfm_extensions_get_table_columns(cmark_node *node);
extern uint8_t *cmark_gfm_extensions_get_table_alignments(cmark_node *node);
extern int cmark_gfm_extensions_get_table_row_is_header(cmark_node *node);

// The node types the extensions register at run time (32-bit values the native code loads through the load
// helpers' GOT entries).
extern int CMARK_NODE_STRIKETHROUGH;
extern int CMARK_NODE_TABLE;
extern int CMARK_NODE_TABLE_ROW;
extern int CMARK_NODE_TABLE_CELL;

// Native _NSAttributedStringFromMarkdownWithCreator, 0x1c0601d2c, external: parses the UTF-8 Markdown bytes
// with libcmark-gfm and feeds the creator the text runs and their attributes. YES on success; NO with the
// error stored through error (when not NULL) on failure. Signature from the callers' and the function's
// register use.
OF_EXPORT BOOL _ONSAttributedStringFromMarkdownWithCreator(
    id creator, const char *bytes, size_t length, id options, id baseURL, id *error
);

// Interfaces of families not yet restored on this branch.

// Native NSPresentationIntentAttributeName, 0x2227daf10 (__DATA_CONST,__const), external (Foundation's
// attribute name constant, outside this family): the string pointer attributesForIntents loads. Type from
// Foundation's NSAttributedString.h (SDK).

#ifdef __OBJC__
// Classes the unit references by their class symbols, outside this family: the presentation intent, the
// parsing options and source position classes, the concrete creator, the constant integer number class of
// the alignment literals, and the classes reached through the shared cache's GOT (NSMutableArray, NSURL).

// Native category NSAttributedString(_NSAttributedStringFromMarkdown) (its other methods are outside the
// scope and not restored).
@interface ONSAttributedString (_NSAttributedStringFromMarkdown)
- (id)initWithMarkdownString:(id)markdownString options:(id)options baseURL:(id)baseURL error:(id *)error;
@end
#endif

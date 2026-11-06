//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#import "Foundation/String/ONSLexicon.h"

#import "Foundation/String/ONSString.h"

// iOS Simulator 27.0 (24A434), arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified from the captured assembly. Manual reference counting.
// Reading: Local/Scratch/cf-core-r2-fi/Reading.md. Organization inferred from the native function name.

// Native __NSLexiconIterateFormsOfWord, 0x1c05212cc. Was a private external.
// iOS Device divergence: 0x180f11de4 uses separate cached lowercase and uppercase block invokes
// (verified). The Simulator inlines those paths. Both pass form, transform and stop to every visit,
// including the two calls for which Hex-Rays omits the last three arguments.
void _NSLexiconIterateFormsOfWord(id word, id locale, LexiconWordFormVisitor visitor) {
    BOOL stop = NO;
    // Native ____NSLexiconIterateFormsOfWord_block_invoke, 0x1c0521614. Non-external.
    visitor(
        word,
        ^id(id form) {
          return form;
        },
        &stop
    );
    if (stop) {
        return;
    }

    id lowercase = nil;
    id uppercase = nil;
    // This byref variable emits ___Block_byref_object_copy__6 (0x1c0fdcbb4)
    // and ___Block_byref_object_dispose__6 (0x1c0fdcbc4), with flags 131.
    __block id capitalized = nil;
    __unsafe_unretained id capturedLocale = locale;
    __unsafe_unretained id capturedWord = word;

    // Native ____NSLexiconIterateFormsOfWord_block_invoke_3, 0x1c052161c. Non-external.
    id (^capitalizedWord)(void) = ^id {
      if (capitalized == nil) {
          capitalized = [capturedWord capitalizedStringWithLocale:capturedLocale];
      }
      return capitalized;
    };

    LexiconWordCase restoreCase;
    lowercase = [word lowercaseStringWithLocale:locale];
    if ([word isEqual:lowercase]) {
        // Native ____NSLexiconIterateFormsOfWord_block_invoke_4, 0x1c0521668. Non-external.
        restoreCase = ^id(id form) {
          return [form lowercaseStringWithLocale:capturedLocale];
        };
    } else {
        if (uppercase == nil) {
            uppercase = [word uppercaseStringWithLocale:locale];
        }
        if ([word isEqual:uppercase]) {
            // Native ____NSLexiconIterateFormsOfWord_block_invoke_5, 0x1c0521674. Non-external.
            restoreCase = ^id(id form) {
              return [form uppercaseStringWithLocale:capturedLocale];
            };
        } else if ([word isEqual:capitalizedWord()]) {
            // Native ____NSLexiconIterateFormsOfWord_block_invoke_6, 0x1c0521680. Non-external.
            restoreCase = ^id(id form) {
              return [form capitalizedStringWithLocale:capturedLocale];
            };
        } else {
            return;
        }
    }

    if (lowercase == nil) {
        lowercase = [word lowercaseStringWithLocale:locale];
    }
    if (![word isEqual:lowercase]) {
        if (lowercase == nil) {
            lowercase = [word lowercaseStringWithLocale:locale];
        }
        visitor(lowercase, restoreCase, &stop);
        if (stop) {
            return;
        }
    }

    if (![word isEqual:capitalizedWord()]) {
        visitor(capitalizedWord(), restoreCase, &stop);
        if (stop) {
            return;
        }
    }

    if (uppercase == nil) {
        uppercase = [word uppercaseStringWithLocale:locale];
    }
    if (![word isEqual:uppercase]) {
        if (uppercase == nil) {
            uppercase = [word uppercaseStringWithLocale:locale];
        }
        visitor(uppercase, restoreCase, &stop);
    }
}

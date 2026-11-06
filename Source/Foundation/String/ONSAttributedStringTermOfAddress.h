//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>
#include <objc/objc.h>

// Shared declarations of String/ONSAttributedStringTermOfAddress.m, the term-of-address helper the inflection
// unit calls. iOS Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence:
// verified. iOS Device 24A446: unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md.

// Native preferredTermOfAddressForLanguageFromList, 0x1c05c4ba4 (was a private external): the
// pronoun-extended copy of the first NSTermOfAddress of the list that supports the language, or nil.
OF_EXTERN id preferredTermOfAddressForLanguageFromList(id languageIdentifier, id list);

// Interfaces of families not yet restored on this branch.

// NSTermOfAddress (__DATA,__objc_data, external) lies outside this family; the function references its class
// symbol.
#ifdef __OBJC__
#endif

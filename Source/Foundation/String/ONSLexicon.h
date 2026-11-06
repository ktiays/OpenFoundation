//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>
#include <objc/objc.h>

typedef id (^LexiconWordCase)(id word);
typedef void (^LexiconWordFormVisitor)(id form, LexiconWordCase restoreCase, BOOL *stop);

OF_EXTERN void _NSLexiconIterateFormsOfWord(id word, id locale, LexiconWordFormVisitor visitor);

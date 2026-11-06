//
//  Created by ktiays on 2026/11/03.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include "CoreFoundation/OCFBuild.h"
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

// These symbols are local in the native image. Keep hidden linkage across the FG and FI source units.
OF_EXTERN BOOL _NSGrammaticalGenderGetFromExternalRepresentation(id representation, NSInteger *value);
OF_EXTERN BOOL _NSGrammaticalNumberGetFromExternalRepresentation(id representation, NSInteger *value);
OF_EXTERN BOOL _NSGrammaticalPartOfSpeechGetFromExternalRepresentation(id representation, NSInteger *value);
OF_EXTERN BOOL _NSGrammaticalCaseGetFromExternalRepresentation(id representation, NSInteger *value);
OF_EXTERN BOOL _NSGrammaticalPronounTypeGetFromExternalRepresentation(id representation, NSInteger *value);

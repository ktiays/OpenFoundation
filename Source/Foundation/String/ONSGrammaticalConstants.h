//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <objc/NSObjCRuntime.h>

// The grammatical values without a public enumeration case, as the Swift code of the Foundation image loads
// them: file-local constants in __TEXT,__const (iOS Simulator 24A434, Foundation UUID
// 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified). Native _NSGrammaticalGenderNeutral 0x1c100d850
// (4), _NSGrammaticalPartOfSpeechProperNoun 0x1c100d858 (15), _NSGrammaticalCaseInstrumental 0x1c100d860 (15)
// and _NSGrammaticalCaseComitative 0x1c100d868 (16). Static, so each importer keeps its own copy, as
// natively.
static const NSInteger _ONSGrammaticalGenderNeutral = 4;
static const NSInteger _ONSGrammaticalPartOfSpeechProperNoun = 15;
static const NSInteger _ONSGrammaticalCaseInstrumental = 15;
static const NSInteger _ONSGrammaticalCaseComitative = 16;

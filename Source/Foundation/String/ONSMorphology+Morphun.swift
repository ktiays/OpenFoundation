//
//  Created by ktiays on 2026/11/06.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

// Foundation, iOS Simulator 24A434, arm64, UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03.
// Confidence: verified (assembly read in full). iOS Device divergence: iOS Device 24A446 remains unread.
//
// The morphun constraints of NSMorphology and NSMorphologyPronoun: the grammemes as morphun feature names and
// values, which -[NSMorphology(_Morphun) addConstraintsTo:] and _NSLexiconMorphunDictionary read through
// -_morphunConstraints (declared for them in String/ONSMorphologyMorphunConstraints.h).
//
// The native grammeme accessors are imported as NSGrammatical* types; this module imports them as Int (the
// restored headers declare NSInteger), so the value mappings are fileprivate functions here where native has
// fileprivate extensions on those types. Each grammeme getter is sent twice natively: once to compute the value
// and once more when it is stored (the second result is unused); both sends are kept. The specialized
// Dictionary members these functions call (subscript setter 0x1c08527f4, _NativeDictionary._insert,
// _copyOrMoveAndResize and __RawDictionaryStorage.find) are compiler-generated.

// Native NSGrammaticalPartOfSpeech.morphunValue() (fileprivate), 0x1c0f79d58: the value table 0x1c12881d8 (its
// object words 0x1c1288250) for 0-14, "properNoun" for _NSGrammaticalPartOfSpeechProperNoun, otherwise nil.
private func partOfSpeechMorphunValue(_ partOfSpeech: Int) -> String? {
    switch partOfSpeech {
    case 1:
        return "determiner"
    case 2:
        return "pronoun"
    case 3:
        return "letter"
    case 4:
        return "adverb"
    case 5:
        return "particle"
    case 6:
        return "adjective"
    case 7:
        return "adposition"
    case 8:
        return "verb"
    case 9:
        return "noun"
    case 10:
        return "conjunction"
    case 11:
        return "number"
    case 12:
        return "interjection"
    case 13:
        return "preposition"
    case 14:
        return "abbreviation"
    case _ONSGrammaticalPartOfSpeechProperNoun:
        return "properNoun"
    default:
        return nil
    }
}

// Native NSGrammaticalCase.morphunValue() (fileprivate), 0x1c0f79db0: the value table 0x1c12882c8 (its object
// words 0x1c1288340) for 0-14, "instrumental" for _NSGrammaticalCaseInstrumental, "comitative" for
// _NSGrammaticalCaseComitative, otherwise nil.
private func grammaticalCaseMorphunValue(_ grammaticalCase: Int) -> String? {
    switch grammaticalCase {
    case 1:
        return "nominative"
    case 2:
        return "accusative"
    case 3:
        return "dative"
    case 4:
        return "genitive"
    case 5:
        return "prepositional"
    case 6:
        return "ablative"
    case 7:
        return "adessive"
    case 8:
        return "allative"
    case 9:
        return "elative"
    case 10:
        return "illative"
    case 11:
        return "essive"
    case 12:
        return "inessive"
    case 13:
        return "locative"
    case 14:
        return "translative"
    case _ONSGrammaticalCaseInstrumental:
        return "instrumental"
    case _ONSGrammaticalCaseComitative:
        return "comitative"
    default:
        return nil
    }
}

// The value mappings the native code inlines into NSMorphology.morphunConstraints().
private func genderMorphunValue(_ gender: Int) -> String? {
    switch gender {
    case 1:
        return "feminine"
    case 2:
        return "masculine"
    case 3:
        return "neuter"
    case _ONSGrammaticalGenderNeutral:
        return "common"
    default:
        return nil
    }
}

// The number table 0x1c12883b8 (object words 0x1c12883e8): zero (2) has no value.
private func numberMorphunValue(_ number: Int) -> String? {
    switch number {
    case 1:
        return "singular"
    case 3:
        return "plural"
    case 4:
        return "dual"
    case 5, 6:
        return "plural"
    default:
        return nil
    }
}

private func determinationMorphunValue(_ determination: Int) -> String? {
    switch determination {
    case 1:
        return "independent"
    case 2:
        return "dependent"
    default:
        return nil
    }
}

private func definitenessMorphunValue(_ definiteness: Int) -> String? {
    switch definiteness {
    case 1:
        return "indefinite"
    case 2:
        return "definite"
    default:
        return nil
    }
}

private func personMorphunValue(_ person: Int) -> String? {
    switch person {
    case 1:
        return "first"
    case 2:
        return "second"
    case 3:
        return "third"
    default:
        return nil
    }
}

private func pronounTypeMorphunValue(_ pronounType: Int) -> String? {
    switch pronounType {
    case 1:
        return "personal"
    case 2:
        return "reflexive"
    case 3:
        return "possessive"
    default:
        return nil
    }
}

private func clusivityMorphunValue(_ clusivity: Int) -> String? {
    switch clusivity {
    case 1:
        return "inclusive"
    case 2:
        return "exclusive"
    default:
        return nil
    }
}

extension ONSMorphology {
    // Native NSMorphology.morphunConstraints(), 0x1c0f79e38: each set grammeme under its morphun feature name, in
    // the order gender, count, pos, case, determination, definiteness, person, pronounType, clusivity; and
    // "adjectival" = "predicative" when _adjectival is 1.
    func morphunConstraints() -> [String: String] {
        var constraints: [String: String] = [:]
        if genderMorphunValue(grammaticalGender()) != nil {
            constraints["gender"] = genderMorphunValue(grammaticalGender())
        }
        if numberMorphunValue(number()) != nil {
            constraints["count"] = numberMorphunValue(number())
        }
        if partOfSpeechMorphunValue(partOfSpeech()) != nil {
            constraints["pos"] = partOfSpeechMorphunValue(partOfSpeech())
        }
        if grammaticalCaseMorphunValue(grammaticalCase()) != nil {
            constraints["case"] = grammaticalCaseMorphunValue(grammaticalCase())
        }
        if determinationMorphunValue(determination()) != nil {
            constraints["determination"] = determinationMorphunValue(determination())
        }
        if definitenessMorphunValue(definiteness()) != nil {
            constraints["definiteness"] = definitenessMorphunValue(definiteness())
        }
        if personMorphunValue(grammaticalPerson()) != nil {
            constraints["person"] = personMorphunValue(grammaticalPerson())
        }
        if pronounTypeMorphunValue(pronounType()) != nil {
            constraints["pronounType"] = pronounTypeMorphunValue(pronounType())
        }
        if clusivityMorphunValue(_clusivity()) != nil {
            constraints["clusivity"] = clusivityMorphunValue(_clusivity())
        }
        if _adjectival() == 1 {
            _ = _adjectival()
            constraints["adjectival"] = "predicative"
        }
        return constraints
    }

    // Native @objc NSMorphology.morphunConstraints(), 0x1c0f7b688 (selector _morphunConstraints): the
    // constraints bridged with _NativeDictionary.bridged.
    @objc(_morphunConstraints)
    func _morphunConstraintsObject() -> AnyObject {
        return morphunConstraints()._bridgeToObjectiveCImpl()
    }
}

extension ONSMorphologyPronoun {
    // Native NSMorphologyPronoun.morphunConstraints(), 0x1c0f7b6d0: the morphology's constraints, then those of
    // the dependent morphology, if any, under "dependency=" + name. The morphology is messaged without a nil
    // check, as natively (the bit cast drops the Optional the import adds).
    func morphunConstraints() -> [String: String] {
        let morphology = unsafeBitCast(self.morphology() as ONSMorphology?, to: ONSMorphology.self)
        var constraints = [String: String]._unconditionallyBridgeFromObjectiveC(
            morphology._morphunConstraintsObject()
        )
        guard let dependentMorphology = dependentMorphology() else {
            return constraints
        }
        let dependentConstraints = [String: String]._unconditionallyBridgeFromObjectiveC(
            dependentMorphology._morphunConstraintsObject()
        )
        for (name, value) in dependentConstraints {
            var key = "dependency="
            key.append(name)
            constraints[key] = value
        }
        return constraints
    }

    // Native @objc NSMorphologyPronoun.morphunConstraints(), 0x1c0f7beb8 (selector _morphunConstraints): the
    // constraints bridged with _NativeDictionary.bridged.
    @objc(_morphunConstraints)
    func _morphunConstraintsObject() -> AnyObject {
        return morphunConstraints()._bridgeToObjectiveCImpl()
    }
}

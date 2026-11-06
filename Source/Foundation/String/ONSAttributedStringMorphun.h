//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#pragma once

#include <OpenCoreFoundation/OCFBuild.h>
#include <OpenCoreFoundation/Base/OCFBase.h>
#include "CoreFoundation/Collections/OCFArray.h"
#include <dispatch/dispatch.h>
#include <stdint.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

// Shared declarations of the morphun cluster (String/ONSAttributedStringMorphun.m), the plain functions of
// Foundation's NSMorphology(_Morphun) unit (0x1c057ed5c-0x1c057ff5f) that the inflection unit calls. iOS
// Simulator 24A434, arm64, Foundation UUID 49FCDA5F-A93E-3259-857C-FC75AC2C7B03; confidence: verified unless
// a declaration says otherwise. iOS Device 24A446: unread. Reading:
// Source/Foundation/Collections/FoundationSetsAttributedReading.md.

// Native _NSRequiresMorphunInflectionForLanguageIdentifier, 0x1c057ed5c (was a private external): whether the
// language identifier begins with one of the language codes the inflection engine serves.
OF_EXTERN BOOL _NSRequiresMorphunInflectionForLanguageIdentifier(id languageIdentifier);

// Native _NSLexiconInflectedStringUsingMorphun, 0x1c057f55c (was a private external). Restored caller;
// the soft-link initializer and its function-pointer state are shared below.
OF_EXTERN id _NSLexiconInflectedStringUsingMorphun(
    id string,
    NSUInteger location,
    NSUInteger length,
    id languageIdentifier,
    id morphology,
    id featureName,
    id replacements
);

// Native function-pointer slots 0x22e76c710-0x22e76c760, eleven consecutive 8-byte fields. Signatures
// follow X-register arguments and the width actually consumed by 0x1c057f55c; the library symbol names
// come from the loader at 0x1c0582224. These slots are used by the inflected-string path.
typedef struct {
    void *(*getDefaultProvider)(OCFTypeRef *error);
    void *(*getCommonFactory)(void *provider, OCFTypeRef locale, OCFTypeRef *error);
    void *(*getSemanticModel)(void *factory, OCFTypeRef *error);
    void *(*createSpeakable)(OCFStringRef string, OCFTypeRef *error);
    void *(*createInflectable)(void *model, void *source, OCFTypeRef *error);
    uint8_t (*inflectionExists)(void *inflectable, OCFTypeRef *error);
    void *(*copyFeature)(void *inflectable, OCFStringRef feature, OCFTypeRef *error);
    void *(*toSpeakable)(void *inflectable, OCFTypeRef *error);
    OCFStringRef (*copyPrint)(void *speakable, OCFTypeRef *error);
    void (*releaseSpeakable)(void *speakable);
    void (*releaseInflectable)(void *inflectable);
} MorphunFunctions;

_Static_assert(
    sizeof(MorphunFunctions) == 88, "The captured Morphun pointer table contains eleven pointers."
);
OF_EXTERN MorphunFunctions morphunFunctions;

// Native __MergedGlobals, 0x22e76c708, followed by slots 0x22e76c768–0x22e76c838 and
// 0x22e76c848–0x22e76c878, non-external. The loader stores dlsym results as pointer-width values.
// Call signatures belong to the consumers of the individual slots.
typedef struct {
    void (*minf_putConstraintByName)(void *, OCFStringRef, OCFStringRef, OCFTypeRef *); // 0x22e76c708
    void *(*mdaf_getAnalyzer)(void *, id, OCFTypeRef *);                                // 0x22e76c768
    void *(*mtok_createTokenizer)(id, OCFTypeRef *);                                    // 0x22e76c770
    void (*mtok_setStyle)(void *, int32_t, OCFTypeRef *);                               // 0x22e76c778
    void (*mtok_destroy)(void *);                                                       // 0x22e76c780
    void *(*mddmd_getDictionary)(id, OCFTypeRef *);                                     // 0x22e76c788
    void *(*mtok_createTokenChain)(void *, OCFStringRef, OCFTypeRef *);                 // 0x22e76c790
    void *(*mca_createTokenChain)(void *, void *, OCFTypeRef *);                        // 0x22e76c798
    void *(*mtokc_getToken)(void *, int32_t, OCFTypeRef *);                             // 0x22e76c7a0
    uint32_t (*mtok_isType)(void *, int32_t, OCFTypeRef *);                             // 0x22e76c7a8
    void *(*matmwt_getMorphologicalWordToken)(void *, OCFTypeRef *);                    // 0x22e76c7b0
    OCFArrayRef (*matmwt_createLemmaMappings)(void *, OCFTypeRef *);                    // 0x22e76c7b8
    uint64_t (*matlm_getSurfaceFormGrammemes)(const void *, OCFTypeRef *);              // 0x22e76c7c0
    id (*mddmd_createPropertyNames)(const void *, uint64_t, OCFTypeRef *);              // 0x22e76c7c8
    void (*mtokc_destroy)(void *);                                                      // 0x22e76c7d0
    void (*mdv_destroy)(const void *);                                                  // 0x22e76c7d8
    void *(*mpron_createFromInitialPronoun)(void *, OCFStringRef, OCFTypeRef *);        // 0x22e76c7e0
    void *(*mpron_toSemanticFeatureConcept)(void *, OCFTypeRef *);                      // 0x22e76c7e8
    void (*msfc_putConstraintByName)(void *, OCFStringRef, OCFStringRef, OCFTypeRef *); // 0x22e76c7f0
    void (*msfc_clearConstraintByName)(void *, OCFStringRef, OCFTypeRef *);             // 0x22e76c7f8
    const void *(*mdv_create)(void *, OCFStringRef, id, OCFTypeRef *);                  // 0x22e76c800
    void *(*mpron_createWithCustom)(void *, OCFArrayRef, const void *, OCFTypeRef *);   // 0x22e76c808
    void *(*msfc_createFeatureValueByNameCopy)(void *, OCFStringRef, OCFTypeRef *);     // 0x22e76c810
    uint8_t (*msfc_isExists)(void *, OCFTypeRef *);                                     // 0x22e76c818
    void *(*minf_toSemanticFeatureConcept)(void *, OCFTypeRef *);                       // 0x22e76c820
    void *(*msfc_toSpeakableStringWithDependencyCopy)(void *, void *, OCFTypeRef *);    // 0x22e76c828
    void *(*msfc_toSpeakableStringCopy)(void *, OCFTypeRef *);                          // 0x22e76c830
    void (*mpron_destroy)(void *);                                                      // 0x22e76c838
    void *(*mdaf_create)(int32_t, OCFTypeRef *);                                        // 0x22e76c848
    void *mpron_toPronounConcept;                                                       // 0x22e76c850
    void *mpron_createWithDefaults;                                                     // 0x22e76c858
    void *msfc_createConstraintByNameCopy;                                              // 0x22e76c860
    void *msfc_reset;                                                                   // 0x22e76c868
    void *msfc_clone;                                                                   // 0x22e76c870
    void *msfc_destroy;                                                                 // 0x22e76c878
} MorphunLexiconFunctions;

OF_EXTERN MorphunLexiconFunctions morphunLexiconFunctions;
// Native once predicate 0x22e76c840.
OF_EXTERN dispatch_once_t morphunLoadOnceToken;
// Native ___NSLexiconLoadMorphunIfNeeded_block_invoke, 0x1c0582224, non-external.
OF_EXTERN void morphunLoadInvoke(void *block) __asm("____NSLexiconLoadMorphunIfNeeded_block_invoke");

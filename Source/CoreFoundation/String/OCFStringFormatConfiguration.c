//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/String/OCFStringFormatConfiguration.h"
#include "CoreFoundation/PlugIn/OCFBundleLocalization.h"

#include "CoreFoundation/Objective-C/OCFNSObject.h"
#include "CoreFoundation/PlugIn/OCFBundleMain.h"

#include <dispatch/dispatch.h>
#include <objc/message.h>
#include <objc/runtime.h>

// Inferred organization: __CFStringCopyWithFomatStringConfiguration (0x1c02caf38),
// __CFStringCreateWithWidthContexts (0x1c02caf88) and __CFStringCreateWithMarkdownAndConfiguration
// (0x1c02cbea0) belong to the unit around __CFStringGetFormatSpecifierConfiguration (0x1c02caef8) and
// __CFCopyResolvedFormatStringWithConfiguration (0x1c02cb238), restored by another sub-track; they are kept
// in this translation unit until that unit is merged. The native source path is not in the evidence. Unless a
// definition says otherwise: iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC; confidence:
// verified; iOS Device 24A446: unread. Reading: Documentation/Reference/Research/CoreFoundationString.md.
// Folds the compiler-outlined piece __CFStringCreateWithMarkdownAndConfiguration.cold.1 (0x1c03122c4).

// libobjc interfaces (supplied).
extern id objc_alloc(Class cls);
extern BOOL objc_opt_respondsToSelector(id object, SEL selector);

// Native __NSVariableWidthStringClass, 0x22e6d3b58 (__DATA,__bss). Non-external.
static Class __NSVariableWidthStringClass;

// Native __NSCFLocalizedAttributedStringClass, 0x22e6d3b90, and
// __NSGetCFLocalizedAttributedStringClass.onceToken, 0x22e6d3b98 (__DATA,__bss). Non-external.
static Class __NSCFLocalizedAttributedStringClass;
static dispatch_once_t __NSGetCFLocalizedAttributedStringClassOnceToken;

// Native _CFStringCopyWithFomatStringConfiguration, 0x1c02caf38. Hidden (was private external). An object
// that responds to -_copyFormatStringWithConfiguration: answers it; any other is retained and returned.
OCFStringRef _CFStringCopyWithFomatStringConfiguration(OCFStringRef string, const void *configuration) {
    SEL selector = sel_getUid("_copyFormatStringWithConfiguration:");
    if (objc_opt_respondsToSelector((id)string, selector)) {
        return ((OCFStringRef (*)(id, SEL, const void *))objc_msgSend)((id)string, selector, configuration);
    }
    OCFRetain(string);
    return string;
}

// Native _CFStringCreateWithWidthContexts, 0x1c02caf88. Hidden (was private external). The class
// __NSVariableWidthString is looked up through __CFLookUpClass once it is found (a failed lookup is retried
// on the next call) and NULL is returned while it is absent; otherwise +alloc then -initWithWidthContexts:.
OCFStringRef _CFStringCreateWithWidthContexts(OCFDictionaryRef widthContexts) {
    Class cls = __NSVariableWidthStringClass;
    if (cls == Nil) {
        cls = (Class)__CFLookUpClass(OF_NAME_STRING(__ONSVariableWidthString));
        __NSVariableWidthStringClass = cls;
        if (cls == Nil) {
            return NULL;
        }
    }
    id object = objc_alloc(cls);
    return ((OCFStringRef (*)(id, SEL, OCFDictionaryRef))objc_msgSend)(
        object, sel_getUid("initWithWidthContexts:"), widthContexts
    );
}

// Native _CFStringCreateWithMarkdownAndConfiguration, 0x1c02cbea0. Hidden (was private external). A NULL
// bundle means the main bundle; its localizations are copied and released after
// +[__NSCFLocalizedAttributedString
// copyStringWithMarkdown:formatConfiguration:bundleLocalizations:localization:]. The class is resolved once
// through dispatch_once (.cold.1, OUTLINED_FUNCTION_0_56 with
// ____NSGetCFLocalizedAttributedStringClass_block_invoke, 0x1c02cbf84: objc_lookUpClass).
OCFStringRef _CFStringCreateWithMarkdownAndConfiguration(
    OCFStringRef markdown, const void *configuration, OCFBundleRef bundle, OCFStringRef localization
) {
    if (bundle == NULL) {
        bundle = OCFBundleGetMainBundle();
    }
    OCFArrayRef localizations = OCFBundleCopyBundleLocalizations(bundle);
    dispatch_once(&__NSGetCFLocalizedAttributedStringClassOnceToken, ^{
      __NSCFLocalizedAttributedStringClass =
          objc_lookUpClass(OF_NAME_STRING(__ONSCFLocalizedAttributedString));
    });
    OCFStringRef result =
        ((OCFStringRef (*)(id, SEL, OCFStringRef, const void *, OCFArrayRef, OCFStringRef))objc_msgSend)(
            (id)__NSCFLocalizedAttributedStringClass,
            sel_getUid("copyStringWithMarkdown:formatConfiguration:bundleLocalizations:localization:"),
            markdown,
            configuration,
            localizations,
            localization
        );
    OCFRelease(localizations);
    return result;
}

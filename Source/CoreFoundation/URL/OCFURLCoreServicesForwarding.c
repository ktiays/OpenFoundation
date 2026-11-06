//
//  Created by ktiays on 2026/10/31.
//  Copyright (c) 2026 ktiays. All rights reserved.
//

#include "CoreFoundation/URL/OCFURLCoreServicesForwarding.h"

// Inferred organization: CFURLCreateBookmarkData (0x1c02b6558) through CFURLEnumeratorGetSourceDidChange
// (0x1c02b6594) are one run of single-instruction functions in the iOS Simulator __text section (between the
// NSCache unit and __CFBasicHashGetCallbacks), each a tail call (B) of the CoreServicesInternal function of
// the same name with a leading underscore; the run is kept in its own translation unit. Only the four
// bookmark functions the bridged NSURL methods reach are defined here, in native address order; the other
// forwarders of the run (CFURLCreateByResolvingBookmarkData 0x1c02b655c,
// CFURLCreateResourcePropertyForKeyFromBookmarkData 0x1c02b6564, _CFURLCreateByResolvingAliasFile,
// _CFURLBookmarkCopyDescription and the CFURLEnumerator functions) are not restored on this branch. The
// native source path is not in the evidence. iOS Simulator 24A434, UUID D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC;
// confidence: verified (one B instruction each, Local/Evidence/cf-core/iphone-simulator-corpus-delta7.json;
// the targets are undefined imports "from CoreServicesInternal" in Local/Evidence/cf-core/iphone-simulator-
// nm.txt). iOS Device 24A446: unread. Reading: not yet written.
//
// Cut: CoreServicesInternal is another image the native code imports directly (no soft link, no NULL check),
// so each target is an unsupported cut: declared below with its native name and left undefined, linked
// unbound (weak, to null; Configuration/LinkDependencies.json), so a call faults at the null address. The
// targets' signatures are the forwarders' (the
// arguments pass through unchanged; confidence: medium).
extern OCFDataRef _CFURLCreateBookmarkData(
    OCFAllocatorRef allocator,
    OCFURLRef url,
    OCFOptionFlags options,
    OCFArrayRef resourcePropertiesToInclude,
    OCFURLRef relativeToURL,
    OCFErrorRef *error
);
extern OCFDictionaryRef _CFURLCreateResourcePropertiesForKeysFromBookmarkData(
    OCFAllocatorRef allocator, OCFArrayRef resourcePropertiesToReturn, OCFDataRef bookmark
);
extern OCFDataRef
_CFURLCreateBookmarkDataFromFile(OCFAllocatorRef allocator, OCFURLRef fileURL, OCFErrorRef *errorRef);
extern Boolean _CFURLWriteBookmarkDataToFile(
    OCFDataRef bookmarkRef, OCFURLRef fileURL, OCFOptionFlags options, OCFErrorRef *errorRef
);

// Native CFURLCreateBookmarkData, 0x1c02b6558. External; a tail call of CoreServicesInternal's
// _CFURLCreateBookmarkData (unsupported cut).
OCFDataRef OCFURLCreateBookmarkData(
    OCFAllocatorRef allocator,
    OCFURLRef url,
    OCFOptionFlags options,
    OCFArrayRef resourcePropertiesToInclude,
    OCFURLRef relativeToURL,
    OCFErrorRef *error
) {
    return _CFURLCreateBookmarkData(
        allocator, url, options, resourcePropertiesToInclude, relativeToURL, error
    );
}

// Native CFURLCreateResourcePropertiesForKeysFromBookmarkData, 0x1c02b6560. External; a tail call of
// CoreServicesInternal's _CFURLCreateResourcePropertiesForKeysFromBookmarkData (unsupported cut).
OCFDictionaryRef OCFURLCreateResourcePropertiesForKeysFromBookmarkData(
    OCFAllocatorRef allocator, OCFArrayRef resourcePropertiesToReturn, OCFDataRef bookmark
) {
    return _CFURLCreateResourcePropertiesForKeysFromBookmarkData(
        allocator, resourcePropertiesToReturn, bookmark
    );
}

// Native CFURLCreateBookmarkDataFromFile, 0x1c02b6568. External; a tail call of CoreServicesInternal's
// _CFURLCreateBookmarkDataFromFile (unsupported cut).
OCFDataRef
OCFURLCreateBookmarkDataFromFile(OCFAllocatorRef allocator, OCFURLRef fileURL, OCFErrorRef *errorRef) {
    return _CFURLCreateBookmarkDataFromFile(allocator, fileURL, errorRef);
}

// Native CFURLWriteBookmarkDataToFile, 0x1c02b656c. External; a tail call of CoreServicesInternal's
// _CFURLWriteBookmarkDataToFile (unsupported cut).
Boolean OCFURLWriteBookmarkDataToFile(
    OCFDataRef bookmarkRef, OCFURLRef fileURL, OCFOptionFlags options, OCFErrorRef *errorRef
) {
    return _CFURLWriteBookmarkDataToFile(bookmarkRef, fileURL, options, errorRef);
}

# Port/Headers: SDK-shaped headers for consumers (wasm32)

`<CoreFoundation/…>` and `<Foundation/…>` in the SDK's shape, for programs that link the port's
`OpenCoreFoundation` and `OpenFoundation` on wasm32 (OpenCoreAnimation, later OpenCoreGraphics). OpenFoundation's
own build does not use them. They declare only what the consumers use, with the types of the restored
implementation: CF and NS names (the port builds with `OF_SYSTEM_NAMES=1`), `CFIndex`/`NSInteger` `long`, `BOOL`
`bool`, `CGFloat` `double`, `UInt32` `unsigned int` (arm64's widths), no nullability annotations, and no CF
SPI (`_CFRuntimeRegisterClass` and the other SPI the consumers call stay declared by the consumers).

Consumer flags: `-I Port/Headers -I Port/System/include`, libdispatch's, objc4's and libmach's headers,
`-DNS_BUILD_32_LIKE_64=1 -DOS_OBJECT_USE_OBJC=0 -U__OBJC_BOOL_IS_BOOL -D__OBJC_BOOL_IS_BOOL=1` and objc4's
Objective-C flags (`Tests/compile-headers.sh` spells them out).

## Verification

Run after `source <objc4>/build/wasm-toolchain/env.sh`. `<build>` is a configured build of `Port/` with a
`compile_commands.json` (`-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`, or OpenCoreAnimation's build, which adds
`Port/` as `Port/OpenFoundation`); `<lib>` is the directory holding its `libOpenCoreFoundation.a`,
`libOpenFoundation.a` and `libPortSystem.a`.

1. Every header compiles on its own in C (gnu17), C++20, Objective-C and Objective-C++ with `-Wall -Wextra
   -Werror`:

   ```sh
   Port/Headers/Tests/compile-headers.sh <scratch>
   ```

2. C-level declarations against the restored ones. The two sets cannot share a unit (C does not let a unit
   redefine the same anonymous structures and enumerators), so `check-declarations.py` reads the restored
   declarations from clang's AST of every restored header and of `Port/CoreFoundation`/`Port/Foundation`
   (OpenFoundation's exact flags, `OFPrefixMapping.h` force-included) and compiles, with the same flags, a unit
   that includes the umbrella header and redeclares every function and variable with the restored type
   (a difference is a "conflicting types" error), asserts typedef compatibility, field types in order and
   enumerator values:

   ```sh
   Port/Headers/Tests/check-declarations.sh <build> <lib> <scratch>
   ```

   Result (2026-11-07): all three pass. The NSException name constants and
   `NSSystemClockDidChangeNotification` are `CFStringRef` in the restored declarations and `NSString *` here
   (one object pointer either way; compared as such).

3. Objective-C methods at run time. `generate-method-check.py` turns every class's declared methods (interface,
   categories, adopted protocols) into a protocol; the program compares each protocol method's type encoding
   with the implementation's found through the class's method lists (no `+initialize`), at the calling
   convention's level (pointees, structure tags, signedness and offsets ignored; exact differences are listed
   as `SPELLING`):

   ```sh
   Port/Headers/Tests/run-method-check.sh <lib> <scratch>
   ```

   Result (2026-11-07): 723 methods checked, no mismatch. Eleven spelling differences: the NSArray family
   implements its range methods with `CFRange` (`{?=ii}`, not `{_NSRange=II}`) and
   `-countByEnumeratingWithState:objects:count:` with `void *`. 58 abstract class-cluster methods are
   implemented only by concrete subclasses (`__NSCFData`, `__NSCFTimer`, `NSKeyedArchiver`, …).

## Declared, not implemented

Declared because a consumer uses them; OpenFoundation does not define them (link errors for functions,
variables and classes; `doesNotRecognizeSelector:` for methods).

- Classes: `NSAutoreleasePool`, `NSRunLoop` (`+mainRunLoop`, `-getCFRunLoop`, `-addTimer:forMode:`),
  `NSPredicate` (`+predicateWithBlock:`), `NSDateFormatter` (`-stringFromDate:`, `-dateFormat`,
  `-setDateFormat:`), `NSPropertyListSerialization` (`+propertyListWithData:options:format:error:`,
  `+dataWithPropertyList:format:options:error:`).
- `NSObject`: `-valueForKey:`, `-setValue:forKey:`, `-valueForKeyPath:`, `-setValue:forKeyPath:`,
  `-valueForUndefinedKey:`, `-willChangeValueForKey:`, `-didChangeValueForKey:`,
  `-performSelectorOnMainThread:withObject:waitUntilDone:`; `-classForKeyedArchiver` and
  `-replacementObjectForCoder:` exist only in a few subclasses.
- `NSArray -filteredArrayUsingPredicate:`.
- `NSData`: `-getBytes:length:`, `-writeToFile:atomically:`, `-writeToFile:options:error:`,
  `-initWithContentsOfURL:`, `-initWithContentsOfURL:options:error:`, `-base64EncodedStringWithOptions:`,
  `-initWithBase64EncodedData:options:`.
- `NSURL`: `+URLWithString:`, `+URLWithString:relativeToURL:`, `+fileURLWithPath:`,
  `+fileURLWithPath:isDirectory:`, `-initWithString:relativeToURL:`, `-absoluteString`, `-path`,
  `-relativePath`, `-isFileURL`, `-URLByDeletingLastPathComponent`.
- `NSTimer +timerWithTimeInterval:target:selector:userInfo:repeats:`, `NSHashTable +weakObjectsHashTable`,
  `NSXPCCoder -encodeXPCObject:forKey:` and `-decodeXPCObjectOfType:forKey:`.
- Methods of protocols the SDK's classes adopt: `NSData` and `NSURL` `-initWithCoder:`, `-encodeWithCoder:`,
  `NSData -mutableCopyWithZone:`, `NSURL -copyWithZone:`, `NSHashTable +supportsSecureCoding`.
- Functions and variables: `NSTemporaryDirectory`, `NSRunLoopCommonModes`,
  `CFCalendarComposeAbsoluteTime`, `CFMachPortCreateWithPort`, `CFMachPortInvalidate`,
  `CFMachPortCreateRunLoopSource`, `CFNotificationCenterRemoveObserver`, `CFRunLoopObserverCreate`,
  `CFRunLoopRun`, `CFRunLoopStop`, `CFURLCopyFragment`, `CFURLCopyParameterString`, `CFURLCreateWithBytes`,
  `CFUUIDCreate`. (`CFDataGetBytes`, `kCFErrorDescriptionKey` and `CFAttributedStringCreate` are port definitions in
  `Port/CoreFoundation/OCFPortDefinitions.c`.)

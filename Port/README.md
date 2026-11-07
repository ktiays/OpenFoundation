# Port: WebAssembly (wasm32)

An experimental port of the iOS Simulator restoration (MATCH=iphonesimulator) to WebAssembly (TARGET=wasm,
wasm32, Emscripten with pthreads), kept on `main` but not admitted (`Configuration/Platforms.json` lists only
the iOS Simulator pair; Make and Xcode never compile `Port/`). Owner decisions, 2026-11-07: wasm32, not wasm64;
trim what the consumers do not need and give a trimmed part a simple implementation when it turns out to be
needed; rewrite the Swift-implemented units the port needs in C or Objective-C.

Apple builds are meant to stay unchanged: `Source/` reaches the port only through
`Source/CoreFoundation/OFPortHooks.h` (`OF_PORT_ANSWER`, `OF_PORT_RETURN`, `OF_PORT_FUNCTION`, `OF_PORT_NATIVE`, each site a row of
`hooks.tsv`) and through three macros of `OCFBuild.h` that expand to the native code on Apple targets
(`OF_SYMBOL`, `OF_ABI_ASSERT`, `OF_UINT64_BITFIELD`). A handful of raw arm64 offsets became equivalent
expressions (`offsetof`, `sizeof`) that evaluate to the same constants on arm64, with `OF_ABI_ASSERT`s pinning
the arm64 values. A port change is landed only after the Release binaries of both naming modes compare
identical (`cmp`) with those built without it, from the same directory (the debug map records the source
directory, and a different path can even reorder duplicate GOT slots under LTO).

One exception predates that rule: the port's macros (`OF_PORT`, `OF_SYMBOL`, `OF_ABI_ASSERT`, `OF_WORD_SHIFT`,
`OF_WORD_BIT`, `OF_UINT64_BITFIELD`) live in `OCFBuild.h`, a packaged public header. A framework built with
them therefore ships a different `Headers/OCFBuild.h`, and its ad-hoc signature, which hashes the bundle's
resources, differs from one built before the port. The Mach-O contents up to the signature are identical in
both naming modes (checked 2026-11-07 against `main` before the port landed).

## Compatibility

| Area | wasm32 status |
| --- | --- |
| Objective-C runtime | objc4's Emscripten port, threads Release runtime (ARC, weak references, Blocks, Wasm exceptions) |
| CoreFoundation strings, numbers, arrays, dictionaries, sets, data | Working; covered by the smoke test |
| Foundation strings (formatting, searching, case mapping, scanning, regular expressions with ICU) | Working |
| Foundation collections (`NSArray`, `NSMutableArray`, `NSDictionary`, `NSSet` and their literals) | Working |
| `NSData`/`NSMutableData` | Port entry points over CFData (`Port/Foundation/ONSDataPort.m`) |
| Constant number literals (`@1`, `@1.5`, `@1.5f`) | Working; the floating-point classes are port implementations |
| Exceptions (`@throw`, `@catch`, `@finally`, NSException from restored methods) | Working |
| Run loops and dispatch | Timers and the main queue under Node; a browser's main thread cannot block, so its run loop must be driven from the event loop (not built) |
| Dispatch Mach sources and channels | `DISPATCH_SOURCE_TYPE_MACH_RECV`, `DISPATCH_SOURCE_TYPE_MACH_SEND` (dead names) and the receive side of `dispatch_mach_t` over libmach; `DISPATCH_SOURCE_TYPE_MEMORYPRESSURE` registers and never fires (no memory pressure signal in WebAssembly); the channels' send side, replies and `dispatch_mig_server` are not supplied (they fail to link) |
| Tagged pointer objects | None on wasm32 (objc4 enables them only with 64-bit pointers); CF's tagged-string paths answer their native fallbacks |
| `NSLocale` (Swift-implemented), and what reaches it (`+[NSLocale systemLocale]`, NSJSONSerialization writing) | Not translated yet |
| `NSDecimalNumber`, decimal scanning (Swift-implemented Decimal) | Trimmed, pending translation |
| `-forwardInvocation:` forwarding | Not restored; fast forwarding (`-forwardingTargetForSelector:`) works |

## Layout

| Path | What |
| --- | --- |
| `CMakeLists.txt` | The wasm build: OpenCoreFoundation, OpenFoundation, PortSystem and the Smoke test |
| `Trim.tsv` | The restored sources the port leaves out, with the reason and the replacement |
| `hooks.tsv` | Every hook site in `Source/` and why a library could not serve |
| `libdispatch/` | Apple's libdispatch-1542.100.32 (Apache 2.0), pinned by tag and commit, its patch series and build script |
| `System/` | Darwin's libSystem additions for Emscripten: `os_unfair_lock`, os_log and signposts, notify, XPC values, malloc zones, dyld and process queries, libc additions. Mach (ports and rights, `mach_msg` with descriptors, notifications, port sets, the MIG runtime, VM calls and memory entries, `mk_timer`, the clocks, the bootstrap name service) is libmach's, which `CMakeLists.txt` adds and `PortSystem` links |
| `CoreFoundation/`, `Foundation/` | Definitions in place of trimmed, Swift-implemented or unrestored native code |
| `Tests/Smoke.m` | A smoke test run under Node |
| `Tests/DispatchMach.c` | libdispatch's Mach sources and channels on libmach (`DispatchMachTest`) |

## Building

Needs the objc4 checkout (the sibling `../objc4`, or `OBJC4_ROOT`) with its Emscripten toolchain and the threads
runtime built (`scripts/prepare-wasm-toolchain.py`, `scripts/build-wasm.sh <Debug|Release> threads`), and the
libmach checkout (the sibling `../libmach`, or `LIBMACH_ROOT` for both the CMake build and
`Port/libdispatch/build.sh`), whose `CMakeLists.txt` the build adds. libmach's own tests run in its checkout
(`scripts/test-wasm.sh`).

```sh
Port/libdispatch/build.sh Release
source ../objc4/build/wasm-toolchain/env.sh
emcmake cmake -S Port -B .build/wasm/Release/OpenFoundation -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C .build/wasm/Release/OpenFoundation Smoke
node .build/wasm/Release/OpenFoundation/Smoke.js
ninja -C .build/wasm/Release/OpenFoundation DispatchMachTest
node .build/wasm/Release/OpenFoundation/DispatchMachTest.js
```

A Debug build of OpenFoundation can link objc4's Release runtime with `-DOBJC4_CONFIGURATION=Release`.

AddressSanitizer: `Port/libdispatch/build.sh Debug address`, then configure a separate build directory with
`-DOF_SANITIZE_ADDRESS=ON` (the restored code, the port and libdispatch are instrumented; objc4 is not, and
Objective-C metadata globals stay unpoisoned because objc4 walks them as arrays). `-DOF_PROXY_TO_PTHREAD=ON` runs
`main` on a pthread instead of the browser's thread.

## Platform choices

- The program's `main` runs on the browser's thread, the thread constructors run on and that CoreFoundation and
  libdispatch record as the main thread, so the main run loop drains the main dispatch queue. A browser does not
  let that thread block: Emscripten turns its futex waits into spins, so `CFRunLoopRunInMode` with a timeout
  spins there instead of sleeping. Node allows the wait.
- Mach is libmach's: Apple's headers and XNU's behaviour within one process (its
  `Documentation/Reference/WebAssembly.md` lists what wasm32 simplifies). Every unit sees its headers after
  Port/System's own, and `PortSystem` links it, so a consumer of `PortSystem` gets both.
- libdispatch's run-loop handle is a libmach receive right (patch 0004, `src/mach_emscripten.c`), so the restored
  CFRunLoop waits on it unchanged. libdispatch's Emscripten event backend handles timers, custom sources and Mach
  sources; descriptor and signal sources do not register.
- `src/mach_emscripten.c` (patch 0004) is the only file of libdispatch built with Mach headers (libmach's), so
  HAVE_MACH stays unset. It forwards the event backend's Mach waits (patch 0003) to libmach's port watches
  (`<libmach/wait.h>`): a watch of a receive right or port set is called back once per armed wait when a message
  is queued, a watch of a send right when its port dies. It defines `voucher_mach_msg_*` in the form libdispatch
  has without Mach vouchers, and the receive side of Mach channels over a receive source with the public
  dispatch objects, since libdispatch's `mach.c` needs HAVE_MACH (kevent message delivery, vouchers, special
  reply ports); they keep `mach.c`'s event order (CONNECTED first; DISCONNECTED, then CANCELED, last).
- `NSInteger` is `long` (`NS_BUILD_32_LIKE_64`) and `BOOL` is `bool`, as the arm64 restoration declares them.
- Dispatch objects are C objects (`OS_OBJECT_USE_OBJC=0`), as objc4 is built.
- Every `_os_feature_enabled_impl` flag is off, so the native code takes its non-Swift paths.
- ICU is Emscripten's port (68.2, stub data) with ICU's renaming force-included.
- The libraries link whole in Darwin's initialization order, libdispatch, libobjc, CoreFoundation, Foundation,
  because constructors of one priority run in link order; Foundation's initialization runs from a constructor of
  the OpenFoundation archive. libmach links after them, not whole, and initializes before all of them (its
  constructor has the earliest priority, as libSystem initializes Mach first).
- Indirect calls must match the callee's type exactly (wasm `call_indirect`), unlike arm64; a native function
  stored under another function type goes through a hook (`autoreleasePoolPopHook`).

## Status (2026-11-07)

Everything compiles and links (`Smoke.wasm`, about 4.5 MB in Release). Under Node, Debug, Release and the
AddressSanitizer build pass all 28 smoke checks: CFString creation and formatting (`%@`, `%d`, `%ld`, `%.2f`),
CFNumber, CFArray, CFDictionary, `+[NSString stringWithFormat:]`, `+[NSArray arrayWithObjects:]`,
`@throw`/`@catch`, an NSRangeException raised by a restored method, a CFRunLoopTimer, a block sent with
`dispatch_async` to the main queue drained by the main run loop, and seventeen regressions for the defects below
that the objc-wasm site (a browser application written in Objective-C on this port) found. AddressSanitizer
reports no memory errors; its leak report at exit lists process-lifetime state (the main thread's autorelease
page and CoreFoundation TSD).

Constant strings need objc4's compiler patch `0002-wasm32-cfstring-info-header.patch` (objc4 `main`):
wasm32 `@"..."` and `CFSTR` records carry the 64-bit info word at offset 8 that the restored CoreFoundation
reads.

Defects found on the way, now fixed:

- LP64 literals in the restored code: shifts by 32 or more of `long`-sized values (`OF_WORD_SHIFT`), 64-bit
  constants such as `NSNotFound`, raw offsets, and pointer-sized element counts written as `<< 3` or `* 8`
  (object arrays, property-list and run-loop stack buffers, ICU converter slots, deferred string moves). On
  arm64 the replacements evaluate to the same constants.
- `_CFRuntimeCreateInstance` asked an allocator that honors the zero hint for memory with hint 0; native
  0x1c01e5970 passes 1. Instances from the system allocator were not cleared (an Apple-build defect as well).
- libdispatch (patch 0002): without the cooperative workqueue, root-queue initialization wrote a thread pool's
  attributes and semaphore through the cooperative queues' null contexts.
- libdispatch initialized after CoreFoundation's constructor had used it: its first TSD write went to key 0,
  objc4's per-thread data (the "corrupted synchronization cache" on the main thread).
- Function types: the autorelease-pool pop hook, `_NSSearchPathsForDirectoryInDomain` (Swift calling
  convention) and the `utext_close` declaration.
- NSMutableArray's deque and CFArray's buffer took their capacity as the allocation size `>> 3`; with
  four-byte pointers that was half the slots, so the ring wrapped early (`addObject:` past the initial
  capacity read back earlier objects; `-componentsSeparatedByString:` answered a wrong last component). Now
  `(uint32_t)size / (uint32_t)sizeof(id)`, the same 32-bit shift on arm64 (and `size / sizeof(const void *)`
  in CFArray).
- `kNotFound` in ONSString.m was `(ONSUInteger)0x7FFFFFFFFFFFFFFF`, which truncates to `0xFFFFFFFF`, while
  `NSNotFound` is `LONG_MAX`. Now `LONG_MAX`.
- Immutable CFStrings with external contents (contents kinds 0x20, 0x40, 0x60) stored their length and read
  their contents deallocator at the raw arm64 offsets 0x18 and 0x20: the length read back as 0 and freeing the
  string called a garbage deallocator (`-stringByAppendingString:`, `+stringWithCharacters:length:` for long
  ASCII input, `-quotedStringRepresentation` and so collection descriptions). Now `offsetof` the fields.
- The string scanners (`-integerValue`, `-doubleValue`, NSScanner) send `-getCharacters:range:` through an
  `objc_msgSend` cast that splits the NSRange into two NSUInteger arguments, which wasm32 does not pass like
  the structure: hook `scanBufferFetch`.
- `_CFStringCreateIndirectASCIITaggedPointerString` built a tagged pointer that truncated to an invalid object
  (`NSStringFromSelector`): hook `indirectASCIITaggedPointerString` answers the native fallbacks.
- `OFPort_makeBlockClasses` gave objc4's concrete block classes NSBlock as their direct superclass, leaving a
  heap block with NSObject's `-retain` (an unaligned atomic access). They now get `__NSStackBlock`,
  `__NSMallocBlock` and `__NSGlobalBlock`, as the native function's second path does.
- `kCFAllocatorMalloc`'s context stored libsystem's `malloc`, `realloc`, `free`, `malloc_type_malloc` and
  `malloc_type_realloc` under CFAllocator callback types with extra arguments, so freeing a string with
  malloc'd external contents trapped in `CFAllocatorDeallocate`. Hook form `OF_PORT_FUNCTION` (a function
  address in a static initializer) substitutes functions of the exact callback types.
- `+[NSRegularExpression initialize]` sends `+setVersion:`; the port's NSObject category supplies `+version` and
  `+setVersion:` over `class_getVersion`/`class_setVersion`.

Open:

- **The main thread in a browser.** It cannot block; a run loop there needs to be driven from the browser's
  event loop rather than sleeping in `mach_msg` (not built).
- **Swift units.** Translated: `__NSConcreteUUID`, `_NSSwiftProcessInfo`, the locale notification counter and
  the search-path query. Decimal (about 2,400 lines of Swift) is trimmed with NSDecimalNumber and NSScanner's
  decimal scanning, pending translation.
- **Not restored but reached:** `CFRunLoopRun`, `CFRunLoopStop`, CoreFoundation's NSObject methods, the
  `NSData`/`NSMutableData` entry points and the constant floating-point number classes (the port supplies
  simple ones for these), full message forwarding to `-forwardInvocation:`, and `NSLocale`'s Swift
  implementation (`+_system`, `+_current`), which NSJSONSerialization's writer reaches.
- **objc4 Debug runtime.** Link the Release runtime (`-DOBJC4_CONFIGURATION=Release`); the Debug runtime's
  lock checking runs into the constructor order.
- **Remaining LP64 assumptions.** The smoke test covers a small part of the restored code; other literal sizes
  and offsets may remain outside the paths it reaches. Known candidates: the `0x7FFFFFFFFFFFFFFF` sentinels in
  `ONSAttributedStringMorphun.m`, `OCFDateComponents.c` and `OCFNSDateComponents.m`, and immutable CFString
  creation's extra-byte sizes (8 and 16), which over-allocate on wasm32 but are harmless.

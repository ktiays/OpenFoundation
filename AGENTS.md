# OpenFoundation Engineering Rules

## Goal

Restore, one to one, the part of the iOS Foundation and CoreFoundation
implementations that the consumers require, and run that restoration on other
platforms (owner decision, 2026-10-29). The consumers are OpenCoreAnimation and
OpenCoreGraphics; the objective is a cross-platform OpenCoreAnimation. The rest
of the framework surface is not a goal of this project.

Inside the declared scope ("Scope"), the native implementation's boundaries,
data layouts, ownership, call order, errors and observable behavior are the
architecture. Restoration comes before refactoring or a design of this project's
own; portability follows the restoration ("Porting"). Never describe the
result as an equivalent replacement of Foundation or CoreFoundation, only as a
restoration of its declared scope, and never describe an incomplete scope as
complete.

The engineering baseline follows OpenCoreAnimation and OpenCoreGraphics, as
requested on 2026-10-29. Their source trees are references for engineering
conventions and consumer requirements; they do not establish Foundation behavior.

## Language

All project documentation, source code, identifiers, comments, commit messages,
build output and test diagnostics use professional English unless the user
explicitly requests an exception. Conversation may use the user's language.

## Scope

- The scope is derived, never chosen by hand. Its roots are the CF/NS
  symbols, Objective-C classes and selectors that the consumers' admitted
  builds import from CoreFoundation and Foundation, measured on their linked
  binaries; a lexical source scan locates candidates and is not a root. The
  scope is those roots closed over the pinned images' call graphs in IDA.
- `Configuration/Scope.json` records the scope: consumer revisions and builds,
  every root, every native function in the closure with its image, address and
  UUID, the roots that reach it, and every cut. `Tools/scope.py` regenerates and
  checks it from evidence under `Local/Evidence/scope`. A root or function added
  without a consumer reaching it needs a recorded owner decision.
- A cut is where the closure leaves the scope: an edge into another image, or
  into native code the owner decides not to restore. Classify every cut:
  **restored** (the target joins the scope), **external** (another library the
  native image links, such as libobjc, libdispatch, ICU or libsystem; it is
  supplied, never reimplemented here), **downgraded** (the native code's own
  path when the dependency is absent) or **unsupported** (a build that needs it
  fails, naming the cut). Never cut with a stub, a success return or a fitted
  value.
- Indirect edges (Objective-C messages, CF callbacks, blocks, function
  pointers, Swift witnesses) are resolved by reading; an unresolved edge is
  recorded as unread and keeps its group open.
- Consumers' new requirements enter by regenerating the scope at their new
  revision. A scope reduction never removes a restored definition silently;
  record why it is no longer reached.

## Reading Apple Frameworks

- Read every implementation fact through the IDA MCP server: layouts, field
  order, constants, branch conditions, defaults, callbacks and call order.
  Behavioral probes and LLDB measurements confirm a reading; they do not replace it.
- Pin the OS version/build, architecture, image UUID and evidence source.
  `Configuration/ReferenceImages.json` is the identity manifest. Mach-O metadata,
  SDK declarations and consumer inventories establish identity or interfaces,
  never implementation behavior.
- Keep canonical binaries and IDA databases under
  `Local/Databases/iphone-simulator` and `Local/Databases/iphone-device`.
  Open an APFS clone in `Local/Scratch/<investigation>` (`cp -c`), never the
  canonical database. Pass the returned `database_id` on every IDA call and
  close the database when finished. Never unlock or terminate another session.
- Do not infer implementation from runtime introspection, class dumps, memory,
  old project code, third-party headers, or Apple's open-source releases.
  Those may locate a question; the pinned implementation settles it.
- Do not download or import another Foundation/CoreFoundation implementation
  as a substitute for reverse engineering. swift-corelibs-foundation,
  swift-foundation, Apple's CF releases and GNUstep may locate a question; they
  never supply code or settle behavior. A dependency is reused only when
  explicit native evidence establishes that dependency and its exact boundary.
  Preserve its licensing and provenance. Obtain the user's decision before
  changing this source-of-evidence policy.
- Cite the native function, IDA address, image UUID and confidence at the source
  definition, in its tests and in its reading. Mark uncertainty and unread code
  explicitly. Unread behavior is not built.
- Read a dependency group end to end before implementing it. Preserve one native
  function per restored function, its call order and native language boundaries.
  C linkage alone does not establish that the source language was C. Infer
  neither ARC nor manual ownership for a whole framework from another library.
- Never forward replacement operations to system Foundation or CoreFoundation.
  System calls are permitted in reference probes and differential tests only.
  Never add success-returning stubs, fabricated layouts, fitted values or partial
  dependency implementations to make a consumer link.
- Preserve ABI layout, signed zero, NaNs, floating-point operation order, thread
  state, exception behavior, retain/autorelease timing and callback ordering.
- Follow OpenCoreAnimation's defect policy: memory-safety defects, unintended
  leaks, data races and ordinary-use crashes are fixed only after establishing
  the native defect. Record the native function/address, confidence and precise
  divergence in the reading and at the definition (`Foundation defect:` or
  `CoreFoundation defect:`). Keep well-defined unusual behavior. Never trigger
  a known destructive native defect in a differential test.
- Do not modify installed frameworks or disable platform protections.

## Platforms and Stages

- Stage 1 matches iOS 27.0 Simulator (24A434), arm64. Stage 2 targets iOS 27.0.1
  iOS Device (24A446), arm64e reference images. Do not claim that a shared source
  version or matching simulator results establish iOS Device equivalence.
- Name the builds "iOS Simulator" and "iOS Device" in code and documentation.
- Compare every restored function with the iOS Device image before handoff.
  A difference carries an `iOS Device divergence:` comment naming the function,
  address and confidence, or stating that the iOS Device side remains unread.
- `TARGET` is where the code runs; `MATCH` is the implementation it reproduces.
  They are independent inputs, as in OpenCoreAnimation. Only pairs admitted by
  `Configuration/Platforms.json` build. Currently only
  `TARGET=iphonesimulator MATCH=iphonesimulator` is admitted. Unsupported pairs
  fail before compilation. A foreign TARGET is admitted only under "Porting";
  there is no other cross-platform fallback, and macOS behavior is never used
  silently.
- Use Apple's TargetConditionals macros for native platform conditions.
  arm64 is the only build architecture at this stage.

## Porting

Owner decision, 2026-10-29, following OpenCoreAnimation's porting rules: the
restored scope runs on other platforms without changing what any Apple build
compiles.

- **A port is a TARGET, never a MATCH.** `MATCH` stays whose implementation is
  reproduced; a port adds a `TARGET` (for example `android` or `linux`) that
  runs an existing MATCH's restoration. Restored function bodies are never
  edited for a port beyond the hooks below.
- **How a port changes behavior**, the first that serves:
  0. *Leave a cut out*: the TARGET's row in `Configuration/PlatformDependencies.tsv`
     marks a cut unsupported, or downgraded through the native code's own path.
  1. *Supply the API*: the TARGET gets the same lower-level C interfaces the
     restored code calls (Mach time, locks, libdispatch, libobjc, ICU,
     libsystem), as libraries on its link line or from `Port/`, so call sites
     stay as they are. The table classifies such an entry as **substituted**.
  2. *A hook*: one expression's value or a whole function, through the port
     hook macros, on every Apple TARGET expanding to the restored code alone.
     Each hook is a row of `Port/hooks.tsv` stating why 0–1 do not serve.
- **Supplied libraries** are recorded with provenance and license in the
  table: the objc4 Android port for libobjc, swift-corelibs-libdispatch for
  libdispatch, a pinned ICU for libicucore. A supplied library implements its
  own interface only; it never supplies Foundation or CoreFoundation behavior.
- **Nothing else enters `Source/`**: no port logic in a restored function, no
  `#if` on a port TARGET outside the hook header, no include of a port header.
  Port code lives in `Port/`.
- **Apple builds stay byte-identical.** A port change leaves the Release binary
  of every admitted Apple pair identical to main's (`cmp`).
- The port infrastructure (hook header, `Port/hooks.tsv`, the platform
  dependency table and its check) is not built yet. No port TARGET is admitted
  until it is and until every cut on that TARGET is classified.

## Naming and Coexistence

- Keep Foundation and CoreFoundation as distinct framework products:
  `OpenFoundation.framework` and `OpenCoreFoundation.framework`.
- Follow the siblings' declaration-derived naming switch. Source declarations
  spell `OCF` in place of `CF` and `ONS` in place of `NS`, after leading
  underscores or `k` where present: `OCFRetain`, `_OCFRuntimeRegisterClass`,
  `kOCFAllocatorDefault`, `ONSObject`, `ONSString`. Preserve native internal
  names unless an actual collision requires a mapping.
- One switch, `OF_SYSTEM_NAMES=NO|YES`, controls both products. The default
  `NO` preserves prefixed names for comparison alongside the system. `YES`
  maps declared names to native names through a generated force-included header.
  It is an isolated replacement build and cannot share a global Objective-C
  class table or C symbol namespace with the system implementation.
- `Tools/prefix-mapping.py` derives the mapping from declarations. Ordinary
  identifiers use concrete names, never naming macros. Runtime names use
  `OF_NAME_STRING(fullIdentifier)`. Extend declaration discovery and its
  regressions together before introducing a new declaration form. Unclassified
  exported names, native names outside CF/NS, C++ namespaces and category
  selectors on shared classes need an explicit recorded naming decision before
  they are exposed. No such names are implemented in the bootstrap.
- Generate installed headers with concrete selected names; never ship the
  implementation mapping or global aliases to system identifiers. The packaged
  `OCFBuild.h` rejects a conflicting consumer naming mode.
- `Configuration/PublicHeaders.json` is the shared install/visibility registry
  for both builds. Private source headers are never installed implicitly.
- Selectors of restored classes retain their native spelling. Categories on a
  system-owned class must not alter that class in coexistence mode.
- Prefixing does not establish object interoperability. Never pass restored
  CF/NS objects to native consumers, or native objects to restored consumers,
  without a separately read and validated boundary. Toll-free bridging, tagged
  pointers, constant strings, Objective-C class registration and allocator
  ownership are first-class restoration work.
- Infrastructure uses UpperCamelCase types, lowerCamelCase C/C++ functions,
  file-scope constants `kName`, and no borrowed native implementation names.
  Preserve recovered names and native boundaries in restored code.
- Use Apple's vocabulary: allocator, type registration, retain, release,
  autorelease pool, run loop, collection, encoding, archive, notification.
  Do not import frontend/React terminology, or Plan/Planner/Planning as
  architecture terminology.

## Source Layout and Build

- `Source/CoreFoundation` and `Source/Foundation` hold each product's sources
  and private headers, grouped by native subsystem. Follow recovered source
  paths when available; document inferred directory organization. Tests mirror
  those paths under `Tests` (differential) and `StandaloneTests` (isolated).
- Every production C-family implementation has a corresponding header. Include private
  headers from `Source/` using the full path, never a bare basename or `../`.
  Public imports use the framework name. Use `#pragma once`; `#import` for
  Objective-C headers and `#include` for C/C++ headers. Header maps stay off.
- Every first-party source/tool/configuration file starts with the Xcode header
  block, adapted to the file's comment syntax and actual creation date:

  ```
  //
  //  Created by ktiays on 2026/10/29.
  //  Copyright (c) 2026 ktiays. All rights reserved.
  //
  ```

  Third-party files retain their original headers. Structured data and project
  files use their native formats.
- Maintain the native Xcode project and direct Make build on the same sources,
  compiler policy and packaged headers. Xcode uses filesystem-synchronized
  groups, with separate membership for the two products and both test tools.
  Do not flatten the navigator or add explicit per-source file references.
- Shared defaults live in `Configuration/Build.xcconfig`. The Make compiler
  driver reads its literal assignments. C11 and C++20 are initial tooling
  settings, not claims about the native language. MRC is the initial Objective-C
  build setting; establish ownership for each restored unit before compiling it.
  Debug uses no LTO; Release uses full LTO, following the sibling build policy.
- Discover sources and formatter inputs recursively. Keep generated mappings
  outside every scanned source root. Build products stay under `.build` and
  are separated by TARGET, MATCH, configuration and naming mode.
- Link dependencies only when the first restored implementation calls them,
  using native load-command strength. Private SDK gaps use minimal text stubs
  with verified install names and called symbols. No high-level system
  Foundation/CoreFoundation dependency is allowed in production.

## Formatting

- `.clang-format` is byte-identical to OpenCoreAnimation's initialization
  baseline and authoritative for C, C++, Objective-C, Objective-C++ and headers.
  Swift uses Xcode's swift-format and `.swift-format` from OpenCoreGraphics.
- Four spaces, no tabs in source, a 110-column limit and attached braces.
  Pointers and references attach to the name (`int *ptr`, `Type &ref`).
- Always brace control structures, including single statements. One statement
  per line; no nonempty one-line functions, blocks, loops, ifs or case labels.
- Multiline parameters/arguments indent four spaces; the closing parenthesis
  has its own line, followed by ` {` for a definition. Separate definitions
  and declarations with a blank line. Indent switch cases within the switch.
- Run `make format` after source edits and `make format-check` before handoff.

## Progress and Verification

- `Configuration/Restoration.json` records restored definitions, native symbols,
  addresses, image UUIDs, confidence, readings, tests and iOS Device status.
  Only the two empty bootstrap units are exempt. `Tools/progress.py` and the
  prefix checker run in both build systems. No unrecorded translation unit or
  undeclared export may ship. A definition is marked restored only when its
  whole dependency group is independently implemented and tested; a group that
  reaches a cut is whole when every cut is classified under "Scope".
- Restored definitions outside `Configuration/Scope.json` are kept but
  reported separately; new restoration work starts only inside the scope.
  Progress is reported against the scope, never as a share of the framework.
- Progress follows the dependency graph, bottom up. No restored function may
  depend on a placeholder or a call into the system implementation. Strongly
  connected groups are read and restored together. ABI and bridging decisions
  precede collection or consumer integration work.
- `make test` runs isolated tests, reference-image guards and available public
  differential cases. `make validate` checks packaging/dependencies and internal
  and infrastructure regressions. `make coexistence` checks both header orders
  and linker orders in C, C++, Objective-C and Objective-C++. Keep test counts truthful; zero
  restored APIs is zero behavior coverage, even when bootstrap checks pass.
- Before handoff, check Debug and Release in both naming modes with Make and
  Xcode. Run simulator tests against the pinned runtime; each session gets its
  own simulator. Never share another investigation's booted simulator or derived
  data. System-name tests run without native Foundation/CoreFoundation loaded;
  differential tests run in prefixed mode.
- **Trace comparison** (owner decision, 2026-10-30). Object-runtime groups
  are never run beside the system implementation in one process: per-thread
  keys, tagged-pointer classes and the Objective-C class table are process
  global. They are compared across processes instead. One scenario source
  under `TraceTests`, written with native names, builds once against the
  system frameworks (reference) and once in `OF_SYSTEM_NAMES=YES` against the
  restored frameworks with no native Foundation/CoreFoundation loaded
  (replacement). Each run prints a normalized trace (values, bytes, type IDs,
  retain counts, descriptions, callback order, failures; object identities as
  first-seen indices, never addresses), and the two traces must match exactly.
  The reference traces are recorded per MATCH under `TraceTests/Recorded` with
  their image identity and serve as the expected traces of port TARGETs.
  Prefixed mode stays the in-process differential for pure functions and the
  compile/link coexistence check for object groups. No scenario observes
  pointer values, timing or other nondeterminism; one that needs it is recorded
  as unsupported, never normalized away.
- Resolve native and replacement functions independently. Verify their owning
  images and native UUID before calling; never compare the oracle with itself.
  Guard missing functions and fixtures. Setup failures exit 77, incompatibilities
  exit 1, success exits 0. Do not count setup failures as behavioral results.
- Preserve the native comparison policy: bytes, bit patterns, exceptions,
  ownership and event order as appropriate. Never widen a tolerance to pass.
  Validate lifetime changes with static analysis, ASan and allocation/leak
  checks. Performance claims require optimized builds and an iOS Device.
- Consumer replacement is accepted only after their source/build adaptation,
  exported/undefined-symbol checks and their own tests pass without system
  Foundation/CoreFoundation. An include substitution alone does not qualify.

## Tracks and History

- `Documentation/Tracks.md` is the work ledger: scope, owned paths,
  dependencies and state. Keep status there, not in per-turn reports.
- After the initial bootstrap, implementation tracks use isolated worktrees
  and `codex/<track>` branches. Independent tracks may run in parallel when
  authorized. Shared build configuration, public headers and integration belong
  to the coordinating session. Share verified interfaces before consumers.
- `main` stays linear. Land a completed track with `git merge --squash` and
  `git commit`, never a merge commit or a pull that merges. Rebase a track when
  main moves, validate the squashed result, and remove finished worktrees.
- No co-author trailers or attribution lines in commits: no `Co-Authored-By`,
  tool/model names, generated-with text or tool email addresses. Use an English
  subject and a body describing the change where needed.
- Do not push or publish merely because a local initialization was requested.

## Local Evidence and Documentation

- `Local/` is unversioned: canonical images/databases, raw evidence under
  `Local/Evidence`, scratch and APFS clones under `Local/Scratch/<investigation>`.
  Use this directory for research scratch so it survives reboots. Keep required
  evidence; remove obsolete clones, probes and derived builds when a track lands.
- `Documentation/README.md` is the index. The reference and subsystem readings
  specify the implementation, with confidence and unread boundaries explicit.
  Update existing readings before adding overlapping documents. Version concise
  conclusions and necessary manifests, never large binaries, databases or dumps.
- Keep documentation paths and commands verified. Do not copy sibling framework
  UUIDs, API counts, method ownership or implementation conclusions into this
  project's reference.

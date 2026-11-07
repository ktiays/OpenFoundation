#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Build both framework bundles directly with Apple's compiler, alongside the Xcode build."""

import argparse
import json
import re
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys
from common import (ROOT, PRODUCTS, SOURCE_SUFFIXES, apple_labels, build_directory, check_platform,
                    defaults, output, run)


def tool(name, *arguments):
    return run([sys.executable, ROOT / "Tools" / name, *arguments])


LABEL = re.compile(r'__asm(?:__)?\(\s*((?:"[^"]*"\s*|OF_NAME_STRING\(\w+\)\s*)+)\)')
ASSEMBLY_REFERENCE = re.compile(r'\b(?:bl|b|adrp|adr)\s+(_[A-Za-z_][A-Za-z0-9_$]*)')


def lto_preserved(product, name):
    """Symbols a product's sources name only through an assembler label or reference from inline assembly. Under
    full LTO such a reference is invisible to the optimizer, which would internalize and drop the definition, so the
    link keeps each of them (-u); the set is not exported and a non-LTO link is unchanged."""
    module = "CoreFoundation" if product == "OpenCoreFoundation" else "Foundation"
    symbols = set()
    for path in sorted((ROOT / "Source" / module).rglob("*")):
        if path.suffix not in SOURCE_SUFFIXES | {".h"}:
            continue
        text = apple_labels(path.read_text())
        for match in LABEL.finditer(text):
            parts = re.findall(r'"([^"]*)"|OF_NAME_STRING\((\w+)\)', match.group(1))
            label = "".join(literal or name(identifier) for literal, identifier in parts)
            if re.fullmatch(r"_[A-Za-z_][A-Za-z0-9_$.]*", label):
                symbols.add(label)
        for literal in re.findall(r'"((?:\\.|[^"\\])*)"', text):
            symbols.update(ASSEMBLY_REFERENCE.findall(literal))
    return sorted(symbols)


def link_dependencies(generated, mode):
    """Write each product's link response file (Configuration/LinkDependencies.json): libraries, re-exported
    libraries, upward text stubs of the other product (native LC_LOAD_UPWARD_DYLIB) and one weak text stub of the
    unbound symbols (unsupported cuts and unrestored dependencies), whose install name names no image, so every
    reference binds to null instead of loading a system library. Both build systems link with the response file."""
    config = json.loads((ROOT / "Configuration/LinkDependencies.json").read_text())

    def name(symbol):
        return re.sub(r"OCF(?=[A-Z])|ONS(?=[A-Z])", lambda match: match[0][1:], symbol, count=1) if mode == "YES" else symbol

    def stub(path, install_name, symbols, classes):
        path.write_text("--- !tapi-tbd\ntbd-version: 4\ntargets: [ arm64-ios-simulator ]\n"
                        "install-name: '" + install_name + "'\nexports:\n"
                        "  - targets: [ arm64-ios-simulator ]\n"
                        "    symbols: [ " + ", ".join("'" + item + "'" for item in symbols) + " ]\n"
                        "    objc-classes: [ " + ", ".join("'" + item + "'" for item in classes) + " ]\n...\n")

    generated.mkdir(parents=True, exist_ok=True)
    for product, row in config["products"].items():
        arguments = ["-l" + library for library in row.get("libraries", [])]
        arguments += ["-Wl,-reexport-l" + library for library in row.get("reexport_libraries", [])]
        for upward in row.get("upward", []):
            path = generated / (product + "-" + upward["product"] + "-upward.tbd")
            stub(path, upward["install_name"], [name(item) for item in upward.get("symbols", [])],
                 [name(item) for item in upward.get("objc_classes", [])])
            arguments.append("-Wl,-upward_library," + str(path))
        symbols, classes = [], []
        for group in row.get("unbound", {}).values():
            symbols += [name(item) for item in group.get("symbols", [])]
            symbols += group.get("symbols_by_mode", {}).get(mode, [])
            classes += [name(item) for item in group.get("objc_classes", [])]
        if symbols or classes:
            path = generated / (product + "-unbound.tbd")
            stub(path, "@rpath/" + product + "Unbound.dylib", symbols, classes)
            arguments.append("-Wl,-weak_library," + str(path))
        arguments += ["-Wl,-u," + symbol for symbol in lto_preserved(product, name)]
        (generated / (product + "-link.rsp")).write_text("\n".join(arguments) + "\n")
    # The test tools contain both products' objects in one image: their libraries and unbound stubs, no upward
    # stub and no re-export (Tools/build.py build_tests).
    arguments = ["-l" + library for row in config["products"].values()
                 for library in [*row.get("libraries", []), *row.get("reexport_libraries", [])]]
    arguments += ["-Wl,-weak_library," + str(generated / (product + "-unbound.tbd"))
                  for product in config["products"] if (generated / (product + "-unbound.tbd")).is_file()]
    # Under LTO an executable keeps no global symbol by default; the tests resolve replacements by name.
    arguments.append("-Wl,-export_dynamic")
    arguments += ["-Wl,-u," + symbol for symbol in sorted({symbol for product in config["products"]
                                                          for symbol in lto_preserved(product, name)})]
    (generated / "Tests-link.rsp").write_text("\n".join(arguments) + "\n")


def prepare(directory, mode):
    directory.mkdir(parents=True, exist_ok=True)
    generated = directory / "Generated"
    tool("check.py")
    link_dependencies(generated, mode)
    tool("prefix-mapping.py", "generate", generated / "OFPrefixMapping.h")
    tool("reference-images.py", "header", generated / "ReferencePins.h")
    for product in PRODUCTS:
        tool("prefix-mapping.py", "headers", directory / (product + ".framework"),
             "--product", product, "--system-names", mode)
    return generated


def framework_info(product):
    return {"CFBundleDevelopmentRegion": "en", "CFBundleExecutable": product,
            "CFBundleIdentifier": "me.ktiays." + product, "CFBundleInfoDictionaryVersion": "6.0",
            "CFBundleName": product, "CFBundlePackageType": "FMWK", "CFBundleShortVersionString": "0.1.0",
            "CFBundleVersion": "1", "CFBundleSupportedPlatforms": ["iPhoneSimulator"],
            "MinimumOSVersion": defaults()["IPHONEOS_DEPLOYMENT_TARGET"]}


def flags(directory, configuration, mode):
    config = defaults()
    sdk = output(["xcrun", "--sdk", "iphonesimulator", "--show-sdk-path"])
    return ["-target", "arm64-apple-ios" + config["IPHONEOS_DEPLOYMENT_TARGET"] + "-simulator",
            "-isysroot", sdk, "-F", str(directory), "-I", str(ROOT / "Source"),
            "-I", str(ROOT / "Tests"), "-I", str(ROOT / "StandaloneTests"),
            "-I", str(directory / "Generated"),
            "-include", str(directory / "Generated/OFPrefixMapping.h"),
            "-DOF_SYSTEM_NAMES=" + str(int(mode == "YES")), "-fvisibility=hidden", "-fblocks",
            "-ffp-contract=off", "-Wall", "-Wextra", "-Werror", "-g",
            *( ["-O0"] if configuration == "Debug" else ["-O2", "-flto"] )]


def compile_sources(paths, directory, configuration, mode):
    objects = []
    for path in paths:
        obj = directory / "Objects" / path.relative_to(ROOT).with_suffix(path.suffix + ".o")
        obj.parent.mkdir(parents=True, exist_ok=True)
        cpp = path.suffix in {".cpp", ".mm"}
        standard = defaults()["CLANG_CXX_LANGUAGE_STANDARD" if cpp else "GCC_C_LANGUAGE_STANDARD"]
        extra = ["-fno-objc-arc", "-fobjc-weak"] if path.suffix in {".m", ".mm"} else []
        run(["xcrun", "clang++" if cpp else "clang", *flags(directory, configuration, mode),
             "-std=" + standard, *extra, "-c", path, "-o", obj])
        objects.append(obj)
    return objects


def link_flags(directory, configuration):
    config = defaults()
    sdk = output(["xcrun", "--sdk", "iphonesimulator", "--show-sdk-path"])
    return ["-target", "arm64-apple-ios" + config["IPHONEOS_DEPLOYMENT_TARGET"] + "-simulator",
            "-isysroot", sdk, "-F", directory, *( ["-flto"] if configuration == "Release" else [] )]


def swift_build_module():
    import importlib.util
    spec = importlib.util.spec_from_file_location("build_swift", ROOT / "Tools/swift-build.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def build_frameworks(directory, configuration, mode):
    prepare(directory, mode)
    for product, module in zip(PRODUCTS, ("CoreFoundation", "Foundation")):
        sources = sorted(path for path in (ROOT / "Source" / module).rglob("*")
                         if path.suffix in SOURCE_SUFFIXES)
        objects = compile_sources(sources, directory, configuration, mode)
        swift = swift_build_module()
        swift_objects = swift.compile_swift(product, directory, configuration, mode)
        objects.extend(swift_objects)
        swift_flags = swift.link_flags() if swift_objects else []
        bundle = directory / (product + ".framework")
        executable = bundle / product
        dependency = (["-framework", "OpenCoreFoundation"] if product == "OpenFoundation"
                      else defaults()["OF_CORE_FOUNDATION_LDFLAGS"].split())
        response = directory / "Generated" / (product + "-link.rsp")
        if response.is_file():
            dependency.append("@" + str(response))
        driver = "clang++" if any(path.suffix in {".cpp", ".mm"} for path in sources) else "clang"
        run(["xcrun", driver, *link_flags(directory, configuration), "-dynamiclib", *objects,
             *dependency, *swift_flags, "-Wl,-install_name,@rpath/" + product + ".framework/" + product,
             "-Wl,-rpath,@loader_path/..", "-o", executable])
        (bundle / "Info.plist").write_bytes(plistlib.dumps(framework_info(product)))
        run(["xcrun", "dsymutil", executable, "-o", directory / (product + ".framework.dSYM")])
        run(["codesign", "--force", "--sign", "-", bundle], stdout=sys.stdout, stderr=sys.stderr)
    print("Built OpenCoreFoundation.framework and OpenFoundation.framework (" + configuration + ", " + mode + ").")


def framework_objects(directory):
    """The object files build_frameworks links into both products, in link order."""
    objects = []
    for product, module in zip(PRODUCTS, ("CoreFoundation", "Foundation")):
        sources = sorted(path for path in (ROOT / "Source" / module).rglob("*") if path.suffix in SOURCE_SUFFIXES)
        objects += [directory / "Objects" / path.relative_to(ROOT).with_suffix(path.suffix + ".o") for path in sources]
        swift = directory / "Objects/Swift" / (product + ".o")
        if swift.is_file():
            objects.append(swift)
    return objects


def build_tests(directory, configuration, mode):
    """Each test tool links the frameworks' own objects into one image (both products once, their hidden
    definitions reachable by the tests); the packaged frameworks are checked by validate."""
    prepare(directory, mode)
    swift_flags = swift_build_module().link_flags()
    for name, source_root in (("StandaloneTests", "StandaloneTests"), ("DifferentialTests", "Tests")):
        if name == "DifferentialTests" and mode == "YES":
            continue
        sources = sorted(path for path in (ROOT / source_root).rglob("*") if path.suffix in SOURCE_SUFFIXES)
        objects = compile_sources(sources, directory, configuration, mode)
        run(["xcrun", "clang++", *link_flags(directory, configuration), *objects, *framework_objects(directory),
             "@" + str(directory / "Generated/Tests-link.rsp"), *swift_flags, "-Wl,-rpath,@executable_path",
             "-o", directory / name])
        run(["codesign", "--force", "--sign", "-", directory / name])


def validate(directory, mode):
    ledger = json.loads((ROOT / "Configuration/Restoration.json").read_text())
    for product in PRODUCTS:
        framework = directory / (product + ".framework")
        executable = framework / product
        if not executable.is_file():
            raise SystemExit("Missing product: " + str(executable))
        libraries = output(["xcrun", "otool", "-L", executable])
        forbidden = ("/Foundation.framework/", "/CoreFoundation.framework/")
        if any(name in libraries for name in forbidden):
            raise SystemExit(product + " forwards to system Foundation/CoreFoundation.")
        symbols = output(["xcrun", "nm", "-gUj", executable]).splitlines()
        expected = set()
        for row in ledger["definitions"]:
            if row.get("product") != product or not row.get("exported"):
                continue
            if row.get("exported_symbols"):
                expected.update(row["exported_symbols"][mode])
            else:
                expected.add("_" + (row["native_symbol"] if mode == "YES" else row["symbol"]))
        if set(symbols) != expected:
            raise SystemExit(product + ": exported symbols differ from the restoration ledger: " + repr(symbols))
        for path in (framework / "Headers" / (product + ".h"), framework / "Modules/module.modulemap"):
            if not path.is_file():
                raise SystemExit("Missing packaged header/module: " + str(path))
        run(["codesign", "--verify", framework])
    client = directory / "packaging-client.c"
    client.write_text("#include <OpenFoundation/OpenFoundation.h>\n"
                      f"#if OF_SYSTEM_NAMES != {int(mode == 'YES')}\n#error Wrong packaged mode\n#endif\n")
    sdk = output(["xcrun", "--sdk", "iphonesimulator", "--show-sdk-path"])
    command = ["xcrun", "clang", "-target", "arm64-apple-ios27.0-simulator", "-isysroot", sdk,
               "-F", str(directory), "-fsyntax-only", str(client)]
    run(command)
    conflict = subprocess.run([*command, "-DOF_SYSTEM_NAMES=" + str(int(mode != "YES"))],
                              capture_output=True, text=True)
    if conflict.returncode == 0 or "Consumer naming mode does not match" not in conflict.stderr:
        raise SystemExit("Packaged headers did not reject an incompatible consumer naming mode.")
    exported = sum(bool(row.get("exported")) for row in ledger["definitions"])
    print(f"Packaging and dependency checks passed. Exported functions and data: {exported} ledger rows.")


def run_tests(directory, mode):
    from simulator import session, spawn
    with session() as udid:
        spawn(udid, directory / "StandaloneTests")
        if mode == "NO":
            spawn(udid, directory / "DifferentialTests")
        else:
            print("Differential tests require prefixed mode; system-name mode runs in isolation.")


def coexistence(directory, configuration, mode):
    if mode != "NO":
        raise SystemExit("Coexistence requires OF_SYSTEM_NAMES=NO.")
    for language, suffix in (("c", ".c"), ("objective-c", ".m"), ("c++", ".cpp"), ("objective-c++", ".mm")):
        objc = language.startswith("objective-c")
        cpp = language.endswith("++")
        product = "OpenFoundation" if objc else "OpenCoreFoundation"
        native = "Foundation" if objc else "CoreFoundation"
        includes = [f"#include <{product}/{product}.h>", f"#include <{native}/{native}.h>"]
        for order in (0, 1):
            path = directory / ("coexistence-" + language + "-" + str(order) + suffix)
            path.write_text("\n".join(includes[::1 if order == 0 else -1]) + "\nint main(void) { return 0; }\n")
            for link_order in (0, 1):
                libraries = [product, native]
                if link_order:
                    libraries.reverse()
                driver = "clang++" if cpp else "clang"
                run(["xcrun", driver, *flags(directory, configuration, mode), "-x", language, path,
                     "-std=" + defaults()["CLANG_CXX_LANGUAGE_STANDARD" if cpp else "GCC_C_LANGUAGE_STANDARD"],
                     *[argument for library in libraries for argument in ("-framework", library)],
                     "-framework", "OpenCoreFoundation", "-Wl,-rpath,@executable_path", "-o",
                     directory / (path.stem + "-" + str(link_order))])
    print("Header/link coexistence passed: 4 languages x 2 include orders x 2 link orders.")
    print("Object interoperability remains unimplemented.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("framework", "test", "validate", "coexistence", "xcode", "clean", "test-build"))
    parser.add_argument("--target", default="iphonesimulator")
    parser.add_argument("--match", default="iphonesimulator")
    parser.add_argument("--configuration", default="Debug")
    parser.add_argument("--system-names", default="NO")
    parser.add_argument("--products", type=Path)
    args = parser.parse_args()
    if args.action == "clean":
        shutil.rmtree(ROOT / ".build/Make", ignore_errors=True)
        return
    check_platform(args.target, args.match, args.configuration, args.system_names)
    directory = (args.products or build_directory(args.target, args.match, args.configuration, args.system_names)).resolve()
    if args.action == "xcode":
        run(["xcodebuild", "build", "-project", ROOT / "OpenFoundation.xcodeproj", "-scheme", "OpenFoundation",
             "-sdk", args.target, "-configuration", args.configuration,
             "-derivedDataPath", ROOT / ".build/Xcode" / (args.configuration + "-" + args.system_names),
             "OF_SYSTEM_NAMES=" + args.system_names, "OF_MATCH=" + args.match])
        return
    if not args.products:
        build_frameworks(directory, args.configuration, args.system_names)
    if args.action in ("test", "test-build") and not args.products:
        build_tests(directory, args.configuration, args.system_names)
    if args.action != "test-build":
        validate(directory, args.system_names)
    if args.action == "test":
        run_tests(directory, args.system_names)
    elif args.action == "validate":
        tool("check.py", "--self-test")
        print("Internal definition and naming infrastructure regressions passed; behavioral cases run in the test tools.")
    elif args.action == "coexistence":
        coexistence(directory, args.configuration, args.system_names)


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        raise SystemExit(error.returncode)

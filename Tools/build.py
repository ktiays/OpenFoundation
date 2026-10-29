#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Build both framework bundles directly with Apple's compiler, alongside the Xcode build."""

import argparse
import json
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys
from common import (ROOT, PRODUCTS, SOURCE_SUFFIXES, build_directory, check_platform,
                    defaults, internal_test_sources, output, run)


def tool(name, *arguments):
    return run([sys.executable, ROOT / "Tools" / name, *arguments])


def prepare(directory, mode):
    directory.mkdir(parents=True, exist_ok=True)
    generated = directory / "Generated"
    tool("check.py")
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
        extra = ["-fno-objc-arc"] if path.suffix in {".m", ".mm"} else []
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
        driver = "clang++" if any(path.suffix in {".cpp", ".mm"} for path in sources) else "clang"
        run(["xcrun", driver, *link_flags(directory, configuration), "-dynamiclib", *objects,
             *dependency, *swift_flags, "-Wl,-install_name,@rpath/" + product + ".framework/" + product,
             "-Wl,-rpath,@loader_path/..", "-o", executable])
        (bundle / "Info.plist").write_bytes(plistlib.dumps(framework_info(product)))
        run(["xcrun", "dsymutil", executable, "-o", directory / (product + ".framework.dSYM")])
        run(["codesign", "--force", "--sign", "-", bundle], stdout=sys.stdout, stderr=sys.stderr)
    print("Built OpenCoreFoundation.framework and OpenFoundation.framework (" + configuration + ", " + mode + ").")


def build_tests(directory, configuration, mode):
    prepare(directory, mode)
    for name, source_root in (("StandaloneTests", "StandaloneTests"), ("DifferentialTests", "Tests")):
        if name == "DifferentialTests" and mode == "YES":
            continue
        sources = sorted(path for path in (ROOT / source_root).rglob("*") if path.suffix in SOURCE_SUFFIXES)
        sources.extend(internal_test_sources())
        objects = compile_sources(sources, directory, configuration, mode)
        run(["xcrun", "clang", *link_flags(directory, configuration), *objects,
             "-Wl,-needed_framework,OpenCoreFoundation", "-Wl,-needed_framework,OpenFoundation",
             *defaults()["OF_CORE_FOUNDATION_LDFLAGS"].split(),
             "-Wl,-rpath,@executable_path", "-o", directory / name])
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
        expected = {"_" + (row["native_symbol"] if mode == "YES" else row["symbol"]) for row in ledger["definitions"]
                    if row.get("product") == product and row.get("exported")}
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
    print(f"Packaging and dependency checks passed. Exported restored functions/data: {exported}.")


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

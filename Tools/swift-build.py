#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Compile selected-name Swift implementations identically for Make and Xcode."""

import argparse
import importlib.util
import json
from pathlib import Path
import re
import shutil
import subprocess
from common import ROOT, defaults, output, run, write_changed


def prefix_module():
    spec = importlib.util.spec_from_file_location("swift_prefix", ROOT / "Tools/prefix-mapping.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def swift_configuration(product, root=ROOT):
    path = root / "Configuration/SwiftSources.json"
    if not path.exists():
        return None
    return json.loads(path.read_text())["products"].get(product)


def translate_source(text, mapping):
    text = prefix_module().translate(text, mapping)
    return re.sub(
        r'@_(cdecl|silgen_name)\(\s*"([^"\n]+)"\s*\)',
        lambda match: '@_' + match[1] + '("' + mapping.get(match[2], match[2]) + '")', text
    )


def prepare_sources(product, directory, mode, root=ROOT):
    module = "CoreFoundation" if product == "OpenCoreFoundation" else "Foundation"
    sources = sorted((root / "Source" / module).rglob("*.swift"))
    if not sources:
        return [], None
    config = swift_configuration(product, root)
    if not config:
        raise SystemExit("Swift source has no recorded module/bridge policy: " + product)
    prefix = prefix_module()
    mapping = prefix.discover(root / "Source")
    selected = mapping if mode == "YES" else {}
    generated = directory / "Generated/Swift"
    paths = []
    for path in sources:
        destination = generated / path.relative_to(root).parent / ("OF_" + path.name)
        write_changed(destination, translate_source(path.read_text(), selected))
        paths.append(destination)
    imports = directory / "Generated/SwiftImports"
    for name in ("OpenCoreFoundation", "OpenFoundation"):
        origin = directory / (name + ".framework/Headers")
        target = imports / (name + ".framework/Headers")
        target.mkdir(parents=True, exist_ok=True)
        for header in origin.rglob("*.h"):
            write_changed(target / header.relative_to(origin), header.read_text())
        modulemap = target.parent / "Modules"
        if modulemap.exists():
            shutil.rmtree(modulemap)
    bridge = imports / (product + "Bridge.h")
    headers = []
    for name in config["bridge_headers"]:
        path = root / "Source" / name
        if not path.is_file() or (root / "Source").resolve() not in path.resolve().parents:
            raise SystemExit("Invalid private Swift bridge header: " + name)
        headers.append('#include "' + str(path) + '"')
    write_changed(bridge, "// Generated private textual bridge; never installed.\n" + "\n".join(headers) + "\n")
    return paths, bridge


def compiler_arguments(product, directory, configuration, mode, root=ROOT):
    sources, bridge = prepare_sources(product, directory, mode, root)
    if not sources:
        return [], []
    config = swift_configuration(product, root)
    shared = defaults()
    sdk = output(["xcrun", "--sdk", "iphonesimulator", "--show-sdk-path"])
    arguments = ["xcrun", "swiftc", "-parse-as-library", "-whole-module-optimization",
                 "-module-name", config["module_names"][mode], "-swift-version", "6",
                 "-target", "arm64-apple-ios" + shared["IPHONEOS_DEPLOYMENT_TARGET"] + "-simulator",
                 "-sdk", sdk, "-F", directory / "Generated/SwiftImports", "-I", root / "Source",
                 "-import-objc-header", bridge, "-Xcc", "-include", "-Xcc",
                 directory / "Generated/OFPrefixMapping.h", "-Xcc", "-DOF_SYSTEM_NAMES=" + str(int(mode == "YES")),
                 "-module-cache-path", directory / "Generated/SwiftModuleCache",
                 "-g", "-Onone" if configuration == "Debug" else "-O"]
    if configuration == "Release":
        arguments.append("-lto=llvm-full")
    return [str(value) for value in arguments], sources


def compile_swift(product, directory, configuration, mode, root=ROOT):
    arguments, sources = compiler_arguments(product, directory, configuration, mode, root)
    if not sources:
        return []
    destination = directory / "Objects/Swift" / (product + ".o")
    destination.parent.mkdir(parents=True, exist_ok=True)
    run([*arguments, "-emit-object", *sources, "-o", destination])
    return [destination]


def link_flags():
    sdk = Path(output(["xcrun", "--sdk", "iphonesimulator", "--show-sdk-path"]))
    compiler = Path(output(["xcrun", "--find", "swiftc"]))
    runtime = compiler.parents[1] / "lib/swift/iphonesimulator"
    return ["-L", sdk / "usr/lib/swift", "-L", runtime, "-Wl,-rpath,/usr/lib/swift"]


def dump_ast(product, directory, mode="NO", root=ROOT):
    arguments, sources = compiler_arguments(product, directory, "Debug", mode, root)
    if not sources:
        return None
    arguments = [argument for argument in arguments if argument != "-whole-module-optimization"]
    arguments.insert(2, "-frontend")
    process = subprocess.run([*arguments, "-dump-ast", "-dump-ast-format", "json", *map(str, sources)],
                             capture_output=True, text=True)
    if process.returncode:
        raise SystemExit("Swift definition audit could not type-check sources:\n" + process.stderr)
    for text in (process.stdout, process.stderr):
        start = text.find("{")
        if start >= 0:
            try:
                return json.loads(text[start:])
            except json.JSONDecodeError:
                pass
    raise SystemExit("Swift compiler did not produce a readable JSON AST.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("product", choices=("OpenCoreFoundation", "OpenFoundation"))
    parser.add_argument("--products", type=Path, required=True)
    parser.add_argument("--configuration", choices=("Debug", "Release"), required=True)
    parser.add_argument("--system-names", choices=("NO", "YES"), required=True)
    parser.add_argument("--root", type=Path, default=ROOT)
    args = parser.parse_args()
    compile_swift(args.product, args.products.resolve(), args.configuration, args.system_names, args.root.resolve())


if __name__ == "__main__":
    main()

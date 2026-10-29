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
                 "-g", "-warnings-as-errors", "-Onone" if configuration == "Debug" else "-O"]
    if configuration == "Release":
        arguments.append("-lto=llvm-full")
    return [str(value) for value in arguments], sources


def compile_swift(product, directory, configuration, mode, root=ROOT):
    arguments, sources = compiler_arguments(product, directory, configuration, mode, root)
    if not sources:
        return []
    destination = directory / "Objects/Swift" / (product + ".o")
    destination.parent.mkdir(parents=True, exist_ok=True)
    run([*arguments, "-emit-object", *sources, "-o", destination], cwd=directory)
    return [destination]


def link_flags():
    sdk = Path(output(["xcrun", "--sdk", "iphonesimulator", "--show-sdk-path"]))
    compiler = Path(output(["xcrun", "--find", "swiftc"]))
    runtime = compiler.parents[1] / "lib/swift/iphonesimulator"
    return ["-L", sdk / "usr/lib/swift", "-L", runtime, "-Wl,-rpath,/usr/lib/swift"]


def dump_ast(product, directory, mode="NO", root=ROOT):
    arguments, sources = compiler_arguments(product, directory, "Debug", mode, root)
    if not sources:
        return []
    arguments = [argument for argument in arguments if argument != "-whole-module-optimization"]
    arguments.insert(2, "-frontend")
    trees = []
    for primary in sources:
        command = [*arguments, "-dump-ast", "-dump-ast-format", "json", "-primary-file", str(primary),
                   *[str(path) for path in sources if path != primary]]
        process = subprocess.run(command, capture_output=True, text=True)
        if process.returncode:
            raise SystemExit("Swift definition audit could not type-check sources:\n" + process.stderr)
        for text in (process.stdout, process.stderr):
            start = text.find("{")
            if start >= 0:
                try:
                    trees.append(json.loads(text[start:]))
                    break
                except json.JSONDecodeError:
                    pass
        else:
            raise SystemExit("Swift compiler did not produce a readable JSON AST.")
    return trees


def source_definitions(product, directory, root=ROOT):
    definitions = {}
    generated_root = directory / "Generated/Swift"
    for tree in dump_ast(product, directory, root=root):
        generated = Path(tree["filename"])
        relative = generated.relative_to(generated_root)
        name = relative.name.removeprefix("OF_") if hasattr(str, "removeprefix") else relative.name[3:]
        path = str(relative.parent / name)

        def visit(node, in_function=False, in_type=False):
            if isinstance(node, list):
                for child in node:
                    visit(child, in_function, in_type)
                return
            if not isinstance(node, dict):
                return
            kind = node.get("_kind")
            implicit = node.get("implicit", False)
            usr = node.get("usr")
            function = kind in {"func_decl", "accessor_decl", "constructor_decl", "destructor_decl"}
            declared_type = kind in {"struct_decl", "enum_decl", "class_decl"}
            if usr and not implicit:
                definition_kind = None
                if function and node.get("body"):
                    definition_kind = "function"
                elif declared_type:
                    definition_kind = "type"
                elif kind == "var_decl" and not in_function and not in_type and node.get("readImpl") == "stored":
                    definition_kind = "data"
                if definition_kind:
                    definitions[(path, usr)] = {"kind": definition_kind, "range": node.get("range"),
                                                "name": node.get("name"), "attrs": node.get("attrs", [])}
            for value in node.values():
                if isinstance(value, (dict, list)):
                    visit(value, in_function or function, in_type or declared_type or kind == "extension_decl")

        visit(tree)
    return definitions


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

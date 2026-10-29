#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Require an exact definition ledger, with binary readings and independent verification."""

import importlib.util
import json
from pathlib import Path
import re
import subprocess
import tempfile
from common import ROOT, PRODUCTS, SOURCE_SUFFIXES, defaults, output


def location_file(node, inherited):
    location = node.get("loc", node.get("range", {}).get("begin", {}))
    location = location.get("expansionLoc", location)
    name = location.get("file")
    return Path(name).resolve() if name else inherited


def definitions_from_ast(tree, source_root):
    definitions = set()
    source_root = source_root.resolve()

    def visit(node, inherited, in_function=False, objc_owner=None):
        path = location_file(node, inherited)
        kind = node.get("kind")
        owned = path and (path == source_root or source_root in path.parents)
        implicit = node.get("isImplicit", False)
        children = node.get("inner", [])
        body = any(child.get("kind") in {"CompoundStmt", "CXXTryStmt"} for child in children)
        if kind in {"ObjCImplementationDecl", "ObjCCategoryImplDecl"}:
            objc_owner = node.get("name")
            if kind == "ObjCCategoryImplDecl":
                interface = node.get("classInterface", {}).get("name", "<unknown>")
                objc_owner = interface + "(" + objc_owner + ")"
            if owned and not implicit:
                definitions.add((str(path), objc_owner))
        function_kind = kind in {"FunctionDecl", "CXXMethodDecl", "CXXConstructorDecl",
                                 "CXXDestructorDecl", "CXXConversionDecl", "ObjCMethodDecl"}
        if owned and not implicit:
            if function_kind and (body or node.get("explicitlyDefaulted")):
                symbol = node.get("name")
                if kind == "ObjCMethodDecl":
                    prefix = "-" if node.get("instance", True) else "+"
                    symbol = prefix + "[" + objc_owner + " " + symbol + "]"
                elif kind != "FunctionDecl" or node.get("mangledName", "").startswith("_Z"):
                    symbol = node.get("mangledName", symbol)
                definitions.add((str(path), symbol))
            elif kind == "VarDecl" and (not in_function or node.get("storageClass") == "static"):
                if node.get("storageClass") != "extern" or "init" in node:
                    definitions.add((str(path), node["name"]))
        for child in children:
            visit(child, path, in_function or function_kind, objc_owner)

    previous = None
    for node in tree.get("inner", []):
        previous = location_file(node, previous)
        visit(node, previous)
    return definitions


def collect_definitions(paths, source_root=None):
    """Use compiler syntax, including hidden functions/data and header definitions."""
    source_root = source_root or ROOT / "Source"
    spec = importlib.util.spec_from_file_location("progress_prefix", ROOT / "Tools/prefix-mapping.py")
    prefix = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(prefix)
    mapping = prefix.discover(ROOT / "Source")
    parent = ROOT / ".build/Progress"
    parent.mkdir(parents=True, exist_ok=True)
    config = defaults()
    sdk = output(["xcrun", "--sdk", "iphonesimulator", "--show-sdk-path"])
    result = set()
    with tempfile.TemporaryDirectory(dir=parent) as directory:
        directory = Path(directory)
        for product in PRODUCTS:
            prefix.package(product, directory / (product + ".framework"), "NO", mapping)
        for path in sorted(paths):
            cpp = path.suffix in {".cpp", ".mm"}
            command = ["xcrun", "clang++" if cpp else "clang", "-target",
                       "arm64-apple-ios" + config["IPHONEOS_DEPLOYMENT_TARGET"] + "-simulator",
                       "-isysroot", sdk, "-F", directory, "-I", ROOT / "Source", "-fblocks",
                       "-DOF_SYSTEM_NAMES=0", "-std=" + config[
                           "CLANG_CXX_LANGUAGE_STANDARD" if cpp else "GCC_C_LANGUAGE_STANDARD"],
                       "-Xclang", "-ast-dump=json", "-fsyntax-only", path]
            if path.suffix in {".m", ".mm"}:
                command.insert(2, "-fno-objc-arc")
            process = subprocess.run([str(item) for item in command], capture_output=True, text=True)
            if process.returncode:
                raise SystemExit("Definition audit could not parse " + str(path) + ":\n" + process.stderr)
            result.update(definitions_from_ast(json.loads(process.stdout), source_root))
    return {(str(Path(path).relative_to(ROOT)), symbol) for path, symbol in result}


def check_definition_records(actual, recorded):
    if actual != recorded:
        missing = sorted(actual - recorded)
        extra = sorted(recorded - actual)
        raise SystemExit("Production definition/ledger mismatch. Unrecorded: " + repr(missing) +
                         "; absent definitions: " + repr(extra))


def self_test():
    scratch = ROOT / ".build/ProgressRegression"
    scratch.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=scratch) as directory:
        source = Path(directory) / "Definitions.c"
        source.write_text("extern int declaredOnly(void);\n"
                          "extern int externalData;\n"
                          "static const unsigned char recordedData[2] = {1, 2};\n"
                          "static int hiddenFunction(void) { return recordedData[0]; }\n"
                          "int exportedFunction(void) { return hiddenFunction(); }\n")
        header = Path(directory) / "Definitions.h"
        header.write_text("static inline int headerFunction(void) { return 3; }\n")
        source.write_text('#include "Definitions.h"\n' + source.read_text())
        objc = Path(directory) / "Definitions.m"
        objc.write_text("__attribute__((objc_root_class)) @interface AuditObject\n"
                        "- (int)value;\n@end\n"
                        "@implementation AuditObject\n- (int)value { return 7; }\n@end\n")
        actual = collect_definitions([source, objc], Path(directory))
        path = str(source.relative_to(ROOT))
        expected = {(path, "recordedData"), (path, "hiddenFunction"), (path, "exportedFunction"),
                    (str(header.relative_to(ROOT)), "headerFunction"),
                    (str(objc.relative_to(ROOT)), "AuditObject"),
                    (str(objc.relative_to(ROOT)), "-[AuditObject value]")}
        check_definition_records(actual, expected)
        for invalid in (expected - {(path, "hiddenFunction")}, expected | {(path, "inventedFunction")}):
            try:
                check_definition_records(actual, invalid)
            except SystemExit:
                pass
            else:
                raise SystemExit("Definition audit accepted an unrecorded or absent native definition.")
    print("Definition-ledger regressions passed: data, header/hidden/exported functions, "
          "Objective-C classes/methods and missing/extra rows.")


def check():
    ledger = json.loads((ROOT / "Configuration/Restoration.json").read_text())
    paths = {path for path in (ROOT / "Source").rglob("*") if path.suffix in SOURCE_SUFFIXES}
    exclusions = {row["path"] for row in ledger["exclusions"]}
    expected_bootstrap = {"Source/CoreFoundation/Bootstrap.c", "Source/Foundation/Bootstrap.c"}
    if exclusions != expected_bootstrap:
        raise SystemExit("Only the two empty bootstrap translation units may bypass the restoration ledger.")
    for name in exclusions:
        text = (ROOT / name).read_text()
        code = re.sub(r"//[^\n]*|/\*[\s\S]*?\*/|^\s*#.*$|_Static_assert\([^;]*;", "", text,
                      flags=re.MULTILINE)
        if code.strip():
            raise SystemExit(f"{name}: the bootstrap must remain empty.")
    restored_paths = set()
    definitions = set()
    references = json.loads((ROOT / "Configuration/ReferenceImages.json").read_text())
    simulator_images = references["iphone-simulator"]["images"]
    uuids = {row["uuid"] for row in simulator_images.values()}
    for row in ledger["definitions"]:
        for key in ("path", "symbol", "native_symbol", "address", "reference_uuid", "confidence",
                    "reading", "tests", "ios_device"):
            if not row.get(key):
                raise SystemExit("Missing restoration field: " + key)
        if row["confidence"] != "verified":
            raise SystemExit("Unread or inferred code cannot be marked restored: " + row["symbol"])
        if row["reference_uuid"] not in uuids:
            raise SystemExit("Definition is not pinned to a simulator reference image: " + row["symbol"])
        key = (row["path"], row["symbol"])
        if key in definitions:
            raise SystemExit("Duplicate restoration definition: " + repr(key))
        definitions.add(key)
        restored_paths.add(row["path"])
        for name in [row["reading"], *row["tests"]]:
            if not (ROOT / name).is_file():
                raise SystemExit("Missing reading or test: " + name)
    translation_units = {str(path.relative_to(ROOT)) for path in paths}
    recorded_units = {path for path in restored_paths if Path(path).suffix in SOURCE_SUFFIXES}
    if translation_units != recorded_units | exclusions:
        mismatch = translation_units ^ (recorded_units | exclusions)
        raise SystemExit("Production source/ledger mismatch: " + repr(mismatch))
    check_definition_records(collect_definitions(paths), definitions)
    functions = sum(row.get("kind", "function") == "function" for row in ledger["definitions"])
    print(f"Restoration ledger passed: {len(definitions)} definitions ({functions} functions). "
          "Bootstrap is not API coverage.")


if __name__ == "__main__":
    check()

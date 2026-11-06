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


def parse_definitions(job):
    """One translation unit's definitions (a worker of collect_definitions)."""
    path, command, source_root = job
    process = subprocess.run([str(item) for item in command], capture_output=True, text=True)
    if process.returncode:
        raise SystemExit("Definition audit could not parse " + str(path) + ":\n" + process.stderr)
    return definitions_from_ast(json.loads(process.stdout), source_root)


def collect_definitions(paths, source_root=None):
    """Use compiler syntax, including hidden functions/data and header definitions. Translation units are
    parsed in parallel worker processes."""
    import multiprocessing
    import sys
    import types
    source_root = source_root or ROOT / "Source"
    spec = importlib.util.spec_from_file_location("progress_prefix", ROOT / "Tools/prefix-mapping.py")
    prefix = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(prefix)
    mapping = prefix.discover(ROOT / "Source")
    parent = ROOT / ".build/Progress"
    parent.mkdir(parents=True, exist_ok=True)
    config = defaults()
    sdk = output(["xcrun", "--sdk", "iphonesimulator", "--show-sdk-path"])
    # Workers are forked; the worker function is found through this module's entry in sys.modules, which a
    # module loaded by path (another tool's import) does not have until it is registered here.
    if sys.modules.get(__name__) is None or getattr(sys.modules[__name__], "parse_definitions", None) is not parse_definitions:
        registered = types.ModuleType(__name__)
        registered.__dict__.update(globals())
        sys.modules[__name__] = registered
    result = set()
    with tempfile.TemporaryDirectory(dir=parent) as directory:
        directory = Path(directory)
        for product in PRODUCTS:
            prefix.package(product, directory / (product + ".framework"), "NO", mapping)
        jobs = []
        for path in sorted(paths):
            cpp = path.suffix in {".cpp", ".mm"}
            command = ["xcrun", "clang++" if cpp else "clang", "-target",
                       "arm64-apple-ios" + config["IPHONEOS_DEPLOYMENT_TARGET"] + "-simulator",
                       "-isysroot", sdk, "-F", directory, "-I", ROOT / "Source", "-fblocks",
                       "-DOF_SYSTEM_NAMES=0", "-std=" + config[
                           "CLANG_CXX_LANGUAGE_STANDARD" if cpp else "GCC_C_LANGUAGE_STANDARD"],
                       "-Xclang", "-ast-dump=json", "-fsyntax-only", path]
            if path.suffix in {".m", ".mm"}:
                command[2:2] = ["-fno-objc-arc", "-fobjc-weak"]
            jobs.append((path, command, source_root))
        if len(jobs) > 1:
            with multiprocessing.get_context("fork").Pool() as pool:
                for definitions in pool.imap_unordered(parse_definitions, jobs):
                    result.update(definitions)
        else:
            for job in jobs:
                result.update(parse_definitions(job))
    return {(str(Path(path).relative_to(ROOT)), symbol) for path, symbol in result}


def collect_swift_definitions():
    if not list((ROOT / "Source").rglob("*.swift")):
        return {}
    spec = importlib.util.spec_from_file_location("progress_swift", ROOT / "Tools/swift-build.py")
    swift = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(swift)
    prefix = swift.prefix_module()
    mapping = prefix.discover(ROOT / "Source")
    parent = ROOT / ".build/Progress"
    parent.mkdir(parents=True, exist_ok=True)
    result = {}
    with tempfile.TemporaryDirectory(dir=parent) as directory:
        directory = Path(directory)
        for product in PRODUCTS:
            prefix.package(product, directory / (product + ".framework"), "NO", mapping)
        generated = directory / "Generated"
        generated.mkdir(exist_ok=True)
        (generated / "OFPrefixMapping.h").write_text(prefix.mapping_header(mapping))
        for product in PRODUCTS:
            result.update(swift.source_definitions(product, directory))
    return result


def check_swift_records(rows, actual):
    explicit = [row for row in rows if not row.get("compiler_generated")]
    recorded = {(row["path"], row.get("source_usr")) for row in explicit}
    check_definition_records(set(actual), recorded)
    by_symbol = {(row["path"], row["symbol"]): row for row in explicit}
    for row in rows:
        if not row.get("compiler_generated"):
            continue
        source = by_symbol.get((row["path"], row.get("source_definition")))
        if not source or not row.get("emitted_symbol"):
            raise SystemExit("Generated Swift entry has no verified authored source: " + row["symbol"])
        node = actual[(source["path"], source["source_usr"])]
        bindings = {"_" + attribute["name"] for attribute in node["attrs"]
                    if attribute.get("_kind") == "cdecl_attr"}
        if row["emitted_symbol"] not in bindings:
            raise SystemExit("Generated Swift entry is not its source's actual C binding: " + row["symbol"])


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


RESTORED_FIELDS = ("path", "symbol", "native_symbol", "address", "reference_uuid", "confidence", "reading", "tests",
                   "ios_device")
# Issues an implemented definition may carry (AGENTS.md, "Progress and Verification"). Each is required exactly
# when its condition holds, so a gap cannot go unrecorded.
ISSUE_CODES = {"untested", "reading-unversioned", "native-identity-uncited", "confidence-below-verified",
               "device-unread", "open-edges"}


def check_restored_row(row, uuids):
    for key in RESTORED_FIELDS:
        if not row.get(key):
            raise SystemExit("Missing restoration field: " + key)
    if row["confidence"] != "verified":
        raise SystemExit("Unread or inferred code cannot be marked restored: " + row["symbol"])
    if row["reference_uuid"] not in uuids:
        raise SystemExit("Definition is not pinned to a simulator reference image: " + row["symbol"])
    for name in [row["reading"], *row["tests"]]:
        if not (ROOT / name).is_file():
            raise SystemExit("Missing reading or test: " + name)


def check_implemented_row(row, uuids):
    """An implemented definition is built from a reading but not yet restored; every gap is an issue."""
    symbol = row["symbol"]
    issues = row.get("issues")
    if not isinstance(issues, list) or not issues:
        raise SystemExit("Implemented definition without recorded issues: " + symbol)
    codes = [issue.get("code") for issue in issues]
    unknown = set(codes) - ISSUE_CODES
    if unknown or len(set(codes)) != len(codes):
        raise SystemExit("Unknown or repeated issue codes for " + symbol + ": " + repr(codes))
    if any(not issue.get("detail") for issue in issues):
        raise SystemExit("Issue without detail for " + symbol)
    for key in ("product", "kind"):
        if not row.get(key):
            raise SystemExit("Missing restoration field for " + symbol + ": " + key)

    def require(condition, code):
        if bool(condition) != (code in codes):
            raise SystemExit(f"{symbol}: issue '{code}' must be recorded exactly when it applies")

    tests = row.get("tests", [])
    for name in tests:
        if not (ROOT / name).is_file():
            raise SystemExit("Missing test: " + name)
    require(not tests, "untested")
    reading = row.get("reading")
    if reading and not (ROOT / reading).is_file():
        raise SystemExit("Missing reading: " + reading)
    require(not reading, "reading-unversioned")
    identity = all(row.get(key) for key in ("native_symbol", "address", "reference_uuid"))
    if identity and row["reference_uuid"] not in uuids:
        raise SystemExit("Definition is not pinned to a simulator reference image: " + symbol)
    require(not identity, "native-identity-uncited")
    require(row.get("confidence") != "verified", "confidence-below-verified")
    device = row.get("ios_device") or {}
    if device.get("status") not in ("compared", "unread"):
        raise SystemExit("iOS Device status must be compared or unread: " + symbol)
    require(device["status"] == "unread", "device-unread")
    return codes


def check():
    ledger = json.loads((ROOT / "Configuration/Restoration.json").read_text())
    paths = {path for path in (ROOT / "Source").rglob("*") if path.suffix in SOURCE_SUFFIXES | {".swift"}}
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
    statuses = {"restored": 0, "implemented": 0}
    issue_counts = {}
    for row in ledger["definitions"]:
        status = row.get("status")
        if status not in statuses:
            raise SystemExit("Unknown restoration status for " + row.get("symbol", "?") + ": " + repr(status))
        statuses[status] += 1
        key = (row.get("path"), row.get("symbol"))
        if not all(key):
            raise SystemExit("Missing restoration field: path or symbol")
        if key in definitions:
            raise SystemExit("Duplicate restoration definition: " + repr(key))
        definitions.add(key)
        restored_paths.add(row["path"])
        if status == "restored":
            check_restored_row(row, uuids)
        else:
            for code in check_implemented_row(row, uuids):
                issue_counts[code] = issue_counts.get(code, 0) + 1
    translation_units = {str(path.relative_to(ROOT)) for path in paths}
    recorded_units = {path for path in restored_paths if Path(path).suffix in SOURCE_SUFFIXES | {".swift"}}
    if translation_units != recorded_units | exclusions:
        mismatch = translation_units ^ (recorded_units | exclusions)
        raise SystemExit("Production source/ledger mismatch: " + repr(mismatch))
    c_paths = {path for path in paths if path.suffix in SOURCE_SUFFIXES}
    c_records = {(row["path"], row["symbol"]) for row in ledger["definitions"]
                 if Path(row["path"]).suffix != ".swift"}
    check_definition_records(collect_definitions(c_paths), c_records)
    swift_rows = [row for row in ledger["definitions"] if Path(row["path"]).suffix == ".swift"]
    check_swift_records(swift_rows, collect_swift_definitions())
    functions = sum(row.get("kind", "function") == "function" and row["status"] == "restored"
                    for row in ledger["definitions"])
    print(f"Restoration ledger passed: {statuses['restored']} restored definitions ({functions} functions), "
          f"{statuses['implemented']} implemented with recorded issues. Bootstrap is not API coverage.")
    if issue_counts:
        print("Open issues of implemented definitions: " +
              ", ".join(f"{code} {count}" for code, count in sorted(issue_counts.items())))


if __name__ == "__main__":
    check()

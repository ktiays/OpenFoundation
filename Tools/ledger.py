#!/usr/bin/env python3
#
#  Created by ktiays on 2026/11/06.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Record every unrecorded definition in Configuration/Restoration.json as `implemented`, with its open issues.

The native identity comes from the citation comment directly above the definition, confidence and reading from
the definition's comment or its file header, and the export lists from frameworks built in both naming modes
(`--products-no`, `--products-yes`). Restored rows are never changed. A row this tool wrote is rewritten when its
definition changes, so the issues stay current; `--check` fails when a row would change (AGENTS.md, "Progress and
Verification").
"""

import argparse
import importlib.util
import json
from pathlib import Path
import re
import subprocess
from common import ROOT, SOURCE_SUFFIXES, output

UUIDS = {"OpenCoreFoundation": "D2C8AA56-482C-35EA-B4D3-54C2B5F58ADC",
         "OpenFoundation": "49FCDA5F-A93E-3259-857C-FC75AC2C7B03"}
ADDRESS = re.compile(r"\b0x(1c[0-9a-f]{7}|2[0-9a-f]{8})\b", re.IGNORECASE)
NATIVE = re.compile(r"Native (?:class |specialized |static |@objc |closure #\d+ in )*([^\s,;:]+(?: [^\s,;:\]]+\])?)")


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, ROOT / path)
    loaded = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(loaded)
    return loaded


def native_name(symbol):
    return re.sub(r"OCF(?=[A-Z])|ONS(?=[A-Z])", lambda match: match[0][1:], symbol, count=1)


def c_ast_definitions(job):
    """One translation unit's definitions with their source lines, by the rules of progress.definitions_from_ast."""
    path, command = job
    process = subprocess.run(command, capture_output=True, text=True)
    if process.returncode:
        raise SystemExit("Definition audit could not parse " + str(path) + ":\n" + process.stderr)
    tree = json.loads(process.stdout)
    source_root = (ROOT / "Source").resolve()
    found = {}

    def location_file(node, inherited):
        location = node.get("loc", node.get("range", {}).get("begin", {}))
        location = location.get("expansionLoc", location)
        name = location.get("file")
        return Path(name).resolve() if name else inherited

    def line(node, edge):
        location = node.get("range", {}).get(edge, {})
        location = location.get("expansionLoc", location)
        return location.get("line")

    def visit(node, inherited, last_line, in_function=False, objc_owner=None):
        path = location_file(node, inherited)
        begin = line(node, "begin") or node.get("loc", {}).get("line") or last_line
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
                found[(str(path), objc_owner)] = (begin, line(node, "end"), "class")
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
                found[(str(path), symbol)] = (begin, line(node, "end"), "function")
            elif kind == "VarDecl" and (not in_function or node.get("storageClass") == "static"):
                if node.get("storageClass") != "extern" or "init" in node:
                    found[(str(path), node["name"])] = (begin, line(node, "end"), "data")
        for child in children:
            last_line = visit(child, path, begin or last_line, in_function or function_kind, objc_owner) or last_line
        return begin

    previous = None
    last_line = None
    for node in tree.get("inner", []):
        previous = location_file(node, previous)
        last_line = visit(node, previous, last_line) or last_line
    return {(str(Path(name).relative_to(ROOT)), symbol): value for (name, symbol), value in found.items()}


def c_definitions(paths):
    """(path, symbol) -> (first line, last line, kind), from the same compiler syntax progress.py audits."""
    from concurrent.futures import ProcessPoolExecutor
    from common import PRODUCTS, defaults
    prefix = module("ledger_prefix", "Tools/prefix-mapping.py")
    mapping = prefix.discover(ROOT / "Source")
    directory = ROOT / ".build/Ledger/Frameworks"
    directory.mkdir(parents=True, exist_ok=True)
    for product in PRODUCTS:
        prefix.package(product, directory / (product + ".framework"), "NO", mapping)
    config = defaults()
    sdk = output(["xcrun", "--sdk", "iphonesimulator", "--show-sdk-path"])
    jobs = []
    for path in sorted(paths):
        cpp = path.suffix in {".cpp", ".mm"}
        command = ["xcrun", "clang++" if cpp else "clang", "-target",
                   "arm64-apple-ios" + config["IPHONEOS_DEPLOYMENT_TARGET"] + "-simulator",
                   "-isysroot", sdk, "-F", str(directory), "-I", str(ROOT / "Source"), "-fblocks",
                   "-DOF_SYSTEM_NAMES=0", "-std=" + config[
                       "CLANG_CXX_LANGUAGE_STANDARD" if cpp else "GCC_C_LANGUAGE_STANDARD"],
                   "-Xclang", "-ast-dump=json", "-fsyntax-only", str(path)]
        if path.suffix in {".m", ".mm"}:
            command[2:2] = ["-fno-objc-arc", "-fobjc-weak"]
        jobs.append((path, command))
    found = {}
    with ProcessPoolExecutor() as pool:
        for result in pool.map(c_ast_definitions, jobs):
            for key, value in result.items():
                if key not in found or (found[key][0] is None and value[0] is not None):
                    found[key] = value
    return found


def header(text):
    """The file-level Confidence and Reading statements (the first of each, joined over continuation lines)."""
    result = {}
    lines = text.splitlines()
    for index, line in enumerate(lines[:80]):
        for key in ("Confidence", "Reading"):
            match = re.search(key + r": (.*)", line)
            if match and key not in result:
                statement = match.group(1)
                follow = index + 1
                while follow < len(lines) and lines[follow].startswith("// ") and not re.match(
                        r"// [\w -]{1,40}:", lines[follow]) and not statement.endswith("."):
                    statement += " " + lines[follow][3:]
                    follow += 1
                result[key] = statement.strip()
    return result


def comment_above(lines, first):
    """The comment block directly above a definition (1-based first line), stopping at code or a blank line."""
    block = []
    index = first - 2
    while index >= 0:
        stripped = lines[index].strip()
        if stripped.startswith("//") or stripped.startswith("*") or stripped.startswith("/*"):
            block.insert(0, stripped)
        elif stripped.startswith("__attribute__") or stripped.startswith("#if") or stripped.startswith("@"):
            pass
        else:
            break
        index -= 1
    return " ".join(block)


def reading_path(statement):
    if not statement:
        return None
    for candidate in re.findall(r"[\w./+-]+\.md\b", statement):
        for root in ("", "Source/", "Documentation/"):
            path = ROOT / (root + candidate)
            if path.is_file() and not candidate.startswith("Local/"):
                return root + candidate
    return None


def exports(directory, product):
    executable = Path(directory) / (product + ".framework") / product
    return set(output(["xcrun", "nm", "-gUj", str(executable)]).splitlines())


UNBOUND = None


def unbound_symbols():
    global UNBOUND
    if UNBOUND is not None:
        return UNBOUND
    config = json.loads((ROOT / "Configuration/LinkDependencies.json").read_text())
    names = set()
    for row in config["products"].values():
        for group in row.get("unbound", {}).values():
            names.update(item.lstrip("_") for item in group.get("symbols", []))
            names.update(group.get("objc_classes", []))
    UNBOUND = names
    return names


def issues_for(row, citation, file_header, product, body):
    issues = []
    if not row.get("tests"):
        issues.append({"code": "untested", "detail": "No test exercises this definition yet."})
    if not row.get("reading"):
        detail = "Reading not versioned: " + (file_header.get("Reading") or "no reading cited") + "."
        issues.append({"code": "reading-unversioned", "detail": detail})
    if not all(row.get(key) for key in ("native_symbol", "address", "reference_uuid")):
        issues.append({"code": "native-identity-uncited",
                       "detail": "No native symbol and address are cited at the definition."})
    if row.get("confidence") != "verified":
        issues.append({"code": "confidence-below-verified",
                       "detail": "Confidence: " + (row.get("confidence_statement") or "not stated") + "."})
    issues.append({"code": "device-unread", "detail": "iOS Device 24A446 not compared."})
    unbound = sorted(name for name in unbound_symbols() if name in body and re.search(r"\b" + re.escape(name) + r"\b", body))
    if unbound:
        issues.append({"code": "open-edges", "detail": "Reaches unbound symbols (Configuration/"
                       "LinkDependencies.json): " + ", ".join(unbound) + "."})
    return issues


def confidence(statement):
    if not statement:
        return None, None
    lowered = statement.lower()
    if lowered.startswith("verified") and not re.search(r"except|medium|inferred|unread|low\b", lowered):
        return "verified", statement
    return "partial", statement


def build_row(path, symbol, first, last, kind, lines, file_header, product, exported_no, exported_yes):
    citation = comment_above(lines, first) if first else ""
    body = "\n".join(lines[(first or 1) - 1:(last or first or 1)])
    address = ADDRESS.search(citation)
    native = NATIVE.search(citation)
    local = re.search(r"confidence:? (verified|medium|low|high|inferred)", citation, re.IGNORECASE)
    file_level = re.search(r"confidence:? (verified|medium|low|high|inferred)", "\n".join(lines), re.IGNORECASE)
    level, statement = confidence(file_header.get("Confidence"))
    if local:
        level, statement = ("verified", None) if local.group(1).lower() == "verified" else (
            "partial", "at the definition: " + local.group(0))
    elif level is None and file_level:
        level, statement = ("verified", None) if file_level.group(1).lower() == "verified" else (
            "partial", "in the file: " + file_level.group(0))
    if kind == "class" and not address:
        native_class = native_name(symbol.split("(")[0])
        header_path = ROOT / path.replace(Path(path).suffix, ".h")
        texts = [" ".join(lines)] + ([header_path.read_text()] if header_path.is_file() else [])
        for text in texts:
            match = re.search(r"(?:class |Native )" + re.escape(native_class) + r"\b[^.;]{0,160}?\b0x(2[0-9a-f]{8})\b",
                              text.replace("\n// ", " "))
            if match:
                address = re.match(r"0x(.*)", "0x" + match.group(1))
                native = None
                citation = citation or ("Native class " + native_class)
                break
    row = {"path": path, "symbol": symbol, "status": "implemented", "kind": kind, "product": product,
           "first_line": first or 0, "last_line": last or first or 0,
           "confidence": level or "unstated", "tests": [], "ios_device": {"status": "unread"}}
    if statement and level != "verified":
        row["confidence_statement"] = statement
    if address:
        row["address"] = "0x" + address.group(1).lower()
        row["reference_uuid"] = UUIDS[product]
        row["native_symbol"] = native.group(1) if native else native_name(symbol.split("(")[0])
    reading = reading_path(re.search(r"Reading: (.*)", citation).group(1)) if "Reading:" in citation else None
    row["reading"] = reading or reading_path(file_header.get("Reading"))
    if not row["reading"]:
        del row["reading"]
    exported = []
    if kind == "class" and "(" not in symbol:
        candidates = {"_OBJC_CLASS_$_" + symbol, "_OBJC_METACLASS_$_" + symbol, "_OBJC_EHTYPE_$_" + symbol}
        exported = sorted(name for name in exported_no
                          if name in candidates or name.startswith("_OBJC_IVAR_$_" + symbol + "."))
    elif kind in ("function", "data") and "_" + symbol in exported_no:
        exported = ["_" + symbol]
    if exported:
        row["exported"] = True
        mapped = sorted(native_name(name[1:]) and "_" + native_name(name[1:]) for name in exported)
        missing = [name for name in mapped if name not in exported_yes]
        if missing:
            raise SystemExit("Export without a system-names counterpart: " + repr(missing))
        if exported != ["_" + symbol]:
            row["exported_symbols"] = {"NO": exported, "YES": mapped}
        if kind in ("function", "data"):
            row["native_symbol"] = native_name(symbol)
    else:
        row["exported"] = False
    row["issues"] = issues_for(row, citation, file_header, product, body)
    return row


def attach_labeled_exports(rows, texts, exported, restored):
    """Exports defined under an assembler label (an __asm name or a .globl/.set alias): each is attached to the
    row whose definition holds the label, else to the class row of its file."""
    covered = set()
    for row in restored:
        if row.get("exported"):
            covered.update(row.get("exported_symbols", {}).get("NO", []) or ["_" + row["symbol"]])
    for row in rows:
        covered.update(row.get("exported_symbols", {}).get("NO", []))
        if row.get("exported") and not row.get("exported_symbols"):
            covered.add("_" + row["symbol"])
    for product in UUIDS:
        for name in sorted(exported["NO"][product] - covered):
            if name.startswith(("_OBJC_", "_$s")):
                continue
            label = name[1:]
            pattern = re.compile(r'(?:__asm\(|\.globl |\.set )[^)\n]*OF_NAME_STRING\(' + re.escape(label) + r'\)')
            for path, (lines, _) in texts.items():
                if not path.startswith("Source/" + ("CoreFoundation" if product == "OpenCoreFoundation" else "Foundation")):
                    continue
                text = "\n".join(lines)
                match = pattern.search(text)
                if not match:
                    continue
                line = text.count("\n", 0, match.start()) + 1
                candidates = [row for row in rows if row["path"] == path]
                target = next((row for row in candidates if row.get("first_line", 0) <= line <= row.get("last_line", -1)),
                              None) or next((row for row in candidates if row["kind"] == "class"), None)
                if target is None:
                    raise SystemExit("No definition holds the exported label " + name)
                symbols = target.get("exported_symbols") or {"NO": [], "YES": []}
                if target.get("exported") and not target.get("exported_symbols"):
                    symbols = {"NO": ["_" + target["symbol"]], "YES": ["_" + target["native_symbol"]]}
                yes = "_" + native_name(label)
                if yes not in exported["YES"][product]:
                    raise SystemExit("Labeled export without a system-names counterpart: " + name)
                symbols["NO"] = sorted(symbols["NO"] + [name])
                symbols["YES"] = sorted(symbols["YES"] + [yes])
                target["exported"] = True
                target["exported_symbols"] = symbols
                break
            else:
                raise SystemExit("Exported symbol without a recorded definition: " + name)


def swift_rows(recorded, exported_no, exported_yes):
    swift = module("ledger_swift", "Tools/swift-build.py")
    prefix = swift.prefix_module()
    mapping = prefix.discover(ROOT / "Source")
    directory = ROOT / ".build/Ledger"
    directory.mkdir(parents=True, exist_ok=True)
    for product in ("OpenCoreFoundation", "OpenFoundation"):
        prefix.package(product, directory / (product + ".framework"), "NO", mapping)
    generated = directory / "Generated"
    generated.mkdir(exist_ok=True)
    (generated / "OFPrefixMapping.h").write_text(prefix.mapping_header(mapping))
    rows = []
    demangled_yes = {}
    yes_swift = sorted(name for name in exported_yes if name.startswith("_$s"))
    if yes_swift:
        text = output(["xcrun", "swift-demangle", "--compact", *yes_swift]).splitlines()
        demangled_yes = dict(zip(text, yes_swift))
    no_swift = sorted(name for name in exported_no if name.startswith("_$s"))
    assigned = set()
    definitions = swift.source_definitions("OpenFoundation", directory)
    usrs = sorted({usr for _, usr in definitions if usr.startswith("s:")})
    readable = dict(zip(usrs, output(["xcrun", "swift-demangle", "--compact",
                                      *["$s" + usr[2:] for usr in usrs]]).splitlines()))
    for (path, usr), node in sorted(definitions.items()):
        if (path, usr) in recorded:
            continue
        source = ROOT / path
        text = source.read_text()
        lines = text.splitlines()
        span = node.get("range") or {}
        begin = text.count("\n", 0, span["start"]) + 1 if "start" in span else None
        end = text.count("\n", 0, span["end"]) + 1 if "end" in span else begin
        mangled = "_$s" + usr[2:] if usr.startswith("s:") else None
        row = build_row(path, readable.get(usr, usr), begin, end, node["kind"], lines, header(text),
                        "OpenFoundation", set(), set())
        row["source_usr"] = usr
        if span:
            row["source_range"] = {"start": span["start"], "end": span["end"]}
        if mangled in exported_no:
            names = [mangled] + [name for name in no_swift if name not in assigned and name != mangled
                                 and name.endswith("Mc") and path.endswith("ObjectiveCBridgeable.swift")]
            assigned.update(names)
            demangled = output(["xcrun", "swift-demangle", "--compact", *names]).splitlines()
            yes = []
            for item in demangled:
                key = item.replace("OpenFoundation", "Foundation").replace("ONS", "NS")
                if key not in demangled_yes:
                    raise SystemExit("Swift export without a system-names counterpart: " + item)
                yes.append(demangled_yes[key])
            row["exported"] = True
            row["exported_symbols"] = {"NO": names, "YES": yes}
            row["issues"] = [issue for issue in row["issues"]]
        rows.append(row)
    leftover = [name for name in no_swift if name not in assigned]
    return rows, leftover


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--products-no", required=True)
    parser.add_argument("--products-yes", required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    ledger_path = ROOT / "Configuration/Restoration.json"
    ledger = json.loads(ledger_path.read_text())
    restored = [row for row in ledger["definitions"] if row["status"] == "restored"]
    recorded = {(row["path"], row["symbol"]) for row in restored}
    recorded_usr = {(row["path"], row.get("source_usr")) for row in restored if row["path"].endswith(".swift")}
    paths = {path for path in (ROOT / "Source").rglob("*") if path.suffix in SOURCE_SUFFIXES}
    exclusions = {row["path"] for row in ledger["exclusions"]}
    paths = {path for path in paths if str(path.relative_to(ROOT)) not in exclusions}
    exported = {mode: {product: exports(directory, product) for product in UUIDS}
                for mode, directory in (("NO", args.products_no), ("YES", args.products_yes))}
    rows = []
    texts = {}
    for (path, symbol), (first, last, kind) in sorted(c_definitions(paths).items()):
        if (path, symbol) in recorded:
            continue
        if path not in texts:
            text = (ROOT / path).read_text()
            texts[path] = (text.splitlines(), header(text))
        product = "OpenCoreFoundation" if path.startswith("Source/CoreFoundation/") else "OpenFoundation"
        rows.append(build_row(path, symbol, first, last, kind, texts[path][0], texts[path][1],
                              product, exported["NO"][product], exported["YES"][product]))
    print(f"C and Objective-C: {len(rows)} new rows.", flush=True)
    attach_labeled_exports(rows, texts, exported, restored)
    swift, leftover = swift_rows(recorded_usr, exported["NO"]["OpenFoundation"], exported["YES"]["OpenFoundation"])
    rows += swift
    if leftover:
        raise SystemExit("Swift exports without a recorded definition: " + repr(leftover))
    for row in rows:
        row.pop("first_line", None)
        row.pop("last_line", None)
    definitions = restored + rows
    text = json.dumps({**ledger, "definitions": definitions}, indent=2, ensure_ascii=False) + "\n"
    if args.check:
        if ledger_path.read_text() != text:
            raise SystemExit("Configuration/Restoration.json is not current; run Tools/ledger.py")
        print("Ledger current")
        return
    ledger_path.write_text(text)
    print(f"Recorded {len(rows)} implemented definitions beside {len(restored)} restored.")


if __name__ == "__main__":
    main()

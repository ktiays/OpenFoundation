#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Discover prefixed declarations, validate uses, and package concrete header spellings."""

import argparse
import json
from pathlib import Path
import re
from common import ROOT, FORMAT_SUFFIXES, write_changed

TOKEN = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\b[A-Za-z_]\w*\b|.', re.DOTALL)
PREFIXED = re.compile(r"^(?:_+)?(?:k)?(?:OCF|ONS)[A-Z]\w*$")


def native_name(name):
    return re.sub(r"OCF(?=[A-Z])|ONS(?=[A-Z])", lambda match: match[0][1:], name, count=1)


def code_only(text):
    return re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', " ", text)


def discover(root):
    paths = sorted(path for path in root.rglob("*") if path.suffix in FORMAT_SUFFIXES)
    declarations = set()
    texts = {path: path.read_text() for path in paths}
    for path, text in texts.items():
        if path.suffix not in {".h", ".hpp"}:
            continue
        code = code_only(text)
        for pattern in (
            r"\b(?:struct|union|enum|class)\s+((?:_+)?(?:OCF|ONS)[A-Z]\w*)",
            r"@(?:interface|protocol|class)\s+((?:_+)?(?:OCF|ONS)[A-Z]\w*)",
            r"\btypedef\b[^;]*?\b((?:_+)?(?:OCF|ONS)[A-Z]\w*)\s*;",
            r"\b((?:_+)?(?:OCF|ONS)[A-Z]\w*)\s*\([^;{}]*\)\s*;",
            r"\b(?:extern|OF_EXPORT|OF_EXTERN)\b[^;{}]*?\b((?:_+)?k?(?:OCF|ONS)[A-Z]\w*)\s*;",
            r"\b((?:_+)?k?(?:OCF|ONS)[A-Z]\w*)\s*=",
        ):
            declarations.update(re.findall(pattern, code))
    errors = []
    for path, text in texts.items():
        for line_number, line in enumerate(text.splitlines(), 1):
            if re.match(r"\s*#\s*(?:include|import|define)\b", line):
                continue
            for name in re.findall(r"\b[A-Za-z_]\w*\b", code_only(line)):
                if PREFIXED.fullmatch(name) and name not in declarations:
                    errors.append(f"{path}:{line_number}: undeclared prefixed identifier {name}")
        for literal in re.findall(r'"(?:\\.|[^"\\])*"', re.sub(r"//[^\n]*|/\*[\s\S]*?\*/", "", text)):
            if literal[1:-1] in declarations or literal[1:-1] in {native_name(name) for name in declarations}:
                errors.append(f"{path}: use OF_NAME_STRING for a mapped runtime name: {literal}")
    if errors:
        raise SystemExit("\n".join(errors))
    return {name: native_name(name) for name in sorted(declarations)}


def translate(text, mapping):
    includes = re.compile(r"^\s*#\s*(?:include|import)\b.*$", re.MULTILINE)
    result = []
    position = 0
    for include in includes.finditer(text):
        result.append(TOKEN.sub(lambda match: mapping.get(match[0], match[0]), text[position:include.start()]))
        result.append(include[0])
        position = include.end()
    result.append(TOKEN.sub(lambda match: mapping.get(match[0], match[0]), text[position:]))
    return "".join(result)


def mapping_header(mapping):
    return ("// Generated from source declarations. Do not edit.\n#pragma once\n"
            "#if OF_SYSTEM_NAMES\n" + "".join(f"#define {name} {native}\n" for name, native in mapping.items())
            + "#endif\n")


def package(product, destination, mode, mapping):
    source = ROOT / "Source" / ("CoreFoundation" if product == "OpenCoreFoundation" else "Foundation")
    headers = destination / "Headers"
    headers.mkdir(parents=True, exist_ok=True)
    registry = json.loads((ROOT / "Configuration/PublicHeaders.json").read_text())
    for installed in headers.rglob("*.h"):
        if str(installed.relative_to(headers)) not in registry[product]:
            installed.unlink()
    for name in registry[product]:
        path = source / name
        if not path.is_file() or source.resolve() not in path.resolve().parents:
            raise SystemExit("Invalid public header registry entry: " + name)
        text = path.read_text()
        if path.name == "OCFBuild.h":
            text, substitutions = re.subn(r"#ifndef OF_SYSTEM_NAMES\n\s*#define OF_SYSTEM_NAMES 0\n#endif", (
                f"#if defined(OF_SYSTEM_NAMES) && OF_SYSTEM_NAMES != {int(mode == 'YES')}\n"
                '#error "Consumer naming mode does not match the packaged framework."\n'
                "#endif\n#ifndef OF_SYSTEM_NAMES\n"
                f"#define OF_SYSTEM_NAMES {int(mode == 'YES')}\n#endif"
            ), text)
            if substitutions != 1:
                raise SystemExit("Cannot stamp the packaged naming mode in OCFBuild.h.")
        if path.name == product + ".h":
            guard = (f"#if defined(OF_SYSTEM_NAMES) && OF_SYSTEM_NAMES != {int(mode == 'YES')}\n"
                     '#error "Consumer naming mode does not match the packaged framework."\n'
                     "#endif\n#ifndef OF_SYSTEM_NAMES\n"
                     f"#define OF_SYSTEM_NAMES {int(mode == 'YES')}\n#endif")
            text = text.replace("#pragma once", "#pragma once\n\n" + guard, 1)
        write_changed(headers / path.relative_to(source), translate(text, mapping) if mode == "YES" else text)
    write_changed(destination / "Modules/module.modulemap", (
        f'framework module {product} {{\n    umbrella header "{product}.h"\n    export *\n}}\n'
    ))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("check", "generate", "list", "headers"))
    parser.add_argument("destination", nargs="?")
    parser.add_argument("--source", type=Path, default=ROOT / "Source")
    parser.add_argument("--product", choices=("OpenCoreFoundation", "OpenFoundation"))
    parser.add_argument("--system-names", choices=("NO", "YES"), default="NO")
    args = parser.parse_args()
    mapping = discover(args.source)
    if args.action == "generate":
        if not args.destination:
            parser.error("generate needs a destination outside Source")
        destination = Path(args.destination).resolve()
        if args.source.resolve() in destination.parents:
            parser.error("generated mappings must be outside the scanned source tree")
        write_changed(destination, mapping_header(mapping))
    elif args.action == "list":
        print(json.dumps(mapping, indent=2))
    elif args.action == "headers":
        if not args.product or not args.destination:
            parser.error("headers needs --product and a framework destination")
        package(args.product, Path(args.destination), args.system_names, mapping)
    else:
        print(f"Prefix check passed: {len(mapping)} declared names.")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Inventory consumer source references; these are integration requirements, not binary evidence."""

import argparse
from collections import Counter
from datetime import datetime, timedelta, timezone
import hashlib
import json
from pathlib import Path
import re
from common import ROOT, FORMAT_SUFFIXES, output, write_changed


def inventory(repository, directories):
    revision = output(["git", "-C", repository, "rev-parse", "HEAD"])
    initial_status = output(["git", "-C", repository, "status", "--porcelain"])
    digest = hashlib.sha256()
    inputs = []
    tokens = Counter()
    imports = set()
    files = 0
    for directory in directories:
        for path in sorted((repository / directory).rglob("*")):
            if path.suffix not in FORMAT_SUFFIXES | {".swift"}:
                continue
            text = path.read_text()
            inputs.append((path, text))
            digest.update(str(path.relative_to(repository)).encode() + b"\0" + text.encode() + b"\0")
            imports.update(re.findall(r"#\s*(?:include|import)\s*<(Foundation/[^>]+|CoreFoundation/[^>]+)>", text))
            code = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', " ", text)
            names = re.findall(r"\b(?:_+)?(?:k)?(?:CF|NS)[A-Z]\w*\b", code)
            if names:
                files += 1
                tokens.update(names)
    if (revision != output(["git", "-C", repository, "rev-parse", "HEAD"])
            or initial_status != output(["git", "-C", repository, "status", "--porcelain"])
            or any(path.read_text() != text for path, text in inputs)):
        raise SystemExit("Consumer changed during inventory; rerun on a stable checkout: " + str(repository))
    return {"commit": revision, "source_sha256": digest.hexdigest(),
            "working_tree_dirty": bool(initial_status),
            "source_roots": directories, "files_with_references": files,
            "system_headers": sorted(imports), "token_occurrences": dict(sorted(tokens.items()))}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--coregraphics", type=Path, required=True)
    parser.add_argument("--coreanimation", type=Path, required=True)
    args = parser.parse_args()
    result = {"schema_version": 1, "recorded_on": datetime.now(timezone(timedelta(hours=8))).date().isoformat(),
              "scope": "Lexical source references, including types/macros/private APIs; no symbol ownership or behavior claim.",
              "consumers": {"OpenCoreGraphics": inventory(args.coregraphics, ["src", "include"]),
                            "OpenCoreAnimation": inventory(args.coreanimation, ["Source"])}}
    write_changed(ROOT / "Documentation/Reference/Consumers.json", json.dumps(result, indent=2) + "\n")
    for name, row in result["consumers"].items():
        print(name + ": " + str(len(row["token_occurrences"])) + " distinct CF/NS source tokens in "
              + str(row["files_with_references"]) + " files.")


if __name__ == "__main__":
    main()

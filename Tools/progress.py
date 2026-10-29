#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Require a reading and validation record for every production translation unit."""

import json
from common import ROOT, SOURCE_SUFFIXES


def check():
    ledger = json.loads((ROOT / "Configuration/Restoration.json").read_text())
    paths = {str(path.relative_to(ROOT)) for path in (ROOT / "Source").rglob("*")
             if path.suffix in SOURCE_SUFFIXES}
    exclusions = {row["path"] for row in ledger["exclusions"]}
    expected_bootstrap = {"Source/CoreFoundation/Bootstrap.c", "Source/Foundation/Bootstrap.c"}
    if exclusions != expected_bootstrap:
        raise SystemExit("Only the two empty bootstrap translation units may bypass the restoration ledger.")
    for name in exclusions:
        text = (ROOT / name).read_text()
        import re
        code = re.sub(r"//[^\n]*|/\*[\s\S]*?\*/|^\s*#.*$|_Static_assert\([^;]*;", "", text,
                      flags=re.MULTILINE)
        if code.strip():
            raise SystemExit(f"{name}: the bootstrap must remain empty.")
    restored_paths = set()
    symbols = set()
    for row in ledger["definitions"]:
        for key in ("path", "symbol", "native_symbol", "address", "reference_uuid", "confidence", "reading", "tests", "ios_device"):
            if not row.get(key):
                raise SystemExit("Missing restoration field: " + key)
        if row["confidence"] != "verified":
            raise SystemExit("Unread or inferred code cannot be marked restored: " + row["symbol"])
        if row["symbol"] in symbols:
            raise SystemExit("Duplicate restoration definition: " + row["symbol"])
        symbols.add(row["symbol"])
        restored_paths.add(row["path"])
        for name in [row["reading"], *row["tests"]]:
            if not (ROOT / name).is_file():
                raise SystemExit("Missing reading or test: " + name)
    if paths != restored_paths | exclusions:
        raise SystemExit("Production source/ledger mismatch: " + repr(paths ^ (restored_paths | exclusions)))
    print(f"Restoration ledger passed: {len(symbols)} restored definitions. Bootstrap is not API coverage.")


if __name__ == "__main__":
    check()

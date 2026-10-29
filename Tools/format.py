#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Apply the same C-family and Swift formatters as the sibling projects."""

import argparse
import shutil
from common import ROOT, FORMAT_SUFFIXES, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    roots = [ROOT / name for name in ("Source", "Tests", "StandaloneTests")]
    paths = sorted(path for root in roots for path in root.rglob("*") if path.suffix in FORMAT_SUFFIXES)
    formatter = shutil.which("clang-format")
    if not formatter:
        raise SystemExit("clang-format is required.")
    if paths:
        run([formatter, *( ["--dry-run", "--Werror"] if args.check else ["-i"] ), *paths])
    swift = sorted(path for root in roots for path in root.rglob("*.swift"))
    if swift:
        run(["xcrun", "swift-format", "lint" if args.check else "format",
             *( ["--strict"] if args.check else ["--in-place"] ), "--configuration", ROOT / ".swift-format", *swift])
    print("Formatting check passed." if args.check else "Sources formatted.")


if __name__ == "__main__":
    main()

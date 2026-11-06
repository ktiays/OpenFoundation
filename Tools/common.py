#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Shared build policy and subprocess handling; no restored framework behavior."""

import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE_SUFFIXES = {".c", ".cpp", ".m", ".mm"}
FORMAT_SUFFIXES = SOURCE_SUFFIXES | {".h", ".hpp", ".inc"}
PRODUCTS = ("OpenCoreFoundation", "OpenFoundation")


def run(arguments, **kwargs):
    return subprocess.run([str(item) for item in arguments], check=True, text=True, **kwargs)


def output(arguments):
    return subprocess.check_output([str(item) for item in arguments], text=True).strip()


def write_changed(path, text):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_text() != text:
        with tempfile.NamedTemporaryFile(mode="w", dir=path.parent, delete=False) as temporary:
            temporary.write(text)
            temporary_path = Path(temporary.name)
        temporary_path.replace(path)
    path.chmod(0o644)


def defaults():
    result = {}
    for line in (ROOT / "Configuration/Build.xcconfig").read_text().splitlines():
        match = re.fullmatch(r"([A-Z_]+)\s*=\s*([^/]*?)\s*", line)
        if match:
            result[match[1]] = match[2]
    return result


def check_platform(target, match, configuration, mode):
    policy = json.loads((ROOT / "Configuration/Platforms.json").read_text())
    pairs = policy["pairs"]
    if not any(row["target"] == target and row["match"] == match for row in pairs):
        raise SystemExit("Unsupported TARGET/MATCH pair: " + target + "/" + match)
    if configuration not in ("Debug", "Release"):
        raise SystemExit("CONFIGURATION must be Debug or Release.")
    if mode not in ("NO", "YES"):
        raise SystemExit("OF_SYSTEM_NAMES must be NO or YES in build commands.")
    return next(row for row in pairs if row["target"] == target and row["match"] == match)


def build_directory(target, match, configuration, mode):
    return ROOT / ".build/Make" / (target + "-" + match + "-" + configuration + "-" + mode)


def xcode_environment():
    return (
        os.environ.get("PLATFORM_NAME", "iphonesimulator"),
        os.environ.get("OF_MATCH", "iphonesimulator"),
        os.environ.get("CONFIGURATION", "Debug"),
        os.environ.get("OF_SYSTEM_NAMES", "NO"),
    )

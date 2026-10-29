#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Check source policy and regression cases for naming/packaging infrastructure."""

import argparse
import importlib.util
import json
import re
import tempfile
from pathlib import Path
from common import ROOT, run
import progress


def load_prefix_module():
    spec = importlib.util.spec_from_file_location("prefix_mapping", ROOT / "Tools/prefix-mapping.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def self_test(prefix):
    scratch = ROOT / "Local/Scratch"
    scratch.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=scratch) as directory:
        root = Path(directory)
        header = root / "Example.h"
        header.write_text("typedef long OCFIndex;\nextern OCFIndex OCFExample(OCFIndex value);\n"
                          "@interface ONSString\n@end\nextern const int kOCFExample;\n"
                          "typedef struct { OCFIndex location; OCFIndex length; } OCFRange;\n"
                          "typedef struct _ONSRange { unsigned long location; unsigned long length; } ONSRange;\n"
                          "typedef union { struct { int value; } nested; int raw; } OCFNested;\n"
                          "typedef enum { OCFExampleZero = 0 } OCFExampleEnum;\n"
                          "typedef unsigned long (*OCFHashCallBack)(const void *value);\n"
                          "typedef void (*ONSCallback)(void (*callback)(int));\n"
                          "extern const OCFIndex kOCFArray[4];\n")
        mapping = prefix.discover(root)
        expected = {"OCFIndex": "CFIndex", "OCFExample": "CFExample", "ONSString": "NSString", "kOCFExample": "kCFExample",
                    "OCFRange": "CFRange", "_ONSRange": "_NSRange", "ONSRange": "NSRange",
                    "OCFNested": "CFNested", "OCFExampleZero": "CFExampleZero",
                    "OCFExampleEnum": "CFExampleEnum", "OCFHashCallBack": "CFHashCallBack",
                    "ONSCallback": "NSCallback", "kOCFArray": "kCFArray"}
        if mapping != expected:
            raise SystemExit("Declaration-derived prefix mapping regression.")
        swift = root / "Example.swift"
        swift.write_text('@_cdecl("OCFExample")\nfunc OCFBridge(_ value: OCFIndex) -> OCFIndex { value }\n')
        swift_mapping = prefix.discover(root)
        if swift_mapping != {**expected, "OCFBridge": "CFBridge"}:
            raise SystemExit("Swift declaration discovery regression.")
        spec = importlib.util.spec_from_file_location("swift_build_test", ROOT / "Tools/swift-build.py")
        swift_build = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(swift_build)
        translated_swift = swift_build.translate_source(swift.read_text(), swift_mapping)
        if '@_cdecl("CFExample")' not in translated_swift or 'func CFBridge(_ value: CFIndex)' not in translated_swift:
            raise SystemExit("Swift linkage-literal or concrete identifier translation regression.")
        external = '@_silgen_name("$s5Swift17externalFunction")\n'
        if swift_build.translate_source(external, swift_mapping) != external:
            raise SystemExit("Swift naming changed an external runtime linkage name.")
        swift.unlink()
        translated = prefix.translate('OCFExample(); /* OCFExample */ "OCFExample";', mapping)
        if translated != 'CFExample(); /* OCFExample */ "OCFExample";':
            raise SystemExit("Token mapping changed a comment or a string literal.")
        include = '#include <OpenFoundation/ONSString.h>\nONSString *value;\n'
        if prefix.translate(include, mapping) != '#include <OpenFoundation/ONSString.h>\nNSString *value;\n':
            raise SystemExit("Token mapping changed an include path.")
        for invalid in ("OCFMissing();", 'const char *name = "ONSString";', 'const char *name = "NSString";'):
            implementation = root / "Example.c"
            implementation.write_text(invalid)
            try:
                prefix.discover(root)
            except SystemExit:
                pass
            else:
                raise SystemExit("Prefix checker accepted an invalid declaration or runtime name.")
            implementation.unlink()
        source = root / "Source/CoreFoundation"
        source.mkdir(parents=True)
        (root / "Configuration").mkdir()
        (root / "Configuration/PublicHeaders.json").write_text(json.dumps({"OpenCoreFoundation": ["OCFBuild.h", "OpenCoreFoundation.h"]}))
        (source / "OCFBuild.h").write_text("#pragma once\n#ifndef OF_SYSTEM_NAMES\n#define OF_SYSTEM_NAMES 0\n#endif\n")
        (source / "OpenCoreFoundation.h").write_text("#pragma once\nextern void OCFExample(void);\n")
        (source / "Private.h").write_text("#pragma once\n")
        bundle = root / "OpenCoreFoundation.framework"
        (bundle / "Headers").mkdir(parents=True)
        (bundle / "Headers/Stale.h").write_text("// Obsolete installed header.\n")
        original_root = prefix.ROOT
        try:
            prefix.ROOT = root
            prefix.package("OpenCoreFoundation", bundle, "YES", mapping)
        finally:
            prefix.ROOT = original_root
        if (bundle / "Headers/Private.h").exists() or (bundle / "Headers/Stale.h").exists():
            raise SystemExit("Packaging exposed a private or obsolete header.")
        packaged = (bundle / "Headers/OpenCoreFoundation.h").read_text()
        if "CFExample(void)" not in packaged or "OF_SYSTEM_NAMES != 1" not in packaged:
            raise SystemExit("Packaged native declarations or naming guard are incorrect.")
    print("Naming infrastructure regressions passed.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    progress.check()
    prefix = load_prefix_module()
    prefix.discover(ROOT / "Source")
    for path in (ROOT / "Source").rglob("*"):
        if not path.is_file():
            continue
        text = path.read_text()
        if path.suffix in {".S", ".s", ".metal"}:
            raise SystemExit("Integrate this source language in both build systems and the ledger first: " + str(path))
        if re.search(r"#\s*(?:include|import)\s*<(?:Foundation|CoreFoundation)/", text):
            raise SystemExit("Production code imports a system replacement framework: " + str(path))
    if args.self_test:
        self_test(prefix)
        progress.self_test()
    print("Source policy checks passed.")


if __name__ == "__main__":
    main()

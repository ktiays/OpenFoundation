#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Summarize identity-guarded IDA symbol inventories without claiming API coverage."""

import argparse
import json
from common import ROOT, write_changed


def summarize():
    references = json.loads((ROOT / "Configuration/ReferenceImages.json").read_text())
    images = []
    for platform in ("iphone-simulator", "iphone-device"):
        reference = references[platform]
        for framework in ("CoreFoundation", "Foundation"):
            source = "Local/Evidence/cf-runtime-reference/" + platform + "-" + framework + "-inventory.json"
            inventory = json.loads((ROOT / source).read_text())
            identity = inventory["identity"]
            expected = reference["images"][framework]
            for key in ("uuid", "sha256"):
                if identity.get(key) != expected[key]:
                    raise SystemExit("IDA inventory identity does not match the pinned image: " + source)
            for key in ("os_version", "os_build", "architecture"):
                if identity.get(key) != reference[key]:
                    raise SystemExit("IDA inventory platform does not match: " + source + ": " + key)
            exports = inventory["exports"]
            imports = inventory["imports"]
            selected = {}
            for name in ("NSObject", "NSDate", "NSDateComponents", "NSValue", "NSString", "__NSCFString"):
                symbol = "_OBJC_CLASS_$_" + name
                defined = [row for row in exports if row["name"] == symbol]
                imported = [row for row in imports if row["name"] == symbol]
                selected[name] = {
                    "exports": [{"symbol": row["name"], "address": row["address"]} for row in defined],
                    "imports": [{"symbol": row["name"], "module": row["module"]} for row in imported]}
            images.append({"platform": platform, "framework": framework,
                           "os_version": reference["os_version"], "os_build": reference["os_build"],
                           "architecture": reference["architecture"], "uuid": expected["uuid"],
                           "sha256": expected["sha256"], "evidence": source,
                           "exported_entries": len(exports), "imported_entries": len(imports),
                           "objc_symbol_entries": len(inventory["objc_symbols"]),
                           "selected_class_symbol_boundaries": selected})
    return {"schema_version": 1,
            "scope": ("IDA export/import and Objective-C symbol inventory. Counts are not API or behavioral "
                      "coverage; method ownership, class clusters, reexport traversal and runtime layouts "
                      "remain unread."),
            "images": images}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    destination = ROOT / "Documentation/Reference/NativeInventory.json"
    text = json.dumps(summarize(), indent=2) + "\n"
    if args.check:
        if not destination.is_file() or destination.read_text() != text:
            raise SystemExit("Native symbol inventory is stale; regenerate it from the pinned IDA evidence.")
    else:
        write_changed(destination, text)
    print("IDA symbol inventory verified for four pinned images; counts are not API coverage.")


if __name__ == "__main__":
    main()

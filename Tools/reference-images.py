#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Pin image identity. Mach-O metadata and runtime identity do not establish API behavior."""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import uuid
from common import ROOT, run, write_changed

MANIFEST = ROOT / "Configuration/ReferenceImages.json"


def macho_metadata(path):
    data = path.read_bytes()
    magic, cpu, subtype, _, count, size, _, _ = struct.unpack_from("<8I", data)
    if magic != 0xFEEDFACF or cpu != 0x100000C:
        raise SystemExit("Expected a thin arm64 Mach-O image: " + str(path))
    result = {"sha256": hashlib.sha256(data).hexdigest(), "bytes": len(data),
              "architecture": "arm64e" if subtype & 0xFFFFFF == 2 else "arm64"}
    offset = 32
    for _ in range(count):
        command, length = struct.unpack_from("<II", data, offset)
        if length < 8 or offset + length > 32 + size:
            raise SystemExit("Malformed load commands: " + str(path))
        if command == 0x1B:
            result["uuid"] = str(uuid.UUID(bytes=data[offset + 8:offset + 24])).upper()
        elif command == 0x32:
            result["macho_platform"] = struct.unpack_from("<I", data, offset + 8)[0]
        offset += length
    if "uuid" not in result:
        raise SystemExit("Missing LC_UUID: " + str(path))
    return result


def generate_header(destination):
    images = json.loads(MANIFEST.read_text())["iphone-simulator"]["images"]
    text = "// Generated reference identity pins.\n#pragma once\n"
    for image, macro in (("CoreFoundation", "CF"), ("Foundation", "NS")):
        text += '#define OF_REFERENCE_' + macro + '_UUID "' + images.get(image, {}).get("uuid", "") + '"\n'
    write_changed(destination, text)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    subs = parser.add_subparsers(dest="action", required=True)
    header = subs.add_parser("header")
    header.add_argument("destination", type=Path)
    capture = subs.add_parser("capture-simulator")
    capture.add_argument("--products", type=Path, required=True)
    record = subs.add_parser("record")
    record.add_argument("--platform", choices=("iphone-simulator", "iphone-device"), required=True)
    record.add_argument("--framework-root", type=Path, required=True)
    args = parser.parse_args()
    if args.action == "header":
        generate_header(args.destination)
        return
    manifest = json.loads(MANIFEST.read_text())
    if args.action == "capture-simulator":
        from simulator import session, spawn
        with session() as udid:
            text = spawn(udid, args.products.resolve() / "DifferentialTests", "--capture-reference", capture=True)
        for line in text.splitlines():
            if line.startswith("REFERENCE "):
                _, name, identity = line.split()
                previous = manifest["iphone-simulator"]["images"].get(name, {})
                if previous.get("uuid") and previous["uuid"] != identity:
                    raise SystemExit("Runtime image differs from the existing identity pin: " + name)
                manifest["iphone-simulator"]["images"][name] = {**previous,
                    "uuid": identity, "provenance": "LC_UUID of the loaded image in iOS Simulator 24A434",
                    "binary_reading": previous.get("binary_reading", "unread")
                }
        if set(manifest["iphone-simulator"]["images"]) != {"Foundation", "CoreFoundation"}:
            raise SystemExit("Incomplete simulator identity capture.")
    else:
        row = manifest[args.platform]
        for name in ("CoreFoundation", "Foundation"):
            source = args.framework_root / (name + ".framework") / name
            metadata = macho_metadata(source)
            expected_platform = 7 if args.platform == "iphone-simulator" else 2
            if metadata["macho_platform"] != expected_platform or metadata["architecture"] != row["architecture"]:
                raise SystemExit("Image platform or architecture does not match: " + str(source))
            previous = row["images"].get(name, {})
            if previous.get("uuid") and previous["uuid"] != metadata["uuid"]:
                raise SystemExit("Image UUID differs from the existing pin: " + str(source))
            destination = ROOT / "Local/Databases" / args.platform / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            if not destination.exists():
                run(["cp", "-c", source, destination])
            if hashlib.sha256(destination.read_bytes()).hexdigest() != metadata["sha256"]:
                raise SystemExit("Canonical reference copy differs: " + str(destination))
            metadata.update({"local_path": str(destination.relative_to(ROOT)),
                             "provenance": str(source), "binary_reading": previous.get("binary_reading", "unread")})
            row["images"][name] = {**previous, **metadata}
    write_changed(MANIFEST, json.dumps(manifest, indent=2) + "\n")
    print("Reference image identities recorded; framework behavior remains unread.")


if __name__ == "__main__":
    main()

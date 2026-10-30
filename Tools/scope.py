#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Derive the consumer-required restoration scope (AGENTS.md, "Scope").

`roots` measures what linked consumer binaries import from CoreFoundation and
Foundation, plus the selectors they reference. `Tools/scope-ida.py` closes
those roots over a pinned image's call graph inside IDA. `assemble` merges the
evidence into Configuration/Scope.json; `--check` verifies it is current.
"""

import argparse
import copy
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
from common import ROOT, output, write_changed

EVIDENCE = ROOT / "Local/Evidence/scope"
SCOPE = ROOT / "Configuration/Scope.json"
NATIVE = ("CoreFoundation", "Foundation")
IMPORT_LINE = re.compile(r"^\s+0x[0-9A-F]+\s+(\S+)\s+\(from ([^)]+)\)")
LOAD_SEGMENT64 = 0x19
LOAD_UUID = 0x1B


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def macho_sections(data):
    magic, _, _, _, count, _, _, _ = struct.unpack_from("<IiiIIIII", data, 0)
    if magic != 0xFEEDFACF:
        raise SystemExit("Not a thin 64-bit Mach-O image")
    offset = 32
    sections = {}
    uuid = None
    for _ in range(count):
        command, size = struct.unpack_from("<II", data, offset)
        if command == LOAD_SEGMENT64:
            segment = data[offset + 8:offset + 24].rstrip(b"\0").decode()
            _, _, _, _, _, _, section_count, _ = struct.unpack_from("<QQQQiiII", data, offset + 24)
            cursor = offset + 72
            for _ in range(section_count):
                name = data[cursor:cursor + 16].rstrip(b"\0").decode()
                address, length, file_offset = struct.unpack_from("<QQI", data, cursor + 32)
                sections[(segment, name)] = (address, length, file_offset)
                cursor += 80
        elif command == LOAD_UUID:
            raw = data[offset + 8:offset + 24].hex().upper()
            uuid = "-".join((raw[0:8], raw[8:12], raw[12:16], raw[16:20], raw[20:32]))
        offset += size
    return sections, uuid


def selector_references(data, sections):
    method_names = [value for key, value in sections.items() if key[1] == "__objc_methname"]
    references = [value for key, value in sections.items() if key[1] == "__objc_selrefs"]
    if not method_names or not references:
        return []
    name_address, name_length, name_offset = method_names[0]
    selectors = set()
    for _, length, file_offset in references:
        for index in range(0, length, 8):
            value = struct.unpack_from("<Q", data, file_offset + index)[0]
            # Chained-fixup rebases keep the target in the low 36 bits (32 for authenticated pointers);
            # every decoded target must land inside __objc_methname, otherwise the format is not understood.
            target = value & (0xFFFFFFFF if value >> 63 else 0xFFFFFFFFF)
            if not name_address <= target < name_address + name_length:
                raise SystemExit("Selector reference 0x%x is outside __objc_methname" % value)
            start = name_offset + target - name_address
            selectors.add(data[start:data.index(b"\0", start)].decode())
    return sorted(selectors)


def consumer_roots(name, commit, build, binary):
    binary = Path(binary).resolve()
    data = binary.read_bytes()
    sections, uuid = macho_sections(data)
    imports = defaultdict(set)
    for line in output(["xcrun", "dyld_info", "-imports", str(binary)]).splitlines():
        match = IMPORT_LINE.match(line)
        if match:
            imports[match.group(2)].add(match.group(1))
    return {"consumer": name, "commit": commit, "build": build, "binary": str(binary.relative_to(ROOT))
            if binary.is_relative_to(ROOT) else str(binary), "sha256": sha256(binary), "uuid": uuid,
            "imports": {library: sorted(imports[library]) for library in NATIVE if library in imports},
            "other_libraries": sorted(set(imports) - set(NATIVE)),
            "selectors": selector_references(data, sections)}


def roots(arguments):
    results = []
    for name, commit, build, binary in arguments.consumer:
        results.append(consumer_roots(name, commit, build, binary))
    EVIDENCE.mkdir(parents=True, exist_ok=True)
    write_changed(EVIDENCE / "roots.json", json.dumps({"schema_version": 1, "consumers": results}, indent=1) + "\n")
    for row in results:
        counts = ", ".join("%s %d" % (library, len(symbols)) for library, symbols in row["imports"].items())
        print("%s (%s): %s; %d selectors" % (row["consumer"], row["build"], counts, len(row["selectors"])))


def export_addresses(image):
    """External defined symbols from the image's symbol table, so aliases IDA names once still resolve."""
    addresses = {}
    for line in output(["nm", "-m", str(image)]).splitlines():
        match = re.match(r"^([0-9a-f]{16}) \(([^)]+)\) external (\S+)$", line)
        if match:
            addresses[match.group(3)] = "0x" + match.group(1).lstrip("0")
    return addresses


def frontier(arguments):
    """Write one image's root list for Tools/scope-ida.py from the consumer roots and earlier closures."""
    image = arguments.image
    roots_file = json.loads((EVIDENCE / "roots.json").read_text())
    symbols = defaultdict(set)
    selectors = defaultdict(set)
    classes = defaultdict(set)
    for row in roots_file["consumers"]:
        for symbol in row["imports"].get(image, []):
            symbols[symbol].add(row["consumer"])
            if symbol.startswith("_OBJC_CLASS_$_"):
                classes[symbol[len("_OBJC_CLASS_$_"):]].add(row["consumer"])
        for selector in row["selectors"]:
            selectors[selector].add(row["consumer"])
    # Cross-image edges found by earlier closures of the other image enter as roots of this one.
    for other in NATIVE:
        path = EVIDENCE / ("closure-%s-%s.json" % (arguments.platform, other))
        if other == image or not path.exists():
            continue
        closure = json.loads(path.read_text())
        for edge in closure["imports"]:
            if edge["module"] == image:
                symbols[edge["name"]].add(other)
                if edge["name"].startswith("_OBJC_CLASS_$_"):
                    classes[edge["name"][len("_OBJC_CLASS_$_"):]].add(other)
        # Messages the other image's closure sends may land on this image's root classes.
        for selector in closure["selectors_sent"]:
            selectors[selector].add(other)
    exports = export_addresses(ROOT / "Local/Databases" / arguments.platform / image)
    request = {"image": image, "platform": arguments.platform,
               "symbols": {name: sorted(value) for name, value in sorted(symbols.items())},
               "addresses": {name: exports[name] for name in sorted(symbols) if name in exports},
               "classes": {name: sorted(value) for name, value in sorted(classes.items())},
               "selectors": {name: sorted(value) for name, value in sorted(selectors.items())},
               "output": str(EVIDENCE / ("closure-%s-%s.json" % (arguments.platform, image)))}
    path = EVIDENCE / ("request-%s-%s.json" % (arguments.platform, image))
    write_changed(path, json.dumps(request, indent=1) + "\n")
    print("%s: %d symbols, %d classes, %d selectors -> %s" % (
        image, len(symbols), len(classes), len(selectors), path.relative_to(ROOT)))


VERBS = {"Create", "Get", "Set", "Copy", "Is", "Add", "Remove", "Append", "Apply", "Contains", "Replace",
          "Insert", "Find", "Has", "Compare", "Make", "Register", "Init", "Open", "Close", "Read", "Write",
          "Delete", "Exchange", "Sort", "Search", "Perform", "Wake", "Run", "Stop", "Invalidate", "Schedule",
          "Unschedule", "Post", "Retain", "Release", "Equal", "Hash", "Autorelease", "Show", "Log", "Trim",
          "Pad", "Lowercase", "Uppercase", "Capitalize", "Fold", "Normalize", "Transform", "Fill", "Initialize",
          "Allocate", "Deallocate", "Reallocate", "Preferred", "Clear", "Increment", "Decrement", "Flip",
          "Count", "Grow", "Cache", "Should", "Convert", "Decode", "Encode", "Ensure", "Resolve", "Synchronize"}
RUNTIME_NAMES = {"Retain", "Release", "Equal", "Hash", "Autorelease", "GetTypeID", "CopyDescription",
                 "GetAllocator", "GetRetainCount", "MakeCollectable", "CopyTypeIDDescription", "Show"}
EXTERNAL_MODULES = {"libobjc", "libSystem", "libicucore", "libswiftCore"}


def family(name):
    """A name-derived subsystem label used to plan readings; it is inferred, never a source-file claim."""
    match = re.match(r"^([-+])\[([^ (\]]+)", name)
    if match:
        return "objc:" + match.group(2)
    if name.startswith("_$s") or name.startswith("$s"):
        return "swift"
    if name.startswith("__Z"):
        return "c++"
    if "block_invoke" in name:
        name = name.split("_block_invoke")[0]
    stripped = name.lstrip("_")
    match = re.match(r"^(?:CF|NS)(?=[A-Z])", stripped)
    if not match:
        return "other"
    rest = stripped[match.end():]
    if any(rest.startswith(word) for word in RUNTIME_NAMES):
        return stripped[:2] + "Runtime"
    words = re.findall(r"[A-Z]+(?![a-z])|[A-Z][a-z0-9]*", rest)
    family = []
    for word in words:
        if word in VERBS and family:
            break
        family.append(word)
        if len(family) == 3:
            break
    return stripped[:2] + "".join(family) if family else stripped[:2] + "other"


def components(nodes, edges):
    """Tarjan's strongly connected components (iterative) and their bottom-up levels."""
    index = {}
    low = {}
    on_stack = set()
    stack = []
    result = {}
    counter = [0]
    for start in nodes:
        if start in index:
            continue
        work = [(start, iter(edges.get(start, ())))]
        index[start] = low[start] = counter[0]
        counter[0] += 1
        stack.append(start)
        on_stack.add(start)
        while work:
            node, children = work[-1]
            advanced = False
            for child in children:
                if child not in index:
                    index[child] = low[child] = counter[0]
                    counter[0] += 1
                    stack.append(child)
                    on_stack.add(child)
                    work.append((child, iter(edges.get(child, ()))))
                    advanced = True
                    break
                if child in on_stack:
                    low[node] = min(low[node], index[child])
            if advanced:
                continue
            work.pop()
            if work:
                low[work[-1][0]] = min(low[work[-1][0]], low[node])
            if low[node] == index[node]:
                identifier = len(set(result.values()))
                while True:
                    member = stack.pop()
                    on_stack.discard(member)
                    result[member] = identifier
                    if member == node:
                        break
    # Tarjan emits components in reverse topological order: callees before callers.
    level = {}
    members = defaultdict(list)
    for node, identifier in result.items():
        members[identifier].append(node)
    for identifier in sorted(members):
        depth = 0
        for node in members[identifier]:
            for child in edges.get(node, ()):
                if result[child] != identifier:
                    depth = max(depth, level[result[child]] + 1)
        level[identifier] = depth
    return result, level, members


def compact_json(value):
    """Indented JSON whose leaf rows (functions, families, components, symbol lists) stay on one line."""
    value = copy.deepcopy(value)
    placeholders = {}

    def hold(item):
        key = "@@row%d@@" % len(placeholders)
        placeholders[key] = json.dumps(item, separators=(",", ":"))
        return key

    for image in value["images"]:
        image["functions"] = [hold(row) for row in image["functions"]]
        image["families"] = {name: hold(row) for name, row in image["families"].items()}
        image["strongly_connected"] = [hold(row) for row in image["strongly_connected"]]
        image["data_roots"] = [hold(row) for row in image["data_roots"]]
        image["unresolved_roots"] = [hold(row) for row in image["unresolved_roots"]]
        for cut in image["cuts"].values():
            cut["symbols"] = hold(cut["symbols"])
    text = json.dumps(value, indent=1)
    for key, row in placeholders.items():
        text = text.replace('"%s"' % key, row)
    return text + "\n"


def assemble(arguments):
    roots_file = json.loads((EVIDENCE / "roots.json").read_text())
    pinned = json.loads((ROOT / "Configuration/ReferenceImages.json").read_text())[arguments.platform]["images"]
    restoration = json.loads((ROOT / "Configuration/Restoration.json").read_text())
    # Restored definitions match by native address, which avoids Mach-O/Swift spelling differences.
    restored = {int(row["address"], 16) for row in restoration["definitions"] if row.get("address")}
    consumers = {row["consumer"] for row in roots_file["consumers"]}
    closures = {}
    for image in NATIVE:
        path = EVIDENCE / ("closure-%s-%s.json" % (arguments.platform, image))
        if not path.exists():
            raise SystemExit("Missing closure evidence: " + str(path.relative_to(ROOT)))
        closure = json.loads(path.read_text())
        if pinned[image]["uuid"] != closure["uuid"] or pinned[image]["sha256"] != closure["sha256"]:
            raise SystemExit("Closure evidence does not match the pinned %s image" % image)
        closures[image] = closure
    by_name = {image: {row["name"]: row for row in closure["functions"]} for image, closure in closures.items()}
    nodes = []
    edges = {}
    for image, closure in closures.items():
        for row in closure["functions"]:
            node = (image, row["name"])
            nodes.append(node)
            targets = [(image, name) for name in row["calls"] + row["address_taken"]]
            for edge in row["imports"]:
                module, name = edge.split(":", 1)
                if module in by_name and name in by_name[module]:
                    targets.append((module, name))
            edges[node] = targets
    component, level, members = components(nodes, edges)
    # A function reached through the other image inherits every consumer of that image's closure.
    image_consumers = {image: set().union(*(set(row["roots"]) & consumers for row in closure["functions"]))
                      for image, closure in closures.items()}
    result = {"schema_version": 1, "platform": arguments.platform,
              "method": "Consumer link-level roots (imports from CoreFoundation/Foundation and referenced "
                        "selectors of linked consumer binaries) closed in IDA over direct and tail calls, taken "
                        "function addresses and __got targets, across both images to a fixpoint. Objective-C "
                        "messages expand only to methods of classes the closure relates to (tiers "
                        "message-candidate and message-reachable). CF runtime class callbacks are followed "
                        "through the class records of classes instantiated with a constant type ID or "
                        "registered with a record address inside the closure; other CF callbacks stored in "
                        "data, blocks invoked through pointers and Swift witnesses are not followed. Families "
                        "are name-derived planning labels; components and levels use call edges only.",
              "consumers": [{key: row[key] for key in ("consumer", "commit", "build", "sha256", "uuid")}
                            | {"root_symbols": {library: len(symbols) for library, symbols in row["imports"].items()},
                               "selectors": len(row["selectors"])}
                            for row in roots_file["consumers"]],
              "images": []}
    component_sizes = {identifier: len(nodes) for identifier, nodes in members.items()}
    for image, closure in closures.items():
        functions = []
        families = defaultdict(lambda: {"functions": 0, "bytes": 0, "restored": 0, "tiers": defaultdict(int),
                                        "max_level": 0})
        for row in closure["functions"]:
            node = (image, row["name"])
            reach = set(row["roots"]) & consumers
            for other in NATIVE:
                if other in row["roots"]:
                    reach |= image_consumers[other]
            label = family(row["name"])
            identifier = component[node]
            entry = {"name": row["name"], "address": row["address"], "size": row["size"], "tier": row["tier"],
                     "family": label, "component": identifier, "level": level[identifier],
                     "consumers": sorted(reach), "restored": int(row["address"], 16) in restored}
            functions.append(entry)
            summary = families[label]
            summary["functions"] += 1
            summary["bytes"] += row["size"]
            summary["restored"] += entry["restored"]
            summary["tiers"][row["tier"]] += 1
            summary["max_level"] = max(summary["max_level"], entry["level"])
        cuts = defaultdict(set)
        for edge in closure["imports"]:
            if edge["module"] not in NATIVE:
                cuts[edge["module"]].add(edge["name"])
        large = sorted({entry["component"] for entry in functions if component_sizes[entry["component"]] > 1})
        result["images"].append({
            "image": image, "uuid": closure["uuid"], "sha256": closure["sha256"],
            "unresolved_roots": closure["unresolved_roots"],
            "data_roots": [{key: row[key] for key in ("symbol", "address", "section")} for row in
                           closure["data_roots"]],
            "function_count": len(functions),
            "function_bytes": sum(row["size"] for row in functions),
            "tiers": {tier: sum(1 for row in functions if row["tier"] == tier) for tier in
                      ("symbol", "message-candidate", "message-reachable")},
            "restored_count": sum(1 for row in functions if row["restored"]),
            "strongly_connected": [{"component": identifier, "functions": component_sizes[identifier],
                                    "level": level[identifier]} for identifier in large],
            "families": {name: dict(value, tiers=dict(value["tiers"])) for name, value in
                         sorted(families.items(), key=lambda item: -item[1]["bytes"])},
            "cuts": {module: {"classification": "external" if module in EXTERNAL_MODULES else "unclassified",
                              "symbols": sorted(names)} for module, names in sorted(cuts.items())},
            "selectors_sent": len(closure["selectors_sent"]),
            "functions": functions})
    text = compact_json(result)
    if arguments.check:
        if not SCOPE.exists() or SCOPE.read_text() != text:
            raise SystemExit("Configuration/Scope.json is not current; run Tools/scope.py assemble")
        print("Scope current")
        return
    write_changed(SCOPE, text)
    for row in result["images"]:
        print("%s: %d functions, %d bytes, tiers %s, %d restored, %d cut modules, largest component %d" % (
            row["image"], row["function_count"], row["function_bytes"], row["tiers"], row["restored_count"],
            len(row["cuts"]), max([item["functions"] for item in row["strongly_connected"]] or [1])))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    roots_command = commands.add_parser("roots", help="measure consumer binaries")
    roots_command.add_argument("--consumer", nargs=4, action="append", required=True,
                              metavar=("NAME", "COMMIT", "BUILD", "BINARY"))
    roots_command.set_defaults(handler=roots)
    frontier_command = commands.add_parser("frontier", help="write an IDA closure request for one image")
    frontier_command.add_argument("--image", choices=NATIVE, required=True)
    frontier_command.add_argument("--platform", default="iphone-simulator")
    frontier_command.set_defaults(handler=frontier)
    assemble_command = commands.add_parser("assemble", help="write or check Configuration/Scope.json")
    assemble_command.add_argument("--platform", default="iphone-simulator")
    assemble_command.add_argument("--check", action="store_true")
    assemble_command.set_defaults(handler=assemble)
    arguments = parser.parse_args()
    arguments.handler(arguments)


if __name__ == "__main__":
    main()

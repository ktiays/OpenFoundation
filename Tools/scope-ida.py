#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

# IDAPython closure of consumer roots over one pinned image (AGENTS.md, "Scope"). Run through the IDA MCP
# server's run_script inside a scratch clone, after setting SCOPE_REQUEST to a request file written by
# `Tools/scope.py frontier`:
#
#     SCOPE_REQUEST = "/absolute/path/Local/Evidence/scope/request-iphone-simulator-CoreFoundation.json"
#     exec(open("/absolute/path/Tools/scope-ida.py").read())
#
# Edges followed: direct and tail calls, and addresses of functions taken in code or reached through __got.
# Objective-C messages are recorded per function (selector names) and are not expanded, except for the
# consumer's own messages: a method whose selector the consumer references and whose class is a root class,
# one of its superclasses, a category on one of them, or a subclass of a root class in this image, is a
# `message-candidate` root. Those roots are an over-approximation that readings narrow.

import json
import re
import struct
import ida_bytes
import ida_funcs
import ida_nalt
import ida_name
import ida_segment
import idautils
import idc

METHOD_NAME = re.compile(r"^([-+])\[([^ (\]]+)(?:\(([^)]*)\))? (.+)\]$")
TIER_RANK = {"message-reachable": 1, "message-candidate": 2, "symbol": 3}


def segment_name(ea):
    segment = ida_segment.getseg(ea)
    return ida_segment.get_segm_name(segment) if segment else ""


def image_identity():
    header = ida_segment.get_first_seg().start_ea
    count = ida_bytes.get_dword(header + 16)
    offset = header + 32
    for _ in range(count):
        command = ida_bytes.get_dword(offset)
        size = ida_bytes.get_dword(offset + 4)
        if command == 0x1B:
            raw = ida_bytes.get_bytes(offset + 8, 16).hex().upper()
            return "-".join((raw[0:8], raw[8:12], raw[12:16], raw[16:20], raw[20:32]))
        offset += size
    raise RuntimeError("LC_UUID not found")


def import_modules():
    """Import modules keyed by UNDEF address and by symbol name.

    Images extracted from the dyld shared cache keep their stubs, which load pointers from other images'
    regions; a stub is identified by the import name IDA gives its first instruction.
    """
    modules = {}
    for index in range(ida_nalt.get_import_module_qty()):
        module = ida_nalt.get_import_module_name(index) or ""
        short = module.rsplit("/", 1)[-1].split(".")[0]

        def visit(ea, name, ordinal, short=short):
            modules[ea] = short
            if name:
                modules[name] = short
            return True

        ida_nalt.enum_import_names(index, visit)
    return modules


def c_string(ea):
    value = ida_bytes.get_strlit_contents(ea, -1, ida_nalt.STRTYPE_C)
    return value.decode("utf-8", "replace") if value else None


def function_start(ea):
    function = ida_funcs.get_func(ea)
    return function.start_ea if function else None


class Closure:
    def __init__(self, request):
        self.request = request
        self.modules = import_modules()
        self.functions = {}
        self.imports = {}
        self.unresolved = []
        self.data_roots = []
        self.selectors_sent = {}

    def import_of(self, ea):
        """Name and module of an imported symbol reached through a stub, a __got entry or UNDEF."""
        segment = segment_name(ea)
        if segment == "UNDEF" or ea in self.modules:
            return ida_name.get_name(ea), self.modules.get(ea, "?")
        if segment == "__got":
            for target in idautils.DataRefsFrom(ea):
                if target in self.modules or segment_name(target) == "UNDEF":
                    return ida_name.get_name(target), self.modules.get(target, "?")
        if segment == "__stubs":
            name = ida_name.get_name(function_start(ea) or ea) or ""
            for candidate in (name, name[1:] if name.startswith("_") else "_" + name):
                if candidate in self.modules:
                    return name, self.modules[candidate]
            return name, "?"
        return None

    def internal_target(self, ea):
        """A function start inside this image reached from a __got entry, or None."""
        if segment_name(ea) != "__got":
            return None
        for target in idautils.DataRefsFrom(ea):
            if segment_name(target) == "__text" and function_start(target) == target:
                return target
        return None

    def scan(self, start):
        function = ida_funcs.get_func(start)
        row = {"name": ida_name.get_name(start), "address": "0x%x" % start,
               "size": function.end_ea - function.start_ea if function else 0,
               "calls": set(), "address_taken": set(), "imports": set(), "selectors": set(),
               "classes": set(), "cfstrings": 0, "data": set()}
        if not function:
            return row
        for head in idautils.FuncItems(start):
            for target in idautils.CodeRefsFrom(head, False):
                self.classify(start, row, target, True)
            for target in idautils.DataRefsFrom(head):
                self.classify(start, row, target, False)
        return row

    def classify(self, owner, row, target, code):
        segment = segment_name(target)
        if segment == "__text":
            begin = function_start(target)
            if begin is None or begin == owner:
                return
            if code:
                row["calls"].add(begin)
            elif begin == target:
                row["address_taken"].add(begin)
            return
        if segment == "__objc_stubs":
            name = ida_name.get_name(function_start(target) or target)
            selector = name.split("$", 1)[1] if "$" in name else name
            row["selectors"].add(selector)
            return
        if segment == "__objc_selrefs":
            pointer = ida_bytes.get_qword(target)
            row["selectors"].add(c_string(pointer) or ida_name.get_name(target))
            return
        if segment in ("__objc_classrefs", "__objc_superrefs"):
            pointer = ida_bytes.get_qword(target)
            row["classes"].add(ida_name.get_name(pointer) or ida_name.get_name(target))
            return
        if segment == "__cfstring":
            row["cfstrings"] += 1
            return
        if segment == "__got":
            pointer = ida_bytes.get_qword(target)
            if segment_name(pointer) == "__objc_data":
                row["classes"].add(ida_name.get_name(pointer))
                return
        internal = self.internal_target(target)
        if internal is not None:
            row["address_taken"].add(internal)
            return
        imported = self.import_of(target)
        if imported and imported[0]:
            row["imports"].add(imported)
            return
        if segment and not segment.startswith("__text"):
            name = ida_name.get_name(target) or ("%s+0x%x" % (segment, target))
            row["data"].add(name)

    def method_index(self):
        methods = []
        for ea in idautils.Functions():
            match = METHOD_NAME.match(ida_name.get_name(ea) or "")
            if match:
                methods.append((ea,) + match.groups())
        return methods

    def superclass(self, name):
        ea = ida_name.get_name_ea(idc.BADADDR, "_OBJC_CLASS_$_" + name)
        if ea == idc.BADADDR or segment_name(ea) in ("UNDEF", ""):
            return None
        pointer = ida_bytes.get_qword(ea + 8)
        target = ida_name.get_name(pointer) or ""
        if not target:
            imported = self.import_of(pointer)
            target = imported[0] if imported else ""
        return target[len("_OBJC_CLASS_$_"):] if target.startswith("_OBJC_CLASS_$_") else None

    def roots(self):
        result = {}
        for symbol, sources in self.request["symbols"].items():
            ea = ida_name.get_name_ea(idc.BADADDR, symbol)
            if ea == idc.BADADDR and symbol in self.request["addresses"]:
                ea = int(self.request["addresses"][symbol], 16)
            if ea == idc.BADADDR or segment_name(ea) in ("UNDEF", ""):
                self.unresolved.append({"symbol": symbol, "reached_from": sources})
                continue
            if segment_name(ea) == "__text":
                result.setdefault(ea, ["symbol", set()])[1].update(sources)
            else:
                self.data_roots.append({"symbol": symbol, "address": "0x%x" % ea, "section": segment_name(ea),
                                       "ida_name": ida_name.get_name(ea), "reached_from": sources})
        classes = set(self.request["classes"])
        lineage = {}
        all_classes = set()
        for ea, name in idautils.Names():
            if name.startswith("_OBJC_CLASS_$_") and segment_name(ea) not in ("UNDEF", ""):
                all_classes.add(name[len("_OBJC_CLASS_$_"):])
        for name in all_classes:
            chain = []
            current = name
            while current and len(chain) < 32:
                chain.append(current)
                current = self.superclass(current)
            lineage[name] = chain
        related = {}
        for name, chain in lineage.items():
            for root in classes:
                if name == root or root in chain:
                    related.setdefault(name, set()).update(self.request["classes"][root])
        for root in classes:
            for ancestor in lineage.get(root, [root]):
                related.setdefault(ancestor, set()).update(self.request["classes"][root])
        self.lineage = lineage
        self.subclasses = {}
        for name, chain in lineage.items():
            for ancestor in chain[1:]:
                self.subclasses.setdefault(ancestor, set()).add(name)
        self.methods = self.method_index()
        self.related = related
        selectors = self.request["selectors"]
        for ea, _, class_name, category, selector in self.methods:
            if class_name in related and selector in selectors:
                entry = result.setdefault(ea, ["message-candidate", set()])
                entry[1].update(set(selectors[selector]) & related[class_name] or related[class_name])
        return result

    def class_family(self, name):
        """A referenced class, its superclasses and its subclasses in this image."""
        family = set(self.lineage.get(name, [name]))
        pending = [name]
        while pending:
            for subclass in self.subclasses.get(pending.pop(), ()):
                if subclass not in family:
                    family.add(subclass)
                    pending.append(subclass)
        return family

    def run(self):
        roots = self.roots()
        tier = {}
        reach = {}
        work = []
        for ea, (kind, sources) in roots.items():
            tier[ea] = kind
            reach[ea] = set(sources)
            work.append(ea)
        rows = {}
        self.propagate(work, tier, reach, rows)
        # Bounded message expansion: a selector sent inside the closure reaches its implementations only in
        # classes the closure already relates to (root classes and their families, and classes referenced
        # by closure functions with their families). Repeat until no implementation is added.
        while True:
            classes = set(self.related)
            for row in rows.values():
                for reference in row["classes"]:
                    if reference.startswith("_OBJC_CLASS_$_"):
                        classes |= self.class_family(reference[len("_OBJC_CLASS_$_"):])
            senders = {}
            for ea, row in rows.items():
                for selector in row["selectors"]:
                    senders.setdefault(selector, set()).add(ea)
            added = []
            for ea, _, class_name, category, selector in self.methods:
                if ea in tier or class_name not in classes or selector not in senders:
                    continue
                tier[ea] = "message-reachable"
                reach[ea] = set()
                for sender in senders[selector]:
                    reach[ea] |= reach[sender]
                added.append(ea)
            if not added:
                break
            self.propagate(added, tier, reach, rows)
        return self.collect(rows, tier, reach)

    def propagate(self, work, tier, reach, rows):
        while work:
            ea = work.pop()
            if ea not in rows:
                rows[ea] = self.scan(ea)
            row = rows[ea]
            for target in row["calls"] | row["address_taken"]:
                previous = tier.get(target)
                if previous is None or TIER_RANK[tier[ea]] > TIER_RANK[previous]:
                    tier[target] = tier[ea]
                before = len(reach.setdefault(target, set()))
                reach[target] |= reach[ea]
                if tier[target] != previous or len(reach[target]) != before or target not in rows:
                    work.append(target)

    def collect(self, rows, tier, reach):
        output = []
        for ea, row in sorted(rows.items()):
            for name, module in row["imports"]:
                self.imports.setdefault((module, name), set()).add(row["name"])
            for selector in row["selectors"]:
                self.selectors_sent[selector] = self.selectors_sent.get(selector, 0) + 1
            output.append({"name": row["name"], "address": row["address"], "size": row["size"],
                           "tier": tier[ea], "roots": sorted(reach[ea]),
                           "calls": sorted(rows[target]["name"] for target in row["calls"] if target in rows),
                           "address_taken": sorted(rows[target]["name"] for target in row["address_taken"]
                                                   if target in rows),
                           "imports": sorted("%s:%s" % (module, name) for name, module in row["imports"]),
                           "selectors": sorted(row["selectors"]), "classes": sorted(row["classes"]),
                           "cfstrings": row["cfstrings"], "data": sorted(row["data"])})
        return output


def main():
    request = json.load(open(SCOPE_REQUEST))
    closure = Closure(request)
    functions = closure.run()
    result = {"schema_version": 1, "image": request["image"], "platform": request["platform"],
              "uuid": image_identity(), "sha256": ida_nalt.retrieve_input_file_sha256().hex(),
              "database": idc.get_idb_path(), "functions": functions,
              "unresolved_roots": closure.unresolved, "data_roots": closure.data_roots,
              "imports": [{"module": module, "name": name, "callers": len(callers)}
                          for (module, name), callers in sorted(closure.imports.items())],
              "selectors_sent": dict(sorted(closure.selectors_sent.items()))}
    with open(request["output"], "w") as handle:
        json.dump(result, handle, indent=1)
        handle.write("\n")
    tiers = {}
    for row in functions:
        tiers[row["tier"]] = tiers.get(row["tier"], 0) + 1
    return {"functions": len(functions), "tiers": tiers, "unresolved_roots": len(closure.unresolved),
            "data_roots": len(closure.data_roots),
            "imports": len(result["imports"]), "selectors_sent": len(result["selectors_sent"])}


SCOPE_RESULT = main()

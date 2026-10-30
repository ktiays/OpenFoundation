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
#
# CF runtime class callbacks are followed through the native class records: a closure function that passes a
# constant type ID to an instance-creating runtime entry (W1 of __CFRuntimeCreateInstance and
# __CFRuntimeInitStaticInstance, W1 of __CFRuntimeSetInstanceTypeID and __CFRuntimeSetInstanceTypeIDAndIsa)
# instantiates the builtin class at that index of ___CFRuntimeBuiltinClassTable, and one that passes a record
# address in X0 to __CFRuntimeRegisterClass registers that record. Every function pointer in such a record's
# callback slots (offsets 16-80) is reached with the site's tier and consumers. A site whose argument is not a
# constant or a record address in the same basic block is reported in `unresolved_class_sites`.
#
# Functions whose addresses are stored in data that closure functions or data roots reference (to the next
# named address, following data-to-data pointers three levels) are reached with the referencing tier and are
# listed in `data_callbacks`; Objective-C metadata, string sections and the runtime class tables and records
# are excluded. A referenced data object that is a CF static instance (an Objective-C class isa, then an info
# word with the static bit 0x80, a builtin type and a zero retain field) reaches its class's callbacks; the
# types are listed in `static_instances`.
#
# Image initializers (__init_offsets and __mod_init_func entries) and +load methods are roots of every consumer
# in the request, because they run whenever the image is loaded; they are listed in `initializers`.

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
TYPE_ID_ENTRIES = ("__CFRuntimeCreateInstance", "__CFRuntimeInitStaticInstance", "__CFRuntimeSetInstanceTypeID",
                   "__CFRuntimeSetInstanceTypeIDAndIsa")
REGISTER_ENTRY = "__CFRuntimeRegisterClass"
BUILTIN_TABLE = "___CFRuntimeBuiltinClassTable"
BUILTIN_COUNT = 72
CALLBACK_OFFSETS = range(16, 88, 8)
DATA_SKIPPED = ("__objc_", "__cfstring", "__cstring", "__ustring", "__oslogstring")


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
        self.unresolved_class_sites = []
        self.initializers = []
        self.data_callbacks = {}
        self.static_instances = {}
        self.builtin_table = None
        self.class_callbacks = {}

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
               "classes": set(), "cfstrings": 0, "data": set(), "data_addresses": set()}
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
            row["data_addresses"].add(target)

    def runtime_entries(self):
        """Addresses of the type-ID entries and the registration entry present in this image."""
        entries = {}
        for name in TYPE_ID_ENTRIES + (REGISTER_ENTRY,):
            ea = ida_name.get_name_ea(idc.BADADDR, name)
            if ea != idc.BADADDR and segment_name(ea) == "__text":
                entries[ea] = name
        table = ida_name.get_name_ea(idc.BADADDR, BUILTIN_TABLE)
        return entries, (table if table != idc.BADADDR else None)

    def argument(self, call, register):
        """The value written to W/X<register> before `call` in its basic block: ('constant', value),
        ('address', ea) or None."""
        names = ("W%d" % register, "X%d" % register)
        function = ida_funcs.get_func(call)
        if not function:
            return None
        head = call
        # A branch target (an instruction with a code reference other than fall-through) starts the block.
        block_start = bool(list(idautils.CodeRefsTo(call, False)))
        while not block_start:
            head = idc.prev_head(head, function.start_ea)
            if head == idc.BADADDR or head < function.start_ea:
                return None
            block_start = bool(list(idautils.CodeRefsTo(head, False))) or head == function.start_ea
            mnemonic = idc.print_insn_mnem(head)
            if mnemonic in ("BL", "BLR", "B", "BR", "RET", "CBZ", "CBNZ", "TBZ", "TBNZ") or mnemonic.startswith("B."):
                return None
            if idc.print_operand(head, 0) not in names:
                continue
            if mnemonic in ("MOV", "MOVZ") and idc.get_operand_type(head, 1) == idc.o_imm:
                return ("constant", idc.get_operand_value(head, 1) & 0xffffffff)
            if mnemonic in ("ADRL", "ADR", "ADD"):
                targets = [target for target in idautils.DataRefsFrom(head) if segment_name(target) != "__text"]
                if targets:
                    return ("address", targets[-1])
            return None
        return None

    def record_callbacks(self, record):
        callbacks = []
        for offset in CALLBACK_OFFSETS:
            pointer = ida_bytes.get_qword(record + offset)
            if pointer and segment_name(pointer) == "__text" and function_start(pointer) == pointer:
                callbacks.append(pointer)
        return callbacks

    def class_edges(self, ea, entries, table):
        """Callback functions of the classes `ea` instantiates or registers, and its unresolved sites."""
        callbacks = set()
        unresolved = []
        function = ida_funcs.get_func(ea)
        if not function or not entries:
            return callbacks, unresolved
        for head in idautils.FuncItems(ea):
            for target in idautils.CodeRefsFrom(head, False):
                if target not in entries:
                    continue
                name = entries[target]
                if name == REGISTER_ENTRY:
                    value = self.argument(head, 0)
                    if value and value[0] == "address":
                        callbacks.update(self.record_callbacks(value[1]))
                        continue
                else:
                    value = self.argument(head, 1)
                    if value and value[0] == "constant" and table is not None and value[1] < BUILTIN_COUNT:
                        record = ida_bytes.get_qword(table + 8 * value[1])
                        if record:
                            callbacks.update(self.record_callbacks(record))
                        continue
                unresolved.append({"function": ida_name.get_name(ea), "site": "0x%x" % head, "entry": name,
                                   "argument": value[0] if value else None})
        return callbacks, unresolved

    def static_instance_type(self, address):
        """The builtin type ID of a CF static instance at `address` (an Objective-C class isa, then an info
        word with the static bit 0x80, a builtin type in bits 8-17 and a zero retain field), or None."""
        isa = ida_bytes.get_qword(address)
        info = ida_bytes.get_qword(address + 8)
        if not isa or not segment_name(isa).startswith("__objc_data"):
            return None
        if info & 0x80 == 0:
            return None
        type_id = (info >> 8) & 0x3ff
        if info & ~0x3ffff != 0 or type_id == 0 or type_id >= BUILTIN_COUNT:
            return None
        return type_id

    def data_functions(self, address, excluded, depth=0, seen=None):
        """Functions whose addresses are stored in the data object at `address` (to the next named address,
        at most 4 KB), following pointers into other data objects up to three levels."""
        seen = set() if seen is None else seen
        if address in seen or address in excluded or depth > 3:
            return set()
        seen.add(address)
        segment = segment_name(address)
        if not segment or segment.startswith(DATA_SKIPPED) or segment in ("__text", "__stubs", "__got", "UNDEF"):
            return set()
        end = address + 8
        while end < address + 4096 and not ida_name.get_name(end) and segment_name(end) == segment:
            end += 8
        functions = set()
        type_id = self.static_instance_type(address)
        if type_id is not None and self.builtin_table is not None:
            record = ida_bytes.get_qword(self.builtin_table + 8 * type_id)
            if record:
                functions |= set(self.record_callbacks(record))
                self.static_instances.setdefault(type_id, set()).add(ida_name.get_name(address) or "0x%x" % address)
        start = address - address % 8
        for slot in range(start, end, 8):
            pointer = ida_bytes.get_qword(slot)
            target_segment = segment_name(pointer)
            if target_segment == "__text":
                if function_start(pointer) == pointer:
                    functions.add(pointer)
            elif target_segment and pointer not in excluded:
                functions |= self.data_functions(pointer, excluded, depth + 1, seen)
        return functions

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
        # Image initializers run whenever a consumer loads the image, so they are roots of every consumer
        # that reaches it: __init_offsets holds 32-bit offsets from the image base, __mod_init_func pointers.
        consumers = set()
        for sources in list(self.request["symbols"].values()) + list(self.request["classes"].values()):
            consumers.update(sources)
        self.initializers = []
        base = ida_nalt.get_imagebase()
        for segment_ea in idautils.Segments():
            name = segment_name(segment_ea)
            if name not in ("__init_offsets", "__mod_init_func"):
                continue
            width = 4 if name == "__init_offsets" else 8
            for slot in range(segment_ea, ida_segment.getseg(segment_ea).end_ea, width):
                if width == 4:
                    target = base + (ida_bytes.get_dword(slot) & 0xffffffff)
                else:
                    target = ida_bytes.get_qword(slot)
                if segment_name(target) == "__text" and function_start(target) == target:
                    result.setdefault(target, ["symbol", set()])[1].update(consumers)
                    self.initializers.append({"function": ida_name.get_name(target), "address": "0x%x" % target,
                                              "section": name, "slot": "0x%x" % slot})
                else:
                    self.unresolved.append({"symbol": "%s+0x%x" % (name, slot - segment_ea),
                                            "reached_from": sorted(consumers)})
        # +load methods run when the image is loaded (the linker lists their classes and categories in
        # __objc_nlclslist and __objc_nlcatlist), so they are roots of the same consumers.
        for ea in idautils.Functions():
            match = METHOD_NAME.match(ida_name.get_name(ea) or "")
            if match and match.group(1) == "+" and match.group(4) == "load":
                result.setdefault(ea, ["symbol", set()])[1].update(consumers)
                self.initializers.append({"function": ida_name.get_name(ea), "address": "0x%x" % ea,
                                          "section": "+load", "slot": None})
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
        entries, table = self.runtime_entries()
        self.builtin_table = table
        scanned = set()
        data_scanned = set()
        excluded_data = set()
        for name in (BUILTIN_TABLE, "___CFRuntimeBuiltinObjCClassTable", "___CFRuntimeClassTables"):
            ea = ida_name.get_name_ea(idc.BADADDR, name)
            if ea != idc.BADADDR:
                excluded_data.add(ea)
        if table is not None:
            for index in range(BUILTIN_COUNT):
                record = ida_bytes.get_qword(table + 8 * index)
                if record:
                    excluded_data.add(record)
        # Data roots are referenced by the consumers themselves.
        root_functions = set()
        for row in self.data_roots:
            root_functions |= self.data_functions(int(row["address"], 16), excluded_data)
        consumers = set()
        for sources in list(self.request["symbols"].values()):
            consumers.update(sources)
        seeded = []
        for target in root_functions:
            if target not in tier:
                tier[target] = "symbol"
                reach[target] = set(consumers)
                seeded.append(target)
        self.propagate(seeded, tier, reach, rows)
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
            # Functions stored in data the closure references (allocator objects, callback tables). The
            # builtin class tables and their records are excluded: class callbacks are reached through
            # instantiation sites only.
            for ea in [ea for ea in rows if ea not in data_scanned]:
                data_scanned.add(ea)
                functions = set()
                for address in rows[ea]["data_addresses"]:
                    functions |= self.data_functions(address, excluded_data)
                functions.discard(ea)
                if functions:
                    self.data_callbacks[ea] = functions
                for target in functions:
                    previous = tier.get(target)
                    if previous is None or TIER_RANK[tier[ea]] > TIER_RANK[previous]:
                        tier[target] = tier[ea]
                    before = len(reach.setdefault(target, set()))
                    reach[target] |= reach[ea]
                    if tier[target] != previous or len(reach[target]) != before or target not in rows:
                        added.append(target)
            # Class callbacks of the classes closure functions instantiate or register.
            for ea in [ea for ea in rows if ea not in scanned]:
                scanned.add(ea)
                callbacks, unresolved = self.class_edges(ea, entries, table)
                self.unresolved_class_sites.extend(unresolved)
                if callbacks:
                    self.class_callbacks[ea] = callbacks
                for target in callbacks:
                    previous = tier.get(target)
                    if previous is None or TIER_RANK[tier[ea]] > TIER_RANK[previous]:
                        tier[target] = tier[ea]
                    before = len(reach.setdefault(target, set()))
                    reach[target] |= reach[ea]
                    if tier[target] != previous or len(reach[target]) != before or target not in rows:
                        added.append(target)
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
                           "class_callbacks": sorted(rows[target]["name"]
                                                     for target in self.class_callbacks.get(ea, ())
                                                     if target in rows),
                           "data_callbacks": sorted(rows[target]["name"]
                                                    for target in self.data_callbacks.get(ea, ())
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
              "unresolved_class_sites": closure.unresolved_class_sites,
              "initializers": closure.initializers,
              "static_instances": {str(k): sorted(v) for k, v in sorted(closure.static_instances.items())},
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
            "data_roots": len(closure.data_roots), "class_callbacks": sum(map(len, closure.class_callbacks.values())),
            "data_callbacks": sum(map(len, closure.data_callbacks.values())),
            "unresolved_class_sites": len(closure.unresolved_class_sites),
            "initializers": [row["function"] for row in closure.initializers],
            "static_instance_types": sorted(closure.static_instances),
            "imports": len(result["imports"]), "selectors_sent": len(result["selectors_sent"])}


SCOPE_RESULT = main()

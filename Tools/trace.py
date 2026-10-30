#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/30.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Cross-process trace comparison for object-runtime groups (AGENTS.md, "Trace comparison").

record   build the reference side against the system frameworks, run it three times in fresh processes,
         require identical traces and write TraceTests/Recorded/<match>/Traces.trace
check    run the reference side again and compare it with the recorded trace (drift and determinism)
compare  build the replacement side in OF_SYSTEM_NAMES=YES against the restored frameworks and compare
         it with the recorded trace; a scenario import the restored frameworks do not export is a setup
         failure (77), never a compatibility result
"""

import argparse
import subprocess
import sys
from pathlib import Path
from common import ROOT, build_directory, check_platform, defaults, output, run

SOURCES = ROOT / "TraceTests"
RUNS = 3


def recorded_path(match):
    return SOURCES / "Recorded" / match / "Traces.trace"


def compile_flags(configuration):
    config = defaults()
    sdk = output(["xcrun", "--sdk", "iphonesimulator", "--show-sdk-path"])
    return ["-target", "arm64-apple-ios" + config["IPHONEOS_DEPLOYMENT_TARGET"] + "-simulator",
            "-isysroot", sdk, "-I", str(SOURCES), "-std=" + config["GCC_C_LANGUAGE_STANDARD"],
            "-Wall", "-Wextra", "-Werror", "-g", *(["-O0"] if configuration == "Debug" else ["-O2"])]


def sources():
    return sorted(path for path in SOURCES.rglob("*.c"))


def build_reference(directory, configuration):
    directory.mkdir(parents=True, exist_ok=True)
    generated = directory / "Generated"
    run([sys.executable, ROOT / "Tools/reference-images.py", "header", generated / "ReferencePins.h"])
    executable = directory / "ReferenceTraces"
    run(["xcrun", "clang", *compile_flags(configuration), "-I", str(generated), "-DOF_TRACE_REFERENCE=1",
         *sources(), "-framework", "CoreFoundation", "-o", executable])
    run(["codesign", "--force", "--sign", "-", executable])
    return executable


def build_replacement(directory, products, configuration):
    directory.mkdir(parents=True, exist_ok=True)
    executable = directory / "ReplacementTraces"
    run(["xcrun", "clang", *compile_flags(configuration), "-I", str(products / "Generated"),
         "-F", str(products), "-DOF_TRACE_REFERENCE=0", "-DOF_SYSTEM_NAMES=1", *sources(),
         "-framework", "OpenCoreFoundation", "-Wl,-rpath," + str(products), "-o", executable])
    run(["codesign", "--force", "--sign", "-", executable])
    return executable


def core_foundation_imports(executable):
    """Symbols the reference side binds to CoreFoundation."""
    imports = set()
    for line in output(["xcrun", "dyld_info", "-imports", executable]).splitlines():
        fields = line.split()
        # "0x0003  _CFStringCreateWithCString  (from CoreFoundation)"
        if len(fields) == 4 and fields[0].startswith("0x") and fields[2:] == ["(from", "CoreFoundation)"]:
            imports.add(fields[1])
    return imports


def spawn(udid, executable):
    result = subprocess.run(["xcrun", "simctl", "spawn", udid, str(executable)], capture_output=True, text=True)
    if result.stderr:
        sys.stderr.write(result.stderr)
    return result.returncode, result.stdout


def body(trace):
    return [line for line in trace.splitlines() if not line.startswith("#")]


def summary(trace):
    lines = body(trace)
    scenarios = sum(line.startswith("== ") for line in lines)
    return "%d scenarios, %d trace lines" % (scenarios, len(lines) - scenarios)


def first_difference(expected, actual):
    for index, (left, right) in enumerate(zip(expected, actual)):
        if left != right:
            return index, left, right
    if len(expected) != len(actual):
        index = min(len(expected), len(actual))
        return index, expected[index] if index < len(expected) else "<end>", actual[index] if index < len(actual) else "<end>"
    return None


def report(label, expected, actual):
    difference = first_difference(expected, actual)
    if difference is None:
        return True
    index, left, right = difference
    print("%s: trace differs at line %d:\n  expected: %s\n  actual:   %s" % (label, index + 1, left, right),
          file=sys.stderr)
    return False


def run_reference(executable, count):
    from simulator import session
    traces = []
    with session() as udid:
        for _ in range(count):
            status, trace = spawn(udid, executable)
            if status != 0:
                raise SystemExit(status)
            traces.append(trace)
    for index, trace in enumerate(traces[1:], 2):
        if not report("reference run %d against run 1" % index, body(traces[0]), body(trace)):
            raise SystemExit("The reference trace is not deterministic; fix the scenario, never the comparison.")
    return traces[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("action", choices=("record", "check", "compare"))
    parser.add_argument("--target", default="iphonesimulator")
    parser.add_argument("--match", default="iphonesimulator")
    parser.add_argument("--configuration", default="Debug")
    args = parser.parse_args()
    check_platform(args.target, args.match, args.configuration, "YES")
    directory = ROOT / ".build/Trace" / (args.target + "-" + args.match + "-" + args.configuration)
    recorded = recorded_path(args.match)
    reference = build_reference(directory / "Reference", args.configuration)
    if args.action == "record":
        trace = run_reference(reference, RUNS)
        recorded.parent.mkdir(parents=True, exist_ok=True)
        recorded.write_text(trace)
        print("Recorded %s, identical over %d reference runs: %s" % (
            summary(trace), RUNS, recorded.relative_to(ROOT)))
        return
    if not recorded.is_file():
        print("Setup failure: no recorded trace at " + str(recorded.relative_to(ROOT)), file=sys.stderr)
        raise SystemExit(77)
    expected = recorded.read_text()
    if args.action == "check":
        trace = run_reference(reference, 1)
        identity = [line for line in expected.splitlines() if line.startswith("# CoreFoundation ")]
        if identity != [line for line in trace.splitlines() if line.startswith("# CoreFoundation ")]:
            print("Setup failure: the recorded trace names another CoreFoundation image.", file=sys.stderr)
            raise SystemExit(77)
        if not report("reference against recorded", body(expected), body(trace)):
            raise SystemExit(1)
        print("Reference trace matches the recorded trace: %s." % summary(trace))
        return
    products = build_directory(args.target, args.match, args.configuration, "YES")
    framework = products / "OpenCoreFoundation.framework/OpenCoreFoundation"
    if not framework.is_file():
        print("Setup failure: build the restored frameworks first (make framework OF_SYSTEM_NAMES=YES).",
              file=sys.stderr)
        raise SystemExit(77)
    exported = {name[1:] if name.startswith("_") else name
                for name in output(["xcrun", "nm", "-gUj", framework]).splitlines()}
    needed = {name[1:] if name.startswith("_") else name for name in core_foundation_imports(reference)}
    missing = sorted(needed - exported)
    if missing:
        print("Setup failure: the restored CoreFoundation lacks %d definitions the scenarios use: %s" % (
            len(missing), ", ".join(missing)), file=sys.stderr)
        raise SystemExit(77)
    replacement = build_replacement(directory / "Replacement", products, args.configuration)
    from simulator import session
    with session() as udid:
        status, trace = spawn(udid, replacement)
    if status != 0:
        raise SystemExit(status)
    if not report("replacement against recorded", body(expected), body(trace)):
        raise SystemExit(1)
    print("Replacement trace matches the recorded reference trace: %s." % summary(trace))


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        raise SystemExit(error.returncode)

#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Create an isolated simulator per test session, pin its build, and delete it on exit."""

from contextlib import contextmanager
import hashlib
import json
import os
import subprocess
import sys
import uuid
from common import ROOT, output, run

RUNTIME = "com.apple.CoreSimulator.SimRuntime.iOS-27-0"
BUILD = "24A434"
DEVICE_TYPE = "com.apple.CoreSimulator.SimDeviceType.iPhone-17-Pro"


@contextmanager
def session():
    runtimes = json.loads(output(["xcrun", "simctl", "list", "runtimes", "--json"]))["runtimes"]
    runtime = next((item for item in runtimes if item["identifier"] == RUNTIME), None)
    if not runtime or runtime.get("buildversion") != BUILD or not runtime.get("isAvailable"):
        print("Setup failure: iOS 27.0 Simulator runtime (24A434) is required.", file=sys.stderr)
        raise SystemExit(77)
    checkout = hashlib.sha256(str(ROOT).encode()).hexdigest()[:8]
    name = "OpenFoundation-" + checkout + "-" + uuid.uuid4().hex[:8]
    udid = output(["xcrun", "simctl", "create", name, DEVICE_TYPE, RUNTIME])
    try:
        with open(os.devnull, "w") as null:
            run(["xcrun", "simctl", "bootstatus", udid, "-b"], stdout=null, timeout=240)
        yield udid
    finally:
        with open(os.devnull, "w") as null:
            subprocess.run(["xcrun", "simctl", "shutdown", udid], stdout=null, check=False)
        run(["xcrun", "simctl", "delete", udid])


def spawn(udid, executable, *arguments, capture=False):
    command = ["xcrun", "simctl", "spawn", udid, executable, *arguments]
    return output(command) if capture else run(command)

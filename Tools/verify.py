#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Verify the complete bootstrap matrix; retain logs outside version control."""

import sys
from common import ROOT, build_directory, run


def main():
    run(["make", "check"])
    logs = ROOT / "Local/Scratch/verification"
    logs.mkdir(parents=True, exist_ok=True)
    for configuration in ("Debug", "Release"):
        for mode in ("NO", "YES"):
            arguments = [sys.executable, ROOT / "Tools/build.py", "--configuration", configuration,
                         "--system-names", mode]
            label = configuration + "-" + mode
            print("Verifying Make " + label + "...", flush=True)
            with (logs / ("Make-" + label + ".log")).open("w") as log:
                run([*arguments, "test"], stdout=log, stderr=log)
                products = build_directory("iphonesimulator", "iphonesimulator", configuration, mode)
                run([*arguments, "--products", products, "validate"], stdout=log, stderr=log)
                if mode == "NO":
                    run([*arguments, "--products", products, "coexistence"], stdout=log, stderr=log)
            print("Verifying Xcode " + label + "...", flush=True)
            with (logs / ("Xcode-" + label + ".log")).open("w") as log:
                run([*arguments, "xcode"], stdout=log, stderr=log)
                products = ROOT / ".build/Xcode" / label / "Build/Products" / (configuration + "-iphonesimulator")
                run([*arguments, "--products", products, "test"], stdout=log, stderr=log)
                run([*arguments, "--products", products, "validate"], stdout=log, stderr=log)
                if mode == "NO":
                    run([*arguments, "--products", products, "coexistence"], stdout=log, stderr=log)
    print("Bootstrap matrix passed: Make/Xcode x Debug/Release x prefixed/system names.")
    print("Restored API coverage remains zero.")


if __name__ == "__main__":
    main()

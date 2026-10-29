#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Run the same platform, prefix, ledger and header policy in native Xcode targets."""

import os
from pathlib import Path
import sys
from common import check_platform, xcode_environment, run, ROOT
from build import prepare, swift_build_module


def main():
    target, match, configuration, mode = xcode_environment()
    check_platform(target, match, configuration, mode)
    if os.environ.get("ARCHS", "arm64") != "arm64":
        raise SystemExit("Only arm64 is supported.")
    directory = Path(os.environ["BUILT_PRODUCTS_DIR"])
    if sys.argv[1] == "prepare":
        prepare(directory, mode)
    elif sys.argv[1] == "swift":
        swift_build_module().compile_swift(os.environ["PRODUCT_NAME"], directory, configuration, mode)
    elif sys.argv[1] == "package":
        product = os.environ["PRODUCT_NAME"]
        run([sys.executable, ROOT / "Tools/prefix-mapping.py", "headers", directory / (product + ".framework"),
             "--product", product, "--system-names", mode])


if __name__ == "__main__":
    main()

#!/bin/bash
#
#  Created by ktiays on 2026/11/07.
#  Copyright (c) 2026 ktiays. All rights reserved.
#
# Runs check-declarations.py for the three checked units: CoreFoundation.h in C and in Objective-C, and
# Foundation.h in Objective-C. Run after `source <objc4>/build/wasm-toolchain/env.sh`.
#
#   Port/Headers/Tests/check-declarations.sh <build with compile_commands.json> <library directory> <scratch>

set -euo pipefail

tests="$(cd "$(dirname "$0")" && pwd)"
build="$1"
libraries="$2"
scratch="$3"
nm="$(dirname "$(which emcc)")/../bin/llvm-nm"
status=0
for check in "c CoreFoundation/CoreFoundation.h" "objective-c CoreFoundation/CoreFoundation.h" \
    "objective-c Foundation/Foundation.h"; do
    set -- ${check}
    python3 "${tests}/check-declarations.py" --compile-commands "${build}/compile_commands.json" \
        --language "$1" --umbrella "$2" --scratch "${scratch}" --nm "${nm}" \
        --archives "${libraries}/libOpenCoreFoundation.a" "${libraries}/libOpenFoundation.a" \
        "${libraries}/libPortSystem.a" || status=1
done
exit ${status}

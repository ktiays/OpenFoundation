#!/bin/bash
#
#  Created by ktiays on 2026/11/07.
#  Copyright (c) 2026 ktiays. All rights reserved.
#
# Compiles every header of Port/Headers on its own in C (gnu17), C++20, Objective-C and Objective-C++ with the flags
# the port's consumers use (OpenCoreAnimation's: wasm32, NSInteger long, BOOL bool, Apple's Objective-C ABI as
# objc4 builds it). Run after `source <objc4>/build/wasm-toolchain/env.sh`.
#
#   Port/Headers/Tests/compile-headers.sh [scratch directory]

set -euo pipefail

headers="$(cd "$(dirname "$0")/.." && pwd)"
port="$(cd "${headers}/.." && pwd)"
root="$(cd "${port}/.." && pwd)"
objc4="${OBJC4_ROOT:-${root}/../objc4}"
if [[ ! -d "${objc4}/platform/wasm" ]]; then
    objc4="${root}/../../../objc4"
fi
objc4Configuration="${OBJC4_CONFIGURATION:-Release}"
# libmach supplies <mach/…> (LIBMACH_ROOT, default: the sibling ../libmach), its headers in the order its
# CMakeLists.txt gives them.
libmach="${LIBMACH_ROOT:-${root}/../libmach}"
if [[ ! -f "${libmach}/CMakeLists.txt" ]]; then
    libmach="${root}/../../../libmach"
fi
# The libdispatch headers Port/libdispatch/build.sh installed (Release or Debug).
libdispatch="${root}/.build/wasm/Release/libdispatch/include"
if [[ ! -d "${libdispatch}" ]]; then
    libdispatch="${root}/.build/wasm/Debug/libdispatch/include"
fi
scratch="${1:-$(mktemp -d)}"
mkdir -p "${scratch}"

common=(
    -target wasm32-unknown-emscripten -fsyntax-only -pthread -fblocks -fwasm-exceptions
    -Wall -Wextra -Werror -Wno-unused-function -Wstrict-prototypes
    -DNS_BUILD_32_LIKE_64=1 -DOS_OBJECT_USE_OBJC=0 -U__OBJC_BOOL_IS_BOOL -D__OBJC_BOOL_IS_BOOL=1
    -I"${headers}"
    -I"${port}/System/include"
    -isystem "${libdispatch}"
    -isystem "${objc4}/build/wasm-threads-${objc4Configuration}/include"
    -isystem "${objc4}/platform/wasm/include"
    -isystem "${objc4}/platform/android/include"
    -isystem "${objc4}/third-party/BlocksRuntime"
    -isystem "${libmach}/Source/Compatibility/Emscripten"
    -isystem "${libmach}/Source/Apple/include"
    -isystem "${libmach}/Source/Compatibility/Darwin"
    -include "${objc4}/platform/wasm/include/objc-wasm-compiler.h"
    -include "${port}/System/include/PortDarwin.h"
)
objc=(-fobjc-runtime=macosx-26.0 -fno-objc-convert-messages-to-runtime-calls -fno-objc-arc -fobjc-weak -fobjc-exceptions)

failures=0
for header in "${headers}"/CoreFoundation/*.h "${headers}"/Foundation/*.h; do
    name="$(basename "$(dirname "${header}")")/$(basename "${header}")"
    for language in c c++ objective-c objective-c++; do
        case "${language}" in
            c) flags=(-x c -std=gnu17) ;;
            c++) flags=(-x c++ -std=c++20) ;;
            objective-c) flags=(-x objective-c -std=gnu17 "${objc[@]}") ;;
            objective-c++) flags=(-x objective-c++ -std=c++20 "${objc[@]}") ;;
        esac
        # Foundation is an Objective-C interface; its plain C and C++ form is what the SDK's headers give
        # there (the C declarations only).
        source="${scratch}/check.${language//[^a-z]/}"
        printf '#include <%s>\n' "${name}" > "${source}"
        if ! em++ "${common[@]}" "${flags[@]}" "${source}" > "${scratch}/output.txt" 2>&1; then
            echo "FAILED ${name} (${language})"
            sed -n '1,20p' "${scratch}/output.txt"
            failures=$((failures + 1))
        fi
    done
done

if [[ ${failures} -ne 0 ]]; then
    echo "${failures} header compilations failed"
    exit 1
fi
echo "All headers compile on their own in C (gnu17), C++20, Objective-C and Objective-C++"

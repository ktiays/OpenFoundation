#!/bin/bash
#
#  Created by ktiays on 2026/11/07.
#  Copyright (c) 2026 ktiays. All rights reserved.
#
# The run-time check of Port/Headers/Foundation: generates MethodCheck.m from the headers' AST
# (generate-method-check.py), compiles it as a consumer would, links it as Port/Tests/Smoke.m is linked (the
# whole libdispatch, libobjc4, OpenCoreFoundation, OpenFoundation and PortSystem archives, then libmach, the main
# module with objc4's adapter support) and runs it under Node. Run after `source <objc4>/build/wasm-toolchain/env.sh`.
#
#   Port/Headers/Tests/run-method-check.sh <directory with libOpenCoreFoundation.a> [scratch directory]

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
libraries="$(cd "$1" && pwd)"
scratch="${2:-$(mktemp -d)}"
mkdir -p "${scratch}"
configuration="${LIBDISPATCH_CONFIGURATION:-Debug}"
libdispatch="${root}/.build/wasm/${configuration}/libdispatch"

flags=(
    -x objective-c -std=gnu17 -pthread -fblocks -fwasm-exceptions -mtail-call -fPIC
    -fobjc-runtime=macosx-26.0 -fno-objc-convert-messages-to-runtime-calls -fno-objc-arc -fobjc-weak -fobjc-exceptions
    -DNS_BUILD_32_LIKE_64=1 -DOS_OBJECT_USE_OBJC=0 -U__OBJC_BOOL_IS_BOOL -D__OBJC_BOOL_IS_BOOL=1
    -I"${headers}"
    -I"${port}/System/include"
    -isystem "${libdispatch}/include"
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

printf '#import <Foundation/Foundation.h>\n' > "${scratch}/FoundationUmbrella.m"
emcc "${flags[@]}" -fsyntax-only -Xclang -ast-dump=json "${scratch}/FoundationUmbrella.m" > "${scratch}/Foundation.json"
python3 "${headers}/Tests/generate-method-check.py" --ast "${scratch}/Foundation.json" --output "${scratch}/MethodCheck.m"

emcc "${flags[@]}" -Wno-objc-protocol-method-implementation -c "${scratch}/MethodCheck.m" -o "${scratch}/MethodCheck.o"
em++ -o "${scratch}/MethodCheck.js" "${scratch}/MethodCheck.o" \
    -pthread -mtail-call -fwasm-exceptions -sUSE_ICU=1 \
    -sMAIN_MODULE=2 -sALLOW_TABLE_GROWTH=1 -sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=2GB \
    -sEXIT_RUNTIME=1 -sPTHREAD_POOL_SIZE=8 --profiling-funcs \
    -Wl,--undefined=objc_wasm_init -Wl,--export=objc_wasm_forward_invoke -Wl,--export=__heap_base \
    "--js-library=${objc4}/platform/wasm/library-objc4.js" \
    -sASSERTIONS=0 -sEXCEPTION_STACK_TRACES=0 \
    -Wl,--whole-archive "${libdispatch}/src/libdispatch.a" "${objc4}/build/wasm-threads-${objc4Configuration}/libobjc4.a" \
    "${libraries}/libOpenCoreFoundation.a" "${libraries}/libOpenFoundation.a" "${libraries}/libPortSystem.a" \
    -Wl,--no-whole-archive "${libraries}/libmach/libmach.a"

node "${scratch}/MethodCheck.js"

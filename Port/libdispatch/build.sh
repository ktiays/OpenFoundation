#!/bin/bash
#
#  Created by ktiays on 2026/11/07.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

# Builds the supplied libdispatch for Emscripten wasm32 (pthreads): Apple's libdispatch release, pinned by tag
# and commit, with the patch series in this directory. The source is cloned into Local/Vendor (unversioned);
# products go to .build/wasm/<configuration>/libdispatch.
#
#   Port/libdispatch/build.sh [Debug|Release] [address]
#
# A second argument "address" builds with AddressSanitizer into .build/wasm/<configuration>-ASan/libdispatch.
# OBJC4_ROOT names the objc4 checkout whose Emscripten toolchain compiles everything in the process (default:
# the sibling ../objc4); LIBMACH_ROOT the libmach checkout whose headers src/mach_emscripten.c includes
# (default: the sibling ../libmach).

set -euo pipefail

configuration="${1:-Release}"
sanitizer="${2:-}"
root="$(cd "$(dirname "$0")/../.." && pwd)"
here="$root/Port/libdispatch"
objc4="${OBJC4_ROOT:-$(cd "$root/.." && pwd)/objc4}"
if [[ ! -f "$objc4/build/wasm-toolchain/env.sh" ]]; then
    objc4="$(cd "$root/../../.." 2>/dev/null && pwd)/objc4"
fi
if [[ ! -f "$objc4/build/wasm-toolchain/env.sh" ]]; then
    echo "error: no objc4 Emscripten toolchain; set OBJC4_ROOT and run objc4's scripts/prepare-wasm-toolchain.py" >&2
    exit 77
fi

url="https://github.com/apple-oss-distributions/libdispatch.git"
tag="libdispatch-1542.100.32"
commit="701f4d1a24ae9c6863901bbbb22624b7d1b87321"
source="$root/Local/Vendor/libdispatch"
suffix=""
if [[ "$sanitizer" == "address" ]]; then
    suffix="-ASan"
elif [[ -n "$sanitizer" ]]; then
    echo "error: unknown sanitizer $sanitizer" >&2
    exit 1
fi
build="$root/.build/wasm/$configuration$suffix/libdispatch"
mkdir -p "$(dirname "$build")"

if [[ ! -d "$source/.git" ]]; then
    mkdir -p "$(dirname "$source")"
    git clone --quiet "$url" "$source"
fi
git -C "$source" fetch --quiet --tags origin "refs/tags/$tag:refs/tags/$tag" 2>/dev/null || true
if [[ "$(git -C "$source" rev-parse "$tag^{commit}")" != "$commit" ]]; then
    echo "error: $tag does not name the pinned commit $commit" >&2
    exit 1
fi

# Rebuild the patched tree from the pinned commit every time, so the series is the only input.
git -C "$source" checkout --quiet --detach "$commit"
git -C "$source" reset --quiet --hard "$commit"
git -C "$source" clean --quiet -fdx
while read -r patch; do
    [[ -z "$patch" ]] && continue
    git -C "$source" apply --whitespace=nowarn "$here/patches/$patch"
done < "$here/patches/series"

# shellcheck disable=SC1091
source "$objc4/build/wasm-toolchain/env.sh"
flags="-pthread -fPIC -fblocks -mtail-call -fwasm-exceptions -DOS_OBJECT_USE_OBJC=0 -include $objc4/platform/wasm/include/sys/cdefs.h"
if [[ "$sanitizer" == "address" ]]; then
    flags="$flags -fsanitize=address"
fi
# libdispatch's use of Mach (patch 0004, src/mach_emscripten.c) compiles against libmach's headers
# (LIBMACH_ROOT, default: the sibling ../libmach) as OpenFoundation's units see them: objc4's headers and
# Port/System's Darwin additions first, then libmach's in the order its CMakeLists.txt gives them (its
# Emscripten overlay, Apple's headers, the Darwin headers musl lacks, <libmach/…>). No other file of
# libdispatch sees them, so HAVE_MACH stays unset.
libmach="${LIBMACH_ROOT:-$(cd "$root/.." && pwd)/libmach}"
if [[ ! -f "$libmach/CMakeLists.txt" ]]; then
    libmach="$(cd "$root/../../.." 2>/dev/null && pwd)/libmach"
fi
if [[ ! -f "$libmach/CMakeLists.txt" ]]; then
    echo "error: no libmach checkout; set LIBMACH_ROOT" >&2
    exit 1
fi
machIncludes="$objc4/platform/wasm/include;$objc4/platform/android/include;$root/Port/System/include"
machIncludes="$machIncludes;$libmach/Source/Compatibility/Emscripten"
machIncludes="$machIncludes;$libmach/Source/Apple/include;$libmach/Source/Compatibility/Darwin;$libmach/Source"
emcmake cmake -S "$source" -B "$build" -G Ninja \
    -DCMAKE_BUILD_TYPE="$configuration" \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_TESTING=OFF \
    -DENABLE_SWIFT=OFF \
    "-DDISPATCH_EMSCRIPTEN_MACH_INCLUDE_DIR=$machIncludes" \
    -DCMAKE_C_FLAGS="$flags" \
    -DCMAKE_CXX_FLAGS="$flags" > "$build.configure.log" 2>&1 || {
    tail -30 "$build.configure.log" >&2
    exit 1
}
ninja -C "$build" dispatch > "$build.build.log" 2>&1 || {
    grep -E "error:|FAILED" "$build.build.log" | head -40 >&2
    exit 1
}

# Headers as consumers include them: <dispatch/dispatch.h>, <os/object.h> and the private SPI.
include="$build/include"
rm -rf "$include"
mkdir -p "$include"
cp -R "$source/dispatch" "$source/os" "$source/private" "$include/"
echo "libdispatch: $build/src/libdispatch.a"
echo "headers:     $include"

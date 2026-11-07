#
#  Created by ktiays on 2026/11/07.
#  Copyright (c) 2026 ktiays. All rights reserved.
#
# Builds OpenCoreFoundation and OpenFoundation for WebAssembly (TARGET=wasm, wasm32, Emscripten with pthreads):
# the iOS Simulator restoration (MATCH=iphonesimulator) less the sources Port/Trim.tsv leaves out, plus the
# port's own libraries in Port/. Apple builds do not use this file.
#
#   Port/libdispatch/build.sh Release
#   source <objc4>/build/wasm-toolchain/env.sh
#   emcmake cmake -S Port -B .build/wasm/Release/OpenFoundation -G Ninja -DCMAKE_BUILD_TYPE=Release
#   cmake --build .build/wasm/Release/OpenFoundation

cmake_minimum_required(VERSION 3.28)

project(OpenFoundationWasm LANGUAGES C CXX)

# Emscripten's toolchain file names no Objective-C compiler; emcc compiles Objective-C with the objc4 patch.
set(CMAKE_OBJC_COMPILER "${CMAKE_C_COMPILER}")
enable_language(OBJC)
# CMake records the libraries its probe link of emcc used (a non-threaded libc among them); emcc chooses the
# system libraries itself, as objc4_add_wasm_module also arranges.
set(CMAKE_OBJC_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_OBJC_IMPLICIT_LINK_DIRECTORIES "")

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "Port/CMakeLists.txt builds for WebAssembly only: configure with emcmake")
endif()
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Debug or Release" FORCE)
endif()

get_filename_component(root "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# The naming mode (AGENTS.md, "Naming and Coexistence"). A wasm process has no system Foundation, so the
# default is the native spelling.
option(OF_SYSTEM_NAMES "Spell declared names as the native CF/NS names" ON)

# AddressSanitizer for the restored and port code (objc4 and libdispatch stay uninstrumented; ASan still
# checks their heap through its allocator).
option(OF_SANITIZE_ADDRESS "Build with AddressSanitizer" OFF)

# main runs on the browser's thread by default, which CoreFoundation and libdispatch record as the main thread
# (constructors run there); ON moves main to a pthread (Emscripten's PROXY_TO_PTHREAD), where its run loop is
# not the main run loop.
option(OF_PROXY_TO_PTHREAD "Run main on a pthread" OFF)
if(OF_SYSTEM_NAMES)
    set(systemNames 1)
else()
    set(systemNames 0)
endif()

# The objc4 checkout whose Emscripten toolchain and runtime the whole process uses: the sibling ../objc4 of
# the repository, or of the main checkout when building from a worktree under .worktrees/.
set(objc4Default "${root}/../objc4")
if(NOT EXISTS "${objc4Default}/build/wasm-toolchain/env.sh" AND EXISTS "${root}/../../../objc4/build/wasm-toolchain/env.sh")
    set(objc4Default "${root}/../../../objc4")
endif()
set(OBJC4_ROOT "${objc4Default}" CACHE PATH "The objc4 checkout (Emscripten port)")
get_filename_component(OBJC4_ROOT "${OBJC4_ROOT}" ABSOLUTE)
# The objc4 configuration linked; Debug OpenFoundation may link objc4's Release runtime.
set(OBJC4_CONFIGURATION "${CMAKE_BUILD_TYPE}" CACHE STRING "The objc4 build configuration to link (Debug or Release)")
set(objc4Build "${OBJC4_ROOT}/build/wasm-threads-${OBJC4_CONFIGURATION}")
if(NOT EXISTS "${objc4Build}/libobjc4.a")
    message(FATAL_ERROR "${objc4Build}/libobjc4.a is missing: run ${OBJC4_ROOT}/scripts/build-wasm.sh ${CMAKE_BUILD_TYPE} threads")
endif()

# The supplied libdispatch (Port/libdispatch/build.sh).
if(OF_SANITIZE_ADDRESS)
    set(libdispatchBuild "${root}/.build/wasm/${CMAKE_BUILD_TYPE}-ASan/libdispatch")
    set(libdispatchArguments "${CMAKE_BUILD_TYPE} address")
else()
    set(libdispatchBuild "${root}/.build/wasm/${CMAKE_BUILD_TYPE}/libdispatch")
    set(libdispatchArguments "${CMAKE_BUILD_TYPE}")
endif()
if(NOT EXISTS "${libdispatchBuild}/src/libdispatch.a")
    message(FATAL_ERROR "${libdispatchBuild}/src/libdispatch.a is missing: run Port/libdispatch/build.sh ${libdispatchArguments}")
endif()

# libmach: Apple's Mach interfaces for the whole process (ports and rights, mach_msg with descriptors,
# notifications, port sets, the MIG runtime, VM calls and memory entries, mk_timer, the Mach clocks, the task
# and thread calls and the bootstrap name service), built for wasm32 by its own CMakeLists.txt. The sibling
# ../libmach of the repository, or of the main checkout when building from a worktree under .worktrees/.
set(libmachDefault "${root}/../libmach")
if(NOT EXISTS "${libmachDefault}/CMakeLists.txt" AND EXISTS "${root}/../../../libmach/CMakeLists.txt")
    set(libmachDefault "${root}/../../../libmach")
endif()
set(LIBMACH_ROOT "${libmachDefault}" CACHE PATH "The libmach checkout")
get_filename_component(LIBMACH_ROOT "${LIBMACH_ROOT}" ABSOLUTE)
if(NOT EXISTS "${LIBMACH_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "${LIBMACH_ROOT}/CMakeLists.txt is missing: set LIBMACH_ROOT to a libmach checkout")
endif()
if(NOT TARGET libmach::mach)
    add_subdirectory("${LIBMACH_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/libmach" EXCLUDE_FROM_ALL)
endif()

# <OpenCoreFoundation/...> and <OpenFoundation/...> name the source tree directly.
set(frameworkIncludes "${CMAKE_CURRENT_BINARY_DIR}/FrameworkIncludes")
file(MAKE_DIRECTORY "${frameworkIncludes}")
file(CREATE_LINK "${root}/Source/CoreFoundation" "${frameworkIncludes}/OpenCoreFoundation" SYMBOLIC)
file(CREATE_LINK "${root}/Source/Foundation" "${frameworkIncludes}/OpenFoundation" SYMBOLIC)

# The declaration-derived name mapping every translation unit force-includes.
find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
set(prefixMapping "${CMAKE_CURRENT_BINARY_DIR}/Generated/OFPrefixMapping.h")
if(NOT EXISTS "${prefixMapping}")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${root}/Tools/prefix-mapping.py" generate "${prefixMapping}"
        WORKING_DIRECTORY "${root}"
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

# The restored sources, less the trimmed ones.
file(GLOB_RECURSE coreFoundationSources CONFIGURE_DEPENDS
    "${root}/Source/CoreFoundation/*.c" "${root}/Source/CoreFoundation/*.m")
file(GLOB_RECURSE foundationSources CONFIGURE_DEPENDS
    "${root}/Source/Foundation/*.c" "${root}/Source/Foundation/*.m")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/Trim.tsv" trimRows REGEX "^[^#]")
list(REMOVE_AT trimRows 0)
foreach(row IN LISTS trimRows)
    string(REGEX REPLACE "\t.*" "" pattern "${row}")
    foreach(list IN ITEMS coreFoundationSources foundationSources)
        set(kept)
        foreach(path IN LISTS ${list})
            file(RELATIVE_PATH relative "${root}" "${path}")
            if(NOT relative MATCHES "${pattern}")
                list(APPEND kept "${path}")
            endif()
        endforeach()
        set(${list} ${kept})
    endforeach()
endforeach()

file(GLOB_RECURSE portSystemSources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/System/Source/*.c")
file(GLOB_RECURSE portCoreFoundationSources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/CoreFoundation/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/CoreFoundation/*.m")
file(GLOB_RECURSE portFoundationSources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Foundation/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/Foundation/*.m")

# Compiler settings shared with the Apple builds (Configuration/Build.xcconfig), and the objc4 ABI contract.
set(commonOptions
    -fblocks -mtail-call -fwasm-exceptions -pthread -fPIC -fvisibility=hidden -ffp-contract=off
    # Dispatch objects are C objects: libdispatch is built without Objective-C, as objc4 expects.
    -DOS_OBJECT_USE_OBJC=0
    "SHELL:-include ${OBJC4_ROOT}/platform/wasm/include/objc-wasm-compiler.h"
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/System/include/PortDarwin.h"
    -sUSE_ICU=1
    # The restored sources declare the ICU functions they call; ICU's renaming maps them to the versioned
    # entry points of Emscripten's ICU port.
    "SHELL:-include unicode/urename.h"
    "SHELL:-include ${prefixMapping}"
    -DOF_SYSTEM_NAMES=${systemNames}
    -DOF_PORT=1
    # NSInteger is long, as on arm64 and in the restored declarations (Apple's switch for 32-bit targets).
    -DNS_BUILD_32_LIKE_64=1
    # BOOL is bool, as on arm64 iOS (objc4 honors the compiler's __OBJC_BOOL_IS_BOOL).
    -U__OBJC_BOOL_IS_BOOL
    -D__OBJC_BOOL_IS_BOOL=1
    -Wno-unreachable-code
    # Native declarations write `typedef enum E : T E;`, which Apple's clang accepts in C.
    -Wno-elaborated-enum-base
    -mnontrapping-fptoint
)
set(objcOptions -fobjc-runtime=macosx-26.0 -fno-objc-convert-messages-to-runtime-calls -fno-objc-arc -fobjc-weak)
set(includeDirectories
    "${CMAKE_CURRENT_SOURCE_DIR}/System/include"
    "${root}/Source"
    "${frameworkIncludes}"
    "${CMAKE_CURRENT_BINARY_DIR}/Generated"
    "${libdispatchBuild}/include"
    "${objc4Build}/include"
    "${OBJC4_ROOT}/platform/wasm/include"
    "${OBJC4_ROOT}/platform/android/include"
    "${OBJC4_ROOT}/third-party/BlocksRuntime"
)

if(OF_SANITIZE_ADDRESS)
    list(APPEND commonOptions -fsanitize=address)
    # objc4 walks the linker-collected Objective-C metadata arrays; redzones between their entries would
    # break the walk, so Objective-C globals are not instrumented.
    list(APPEND objcOptions "SHELL:-mllvm -asan-globals=0")
endif()

# Every unit sees libmach's headers (<mach/…>, <servers/bootstrap.h>, <libmach/…>) after the port's own.
function(configureTarget target)
    target_include_directories(${target} PRIVATE ${includeDirectories})
    target_link_libraries(${target} PRIVATE libmach::mach)
    target_compile_options(${target} PRIVATE ${commonOptions} "$<$<COMPILE_LANGUAGE:OBJC>:${objcOptions}>")
    set_target_properties(${target} PROPERTIES C_STANDARD 11 C_EXTENSIONS ON OBJC_STANDARD 11)
endfunction()

add_library(PortSystem STATIC ${portSystemSources})
configureTarget(PortSystem)
# Consumers of PortSystem (OpenCoreAnimation's Port::System) get libmach's headers and library through it.
target_link_libraries(PortSystem PUBLIC libmach::mach)

add_library(OpenCoreFoundation STATIC ${coreFoundationSources} ${portCoreFoundationSources})
configureTarget(OpenCoreFoundation)

add_library(OpenFoundation STATIC ${foundationSources} ${portFoundationSources})
configureTarget(OpenFoundation)

list(LENGTH coreFoundationSources coreFoundationCount)
list(LENGTH foundationSources foundationCount)
message(STATUS "OpenCoreFoundation: ${coreFoundationCount} restored sources; OpenFoundation: ${foundationCount}")

# The smoke test (Port/Tests/Smoke.m), linked as objc4 links an Objective-C program: the main module with the
# runtime's adapter support, the whole restored libraries (their classes and categories are reached by name),
# ICU, and the program's main on a pthread so that run loops may block.
add_executable(Smoke "${CMAKE_CURRENT_SOURCE_DIR}/Tests/Smoke.m")
configureTarget(Smoke)
set_target_properties(Smoke PROPERTIES SUFFIX ".js" LINKER_LANGUAGE CXX)
target_link_options(Smoke PRIVATE
    -pthread -mtail-call -fwasm-exceptions -sUSE_ICU=1
    -sMAIN_MODULE=2 -sALLOW_TABLE_GROWTH=1 -sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=2GB
    "$<$<BOOL:${OF_PROXY_TO_PTHREAD}>:-sPROXY_TO_PTHREAD=1>" -sEXIT_RUNTIME=1 -sPTHREAD_POOL_SIZE=8 --profiling-funcs
    "LINKER:--undefined=objc_wasm_init"
    "LINKER:--export=objc_wasm_forward_invoke"
    "LINKER:--export=__heap_base"
    "--js-library=${OBJC4_ROOT}/platform/wasm/library-objc4.js"
    # Emscripten's exception stack traces (implied by its -O0 assertions) cannot read objc4's
    # Objective-C exception type information.
    -sASSERTIONS=0 -sEXCEPTION_STACK_TRACES=0
    "$<$<BOOL:${OF_SANITIZE_ADDRESS}>:-fsanitize=address>"
)
# Constructors of one priority run in link order, so the libraries link whole in Darwin's initialization
# order: libdispatch (initialized by libSystem before any other library; CoreFoundation's initializer already
# uses it, and before libdispatch_init its per-thread key is unset, so its first TSD write lands in whatever key
# 0 is), then libobjc, which dyld initializes before CoreFoundation and Foundation.
target_link_libraries(Smoke PRIVATE
    "-Wl,--whole-archive" "${libdispatchBuild}/src/libdispatch.a" "${objc4Build}/libobjc4.a"
    OpenCoreFoundation OpenFoundation PortSystem
    "-Wl,--no-whole-archive"
)

# The test of libdispatch's Mach sources and channels on libmach (Port/Tests/DispatchMach.c), run under Node
# (`node DispatchMachTest.js`). Every unit of the port is compiled with objc4's compiler header, so the program
# links the runtime as Smoke does.
add_executable(DispatchMachTest EXCLUDE_FROM_ALL "${CMAKE_CURRENT_SOURCE_DIR}/Tests/DispatchMach.c")
configureTarget(DispatchMachTest)
set_target_properties(DispatchMachTest PROPERTIES SUFFIX ".js" LINKER_LANGUAGE CXX)
target_link_options(DispatchMachTest PRIVATE
    -pthread -mtail-call -fwasm-exceptions
    -sMAIN_MODULE=2 -sALLOW_TABLE_GROWTH=1 -sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=2GB
    -sEXIT_RUNTIME=1 -sPTHREAD_POOL_SIZE=8
    "LINKER:--undefined=objc_wasm_init"
    "LINKER:--export=objc_wasm_forward_invoke"
    "LINKER:--export=__heap_base"
    "--js-library=${OBJC4_ROOT}/platform/wasm/library-objc4.js"
    -sASSERTIONS=0 -sEXCEPTION_STACK_TRACES=0
    "$<$<BOOL:${OF_SANITIZE_ADDRESS}>:-fsanitize=address>"
)
target_link_libraries(DispatchMachTest PRIVATE
    "-Wl,--whole-archive" "${libdispatchBuild}/src/libdispatch.a" "${objc4Build}/libobjc4.a" "-Wl,--no-whole-archive"
    PortSystem
)

#!/usr/bin/env python3
#
#  Created by ktiays on 2026/10/29.
#  Copyright (c) 2026 ktiays. All rights reserved.
#

"""Generate the native, filesystem-synchronized Xcode project deterministically."""

import argparse
import hashlib
import json
import re
from common import ROOT, SOURCE_SUFFIXES, internal_test_sources, write_changed


def ident(name):
    return hashlib.sha256(name.encode()).hexdigest()[:24].upper()


def openstep(value, depth=0):
    indent = "    " * depth
    child = "    " * (depth + 1)
    if isinstance(value, dict):
        if not value:
            return "{}"
        return "{\n" + "\n".join(child + openstep(key) + " = " + openstep(item, depth + 1) + ";"
                                  for key, item in value.items()) + "\n" + indent + "}"
    if isinstance(value, list):
        return "(\n" + "\n".join(child + openstep(item, depth + 1) + "," for item in value) + "\n" + indent + ")" if value else "()"
    value = str(value)
    return value if re.fullmatch(r"[A-Za-z0-9_./]+", value) else json.dumps(value)


def generate():
    objects = {}

    def add(label, isa, **body):
        key = ident(label)
        objects[key] = {"isa": isa, **body}
        return key

    config_file = add("shared-config", "PBXFileReference", path="Configuration/Build.xcconfig",
                      lastKnownFileType="text.xcconfig", sourceTree="SOURCE_ROOT")

    def configs(name, values):
        configurations = []
        for configuration in ("Debug", "Release"):
            settings = dict(values)
            settings.update({"GCC_OPTIMIZATION_LEVEL": "0" if configuration == "Debug" else "2",
                             "LLVM_LTO": "NO" if configuration == "Debug" else "YES"})
            configurations.append(add(name + configuration, "XCBuildConfiguration", name=configuration,
                                      baseConfigurationReference=config_file, buildSettings=settings))
        return add(name + "configs", "XCConfigurationList", buildConfigurations=configurations,
                   defaultConfigurationIsVisible="0", defaultConfigurationName="Release")

    def folder(name, path):
        return add(name, "PBXFileSystemSynchronizedRootGroup", path=path, sourceTree="<group>")

    core_source = folder("core-source", "Source/CoreFoundation")
    foundation_source = folder("foundation-source", "Source/Foundation")
    source = add("source", "PBXGroup", name="Source", children=[core_source, foundation_source], sourceTree="<group>")
    tests = folder("tests", "Tests")
    standalone = folder("standalone", "StandaloneTests")
    other = [folder(name, name) for name in ("Configuration", "Documentation", "Tools", "Resources")]
    root_files = [add(name, "PBXFileReference", path=name, sourceTree="<group>", lastKnownFileType=kind)
                  for name, kind in (("README.md", "net.daringfireball.markdown"),
                                     ("AGENTS.md", "net.daringfireball.markdown"), ("Makefile", "sourcecode.make"),
                                     (".clang-format", "text"), (".swift-format", "text"))]
    product_references = {}
    for name in ("OpenCoreFoundation", "OpenFoundation", "StandaloneTests", "DifferentialTests"):
        framework = name.startswith("Open")
        product_references[name] = add(name + "-product", "PBXFileReference", includeInIndex="0",
                                       path=name + ".framework" if framework else name,
                                       explicitFileType="wrapper.framework" if framework else "compiled.mach-o.executable",
                                       sourceTree="BUILT_PRODUCTS_DIR")
    products = add("products", "PBXGroup", name="Products", children=list(product_references.values()), sourceTree="<group>")
    root = add("root", "PBXGroup", children=[source, tests, standalone, *other, *root_files, products], sourceTree="<group>")

    def phase(label, kind, files=None, **body):
        return add(label, kind, buildActionMask="2147483647", files=files or [],
                   runOnlyForDeploymentPostprocessing="0", **body)

    def script(name, action):
        return phase(name, "PBXShellScriptBuildPhase", name=action.title() + " Framework Policy", alwaysOutOfDate="1",
                     inputPaths=[], outputPaths=(["$(BUILT_PRODUCTS_DIR)/Objects/Swift/$(PRODUCT_NAME).o"]
                                                if action == "swift" else []), shellPath="/bin/sh",
                     shellScript='set -eu\n/usr/bin/python3 "$SRCROOT/Tools/xcode-phase.py" ' + action + "\n")

    def dependency(name, target):
        proxy = add(name + "proxy", "PBXContainerItemProxy", containerPortal=ident("project"), proxyType="1",
                    remoteGlobalIDString=ident(target), remoteInfo=target)
        return add(name + "dependency", "PBXTargetDependency", target=ident(target), targetProxy=proxy)

    shared = {
        "OF_SYSTEM_NAMES_VALUE_NO": "0", "OF_SYSTEM_NAMES_VALUE_YES": "1",
        "GCC_PREPROCESSOR_DEFINITIONS": "$(inherited) OF_SYSTEM_NAMES=$(OF_SYSTEM_NAMES_VALUE_$(OF_SYSTEM_NAMES))",
        "HEADER_SEARCH_PATHS": '"$(SRCROOT)/Source" "$(SRCROOT)/Tests" "$(SRCROOT)/StandaloneTests" "$(BUILT_PRODUCTS_DIR)/Generated"',
        "FRAMEWORK_SEARCH_PATHS": '"$(BUILT_PRODUCTS_DIR)"',
        "OTHER_CFLAGS": '$(inherited) -fblocks -ffp-contract=off -Wall -Wextra -Werror -include "$(BUILT_PRODUCTS_DIR)/Generated/OFPrefixMapping.h"',
        "LD_RUNPATH_SEARCH_PATHS": "@executable_path @loader_path/..",
    }
    target_ids = []
    for name, group in (("OpenCoreFoundation", core_source), ("OpenFoundation", foundation_source),
                         ("StandaloneTests", standalone), ("DifferentialTests", tests)):
        framework = name.startswith("Open")
        source_module = "CoreFoundation" if name == "OpenCoreFoundation" else "Foundation"
        swift_sources = sorted((ROOT / "Source" / source_module).rglob("*.swift")) if framework else []
        settings = {"PRODUCT_NAME": name, **shared}
        phases = [script(name + "-prepare", "prepare")]
        if framework:
            public = json.loads((ROOT / "Configuration/PublicHeaders.json").read_text())[name]
            exception = add(name + "-headers", "PBXFileSystemSynchronizedBuildFileExceptionSet",
                            target=ident(name), publicHeaders=public,
                            membershipExceptions=[str(path.relative_to(ROOT / "Source" / source_module))
                                                  for path in swift_sources])
            objects[group]["exceptions"] = [exception]
            phases.append(phase(name + "-header-phase", "PBXHeadersBuildPhase"))
            settings.update({"PRODUCT_BUNDLE_IDENTIFIER": "me.ktiays." + name,
                             "INFOPLIST_FILE": "Resources/" + name + "-Info.plist",
                             "DYLIB_INSTALL_NAME_BASE": "@rpath", "DEFINES_MODULE": "YES",
                             "MODULEMAP_FILE": "Resources/" + name + ".modulemap"})
        phases.append(phase(name + "-sources", "PBXSourcesBuildPhase"))
        if swift_sources:
            settings["OTHER_LDFLAGS"] = ('$(inherited) "$(BUILT_PRODUCTS_DIR)/Objects/Swift/$(PRODUCT_NAME).o" '
                                         '-L"$(SDKROOT)/usr/lib/swift" '
                                         '-L"$(DEVELOPER_DIR)/Toolchains/XcodeDefault.xctoolchain/usr/lib/swift/$(PLATFORM_NAME)" '
                                         '-Wl,-rpath,/usr/lib/swift')
        dependencies = []
        links = []
        if name != "OpenCoreFoundation":
            dependencies.append(dependency(name + "-core", "OpenCoreFoundation"))
            links.append("OpenCoreFoundation")
        if not framework:
            dependencies.append(dependency(name + "-foundation", "OpenFoundation"))
            links.append("OpenFoundation")
        build_files = [add(name + "-link-" + library, "PBXBuildFile", fileRef=product_references[library]) for library in links]
        phases.append(phase(name + "-links", "PBXFrameworksBuildPhase", build_files))
        if not framework:
            settings["OTHER_LDFLAGS"] = "-Wl,-needed_framework,OpenFoundation -Wl,-needed_framework,OpenCoreFoundation"
        else:
            phases.append(script(name + "-package", "package"))
        synchronized_groups = [group]
        if not framework:
            selected = set(internal_test_sources())
            for source_group, module in ((core_source, "CoreFoundation"), (foundation_source, "Foundation")):
                source_root = ROOT / "Source" / module
                if not any(source_root in path.parents for path in selected):
                    continue
                excluded = sorted(str(path.relative_to(source_root)) for path in source_root.rglob("*")
                                  if path.suffix in SOURCE_SUFFIXES and path not in selected)
                exception = add(name + "-internal-" + module, "PBXFileSystemSynchronizedBuildFileExceptionSet",
                                target=ident(name), membershipExceptions=excluded)
                objects[source_group].setdefault("exceptions", []).append(exception)
                synchronized_groups.append(source_group)
        target_ids.append(add(name, "PBXNativeTarget", name=name, productName=name, productReference=product_references[name],
                              productType="com.apple.product-type.framework" if framework else "com.apple.product-type.tool",
                              fileSystemSynchronizedGroups=synchronized_groups, buildConfigurationList=configs(name, settings),
                              buildPhases=phases, buildRules=[], dependencies=dependencies))
    project = add("project", "PBXProject", attributes={"LastUpgradeCheck": "2710"},
                  buildConfigurationList=configs("project", {}), compatibilityVersion="Xcode 16.0",
                  preferredProjectObjectVersion="77", developmentRegion="en", knownRegions=["en", "Base"],
                  mainGroup=root, productRefGroup=products, projectDirPath="", projectRoot="", targets=target_ids)
    project_text = "// !$*UTF8*$!\n" + openstep({"archiveVersion": "1", "classes": {}, "objectVersion": "77",
                                               "objects": objects, "rootObject": project}) + "\n"

    def reference(name):
        product = name + ".framework" if name.startswith("Open") else name
        return (f'<BuildableReference BuildableIdentifier="primary" BlueprintIdentifier="{ident(name)}" '
                f'BuildableName="{product}" BlueprintName="{name}" ReferencedContainer="container:OpenFoundation.xcodeproj"/>')

    scheme = '<?xml version="1.0" encoding="UTF-8"?>\n<Scheme LastUpgradeVersion="2710" version="1.3">\n'
    scheme += '<BuildAction parallelizeBuildables="YES" buildImplicitDependencies="YES"><BuildActionEntries>\n'
    for name in product_references:
        scheme += ('<BuildActionEntry buildForTesting="YES" buildForRunning="YES" buildForProfiling="YES" '
                   'buildForArchiving="YES" buildForAnalyzing="YES">' + reference(name) + '</BuildActionEntry>\n')
    scheme += '</BuildActionEntries></BuildAction>\n'
    scheme += ('<LaunchAction buildConfiguration="Debug" selectedDebuggerIdentifier="Xcode.DebuggerFoundation.Debugger.LLDB" '
               'selectedLauncherIdentifier="Xcode.IDEFoundation.Launcher.LLDB" launchStyle="0" useCustomWorkingDirectory="NO">'
               '<BuildableProductRunnable runnableDebuggingMode="0">' + reference("StandaloneTests") + '</BuildableProductRunnable></LaunchAction>\n')
    scheme += '<AnalyzeAction buildConfiguration="Debug"/><ArchiveAction buildConfiguration="Release" revealArchiveInOrganizer="YES"/>\n</Scheme>\n'
    folder = ROOT / "OpenFoundation.xcodeproj"
    return {folder / "project.pbxproj": project_text,
            folder / "xcshareddata/xcschemes/OpenFoundation.xcscheme": scheme}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    for path, text in generate().items():
        if args.check:
            if not path.is_file() or path.read_text() != text:
                raise SystemExit("Regenerate the Xcode project: make project")
        else:
            write_changed(path, text)
    print("Xcode project is current." if args.check else "Generated OpenFoundation.xcodeproj.")


if __name__ == "__main__":
    main()

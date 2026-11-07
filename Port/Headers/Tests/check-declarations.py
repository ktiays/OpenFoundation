#!/usr/bin/env python3
#
#  Created by ktiays on 2026/11/07.
#  Copyright (c) 2026 ktiays. All rights reserved.
#
"""Checks the C-level declarations of Port/Headers against OpenFoundation's own declarations.

The SDK-shaped headers and the restored headers cannot share one translation unit: both define the same
anonymous structures and enumerators, which C does not let a unit redefine. The check therefore reads the
restored declarations from clang's AST of OpenFoundation's headers (compiled with OpenFoundation's exact flags
and its force-included OFPrefixMapping.h, so every name is the CF/NS one) and writes a check unit that includes
the SDK-shaped umbrella header and

- redeclares every function and variable the umbrella declares with the type the restored declaration has, so
  that a differing type is a compile error ("conflicting types");
- asserts that every typedef the umbrella declares is compatible with the restored typedef;
- asserts, field by field, that every structure has the restored structure's field types in its order;
- asserts that every enumerator has the restored enumerator's value.

The check unit is compiled with OpenFoundation's flags in the requested language. Names the umbrella declares
that no restored header or port definition declares are listed with whether OpenFoundation's libraries define
them (unverified signature) or not (declared, not implemented).

    check-declarations.py --compile-commands <build>/compile_commands.json --language c \\
        --umbrella CoreFoundation/CoreFoundation.h --scratch <directory> [--archives <lib>.a ...]
"""

import argparse
import concurrent.futures
import json
import os
import re
import shlex
import subprocess
import sys

HEADERS = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
PORT = os.path.dirname(HEADERS)
ROOT = os.path.dirname(PORT)

# Restored spellings of SDK types that the restored headers name locally; the check aliases them to the SDK
# type and compares the layouts field by field.
ALIASES = {
    'RunLoopTimerContext': 'CFRunLoopTimerContext',
    'RunLoopObserverContext': 'CFRunLoopObserverContext',
    'ValuePoint': 'CGPoint',
    'ValueSize': 'CGSize',
    'ValueRect': 'CGRect',
}

REFERENCE_SOURCES = {
    'c': 'Source/CoreFoundation/Collections/OCFArray.c',
    'objective-c': 'Source/Foundation/String/ONSString.m',
}


def compilerArguments(compileCommands, sourceSuffix):
    for entry in json.load(open(compileCommands)):
        if entry['file'].endswith(sourceSuffix):
            arguments = entry.get('arguments') or shlex.split(entry['command'])
            kept = []
            skip = False
            for argument in arguments[1:]:
                if skip:
                    skip = False
                    continue
                if argument in ('-o', '-c'):
                    skip = argument == '-o'
                    continue
                if argument == entry['file'] or argument.endswith(sourceSuffix):
                    continue
                if argument in ('-x',):
                    skip = True
                    continue
                if argument in ('objective-c', 'c'):
                    continue
                kept.append(argument)
            return arguments[0], kept, entry['directory']
    sys.exit(f'no compile command for {sourceSuffix} in {compileCommands}')


def dumpAST(compiler, flags, directory, language, source, extra=(), optional=False):
    command = [compiler, *flags, *extra, '-x', language, '-fsyntax-only', '-Xclang', '-ast-dump=json', source]
    result = subprocess.run(command, cwd=directory, capture_output=True, text=True)
    if result.returncode != 0 and optional:
        return None
    if result.returncode != 0:
        sys.stderr.write(result.stderr[-4000:])
        sys.exit(f'AST dump failed: {source}')
    return json.loads(result.stdout)


class Declarations:
    """Top-level declarations of one AST, with the file each comes from."""

    def __init__(self):
        self.functions = {}
        self.variables = {}
        self.typedefs = {}
        self.records = {}
        self.enumerators = {}
        self.enums = {}
        self.files = {}

    def collect(self, ast, accept):
        state = {'file': None}
        recordsByID = {}
        recordsByName = {}
        enumsByID = {}

        def visitLocation(location):
            if not isinstance(location, dict):
                return None
            if 'spellingLoc' in location or 'expansionLoc' in location:
                visitLocation(location.get('spellingLoc'))
                return visitLocation(location.get('expansionLoc'))
            if 'file' in location:
                state['file'] = location['file']
            if 'includedFrom' in location:
                pass
            return state['file'] if 'offset' in location else None

        def walk(node):
            # Follows the emission order of clang's JSON dumper, which prints a file name only when it differs
            # from the previous location's.
            if isinstance(node, list):
                for item in node:
                    walk(item)
                return None
            if not isinstance(node, dict):
                return None
            declFile = None
            for key, value in node.items():
                if key == 'loc':
                    declFile = visitLocation(value)
                elif key == 'range':
                    visitLocation(value.get('begin'))
                    visitLocation(value.get('end'))
                elif isinstance(value, (dict, list)):
                    walk(value)
            return declFile

        for node in ast.get('inner', []):
            file = walk(node)
            kind = node.get('kind')
            if kind == 'RecordDecl' and 'inner' in node:
                recordsByID[node['id']] = node
                if node.get('name'):
                    recordsByName[node['name']] = node
            if kind == 'EnumDecl':
                enumsByID[node['id']] = node
            if not file or not accept(file):
                continue
            name = node.get('name')
            qualType = node.get('type', {}).get('qualType')
            if kind == 'FunctionDecl' and name and node.get('storageClass') == 'static':
                continue
            if kind == 'FunctionDecl' and name:
                self.functions.setdefault(name, qualType)
                self.files.setdefault(name, file)
            elif kind == 'VarDecl' and name and node.get('storageClass') != 'static':
                self.variables.setdefault(name, qualType)
                self.files.setdefault(name, file)
            elif kind == 'VarDecl' and name:
                self.enumerators.setdefault(name, ('static', qualType))
                self.files.setdefault(name, file)
            elif kind == 'TypedefDecl' and name:
                tag = findTag(node.get('inner', [])) or (None, None)
                # A typedef that names an attributed redeclaration ("typedef struct A T T;") refers to a
                # redeclaration without a body; the definition is found by its tag.
                record = recordsByID.get(tag[0]) or (recordsByName.get(tag[1]) if tag[1] else None)
                enum = enumsByID.get(tag[0])
                self.typedefs.setdefault(name, (qualType, record, enum))
                self.files.setdefault(name, file)
            elif kind == 'RecordDecl' and name and 'inner' in node:
                self.records.setdefault(name, node)
            elif kind == 'EnumDecl':
                if name:
                    self.enums.setdefault(name, node)
                previous = -1
                for constant in node.get('inner', []):
                    if constant.get('kind') != 'EnumConstantDecl':
                        continue
                    value = constantValue(constant)
                    if value is None:
                        value = previous + 1
                    previous = value
                    self.enumerators.setdefault(constant['name'], ('enum', value))
                    self.files.setdefault(constant['name'], file)


def findTag(node):
    """The structure or enumeration a typedef names directly (through elaboration only, not pointers)."""
    if isinstance(node, list):
        return findTag(node[0]) if node else None
    if not isinstance(node, dict):
        return None
    if 'ownedTagDecl' in node:
        return (node['ownedTagDecl']['id'], node['ownedTagDecl'].get('name'))
    if node.get('kind') in ('RecordType', 'EnumType') and 'decl' in node:
        return (node['decl']['id'], node['decl'].get('name'))
    if node.get('kind') in ('ElaboratedType', 'ParenType', 'QualType'):
        return findTag(node.get('inner', []))
    return None


def constantValue(node):
    if isinstance(node, dict):
        if node.get('kind') == 'ConstantExpr' and 'value' in node:
            return int(node['value'])
        for child in node.get('inner', []):
            value = constantValue(child)
            if value is not None:
                return value
    return None


def fields(record):
    return [(field.get('name'), field['type']['qualType']) for field in record.get('inner', [])
            if field.get('kind') == 'FieldDecl']


def identifier(text):
    return re.sub(r'[^A-Za-z0-9_]', '_', text)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--compile-commands', required=True)
    parser.add_argument('--language', choices=('c', 'objective-c'), required=True)
    parser.add_argument('--umbrella', required=True)
    parser.add_argument('--scratch', required=True)
    parser.add_argument('--archives', nargs='*', default=[])
    parser.add_argument('--nm', default='llvm-nm')
    parser.add_argument('--headers', default=HEADERS, help='the SDK-shaped headers (default: Port/Headers)')
    arguments = parser.parse_args()
    headers = os.path.abspath(arguments.headers)
    os.makedirs(arguments.scratch, exist_ok=True)
    language = arguments.language
    stem = identifier(arguments.umbrella) + '-' + identifier(language)

    compiler, flags, directory = compilerArguments(arguments.compile_commands, REFERENCE_SOURCES[language])
    headerFlags = ['-I' + headers]

    # The restored side: every restored header (the Objective-C ones only in Objective-C), and the port's
    # definitions of entry points no restored header declares.
    # The C headers of CoreFoundation compile together; the Objective-C headers of CoreFoundation and
    # Foundation do not (some redeclare bridged classes, trimmed units assert LP64 layouts), so in Objective-C
    # each of them is a unit of its own and a header that does not compile alone is skipped and listed.
    cHeaders = []
    objcHeaders = []
    for area in ('CoreFoundation', 'Foundation'):
        base = os.path.join(ROOT, 'Source', area)
        for folder, _, names in sorted(os.walk(base)):
            for name in sorted(names):
                if not name.endswith('.h'):
                    continue
                relative = os.path.relpath(os.path.join(folder, name), os.path.join(ROOT, 'Source'))
                if area == 'CoreFoundation' and '/Objective-C/' not in '/' + relative:
                    cHeaders.append(relative)
                else:
                    objcHeaders.append(relative)
    suffix = 'm' if language != 'c' else 'c'
    restoredUnit = os.path.join(arguments.scratch, f'{stem}-restored.{suffix}')
    with open(restoredUnit, 'w') as unit:
        for relative in cHeaders:
            unit.write(f'#include "{relative}"\n')
    restored = Declarations()
    inRestored = lambda file: os.path.abspath(file).startswith(os.path.join(ROOT, 'Source')) or \
        '/FrameworkIncludes/' in file
    restored.collect(dumpAST(compiler, flags, directory, language, restoredUnit), inRestored)
    skipped = []
    if language != 'c' and not arguments.umbrella.startswith('CoreFoundation/'):
        units = []
        for index, relative in enumerate(objcHeaders):
            unitPath = os.path.join(arguments.scratch, f'{stem}-restored-{index}.m')
            with open(unitPath, 'w') as unit:
                unit.write(f'#include "{relative}"\n')
            units.append((relative, unitPath))
        with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as pool:
            results = list(pool.map(
                lambda item: (item[0], dumpAST(compiler, flags, directory, language, item[1], optional=True)),
                units))
        for relative, ast in results:
            if ast is None:
                skipped.append(relative)
            else:
                restored.collect(ast, inRestored)
    portDefinitions = []
    for area in ('CoreFoundation', 'Foundation'):
        if area == 'Foundation' and language == 'c':
            continue
        folder = os.path.join(PORT, area)
        for name in sorted(os.listdir(folder)):
            if name.endswith('.c') or (name.endswith('.m') and language != 'c'):
                portDefinitions.append(os.path.join(folder, name))
    for source in portDefinitions:
        sourceLanguage = 'objective-c' if source.endswith('.m') else 'c'
        restored.collect(dumpAST(compiler, flags, directory, sourceLanguage, source),
                         lambda file, source=source: os.path.abspath(file) == source)

    # The SDK-shaped side.
    umbrellaUnit = os.path.join(arguments.scratch, f'{stem}-headers.{"m" if language != "c" else "c"}')
    with open(umbrellaUnit, 'w') as unit:
        unit.write(f'#include <{arguments.umbrella}>\n')
    sdk = Declarations()
    sdk.collect(dumpAST(compiler, flags, directory, language, umbrellaUnit, headerFlags),
                lambda file: os.path.abspath(file).startswith(headers + os.sep))

    lines = [f'#include <{arguments.umbrella}>', '', '// Generated by check-declarations.py. Do not edit.', '']
    checked = []
    unchecked = []
    report = []

    # Restored typedef names for SDK types (ALIASES) are defined as the SDK type, so that restored declarations
    # and fields spelled with them resolve; their layouts are compared below.
    aliases = [alias for alias, target in sorted(ALIASES.items()) if target in sdk.typedefs and alias not in sdk.typedefs]
    for alias in aliases:
        lines.append(f'typedef {ALIASES[alias]} {alias};')
    if aliases:
        lines.append('')

    for name in sorted(sdk.typedefs):
        qualType, record, enum = sdk.typedefs[name]
        restoredName = name
        if name not in restored.typedefs:
            restoredName = next((alias for alias, target in ALIASES.items() if target == name and
                                 alias in restored.typedefs), None)
            if restoredName is None:
                continue
        restoredType, restoredRecord, restoredEnum = restored.typedefs[restoredName]
        if record is not None and restoredRecord is not None:
            mine = fields(record)
            theirs = fields(restoredRecord)
            if len(mine) != len(theirs):
                report.append(f'structure {name}: {len(mine)} fields, restored {restoredName} has {len(theirs)}')
                continue
            for (field, _), (restoredField, restoredFieldType) in zip(mine, theirs):
                if field != restoredField:
                    report.append(f'structure {name}: field {field} is {restoredField} in {restoredName}')
                lines.append(
                    f'_Static_assert(__builtin_types_compatible_p(__typeof__((({name} *)0)->{field}), '
                    f'{restoredFieldType}), "{name}.{field}");'
                )
            checked.append(f'struct {name}')
            continue
        if record is not None or restoredRecord is not None:
            report.append(f'typedef {name}: structure on one side only')
            continue
        compared = restoredType
        if restoredEnum is not None and 'fixedUnderlyingType' in restoredEnum:
            compared = restoredEnum['fixedUnderlyingType']['qualType']
        if restoredEnum is not None and 'fixedUnderlyingType' not in restoredEnum:
            report.append(f'typedef {name}: the restored enumeration has no fixed underlying type')
            continue
        lines.append(f'_Static_assert(__builtin_types_compatible_p({name}, {compared}), "typedef {name}");')
        checked.append(f'typedef {name}')
    lines.append('')

    for name in sorted(sdk.enumerators):
        if name not in restored.enumerators:
            continue
        kind, value = restored.enumerators[name]
        if kind != 'enum':
            continue
        lines.append(f'_Static_assert((long long)({name}) == {value}LL, "{name} == {value}");')
        checked.append(f'constant {name}')
    lines.append('')

    for name in sorted(sdk.functions):
        if name.startswith('__builtin'):
            continue
        restoredType = restored.functions.get(name)
        if restoredType is None:
            unchecked.append(name)
            continue
        lines.append(f'extern {declarator(restoredType, name)};')
        checked.append(f'function {name}')
    lines.append('')

    tollFree = []
    for name in sorted(sdk.variables):
        restoredType = restored.variables.get(name)
        if restoredType is None:
            unchecked.append(name)
            continue
        # Constants the restored CoreFoundation defines as CFStringRef and the SDK declares as NSString *
        # (toll-free bridged, both one object pointer) are compared as object pointers.
        if language != 'c' and re.fullmatch(r'const CFStringRef', restoredType) and \
                re.search(r'NSString \*const|const (NSExceptionName|NSNotificationName|NSErrorDomain|NSRunLoopMode)',
                          sdk.variables[name]):
            lines.append(f'_Static_assert(__builtin_types_compatible_p(__typeof__((NSString *)(CFStringRef)0), '
                         f'__typeof__((NSString *){name})), "{name}");')
            tollFree.append(name)
            checked.append(f'variable {name}')
            continue
        lines.append(f'extern {declarator(restoredType, name)};')
        checked.append(f'variable {name}')

    checkUnit = os.path.join(arguments.scratch, f'{stem}-check.{"m" if language != "c" else "c"}')
    with open(checkUnit, 'w') as unit:
        unit.write('\n'.join(lines) + '\n')
    command = [compiler, *flags, *headerFlags, '-x', language, '-fsyntax-only', '-Wno-gnu-folding-constant',
               checkUnit]
    result = subprocess.run(command, cwd=directory, capture_output=True, text=True)
    diagnostics = [line for line in result.stderr.splitlines() if 'error:' in line or 'warning:' in line]

    defined = set()
    for archive in arguments.archives:
        output = subprocess.run([arguments.nm, '--defined-only', '-g', archive], capture_output=True, text=True)
        for line in output.stdout.splitlines():
            parts = line.split()
            if len(parts) == 3:
                defined.add(parts[2])

    print(f'{arguments.umbrella} ({language}): {len(checked)} declarations compared with the restored ones')
    print(f'  check unit: {checkUnit}')
    if tollFree:
        print(f'  CFStringRef in the restored declaration, NSString * in the SDK\'s ({len(tollFree)}): '
              + ' '.join(tollFree))
    if skipped:
        print(f'  restored headers that do not compile on their own, not read ({len(skipped)}): ' + ' '.join(skipped))
    for line in report:
        print('  MISMATCH ' + line)
    for line in diagnostics:
        print('  ' + line.replace(arguments.scratch + '/', ''))
    if unchecked:
        implemented = [name for name in unchecked if name in defined]
        missing = [name for name in unchecked if name not in defined]
        if arguments.archives:
            print(f'  declared without a restored declaration, defined by the libraries ({len(implemented)}): '
                  + ' '.join(implemented))
            print(f'  declared, not implemented ({len(missing)}): ' + ' '.join(missing))
        else:
            print(f'  declared without a restored declaration ({len(unchecked)}): ' + ' '.join(unchecked))
    failed = result.returncode != 0 or report
    print('  RESULT: ' + ('FAILED' if failed else 'passed'))
    return 1 if failed else 0


def declarator(qualType, name):
    """A declaration of `name` with the type clang printed as `qualType`: __typeof__ accepts any type name,
    function types included ("extern __typeof__(void (int)) f;" declares a function)."""
    return f'__typeof__({qualType}) {name}'


if __name__ == '__main__':
    sys.exit(main())

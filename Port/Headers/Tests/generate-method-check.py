#!/usr/bin/env python3
#
#  Created by ktiays on 2026/11/07.
#  Copyright (c) 2026 ktiays. All rights reserved.
#
"""Generates MethodCheck.m, the run-time check of the Objective-C declarations of Port/Headers/Foundation.

For every class the SDK-shaped headers declare methods for (its interface, its categories and the protocols it
adopts there), the program gets a protocol that redeclares those methods, so that the compiler records each
declaration's type encoding. At run time the program compares each protocol method's encoding with the encoding
of the method OpenFoundation's class (or a superclass) implements, and lists the methods no class in the chain
implements. Class lookups go through method lists, so no +initialize runs.

    generate-method-check.py --ast <Foundation.h AST as JSON> --output <MethodCheck.m>
"""

import argparse
import json
import os
import re
import sys

HEADERS = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))


def walkDecls(ast):
    state = {'file': None}

    def visitLocation(location):
        if not isinstance(location, dict):
            return None
        if 'spellingLoc' in location or 'expansionLoc' in location:
            visitLocation(location.get('spellingLoc'))
            return visitLocation(location.get('expansionLoc'))
        if 'file' in location:
            state['file'] = location['file']
        return state['file'] if 'offset' in location else None

    def walk(node):
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
        yield node, file


def substitute(text, parameters):
    for name, bound in parameters.items():
        text = re.sub(r'\b' + re.escape(name) + r'\b(<[^>]*>)?',
                      lambda match: bound + (match.group(1) or ''), text)
    return text


def declaration(method, parameters):
    returnType = substitute(method['returnType']['qualType'], parameters)
    sign = '-' if method.get('instance') else '+'
    selector = method['name']
    arguments = [child for child in method.get('inner', []) if child.get('kind') == 'ParmVarDecl']
    if ':' not in selector:
        text = f'{sign} ({returnType}){selector}'
    else:
        pieces = selector.split(':')[:-1]
        parts = []
        for index, piece in enumerate(pieces):
            argumentType = substitute(arguments[index]['type']['qualType'], parameters)
            parts.append(f'{piece}:({argumentType})a{index}')
        text = f'{sign} ({returnType})' + ' '.join(parts)
    if method.get('variadic'):
        text += ', ...'
    return text + ';'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--ast', required=True)
    parser.add_argument('--output', required=True)
    arguments = parser.parse_args()
    ast = json.load(open(arguments.ast))

    protocols = {}
    classes = {}
    order = []

    def entry(name):
        if name not in classes:
            classes[name] = {}
            order.append(name)
        return classes[name]

    def addMethods(target, node, parameters):
        for child in node.get('inner', []):
            if child.get('kind') == 'ObjCMethodDecl':
                key = (bool(child.get('instance')), child['name'])
                target.setdefault(key, declaration(child, parameters))

    def typeParameters(node):
        return {child['name']: child['type']['qualType'] for child in node.get('inner', [])
                if child.get('kind') == 'ObjCTypeParamDecl'}

    for node, file in walkDecls(ast):
        if not file or not os.path.abspath(file).startswith(os.path.join(HEADERS, 'Foundation') + os.sep):
            continue
        kind = node.get('kind')
        if kind == 'ObjCProtocolDecl' and 'inner' in node:
            methods = {}
            addMethods(methods, node, {})
            inherited = [protocol['name'] for protocol in node.get('protocols', [])]
            protocols[node['name']] = (methods, inherited)
        elif kind == 'ObjCInterfaceDecl' and ('inner' in node or 'name' in node.get('super', {})):
            target = entry(node['name'])
            addMethods(target, node, typeParameters(node))
            target.setdefault(('protocols', ''), []).extend(p['name'] for p in node.get('protocols', []))
        elif kind == 'ObjCCategoryDecl':
            target = entry(node['interface']['name'])
            addMethods(target, node, typeParameters(node))
            target.setdefault(('protocols', ''), []).extend(p['name'] for p in node.get('protocols', []))

    def protocolMethods(name, seen):
        if name in seen or name not in protocols:
            return {}
        seen.add(name)
        methods, inherited = protocols[name]
        result = dict(methods)
        for parent in inherited:
            for key, value in protocolMethods(parent, seen).items():
                result.setdefault(key, value)
        return result

    lines = [
        '//',
        '//  Generated by Port/Headers/Tests/generate-method-check.py. Do not edit.',
        '//',
        '',
        '#import <Foundation/Foundation.h>',
        '#include <objc/runtime.h>',
        '#include <stdio.h>',
        '#include <stdlib.h>',
        '#include <string.h>',
        '',
    ]
    entries = []
    total = 0
    for name in order:
        methods = {key: value for key, value in classes[name].items() if key[0] != 'protocols'}
        for protocol in classes[name].get(('protocols', ''), []):
            for key, value in protocolMethods(protocol, set()).items():
                methods.setdefault(key, value)
        lines.append(f'@protocol PortCheck_{name}')
        for key in sorted(methods, key=lambda k: (not k[0], k[1])):
            lines.append(methods[key])
        lines.append('@end')
        lines.append('')
        entries.append(name)
        total += len(methods)

    lines.append('typedef struct {')
    lines.append('    const char *className;')
    lines.append('    Protocol *protocol;')
    lines.append('} Entry;')
    lines.append('')
    lines.append('static size_t entries(Entry *list) {')
    lines.append('    size_t count = 0;')
    for name in entries:
        lines.append(f'    list[count++] = (Entry){{"{name}", @protocol(PortCheck_{name})}};')
    lines.append('    return count;')
    lines.append('}')
    lines.append(f'')
    lines.append(f'#define kEntryCapacity {len(entries)}')
    lines.append(RUNTIME_PART)
    with open(arguments.output, 'w') as output:
        output.write('\n'.join(lines) + '\n')
    print(f'{len(entries)} classes, {total} methods')
    return 0


RUNTIME_PART = r'''
// Method encodings are compared at the calling convention's level: frame offsets and the qualifiers that do
// not change it (const, in, inout, out, bycopy, byref, oneway) are dropped, every pointer is "^" whatever it
// points to, structure tags are dropped, integers of one width compare equal whatever their signedness, and
// wasm32's int, long and bool-sized integers fold together as they are all passed as i32. `exact` keeps tags,
// pointees and integer spellings so that spelling differences can be listed.
static const char *encodeType(const char *p, char *out, size_t *length, size_t size, int exact, int emit);

static void put(char *out, size_t *length, size_t size, char character, int emit) {
    if (emit && *length + 1 < size) {
        out[(*length)++] = character;
        out[*length] = '\0';
    }
}

static const char *skipQualifiers(const char *p) {
    while (*p != '\0' && strchr("rnNoORVA", *p) != NULL) {
        p++;
    }
    return p;
}

static const char *encodeType(const char *p, char *out, size_t *length, size_t size, int exact, int emit) {
    p = skipQualifiers(p);
    char c = *p;
    if (c == '\0') {
        return p;
    }
    p++;
    switch (c) {
        case '^':
            put(out, length, size, '^', emit);
            return encodeType(p, out, length, size, exact, emit && exact);
        case '{':
        case '(': {
            put(out, length, size, c, emit);
            char close = c == '{' ? '}' : ')';
            const char *name = p;
            while (*p != '\0' && *p != '=' && *p != close) {
                p++;
            }
            if (exact) {
                for (const char *q = name; q < p; q++) {
                    put(out, length, size, *q, emit);
                }
            }
            if (*p == '=') {
                put(out, length, size, '=', emit && exact);
                p++;
                while (*p != '\0' && *p != close) {
                    p = encodeType(p, out, length, size, exact, emit);
                }
            }
            if (*p == close) {
                p++;
            }
            put(out, length, size, close, emit);
            return p;
        }
        case '[':
            put(out, length, size, '[', emit);
            while (*p >= '0' && *p <= '9') {
                put(out, length, size, *p++, emit);
            }
            p = encodeType(p, out, length, size, exact, emit);
            if (*p == ']') {
                p++;
            }
            put(out, length, size, ']', emit);
            return p;
        case 'b':
            put(out, length, size, 'b', emit);
            while (*p >= '0' && *p <= '9') {
                put(out, length, size, *p++, emit);
            }
            return p;
        case '@':
            put(out, length, size, '@', emit);
            if (*p == '?') {
                put(out, length, size, '?', emit);
                p++;
            } else if (*p == '"') {
                p++;
                while (*p != '\0' && *p != '"') {
                    p++;
                }
                if (*p == '"') {
                    p++;
                }
            }
            return p;
        default:
            break;
    }
    if (!exact) {
        switch (c) {
            case 'C': c = 'c'; break;
            case 'B': c = 'c'; break;
            case 'S': c = 's'; break;
            case 'I':
            case 'l':
            case 'L': c = 'i'; break;
            case 'Q': c = 'q'; break;
            default: break;
        }
    }
    put(out, length, size, c, emit);
    return p;
}

static void normalize(const char *encoding, char *out, size_t size, int exact) {
    size_t length = 0;
    out[0] = '\0';
    const char *p = encoding;
    while (*p != '\0') {
        p = encodeType(p, out, &length, size, exact, 1);
        while (*p >= '0' && *p <= '9') {
            p++;
        }
    }
}

static Method ownMethod(Class cls, SEL selector) {
    unsigned int count = 0;
    Method *methods = class_copyMethodList(cls, &count);
    Method found = NULL;
    for (unsigned int index = 0; index < count; index++) {
        if (method_getName(methods[index]) == selector) {
            found = methods[index];
            break;
        }
    }
    free(methods);
    return found;
}

static Method chainMethod(Class cls, SEL selector, BOOL instance, Class *owner) {
    for (Class current = cls; current != Nil; current = class_getSuperclass(current)) {
        Method method = ownMethod(instance ? current : object_getClass((id)current), selector);
        if (method != NULL) {
            *owner = current;
            return method;
        }
    }
    return NULL;
}

static int subclassImplementers(Class cls, SEL selector, BOOL instance, char *names, size_t size) {
    unsigned int count = 0;
    Class *list = objc_copyClassList(&count);
    int found = 0;
    names[0] = '\0';
    for (unsigned int index = 0; index < count; index++) {
        Class candidate = list[index];
        if (candidate == cls) {
            continue;
        }
        BOOL inherits = NO;
        for (Class current = class_getSuperclass(candidate); current != Nil; current = class_getSuperclass(current)) {
            if (current == cls) {
                inherits = YES;
                break;
            }
        }
        if (!inherits || ownMethod(instance ? candidate : object_getClass((id)candidate), selector) == NULL) {
            continue;
        }
        if (found < 3) {
            if (found > 0) {
                strncat(names, " ", size - strlen(names) - 1);
            }
            strncat(names, class_getName(candidate), size - strlen(names) - 1);
        }
        found++;
    }
    free(list);
    return found;
}

int main(void) {
    int checked = 0;
    int mismatches = 0;
    int missing = 0;
    int subclassOnly = 0;
    int missingClasses = 0;
    int spellings = 0;
    Entry kEntries[kEntryCapacity];
    size_t entryCount = entries(kEntries);
    for (size_t entry = 0; entry < entryCount; entry++) {
        Class cls = objc_getClass(kEntries[entry].className);
        for (int pass = 0; pass < 2; pass++) {
            BOOL instance = pass == 0;
            unsigned int count = 0;
            struct objc_method_description *descriptions =
                protocol_copyMethodDescriptionList(kEntries[entry].protocol, YES, instance, &count);
            if (cls == Nil) {
                unsigned int classCount = 0;
                free(protocol_copyMethodDescriptionList(kEntries[entry].protocol, YES, NO, &classCount));
                if (instance) {
                    missingClasses++;
                }
                if (count == 0 && classCount == 0 && instance) {
                    printf("CLASS MISSING %s (no methods declared)\n", kEntries[entry].className);
                }
                if (count > 0) {
                    printf("CLASS MISSING %s (%u %s methods declared)\n", kEntries[entry].className, count,
                           instance ? "instance" : "class");
                    missing += count;
                }
                free(descriptions);
                continue;
            }
            for (unsigned int index = 0; index < count; index++) {
                checked++;
                SEL selector = descriptions[index].name;
                Class owner = Nil;
                Method method = chainMethod(cls, selector, instance, &owner);
                if (method == NULL) {
                    char names[256];
                    int implementers = subclassImplementers(cls, selector, instance, names, sizeof(names));
                    if (implementers > 0) {
                        printf("SUBCLASS ONLY %c[%s %s] (%s%s)\n", instance ? '-' : '+', kEntries[entry].className,
                               sel_getName(selector), names, implementers > 3 ? " ..." : "");
                        subclassOnly++;
                    } else {
                        printf("NOT IMPLEMENTED %c[%s %s]\n", instance ? '-' : '+', kEntries[entry].className,
                               sel_getName(selector));
                        missing++;
                    }
                    continue;
                }
                char declared[512];
                char implemented[512];
                normalize(descriptions[index].types, declared, sizeof(declared), 0);
                normalize(method_getTypeEncoding(method), implemented, sizeof(implemented), 0);
                if (strcmp(declared, implemented) != 0) {
                    printf("MISMATCH %c[%s %s] declared %s, implemented %s by %s\n", instance ? '-' : '+',
                           kEntries[entry].className, sel_getName(selector), descriptions[index].types,
                           method_getTypeEncoding(method), class_getName(owner));
                    mismatches++;
                    continue;
                }
                normalize(descriptions[index].types, declared, sizeof(declared), 1);
                normalize(method_getTypeEncoding(method), implemented, sizeof(implemented), 1);
                if (strcmp(declared, implemented) != 0) {
                    printf("SPELLING %c[%s %s] declared %s, implemented %s by %s\n", instance ? '-' : '+',
                           kEntries[entry].className, sel_getName(selector), descriptions[index].types,
                           method_getTypeEncoding(method), class_getName(owner));
                    spellings++;
                }
            }
            free(descriptions);
        }
    }
    printf("checked %d methods: %d mismatches, %d spelling differences, %d implemented only by subclasses, "
           "%d not implemented (%d classes missing)\n", checked, mismatches, spellings, subclassOnly, missing,
           missingClasses);
    return mismatches == 0 ? 0 : 1;
}
'''


if __name__ == '__main__':
    sys.exit(main())

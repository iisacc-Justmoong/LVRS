#!/usr/bin/env python3
"""Audit legacy/native platform coverage and emit a native-only test oracle."""
from pathlib import Path
import argparse
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[1]


def audit(root=ROOT):
    root = Path(root)
    fixture = json.loads((root / 'tests/native/fixtures/platform-profiles.json').read_text())
    header = (root / 'src/backend/platform/platforminfo.h').read_bytes()
    source = (root / 'src/backend/platform/platforminfo.cpp').read_bytes()
    for label, data in [('header', header), ('cpp', source)]:
        if hashlib.sha256(data).hexdigest() != fixture['legacy_' + label + '_sha256']:
            raise ValueError('Legacy platform ' + label + ' changed; review the complete fixture before updating its hash')
    keys = set(re.findall(r'profile\.insert\(QStringLiteral\("([^"]+)"\)', source.decode()))
    native = (root / 'src/LVRS/PlatformInfo.cpp').read_text()
    if keys != set(re.findall(r'put\("([^"]+)"', native)):
        raise ValueError('Native platform metadata keys differ from the original profile')
    if len(keys) != 41 or any(set(row) != keys for row in fixture['profiles'].values()):
        raise ValueError('The complete seven-profile, 41-field oracle is required')
    block = re.search(r'constexpr Alias aliases\[\]\{(.*?)\n\};', native, re.S)
    aliases = re.findall(r'\{"([^"]+)","([^"]+)"\}', block[1] if block else '')
    if set(aliases) != {tuple(pair) for pair in fixture['aliases']} or len(aliases) != 22:
        raise ValueError('Native platform aliases differ from the original aliases')
    properties = re.findall(r'Q_PROPERTY\([^\n]*?\bREAD\s+(\w+)', header.decode())
    queries = re.findall(r'Q_INVOKABLE\s+\w+\s+(\w+)\(', header.decode())
    public = (root / 'src/LVRS/PlatformInfo.h').read_text()
    if any(not re.search(r'\b' + name + r'\s*\(', public) for name in properties + queries):
        raise ValueError('A declared platform property or query is missing from the native API')
    return fixture, {'legacy_properties': len(properties), 'legacy_queries': len(queries),
                     'profile_fields': len(keys), 'profiles': len(fixture['profiles']), 'aliases': len(aliases)}


def literal(value):
    if isinstance(value, dict):
        return 'ii::core::Value(ii::core::Value::Object{' + ','.join(
            '{' + json.dumps(key) + ',' + literal(item) + '}' for key, item in value.items()) + '})'
    if isinstance(value, bool):
        return 'ii::core::Value(' + str(value).lower() + ')'
    return 'ii::core::Value(' + json.dumps(value) + ')'


def render(fixture):
    result = '#pragma once\n#include <LVRS/PlatformInfo.h>\n#include <array>\nnamespace platform_oracle {\n'
    result += 'inline constexpr std::array<std::pair<std::string_view,std::string_view>,22> aliases{{'
    result += ','.join('{' + json.dumps(a) + ',' + json.dumps(b) + '}' for a, b in fixture['aliases']) + '}};\n'
    result += 'inline ii::core::Value profile(std::string_view target) {\n'
    for target, profile in fixture['profiles'].items():
        result += 'if (target==' + json.dumps(target) + ') return ' + literal(profile) + ';\n'
    return result + 'throw std::invalid_argument("Unknown platform oracle profile");\n}\n}\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--header', type=Path)
    args = parser.parse_args()
    fixture, summary = audit(args.root)
    if args.header:
        text = render(fixture)
        if not args.header.exists() or args.header.read_text() != text:
            args.header.write_text(text)
    print(json.dumps(summary))


if __name__ == '__main__':
    main()

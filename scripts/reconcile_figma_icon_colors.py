#!/usr/bin/env python3
"""Apply independently verified Figma paints while preserving SVG geometry.

The input directory contains Figma node readbacks as JSON arrays with ``id``,
base64 ``svg`` and ``vars`` fields. A partial readback is explicitly recorded;
icons whose geometry or structure cannot be matched are never guessed.
"""
import argparse
import base64
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import unicodedata
import xml.etree.ElementTree as ET

PAINT = {'fill', 'stroke', 'fill-opacity', 'stroke-opacity', 'stop-color', 'stop-opacity', 'opacity'}
NUMBER = re.compile(r'[-+]?(?:\d*\.\d+|\d+\.?\d*)(?:[eE][-+]?\d+)?')
TAG = re.compile(r'<(?![!?/])(?:[\w.-]+:)?[\w.-]+\b[^>]*>')
ALL_TAGS = re.compile(r'<[^>]*>')
ATTRIBUTE = re.compile(r'\s+([\w:.-]+)\s*=\s*(["\'])(.*?)\2')
SAFE_PAINT = re.compile(r'(?:#[0-9a-fA-F]{3,8}|none|transparent|black|white|[-+]?(?:\d*\.\d+|\d+\.?\d*))\Z')
FILL_RULES = {'fill-rule', 'clip-rule'}
URL_PAINT = re.compile(r'url\(#([^)]*)\)\Z')


def digest(data):
    return hashlib.sha256(data).hexdigest()


def same_geometry(a, b):
    if a == b:
        return True
    if len(a) > 20000 or len(b) > 20000 or NUMBER.sub('@', a) != NUMBER.sub('@', b):
        return False
    aa, bb = NUMBER.findall(a), NUMBER.findall(b)
    # Allow subpixel export rounding, while preserving the original coordinates.
    return len(aa) == len(bb) and all(abs(float(x) - float(y)) < .25 for x, y in zip(aa, bb))


def fingerprint_svg(data, scale=100):
    """Compact geometry fingerprints and ordered paints for remote readback."""
    try:
        source = data.decode('ascii')
    except UnicodeDecodeError:
        return None
    if ALL_TAGS.sub('', source).strip():
        return None
    parts = [[], []]
    paints = []
    index = 0
    for match in ALL_TAGS.finditer(source):
        tag = match.group()
        if re.match(r'<\s*[!?]', tag):
            continue
        parsed = re.match(r'<\s*(/)?\s*([\w:.-]+)', tag)
        if not parsed:
            return None
        closing, name = bool(parsed[1]), parsed[2]
        marker = ('C:' if closing else 'O:') + name + (':S' if re.search(r'/\s*>$', tag) else '')
        for part in parts:
            part.append(marker)
        if closing:
            continue
        attrs = {a[1]: a[3] for a in ATTRIBUTE.finditer(tag)}
        for key in sorted(attrs):
            value = attrs[key]
            if key in PAINT:
                paints.append({'index': index, 'key': key, 'value': value})
            elif index:
                for offset, part in zip((0, .5), parts):
                    normalized = NUMBER.sub(lambda m: str(math.floor(float(m[0]) * scale + offset)), value)
                    part.append('A:' + key + ':' + normalized)
        index += 1
    def fnv(value):
        h = 2166136261
        for byte in value.encode('ascii'):
            h = ((h ^ byte) * 16777619) & 0xffffffff
        return h
    return {'geometry': [fnv('\0'.join(part)) for part in parts], 'paints': paints}


def apply_paint_edits(original, edits):
    """Apply index-addressed paints only when the original still matches."""
    text = original.decode('utf-8')
    tags = list(TAG.finditer(text))
    grouped = {}
    for edit in edits:
        if (edit['key'] not in PAINT or not SAFE_PAINT.fullmatch(edit['before'])
                or not SAFE_PAINT.fullmatch(edit['after']) or not 0 <= edit['index'] < len(tags)):
            raise ValueError('Invalid paint edit')
        grouped.setdefault(edit['index'], []).append(edit)
    for index in sorted(grouped, reverse=True):
        tag = tags[index]
        updated = tag.group()
        for edit in grouped[index]:
            expression = re.compile(r'(\s' + re.escape(edit['key']) + r'\s*=\s*)(["\'])(.*?)(\2)')
            found = expression.search(updated)
            if not found or found.group(3) != edit['before']:
                raise ValueError('Original paint changed during readback')
            updated = updated[:found.start(3)] + edit['after'] + updated[found.end(3):]
        text = text[:tag.start()] + updated + text[tag.end():]
    result = text.encode('utf-8')
    ET.fromstring(result)
    return result


def embedded_png_svg(original, png, width, height):
    """Keep the established SVG viewport while embedding a Figma-rendered PNG."""
    if png[:8] != b'\x89PNG\r\n\x1a\n' or len(png) < 24:
        raise ValueError('Invalid PNG readback')
    pixels = struct.unpack('>II', png[16:24])
    if pixels != (width * 8, height * 8):
        raise ValueError('Unexpected Figma PNG scale')
    root = ET.fromstring(original)
    viewport = root.attrib.get('viewBox')
    if not viewport or not re.fullmatch(r'[-+\d.eE, ]+', viewport):
        raise ValueError('Invalid existing SVG viewport')
    encoded = base64.b64encode(png).decode('ascii')
    svg = (f'<svg fill="none" xmlns="http://www.w3.org/2000/svg" '
           f'xmlns:xlink="http://www.w3.org/1999/xlink" width="100%" height="100%" '
           f'viewBox="{viewport}" preserveAspectRatio="xMidYMid meet">\n'
           f'<image width="{width}" height="{height}" preserveAspectRatio="none" '
           f'xlink:href="data:image/png;base64,{encoded}"/>\n</svg>\n')
    result = svg.encode('utf-8')
    ET.fromstring(result)
    return result


def current_vector_svg(original, export):
    """Use Figma's vector artwork while retaining the existing responsive viewport."""
    old_root, new_root = ET.fromstring(original), ET.fromstring(export)
    viewport = old_root.attrib.get('viewBox', '')
    if not re.fullmatch(r'[-+\d.eE, ]+', viewport):
        raise ValueError('Invalid existing SVG viewport')
    unsupported = {'mask', 'filter', 'pattern', 'image', 'text', 'script', 'style', 'foreignObject'}
    for node in new_root.iter():
        name = node.tag.rsplit('}', 1)[-1]
        if name in unsupported or name.startswith('animate'):
            raise ValueError('SVG requires separate compatibility handling: ' + name)
        for key, value in node.attrib.items():
            if key.rsplit('}', 1)[-1] == 'href' and not value.startswith('#'):
                raise ValueError('External SVG reference is not allowed')
    source = export.decode('utf-8')
    root_tag = TAG.search(source)
    if not root_tag:
        raise ValueError('SVG root is missing')
    tag = root_tag.group()
    for key, value in [('width', '100%'), ('height', '100%'), ('viewBox', viewport),
                       ('preserveAspectRatio', 'xMidYMid meet')]:
        expression = re.compile(r'\s+' + re.escape(key) + r'\s*=\s*(["\']).*?\1')
        tag = expression.sub('', tag)
        tag = tag[:-1] + f' {key}="{value}">'
    result = (source[:root_tag.start()] + tag + source[root_tag.end():]).encode('utf-8')
    ET.fromstring(result)
    return result


def reconcile(original, export, geometry_evidence=None):
    """Return geometry-preserving SVG bytes, or None if matching is ambiguous."""
    old_root, new_root = ET.fromstring(original), ET.fromstring(export)
    old_nodes, new_nodes = list(old_root.iter()), list(new_root.iter())
    tags = list(TAG.finditer(original.decode('utf-8')))
    if len(old_nodes) != len(new_nodes) or len(tags) != len(old_nodes):
        return None
    id_map = {}
    for old, new in zip(old_nodes, new_nodes):
        if 'id' in old.attrib and 'id' in new.attrib:
            if old.attrib['id'] in id_map and id_map[old.attrib['id']] != new.attrib['id']:
                return None
            id_map[old.attrib['id']] = new.attrib['id']
    replacements = []
    for index, (old, new) in enumerate(zip(old_nodes, new_nodes)):
        if old.tag != new.tag:
            return None
        if ((old.text or '').strip() != (new.text or '').strip()
                or (old.tail or '').strip() != (new.tail or '').strip()):
            return None
        if index == 0:
            keys = PAINT & (set(old.attrib) | set(new.attrib))
        else:
            if set(old.attrib) - FILL_RULES != set(new.attrib) - FILL_RULES:
                return None
            keys = set(old.attrib) - FILL_RULES
        changes = {}
        for key in keys:
            before, after = old.attrib.get(key), new.attrib.get(key)
            if before == after:
                continue
            if key == 'id' and id_map.get(before) == after:
                continue
            old_url = URL_PAINT.fullmatch(before or '')
            new_url = URL_PAINT.fullmatch(after or '')
            if old_url and new_url and id_map.get(old_url[1]) == new_url[1]:
                continue
            if key in PAINT and before is not None and after is not None:
                if not SAFE_PAINT.fullmatch(before) or not SAFE_PAINT.fullmatch(after):
                    return None
                changes[key] = (before, after)
            elif index or key in PAINT:
                if (key == 'd' and before is not None and after is not None
                        and geometry_evidence
                        and geometry_evidence.get(digest(before.encode()) + ':' + digest(after.encode()), 0) >= .985):
                    continue
                if before is None or after is None or not same_geometry(before, after):
                    return None
        replacements.append(changes)

    source = original.decode('utf-8')
    for tag, changes in reversed(list(zip(tags, replacements))):
        updated = tag.group()
        for key, (before, after) in changes.items():
            expression = re.compile(r'(\s' + re.escape(key) + r'\s*=\s*)(["\'])(.*?)(\2)')
            found = expression.search(updated)
            if not found or found.group(3) != before:
                return None
            updated = updated[:found.start(3)] + after + updated[found.end(3):]
        source = source[:tag.start()] + updated + source[tag.end():]
    result = source.encode('utf-8')
    ET.fromstring(result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--readback-dir', type=Path, required=True)
    parser.add_argument('--captured-on', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    manifest_path = root / 'docs/figma-iconset-manifest.json'
    manifest = json.loads(manifest_path.read_text())
    by_id = {icon['nodeId']: icon for icon in manifest['icons']}
    rows = {}
    for path in sorted(args.readback_dir.glob('w*.json')):
        for row in json.loads(path.read_text()):
            if row['id'] in rows and rows[row['id']] != row:
                raise ValueError('Conflicting readbacks for ' + row['id'])
            rows[row['id']] = row
    for path in sorted(args.readback_dir.glob('m*.json')):
        for row in json.loads(path.read_text()):
            previous = rows.get(row['id'])
            if previous and ('svg' in previous or previous.get('status') == 'matched'):
                continue
            rows[row['id']] = row
    geometry_path = args.readback_dir / 'raster-geometry.json'
    geometry_evidence = json.loads(geometry_path.read_text()) if geometry_path.exists() else {}
    prepared, unresolved = [], []
    for node_id, row in rows.items():
        icon = by_id.get(node_id)
        if not icon:
            unresolved.append(node_id)
            continue
        path = root / 'resources/iconset' / icon['filename']
        original = path.read_bytes()
        if digest(original) != icon['sha256']:
            raise ValueError('The icon changed since its manifest: ' + icon['filename'])
        export = None
        if 'svg' in row:
            export = base64.b64decode(row['svg'], validate=True)
            try:
                updated = reconcile(original, export, geometry_evidence.get(node_id, {}))
            except (ET.ParseError, UnicodeError):
                updated = None
        else:
            fingerprint = fingerprint_svg(original, row.get('scale', 100))
            if row.get('status') != 'matched' or not fingerprint or row.get('geometry') != fingerprint['geometry']:
                updated = None
            else:
                try:
                    if row.get('sha') == digest(original):
                        updated = apply_paint_edits(original, row['edits'])
                    else:
                        reverse = [{**edit, 'before': edit['after'], 'after': edit['before']} for edit in row['edits']]
                        restored = apply_paint_edits(original, reverse)
                        updated = original if digest(restored) == row.get('sha') else None
                except (ValueError, KeyError, ET.ParseError, UnicodeError):
                    updated = None
        if updated is None:
            unresolved.append(node_id)
            continue
        prepared.append((icon, path, original, updated, export, row))
    # Both resources were already image-based. Preserve their existing SVG
    # viewport and embed the current Figma 8x render without relying on masks.
    bitmap_path = args.readback_dir / 'bitmaps.json'
    if bitmap_path.exists():
        for bitmap in json.loads(bitmap_path.read_text()):
            if bitmap['id'] not in {'222:11372', '222:11373'}:
                continue
            icon = by_id[bitmap['id']]
            if icon['name'] not in {'smartSelect', 'translateObject'} or icon.get('rasterCompatibility') != 'direct-embedded-image':
                raise ValueError('Unexpected bitmap compatibility target')
            path = root / 'resources/iconset' / icon['filename']
            original = path.read_bytes()
            if digest(original) != icon['sha256']:
                raise ValueError('The icon changed since its manifest: ' + icon['filename'])
            png = base64.b64decode(bitmap['png'], validate=True)
            updated = embedded_png_svg(original, png, icon['width'], icon['height'])
            prepared = [item for item in prepared if item[0]['nodeId'] != bitmap['id']]
            unresolved = [item for item in unresolved if item != bitmap['id']]
            prepared.append((icon, path, original, updated, None,
                             {'bitmapPngSha256': digest(png), 'vars': rows.get(bitmap['id'], {}).get('vars', [])}))
    # A few current vectors cannot be paired path-by-path. Their Figma SVG is
    # still authoritative; retain the established responsive viewport and skip
    # any effect or external reference requiring separate renderer handling.
    still_unresolved = []
    for node_id in unresolved:
        row, icon = rows[node_id], by_id[node_id]
        if 'svg' not in row:
            still_unresolved.append(node_id)
            continue
        path = root / 'resources/iconset' / icon['filename']
        original = path.read_bytes()
        export = base64.b64decode(row['svg'], validate=True)
        try:
            updated = current_vector_svg(original, export)
        except (ValueError, ET.ParseError, UnicodeError):
            still_unresolved.append(node_id)
            continue
        prepared.append((icon, path, original, updated, export, {**row, 'fullVectorExport': True}))
    unresolved = still_unresolved
    fixture_path = root / 'tests/fixtures/figma-contract.json'
    fixture = json.loads(fixture_path.read_text())
    changed_hashes = {'iconset/' + icon['filename']: (digest(original), digest(updated))
                      for icon, _, original, updated, _, _ in prepared if original != updated}
    for asset, previous in fixture.get('assets', {}).items():
        key = unicodedata.normalize('NFKC', asset).casefold()
        matches = [(before, after) for name, (before, after) in changed_hashes.items()
                   if unicodedata.normalize('NFKC', name).casefold() == key]
        if matches:
            if len(matches) != 1 or previous != matches[0][0]:
                raise ValueError('Fixture hash did not match the previous icon: ' + asset)
            fixture['assets'][asset] = matches[0][1]
    for icon, path, original, updated, export, row in prepared:
        if path.read_bytes() != original:
            raise ValueError('The icon changed during reconciliation: ' + icon['filename'])
        if updated != original:
            path.write_bytes(updated)
        icon['sha256'] = digest(updated)
        was_changed = icon.get('colorReadback', {}).get('changed', False)
        evidence = {'capturedOn': args.captured_on, 'variableIds': sorted(set(row.get('vars', []))),
                    'changed': was_changed or updated != original}
        if export is not None:
            evidence['nativeExportSha256'] = digest(export)
            if row.get('fullVectorExport') or icon.get('colorReadback', {}).get('fullVectorExport'):
                evidence['fullVectorExport'] = True
            if icon['nodeId'] in geometry_evidence:
                evidence['geometryRasterScores'] = geometry_evidence[icon['nodeId']]
        elif 'bitmapPngSha256' in row:
            evidence['figmaPngSha256'] = row['bitmapPngSha256']
            evidence['figmaPngScale'] = 8
        else:
            evidence['sourceSha256'] = row['sha']
            evidence['geometryFingerprint'] = row['geometry']
            evidence['geometryScale'] = row.get('scale', 100)
            evidence['paintEdits'] = row['edits']
        icon['colorReadback'] = evidence
        if 'paletteGeometrySha256' in icon:
            geometry = ET.fromstring(updated)
            for element in geometry.iter():
                for key in ('fill', 'stroke', 'stop-color', 'flood-color', 'style'):
                    element.attrib.pop(key, None)
            icon['paletteGeometrySha256'] = digest(ET.tostring(geometry))
    manifest.pop('partialColorReadback', None)
    manifest['colorReadbackCoverage'] = {'capturedOn': args.captured_on, 'captured': len(rows),
        'verified': len(prepared), 'changed': sum(icon['colorReadback']['changed'] for icon, _, _, _, _, _ in prepared),
        'unresolvedNodeIds': sorted(unresolved), 'notCaptured': len(by_id) - len(rows)}
    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n')
    fixture_path.write_text(json.dumps(fixture, ensure_ascii=False, indent=2) + '\n')
    if bitmap_path.exists():
        translate = next((row for row in json.loads(bitmap_path.read_text()) if row['id'] == '222:11373'), None)
        if translate:
            (root / 'tests/fixtures/translate-object-mask.png').write_bytes(base64.b64decode(translate['png'], validate=True))
    print(json.dumps(manifest['colorReadbackCoverage'], ensure_ascii=False))


if __name__ == '__main__':
    main()

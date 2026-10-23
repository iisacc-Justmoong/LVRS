#!/usr/bin/env python3
"""Embed the authored LVRS SVG inventory; build tooling, never app runtime."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import xml.etree.ElementTree as ET

parser=argparse.ArgumentParser()
parser.add_argument('--root',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--manifest',type=Path,required=True)
args=parser.parse_args();root=args.root.resolve()
assets=sorted((root/'resources/iconset').glob('*.svg'))
if not assets: raise SystemExit('No LVRS SVG assets')
table={p.stem:p.stem for p in assets};sources={};manifest=[]
for p in assets:
    svg=p.read_text();tree=ET.fromstring(svg)
    tags={n.tag.rsplit('}',1)[-1] for n in tree.iter()}
    unsupported=sorted(t for t in tags if t in {'filter','script','foreignObject','set'} or t.startswith('animate'))
    if ')LVRS_SVG"' in svg: raise SystemExit('SVG raw-string delimiter collision: '+str(p))
    sources[p.stem]=svg
    manifest.append({'name':p.stem,'path':str(p.relative_to(root)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'unsupported_elements':unsupported})
for node in ET.parse(root/'resources/figma_icon_aliases.qrc').getroot().iter('file'):
    alias=Path(node.attrib.get('alias','')).stem;target=(root/'resources'/node.text).resolve()
    if target.parent!=(root/'resources/iconset').resolve() or target.stem not in sources: raise SystemExit('Invalid icon alias target: '+str(target))
    if alias: table.setdefault(alias,target.stem)
theme=(root/'src/qml/Theme.qml').read_text()
block=re.search(r'iconNameAliases:\s*\(\{([\s\S]*?)\}\)',theme)
if not block: raise SystemExit('Missing Theme icon aliases')
aliases=re.findall(r'"([^"]+)"\s*:\s*"([^"]+)"',block[1])
lines=['// Generated from the authored LVRS SVGs. No runtime file or QML dependency.', '#include <LVRS/Icons.h>','#include <utility>','namespace lvrs::detail {']
for i,(name,svg) in enumerate(sources.items()):
    if len(svg.encode('utf-8'))<=60000:
        lines.append(f'static constexpr std::string_view svg_{i}=R"LVRS_SVG({svg})LVRS_SVG";')
    else:
        # Two authored embedded-image assets exceed the portable 65536-character
        # string-literal limit. A byte array preserves them without compiler extensions.
        raw=svg.encode('utf-8');lines.append(f'static constexpr char svg_{i}_bytes[]={{')
        for begin in range(0,len(raw),64): lines.append(','.join(f'char({b})' for b in raw[begin:begin+64])+',')
        lines.append(f'}};static constexpr std::string_view svg_{i}{{svg_{i}_bytes,sizeof(svg_{i}_bytes)}};')
index={name:i for i,name in enumerate(sources)}
lines+=['std::span<const IconSource> embedded_icons() {','static const IconSource entries[]={']
for name,canonical in sorted(table.items()): lines.append('{'+json.dumps(name)+',svg_'+str(index[canonical])+'},')
lines+=['};return entries;}','std::span<const std::pair<std::string_view,std::string_view>> icon_aliases() {','static constexpr std::pair<std::string_view,std::string_view> entries[]={']
for a,b in aliases:
    if b not in table: raise SystemExit('Unknown Theme icon alias: '+b)
    lines.append('{'+json.dumps(a)+','+json.dumps(b)+'},')
lines+=['};return entries;}','} // namespace lvrs::detail','']
args.output.parent.mkdir(parents=True,exist_ok=True)
args.output.write_text('\n'.join(lines))
args.manifest.write_text(json.dumps({'schema':1,'assets':manifest,'lookup_names':len(table),'theme_aliases':dict(aliases)},ensure_ascii=False,indent=2)+'\n')
print(f'Embedded {len(assets)} SVGs, {len(table)} lookup names, {sum(bool(x["unsupported_elements"]) for x in manifest)} explicitly unsupported SVGs')

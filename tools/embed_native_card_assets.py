"""Embed authored Card SVGs as immutable C++ data; no GUI/runtime execution."""
from pathlib import Path
import argparse, hashlib, json
p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--manifest',type=Path,required=True);a=p.parse_args()
files=sorted((a.root/'resources/images/card').glob('*.svg'))
assert files
lines=['// Generated from authored Card SVGs. Do not edit.','#include <LVRS/Icons.h>','namespace lvrs::detail {']
manifest=[]
for i,path in enumerate(files):
    data=path.read_text();assert ')lvrs_card"' not in data
    lines.append(f'static constexpr char card_{i}[]=R"lvrs_card({data})lvrs_card";')
    manifest.append({'path':str(path.relative_to(a.root)),'name':path.stem,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
lines+=['std::span<const IconSource> embedded_card_icons(){','static constexpr IconSource entries[]={']
lines += [f'{{"{path.stem}",card_{i}}},' for i,path in enumerate(files)]
lines+=['};return entries;}','}','']
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(lines));a.manifest.write_text(json.dumps({'schema':1,'assets':manifest},indent=2)+'\n')

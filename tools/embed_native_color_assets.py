#!/usr/bin/env python3
"""Compile the unmodified wheel and color-action ring PNGs as native data."""
import argparse
import hashlib
import json
from pathlib import Path

parser=argparse.ArgumentParser()
parser.add_argument('--root',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--manifest',type=Path,required=True)
args=parser.parse_args()
assets=[('color_ring_png','resources/images/colorpicker/hue-ring.png',448),
        ('color_button_ring_png','resources/images/color-picker-hue-ring.png',320)]
functions=[]
manifest=[]
for symbol,path,side in assets:
    source=args.root/path
    data=source.read_bytes()
    assert data[:8]==b'\x89PNG\r\n\x1a\n'
    assert int.from_bytes(data[16:20],'big')==side and int.from_bytes(data[20:24],'big')==side
    rows=[','.join(str(b) for b in data[i:i+32]) for i in range(0,len(data),32)]
    functions.append('std::span<const std::uint8_t> '+symbol+'() {\nstatic constexpr std::uint8_t data[]{\n'+',\n'.join(rows)+'\n};return data;\n}\n')
    manifest.append({'source':path,'sha256':hashlib.sha256(data).hexdigest(),
        'width':side,'height':side,'embedded_bytes':len(data),'source_bytes_modified':False})
args.output.parent.mkdir(parents=True,exist_ok=True)
args.output.write_text('#include <span>\n#include <cstdint>\nnamespace lvrs::detail {\n'
    +''.join(functions)+'}\n')
args.manifest.write_text(json.dumps({**manifest[0],'button_ring':manifest[1]},indent=2)+'\n')

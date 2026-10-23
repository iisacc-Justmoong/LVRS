"""Embed the original Alert SVG bytes without changing their authored content."""
from pathlib import Path
import argparse,hashlib,json
p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--manifest',type=Path,required=True);a=p.parse_args()
lines=['#include <string_view>','namespace lvrs::detail {'];rows=[]
for name in ['alert-adjustments','alert-file-text']:
 source=a.root/'resources/images'/(name+'.svg');raw=source.read_bytes();text=raw.decode('utf-8');symbol=name.replace('-','_')
 lines.append('std::string_view '+symbol+'(){return '+json.dumps(text)+';}')
 rows.append({'source':str(source.relative_to(a.root)),'sha256':hashlib.sha256(raw).hexdigest(),'embedded_bytes':len(raw),'source_bytes_modified':False})
lines+=['}'];a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(lines)+'\n');a.manifest.write_text(json.dumps(rows,indent=2)+'\n')

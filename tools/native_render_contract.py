"""Source-pinned RenderMonitor API/default audit, not an executed Qt oracle."""
from pathlib import Path
import argparse,hashlib,json,re
ROOT=Path(__file__).resolve().parents[1]

def audit(root=ROOT):
    spec=json.loads((root/'tests/native/fixtures/render-monitor-contract.json').read_text())
    for file,digest in spec['original_sha256'].items():
        if hashlib.sha256((root/file).read_bytes()).hexdigest()!=digest:raise ValueError('Original render monitor source drift: '+file)
    original=(root/'src/backend/runtime/renderingmonitor.h').read_text()
    properties=re.findall(r'Q_PROPERTY\(\w+\s+(\w+)\s',original)
    methods=re.findall(r'Q_INVOKABLE\s+\w+\s+(\w+)\s*\(',original)
    if sorted(properties)!=sorted(spec['properties']) or sorted(methods)!=sorted(spec['methods']):raise ValueError('Original render monitor API inventory changed')
    native=(root/'src/Monitoring/RenderMonitor.h').read_text()
    body=re.search(r'class\s+RenderMonitor\s+final\s*\{(.*?)^\};',native,re.S|re.M)
    if not body:raise ValueError('Native RenderMonitor class missing')
    for name in properties+methods+spec['setters']:
        if not re.search(r'\b'+name+r'\s*\(',body[1]):raise ValueError('Missing native render monitor API: '+name)
    implementation=(root/'src/Monitoring/RenderMonitor.cpp').read_text()
    for key,value in spec['native_defaults'].items():
        match=re.search(r'\b'+key+r'\s*=\s*(true|false|[0-9]+)',implementation)
        if not match or match[1]!=str(value).lower():raise ValueError('Native render monitor default differs: '+key)
    for key,value in spec['original_defaults'].items():
        match=re.search(r'\b'+key+r'\s*=\s*(true|false|[0-9]+)',original)
        if not match or match[1]!=str(value).lower():raise ValueError('Original render monitor default differs: '+key)
    snapshot=re.search(r'RenderMonitor::performanceSnapshot\(\)const\{(.*?)\n\}',implementation,re.S)
    if not snapshot:raise ValueError('Native render snapshot missing')
    keys=re.findall(r'\{\"([^\"]+)\",V\(',snapshot[1])
    if sorted(keys)!=sorted(spec['snapshot_keys']):raise ValueError('Render snapshot fields changed')
    return {'properties':len(properties),'methods':len(methods),'setters':len(spec['setters']),'defaults':spec['native_defaults'],'original_sha256':spec['original_sha256'],'native_runtime_executed':False,'original_runtime_executed':False,'scope':'Class-scoped declaration/default/source audit; behavioral, visual and platform equivalence require the final runtime batch.'}

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--root',type=Path,default=ROOT);args=parser.parse_args();print(json.dumps(audit(args.root),indent=2))

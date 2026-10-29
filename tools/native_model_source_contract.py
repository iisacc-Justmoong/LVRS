"""Audit original declarations against the explicit native reader contract."""
from pathlib import Path
import argparse, hashlib, json, re

ROOT=Path(__file__).resolve().parents[1]

def audit(root=ROOT):
    spec=json.loads((root/'tests/native/fixtures/model-source-contract.json').read_text())
    for relative,digest in spec['original_sha256'].items():
        if hashlib.sha256((root/relative).read_bytes()).hexdigest()!=digest:
            raise ValueError('Original source drift: '+relative)
    native=(root/'src/Models/ModelSource.h').read_text()
    adapter=(root/'src/Models/ModelAdapter.h').read_text()
    for model,header,original in [('ModelSource',native,'src/backend/model/modelsource.h'),('ModelAdapter',adapter,'src/backend/navigation/modeladapter.h')]:
        declaration=re.search(r'\bclass\s+'+re.escape(model)+r'\s+final\s*\{(.*?)^\};',header,re.S|re.M)
        if not declaration:raise ValueError('Missing native class: '+model)
        members=declaration[1]
        text=(root/original).read_text()
        invokables=re.findall(r'Q_INVOKABLE\s+[\w:<>]+\s+(\w+)\s*\(',text)
        if sorted(invokables)!=sorted(spec[model]['methods']):raise ValueError('Original method coverage changed: '+model)
        properties=re.findall(r'Q_PROPERTY\(\w+\s+(\w+)\s',text)
        if sorted(properties)!=sorted(spec[model]['properties']):raise ValueError('Original property coverage changed: '+model)
        for name in invokables+properties:
            if not re.search(r'\b'+re.escape(name)+r'\s*\(',members):raise ValueError('Missing native API: '+model+'.'+name)
    return {'ModelSource_properties':len(spec['ModelSource']['properties']),
            'ModelSource_methods':len(spec['ModelSource']['methods']),
            'ModelAdapter_methods':len(spec['ModelAdapter']['methods']),
            'original_sha256':spec['original_sha256'],
            'original_runtime_executed':False,'native_runtime_executed':False,
            'scope':'declaration audit; dynamic types and inherited/platform behavior remain partial'}

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--root',type=Path,default=ROOT);args=parser.parse_args()
    print(json.dumps(audit(args.root),indent=2))

if __name__=='__main__':main()

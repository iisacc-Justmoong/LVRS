"""Source-pinned FontPolicy declarations, tokens, substitutions and assets."""
from pathlib import Path
import argparse,hashlib,json,re
ROOT=Path(__file__).resolve().parents[1]

def audit(root=ROOT,gui=None):
    gui=gui or root.parent/'iiGUI'
    spec=json.loads((root/'tests/native/fixtures/font-policy-contract.json').read_text())
    for file,digest in spec['original_sha256'].items():
        if hashlib.sha256((root/file).read_bytes()).hexdigest()!=digest:raise ValueError('Original font policy drift: '+file)
    source=(root/'src/backend/fonts/fontpolicy.h').read_text()
    properties=re.findall(r'Q_PROPERTY\(\w+\s+(\w+)\s',source)
    methods=re.findall(r'Q_INVOKABLE\s+\w+\s+(\w+)\s*\(',source)
    statics=re.findall(r'static\s+(?:bool|void)\s+(\w+)\s*\(',source.split('private:')[0])
    if sorted(properties)!=sorted(spec['properties']) or sorted(methods)!=sorted(spec['methods']) or sorted(statics)!=sorted(spec['statics']):raise ValueError('Original font API inventory changed')
    native=(root/'src/LVRS/FontPolicy.h').read_text();body=re.search(r'class\s+FontPolicy\s+final\s*\{(.*?)^\};',native,re.S|re.M)
    if not body:raise ValueError('Native FontPolicy class missing')
    for name in properties+methods+statics:
        if not re.search(r'\b'+name+r'\s*\(',body[1]):raise ValueError('Missing native font API: '+name)
    tokens=[{'pixels':int(size),'weight':int(weight),'style':style} for size,weight,style in re.findall(r'TextStyleToken\{(\d+),(\d+),"([^"]+)"\}',(gui/'src/iiGUI/Appearance.cpp').read_text())]
    if tokens!=spec['tokens']:raise ValueError('Shared native font tokens differ from the original')
    manifest=json.loads((gui/'resources/lvrs-native-appearance.json').read_text())
    if [font['file'] for font in manifest['fonts']]!=spec['fonts']:raise ValueError('Native font resource inventory differs')
    for font in manifest['fonts']:
        for file in [gui/'resources/native'/font['file'],root/'resources/font'/font['file']]:
            if hashlib.sha256(file.read_bytes()).hexdigest()!=font['sha256']:raise ValueError('Native font bytes differ: '+font['file'])
    implementation=(root/'src/LVRS/FontPolicy.cpp').read_text()
    targets=re.search(r'\btargets\{(.*?)\};',implementation,re.S)[1]
    if re.findall(r'"([^"]+)"',targets)!=spec['substitutions']:raise ValueError('Native font substitution targets differ')
    return {'properties':len(properties),'methods':len(methods),'statics':len(statics),'tokens':len(tokens),'substitutions':len(spec['substitutions']),'fonts':len(manifest['fonts']),'original_sha256':spec['original_sha256'],'native_runtime_executed':False,'original_runtime_executed':False,'scope':'Declaration/data/asset audit; not a complete behavioral or platform oracle'}

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--root',type=Path,default=ROOT);parser.add_argument('--gui',type=Path);args=parser.parse_args();print(json.dumps(audit(args.root,args.gui),indent=2))
if __name__=='__main__':main()

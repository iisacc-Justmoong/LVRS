"""Native LVRS source boundary, never imported by a shipped application."""
from pathlib import Path
import re
import sys
root=Path(sys.argv[1]); errors=[]
for p in (root/'src/LVRS').glob('*'):
    if p.suffix not in {'.h','.cpp'}: continue
    s=p.read_text()
    if re.search(r'^\s*#\s*include\s*[<"](?:Qt|Q[A-Z]|AppKit|Foundation|windows\.h|X11/)',s,re.M): errors.append(f'{p}: foreign/native widget types in framework API')
    if re.search(r'\b(?:Q_OBJECT|QML_NAMED_ELEMENT|QQmlEngine|QQuickItem)\b',s): errors.append(f'{p}: Qt runtime')
    if p.suffix=='.h' and not p.with_suffix('.cpp').exists(): errors.append(f'{p}: missing adjacent implementation')
for p in [root/'cmake/LVRSNative.cmake',root/'cmake/LVRSNativeConfig.cmake.in']:
    if re.search(r'(?:find_package|find_dependency)\s*\(\s*Qt',p.read_text()): errors.append(f'{p}: Qt dependency')
if errors: raise SystemExit('\n'.join(errors))
print('LVRS native API: C++23 + iiGUI; no Qt, OS controls or separate runtime')

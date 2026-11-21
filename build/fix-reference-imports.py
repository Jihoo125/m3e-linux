#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parent.parent
for path in (root / 'build/reference-js').rglob('*.js'):
    text = path.read_text(encoding='utf-8')
    import re
    text = re.sub(r"(from\s+['\"])(\.[^'\"]*?)(['\"])", lambda m: m.group(1)+m.group(2)+'.js'+m.group(3) if not m.group(2).endswith('.js') else m.group(0), text)
    path.write_text(text)

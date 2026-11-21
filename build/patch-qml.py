#!/usr/bin/env python3
from pathlib import Path
p = Path(__file__).resolve().parent.parent / "build/qml/qml/Materialized/qmldir"
s = p.read_text()
s = s.replace('optional plugin m3plugin', 'module Materialized\noptional plugin m3plugin')
p.write_text(s)
print(s)

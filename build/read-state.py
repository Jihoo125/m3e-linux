#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parent.parent
for path in [root / 'libs/libm3-qt/qml/M3StateLayer.qml', root / 'build/qml/qml/Materialized/M3StateLayer.qml']:
    print('===', path)
    print(path.read_text())

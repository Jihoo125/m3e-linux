#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parent.parent
fs = [
    root / 'libs/libm3-qt/qml/M3StateLayer.qml',
    root / 'libs/libm3-qt/qml/M3TextField.qml',
    root / 'installer/matinstall-core/src/installer.cpp',
    root / 'installer/matinstall-core/src/process.cpp',
]
for path in fs:
    print('\n===', path.relative_to(root))
    print(path.read_text()[:6000])

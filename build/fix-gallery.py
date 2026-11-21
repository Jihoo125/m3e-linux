#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parent.parent
for path in (root / 'libs/libm3-qt/qml').glob('M3*.qml'):
    text = path.read_text()
    if 'Materialized as M3' not in text:
        continue
    # Keep the module import for the name. Components live at the module root
    # in resources, avoiding a duplicated generated directory in Qt 6.2.
    pass
# For development a dedicated load directory is created in main.cpp.

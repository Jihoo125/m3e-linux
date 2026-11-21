#!/usr/bin/env python3
from __future__ import annotations
import sys
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent
p = ROOT / "libs/libm3-qt/CMakeLists.txt"
s = p.read_text()
s = s.replace('    URI Materialized\n', '    URI Materialized\n    NO_GENERATE_PLUGIN_SOURCE\n')
s = s.replace('    URI Materialized\n    NO_GENERATE_PLUGIN_SOURCE\n    NO_GENERATE_PLUGIN_SOURCE\n', '    URI Materialized\n    NO_GENERATE_PLUGIN_SOURCE\n')
if 'NO_CREATE_PLUGIN_TARGET' in s:
    print('already disabled generated plugin target')
else:
    s = s.replace('    NO_GENERATE_QMLDIR\n', '    NO_GENERATE_QMLDIR\n    NO_CREATE_PLUGIN_TARGET\n')
p.write_text(s)

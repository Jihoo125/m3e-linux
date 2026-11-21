#!/usr/bin/env python3
import material_color_utilities as m
for name in ['Hct', 'DynamicScheme', 'TonalPalette', 'Score', 'Variant']:
    obj = getattr(m, name, None)
    print(name, [x for x in dir(obj) if not x.startswith('_')] if obj else None)
print(m.__file__)
print('ARGB', m.argb_from_hex('#6750A4'))

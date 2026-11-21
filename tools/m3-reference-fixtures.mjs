import { SchemeTonalSpot } from './scheme/scheme_tonal_spot.js';
import { SchemeNeutral } from './scheme/scheme_neutral.js';
import { SchemeVibrant } from './scheme/scheme_vibrant.js';
import { SchemeExpressive } from './scheme/scheme_expressive.js';
import { SchemeFidelity } from './scheme/scheme_fidelity.js';
import { SchemeContent } from './scheme/scheme_content.js';
import { SchemeMonochrome } from './scheme/scheme_monochrome.js';
import { SchemeRainbow } from './scheme/scheme_rainbow.js';
import { SchemeFruitSalad } from './scheme/scheme_fruit_salad.js';
import { Hct } from './hct/hct.js';

const variants = [SchemeMonochrome, SchemeNeutral, SchemeTonalSpot, SchemeVibrant, SchemeExpressive, SchemeFidelity, SchemeContent, SchemeRainbow, SchemeFruitSalad];
const result = {};
for (const argb of [0xff000000, 0xffffffff, 0xff6750a4, 0xff0000ff, 0xffff0000, 0xff00ff00, 0xffffc107, 0xff4c662b, 0xff00639a, 0xff7f7f7f, 0xff123456, 0xffffd8e4]) {
  const hct = Hct.fromInt(argb);
  let item = { hct: { hue: hct.hue, chroma: hct.chroma, tone: hct.tone }, schemes: {} };
  for (let v = 0; v < variants.length; ++v) {
    for (const dark of [false, true]) {
      const scheme = new variants[v](hct, dark, 0, '2021', 'phone');
      const roles = {};
      for (const role in scheme) {
        if (typeof scheme[role] === 'number') roles[role] = scheme[role];
      }
      const key = `${v}:${dark ? 1 : 0}`;
      item.schemes[key] = roles;
    }
  }
  result[argb >>> 0] = item;
}
console.log(JSON.stringify(result, null, 2));

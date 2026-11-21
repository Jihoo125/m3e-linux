#include "m3/palettes.hpp"
#include "m3/color.hpp"
#include "m3/math.hpp"

namespace m3 {

Hct TonalPalette::createKeyColor(double hue, double requestedChroma) {
  int lowerTone = 0, upperTone = 100;
  std::map<int, double> chromaCache;
  auto maxChroma = [&](int tone) {
    auto it = chromaCache.find(tone);
    if (it != chromaCache.end()) return it->second;
    const double c = Hct::from(hue, 200.0, tone).chroma;
    chromaCache[tone] = c;
    return c;
  };
  while (lowerTone < upperTone) {
    const int midTone = (lowerTone + upperTone) / 2;
    const bool ascending = maxChroma(midTone) < maxChroma(midTone + 1);
    if (maxChroma(midTone) >= requestedChroma - 0.01) {
      if (std::abs(lowerTone - 50) < std::abs(upperTone - 50)) {
        upperTone = midTone;
      } else {
        if (lowerTone == midTone) return Hct::from(hue, requestedChroma, lowerTone);
        lowerTone = midTone;
      }
    } else if (ascending) {
      lowerTone = midTone + 1;
    } else {
      upperTone = midTone;
    }
  }
  return Hct::from(hue, requestedChroma, lowerTone);
}
TonalPalette::TonalPalette(double h, double c, Hct key)
    : hue_(h), chroma_(c), keyColor_(key) {}
TonalPalette TonalPalette::fromInt(uint32_t argb) {
  return fromHct(Hct::fromInt(argb));
}
TonalPalette TonalPalette::fromHct(const Hct& hct) {
  return TonalPalette(hct.hue, hct.chroma, hct);
}
TonalPalette TonalPalette::fromHueAndChroma(double hue, double chroma) {
  return TonalPalette(hue, chroma, createKeyColor(hue, chroma));
}
uint32_t TonalPalette::tone(double t) const {
  auto it = cache_.find(t);
  if (it != cache_.end()) return it->second;
  uint32_t argb;
  if (t == 99 && Hct::isYellow(hue_)) {
    const uint32_t a = tone(98), b = tone(100);
    argb = argbFromRgb(static_cast<int>(std::round((redFromArgb(a) + redFromArgb(b)) / 2.0)),
                       static_cast<int>(std::round((greenFromArgb(a) + greenFromArgb(b)) / 2.0)),
                       static_cast<int>(std::round((blueFromArgb(a) + blueFromArgb(b)) / 2.0)));
  } else {
    argb = Hct::from(hue_, chroma_, t).toInt();
  }
  cache_[t] = argb;
  return argb;
}
Hct TonalPalette::getHct(double t) const { return Hct::fromInt(tone(t)); }

CorePalette CorePalette::of(uint32_t argb) {
  const Hct hct = Hct::fromInt(argb);
  const double h = hct.hue, c = hct.chroma;
  return {TonalPalette::fromHueAndChroma(h, std::max(48.0, c)),
          TonalPalette::fromHueAndChroma(h, 16),
          TonalPalette::fromHueAndChroma(h + 60, 24),
          TonalPalette::fromHueAndChroma(h, 4),
          TonalPalette::fromHueAndChroma(h, 8),
          TonalPalette::fromHueAndChroma(25, 84)};
}
CorePalette CorePalette::contentOf(uint32_t argb) {
  const Hct hct = Hct::fromInt(argb);
  const double h = hct.hue, c = hct.chroma;
  return {TonalPalette::fromHueAndChroma(h, c),
          TonalPalette::fromHueAndChroma(h, c / 3),
          TonalPalette::fromHueAndChroma(h + 60, c / 2),
          TonalPalette::fromHueAndChroma(h, std::min(c / 12, 4.0)),
          TonalPalette::fromHueAndChroma(h, std::min(c / 6, 8.0)),
          TonalPalette::fromHueAndChroma(25, 84)};
}

}  // namespace m3

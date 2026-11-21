#pragma once

#include "m3/hct.hpp"
#include <cstdint>
#include <map>

namespace m3 {

class TonalPalette {
 public:
  static TonalPalette fromInt(uint32_t argb);
  static TonalPalette fromHct(const Hct& hct);
  static TonalPalette fromHueAndChroma(double hue, double chroma);
  static TonalPalette FromInt(uint32_t argb) { return fromInt(argb); }
  static TonalPalette FromHct(const Hct& hct) { return fromHct(hct); }

  uint32_t tone(double tone) const;
  uint32_t Tone(int tone) const { return this->tone(tone); }
  Hct getHct(double tone) const;
  double hue() const { return hue_; }
  double chroma() const { return chroma_; }
  const Hct& keyColor() const { return keyColor_; }

 private:
  TonalPalette(double hue, double chroma, Hct keyColor);
  static Hct createKeyColor(double hue, double chroma);
  double hue_;
  double chroma_;
  Hct keyColor_;
  mutable std::map<double, uint32_t> cache_;
};

struct CorePalette {
  TonalPalette a1;
  TonalPalette a2;
  TonalPalette a3;
  TonalPalette n1;
  TonalPalette n2;
  TonalPalette error;

  static CorePalette of(uint32_t argb);
  static CorePalette contentOf(uint32_t argb);
};

}  // namespace m3

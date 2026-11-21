#pragma once

#include <cstdint>

namespace m3 {

class Hct {
 public:
  double hue = 0;
  double chroma = 0;
  double tone = 0;

  static Hct from(double hue, double chroma, double tone) {
    return fromHueChromaTone(hue, chroma, tone);
  }
  static Hct fromHueChromaTone(double hue, double chroma, double tone);
  static Hct fromInt(uint32_t argb);
  static Hct From(uint32_t argb) { return fromInt(argb); }
  uint32_t toInt() const { return argb_; }
  uint32_t ToArgb() const { return toInt(); }
  void setHue(double newHue);
  void setChroma(double newChroma);
  void setTone(double newTone);
  static bool isBlue(double hue) { return hue >= 250 && hue < 270; }
  static bool isYellow(double hue) { return hue >= 105 && hue < 125; }
  static bool isCyan(double hue) { return hue >= 170 && hue < 207; }

 private:
  explicit Hct(uint32_t argb);
  void setInternalState(uint32_t argb);
  uint32_t argb_ = 0;
};

uint32_t solveHct(double hueDegrees, double chroma, double lstar);

}  // namespace m3

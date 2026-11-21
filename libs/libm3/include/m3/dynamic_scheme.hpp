#pragma once

#include "m3/palettes.hpp"
#include <cstdint>
#include <map>
#include <string>

namespace m3 {

enum class Variant {
  kMonochrome = 0,
  kNeutral = 1,
  kTonalSpot = 2,
  kVibrant = 3,
  kExpressive = 4,
  kFidelity = 5,
  kContent = 6,
  kRainbow = 7,
  kFruitSalad = 8,
  // Upper-case aliases match material-color-utilities' enum spelling.
  MONOCHROME = kMonochrome,
  NEUTRAL = kNeutral,
  TONAL_SPOT = kTonalSpot,
  VIBRANT = kVibrant,
  EXPRESSIVE = kExpressive,
  FIDELITY = kFidelity,
  CONTENT = kContent,
  RAINBOW = kRainbow,
  FRUIT_SALAD = kFruitSalad,
};

struct DynamicScheme {
  Hct sourceColorHct;
  uint32_t sourceColorArgb;
  Variant variant;
  double contrastLevel;
  bool isDark;
  TonalPalette primaryPalette;
  TonalPalette secondaryPalette;
  TonalPalette tertiaryPalette;
  TonalPalette neutralPalette;
  TonalPalette neutralVariantPalette;
  TonalPalette errorPalette;

  DynamicScheme(Hct source, Variant variant, bool isDark,
                double contrastLevel = 0.0);
  std::map<std::string, uint32_t> roles() const;
  uint32_t getArgb(const std::string& role) const;
};

std::map<std::string, uint32_t> schemeFromArgb(
    uint32_t seed, bool dark, Variant variant,
    double contrast = 0.0);

}  // namespace m3

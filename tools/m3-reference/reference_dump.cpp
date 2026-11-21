#include <iomanip>
#include <iostream>
#include <string>

#include "cpp/cam/hct.h"
#include "cpp/scheme/scheme_content.h"
#include "cpp/scheme/scheme_expressive.h"
#include "cpp/scheme/scheme_fidelity.h"
#include "cpp/scheme/scheme_fruit_salad.h"
#include "cpp/scheme/scheme_monochrome.h"
#include "cpp/scheme/scheme_neutral.h"
#include "cpp/scheme/scheme_rainbow.h"
#include "cpp/scheme/scheme_tonal_spot.h"
#include "cpp/scheme/scheme_vibrant.h"
#include "cpp/utils/utils.h"

using namespace material_color_utilities;

int main(int argc, char** argv) {
  Argb seed = argc > 1 ? static_cast<Argb>(std::stoul(argv[1], nullptr, 16)) : 0xff6750a4;
  const int variant = argc > 2 ? std::stoi(argv[2]) : 2;
  const bool is_dark = argc > 3 && std::string_view(argv[3]) == "dark";
  const Hct hct(seed);
  std::cout << std::fixed << std::setprecision(9);
  std::cout << hct.get_hue() << " " << hct.get_chroma() << " " << hct.get_tone() << "\n";
  auto dump = [&](const DynamicScheme& s) {
#define OUTPUT(name) std::cout << #name << " " << HexFromArgb(s.Get##name()) << "\n"
    OUTPUT(Primary); OUTPUT(OnPrimary); OUTPUT(PrimaryContainer); OUTPUT(OnPrimaryContainer);
    OUTPUT(Secondary); OUTPUT(OnSecondary); OUTPUT(SecondaryContainer); OUTPUT(OnSecondaryContainer);
    OUTPUT(Tertiary); OUTPUT(OnTertiary); OUTPUT(TertiaryContainer); OUTPUT(OnTertiaryContainer);
    OUTPUT(Error); OUTPUT(OnError); OUTPUT(ErrorContainer); OUTPUT(OnErrorContainer);
    OUTPUT(Background); OUTPUT(OnBackground); OUTPUT(Surface); OUTPUT(OnSurface);
    OUTPUT(SurfaceVariant); OUTPUT(OnSurfaceVariant); OUTPUT(SurfaceDim); OUTPUT(SurfaceBright);
    OUTPUT(SurfaceContainerLowest); OUTPUT(SurfaceContainerLow); OUTPUT(SurfaceContainer);
    OUTPUT(SurfaceContainerHigh); OUTPUT(SurfaceContainerHighest); OUTPUT(InverseSurface);
    OUTPUT(InverseOnSurface); OUTPUT(InversePrimary); OUTPUT(Outline); OUTPUT(OutlineVariant);
    OUTPUT(Shadow); OUTPUT(Scrim); OUTPUT(SurfaceTint);
    OUTPUT(PrimaryFixed); OUTPUT(PrimaryFixedDim); OUTPUT(OnPrimaryFixed); OUTPUT(OnPrimaryFixedVariant);
    OUTPUT(SecondaryFixed); OUTPUT(SecondaryFixedDim); OUTPUT(OnSecondaryFixed); OUTPUT(OnSecondaryFixedVariant);
    OUTPUT(TertiaryFixed); OUTPUT(TertiaryFixedDim); OUTPUT(OnTertiaryFixed); OUTPUT(OnTertiaryFixedVariant);
#undef OUTPUT
  };
  switch (static_cast<Variant>(variant)) {
    case Variant::kMonochrome: dump(SchemeMonochrome(hct, is_dark, 0)); break;
    case Variant::kNeutral: dump(SchemeNeutral(hct, is_dark, 0)); break;
    case Variant::kTonalSpot: dump(SchemeTonalSpot(hct, is_dark, 0)); break;
    case Variant::kVibrant: dump(SchemeVibrant(hct, is_dark, 0)); break;
    case Variant::kExpressive: dump(SchemeExpressive(hct, is_dark, 0)); break;
    case Variant::kFidelity: dump(SchemeFidelity(hct, is_dark, 0)); break;
    case Variant::kContent: dump(SchemeContent(hct, is_dark, 0)); break;
    case Variant::kRainbow: dump(SchemeRainbow(hct, is_dark, 0)); break;
    case Variant::kFruitSalad: dump(SchemeFruitSalad(hct, is_dark, 0)); break;
  }
}

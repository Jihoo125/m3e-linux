#include <iomanip>
#include <iostream>
#include "cpp/cam/hct.h"
#define private public
#include "cpp/temperature/temperature_cache.h"
#undef private

using namespace material_color_utilities;
int main(int argc, char **argv) {
    const Argb seed = argc > 1 ? static_cast<Argb>(std::stoul(argv[1], nullptr, 16)) : 0xff6750a4;
    TemperatureCache cache{Hct(seed)};
    auto dump = [](const char *name, Hct color) {
        std::cout << name << " " << std::setprecision(15) << color.get_hue() << " "
                  << color.get_chroma() << " " << color.get_tone() << " " << std::hex << color.ToInt() << std::dec << "\n";
    };
    dump("input", Hct(seed));
    dump("warmest", cache.GetWarmest());
    dump("coldest", cache.GetColdest());
    dump("complement", cache.GetComplement());
    for (const auto &color : cache.GetAnalogousColors(3, 6)) dump("analogous", color);
    const auto& by_hue = cache.GetHctsByHue();
    for (int h = 0; h < 360; h += 30) dump("hue", by_hue[h]);
}

#include <iomanip>
#include <iostream>
#include "m3/m3.hpp"

int main(int argc, char **argv) {
    const uint32_t seed = argc > 1 ? static_cast<uint32_t>(std::stoul(argv[1], nullptr, 16)) : 0xff6750a4;
    m3::TemperatureCache cache(m3::Hct::fromInt(seed));
    auto dump = [](const char *name, const m3::Hct &color) {
        std::cout << name << " " << std::setprecision(15) << color.hue << " "
                  << color.chroma << " " << color.tone << " " << std::hex << color.toInt() << std::dec << "\n";
    };
    dump("input", m3::Hct::fromInt(seed));
    dump("warmest", cache.warmest());
    dump("coldest", cache.coldest());
    dump("complement", cache.complement());
    for (const auto &color : cache.analogous(3, 6)) dump("analogous", color);
    const auto& by_hue = cache.hctsByHue();
    for (int h = 0; h < 360; h += 30) dump("hue", by_hue[h]);
}

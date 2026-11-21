#include <iomanip>
#include <iostream>
#include <string>
#include "m3/m3.hpp"

int main(int argc, char **argv)
{
    std::string source = argc > 1 ? argv[1] : "#6750A4";
    if (source[0] == '#') source.erase(0, 1);
    const uint32_t rgb = static_cast<uint32_t>(std::stoul(source, nullptr, 16));
    const bool dark = argc > 2 && std::string(argv[2]) == "dark";
    const m3::Variant variant = argc > 3 ? static_cast<m3::Variant>(std::stoi(argv[3])) : m3::Variant::kTonalSpot;
    const auto scheme = m3::schemeFromArgb(0xff000000u | rgb, dark, variant);
    const auto hct = m3::Hct::fromInt(0xff000000u | rgb);
    std::cout << std::fixed << std::setprecision(9);
    std::cout << "HCT " << hct.hue << " " << hct.chroma << " " << hct.tone << "\n";
    for (const auto &[name, argb] : scheme)
        std::cout << name << " #" << std::hex << std::setw(8) << std::setfill('0') << argb << std::dec << "\n";
    return 0;
}

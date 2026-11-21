#pragma once

#include <cstdint>
#include <map>
#include <vector>

namespace m3 {

class QuantizerWu {
 public:
  std::vector<uint32_t> quantize(const std::vector<uint32_t>& pixels,
                                 int maxColors);
};

std::map<uint32_t, uint32_t> quantizeCelebi(
    const std::vector<uint32_t>& pixels, int maxColors = 128);
std::vector<uint32_t> quantizeWu(
    const std::vector<uint32_t>& pixels, int maxColors = 128);
std::map<uint32_t, uint32_t> quantizeWsmeans(
    const std::vector<uint32_t>& pixels,
    const std::vector<uint32_t>& startingClusters = {}, int maxColors = 128);

inline std::vector<uint32_t> QuantizeCelebi(
    const std::vector<uint32_t>& pixels, int count = 128) {
  std::vector<uint32_t> result;
  for (const auto& [argb, population] : quantizeCelebi(pixels, count)) {
    (void)population;
    result.push_back(argb);
  }
  return result;
}

}  // namespace m3

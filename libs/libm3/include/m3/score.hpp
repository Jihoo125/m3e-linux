#pragma once

#include <cstdint>
#include <map>
#include <vector>

namespace m3 {

struct ScoreOptions {
  int desired = 4;
  uint32_t fallbackColorArgb = 0xff4285f4;
  bool filter = true;
};

std::vector<uint32_t> score(const std::map<uint32_t, double>& colorPopulation,
                            ScoreOptions options = {});
inline std::vector<uint32_t> Score(
    const std::map<uint32_t, double>& colorPopulation) {
  return score(colorPopulation);
}
uint32_t sourceColorFromImage(const std::vector<uint32_t>& pixels);

}  // namespace m3

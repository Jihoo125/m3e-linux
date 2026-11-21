#pragma once

#include "m3/hct.hpp"
#include <optional>
#include <vector>

namespace m3 {

class TemperatureCache {
 public:
  explicit TemperatureCache(const Hct& input);
  const std::vector<Hct>& hctsByTemp();
  const std::vector<Hct>& hctsByHue();
  Hct warmest();
  Hct coldest();
  Hct complement();
  double relativeTemperature(const Hct& hct) const;
  double inputRelativeTemperature();
  std::vector<Hct> analogous(int count = 5, int divisions = 12);
  static double rawTemperature(const Hct& color);
  static bool isBetween(double angle, double a, double b);

 private:
  Hct input_;
  std::vector<Hct> hctsByHue_;
  std::vector<Hct> hctsByTemp_;
  std::vector<double> tempsByHue_;
  double inputTemperature_ = 0;
  Hct coldest_{Hct::fromInt(0xff000000)};
  Hct warmest_{Hct::fromInt(0xff000000)};
  std::optional<Hct> complementCache_;
  bool loaded_ = false;
  double inputRelativeTemperatureCache_ = -1;
  void load();
};

Hct fixIfDisliked(const Hct& hct);
bool isDisliked(const Hct& hct);

}  // namespace m3

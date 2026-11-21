#include "m3/temperature.hpp"
#include "m3/color.hpp"
#include "m3/math.hpp"
#include <cmath>
#include "m3/color.hpp"

#include <algorithm>

namespace m3 {

TemperatureCache::TemperatureCache(const Hct& input) : input_(input) {}
const std::vector<Hct>& TemperatureCache::hctsByHue() {
  if (hctsByHue_.empty()) {
    hctsByHue_.reserve(361);
    for (int hue = 0; hue <= 360; ++hue)
      hctsByHue_.push_back(Hct::from(hue, input_.chroma, input_.tone));
  }
  return hctsByHue_;
}
void TemperatureCache::load() {
  if (loaded_) return;
  const auto& hues = hctsByHue();
  std::vector<std::pair<double, size_t>> sorted;
  sorted.reserve(hues.size() + 1);
  for (size_t i = 0; i < hues.size(); ++i) {
    tempsByHue_.push_back(rawTemperature(hues[i]));
    sorted.emplace_back(tempsByHue_.back(), i);
  }
  inputTemperature_ = rawTemperature(input_);
  sorted.emplace_back(inputTemperature_, hues.size());
  std::stable_sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
    return a.first < b.first;
  });
  hctsByTemp_.reserve(hues.size() + 1);
  for (const auto& [temp, index] : sorted)
    hctsByTemp_.push_back(index == hues.size() ? input_ : hues[index]);
  coldest_ = hctsByTemp_.front();
  warmest_ = hctsByTemp_.back();
  loaded_ = true;
}
const std::vector<Hct>& TemperatureCache::hctsByTemp() {
  load();
  return hctsByTemp_;
}
Hct TemperatureCache::warmest() { load(); return warmest_; }
Hct TemperatureCache::coldest() { load(); return coldest_; }
double TemperatureCache::relativeTemperature(const Hct& hct) const {
  const double range = rawTemperature(warmest_) - rawTemperature(coldest_);
  if (range == 0) return 0.5;
  return (rawTemperature(hct) - rawTemperature(coldest_)) / range;
}
double TemperatureCache::inputRelativeTemperature() {
  load();
  if (inputRelativeTemperatureCache_ < 0)
    inputRelativeTemperatureCache_ = relativeTemperature(input_);
  return inputRelativeTemperatureCache_;
}
bool TemperatureCache::isBetween(double angle, double a, double b) {
  return a < b ? a <= angle && angle <= b : a <= angle || angle <= b;
}
double TemperatureCache::rawTemperature(const Hct& color) {
  const auto lab = labFromArgb(color.toInt());
  const double hue = sanitizeDegreesDouble(std::atan2(lab[2], lab[1]) * 180 /
                                          std::acos(-1.0));
  const double chroma = std::sqrt(lab[1] * lab[1] + lab[2] * lab[2]);
  return -0.5 + 0.02 * std::pow(chroma, 1.07) *
      std::cos(sanitizeDegreesDouble(hue - 50) * std::acos(-1.0) / 180.0);
}
Hct TemperatureCache::complement() {
  if (complementCache_) return *complementCache_;
  load();
  const double coldestHue = coldest_.hue;
  const double coldestTemp = rawTemperature(coldest_);
  const double warmestHue = warmest_.hue;
  const double warmestTemp = rawTemperature(warmest_);
  const double range = warmestTemp - coldestTemp;
  const bool startBetween = isBetween(input_.hue, coldestHue, warmestHue);
  const double startHue = startBetween ? warmestHue : coldestHue;
  const double endHue = startBetween ? coldestHue : warmestHue;
  double smallestError = 1000;
  Hct answer = hctsByHue()[static_cast<int>(std::round(input_.hue)) % 360];
  const double complementRelativeTemp = 1.0 - inputRelativeTemperature();
  for (double addend = 0; addend <= 360; ++addend) {
    const double hue = sanitizeDegreesDouble(startHue + addend);
    if (!isBetween(hue, startHue, endHue)) continue;
    const int index = static_cast<int>(std::round(hue)) % 360;
    const Hct possible = hctsByHue()[index];
    const double relative = (rawTemperature(possible) - coldestTemp) / range;
    const double error = std::abs(complementRelativeTemp - relative);
    if (error < smallestError) {
      smallestError = error;
      answer = possible;
    }
  }
  complementCache_ = answer;
  return answer;
}
std::vector<Hct> TemperatureCache::analogous(int count, int divisions) {
  load();
  if (count <= 0 || divisions <= 0) return {};
  const int startHue = static_cast<int>(std::round(input_.hue)) % 360;
  const auto& hues = hctsByHue();
  const Hct startHct = hues[startHue];
  double lastTemp = relativeTemperature(startHct);
  std::vector<Hct> allColors{startHct};
  double absoluteTotal = 0;
  for (int i = 0; i < 360; ++i) {
    const int hue = sanitizeDegreesInt(startHue + i);
    const double temp = relativeTemperature(hues[hue]);
    absoluteTotal += std::abs(temp - lastTemp);
    lastTemp = temp;
  }
  int hueAddend = 1;
  const double tempStep = absoluteTotal / divisions;
  double totalTempDelta = 0;
  lastTemp = relativeTemperature(startHct);
  while (static_cast<int>(allColors.size()) < divisions) {
    const int hue = sanitizeDegreesInt(startHue + hueAddend);
    const Hct hct = hues[hue];
    const double temp = relativeTemperature(hct);
    const double tempDelta = std::abs(temp - lastTemp);
    totalTempDelta += tempDelta;
    bool satisfied = totalTempDelta >= allColors.size() * tempStep;
    int indexAddend = 1;
    while (satisfied && static_cast<int>(allColors.size()) < divisions) {
      allColors.push_back(hct);
      satisfied = totalTempDelta >= (allColors.size() + indexAddend) * tempStep;
      ++indexAddend;
    }
    lastTemp = temp;
    ++hueAddend;
    if (hueAddend > 360) {
      while (static_cast<int>(allColors.size()) < divisions) allColors.push_back(hct);
      break;
    }
  }
  std::vector<Hct> answers{input_};
  const int increaseCount = (count - 1) / 2;
  auto at = [&](int index) {
    index %= static_cast<int>(allColors.size());
    if (index < 0) index += static_cast<int>(allColors.size());
    return allColors[index];
  };
  for (int i = 1; i <= increaseCount; ++i)
    answers.insert(answers.begin(), at(-i));
  for (int i = 1; i <= count - increaseCount - 1; ++i)
    answers.push_back(at(i));
  return answers;
}

bool isDisliked(const Hct& hct) {
  return std::round(hct.hue) >= 90 && std::round(hct.hue) <= 111 &&
         std::round(hct.chroma) > 16 && std::round(hct.tone) < 65;
}
Hct fixIfDisliked(const Hct& hct) {
  return isDisliked(hct) ? Hct::from(hct.hue, hct.chroma, 70) : hct;
}

}  // namespace m3

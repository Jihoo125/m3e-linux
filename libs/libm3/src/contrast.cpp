#include "m3/contrast.hpp"
#include "m3/color.hpp"
#include "m3/math.hpp"

#include <cmath>

namespace m3 {

double ratioOfYs(double y1, double y2) {
  const double lighter = y1 > y2 ? y1 : y2;
  const double darker = lighter == y2 ? y1 : y2;
  return (lighter + 5.0) / (darker + 5.0);
}
double ratioOfTones(double a, double b) {
  return ratioOfYs(yFromLstar(clampDouble(0, 100, a)),
                   yFromLstar(clampDouble(0, 100, b)));
}
double contrastRatio(double a, double b) { return ratioOfTones(a, b); }
double contrastRatioOfArgb(uint32_t a, uint32_t b) {
  return ratioOfYs(xyzFromArgb(a)[1], xyzFromArgb(b)[1]);
}
double lighter(double tone, double ratio) {
  if (tone < 0 || tone > 100) return -1;
  const double darkY = yFromLstar(tone);
  const double lightY = ratio * (darkY + 5) - 5;
  const double actual = ratioOfYs(lightY, darkY);
  if (actual < ratio && std::abs(actual - ratio) > 0.04) return -1;
  const double result = lstarFromY(lightY) + 0.4;
  return result < 0 || result > 100 ? -1 : result;
}
double darker(double tone, double ratio) {
  if (tone < 0 || tone > 100) return -1;
  const double lightY = yFromLstar(tone);
  const double darkY = (lightY + 5) / ratio - 5;
  const double actual = ratioOfYs(lightY, darkY);
  if (actual < ratio && std::abs(actual - ratio) > 0.04) return -1;
  const double result = lstarFromY(darkY) - 0.4;
  return result < 0 || result > 100 ? -1 : result;
}
double lighterUnsafe(double t, double r) {
  const double result = lighter(t, r);
  return result < 0 ? 100 : result;
}
double darkerUnsafe(double t, double r) {
  const double result = darker(t, r);
  return result < 0 ? 0 : result;
}
bool tonePrefersLightForeground(double tone) { return std::round(tone) < 60; }
bool toneAllowsLightForeground(double tone) { return std::round(tone) <= 49; }
double enableLightForeground(double tone) {
  return tonePrefersLightForeground(tone) && !toneAllowsLightForeground(tone)
             ? 49 : tone;
}
double foregroundTone(double bgTone, double ratio) {
  const double lightTone = lighterUnsafe(bgTone, ratio);
  const double darkTone = darkerUnsafe(bgTone, ratio);
  const double lightRatio = ratioOfTones(lightTone, bgTone);
  const double darkRatio = ratioOfTones(darkTone, bgTone);
  if (tonePrefersLightForeground(bgTone)) {
    const bool negligible = std::abs(lightRatio - darkRatio) < 0.1 &&
                            lightRatio < ratio && darkRatio < ratio;
    return lightRatio >= ratio || lightRatio >= darkRatio || negligible
               ? lightTone : darkTone;
  }
  return darkRatio >= ratio || darkRatio >= lightRatio ? darkTone : lightTone;
}

}  // namespace m3

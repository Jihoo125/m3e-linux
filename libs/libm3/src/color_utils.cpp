// Implements the numerical algorithms in material-color-utilities (Apache-2.0).
#include "m3/color.hpp"

#include "m3/math.hpp"

#include <cstdio>
#include <cmath>

namespace m3 {
namespace {
constexpr double kSrgbToXyz[3][3] = {
    {0.41233895, 0.35762064, 0.18051042},
    {0.2126, 0.7152, 0.0722},
    {0.01932141, 0.11916382, 0.95034478}};
constexpr double kXyzToSrgb[3][3] = {
    {3.2413774792388685, -1.5376652402851851, -0.49885366846268053},
    {-0.9691452513005321, 1.8758853451067872, 0.04156585616912061},
    {0.05562093689691305, -0.20395524564742123, 1.0571799111220335}};
double labF(double t) {
  constexpr double e = 216.0 / 24389.0;
  constexpr double kappa = 24389.0 / 27.0;
  return t > e ? std::cbrt(t) : (kappa * t + 16) / 116;
}
double labInvF(double t) {
  constexpr double e = 216.0 / 24389.0;
  constexpr double kappa = 24389.0 / 27.0;
  const double t3 = t * t * t;
  return t3 > e ? t3 : (116 * t - 16) / kappa;
}
}  // namespace

double sanitizeDegreesDouble(double degrees) {
  degrees = std::fmod(degrees, 360.0);
  return degrees < 0 ? degrees + 360 : degrees;
}
double differenceDegrees(double a, double b) {
  return 180.0 - std::abs(std::abs(a - b) - 180.0);
}
double signum(double value) {
  return value < 0 ? -1 : value > 0 ? 1 : 0;
}
uint32_t argbFromRgb(int r, int g, int b) {
  return 0xff000000u | (uint32_t(clampInt(0, 255, r)) << 16) |
         (uint32_t(clampInt(0, 255, g)) << 8) | uint32_t(clampInt(0, 255, b));
}
int alphaFromArgb(uint32_t a) { return int(a >> 24 & 255); }
int redFromArgb(uint32_t a) { return int(a >> 16 & 255); }
int greenFromArgb(uint32_t a) { return int(a >> 8 & 255); }
int blueFromArgb(uint32_t a) { return int(a & 255); }
bool isOpaque(uint32_t a) { return alphaFromArgb(a) >= 255; }
double linearized(int c) {
  const double normalized = c / 255.0;
  return normalized <= 0.040449936 ? normalized / 12.92 * 100.0
                                   : std::pow((normalized + 0.055) / 1.055, 2.4) * 100.0;
}
int delinearized(double component) {
  double normalized = component / 100.0;
  double d = normalized <= 0.0031308 ? normalized * 12.92
                                     : 1.055 * std::pow(normalized, 1.0 / 2.4) - 0.055;
  return clampInt(0, 255, static_cast<int>(std::round(d * 255.0)));
}
uint32_t argbFromLinrgb(const std::vector<double>& v) {
  return argbFromRgb(delinearized(v[0]), delinearized(v[1]), delinearized(v[2]));
}
std::vector<double> xyzFromArgb(uint32_t argb) {
  const double c[3] = {linearized(redFromArgb(argb)),
                       linearized(greenFromArgb(argb)),
                       linearized(blueFromArgb(argb))};
  std::vector<double> result(3);
  for (int row = 0; row < 3; ++row)
    result[row] = kSrgbToXyz[row][0] * c[0] + kSrgbToXyz[row][1] * c[1] +
                  kSrgbToXyz[row][2] * c[2];
  return result;
}
uint32_t argbFromXyz(double x, double y, double z) {
  const double xyz[3] = {x, y, z};
  double lin[3];
  for (int row = 0; row < 3; ++row)
    lin[row] = kXyzToSrgb[row][0] * xyz[0] + kXyzToSrgb[row][1] * xyz[1] +
               kXyzToSrgb[row][2] * xyz[2];
  return argbFromRgb(delinearized(lin[0]), delinearized(lin[1]),
                     delinearized(lin[2]));
}
std::vector<double> labFromArgb(uint32_t argb) {
  const auto xyz = xyzFromArgb(argb);
  const auto wp = whitePointD65();
  const double fx = labF(xyz[0] / wp[0]);
  const double fy = labF(xyz[1] / wp[1]);
  const double fz = labF(xyz[2] / wp[2]);
  return {116 * fy - 16, 500 * (fx - fy), 200 * (fy - fz)};
}
uint32_t argbFromLab(double l, double a, double b) {
  const auto wp = whitePointD65();
  const double fy = (l + 16) / 116;
  const double fx = a / 500 + fy;
  const double fz = fy - b / 200;
  return argbFromXyz(labInvF(fx) * wp[0], labInvF(fy) * wp[1],
                     labInvF(fz) * wp[2]);
}
double lstarFromY(double y) { return labF(y / 100) * 116 - 16; }
double lstarFromArgb(uint32_t argb) { return lstarFromY(xyzFromArgb(argb)[1]); }
double yFromLstar(double lstar) { return 100 * labInvF((lstar + 16) / 116); }
uint32_t argbFromLstar(double lstar) {
  const int c = delinearized(yFromLstar(lstar));
  return argbFromRgb(c, c, c);
}
std::vector<double> whitePointD65() { return {95.047, 100.0, 108.883}; }
std::string argbToHex(uint32_t argb, bool includeAlpha) {
  char buf[10];
  if (includeAlpha) {
    std::snprintf(buf, sizeof(buf), "#%08X", argb);
  } else {
    std::snprintf(buf, sizeof(buf), "#%06X", argb & 0xffffffu);
  }
  return buf;
}

}  // namespace m3

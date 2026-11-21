// CAM16 forward/reverse transforms, ported from material-color-utilities.
#include "m3/cam16.hpp"
#include "m3/color.hpp"
#include "m3/math.hpp"

namespace m3 {

ViewingConditions ViewingConditions::make(std::vector<double> whitePoint,
                                           double adaptingLuminance,
                                           double backgroundLstar,
                                           double surround,
                                           bool discountingIlluminant) {
  if (adaptingLuminance < 0) adaptingLuminance = (200.0 / std::acos(-1.0)) *
                                               yFromLstar(50.0) / 100.0;
  const double rW = whitePoint[0] * 0.401288 + whitePoint[1] * 0.650173 - whitePoint[2] * 0.051461;
  const double gW = whitePoint[0] * -0.250268 + whitePoint[1] * 1.204414 + whitePoint[2] * 0.045854;
  const double bW = whitePoint[0] * -0.002079 + whitePoint[1] * 0.048952 + whitePoint[2] * 0.953127;
  const double f = 0.8 + surround / 10.0;
  const double c = f >= 0.9 ? lerp(0.59, 0.69, (f - 0.9) * 10)
                             : lerp(0.525, 0.59, (f - 0.8) * 10);
  double d = discountingIlluminant ? 1.0 : f *
      (1.0 - (1.0 / 3.6) * std::exp((-adaptingLuminance - 42.0) / 92.0));
  d = clampDouble(0, 1, d);
  const double k = 1.0 / (5.0 * adaptingLuminance + 1.0);
  const double k4 = k * k * k * k;
  const double k4F = 1.0 - k4;
  const double fl = k4 * adaptingLuminance + 0.1 * k4F * k4F *
      std::cbrt(5.0 * adaptingLuminance);
  if (backgroundLstar < 30.0) backgroundLstar = 30.0;
  const double n = yFromLstar(backgroundLstar) / whitePoint[1];
  const double z = 1.48 + std::sqrt(n);
  const double nbb = 0.725 / std::pow(n, 0.2);
  const double rgbD[3] = {d * (100 / rW) + 1 - d,
                          d * (100 / gW) + 1 - d,
                          d * (100 / bW) + 1 - d};
  const double rWValues[3] = {rW, gW, bW};
  double awAccum = 0;
  const double weights[3] = {2.0, 1.0, 0.05};
  for (int i = 0; i < 3; ++i) {
    const double af = std::pow(fl * rgbD[i] * rWValues[i] / 100.0, 0.42);
    awAccum += weights[i] * (400.0 * af / (af + 27.13));
  }
  return {n, awAccum * nbb, nbb, nbb, c, f,
          {rgbD[0], rgbD[1], rgbD[2]}, fl, std::pow(fl, 0.25), z};
}
ViewingConditions ViewingConditions::srgb() { return make(); }

Cam16 Cam16::fromInt(uint32_t argb) {
  return fromIntInViewingConditions(argb, ViewingConditions::srgb());
}
Cam16 Cam16::fromIntInViewingConditions(uint32_t argb,
                                         const ViewingConditions& vc) {
  const auto xyz = xyzFromArgb(argb);
  return fromXyzInViewingConditions(xyz[0], xyz[1], xyz[2], vc);
}
Cam16 Cam16::fromXyzInViewingConditions(double x, double y, double z,
                                         const ViewingConditions& vc) {
  const double rC = 0.401288 * x + 0.650173 * y - 0.051461 * z;
  const double gC = -0.250268 * x + 1.204414 * y + 0.045854 * z;
  const double bC = -0.002079 * x + 0.048952 * y + 0.953127 * z;
  const double rgbD[3] = {vc.rgbD[0] * rC, vc.rgbD[1] * gC, vc.rgbD[2] * bC};
  double rA, gA, bA;
  double* values[3] = {&rA, &gA, &bA};
  for (int i = 0; i < 3; ++i) {
    const double af = std::pow(vc.fl * std::abs(rgbD[i]) / 100.0, 0.42);
    *values[i] = signum(rgbD[i]) * 400.0 * af / (af + 27.13);
  }
  const double a = (11 * rA - 12 * gA + bA) / 11.0;
  const double b = (rA + gA - 2 * bA) / 9.0;
  const double u = (20 * rA + 20 * gA + 21 * bA) / 20.0;
  const double p2 = (40 * rA + 20 * gA + bA) / 20.0;
  const double hue = sanitizeDegreesDouble(std::atan2(b, a) * 180 / std::acos(-1.0));
  const double hueRadians = hue * std::acos(-1.0) / 180.0;
  const double ac = p2 * vc.nbb;
  const double j = 100 * std::pow(ac / vc.aw, vc.c * vc.z);
  const double q = (4 / vc.c) * std::sqrt(j / 100) * (vc.aw + 4) * vc.fLRoot;
  const double huePrime = hue < 20.14 ? hue + 360 : hue;
  const double eHue = 0.25 * (std::cos(huePrime * std::acos(-1.0) / 180 + 2) + 3.8);
  const double p1 = (50000.0 / 13.0) * eHue * vc.nc * vc.ncb;
  const double t = p1 * std::sqrt(a * a + b * b) / (u + 0.305);
  const double alpha = std::pow(t, 0.9) *
      std::pow(1.64 - std::pow(0.29, vc.n), 0.73);
  const double chroma = alpha * std::sqrt(j / 100.0);
  const double m = chroma * vc.fLRoot;
  const double s = 50 * std::sqrt(alpha * vc.c / (vc.aw + 4));
  const double jstar = (1 + 100 * 0.007) * j / (1 + 0.007 * j);
  const double mstar = std::log1p(0.0228 * m) / 0.0228;
  return {hue, chroma, j, q, m, s, jstar, mstar * std::cos(hueRadians),
          mstar * std::sin(hueRadians)};
}
Cam16 Cam16::fromJch(double j, double c, double h) {
  return fromJchInViewingConditions(j, c, h, ViewingConditions::srgb());
}
Cam16 Cam16::fromJchInViewingConditions(double j, double c, double h,
                                         const ViewingConditions& vc) {
  const double q = (4 / vc.c) * std::sqrt(j / 100) * (vc.aw + 4) * vc.fLRoot;
  const double m = c * vc.fLRoot;
  const double alpha = c / std::sqrt(j / 100.0);
  const double s = 50 * std::sqrt(alpha * vc.c / (vc.aw + 4));
  const double hRad = h * std::acos(-1.0) / 180.0;
  const double jstar = (1 + 100 * 0.007) * j / (1 + 0.007 * j);
  const double mstar = std::log1p(0.0228 * m) / 0.0228;
  return {h, c, j, q, m, s, jstar, mstar * std::cos(hRad), mstar * std::sin(hRad)};
}
Cam16 Cam16::fromUcs(double jstar, double astar, double bstar) {
  return fromUcsInViewingConditions(jstar, astar, bstar, ViewingConditions::srgb());
}
Cam16 Cam16::fromUcsInViewingConditions(double jstar, double astar,
                                         double bstar, const ViewingConditions& vc) {
  const double mstar = std::sqrt(astar * astar + bstar * bstar);
  const double m = std::expm1(mstar * 0.0228) / 0.0228;
  const double c = m / vc.fLRoot;
  double h = std::atan2(bstar, astar) * 180 / std::acos(-1.0);
  if (h < 0) h += 360;
  const double j = jstar / (1 - (jstar - 100) * 0.007);
  return fromJchInViewingConditions(j, c, h, vc);
}
uint32_t Cam16::toInt() const { return viewed(ViewingConditions::srgb()); }
std::vector<double> Cam16::xyzInViewingConditions(const ViewingConditions& vc) const {
  const double alpha = chroma == 0.0 || j == 0.0 ? 0.0 : chroma / std::sqrt(j / 100.0);
  const double t = std::pow(alpha / std::pow(1.64 - std::pow(0.29, vc.n), 0.73), 1.0 / 0.9);
  const double hRad = hue * std::acos(-1.0) / 180.0;
  const double eHue = 0.25 * (std::cos(hRad + 2.0) + 3.8);
  const double ac = vc.aw * std::pow(j / 100.0, 1.0 / vc.c / vc.z);
  const double p1 = eHue * (50000.0 / 13.0) * vc.nc * vc.ncb;
  const double p2 = ac / vc.nbb;
  const double hSin = std::sin(hRad), hCos = std::cos(hRad);
  const double gamma = 23.0 * (p2 + 0.305) * t /
                       (23.0 * p1 + 11 * t * hCos + 108 * t * hSin);
  const double a = gamma * hCos, b = gamma * hSin;
  const double adapted[3] = {(460 * p2 + 451 * a + 288 * b) / 1403.0,
                             (460 * p2 - 891 * a - 261 * b) / 1403.0,
                             (460 * p2 - 220 * a - 6300 * b) / 1403.0};
  double rgbF[3];
  for (int i = 0; i < 3; ++i) {
    const double base = std::max(0.0, 27.13 * std::abs(adapted[i]) /
                                 (400.0 - std::abs(adapted[i])));
    rgbF[i] = signum(adapted[i]) * (100.0 / vc.fl) *
              std::pow(base, 1.0 / 0.42) / vc.rgbD[i];
  }
  return {1.86206786 * rgbF[0] - 1.01125463 * rgbF[1] + 0.14918677 * rgbF[2],
          0.38752654 * rgbF[0] + 0.62144744 * rgbF[1] - 0.00897398 * rgbF[2],
          -0.01584150 * rgbF[0] - 0.03412294 * rgbF[1] + 1.04996444 * rgbF[2]};
}
uint32_t Cam16::viewed(const ViewingConditions& vc) const {
  const auto xyz = xyzInViewingConditions(vc);
  return argbFromXyz(xyz[0], xyz[1], xyz[2]);
}
double Cam16::distance(const Cam16& other) const {
  const double dj = jstar - other.jstar, da = astar - other.astar;
  const double db = bstar - other.bstar;
  return 1.41 * std::pow(std::sqrt(dj * dj + da * da + db * db), 0.63);
}

}  // namespace m3

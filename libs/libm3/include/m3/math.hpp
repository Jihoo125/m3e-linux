#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace m3 {

constexpr int clampInt(int lower, int upper, int input) {
  return std::clamp(input, lower, upper);
}

constexpr double clampDouble(double lower, double upper, double input) {
  return std::clamp(input, lower, upper);
}

constexpr double lerp(double start, double stop, double amount) {
  return (1.0 - amount) * start + amount * stop;
}

double sanitizeDegreesDouble(double degrees);
constexpr int sanitizeDegreesInt(int degrees) {
  degrees %= 360;
  return degrees < 0 ? degrees + 360 : degrees;
}

double differenceDegrees(double a, double b);
double signum(double value);

}  // namespace m3

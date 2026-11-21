#pragma once

#include <cstdint>

namespace m3 {

double contrastRatio(double toneA, double toneB);
double ratioOfTones(double toneA, double toneB);
double ratioOfYs(double y1, double y2);
double contrastRatioOfArgb(uint32_t a, uint32_t b);
double lighter(double tone, double ratio);
double darker(double tone, double ratio);
double lighterUnsafe(double tone, double ratio);
double darkerUnsafe(double tone, double ratio);
double foregroundTone(double bgTone, double ratio);
bool toneAllowsLightForeground(double tone);
bool tonePrefersLightForeground(double tone);
double enableLightForeground(double tone);

struct Contrast {
  static double ratioOfTones(double a, double b) { return m3::ratioOfTones(a, b); }
  static double ratioOfYs(double a, double b) { return m3::ratioOfYs(a, b); }
  static double ratioOfArgb(uint32_t a, uint32_t b) {
    return contrastRatioOfArgb(a, b);
  }
  static double lighter(double t, double r) { return m3::lighter(t, r); }
  static double darker(double t, double r) { return m3::darker(t, r); }
  static double lighterUnsafe(double t, double r) {
    return m3::lighterUnsafe(t, r);
  }
  static double darkerUnsafe(double t, double r) {
    return m3::darkerUnsafe(t, r);
  }
};

}  // namespace m3

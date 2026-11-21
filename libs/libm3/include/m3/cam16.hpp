#pragma once

#include "m3/math.hpp"
#include <vector>

namespace m3 {

struct ViewingConditions {
  double n = 0;
  double aw = 0;
  double nbb = 0;
  double ncb = 0;
  double c = 0;
  double nc = 0;
  std::vector<double> rgbD;
  double fl = 0;
  double fLRoot = 0;
  double z = 0;

  static ViewingConditions make(
      std::vector<double> whitePoint = {95.047, 100.0, 108.883},
      double adaptingLuminance = 11.725677948856817,
      double backgroundLstar = 50.0, double surround = 2.0,
      bool discountingIlluminant = false);
  static ViewingConditions srgb();
};

struct Cam16 {
  double hue = 0;
  double chroma = 0;
  double j = 0;
  double q = 0;
  double m = 0;
  double s = 0;
  double jstar = 0;
  double astar = 0;
  double bstar = 0;

  static Cam16 fromInt(uint32_t argb);
  static Cam16 fromIntInViewingConditions(uint32_t argb,
                                          const ViewingConditions& vc);
  static Cam16 fromJch(double j, double chroma, double hue);
  static Cam16 fromJchInViewingConditions(double j, double chroma, double hue,
                                          const ViewingConditions& vc);
  static Cam16 fromUcs(double jstar, double astar, double bstar);
  static Cam16 fromUcsInViewingConditions(double jstar, double astar,
                                          double bstar,
                                          const ViewingConditions& vc);
  static Cam16 fromXyzInViewingConditions(double x, double y, double z,
                                          const ViewingConditions& vc);
  uint32_t toInt() const;
  uint32_t viewed(const ViewingConditions& vc) const;
  std::vector<double> xyzInViewingConditions(
      const ViewingConditions& vc) const;
  double distance(const Cam16& other) const;
};

}  // namespace m3

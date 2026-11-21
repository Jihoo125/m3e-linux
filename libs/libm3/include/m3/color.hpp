#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace m3 {

uint32_t argbFromRgb(int red, int green, int blue);
uint32_t argbFromLinrgb(const std::vector<double>& linrgb);
uint32_t argbFromXyz(double x, double y, double z);
uint32_t argbFromLab(double l, double a, double b);
uint32_t argbFromLstar(double lstar);
std::vector<double> xyzFromArgb(uint32_t argb);
std::vector<double> labFromArgb(uint32_t argb);
double lstarFromArgb(uint32_t argb);
double lstarFromY(double y);
double yFromLstar(double lstar);
double linearized(int rgbComponent);
int delinearized(double rgbComponent);
int alphaFromArgb(uint32_t argb);
int redFromArgb(uint32_t argb);
int greenFromArgb(uint32_t argb);
int blueFromArgb(uint32_t argb);
bool isOpaque(uint32_t argb);
std::vector<double> whitePointD65();
std::string argbToHex(uint32_t argb, bool includeAlpha = true);

}  // namespace m3

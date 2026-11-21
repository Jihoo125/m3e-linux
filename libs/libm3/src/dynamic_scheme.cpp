#include "m3/dynamic_scheme.hpp"
#include "m3/contrast.hpp"
#include "m3/temperature.hpp"
#include "m3/math.hpp"

#include <cctype>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>

namespace m3 {
namespace {

double rotateHue(const Hct& source, const std::vector<double>& breakpoints,
                 const std::vector<double>& rotations) {
  const size_t size = std::min(breakpoints.size(), rotations.size());
  for (size_t i = 0; i < size; ++i) {
    if (source.hue > breakpoints[i] && source.hue < breakpoints[i + 1])
      return sanitizeDegreesDouble(source.hue + rotations[i]);
  }
  return source.hue;
}

enum class PaletteKind { kPrimary, kSecondary, kTertiary, kNeutral,
                         kNeutralVariant, kError };
struct ContrastCurve {
  double low, normal, medium, high;
  double get(double level) const {
    if (level <= -1) return low;
    if (level < 0) return lerp(low, normal, level + 1);
    if (level < 0.5) return lerp(normal, medium, level / 0.5);
    if (level < 1) return lerp(medium, high, (level - 0.5) / 0.5);
    return high;
  }
};
enum class Polarity { kNone, kNearer, kLighter };
struct Role {
  std::string name;
  PaletteKind palette;
  std::function<double(const DynamicScheme&)> tone;
  bool isBackground = false;
  std::string background;
  std::string secondBackground;
  std::optional<ContrastCurve> curve;
  Polarity polarity = Polarity::kNone;
  std::string pairA, pairB;
};
using Tone = std::function<double(const DynamicScheme&)>;
Role role(std::string name, PaletteKind palette, Tone tone,
          bool isBackground = false, std::string background = {},
          std::optional<ContrastCurve> curve = std::nullopt,
          std::string secondBackground = {}, Polarity polarity = Polarity::kNone,
          std::string pairA = {}, std::string pairB = {}) {
  return {std::move(name), palette, std::move(tone), isBackground,
          std::move(background), std::move(secondBackground), curve, polarity,
          std::move(pairA), std::move(pairB)};
}
bool fidelity(const DynamicScheme& s) {
  return s.variant == Variant::kFidelity || s.variant == Variant::kContent;
}
bool mono(const DynamicScheme& s) { return s.variant == Variant::kMonochrome; }
double findDesiredChromaByTone(double hue, double chroma, double tone,
                               bool decreasing) {
  double answer = tone;
  Hct closest = Hct::from(hue, chroma, tone);
  if (closest.chroma < chroma) {
    double peak = closest.chroma;
    while (closest.chroma < chroma) {
      answer += decreasing ? -1 : 1;
      const Hct potential = Hct::from(hue, chroma, answer);
      if (peak > potential.chroma) break;
      if (std::abs(potential.chroma - chroma) < 0.4) break;
      if (std::abs(potential.chroma - chroma) < std::abs(closest.chroma - chroma))
        closest = potential;
      peak = std::max(peak, potential.chroma);
    }
  }
  return answer;
}

std::vector<Role> makeRoles() {
  std::vector<Role> r;
  auto keyRoles = [&r](const std::string& prefix, PaletteKind kind) {
    r.push_back(role(prefix + "PaletteKeyColor", kind, [kind](const DynamicScheme& s) {
      switch (kind) {
        case PaletteKind::kPrimary: return s.primaryPalette.keyColor().tone;
        case PaletteKind::kSecondary: return s.secondaryPalette.keyColor().tone;
        case PaletteKind::kTertiary: return s.tertiaryPalette.keyColor().tone;
        case PaletteKind::kNeutral: return s.neutralPalette.keyColor().tone;
        case PaletteKind::kNeutralVariant: return s.neutralVariantPalette.keyColor().tone;
        case PaletteKind::kError: return s.errorPalette.keyColor().tone;
      }
      return 50.0;
    }));
  };
  keyRoles("primary", PaletteKind::kPrimary);
  keyRoles("secondary", PaletteKind::kSecondary);
  keyRoles("tertiary", PaletteKind::kTertiary);
  keyRoles("neutral", PaletteKind::kNeutral);
  keyRoles("neutralVariant", PaletteKind::kNeutralVariant);
  keyRoles("error", PaletteKind::kError);
  const auto N = PaletteKind::kNeutral, NV = PaletteKind::kNeutralVariant;
  const auto P = PaletteKind::kPrimary, Q = PaletteKind::kSecondary;
  const auto T = PaletteKind::kTertiary, E = PaletteKind::kError;
  r.push_back(role("background", N, [](const auto& s) { return s.isDark ? 6.0 : 98.0; }, true));
  r.push_back(role("onBackground", N, [](const auto& s) { return s.isDark ? 90.0 : 10.0; },
                   false, "background", ContrastCurve{3,3,4.5,7}));
  r.push_back(role("surface", N, [](const auto& s) { return s.isDark ? 6.0 : 98.0; }, true));
  r.push_back(role("surfaceDim", N, [](const auto& s) {
    return s.isDark ? 6.0 : ContrastCurve{87,87,80,75}.get(s.contrastLevel);
  }, true));
  r.push_back(role("surfaceBright", N, [](const auto& s) {
    return s.isDark ? ContrastCurve{24,24,29,34}.get(s.contrastLevel) : 98.0;
  }, true));
  r.push_back(role("surfaceContainerLowest", N, [](const auto& s) {
    return s.isDark ? ContrastCurve{4,4,2,0}.get(s.contrastLevel) : 100.0;
  }, true));
  r.push_back(role("surfaceContainerLow", N, [](const auto& s) {
    return s.isDark ? ContrastCurve{10,10,11,12}.get(s.contrastLevel)
                    : ContrastCurve{96,96,96,95}.get(s.contrastLevel);
  }, true));
  r.push_back(role("surfaceContainer", N, [](const auto& s) {
    return s.isDark ? ContrastCurve{12,12,16,20}.get(s.contrastLevel)
                    : ContrastCurve{94,94,92,90}.get(s.contrastLevel);
  }, true));
  r.push_back(role("surfaceContainerHigh", N, [](const auto& s) {
    return s.isDark ? ContrastCurve{17,17,21,25}.get(s.contrastLevel)
                    : ContrastCurve{92,92,88,85}.get(s.contrastLevel);
  }, true));
  r.push_back(role("surfaceContainerHighest", N, [](const auto& s) {
    return s.isDark ? ContrastCurve{22,22,26,30}.get(s.contrastLevel)
                    : ContrastCurve{90,90,84,80}.get(s.contrastLevel);
  }, true));
  r.push_back(role("onSurface", N, [](const auto& s) { return s.isDark ? 90.0 : 10.0; },
                   false, "@highest", ContrastCurve{4.5,7,11,21}));
  r.push_back(role("surfaceVariant", NV, [](const auto& s) { return s.isDark ? 30.0 : 90.0; }, true));
  r.push_back(role("onSurfaceVariant", NV, [](const auto& s) { return s.isDark ? 80.0 : 30.0; },
                   false, "@highest", ContrastCurve{3,4.5,7,11}));
  r.push_back(role("inverseSurface", N, [](const auto& s) { return s.isDark ? 90.0 : 20.0; }, true));
  r.push_back(role("inverseOnSurface", N, [](const auto& s) { return s.isDark ? 20.0 : 95.0; },
                   false, "inverseSurface", ContrastCurve{4.5,7,11,21}));
  r.push_back(role("outline", NV, [](const auto& s) { return s.isDark ? 60.0 : 50.0; },
                   false, "@highest", ContrastCurve{1.5,3,4.5,7}));
  r.push_back(role("outlineVariant", NV, [](const auto& s) { return s.isDark ? 30.0 : 80.0; },
                   false, "@highest", ContrastCurve{1,1,3,4.5}));
  r.push_back(role("shadow", N, [](const auto&) { return 0.0; }));
  r.push_back(role("scrim", N, [](const auto&) { return 0.0; }));
  r.push_back(role("surfaceTint", P, [](const auto& s) { return s.isDark ? 80.0 : 40.0; }, true));

  auto accent = [&](const std::string& prefix, PaletteKind palette) {
    const std::string container = prefix + "Container";
    const std::string fixed = prefix + "Fixed", fixedDim = prefix + "FixedDim";
    auto pair = Polarity::kNearer;
    if (prefix == "primary") {
      r.push_back(role("primary", P, [](const auto& s) {
        if (mono(s)) return s.isDark ? 100.0 : 0.0;
        return s.isDark ? 80.0 : 40.0;
      }, true, "@highest", ContrastCurve{3,4.5,7,7}, {}, pair, container, prefix));
      r.push_back(role("onPrimary", P, [](const auto& s) {
        if (mono(s)) return s.isDark ? 10.0 : 90.0;
        return s.isDark ? 20.0 : 100.0;
      }, false, "primary", ContrastCurve{4.5,7,11,21}));
      r.push_back(role(container, P, [](const auto& s) {
        if (fidelity(s)) return s.sourceColorHct.tone;
        if (mono(s)) return s.isDark ? 85.0 : 25.0;
        return s.isDark ? 30.0 : 90.0;
      }, true, "@highest", ContrastCurve{1,1,3,4.5}, {}, pair, container, prefix));
      r.push_back(role("onPrimaryContainer", P, [](const auto& s) {
        if (fidelity(s)) {
          // Fidelity's initial foreground uses the unadjusted container tone.
          return foregroundTone(s.sourceColorHct.tone, 4.5);
        }
        if (mono(s)) return s.isDark ? 0.0 : 100.0;
        return s.isDark ? 90.0 : 30.0;
      }, false, container, ContrastCurve{3,4.5,7,11}));
      r.push_back(role("inversePrimary", P, [](const auto& s) { return s.isDark ? 40.0 : 80.0; },
                       false, "inverseSurface", ContrastCurve{3,4.5,7,7}));
    } else if (prefix == "secondary") {
      r.push_back(role("secondary", Q, [](const auto& s) { return s.isDark ? 80.0 : 40.0; },
                       true, "@highest", ContrastCurve{3,4.5,7,7}, {}, pair, container, prefix));
      r.push_back(role("onSecondary", Q, [](const auto& s) {
        if (mono(s)) return s.isDark ? 10.0 : 100.0;
        return s.isDark ? 20.0 : 100.0;
      }, false, "secondary", ContrastCurve{4.5,7,11,21}));
      r.push_back(role(container, Q, [](const auto& s) {
        const double initial = s.isDark ? 30.0 : 90.0;
        if (mono(s)) return s.isDark ? 30.0 : 85.0;
        if (!fidelity(s)) return initial;
        return findDesiredChromaByTone(s.secondaryPalette.hue(), s.secondaryPalette.chroma(),
                                       initial, !s.isDark);
      }, true, "@highest", ContrastCurve{1,1,3,4.5}, {}, pair, container, prefix));
      r.push_back(role("onSecondaryContainer", Q, [](const auto& s) {
        if (mono(s)) return s.isDark ? 90.0 : 10.0;
        if (!fidelity(s)) return s.isDark ? 90.0 : 30.0;
        const double initial = s.isDark ? 30.0 : 90.0;
        const double containerTone = findDesiredChromaByTone(
            s.secondaryPalette.hue(), s.secondaryPalette.chroma(), initial, !s.isDark);
        return foregroundTone(containerTone, 4.5);
      }, false, container, ContrastCurve{3,4.5,7,11}));
    } else if (prefix == "tertiary") {
      r.push_back(role("tertiary", T, [](const auto& s) {
        if (mono(s)) return s.isDark ? 90.0 : 25.0;
        return s.isDark ? 80.0 : 40.0;
      }, true, "@highest", ContrastCurve{3,4.5,7,7}, {}, pair, container, prefix));
      r.push_back(role("onTertiary", T, [](const auto& s) {
        if (mono(s)) return s.isDark ? 10.0 : 90.0;
        return s.isDark ? 20.0 : 100.0;
      }, false, "tertiary", ContrastCurve{4.5,7,11,21}));
      r.push_back(role(container, T, [](const auto& s) {
        if (mono(s)) return s.isDark ? 60.0 : 49.0;
        if (!fidelity(s)) return s.isDark ? 30.0 : 90.0;
        return fixIfDisliked(s.tertiaryPalette.getHct(s.sourceColorHct.tone)).tone;
      }, true, "@highest", ContrastCurve{1,1,3,4.5}, {}, pair, container, prefix));
      r.push_back(role("onTertiaryContainer", T, [](const auto& s) {
        if (mono(s)) return s.isDark ? 0.0 : 100.0;
        if (!fidelity(s)) return s.isDark ? 90.0 : 30.0;
        const double containerTone = fixIfDisliked(
            s.tertiaryPalette.getHct(s.sourceColorHct.tone)).tone;
        return foregroundTone(containerTone, 4.5);
      }, false, container, ContrastCurve{3,4.5,7,11}));
    } else if (prefix == "error") {
      r.push_back(role("error", E, [](const auto& s) { return s.isDark ? 80.0 : 40.0; },
                       true, "@highest", ContrastCurve{3,4.5,7,7}, {}, pair, container, prefix));
      r.push_back(role("onError", E, [](const auto& s) { return s.isDark ? 20.0 : 100.0; },
                       false, "error", ContrastCurve{4.5,7,11,21}));
      r.push_back(role(container, E, [](const auto& s) { return s.isDark ? 30.0 : 90.0; },
                       true, "@highest", ContrastCurve{1,1,3,4.5}, {}, pair, container, prefix));
      r.push_back(role("onErrorContainer", E, [](const auto& s) {
        if (mono(s)) return s.isDark ? 90.0 : 10.0;
        return s.isDark ? 90.0 : 30.0;
      }, false, container, ContrastCurve{3,4.5,7,11}));
    }
    Tone fixedTone;
    Tone fixedDimTone;
    Tone onFixedTone = [](const auto&) { return 10.0; };
    Tone onFixedVariantTone = [](const auto&) { return 30.0; };
    if (prefix == "primary") {
      fixedTone = [](const auto& s) { return mono(s) ? 40.0 : 90.0; };
      fixedDimTone = [](const auto& s) { return mono(s) ? 30.0 : 80.0; };
      onFixedTone = [](const auto& s) { return mono(s) ? 100.0 : 10.0; };
      onFixedVariantTone = [](const auto& s) { return mono(s) ? 90.0 : 30.0; };
    } else if (prefix == "secondary") {
      fixedTone = [](const auto& s) { return mono(s) ? 80.0 : 90.0; };
      fixedDimTone = [](const auto& s) { return mono(s) ? 70.0 : 80.0; };
      onFixedVariantTone = [](const auto& s) { return mono(s) ? 25.0 : 30.0; };
    } else {
      fixedTone = [](const auto& s) { return mono(s) ? 40.0 : 90.0; };
      fixedDimTone = [](const auto& s) { return mono(s) ? 30.0 : 80.0; };
      onFixedTone = [](const auto& s) { return mono(s) ? 100.0 : 10.0; };
      onFixedVariantTone = [](const auto& s) { return mono(s) ? 90.0 : 30.0; };
    }
    r.push_back(role(fixed, palette, fixedTone, true, "@highest",
                     ContrastCurve{1,1,3,4.5}, {}, Polarity::kLighter, fixed, fixedDim));
    r.push_back(role(fixedDim, palette, fixedDimTone, true, "@highest",
                     ContrastCurve{1,1,3,4.5}, {}, Polarity::kLighter, fixed, fixedDim));
    r.push_back(role("on" + std::string(1, static_cast<char>(std::toupper(prefix[0]))) +
                     prefix.substr(1) + "Fixed", palette, onFixedTone, false, fixedDim,
                     ContrastCurve{4.5,7,11,21}, fixed));
    r.push_back(role("on" + std::string(1, static_cast<char>(std::toupper(prefix[0]))) +
                     prefix.substr(1) + "FixedVariant", palette, onFixedVariantTone,
                     false, fixedDim, ContrastCurve{3,4.5,7,11}, fixed));
  };
  accent("primary", P);
  accent("secondary", Q);
  accent("tertiary", T);
  accent("error", E);
  return r;
}

class ToneSolver {
 public:
  explicit ToneSolver(const DynamicScheme& scheme) : s_(scheme), roles_(makeRoles()) {
    for (const auto& r : roles_) indices_[r.name] = &r;
  }
  double get(const std::string& name) {
    if (name == "@highest") return get(s_.isDark ? "surfaceBright" : "surfaceDim");
    auto cached = cache_.find(name);
    if (cached != cache_.end()) return cached->second;
    if (resolving_.count(name)) return initial(*indices_.at(name));
    resolving_.insert(name);
    const Role& color = *indices_.at(name);
    double answer = 0;
    if (color.polarity != Polarity::kNone)
      answer = paired(color);
    else
      answer = single(color);
    resolving_.erase(name);
    cache_[name] = answer;
    return answer;
  }
  uint32_t argb(const std::string& name) {
    const Role& r = *indices_.at(name);
    return palette(r.palette).tone(get(name));
  }
  const std::vector<Role>& roles() const { return roles_; }
 private:
  const TonalPalette& palette(PaletteKind p) const {
    switch (p) {
      case PaletteKind::kPrimary: return s_.primaryPalette;
      case PaletteKind::kSecondary: return s_.secondaryPalette;
      case PaletteKind::kTertiary: return s_.tertiaryPalette;
      case PaletteKind::kNeutral: return s_.neutralPalette;
      case PaletteKind::kNeutralVariant: return s_.neutralVariantPalette;
      case PaletteKind::kError: return s_.errorPalette;
    }
    return s_.primaryPalette;
  }
  double initial(const Role& r) const { return r.tone(s_); }
  double single(const Role& color) {
    double answer = initial(color);
    if (color.background.empty() || !color.curve) return answer;
    const double bgTone = get(color.background);
    const double desired = color.curve->get(s_.contrastLevel);
    if (ratioOfTones(bgTone, answer) < desired || s_.contrastLevel < 0)
      answer = foregroundTone(bgTone, desired);
    if (color.isBackground && answer >= 50 && answer < 60)
      answer = ratioOfTones(49, bgTone) >= desired ? 49 : 60;
    if (color.secondBackground.empty()) return answer;
    const double bg1 = bgTone, bg2 = get(color.secondBackground);
    const double upper = std::max(bg1, bg2), lower = std::min(bg1, bg2);
    if (ratioOfTones(upper, answer) >= desired && ratioOfTones(lower, answer) >= desired)
      return answer;
    const double light = lighter(upper, desired), dark = darker(lower, desired);
    const bool hasLight = light >= 0, hasDark = dark >= 0;
    if (tonePrefersLightForeground(bg1) || tonePrefersLightForeground(bg2))
      return hasLight ? light : 100;
    if (hasLight != hasDark) return hasLight ? light : dark;
    return hasDark ? dark : 0;
  }
  double paired(const Role& color) {
    const Role& a = *indices_.at(color.pairA);
    const Role& b = *indices_.at(color.pairB);
    const bool aNearer = color.polarity == Polarity::kNearer ||
                         (color.polarity == Polarity::kLighter && !s_.isDark);
    const Role& nearer = aNearer ? a : b;
    const Role& farther = aNearer ? b : a;
    const bool amNearer = color.name == nearer.name;
    const double expansion = s_.isDark ? 1.0 : -1.0;
    double nTone = initial(nearer), fTone = initial(farther);
    const double delta = 10.0;
    if (!color.background.empty() && nearer.curve && farther.curve) {
      const double bg = get(color.background);
      const double nContrast = nearer.curve->get(s_.contrastLevel);
      const double fContrast = farther.curve->get(s_.contrastLevel);
      if (ratioOfTones(bg, nTone) < nContrast) nTone = foregroundTone(bg, nContrast);
      if (ratioOfTones(bg, fTone) < fContrast) fTone = foregroundTone(bg, fContrast);
      if (s_.contrastLevel < 0) {
        nTone = foregroundTone(bg, nContrast);
        fTone = foregroundTone(bg, fContrast);
      }
    }
    if ((fTone - nTone) * expansion < delta) {
      fTone = clampDouble(0, 100, nTone + delta * expansion);
      if ((fTone - nTone) * expansion < delta)
        nTone = clampDouble(0, 100, fTone - delta * expansion);
    }
    if (nTone >= 50 && nTone < 60) {
      if (expansion > 0) {
        nTone = 60; fTone = std::max(fTone, nTone + delta * expansion);
      } else {
        nTone = 49; fTone = std::min(fTone, nTone + delta * expansion);
      }
    } else if (fTone >= 50 && fTone < 60) {
      // All 2021 fixed colors use stayTogether=true; accents use false.
      const bool stayTogether = color.polarity == Polarity::kLighter;
      if (stayTogether) {
        if (expansion > 0) {
          nTone = 60; fTone = std::max(fTone, nTone + delta * expansion);
        } else {
          nTone = 49; fTone = std::min(fTone, nTone + delta * expansion);
        }
      } else {
        fTone = expansion > 0 ? 60 : 49;
      }
    }
    return amNearer ? nTone : fTone;
  }
  const DynamicScheme& s_;
  std::vector<Role> roles_;
  std::map<std::string, const Role*> indices_;
  std::map<std::string, double> cache_;
  std::set<std::string> resolving_;
};
}  // namespace

DynamicScheme::DynamicScheme(Hct source, Variant v, bool dark, double contrast)
    : sourceColorHct(source), sourceColorArgb(source.toInt()), variant(v),
      contrastLevel(contrast), isDark(dark),
      primaryPalette(TonalPalette::fromHueAndChroma(0, 0)),
      secondaryPalette(TonalPalette::fromHueAndChroma(0, 0)),
      tertiaryPalette(TonalPalette::fromHueAndChroma(0, 0)),
      neutralPalette(TonalPalette::fromHueAndChroma(0, 0)),
      neutralVariantPalette(TonalPalette::fromHueAndChroma(0, 0)),
      errorPalette(TonalPalette::fromHueAndChroma(25, 84)) {
  const double h = source.hue, c = source.chroma;
  switch (v) {
    case Variant::kContent:
    case Variant::kFidelity:
      primaryPalette = TonalPalette::fromHueAndChroma(h, c);
      secondaryPalette = TonalPalette::fromHueAndChroma(h, std::max(c - 32, c * 0.5));
      if (v == Variant::kContent) {
        tertiaryPalette = TonalPalette::fromHct(fixIfDisliked(
            TemperatureCache(source).analogous(3, 6)[2]));
      } else {
        tertiaryPalette = TonalPalette::fromHct(fixIfDisliked(
            TemperatureCache(source).complement()));
      }
      neutralPalette = TonalPalette::fromHueAndChroma(h, c / 8);
      neutralVariantPalette = TonalPalette::fromHueAndChroma(h, c / 8 + 4);
      break;
    case Variant::kMonochrome:
      primaryPalette = secondaryPalette = tertiaryPalette = neutralPalette =
          neutralVariantPalette = TonalPalette::fromHueAndChroma(h, 0);
      break;
    case Variant::kNeutral:
      primaryPalette = TonalPalette::fromHueAndChroma(h, 12);
      secondaryPalette = TonalPalette::fromHueAndChroma(h, 8);
      tertiaryPalette = TonalPalette::fromHueAndChroma(h, 16);
      neutralPalette = neutralVariantPalette = TonalPalette::fromHueAndChroma(h, 2);
      break;
    case Variant::kTonalSpot:
      primaryPalette = TonalPalette::fromHueAndChroma(h, 36);
      secondaryPalette = TonalPalette::fromHueAndChroma(h, 16);
      tertiaryPalette = TonalPalette::fromHueAndChroma(sanitizeDegreesDouble(h + 60), 24);
      neutralPalette = TonalPalette::fromHueAndChroma(h, 6);
      neutralVariantPalette = TonalPalette::fromHueAndChroma(h, 8);
      break;
    case Variant::kExpressive:
      primaryPalette = TonalPalette::fromHueAndChroma(sanitizeDegreesDouble(h + 240), 40);
      secondaryPalette = TonalPalette::fromHueAndChroma(
          rotateHue(source, {0,21,51,121,151,191,271,321,360},
                    {45,95,45,20,45,90,45,45,45}), 24);
      tertiaryPalette = TonalPalette::fromHueAndChroma(
          rotateHue(source, {0,21,51,121,151,191,271,321,360},
                    {120,120,20,45,20,15,20,120,120}), 32);
      neutralPalette = TonalPalette::fromHueAndChroma(sanitizeDegreesDouble(h + 15), 8);
      neutralVariantPalette = TonalPalette::fromHueAndChroma(sanitizeDegreesDouble(h + 15), 12);
      break;
    case Variant::kVibrant:
      primaryPalette = TonalPalette::fromHueAndChroma(h, 200);
      secondaryPalette = TonalPalette::fromHueAndChroma(
          rotateHue(source, {0,41,61,101,131,181,251,301,360},
                    {18,15,10,12,15,18,15,12,12}), 24);
      tertiaryPalette = TonalPalette::fromHueAndChroma(
          rotateHue(source, {0,41,61,101,131,181,251,301,360},
                    {35,30,20,25,30,35,30,25,25}), 32);
      neutralPalette = TonalPalette::fromHueAndChroma(h, 10);
      neutralVariantPalette = TonalPalette::fromHueAndChroma(h, 12);
      break;
    case Variant::kRainbow:
      primaryPalette = TonalPalette::fromHueAndChroma(h, 48);
      secondaryPalette = TonalPalette::fromHueAndChroma(h, 16);
      tertiaryPalette = TonalPalette::fromHueAndChroma(sanitizeDegreesDouble(h + 60), 24);
      neutralPalette = neutralVariantPalette = TonalPalette::fromHueAndChroma(h, 0);
      break;
    case Variant::kFruitSalad:
      primaryPalette = TonalPalette::fromHueAndChroma(sanitizeDegreesDouble(h - 50), 48);
      secondaryPalette = TonalPalette::fromHueAndChroma(sanitizeDegreesDouble(h - 50), 36);
      tertiaryPalette = TonalPalette::fromHueAndChroma(h, 36);
      neutralPalette = TonalPalette::fromHueAndChroma(h, 10);
      neutralVariantPalette = TonalPalette::fromHueAndChroma(h, 16);
      break;
  }
}
std::map<std::string, uint32_t> DynamicScheme::roles() const {
  ToneSolver solver(*this);
  std::map<std::string, uint32_t> result;
  for (const auto& r : solver.roles()) result[r.name] = solver.argb(r.name);
  return result;
}
uint32_t DynamicScheme::getArgb(const std::string& name) const {
  ToneSolver solver(*this);
  return solver.argb(name);
}
std::map<std::string, uint32_t> schemeFromArgb(uint32_t seed, bool dark,
                                               Variant variant, double contrast) {
  return DynamicScheme(Hct::fromInt(seed), variant, dark, contrast).roles();
}

}  // namespace m3

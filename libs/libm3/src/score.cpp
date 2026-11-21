#include "m3/score.hpp"
#include "m3/color.hpp"
#include "m3/hct.hpp"
#include "m3/math.hpp"
#include "m3/quantize.hpp"

#include <algorithm>
#include <cmath>

namespace m3 {

std::vector<uint32_t> score(const std::map<uint32_t,double>& input,
                            ScoreOptions options) {
  if (options.desired <= 0) return {options.fallbackColorArgb};
  if (input.empty()) return {options.fallbackColorArgb};
  std::vector<Hct> colorsHct;
  std::vector<double> huePopulation(360,0), excited(360,0);
  double populationSum=0;
  for (const auto& [argb,population] : input) {
    colorsHct.push_back(Hct::fromInt(argb));
    huePopulation[clampInt(0,359,static_cast<int>(std::floor(colorsHct.back().hue)))]+=population;
    populationSum+=population;
  }
  for (int hue=0;hue<360;++hue) {
    const double proportion=huePopulation[hue]/populationSum;
    for (int i=hue-14;i<hue+16;++i) excited[sanitizeDegreesInt(i)]+=proportion;
  }
  struct Scored { Hct hct; double score; };
  std::vector<Scored> scored;
  for (const Hct& hct:colorsHct) {
    const int hue=sanitizeDegreesInt(static_cast<int>(std::round(hct.hue)));
    const double proportion=excited[hue];
    if (options.filter && (hct.chroma<5 || proportion<=0.01)) continue;
    const double proportionScore=proportion*100*0.7;
    const double chromaWeight=hct.chroma<48?0.1:0.3;
    scored.push_back({hct,proportionScore+(hct.chroma-48)*chromaWeight});
  }
  // stable_sort is equivalent to JS's stable Array.sort and MCU Java's sort.
  std::stable_sort(scored.begin(),scored.end(),[](const auto& a,const auto& b) {
    return a.score>b.score;
  });
  std::vector<Hct> chosen;
  for (int difference=90;difference>=15;--difference) {
    chosen.clear();
    for (const auto& item:scored) {
      bool duplicate=false;
      for (const Hct& h:chosen) {
        if (differenceDegrees(item.hct.hue,h.hue)<difference) { duplicate=true; break; }
      }
      if (!duplicate) chosen.push_back(item.hct);
      if (static_cast<int>(chosen.size())>=options.desired) break;
    }
    if (static_cast<int>(chosen.size())>=options.desired) break;
  }
  std::vector<uint32_t> result;
  if (chosen.empty()) result.push_back(options.fallbackColorArgb);
  for (const Hct& hct:chosen) result.push_back(hct.toInt());
  return result;
}
uint32_t sourceColorFromImage(const std::vector<uint32_t>& pixels) {
  auto quantized=quantizeCelebi(pixels,128);
  std::map<uint32_t,double> populations;
  for (const auto& [argb,count]:quantized) populations[argb]=count;
  return score(populations)[0];
}

}  // namespace m3

// Xiaolin Wu's RGB-cube quantizer and Celebi's Wu-initialized CIELAB k-means.
#include "m3/quantize.hpp"
#include "m3/color.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>

namespace m3 {
namespace {
constexpr int kSide = 33, kSize = 35937;
// Java-style integer truncation for exact porting where all integral inputs
// are non-negative (all moment volumes produced by Wu are non-negative).
int roundHalfUp(double value) { return static_cast<int>(std::floor(value + 0.5)); }
int indexOf(int r, int g, int b) {
  return (r << 10) + (r << 6) + r + (g << 5) + g + b;
}
struct Box { int r0=0,r1=0,g0=0,g1=0,b0=0,b1=0,vol=0; };
struct MaxResult { int cut=-1; double maximum=0; };
template<class T>
double volume(const Box& b, const std::vector<T>& m) {
  return static_cast<double>(m[indexOf(b.r1,b.g1,b.b1)] -
      m[indexOf(b.r1,b.g1,b.b0)] - m[indexOf(b.r1,b.g0,b.b1)] +
      m[indexOf(b.r1,b.g0,b.b0)] - m[indexOf(b.r0,b.g1,b.b1)] +
      m[indexOf(b.r0,b.g1,b.b0)] + m[indexOf(b.r0,b.g0,b.b1)] -
      m[indexOf(b.r0,b.g0,b.b0)]);
}
template<class T>
double bottom(const Box& b, int d, const std::vector<T>& m) {
  if (d == 0)
    return static_cast<double>(-m[indexOf(b.r0,b.g1,b.b1)] +
       m[indexOf(b.r0,b.g1,b.b0)] + m[indexOf(b.r0,b.g0,b.b1)] -
       m[indexOf(b.r0,b.g0,b.b0)]);
  if (d == 1)
    return static_cast<double>(-m[indexOf(b.r1,b.g0,b.b1)] +
       m[indexOf(b.r1,b.g0,b.b0)] + m[indexOf(b.r0,b.g0,b.b1)] -
       m[indexOf(b.r0,b.g0,b.b0)]);
  return static_cast<double>(-m[indexOf(b.r1,b.g1,b.b0)] +
     m[indexOf(b.r1,b.g0,b.b0)] + m[indexOf(b.r0,b.g1,b.b0)] -
     m[indexOf(b.r0,b.g0,b.b0)]);
}
template<class T>
double top(const Box& b, int d, int p, const std::vector<T>& m) {
  if (d == 0)
    return static_cast<double>(m[indexOf(p,b.g1,b.b1)] - m[indexOf(p,b.g1,b.b0)] -
       m[indexOf(p,b.g0,b.b1)] + m[indexOf(p,b.g0,b.b0)]);
  if (d == 1)
    return static_cast<double>(m[indexOf(b.r1,p,b.b1)] - m[indexOf(b.r1,p,b.b0)] -
       m[indexOf(b.r0,p,b.b1)] + m[indexOf(b.r0,p,b.b0)]);
  return static_cast<double>(m[indexOf(b.r1,b.g1,p)] - m[indexOf(b.r1,b.g0,p)] -
     m[indexOf(b.r0,b.g1,p)] + m[indexOf(b.r0,b.g0,p)]);
}

class Wu {
 public:
  std::vector<uint32_t> run(const std::vector<uint32_t>& pixels, int maxColors) {
    if (pixels.empty() || maxColors <= 0) return {};
    weights.assign(kSize, 0); mr.assign(kSize, 0); mg.assign(kSize, 0);
    mb.assign(kSize, 0); m2.assign(kSize, 0);
    for (uint32_t pixel : pixels) {
      const int r = redFromArgb(pixel), g = greenFromArgb(pixel), b = blueFromArgb(pixel);
      const int idx = indexOf((r >> 3) + 1, (g >> 3) + 1, (b >> 3) + 1);
      ++weights[idx]; mr[idx] += r; mg[idx] += g; mb[idx] += b;
      m2[idx] += r*r + g*g + b*b;
    }
    for (int r = 1; r < kSide; ++r) {
      std::vector<int64_t> area(kSide), ar(kSide), ag(kSide), ab(kSide);
      std::vector<double> a2(kSide);
      for (int g = 1; g < kSide; ++g) {
        int64_t line=0, lr=0, lg=0, lb=0; double l2=0;
        for (int b = 1; b < kSide; ++b) {
          const int idx = indexOf(r,g,b), prev = indexOf(r-1,g,b);
          line += weights[idx]; lr += mr[idx]; lg += mg[idx]; lb += mb[idx]; l2 += m2[idx];
          area[b] += line; ar[b] += lr; ag[b] += lg; ab[b] += lb; a2[b] += l2;
          weights[idx] = weights[prev] + area[b]; mr[idx] = mr[prev] + ar[b];
          mg[idx] = mg[prev] + ag[b]; mb[idx] = mb[prev] + ab[b]; m2[idx] = m2[prev] + a2[b];
        }
      }
    }
    cubes.resize(maxColors);
    std::vector<double> volVariance(maxColors, 0);
    cubes[0].r1 = cubes[0].g1 = cubes[0].b1 = kSide - 1;
    int generatedCount = maxColors, next = 0;
    for (int i = 1; i < maxColors; ++i) {
      if (cut(cubes[next], cubes[i])) {
        volVariance[next] = cubes[next].vol > 1 ? variance(cubes[next]) : 0;
        volVariance[i] = cubes[i].vol > 1 ? variance(cubes[i]) : 0;
      } else {
        volVariance[next] = 0; --i;
      }
      next = 0; double temp = volVariance[0];
      for (int j = 1; j <= i; ++j) {
        if (volVariance[j] > temp) { temp = volVariance[j]; next = j; }
      }
      if (temp <= 0) { generatedCount = i + 1; break; }
    }
    std::vector<uint32_t> result;
    for (int i = 0; i < generatedCount; ++i) {
      const double w = volume(cubes[i], weights);
      if (w > 0) {
        const int r = roundHalfUp(volume(cubes[i], mr) / w);
        const int g = roundHalfUp(volume(cubes[i], mg) / w);
        const int b = roundHalfUp(volume(cubes[i], mb) / w);
        result.push_back(argbFromRgb(r,g,b));
      }
    }
    return result;
  }
 private:
  double variance(const Box& b) {
    const double dr = volume(b,mr), dg = volume(b,mg), db = volume(b,mb);
    const double xx = volumeDouble(b,m2);
    const double w = volume(b,weights);
    return xx - (dr*dr + dg*dg + db*db) / w;
  }
  double volumeDouble(const Box& b, const std::vector<double>& m) {
    return ::m3::volume(b,m);
  }
  MaxResult maximize(const Box& cube, int direction, int first, int last,
                     double wholeR, double wholeG, double wholeB, double wholeW) {
    double br=bottom(cube,direction,mr), bg=bottom(cube,direction,mg);
    double bb=bottom(cube,direction,mb), bw=bottom(cube,direction,weights);
    MaxResult result;
    for (int i = first; i < last; ++i) {
      double hr=br+top(cube,direction,i,mr), hg=bg+top(cube,direction,i,mg);
      double hb=bb+top(cube,direction,i,mb), hw=bw+top(cube,direction,i,weights);
      if (hw == 0) continue;
      double temp = (hr*hr + hg*hg + hb*hb) / hw;
      hr=wholeR-hr; hg=wholeG-hg; hb=wholeB-hb; hw=wholeW-hw;
      if (hw == 0) continue;
      temp += (hr*hr + hg*hg + hb*hb) / hw;
      if (temp > result.maximum) result = {i, temp};
    }
    return result;
  }
  bool cut(Box& one, Box& two) {
    const double wr=volume(one,mr), wg=volume(one,mg), wb=volume(one,mb), ww=volume(one,weights);
    const MaxResult r=maximize(one,0,one.r0+1,one.r1,wr,wg,wb,ww);
    const MaxResult g=maximize(one,1,one.g0+1,one.g1,wr,wg,wb,ww);
    const MaxResult b=maximize(one,2,one.b0+1,one.b1,wr,wg,wb,ww);
    int d;
    if (r.maximum >= g.maximum && r.maximum >= b.maximum) {
      if (r.cut < 0) return false;
      d = 0;
    } else if (g.maximum >= r.maximum && g.maximum >= b.maximum) d = 1;
    else d = 2;
    two.r1=one.r1; two.g1=one.g1; two.b1=one.b1;
    if (d == 0) {
      one.r1=r.cut; two.r0=one.r1; two.g0=one.g0; two.b0=one.b0;
    } else if (d == 1) {
      one.g1=g.cut; two.r0=one.r0; two.g0=one.g1; two.b0=one.b0;
    } else {
      one.b1=b.cut; two.r0=one.r0; two.g0=one.g0; two.b0=one.b1;
    }
    one.vol=(one.r1-one.r0)*(one.g1-one.g0)*(one.b1-one.b0);
    two.vol=(two.r1-two.r0)*(two.g1-two.g0)*(two.b1-two.b0);
    return true;
  }
  std::vector<int64_t> weights, mr, mg, mb;
  std::vector<double> m2;
  std::vector<Box> cubes;
};

double labDistance(const std::array<double,3>& a, const std::array<double,3>& b) {
  const double dl=a[0]-b[0], da=a[1]-b[1], db=a[2]-b[2];
  return dl*dl + da*da + db*db;
}
std::array<double,3> labArray(uint32_t argb) {
  const auto v=labFromArgb(argb); return {v[0],v[1],v[2]};
}
// Matches Java Random exactly (the C++ implementation may use integer math
// because the 48-bit recurrence fits in uint64_t).
class SeededRandom {
 public:
  explicit SeededRandom(uint64_t seed) : seed_((seed ^ 0x5deece66dULL) & ((1ULL<<48)-1)) {}
  uint64_t nextBits(int bits) {
    seed_ = (seed_ * 0x5deece66dULL + 0xb) & ((1ULL<<48)-1);
    return seed_ >> (48-bits);
  }
  int nextInt(int bound) {
    if (bound <= 0) return 0;
    if ((bound & -bound) == bound)
      return static_cast<int>((bound * static_cast<int64_t>(nextBits(31))) / 0x80000000LL);
    uint64_t bits; int value;
    do { bits=nextBits(31); value=static_cast<int>(bits%bound); }
    while (bits - static_cast<uint64_t>(value) + (bound-1) >= 0x80000000ULL);
    return value;
  }
  double nextDouble() {
    return (nextBits(26)*0x8000000ULL + nextBits(27)) / 9007199254740992.0;
  }
 private: uint64_t seed_;
};
}  // namespace

std::vector<uint32_t> QuantizerWu::quantize(const std::vector<uint32_t>& pixels,
                                            int maxColors) {
  return Wu().run(pixels,maxColors);
}

std::map<uint32_t,uint32_t> quantizeWsmeans(
    const std::vector<uint32_t>& inputPixels,
    const std::vector<uint32_t>& startingClusters, int maxColors) {
  std::map<uint32_t,uint32_t> out;
  if (inputPixels.empty() || maxColors <= 0) return out;
  SeededRandom random(0x42688);
  std::map<uint32_t,uint32_t> countsMap;
  std::vector<std::array<double,3>> points;
  std::vector<uint32_t> pixels;
  for (uint32_t input : inputPixels) {
    if (countsMap[input]++ == 0) {
      points.push_back(labArray(input)); pixels.push_back(input);
    }
  }
  const int pointCount = static_cast<int>(points.size());
  std::vector<int> counts(pointCount);
  for (int i = 0; i < pointCount; ++i) counts[i] = countsMap[pixels[i]];
  int clusterCount = std::min(maxColors,pointCount);
  if (!startingClusters.empty())
    clusterCount = std::min(clusterCount,static_cast<int>(startingClusters.size()));
  std::vector<std::array<double,3>> clusters;
  for (uint32_t c : startingClusters) clusters.push_back(labArray(c));
  const int needed=clusterCount-static_cast<int>(clusters.size());
  if (startingClusters.empty() && needed > 0) {
    for (int i=0;i<needed;++i)
      clusters.push_back({random.nextDouble()*100,
                          random.nextDouble()*201-100,
                          random.nextDouble()*201-100});
  }
  std::vector<int> clusterIndices;
  clusterIndices.reserve(pointCount);
  for (int i=0;i<pointCount;++i) clusterIndices.push_back(random.nextInt(clusterCount));
  std::vector<std::vector<int>> indexMatrix(clusterCount,std::vector<int>(clusterCount));
  std::vector<std::vector<std::pair<double,int>>> distances(clusterCount);
  for (auto& v : distances) {
    v.resize(clusterCount);
    for (auto& x : v) x={-1,-1};
  }
  std::vector<int64_t> pixelCountSums(clusterCount,0);
  for (int iteration=0;iteration<10;++iteration) {
    for (int i=0;i<clusterCount;++i) {
      for (int j=i+1;j<clusterCount;++j) {
        const double distance=labDistance(clusters[i],clusters[j]);
        distances[j][i]={distance,i}; distances[i][j]={distance,j};
      }
      std::stable_sort(distances[i].begin(),distances[i].end());
      for (int j=0;j<clusterCount;++j) indexMatrix[i][j]=distances[i][j].second;
    }
    int pointsMoved=0;
    for (int i=0;i<pointCount;++i) {
      const auto& point=points[i];
      const int previous=clusterIndices[i];
      const double previousDistance=labDistance(point,clusters[previous]);
      double minimum=previousDistance; int next=-1;
      for (int j=0;j<clusterCount;++j) {
        if (distances[previous][j].first >= 4*previousDistance) continue;
        const double distance=labDistance(point,clusters[j]);
        if (distance<minimum) { minimum=distance; next=j; }
      }
      if (next != -1) {
        const double change=std::abs(std::sqrt(minimum)-std::sqrt(previousDistance));
        if (change>3) { ++pointsMoved; clusterIndices[i]=next; }
      }
    }
    if (pointsMoved==0 && iteration!=0) break;
    std::vector<std::array<double,3>> sums(clusterCount,{0,0,0});
    std::fill(pixelCountSums.begin(),pixelCountSums.end(),0);
    for (int i=0;i<pointCount;++i) {
      const int idx=clusterIndices[i]; const int count=counts[i];
      pixelCountSums[idx]+=count;
      for (int k=0;k<3;++k) sums[idx][k]+=points[i][k]*count;
    }
    for (int i=0;i<clusterCount;++i) {
      const auto count=pixelCountSums[i];
      if (count==0) { clusters[i]={0,0,0}; continue; }
      for (int k=0;k<3;++k) clusters[i][k]=sums[i][k]/count;
    }
  }
  for (int i=0;i<clusterCount;++i) {
    const auto count=pixelCountSums[i]; if (!count) continue;
    const auto p=clusters[i]; const uint32_t c=argbFromLab(p[0],p[1],p[2]);
    if (!out.count(c)) out[c]=static_cast<uint32_t>(count);
  }
  return out;
}
std::map<uint32_t,uint32_t> quantizeCelebi(
    const std::vector<uint32_t>& pixels,int maxColors) {
  return quantizeWsmeans(pixels,Wu().run(pixels,maxColors),maxColors);
}
std::vector<uint32_t> quantizeWu(
    const std::vector<uint32_t>& pixels,int maxColors) {
  return Wu().run(pixels,maxColors);
}

}  // namespace m3

// HCT's exact sRGB gamut solver: Newton iteration followed by hue-segment
// bisection at the cube boundary. Ported from MCU's hct_solver.ts.
#include "m3/hct.hpp"
#include "m3/cam16.hpp"
#include "m3/color.hpp"
#include "m3/math.hpp"

#include <array>
#include <vector>

namespace m3 {
namespace {
using Vec3 = std::array<double, 3>;
constexpr double kPi = 3.14159265358979323846;
constexpr double kScaledDiscountFromLinrgb[3][3] = {
    {0.001200833568784504, 0.002389694492170889, 0.0002795742885861124},
    {0.0005891086651375999, 0.0029785502573438758, 0.0003270666104008398},
    {0.00010146692491640572, 0.0005364214359186694, 0.0032979401770712076}};
constexpr double kLinrgbFromScaledDiscount[3][3] = {
    {1373.2198709594231, -1100.4251190754821, -7.278681089101213},
    {-271.815969077903, 559.6580465940733, -32.46047482791194},
    {1.9622899599665666, -57.173814538844006, 308.7233197812385}};
constexpr double kYFromLinrgb[3] = {0.2126, 0.7152, 0.0722};
constexpr double kCriticalPlanes[255] = {
    0.015176349177441876,0.045529047532325624,0.07588174588720938,
    0.10623444424209313,0.13658714259697685,0.16693984095186062,
    0.19729253930674434,0.2276452376616281,0.2579979360165119,
    0.28835063437139563,0.3188300904430532,0.350925934958123,
    0.3848314933096426,0.42057480301049466,0.458183274052838,
    0.4976837250274023,0.5391024159806381,0.5824650784040898,
    0.6277969426914107,0.6751227633498623,0.7244668422128921,
    0.775853049866786,0.829304845476233,0.8848452951698498,
    0.942497089126609,1.0022825574869039,1.0642236851973577,
    1.1283421258858297,1.1946592148522128,1.2631959812511864,
    1.3339731595349034,1.407011200216447,1.4823302800086415,
    1.5599503113873272,1.6398909516233677,1.7221716113234105,
    1.8068114625156377,1.8938294463134073,1.9832442801866852,
    2.075074464868551,2.1693382909216234,2.2660538449872063,
    2.36523901573795,2.4669114995532007,2.5710888059345764,
    2.6777882626779785,2.7870270208169257,2.898822059350997,
    3.0131901897720907,3.1301480604002863,3.2497121605402226,
    3.3718988244681087,3.4967242352587946,3.624204428461639,
    3.754355295633311,3.887192587735158,4.022731918402185,
    4.160988767090289,4.301978482107941,4.445716283538092,
    4.592217266055746,4.741496401646282,4.893568542229298,
    5.048448422192488,5.20615066083972,5.3666897647573375,
    5.5300801301023865,5.696336044816294,5.865471690767354,
    6.037501145825082,6.212438385869475,6.390297286737924,
    6.571091626112461,6.7548350853498045,6.941541251256611,
    7.131223617812143,7.323895587840543,7.5195704746346665,
    7.7182615035334345,7.919981813454504,8.124744458384042,
    8.332562408825165,8.543448553206703,8.757415699253682,
    8.974476575321063,9.194643831691977,9.417930041841839,
    9.644347703669503,9.873909240696694,10.106627003236781,
    10.342513269534024,10.58158024687427,10.8238400726681,
    11.069304815507364,11.317986476196008,11.569896988756009,
    11.825048221409341,12.083451977536606,12.345119996613247,
    12.610063955123938,12.878295467455942,13.149826086772048,
    13.42466730586372,13.702830557985108,13.984327217668513,
    14.269168601521828,14.55736596900856,14.848930523210871,
    15.143873411576273,15.44220572664832,15.743938506781891,
    16.04908273684337,16.35764934889634,16.66964922287304,
    16.985093187232053,17.30399201960269,17.62635644741625,
    17.95219714852476,18.281524751807332,18.614349837764564,
    18.95068293910138,19.290534541298456,19.633915083172692,
    19.98083495742689,20.331304511189067,20.685334046541502,
    21.042933821039977,21.404114048223256,21.76888489811322,
    22.137256497705877,22.50923893145328,22.884842241736916,
    23.264076429332462,23.6469514538663,24.033477234264016,
    24.42366364919083,24.817520537484558,25.21505769858089,
    25.61628489293138,26.021211842414342,26.429848230738664,
    26.842203703840827,27.258287870275353,27.678110301598522,
    28.10168053274597,28.529008062403893,28.96010235337422,
    29.39497283293396,29.83362889318845,30.276079891419332,
    30.722335150426627,31.172403958865512,31.62629557157785,
    32.08401920991837,32.54558406207592,33.010999283389665,
    33.4802739966603,33.953417292456834,34.430438229418264,
    34.911345834551085,35.39614910352207,35.88485700094671,
    36.37747846067349,36.87402238606382,37.37449765026789,
    37.87891309649659,38.38727753828926,38.89959975977785,
    39.41588851594697,39.93615253289054,40.460400508064545,
    40.98864111053629,41.520882981230194,42.05713473317016,
    42.597404951718396,43.141702194811224,43.6900349931913,
    44.24241185063697,44.798841244188324,45.35933162437017,
    45.92389141541209,46.49252901546552,47.065252796817916,
    47.64207110610409,48.22299226451468,48.808024568002054,
    49.3971762874833,49.9904556690408,50.587870934119984,
    51.189430279724725,51.79514187861014,52.40501387947288,
    53.0190544071392,53.637271562750364,54.259673423945976,
    54.88626804504493,55.517063457223934,56.15206766869424,
    56.79128866487574,57.43473440856916,58.08241284012621,
    58.734331877617365,59.39049941699807,60.05092333227251,
    60.715611475655585,61.38457167773311,62.057811747619894,
    62.7353394731159,63.417162620860914,64.10328893648692,
    64.79372614476921,65.48848194977529,66.18756403501224,
    66.89098006357258,67.59873767827808,68.31084450182222,
    69.02730813691093,69.74813616640164,70.47333615344107,
    71.20291564160104,71.93688215501312,72.67524319850172,
    73.41800625771542,74.16517879925733,74.9167682708136,
    75.67278210128072,76.43322770089146,77.1981124613393,
    77.96744375590167,78.74122893956174,79.51947534912904,
    80.30219030335869,81.08938110306934,81.88105503125999,
    82.67721935322541,83.4778813166706,84.28304815182372,
    85.09272707154808,85.90692527145302,86.72564993000343,
    87.54890820862819,88.3767072518277,89.2090541872801,
    90.04595612594655,90.88742016217518,91.73345337380438,
    92.58406282226491,93.43925555268066,94.29903859396902,
    95.16341895893969,96.03240364439274,96.9059996312159,
    97.78421388448044,98.6670533535366,99.55452497210776};
Vec3 multiply(const Vec3& v, const double matrix[3][3]) {
  Vec3 result{};
  for (int r = 0; r < 3; ++r)
    result[r] = matrix[r][0] * v[0] + matrix[r][1] * v[1] + matrix[r][2] * v[2];
  return result;
}
double trueDelinearized(double component) {
  const double normalized = component / 100.0;
  const double d = normalized <= 0.0031308 ? normalized * 12.92
      : 1.055 * std::pow(normalized, 1.0 / 2.4) - 0.055;
  return d * 255;
}
double chromaticAdaptation(double component) {
  const double af = std::pow(std::abs(component), 0.42);
  return signum(component) * 400 * af / (af + 27.13);
}
double hueOf(const Vec3& linrgb) {
  const auto scaled = multiply(linrgb, kScaledDiscountFromLinrgb);
  const double rA = chromaticAdaptation(scaled[0]);
  const double gA = chromaticAdaptation(scaled[1]);
  const double bA = chromaticAdaptation(scaled[2]);
  return std::atan2((rA + gA - 2 * bA) / 9.0,
                    (11 * rA - 12 * gA + bA) / 11.0);
}
double sanitizeRadians(double angle) {
  return std::fmod(angle + kPi * 8, kPi * 2);
}
bool inCyclicOrder(double a, double b, double c) {
  return sanitizeRadians(b - a) < sanitizeRadians(c - a);
}
Vec3 setCoordinate(const Vec3& source, double coordinate,
                   const Vec3& target, int axis) {
  const double t = (coordinate - source[axis]) / (target[axis] - source[axis]);
  Vec3 result;
  for (int i = 0; i < 3; ++i) result[i] = source[i] + (target[i] - source[i]) * t;
  return result;
}
Vec3 nthVertex(double y, int n) {
  const double coordA = n % 4 <= 1 ? 0.0 : 100.0;
  const double coordB = n % 2 == 0 ? 0.0 : 100.0;
  Vec3 result;
  if (n < 4) {
    result = {(y - coordA * kYFromLinrgb[1] - coordB * kYFromLinrgb[2]) /
                  kYFromLinrgb[0], coordA, coordB};
  } else if (n < 8) {
    result = {coordB, (y - coordB * kYFromLinrgb[0] - coordA * kYFromLinrgb[2]) /
                  kYFromLinrgb[1], coordA};
  } else {
    result = {coordA, coordB, (y - coordA * kYFromLinrgb[0] -
                  coordB * kYFromLinrgb[1]) / kYFromLinrgb[2]};
  }
  const double value = n < 4 ? result[0] : n < 8 ? result[1] : result[2];
  return value >= 0 && value <= 100 ? result : Vec3{-1, -1, -1};
}
std::array<Vec3, 2> bisectToSegment(double y, double targetHue) {
  Vec3 left{-1, -1, -1}, right = left;
  double leftHue = 0, rightHue = 0;
  bool initialized = false, uncut = true;
  for (int n = 0; n < 12; ++n) {
    const Vec3 mid = nthVertex(y, n);
    if (mid[0] < 0) continue;
    const double midHue = hueOf(mid);
    if (!initialized) {
      left = right = mid;
      leftHue = rightHue = midHue;
      initialized = true;
      continue;
    }
    if (uncut || inCyclicOrder(leftHue, midHue, rightHue)) {
      uncut = false;
      if (inCyclicOrder(leftHue, targetHue, midHue)) {
        right = mid; rightHue = midHue;
      } else {
        left = mid; leftHue = midHue;
      }
    }
  }
  return {left, right};
}
Vec3 bisectToLimit(double y, double targetHue) {
  auto segment = bisectToSegment(y, targetHue);
  Vec3 left = segment[0], right = segment[1];
  double leftHue = hueOf(left);
  for (int axis = 0; axis < 3; ++axis) {
    if (left[axis] == right[axis]) continue;
    int lPlane = -1, rPlane = 255;
    if (left[axis] < right[axis]) {
      lPlane = static_cast<int>(std::floor(trueDelinearized(left[axis]) - 0.5));
      rPlane = static_cast<int>(std::ceil(trueDelinearized(right[axis]) - 0.5));
    } else {
      lPlane = static_cast<int>(std::ceil(trueDelinearized(left[axis]) - 0.5));
      rPlane = static_cast<int>(std::floor(trueDelinearized(right[axis]) - 0.5));
    }
    for (int i = 0; i < 8; ++i) {
      if (std::abs(rPlane - lPlane) <= 1) break;
      const int mPlane = static_cast<int>(std::floor((lPlane + rPlane) / 2.0));
      const Vec3 mid = setCoordinate(left, kCriticalPlanes[mPlane], right, axis);
      const double midHue = hueOf(mid);
      if (inCyclicOrder(leftHue, targetHue, midHue)) {
        right = mid; rPlane = mPlane;
      } else {
        left = mid; leftHue = midHue; lPlane = mPlane;
      }
    }
  }
  return {(left[0] + right[0]) / 2, (left[1] + right[1]) / 2,
          (left[2] + right[2]) / 2};
}
double inverseChromaticAdaptation(double adapted) {
  const double abs = std::abs(adapted);
  return signum(adapted) * std::pow(
      std::max(0.0, 27.13 * abs / (400.0 - abs)), 1.0 / 0.42);
}
uint32_t findResultByJ(double hueRadians, double chroma, double y) {
  double j = std::sqrt(y) * 11;
  const auto vc = ViewingConditions::srgb();
  const double tInnerCoeff = 1 / std::pow(1.64 - std::pow(0.29, vc.n), 0.73);
  const double eHue = 0.25 * (std::cos(hueRadians + 2.0) + 3.8);
  const double p1 = eHue * (50000.0 / 13.0) * vc.nc * vc.ncb;
  const double hSin = std::sin(hueRadians), hCos = std::cos(hueRadians);
  for (int iteration = 0; iteration < 5; ++iteration) {
    const double jNorm = j / 100.0;
    const double alpha = chroma == 0 || j == 0 ? 0 : chroma / std::sqrt(jNorm);
    const double t = std::pow(alpha * tInnerCoeff, 1.0 / 0.9);
    const double ac = vc.aw * std::pow(jNorm, 1.0 / vc.c / vc.z);
    const double p2 = ac / vc.nbb;
    const double gamma = 23 * (p2 + 0.305) * t /
        (23 * p1 + 11 * t * hCos + 108 * t * hSin);
    const double a = gamma * hCos, b = gamma * hSin;
    const Vec3 linrgb = multiply(
        {inverseChromaticAdaptation((460 * p2 + 451 * a + 288 * b) / 1403.0),
         inverseChromaticAdaptation((460 * p2 - 891 * a - 261 * b) / 1403.0),
         inverseChromaticAdaptation((460 * p2 - 220 * a - 6300 * b) / 1403.0)},
        kLinrgbFromScaledDiscount);
    if (linrgb[0] < 0 || linrgb[1] < 0 || linrgb[2] < 0) return 0;
    const double fnj = kYFromLinrgb[0] * linrgb[0] +
        kYFromLinrgb[1] * linrgb[1] + kYFromLinrgb[2] * linrgb[2];
    if (fnj <= 0) return 0;
    if (iteration == 4 || std::abs(fnj - y) < 0.002) {
      if (linrgb[0] > 100.01 || linrgb[1] > 100.01 || linrgb[2] > 100.01) return 0;
      return argbFromLinrgb({linrgb[0], linrgb[1], linrgb[2]});
    }
    j = j - (fnj - y) * j / (2 * fnj);
  }
  return 0;
}
}  // namespace

uint32_t solveHct(double hueDegrees, double chroma, double lstar) {
  if (chroma < 0.0001 || lstar < 0.0001 || lstar > 99.9999)
    return argbFromLstar(lstar);
  hueDegrees = sanitizeDegreesDouble(hueDegrees);
  const double hueRadians = hueDegrees / 180 * kPi;
  const double y = yFromLstar(lstar);
  const uint32_t exact = findResultByJ(hueRadians, chroma, y);
  if (exact != 0) return exact;
  const Vec3 linrgb = bisectToLimit(y, hueRadians);
  return argbFromLinrgb({linrgb[0], linrgb[1], linrgb[2]});
}
Hct Hct::fromHueChromaTone(double hue, double chroma, double tone) {
  return Hct(solveHct(hue, chroma, tone));
}
Hct::Hct(uint32_t argb) { setInternalState(argb); }
void Hct::setInternalState(uint32_t argb) {
  const Cam16 cam = Cam16::fromInt(argb);
  hue = cam.hue;
  chroma = cam.chroma;
  tone = lstarFromArgb(argb);
  argb_ = argb;
}
Hct Hct::fromInt(uint32_t argb) { return Hct(argb); }
void Hct::setHue(double value) { setInternalState(solveHct(value, chroma, tone)); }
void Hct::setChroma(double value) { setInternalState(solveHct(hue, value, tone)); }
void Hct::setTone(double value) { setInternalState(solveHct(hue, chroma, value)); }

}  // namespace m3

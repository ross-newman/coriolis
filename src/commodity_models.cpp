#include "open_elite/commodity_models.h"

#include <cmath>

#include "open_elite/model_builder.h"
#include "open_elite/model_parts.h"

namespace open_elite {

namespace {

using namespace parts;

/// Food: a wooden crate of produce, with an apple and a pear on top
Model BuildFood() {
  ModelBuilder b("FOOD");
  const Rgb wood{176, 122, 66};
  const Rgb dark{120, 80, 44};
  const Rgb slat{214, 166, 96};
  b.Box(-26.0f, -16.0f, -20.0f, 26.0f, 10.0f, 20.0f, wood, dark, false);
  for (const float z : {-20.0f, -6.0f, 12.0f}) {
    b.Box(-26.0f, 10.0f, z, 26.0f, 13.0f, z + 8.0f, slat, slat, false);
  }
  Ball(b, -10.0f, 21.0f, 0.0f, 9.0f, 8, 4, {{210, 52, 48}, {170, 40, 40}});
  Ball(b, 12.0f, 19.0f, 4.0f, 7.0f, 8, 4, {{92, 170, 64}, {70, 140, 50}});
  return b.Finish(3.2f);
}

/// Textiles: three rolls of cloth
Model BuildTextiles() {
  ModelBuilder b("TEXTILES");
  struct Roll {
    float x;
    float y;
    Rgb colour;
    Rgb shade;
  };
  const Roll rolls[] = {{-16.0f, -10.0f, {64, 110, 210}, {44, 80, 160}},
                        {16.0f, -10.0f, {206, 66, 80}, {158, 46, 60}},
                        {0.0f, 12.0f, {80, 180, 110}, {56, 140, 84}}};
  for (const Roll& r : rolls) {
    Tube(b, 12, 15.0f, r.x, r.y, -30.0f, 30.0f, {r.colour, r.shade}, {236, 230, 214}, r.shade);
  }
  return b.Finish(3.2f);
}

/// Radioactives: a barrel with hazard stripes, steel belts and a green glowing lid
Model BuildRadioactives() {
  ModelBuilder b("RADIOACTIVES");
  const Rgb yellow{240, 204, 50};
  const Rgb black{40, 40, 44};
  const Rgb steel{150, 154, 166};
  Tube(b, 12, 20.0f, 0.0f, 0.0f, -30.0f, 30.0f, {yellow, black}, {40, 220, 100}, black, true);
  for (const float z : {-17.0f, 11.0f}) {
    Tube(b, 12, 21.0f, 0.0f, 0.0f, z, z + 5.0f, {steel}, steel, steel);
  }
  return b.Finish(3.2f);
}

/// Slaves: a cage with a lock and a red light on top
Model BuildSlaves() {
  ModelBuilder b("SLAVES");
  const Rgb iron{130, 134, 146};
  const Rgb dark{54, 58, 70};
  const Rgb rust{170, 110, 56};
  b.Box(-26.0f, -22.0f, -26.0f, 26.0f, -18.0f, 26.0f, dark, dark, false);
  b.Box(-26.0f, 18.0f, -26.0f, 26.0f, 22.0f, 26.0f, dark, dark, false);
  for (const float x : {-22.0f, -11.0f, 0.0f, 11.0f, 22.0f}) {
    for (const float z : {-24.0f, 24.0f}) {
      b.Box(x - 1.5f, -18.0f, z - 1.5f, x + 1.5f, 18.0f, z + 1.5f, iron, iron, false);
    }
  }
  for (const float z : {-12.0f, 0.0f, 12.0f}) {
    for (const float x : {-24.0f, 24.0f}) {
      b.Box(x - 1.5f, -18.0f, z - 1.5f, x + 1.5f, 18.0f, z + 1.5f, iron, iron, false);
    }
  }
  b.Box(-5.0f, -6.0f, 25.5f, 5.0f, 6.0f, 30.0f, rust, rust, false);
  b.Box(-3.0f, 22.0f, -3.0f, 3.0f, 27.0f, 3.0f, {220, 60, 50}, {220, 60, 50}, true);
  return b.Finish(3.2f);
}

/// Liquor: a green bottle with a label and a cork
Model BuildLiquor() {
  ModelBuilder b("LIQUOR");
  const Rgb glass{40, 130, 70};
  const Rgb deep{30, 100, 56};
  b.Centre(0.0f, 0.0f, -8.0f);
  const Ring base = b.Ring(10, 12.0f, 12.0f, -34.0f, kPi / 10.0f);
  const Ring shoulder = b.Ring(10, 12.0f, 12.0f, 6.0f, kPi / 10.0f);
  const Ring neck_base = b.Ring(10, 5.0f, 5.0f, 18.0f, kPi / 10.0f);
  b.Band(base, shoulder, {glass, deep});
  b.Band(shoulder, neck_base, {glass, deep});
  b.Poly(neck_base, glass);
  b.Poly(Reversed(base), deep);
  Tube(b, 10, 5.0f, 0.0f, 0.0f, 16.0f, 34.0f, {glass, deep}, deep, deep);
  Tube(b, 10, 12.4f, 0.0f, 0.0f, -24.0f, -6.0f, {{238, 226, 190}}, {238, 226, 190}, {238, 226, 190});
  Tube(b, 8, 4.0f, 0.0f, 0.0f, 34.0f, 40.0f, {{170, 120, 70}}, {196, 146, 90}, {170, 120, 70});
  return b.Finish(3.2f);
}

/// Luxuries: a purple gift box with a gold ribbon and bow
Model BuildLuxuries() {
  ModelBuilder b("LUXURIES");
  const Rgb purple{136, 74, 190};
  const Rgb deep{100, 52, 150};
  const Rgb gold{244, 200, 70};
  b.Box(-22.0f, -14.0f, -22.0f, 22.0f, 10.0f, 22.0f, purple, deep, false);
  b.Box(-24.0f, 10.0f, -24.0f, 24.0f, 16.0f, 24.0f, deep, deep, false);
  b.Box(-3.0f, -14.5f, -24.5f, 3.0f, 16.5f, 24.5f, gold, gold, false);
  b.Box(-24.5f, -14.5f, -3.0f, 24.5f, 16.8f, 3.0f, gold, gold, false);
  Ball(b, -7.0f, 22.0f, 0.0f, 7.0f, 8, 4, {gold, {214, 166, 50}});
  Ball(b, 7.0f, 22.0f, 0.0f, 7.0f, 8, 4, {gold, {214, 166, 50}});
  Ball(b, 0.0f, 20.0f, 0.0f, 4.0f, 8, 4, {{255, 224, 120}});
  return b.Finish(3.2f);
}

/// Narcotics: a pill bottle and two capsules
Model BuildNarcotics() {
  ModelBuilder b("NARCOTICS");
  const Rgb amber{214, 140, 40};
  const Rgb deep{170, 104, 28};
  const Rgb white{238, 238, 244};
  Tube(b, 12, 14.0f, 0.0f, 0.0f, -26.0f, 18.0f, {amber, deep}, deep, deep);
  Tube(b, 12, 14.4f, 0.0f, 0.0f, -16.0f, 8.0f, {white}, white, white);
  Tube(b, 12, 15.0f, 0.0f, 0.0f, 18.0f, 28.0f, {white, {200, 200, 210}}, white, white);
  // Two capsules lying beside it, red and white
  Tube(b, 8, 4.0f, 26.0f, -10.0f, -10.0f, 0.0f, {{206, 56, 56}}, {206, 56, 56}, {206, 56, 56});
  Tube(b, 8, 4.0f, 26.0f, -10.0f, 0.0f, 10.0f, {white}, white, white);
  Tube(b, 8, 4.0f, -24.0f, -12.0f, 2.0f, 12.0f, {{206, 56, 56}}, {206, 56, 56}, {206, 56, 56});
  Tube(b, 8, 4.0f, -24.0f, -12.0f, 12.0f, 22.0f, {white}, white, white);
  return b.Finish(3.2f);
}

/// Computers: a monitor with text on the screen, on a stand, with a keyboard
Model BuildComputers() {
  ModelBuilder b("COMPUTERS");
  const Rgb steel{150, 156, 170};
  const Rgb dark{50, 54, 66};
  const Rgb screen{8, 30, 20};
  const Rgb green{90, 230, 130};
  b.Box(-26.0f, -6.0f, -14.0f, 26.0f, 28.0f, 8.0f, steel, dark, false);
  b.Box(-6.0f, -12.0f, -8.0f, 6.0f, -6.0f, 2.0f, dark, dark, false);
  b.Box(-20.0f, -18.0f, -14.0f, 20.0f, -12.0f, 10.0f, steel, dark, false);
  b.Box(-24.0f, -22.0f, 12.0f, 24.0f, -17.0f, 28.0f, dark, dark, false);
  b.Centre(0.0f, 11.0f, 0.0f);
  const float z = 8.4f;
  const float zt = 8.9f;
  b.Quad(b.Vertex(-21.0f, -1.0f, z), b.Vertex(21.0f, -1.0f, z), b.Vertex(21.0f, 23.0f, z), b.Vertex(-21.0f, 23.0f, z),
         screen);
  const auto line = [&](float x0, float x1, float y0) {
    b.Quad(b.Vertex(x0, y0, zt), b.Vertex(x1, y0, zt), b.Vertex(x1, y0 + 1.8f, zt), b.Vertex(x0, y0 + 1.8f, zt), green,
           true);
  };
  line(-17.0f, 12.0f, 17.0f);
  line(-17.0f, 4.0f, 12.0f);
  line(-17.0f, 16.0f, 7.0f);
  line(-17.0f, -12.0f, 2.0f);
  return b.Finish(3.2f);
}

/// Machinery: a toothed gear on an axle, with an orange hub
Model BuildMachinery() {
  ModelBuilder b("MACHINERY");
  const Rgb steel{150, 158, 176};
  const Rgb shade{104, 112, 132};
  const Rgb orange{232, 140, 40};
  const Rgb deep{180, 100, 30};
  Tube(b, 20, 18.0f, 0.0f, 0.0f, -7.0f, 7.0f, {steel, shade}, {188, 196, 212}, shade);
  const int teeth = 10;
  for (int i = 0; i < teeth; ++i) {
    const float a = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(teeth);
    const float c = std::cos(a);
    const float s = std::sin(a);
    const float r0 = 16.0f;
    const float r1 = 26.0f;
    const float w0 = 5.0f;
    const float w1 = 3.5f;
    Plate(b,
          {{c * r0 - s * w0, s * r0 + c * w0},
           {c * r1 - s * w1, s * r1 + c * w1},
           {c * r1 + s * w1, s * r1 - c * w1},
           {c * r0 + s * w0, s * r0 - c * w0}},
          Axis::kZ, -6.0f, 6.0f, steel, shade, shade);
  }
  Tube(b, 8, 8.0f, 0.0f, 0.0f, -11.0f, 11.0f, {orange, deep}, orange, deep);
  Tube(b, 6, 3.5f, 0.0f, 0.0f, -26.0f, 26.0f, {shade}, shade, shade);
  return b.Finish(3.2f);
}

/// Alloys: a bundle of seven metal rods
Model BuildAlloys() {
  ModelBuilder b("ALLOYS");
  const Rgb silver{190, 198, 212};
  const Rgb blue{130, 144, 172};
  Tube(b, 8, 7.0f, 0.0f, 0.0f, -38.0f, 38.0f, {silver, blue}, {226, 232, 242}, blue);
  for (int i = 0; i < 6; ++i) {
    const float a = 2.0f * kPi * static_cast<float>(i) / 6.0f;
    Tube(b, 8, 7.0f, 14.0f * std::cos(a), 14.0f * std::sin(a), -34.0f, 34.0f, {i % 2 ? silver : blue, silver},
         {226, 232, 242}, blue);
  }
  return b.Finish(3.2f);
}

/// Firearms: a rifle with a wooden stock and grip
Model BuildFirearms() {
  ModelBuilder b("FIREARMS");
  const Rgb dark{46, 50, 58};
  const Rgb steel{110, 116, 130};
  const Rgb wood{150, 98, 52};
  b.Box(-5.0f, -4.0f, -16.0f, 5.0f, 9.0f, 22.0f, steel, dark, false);
  Tube(b, 8, 3.5f, 0.0f, 5.0f, 22.0f, 58.0f, {dark, steel}, dark, dark);
  b.Box(-4.0f, -20.0f, -14.0f, 4.0f, -4.0f, -4.0f, wood, wood, false);
  b.Box(-4.0f, -6.0f, -36.0f, 4.0f, 8.0f, -16.0f, wood, wood, false);
  b.Box(-2.0f, -10.0f, 6.0f, 2.0f, -4.0f, 8.0f, dark, dark, false);
  b.Box(-2.0f, -10.0f, 6.0f, 2.0f, -8.0f, 16.0f, dark, dark, false);
  b.Box(-2.0f, -10.0f, 14.0f, 2.0f, -4.0f, 16.0f, dark, dark, false);
  b.Box(-1.5f, 9.0f, 14.0f, 1.5f, 13.0f, 16.0f, dark, dark, false);
  return b.Finish(3.2f);
}

/// Furs: a pelt lying flat, with a lighter back, spots, a head, a tail and four legs
Model BuildFurs() {
  ModelBuilder b("FURS");
  const Rgb brown{150, 98, 56};
  const Rgb deep{104, 66, 36};
  const Rgb tan{206, 158, 104};
  const Rgb spot{84, 52, 30};
  const float radius[8] = {30.0f, 26.0f, 24.0f, 27.0f, 30.0f, 26.0f, 24.0f, 27.0f};
  Outline body;
  Outline back;
  for (int i = 0; i < 8; ++i) {
    const float a = 2.0f * kPi * static_cast<float>(i) / 8.0f + kPi / 8.0f;
    body.push_back({std::cos(a) * radius[i], std::sin(a) * radius[i] * 0.75f});
    back.push_back({std::cos(a) * radius[i] * 0.78f, std::sin(a) * radius[i] * 0.58f});
  }
  Plate(b, body, Axis::kY, 0.0f, 4.0f, brown, deep, deep);
  Plate(b, back, Axis::kY, 4.0f, 6.0f, tan, brown, brown);
  for (const std::pair<float, float>& at : {std::make_pair(-8.0f, 4.0f), std::make_pair(6.0f, -6.0f),
                                           std::make_pair(10.0f, 8.0f), std::make_pair(-14.0f, -6.0f)}) {
    Plate(b, {{at.first - 3.0f, at.second - 2.0f}, {at.first + 3.0f, at.second - 2.0f}, {at.first + 3.0f, at.second + 2.0f},
              {at.first - 3.0f, at.second + 2.0f}},
          Axis::kY, 6.0f, 7.0f, spot, spot, spot);
  }
  // The head, the tail and the legs
  Plate(b, {{30.0f, -6.0f}, {44.0f, -3.0f}, {44.0f, 3.0f}, {30.0f, 6.0f}}, Axis::kY, 0.0f, 4.0f, brown, deep, deep);
  Plate(b, {{-30.0f, -3.0f}, {-52.0f, -2.0f}, {-52.0f, 2.0f}, {-30.0f, 3.0f}}, Axis::kY, 0.0f, 3.0f, tan, brown, brown);
  for (const float x : {-20.0f, 20.0f}) {
    for (const float z : {-1.0f, 1.0f}) {
      Plate(b, {{x - 5.0f, z * 18.0f}, {x + 5.0f, z * 18.0f}, {x + 3.0f, z * 32.0f}, {x - 3.0f, z * 32.0f}}, Axis::kY, 0.0f,
            4.0f, brown, deep, deep);
    }
  }
  return b.Finish(3.2f);
}

/// Minerals: a cluster of crystals growing from a dark stone
Model BuildMinerals() {
  ModelBuilder b("MINERALS");
  const Rgb stone{92, 84, 80};
  const Rgb dark{62, 56, 54};
  Tube(b, 8, 24.0f, 0.0f, 0.0f, -14.0f, -4.0f, {stone, dark}, stone, dark);
  struct Spike {
    Vec3 dir;
    float radius;
    float length;
    Rgb colour;
    Rgb tip;
  };
  const Spike spikes[] = {{{0.0f, 0.0f, 1.0f}, 6.5f, 44.0f, {70, 200, 190}, {150, 240, 230}},
                          {{0.40f, 0.0f, 0.92f}, 5.0f, 34.0f, {110, 150, 230}, {180, 200, 250}},
                          {{-0.40f, 0.10f, 0.91f}, 5.5f, 36.0f, {170, 110, 220}, {210, 170, 250}},
                          {{0.10f, 0.42f, 0.90f}, 4.5f, 28.0f, {70, 200, 190}, {150, 240, 230}},
                          {{0.10f, -0.42f, 0.90f}, 4.5f, 30.0f, {110, 150, 230}, {180, 200, 250}}};
  for (const Spike& s : spikes) {
    const float l = std::sqrt(s.dir.x * s.dir.x + s.dir.y * s.dir.y + s.dir.z * s.dir.z);
    Crystal(b, {s.dir.x * 6.0f, s.dir.y * 6.0f, -4.0f}, {s.dir.x / l, s.dir.y / l, s.dir.z / l}, s.radius, s.length,
            s.colour, s.tip);
  }
  return b.Finish(3.2f);
}

/// A trapezoid bar, 40 long, on its bottom at (x, y, z) and running along the axis. Its top is narrower than its base.
void Ingot(ModelBuilder& b, Axis axis, float x, float y, float z, Rgb body, Rgb end) {
  const float centre = axis == Axis::kZ ? x : z;
  const Outline profile{{centre - 14.0f, y}, {centre + 14.0f, y}, {centre + 9.0f, y + 8.0f}, {centre - 9.0f, y + 8.0f}};
  if (axis == Axis::kZ) {
    Plate(b, profile, Axis::kZ, z - 20.0f, z + 20.0f, end, body, end);
  } else {
    Plate(b, profile, Axis::kX, x - 20.0f, x + 20.0f, end, body, end);
  }
}

/// Gold: a pyramid of three gold bars
Model BuildGold() {
  ModelBuilder b("GOLD");
  const Rgb gold{244, 200, 64};
  const Rgb end{200, 150, 36};
  Ingot(b, Axis::kZ, -16.0f, 0.0f, 0.0f, gold, end);
  Ingot(b, Axis::kZ, 16.0f, 0.0f, 0.0f, gold, end);
  Ingot(b, Axis::kZ, 0.0f, 8.0f, 0.0f, {255, 224, 110}, end);
  return b.Finish(3.2f);
}

/// Platinum: four silver-white bars laid crosswise in two layers
Model BuildPlatinum() {
  ModelBuilder b("PLATINUM");
  const Rgb body{226, 232, 242};
  const Rgb end{160, 172, 192};
  Ingot(b, Axis::kZ, -12.0f, 0.0f, 0.0f, body, end);
  Ingot(b, Axis::kZ, 12.0f, 0.0f, 0.0f, body, end);
  Ingot(b, Axis::kX, 0.0f, 8.0f, -12.0f, {240, 244, 252}, end);
  Ingot(b, Axis::kX, 0.0f, 8.0f, 12.0f, {240, 244, 252}, end);
  return b.Finish(3.2f);
}

/// Gem-stones: a cut gem with a table on top, a crown, a girdle and a point below
Model BuildGemStones() {
  ModelBuilder b("GEM-STONES");
  const Rgb ice{200, 236, 255};
  const Rgb cyan{90, 200, 240};
  const Rgb blue{50, 120, 220};
  const Rgb deep{36, 84, 180};
  b.Centre(0.0f, 0.0f, -4.0f);
  const Ring table = b.Ring(8, 11.0f, 11.0f, 14.0f, kPi / 8.0f);
  const Ring girdle = b.Ring(8, 24.0f, 24.0f, 2.0f);
  const Ring under = b.Ring(8, 24.0f, 24.0f, -1.0f);
  const std::size_t point = b.Vertex(0.0f, 0.0f, -28.0f);
  b.Poly(table, {240, 250, 255}, true);
  b.Band(table, girdle, {ice, cyan});
  b.Band(girdle, under, {cyan, ice});
  b.Fan(point, Reversed(under), {blue, cyan, deep, cyan});
  return b.Finish(3.2f);
}

/// Alien items: a glowing two-coloured spindle inside a ring of glowing segments
Model BuildAlienItems() {
  ModelBuilder b("ALIEN ITEMS");
  const Rgb violet{140, 70, 200};
  const Rgb teal{60, 220, 200};
  const Rgb dark{30, 110, 110};
  b.Centre(0.0f, 0.0f, 0.0f);
  const Ring middle = b.Ring(6, 16.0f, 16.0f, 0.0f);
  const std::size_t top = b.Vertex(0.0f, 0.0f, 30.0f);
  const std::size_t bottom = b.Vertex(0.0f, 0.0f, -30.0f);
  for (std::size_t i = 0; i < 6; ++i) {
    const std::size_t j = (i + 1) % 6;
    b.Tri(top, middle[i], middle[j], i % 2 ? violet : teal, i % 2 == 0);
    b.Tri(bottom, middle[j], middle[i], i % 2 ? teal : violet, i % 2 == 1);
  }
  const int segments = 10;
  for (int i = 0; i < segments; ++i) {
    const float a0 = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(segments);
    const float a1 = a0 + 2.0f * kPi * 0.8f / static_cast<float>(segments);
    const float r0 = 26.0f;
    const float r1 = 34.0f;
    Plate(b,
          {{r0 * std::cos(a0), r0 * std::sin(a0)},
           {r1 * std::cos(a0), r1 * std::sin(a0)},
           {r1 * std::cos(a1), r1 * std::sin(a1)},
           {r0 * std::cos(a1), r0 * std::sin(a1)}},
          Axis::kZ, -2.0f, 2.0f, teal, dark, dark, true);
  }
  return b.Finish(3.2f);
}

/// The models, in Commodity order
std::vector<Model> BuildAllCommodityModels() {
  std::vector<Model> goods;
  goods.push_back(BuildFood());
  goods.push_back(BuildTextiles());
  goods.push_back(BuildRadioactives());
  goods.push_back(BuildSlaves());
  goods.push_back(BuildLiquor());
  goods.push_back(BuildLuxuries());
  goods.push_back(BuildNarcotics());
  goods.push_back(BuildComputers());
  goods.push_back(BuildMachinery());
  goods.push_back(BuildAlloys());
  goods.push_back(BuildFirearms());
  goods.push_back(BuildFurs());
  goods.push_back(BuildMinerals());
  goods.push_back(BuildGold());
  goods.push_back(BuildPlatinum());
  goods.push_back(BuildGemStones());
  goods.push_back(BuildAlienItems());
  return goods;
}

}  // namespace

const std::vector<Model>& AllCommodityModels() {
  static const std::vector<Model> goods = BuildAllCommodityModels();
  return goods;
}

const Model& GetCommodityModel(Commodity item) { return AllCommodityModels()[static_cast<std::size_t>(item)]; }

}  // namespace open_elite

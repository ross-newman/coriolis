#include "open_elite/ships.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <utility>

#include "open_elite/hash.h"
#include "open_elite/model_builder.h"

namespace open_elite {

namespace {

/// A cross section of a lofted hull
struct HullSection {
  float z;           // position along the ship
  float hw;          // half width at the wing edges
  float top;         // roof height on the centre line
  float upper;       // wing edge, upper side
  float lower;       // wing edge, lower side
  float keel;        // lowest point on the centre line
  float shoulder_x;  // where the roof meets the sides, as a fraction of the half width
  float shoulder_y;  // ... and of the roof height
};

/// Loft a hull through cross sections, with a pointed nose and a flat tail
void AddHull(ModelBuilder& b, const std::vector<HullSection>& sections, Vec3 nose, Rgb hull, Rgb roof, Rgb canopy,
             Rgb tail) {
  // Each ring: roof centre, right roof, right wing edge (upper, lower), keel, then the same on the left
  std::vector<std::array<std::size_t, 8>> ring;
  float z_sum = 0.0f;
  for (const HullSection& s : sections) {
    std::array<std::size_t, 8> r{};
    r[0] = b.Vertex(0.0f, s.top, s.z);
    r[1] = b.Vertex(s.hw * s.shoulder_x, s.top * s.shoulder_y, s.z);
    r[2] = b.Vertex(s.hw, s.upper, s.z);
    r[3] = b.Vertex(s.hw, s.lower, s.z);
    r[4] = b.Vertex(0.0f, s.keel, s.z);
    r[5] = b.Vertex(-s.hw, s.lower, s.z);
    r[6] = b.Vertex(-s.hw, s.upper, s.z);
    r[7] = b.Vertex(-s.hw * s.shoulder_x, s.top * s.shoulder_y, s.z);
    ring.push_back(r);
    z_sum += s.z;
  }
  b.Centre(0.0f, 0.0f, z_sum / static_cast<float>(sections.size()));

  const auto is_roof = [](std::size_t i) { return i == 0 || i == 1 || i == 6 || i == 7; };
  const std::size_t point = b.Vertex(nose.x, nose.y, nose.z);
  for (std::size_t i = 0; i < 8; ++i) {
    b.Tri(point, ring.front()[i], ring.front()[(i + 1) % 8], is_roof(i) ? roof : hull);
  }
  for (std::size_t s = 0; s + 1 < sections.size(); ++s) {
    for (std::size_t i = 0; i < 8; ++i) {
      const std::size_t j = (i + 1) % 8;
      Rgb colour = is_roof(i) ? roof : hull;
      if (s == 0 && (i == 0 || i == 7)) {
        colour = canopy;
      }
      b.Quad(ring[s][i], ring[s][j], ring[s + 1][j], ring[s + 1][i], colour);
    }
  }
  const std::size_t rear = b.Vertex(0.0f, 0.0f, sections.back().z);
  for (std::size_t i = 0; i < 8; ++i) {
    b.Tri(rear, ring.back()[i], ring.back()[(i + 1) % 8], tail, false, 0x2);
  }
}

/// The Cobra Mk III, modelled from a three view silhouette (the Elite Wiki, wiki.alioth.net) (a flat wedge with a blunt
/// nose, a ridge behind the cockpit and wings swept at 45 degrees). The same blueprint is used for the bounty hunter
/// and the trader, which differ only in their paint.
Model BuildCobra(const char* name, Rgb hull, Rgb roof, Rgb accent, Rgb dark) {
  ModelBuilder b(name);
  const std::size_t f1 = b.Vertex(-31.0f, 0.0f, 50.0f);
  const std::size_t f2 = b.Vertex(31.0f, 0.0f, 50.0f);
  const std::size_t apex = b.Vertex(0.0f, 17.0f, 14.0f);
  const std::size_t w1 = b.Vertex(-110.0f, 0.0f, -27.0f);
  const std::size_t w2 = b.Vertex(110.0f, 0.0f, -27.0f);
  const std::size_t r1 = b.Vertex(-110.0f, 0.0f, -51.0f);
  const std::size_t r2 = b.Vertex(110.0f, 0.0f, -51.0f);
  const std::size_t top = b.Vertex(0.0f, 6.0f, -51.0f);
  const std::size_t keel = b.Vertex(0.0f, -14.0f, -51.0f);
  b.Centre(0.0f, 1.0f, -1.0f);

  // Upper surface
  b.Tri(f1, f2, apex, accent);
  b.Tri(f2, w2, apex, roof);
  b.Tri(f1, apex, w1, roof);
  b.Quad(apex, w2, r2, top, roof);
  b.Quad(apex, top, r1, w1, roof);
  // Lower surface
  b.Tri(f1, f2, keel, hull);
  b.Tri(f2, w2, keel, hull);
  b.Tri(w2, r2, keel, hull);
  b.Tri(f1, w1, keel, hull);
  b.Tri(w1, r1, keel, hull);
  // Tail, with the glow of the engines
  b.Quad(r1, top, r2, keel, dark);
  const std::size_t g0 = b.Vertex(-26.0f, -7.0f, -51.6f);
  const std::size_t g1 = b.Vertex(26.0f, -7.0f, -51.6f);
  const std::size_t g2 = b.Vertex(16.0f, 1.0f, -51.6f);
  const std::size_t g3 = b.Vertex(-16.0f, 1.0f, -51.6f);
  b.Quad(g0, g1, g2, g3, {255, 160, 56}, true);
  return b.Finish(3.2f);
}

Model BuildSidewinder() {
  ModelBuilder b("SIDEWINDER");
  const Rgb hull{168, 178, 196};
  const Rgb roof{204, 210, 222};
  const Rgb accent{196, 60, 52};
  const Rgb dark{54, 58, 70};
  const std::array<std::size_t, 5> rim{b.Vertex(0.0f, 0.0f, 55.0f), b.Vertex(48.0f, 0.0f, -32.0f),
                                       b.Vertex(16.0f, 0.0f, -44.0f), b.Vertex(-16.0f, 0.0f, -44.0f),
                                       b.Vertex(-48.0f, 0.0f, -32.0f)};
  const std::size_t top = b.Vertex(0.0f, 13.0f, -6.0f);
  const std::size_t bottom = b.Vertex(0.0f, -9.0f, -6.0f);
  b.Centre(0.0f, 1.0f, -6.0f);
  const std::array<Rgb, 5> upper{roof, accent, dark, accent, roof};
  const std::array<Rgb, 5> lower{hull, hull, dark, hull, hull};
  for (std::size_t i = 0; i < 5; ++i) {
    const std::size_t j = (i + 1) % 5;
    b.Tri(top, rim[i], rim[j], upper[i]);
    b.Tri(bottom, rim[i], rim[j], lower[i]);
  }
  b.Box(-9.0f, -5.0f, -53.0f, 9.0f, 5.0f, -43.0f, dark, {255, 160, 56}, true);
  return b.Finish(3.2f);
}

Model BuildViper() {
  ModelBuilder b("VIPER");
  AddHull(b,
          {{42.0f, 14.0f, 9.0f, 4.0f, -4.0f, -9.0f, 0.70f, 0.85f},
           {6.0f, 34.0f, 15.0f, 10.0f, -10.0f, -15.0f, 0.75f, 0.90f},
           {-44.0f, 38.0f, 15.0f, 11.0f, -11.0f, -15.0f, 0.75f, 0.90f}},
          {0.0f, 0.0f, 92.0f}, {70, 92, 150}, {206, 212, 226}, {30, 40, 60}, {40, 44, 56});
  for (const float x : {-22.0f, 22.0f}) {
    b.Box(x - 9.0f, -9.0f, -60.0f, x + 9.0f, 9.0f, -44.0f, {50, 56, 72}, {255, 160, 56}, true);
  }
  return b.Finish(3.2f);
}

Model BuildMamba() {
  ModelBuilder b("MAMBA");
  AddHull(b,
          {{62.0f, 8.0f, 5.0f, 2.0f, -2.0f, -5.0f, 0.5f, 0.6f},
           {8.0f, 40.0f, 10.0f, 4.0f, -4.0f, -10.0f, 0.5f, 0.6f},
           {-40.0f, 30.0f, 9.0f, 4.0f, -4.0f, -9.0f, 0.5f, 0.6f}},
          {0.0f, 0.0f, 100.0f}, {214, 170, 50}, {236, 202, 90}, {36, 40, 56}, {40, 40, 44});
  for (const float x : {-16.0f, 0.0f, 16.0f}) {
    b.Box(x - 6.0f, -5.0f, -54.0f, x + 6.0f, 5.0f, -40.0f, {60, 56, 50}, {255, 160, 56}, true);
  }
  return b.Finish(3.2f);
}

Model BuildPython() {
  ModelBuilder b("PYTHON");
  AddHull(b,
          {{72.0f, 18.0f, 15.0f, 8.0f, -8.0f, -12.0f, 0.80f, 0.90f},
           {36.0f, 44.0f, 26.0f, 16.0f, -14.0f, -24.0f, 0.80f, 0.90f},
           {-72.0f, 46.0f, 26.0f, 16.0f, -14.0f, -24.0f, 0.80f, 0.90f}},
          {0.0f, -6.0f, 110.0f}, {176, 148, 108}, {204, 178, 134}, {40, 62, 96}, {64, 56, 48});
  for (const float x : {-26.0f, 0.0f, 26.0f}) {
    b.Box(x - 9.0f, -12.0f, -92.0f, x + 9.0f, 8.0f, -72.0f, {78, 70, 62}, {255, 160, 56}, true);
  }
  return b.Finish(3.2f);
}

Model BuildThargoid() {
  ModelBuilder b("THARGOID");
  const Rgb green{74, 168, 84};
  const Rgb light{96, 196, 108};
  const Rgb dark{30, 92, 48};
  const float phase = kPi / 8.0f;
  b.Centre(0.0f, 0.0f, 4.0f);
  const std::vector<std::size_t> rim_rear = b.Ring(8, 60.0f, 60.0f, -14.0f, phase);
  const std::vector<std::size_t> rim_front = b.Ring(8, 60.0f, 60.0f, 14.0f, phase);
  const std::vector<std::size_t> shoulder = b.Ring(8, 40.0f, 40.0f, 14.0f, phase);
  const std::vector<std::size_t> collar = b.Ring(8, 30.0f, 30.0f, 30.0f, phase);
  const std::vector<std::size_t> iris = b.Ring(8, 14.0f, 14.0f, 30.4f, phase);
  b.Band(rim_rear, rim_front, {green, light});
  b.Band(rim_front, shoulder, {light, green});
  b.Band(shoulder, collar, {dark, green});
  b.Poly(collar, dark);
  b.Poly(iris, {200, 255, 210}, true);
  b.Poly(std::vector<std::size_t>(rim_rear.rbegin(), rim_rear.rend()), dark);
  return b.Finish(3.2f);
}

Model BuildThargon() {
  ModelBuilder b("THARGON");
  const Rgb copper{176, 98, 64};
  const Rgb dark{104, 52, 40};
  const float phase = kPi / 8.0f;
  b.Centre(0.0f, 0.0f, 6.0f);
  const std::vector<std::size_t> rear = b.Ring(8, 40.0f, 40.0f, -12.0f, phase);
  const std::vector<std::size_t> front = b.Ring(8, 40.0f, 40.0f, 10.0f, phase);
  const std::size_t tip = b.Vertex(0.0f, 0.0f, 40.0f);
  b.Band(rear, front, {copper, dark});
  b.Fan(tip, front, {dark, copper});
  b.Poly(std::vector<std::size_t>(rear.rbegin(), rear.rend()), dark);
  return b.Finish(3.2f);
}

Model BuildCoriolis() {
  ModelBuilder b("CORIOLIS STATION", Spin::kRoll);
  const Rgb square{176, 186, 204};
  const Rgb corner{120, 132, 156};
  const float r = 60.0f;
  b.Centre(0.0f, 0.0f, 0.0f);
  // A cuboctahedron: 12 vertices at the middles of the edges of a cube
  std::size_t index[3][3][3];
  for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {
      for (int z = -1; z <= 1; ++z) {
        if ((x != 0) + (y != 0) + (z != 0) == 2) {
          index[x + 1][y + 1][z + 1] =
              b.Vertex(static_cast<float>(x) * r, static_cast<float>(y) * r, static_cast<float>(z) * r);
        }
      }
    }
  }
  // Triangles on the corners of the cube
  for (int sx = -1; sx <= 1; sx += 2) {
    for (int sy = -1; sy <= 1; sy += 2) {
      for (int sz = -1; sz <= 1; sz += 2) {
        b.Tri(index[sx + 1][sy + 1][1], index[sx + 1][1][sz + 1], index[1][sy + 1][sz + 1], corner);
      }
    }
  }
  // Squares on the faces of the cube
  for (int s = -1; s <= 1; s += 2) {
    b.Quad(index[1 + s][2][1], index[1 + s][1][2], index[1 + s][0][1], index[1 + s][1][0], square);
    b.Quad(index[2][1 + s][1], index[1][1 + s][2], index[0][1 + s][1], index[1][1 + s][0], square);
    b.Quad(index[2][1][1 + s], index[1][2][1 + s], index[0][1][1 + s], index[1][0][1 + s], square);
  }
  // The docking slot in the face at the front (+z)
  const float z1 = r + 0.3f;
  const float z2 = r + 0.6f;
  b.Quad(b.Vertex(-27.0f, -13.0f, z1), b.Vertex(27.0f, -13.0f, z1), b.Vertex(27.0f, 13.0f, z1),
         b.Vertex(-27.0f, 13.0f, z1), {236, 206, 76}, true);
  b.Quad(b.Vertex(-21.0f, -8.0f, z2), b.Vertex(21.0f, -8.0f, z2), b.Vertex(21.0f, 8.0f, z2), b.Vertex(-21.0f, 8.0f, z2),
         {8, 10, 16});
  return b.Finish(3.2f);
}

Model BuildMissile() {
  ModelBuilder b("MISSILE");
  const Rgb body{228, 230, 236};
  const Rgb red{208, 52, 44};
  const Rgb dark{90, 96, 110};
  b.Box(-6.0f, -6.0f, -40.0f, 6.0f, 6.0f, 30.0f, body, {255, 160, 56}, true);
  // Nose cone
  b.Centre(0.0f, 0.0f, 40.0f);
  const std::vector<std::size_t> base = {b.Vertex(-6.0f, -6.0f, 30.0f), b.Vertex(6.0f, -6.0f, 30.0f),
                                         b.Vertex(6.0f, 6.0f, 30.0f), b.Vertex(-6.0f, 6.0f, 30.0f)};
  b.Fan(b.Vertex(0.0f, 0.0f, 58.0f), base, {red});
  // Fins
  b.Box(6.0f, -1.0f, -42.0f, 20.0f, 1.0f, -12.0f, dark, dark, false);
  b.Box(-20.0f, -1.0f, -42.0f, -6.0f, 1.0f, -12.0f, dark, dark, false);
  b.Box(-1.0f, 6.0f, -42.0f, 1.0f, 20.0f, -12.0f, dark, dark, false);
  b.Box(-1.0f, -20.0f, -42.0f, 1.0f, -6.0f, -12.0f, dark, dark, false);
  return b.Finish(3.2f);
}

Model BuildAsteroid() {
  ModelBuilder b("ASTEROID");
  b.Centre(0.0f, 0.0f, 0.0f);
  // An icosahedron, split once, with each vertex pushed in or out to make a rock
  const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
  std::vector<Vec3> unit = {{-1, t, 0},  {1, t, 0},  {-1, -t, 0}, {1, -t, 0}, {0, -1, t},  {0, 1, t},
                            {0, -1, -t}, {0, 1, -t}, {t, 0, -1},  {t, 0, 1},  {-t, 0, -1}, {-t, 0, 1}};
  std::vector<std::array<std::size_t, 3>> faces = {{0, 11, 5}, {0, 5, 1},  {0, 1, 7},   {0, 7, 10}, {0, 10, 11},
                                                   {1, 5, 9},  {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
                                                   {3, 9, 4},  {3, 4, 2},  {3, 2, 6},   {3, 6, 8},  {3, 8, 9},
                                                   {4, 9, 5},  {2, 4, 11}, {6, 2, 10},  {8, 6, 7},  {9, 8, 1}};
  for (Vec3& v : unit) {
    const float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    v = {v.x / length, v.y / length, v.z / length};
  }
  std::vector<std::array<std::size_t, 3>> split;
  std::map<std::pair<std::size_t, std::size_t>, std::size_t> middles;
  auto middle = [&](std::size_t a, std::size_t c) {
    const auto key = std::make_pair(std::min(a, c), std::max(a, c));
    const auto found = middles.find(key);
    if (found != middles.end()) {
      return found->second;
    }
    Vec3 m{unit[a].x + unit[c].x, unit[a].y + unit[c].y, unit[a].z + unit[c].z};
    const float length = std::sqrt(m.x * m.x + m.y * m.y + m.z * m.z);
    unit.push_back({m.x / length, m.y / length, m.z / length});
    middles[key] = unit.size() - 1;
    return unit.size() - 1;
  };
  for (const auto& f : faces) {
    const std::size_t ab = middle(f[0], f[1]);
    const std::size_t bc = middle(f[1], f[2]);
    const std::size_t ca = middle(f[2], f[0]);
    split.push_back({f[0], ab, ca});
    split.push_back({f[1], bc, ab});
    split.push_back({f[2], ca, bc});
    split.push_back({ab, bc, ca});
  }
  std::vector<std::size_t> vertex(unit.size());
  for (std::size_t i = 0; i < unit.size(); ++i) {
    const float bump =
        0.74f + 0.46f * static_cast<float>(Hash32(static_cast<std::uint32_t>(i) * 977U + 13U) % 1000U) / 999.0f;
    vertex[i] = b.Vertex(unit[i].x * bump * 1.0f, unit[i].y * bump * 0.78f, unit[i].z * bump * 0.92f);
  }
  for (std::size_t i = 0; i < split.size(); ++i) {
    const auto shade = static_cast<std::uint8_t>(104 + Hash32(static_cast<std::uint32_t>(i) * 31U + 7U) % 40U);
    b.Tri(vertex[split[i][0]], vertex[split[i][1]], vertex[split[i][2]],
          {shade, static_cast<std::uint8_t>(shade - 14), static_cast<std::uint8_t>(shade - 30)});
  }
  return b.Finish(3.0f);
}

Model BuildCanister() {
  ModelBuilder b("CARGO CANISTER");
  const Rgb orange{226, 142, 44};
  const Rgb yellow{238, 200, 70};
  const Rgb dark{84, 84, 96};
  b.Centre(0.0f, 0.0f, 0.0f);
  const std::vector<std::size_t> end_rear = b.Ring(6, 9.0f, 9.0f, -26.0f);
  const std::vector<std::size_t> rear = b.Ring(6, 15.0f, 15.0f, -18.0f);
  const std::vector<std::size_t> front = b.Ring(6, 15.0f, 15.0f, 18.0f);
  const std::vector<std::size_t> end_front = b.Ring(6, 9.0f, 9.0f, 26.0f);
  b.Band(end_rear, rear, {dark});
  b.Band(rear, front, {orange, yellow});
  b.Band(front, end_front, {dark});
  b.Poly(end_front, {60, 62, 72});
  b.Poly(std::vector<std::size_t>(end_rear.rbegin(), end_rear.rend()), {60, 62, 72});
  return b.Finish(3.0f);
}

Model BuildEscapePod() {
  ModelBuilder b("ESCAPE POD");
  const Rgb white{226, 228, 234};
  const Rgb orange{222, 112, 52};
  b.Centre(0.0f, 0.0f, 4.0f);
  const std::size_t tip = b.Vertex(0.0f, 0.0f, 44.0f);
  const std::vector<std::size_t> shoulder = b.Ring(6, 11.0f, 11.0f, 18.0f);
  const std::vector<std::size_t> mid = b.Ring(6, 13.0f, 13.0f, -16.0f);
  const std::vector<std::size_t> rear = b.Ring(6, 9.0f, 9.0f, -26.0f);
  b.Fan(tip, shoulder, {orange, white});
  b.Band(shoulder, mid, {white, orange});
  b.Band(mid, rear, {white});
  b.Poly(std::vector<std::size_t>(rear.rbegin(), rear.rend()), {255, 160, 56}, true);
  return b.Finish(3.0f);
}

/// The 13 ships, in the order of the ship list of the original game. The two Cobra Mk III entries share a model.
std::vector<Model> BuildAllShips() {
  std::vector<Model> ships;
  ships.push_back(BuildSidewinder());
  ships.push_back(BuildViper());
  ships.push_back(BuildMamba());
  ships.push_back(BuildPython());
  ships.push_back(
      BuildCobra("COBRA MK III BOUNTY HUNTER", {88, 94, 112}, {120, 128, 150}, {200, 44, 44}, {40, 42, 52}));
  ships.push_back(BuildThargoid());
  ships.push_back(BuildCobra("COBRA MK III TRADER", {204, 194, 164}, {230, 222, 196}, {44, 150, 150}, {84, 80, 70}));
  ships.push_back(BuildCoriolis());
  ships.push_back(BuildMissile());
  ships.push_back(BuildAsteroid());
  ships.push_back(BuildCanister());
  ships.push_back(BuildThargon());
  ships.push_back(BuildEscapePod());
  return ships;
}

}  // namespace

const std::vector<Model>& AllShips() {
  static const std::vector<Model> ships = BuildAllShips();
  return ships;
}

const Model& GetShip(ShipId id) { return AllShips().at(static_cast<std::size_t>(id)); }

}  // namespace open_elite

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

/// The Python. The same model is used for the trader and the pirate, which differ only in their paint.
Model BuildPython(const char* name, Rgb hull, Rgb roof, Rgb canopy, Rgb tail, Rgb engine) {
  ModelBuilder b(name);
  AddHull(b,
          {{72.0f, 18.0f, 15.0f, 8.0f, -8.0f, -12.0f, 0.80f, 0.90f},
           {36.0f, 44.0f, 26.0f, 16.0f, -14.0f, -24.0f, 0.80f, 0.90f},
           {-72.0f, 46.0f, 26.0f, 16.0f, -14.0f, -24.0f, 0.80f, 0.90f}},
          {0.0f, -6.0f, 110.0f}, hull, roof, canopy, tail);
  for (const float x : {-26.0f, 0.0f, 26.0f}) {
    b.Box(x - 9.0f, -12.0f, -92.0f, x + 9.0f, 8.0f, -72.0f, engine, {255, 160, 56}, true);
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

/// A rectangle about the z axis, in the plane z
std::vector<std::size_t> AddRect(ModelBuilder& b, float hw, float hh, float z) {
  return {b.Vertex(-hw, -hh, z), b.Vertex(hw, -hh, z), b.Vertex(hw, hh, z), b.Vertex(-hw, hh, z)};
}

/// Fill the flat ring between two outlines in a plane of constant z, the inner one inside the outer one and both
/// around the z axis. The centre of the mesh must be behind the plane.
void AddFrame(ModelBuilder& b, const std::vector<std::size_t>& outer, const std::vector<std::size_t>& inner, Rgb colour,
              bool emissive = false) {
  const auto by_angle = [&b](const std::vector<std::size_t>& ring) {
    std::vector<std::pair<float, std::size_t>> sorted;
    for (const std::size_t i : ring) {
      const Vec3 p = b.Position(i);
      sorted.push_back({std::atan2(p.y, p.x), i});
    }
    std::sort(sorted.begin(), sorted.end());
    return sorted;
  };
  const float turn = 2.0f * kPi;
  const auto o = by_angle(outer);
  auto n = by_angle(inner);
  // Begin the inner outline at the vertex whose angle is nearest the first outer one, and keep the angles rising
  std::size_t start = 0;
  float nearest = 1e9f;
  for (std::size_t i = 0; i < n.size(); ++i) {
    float d = std::fabs(n[i].first - o[0].first);
    d = std::min(d, turn - d);
    if (d < nearest) {
      nearest = d;
      start = i;
    }
  }
  std::rotate(n.begin(), n.begin() + static_cast<std::ptrdiff_t>(start), n.end());
  for (std::size_t i = 1; i < n.size(); ++i) {
    while (n[i].first < n[i - 1].first) {
      n[i].first += turn;
    }
  }
  const std::size_t m = o.size();
  const std::size_t c = n.size();
  const auto outer_angle = [&](std::size_t k) { return o[k % m].first + turn * static_cast<float>(k / m); };
  const auto inner_angle = [&](std::size_t k) { return n[k % c].first + turn * static_cast<float>(k / c); };
  // Walk round both outlines together, making a triangle at each step along whichever is behind
  std::size_t i = 0;
  std::size_t j = 0;
  while (i < m || j < c) {
    if (j >= c || (i < m && outer_angle(i + 1) <= inner_angle(j + 1))) {
      b.Tri(o[i % m].second, n[j % c].second, o[(i + 1) % m].second, colour, emissive, 0);
      ++i;
    } else {
      b.Tri(o[i % m].second, n[j % c].second, n[(j + 1) % c].second, colour, emissive, 0);
      ++j;
    }
  }
}

/// The inside of a docking bay: a tunnel of panels with strips of light, running back from a rectangular mouth in the
/// plane z = z_mouth to a back wall. Returns the corners of the mouth. The mesh centre is left somewhere inside the
/// tunnel, so set it again afterwards.
std::vector<std::size_t> AddBay(ModelBuilder& b, float z_mouth, float hw, float hh, float depth, int segments) {
  const float lift = 0.03f * hw;
  const float step = depth / static_cast<float>(segments);
  const float z_back = z_mouth - depth;
  std::vector<std::vector<std::size_t>> ring;
  for (int k = 0; k <= segments; ++k) {
    ring.push_back(AddRect(b, hw, hh, z_mouth - step * static_cast<float>(k)));
  }
  const auto shaded = [](Rgb c, float f) {
    return Rgb{static_cast<std::uint8_t>(c.r * f), static_cast<std::uint8_t>(c.g * f), static_cast<std::uint8_t>(c.b * f)};
  };
  // A panel of the wall, facing the axis of the tunnel: the mesh centre is out beyond the wall
  const auto panel = [&b](Vec3 centre, std::size_t p, std::size_t q, std::size_t r, std::size_t s, Rgb colour) {
    b.Centre(centre.x, centre.y, centre.z);
    b.Quad(p, q, r, s, colour);
  };
  // A light, drawn as a flat lit shape just in front of the wall
  const auto light = [&b](Vec3 centre, const std::array<Vec3, 4>& p, Rgb colour) {
    b.Centre(centre.x, centre.y, centre.z);
    std::array<std::size_t, 4> v{};
    for (std::size_t i = 0; i < 4; ++i) {
      v[i] = b.Vertex(p[i].x, p[i].y, p[i].z);
    }
    b.Tri(v[0], v[1], v[2], colour, true, 0);
    b.Tri(v[0], v[2], v[3], colour, true, 0);
  };
  const Rgb amber{255, 196, 84};
  const Rgb white{225, 238, 255};
  const Rgb guide{236, 206, 76};
  for (int k = 0; k < segments; ++k) {
    const auto s = static_cast<std::size_t>(k);
    const float z0 = z_mouth - step * static_cast<float>(k);
    const float z1 = z0 - step;
    const float zm = (z0 + z1) / 2.0f;
    // Brighter towards the mouth, where daylight spills in; every other panel a little darker
    const float deep = segments > 1 ? static_cast<float>(k) / static_cast<float>(segments - 1) : 0.0f;
    const float tone = (1.0f - 0.45f * deep) * (k % 2 == 0 ? 1.0f : 0.82f);
    panel({0.0f, -3.0f * hh, zm}, ring[s][0], ring[s][1], ring[s + 1][1], ring[s + 1][0], shaded({70, 76, 90}, tone));
    panel({0.0f, 3.0f * hh, zm}, ring[s][3], ring[s][2], ring[s + 1][2], ring[s + 1][3], shaded({84, 92, 108}, tone));
    panel({-3.0f * hw, 0.0f, zm}, ring[s][0], ring[s][3], ring[s + 1][3], ring[s + 1][0], shaded({92, 100, 116}, tone));
    panel({3.0f * hw, 0.0f, zm}, ring[s][1], ring[s][2], ring[s + 1][2], ring[s + 1][1], shaded({92, 100, 116}, tone));
    const float a = z0 - 0.06f * step;
    const float e = z1 + 0.06f * step;
    if (k % 3 != 2) {
      for (const float side : {-1.0f, 1.0f}) {
        const float x = side * (hw - lift);
        light({side * 3.0f * hw, 0.0f, zm}, {{{x, 0.35f * hh, a}, {x, 0.5f * hh, a}, {x, 0.5f * hh, e}, {x, 0.35f * hh, e}}},
              amber);
      }
    }
    if (k % 2 == 0) {
      const float a2 = z0 - 0.12f * step;
      const float e2 = z1 + 0.12f * step;
      const float y = hh - lift;
      light({0.0f, 3.0f * hh, zm}, {{{-0.3f * hw, y, a2}, {0.3f * hw, y, a2}, {0.3f * hw, y, e2}, {-0.3f * hw, y, e2}}}, white);
      const float f = -hh + lift;
      const float a3 = z0 - 0.18f * step;
      const float e3 = z1 + 0.18f * step;
      light({0.0f, -3.0f * hh, zm}, {{{-0.04f * hw, f, a3}, {0.04f * hw, f, a3}, {0.04f * hw, f, e3}, {-0.04f * hw, f, e3}}},
            guide);
    }
  }
  // The back wall, with two lit panels
  const std::vector<std::size_t>& end = ring.back();
  panel({0.0f, 0.0f, z_back - depth}, end[0], end[1], end[2], end[3], {38, 42, 52});
  const float zb = z_back + lift;
  for (const float side : {-1.0f, 1.0f}) {
    const float x0 = side * 0.2f * hw;
    const float x1 = side * 0.8f * hw;
    light({0.0f, 0.0f, z_back - depth}, {{{x0, -0.5f * hh, zb}, {x1, -0.5f * hh, zb}, {x1, 0.5f * hh, zb}, {x0, 0.5f * hh, zb}}},
          {160, 60, 48});
  }
  return ring.front();
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
  // Squares on the faces of the cube, the one at the front (+z) being left for the bay
  std::vector<std::size_t> front;
  for (int s = -1; s <= 1; s += 2) {
    b.Quad(index[1 + s][2][1], index[1 + s][1][2], index[1 + s][0][1], index[1 + s][1][0], square);
    b.Quad(index[2][1 + s][1], index[1][1 + s][2], index[0][1 + s][1], index[1][1 + s][0], square);
    if (s == 1) {
      front = {index[2][1][2], index[1][2][2], index[0][1][2], index[1][0][2]};
    } else {
      b.Quad(index[2][1][1 + s], index[1][2][1 + s], index[0][1][1 + s], index[1][0][1 + s], square);
    }
  }
  // The docking bay in the face at the front: a lit rim round the mouth of a hollow tunnel
  const std::vector<std::size_t> mouth = AddBay(b, r, 21.0f, 8.0f, 60.0f, 12);
  const std::vector<std::size_t> rim = AddRect(b, 27.0f, 13.0f, r);
  b.Centre(0.0f, 0.0f, 0.0f);
  AddFrame(b, front, rim, square);
  AddFrame(b, rim, mouth, {236, 206, 76}, true);
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

/// A rock: an icosahedron, split once or not, with each vertex pushed in or out
struct RockOptions {
  const char* name;
  bool split;
  Vec3 stretch;
  std::uint32_t seed;
  float bump_min;
  float bump_range;
  std::uint8_t shade;
  /// A lit docking bay in the front, for the rock hermit
  bool bay = false;
};

Model BuildRock(const RockOptions& o) {
  ModelBuilder b(o.name);
  b.Centre(0.0f, 0.0f, 0.0f);
  // An icosahedron, split once or not, with each vertex pushed in or out to make a rock
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
  if (o.split) {
    for (const auto& f : faces) {
      const std::size_t ab = middle(f[0], f[1]);
      const std::size_t bc = middle(f[1], f[2]);
      const std::size_t ca = middle(f[2], f[0]);
      split.push_back({f[0], ab, ca});
      split.push_back({f[1], bc, ab});
      split.push_back({f[2], ca, bc});
      split.push_back({ab, bc, ca});
    }
  } else {
    split = faces;
  }
  std::vector<std::size_t> vertex(unit.size());
  for (std::size_t i = 0; i < unit.size(); ++i) {
    const float bump = o.bump_min + o.bump_range *
                                        static_cast<float>(Hash32(static_cast<std::uint32_t>(i) * 977U + 13U + o.seed) % 1000U) /
                                        999.0f;
    vertex[i] = b.Vertex(unit[i].x * bump * o.stretch.x, unit[i].y * bump * o.stretch.y, unit[i].z * bump * o.stretch.z);
  }
  for (std::size_t i = 0; i < split.size(); ++i) {
    const auto shade = static_cast<std::uint8_t>(o.shade + Hash32(static_cast<std::uint32_t>(i) * 31U + 7U) % 40U);
    b.Tri(vertex[split[i][0]], vertex[split[i][1]], vertex[split[i][2]],
          {shade, static_cast<std::uint8_t>(shade - 14), static_cast<std::uint8_t>(shade - 30)});
  }
  if (o.bay) {
    // A hangar built onto the front of the rock, its end hollowed out into a bay
    const Rgb casing{104, 100, 96};
    const float z_front = o.stretch.z * 1.24f;
    const float z_root = o.stretch.z * 0.70f;
    const std::vector<std::size_t> mouth = AddBay(b, z_front, 0.25f, 0.10f, 0.20f, 6);
    const std::vector<std::size_t> rim = AddRect(b, 0.32f, 0.16f, z_front);
    const std::vector<std::size_t> casing_front = AddRect(b, 0.42f, 0.26f, z_front);
    const std::vector<std::size_t> casing_root = AddRect(b, 0.42f, 0.26f, z_root);
    b.Centre(0.0f, 0.0f, (z_front + z_root) / 2.0f);
    for (std::size_t i = 0; i < 4; ++i) {
      const std::size_t j = (i + 1) % 4;
      b.Quad(casing_root[i], casing_root[j], casing_front[j], casing_front[i], casing);
    }
    b.Centre(0.0f, 0.0f, 0.0f);
    AddFrame(b, casing_front, rim, casing);
    AddFrame(b, rim, mouth, {236, 206, 76}, true);
  }
  return b.Finish(3.0f);
}

Model BuildAsteroid() { return BuildRock({"ASTEROID", true, {1.0f, 0.78f, 0.92f}, 0, 0.74f, 0.46f, 104}); }

Model BuildBoulder() { return BuildRock({"BOULDER", false, {1.0f, 0.85f, 0.95f}, 41, 0.70f, 0.50f, 120}); }

Model BuildSplinter() { return BuildRock({"SPLINTER", false, {0.45f, 0.50f, 1.70f}, 7, 0.75f, 0.40f, 96}); }

Model BuildRockHermit() {
  return BuildRock({"ROCK HERMIT", true, {1.0f, 0.90f, 1.0f}, 3, 0.88f, 0.14f, 92, true});
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

constexpr float kSqrt2 = 1.41421356f;

/// A rectangular ring of vertices: w and h are the half width and half height
std::vector<std::size_t> RectRing(ModelBuilder& b, float w, float h, float z, float cy = 0.0f) {
  return b.Ring(4, w * kSqrt2, h * kSqrt2, z, kPi / 4.0f, 0.0f, cy);
}

std::vector<std::size_t> Reversed(const std::vector<std::size_t>& v) {
  return std::vector<std::size_t>(v.rbegin(), v.rend());
}

/// A flat slab with any convex outline in the x-z plane, from y0 up to y1. Wings and plates are made of these.
void AddSlab(ModelBuilder& b, const std::vector<std::pair<float, float>>& outline, float y0, float y1, Rgb top,
             Rgb side, Rgb bottom) {
  float cx = 0.0f;
  float cz = 0.0f;
  for (const auto& p : outline) {
    cx += p.first / static_cast<float>(outline.size());
    cz += p.second / static_cast<float>(outline.size());
  }
  b.Centre(cx, (y0 + y1) / 2.0f, cz);
  std::vector<std::size_t> upper;
  std::vector<std::size_t> lower;
  for (const auto& p : outline) {
    upper.push_back(b.Vertex(p.first, y1, p.second));
    lower.push_back(b.Vertex(p.first, y0, p.second));
  }
  b.Poly(upper, top);
  b.Poly(Reversed(lower), bottom);
  for (std::size_t i = 0; i < outline.size(); ++i) {
    const std::size_t j = (i + 1) % outline.size();
    b.Quad(upper[i], upper[j], lower[j], lower[i], side);
  }
}

/// A slab on each side of the ship: the outline is given for the right side and mirrored for the left
void AddSlabPair(ModelBuilder& b, std::vector<std::pair<float, float>> outline, float y0, float y1, Rgb top, Rgb side,
                 Rgb bottom) {
  AddSlab(b, outline, y0, y1, top, side, bottom);
  for (auto& p : outline) {
    p.first = -p.first;
  }
  AddSlab(b, outline, y0, y1, top, side, bottom);
}

/// An engine block with a glowing rear face, whose rear face is at z and which runs forward by `length`
void AddEngine(ModelBuilder& b, float x, float y, float z, float w, float h, float length, Rgb body) {
  b.Box(x - w, y - h, z - length, x + w, y + h, z, body, {255, 160, 56}, true);
}

Model BuildAlloyPlate() {
  ModelBuilder b("ALLOY PLATE");
  const Rgb face{168, 176, 190};
  const Rgb edge{112, 120, 136};
  b.Centre(0.0f, 0.0f, 0.0f);
  const std::vector<std::size_t> back = b.Ring(6, 40.0f, 34.0f, -4.0f, kPi / 6.0f);
  const std::vector<std::size_t> front = b.Ring(6, 40.0f, 34.0f, 4.0f, kPi / 6.0f);
  b.Band(back, front, {edge, face});
  b.Poly(front, face);
  b.Poly(Reversed(back), {138, 146, 162});
  return b.Finish(3.0f);
}

Model BuildShuttle() {
  ModelBuilder b("SHUTTLE");
  const Rgb white{222, 224, 230};
  const Rgb blue{60, 100, 170};
  const Rgb dark{54, 58, 70};
  b.Centre(0.0f, 0.0f, 0.0f);
  const std::vector<std::size_t> rear = RectRing(b, 15.0f, 11.0f, -44.0f);
  const std::vector<std::size_t> middle = RectRing(b, 15.0f, 11.0f, 22.0f);
  const std::vector<std::size_t> front = RectRing(b, 9.0f, 6.0f, 46.0f, -2.0f);
  b.Band(rear, middle, {white, blue, white, blue});
  b.Band(middle, front, {white, {44, 60, 90}, white, blue});
  b.Poly(front, {44, 60, 90});
  b.Poly(Reversed(rear), dark);
  AddSlabPair(b, {{14.0f, 14.0f}, {40.0f, -8.0f}, {40.0f, -28.0f}, {14.0f, -28.0f}}, -2.0f, 2.0f, white, blue, white);
  AddEngine(b, -8.0f, 0.0f, -44.0f, 5.0f, 5.0f, 6.0f, dark);
  AddEngine(b, 8.0f, 0.0f, -44.0f, 5.0f, 5.0f, 6.0f, dark);
  return b.Finish(3.2f);
}

Model BuildTransporter() {
  ModelBuilder b("TRANSPORTER");
  const Rgb hull{150, 156, 168};
  const Rgb stripe{214, 160, 56};
  const Rgb dark{54, 58, 70};
  b.Centre(0.0f, 0.0f, 0.0f);
  const std::vector<std::size_t> rear = RectRing(b, 20.0f, 14.0f, -90.0f);
  const std::vector<std::size_t> middle = RectRing(b, 20.0f, 14.0f, 40.0f);
  const std::vector<std::size_t> cab = RectRing(b, 13.0f, 9.0f, 78.0f, -2.0f);
  b.Band(rear, middle, {hull, stripe, hull, stripe});
  b.Band(middle, cab, {hull, {48, 70, 110}, hull, dark});
  b.Poly(cab, {48, 70, 110});
  b.Poly(Reversed(rear), dark);
  for (const float z : {-76.0f, -34.0f}) {
    b.Box(-14.0f, 14.0f, z, 14.0f, 24.0f, z + 34.0f, {176, 110, 60}, {128, 78, 44}, false);
  }
  AddEngine(b, -10.0f, 0.0f, -90.0f, 7.0f, 7.0f, 6.0f, dark);
  AddEngine(b, 10.0f, 0.0f, -90.0f, 7.0f, 7.0f, 6.0f, dark);
  return b.Finish(3.2f);
}

Model BuildBoa() {
  ModelBuilder b("BOA");
  AddHull(b,
          {{55.0f, 24.0f, 16.0f, 10.0f, -8.0f, -14.0f, 0.80f, 0.90f},
           {15.0f, 48.0f, 22.0f, 12.0f, -12.0f, -20.0f, 0.85f, 0.90f},
           {-60.0f, 50.0f, 22.0f, 12.0f, -12.0f, -20.0f, 0.85f, 0.90f}},
          {0.0f, -2.0f, 95.0f}, {96, 108, 72}, {128, 142, 98}, {40, 60, 80}, {50, 52, 44});
  for (const float x : {-28.0f, 0.0f, 28.0f}) {
    AddEngine(b, x, -2.0f, -60.0f, 10.0f, 9.0f, 16.0f, {56, 60, 52});
  }
  return b.Finish(3.2f);
}

Model BuildAnaconda() {
  ModelBuilder b("ANACONDA");
  AddHull(b,
          {{90.0f, 12.0f, 8.0f, 5.0f, -4.0f, -8.0f, 0.70f, 0.80f},
           {40.0f, 26.0f, 14.0f, 9.0f, -8.0f, -12.0f, 0.80f, 0.85f},
           {-30.0f, 34.0f, 16.0f, 10.0f, -9.0f, -14.0f, 0.80f, 0.90f},
           {-100.0f, 28.0f, 14.0f, 8.0f, -8.0f, -12.0f, 0.80f, 0.90f}},
          {0.0f, 0.0f, 150.0f}, {70, 100, 110}, {100, 138, 150}, {30, 46, 66}, {44, 52, 56});
  b.Box(-8.0f, 14.0f, -30.0f, 8.0f, 22.0f, 6.0f, {58, 80, 90}, {58, 80, 90}, false);
  for (const float x : {-14.0f, 14.0f}) {
    AddEngine(b, x, -1.0f, -100.0f, 9.0f, 8.0f, 14.0f, {48, 56, 60});
  }
  return b.Finish(3.2f);
}

Model BuildCobraMk1() {
  ModelBuilder b("COBRA MK I");
  AddHull(b,
          {{40.0f, 10.0f, 8.0f, 3.0f, -3.0f, -7.0f, 0.70f, 0.80f},
           {0.0f, 28.0f, 12.0f, 5.0f, -5.0f, -10.0f, 0.75f, 0.85f},
           {-36.0f, 30.0f, 12.0f, 5.0f, -5.0f, -10.0f, 0.75f, 0.85f}},
          {0.0f, 0.0f, 70.0f}, {196, 120, 60}, {228, 160, 96}, {40, 52, 72}, {70, 50, 40});
  for (const float x : {-16.0f, 16.0f}) {
    AddEngine(b, x, -2.0f, -36.0f, 7.0f, 6.0f, 12.0f, {80, 60, 50});
  }
  return b.Finish(3.2f);
}

Model BuildAdder() {
  ModelBuilder b("ADDER");
  AddHull(b,
          {{52.0f, 14.0f, 10.0f, 8.0f, -6.0f, -10.0f, 0.90f, 0.95f},
           {10.0f, 30.0f, 14.0f, 12.0f, -8.0f, -12.0f, 0.95f, 0.95f},
           {-48.0f, 32.0f, 14.0f, 12.0f, -8.0f, -12.0f, 0.95f, 0.95f}},
          {0.0f, -2.0f, 75.0f}, {170, 172, 180}, {208, 210, 218}, {40, 60, 100}, {60, 60, 70});
  for (const float x : {-18.0f, 18.0f}) {
    AddEngine(b, x, -2.0f, -48.0f, 9.0f, 8.0f, 14.0f, {70, 72, 84});
  }
  return b.Finish(3.2f);
}

Model BuildKrait() {
  ModelBuilder b("KRAIT");
  AddHull(b,
          {{70.0f, 8.0f, 6.0f, 3.0f, -3.0f, -5.0f, 0.60f, 0.70f},
           {10.0f, 14.0f, 8.0f, 4.0f, -4.0f, -6.0f, 0.60f, 0.70f},
           {-50.0f, 14.0f, 8.0f, 4.0f, -4.0f, -6.0f, 0.60f, 0.70f}},
          {0.0f, 0.0f, 110.0f}, {120, 120, 150}, {160, 160, 190}, {36, 40, 64}, {50, 50, 60});
  AddSlabPair(b, {{13.0f, 20.0f}, {62.0f, -40.0f}, {62.0f, -52.0f}, {13.0f, -50.0f}}, -2.0f, 2.0f, {196, 72, 62},
              {120, 50, 46}, {150, 56, 50});
  AddEngine(b, 0.0f, 0.0f, -50.0f, 7.0f, 6.0f, 12.0f, {60, 60, 72});
  return b.Finish(3.2f);
}

Model BuildGecko() {
  ModelBuilder b("GECKO");
  AddSlab(b, {{0.0f, 60.0f}, {48.0f, -22.0f}, {34.0f, -40.0f}, {-34.0f, -40.0f}, {-48.0f, -22.0f}}, -6.0f, 6.0f,
          {96, 150, 96}, {64, 100, 70}, {70, 110, 76});
  AddSlab(b, {{0.0f, 36.0f}, {11.0f, 4.0f}, {-11.0f, 4.0f}}, 6.0f, 13.0f, {36, 50, 80}, {36, 50, 80}, {36, 50, 80});
  for (const float x : {-16.0f, 16.0f}) {
    AddEngine(b, x, 0.0f, -40.0f, 6.0f, 5.0f, 10.0f, {60, 80, 64});
  }
  return b.Finish(3.2f);
}

Model BuildWorm() {
  ModelBuilder b("WORM");
  const Rgb body{128, 150, 96};
  const Rgb dark{86, 106, 64};
  b.Centre(0.0f, 0.0f, 0.0f);
  const std::size_t tip = b.Vertex(0.0f, 0.0f, 54.0f);
  const std::vector<std::size_t> head = b.Ring(6, 18.0f, 14.0f, 26.0f);
  const std::vector<std::size_t> shoulder = b.Ring(6, 27.0f, 21.0f, -4.0f);
  const std::vector<std::size_t> tail = b.Ring(6, 20.0f, 16.0f, -36.0f);
  b.Fan(tip, head, {body, dark});
  b.Band(head, shoulder, {dark, body});
  b.Band(shoulder, tail, {body, dark});
  b.Poly(Reversed(tail), {255, 170, 70}, true);
  for (const float x : {-36.0f, 36.0f}) {
    b.Box(x - 9.0f, -7.0f, -24.0f, x + 9.0f, 7.0f, 6.0f, dark, {255, 170, 70}, true);
  }
  return b.Finish(3.2f);
}

Model BuildAspMk2() {
  ModelBuilder b("ASP MK II");
  AddHull(b,
          {{80.0f, 8.0f, 7.0f, 3.0f, -3.0f, -6.0f, 0.60f, 0.80f},
           {20.0f, 16.0f, 10.0f, 5.0f, -5.0f, -8.0f, 0.70f, 0.85f},
           {-50.0f, 18.0f, 10.0f, 5.0f, -5.0f, -8.0f, 0.70f, 0.85f}},
          {0.0f, 0.0f, 125.0f}, {150, 170, 200}, {198, 212, 232}, {30, 50, 90}, {50, 56, 70});
  AddSlabPair(b, {{16.0f, 10.0f}, {70.0f, -30.0f}, {70.0f, -48.0f}, {16.0f, -44.0f}}, -1.5f, 1.5f, {120, 140, 176},
              {84, 100, 130}, {96, 112, 144});
  for (const float x : {-10.0f, 10.0f}) {
    AddEngine(b, x, 0.0f, -50.0f, 6.0f, 6.0f, 10.0f, {56, 62, 78});
  }
  return b.Finish(3.2f);
}

Model BuildFerDeLance() {
  ModelBuilder b("FER-DE-LANCE");
  AddHull(b,
          {{90.0f, 6.0f, 5.0f, 2.0f, -2.0f, -4.0f, 0.60f, 0.70f},
           {30.0f, 16.0f, 8.0f, 4.0f, -4.0f, -7.0f, 0.70f, 0.80f},
           {-30.0f, 18.0f, 9.0f, 4.0f, -4.0f, -7.0f, 0.70f, 0.80f},
           {-60.0f, 14.0f, 8.0f, 4.0f, -4.0f, -7.0f, 0.70f, 0.80f}},
          {0.0f, 0.0f, 150.0f}, {170, 50, 44}, {214, 170, 70}, {30, 40, 64}, {50, 40, 40});
  AddSlabPair(b, {{16.0f, -16.0f}, {62.0f, -70.0f}, {62.0f, -84.0f}, {16.0f, -60.0f}}, -1.5f, 1.5f, {214, 170, 70},
              {150, 118, 50}, {170, 130, 54});
  for (const float x : {-8.0f, 8.0f}) {
    AddEngine(b, x, 0.0f, -60.0f, 5.0f, 5.0f, 10.0f, {70, 44, 44});
  }
  return b.Finish(3.2f);
}

Model BuildMoray() {
  ModelBuilder b("MORAY");
  const Rgb hull{64, 128, 150};
  const Rgb light{96, 170, 190};
  const Rgb dark{30, 60, 76};
  b.Centre(0.0f, 0.0f, 0.0f);
  const std::size_t tip = b.Vertex(0.0f, 0.0f, 72.0f);
  const std::vector<std::size_t> nose = b.Ring(8, 38.0f, 10.0f, 34.0f);
  const std::vector<std::size_t> wide = b.Ring(8, 72.0f, 14.0f, -8.0f);
  const std::vector<std::size_t> rear = b.Ring(8, 34.0f, 9.0f, -40.0f);
  b.Fan(tip, nose, {light, hull});
  b.Band(nose, wide, {hull, light});
  b.Band(wide, rear, {light, hull});
  b.Poly(Reversed(rear), dark);
  for (const float x : {-18.0f, 18.0f}) {
    AddEngine(b, x, 0.0f, -40.0f, 8.0f, 6.0f, 8.0f, dark);
  }
  return b.Finish(3.2f);
}

Model BuildConstrictor() {
  ModelBuilder b("CONSTRICTOR");
  AddHull(b,
          {{70.0f, 22.0f, 14.0f, 10.0f, -8.0f, -12.0f, 0.80f, 0.85f},
           {10.0f, 40.0f, 18.0f, 12.0f, -10.0f, -16.0f, 0.85f, 0.90f},
           {-64.0f, 42.0f, 18.0f, 12.0f, -10.0f, -16.0f, 0.85f, 0.90f}},
          {0.0f, -3.0f, 100.0f}, {52, 56, 64}, {78, 84, 96}, {150, 40, 40}, {30, 32, 38});
  AddSlabPair(b, {{40.0f, 6.0f}, {62.0f, -22.0f}, {62.0f, -40.0f}, {40.0f, -34.0f}}, -3.0f, 3.0f, {150, 40, 40},
              {100, 30, 30}, {110, 34, 34});
  for (const float x : {-28.0f, 0.0f, 28.0f}) {
    AddEngine(b, x, -2.0f, -64.0f, 10.0f, 10.0f, 14.0f, {40, 42, 50});
  }
  return b.Finish(3.2f);
}

Model BuildCougar() {
  ModelBuilder b("COUGAR");
  const Rgb matte{46, 50, 60};
  const Rgb edge{32, 34, 42};
  AddSlab(b, {{0.0f, 80.0f}, {55.0f, -20.0f}, {30.0f, -50.0f}, {-30.0f, -50.0f}, {-55.0f, -20.0f}}, -5.0f, 5.0f,
          matte, edge, {36, 40, 48});
  AddSlab(b, {{0.0f, 40.0f}, {16.0f, -10.0f}, {-16.0f, -10.0f}}, 5.0f, 12.0f, {70, 78, 96}, edge, edge);
  for (const float x : {-14.0f, 14.0f}) {
    AddEngine(b, x, 0.0f, -50.0f, 7.0f, 4.0f, 8.0f, edge);
  }
  return b.Finish(3.2f);
}

/// A dodecahedron station: twelve pentagons, with the docking slot in the one that faces forward
Model BuildDodo() {
  ModelBuilder b("DODO STATION", Spin::kRoll);
  const Rgb light{196, 176, 150};
  const Rgb dark{150, 132, 112};
  const float phi = (1.0f + std::sqrt(5.0f)) / 2.0f;
  const float inverse = 1.0f / phi;
  const float r = 40.0f;

  std::vector<Vec3> corner;
  for (const float x : {-1.0f, 1.0f}) {
    for (const float y : {-1.0f, 1.0f}) {
      for (const float z : {-1.0f, 1.0f}) {
        corner.push_back({x, y, z});
      }
    }
  }
  for (const float s : {-1.0f, 1.0f}) {
    for (const float t : {-1.0f, 1.0f}) {
      corner.push_back({0.0f, s * inverse, t * phi});
      corner.push_back({s * inverse, t * phi, 0.0f});
      corner.push_back({s * phi, 0.0f, t * inverse});
    }
  }

  // The face directions are (0, +-phi, +-1) and their rotations among the axes. Turn the ship so that (0, phi, 1)
  // looks along +z.
  const float angle = std::atan(phi);
  const float c = std::cos(angle);
  const float s = std::sin(angle);
  b.Centre(0.0f, 0.0f, 0.0f);
  std::vector<std::size_t> vertex;
  std::vector<Vec3> placed;
  for (const Vec3& v : corner) {
    const Vec3 p{v.x * r, (v.y * c - v.z * s) * r, (v.y * s + v.z * c) * r};
    placed.push_back(p);
    vertex.push_back(b.Vertex(p.x, p.y, p.z));
  }

  std::vector<Vec3> normals;
  for (const float a : {-1.0f, 1.0f}) {
    for (const float d : {-1.0f, 1.0f}) {
      normals.push_back({0.0f, a * phi, d});
      normals.push_back({a, 0.0f, d * phi});
      normals.push_back({a * phi, d, 0.0f});
    }
  }
  float front_z = 0.0f;
  std::vector<std::size_t> front_face;
  Rgb front_colour = light;
  std::size_t face_number = 0;
  for (const Vec3& n : normals) {
    // The same turn, applied to the direction
    const Vec3 turned{n.x, n.y * c - n.z * s, n.y * s + n.z * c};
    std::vector<std::pair<float, std::size_t>> around;
    float best = -1e9f;
    for (std::size_t i = 0; i < corner.size(); ++i) {
      best = std::max(best, corner[i].x * n.x + corner[i].y * n.y + corner[i].z * n.z);
    }
    // Two axes at right angles to the face direction, to put its corners in order round the face
    const Vec3 helper = std::fabs(turned.z) < 0.9f * std::sqrt(turned.x * turned.x + turned.y * turned.y + turned.z * turned.z)
                            ? Vec3{0.0f, 0.0f, 1.0f}
                            : Vec3{1.0f, 0.0f, 0.0f};
    Vec3 u{turned.y * helper.z - turned.z * helper.y, turned.z * helper.x - turned.x * helper.z,
           turned.x * helper.y - turned.y * helper.x};
    const float u_length = std::sqrt(u.x * u.x + u.y * u.y + u.z * u.z);
    u = {u.x / u_length, u.y / u_length, u.z / u_length};
    const Vec3 w{turned.y * u.z - turned.z * u.y, turned.z * u.x - turned.x * u.z, turned.x * u.y - turned.y * u.x};
    for (std::size_t i = 0; i < corner.size(); ++i) {
      if (corner[i].x * n.x + corner[i].y * n.y + corner[i].z * n.z > best - 1e-3f) {
        const Vec3& p = placed[i];
        around.push_back({std::atan2(p.x * w.x + p.y * w.y + p.z * w.z, p.x * u.x + p.y * u.y + p.z * u.z), i});
      }
    }
    std::sort(around.begin(), around.end());
    std::vector<std::size_t> face;
    for (const auto& a : around) {
      face.push_back(vertex[a.second]);
      if (n.x == 0.0f && n.y > 0.0f && n.z > 0.0f) {
        front_z = std::max(front_z, placed[a.second].z);
      }
    }
    const Rgb colour = face_number++ % 2 == 0 ? light : dark;
    if (n.x == 0.0f && n.y > 0.0f && n.z > 0.0f) {
      // The front face is left for the bay
      front_face = face;
      front_colour = colour;
    } else {
      b.Poly(face, colour);
    }
  }

  // The docking bay in the face at the front (+z): a lit rim round the mouth of a hollow tunnel
  const std::vector<std::size_t> mouth = AddBay(b, front_z, 18.0f, 7.0f, 50.0f, 10);
  const std::vector<std::size_t> rim = AddRect(b, 24.0f, 11.0f, front_z);
  b.Centre(0.0f, 0.0f, 0.0f);
  AddFrame(b, front_face, rim, front_colour);
  AddFrame(b, rim, mouth, {236, 206, 76}, true);
  return b.Finish(3.2f);
}

/// A wheel station: a big rotating rim joined to a hub by spokes, the hub carrying the docking bay at the front. It is
/// the classic rotating wheel space station (the von Braun wheel) and is drawn here as an original model.
Model BuildWheelStation() {
  ModelBuilder b("WHEEL STATION", Spin::kRoll);
  const Rgb hull{200, 204, 212};
  const Rgb shade{148, 154, 170};
  const Rgb dark{54, 58, 72};
  const Rgb lit{240, 220, 130};
  const float rim_radius = 700.0f;
  const float rim_w = 60.0f;
  const float rim_h = 100.0f;
  const float hub_radius = 112.0f;
  const int segments = 36;
  const int spokes = 6;
  const float turn = 2.0f * kPi;

  // The rim: a ring of 36 boxes, each its own convex piece. Each cross section is the outer front corner, inner front
  // corner, inner back corner and outer back corner.
  const float radii[4] = {rim_radius + rim_w, rim_radius - rim_w, rim_radius - rim_w, rim_radius + rim_w};
  const float depths[4] = {rim_h, rim_h, -rim_h, -rim_h};
  std::vector<std::array<std::size_t, 4>> section;
  for (int i = 0; i < segments; ++i) {
    const float angle = turn * static_cast<float>(i) / static_cast<float>(segments);
    std::array<std::size_t, 4> s{};
    for (std::size_t k = 0; k < 4; ++k) {
      s[k] = b.Vertex(radii[k] * std::cos(angle), radii[k] * std::sin(angle), depths[k]);
    }
    section.push_back(s);
  }
  for (int i = 0; i < segments; ++i) {
    const std::size_t j = static_cast<std::size_t>((i + 1) % segments);
    const std::size_t a = static_cast<std::size_t>(i);
    const float middle = turn * (static_cast<float>(i) + 0.5f) / static_cast<float>(segments);
    b.Centre(rim_radius * std::cos(middle), rim_radius * std::sin(middle), 0.0f);
    const bool even = i % 2 == 0;
    // Front: plain plating alternating with lit windows
    b.Quad(section[a][0], section[a][1], section[j][1], section[j][0], even ? hull : lit, !even);
    // Inner face, towards the hub
    b.Quad(section[a][1], section[a][2], section[j][2], section[j][1], even ? shade : hull);
    // Back
    b.Quad(section[a][2], section[a][3], section[j][3], section[j][2], shade);
    // Outer face
    b.Quad(section[a][3], section[a][0], section[j][0], section[j][3], dark);
  }

  // The spokes: square beams from the hub to the rim, their ends buried in both
  const float spoke_w = 14.0f;
  const float spoke_h = 22.0f;
  const float r0 = hub_radius - 4.0f;
  const float r1 = rim_radius - rim_w + 4.0f;
  for (int n = 0; n < spokes; ++n) {
    const float angle = turn * static_cast<float>(n) / static_cast<float>(spokes);
    const float dx = std::cos(angle);
    const float dy = std::sin(angle);
    const float side_x = -dy;
    const float side_y = dx;
    const float across[4] = {spoke_w, spoke_w, -spoke_w, -spoke_w};
    const float depth[4] = {spoke_h, -spoke_h, -spoke_h, spoke_h};
    b.Centre(dx * (r0 + r1) / 2.0f, dy * (r0 + r1) / 2.0f, 0.0f);
    std::array<std::size_t, 4> inner{};
    std::array<std::size_t, 4> outer{};
    for (std::size_t k = 0; k < 4; ++k) {
      inner[k] = b.Vertex(dx * r0 + side_x * across[k], dy * r0 + side_y * across[k], depth[k]);
      outer[k] = b.Vertex(dx * r1 + side_x * across[k], dy * r1 + side_y * across[k], depth[k]);
    }
    const Rgb colours[4] = {shade, shade, shade, hull};
    for (std::size_t k = 0; k < 4; ++k) {
      const std::size_t kk = (k + 1) % 4;
      b.Quad(inner[k], inner[kk], outer[kk], outer[k], colours[k]);
    }
  }

  // The hub, with a drum on the front that holds the docking bay, and a spindle at the back
  b.Centre(0.0f, 0.0f, 0.0f);
  const int sides = 16;
  const std::vector<std::size_t> hub_rear = b.Ring(sides, hub_radius, hub_radius, -55.0f);
  const std::vector<std::size_t> hub_front = b.Ring(sides, hub_radius, hub_radius, 55.0f);
  const std::vector<std::size_t> collar_a = b.Ring(sides, 100.0f, 100.0f, 62.0f);
  const std::vector<std::size_t> collar_b = b.Ring(sides, 100.0f, 100.0f, 420.0f);
  const std::vector<std::size_t> spindle_a = b.Ring(sides, 30.0f, 30.0f, -62.0f);
  const std::vector<std::size_t> spindle_b = b.Ring(sides, 30.0f, 30.0f, -110.0f);
  b.Band(hub_rear, hub_front, {shade, hull});
  b.Band(hub_front, collar_a, {hull, shade});
  b.Band(collar_a, collar_b, {shade, hull});
  b.Band(hub_rear, spindle_a, {shade, hull});
  b.Band(spindle_a, spindle_b, {hull, shade});
  b.Poly(Reversed(spindle_b), dark);
  // The bay: a lit rim round the mouth of a tunnel that runs back into the hub
  const std::vector<std::size_t> mouth = AddBay(b, 420.0f, 80.0f, 30.0f, 465.0f, 20);
  const std::vector<std::size_t> rim = AddRect(b, 86.0f, 36.0f, 420.0f);
  b.Centre(0.0f, 0.0f, 0.0f);
  AddFrame(b, collar_b, rim, hull);
  AddFrame(b, rim, mouth, lit, true);
  return b.Finish(3.2f);
}

using Outline = std::vector<std::pair<float, float>>;

/// A flat piece with a convex outline in the x-y plane, from z0 (the back) to z1 (the front)
void AddPlate(ModelBuilder& b, const Outline& outline, float z0, float z1, Rgb front, Rgb side, Rgb back,
              bool glow = false) {
  float cx = 0.0f;
  float cy = 0.0f;
  for (const auto& p : outline) {
    cx += p.first / static_cast<float>(outline.size());
    cy += p.second / static_cast<float>(outline.size());
  }
  b.Centre(cx, cy, (z0 + z1) / 2.0f);
  std::vector<std::size_t> near;
  std::vector<std::size_t> far;
  for (const auto& p : outline) {
    near.push_back(b.Vertex(p.first, p.second, z1));
    far.push_back(b.Vertex(p.first, p.second, z0));
  }
  b.Poly(near, front, glow);
  b.Poly(Reversed(far), back);
  for (std::size_t i = 0; i < outline.size(); ++i) {
    const std::size_t j = (i + 1) % outline.size();
    b.Quad(near[i], near[j], far[j], far[i], side);
  }
}

/// A straight bar from (x0, y0) to (x1, y1), w0 wide at the start and w1 at the end (half widths)
Outline Bar(float x0, float y0, float x1, float y1, float w0, float w1) {
  const float dx = x1 - x0;
  const float dy = y1 - y0;
  const float length = std::sqrt(dx * dx + dy * dy);
  const float nx = -dy / length;
  const float ny = dx / length;
  return {{x0 + nx * w0, y0 + ny * w0}, {x1 + nx * w1, y1 + ny * w1}, {x1 - nx * w1, y1 - ny * w1},
          {x0 - nx * w0, y0 - ny * w0}};
}

Outline Mirrored(Outline outline) {
  for (auto& p : outline) {
    p.first = -p.first;
  }
  return outline;
}

/// A curved strip through the points `pts`, `half` wide on either side of the line at the start and `half_end` at the
/// end, as a row of convex
/// quads that share their edges (so they cannot fight over the same pixels)
std::vector<Outline> CurvedStrip(const std::vector<std::pair<float, float>>& pts, float half, float half_end) {
  const std::size_t n = pts.size();
  std::vector<std::pair<float, float>> left(n);
  std::vector<std::pair<float, float>> right(n);
  for (std::size_t i = 0; i < n; ++i) {
    const std::size_t a = i == 0 ? 0 : i - 1;
    const std::size_t c = i + 1 == n ? n - 1 : i + 1;
    float dx = pts[c].first - pts[a].first;
    float dy = pts[c].second - pts[a].second;
    const float length = std::sqrt(dx * dx + dy * dy);
    dx /= length;
    dy /= length;
    const float w = n > 1 ? half + (half_end - half) * static_cast<float>(i) / static_cast<float>(n - 1) : half;
    left[i] = {pts[i].first - dy * w, pts[i].second + dx * w};
    right[i] = {pts[i].first + dy * w, pts[i].second - dx * w};
  }
  std::vector<Outline> quads;
  for (std::size_t i = 0; i + 1 < n; ++i) {
    quads.push_back({left[i], left[i + 1], right[i + 1], right[i]});
  }
  return quads;
}

/// The Elite emblem in 3D, facing +z: a pair of swept wings with curling feathers round a small ship, the ELITE banner
/// with flared wing tips, a chevron, a spear below and a dark backing plate. Drawn here as original work from the shape
/// of the emblem: every part is its own convex plate, layered in depth.
Model BuildEliteLogo() {
  ModelBuilder b("ELITE LOGO", Spin::kSway);
  const Rgb gold{255, 204, 70};
  const Rgb bright{255, 232, 140};
  const Rgb amber{252, 184, 52};
  const Rgb deep{176, 112, 28};
  const Rgb edge{122, 80, 22};
  const Rgb silver{244, 247, 252};
  const Rgb silver_mid{168, 174, 190};
  const Rgb steel{54, 58, 74};
  const Rgb shadow{52, 40, 26};

  // Place a piece and its mirror image
  const auto both = [&](const Outline& o, float z0, float z1, Rgb front, Rgb side, Rgb back, bool glow = false) {
    AddPlate(b, o, z0, z1, front, side, back, glow);
    AddPlate(b, Mirrored(o), z0, z1, front, side, back, glow);
  };
  const auto curve = [&](const std::vector<std::pair<float, float>>& pts, float half, float half_end, float z0, float z1,
                         Rgb front, bool glow) {
    for (const Outline& quad : CurvedStrip(pts, half, half_end)) {
      both(quad, z0, z1, front, edge, edge, glow);
    }
  };

  // The dark backing plate behind the lower half
  AddPlate(b, {{-212.0f, -36.0f}, {212.0f, -36.0f}, {0.0f, -176.0f}}, -10.0f, -3.0f, shadow, shadow, shadow);

  // Each wing is one slab: a broad plank swept up to the tip, narrowing to the feathers that hang towards the banner
  const Outline wing{{390.0f, 432.0f}, {318.0f, 436.0f}, {75.0f, 165.0f}, {15.0f, 60.0f}, {165.0f, 50.0f},
                     {240.0f, 165.0f}};
  // A dark copy behind it, shifted down and out, gives the wing some depth
  Outline behind = wing;
  for (auto& p : behind) {
    p.first += 9.0f;
    p.second -= 11.0f;
  }
  both(behind, -9.0f, -1.0f, edge, edge, edge);
  both(wing, 0.0f, 10.0f, gold, edge, edge);
  // The panel inside the gold rim: the wing drawn smaller about its middle
  float mx = 0.0f;
  float my = 0.0f;
  for (const auto& p : wing) {
    mx += p.first / static_cast<float>(wing.size());
    my += p.second / static_cast<float>(wing.size());
  }
  Outline panel = wing;
  for (auto& p : panel) {
    p.first = mx + (p.first - mx) * 0.86f;
    p.second = my + (p.second - my) * 0.86f;
  }
  both(panel, 10.0f, 14.0f, amber, edge, edge);
  // A fine mesh across the panel of the upper wing, as on the original: thin lines from one long edge to the other
  for (int i = 1; i <= 14; ++i) {
    const float t = 0.05f + 0.035f * static_cast<float>(i);
    const auto lerp = [](const std::pair<float, float>& p, const std::pair<float, float>& q, float f) {
      return std::make_pair(p.first + (q.first - p.first) * f, p.second + (q.second - p.second) * f);
    };
    const auto left = lerp(panel[1], panel[2], t);
    const auto right = lerp(panel[0], panel[5], t);
    const auto from = lerp(left, right, 0.04f);
    const auto to = lerp(left, right, 0.96f);
    both(Bar(from.first, from.second, to.first, to.second, 1.4f, 1.4f), 14.0f, 15.0f, deep, deep, deep);
  }
  // Two feathers: tapering blades curling down the wing, each a gold border round an amber face, standing out of it
  const std::vector<std::pair<float, float>> feather_a{{236.0f, 262.0f}, {196.0f, 226.0f}, {160.0f, 190.0f},
                                                       {150.0f, 150.0f}, {168.0f, 112.0f}, {150.0f, 78.0f}};
  const std::vector<std::pair<float, float>> feather_b{{286.0f, 224.0f}, {250.0f, 192.0f}, {224.0f, 156.0f},
                                                       {228.0f, 118.0f}, {206.0f, 82.0f}};
  for (const auto& feather : {feather_a, feather_b}) {
    curve(feather, 15.0f, 9.0f, 15.0f, 18.0f, gold, false);
    curve(feather, 9.5f, 4.5f, 18.0f, 20.0f, amber, false);
    curve(feather, 2.5f, 1.5f, 20.0f, 21.0f, bright, true);
  }
  // The wing's own bright edge along the long upper side and the outer side
  curve({{318.0f, 436.0f}, {196.0f, 300.0f}, {75.0f, 165.0f}}, 4.5f, 4.5f, 10.0f, 15.0f, bright, true);
  curve({{390.0f, 432.0f}, {315.0f, 300.0f}, {240.0f, 165.0f}}, 4.5f, 4.5f, 10.0f, 15.0f, bright, true);
  // The hooks on the outer edge
  both(Bar(300.0f, 282.0f, 352.0f, 282.0f, 7.0f, 7.0f), 4.0f, 14.0f, bright, edge, edge, true);
  both(Bar(346.0f, 282.0f, 346.0f, 322.0f, 7.0f, 7.0f), 4.0f, 14.0f, bright, edge, edge, true);
  both(Bar(234.0f, 160.0f, 284.0f, 160.0f, 7.0f, 7.0f), 4.0f, 14.0f, bright, edge, edge, true);
  both(Bar(278.0f, 160.0f, 278.0f, 198.0f, 7.0f, 7.0f), 4.0f, 14.0f, bright, edge, edge, true);

  // The ship in the middle: a spindle with a dark canopy, short wings, two antennae and a stem down to the banner
  AddPlate(b, {{0.0f, 214.0f}, {-15.0f, 172.0f}, {-12.0f, 124.0f}, {0.0f, 94.0f}, {12.0f, 124.0f}, {15.0f, 172.0f}},
           -4.0f, 16.0f, bright, gold, gold, true);
  AddPlate(b, {{0.0f, 190.0f}, {-7.0f, 164.0f}, {0.0f, 150.0f}, {7.0f, 164.0f}}, 16.0f, 19.0f, steel, steel, steel);
  AddPlate(b, Bar(-36.0f, 100.0f, 36.0f, 100.0f, 4.0f, 4.0f), 2.0f, 8.0f, gold, edge, edge);
  both(Bar(10.0f, 176.0f, 16.0f, 250.0f, 3.5f, 2.0f), 0.0f, 6.0f, gold, edge, edge);
  AddPlate(b, Bar(0.0f, 98.0f, 0.0f, 36.0f, 10.0f, 12.0f), 0.0f, 8.0f, gold, edge, edge);

  // The banner and the flared wing tips on either side of it
  AddPlate(b, {{-176.0f, -36.0f}, {176.0f, -36.0f}, {176.0f, 36.0f}, {-176.0f, 36.0f}}, 0.0f, 8.0f, steel, edge, edge);
  both(Bar(170.0f, 30.0f, 276.0f, 30.0f, 7.0f, 7.0f), 0.0f, 9.0f, gold, edge, edge);
  both(Bar(170.0f, 8.0f, 252.0f, 8.0f, 7.0f, 7.0f), 0.0f, 9.0f, gold, edge, edge);
  both(Bar(170.0f, -14.0f, 232.0f, -14.0f, 7.0f, 7.0f), 0.0f, 9.0f, gold, edge, edge);
  both(Bar(170.0f, -36.0f, 214.0f, -36.0f, 6.0f, 6.0f), 0.0f, 9.0f, gold, edge, edge);
  // The frame round the banner
  AddPlate(b, Bar(-182.0f, 40.0f, 182.0f, 40.0f, 5.0f, 5.0f), 0.0f, 10.0f, bright, edge, edge, true);
  AddPlate(b, Bar(-182.0f, -40.0f, 182.0f, -40.0f, 5.0f, 5.0f), 0.0f, 10.0f, bright, edge, edge, true);

  // The letters, bevelled: a wider grey slab with a smaller bright one on top
  float x = -164.0f;
  const auto stroke = [&](float x0, float y0, float x1, float y1) {
    AddPlate(b, {{x + x0, y0}, {x + x1, y0}, {x + x1, y1}, {x + x0, y1}}, 8.0f, 16.0f, silver_mid, silver_mid, steel);
    const float inset = 2.5f;
    AddPlate(b,
             {{x + x0 + inset, y0 + inset}, {x + x1 - inset, y0 + inset}, {x + x1 - inset, y1 - inset},
              {x + x0 + inset, y1 - inset}},
             16.0f, 20.0f, silver, silver_mid, silver_mid, true);
  };
  // E
  stroke(0.0f, -30.0f, 13.0f, 30.0f);
  stroke(13.0f, -30.0f, 60.0f, -17.0f);
  stroke(13.0f, -6.5f, 52.0f, 6.5f);
  stroke(13.0f, 17.0f, 60.0f, 30.0f);
  x += 78.0f;
  // L
  stroke(0.0f, -30.0f, 13.0f, 30.0f);
  stroke(13.0f, -30.0f, 54.0f, -17.0f);
  x += 72.0f;
  // I
  stroke(0.0f, 17.0f, 22.0f, 30.0f);
  stroke(0.0f, -30.0f, 22.0f, -17.0f);
  stroke(4.5f, -17.0f, 17.5f, 17.0f);
  x += 40.0f;
  // T
  stroke(0.0f, 17.0f, 60.0f, 30.0f);
  stroke(23.5f, -30.0f, 36.5f, 17.0f);
  x += 78.0f;
  // E
  stroke(0.0f, -30.0f, 13.0f, 30.0f);
  stroke(13.0f, -30.0f, 60.0f, -17.0f);
  stroke(13.0f, -6.5f, 52.0f, 6.5f);
  stroke(13.0f, 17.0f, 60.0f, 30.0f);

  // The chevrons under the banner, the spear and its ball
  both(Bar(26.0f, -98.0f, 168.0f, -58.0f, 9.0f, 8.0f), 0.0f, 9.0f, gold, edge, edge);
  both(Bar(26.0f, -128.0f, 118.0f, -102.0f, 7.0f, 6.0f), 0.0f, 9.0f, amber, edge, edge);
  AddPlate(b, {{0.0f, -62.0f}, {30.0f, -118.0f}, {0.0f, -232.0f}, {-30.0f, -118.0f}}, -2.0f, 14.0f, bright, gold, edge);
  AddPlate(b, {{0.0f, -84.0f}, {12.0f, -122.0f}, {0.0f, -190.0f}, {-12.0f, -122.0f}}, 14.0f, 17.0f, amber, deep, deep);
  Outline ball;
  for (int i = 0; i < 10; ++i) {
    const float a = 2.0f * kPi * static_cast<float>(i) / 10.0f;
    ball.push_back({11.0f * std::cos(a), -247.0f + 11.0f * std::sin(a)});
  }
  AddPlate(b, ball, -2.0f, 12.0f, gold, edge, edge);
  // The renderer looks at a ship from the front with its +x on the left, so turn the emblem about to read the right way
  Model model = b.Finish(3.2f);
  for (Vec3& v : model.vertices) {
    v.x = -v.x;
  }
  for (Face& f : model.faces) {
    f.centre.x = -f.centre.x;
  }
  return model;
}

/// The ship with its real size
Model Sized(Model model, float metres) {
  model.size_metres = metres;
  return model;
}

/// The ships, in ShipId order. The Cobra Mk III entries share a model, and so do the two Pythons.
std::vector<Model> BuildAllShips() {
  std::vector<Model> ships;
  ships.push_back(BuildSidewinder());
  ships.push_back(BuildViper());
  ships.push_back(BuildMamba());
  ships.push_back(BuildPython("PYTHON", {176, 148, 108}, {204, 178, 134}, {40, 62, 96}, {64, 56, 48}, {78, 70, 62}));
  ships.push_back(
      BuildCobra("COBRA MK III BOUNTY HUNTER", {88, 94, 112}, {120, 128, 150}, {200, 44, 44}, {40, 42, 52}));
  ships.push_back(BuildThargoid());
  ships.push_back(BuildCobra("COBRA MK III TRADER", {204, 194, 164}, {230, 222, 196}, {44, 150, 150}, {84, 80, 70}));
  ships.push_back(Sized(BuildCoriolis(), 1000.0f));
  ships.push_back(BuildMissile());
  ships.push_back(BuildAsteroid());
  ships.push_back(BuildCanister());
  ships.push_back(BuildThargon());
  ships.push_back(BuildEscapePod());
  ships.push_back(BuildAlloyPlate());
  ships.push_back(BuildBoulder());
  ships.push_back(BuildSplinter());
  ships.push_back(BuildShuttle());
  ships.push_back(BuildTransporter());
  ships.push_back(BuildBoa());
  ships.push_back(BuildAnaconda());
  ships.push_back(BuildRockHermit());
  ships.push_back(BuildCobraMk1());
  ships.push_back(BuildAdder());
  ships.push_back(BuildKrait());
  ships.push_back(BuildGecko());
  ships.push_back(BuildWorm());
  ships.push_back(BuildAspMk2());
  ships.push_back(BuildFerDeLance());
  ships.push_back(BuildMoray());
  ships.push_back(BuildConstrictor());
  ships.push_back(BuildCougar());
  ships.push_back(Sized(BuildDodo(), 800.0f));
  // Much bigger than the other stations
  ships.push_back(Sized(BuildWheelStation(), 10000.0f));
  // The paint of the pirates, and the logo
  ships.push_back(BuildCobra("COBRA MK III PIRATE", {52, 54, 60}, {82, 60, 56}, {208, 120, 36}, {30, 28, 30}));
  ships.push_back(BuildPython("PYTHON PIRATE", {96, 52, 48}, {140, 76, 62}, {60, 90, 60}, {50, 34, 32}, {64, 44, 40}));
  ships.push_back(BuildEliteLogo());
  return ships;
}

}  // namespace

const std::vector<Model>& AllShips() {
  static const std::vector<Model> ships = BuildAllShips();
  return ships;
}

const Model& GetShip(ShipId id) { return AllShips().at(static_cast<std::size_t>(id)); }

}  // namespace open_elite

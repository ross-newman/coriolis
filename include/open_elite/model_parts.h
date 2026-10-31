#ifndef OPEN_ELITE_MODEL_PARTS_H_
#define OPEN_ELITE_MODEL_PARTS_H_

#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

#include "open_elite/model_builder.h"

namespace open_elite {

/// Small convex solids to build simple models from, in the style of the ships. Each one is its own mesh, so it only has
/// to be convex by itself; they may overlap or touch.
namespace parts {

using Ring = std::vector<std::size_t>;
/// A flat shape as (u, v) points in order round its edge
using Outline = std::vector<std::pair<float, float>>;

/// The way a plate is extruded: along z the outline is in (x, y), along y it is in (x, z) and along x it is in (z, y)
enum class Axis { kX, kY, kZ };

inline Ring Reversed(const Ring& ring) { return Ring(ring.rbegin(), ring.rend()); }

/// A closed tube along z from z0 (the rear) to z1 (the front), centred on (cx, cy). Its front face can glow.
inline void Tube(ModelBuilder& b, int sides, float radius, float cx, float cy, float z0, float z1,
                 const std::vector<Rgb>& side, Rgb front, Rgb rear, bool front_glow = false) {
  b.Centre(cx, cy, (z0 + z1) / 2.0f);
  const Ring back = b.Ring(sides, radius, radius, z0, kPi / static_cast<float>(sides), cx, cy);
  const Ring head = b.Ring(sides, radius, radius, z1, kPi / static_cast<float>(sides), cx, cy);
  b.Band(back, head, side);
  b.Poly(head, front, front_glow);
  b.Poly(Reversed(back), rear);
}

/// A ball of radius r at (cx, cy, cz): `sides` round and `bands` from pole to pole, the colours repeating
inline void Ball(ModelBuilder& b, float cx, float cy, float cz, float r, int sides, int bands,
                 const std::vector<Rgb>& colours) {
  b.Centre(cx, cy, cz);
  const std::size_t nose = b.Vertex(cx, cy, cz + r);
  const std::size_t tail = b.Vertex(cx, cy, cz - r);
  std::vector<Ring> rings;
  for (int k = 1; k < bands; ++k) {
    const float theta = kPi * static_cast<float>(k) / static_cast<float>(bands);
    rings.push_back(b.Ring(sides, r * std::sin(theta), r * std::sin(theta), cz + r * std::cos(theta),
                           kPi / static_cast<float>(sides), cx, cy));
  }
  b.Fan(nose, rings.front(), colours);
  for (std::size_t i = 0; i + 1 < rings.size(); ++i) {
    b.Band(rings[i], rings[i + 1], {colours[i % colours.size()], colours[(i + 1) % colours.size()]});
  }
  b.Fan(tail, Reversed(rings.back()), colours);
}

/// A flat piece with a convex outline, from a0 (the back) to a1 (the front) along the axis
inline void Plate(ModelBuilder& b, const Outline& outline, Axis axis, float a0, float a1, Rgb front, Rgb side, Rgb back,
                  bool glow = false) {
  const auto place = [&](float u, float v, float a) {
    switch (axis) {
      case Axis::kX:
        return b.Vertex(a, v, u);
      case Axis::kY:
        return b.Vertex(u, a, v);
      default:
        return b.Vertex(u, v, a);
    }
  };
  float cu = 0.0f;
  float cv = 0.0f;
  for (const auto& p : outline) {
    cu += p.first / static_cast<float>(outline.size());
    cv += p.second / static_cast<float>(outline.size());
  }
  const float ca = (a0 + a1) / 2.0f;
  switch (axis) {
    case Axis::kX:
      b.Centre(ca, cv, cu);
      break;
    case Axis::kY:
      b.Centre(cu, ca, cv);
      break;
    default:
      b.Centre(cu, cv, ca);
      break;
  }
  Ring near;
  Ring far;
  for (const auto& p : outline) {
    near.push_back(place(p.first, p.second, a1));
    far.push_back(place(p.first, p.second, a0));
  }
  b.Poly(near, front, glow);
  b.Poly(Reversed(far), back);
  for (std::size_t i = 0; i < outline.size(); ++i) {
    const std::size_t j = (i + 1) % outline.size();
    b.Quad(near[i], near[j], far[j], far[i], side);
  }
}

/// A six sided crystal from `base` in the direction `dir` (a unit vector), `length` long and `radius` thick, ending in
/// a point
inline void Crystal(ModelBuilder& b, Vec3 base, Vec3 dir, float radius, float length, Rgb side, Rgb tip_colour) {
  // Two directions at right angles to dir
  const Vec3 helper = std::fabs(dir.y) < 0.9f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{1.0f, 0.0f, 0.0f};
  Vec3 u{dir.y * helper.z - dir.z * helper.y, dir.z * helper.x - dir.x * helper.z, dir.x * helper.y - dir.y * helper.x};
  const float ul = std::sqrt(u.x * u.x + u.y * u.y + u.z * u.z);
  u = {u.x / ul, u.y / ul, u.z / ul};
  const Vec3 v{dir.y * u.z - dir.z * u.y, dir.z * u.x - dir.x * u.z, dir.x * u.y - dir.y * u.x};
  const float shoulder = 0.7f * length;
  b.Centre(base.x + dir.x * length / 2.0f, base.y + dir.y * length / 2.0f, base.z + dir.z * length / 2.0f);
  Ring low;
  Ring high;
  for (int i = 0; i < 6; ++i) {
    const float a = 2.0f * kPi * static_cast<float>(i) / 6.0f;
    const float c = radius * std::cos(a);
    const float s = radius * std::sin(a);
    const Vec3 offset{u.x * c + v.x * s, u.y * c + v.y * s, u.z * c + v.z * s};
    low.push_back(b.Vertex(base.x + offset.x, base.y + offset.y, base.z + offset.z));
    high.push_back(b.Vertex(base.x + offset.x + dir.x * shoulder, base.y + offset.y + dir.y * shoulder,
                            base.z + offset.z + dir.z * shoulder));
  }
  const std::size_t tip = b.Vertex(base.x + dir.x * length, base.y + dir.y * length, base.z + dir.z * length);
  b.Band(low, high, {side});
  b.Fan(tip, high, {tip_colour, side});
  b.Poly(Reversed(low), side);
}

}  // namespace parts

}  // namespace open_elite

#endif  // OPEN_ELITE_MODEL_PARTS_H_

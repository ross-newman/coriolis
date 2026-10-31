#ifndef CORIOLIS_MODEL_H_
#define CORIOLIS_MODEL_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "coriolis/types.h"

namespace coriolis {

/// One triangle of a ship model
struct Face {
  /// Indices of the three vertices
  std::array<std::size_t, 3> v;
  /// Surface colour
  Rgb colour;
  /// Emissive surfaces (engine glow, lights) ignore the lighting
  bool emissive;
  /// Edges to outline: bit 0 is v0-v1, bit 1 is v1-v2 and bit 2 is v2-v0
  std::uint8_t edges;
  /// The centre of the mesh the face belongs to, used to point the normal outwards
  Vec3 centre;
};

/// How a ship spins on the display
enum class Spin {
  /// Turn about the vertical axis
  kYaw,
  /// Roll about the ship's own long axis, as a space station does, showing its front to the viewer
  kRoll,
  /// Face the viewer and swing gently from side to side, for a flat emblem
  kSway
};

/// A ship: vertices and triangles
struct Model {
  std::string name;
  Spin spin = Spin::kYaw;
  /// How big the ship really is across, in metres, or 0 when not set. Models are all drawn at the same size.
  float size_metres = 0.0f;
  std::vector<Vec3> vertices;
  std::vector<Face> faces;
};

}  // namespace coriolis

#endif  // CORIOLIS_MODEL_H_

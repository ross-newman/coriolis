#ifndef CORIOLIS_BLUEPRINT_H_
#define CORIOLIS_BLUEPRINT_H_

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "coriolis/json.h"
#include "coriolis/model.h"
#include "coriolis/types.h"

namespace coriolis {

/// A blueprint is everything the game needs to know about one kind of ship, kept as data: the shape, the handling,
/// the weapon mounts, the parts that can be shot off and the behaviour profile. Blueprints are JSON files so ships
/// can be added or changed without rebuilding. Lengths are in mesh units; `scale` turns them into metres.

/// What sort of object the blueprint describes
enum class HullKind { kShip, kStation, kMissile, kCanister, kAsteroid, kEscapePod };

/// One point of the shape. `visibility` is the distance in metres beyond which the point is not drawn in the
/// wireframe overlay, 0 meaning always.
struct BlueprintVertex {
  Vec3 position;
  float visibility = 0.0f;
};

/// A triangle of the hull, wound counter-clockwise seen from outside
struct BlueprintTriangle {
  std::array<std::uint32_t, 3> v;
  Rgb colour;
  /// Emissive surfaces (engine glow, lights) ignore the lighting
  bool emissive = false;
  /// Edges to outline: bit 0 is v0-v1, bit 1 is v1-v2 and bit 2 is v2-v0
  std::uint8_t edges = 0;
};

/// A line of the wireframe overlay
struct BlueprintEdge {
  std::uint32_t a = 0;
  std::uint32_t b = 0;
  float visibility = 0.0f;
};

enum class HardpointKind { kLaser, kMissile, kScoop, kEngine, kDock };

/// A place on the hull where something is mounted
struct Hardpoint {
  HardpointKind kind = HardpointKind::kLaser;
  Vec3 position{0, 0, 0};
  /// The way the mount points, a unit vector
  Vec3 direction{0, 0, 1};
};

/// A part of the ship that takes damage on its own and can be shot out
struct Subsystem {
  std::string name;
  Vec3 centre{0, 0, 0};
  /// Radius in mesh units
  float radius = 1.0f;
  float health = 100.0f;
};

/// Numbers that drive the flight model, combat and the economy. Speeds are in metres per second, angles in
/// radians per second.
struct BlueprintStats {
  float mass_kg = 1000.0f;
  float max_speed = 100.0f;
  float acceleration = 20.0f;
  float max_pitch_rate = 1.0f;
  float max_roll_rate = 1.5f;
  float max_yaw_rate = 0.5f;
  float hull = 100.0f;
  float shield = 0.0f;
  float energy = 100.0f;
  float energy_recharge = 5.0f;
  /// How far from the centre of the ship a laser still counts as a hit, in metres
  float target_radius = 5.0f;
  /// Beyond this distance the ship is drawn as a dot, in metres
  float visibility_distance = 10000.0f;
  /// Reward in credits for destroying the ship
  float bounty = 0.0f;
  /// Combat rank points for destroying the ship
  float kill_points = 0.0f;
  /// The most cargo canisters it drops when destroyed
  std::uint32_t max_cargo_drop = 0;
  /// Missiles carried
  std::uint32_t missiles = 0;
};

struct Blueprint {
  /// Lower case letters, digits and underscores
  std::string id;
  /// Name shown to the player
  std::string name;
  HullKind kind = HullKind::kShip;
  Spin spin = Spin::kYaw;
  /// Metres per mesh unit
  float scale = 1.0f;
  BlueprintStats stats;
  /// Name of a behaviour profile, empty for objects that do not act
  std::string ai_profile;
  std::vector<BlueprintVertex> vertices;
  std::vector<BlueprintTriangle> triangles;
  std::vector<BlueprintEdge> edges;
  std::vector<Hardpoint> hardpoints;
  std::vector<Subsystem> subsystems;
};

/// The version written to and expected in the "version" member of a blueprint file
constexpr int kBlueprintVersion = 1;

const char* HullKindName(HullKind kind);
const char* HardpointKindName(HardpointKind kind);

/// Everything wrong with a blueprint, one message each. Empty if it is good.
std::vector<std::string> ValidateBlueprint(const Blueprint& blueprint);

Json BlueprintToJson(const Blueprint& blueprint);
/// Returns false and says why in *error if the JSON is not a blueprint. The result is also validated.
bool BlueprintFromJson(const Json& json, Blueprint* out, std::string* error);

/// Read or write a blueprint file. Errors name the file.
bool LoadBlueprintFile(const std::string& path, Blueprint* out, std::string* error);
bool SaveBlueprintFile(const Blueprint& blueprint, const std::string& path, std::string* error);

/// The shape of a Blueprint as a Model the software renderer can draw
Model BlueprintToModel(const Blueprint& blueprint);
/// A Blueprint holding the shape of a Model, with default stats. Triangles are rewound to face outwards.
Blueprint BlueprintFromModel(const Model& model, const std::string& id);

/// The built in ships as blueprints, in ShipId order
const std::vector<Blueprint>& BuiltInBlueprints();

/// A built in blueprint by id, or nullptr
const Blueprint* FindBuiltInBlueprint(const std::string& id);

}  // namespace coriolis

#endif  // CORIOLIS_BLUEPRINT_H_

#include <algorithm>
#include <array>
#include <cmath>

#include "open_elite/blueprint.h"
#include "open_elite/ships.h"

namespace open_elite {

namespace {

/// The handling, armour and weapons of a ship, apart from its shape. These are this project's own numbers.
struct Info {
  const char* id;
  HullKind kind;
  const char* ai;
  /// Metres per mesh unit, or 0 to work it out from the model's size_metres
  float scale;
  float mass_kg;
  float max_speed;
  float acceleration;
  float pitch;
  float roll;
  float yaw;
  float hull;
  float shield;
  float energy;
  float recharge;
  float target_radius;
  float visibility;
  float bounty;
  float kill_points;
  std::uint32_t cargo;
  std::uint32_t missiles;
  int lasers;
  int engines;
  bool scoop;
};

constexpr HullKind kShip = HullKind::kShip;
constexpr HullKind kStation = HullKind::kStation;
constexpr HullKind kCanister = HullKind::kCanister;
constexpr HullKind kAsteroid = HullKind::kAsteroid;

// clang-format off
// id, kind, ai, scale, mass, speed, accel, pitch, roll, yaw, hull, shield, energy, recharge, target, visibility,
// bounty, kill points, cargo, missiles, lasers, engines, scoop
const std::array<Info, kShipCount> kInfo{{
    {"sidewinder", kShip, "pirate_light", 5.5f, 9000, 450, 90, 1.8f, 2.6f, 0.9f, 70, 40, 60, 4, 22, 12000, 50, 0.33f, 0, 0, 1, 1, false},
    {"viper", kShip, "police", 7.0f, 14000, 420, 80, 1.6f, 2.3f, 0.8f, 120, 80, 100, 6, 28, 12000, 0, 0.1f, 0, 1, 2, 2, false},
    {"mamba", kShip, "pirate_medium", 6.5f, 12000, 500, 100, 1.7f, 2.4f, 0.8f, 90, 70, 90, 5, 20, 12000, 150, 0.5f, 1, 2, 2, 3, false},
    {"python", kShip, "trader_armed", 15.0f, 90000, 250, 35, 0.7f, 1.0f, 0.35f, 400, 250, 200, 8, 60, 15000, 200, 0.66f, 5, 3, 2, 2, false},
    {"cobra_mk3_bounty_hunter", kShip, "bounty_hunter", 8.5f, 20000, 400, 70, 1.3f, 2.0f, 0.7f, 200, 150, 150, 8, 34, 14000, 120, 0.9f, 3, 2, 2, 2, true},
    {"thargoid", kShip, "alien_mothership", 17.0f, 80000, 300, 45, 0.9f, 0.9f, 0.6f, 450, 300, 300, 10, 75, 20000, 500, 2.6f, 0, 6, 2, 0, false},
    {"cobra_mk3_trader", kShip, "trader", 8.5f, 20000, 380, 65, 1.2f, 1.9f, 0.7f, 160, 100, 100, 6, 34, 14000, 0, 0.9f, 3, 0, 1, 2, true},
    {"coriolis", kStation, "station", 0.0f, 1e9f, 0, 0, 0.0f, 0.06f, 0.0f, 20000, 20000, 5000, 100, 150, 60000, 0, 0.0f, 0, 0, 0, 0, false},
    {"missile", HullKind::kMissile, "", 1.4f, 150, 1100, 400, 3.5f, 3.0f, 2.5f, 10, 0, 20, 0, 8, 6000, 0, 0.2f, 0, 0, 0, 1, false},
    {"asteroid", kAsteroid, "", 25.0f, 5e6f, 30, 0, 0.05f, 0.05f, 0.05f, 300, 0, 0, 0, 60, 20000, 0, 0.03f, 3, 0, 0, 0, false},
    {"cargo_canister", kCanister, "", 0.9f, 500, 20, 0, 0.1f, 0.1f, 0.1f, 10, 0, 0, 0, 8, 4000, 0, 0.04f, 0, 0, 0, 0, false},
    {"thargon", kShip, "alien_drone", 1.6f, 400, 480, 120, 2.5f, 3.2f, 1.2f, 30, 0, 30, 3, 9, 8000, 20, 0.13f, 0, 0, 1, 1, false},
    {"escape_pod", HullKind::kEscapePod, "drifter", 1.2f, 300, 120, 30, 0.4f, 0.4f, 0.4f, 15, 0, 10, 1, 10, 5000, 0, 0.06f, 0, 0, 0, 1, false},
    {"alloy_plate", kCanister, "", 1.5f, 800, 15, 0, 0.1f, 0.1f, 0.1f, 20, 0, 0, 0, 10, 4000, 0, 0.04f, 0, 0, 0, 0, false},
    {"boulder", kAsteroid, "", 8.0f, 4e5f, 30, 0, 0.1f, 0.1f, 0.1f, 80, 0, 0, 0, 22, 8000, 0, 0.02f, 1, 0, 0, 0, false},
    {"splinter", kAsteroid, "", 4.0f, 5e4f, 40, 0, 0.2f, 0.2f, 0.2f, 30, 0, 0, 0, 12, 6000, 0, 0.04f, 0, 0, 0, 0, false},
    {"shuttle", kShip, "trader", 6.5f, 12000, 300, 50, 0.9f, 1.2f, 0.5f, 80, 40, 60, 4, 26, 12000, 0, 0.06f, 1, 0, 0, 2, false},
    {"transporter", kShip, "trader", 12.0f, 60000, 220, 30, 0.5f, 0.7f, 0.3f, 220, 120, 120, 5, 45, 14000, 0, 0.07f, 4, 0, 1, 2, false},
    {"boa", kShip, "pirate_heavy", 14.0f, 80000, 260, 38, 0.6f, 0.9f, 0.35f, 380, 220, 180, 8, 55, 15000, 250, 0.83f, 4, 2, 2, 3, false},
    {"anaconda", kShip, "trader_armed", 20.0f, 120000, 230, 30, 0.5f, 0.7f, 0.3f, 520, 350, 250, 10, 75, 16000, 350, 1.0f, 7, 2, 2, 2, false},
    {"rock_hermit", kStation, "", 60.0f, 5e8f, 0, 0, 0.0f, 0.0f, 0.0f, 800, 0, 100, 5, 120, 30000, 80, 0.33f, 0, 0, 0, 0, false},
    {"cobra_mk1", kShip, "pirate_light", 6.5f, 14000, 340, 60, 1.2f, 1.8f, 0.7f, 110, 60, 70, 5, 26, 12000, 80, 0.66f, 2, 1, 1, 2, false},
    {"adder", kShip, "trader_armed", 6.8f, 15000, 360, 65, 1.2f, 1.8f, 0.7f, 100, 60, 80, 5, 26, 12000, 70, 0.35f, 2, 0, 1, 2, false},
    {"krait", kShip, "pirate_light", 6.0f, 10000, 480, 95, 1.8f, 2.5f, 0.9f, 85, 60, 80, 5, 22, 12000, 100, 0.33f, 1, 0, 1, 1, false},
    {"gecko", kShip, "pirate_light", 5.0f, 8000, 460, 95, 1.9f, 2.6f, 0.9f, 65, 40, 60, 4, 20, 11000, 55, 0.33f, 0, 0, 1, 2, false},
    {"worm", kShip, "alien_drone", 5.5f, 9000, 420, 90, 1.8f, 2.4f, 0.9f, 60, 30, 55, 4, 22, 10000, 40, 0.2f, 0, 0, 1, 2, false},
    {"asp_mk2", kShip, "bounty_hunter", 7.5f, 18000, 520, 100, 1.5f, 2.2f, 0.8f, 150, 120, 130, 7, 28, 14000, 200, 1.08f, 2, 2, 2, 2, false},
    {"fer_de_lance", kShip, "pirate_medium", 8.5f, 20000, 480, 90, 1.4f, 2.0f, 0.8f, 190, 150, 150, 8, 30, 14000, 250, 1.25f, 2, 2, 2, 2, false},
    {"moray", kShip, "trader_armed", 7.5f, 16000, 360, 60, 1.1f, 1.5f, 0.6f, 130, 80, 100, 6, 32, 12000, 100, 0.75f, 1, 1, 1, 2, false},
    {"constrictor", kShip, "elite_hunter", 9.0f, 30000, 560, 110, 1.5f, 2.2f, 0.8f, 450, 350, 300, 12, 36, 18000, 600, 5.33f, 3, 4, 2, 3, false},
    {"cougar", kShip, "elite_hunter", 8.5f, 22000, 540, 105, 1.6f, 2.3f, 0.85f, 380, 300, 250, 11, 34, 16000, 550, 5.33f, 2, 4, 2, 2, false},
    {"dodo", kStation, "station", 0.0f, 8e8f, 0, 0, 0.0f, 0.08f, 0.0f, 16000, 16000, 4000, 80, 130, 50000, 0, 0.0f, 0, 0, 0, 0, false},
    {"wheel_station", kStation, "station", 0.0f, 5e10f, 0, 0, 0.0f, 0.03f, 0.0f, 90000, 90000, 20000, 400, 700, 200000, 0, 0.0f, 0, 0, 0, 0, false},
    {"cobra_mk3_pirate", kShip, "pirate_medium", 8.5f, 20000, 400, 70, 1.3f, 2.0f, 0.7f, 150, 100, 110, 6, 34, 14000, 175, 1.0f, 3, 1, 2, 2, false},
    {"python_pirate", kShip, "pirate_heavy", 15.0f, 90000, 250, 35, 0.7f, 1.0f, 0.35f, 400, 250, 200, 8, 60, 15000, 300, 1.0f, 5, 3, 2, 2, false},
    {"elite_logo", kCanister, "", 6.0f, 1000, 0, 0, 0.0f, 0.0f, 0.0f, 100, 0, 0, 0, 14, 20000, 0, 0.0f, 0, 0, 0, 0, false},
}};
// clang-format on

Blueprint Build(std::size_t index, const Model& model) {
  const Info& info = kInfo[index];
  Blueprint b = BlueprintFromModel(model, info.id);
  b.kind = info.kind;
  b.ai_profile = info.ai;
  b.scale = info.scale > 0.0f ? info.scale : model.size_metres / 6.4f;

  BlueprintStats& s = b.stats;
  s.mass_kg = info.mass_kg;
  s.max_speed = info.max_speed;
  s.acceleration = info.acceleration;
  s.max_pitch_rate = info.pitch;
  s.max_roll_rate = info.roll;
  s.max_yaw_rate = info.yaw;
  s.hull = info.hull;
  s.shield = info.shield;
  s.energy = info.energy;
  s.energy_recharge = info.recharge;
  s.target_radius = info.target_radius;
  s.visibility_distance = info.visibility;
  s.bounty = info.bounty;
  s.kill_points = info.kill_points;
  s.max_cargo_drop = info.cargo;
  s.missiles = info.missiles;

  // Mounts and parts, placed from the size of the shape
  float x_max = 0.0f;
  float y_max = 0.0f;
  float z_max = -1e9f;
  float z_min = 1e9f;
  for (const BlueprintVertex& v : b.vertices) {
    x_max = std::max(x_max, std::fabs(v.position.x));
    y_max = std::max(y_max, std::fabs(v.position.y));
    z_max = std::max(z_max, v.position.z);
    z_min = std::min(z_min, v.position.z);
  }
  const auto spread = [](int count, int i) {
    return count == 1 ? 0.0f : (static_cast<float>(i) / static_cast<float>(count - 1) * 2.0f - 1.0f);
  };
  for (int i = 0; i < info.lasers; ++i) {
    const float x = info.lasers == 1 ? 0.0f : spread(info.lasers, i) * 0.35f * x_max;
    b.hardpoints.push_back({HardpointKind::kLaser, {x, 0.0f, z_max * 0.85f}, {0.0f, 0.0f, 1.0f}});
  }
  if (info.missiles > 0 || info.kind == HullKind::kMissile) {
    b.hardpoints.push_back({HardpointKind::kMissile, {0.0f, -0.4f * y_max, z_max * 0.5f}, {0.0f, 0.0f, 1.0f}});
  }
  for (int i = 0; i < info.engines; ++i) {
    b.hardpoints.push_back(
        {HardpointKind::kEngine, {spread(info.engines, i) * 0.4f * x_max, 0.0f, z_min}, {0.0f, 0.0f, -1.0f}});
  }
  if (info.scoop) {
    b.hardpoints.push_back({HardpointKind::kScoop, {0.0f, -0.5f * y_max, z_max * 0.9f}, {0.0f, 0.0f, 1.0f}});
  }
  if (info.kind == HullKind::kStation) {
    // Every station in the set has its bay at the front, on the axis
    b.hardpoints.push_back({HardpointKind::kDock, {0.0f, 0.0f, z_max}, {0.0f, 0.0f, 1.0f}});
  }
  if (info.kind == HullKind::kShip) {
    const float r = std::max(0.3f, 0.25f * std::max(x_max, y_max));
    if (info.engines > 0) {
      b.subsystems.push_back({"engines", {0.0f, 0.0f, z_min * 0.8f}, r, 0.25f * info.hull});
    }
    if (info.lasers > 0) {
      b.subsystems.push_back({"weapons", {0.0f, 0.0f, z_max * 0.7f}, r, 0.2f * info.hull});
    }
    b.subsystems.push_back({"reactor", {0.0f, 0.0f, 0.0f}, r, 0.3f * info.hull});
  }
  return b;
}

std::vector<Blueprint> BuildAll() {
  std::vector<Blueprint> all;
  const std::vector<Model>& models = AllShips();
  for (std::size_t i = 0; i < kShipCount; ++i) {
    all.push_back(Build(i, models[i]));
  }
  return all;
}

}  // namespace

const std::vector<Blueprint>& BuiltInBlueprints() {
  static const std::vector<Blueprint> all = BuildAll();
  return all;
}

const Blueprint* FindBuiltInBlueprint(const std::string& id) {
  for (const Blueprint& b : BuiltInBlueprints()) {
    if (b.id == id) {
      return &b;
    }
  }
  return nullptr;
}

}  // namespace open_elite

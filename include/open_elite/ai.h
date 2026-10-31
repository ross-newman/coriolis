#ifndef OPEN_ELITE_AI_H_
#define OPEN_ELITE_AI_H_

#include <string>

#include "open_elite/world.h"

namespace open_elite {

/// What kind of pilot a ship has. The blueprint names one in `ai_profile`.
///
/// A fighting pilot works through a small set of moods: it looks for a target, approaches it, makes attack runs firing
/// at the point where the target will be, breaks away at close range or after a while and comes round again, and runs
/// for it when its hull gets low. A trader just flies to the station and runs from anything hostile. These are the same
/// ideas as in the original game, with a few more: leading the target, using missiles and ECM, and flying in packs.
struct AiProfile {
  const char* name;
  /// Hostile to the player without being provoked
  bool attacks_player;
  /// Hostile to pirates and aliens, and to a wanted player
  bool hunts_pirates;
  /// Hostile to traders
  bool hunts_traders;
  /// Fights at all (everyone fights back at whoever hurt them)
  bool combat;
  /// Runs from anything hostile and flies to the station
  bool flees;
  /// 0 to 1: how long it presses an attack before breaking off, and how close it comes
  double aggression;
  /// 0 to 1: how well it aims
  double accuracy;
  /// Runs when the hull falls below this fraction
  double flee_hull;
  double detect_range;
  bool uses_missiles;
  bool uses_ecm;
  /// How many drones it can launch
  int drones;
  double laser_damage;
  /// Throttle when not in a fight
  double cruise_throttle;
};

const AiProfile* FindAiProfile(const std::string& name);

/// Is `other` something `self` wants to shoot?
bool IsHostile(const World& world, const Entity& self, const Entity& other);

/// Think and set the ship's controls
void UpdateAi(World& world, Entity& ship, double dt);

/// The docking computer: fly the ship to the station's bay, matching the station's roll. Sets the controls and returns
/// true while it is flying; false if there is no station that will take the ship.
bool UpdateAutopilot(World& world, Entity& ship, double dt);

}  // namespace open_elite

#endif  // OPEN_ELITE_AI_H_

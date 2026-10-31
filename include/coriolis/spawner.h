#ifndef CORIOLIS_SPAWNER_H_
#define CORIOLIS_SPAWNER_H_

#include "coriolis/asteroids.h"
#include "coriolis/galaxy.h"
#include "coriolis/world.h"

namespace coriolis {

/// How far from the station its safe zone reaches: no ambushes start inside it
constexpr double kSafeZone = 9000.0;

/// Fills space with traffic: traders on their way to the station, packs of pirates, police, bounty hunters and, rarely,
/// aliens. What turns up depends on the system's government and tech level and on what the player has done: the police
/// come faster for a wanted pilot, and a pilot with a high rank meets tougher pirates.
class Spawner {
 public:
  /// Scatter asteroids about the player, for a new arrival. Every system has its own field, the same each time: loose
  /// rocks of every size from splinters to giants, the big ones with debris around them. The rocks are made as they are
  /// needed and kept by the spawner, which has to outlive the world they are put in.
  void PopulateRocks(World& world, const StarSystem& system);

  /// The asteroids made so far
  const AsteroidCatalog& rocks() const { return rocks_; }

  /// Maybe add something. Call every step with the step's length. `kills` is the player's combat score and `wanted`
  /// whether the police are after the player. When `peaceful`, only traders and police come: no pirates, bounty
  /// hunters or aliens.
  void Update(World& world, const StarSystem& system, double dt, double kills, bool wanted, bool peaceful = false);

 private:
  Vec3d PlaceAround(World& world, const Vec3d& player_position, double min_range, double max_range,
                    bool outside_safe_zone);
  void SpawnPack(World& world, const Vec3d& player_position, const char* const* pool, int pool_size, Team team,
                 int count);

  int tick_ = 0;
  AsteroidCatalog rocks_;
};

}  // namespace coriolis

#endif  // CORIOLIS_SPAWNER_H_

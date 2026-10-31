#ifndef CORIOLIS_ASTEROIDS_H_
#define CORIOLIS_ASTEROIDS_H_

#include <cstdint>
#include <map>
#include <memory>

#include "coriolis/blueprint.h"
#include "coriolis/model.h"
#include "coriolis/random.h"

namespace coriolis {

/// How big a rock is. Each class has its own range of radii, from a few metres to most of a kilometre.
enum class RockClass { kSplinter, kBoulder, kAsteroid, kLarge, kGiant };

constexpr int kRockClassCount = 5;

/// What a rock is made of: its colour, its mass and how many minerals it gives up
enum class RockType { kRocky, kMetallic, kIcy, kCarbon };

/// Everything that makes one asteroid what it is. The same parameters always give the same asteroid.
struct AsteroidParams {
  /// Drives every random choice of the shape
  std::uint32_t seed = 0;
  RockClass size_class = RockClass::kAsteroid;
  RockType type = RockType::kRocky;
  /// Radius of the smallest sphere round the rock, in metres
  float radius_m = 100.0f;
  /// How much longer the long axis is than the others: 0 is round
  float elongation = 0.4f;
  /// How much the long axis is squashed flat on one side: 1 is not flattened
  float flatness = 1.0f;
  /// Big smooth bulges and hollows
  int lumps = 4;
  /// Round dents
  int craters = 2;
  /// Size of the vertex to vertex bumps, as a fraction of the radius
  float roughness = 0.06f;
  /// Times the starting 20 sided shape is split: 0 to 2 (20, 80 or 320 faces)
  int detail = 1;
};

/// The radius range of a class, in metres
float RockClassMinRadius(RockClass size_class);
float RockClassMaxRadius(RockClass size_class);
const char* RockClassName(RockClass size_class);

/// A class chosen by weight: mostly small rocks, a few large ones and now and then a giant
RockClass PickRockClass(Random& rng);

/// The parameters of the asteroid of the class that goes with the seed: its radius, type, lengthening, roughness
/// and so on are all worked out from the seed, within the limits of the class
AsteroidParams MakeAsteroidParams(std::uint32_t seed, RockClass size_class);

/// The shape of the asteroid: a split 20 sided shape pushed in and out by lumps, craters and bumps, then stretched.
/// Centred and scaled to the same size as the ships. The faces point the way their winding says, so the rock does not
/// have to be convex.
Model GenerateAsteroidModel(const AsteroidParams& params);

/// The asteroid as a blueprint of kind asteroid: its shape, size in metres, mass, hull and the minerals it drops, all
/// from its size and type
Blueprint GenerateAsteroidBlueprint(const AsteroidParams& params);

/// Keeps the asteroids that have been made, so that a world can point at them: a blueprint has to outlive the world that
/// uses it, and the cockpit keeps a model for each blueprint it has drawn. Asking for the same seed and class again
/// gives the very same blueprint, so a system's rocks are made once however often it is visited.
class AsteroidCatalog {
 public:
  /// The asteroid for the seed and class. The reference stays good for the life of the catalog.
  const Blueprint& Get(std::uint32_t seed, RockClass size_class);

  /// How many different asteroids have been made
  std::size_t size() const { return rocks_.size(); }

 private:
  std::map<std::uint64_t, std::unique_ptr<Blueprint>> rocks_;
};

}  // namespace coriolis

#endif  // CORIOLIS_ASTEROIDS_H_

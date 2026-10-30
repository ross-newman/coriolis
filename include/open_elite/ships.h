#ifndef OPEN_ELITE_SHIPS_H_
#define OPEN_ELITE_SHIPS_H_

#include <cstddef>
#include <vector>

#include "open_elite/model.h"

namespace open_elite {

/// The 13 ships, in the order of the ship list of the original game. The two Cobra Mk III entries share a model
/// and differ only in their paint.
enum class ShipId {
  kSidewinder,
  kViper,
  kMamba,
  kPython,
  kCobraMk3BountyHunter,
  kThargoid,
  kCobraMk3Trader,
  kCoriolis,
  kMissile,
  kAsteroid,
  kCargoCanister,
  kThargon,
  kEscapePod
};

/// The number of ships
constexpr std::size_t kShipCount = 13;

/// Every ship, in ShipId order. Built on first use.
const std::vector<Model>& AllShips();

/// One ship
const Model& GetShip(ShipId id);

}  // namespace open_elite

#endif  // OPEN_ELITE_SHIPS_H_

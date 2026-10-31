#ifndef CORIOLIS_SHIPS_H_
#define CORIOLIS_SHIPS_H_

#include <cstddef>
#include <vector>

#include "coriolis/model.h"

namespace coriolis {

/// The ships. The first 13 are the ones of the original cassette ship list, in its order (the Cobra Mk III
/// entries share a model and differ only in their paint, and so do the Pythons). The rest are the further ships of the later versions of the
/// game. All of the models are original work: see the README.
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
  kEscapePod,
  kAlloyPlate,
  kBoulder,
  kSplinter,
  kShuttle,
  kTransporter,
  kBoa,
  kAnaconda,
  kRockHermit,
  kCobraMk1,
  kAdder,
  kKrait,
  kGecko,
  kWorm,
  kAspMk2,
  kFerDeLance,
  kMoray,
  kConstrictor,
  kCougar,
  kDodo,
  kWheelStation,
  kCobraMk3Pirate,
  kPythonPirate,
  kEliteLogo
};

/// The number of ships
constexpr std::size_t kShipCount = 36;

/// Every ship, in ShipId order. Built on first use.
const std::vector<Model>& AllShips();

/// One ship
const Model& GetShip(ShipId id);

}  // namespace coriolis

#endif  // CORIOLIS_SHIPS_H_

#ifndef CORIOLIS_BOT_H_
#define CORIOLIS_BOT_H_

#include <cstdint>

#include "coriolis/game.h"
#include "coriolis/input.h"
#include "coriolis/random.h"

namespace coriolis {

/// A computer pilot that plays the game through the same input a person uses: it fights the pirates it meets, flies
/// clear of the station, jumps to a neighbouring system, brings the ship in with the docking computer, and then sells
/// its cargo and buys a new load. It is used to test the engine from end to end and as a demonstration or attract mode.
class BotPilot {
 public:
  explicit BotPilot(std::uint64_t seed) : rng_(seed) {}

  /// The input for this frame of flight
  FlightInput Fly(const Game& game);

  /// Called while docked: trade, refuel and choose where to go. Returns true if the ship should launch.
  bool Dock(Game& game);

  int jumps() const { return jumps_; }
  int landings() const { return landings_; }

 private:
  Random rng_;
  int jumps_ = 0;
  int landings_ = 0;
  int home_system_ = -1;
  bool want_jump_ = true;
};

}  // namespace coriolis

#endif  // CORIOLIS_BOT_H_

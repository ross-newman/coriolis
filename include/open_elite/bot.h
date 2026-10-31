#ifndef OPEN_ELITE_BOT_H_
#define OPEN_ELITE_BOT_H_

#include <cstdint>

#include "open_elite/game.h"
#include "open_elite/input.h"
#include "open_elite/random.h"

namespace open_elite {

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

}  // namespace open_elite

#endif  // OPEN_ELITE_BOT_H_

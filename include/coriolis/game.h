#ifndef CORIOLIS_GAME_H_
#define CORIOLIS_GAME_H_

#include <bitset>
#include <cstdint>
#include <string>
#include <vector>

#include "coriolis/commander.h"
#include "coriolis/galaxy.h"
#include "coriolis/input.h"
#include "coriolis/market.h"
#include "coriolis/spawner.h"
#include "coriolis/world.h"

namespace coriolis {

enum class GameMode {
  /// In the station: trade, buy equipment, choose a destination
  kDocked,
  /// In space
  kFlight,
  /// Killed. Start a new game or load one.
  kDead
};

/// The whole game as a headless engine. A front end (the window, the screens, the sound) feeds it the player's input
/// and reads back what to show. It owns the commander, the galaxy and the simulated space around the player, and ties
/// them together: arriving in a system builds its planet, sun and station and starts the traffic; shooting a pirate
/// pays a bounty and raises the combat rank; shooting a trader makes the player an offender; docking at the station
/// ends the flight.
class Game {
 public:
  Game(const Commander& commander, std::uint64_t seed);

  GameMode mode() const { return mode_; }
  const Commander& commander() const { return commander_; }
  Commander& commander() { return commander_; }
  const Galaxy& galaxy() const { return galaxy_; }
  const StarSystem& system() const { return galaxy_.system(commander_.system); }
  Market& market() { return market_; }
  const Market& market() const { return market_; }
  World& world() { return world_; }
  const World& world() const { return world_; }
  /// The player's ship, or nullptr when docked or dead
  Entity* player();
  const Entity* player() const { return const_cast<Game*>(this)->player(); }

  // Docked
  /// Leave the station. With `ride_out` the ship starts at the back of the docking bay and rides out of the mouth under
  /// its own power for a few seconds (`launching`), with the controls locked, before the pilot takes over. Without it
  /// the ship appears just outside the bay, flying free (for tests and scripts).
  bool Launch(bool ride_out = true);
  /// Is the ship still riding out of the bay?
  bool launching() const { return launch_clock_ >= 0.0; }
  /// How far through the ride out of the bay: 0 at the start, 1 when it is over
  double launch_progress() const;
  /// Is the tractor beam landing the ship? Entering the bay does not dock at once: the beam takes hold of the ship, with
  /// the controls locked, draws it into the bay and sets it down on the floor. The game is still in flight until the
  /// ship has landed, and then it is docked and the station screens come up.
  bool landing() const { return landing_clock_ >= 0.0; }
  /// How far through the landing: 0 when the beam takes hold, 1 when the ship is down
  double landing_progress() const;
  bool Buy(Commodity item, int quantity);
  bool Sell(Commodity item, int quantity);
  bool BuyItem(Item item);
  /// Buy a ship from the yard (see `coriolis::BuyShip`), trading in the one flown. Only in the station.
  bool BuyShip(int index);

  // Flight
  /// Advance by `seconds` (the game steps at a fixed 60 Hz inside) with the player's input
  void Update(const FlightInput& input, double seconds);
  void SetFlightMode(FlightMode mode) { flight_mode_ = mode; }
  FlightMode flight_mode() const { return flight_mode_; }
  bool assist() const { return assist_; }
  bool autopilot() const { return autopilot_; }
  /// The throttle setting, 0 to 1
  double throttle() const { return throttle_; }
  EntityId target() const { return target_; }
  EntityId missile_lock() const { return lock_; }

  /// Choose the system to jump to
  void SelectSystem(int index) { selected_system_ = index; }
  int selected_system() const { return selected_system_; }
  /// Begin the hyperspace countdown for the chosen system. False, with a message, if there is not the fuel, it is too
  /// far, or the ship is mass locked by something nearby.
  bool StartJump();
  /// Begin the countdown for the jump to the next galaxy, which uses up the galactic hyperdrive and no fuel. False, with a
  /// message, if there is no hyperdrive or the ship is mass locked.
  bool StartGalacticJump();
  /// Is the countdown running for a jump to the next galaxy?
  bool galactic_jump() const { return galactic_jump_ && jump_timer_ > 0.0; }
  void CancelJump() { jump_timer_ = -1.0; }
  /// Seconds left on the countdown, or a negative number when not jumping
  double jump_countdown() const { return jump_timer_; }
  /// How far the hyperspace zoom has gone, 0 to 1: it rises over the end of the countdown and falls again after the
  /// arrival
  double hyperspace_warp() const;
  /// Is something close enough to block a jump?
  bool MassLocked() const;

  /// Take the messages for the screen (and the log) since the last call
  std::vector<std::string> TakeMessages();
  /// Take the world's events since the last call, for sound and rumble
  std::vector<Event> TakeEvents();

  /// Start the game again with a new commander
  void NewGame(const Commander& commander);

  /// How the player died, in capitals, for the end screen: "SHOT DOWN BY MAMBA", "CRASHED INTO THE PLANET" and so on.
  /// Empty until the player has died.
  const std::string& death_cause() const { return death_cause_; }

 private:
  void Step(const FlightInput& input, double dt);
  void ApplyInput(const FlightInput& input, double dt);
  void ProcessEvents();
  void BuildSystemWorld(bool arriving, bool ride_out = false);
  void SpawnPlayer(const Vec3d& position, const Quat& orientation, double speed);
  void Dock();
  void Jump();
  void AdvanceLaunch(double dt);
  void StartLanding();
  void CaptureInBay();
  void AdvanceLanding(double dt);
  bool Eject();
  void OnKill(const Event& event);
  void Say(const std::string& text) { messages_.push_back(text); }
  void CycleTarget(int step = 1);
  void LockMissile();
  void RefreshMarket();

  Commander commander_;
  Galaxy galaxy_;
  Market market_;
  World world_;
  Spawner spawner_;
  std::string death_cause_;
  std::string crash_cause_;
  GameMode mode_ = GameMode::kDocked;
  FlightMode flight_mode_ = FlightMode::kClassic;
  bool assist_ = true;
  bool autopilot_ = false;
  double throttle_ = 0.0;
  EntityId target_ = kNoEntity;
  EntityId lock_ = kNoEntity;
  int selected_system_ = -1;
  double jump_timer_ = -1.0;
  bool galactic_jump_ = false;
  /// Seconds left of the zoom back out after a jump, or negative
  double arrival_clock_ = -1.0;
  /// No pirates, hunters or aliens until the commander has docked once
  bool first_flight_ = false;
  /// Seconds since the ship started out of the bay, or negative when not launching
  double launch_clock_ = -1.0;
  /// Seconds since the tractor beam took hold, or negative when not landing, and where the ship was at that moment
  double landing_clock_ = -1.0;
  Vec3d landing_from_;
  Vec3d landing_velocity_;
  Quat landing_facing_;
  double accumulator_ = 0.0;
  std::bitset<kActionCount> pending_;
  bool station_angered_ = false;
  std::string last_rank_;
  std::vector<std::string> messages_;
  std::vector<Event> frame_events_;
};

}  // namespace coriolis

#endif  // CORIOLIS_GAME_H_

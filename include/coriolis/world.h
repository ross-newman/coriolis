#ifndef CORIOLIS_WORLD_H_
#define CORIOLIS_WORLD_H_

#include <cstdint>
#include <list>
#include <string>
#include <vector>

#include "coriolis/blueprint.h"
#include "coriolis/math.h"
#include "coriolis/random.h"

namespace coriolis {

/// The simulation: everything that moves, fights and is hit in space around the player. It runs headless, so it can be
/// tested and replayed, and a game, a viewer or a server can sit on top of it. Units are metres and seconds.

using EntityId = std::uint32_t;
constexpr EntityId kNoEntity = 0;

enum class EntityKind { kShip, kStation, kMissile, kCanister, kAsteroid, kEscapePod, kPlanet, kSun };

/// Who an entity fights for
enum class Team { kNeutral, kPlayer, kPolice, kPirate, kTrader, kAlien, kHunter, kStation };

enum class FlightMode {
  /// Pitch and roll set turning rates and the ship always flies where its nose points, as in the original game
  kClassic,
  /// The ship has momentum. Flight assist (on by default) uses the thrusters to hold course and speed; with it off the
  /// ship drifts and turns until the pilot stops it.
  kNewtonian
};

/// What a pilot, human or AI, asks the ship to do
struct ShipControls {
  /// -1 to 1: positive pitches the nose up, rolls the right wing down and yaws the nose right
  double pitch = 0.0;
  double roll = 0.0;
  double yaw = 0.0;
  /// 0 to 1: the speed wanted, as a fraction of top speed (classic flight and Newtonian flight with assist)
  double throttle = 0.0;
  /// -1 to 1: thrust along the nose (Newtonian flight with assist off)
  double thrust = 0.0;
  bool fire = false;
  bool assist = true;
};

/// What an AI pilot is doing
enum class AiMode { kCruise, kApproach, kAttack, kBreak, kFlee };

/// The thinking of an AI pilot. See ai.h.
struct AiBrain {
  bool started = false;
  AiMode mode = AiMode::kCruise;
  double timer = 0.0;
  double think = 0.0;
  Vec3d waypoint;
  Vec3d break_direction{0.0, 1.0, 0.0};
  EntityId target = kNoEntity;
  double missile_cooldown = 0.0;
  double drone_cooldown = 0.0;
  int drones_left = 0;
  double aim_wobble = 0.0;
  /// The kind of thing the pilot is up to (see AiProfile in ai.h)
  const struct AiProfile* profile = nullptr;
};

struct Entity {
  EntityId id = kNoEntity;
  EntityKind kind = EntityKind::kShip;
  Team team = Team::kNeutral;
  /// The shape and stats, or nullptr for a planet or sun
  const Blueprint* blueprint = nullptr;
  bool is_player = false;
  /// The controls are set from outside (by a test or a script), so the AI leaves them alone
  bool manual = false;
  /// Takes no damage and cannot crash (for tests, demos and attract modes)
  bool invulnerable = false;
  bool alive = true;
  /// Gone without an explosion (docked, jumped away, scooped)
  bool silent_removal = false;
  /// Passes through solid things: no bumps and no docking. Used while a ship rides out of the bay.
  bool ghost = false;

  Vec3d position;
  Vec3d velocity;
  Quat orientation;
  /// Turn rates about the ship's own axes in radians per second (pitch, roll, yaw), for Newtonian flight
  Vec3d spin;

  /// Radius of the whole thing in metres, and the radius that counts for hits and collisions
  double radius = 1.0;
  double hit_radius = 1.0;

  FlightMode mode = FlightMode::kClassic;
  ShipControls controls;

  double hull = 100.0;
  double hull_max = 100.0;
  double shield = 0.0;
  double shield_max = 0.0;
  double energy = 100.0;
  double energy_max = 100.0;
  /// Health of each part in the blueprint's subsystem list
  std::vector<double> subsystem_health;

  int missiles = 0;
  bool has_ecm = false;
  bool can_scoop = false;
  double laser_damage = 15.0;
  double weapon_cooldown = 0.0;
  std::uint32_t laser_index = 0;

  /// For a missile, who fired it and what it hunts. For anything else, who hurt it last.
  EntityId owner = kNoEntity;
  EntityId target = kNoEntity;
  EntityId last_attacker = kNoEntity;

  /// Seconds left to live, negative for ever
  double life = -1.0;
  double age = 0.0;
  /// What a cargo canister holds (an index into the commodity list)
  int cargo_item = 0;
  /// A station that has been attacked sends out the police
  bool hostile = false;
  double launch_timer = 0.0;
  /// Set by the spawner to say the ship is part of a pack
  int group = 0;

  AiBrain ai;

  double Speed() const { return Length(velocity); }
  bool IsShipLike() const { return kind == EntityKind::kShip || kind == EntityKind::kEscapePod; }
  const BlueprintStats& stats() const { return blueprint->stats; }
  /// The index of a subsystem by name, or -1
  int SubsystemIndex(const std::string& name) const;
  bool SubsystemDown(const std::string& name) const;
};

/// A laser bolt
struct Projectile {
  Vec3d position;
  Vec3d velocity;
  EntityId owner = kNoEntity;
  double damage = 15.0;
  double life = 2.0;
};

enum class EventKind {
  kLaserFired,
  kShieldHit,
  kHullHit,
  kSubsystemDestroyed,
  kDestroyed,
  kMissileLaunched,
  kMissileDetonated,
  kEcm,
  kEnergyBomb,
  kDocked,
  kCollision,
  kScooped,
  kCrashed
};

/// Something that happened, for sound, rumble, messages and scoring
struct Event {
  EventKind kind = EventKind::kLaserFired;
  /// Usually the one it happened to (or the shooter for kLaserFired) and the one that did it
  EntityId a = kNoEntity;
  EntityId b = kNoEntity;
  Vec3d position;
  /// Damage done, or for kScooped the cargo item
  double amount = 0.0;
  /// What the victim was, for kDestroyed (the victim is gone by the time the event is read)
  const Blueprint* blueprint = nullptr;
  Team team = Team::kNeutral;
  EntityKind entity_kind = EntityKind::kShip;
};

/// Where a station's docking bay is, which way it faces, and how close a ship has to come to be captured by it
Vec3d DockPoint(const Entity& station);
Vec3d DockAxis(const Entity& station);
double DockRadius(const Entity& station);
/// Can the ship dock? With `strict` the ship must be pointing into the bay, the right way up, and slow enough.
bool CanDock(const Entity& ship, const Entity& station, bool strict);

/// The inside of a station's bay: a tunnel running from the mouth (at the dock point, in the face of the station) back
/// into the station. `axis` points out of the mouth, `up` is the station's up, and `right` completes the set. A ship
/// launches from the back of the bay and rides out along the axis.
struct Bay {
  Vec3d mouth;
  Vec3d axis;
  Vec3d up;
  Vec3d right;
  double half_width = 0.0;
  double half_height = 0.0;
  /// How far the tunnel runs back from the mouth
  double depth = 0.0;
};
Bay BayOf(const Entity& station);

class World {
 public:
  explicit World(std::uint64_t seed = 1);

  /// Add a ship, station, missile, canister or asteroid made from a blueprint. The blueprint has to outlive the world.
  EntityId SpawnShip(const Blueprint& blueprint, Team team, const Vec3d& position, const Quat& orientation);
  /// Add a planet or sun
  EntityId SpawnBody(EntityKind kind, double radius, const Vec3d& position);

  /// The entity with the id, or nullptr. The pointer stays good for as long as the entity is in the world, however many
  /// others are added or removed, but hold the id between frames: a destroyed or removed entity is gone.
  Entity* Find(EntityId id);
  const Entity* Find(EntityId id) const;
  std::list<Entity>& entities() { return entities_; }
  const std::list<Entity>& entities() const { return entities_; }
  const std::vector<Projectile>& projectiles() const { return projectiles_; }

  /// The first entity of a kind, or nullptr
  Entity* FindKind(EntityKind kind);
  const Entity* FindKind(EntityKind kind) const;

  /// Advance by dt seconds
  void Step(double dt);

  /// Remove everything
  void Clear();

  /// Fire a missile from `shooter` at `target`. False if it has none or cannot.
  bool FireMissile(EntityId shooter, EntityId target);
  /// Jam the missiles locked on to the ship. False if it has no ECM or too little energy.
  bool UseEcm(EntityId ship);
  /// Destroy everything about that is not a station or a body. False if there is no such ship.
  bool DetonateEnergyBomb(EntityId ship);

  /// Hurt something: shield first, then hull, and the part near the hit
  void ApplyDamage(Entity& victim, double amount, EntityId attacker, const Vec3d& hit_point);

  /// Take the events since the last call
  std::vector<Event> TakeEvents();
  void Emit(const Event& event) { events_.push_back(event); }

  EntityId player() const { return player_; }
  void SetPlayer(EntityId id);
  /// Whether the police hunt the player
  bool player_wanted() const { return player_wanted_; }
  void set_player_wanted(bool wanted) { player_wanted_ = wanted; }

  Random& rng() { return rng_; }
  double time() const { return time_; }

  /// The number of live ships (not stations, rocks or bodies)
  std::size_t CountShips() const;

  /// Remove ships more than `distance` from the player (they have left the neighbourhood)
  void Cull(double distance);

 private:
  void StepAi(double dt);
  void StepMotion(Entity& e, double dt);
  void StepWeapons(Entity& e, double dt);
  void StepProjectiles(double dt);
  void StepContacts(double dt);
  void StepStation(Entity& e, double dt);
  void StepMissile(Entity& e, double dt);
  void Destroy(Entity& e);
  void Detonate(Entity& missile);
  EntityId Add(Entity e);

  // A list, so that a pointer from Find() stays good for as long as the entity exists, however many are added or
  // removed around it. Hold ids, not pointers, across frames: an entity that is destroyed or removed is gone.
  std::list<Entity> entities_;
  std::list<Entity> pending_;
  std::vector<Projectile> projectiles_;
  std::vector<Event> events_;
  Random rng_;
  double time_ = 0.0;
  EntityId next_id_ = 1;
  EntityId player_ = kNoEntity;
  bool player_wanted_ = false;
  bool stepping_ = false;
};

/// The radius of a blueprint's shape in metres
double BoundingRadius(const Blueprint& blueprint);

}  // namespace coriolis

#endif  // CORIOLIS_WORLD_H_

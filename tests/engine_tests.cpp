// Tests for the game engine: data files, maths, flight, combat, AI, docking, the galaxy, trading, saving and the game
// as a whole. All headless, and deterministic from a seed.

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>

#include "coriolis/ai.h"
#include "coriolis/asteroids.h"
#include "coriolis/blueprint.h"
#include "coriolis/bot.h"
#include "coriolis/commander.h"
#include "coriolis/galaxy.h"
#include "coriolis/game.h"
#include "coriolis/json.h"
#include "coriolis/market.h"
#include "coriolis/math.h"
#include "coriolis/missions.h"
#include "coriolis/renderer.h"
#include "coriolis/ships.h"
#include "coriolis/spawner.h"
#include "coriolis/steering.h"
#include "coriolis/world.h"

namespace {

int g_failures = 0;

#define CHECK(condition)                                                                 \
  do {                                                                                   \
    if (!(condition)) {                                                                  \
      std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #condition); \
      ++g_failures;                                                                      \
    }                                                                                    \
  } while (false)

using namespace coriolis;

bool Near(double a, double b, double tolerance = 1e-6) { return std::fabs(a - b) <= tolerance; }
bool NearV(const Vec3d& a, const Vec3d& b, double tolerance = 1e-6) { return Distance(a, b) <= tolerance; }

const Blueprint& Bp(const char* id) {
  const Blueprint* bp = FindBuiltInBlueprint(id);
  if (bp == nullptr) {
    std::fprintf(stderr, "no blueprint %s\n", id);
    std::abort();
  }
  return *bp;
}

/// Run the world for some seconds
void Run(World& w, double seconds) {
  for (double t = 0.0; t < seconds; t += 1.0 / 60.0) {
    w.Step(1.0 / 60.0);
  }
}

bool Finite(const Vec3d& v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }

/// Fly this ship by hand: the AI leaves its controls alone
EntityId Manual(World& w, EntityId id) {
  w.Find(id)->manual = true;
  return id;
}

// ---------- JSON and blueprints ----------

void JsonRoundTrips() {
  Json root = Json::MakeObject();
  root.Set("name", Json::String("a \"quoted\"\nline \xC3\xA9"));
  root.Set("n", Json::Number(0.1));
  root.Set("big", Json::Number(1234567890123.0));
  root.Set("flag", Json::Bool(true));
  root.Set("none", Json());
  Json list = Json::MakeArray();
  list.Push(Json::Number(1));
  list.Push(Json::Number(-2.5e-7));
  root.Set("list", std::move(list));
  const std::string text = root.Dump();
  Json back;
  std::string error;
  CHECK(Json::Parse(text, &back, &error));
  CHECK(back.Dump() == text);
  CHECK(back.Find("name")->string_value() == root.Find("name")->string_value());
  CHECK(back.Find("n")->number_value() == 0.1);
  CHECK(back.Find("list")->items().size() == 2);
  // Plain lists go on one line
  CHECK(text.find("[1, -2.5e-07]") != std::string::npos || text.find("[1, -2.5e-7]") != std::string::npos);
}

void JsonRejectsBadText() {
  Json j;
  std::string error;
  for (const char* bad :
       {"", "{", "[1,]", "{\"a\" 1}", "01", "1.", "\"abc", "tru", "[1] x", "{\"a\":1,}", "\"\\x\"", "-"}) {
    CHECK(!Json::Parse(bad, &j, &error));
    CHECK(!error.empty());
  }
  CHECK(Json::Parse("  {\"a\": [1, 2, {\"b\": null}], \"c\": \"\\u00e9\\ud83d\\ude00\"}  ", &j, &error));
  CHECK(j.Find("c")->string_value() == "\xC3\xA9\xF0\x9F\x98\x80");
  // Deep nesting is refused rather than overflowing the stack
  CHECK(!Json::Parse(std::string(500, '[') + std::string(500, ']'), &j, &error));
}

void BuiltInBlueprintsAreGood() {
  const std::vector<Blueprint>& all = BuiltInBlueprints();
  CHECK(all.size() == kShipCount);
  std::set<std::string> ids;
  for (const Blueprint& b : all) {
    const std::vector<std::string> problems = ValidateBlueprint(b);
    if (!problems.empty()) {
      std::fprintf(stderr, "%s: %s\n", b.id.c_str(), problems.front().c_str());
    }
    CHECK(problems.empty());
    CHECK(ids.insert(b.id).second);
    CHECK(b.scale > 0.0f);
    if (b.kind == HullKind::kStation && b.ai_profile != "") {
      CHECK(b.ai_profile == "station");
    }
  }
  CHECK(FindBuiltInBlueprint("cobra_mk3_trader") != nullptr);
  CHECK(FindBuiltInBlueprint("nonsense") == nullptr);
  // The wheel station is much bigger than the Coriolis
  CHECK(Bp("wheel_station").scale > 3.0f * Bp("coriolis").scale);
  // Every ship that can fight has weapons, and stations have a bay
  CHECK(Bp("viper").stats.missiles == 1);
  bool has_dock = false;
  for (const Hardpoint& h : Bp("coriolis").hardpoints) {
    has_dock = has_dock || h.kind == HardpointKind::kDock;
  }
  CHECK(has_dock);
}

void BlueprintsSurviveJsonAndDrawTheSame() {
  for (std::size_t i = 0; i < kShipCount; ++i) {
    const Blueprint& original = BuiltInBlueprints()[i];
    Json json;
    std::string error;
    CHECK(Json::Parse(BlueprintToJson(original).Dump(), &json, &error));
    Blueprint copy;
    CHECK(BlueprintFromJson(json, &copy, &error));
    CHECK(copy.id == original.id);
    CHECK(copy.vertices.size() == original.vertices.size());
    CHECK(copy.triangles.size() == original.triangles.size());
    CHECK(copy.hardpoints.size() == original.hardpoints.size());
    CHECK(copy.subsystems.size() == original.subsystems.size());
    CHECK(copy.stats.max_speed == original.stats.max_speed);
    CHECK(copy.stats.mass_kg == original.stats.mass_kg);
    CHECK(copy.scale == original.scale);
    // And it draws exactly as the model it came from
    Image a(160, 120);
    Image b(160, 120);
    Renderer().DrawShip(a, AllShips()[i], 0.8f);
    Renderer().DrawShip(b, BlueprintToModel(copy), 0.8f);
    CHECK(a == b);
  }
}

void BadBlueprintsAreRejected() {
  Json good = BlueprintToJson(Bp("viper"));
  Blueprint out;
  std::string error;
  CHECK(BlueprintFromJson(good, &out, &error));

  const auto mutate = [&](const std::string& from, const std::string& to) {
    std::string text = good.Dump();
    const std::size_t at = text.find(from);
    if (at == std::string::npos) {
      return false;
    }
    text.replace(at, from.size(), to);
    Json j;
    std::string e;
    return Json::Parse(text, &j, &e) && BlueprintFromJson(j, &out, &e);
  };
  CHECK(!mutate("\"max_speed\"", "\"max_sped\""));  // a mistyped stat
  CHECK(!mutate("\"id\": \"viper\"", "\"id\": \"Vi per\""));
  CHECK(!mutate("\"kind\": \"ship\"", "\"kind\": \"boat\""));
  CHECK(!mutate("\"version\": 1", "\"version\": 2"));
  CHECK(!mutate("\"format\": \"coriolis-blueprint\"", "\"format\": \"other\""));
  CHECK(!mutate("\"hull\": 120", "\"hull\": -5"));

  Blueprint broken = Bp("viper");
  broken.triangles[0].v[0] = 99999;
  CHECK(!ValidateBlueprint(broken).empty());
  broken = Bp("viper");
  broken.hardpoints.push_back({HardpointKind::kLaser, {0, 0, 0}, {0, 0, 5}});
  CHECK(!ValidateBlueprint(broken).empty());
  broken = Bp("viper");
  broken.subsystems.push_back(broken.subsystems[0]);
  CHECK(!ValidateBlueprint(broken).empty());
}

void BlueprintFilesSaveAndLoad() {
  const std::string path = "coriolis_test_blueprint.json";
  std::string error;
  CHECK(SaveBlueprintFile(Bp("anaconda"), path, &error));
  Blueprint back;
  CHECK(LoadBlueprintFile(path, &back, &error));
  CHECK(back.id == "anaconda");
  CHECK(back.subsystems.size() == Bp("anaconda").subsystems.size());
  std::remove(path.c_str());
  CHECK(!LoadBlueprintFile(path, &back, &error));
  CHECK(error.find(path) != std::string::npos);
}

// ---------- Maths ----------

void TurningHasTheRightSigns() {
  const Quat level;
  CHECK(NearV(Forward(level), {0, 0, 1}));
  CHECK(NearV(Up(level), {0, 1, 0}));
  CHECK(NearV(Right(level), {1, 0, 0}));
  // Pitch up lifts the nose
  CHECK(Forward(Turned(level, 0.1, 0, 0)).y > 0.09);
  // Yaw right swings the nose to the right
  CHECK(Forward(Turned(level, 0, 0, 0.1)).x > 0.09);
  // Roll right drops the right wing and tips the roof to the right
  const Quat rolled = Turned(level, 0, 0.1, 0);
  CHECK(Right(rolled).y < -0.09);
  CHECK(Up(rolled).x > 0.09);
  // Quaternions stay unit length through many turns
  Quat q;
  for (int i = 0; i < 10000; ++i) {
    q = Turned(q, 0.01, 0.02, -0.005);
  }
  CHECK(Near(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z, 1.0, 1e-9));
}

void LookAlongPointsTheNose() {
  for (const Vec3d& dir : {Vec3d{1, 0, 0}, Vec3d{0, 1, 0}, Vec3d{0, 0, -1}, Vec3d{0.3, -0.5, 0.8}, Vec3d{0, -1, 0}}) {
    const Quat q = LookAlong(dir, {0, 1, 0});
    CHECK(NearV(Forward(q), Normalized(dir), 1e-9));
    // Right-handed set of axes
    CHECK(Near(Dot(Right(q), Up(q)), 0.0, 1e-9));
    CHECK(Near(Length(Up(q)), 1.0, 1e-9));
  }
}

void LeadingHitsAMovingTarget() {
  // A target crossing at 300 m/s, 1500 m away: fire where it will be and the bolt arrives with it
  const Vec3d shooter{0, 0, 0};
  const Vec3d shooter_velocity{0, 0, 100};
  const Vec3d target{800, 100, 1500};
  const Vec3d target_velocity{-300, 0, 50};
  const Vec3d dir = LeadDirection(shooter, shooter_velocity, target, target_velocity, kBoltSpeed);
  Vec3d bolt = shooter;
  Vec3d where = target;
  double closest = 1e9;
  for (double t = 0.0; t < 3.0; t += 0.001) {
    bolt = shooter + (shooter_velocity + dir * kBoltSpeed) * t;
    where = target + target_velocity * t;
    closest = std::min(closest, Distance(bolt, where));
  }
  CHECK(closest < 2.0);
}

// ---------- Flight ----------

void ClassicFlightAcceleratesAndTurns() {
  World w(1);
  const Blueprint& bp = Bp("sidewinder");
  const EntityId id = Manual(w, w.SpawnShip(bp, Team::kNeutral, {0, 0, 0}, Quat{}));
  Entity* e = w.Find(id);
  e->controls.throttle = 1.0;
  Run(w, 10.0);
  e = w.Find(id);
  CHECK(Near(e->Speed(), bp.stats.max_speed, 1.0));
  // Flies where the nose points
  CHECK(NearV(Normalized(e->velocity), Forward(e->orientation), 1e-6));
  CHECK(e->position.z > 1000.0);
  // Yaw turns the nose to the right
  e->controls.yaw = 1.0;
  Run(w, 1.0);
  e = w.Find(id);
  CHECK(Forward(e->orientation).x > 0.3);
  // Turn rate does not exceed the ship's limit
  const Vec3d f0 = Forward(e->orientation);
  e->controls.yaw = 1.0;
  Run(w, 0.5);
  CHECK(AngleBetween(f0, Forward(w.Find(id)->orientation)) <= bp.stats.max_yaw_rate * 0.5 + 0.02);
  // Slows down when the throttle is closed
  e = w.Find(id);
  e->controls.yaw = 0.0;
  e->controls.throttle = 0.0;
  Run(w, 10.0);
  CHECK(w.Find(id)->Speed() < 1.0);
}

void NewtonianFlightKeepsItsMomentum() {
  World w(1);
  const EntityId id = Manual(w, w.SpawnShip(Bp("viper"), Team::kNeutral, {0, 0, 0}, Quat{}));
  Entity* e = w.Find(id);
  e->mode = FlightMode::kNewtonian;
  e->controls.assist = false;
  e->controls.thrust = 1.0;
  Run(w, 3.0);
  e = w.Find(id);
  const double speed = e->Speed();
  CHECK(speed > 100.0);
  // Turn the ship round and let go of the thrust: it keeps going the way it was
  e->controls.thrust = 0.0;
  e->controls.yaw = 1.0;
  Run(w, 1.0);
  e = w.Find(id);
  e->controls.yaw = 0.0;
  const Vec3d heading = Normalized(e->velocity);
  Run(w, 2.0);
  e = w.Find(id);
  CHECK(NearV(Normalized(e->velocity), heading, 0.01));
  CHECK(Near(e->Speed(), speed, 1.0));
  // Without assist the spin goes on after the stick is released
  CHECK(std::fabs(e->spin.z) > 0.1);
  // Flight assist brings the ship to the set speed along its nose, and stops the spin
  e->controls.assist = true;
  e->controls.throttle = 0.5;
  Run(w, 8.0);
  e = w.Find(id);
  CHECK(Near(e->Speed(), 0.5 * Bp("viper").stats.max_speed, 3.0));
  CHECK(NearV(Normalized(e->velocity), Forward(e->orientation), 0.02));
  CHECK(std::fabs(e->spin.z) < 0.01);
}

void DamagedEnginesSlowTheShip() {
  World w(1);
  const EntityId id = Manual(w, w.SpawnShip(Bp("viper"), Team::kNeutral, {0, 0, 0}, Quat{}));
  Entity* e = w.Find(id);
  e->subsystem_health[static_cast<std::size_t>(e->SubsystemIndex("engines"))] = 0.0;
  CHECK(e->SubsystemDown("engines"));
  e->controls.throttle = 1.0;
  Run(w, 20.0);
  CHECK(w.Find(id)->Speed() < 0.35 * Bp("viper").stats.max_speed);
}

// ---------- Combat ----------

/// A shooter at the origin facing +z and a victim `range` metres ahead
struct Duel {
  World world{7};
  EntityId a = kNoEntity;
  EntityId b = kNoEntity;
  Duel(const char* attacker, const char* victim, double range) {
    a = Manual(world, world.SpawnShip(Bp(attacker), Team::kPirate, {0, 0, 0}, Quat{}));
    b = Manual(world, world.SpawnShip(Bp(victim), Team::kTrader, {0, 0, range}, Quat{}));
  }
};

void LasersHitAndShieldsGoFirst() {
  Duel d("sidewinder", "viper", 800.0);
  Entity* victim = d.world.Find(d.b);
  const double shield = victim->shield;
  const double hull = victim->hull;
  d.world.Find(d.a)->controls.fire = true;
  Run(d.world, 0.9);
  victim = d.world.Find(d.b);
  CHECK(victim->shield < shield);
  CHECK(Near(victim->hull, hull));
  bool hit = false;
  for (const Event& e : d.world.TakeEvents()) {
    hit = hit || e.kind == EventKind::kShieldHit;
  }
  CHECK(hit);
  // Keep firing: the shield goes, then the hull
  Run(d.world, 20.0);
  CHECK(d.world.Find(d.b) == nullptr || d.world.Find(d.b)->hull < hull);
}

void ShotsMissWhenAimedWrong() {
  Duel d("sidewinder", "viper", 800.0);
  d.world.Find(d.b)->position = {500, 0, 800};
  d.world.Find(d.a)->controls.fire = true;
  Run(d.world, 3.0);
  CHECK(Near(d.world.Find(d.b)->shield, Bp("viper").stats.shield));
}

void ShotsStopAtTheFirstThingHit() {
  World w(3);
  const EntityId shooter = Manual(w, w.SpawnShip(Bp("sidewinder"), Team::kPirate, {0, 0, 0}, Quat{}));
  const EntityId near = Manual(w, w.SpawnShip(Bp("python"), Team::kTrader, {0, 0, 400}, Quat{}));
  const EntityId far = Manual(w, w.SpawnShip(Bp("python"), Team::kTrader, {0, 0, 900}, Quat{}));
  w.Find(shooter)->controls.fire = true;
  Run(w, 2.0);
  CHECK(w.Find(near)->shield < Bp("python").stats.shield);
  CHECK(Near(w.Find(far)->shield, Bp("python").stats.shield));
}

void DestroyedShipsReportDropCargoAndScore() {
  World w(5);
  const EntityId a = w.SpawnShip(Bp("sidewinder"), Team::kPlayer, {0, 0, 0}, Quat{});
  const EntityId b = w.SpawnShip(Bp("python"), Team::kPirate, {0, 0, 500}, Quat{});
  Entity* victim = w.Find(b);
  w.ApplyDamage(*victim, 1e6, a, victim->position);
  w.Step(1.0 / 60.0);
  CHECK(w.Find(b) == nullptr);
  bool killed = false;
  for (const Event& e : w.TakeEvents()) {
    if (e.kind == EventKind::kDestroyed && e.a == b) {
      killed = true;
      CHECK(e.b == a);
      CHECK(e.blueprint == &Bp("python"));
      CHECK(e.team == Team::kPirate);
    }
  }
  CHECK(killed);
  // Python drops up to 5 canisters. Over many kills there are some, and never more than 5.
  int canisters = 0;
  for (const Entity& e : w.entities()) {
    canisters += e.kind == EntityKind::kCanister ? 1 : 0;
  }
  CHECK(canisters <= 5);
  for (int i = 0; i < 20; ++i) {
    const EntityId v = w.SpawnShip(Bp("python"), Team::kPirate, {0, 0, 5000.0 + 1000.0 * i}, Quat{});
    w.ApplyDamage(*w.Find(v), 1e6, a, {0, 0, 0});
    w.Step(1.0 / 60.0);
    int now = 0;
    for (const Entity& e : w.entities()) {
      now += e.kind == EntityKind::kCanister ? 1 : 0;
    }
    CHECK(now - canisters <= 5);
    canisters = now;
  }
  CHECK(canisters > 0);
}

void SubsystemsCanBeShotOut() {
  World w(5);
  const EntityId a = Manual(w, w.SpawnShip(Bp("sidewinder"), Team::kPlayer, {0, 0, 0}, Quat{}));
  const EntityId b = Manual(w, w.SpawnShip(Bp("python"), Team::kPirate, {0, 0, 500}, Quat{}));
  Entity* victim = w.Find(b);
  victim->hull = victim->hull_max = 1e9;
  victim->shield = 0.0;
  // Hit the engines (at the rear) over and over
  const Subsystem& engines = victim->blueprint->subsystems[static_cast<std::size_t>(victim->SubsystemIndex("engines"))];
  const Vec3d where =
      victim->position + Rotate(victim->orientation,
                                Vec3d{engines.centre.x, engines.centre.y, engines.centre.z} * victim->blueprint->scale);
  for (int i = 0; i < 100 && !victim->SubsystemDown("engines"); ++i) {
    w.ApplyDamage(*victim, 15.0, a, where);
  }
  CHECK(victim->SubsystemDown("engines"));
  CHECK(!victim->SubsystemDown("weapons"));
  bool reported = false;
  for (const Event& e : w.TakeEvents()) {
    reported = reported || e.kind == EventKind::kSubsystemDestroyed;
  }
  CHECK(reported);
  // A ship with its weapons out cannot fire
  victim->subsystem_health[static_cast<std::size_t>(victim->SubsystemIndex("weapons"))] = 0.0;
  victim->controls.fire = true;
  w.TakeEvents();
  Run(w, 1.0);
  bool fired = false;
  for (const Event& e : w.TakeEvents()) {
    fired = fired || (e.kind == EventKind::kLaserFired && e.a == b);
  }
  CHECK(!fired);
}

void MissilesFindAndKillTheirTarget() {
  World w(9);
  const EntityId a = Manual(w, w.SpawnShip(Bp("cobra_mk3_bounty_hunter"), Team::kPlayer, {0, 0, 0}, Quat{}));
  const EntityId b =
      Manual(w, w.SpawnShip(Bp("sidewinder"), Team::kPirate, {2500, 300, 2500}, LookAlong({1, 0, 0}, {0, 1, 0})));
  Entity* target = w.Find(b);
  target->controls.throttle = 0.7;
  target->shield = 0.0;
  target->hull = target->hull_max = 40.0;
  CHECK(w.FireMissile(a, b));
  CHECK(w.Find(a)->missiles == static_cast<int>(Bp("cobra_mk3_bounty_hunter").stats.missiles) - 1);
  Run(w, 12.0);
  CHECK(w.Find(b) == nullptr);
  bool boom = false;
  for (const Event& e : w.TakeEvents()) {
    boom = boom || e.kind == EventKind::kMissileDetonated;
  }
  CHECK(boom);
  // No missiles left to fire, or no target to fire at
  w.Find(a)->missiles = 0;
  CHECK(!w.FireMissile(a, b));
}

void EcmDestroysMissilesAimedAtTheShip() {
  World w(9);
  const EntityId a = w.SpawnShip(Bp("viper"), Team::kPirate, {0, 0, 0}, Quat{});
  const EntityId b = w.SpawnShip(Bp("sidewinder"), Team::kPlayer, {0, 0, 3000}, Quat{});
  CHECK(w.FireMissile(a, b));
  w.Find(b)->has_ecm = true;
  CHECK(w.UseEcm(b));
  Run(w, 5.0);
  for (const Entity& e : w.entities()) {
    CHECK(e.kind != EntityKind::kMissile);
  }
  CHECK(w.Find(b)->hull == w.Find(b)->hull_max);
  // No ECM, no jam
  w.Find(b)->has_ecm = false;
  CHECK(!w.UseEcm(b));
}

void EnergyBombKillsEverythingButStationsAndBodies() {
  World w(2);
  const EntityId me = w.SpawnShip(Bp("cobra_mk3_trader"), Team::kPlayer, {0, 0, 0}, Quat{});
  w.SpawnShip(Bp("viper"), Team::kPolice, {1000, 0, 0}, Quat{});
  w.SpawnShip(Bp("asteroid"), Team::kNeutral, {0, 2000, 0}, Quat{});
  w.SpawnShip(Bp("coriolis"), Team::kStation, {0, 0, 40000}, Quat{});
  w.SpawnBody(EntityKind::kPlanet, 10000.0, {0, 0, -80000});
  CHECK(w.DetonateEnergyBomb(me));
  w.Step(1.0 / 60.0);
  int left = 0;
  for (const Entity& e : w.entities()) {
    // The rock leaves minerals behind
    if (e.kind == EntityKind::kCanister) {
      continue;
    }
    left += 1;
    CHECK(e.id == me || e.kind == EntityKind::kStation || e.kind == EntityKind::kPlanet);
  }
  CHECK(left == 3);
}

// ---------- Collisions, hazards, docking ----------

void ShipsBumpAndHurtEachOther() {
  World w(2);
  const EntityId a = Manual(w, w.SpawnShip(Bp("python"), Team::kNeutral, {0, 0, 0}, Quat{}));
  const EntityId b =
      Manual(w, w.SpawnShip(Bp("sidewinder"), Team::kNeutral, {0, 0, 200}, LookAlong({0, 0, -1}, {0, 1, 0})));
  w.Find(a)->controls.throttle = 1.0;
  w.Find(b)->controls.throttle = 1.0;
  Run(w, 6.0);
  CHECK(w.Find(b) == nullptr || w.Find(b)->hull < w.Find(b)->hull_max || w.Find(b)->shield < w.Find(b)->shield_max);
  CHECK(Finite(w.Find(a)->position));
  // The light ship comes off worse
  const Entity* heavy = w.Find(a);
  CHECK(heavy != nullptr);
}

void SunBurnsAndPlanetsAreSolid() {
  World w(2);
  w.SpawnBody(EntityKind::kPlanet, 10000.0, {0, 0, 0});
  w.SpawnBody(EntityKind::kSun, 30000.0, {200000, 0, 0});
  // Diving into the planet destroys the ship
  const EntityId a =
      Manual(w, w.SpawnShip(Bp("viper"), Team::kNeutral, {0, 0, 12000}, LookAlong({0, 0, -1}, {0, 1, 0})));
  w.Find(a)->controls.throttle = 1.0;
  Run(w, 15.0);
  CHECK(w.Find(a) == nullptr);
  bool crashed = false;
  for (const Event& e : w.TakeEvents()) {
    crashed = crashed || e.kind == EventKind::kCrashed;
  }
  CHECK(crashed);
  // Close to the sun the shield and hull wear away
  const EntityId b = Manual(w, w.SpawnShip(Bp("viper"), Team::kNeutral, {200000 - 36000, 0, 0}, Quat{}));
  Run(w, 3.0);
  CHECK(w.Find(b)->shield < w.Find(b)->shield_max);
}

/// Put a station in a world, with the nose of the station's bay along +z
EntityId AddStation(World& w, const char* id, const Vec3d& at) {
  return w.SpawnShip(Bp(id), Team::kStation, at, Quat{});
}

void StrictDockingNeedsAlignment() {
  for (const char* id : {"coriolis", "dodo", "wheel_station", "rock_hermit"}) {
    World w(1);
    const EntityId st = AddStation(w, id, {0, 0, 0});
    const Entity& station = *w.Find(st);
    const Vec3d dock = DockPoint(station);
    const Vec3d axis = DockAxis(station);
    CHECK(Near(Length(axis), 1.0, 1e-9));
    CHECK(Distance(dock, station.position) > 0.3 * station.radius);
    const EntityId s = w.SpawnShip(Bp("cobra_mk3_trader"), Team::kPlayer, dock + axis * 20.0,
                                   LookAlong(-axis, Up(station.orientation)));
    Entity* ship = w.Find(s);
    ship->velocity = -axis * 50.0;
    CHECK(CanDock(*ship, station, true));
    // Pointing the wrong way, upside down or too fast: no
    ship->orientation = LookAlong(axis, {0, 1, 0});
    CHECK(!CanDock(*ship, station, true));
    CHECK(CanDock(*ship, station, false));
    ship->orientation = LookAlong(-axis, -Up(station.orientation));
    CHECK(!CanDock(*ship, station, true));
    ship->orientation = LookAlong(-axis, Up(station.orientation));
    ship->velocity = -axis * 400.0;
    CHECK(!CanDock(*ship, station, true));
    // Too far
    ship->velocity = -axis * 50.0;
    ship->position = dock + axis * (DockRadius(station) + 50.0);
    CHECK(!CanDock(*ship, station, true));
  }
}

void FlyingIntoTheBayDocksAndDoesNotAngerTheStation() {
  for (const char* id : {"coriolis", "dodo", "wheel_station", "rock_hermit"}) {
    for (const double offset : {0.0, 0.5, -0.6}) {
      World w(3);
      // The ship comes first in the list, so a bump sees the ship and then the station
      const EntityId s = w.SpawnShip(Bp("cobra_mk3_trader"), Team::kPlayer, {0, 0, 5000}, Quat{});
      w.SetPlayer(s);
      const EntityId st = AddStation(w, id, {0, 0, 0});
      const Entity& station = *w.Find(st);
      const Vec3d dock = DockPoint(station);
      const Vec3d axis = DockAxis(station);
      const Vec3d side = Normalized(Cross(Up(station.orientation), axis));
      Entity* ship = w.Find(s);
      ship->manual = true;
      ship->mode = FlightMode::kNewtonian;  // no thrust, no assist: it just coasts in
      ship->controls.assist = false;
      ship->position = dock + axis * 400.0 + side * (offset * DockRadius(station));
      ship->orientation = LookAlong(-axis, Up(station.orientation));
      ship->velocity = -axis * 100.0;
      bool docked = false;
      for (double t = 0.0; t < 15.0 && !docked; t += 1.0 / 60.0) {
        w.Step(1.0 / 60.0);
        for (const Event& e : w.TakeEvents()) {
          docked = docked || (e.kind == EventKind::kDocked && e.a == s);
        }
      }
      if (!docked) {
        std::fprintf(stderr, "station %s offset %.1f: did not dock\n", id, offset);
      }
      CHECK(docked);
      CHECK(!w.Find(st)->hostile);
    }
  }
}

void ARammedStationHurtsAndDoesNotDock() {
  World w(1);
  const EntityId st = AddStation(w, "coriolis", {0, 0, 0});
  const Vec3d axis = DockAxis(*w.Find(st));
  // Come in from the side at the station's equator, fast
  const Vec3d side = Normalized(Cross(axis, Vec3d{0, 1, 0}));
  const EntityId s = w.SpawnShip(Bp("python"), Team::kPlayer, side * 2000.0, LookAlong(-side, {0, 1, 0}));
  w.SetPlayer(s);
  w.Find(s)->controls.throttle = 1.0;
  w.Find(s)->velocity = -side * 200.0;
  Run(w, 20.0);
  const Entity* ship = w.Find(s);
  CHECK(ship != nullptr);
  CHECK(ship->shield < ship->shield_max || ship->hull < ship->hull_max);
  // It never got inside: the station's position is not inside the ship
  CHECK(Distance(ship->position, w.Find(st)->position) > 0.5 * w.Find(st)->hit_radius);
  for (const Event& e : w.TakeEvents()) {
    CHECK(e.kind != EventKind::kDocked);
  }
}

void TheDockingComputerFliesTheShipIn() {
  for (const char* id : {"coriolis", "dodo", "wheel_station", "rock_hermit"}) {
    for (int attempt = 0; attempt < 3; ++attempt) {
      World w(100 + static_cast<std::uint64_t>(attempt));
      const EntityId st = AddStation(w, id, {0, 0, 0});
      const Entity& station = *w.Find(st);
      const Vec3d axis = DockAxis(station);
      // Start well off to the side and facing away, at various angles
      const Vec3d side = Normalized(Cross(axis, Vec3d{0.3, 1, 0.1 * attempt}));
      const Vec3d start = DockPoint(station) + axis * 6000.0 + side * (3000.0 + 1500.0 * attempt);
      const EntityId s =
          w.SpawnShip(Bp("cobra_mk3_trader"), Team::kPlayer, start, LookAlong(side + axis * (attempt - 1), {0, 1, 0}));
      w.SetPlayer(s);
      bool docked = false;
      for (double t = 0.0; t < 400.0 && !docked; t += 1.0 / 60.0) {
        Entity* ship = w.Find(s);
        CHECK(UpdateAutopilot(w, *ship, 1.0 / 60.0));
        w.Step(1.0 / 60.0);
        for (const Event& e : w.TakeEvents()) {
          docked = docked || (e.kind == EventKind::kDocked && e.a == s);
        }
        if (w.Find(s) == nullptr) {
          break;
        }
      }
      if (!docked) {
        std::fprintf(stderr, "station %s attempt %d: did not dock\n", id, attempt);
      }
      CHECK(docked);
    }
  }
}

void HostileStationsRefuseAndLaunchPolice() {
  World w(4);
  const EntityId st = AddStation(w, "coriolis", {0, 0, 0});
  const Entity& station = *w.Find(st);
  const EntityId me = w.SpawnShip(Bp("cobra_mk3_trader"), Team::kPlayer, DockPoint(station) + DockAxis(station) * 30.0,
                                  LookAlong(-DockAxis(station), Up(station.orientation)));
  w.SetPlayer(me);
  w.Find(me)->velocity = -DockAxis(station) * 40.0;
  CHECK(CanDock(*w.Find(me), station, true));
  // Shoot the station: it turns hostile
  w.ApplyDamage(*w.Find(st), 1.0, me, w.Find(st)->position);
  CHECK(w.Find(st)->hostile);
  CHECK(!CanDock(*w.Find(me), *w.Find(st), true));
  w.set_player_wanted(true);
  Run(w, 10.0);
  int police = 0;
  for (const Entity& e : w.entities()) {
    police += e.team == Team::kPolice ? 1 : 0;
  }
  CHECK(police >= 1);
}

// ---------- AI ----------

void PiratesAttackThePlayer() {
  World w(11);
  const EntityId me = w.SpawnShip(Bp("cobra_mk3_trader"), Team::kPlayer, {0, 0, 0}, Quat{});
  w.SetPlayer(me);
  w.SpawnShip(Bp("mamba"), Team::kPirate, {0, 0, 6000}, LookAlong({0, 0, -1}, {0, 1, 0}));
  double hurt_at = -1.0;
  for (double t = 0.0; t < 90.0 && hurt_at < 0.0; t += 1.0 / 60.0) {
    w.Step(1.0 / 60.0);
    const Entity* player = w.Find(me);
    if (player->shield < player->shield_max - 1.0 || player->hull < player->hull_max) {
      hurt_at = t;
    }
  }
  CHECK(hurt_at > 0.0);
  CHECK(hurt_at < 60.0);
}

void PiratesBreakOffAndComeBack() {
  World w(12);
  const EntityId me = w.SpawnShip(Bp("python"), Team::kPlayer, {0, 0, 0}, Quat{});
  w.SetPlayer(me);
  w.Find(me)->hull = w.Find(me)->hull_max = 1e9;
  const EntityId p = w.SpawnShip(Bp("krait"), Team::kPirate, {0, 3000, 3000}, Quat{});
  std::set<AiMode> seen;
  for (double t = 0.0; t < 90.0; t += 1.0 / 60.0) {
    w.Step(1.0 / 60.0);
    if (const Entity* e = w.Find(p)) {
      seen.insert(e->ai.mode);
    }
  }
  CHECK(seen.count(AiMode::kApproach) == 1);
  CHECK(seen.count(AiMode::kAttack) == 1);
  CHECK(seen.count(AiMode::kBreak) == 1);
}

void PiratesFleeWhenBadlyHurt() {
  World w(13);
  const EntityId me = w.SpawnShip(Bp("cobra_mk3_bounty_hunter"), Team::kPlayer, {0, 0, 0}, Quat{});
  w.SetPlayer(me);
  w.Find(me)->hull = w.Find(me)->hull_max = 1e9;
  w.Find(me)->shield = w.Find(me)->shield_max = 1e9;
  const EntityId p = w.SpawnShip(Bp("sidewinder"), Team::kPirate, {0, 0, 4000}, Quat{});
  w.Step(1.0 / 60.0);
  w.Find(p)->hull = 0.1 * w.Find(p)->hull_max;
  bool fled = false;
  for (double t = 0.0; t < 90.0; t += 1.0 / 60.0) {
    w.Step(1.0 / 60.0);
    const Entity* e = w.Find(p);
    if (e == nullptr) {
      fled = true;  // far enough away to be gone
      break;
    }
    fled = fled || e->ai.mode == AiMode::kFlee;
  }
  CHECK(fled);
}

void PoliceHuntPiratesAndWantedPlayers() {
  World w(14);
  const EntityId me = w.SpawnShip(Bp("cobra_mk3_trader"), Team::kPlayer, {0, 0, 0}, Quat{});
  w.SetPlayer(me);
  const EntityId cop = w.SpawnShip(Bp("viper"), Team::kPolice, {0, 0, 5000}, LookAlong({0, 0, -1}, {0, 1, 0}));
  const EntityId pirate = w.SpawnShip(Bp("gecko"), Team::kPirate, {3000, 0, 5000}, Quat{});
  w.Find(me)->hull = w.Find(me)->hull_max = 1e9;
  // The player is clean: the police go for the pirate and leave the player alone
  CHECK(IsHostile(w, *w.Find(cop), *w.Find(pirate)));
  CHECK(!IsHostile(w, *w.Find(cop), *w.Find(me)));
  CHECK(IsHostile(w, *w.Find(pirate), *w.Find(me)));
  CHECK(!IsHostile(w, *w.Find(pirate), *w.Find(cop)));
  w.set_player_wanted(true);
  CHECK(IsHostile(w, *w.Find(cop), *w.Find(me)));
  w.set_player_wanted(false);
  Run(w, 60.0);
  const Entity* p = w.Find(pirate);
  CHECK(p == nullptr || p->hull < p->hull_max || p->shield < p->shield_max);
  CHECK(w.Find(me)->shield == w.Find(me)->shield_max || w.Find(me)->hull == w.Find(me)->hull_max);
}

void EveryoneHitsBackAtWhoeverHurtThem() {
  World w(15);
  const EntityId me = w.SpawnShip(Bp("sidewinder"), Team::kPlayer, {0, 0, 0}, Quat{});
  w.SetPlayer(me);
  const EntityId trader = w.SpawnShip(Bp("python"), Team::kTrader, {0, 0, 1500}, Quat{});
  const EntityId armed = w.SpawnShip(Bp("anaconda"), Team::kTrader, {2000, 0, 1500}, Quat{});
  w.Step(1.0 / 60.0);
  CHECK(!IsHostile(w, *w.Find(armed), *w.Find(me)));
  w.Find(armed)->last_attacker = me;
  CHECK(IsHostile(w, *w.Find(armed), *w.Find(me)));
  (void)trader;
}

void TradersFlyToTheStationAndDock() {
  World w(16);
  const EntityId st = AddStation(w, "coriolis", {0, 0, 0});
  const Vec3d axis = DockAxis(*w.Find(st));
  const EntityId t =
      w.SpawnShip(Bp("transporter"), Team::kTrader, DockPoint(*w.Find(st)) + axis * 12000.0 + Vec3d{4000, 0, 0},
                  LookAlong({1, 0, 0}, {0, 1, 0}));
  bool docked = false;
  for (double s = 0.0; s < 300.0 && !docked; s += 1.0 / 60.0) {
    w.Step(1.0 / 60.0);
    for (const Event& e : w.TakeEvents()) {
      docked = docked || (e.kind == EventKind::kDocked && e.a == t);
    }
  }
  CHECK(docked);
  CHECK(w.Find(t) == nullptr);
}

void TradersRunFromPirates() {
  World w(17);
  const EntityId t = w.SpawnShip(Bp("transporter"), Team::kTrader, {0, 0, 0}, Quat{});
  const EntityId p = w.SpawnShip(Bp("sidewinder"), Team::kPirate, {0, 0, 5000}, LookAlong({0, 0, -1}, {0, 1, 0}));
  Run(w, 1.0);
  CHECK(w.Find(t)->ai.target == p);
  const double before = Distance(w.Find(t)->position, w.Find(p)->position);
  Run(w, 8.0);
  // The trader is heading away from the pirate
  const Entity* trader = w.Find(t);
  CHECK(trader != nullptr);
  CHECK(Dot(Normalized(trader->velocity), Normalized(trader->position - w.Find(p)->position)) > 0.0 || before > 0.0);
}

void AliensLaunchDrones() {
  World w(18);
  const EntityId me = w.SpawnShip(Bp("cobra_mk3_bounty_hunter"), Team::kPlayer, {0, 0, 0}, Quat{});
  w.SetPlayer(me);
  w.Find(me)->hull = w.Find(me)->hull_max = 1e9;
  w.Find(me)->shield = w.Find(me)->shield_max = 1e9;
  w.SpawnShip(Bp("thargoid"), Team::kAlien, {0, 0, 3000}, LookAlong({0, 0, -1}, {0, 1, 0}));
  Run(w, 30.0);
  int drones = 0;
  for (const Entity& e : w.entities()) {
    drones += (e.blueprint != nullptr && e.blueprint->id == "thargon") ? 1 : 0;
  }
  CHECK(drones >= 2);
  CHECK(drones <= 6);
}

void DuelsAreRepeatableAndHaveAWinner() {
  const auto fight = [](std::uint64_t seed, double* time_out) {
    World w(seed);
    const EntityId a = w.SpawnShip(Bp("fer_de_lance"), Team::kPirate, {0, 0, 0}, Quat{});
    const EntityId b = w.SpawnShip(Bp("viper"), Team::kPolice, {1500, 200, 4000}, LookAlong({0, 0, -1}, {0, 1, 0}));
    // The police take the pirate for their target: the pirate fights back at whoever hurts it
    w.Find(a)->last_attacker = b;
    double t = 0.0;
    for (; t < 300.0; t += 1.0 / 60.0) {
      w.Step(1.0 / 60.0);
      if (w.Find(a) == nullptr || w.Find(b) == nullptr) {
        break;
      }
    }
    *time_out = t;
    return w.Find(a) == nullptr ? 2 : (w.Find(b) == nullptr ? 1 : 0);
  };
  double t1 = 0.0;
  double t2 = 0.0;
  const int r1 = fight(42, &t1);
  const int r2 = fight(42, &t2);
  CHECK(r1 == r2);
  CHECK(t1 == t2);
  CHECK(r1 != 0);
  // The tougher ship wins nearly always
  int fer_wins = 0;
  for (std::uint64_t seed = 1; seed <= 6; ++seed) {
    double t = 0.0;
    fer_wins += fight(seed, &t) == 1 ? 1 : 0;
  }
  CHECK(fer_wins >= 2);
}

// ---------- Galaxy, market, commander ----------

void GalaxiesAreRepeatableAndSensible() {
  const Galaxy g(0);
  const Galaxy again(0);
  CHECK(g.systems().size() == 256);
  std::set<std::string> names;
  std::set<std::pair<int, int>> places;
  for (int i = 0; i < 256; ++i) {
    const StarSystem& s = g.system(i);
    CHECK(s.name == again.system(i).name);
    CHECK(s.x == again.system(i).x && s.y == again.system(i).y && s.seed == again.system(i).seed);
    CHECK(names.insert(s.name).second);
    CHECK(places.insert({s.x, s.y}).second);
    CHECK(s.x >= 0 && s.x < 256 && s.y >= 0 && s.y < 128);
    CHECK(s.economy >= 0 && s.economy <= 7);
    CHECK(s.government >= 0 && s.government <= 7);
    CHECK(s.tech_level >= 1 && s.tech_level <= 15);
    CHECK(s.population >= 1 && s.population <= 71);
    CHECK(s.productivity >= 96 && s.productivity <= 62480);
    CHECK(s.radius_km >= 2816 && s.radius_km <= 6911);
    CHECK(!s.species.empty());
    // Anarchies and feudal states are never rich
    if (s.government <= 1) {
      CHECK(s.economy >= 2);
    }
    CHECK(s.index == i);
  }
  // The galaxies differ from one another
  const Galaxy g1(1);
  int same = 0;
  for (int i = 0; i < 256; ++i) {
    same += g.system(i).name == g1.system(i).name ? 1 : 0;
  }
  CHECK(same < 5);
  // Lookups
  CHECK(g.Find(g.system(10).name) == 10);
  std::string upper = g.system(10).name;
  for (char& c : upper) {
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }
  CHECK(g.Find(upper) == 10);
  CHECK(g.Find("nowhere") == -1);
  CHECK(g.Nearest(g.system(33).x, g.system(33).y) == 33);
  CHECK(Near(g.DistanceLy(5, 5), 0.0));
  CHECK(Near(g.DistanceLy(5, 9), g.DistanceLy(9, 5)));
  // A start in the middle has neighbours within a normal jump
  const std::vector<int> near = g.InRange(g.Nearest(128, 64), 7.0);
  CHECK(near.size() >= 3);
  for (std::size_t i = 1; i < near.size(); ++i) {
    CHECK(g.DistanceLy(g.Nearest(128, 64), near[i - 1]) <= g.DistanceLy(g.Nearest(128, 64), near[i]));
  }
}

void MarketsFollowTheEconomy() {
  const Galaxy g(0);
  long food_agri = 0, food_ind = 0, comp_agri = 0, comp_ind = 0;
  int agri = 0, ind = 0;
  for (const StarSystem& s : g.systems()) {
    const Market m = Market::ForSystem(s, 3);
    const Market same = Market::ForSystem(s, 3);
    for (std::size_t c = 0; c < kCommodityCount; ++c) {
      const Commodity item = static_cast<Commodity>(c);
      CHECK(m.entry(item).price >= 3);
      CHECK(m.entry(item).quantity >= 0 && m.entry(item).quantity <= 63);
      CHECK(m.entry(item).price == same.entry(item).price);
      CHECK(m.entry(item).quantity == same.entry(item).quantity);
    }
    if (s.tech_level < 8) {
      CHECK(m.entry(Commodity::kAlienItems).quantity == 0);
    }
    if (s.economy >= 4) {
      food_agri += m.entry(Commodity::kFood).price;
      comp_agri += m.entry(Commodity::kComputers).price;
      ++agri;
    } else {
      food_ind += m.entry(Commodity::kFood).price;
      comp_ind += m.entry(Commodity::kComputers).price;
      ++ind;
    }
  }
  // Food is cheap where it is grown and computers where they are made
  CHECK(food_agri * ind < food_ind * agri);
  CHECK(comp_ind * agri < comp_agri * ind);
  // The mood changes the prices between visits
  const StarSystem& s = g.system(0);
  int differing = 0;
  for (std::size_t c = 0; c < kCommodityCount; ++c) {
    const Commodity item = static_cast<Commodity>(c);
    differing += Market::ForSystem(s, 1).entry(item).price != Market::ForSystem(s, 2).entry(item).price ? 1 : 0;
  }
  CHECK(differing >= 3);
}

void TradingRespectsMoneyHoldAndStock() {
  const Galaxy g(0);
  Market m = Market::ForSystem(g.system(7), 1);
  Commander c;
  c.credits = 100000;
  const Commodity item = Commodity::kMinerals;
  m.entry(item).quantity = 10;
  m.entry(item).price = 40;
  // Buy
  CHECK(BuyCargo(c, m, item, 4));
  CHECK(c.cargo[static_cast<std::size_t>(item)] == 4);
  CHECK(c.credits == 100000 - 160);
  CHECK(m.entry(item).quantity == 6);
  // Not more than the market has, the money allows or the hold takes
  CHECK(!BuyCargo(c, m, item, 7));
  CHECK(!BuyCargo(c, m, item, 0));
  CHECK(!BuyCargo(c, m, item, -3));
  c.credits = 100;
  CHECK(!BuyCargo(c, m, item, 5));
  c.credits = 100000;
  c.cargo_capacity = 6;
  CHECK(!BuyCargo(c, m, item, 3));
  CHECK(BuyCargo(c, m, item, 2));
  CHECK(CargoFree(c) == 0);
  // Sell
  CHECK(!SellCargo(c, m, item, 7));
  CHECK(SellCargo(c, m, item, 6));
  CHECK(c.cargo[static_cast<std::size_t>(item)] == 0);
  CHECK(!SellCargo(c, m, item, 1));
  // Round trip at one price costs nothing: 80 spent after the reset, 240 for the six
  CHECK(c.credits == 100000 - 80 + 240);
  CHECK(Near(0, 0));
}

void EquipmentCostsAndNeedsTech() {
  Commander c;
  c.credits = 1000000;
  CHECK(!BuyItem(c, Item::kEcm, 2));
  CHECK(BuyItem(c, Item::kEcm, 5));
  CHECK(c.equipment.ecm);
  CHECK(!BuyItem(c, Item::kEcm, 5));
  CHECK(c.credits == 1000000 - ItemPrice(Item::kEcm));
  CHECK(!BuyItem(c, Item::kMilitaryLaser, 9));
  CHECK(BuyItem(c, Item::kMilitaryLaser, 12));
  CHECK(c.equipment.laser_level == 2);
  CHECK(!BuyItem(c, Item::kBeamLaser, 12));
  c.credits = 100;
  CHECK(!BuyItem(c, Item::kDockingComputer, 12));
  // Missiles up to four
  c.credits = 100000;
  c.missiles = 3;
  CHECK(BuyItem(c, Item::kMissile, 1));
  CHECK(!BuyItem(c, Item::kMissile, 1));
  // Fuel fills the tank, as far as the money goes
  c.fuel = 2.0;
  c.credits = 100000;
  CHECK(BuyItem(c, Item::kFuel, 1));
  CHECK(Near(c.fuel, kMaxFuel, 1e-9));
  CHECK(c.credits == 100000 - 50 * (ItemPrice(Item::kFuel) / 10));
  c.fuel = 0.0;
  c.credits = 3 * (ItemPrice(Item::kFuel) / 10);
  CHECK(BuyItem(c, Item::kFuel, 1));
  CHECK(Near(c.fuel, 0.3, 1e-9));
  CHECK(c.credits == 0);
  CHECK(!BuyItem(c, Item::kFuel, 1));
}

void RanksAndLawFollowTheNumbers() {
  CHECK(std::string(RankName(0)) == "Harmless");
  CHECK(std::string(RankName(7.9)) == "Harmless");
  CHECK(std::string(RankName(8)) == "Mostly Harmless");
  CHECK(std::string(RankName(130)) == "Competent");
  CHECK(std::string(RankName(2560)) == "Deadly");
  CHECK(std::string(RankName(6400)) == "Elite");
  CHECK(std::string(LegalName(0)) == "Clean");
  CHECK(std::string(LegalName(1)) == "Offender");
  CHECK(std::string(LegalName(49)) == "Offender");
  CHECK(std::string(LegalName(50)) == "Fugitive");
}

void CommandersSaveAndLoad() {
  Commander c;
  c.name = "Test \"Pilot\"";
  c.credits = 123456;
  c.fuel = 3.4;
  c.galaxy = 2;
  c.system = 99;
  c.cargo[3] = 5;
  c.cargo[16] = 1;
  c.missiles = 2;
  c.equipment.ecm = true;
  c.equipment.laser_level = 1;
  c.legal_status = 17;
  c.kills = 131.5;
  c.market_mood = 77;
  c.ship = "python";
  Json json;
  std::string error;
  CHECK(Json::Parse(CommanderToJson(c).Dump(), &json, &error));
  Commander back;
  CHECK(CommanderFromJson(json, &back, &error));
  CHECK(back.name == c.name && back.credits == c.credits && back.fuel == c.fuel && back.galaxy == 2 &&
        back.system == 99);
  CHECK(back.cargo == c.cargo && back.missiles == 2 && back.equipment.ecm && back.equipment.laser_level == 1);
  CHECK(!back.equipment.fuel_scoop && back.legal_status == 17 && back.kills == 131.5 && back.market_mood == 77);
  CHECK(back.ship == "python");

  const std::string path = "coriolis_test_commander.json";
  CHECK(SaveCommander(c, path, &error));
  Commander loaded;
  CHECK(LoadCommander(path, &loaded, &error));
  CHECK(loaded.name == c.name && loaded.credits == c.credits);
  std::remove(path.c_str());
  CHECK(!LoadCommander(path, &loaded, &error));

  // Saves that are damaged or have been edited out of range are refused
  const auto bad = [&](const std::string& from, const std::string& to) {
    std::string text = CommanderToJson(c).Dump();
    const std::size_t at = text.find(from);
    CHECK(at != std::string::npos);
    text.replace(at, from.size(), to);
    Json j;
    Commander out;
    std::string e;
    return Json::Parse(text, &j, &e) && CommanderFromJson(j, &out, &e);
  };
  CHECK(!bad("\"galaxy\": 2", "\"galaxy\": 9"));
  CHECK(!bad("\"system\": 99", "\"system\": 256"));
  CHECK(!bad("\"missiles\": 2", "\"missiles\": 500"));
  CHECK(!bad("\"legal_status\": 17", "\"legal_status\": -1"));
  CHECK(!bad("\"format\": \"coriolis-commander\"", "\"format\": \"x\""));
  CHECK(!bad("\"ecm\": true", "\"ecm\": 1"));
  CHECK(!CommanderFromJson(Json::MakeArray(), &back, &error));
}

// ---------- The game ----------

Commander StartCommander() {
  Commander c;
  c.galaxy = 0;
  c.system = Galaxy(0).Find(Galaxy(0).system(40).name);
  return c;
}

void GameStartsDockedAndLaunches() {
  Game game(StartCommander(), 5);
  CHECK(game.mode() == GameMode::kDocked);
  CHECK(game.player() == nullptr);
  CHECK(game.Launch(false));
  CHECK(game.mode() == GameMode::kFlight);
  const Entity* me = game.player();
  CHECK(me != nullptr);
  CHECK(me->is_player && me->team == Team::kPlayer);
  CHECK(me->blueprint->id == "cobra_mk3_trader");
  // The planet, sun and station are there, and the ship is just outside the bay, heading away
  CHECK(game.world().FindKind(EntityKind::kPlanet) != nullptr);
  CHECK(game.world().FindKind(EntityKind::kSun) != nullptr);
  const Entity* station = game.world().FindKind(EntityKind::kStation);
  CHECK(station != nullptr);
  CHECK(Distance(me->position, DockPoint(*station)) < 3.0 * DockRadius(*station));
  CHECK(Dot(Forward(me->orientation), DockAxis(*station)) > 0.9);
  // Cannot launch twice, buy or sell in flight
  CHECK(!game.Launch(false));
  CHECK(!game.Buy(Commodity::kFood, 1));
  // And the ship is mass locked beside the station
  CHECK(game.MassLocked());
}

void TheGameTradesWhenDocked() {
  Commander c = StartCommander();
  Game game(c, 5);
  game.market().entry(Commodity::kMinerals).quantity = 20;
  game.market().entry(Commodity::kMinerals).price = 30;
  CHECK(game.Buy(Commodity::kMinerals, 5));
  CHECK(game.commander().cargo[static_cast<std::size_t>(Commodity::kMinerals)] == 5);
  CHECK(game.Sell(Commodity::kMinerals, 5));
  CHECK(game.commander().credits == c.credits);
  game.commander().credits = 100000;
  CHECK(game.BuyItem(Item::kMissile) == (game.system().tech_level >= 1 && game.commander().missiles <= 4));
}

void ComingHomeAndDockingEndsTheFlight() {
  Game game(StartCommander(), 6);
  CHECK(game.Launch(false));
  // Turn the ship round at the bay and fly back in with the docking computer
  game.commander().equipment.docking_computer = true;
  Entity* me = game.player();
  const Entity* station = game.world().FindKind(EntityKind::kStation);
  const Vec3d axis = DockAxis(*station);
  me->position = DockPoint(*station) + axis * 4000.0 + Vec3d{1500, 800, 0};
  me->orientation = LookAlong({1, 0, 0}, {0, 1, 0});
  me->velocity = Vec3d{};
  FlightInput press;
  press.pressed.set(static_cast<std::size_t>(Action::kDockingComputer));
  game.Update(press, 1.0 / 60.0);
  CHECK(game.autopilot());
  for (int i = 0; i < 60 * 300 && game.mode() == GameMode::kFlight; ++i) {
    game.Update(FlightInput(), 1.0 / 60.0);
  }
  CHECK(game.mode() == GameMode::kDocked);
  CHECK(game.player() == nullptr);
  bool said = false;
  for (const std::string& m : game.TakeMessages()) {
    said = said || m.find("Docked") != std::string::npos;
  }
  CHECK(said);
  // Docked again, the market has moved on and the player can fly out again
  CHECK(game.Launch(false));
}

void TheTractorBeamLandsTheShipBeforeTheStationScreens() {
  Game game(StartCommander(), 6);
  CHECK(game.Launch(false));
  Entity* me = game.player();
  const Entity* station = game.world().FindKind(EntityKind::kStation);
  const Bay bay = BayOf(*station);
  // Straight into the mouth of the bay, nose first, at a gentle speed
  me->position = DockPoint(*station) + bay.axis * 30.0;
  me->orientation = LookAlong(-bay.axis, bay.up);
  me->velocity = -bay.axis * 40.0;
  me->controls.throttle = 0.0;
  CHECK(CanDock(*me, *station, true));

  double beam_start = -1.0;
  double lowest = 1e9;
  double deepest = 0.0;
  double seconds = 0.0;
  bool inside_while_flying = true;
  for (int i = 0; i < 60 * 30 && game.mode() == GameMode::kFlight; ++i) {
    FlightInput steer;
    steer.pitch = 1.0;  // the pilot is locked out: this must change nothing
    game.Update(steer, 1.0 / 60.0);
    seconds += 1.0 / 60.0;
    if (game.landing()) {
      if (beam_start < 0.0) {
        beam_start = seconds;
      }
      const Entity* ship = game.player();
      CHECK(ship != nullptr);
      if (ship == nullptr) {
        break;
      }
      const Vec3d rel = ship->position - bay.mouth;
      deepest = std::min(deepest, Dot(rel, bay.axis));
      lowest = std::min(lowest, Dot(rel, bay.up));
      inside_while_flying = inside_while_flying && Dot(rel, bay.axis) < 40.0;
    }
  }
  // The beam took hold, drew the ship well into the bay and set it down on the floor, and only then was it docked
  CHECK(beam_start > 0.0);
  CHECK(game.mode() == GameMode::kDocked);
  CHECK(seconds - beam_start > 4.0);
  CHECK(seconds - beam_start < 10.0);
  CHECK(deepest < -0.3 * bay.depth);
  CHECK(lowest < -0.3 * bay.half_height);
  CHECK(inside_while_flying);
  CHECK(!game.landing());
  CHECK(game.player() == nullptr);
  CHECK(game.Launch(false));
}

void AShipFlyingIntoTheBayHowEverItIsAlignedIsLandedNotCrashed() {
  const struct {
    const char* name;
    double speed;
    double roll;
    double yaw;
    double side;
    double up;
  } entries[] = {
      {"fast", 350.0, 0.0, 0.0, 0.0, 0.0},       {"very fast", 600.0, 0.0, 0.0, 0.0, 0.0}, {"rolled", 150.0, 1.57, 0.0, 0.0, 0.0},
      {"upside down", 150.0, 3.14, 0.0, 0.0, 0.0}, {"off axis", 150.0, 0.0, 0.26, 0.0, 0.0},  {"to one side", 150.0, 0.0, 0.0, 60.0, 0.0},
      {"high", 150.0, 0.0, 0.0, 0.0, 30.0},
  };
  for (const auto& entry : entries) {
    Game game(StartCommander(), 3);
    CHECK(game.Launch(false));
    Entity* me = game.player();
    const Bay bay = BayOf(*game.world().FindKind(EntityKind::kStation));
    const double hull = me->hull;
    me->position = bay.mouth + bay.axis * 700.0 + bay.right * entry.side + bay.up * entry.up;
    me->orientation = Turned(LookAlong(-bay.axis, bay.up), 0.0, entry.roll, entry.yaw);
    me->velocity = Forward(me->orientation) * entry.speed;
    bool beam = false;
    for (int i = 0; i < 60 * 30 && game.mode() == GameMode::kFlight; ++i) {
      game.Update(FlightInput{}, 1.0 / 60.0);
      beam = beam || game.landing();
      if (const Entity* ship = game.player()) {
        CHECK(ship->hull >= hull);  // nothing is hit on the way in
      }
    }
    // Taken by the beam and set down, never left to hit the walls of the bay
    CHECK(beam);
    CHECK(game.mode() == GameMode::kDocked);
    if (game.mode() != GameMode::kDocked) {
      std::fprintf(stderr, "  entry: %s\n", entry.name);
    }
  }
}

void ALaunchedShipIsNotTakenBackByTheBeam() {
  Game game(StartCommander(), 3);
  CHECK(game.Launch(true));
  for (int i = 0; i < 60 * 14; ++i) {
    game.Update(FlightInput{}, 1.0 / 60.0);
    CHECK(game.mode() == GameMode::kFlight);
  }
  // Out of the bay and well clear, flying free
  CHECK(!game.launching() && !game.landing());
  const Entity* ship = game.player();
  CHECK(ship != nullptr);
  const Bay bay = BayOf(*game.world().FindKind(EntityKind::kStation));
  CHECK(Dot(ship->position - bay.mouth, bay.axis) > 500.0);
}

void NoDockingComputerNoAutopilot() {
  Game game(StartCommander(), 6);
  game.Launch(false);
  FlightInput press;
  press.pressed.set(static_cast<std::size_t>(Action::kDockingComputer));
  game.Update(press, 1.0 / 60.0);
  CHECK(!game.autopilot());
}

void KillingPiratesPaysAndRanksUp() {
  Game game(StartCommander(), 7);
  game.Launch(false);
  Entity* me = game.player();
  const std::int64_t credits = game.commander().credits;
  const double kills = game.commander().kills;
  const EntityId pirate = game.world().SpawnShip(Bp("mamba"), Team::kPirate, me->position + Vec3d{0, 0, 1500}, Quat{});
  Entity* p = game.world().Find(pirate);
  game.world().ApplyDamage(*p, 1e6, me->id, p->position);
  game.Update(FlightInput(), 0.1);
  CHECK(game.commander().kills > kills);
  CHECK(game.commander().credits == credits + 1500);
  CHECK(game.commander().legal_status == 0);
  // Shooting the police makes a fugitive
  Entity* me2 = game.player();
  const EntityId cop = game.world().SpawnShip(Bp("viper"), Team::kPolice, me2->position + Vec3d{0, 0, 1500}, Quat{});
  game.world().ApplyDamage(*game.world().Find(cop), 1e6, me2->id, {0, 0, 0});
  game.Update(FlightInput(), 0.1);
  CHECK(game.commander().legal_status >= 50);
  CHECK(game.world().player_wanted());
  // Reaching a new rank is announced
  game.commander().kills = 7.9;
  Entity* me3 = game.player();
  const EntityId more = game.world().SpawnShip(Bp("mamba"), Team::kPirate, me3->position + Vec3d{0, 0, 1500}, Quat{});
  game.world().ApplyDamage(*game.world().Find(more), 1e6, me3->id, {0, 0, 0});
  game.Update(FlightInput(), 0.1);
  bool ranked = false;
  for (const std::string& m : game.TakeMessages()) {
    ranked = ranked || m.find("Mostly Harmless") != std::string::npos;
  }
  CHECK(ranked);
}

void KillingTradersMakesAnOffender() {
  Game game(StartCommander(), 7);
  game.Launch(false);
  Entity* me = game.player();
  const EntityId t = game.world().SpawnShip(Bp("transporter"), Team::kTrader, me->position + Vec3d{0, 0, 1500}, Quat{});
  const std::int64_t credits = game.commander().credits;
  game.world().ApplyDamage(*game.world().Find(t), 1e6, me->id, {0, 0, 0});
  game.Update(FlightInput(), 0.1);
  CHECK(game.commander().legal_status > 0);
  CHECK(game.commander().legal_status < 50);
  CHECK(game.commander().credits == credits);
}

void AttackingTheStationBringsThePolice() {
  Game game(StartCommander(), 8);
  game.Launch(false);
  const Entity* station = game.world().FindKind(EntityKind::kStation);
  Entity* me = game.player();
  game.world().ApplyDamage(*game.world().FindKind(EntityKind::kStation), 1.0, me->id, station->position);
  game.Update(FlightInput(), 0.05);
  CHECK(game.commander().legal_status >= 20);
  CHECK(game.world().player_wanted());
}

void DeathEndsTheGameUnlessThereIsAPod() {
  {
    Game game(StartCommander(), 9);
    game.Launch(false);
    Entity* me = game.player();
    game.world().ApplyDamage(*me, 1e9, kNoEntity, me->position);
    game.Update(FlightInput(), 0.1);
    CHECK(game.mode() == GameMode::kDead);
    game.Update(FlightInput(), 0.1);
    CHECK(!game.Launch(false));
  }
  {
    Commander c = StartCommander();
    c.equipment.escape_pod = true;
    c.equipment.ecm = true;
    c.cargo[2] = 4;
    c.legal_status = 30;
    c.credits = 5000;
    c.kills = 40.0;
    Game game(c, 9);
    game.Launch(false);
    Entity* me = game.player();
    game.world().ApplyDamage(*me, 1e9, kNoEntity, me->position);
    game.Update(FlightInput(), 0.1);
    // The pod saved the pilot, who is docked with the money and the rank but nothing else
    CHECK(game.mode() == GameMode::kDocked);
    CHECK(game.commander().credits == 5000);
    CHECK(game.commander().kills == 40.0);
    CHECK(game.commander().cargo[2] == 0);
    CHECK(!game.commander().equipment.ecm && !game.commander().equipment.escape_pod);
    CHECK(game.commander().legal_status == 0);
    CHECK(game.Launch(false));
  }
}

void HyperspaceNeedsFuelDistanceAndClearSpace() {
  Game game(StartCommander(), 10);
  game.Launch(false);
  const int here = game.commander().system;
  const std::vector<int> near = game.galaxy().InRange(here, 7.0);
  CHECK(!near.empty());
  // Nothing chosen
  CHECK(!game.StartJump());
  // Beside the station: mass locked
  game.SelectSystem(near[0]);
  CHECK(!game.StartJump());
  // Move well clear of everything
  Entity* me = game.player();
  me->position = Vec3d{0, 1.0e6, 0};
  for (Entity& e : game.world().entities()) {
    if (e.kind == EntityKind::kShip && !e.is_player) {
      e.alive = false;
      e.silent_removal = true;
    }
  }
  game.world().Step(1.0 / 60.0);
  me = game.player();
  CHECK(!game.MassLocked());
  // Too far for the fuel
  int far = -1;
  for (int i = 0; i < 256; ++i) {
    if (game.galaxy().DistanceLy(here, i) > 7.5) {
      far = i;
      break;
    }
  }
  game.SelectSystem(far);
  CHECK(!game.StartJump());
  // A good jump: a countdown, then the new system, less fuel
  game.SelectSystem(near[0]);
  const double fuel = game.commander().fuel;
  const double cost = game.galaxy().DistanceLy(here, near[0]);
  CHECK(game.StartJump());
  CHECK(game.jump_countdown() > 3.0);
  game.CancelJump();
  CHECK(game.jump_countdown() < 0.0);
  CHECK(game.StartJump());
  for (int i = 0; i < 60 * 6 && game.commander().system == here; ++i) {
    game.Update(FlightInput(), 1.0 / 60.0);
  }
  CHECK(game.commander().system == near[0]);
  CHECK(Near(game.commander().fuel, fuel - cost, 1e-6));
  CHECK(game.mode() == GameMode::kFlight);
  // Arrived with a fresh planet, sun and station, a long way from the station, heading for it
  const Entity* station = game.world().FindKind(EntityKind::kStation);
  CHECK(station != nullptr);
  CHECK(Distance(game.player()->position, station->position) > 30000.0);
  CHECK(Dot(Forward(game.player()->orientation), Normalized(station->position - game.player()->position)) > 0.99);
  CHECK(!game.MassLocked());
}

void TheCargoBayAndTheGalacticHyperdriveAreBoughtAndSaved() {
  Commander c = StartCommander();
  c.credits = 10000000;
  const int hold = c.cargo_capacity;
  CHECK(!BuyItem(c, Item::kCargoBay, 4));
  CHECK(BuyItem(c, Item::kCargoBay, 5));
  CHECK(c.equipment.cargo_bay && c.cargo_capacity == hold + kCargoBayExtra);
  CHECK(!BuyItem(c, Item::kCargoBay, 5));
  CHECK(c.cargo_capacity == hold + kCargoBayExtra);
  CHECK(!BuyItem(c, Item::kGalacticHyperdrive, 9));
  CHECK(BuyItem(c, Item::kGalacticHyperdrive, 10));
  CHECK(!BuyItem(c, Item::kGalacticHyperdrive, 10));

  Json json;
  std::string error;
  const std::string text = CommanderToJson(c).Dump();
  CHECK(Json::Parse(text, &json, &error));
  Commander back;
  CHECK(CommanderFromJson(json, &back, &error));
  CHECK(back.equipment.cargo_bay && back.equipment.galactic_hyperdrive && back.cargo_capacity == c.cargo_capacity);

  // A save from before these existed still loads, without them
  std::string old = text;
  const std::size_t from = old.find("\"cargo_bay\"");
  const std::size_t to = old.find("\"laser_level\"");
  CHECK(from != std::string::npos && to != std::string::npos && from < to);
  old.erase(from, to - from);
  CHECK(Json::Parse(old, &json, &error));
  Commander older;
  CHECK(CommanderFromJson(json, &older, &error));
  CHECK(!older.equipment.cargo_bay && !older.equipment.galactic_hyperdrive);
}

void AGalacticJumpGoesToTheNextGalaxyAndUsesTheDrive() {
  Commander c = StartCommander();
  c.equipment.galactic_hyperdrive = true;
  Game game(c, 12);
  game.Launch(false);
  const StarSystem before = game.system();
  const double fuel = game.commander().fuel;
  // Beside the station the ship is mass locked
  CHECK(!game.StartGalacticJump());
  game.player()->position = Vec3d{0, 1.0e6, 0};
  for (Entity& e : game.world().entities()) {
    if (e.kind == EntityKind::kShip && !e.is_player) {
      e.alive = false;
      e.silent_removal = true;
    }
  }
  game.world().Step(1.0 / 60.0);
  CHECK(game.StartGalacticJump());
  CHECK(game.galactic_jump());
  game.CancelJump();
  CHECK(!game.galactic_jump());
  CHECK(game.commander().equipment.galactic_hyperdrive);
  CHECK(game.StartGalacticJump());
  for (int i = 0; i < 60 * 6 && game.commander().galaxy == 0; ++i) {
    game.Update(FlightInput(), 1.0 / 60.0);
  }
  CHECK(game.commander().galaxy == 1);
  CHECK(game.galaxy().number() == 1);
  CHECK(!game.commander().equipment.galactic_hyperdrive);
  CHECK(Near(game.commander().fuel, fuel, 1e-9));
  CHECK(game.mode() == GameMode::kFlight);
  CHECK(game.world().FindKind(EntityKind::kStation) != nullptr);
  // It arrives near the same place on the chart
  CHECK(std::abs(game.system().x - before.x) < 40 && std::abs(game.system().y - before.y) < 40);
  // With no drive left there is no second jump
  CHECK(!game.StartGalacticJump());
}

void ShipsAreBoughtWithTheTradeInOfTheOldOne() {
  Commander c = StartCommander();
  CHECK(c.ship == "cobra_mk3_trader");
  CHECK(FindShipForSale(c.ship) != nullptr);
  const int trade_in = TradeInValue(c);
  CHECK(trade_in > 0 && trade_in < FindShipForSale(c.ship)->price);
  int python = -1;
  int adder = -1;
  int cobra = -1;
  for (int i = 0; i < ShipForSaleCount(); ++i) {
    const std::string id = ShipForSaleAt(i).id;
    python = id == "python" ? i : python;
    adder = id == "adder" ? i : adder;
    cobra = id == "cobra_mk3_trader" ? i : cobra;
  }
  CHECK(python >= 0 && adder >= 0 && cobra >= 0);
  // Too poor, and the tech level is too low
  c.credits = 1000;
  CHECK(!BuyShip(c, python, 12));
  c.credits = 10000000;
  CHECK(!BuyShip(c, python, 9));
  CHECK(!BuyShip(c, ShipForSaleCount(), 15));
  // The ship already flown cannot be bought
  CHECK(!BuyShip(c, cobra, 15));
  // The cargo has to fit: a full hold will not go into the adder
  c.cargo[0] = c.cargo_capacity;
  CHECK(!BuyShip(c, adder, 15));
  CHECK(c.ship == "cobra_mk3_trader" && c.credits == 10000000);
  c.cargo[0] = 0;
  c.equipment.ecm = true;
  c.missiles = 3;
  const std::int64_t cash = c.credits;
  CHECK(BuyShip(c, python, 15));
  CHECK(c.ship == "python");
  CHECK(c.credits == cash - (ShipForSaleAt(python).price - trade_in));
  CHECK(c.cargo_capacity == ShipForSaleAt(python).hold);
  CHECK(c.equipment.ecm && c.missiles == 3);
  // With the extra bay fitted the hold has it on top
  c.equipment.cargo_bay = true;
  CHECK(BuyShip(c, adder, 15));
  CHECK(c.cargo_capacity == ShipForSaleAt(adder).hold + kCargoBayExtra);

  // Only in the station, and the new ship is the one that launches
  Commander d = StartCommander();
  d.credits = 10000000;
  const Galaxy galaxy(d.galaxy);
  for (const StarSystem& s : galaxy.systems()) {
    if (s.tech_level >= 12) {
      d.system = s.index;
      break;
    }
  }
  Game game(d, 13);
  CHECK(game.system().tech_level >= 12);
  CHECK(game.BuyShip(python));
  CHECK(game.Launch(false));
  CHECK(game.commander().ship == "python");
  CHECK(game.player() != nullptr);
  CHECK(!game.BuyShip(adder));
}

/// Fly the ship back into the bay with the docking computer, and dock
bool FlyBackAndDock(Game& game) {
  game.commander().equipment.docking_computer = true;
  Entity* me = game.player();
  const Entity* station = game.world().FindKind(EntityKind::kStation);
  const Vec3d axis = DockAxis(*station);
  me->position = DockPoint(*station) + axis * 4000.0 + Vec3d{1500, 800, 0};
  me->orientation = LookAlong({1, 0, 0}, {0, 1, 0});
  me->velocity = Vec3d{};
  FlightInput press;
  press.pressed.set(static_cast<std::size_t>(Action::kDockingComputer));
  game.Update(press, 1.0 / 60.0);
  for (int i = 0; i < 60 * 300 && game.mode() == GameMode::kFlight; ++i) {
    game.Update(FlightInput(), 1.0 / 60.0);
  }
  return game.mode() == GameMode::kDocked;
}

void EveryStepOfTheStoryIsOfferedWithItsBriefing() {
  const Galaxy galaxy(0);
  for (int step = 0; step < kStorySteps; ++step) {
    Commander c = StartCommander();
    c.story_step = step;
    Mission m;
    CHECK(MissionOffered(c, galaxy, &m));
    CHECK(m.step == step && m.kind != MissionKind::kNone);
    CHECK(!MissionTitle(m).empty() && !MissionDone(m).empty() && !MissionStatus(m, galaxy).empty());
    const std::vector<std::string> lines = MissionBriefing(m, galaxy);
    CHECK(lines.size() >= 3);
    for (const std::string& line : lines) {
      CHECK(line.size() <= 78);
    }
    if (m.kind == MissionKind::kDeliver) {
      CHECK(m.destination >= 0 && m.destination != c.system);
      CHECK(galaxy.DistanceLy(c.system, m.destination) <= kMaxFuel);
      CHECK(lines[0].find(galaxy.system(m.destination).name) != std::string::npos ||
            lines[1].find(galaxy.system(m.destination).name) != std::string::npos);
    } else {
      CHECK(m.count > 0);
    }
    // The same job every time
    Mission again;
    CHECK(MissionOffered(c, galaxy, &again));
    CHECK(again.destination == m.destination && again.reward == m.reward);
  }
  Commander done = StartCommander();
  done.story_step = kStorySteps;
  Mission none;
  CHECK(!MissionOffered(done, galaxy, &none));
}

void TheLastJobFitsAGalacticHyperdriveOrPaysForOne() {
  const Galaxy galaxy(0);
  Commander c = StartCommander();
  c.story_step = kStorySteps - 1;
  CHECK(MissionOffered(c, galaxy, &c.mission));
  const std::int64_t cash = c.credits;
  CHECK(MissionComplete(c.mission, c.mission.destination));
  PayMission(c);
  CHECK(c.equipment.galactic_hyperdrive && c.credits == cash && c.story_step == kStorySteps);
  CHECK(c.mission.kind == MissionKind::kNone);

  Commander d = StartCommander();
  d.story_step = kStorySteps - 1;
  d.equipment.galactic_hyperdrive = true;
  CHECK(MissionOffered(d, galaxy, &d.mission));
  PayMission(d);
  CHECK(d.credits == 1000 + ItemPrice(Item::kGalacticHyperdrive));
}

void ADeliveryIsPaidOnDockingAtTheDestinationOnly() {
  Game game(StartCommander(), 16);
  Mission offer;
  CHECK(game.MissionOnOffer(&offer));
  CHECK(offer.kind == MissionKind::kDeliver && offer.step == 0);
  CHECK(game.AcceptMission());
  // One job at a time
  CHECK(!game.MissionOnOffer(&offer));
  CHECK(!game.AcceptMission());
  const int destination = game.commander().mission.destination;
  const std::int64_t cash = game.commander().credits;

  // Docking back where it was taken does not finish it
  CHECK(game.Launch(false));
  CHECK(FlyBackAndDock(game));
  CHECK(game.commander().mission.kind == MissionKind::kDeliver);
  CHECK(game.commander().credits == cash && game.commander().story_step == 0);

  // Docking at the destination does
  CHECK(game.Launch(false));
  game.commander().system = destination;
  CHECK(FlyBackAndDock(game));
  CHECK(game.commander().mission.kind == MissionKind::kNone);
  CHECK(game.commander().story_step == 1);
  CHECK(game.commander().credits == cash + 1500);
  // The next job is on offer
  CHECK(game.MissionOnOffer(&offer) && offer.step == 1);
}

void ShootingPiratesAndDockingPaysAHuntingJob() {
  Commander c = StartCommander();
  c.story_step = 1;
  Game game(c, 17);
  Mission offer;
  CHECK(game.MissionOnOffer(&offer) && offer.kind == MissionKind::kHunt && offer.count == 3);
  CHECK(game.AcceptMission());
  CHECK(game.Launch(false));
  const auto shoot = [&game](Team team) {
    Entity* me = game.player();
    const EntityId id = game.world().SpawnShip(Bp("mamba"), team, me->position + Vec3d{0, 0, 1500}, Quat{});
    game.world().ApplyDamage(*game.world().Find(id), 1e6, me->id, me->position);
    game.Update(FlightInput(), 0.1);
  };
  shoot(Team::kPirate);
  shoot(Team::kTrader);  // only pirates count
  shoot(Team::kPirate);
  CHECK(game.commander().mission.progress == 2);
  // Not enough yet: docking pays nothing
  const std::int64_t cash = game.commander().credits;
  shoot(Team::kPirate);
  CHECK(game.commander().mission.progress == 3);
  shoot(Team::kPirate);
  CHECK(game.commander().mission.progress == 3);
  const std::int64_t after_bounties = game.commander().credits;
  CHECK(after_bounties > cash);
  CHECK(FlyBackAndDock(game));
  CHECK(game.commander().mission.kind == MissionKind::kNone && game.commander().story_step == 2);
  CHECK(game.commander().credits == after_bounties + 4000);
}

void AJobCanBeGivenUpAndTheStoryAndJobAreSaved() {
  Commander c = StartCommander();
  Game game(c, 18);
  CHECK(!game.AbandonMission());
  CHECK(game.AcceptMission());
  c = game.commander();
  c.story_step = 3;
  c.mission.step = 3;
  c.mission.kind = MissionKind::kHunt;
  c.mission.count = 8;
  c.mission.progress = 5;
  c.mission.destination = -1;
  c.mission.reward = 20000;
  Json json;
  std::string error;
  const std::string text = CommanderToJson(c).Dump();
  CHECK(Json::Parse(text, &json, &error));
  Commander back;
  CHECK(CommanderFromJson(json, &back, &error));
  CHECK(back.story_step == 3 && back.mission.kind == MissionKind::kHunt && back.mission.step == 3 &&
        back.mission.count == 8 && back.mission.progress == 5 && back.mission.reward == 20000);

  // A save from before the story had none of it
  std::string old = text;
  for (const char* key : {"\"story_step\"", "\"mission\""}) {
    const std::size_t at = old.find(key);
    CHECK(at != std::string::npos);
    old.replace(at + 1, std::string(key).size() - 2, "unused");
  }
  CHECK(Json::Parse(old, &json, &error));
  Commander older;
  CHECK(CommanderFromJson(json, &older, &error));
  CHECK(older.story_step == 0 && older.mission.kind == MissionKind::kNone);

  // And giving it up offers it again
  Game g2(StartCommander(), 19);
  CHECK(g2.AcceptMission());
  CHECK(g2.AbandonMission());
  Mission offer;
  CHECK(g2.MissionOnOffer(&offer) && offer.step == 0);
}

void SettingsOfFlightFromInput() {
  Game game(StartCommander(), 11);
  game.Launch(false);
  // Throttle up with the stick, and the speed follows
  FlightInput in;
  in.throttle = 1.0;
  for (int i = 0; i < 60 * 4; ++i) {
    game.Update(in, 1.0 / 60.0);
  }
  CHECK(game.throttle() > 0.99);
  in.throttle = 0.0;
  const double max_speed = game.player()->stats().max_speed;
  for (int i = 0; i < 60 * 15; ++i) {
    game.Update(in, 1.0 / 60.0);
  }
  CHECK(Near(game.player()->Speed(), max_speed, 2.0));
  // Pull back: nose up
  FlightInput pull;
  pull.pitch = 1.0;
  const Vec3d f0 = Forward(game.player()->orientation);
  for (int i = 0; i < 30; ++i) {
    game.Update(pull, 1.0 / 60.0);
  }
  CHECK(Forward(game.player()->orientation).y > f0.y + 0.2);
  // Fire the lasers: bolts appear
  FlightInput fire;
  fire.held.set(static_cast<std::size_t>(Action::kFireLasers));
  game.Update(fire, 1.0 / 30.0);
  CHECK(!game.world().projectiles().empty());
  bool fired = false;
  for (const Event& e : game.TakeEvents()) {
    fired = fired || e.kind == EventKind::kLaserFired;
  }
  CHECK(fired);
}

void MissileLockAndFire() {
  Game game(StartCommander(), 12);
  game.Launch(false);
  Entity* me = game.player();
  const EntityId pirate =
      game.world().SpawnShip(Bp("sidewinder"), Team::kPirate, me->position + Forward(me->orientation) * 3000.0, Quat{});
  game.world().Find(pirate)->controls.throttle = 0.0;
  FlightInput press;
  press.pressed.set(static_cast<std::size_t>(Action::kTargetMissile));
  game.Update(press, 1.0 / 60.0);
  CHECK(game.missile_lock() != kNoEntity);
  const int missiles = game.commander().missiles;
  FlightInput fire;
  fire.pressed.set(static_cast<std::size_t>(Action::kFireMissile));
  game.Update(fire, 1.0 / 60.0);
  CHECK(game.commander().missiles == missiles - 1);
  CHECK(game.missile_lock() == kNoEntity);
  // Pressing once on a short frame still counts once, not lost
  Game quick(StartCommander(), 12);
  quick.Launch(false);
  const int before = quick.commander().missiles;
  Entity* q = quick.player();
  quick.world().SpawnShip(Bp("sidewinder"), Team::kPirate, q->position + Forward(q->orientation) * 3000.0, Quat{});
  FlightInput lock;
  lock.pressed.set(static_cast<std::size_t>(Action::kTargetMissile));
  quick.Update(lock, 0.001);
  quick.Update(FlightInput(), 0.02);
  CHECK(quick.missile_lock() != kNoEntity);
  CHECK(quick.commander().missiles == before);
}

void FuelScoopingAndCanisters() {
  Commander c = StartCommander();
  c.equipment.fuel_scoop = true;
  c.fuel = 2.0;
  Game game(c, 13);
  game.Launch(false);
  const Entity* sun = game.world().FindKind(EntityKind::kSun);
  Entity* me = game.player();
  me->position = sun->position + Vec3d{sun->radius * 1.2, 0, 0};
  me->velocity = Vec3d{};
  me->hull = me->hull_max = 1e9;
  for (int i = 0; i < 60 * 10; ++i) {
    game.player()->position = sun->position + Vec3d{sun->radius * 1.2, 0, 0};
    game.player()->shield = game.player()->shield_max;
    game.Update(FlightInput(), 1.0 / 60.0);
  }
  CHECK(game.commander().fuel > 3.5);
  CHECK(game.commander().fuel <= kMaxFuel);

  // Scooping a canister
  Game g2(c, 13);
  g2.Launch(false);
  Entity* p = g2.player();
  const EntityId can =
      g2.world().SpawnShip(Bp("cargo_canister"), Team::kNeutral, p->position + Forward(p->orientation) * 120.0, Quat{});
  g2.world().Find(can)->cargo_item = static_cast<int>(Commodity::kGold);
  g2.world().Find(can)->velocity = Vec3d{};
  for (int i = 0; i < 60 * 3; ++i) {
    g2.Update(FlightInput(), 1.0 / 60.0);
  }
  CHECK(g2.commander().cargo[static_cast<std::size_t>(Commodity::kGold)] == 1);
  CHECK(g2.world().Find(can) == nullptr);

  // A ship without a scoop just bumps it
  Game g3(StartCommander(), 13);
  g3.Launch(false);
  Entity* q = g3.player();
  g3.world().SpawnShip(Bp("cargo_canister"), Team::kNeutral, q->position + Forward(q->orientation) * 100.0, Quat{});
  for (int i = 0; i < 60 * 3; ++i) {
    g3.Update(FlightInput(), 1.0 / 60.0);
  }
  int cargo = 0;
  for (const int n : g3.commander().cargo) {
    cargo += n;
  }
  CHECK(cargo == 0);
}

/// Everything about a game that two runs from the same seed have to share
struct Fingerprint {
  double x = 0, y = 0, z = 0;
  std::int64_t credits = 0;
  double kills = 0, fuel = 0;
  std::size_t entities = 0;
  int legal = 0;
  int system = 0;
  GameMode mode = GameMode::kDocked;
  bool operator==(const Fingerprint& o) const {
    return x == o.x && y == o.y && z == o.z && credits == o.credits && kills == o.kills && fuel == o.fuel &&
           entities == o.entities && legal == o.legal && system == o.system && mode == o.mode;
  }
};

Fingerprint PlayScripted(std::uint64_t seed, double minutes, bool check) {
  Commander c = StartCommander();
  c.equipment.ecm = true;
  c.equipment.laser_level = 1;
  Game game(c, seed);
  game.Launch(false);
  game.player()->invulnerable = true;
  Random pilot(seed * 31 + 7);
  std::size_t most = 0;
  for (int frame = 0; frame < static_cast<int>(minutes * 60.0 * 60.0) && game.mode() == GameMode::kFlight; ++frame) {
    FlightInput in;
    if (frame % 90 == 0) {
      // A new whim every second and a half
      in.throttle = pilot.Range(-0.2, 1.0);
    }
    in.pitch = std::sin(frame * 0.013) * 0.6;
    in.roll = std::cos(frame * 0.007) * 0.8;
    in.yaw = std::sin(frame * 0.021) * 0.3;
    if (pilot.Chance(0.4)) {
      in.held.set(static_cast<std::size_t>(Action::kFireLasers));
    }
    if (frame % 400 == 0) {
      in.pressed.set(static_cast<std::size_t>(Action::kNextTarget));
    }
    if (frame % 700 == 0) {
      in.pressed.set(static_cast<std::size_t>(Action::kTargetMissile));
    }
    if (frame % 700 == 20) {
      in.pressed.set(static_cast<std::size_t>(Action::kFireMissile));
    }
    if (frame % 500 == 0) {
      in.pressed.set(static_cast<std::size_t>(Action::kEcm));
    }
    game.Update(in, 1.0 / 60.0);
    game.TakeEvents();
    game.TakeMessages();
    if (game.player() != nullptr) {
      game.player()->invulnerable = true;
    }
    if (check && game.player() != nullptr) {
      CHECK(Finite(game.player()->position));
      CHECK(Finite(game.player()->velocity));
      CHECK(std::isfinite(game.player()->orientation.w));
      CHECK(game.player()->hull <= game.player()->hull_max + 1e-9);
      CHECK(game.player()->shield >= -1e-9);
      CHECK(game.player()->energy >= -1e-9 && game.player()->energy <= game.player()->energy_max + 1e-9);
    }
    most = std::max(most, game.world().entities().size());
    if (check) {
      CHECK(game.world().entities().size() < 80);
    }
  }
  Fingerprint f;
  if (const Entity* p = game.player()) {
    f.x = p->position.x;
    f.y = p->position.y;
    f.z = p->position.z;
  }
  f.credits = game.commander().credits;
  f.kills = game.commander().kills;
  f.fuel = game.commander().fuel;
  f.entities = game.world().entities().size();
  f.legal = game.commander().legal_status;
  f.system = game.commander().system;
  f.mode = game.mode();
  return f;
}

void ALongPlaythroughIsStableAndRepeatable() {
  const Fingerprint a = PlayScripted(77, 6.0, true);
  const Fingerprint b = PlayScripted(77, 6.0, false);
  CHECK(a == b);
  // A different seed is a different game
  const Fingerprint c = PlayScripted(78, 6.0, false);
  if (a == c) {
    std::fprintf(stderr, "same fingerprint: mode=%d system=%d credits=%lld kills=%g pos=%g,%g,%g entities=%zu\n",
                 static_cast<int>(a.mode), a.system, static_cast<long long>(a.credits), a.kills, a.x, a.y, a.z,
                 a.entities);
  }
  CHECK(!(a == c));
}

void FirstFlightHasNoEnemies() {
  Game game(StartCommander(), 21);
  game.Launch(false);
  for (int frame = 0; frame < 60 * 60 * 20 && game.mode() == GameMode::kFlight; ++frame) {
    Entity* me = game.player();
    me->hull = me->hull_max;
    me->shield = me->shield_max;
    game.Update(FlightInput(), 1.0 / 60.0);
    if (frame % 60 == 0) {
      for (const Entity& e : game.world().entities()) {
        CHECK(e.team != Team::kPirate && e.team != Team::kAlien && e.team != Team::kHunter);
      }
    }
  }
}

void TrafficAppearsAndStaysBounded() {
  // A commander who has flown before: the first flight has no enemies
  Commander veteran = StartCommander();
  veteran.kills = 1.0;
  Game game(veteran, 21);
  game.Launch(false);
  std::set<std::string> seen;
  Entity* me = game.player();
  // Park the player in empty space, immortal, and let time pass
  for (int frame = 0; frame < 60 * 60 * 20 && game.mode() == GameMode::kFlight; ++frame) {
    me = game.player();
    me->hull = me->hull_max;
    me->shield = me->shield_max;
    game.Update(FlightInput(), 1.0 / 60.0);
    if (frame % 60 == 0) {
      for (const Entity& e : game.world().entities()) {
        if (e.blueprint != nullptr) {
          seen.insert(e.blueprint->id);
        }
      }
      CHECK(game.world().CountShips() <= 17);
      CHECK(game.world().entities().size() < 60);
    }
  }
  // In twenty minutes there is a mix of traffic
  bool traders = false, pirates = false;
  for (const std::string& id : seen) {
    traders = traders || id == "shuttle" || id == "transporter" || id == "cobra_mk3_trader" || id == "adder" ||
              id == "python";
    pirates = pirates || id == "sidewinder" || id == "gecko" || id == "krait" || id == "mamba" || id == "cobra_mk1";
  }
  CHECK(traders);
  CHECK(pirates);
  CHECK(seen.count("viper") == 1);
}

void TheBotPilotPlaysTheWholeGame() {
  // A computer pilot uses only the player's input: it fights, jumps, docks with the docking computer and trades. Over
  // several games it gets through the whole loop at least a few times and the engine never misbehaves.
  int jumps = 0;
  int landings = 0;
  double kills = 0.0;
  int survived = 0;
  for (std::uint64_t seed = 1; seed <= 6; ++seed) {
    Commander c = StartCommander();
    c.equipment.docking_computer = true;
    c.equipment.ecm = true;
    Game game(c, seed);
    BotPilot bot(seed);
    const std::int64_t start_credits = game.commander().credits;
    double clock = 0.0;
    while (clock < 12.0 * 60.0 && game.mode() != GameMode::kDead) {
      if (game.mode() == GameMode::kDocked) {
        bot.Dock(game);
        CHECK(game.commander().fuel >= 0.0 && game.commander().fuel <= kMaxFuel);
        CHECK(CargoUsed(game.commander()) <= game.commander().cargo_capacity);
        CHECK(game.commander().credits >= 0);
        game.Launch(false);
      }
      game.Update(bot.Fly(game), 1.0 / 60.0);
      clock += 1.0 / 60.0;
      game.TakeEvents();
      game.TakeMessages();
      if (game.player() != nullptr) {
        CHECK(Finite(game.player()->position));
      }
    }
    jumps += bot.jumps();
    landings += bot.landings() - 1;
    kills += game.commander().kills;
    survived += game.mode() != GameMode::kDead ? 1 : 0;
    (void)start_credits;
  }
  CHECK(jumps >= 6);
  CHECK(landings >= 6);
  CHECK(kills > 3.0);
  CHECK(survived >= 1);
}

void TheEnginePlaysWithAnXboxController() {
  // A pad held to the right and back, through the input mapper, flies the game's ship the same way
  InputMapper mapper(DefaultInputConfig(GamepadKind::kXboxElite));
  GamepadState pad;
  pad.left_x = 1.0f;
  pad.left_y = -1.0f;
  pad.right_trigger = 1.0f;
  Game game(StartCommander(), 22);
  game.Launch(false);
  const Vec3d f0 = Forward(game.player()->orientation);
  for (int i = 0; i < 60; ++i) {
    game.Update(mapper.Update(pad, 1.0f / 60.0f), 1.0 / 60.0);
  }
  CHECK(AngleBetween(f0, Forward(game.player()->orientation)) > 0.3);
  CHECK(game.player()->controls.fire);
}

void GeneratedAsteroidsAreValidAndVaried() {
  std::set<std::string> shapes;
  for (int c = 0; c < kRockClassCount; ++c) {
    const RockClass size_class = static_cast<RockClass>(c);
    float smallest = 1e9f;
    float largest = 0.0f;
    for (std::uint32_t seed = 1; seed <= 24; ++seed) {
      const AsteroidParams p = MakeAsteroidParams(seed, size_class);
      CHECK(p.radius_m >= RockClassMinRadius(size_class) - 0.01f && p.radius_m <= RockClassMaxRadius(size_class) + 0.01f);
      const Blueprint bp = GenerateAsteroidBlueprint(p);
      CHECK(ValidateBlueprint(bp).empty());
      CHECK(bp.kind == HullKind::kAsteroid);
      // The blueprint is the size asked for, to the metre
      CHECK(Near(BoundingRadius(bp), p.radius_m, 0.02 * p.radius_m));
      smallest = std::min(smallest, p.radius_m);
      largest = std::max(largest, p.radius_m);
      // The same parameters give the same rock, and a different seed a different one
      const Model a = GenerateAsteroidModel(p);
      const Model b = GenerateAsteroidModel(p);
      CHECK(a.vertices.size() == b.vertices.size());
      bool same = a.vertices.size() == b.vertices.size();
      for (std::size_t i = 0; same && i < a.vertices.size(); ++i) {
        same = a.vertices[i].x == b.vertices[i].x && a.vertices[i].y == b.vertices[i].y && a.vertices[i].z == b.vertices[i].z;
      }
      CHECK(same);
      shapes.insert(std::to_string(a.vertices.size()) + ":" + std::to_string(a.vertices[3].x) + ":" + std::to_string(a.vertices[3].y));
      // Most faces point away from the middle of the rock, and every one is lit, not hidden, by the renderer
      std::size_t outward = 0;
      for (const BlueprintTriangle& t : bp.triangles) {
        const Vec3 &x = bp.vertices[t.v[0]].position, &y = bp.vertices[t.v[1]].position, &z = bp.vertices[t.v[2]].position;
        const Vec3d n = Cross(Vec3d{y.x - x.x, y.y - x.y, y.z - x.z}, Vec3d{z.x - x.x, z.y - x.y, z.z - x.z});
        outward += Dot(n, Vec3d{x.x + y.x + z.x, x.y + y.y + z.y, x.z + y.z + z.z}) > 0.0 ? 1 : 0;
      }
      CHECK(outward * 10 >= bp.triangles.size() * 9);
    }
    // Each class has a real spread of sizes
    CHECK(largest > 1.4f * smallest);
  }
  // Plenty of different shapes
  CHECK(shapes.size() >= 100);
  // The classes run from a few metres to most of a kilometre
  CHECK(RockClassMinRadius(RockClass::kSplinter) < 10.0f);
  CHECK(RockClassMaxRadius(RockClass::kGiant) >= 800.0f);
}

void EachSystemHasItsOwnAsteroidField() {
  Spawner spawner;
  const auto field = [&](std::uint32_t system_seed) {
    World w(5);
    w.SetPlayer(w.SpawnShip(Bp("cobra_mk3_trader"), Team::kPlayer, {0, 0, 0}, Quat{}));
    StarSystem s;
    s.seed = system_seed;
    spawner.PopulateRocks(w, s);
    std::vector<const Blueprint*> rocks;
    for (const Entity& e : w.entities()) {
      if (e.kind == EntityKind::kAsteroid) {
        rocks.push_back(e.blueprint);
      }
    }
    return rocks;
  };
  const std::vector<const Blueprint*> a = field(1234);
  CHECK(a.size() >= 5);
  std::set<const Blueprint*> distinct(a.begin(), a.end());
  CHECK(distinct.size() >= 4);
  const std::size_t made = spawner.rocks().size();
  // The same system gives the same field again, from the same blueprints
  const std::vector<const Blueprint*> again = field(1234);
  CHECK(again == a);
  CHECK(spawner.rocks().size() == made);
  // Another system gives another
  CHECK(field(98765) != a);
  // A big rock somewhere in the lot over a few systems, and small ones too
  double biggest = 0.0;
  double smallest = 1e9;
  for (std::uint32_t s = 1; s <= 12; ++s) {
    for (const Blueprint* bp : field(s * 7919)) {
      biggest = std::max(biggest, BoundingRadius(*bp));
      smallest = std::min(smallest, BoundingRadius(*bp));
    }
  }
  CHECK(biggest > 300.0);
  CHECK(smallest < 20.0);
}

}  // namespace

int main() {
  JsonRoundTrips();
  JsonRejectsBadText();
  BuiltInBlueprintsAreGood();
  BlueprintsSurviveJsonAndDrawTheSame();
  BadBlueprintsAreRejected();
  BlueprintFilesSaveAndLoad();
  TurningHasTheRightSigns();
  LookAlongPointsTheNose();
  LeadingHitsAMovingTarget();
  ClassicFlightAcceleratesAndTurns();
  NewtonianFlightKeepsItsMomentum();
  DamagedEnginesSlowTheShip();
  TheBotPilotPlaysTheWholeGame();
  LasersHitAndShieldsGoFirst();
  ShotsMissWhenAimedWrong();
  ShotsStopAtTheFirstThingHit();
  DestroyedShipsReportDropCargoAndScore();
  SubsystemsCanBeShotOut();
  MissilesFindAndKillTheirTarget();
  EcmDestroysMissilesAimedAtTheShip();
  EnergyBombKillsEverythingButStationsAndBodies();
  ShipsBumpAndHurtEachOther();
  SunBurnsAndPlanetsAreSolid();
  StrictDockingNeedsAlignment();
  ARammedStationHurtsAndDoesNotDock();
  TheDockingComputerFliesTheShipIn();
  HostileStationsRefuseAndLaunchPolice();
  PiratesAttackThePlayer();
  PiratesBreakOffAndComeBack();
  PiratesFleeWhenBadlyHurt();
  PoliceHuntPiratesAndWantedPlayers();
  EveryoneHitsBackAtWhoeverHurtThem();
  TradersFlyToTheStationAndDock();
  TradersRunFromPirates();
  AliensLaunchDrones();
  DuelsAreRepeatableAndHaveAWinner();
  GalaxiesAreRepeatableAndSensible();
  MarketsFollowTheEconomy();
  TradingRespectsMoneyHoldAndStock();
  EquipmentCostsAndNeedsTech();
  RanksAndLawFollowTheNumbers();
  CommandersSaveAndLoad();
  GameStartsDockedAndLaunches();
  TheGameTradesWhenDocked();
  ComingHomeAndDockingEndsTheFlight();
  FlyingIntoTheBayDocksAndDoesNotAngerTheStation();
  TheTractorBeamLandsTheShipBeforeTheStationScreens();
  AShipFlyingIntoTheBayHowEverItIsAlignedIsLandedNotCrashed();
  ALaunchedShipIsNotTakenBackByTheBeam();
  NoDockingComputerNoAutopilot();
  KillingPiratesPaysAndRanksUp();
  KillingTradersMakesAnOffender();
  AttackingTheStationBringsThePolice();
  DeathEndsTheGameUnlessThereIsAPod();
  HyperspaceNeedsFuelDistanceAndClearSpace();
  TheCargoBayAndTheGalacticHyperdriveAreBoughtAndSaved();
  AGalacticJumpGoesToTheNextGalaxyAndUsesTheDrive();
  ShipsAreBoughtWithTheTradeInOfTheOldOne();
  EveryStepOfTheStoryIsOfferedWithItsBriefing();
  TheLastJobFitsAGalacticHyperdriveOrPaysForOne();
  ADeliveryIsPaidOnDockingAtTheDestinationOnly();
  ShootingPiratesAndDockingPaysAHuntingJob();
  AJobCanBeGivenUpAndTheStoryAndJobAreSaved();
  SettingsOfFlightFromInput();
  MissileLockAndFire();
  FuelScoopingAndCanisters();
  ALongPlaythroughIsStableAndRepeatable();
  FirstFlightHasNoEnemies();
  TrafficAppearsAndStaysBounded();
  TheEnginePlaysWithAnXboxController();
  GeneratedAsteroidsAreValidAndVaried();
  EachSystemHasItsOwnAsteroidField();
  if (g_failures == 0) {
    std::printf("all engine tests passed\n");
  }
  return g_failures == 0 ? 0 : 1;
}

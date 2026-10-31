// site_gen: write what the project web site (MkDocs, see mkdocs.yml) is made from.
//
//   site_gen [--docs DIRECTORY] [--png DIRECTORY]
//
// The Markdown pages for the ships, stations, things for sale, trade goods and worlds are written into the --docs
// directory (default docs), with every number taken from the game itself. The pictures are written as PNG files into
// the --png directory (default /tmp/coriolis_site): a picture of every ship, thing for sale and trade good, a portrait
// of a selection of planets, the station screens and some moments of play. tools/make_site.sh runs this, turns the
// pictures into compact WebP files in docs/img and builds the site.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "coriolis/asteroids.h"
#include "coriolis/blueprint.h"
#include "coriolis/bot.h"
#include "coriolis/cockpit.h"
#include "coriolis/commander.h"
#include "coriolis/commodity_models.h"
#include "coriolis/equipment_models.h"
#include "coriolis/galaxy.h"
#include "coriolis/game.h"
#include "coriolis/market.h"
#include "coriolis/png.h"
#include "coriolis/renderer.h"
#include "coriolis/screens.h"
#include "coriolis/ships.h"
#include "coriolis/ui.h"

using namespace coriolis;

namespace {

// ----------------------------------------------------------------------------------------------------------------
// Small helpers

/// "1.5", "-0.25", "3": at most two decimals, without trailing zeros
std::string Num(double value) {
  char buffer[48];
  std::snprintf(buffer, sizeof buffer, "%.2f", value);
  std::string s = buffer;
  if (s.find('.') != std::string::npos) {
    while (s.back() == '0') {
      s.pop_back();
    }
    if (s.back() == '.') {
      s.pop_back();
    }
  }
  return s == "-0" ? "0" : s;
}

/// A whole number with commas: 1,250,000
std::string Grouped(double value) {
  std::string digits = std::to_string(static_cast<long long>(std::llround(value)));
  std::string out;
  int count = 0;
  for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
    if (count > 0 && count % 3 == 0 && *it != '-') {
      out += ',';
    }
    out += *it;
    ++count;
  }
  std::reverse(out.begin(), out.end());
  return out;
}

std::string Mass(double kg) { return kg >= 1000.0 ? Grouped(kg / 1000.0) + " t" : Grouped(kg) + " kg"; }

bool Save(const Image& image, const std::string& path) {
  if (!WritePng(image, path)) {
    std::fprintf(stderr, "site_gen: cannot write %s\n", path.c_str());
    return false;
  }
  return true;
}

bool WriteFile(const std::string& path, const std::string& text) {
  std::ofstream file(path);
  file << text;
  if (!file) {
    std::fprintf(stderr, "site_gen: cannot write %s\n", path.c_str());
    return false;
  }
  return true;
}

std::string Index(const char* pattern, std::size_t i) {
  char buffer[64];
  std::snprintf(buffer, sizeof buffer, pattern, i);
  return buffer;
}

/// A table of statistics, two to a row
std::string StatTable(const std::vector<std::pair<std::string, std::string>>& stats) {
  std::string t = "| | | | |\n| --- | --- | --- | --- |\n";
  for (std::size_t i = 0; i < stats.size(); i += 2) {
    t += "| **" + stats[i].first + "** | " + stats[i].second + " |";
    if (i + 1 < stats.size()) {
      t += " **" + stats[i + 1].first + "** | " + stats[i + 1].second + " |\n";
    } else {
      t += " | |\n";
    }
  }
  return t;
}

// ----------------------------------------------------------------------------------------------------------------
// The words. The numbers all come from the game itself; these say what things are for.

/// In ShipId order
const char* const kShipName[kShipCount] = {
    "Sidewinder",  "Viper",       "Mamba",           "Python",       "Cobra Mk III (bounty hunter)",
    "Thargoid",    "Cobra Mk III (trader)", "Coriolis station", "Missile",   "Asteroid",
    "Cargo canister", "Thargon",  "Escape pod",      "Alloy plate",  "Boulder",
    "Splinter",    "Shuttle",     "Transporter",     "Boa",          "Anaconda",
    "Rock hermit", "Cobra Mk I",  "Adder",           "Krait",        "Gecko",
    "Worm",        "Asp Mk II",   "Fer-de-lance",    "Moray",        "Constrictor",
    "Cougar",      "Dodo station", "Wheel station",  "Cobra Mk III (pirate)", "Python (pirate)",
    "Elite logo"};

const char* const kShipText[kShipCount] = {
    /* Sidewinder */ "A small kite-shaped fighter, quick and cheap. Pirates fly it in packs and rely on numbers.",
    /* Viper */ "The police interceptor: two lasers, two engines and a missile. It patrols the system and hunts offenders and fugitives, and comes in pairs for a wanted pilot.",
    /* Mamba */ "Long, slim and fast, with three engines. A medium pirate that carries a missile and fights hard.",
    /* Python */ "A heavy freighter with a big hold and strong shields. Slow to turn, but armed with three missiles and two lasers.",
    /* Cobra Mk III (bounty hunter) */ "Modelled from the proportions of a three view silhouette. Fitted out as a bounty hunter: it goes after pirates and wanted pilots, and has a fuel scoop.",
    /* Thargoid */ "The octagonal alien mothership. It launches swarms of Thargon drones and carries six missiles. Rare, and very dangerous.",
    /* Cobra Mk III (trader) */ "The commander's own ship: the classic all-round trader with a fuel scoop, good manoeuvrability and a hold of 20 tonnes.",
    /* Coriolis */ "",
    /* Missile */ "A homing missile. It leads its target and detonates on contact. ECM can jam it.",
    /* Asteroid */ "A roughened icosphere of rock, a hazard and a source of minerals.",
    /* Cargo canister */ "Cargo drifting in space. Scoop it with a fuel scoop, or shoot a ship and wait for what it drops.",
    /* Thargon */ "A small drone launched by a Thargoid mothership. Tiny and fast, it swarms its target.",
    /* Escape pod */ "The pilot's way out. Hold Backspace to eject; the pod drifts away from the wreck.",
    /* Alloy plate */ "A hexagonal slab of hull plating, left behind when a ship is shot to pieces.",
    /* Boulder */ "Rock that is not split into plates, so it looks rougher than an asteroid. A medium sized lump.",
    /* Splinter */ "A long thin sliver of rock, a fragment of a broken asteroid.",
    /* Shuttle */ "A boxy civilian craft with short wings. It has no weapons and ferries people about the system.",
    /* Transporter */ "A long hauler with containers on its back. Slow and tough, with a single laser for defence.",
    /* Boa */ "A heavy freighter, wide, with three engines. In pirate hands it is a formidable raider.",
    /* Anaconda */ "Very long and narrow, with a dorsal turret. The biggest trader, with shields to match.",
    /* Rock hermit */ "",
    /* Cobra Mk I */ "The short, plain forerunner of the Mk III. Slower and lighter, a favourite of beginner pirates.",
    /* Adder */ "A boxy twin-engine trader with a modest hold. Armed enough to fend off a Sidewinder.",
    /* Krait */ "A slim dart with swept red wings. Light, nimble and bold.",
    /* Gecko */ "A small kite-shaped fighter with a canopy. Quick, and cheap enough to be everywhere.",
    /* Worm */ "A stubby insect-like body with side pods, flown by aliens that hunt in swarms.",
    /* Asp Mk II */ "A sleek explorer with swept wings. Fast, with good shields, so bounty hunters like it.",
    /* Fer-de-lance */ "A long dagger with gold wings. A very good fighter, flown by the more skilled pirates.",
    /* Moray */ "A wide, flat, saucer-like boat. A small trader with a decent hold.",
    /* Constrictor */ "Heavy, angular, dark with red trim: a military ship of the elite hunters. It hits hard and takes a lot of killing.",
    /* Cougar */ "A flat stealth wedge. Another elite hunter: hard to hit, and worth a great deal to the pilot who brings one down.",
    /* Dodo */ "",
    /* Wheel station */ "",
    /* Cobra Mk III (pirate) */ "The same Cobra Mk III model in dark pirate paint, with a missile and two lasers.",
    /* Python (pirate) */ "The same Python model in dull red pirate paint. A heavy raider that is hard to put down.",
    /* Elite logo */ "The winged emblem: two broad swept wings with S-shaped feathers and hooks, a small ship between them and the bevelled ELITE banner, built in layers. It faces the viewer and sways. Not a ship, but an inert object.",
};

struct StationText {
  const char* id;
  const char* shape;
  const char* text;
};

const StationText kStationText[] = {
    {"coriolis", "A cuboctahedron",
     "The standard trading station: a cuboctahedron with a docking slot in one face, rolling slowly about its own axis. "
     "Every bay is entered nose first, level and slowly. A tractor beam takes the ship at the mouth, draws it in and "
     "sets it down on the floor."},
    {"dodo", "A dodecahedron",
     "A dodecahedron with the docking slot in one of its pentagonal faces. Smaller than the Coriolis, it also rolls "
     "about its own axis."},
    {"wheel_station", "A rotating wheel",
     "A rotating wheel and six spokes around a hub, with a lit docking bay at the end of a long tube at the front. "
     "Ten times the size of the Coriolis: it is drawn at the same size as the others here, but is far bigger in the "
     "game."},
    {"rock_hermit", "A hollowed asteroid",
     "A rock with a lit docking bay in its front: an outlaw outpost. The model and its numbers are in the game's "
     "blueprint list, but no system places one yet."},
};

struct ItemText {
  Item item;
  const char* text;
};

const ItemText kItemText[] = {
    {Item::kMissile, "A homing missile that leads its target. Lock on with T, fire with M. ECM jams it."},
    {Item::kEcm, "Electronic counter-measures: jams every missile locked on to the ship. Press C."},
    {Item::kFuelScoop, "Scoops cargo canisters, and with them the cargo of your kills, into the hold."},
    {Item::kDockingComputer, "Flies the ship into the station's bay for you. Press K and watch."},
    {Item::kEnergyBomb, "Destroys everything nearby that is not a station or a planet. Press B, and be sure."},
    {Item::kEscapePod, "Hold Backspace to eject when the ship is lost: the pilot survives."},
    {Item::kBeamLaser, "A better laser than the pulse laser the ship comes with: 26 damage a shot against 18."},
    {Item::kMilitaryLaser, "The best laser in the game: 34 damage a shot."},
    {Item::kFuel, "Fuel for hyperspace, sold by the light year."},
    {Item::kCargoBay, "Fifteen more tonnes of hold. It stays with you when you change ship."},
    {Item::kGalacticHyperdrive,
     "Press J in flight to jump to the next galaxy: the ship arrives at the system nearest the same place on the chart. "
     "It uses no fuel, and the drive is used up."},
};

const char* const kGoodText[kCommodityCount] = {
    /* Food */ "A crate of food rations.",
    /* Textiles */ "Rolls of cloth.",
    /* Radioactives */ "A hazard-striped barrel.",
    /* Slaves */ "A cage.",
    /* Liquor */ "A bottle of liquor.",
    /* Luxuries */ "A gift box.",
    /* Narcotics */ "A pill bottle.",
    /* Computers */ "A monitor.",
    /* Machinery */ "A gear.",
    /* Alloys */ "A bundle of alloy rods.",
    /* Firearms */ "A rifle.",
    /* Furs */ "A pelt.",
    /* Minerals */ "Raw mineral crystals.",
    /* Gold */ "Stacked gold bars, sold by the kilogram.",
    /* Platinum */ "Platinum bars, sold by the kilogram.",
    /* Gem-Stones */ "A cut gem, sold by the gram.",
    /* Alien Items */ "A glowing alien relic. Only the more advanced worlds (tech level 8 and up) have any.",
};

std::string Behaviour(const std::string& ai) {
  static const std::map<std::string, std::string> kNames = {
      {"pirate_light", "Light pirate"},     {"pirate_medium", "Pirate"},        {"pirate_heavy", "Heavy pirate"},
      {"police", "Police"},                 {"trader", "Trader"},               {"trader_armed", "Armed trader"},
      {"bounty_hunter", "Bounty hunter"},   {"alien_mothership", "Alien mothership"},
      {"alien_drone", "Alien drone"},       {"elite_hunter", "Elite hunter"},   {"drifter", "Drifts"},
      {"station", "Station"}};
  const auto it = kNames.find(ai);
  return it == kNames.end() ? "" : it->second;
}

// ----------------------------------------------------------------------------------------------------------------
// Pictures of models

/// Copy a picture into another at a column
void Blit(Image& to, const Image& from, std::uint32_t x0) {
  for (std::uint32_t y = 0; y < from.height() && y < to.height(); ++y) {
    std::memcpy(to.data() + (static_cast<std::size_t>(y) * to.width() + x0) * 3,
                from.data() + static_cast<std::size_t>(y) * from.width() * 3, static_cast<std::size_t>(from.width()) * 3);
  }
}

/// A model from two sides, side by side
bool SavePair(const Model& model, const std::string& path) {
  const Renderer renderer;
  Image pair(1280, 360);
  const float angles[2] = {2.36f, 5.5f};
  for (std::uint32_t i = 0; i < 2; ++i) {
    Image tile(640, 360);
    DrawStars(tile, i * 7);
    renderer.DrawShip(tile, model, angles[i]);
    if (i == 0) {
      DrawCaption(tile, model.name);
    }
    Blit(pair, tile, i * 640);
  }
  return Save(pair, path);
}

// ----------------------------------------------------------------------------------------------------------------
// Worlds

struct WorldInfo {
  const StarSystem* system;
  PlanetPortraitInfo planet;
  std::string station;
};

Commander Pilot(int system) {
  Commander c;
  c.name = "Jameson";
  c.system = system;
  c.credits = 20000;
  c.kills = 70.0;
  c.equipment.docking_computer = true;
  c.equipment.ecm = true;
  c.equipment.fuel_scoop = true;
  return c;
}

/// A selection of the worlds of the first galaxy: the first of each kind of planet, with rings and without
std::vector<WorldInfo> PickWorlds(const Galaxy& galaxy) {
  std::vector<WorldInfo> worlds;
  std::set<std::string> seen;
  for (const StarSystem& s : galaxy.systems()) {
    const PlanetPortraitInfo info = DescribePlanet(s.seed);
    const std::string key = std::string(info.type) + (info.rings ? "+rings" : "");
    if (!seen.insert(key).second) {
      continue;
    }
    WorldInfo w{&s, info, ""};
    Game game(Pilot(s.index), 1);
    game.Launch(false);
    if (const Entity* station = game.world().FindKind(EntityKind::kStation)) {
      w.station = station->blueprint->name;
    }
    worlds.push_back(w);
  }
  return worlds;
}

/// The first system of the galaxy with each kind of station
std::map<std::string, int> StationSystems(const Galaxy& galaxy) {
  std::map<std::string, int> found;
  for (const StarSystem& s : galaxy.systems()) {
    Game game(Pilot(s.index), 1);
    game.Launch(false);
    if (const Entity* station = game.world().FindKind(EntityKind::kStation)) {
      found.insert({station->blueprint->id, s.index});
    }
    if (found.size() >= 3) {
      break;
    }
  }
  return found;
}

// ----------------------------------------------------------------------------------------------------------------
// Moments of play, staged in the engine and drawn by the cockpit

struct Cue {
  const char* id;
  Team team;
  /// Where, in the player's frame: x right, y up, z ahead, in metres
  Vec3d at;
};

Vec3d InFrame(const Entity& me, const Vec3d& at) { return me.position + Rotate(me.orientation, at); }

/// The computer pilot fights ships put in front of it, and the best moment is kept: the most ships close ahead with bolts
/// in the air
bool Fight(const std::string& path, std::uint64_t seed, const std::vector<Cue>& cues, double seconds) {
  Game game(Pilot(40), seed);
  game.Launch(false);
  Entity* me = game.player();
  if (me == nullptr) {
    return false;
  }
  me->position = me->position + Forward(me->orientation) * 15000.0;
  for (const Cue& cue : cues) {
    const Vec3d where = InFrame(*me, cue.at);
    game.world().SpawnShip(*FindBuiltInBlueprint(cue.id), cue.team, where, LookAlong(me->position - where, {0.0, 1.0, 0.0}));
  }
  BotPilot bot(seed);
  Cockpit cockpit;
  cockpit.set_supersample(2);
  Image scratch(1280, 720);
  Image best(1280, 720);
  double best_score = -1e18;
  for (std::uint32_t frame = 1; frame <= static_cast<std::uint32_t>(seconds * 60.0) && game.mode() == GameMode::kFlight;
       ++frame) {
    game.Update(bot.Fly(game), 1.0 / 60.0);
    cockpit.Notice(game, game.TakeEvents(), game.TakeMessages());
    cockpit.Advance(1.0 / 60.0);
    const Entity* player = game.player();
    if (player == nullptr || frame % 12U != 0U) {
      continue;
    }
    const Vec3d nose = Forward(player->orientation);
    int ships = 0;
    double nearness = 0.0;
    for (const Entity& e : game.world().entities()) {
      if (!e.alive || e.kind != EntityKind::kShip || e.id == player->id) {
        continue;
      }
      const Vec3d rel = e.position - player->position;
      const double d = Length(rel);
      if (d > 20.0 && d < 700.0 && Dot(rel, nose) / d > 0.85) {
        ++ships;
        nearness += 1.0 - d / 700.0;
      }
    }
    const double score = ships * 10.0 + nearness * 4.0 + static_cast<double>(game.world().projectiles().size());
    if (score > best_score) {
      cockpit.Draw(scratch, game, frame);
      best = scratch;
      best_score = score;
    }
  }
  return Save(best, path);
}

/// A station seen from outside with a couple of ships about it
bool StationView(const std::string& path, int system, std::uint64_t seed) {
  Game game(Pilot(system), seed);
  game.Launch(false);
  const Entity* station = game.world().FindKind(EntityKind::kStation);
  Entity* me = game.player();
  if (station == nullptr || me == nullptr) {
    return false;
  }
  const double r = station->radius;
  const Vec3d dock = DockPoint(*station);
  const Vec3d axis = DockAxis(*station);
  const Vec3d up = Up(station->orientation);
  const Vec3d side = Normalized(Cross(axis, up));
  me->position = dock + axis * (r * 2.4) + side * (r * 1.0) + up * (r * 0.4);
  me->orientation = LookAlong(station->position - me->position, up);
  me->velocity = {};
  const Cue cues[] = {{"cobra_mk3_trader", Team::kTrader, {-70.0, 25.0, 260.0}},
                      {"adder", Team::kTrader, {90.0, -10.0, 420.0}},
                      {"python", Team::kTrader, {40.0, 60.0, 900.0}}};
  for (const Cue& cue : cues) {
    const Vec3d where = InFrame(*me, cue.at);
    game.world().SpawnShip(*FindBuiltInBlueprint(cue.id), cue.team, where, LookAlong(dock - where, up));
  }
  Cockpit cockpit;
  cockpit.set_supersample(2);
  Image image(1280, 720);
  cockpit.Draw(image, game, 0);
  return Save(image, path);
}

/// The docking computer flies the ship in and the tractor beam has it
bool Landing(const std::string& path, std::uint64_t seed) {
  Game game(Pilot(40), seed);
  game.Launch(false);
  const Entity* station = game.world().FindKind(EntityKind::kStation);
  Entity* me = game.player();
  if (station == nullptr || me == nullptr) {
    return false;
  }
  const Vec3d axis = DockAxis(*station);
  me->position = DockPoint(*station) + axis * 1500.0;
  me->orientation = LookAlong(-axis, Up(station->orientation));
  me->velocity = -axis * 120.0;
  Cockpit cockpit;
  cockpit.set_supersample(2);
  FlightInput press;
  press.pressed.set(static_cast<std::size_t>(Action::kDockingComputer));
  press.held.set(static_cast<std::size_t>(Action::kDockingComputer));
  game.Update(press, 1.0 / 60.0);
  std::uint32_t frame = 0;
  while (frame < 60U * 90U && game.mode() == GameMode::kFlight && !(game.landing() && game.landing_progress() > 0.02)) {
    game.Update(FlightInput{}, 1.0 / 60.0);
    cockpit.Notice(game, game.TakeEvents(), game.TakeMessages());
    cockpit.Advance(1.0 / 60.0);
    ++frame;
  }
  Image image(1280, 720);
  cockpit.Draw(image, game, frame);
  return Save(image, path);
}

/// Riding out of the bay
bool Launching(const std::string& path, std::uint64_t seed) {
  Game game(Pilot(40), seed);
  if (!game.Launch()) {
    return false;
  }
  Cockpit cockpit;
  cockpit.set_supersample(2);
  std::uint32_t frame = 0;
  while (frame < 60U * 20U && game.launching() && game.launch_progress() < 0.2) {
    game.Update(FlightInput{}, 1.0 / 60.0);
    cockpit.Notice(game, game.TakeEvents(), game.TakeMessages());
    cockpit.Advance(1.0 / 60.0);
    ++frame;
  }
  Image image(1280, 720);
  cockpit.Draw(image, game, frame);
  return Save(image, path);
}

/// Close to a rock of the system's asteroid field
bool Rocks(const std::string& path, std::uint64_t seed) {
  Game game(Pilot(40), seed);
  game.Launch(false);
  Entity* me = game.player();
  if (me == nullptr) {
    return false;
  }
  const Vec3d nose = Forward(me->orientation);
  const Vec3d centre = me->position + nose * 25000.0;
  const Entity* rock = nullptr;
  for (const Entity& e : game.world().entities()) {
    if (e.alive && e.kind == EntityKind::kAsteroid && e.radius > 40.0 &&
        (rock == nullptr || Distance(e.position, centre) < Distance(rock->position, centre))) {
      rock = &e;
    }
  }
  if (rock == nullptr) {
    return false;
  }
  me->position = rock->position - nose * (rock->radius * 2.2 + 80.0);
  Cockpit cockpit;
  cockpit.set_supersample(2);
  Image image(1280, 720);
  cockpit.Draw(image, game, 0);
  return Save(image, path);
}

// ----------------------------------------------------------------------------------------------------------------
// Pages

std::string Link(const std::string& name) {
  // The slug MkDocs gives a heading: lower case letters and digits, the rest turned into hyphens
  std::string slug;
  for (const char c : name) {
    if (std::isalnum(static_cast<unsigned char>(c)) != 0) {
      slug += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    } else if ((c == ' ' || c == '-') && !slug.empty() && slug.back() != '-') {
      slug += '-';
    }
  }
  while (!slug.empty() && slug.back() == '-') {
    slug.pop_back();
  }
  return "[" + name + "](#" + slug + ")";
}

std::string ShipEntry(std::size_t i, const Blueprint& b, const Model& model) {
  const BlueprintStats& s = b.stats;
  float lo[3] = {1e9f, 1e9f, 1e9f};
  float hi[3] = {-1e9f, -1e9f, -1e9f};
  for (const BlueprintVertex& v : b.vertices) {
    lo[0] = std::min(lo[0], v.position.x);
    hi[0] = std::max(hi[0], v.position.x);
    lo[1] = std::min(lo[1], v.position.y);
    hi[1] = std::max(hi[1], v.position.y);
    lo[2] = std::min(lo[2], v.position.z);
    hi[2] = std::max(hi[2], v.position.z);
  }
  int lasers = 0;
  int engines = 0;
  bool scoop = false;
  for (const Hardpoint& h : b.hardpoints) {
    lasers += h.kind == HardpointKind::kLaser ? 1 : 0;
    engines += h.kind == HardpointKind::kEngine ? 1 : 0;
    scoop = scoop || h.kind == HardpointKind::kScoop;
  }
  std::string text = kShipText[i];
  std::string shape;
  for (const StationText& st : kStationText) {
    if (b.id == st.id) {
      text = st.text;
      shape = st.shape;
    }
  }
  std::vector<std::pair<std::string, std::string>> stats;
  if (!shape.empty()) {
    stats.push_back({"Shape", shape});
  }
  stats.push_back({"Size", Num((hi[2] - lo[2]) * b.scale) + " × " + Num((hi[0] - lo[0]) * b.scale) + " × " +
                               Num((hi[1] - lo[1]) * b.scale) + " m"});
  stats.push_back({"Mass", Mass(s.mass_kg)});
  if (b.kind == HullKind::kStation) {
    if (s.max_roll_rate > 0.0f) {
      stats.push_back({"Rolls once in", Num(2.0 * kPi / s.max_roll_rate) + " s"});
    }
  } else if (s.max_speed > 0.0f) {
    stats.push_back({"Top speed", Num(s.max_speed) + " m/s"});
    stats.push_back({"Acceleration", Num(s.acceleration) + " m/s²"});
    stats.push_back({"Pitch, roll, yaw", Num(s.max_pitch_rate) + ", " + Num(s.max_roll_rate) + ", " + Num(s.max_yaw_rate) + " rad/s"});
  }
  stats.push_back({"Hull", Grouped(s.hull)});
  if (s.shield > 0.0f) {
    stats.push_back({"Shield", Grouped(s.shield)});
  }
  if (s.energy > 0.0f) {
    stats.push_back({"Energy", Grouped(s.energy) + " (+" + Num(s.energy_recharge) + "/s)"});
  }
  if (lasers > 0) {
    stats.push_back({"Lasers", std::to_string(lasers)});
  }
  if (s.missiles > 0) {
    stats.push_back({"Missiles", std::to_string(s.missiles)});
  }
  if (engines > 0 && b.kind == HullKind::kShip) {
    stats.push_back({"Engines", std::to_string(engines)});
  }
  if (scoop) {
    stats.push_back({"Fuel scoop", "yes"});
  }
  if (s.bounty > 0.0f) {
    stats.push_back({"Bounty", Num(s.bounty) + " Cr"});
  }
  if (s.kill_points > 0.0f) {
    stats.push_back({"Combat points", Num(s.kill_points)});
  }
  if (s.max_cargo_drop > 0) {
    stats.push_back({"Drops up to", std::to_string(s.max_cargo_drop) + " canisters"});
  }
  if (!Behaviour(b.ai_profile).empty() && b.kind != HullKind::kStation) {
    stats.push_back({"Flown as", Behaviour(b.ai_profile)});
  }
  std::string parts;
  for (const Subsystem& sub : b.subsystems) {
    parts += (parts.empty() ? "" : ", ") + sub.name;
  }
  if (!parts.empty()) {
    stats.push_back({"Can be shot out", parts});
  }
  std::string md = "### " + std::string(kShipName[i]) + "\n\n![" + kShipName[i] + "](" + Index("img/ships/ship_%02zu.webp", i) +
                   "){ loading=lazy }\n\n" + text + "\n\n" + StatTable(stats) + "\n";
  (void)model;
  return md;
}

bool WriteShipsPage(const std::string& docs, const std::vector<Blueprint>& blueprints, const std::vector<Model>& models) {
  std::string md =
      "# Ships\n\n"
      "Every ship of the game, drawn by the game's own software renderer from two sides. The first 13 are the ships of the "
      "original cassette ship list, in its order, and the rest are the further ships of the later versions; the "
      "Cobra Mk III and Python each have a pirate paint job on the same model. The models are original work, not "
      "taken from any version of Elite. The numbers are the game's own: speeds in metres a second, turn rates in "
      "radians a second, and hull, shield and energy in points of damage. The stations are on [their own page](stations.md).\n\n"
      "![The 36 ships](ships.png)\n\n"
      "## At a glance\n\n"
      "| Ship | Length | Top speed | Hull | Shield | Lasers | Missiles | Bounty | Flown as |\n"
      "| --- | --- | --- | --- | --- | --- | --- | --- | --- |\n";
  std::string ships_md = "## Ships\n\n";
  std::string objects_md =
      "## Objects in space\n\nThings that are not ships: weapons, rocks, cargo and the emblem. They have blueprints and "
      "models like everything else.\n\n";
  for (std::size_t i = 0; i < blueprints.size(); ++i) {
    const Blueprint& b = blueprints[i];
    if (b.kind == HullKind::kStation) {
      continue;
    }
    float lo = 1e9f;
    float hi = -1e9f;
    int lasers = 0;
    for (const BlueprintVertex& v : b.vertices) {
      lo = std::min(lo, v.position.z);
      hi = std::max(hi, v.position.z);
    }
    for (const Hardpoint& h : b.hardpoints) {
      lasers += h.kind == HardpointKind::kLaser ? 1 : 0;
    }
    const bool ship = b.kind == HullKind::kShip;
    if (ship) {
      md += "| " + Link(kShipName[i]) + " | " + Num((hi - lo) * b.scale) + " m | " + Num(b.stats.max_speed) + " m/s | " +
            Grouped(b.stats.hull) + " | " + Grouped(b.stats.shield) + " | " + std::to_string(lasers) + " | " +
            std::to_string(b.stats.missiles) + " | " + (b.stats.bounty > 0.0f ? Num(b.stats.bounty) + " Cr" : "") + " | " +
            Behaviour(b.ai_profile) + " |\n";
    }
    (ship ? ships_md : objects_md) += ShipEntry(i, b, models[i]);
  }
  return WriteFile(docs + "/ships.md", md + "\n" + ships_md + objects_md);
}

bool WriteStationsPage(const std::string& docs, const std::vector<Blueprint>& blueprints, const std::vector<Model>& models,
                       const std::map<std::string, int>& station_systems, const Galaxy& galaxy) {
  std::string md =
      "# Stations\n\n"
      "Each system has one station, picked from the system's seed so that it is always the same one there. A station "
      "rolls slowly about its own axis and has a docking bay in its front. To dock, fly into the bay nose first, level "
      "and slowly; or switch on a docking computer (K) and let it fly you in. A tractor beam takes the ship at the "
      "mouth, draws it in and sets it down, and the station screens come up. Shoot the station and the police come.\n\n";
  for (std::size_t i = 0; i < blueprints.size(); ++i) {
    const Blueprint& b = blueprints[i];
    if (b.kind != HullKind::kStation) {
      continue;
    }
    md += ShipEntry(i, b, models[i]);
    const auto it = station_systems.find(b.id);
    if (it != station_systems.end()) {
      md += "Find one at " + galaxy.system(it->second).name + ", in the first galaxy.\n\n";
      md += "![" + std::string(kShipName[i]) + " in flight](" + "img/flight/station_" + b.id + ".webp){ loading=lazy }\n\n";
    }
  }
  return WriteFile(docs + "/stations.md", md);
}

bool WriteUpgradesPage(const std::string& docs) {
  std::string md =
      "# Upgrades\n\n"
      "What the equipment screen sells, each as a 3D model drawn from two sides, the same one the screen shows under "
      "the list. A system sells an item only if its tech level is high enough. Prices are in credits.\n\n"
      "| Item | Price | Needs tech level |\n| --- | --- | --- |\n";
  std::string entries;
  for (std::size_t i = 0; i < kItemCount; ++i) {
    const Item item = static_cast<Item>(i);
    const std::string name = ItemName(item);
    md += "| " + Link(name) + " | " + Num(ItemPrice(item) / 10.0) + " Cr" + (item == Item::kFuel ? " a light year" : "") + " | " +
          std::to_string(ItemTechLevel(item)) + " |\n";
    std::string text;
    for (const ItemText& t : kItemText) {
      if (t.item == item) {
        text = t.text;
      }
    }
    entries += "### " + name + "\n\n![" + name + "](" + Index("img/items/item_%02zu.webp", i) + "){ loading=lazy }\n\n" + text +
               "\n\n";
  }
  md += "\n![The equipment screen](img/screens/equipment.webp){ loading=lazy }\n\n" + entries;
  return WriteFile(docs + "/upgrades.md", md);
}

bool WriteGoodsPage(const std::string& docs) {
  std::string md =
      "# Trade goods\n\n"
      "There are 17 goods to trade. The market screen draws the highlighted one in 3D to the right of the list. A price "
      "is the base price plus a step for each step of the system's economy, from rich industrial to poor agricultural, "
      "and drifts from visit to visit. Prices are in credits for a unit (tonne, kilogram or gram).\n\n"
      "| Good | Unit | Base price | Cheaper on | Illegal |\n| --- | --- | --- | --- | --- |\n";
  std::string entries;
  for (std::size_t i = 0; i < kCommodityCount; ++i) {
    const CommodityInfo& info = CommodityData(static_cast<Commodity>(i));
    const std::string name = info.name;
    md += "| " + Link(name) + " | " + UnitName(info.unit) + " | " + Num(info.base_price / 10.0) + " Cr | " +
          (info.economy_factor < 0 ? "agricultural worlds" : "industrial worlds") + " | " + (info.illegal ? "yes" : "") + " |\n";
    entries += "### " + name + "\n\n![" + name + "](" + Index("img/goods/good_%02zu.webp", i) + "){ loading=lazy }\n\n" +
               kGoodText[i] + (info.illegal ? " Illegal: the police take a dim view." : "") + "\n\n";
  }
  md += "\n![The market screen](img/screens/market.webp){ loading=lazy }\n\n" + entries;
  return WriteFile(docs + "/trade-goods.md", md);
}

bool WritePlanetsPage(const std::string& docs, const std::vector<WorldInfo>& worlds) {
  std::string md =
      "# Worlds\n\n"
      "The galaxies are made from a number: eight galaxies of 256 systems, each with a government, an economy, a tech "
      "level, a population, a species and a planet. The planet in the cockpit is drawn from the system's seed, so it "
      "is always the same one: temperate, desert, frozen, ocean, volcanic, violet or toxic worlds, cratered moons, and "
      "gas giants, with continents, ice caps, an atmosphere that glows at the rim and, now and then, rings. These "
      "are portraits of a selection from the first galaxy, one of each kind, as the game draws them.\n\n"
      "![A ringed gas giant](img/screens/hero.webp)\n\n";
  for (const WorldInfo& w : worlds) {
    const StarSystem& s = *w.system;
    std::string kind = w.planet.type;
    if (w.planet.rings) {
      kind += ", ringed";
    }
    std::vector<std::pair<std::string, std::string>> stats = {
        {"Planet", kind},
        {"Radius", Grouped(s.radius_km) + " km"},
        {"Government", GovernmentName(s.government)},
        {"Economy", EconomyName(s.economy)},
        {"Tech level", std::to_string(s.tech_level)},
        {"Population", Num(s.population / 10.0) + " billion"},
        {"Productivity", Grouped(s.productivity) + " M Cr"},
        {"Species", s.species},
        {"Station", w.station},
        {"On the chart", std::to_string(s.x) + ", " + std::to_string(s.y)}};
    char file[64];
    std::snprintf(file, sizeof file, "img/planets/planet_%03d.webp", s.index);
    md += "### " + s.name + "\n\n![" + s.name + "](" + file + "){ loading=lazy }\n\n" + StatTable(stats) + "\n";
  }
  md +=
      "## Asteroid fields\n\n"
      "Each system has its own asteroid field, the same every visit: rocks from splinters of 8 metres to giants of nearly "
      "a kilometre, split 20 sided shapes pushed in and out by lumps, craters and bumps, in rocky, metallic, icy or dark "
      "colours. A row for each size class:\n\n"
      "![Asteroids](img/screens/asteroids.webp){ loading=lazy }\n";
  return WriteFile(docs + "/planets.md", md);
}

}  // namespace

int main(int argc, char** argv) {
  std::string docs = "docs";
  std::string png = "/tmp/coriolis_site";
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--docs" && i + 1 < argc) {
      docs = argv[++i];
    } else if (arg == "--png" && i + 1 < argc) {
      png = argv[++i];
    } else {
      std::fprintf(stderr, "usage: site_gen [--docs DIRECTORY] [--png DIRECTORY]\n");
      return 2;
    }
  }
  const std::string mkdir = "mkdir -p '" + docs + "' '" + png + "/ships' '" + png + "/items' '" + png + "/goods' '" + png +
                            "/planets' '" + png + "/screens' '" + png + "/flight'";
  if (std::system(mkdir.c_str()) != 0) {
    return 1;
  }

  const std::vector<Model>& ships = AllShips();
  const std::vector<Model>& items = AllItemModels();
  const std::vector<Model>& goods = AllCommodityModels();
  const std::vector<Blueprint>& blueprints = BuiltInBlueprints();
  int failures = 0;

  // ---- The models
  for (std::size_t i = 0; i < ships.size(); ++i) {
    failures += SavePair(ships[i], png + Index("/ships/ship_%02zu.png", i)) ? 0 : 1;
  }
  for (std::size_t i = 0; i < items.size(); ++i) {
    failures += SavePair(items[i], png + Index("/items/item_%02zu.png", i)) ? 0 : 1;
  }
  for (std::size_t i = 0; i < goods.size(); ++i) {
    failures += SavePair(goods[i], png + Index("/goods/good_%02zu.png", i)) ? 0 : 1;
  }

  // ---- The planets and the rocks
  const Galaxy galaxy(0);
  const std::vector<WorldInfo> worlds = PickWorlds(galaxy);
  for (const WorldInfo& w : worlds) {
    Image image(800, 600);
    DrawPlanetPortrait(image, w.system->seed, 0.6 + 0.37 * w.system->index);
    char name[64];
    std::snprintf(name, sizeof name, "/planets/planet_%03d.png", w.system->index);
    failures += Save(image, png + name) ? 0 : 1;
  }
  {
    const auto ringed = std::find_if(worlds.begin(), worlds.end(), [](const WorldInfo& w) { return w.planet.rings; });
    Image image(1920, 1080);
    DrawPlanetPortrait(image, (ringed != worlds.end() ? *ringed : worlds.front()).system->seed, 2.4);
    failures += Save(image, png + "/screens/hero.png") ? 0 : 1;
  }
  {
    constexpr std::uint32_t kColumns = 6;
    const Renderer renderer;
    Image sheet(1200, 800);
    const std::uint32_t tile_w = sheet.width() / kColumns;
    const std::uint32_t tile_h = sheet.height() / kRockClassCount;
    for (std::uint32_t row = 0; row < kRockClassCount; ++row) {
      for (std::uint32_t column = 0; column < kColumns; ++column) {
        Image tile(tile_w, tile_h);
        DrawStars(tile, row * 31 + column);
        renderer.DrawShip(tile, GenerateAsteroidModel(MakeAsteroidParams(1 + column, static_cast<RockClass>(row))), 2.36f);
        for (std::uint32_t y = 0; y < tile_h; ++y) {
          std::memcpy(sheet.data() + ((static_cast<std::size_t>(row) * tile_h + y) * sheet.width() + column * tile_w) * 3,
                      tile.data() + static_cast<std::size_t>(y) * tile_w * 3, static_cast<std::size_t>(tile_w) * 3);
        }
      }
    }
    failures += Save(sheet, png + "/screens/asteroids.png") ? 0 : 1;
  }

  // ---- The screens of the station, for a commander who has done a little trading
  {
    int home = 0;
    for (const StarSystem& s : galaxy.systems()) {
      if (s.tech_level >= 10 && s.economy <= 2) {
        home = s.index;
        break;
      }
    }
    Commander commander = Pilot(home);
    commander.credits = 48750;
    commander.fuel = 6.2;
    commander.cargo[static_cast<std::size_t>(Commodity::kMachinery)] = 4;
    commander.cargo[static_cast<std::size_t>(Commodity::kGold)] = 6;
    commander.cargo[static_cast<std::size_t>(Commodity::kAlloys)] = 3;
    commander.equipment.laser_level = 1;
    Game game(commander, 3);
    const std::vector<int> near = game.galaxy().InRange(home, kMaxFuel);
    if (!near.empty()) {
      game.SelectSystem(near.front());
    }
    Image image(1280, 720);
    DrawStatusScreen(image, game);
    failures += Save(image, png + "/screens/status.png") ? 0 : 1;
    DrawMarketScreen(image, game, 13);
    failures += Save(image, png + "/screens/market.png") ? 0 : 1;
    DrawEquipmentScreen(image, game, 3);
    failures += Save(image, png + "/screens/equipment.png") ? 0 : 1;
    DrawShipyardScreen(image, game, 3);
    failures += Save(image, png + "/screens/shipyard.png") ? 0 : 1;
    DrawChartScreen(image, game);
    failures += Save(image, png + "/screens/chart.png") ? 0 : 1;
    DrawSystemScreen(image, game, game.selected_system());
    failures += Save(image, png + "/screens/system.png") ? 0 : 1;
  }
  {
    GameUi ui("", 1);
    for (int i = 0; i < 180; ++i) {
      ui.Update(FlightInput{}, 1.0 / 60.0);
    }
    Image image(1280, 720);
    ui.Draw(image);
    failures += Save(image, png + "/screens/title.png") ? 0 : 1;
  }

  // ---- Moments of play
  const std::map<std::string, int> station_systems = StationSystems(galaxy);
  for (const auto& [id, system] : station_systems) {
    failures += StationView(png + "/flight/station_" + id + ".png", system, 1) ? 0 : 1;
  }
  failures += Fight(png + "/flight/combat_pirates.png", 3,
                    {{"mamba", Team::kPirate, {-30, 10, 260}},
                     {"sidewinder", Team::kPirate, {40, -10, 200}},
                     {"sidewinder", Team::kPirate, {110, 30, 340}},
                     {"krait", Team::kPirate, {-110, -20, 300}}},
                    14.0)
                  ? 0
                  : 1;
  failures += Fight(png + "/flight/combat_heavy.png", 5,
                    {{"python_pirate", Team::kPirate, {0, 20, 380}},
                     {"cobra_mk3_pirate", Team::kPirate, {-90, 0, 250}},
                     {"viper", Team::kPolice, {110, 20, 200}}},
                    14.0)
                  ? 0
                  : 1;
  failures += Fight(png + "/flight/thargoid.png", 7,
                    {{"thargoid", Team::kAlien, {0, 20, 500}},
                     {"thargon", Team::kAlien, {-50, 0, 180}},
                     {"thargon", Team::kAlien, {60, 15, 220}},
                     {"thargon", Team::kAlien, {10, -30, 150}}},
                    14.0)
                  ? 0
                  : 1;
  failures += Fight(png + "/flight/elite.png", 9,
                    {{"constrictor", Team::kPirate, {-40, 10, 260}}, {"cougar", Team::kPirate, {70, 10, 340}}}, 14.0)
                  ? 0
                  : 1;
  failures += Landing(png + "/flight/landing.png", 2) ? 0 : 1;
  failures += Launching(png + "/flight/launch.png", 2) ? 0 : 1;
  failures += Rocks(png + "/flight/rocks.png", 2) ? 0 : 1;

  // ---- The pages
  failures += WriteShipsPage(docs, blueprints, ships) ? 0 : 1;
  failures += WriteStationsPage(docs, blueprints, ships, station_systems, galaxy) ? 0 : 1;
  failures += WriteUpgradesPage(docs) ? 0 : 1;
  failures += WriteGoodsPage(docs) ? 0 : 1;
  failures += WritePlanetsPage(docs, worlds) ? 0 : 1;

  std::printf("%s: %zu ships and objects, %zu items, %zu goods, %zu worlds, %zu stations\n", failures == 0 ? "ok" : "FAILED",
              ships.size(), items.size(), goods.size(), worlds.size(), station_systems.size());
  return failures == 0 ? 0 : 1;
}
